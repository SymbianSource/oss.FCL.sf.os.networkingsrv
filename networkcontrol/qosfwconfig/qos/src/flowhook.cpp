// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
// All rights reserved.
// This component and the accompanying materials are made available
// under the terms of "Eclipse Public License v1.0"
// which accompanies this distribution, and is available
// at the URL "http://www.eclipse.org/legal/epl-v10.html".
//
// Initial Contributors:
// Nokia Corporation - initial contribution.
//
// Contributors:
//
// Description:
//

#include <udp_hdr.h>
#include <tcp_hdr.h>
#include <icmp6_hdr.h>
#include <ip4_hdr.h>
#include <ip6_hdr.h>

#include "qos_prot.h"
#include "flowhook.h"
#include "negotiation.h"
#include "qoserr.h"
#include "interface.h"
#include "modules.h"
#include "policy_mgr.h"
#include "qos_channel.h"
#include <es_prot_internal.h>

CFlowHook::CFlowHook(CProtocolQoS& aProtocol, CFlowContext& aContext) : iProtocol(aProtocol), iContext(aContext)
	{
	LOG(Log::Printf(_L("new\tqos HOOK[%u] on FLOW[%u] size=%d"), (TInt)this, (TInt)&aContext, sizeof(CFlowHook)));
	iProtocol.Open();	// The CProtocolQoS cannot be deleted until all flows have been closed!
	iInterface = NULL;
	iRefCount = 0;
	iSession = NULL;
	iFlowStatus = EFlow_READY;
	iChannel = NULL;
	TSoOwnerInfo owner_info;
	TPckg<TSoOwnerInfo> option(owner_info);
	TInt ret = iContext.RetrieveOption(KSOLProvider, KSoOwnerInfo, option);
	if (ret == KErrNone)
		{
		iUid = owner_info.iUid;
		LOG(Log::Printf(_L("\tqos HOOK[%u] Ownerinfo UID [0x%x 0x%x 0x%x]"),
			(TInt)this,
			owner_info.iUid[0].iUid,
			owner_info.iUid[1].iUid,
			owner_info.iUid[2].iUid ));
		}
	// Turn off HeaderMode flag
	// (so that we don't think QoS has changed in ReadyL, if nothing has been specified)
	iQoS.SetHeaderMode(EFalse);
	}

CFlowHook::~CFlowHook()
	{
	iProtocol.Close();	// May trigger final destruction of CProtocolQoS!
	LOG(Log::Printf(_L("~\tqos HOOK[%u] on FLOW[%u] destructor completed"), (TInt)this, (TInt)&iContext));
	}


void CFlowHook::Open()
	{
	iRefCount++;
	}

void CFlowHook::Close()
	{
	if (--iRefCount < 0)
		{
		LOG(Log::Printf(_L("Close\tqos HOOK[%u] on FLOW[%u] closed with refs=0 -- start shutdown"), (TInt)this, (TInt)&iContext));
		CloseQoS();
		iHookLink.Deque();
		delete this;
		}
	}


