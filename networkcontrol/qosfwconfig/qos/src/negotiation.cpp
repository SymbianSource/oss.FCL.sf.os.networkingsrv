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

#include "qos_prot.h"
#include "qoserr.h"
#include "policy_sap.h"
#include "interface.h"
#include "qos_channel.h"
#include "modules.h"
#include "flowhook.h"
#include "negotiation.h"

// CQoSSessionBase
CQoSSessionBase::CQoSSessionBase(CFlowHook& aHook, TInt aChannelId) 
	: iHook(aHook), iModuleList(aHook.ModuleList()), iChannelId(aChannelId)
	{
	iPending.SetOffset(_FOFF(CNegotiateItem, iLink));
	iFatalError = EFalse;
	iError = KErrNone;
	iNegotiated = aHook.QoSParameters();
	}

CQoSSessionBase::~CQoSSessionBase()
	{
	while (!iPending.IsEmpty())
		{
		CNegotiateItem* request = iPending.First();
		iPending.Remove(*request);
		// Because there is no way to cancel a pending CNegotiateItem,
		// it *CANNOT* be deleted here. Make it a ZOMBIE by removing
		// the session pointer and hope that RequestComplete arrives
		// someday!
		request->Kill();
		}
	iPending.Reset();
	}

void CQoSSessionBase::Run()
	/**
	* Start the session.
	*/
	{
	iCurrent = 0;
	if (iModuleList.Count() == 0)
		{
		// There are no modules present, the negotiation would
		// not do or achieve anything. It is somewhat unclear
		// whether this is an error or not. Treat it as an error
		// and pick the associated error code from the interface
		// (if there is no TrafficControlStatus error on interface
		// then this just completes the request without error)
		FatalError(iHook.Interface() ? iHook.Interface()->TrafficControlStatus() : EQoSNoModules);
		}
	Proceed();
	}

// Proceed negotiation
void CQoSSessionBase::Proceed()
	{
	if (iProceed)
		{
		// Getting here from DoCall, do nothing here
		// Proceeding after DoCall returns! DoCall
		// completed the subrequest immediate!
		return;
		}

	while (!iFatalError && iCurrent < iModuleList.Count())
		{
		RModule* module = iModuleList[iCurrent];
		ASSERT(module);

		const TUint flags = module->Flags();
		if ((flags & KQoSModuleSerialize) && !iPending.IsEmpty())
			{
			// The module requires previous requests to be completed.
			// There is at least one pending request. When it completes,
			// the Proceed() gets called to restart the loop.
			return;
			}
		++iCurrent;

		CNegotiateItem*const item = new CNegotiateItem(this, flags);
		if (item == NULL)
			{
			FatalError(KErrNoMemory);
			break;
			}
		iPending.AddLast(*item);

		LOG(Log::Printf(_L("")));
		LOG(Log::Printf(_L("calling\tmodule[%S] for session[%u]"), &module->Name(), (TInt)this));
		++iProceed;	// ..in case DoCall calls (Sub)RequestComplete!
		const TInt ret = DoCall(*module, *item);
		--iProceed;
		LOG(Log::Printf(_L("<<<\treturns from module[%S] for session[%u]"), &module->Name(), (TInt)this));
		if (ret != KErrNone)
			{
			// DoCall didn't call the module, there will be no (Sub)RequestCompete call!
			// Destroy the item here!
			iPending.Remove(*item);
			delete item;
			FatalError(ret);
			break;
			}
		}

	if (iPending.IsEmpty() || iFatalError)
		{
		RequestComplete();
		//?? In case of iFatalError, should we start a "shutdown" process
		//?? session for those modules that actually succeeded (at least
		//?? in case of some session, like if some Joins succeed, shouldn't
		//?? there be a Leave for those, if the whole session cannot be
		//?? completed? [apparenty there is never more than one module, so
		//?? the issue has not come up yet...]
		delete this;
		}
	}


CInternalQoSChannel* CQoSSessionBase::Channel() const
	/**
	* Return internal channel object or NULL.
	*/

	{
	// The sessions only store the channel id, and when needed, the
	// channel object is located by this. This takes care of the
	// possibility that the channel disappears while sessions is
	// running (if a pointer was stored, the channel object destructor
	// would have to find all sessions referencing it, and currently
	// this would be somewhat complicated (as flows holding the session
	// might not be members of that channel any more).
	if (iChannelId > 0)
		return iHook.Protocol().ChannelMgr()->FindChannel(iChannelId);
	else
		return NULL;
	}

void CQoSSessionBase::FatalError(TInt aErrorCode)
	{
	iFatalError = ETrue;
	iError = aErrorCode;
	}

