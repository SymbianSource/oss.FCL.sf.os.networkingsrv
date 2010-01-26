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

#include "policy_mgr.h"
#include "qos_prot.h"
#include "qoserr.h"
#include "policy_sap.h"
#include "qos_channel.h"
#include "modules.h"

// Send an error message back to applications
void CProtocolQoS::Error(TPfqosMessage &aMsg, struct pfqos_msg &aBase, TInt aError)
	{
	aBase.pfqos_msg_errno = aError;
	Deliver(aMsg, KProviderKey_ExpectInput);
	}


// Dump flowspec policy
void CProtocolQoS::DumpPolicyL(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, 
				   CPolicySelector *sel, CExtensionPolicy* ext)
	{

	T_pfqos_flowspec spec(sel->QoSParameters());
	aMsg.iFlowSpec.iExt = &spec;

	T_pfqos_address dst(EPfqosExtDstAddress, sel->iDstPortMax);
	aMsg.iDstAddr.iExt = &dst;
	aMsg.iDstAddr.iAddr = &sel->iDst;
	aMsg.iDstAddr.iPrefix = &sel->iDstMask;

	T_pfqos_address src(EPfqosExtSrcAddress, sel->iSrcPortMax);
	aMsg.iSrcAddr.iExt = &src;
	aMsg.iSrcAddr.iAddr = &sel->iSrc;
	aMsg.iSrcAddr.iPrefix = &sel->iSrcMask;

	T_pfqos_selector selector(sel);
	aMsg.iSelector.iExt = &selector;

	if (ext)
		{
		TExtensionQueueIter iter(ext->Extensions());
		CExtension *extension;

		while ((extension = iter++) != NULL)
			{
			if (extension->Data().Length() > 0)
				aMsg.AddExtensionL(extension->Data(),extension->Type());
			}
		}

	//??
	// Can't leave pointers hanging.. clear them
	aMsg.iEvent.iExt = NULL;

	aBase.pfqos_msg_len = aMsg.Length64();
	aDst->Deliver(aMsg);
	//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs (not base) here!
	}


// Dump modulespec policy
void CProtocolQoS::DumpPolicy(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, CModuleSelector *sel)
	{
	__ASSERT_ALWAYS(sel, Panic(EPfqosPanic_NullPointer));
	
	aMsg.RemovePolicyData();
	T_pfqos_address dst(EPfqosExtDstAddress, sel->iDstPortMax);
	aMsg.iDstAddr.iExt = &dst;
	aMsg.iDstAddr.iAddr = &sel->iDst;
	aMsg.iDstAddr.iPrefix = &sel->iDstMask;

	T_pfqos_address src(EPfqosExtSrcAddress, sel->iSrcPortMax);
	aMsg.iSrcAddr.iExt = &src;
	aMsg.iSrcAddr.iAddr = &sel->iSrc;
	aMsg.iSrcAddr.iPrefix = &sel->iSrcMask;

	T_pfqos_selector selector(sel);
	aMsg.iSelector.iExt = &selector;

	if (sel != NULL)
		{
		TDblQueIter<CModuleSpec> iter(sel->GetModuleList());
		CModuleSpec *module;

		while ((module = iter++) != NULL)
			{
			T_pfqos_module mod(module->ProtocolId(), module->Flags(), module->Name(), module->FileName(), module->PolicyData()->Data().Length());
			TRAPD(err, aMsg.AddModuleL(mod, module->PolicyData()->Data()));
			if (err != KErrNone)
				{
				aMsg.iError = KErrNoMemory;
				aBase.pfqos_msg_len = aMsg.Length64();
				aDst->Deliver(aMsg);
				//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs (not base) here!
				return;
				}
			}
		}

	//?? these were cleared in RemovePolicyData?
	// Can't leave pointers hanging.. clear them
	aMsg.iEvent.iExt = NULL;
	aMsg.iConfigure.iExt = NULL;
	aMsg.iFlowSpec.iExt = NULL;

	aBase.pfqos_msg_len = aMsg.Length64();
	aDst->Deliver(aMsg);
	//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs (not base) here!
	}