void CFlowHook::FillFlowInfo(TPfqosMessage& aMsg, pfqos_address& aSrc, pfqos_address& aDst, pfqos_selector& aSel, TInetAddr& aMsk)
	/**
	* Fill in flow specific information for PF_QOS Message.
	*
	* Initialize three extesion (EPfqosExtSrcAddress, EPfqosExtDstAddress and EPfqosExtSelector) into
	* TPfqosMessage. The caller must provide the space for the required extension blocks. This only
	* initializes the blocks and sets the iExt references to the aMsg.
 	*
	*
	* @param aMsg The PF_QOS Message to be used
	* @param aSrc The source address block for the Msg
	* @param aDst The destination address block for the Msg
	* @param aSel The selector block for the Msg.
	* @param aMsk Intialized to IPv6 all-ones mask.
	*/
	{
	// The old QOSLIB expects flow specific events have PF_QOS Message blocks
	// src, dst and sel filled from the policy selector that the flow is using.
	// This is very dubious, but at this point fixing the old QOSLIB is not
	// sensible. Because FLOW SPECIFIC OQS can only be requested through the
	// QOSLIB, implement a special backward compatibility for this, if
	// "selector" is located.
	const CSelectorBase* selector = NULL;
	if (iChannel == NULL)
		if ((selector = Policies().iPolicy) == NULL)
			selector = Policies().iDefault;
	
	aMsk.PrefixMask(128);	// All ones IPv6 address.

	aDst.pfqos_address_len = ((sizeof(pfqos_address) + sizeof(TInetAddr) + sizeof(TInetAddr) + 7) / 8);
	aDst.pfqos_ext_type = EPfqosExtDstAddress;
	aDst.reserved = 0;
	if (selector)
		{
		// For QOSLIB Only
		aDst.pfqos_port_max = selector->iDstPortMax;
		aMsg.iDstAddr.iAddr = &selector->iDst;
		aMsg.iDstAddr.iPrefix = &selector->iDstMask;
		}
	else
		{
		aDst.pfqos_port_max = Context().RemotePort();
		aMsg.iDstAddr.iAddr = &Context().RemoteAddr();
		aMsg.iDstAddr.iPrefix = &aMsk;
		}
	aMsg.iDstAddr.iExt = &aDst;

	aSrc.pfqos_address_len = ((sizeof(pfqos_address) + sizeof(TInetAddr) + sizeof(TInetAddr) + 7) / 8);
	aSrc.pfqos_ext_type = EPfqosExtSrcAddress;
	aSrc.reserved = 0;
	if (selector)
		{
		// For QOSLIB Only
		aSrc.pfqos_port_max = selector->iSrcPortMax;;
		aMsg.iSrcAddr.iAddr = &selector->iSrc;
		aMsg.iSrcAddr.iPrefix = &selector->iSrcMask;
		}
	else
		{
		aSrc.pfqos_port_max = Context().LocalPort();
		aMsg.iSrcAddr.iAddr = &Context().LocalAddr();
		aMsg.iSrcAddr.iPrefix = &aMsk;
		}
	aMsg.iSrcAddr.iExt = &aSrc;

	if (selector)
		{
		// For QOSLIB Only
		// Brutally run the new constructor for T_pfqos_selector on aSel.
		// (Need to typecast "const" away from selector -- T_pfqos_selector() is misdeclared, argument should be const!)		
		new (&aSel) T_pfqos_selector((CSelectorBase *)selector);
		}
	else
		{
		aSel.pfqos_selector_len = ((sizeof(pfqos_selector) + 7) / 8);
		aSel.pfqos_ext_type = EPfqosExtSelector;
		aSel.protocol = (TUint16)Context().Protocol();
		aSel.uid1 = Uid().UidType()[0].iUid;
		aSel.uid2 = Uid().UidType()[1].iUid;
		aSel.uid3 = Uid().UidType()[2].iUid;
			aSel.iap_id = Interface() ? Interface()->IapId() : 0;
		aSel.policy_type = EPfqosFlowspecPolicy;
		aSel.priority = EPfqosApplicationPriority;
		aSel.reserved = 0;
		aSel.name[0] = 0;
		}
	aMsg.iSelector.iExt = &aSel;
	}

//
// ReadyL() is called when a flow comes to EFlow_READY-state. 
//
TInt CFlowHook::ReadyL(TPacketHead &/*aHead*/)
	{
	// If there is a session going on, the flow is not ready to go!
	LOG(Log::Printf(_L("")));
	if (iSession)
		{
		LOG(Log::Printf(_L("ReadyL\tqos HOOK[%u] on FLOW[%u] pending on session[%u]"), (TInt)this, (TInt)&iContext, (TInt)iSession));
		return EFlow_PENDING;
		}

	LOG(Log::Printf(_L("ReadyL\tqos HOOK[%u] on FLOW[%u]"), (TInt)this, (TInt)&iContext));
	CNifIfBase *const nif = iContext.Interface();
	if (iInterface && nif != iInterface->Nif())
		{
		// ReadyL can be called multiple times. If the interface has
		// changed, everything must be started from the beginning...
		CloseQoS();
		}

	// Set interface and start QoS negotiation

	// When ReadyL is called, then if iContext.Interface() is normally a non-NULL
	// pointer to the attached NIF (CNifIfBase). However, the stack architecture allows
	// also that the final connect could be virtual interface without any NIF. Because such
	// connect requires a special hook to handle the packets, there are no known uses
	// of such at this point. However, the QoS code is not prepared to handle NULL NIF
	// properly and should not do anything with such flows.
	if (nif == NULL)
		{
		ASSERT(iInterface == NULL); // Logically interface cannot be non-NULL here!
		// ...just return READY and assume rest of the QoS does not panic
		// on iInterface being NULL. [Note: currently there are no known
		// configurations in use, which would lead into this branch]
		return EFlow_READY;
		}

	if (iInterface == NULL)
		{
		//?? Combine FindInterface and AddInterfaceL into single LocateInterfaceL?
		iInterface = iProtocol.InterfaceMgr()->FindInterface(nif);
		if (!iInterface)
			{
			iInterface = iProtocol.InterfaceMgr()->AddInterfaceL(nif);
			}
		ASSERT(iInterface);			// ...at this point, the interface must be present.
		// Find policies and load modules

		// Attach channel if flow belongs to such.
		iChannel = iProtocol.ChannelMgr()->AttachChannel(this);

		// Load modules specified in QoS policy
		LoadModulesL();

		// Open flow in modules (interface must be set before this!!)
		OpenModulesL();

		UpdateQoS();		// does nothing if iChannel!
		}
		
	if (iChannel)
		{
		// Notify Channel
		TInt ret = iChannel->HookReadyL(*this);
		if (ret != 0)
			{
			LOG(Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] ReadyL returns=%d (channel)"), (TInt)this, (TInt)&iContext, ret));
			return ret;	// Flow not ready or has some fatal error...
			}
		}
	else if (iQoSChanged)
		{
		// Pending QoS change, try to start negotiation (ignore errors)
		(void)Negotiate();
		}
#ifdef _LOG
	if (iSession)
		Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] ReadyL Returns=1 pending session [%u]"),
			(TInt)this, (TInt)&iContext, (TInt)iSession);
	else
		Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] ReadyL Returns iFlowStatus=%d"),
			(TInt)this, (TInt)&iContext, iFlowStatus);
