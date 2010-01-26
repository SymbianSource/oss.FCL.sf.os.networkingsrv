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

#include <es_ini.h>
#include <flow.h>
#include "qos_prot.h"
#include "policy_sap.h"
#include "policy_mgr.h"
#include "flowhook.h"
#include "modules.h"
#include "interface.h"
#include "qos_channel.h"
#include <es_prot_internal.h>

#ifdef _LOG

TLogAddress::TLogAddress(const TIpAddress& aAddr, TInt aPort)
	{
	SetAddr(aAddr, aPort);
	}

TLogAddress::TLogAddress(const TInetAddr &aAddr)
	{
	SetAddr(aAddr);
	}

void TLogAddress::SetAddr(const TIpAddress &aAddr, TInt aPort)
	{
	TInetAddr addr(aAddr, aPort);
	addr.SetScope(aAddr.iScope);
	SetAddr(addr);
	}

void TLogAddress::SetAddr(const TInetAddr &aAddr)
	{
	aAddr.OutputWithScope(*this);
	if (aAddr.Port())
		{
		_LIT(KFormat, "#%d");
		AppendFormat(KFormat, aAddr.Port());
		}
	}
#endif


CProtocolQoS::CProtocolQoS()
	{
	LOG(Log::Printf(_L("new\tqos protocol[%u] size=%d"), (TInt)this, sizeof(CProtocolQoS)));
	iSAPlist.SetOffset(_FOFF(CQoSProvider, iSAPlink));
	for (TUint i = 0; i < KFlowTableSize; i++)
		iFlowTable[i].SetOffset(_FOFF(CFlowHook, iHookLink));
	}


CProtocolQoS::~CProtocolQoS()
	{
	LOG(Log::Printf(_L("~\tqos protocol[%u] destructor -- start"), (TInt)this));
	delete iChannelMgr;
	delete iInterfaceManager;
	delete iModuleManager;
	delete iPolicyMgr;

	// CProtoclQoS cannot be destroyed until all flows have been
	// closed -- thus iFlowTable[*] are all empty at this point!

	// CProtocolPosthook destructor automaticly unbinds this from
	// the network, no Unbinds needed here...
	LOG(Log::Printf(_L("~\tqos protocol[%u] destructor -- completed"), (TInt)this));
	}

void CProtocolQoS::InitL(TDesC &/*aTag*/)
	{

	ReadConfigOptions();
	iModuleManager = CModuleManager::NewL(*this);
	iInterfaceManager = CInterfaceManager::NewL(*this);
	iPolicyMgr = CQoSPolicyMgr::NewL(iConfigOptions.iPolicyFile);
	iChannelMgr = CQoSChannelManager::NewL(*this);
	}

void CProtocolQoS::Identify(TServerProtocolDesc* aProtocolDesc) const
	{
	CProtocolQoS::Identify(*aProtocolDesc);
	}

void CProtocolQoS::Identify(TServerProtocolDesc& aDesc)
	{
	_LIT(Kpfqos, "pfqos");

	aDesc.iName = Kpfqos;
	aDesc.iAddrFamily=KAfInet;
	aDesc.iSockType=KSockDatagram;
	aDesc.iProtocol=KProtocolQoS;
	aDesc.iVersion=TVersion(KQoSMajorVersionNumber, KQoSMinorVersionNumber, 
							KQoSBuildVersionNumber);
	aDesc.iByteOrder=EBigEndian;
	aDesc.iServiceInfo=KSIConnectionLess | KSIMessageBased;
	aDesc.iNamingServices=0;
	aDesc.iSecurity=KSocketNoSecurity;
	aDesc.iMessageSize=0xffff;
	aDesc.iServiceTypeInfo= ENeedMBufs | ESocketSupport;
	aDesc.iNumSockets=KUnlimitedSockets;
	}

CServProviderBase* CProtocolQoS::NewSAPL(TUint aSockType)
	{
	if (aSockType != KSockDatagram)
		{
		User::Leave(KErrNotSupported);
		}
	CQoSProvider *pSAP = new (ELeave) CQoSProvider(*this);
	//coverity[leave_without_push]
	LOG(Log::Printf(_L("NewSAPL\tqos SAP[%u] size=%d"), (TInt)pSAP, sizeof(CQoSProvider)));
	pSAP->iFlags = KPfqosEventAll;
	iSAPlist.AddLast(*pSAP);
	return pSAP;
	}