// Dump extension policies
void CProtocolQoS::DumpPolicy(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, CExtensionPolicy *sel)
	{
	if (!sel)
		return;

	//?? missing call to RemovePolicyData?
	
	T_pfqos_address dst(EPfqosExtDstAddress, sel->iDstPortMax);
	aMsg.iDstAddr.iExt = &dst;
	aMsg.iDstAddr.iAddr = &sel->iDst;
	aMsg.iDstAddr.iPrefix = &sel->iDstMask;

	T_pfqos_address src(EPfqosExtSrcAddress, sel->iSrcPortMax);
	aMsg.iSrcAddr.iExt = &src;
	aMsg.iSrcAddr.iAddr = &sel->iSrc;
	aMsg.iSrcAddr.iPrefix = &sel->iSrcMask;

	T_pfqos_selector selector(sel);
	aMsg.iSelector.iExt = &selector;

	TExtensionQueueIter iter(sel->Extensions());
	CExtension *extension;

	while ((extension = iter++) != NULL)
	if (extension->Data().Length() > 0)
		{
		TRAPD(err, aMsg.AddExtensionL(extension->Data(),extension->Type()));
		if (err != KErrNone)
			{
			aMsg.iError = KErrNoMemory;
			aBase.pfqos_msg_len = aMsg.Length64();
			aDst->Deliver(aMsg);
			//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs (not base) here!
			return;
			}
		}

	//??
	// Can't leave pointers hanging.. clear them
	aMsg.iEvent.iExt = NULL;

	aBase.pfqos_msg_len = aMsg.Length64();
	aDst->Deliver(aMsg);
	//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs (not base) here!
	}