#endif
	return iSession ? EFlow_PENDING : iFlowStatus;
	}



TInt CFlowHook::ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo)
	{
	LOG(Log::Printf(_L("")));
	LOG(Log::Printf(_L("ApplyL\tqos HOOK[%u] on FLOW[%u]"), (TInt)this, (TInt)&iContext));

	const TInt N = iModuleList.Count();
	//?? Note: ApplyL of the TrafficControl module has not been called before,
	//?? so keep that semantics unchanged. But, is it actually correct by spec?
	//?? (if present, control module is at index 0)
	for (TInt i = iHasControlModule ? 1 : 0; i < N; ++i)
		{
		iModuleList[i]->Module()->ApplyL(aPacket, aInfo);
		}
	return KErrNone;
	}

TInt CFlowHook::Negotiate()
	{
	if (!Interface() || iChannel || iQoSChanged == 0)
		{
		return KErrGeneral;
		}

	// Check if any modules are associated to a policy
	// Send reply msg indicating that no modules are associated to a QoS 
	// policy
	if (iModuleList.Count() == 0)
		{
		iProtocol.Event(KPfqosEventFailure, *this, iQoS, EQoSNoModules);
		return KErrNone;
		}

	// only one QoS negotiation can be active at once
	if (iSession == NULL)
		{
		iQoSChanged = 0;
		return StartPendingRequest(new CNegotiateSession(*this));
		}
	// Busy negotiating something else, start this later...
	return KErrNone;
	}
	
void CFlowHook::LoadModulesL()
	/**
	* Load modules specified in a QoS policy.
	*/
	{
	ASSERT(iModuleList.Count() == 0);	// The list must be empty before this!
	ASSERT(!iHasControlModule);			// The list is empty, there cannot be control module either!

	RModule* rmodule=NULL;
	if (iInterface->TrafficControl())
		{
		// Interface already has a control module. Make a copy reference
		// to the same module and add to the slot 0 in the module list.
		rmodule = new (ELeave) RModule(*iInterface->TrafficControl());
		if (iModuleList.Append(rmodule) != KErrNone)
			{
			delete rmodule;
			User::Leave(KErrNoMemory);
			}
		iHasControlModule = TRUE;
		}

	Policies().iModules = (CModuleSelector*)iProtocol.PolicyMgr()->FindPolicy(&Context(),
			EPfqosModulespecPolicy, Uid().UidType(), 
			Interface()->IapId(), EPfqosApplicationPriority, Interface()->InterfaceName());

	CModuleSelector*const sel = Policies().iModules;
	if (!sel)
		{
		// No other modules.
		return;
		}
	TDblQueIter<CModuleSpec> iter(sel->GetModuleList());
	CModuleSpec *module;
	while ((module = iter++) != NULL)
		{
		TRAPD(err, rmodule = iProtocol.ModuleMgr()->LoadModuleL(*module, iProtocol.NetworkService()->Protocol()));
		if (err == KErrNone)
			{
			if (rmodule->Flags() & KQoSModuleBindToInterface)
				{
				// If module has to be binded to an interface, the CNifIfBase
				// Name must match with the name specified in modulespec 
				// selector.
				if (iHasControlModule || sel->iName.Compare(iInterface->InterfaceName()) != 0)
					{
					//coverity[leave_without_push]
					delete rmodule;
					}
				else
					{
					iInterface->SetTrafficControl(rmodule);
					LOG(Log::Printf(_L("\t\tCalling module[%S]::InterfaceAttached(IF [%S])"),
						&rmodule->Name(), &iInterface->Name()));
					rmodule->Module()->InterfaceAttached(iInterface->Name(), iInterface->Nif());
					// Make a copy of the control module reference and place it at index 0.
					rmodule = new (ELeave) RModule(*rmodule);
					if (iModuleList.Insert(rmodule, 0) != KErrNone)
						{
						delete rmodule;
						User::Leave(KErrNoMemory);
						}
					iHasControlModule = TRUE;
					}
				}
			else
				{
				if (iModuleList.Append(rmodule) != KErrNone)
					{
					delete rmodule;
					}
				}
			}
		}
	}