void CProtocolQoS::NetworkAttachedL()
	{
	LOG(Log::Printf(_L("<>\tqos NetworkAttachedL")));

	// Bind as "output hook" with priority 1
	NetworkService()->Protocol()->BindL(this, BindFlowHook());

	// Bind as "post hook" (also solicits StartSending calls)
	NetworkService()->Protocol()->BindL(this, BindPostHook());
	}

void CProtocolQoS::NetworkDetached()
	{
	LOG(Log::Printf(_L("<>\tqos NetworkDetached -- start")));

	// Call Unbind to QoS modules and interfaces at this time to make sure 
	// that there is no CFlowContext references in queued packets.
	iModuleManager->Unbind(NetworkService()->Protocol(), 0);
	
	// All packets should now be dropped
	CloseFlows();

	// Unbind hook
	NetworkService()->Protocol()->Unbind(this, 0);
	LOG(Log::Printf(_L("\tqos NetworkDetached -- completed")));
	}

TInt CProtocolQoS::Send(RMBufChain& aPacket, CProtocolBase* aSrc)
	{
	const RMBufSendInfo *const info = RMBufSendPacket::PeekInfoInChain(aPacket);
	for (;;)	// NOT A LOOP, JUST FOR BREAK EXITS!
		{
		if (info == NULL)
			{
			// If there is no info, it is probably an error. Let the
			// default packet processing handle the mess...
			break;
			}

		const CFlowContext*const  context = info->iFlow.FlowContext();
		CFlowHook *hook = FindHook(context);
		if (hook == NULL)
			{
			// CFlowHook does not exist for this context, assume
			// QoS is not interested on this -- fall to default.
			break;
			}
		CInterface* iface = hook->Interface();
		if (iface == NULL)
			{
			// No interface at this stage is some kind of internal
			// error in QoS, but just fall to default packet path.
			break;			
			}
		RModule *traffic_control = iface->TrafficControl();
		if (traffic_control == NULL)
			{
			// If the attached interface has no traffic control
			// module, then fall to default packet path (as if
			// no QoS was present!
			break;
			}
		LOG(Log::Printf(_L("Send\tqos HOOK[%u] on FLOW[%u] packet length=%d"), (TInt)hook, (TInt)context, info->iLength));
		return traffic_control->Module()->Send(aPacket);
		}
	// QoS is not interested in this packet flow, just
	// fall to normal packet path.
	LOG(Log::Printf(_L("Send\tqos pass packet")));
	return CProtocolPosthook::Send(aPacket, aSrc);
	}

// StartSending will be called when the interface is ready to 
// receive packets. Interface will be set to unblocked state.
void CProtocolQoS::StartSending(CProtocolBase* /*aFace*/)
	{
	// QoS Framework doest not need this call for anything at this point.
	}

MFlowHook *CProtocolQoS::OpenL(TPacketHead & /*aHead*/, CFlowContext *aFlow)
	{
	CFlowHook *hook = new (ELeave) CFlowHook(*this, *aFlow);
	TDblQue<CFlowHook>& queue = iFlowTable[FlowHashKey(aFlow)];
	queue.AddLast(*hook);
	LOG(Log::Printf(_L("")));
	LOG(Log::Printf(_L("OpenL\tqos HOOK[%u] to FLOW[%u]"), (TInt)hook, (TInt)aFlow));
	return hook;
	}


TInt CProtocolQoS::GetFlowOption(TUint /*aLevel*/, TUint /*aName*/, 
	TDes8& /*aOption*/, const CFlowContext& /*aFlow*/) const
	{
	return KErrNotSupported;
	}