void CQoSSessionBase::SubRequestComplete(TInt aErrorCode, 
	const TQoSParameters* aParams, const TExtensionData& aExtension, 
	CNegotiateItem* aItem)
	{
	LOG(Log::Printf(_L("<>\tqos session[%u] NegotiateItem[%u] SubRequestComplete"), (TInt)this, (TInt)aItem));

	iPending.Remove(*aItem);

	if (aParams)
		{
		iNegotiated = *aParams;
		}

	// add possible extensions to PF_QOS event
	if (aExtension.iData.Length() > 0)
		{
		TRAPD(err, iMsg.AddExtensionL(aExtension.iData, aExtension.iType));
		if (err != KErrNone)
			{
			LOG(Log::Printf(_L("iMsg.AddExtensionL error: %d"), err));
			}
		}

	if (aErrorCode == KErrNone)
		{
		//lint -e{961} Missing 'else' is OK
		if (KQoSModuleSignaling & aItem->Flags())
			{
			iValue |= KQoSModuleSignaling;
			}
		else if (KQoSModulePartialSignaling & aItem->Flags())
			{
			iValue |= KQoSModulePartialSignaling;
			}
		else if (KQoSModuleProvisioning & aItem->Flags())
			{
			iValue |= KQoSModuleProvisioning;
			}
		}
	else
		{
		if (aItem->Flags() & KQoSFatalFailure
		||  aErrorCode == EQoSLeaveFailure)
		    {
			FatalError(aErrorCode);
			}
		}	
	Proceed();
	}

void CQoSSessionBase::DeliverEvent(TUint16 aEvent)
	{
	// If the caller does not set event, choose one by iFatalError
	if (aEvent == 0)
		{
		//?? Event seems to be a bitmask. Should this actually be done
		//?? always (and with OR to the aEvent)?
		aEvent = iFatalError ? KPfqosEventFailure : KPfqosEventConfirm;
		}

	T_pfqos_msg base(EPfqosEvent);
	if (iError != KErrNone)
		{
		base.pfqos_msg_errno = iError;
		}
	iMsg.iBase.iMsg = &base;

	T_pfqos_event event(EPfqosExtEvent, aEvent, iValue);
	iMsg.iEvent.iExt = &event;

	T_pfqos_flowspec spec(iNegotiated);
	iMsg.iFlowSpec.iExt = &spec;

	// Add flow identifying PF_QOS blocks
	// note: the variables must be outside the if-block, because they must exist
	// when Deliver is called!
	TInetAddr msk;
	pfqos_address src, dst;
	pfqos_selector sel;
	iHook.FillFlowInfo(iMsg, src, dst, sel, msk);


	// Add channel identifying PF_QOS block (only present if requested)
	T_pfqos_channel channel(iChannelId);
	if (iChannelId > 0)
		iMsg.iChannel.iExt = &channel;

	iMsg.iNumModules = 0;
	iHook.Protocol().Deliver(iMsg, KProviderKey_RegisteredQoSConf);
	}


// Negotiate
CNegotiateSession::CNegotiateSession(CFlowHook& aHook) 	: CQoSSessionBase(aHook, 0)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] Negotiate for HOOK[%u] size=%d"), (TInt)this, (TInt)&aHook, sizeof(*this)));
	}

CNegotiateSession::~CNegotiateSession()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] Negotiate deleted"), (TInt)this));
	}

TInt CNegotiateSession::DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback)
	{
	LOG(Log::Printf(_L("\t\tmodule[%S]::Negotiate(...)"), &aModule.Name()));
	aModule.Module()->Negotiate(iHook.Context(), iHook.QoSParameters(), aCallback);
	return KErrNone;
	}

void CNegotiateSession::RequestComplete()
	{
	DeliverEvent(0);
	iHook.ClearPendingRequest(iError);
	}


// Create channel session
CCreateChannelSession::CCreateChannelSession(CFlowHook& aHook, TInt aChannelId) : CQoSSessionBase(aHook, aChannelId)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] for HOOK[%u] CreateChannel size=%d"), (TInt)this, (TInt)&aHook, sizeof(*this)));
	}

CCreateChannelSession::~CCreateChannelSession()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] CreateChannel deleted"), (TInt)this));
	}

TInt CCreateChannelSession::DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback)
	{
	LOG(Log::Printf(_L("\t\tmodule[%S]::OpenChannel(channel[%u],..., FLOW[%u])"),
		&aModule.Name(), iChannelId, (TInt)&iHook.Context()));
	CInternalQoSChannel*const channel = iHook.Protocol().ChannelMgr()->FindChannel(iChannelId);
	if (channel == NULL)
		{
		return EQoSChannelDeleted;
		}
	aModule.Module()->OpenChannel(iChannelId, channel->QoSParameters(), channel->Extension(), aCallback, iHook.Context());
	return KErrNone;
	}

void CCreateChannelSession::RequestComplete()
	{
	LOG(Log::Printf(_L("\tqos session[%u] CreateChannel complete"), (TInt)this));
	DeliverEvent(0);
	iHook.SetChannelJoined(TRUE);
	iHook.ClearPendingRequest(iError);
	CInternalQoSChannel*const channel = iHook.Protocol().ChannelMgr()->FindChannel(iChannelId);
	if (channel)
		channel->RequestComplete(iError);
	}



// Negotiate channel session
CNegotiateChannelSession::CNegotiateChannelSession(CFlowHook& aHook, TInt aChannelId) : CQoSSessionBase(aHook, aChannelId)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] for HOOK[%u] NegotiateChannel size=%d"), (TInt)this, (TInt)&aHook, sizeof(*this)));
	}