void CFlowHook::OpenModulesL()
	{
	ASSERT(Interface() != NULL);

	const TInt N = iModuleList.Count();
	for (TInt i = 0; i < N; ++i)
		{
		RModule* const module = iModuleList[i];
		LOG(Log::Printf(_L("")));
		LOG(Log::Printf(_L("calling\tmodule[%S]::OpenL(FLOW[%u]) for HOOK[%u] on IF [%S])"),
			&module->Name(), (TInt)&iContext, (TInt)this, &iInterface->Name()));
		module->Module()->OpenL(iContext, Interface()->Nif());
		LOG(Log::Printf(_L("<<<\treturns from module[%S] for HOOK[%u]"), &module->Name(), (TInt)this));
		}
	//?? If this leaves half way, someone should close the modules
	//?? that were succesfully opened...? [the flow close will do this,
	//?? but to all modules in the list]
	}

void CFlowHook::CloseQoS()
	/**
	* Close the QoS setup for a flow.
	*
	* Cancel QoS done by the ReadyL. Flow has no QoS resources
	* attached after this.
	*/	
	{
	// Note: iSession is self destructive object, which automaticly destructs
	// after the ClearPendingRequest call. If it has not done that yet, force
	// the destruction here (the session destructor must deal with any pending
	// uncompleted actions in some way!) and not call ClearPendingRequest
	// any more.
	delete iSession;
	iSession = NULL;

	for (TInt i = iModuleList.Count(); --i >= 0; )
		{
		RModule* const module = iModuleList[i];

		LOG(Log::Printf(_L("")));
		LOG(Log::Printf(_L("calling\tmodule[%S]::Close(FLOW[%u]) for HOOK[%u]"), &module->Name(), (TInt)&iContext, (TInt)this));
		module->Module()->Close(Context());
		LOG(Log::Printf(_L("<<<\treturns from module[%S] for HOOK[%u]"), &module->Name(), (TInt)this));
		delete module; 
		}
	iModuleList.Reset();
	iHasControlModule = FALSE;

	// Close channel after Close() to QoS modules
	if (iChannel)
		{
		iChannel->Detach(*this);
		iChannel = NULL;
		}
	
	iInterface = NULL;
	}