TInt CProtocolQoS::SetFlowOption(TUint aLevel, TUint aName, 
	const TDesC8 &aOption, CFlowContext& aFlow)
	{
	if (aLevel == STATIC_CAST(TUint,KSOLProvider))
		{
		switch(aName)
			{
			case KSoOwnerInfo:
				{
				TInt ret = aFlow.StoreOption(aLevel, aName, aOption);
				if (ret != KErrNone)
					{
					return ret;
					}
				CFlowHook* hook = FindHook(&aFlow);
				if (hook)
					{
					if (STATIC_CAST(TUint, aOption.Length()) != 
						sizeof(TSoOwnerInfo))
						{
						return KErrArgument;
						}
					const TSoOwnerInfo& owner_info = 
						*(TSoOwnerInfo*)aOption.Ptr();
					hook->SetUid(owner_info.iUid);
					hook->UpdateQoS();
					LOG(Log::Printf(_L("\tqos HOOK[%u] on FLOW[%u] FlowOption Ownerinfo [0x%x 0x%x 0x%x]"),
						(TInt)hook, (TInt)&aFlow,
						owner_info.iUid[0].iUid, 
						owner_info.iUid[1].iUid, 
						owner_info.iUid[2].iUid ));
					}
				}
				return KErrNone;

			default:
				break;
			}
		}
	return KErrNotSupported;
	}

// Event() builds a pfqos-message that is used to inform applications about
// the result of QoS request. Event() does not check event_type or anything,
// caller has to take care of that!
TInt CProtocolQoS::Event(TUint16 aEvent, CFlowHook& aHook, 
						 const TQoSParameters& aSpec, TInt aErrorCode, 
						 TInt aExtensionType, const TDesC8& aExtension)
	{
	TPfqosMessage msg;

	T_pfqos_msg base(EPfqosEvent);
	base.pfqos_msg_errno = aErrorCode;
	msg.iBase.iMsg = &base;

	T_pfqos_flowspec spec(aSpec);
	msg.iFlowSpec.iExt = &spec;

	T_pfqos_event event(EPfqosExtEvent, aEvent, 0);
	msg.iEvent.iExt = &event;

	// Add flow identifying PF_QOS blocks
	TInetAddr msk;
	pfqos_address src, dst;
	pfqos_selector sel;
	aHook.FillFlowInfo(msg, src, dst, sel, msk);

	if (aExtensionType >= 0)
		{
		TRAPD(err, msg.AddExtensionL(aExtension, aExtensionType));
		msg.iError = err;
		}

	msg.iNumModules = 0;
	Deliver(msg, KProviderKey_RegisteredQoSConf);
	return KErrNone;
	}

TInt CProtocolQoS::Event(TUint16 aEvent, TInt aChannelId, 
						 const TQoSParameters& aSpec, TInt aErrorCode, 
						 TInt aExtensionType, const TDesC8& aExtension)
	{
	TPfqosMessage msg;

	T_pfqos_msg base(EPfqosEvent);
	base.pfqos_msg_errno = aErrorCode;
	msg.iBase.iMsg = &base;

	T_pfqos_flowspec* spec = new T_pfqos_flowspec(aSpec);
	if (spec == NULL)
		{
		msg.iError = KErrNoMemory;
		}
	else
		{
		msg.iFlowSpec.iExt = spec;
		}

	T_pfqos_event event(EPfqosExtEvent, aEvent, 0);
	msg.iEvent.iExt = &event;

	T_pfqos_channel channel(aChannelId);
	msg.iChannel.iExt = &channel;

	if (aExtensionType >= 0)
		{
		TRAPD(err, msg.AddExtensionL(aExtension, aExtensionType));
		msg.iError = err;
		}

	msg.iNumModules = 0;
	Deliver(msg, KProviderKey_RegisteredQoSConf);

	delete spec;
	return KErrNone;
	}

void CProtocolQoS::Deliver(TPfqosMessage& aMsg, TUint aMask)
	{
	TDblQueIter<CQoSProvider> iter(iSAPlist);
	CQoSProvider* sap;

	while ((sap = iter++) != NULL)
		{
		if (sap->iFlags & aMask)
			{
			sap->Deliver(aMsg);
			}
		}
	}