// PFQOS_UPDATE message has to contain <header, selector, srcaddr, dstaddr, flowspec OR module(s)>
TInt CProtocolQoS::ExecUpdate(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	if (!aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt || !aMsg.iSelector.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	//
	// Flowspec policies
	//
	if (aMsg.iSelector.iExt->policy_type == EPfqosFlowspecPolicy)
		{
		if (!aMsg.iFlowSpec.iExt)
			{
			Error(aMsg, aBase, EQoSMessageCorrupt);
			return KErrGeneral;
			}

		//
		// Extension policies. NOTE: An extension policy is allowed only if flowspec policy is present.
		// By updating flowspec policy, an additional modules Negotiate() will be called. This allows an
		// additional module to check if an extension policy has been updated.
		CExtensionPolicy *ext = (CExtensionPolicy*)iPolicyMgr->ExactMatch(aMsg, EPfqosExtensionPolicy);
		if (ext)
			{
			ext->iNext.Deque();
			delete ext;
			TRAPD(err, iPolicyMgr->AddExtensionPolicyL(aMsg, (TUint)aSrc));
			if (err != KErrNone)
				{
				Error(aMsg, aBase, err);
				return err;
				}
			}

		CPolicySelector *policy = (CPolicySelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosFlowspecPolicy);
		if (policy)
			{
			Deliver(aMsg, KProviderKey_ExpectInput);
			policy->SetQoSParameters(aMsg.iFlowSpec);
			UpdateFlows();
			}
		else
			{
			aBase.pfqos_msg_errno = KErrNotFound;
			Deliver(aMsg, KProviderKey_ExpectInput);
			}
		return KErrNone;
		}

	// Modulespec policies
	if (aMsg.iSelector.iExt->policy_type == EPfqosModulespecPolicy)
		{
		if (aMsg.iNumModules == 0)
			{
			Error(aMsg, aBase, EQoSNoModules);
			return KErrGeneral;
			}

		CModuleSelector *sel = (CModuleSelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosModulespecPolicy);
		if (sel)
			{
			sel->iNext.Deque();
			delete sel;
			TRAPD(err, iPolicyMgr->AddModulePolicyL(aMsg, (TUint)aSrc));
			if (err != KErrNone)
				{
				Error(aMsg, aBase, err);
				return err;
				}
			}
		else
			aBase.pfqos_msg_errno = KErrNotFound;

		Deliver(aMsg, KProviderKey_ExpectInput);
		}

	return KErrNone;
	}


// PFQOS_ADD message can have one of two formats:
// <hdr, selector, flowspec>: add flowspec
// <hdr, selector, modulespec(s)>: add modulespec
TInt CProtocolQoS::ExecAdd(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	if (!aMsg.iSelector.iExt || !aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	if (aMsg.iSelector.iExt->policy_type == EPfqosFlowspecPolicy)
		{
		if (aMsg.iFlowSpec.iExt != NULL)
			{
			CPolicySelector *policy = (CPolicySelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosFlowspecPolicy);
			if (policy)
				{
				Error(aMsg, aBase, EQoSPolicyExists);
				return EQoSPolicyExists;
				}
			TRAPD(err, policy = iPolicyMgr->AddPolicyL(aMsg));
			if (err != KErrNone)
				{
				Error(aMsg, aBase, err);
				return err;
				}
			// Set owner
			policy->iOwner = (TUint)aSrc;
			CExtensionPolicy *ext = (CExtensionPolicy*)iPolicyMgr->ExactMatch(aMsg, EPfqosExtensionPolicy);

			if (ext)
				{
				Error(aMsg, aBase, EQoSPolicyExists);
				return EQoSPolicyExists;
				}
			TRAP(err, iPolicyMgr->AddExtensionPolicyL(aMsg, (TUint)aSrc));
			if (err != KErrNone)
				{
				Error(aMsg, aBase, err);
				return err;
				}
			
			Deliver(aMsg, KProviderKey_ExpectInput);
			if (policy != NULL)
				{
				// This may generate events, if the policy change
				// affects existing flows.
				UpdateFlows();
				}
			}
		else
			{
			Error(aMsg, aBase, KErrGeneral);
			return KErrGeneral;
			}
		}

	// Add modulespec
	if (aMsg.iSelector.iExt->policy_type == EPfqosModulespecPolicy)
		{
		if (aMsg.iNumModules > 0)
			{
			TRAPD(err, iPolicyMgr->AddModulePolicyL(aMsg, (TUint)aSrc));
			if (err != KErrNone)
				{
				Error(aMsg, aBase, err);
				return err;
				}
			}
		else
			{
			Error(aMsg, aBase, EQoSNoModules);
			return KErrGeneral;
			}
		Deliver(aMsg, KProviderKey_ExpectInput);
		// Note: The change affects the currently active and
		// (and new) flows only when they enter ReadyL phase.
		// If immediate effect is required, all flows should
		// forced to go through the ReadyL here.
		}

	return KErrNone;
	}



//
// PFQOS_DELETE message has to contain <header, selector, srcaddr, dstaddr>
//
TInt CProtocolQoS::ExecDelete(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* /*aSrc*/)
	{
	if (aMsg.iSrcAddr.iExt == NULL || aMsg.iDstAddr.iExt == NULL || aMsg.iSelector.iExt == NULL)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	if (aMsg.iSelector.iExt->policy_type == EPfqosFlowspecPolicy)
		{
		CExtensionPolicy *ext = (CExtensionPolicy*)iPolicyMgr->ExactMatch(aMsg, EPfqosExtensionPolicy);
		if (ext)
			{
			ext->iNext.Deque();
			delete ext;
			}

		CPolicySelector *sel = (CPolicySelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosFlowspecPolicy);
		if (sel)
			{
			// Call Deliver before DeletePolicy as DeletePolicy possibly negotiates
			// new policy  and may generate events for those.
			Deliver(aMsg, KProviderKey_ExpectInput);
			sel->iNext.Deque();
			Release(sel);
			delete sel;
			return KErrNone;
			}
		else
			aBase.pfqos_msg_errno = KErrNotFound;

		Deliver(aMsg, KProviderKey_ExpectInput);
		return KErrNone;
		}

	// Delete modulespec
	if (aMsg.iSelector.iExt->policy_type == EPfqosModulespecPolicy)
		{
		CModuleSelector* sel = (CModuleSelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosModulespecPolicy);
		if (sel)
			{
			sel->iNext.Deque();
			delete sel;
			}
		else
			aBase.pfqos_msg_errno = KErrNotFound;

		Deliver(aMsg, KProviderKey_ExpectInput);
		return KErrNone;
		}

	return KErrNone;
	}


//
// PFQOS_GET message has to contain <header, selector, srcaddr, dstaddr>
//
TInt CProtocolQoS::ExecGet(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{

	if (aMsg.iSrcAddr.iExt == NULL || aMsg.iDstAddr.iExt == NULL || aMsg.iSelector.iExt == NULL)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}
	
	if (aMsg.iSelector.iExt->policy_type == EPfqosFlowspecPolicy)
		{
		CPolicySelector *sel = (CPolicySelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosFlowspecPolicy);
		CExtensionPolicy *ext = (CExtensionPolicy*)iPolicyMgr->ExactMatch(aMsg, EPfqosExtensionPolicy);

		if (sel)
			{
			TRAPD(err, DumpPolicyL(aMsg, aBase, aSrc, sel, ext));
			if (err != KErrNone)
				{
				aBase.pfqos_msg_errno = err;
				aSrc->Deliver(aMsg);
				}
			}
		else
			{
			aBase.pfqos_msg_errno = KErrNotFound;
			aSrc->Deliver(aMsg);
			}
		}

	if (aMsg.iSelector.iExt->policy_type == EPfqosModulespecPolicy)
		{

		CModuleSelector *sel = (CModuleSelector*)iPolicyMgr->ExactMatch(aMsg, EPfqosModulespecPolicy);
		if (sel)
			{
			DumpPolicy(aMsg, aBase, aSrc, sel);
			}
		else
			{
			aBase.pfqos_msg_errno = KErrNotFound;
			aSrc->Deliver(aMsg);
			}
		}

	if (aMsg.iSelector.iExt->policy_type == EPfqosExtensionPolicy)
		{
		CExtensionPolicy *ext = (CExtensionPolicy*)iPolicyMgr->ExactMatch(aMsg, EPfqosExtensionPolicy);
		if (ext)
			{
			DumpPolicy(aMsg, aBase, aSrc, ext);
			}
		else
			{
			aBase.pfqos_msg_errno = KErrNotFound;
			aSrc->Deliver(aMsg);
			return KErrNone;
			}
		}

	return KErrNone;
	}


// For now, send events to ALL PFQOS-sockets.
TInt CProtocolQoS::ExecReject(TPfqosMessage &aMsg, struct pfqos_msg &/*aBase*/, CQoSProvider* /*aSrc*/)
	{
	Deliver(aMsg, KProviderKey_ExpectInput);
	return KErrNone;
	}


// PFQOS_FLUSH deletes all policies (i.e. selectors are not supported yet)
TInt CProtocolQoS::ExecFlush(TPfqosMessage &aMsg, struct pfqos_msg &/*aBase*/, CQoSProvider* /*aSrc*/)
	{
	// Delete all: No selectors supported yet, deletes all selectors
	iPolicyMgr->Flush();
	Deliver(aMsg, KProviderKey_ExpectInput);
	return KErrNone;
	}


// PFQOS_DUMP writes the whole policy database to a socket.
TInt CProtocolQoS::ExecDump(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	TInt seq=1;

	for (TUint i=0; i < KPolicyTableSize; i++)
		{
		TPolicyQueIter iter(iPolicyMgr->Policies(i));
		CSelectorBase *s;

		while ((s = iter++) != NULL)
			{
			TInt err=KErrNone;
			aBase.pfqos_msg_seq = seq++;

	        //lint -e{961} Missing 'else' is OK
			if (s->iType == EPfqosFlowspecPolicy)
				{
				CPolicySelector *sel = (CPolicySelector*)s;
				TRAP(err, DumpPolicyL(aMsg, aBase, aSrc, sel));
				}
			else if (s->iType == EPfqosModulespecPolicy)
				{
				CModuleSelector *sel = (CModuleSelector*)s;
				DumpPolicy(aMsg, aBase, aSrc, sel);
				}
			else if (s->iType == EPfqosExtensionPolicy)
				{
				CExtensionPolicy *sel = (CExtensionPolicy*)s;
				DumpPolicy(aMsg, aBase, aSrc, sel);
				}

			if (err != KErrNone)
				{
				aBase.pfqos_msg_errno = KErrNoMemory;
				// Reply goes only to SAP that sent PFQOS_GET
				aSrc->Deliver(aMsg);
				}
			}
		}

	//?? missing call to RemovePolicyData? The last message is
	//?? supposed to have the header only...
	//?? Should call RemovePolicyData and clear ALL aMsg iExt ptrs, except base here!

	// End message with #seq == 0
	aBase.pfqos_msg_seq = 0;
	aBase.pfqos_msg_len = aMsg.Length64();
	aSrc->Deliver(aMsg);
	return KErrNone;
	}


// PFQOS_CONFIGURE message configures a QoS module. NOT TESTED!!
TInt CProtocolQoS::ExecConfigure(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	if (aMsg.iConfigure.iExt == NULL)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	// Find module and see if it's loaded.
	RModule* module=NULL;
	TRAPD(err, module = ModuleMgr()->OpenModuleL(aMsg.iConfigure.iExt->protocol_id));
	if (err == KErrNone && module)
		{
		TPtr8 data((TUint8 *)aMsg.iConfigure.iExt, sizeof(aMsg.iConfigure.iExt->pfqos_configure_len));
		//coverity[leave_without_push]
		aBase.pfqos_msg_errno = module->Module()->Configure(KSOLQoSModule, KSoConfigure, data);
		//coverity[leave_without_push]
		delete module;
		}
	else
		aBase.pfqos_msg_errno = EQoSNoModules;

	aSrc->Deliver(aMsg);
	return KErrNone;
	}

TInt CProtocolQoS::ExecCreateChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	// The specification of ExecCreateChannel appears to be: create a new channel
	// for the specific connected socket.
	//
	//	There is no concept of identifying the socket here, instead some ad hoc fuzzy
	//	selector logic based on destination address, protocol and ports is used, and it
	//	is assumed to match only one flow, if any [FALSE ASSUMPTION, only works for TCP
	//	and UDP in most cases, and even they can fail because you can have multiple flows
	//	with same (dst,protocol,ports) and even connected sockets, if different source
	//	address is used). [The correct solution would be unique identifier for the RSocket
	//	itself, perhaps, for example, the CSocket address, SAP address or the handle]
	//
	// This must be matched with DeleteChannel call
	if (!aMsg.iSelector.iExt || !aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt || !aMsg.iChannel.iExt || !aMsg.iFlowSpec.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	// create the new qos channel with a flow catcher

	CInternalQoSChannel* channel = NULL;
	TRAPD(err, channel = iChannelMgr->NewChannelL(aMsg, aSrc));
	if (err != KErrNone)
		{
		aBase.pfqos_msg_errno = err;
		}
	else if (channel == NULL)
		{
		// Should never get here!
		aBase.pfqos_msg_errno = KErrNoMemory;
		}
	else
		{
		aMsg.iChannel.iExt->channel_id = channel->ChannelId();
		}
	aSrc->Deliver(aMsg);
	return err;
	}

TInt CProtocolQoS::ExecOpenExistingChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	// Associate PFQOS control SAP with an existing channel. The channel is defined by a
	// a socket (flow). The flow must exist and have a channel attached.
	// This must be matched with DeleteChannel call
	if (!aMsg.iSelector.iExt || !aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt || !aMsg.iChannel.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	CInternalQoSChannel* channel = NULL;
	TRAPD(ret, channel = iChannelMgr->OpenChannelL(aMsg, aSrc));
	if (channel)
		{
		aMsg.iChannel.iExt->channel_id = channel->ChannelId();
		}
	else if (ret == KErrNone)
		{
		ret = KErrNotFound;
		}
	aBase.pfqos_msg_errno = ret;
	aSrc->Deliver(aMsg);
	return KErrNone;
	}


TInt CProtocolQoS::ExecJoin(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	// Join a socket (flow) to a channel identified by the channel id.
	// This does not require that the SAP is associated with the channel

	if (!aMsg.iSelector.iExt || !aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt || !aMsg.iChannel.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}
	aBase.pfqos_msg_errno = iChannelMgr->JoinChannel(aMsg, aMsg.iChannel.iExt->channel_id);
	aSrc->Deliver(aMsg);
	return KErrNone;
	}


TInt CProtocolQoS::ExecLeave(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	// Remove socket (flow) from a channel.
	// Operation is valid only if the flow is joined to the specified channel.
	if (!aMsg.iSelector.iExt || !aMsg.iSrcAddr.iExt || !aMsg.iDstAddr.iExt || !aMsg.iChannel.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}
	aBase.pfqos_msg_errno = iChannelMgr->LeaveChannel(aMsg, aMsg.iChannel.iExt->channel_id);
	aSrc->Deliver(aMsg);
	return KErrNone;
	}


TInt CProtocolQoS::ExecDeleteChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	// This removes the PFQOS control SAP from the channel.
	// The channel is deleted when the last SAP is removed (regardless of the flows attached)
	{
	if (!aMsg.iChannel.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	TInt channelId = aMsg.iChannel.iExt->channel_id;
	aBase.pfqos_msg_errno = iChannelMgr->DetachFromOne(aSrc, channelId);
	aSrc->Deliver(aMsg);
	return KErrNone;
	}


TInt CProtocolQoS::ExecConfigChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	// Modify QoS policy on the channel and trigger negotiation.
	{
	if (!aMsg.iFlowSpec.iExt || !aMsg.iChannel.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

	TInt channelId = aMsg.iChannel.iExt->channel_id;
	CInternalQoSChannel* channel = iChannelMgr->FindChannel(channelId);
	if (channel)
		aBase.pfqos_msg_errno = channel->SetPolicy(aMsg);
	else
		aBase.pfqos_msg_errno = KErrNotFound;

	aSrc->Deliver(aMsg);
	return KErrNone;
	}


TInt CProtocolQoS::ExecLoadFile(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	if (!aMsg.iConfigFile.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

#if _UNICODE
	TPtrC8 tmp((TUint8*)aMsg.iConfigFile.iExt->filename);
	TFileName filename;
	filename.Copy(tmp);
#else
	TPtrC8 filename((TUint8*)aMsg.iConfigFile.iExt->filename);
#endif // _UNICODE
	TRAPD(err, LoadFileL(filename));
	aBase.pfqos_msg_errno = err;
	aSrc->Deliver(aMsg);
	if (err==KErrNone)
		UpdateFlows();
	return KErrNone;
	}

TInt CProtocolQoS::ExecUnloadFile(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc)
	{
	if (!aMsg.iConfigFile.iExt)
		{
		Error(aMsg, aBase, EQoSMessageCorrupt);
		return KErrGeneral;
		}

#if _UNICODE
	TPtrC8 tmp((TUint8*)aMsg.iConfigFile.iExt->filename);
	TFileName filename;
	filename.Copy(tmp);
#else
	TPtrC8 filename((TUint8*)aMsg.iConfigFile.iExt->filename);
#endif // _UNICODE
	TInt err = UnLoadFile(filename);
	aBase.pfqos_msg_errno = err;
	aSrc->Deliver(aMsg);
	return KErrNone;
	}

TInt CProtocolQoS::Exec(TPfqosMessage &aMsg, CQoSProvider *aSrc)
	{
	if (aMsg.iBase.iMsg)
		{
		//
		// Many of the Exec functions need to modify the
		// message content before it is passed on to the
		// listeners. As the BASE part pointed by aMsg is
		// const, create a modifiable copy of it for the
		// Exec use (change aMsg to point this).
		//
		struct pfqos_msg base = *aMsg.iBase.iMsg;
		aMsg.iBase.iMsg = &base;

		switch (aMsg.iBase.iMsg->pfqos_msg_type)
			{
		case EPfqosUpdate:
			return ExecUpdate(aMsg, base, aSrc);
		case EPfqosAdd:
			return ExecAdd(aMsg, base, aSrc);
		case EPfqosDelete:
			return ExecDelete(aMsg, base, aSrc);
		case EPfqosGet:
			LOG(Log::Printf(_L("CProtocolQoS::Exec case EPfqosGet\n")));
			return ExecGet(aMsg, base, aSrc);
		case EPfqosReject:
			return ExecReject(aMsg, base, aSrc);
		case EPfqosFlush:
			return ExecFlush(aMsg, base, aSrc);
		case EPfqosDump:
			return ExecDump(aMsg, base, aSrc);
		case EPfqosConfigure:
			return ExecConfigure(aMsg, base, aSrc);
		case EPfqosJoin:
			return ExecJoin(aMsg, base, aSrc);
		case EPfqosLeave:
			return ExecLeave(aMsg, base, aSrc);
		case EPfqosCreateChannel:
			return ExecCreateChannel(aMsg, base, aSrc);
		case EPfqosOpenExistingChannel:
			return ExecOpenExistingChannel(aMsg, base, aSrc);
		case EPfqosDeleteChannel:
			return ExecDeleteChannel(aMsg, base, aSrc);
		case EPfqosConfigChannel:
			return ExecConfigChannel(aMsg, base, aSrc);
		case EPfqosLoadFile:
			return ExecLoadFile(aMsg, base, aSrc);
		case EPfqosUnloadFile:
			return ExecUnloadFile(aMsg, base, aSrc);
		case EPfqosReserved:		// RESERVED must not be used
		default:
			return KErrGeneral;		// Invalid message
			}
		}
	return KErrGeneral;
	}