CNegotiateChannelSession::~CNegotiateChannelSession()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] NegotiateChannel deleted"), (TInt)this));
	}

TInt CNegotiateChannelSession::DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback)
	{
	LOG(Log::Printf(_L("\t\tmodule[%S]::NegotiateChannel(channel[%u], ...)"), &aModule.Name(), iChannelId));
	CInternalQoSChannel*const channel = Channel();
	if (channel == NULL)
		{
		return EQoSChannelDeleted;
		}
	aModule.Module()->NegotiateChannel(iChannelId, channel->QoSParameters(), channel->Extension(), aCallback);
	return KErrNone;
	}

void CNegotiateChannelSession::RequestComplete()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] NegotiateChannel complete"), (TInt)this));
	DeliverEvent(0);
	iHook.ClearPendingRequest(iError);

	CInternalQoSChannel*const channel = Channel();
	if (channel)
		channel->RequestComplete(iError);
	}

// Join
CJoinSession::CJoinSession(CFlowHook& aHook, TInt aChannelId) : CQoSSessionBase(aHook, aChannelId)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] for HOOK[%u] JoinChannel size=%d"), (TInt)this, (TInt)&aHook, sizeof(*this)));
	}

CJoinSession::~CJoinSession()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] JoinChannel deleted"), (TInt)this));
	}

TInt CJoinSession::DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback)
	{
	LOG(Log::Printf(_L("\t\tmodule[%S]::Join(channel[%u], FLOW[%u])"), &aModule.Name(), iChannelId, (TInt)&iHook.Context()));
	const CInternalQoSChannel*const channel = Channel();
	if (channel == NULL)
		{
		return EQoSChannelDeleted;
		}
	aModule.Module()->Join(iChannelId, iHook.Context(), aCallback);
	return KErrNone;
	}


void CJoinSession::RequestComplete()
	{
	LOG(Log::Printf(_L("\tqos session[%u] JoinChannel complete"), (TInt)this));
	DeliverEvent(KPfqosEventJoin);
	iHook.SetChannelJoined(TRUE);
	iHook.ClearPendingRequest(iError);

	CInternalQoSChannel*const channel = Channel();
	if (channel)
		channel->RequestComplete(iError);
	}



// Leave
CLeaveSession::CLeaveSession(CFlowHook& aHook, TInt aChannelId) : CQoSSessionBase(aHook, aChannelId)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] for HOOK[%u] LeaveChannel size=%d"), (TInt)this, (TInt)&aHook, sizeof(*this)));
	}

CLeaveSession::~CLeaveSession()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] LeaveChannel deleted"), (TInt)this));
	}

TInt CLeaveSession::DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback)
	{
	LOG(Log::Printf(_L("\t\tmodule[%S]::Leave(channel[%u], FLOW[%u])"), &aModule.Name(), iChannelId, (TInt)&iHook.Context()));
	aModule.Module()->Leave(iChannelId, iHook.Context(), aCallback);
	return KErrNone;
	}

void CLeaveSession::RequestComplete()
	{
	LOG(Log::Printf(_L("\tqos session[%u] LeaveChannel complete"), (TInt)this));
	DeliverEvent(KPfqosEventLeave);
	// Somewhat icky here. RestartQoS would destroy the session, and
	// to prevent that, ClearPendingRequest must be called first. However
	// it also set the parameter as flow status. Use PENDING so that we
	// don't get accidental and useless ReadyL sequence triggered here
	// (because it will be triggered anyway via RestartQoS).
	iHook.ClearPendingRequest(EFlow_PENDING);
	if (!iFatalError)
	    {
	    iHook.RestartQoS();	// Need find out new QoS without channel.
	    }
	}

//
CNegotiateItem::CNegotiateItem(CQoSSessionBase* aSession, TUint aFlags) : iSession(aSession)
	{
	LOG(Log::Printf(_L("new\tqos session[%u] NegotiateItem[%u] size=%d"), (TInt)iSession, (TInt)this, sizeof(*this)));
	iFlags = aFlags;
	}

void CNegotiateItem::Kill()
	{
	// Going negotiation cannot be stopped (there is no call for it), just
	// make this a zombie without a session and hope that RequestComplete
	// happens (if not, then there is a memory leak!)
	iSession = NULL;
	LOG(Log::Printf(_L("\tqos session[%u] NegotiateItem[%u] is now a ZOMBIE!"), (TInt)iSession, (TInt)this));
	}

CNegotiateItem::~CNegotiateItem()
	{
	LOG(Log::Printf(_L("~\tqos session[%u] NegotiateItem[%u] deleted"), (TInt)iSession, (TInt)this));
	}

void CNegotiateItem::RequestComplete(TInt aErrorCode, 
	const TQoSParameters* aParams, const TExtensionData& aExtension)
	{
	
	if (iSession)
		{
		// I'm ALIVE! Not a ZOMBIE!	
		iSession->SubRequestComplete(aErrorCode, aParams, aExtension, this);
		}
	delete this;
	}