CFlowHook* CProtocolQoS::FindHook(const CFlowContext *aContext)
	{
	const TInt key = FlowHashKey(aContext);
	TDblQueIter<CFlowHook> iter(iFlowTable[key]);
	CFlowHook *hook;

	while ((hook = iter++) != NULL)
		{
		if (&hook->Context() == aContext)
			{
			return hook;
			}
		}
	return NULL;
	}

CFlowHook* CProtocolQoS::FindHook(const TQoSConnId &aIdent)
	/**
	* Locate a flow that matches the identification
	* CProtocolQoS maintains a FlowTable that consists of CFlowHook objects.
	* FindHook looks if the identified flow is among them.
	*/
	{
	for (TUint i = 0; i < KFlowTableSize; i++)
		{
		TDblQueIter<CFlowHook> iter(iFlowTable[i]);
		CFlowHook *hook;

		while ((hook = iter++) != NULL)
			{
			if (aIdent.Match(hook->Context()))
				return hook;
			}
		}
	return NULL;
	}


// update flowspecs related to flows
TInt CProtocolQoS::Release(const CPolicySelector* aSel)
	{
	for (TUint i = 0; i < KFlowTableSize; i++)
		{
		TDblQueIter<CFlowHook> iter(iFlowTable[i]);
		CFlowHook *hook;

		while ((hook = iter++) != NULL)
			{
			hook->ReleasePolicy(aSel);
			}
		}
	return KErrNone;
	}

// Update all flows (called when a flowspec policy was added/modified).
void CProtocolQoS::UpdateFlows()
	{
	for (TUint i = 0; i < KFlowTableSize; i++)
		{
		TDblQueIter<CFlowHook> iter(iFlowTable[i]);
		CFlowHook *hook;
		while ((hook = iter++) != NULL)
			{
			hook->UpdateQoS();
			}
		}
	}


void CProtocolQoS::InterfaceAttached(const TDesC &aName, CNifIfBase *aIf)
	{
	(void)aName;	// silence lint?
	// Create and load interface, if not already done
	LOG(Log::Printf(_L("Attach\tqos IF [%S] to NIF[%u]"), &aName, (TInt)aIf));
	if (InterfaceMgr()->FindInterface(aIf) == NULL)
		{
		TRAP_IGNORE((void)InterfaceMgr()->AddInterfaceL(aIf));
		}
	}

void CProtocolQoS::InterfaceDetached(const TDesC& aName, CNifIfBase *aIf)
	{
	(void)aName;	// silence lint?
	CInterface* iface = InterfaceMgr()->FindInterface(aIf);
	if (iface)
		{
		LOG(Log::Printf(_L("Detach\tqos IF [%S] from NIF[%u]"), &iface->Name(), (TInt)aIf));
		// Remove all references to this interface from the hooks
		for (TUint i = 0; i < KFlowTableSize; i++)
			{
			TDblQueIter<CFlowHook> iter(iFlowTable[i]);
			CFlowHook *hook;

			while ((hook = iter++) != NULL)
				{
				if (hook->Interface() == iface)
					{
					hook->CloseQoS();	// ..includes removal of Interface() pointer!
					}
				}
			}

		// Remove all references to this interface from the channels
		// Because all attached flows were removed in above, the channels
		// referencing this flow cannot have any flows attached.
		iChannelMgr->InterfaceDetached(iface);
		
		InterfaceMgr()->RemoveInterface(iface);
		}
	else
		{
		LOG(Log::Printf(_L("Detach\tqos IF [%S] for NIF[%u] does not exist"), &aName, (TInt)aIf));
		}
	}

void CProtocolQoS::BlockFlow(CFlowContext& aFlow)
	{
	CFlowHook *hook = FindHook(&aFlow);
	LOG(Log::Printf(_L("Block\tqos HOOK[%u] on FLOW[%u]"), (TInt)hook, (TInt)&aFlow));
	if (hook)
		{
		hook->Block();
		}
	}


void CProtocolQoS::UnBlockFlow(CFlowContext& aFlow)
	{
	CFlowHook *hook = FindHook(&aFlow);
	LOG(Log::Printf(_L("UnBlock\tqos HOOK[%u] on FLOW[%u]"), (TInt)hook, (TInt)&aFlow));
	if (hook)
		{
		hook->UnBlock();
		}
	}