void CFlowHook::RestartQoS()
	/**
	* Restart the QoS for the flow.
	*
	* This function closes the current QoS framework assigned to the
	* flow and requests a rerun of the ReadyL phase.
	*
	* This should be called when the flow is moved between channel
	* and other QoS modes...
	*/
	{
	LOG(Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] RestartQoS"), (TInt)this, (TInt)&iContext));
	CloseQoS();
	// Force immeadiate call to ReadyL by poking the flow status.
	iContext.SetStatus(EFlow_PENDING);
	LOG(Log::Printf(_L(">>>\tCalling FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
	iFlowStatus = EFlow_READY;
	iContext.SetStatus(EFlow_READY);
	LOG(Log::Printf(_L("<<<\tReturns FLOW[%u]::SetStatus(0)"), (TInt)&iContext));

	//?? The above is somewhat inconvenient, as the SetStatus(EFlow_READY) can
	//?? directly call ReadyL again. Should be careful to avoid infinite
	//?? recursive loops -- it would be better to have a way to trigger
	//?? this in delayed way: either implement a callback here, or hook API
	//?? should have some feature to trigger it.
	}



void CFlowHook::ReleasePolicy(const CPolicySelector* aSel)
	/**
	* The selector has been deleted, remove references
	*/
	{
	ASSERT(aSel);	// should not be called with NULL ...
	if (aSel == NULL)
		return;		// ...but, if it happens, do nothing!

	TInt was_used = 0;
	if (Policies().iPolicy == aSel)
		{
		Policies().iPolicy = NULL;
		was_used = 1;
		}
	if (Policies().iDefault == aSel)
		{
		Policies().iDefault = NULL;
		was_used = 1;
		}
	if (Policies().iOverride == aSel)
		{
		Policies().iOverride = NULL;
		was_used = 1;
		}
	if (was_used)
		UpdateQoS();
	}

void CFlowHook::UpdateQoS()
	{
	// update QoS policy if flow does not belong to a QoS channel
	if (iChannel)
		return;
	
	Policies().iDefault = (CPolicySelector*)iProtocol.PolicyMgr()->FindPolicy(&Context(),
		EPfqosFlowspecPolicy, Uid().UidType(), 
		Interface()->IapId(), EPfqosDefaultPriority);
	Policies().iPolicy = (CPolicySelector*)iProtocol.PolicyMgr()->FindPolicy(&Context(), 
		EPfqosFlowspecPolicy, Uid().UidType(), 
		Interface()->IapId(), EPfqosApplicationPriority);

	//
	// Combine default and application policies
	//
	TQoSParameters spec;
    //lint -e{961} Missing 'else' is OK
	if (Policies().iDefault && Policies().iPolicy)
		{
		spec = Policies().iPolicy->QoSParameters();
		TPolicyCombine::CombineDefault(Policies().iDefault->QoSParameters(), spec);
		}
	else if (Policies().iDefault)
		{
		spec = Policies().iDefault->QoSParameters();
		}
	else if (Policies().iPolicy)
		{
		spec = Policies().iPolicy->QoSParameters();
		}

	Policies().iOverride = (CPolicySelector*)iProtocol.PolicyMgr()->FindPolicy(&Context(),
		EPfqosFlowspecPolicy, Uid().UidType(), 
		Interface()->IapId(), EPfqosOverridePriority);
	//
	// Combine application and override policies
	if (Policies().iOverride && Policies().iPolicy)
		{
		(void)TPolicyCombine::CombineOverride(Policies().iOverride->QoSParameters(), spec);
		}

	AdjustForHeaderMode(spec);
	if (!(spec == iQoS))
		{
		SetQoS(spec);
		iQoSChanged = 1;

		// Force immeadiate call to ReadyL by poking the flow status
		// (if it looks probable that negotiation can be started)
		if (iFlowStatus == EFlow_READY && iSession == NULL)
			{
			//?? This really should be done by some callback mechanism (can generate
			//?? very long call chains -- the latter SetStatus may call ReadyL!)
			iContext.SetStatus(EFlow_PENDING);
			LOG(Log::Printf(_L(">>>\tCalling FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
			iContext.SetStatus(EFlow_READY);
			LOG(Log::Printf(_L("<<<\tReturns FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
			}
		}
	}


void CFlowHook::SetQoS(const TQoSParameters& aSpec)
	{
	iQoS = aSpec;
	AdjustForHeaderMode(iQoS);
	}

TInt CFlowHook::StartPendingRequest(CQoSSessionBase* aSession)
	/**
	* Start pending request and own the session.
	*
	* Starts the pending request and until it completes, owns
	* the object. The ClearPendingRequest is an indication of
	* the end of the ownership (session self destructs).
	*
	* @return KErrNoMemory, if aSession == NULL (allow to use the new action as parameter)
	*/
	{
	delete iSession;			// (If there is another session, just scrap it).
	iSession = aSession;
	if (iSession == NULL)
		return KErrNoMemory;
	iSession->Run();
	return KErrNone;
	}

void CFlowHook::ClearPendingRequest(TInt /*aResult*/)
	/**
	* A pending request has completed.
	* @param aResult The completion result
	*/
	{
	ASSERT(iSession != NULL);		// Should never get here without a session!
	if (iSession)
		{
		iSession = NULL;
		// The result of negotiation, even if fails, does not affect the
		// flow, but the flow needs to be woken up.
		iFlowStatus = EFlow_READY;
		LOG(Log::Printf(_L(">>>\tCalling FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
		iContext.SetStatus(EFlow_READY);
		LOG(Log::Printf(_L("<<<\tReturns FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
		}
	}

void CFlowHook::Block()
	{
	iFlowStatus = EFlow_HOLD;
	LOG(Log::Printf(_L("\tBlocking FLOW[%u]::SetStatus(EFlow_HOLD)"), (TInt)&iContext));
	iContext.SetStatus(EFlow_HOLD);
	};

void CFlowHook::UnBlock()
	{
	iFlowStatus = EFlow_READY;
	LOG(Log::Printf(_L(">>>\tUnblocking FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
	iContext.SetStatus(EFlow_READY);
	LOG(Log::Printf(_L("<<<\tReturns    FLOW[%u]::SetStatus(0)"), (TInt)&iContext));
	};


void CFlowHook::AdjustForHeaderMode(TQoSParameters& aQoS)
	// This code works only if we are running as topmost ReadyL, just
	// under the upper layer protocol. In that case, the HeaderSize()
	// of the CFlowContext includes all overhead caused by other
	// hooks. It still misses the overhead of
	// - upper layder protocol
	// - IP header
	//
	// However, getting the correct filter information requires that we
	// are the last hook. This means that header size estimation only
	// works now, if there are no other hooks adding headers to the
	// flow.
	{
	if (!aQoS.GetHeaderMode())
		return;	// All done, nothing else to do.


	// Try guessing the current header size
	TInt headersize = iContext.HeaderSize();
	switch (iContext.Protocol())
		{
	case KProtocolInetIcmp:
		headersize += TInet6HeaderICMP::MaxHeaderLength();
		break;
	
	case KProtocolInetTcp:
		headersize += TInet6HeaderTCP::MaxHeaderLength();
		break;
	
	default: // For anything else, just make a guess and use UDP
	case KProtocolInetUdp:
		headersize += TInet6HeaderUDP::MaxHeaderLength();
		break;
		}
	// Make an estimate of IP header overhead
	if (iContext.iHead.ip6.Version() == 4)
		// Use Min length for IPv4, because IP options are really not used in this system
		headersize += TInet6HeaderIP4::MinHeaderLength();
	else
		headersize += TInet6HeaderIP::MaxHeaderLength();
	
	
	
	// Adjust parameters for the header overhead

	TInt maxHeaderRate;
	if (aQoS.GetUpLinkAveragePacketSize() > 0)
		{
		maxHeaderRate = (aQoS.GetUplinkBandwidth() + 
						 aQoS.GetUpLinkMaximumBurstSize()) / 
						 aQoS.GetUpLinkAveragePacketSize();
		}
	else if (aQoS.GetUpLinkMaximumPacketSize() > 0)
		{
		maxHeaderRate = (aQoS.GetUplinkBandwidth() + 
						 aQoS.GetUpLinkMaximumBurstSize()) / 
						 aQoS.GetUpLinkMaximumPacketSize();
		}
	else
		{
		maxHeaderRate = 0;
		}
	maxHeaderRate *= headersize;
	LOG(Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] est-headersize = %d maxHeaderRate=%d"),
		(TInt)this, (TInt)&iContext, headersize, maxHeaderRate));


	if (aQoS.GetUplinkBandwidth() > 0)
		{
		aQoS.SetUplinkBandwidth(aQoS.GetUplinkBandwidth() +  maxHeaderRate);
		}
	if (aQoS.GetUpLinkMaximumPacketSize() > 0)
		{
		aQoS.SetUpLinkMaximumPacketSize(aQoS.GetUpLinkMaximumPacketSize() + headersize);
		}

	if (aQoS.GetDownlinkBandwidth() > 0)
		{
		aQoS.SetDownlinkBandwidth(aQoS.GetDownlinkBandwidth() + maxHeaderRate);
		}
	if (aQoS.GetDownLinkMaximumPacketSize() > 0)
		{
		aQoS.SetDownLinkMaximumPacketSize(aQoS.GetDownLinkMaximumPacketSize() + headersize);
		}

	// Constrain the packet size with MTU
	// (to be exactly correct: one should note that if application will be using larger than MTU
	// packets, they will get fragmented, and that overhead might need to computed/estimated
	// here also?)
	const TInt incoming = iContext.InterfaceRMtu();
	const TInt outgoing = iContext.InterfaceSMtu();
	if (aQoS.GetDownLinkMaximumPacketSize() > incoming)
		aQoS.SetDownLinkMaximumPacketSize(incoming);
	if (aQoS.GetUpLinkMaximumPacketSize() > outgoing)
		aQoS.SetUpLinkMaximumPacketSize(outgoing);
	// Adjusted parameters, no "header mode" any more.
	aQoS.SetHeaderMode(EFalse);

	return;
	}