// QoS modules call this method to notify about events.
void CProtocolQoS::NotifyEvent(CFlowContext& aFlow, TInt aEvent, 
							   const TQoSParameters* aSpec, 
							   const TExtensionData& aExtension)
	{
	CFlowHook *hook = FindHook(&aFlow);

	LOG(Log::Printf(_L("")));
	LOG(Log::Printf(_L("Event(%d)\tfor FLOW[%u] on HOOK[%u]"), aEvent, (TInt)&aFlow, (TInt)hook));
	if (hook == NULL)
		{
		return;	// Ignore events that have no matching hook (this should never happen!)
		}
	Event((TUint16)aEvent, *hook, aSpec ? *aSpec : hook->QoSParameters(), 0, aExtension.iType, aExtension.iData);
	}

void CProtocolQoS::NotifyEvent(TInt aChannelId, TInt aEvent, 
	const TQoSParameters* aParams, const TExtensionData& aExtension)
	{
	CInternalQoSChannel* channel = iChannelMgr->FindChannel(aChannelId);
	LOG(Log::Printf(_L("")));
	LOG(Log::Printf(_L("Event(%d)\tfor channel[%u] id=%d"), aEvent, channel, aChannelId));
	if (channel == NULL)
		{
		// Ignore events that have no matching channel -- should never happen!
		return;
		}
	Event((TUint16)aEvent, aChannelId, aParams ? *aParams : channel->QoSParameters(), 0, aExtension.iType, aExtension.iData);
	}

void CProtocolQoS::LoadFileL(const TDesC& aFile)
	{
	PolicyMgr()->LoadFileL(aFile);
	}

TInt CProtocolQoS::UnLoadFile(const TDesC& aFile)
	{
	return PolicyMgr()->UnloadFile(aFile, *this);
	}

CSelectorBase *CProtocolQoS::Lookup(const CFlowContext& aFlow, TUint aType, 
									TUint aPriority, const TDesC& aName)
	{
	CFlowHook* hook = FindHook(&aFlow);
	if (hook == NULL)
		return NULL;
	return PolicyMgr()->FindPolicy(&aFlow, aType, 
		hook->Uid().UidType(), hook->Interface()->IapId(), aPriority, aName);
	}

CSelectorBase* CProtocolQoS::Lookup(const TInetAddr &aLocal, 
									const TInetAddr &aRemote, TUint aProtocol, 
									TUint aLocalPortMax, TUint aRemotePortMax,
									TUint aType, const TUidType& aUid, 
									TUint32 aIapId, TUint aPriority, 
									const TDesC& aName)
	{
	CSelectorBase *sel = PolicyMgr()->ExactMatch(aLocal, aRemote, aProtocol,
						 aLocalPortMax, aRemotePortMax, aType, aUid, aIapId, 
						 aPriority, aName);
	if (sel)
		{
		return sel;
		}
	sel = PolicyMgr()->Match(aLocal.Ip6Address(), aRemote.Ip6Address(), 
			  aProtocol, aLocalPortMax, aRemotePortMax, aType, aUid, aIapId, 
			  aPriority, aName);
	return sel;
	}

void CProtocolQoS::CloseFlows()
	{
	for (TUint i = 0; i < KFlowTableSize; i++)
		{
		TDblQueIter<CFlowHook> iter(iFlowTable[i]);
		CFlowHook *hook;

		while ((hook = iter++) != NULL)
			{
			hook->CloseQoS();
			}
		}
	}

void CProtocolQoS::ReadConfigOptions()
	{
	CESockIniData* config = NULL;
	
	TRAPD(ret, config = CESockIniData::NewL(KQosIniData));
		
	if (ret == KErrNone)
		{
		iConfigOptions.iPolicyFile.Copy(KQosPoliciesIniFile);
		TInt unload_delay=0;
		if (config->FindVar(KQosIniConfig, KQosIniUnloadDelay, unload_delay) 
			&& unload_delay > 0)
			{
			iConfigOptions.iUnloadDelay = unload_delay;
			}
		delete config;
		}
	}

