// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "pfqos_stream.h"
#include "qoslib_glob.h"
#include "qoslib.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

//lint -e{708} does not like union initializers
const TIp6Addr KInet6AddrMask = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
								0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};



//
CChannel::CChannel(CQoSMan* aManager)
	{
	iManager = aManager;
	iPending = ENone;
	iCapabilities = 0;
	iObserver = NULL;
	iChannelId = -1;
	iStatus = EInit;
	}

CChannel::~CChannel()
	{
	iManager->RemoveQoSChannel(this);
	}

CChannel* CChannel::NewL(CQoSMan* aManager, RSocket& aSocket, 
	CQoSParameters* aSpec)
	{
	CChannel* channel = new (ELeave) CChannel(aManager);
	CleanupStack::PushL(channel);
	channel->ConstructL(aSocket, aSpec);
	CleanupStack::Pop();
	return channel;
	}

void CChannel::ConstructL(RSocket& aSocket, CQoSParameters* aSpec)
	{
	if (aSpec)
		{
		iPolicy.CopyL(*aSpec);
		}

	TInt ret = iRequestSelector.SetAddr(aSocket);
	if (ret != KErrNone)
		{
		User::Leave(ret);
		}
	}

TInt CChannel::Close()
	{
	CRequest* request=NULL;
	TRAPD(err, request = CRequest::NewL(NULL, KQoSDefaultBufSize));
	if (err == KErrNone)
		{
		request->iMsg->Init(EPfqosDeleteChannel);
		request->iMsg->AddChannel(iChannelId);
		iPending = EPendingDelete;
		iManager->Send(request);
		}
	return err;
	}

void CChannel::ProcessEvent(TPfqosMessage& aMsg)
	{
	iCapabilities = aMsg.iEvent.iExt->event_value;
	switch (aMsg.iEvent.iExt->event_type)
		{
		
		case KPfqosEventFailure:
			if (iStatus == EChannelCreated)
				{
				iStatus = EInit;
				}
			if (iObserver && (iEventMask & EQoSEventFailure))
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSFailureEvent event(iPolicy, 
					aMsg.iBase.iMsg->pfqos_msg_errno);
				iObserver->Event(event);
				}
			break;
		
		case KPfqosEventConfirm:
			if (iStatus == EChannelCreated)
				{
				iStatus = EChannelReady;
				}
			if (iObserver && iEventMask & EQoSEventConfirm)
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSConfirmEvent event(iPolicy);
				iObserver->Event(event);
				}
			break;
		
		case KPfqosEventAdapt:
			if (iStatus == EChannelCreated)
				{
				iStatus = EChannelReady;
				}
			if (iObserver && iEventMask & EQoSEventAdapt)
				{
				ParseExtensions(aMsg, iPolicy);
				CQoSAdaptEvent event(iPolicy, 
					aMsg.iBase.iMsg->pfqos_msg_errno);
				iObserver->Event(event);
				}
			break;
		
		case KPfqosEventJoin:
			if (iObserver && iEventMask & EQoSEventJoin)
				{
				TInt reason = aMsg.iBase.iMsg->pfqos_msg_errno;
				TQoSSelector sel;
				CreateSelector(sel, aMsg);
				CQoSJoinEvent event(sel, reason);
				iObserver->Event(event);
				}
			break;
		
		case KPfqosEventLeave:
			if (iObserver && iEventMask & EQoSEventLeave)
				{
				TInt reason = aMsg.iBase.iMsg->pfqos_msg_errno;
				TQoSSelector sel;
				CreateSelector(sel, aMsg);
				CQoSLeaveEvent event(sel, reason);
				iObserver->Event(event);
				}
			break;
		
		default:
			return;
		}
	}

void CChannel::ProcessReply(TPfqosMessage& aMsg)
	{
	TInt aErrorCode = aMsg.iBase.iMsg->pfqos_msg_errno;

	if (aErrorCode)
		{
		NotifyError(aErrorCode);
		}
	else
		{
		switch (iPending)
			{
			case EPendingOpenExisting:
				iPending = ENone;
				iChannelId = aMsg.iChannel.iExt->channel_id;
				// mmm
				iStatus = EChannelReady;
				// mmm
				
				if (iObserver && iEventMask & EQoSEventChannel)
					{
					ParseExtensions(aMsg, iPolicy);
					CQoSChannelEvent event(&iPolicy, KErrNone);
					iObserver->Event(event);
					}
				break;
		
			case EPendingOpenExistingSetQoS:
				iChannelId = aMsg.iChannel.iExt->channel_id;
				iStatus = EChannelCreated;
				if (iObserver && iEventMask & EQoSEventChannel)
					{
					CQoSChannelEvent event(NULL, KErrNone);
					iObserver->Event(event);
					}
				TRAPD(err, iManager->SetQoSL(*this));
				//lint -e{961} does not like missing final 'else'
				if (err == KErrNone)
					{
					iPending = EPendingSetPolicy;
					}
				else if (iObserver && iEventMask & EQoSEventFailure)
					{
					CQoSFailureEvent event(iPolicy, err);
					iObserver->Event(event);
					}
				break;
		
			case EPendingOpen:
				{
				_LIT(KText1, "CChannel::ProcessReply");
				__ASSERT_ALWAYS((aMsg.iChannel.iExt != NULL), 
					User::Panic(KText1, 0));
				iPending = ENone;
				iChannelId = aMsg.iChannel.iExt->channel_id;
				iStatus = EChannelCreated;
				}
				break;
		
			case EPendingDelete:
				delete this;
				break;
		
			case EPendingSetPolicy:
			case EPendingJoin:
			case EPendingLeave:
			default:
				iPending = ENone;
				break;
			}
		}
	}


TBool CChannel::Match(TInt aChannelId)
	{
	if (aChannelId >= 0 && aChannelId == iChannelId)
		{
		return ETrue;
		}
	return EFalse;
	}


TBool CChannel::MatchReply(const TPfqosMessage& aMsg, TUint8 aMsgType)
	{
	//lint -e{961} does not like missing final 'else'
	if (((iPending == EPendingOpenExisting && 
		  aMsgType == EPfqosOpenExistingChannel) ||
		 (iPending == EPendingOpenExistingSetQoS && 
		  aMsgType == EPfqosOpenExistingChannel) ||
		 (iPending == EPendingOpen && aMsgType == EPfqosCreateChannel) ||
		 (iPending == EPendingJoin && aMsgType == EPfqosJoin) || 
		 (iPending == EPendingLeave && aMsgType == EPfqosLeave)) && 
		 (iRequestSelector.GetDst().Match(*aMsg.iDstAddr.iAddr)) &&
		 (iRequestSelector.GetSrc().Match(*aMsg.iSrcAddr.iAddr)) &&
		 (iRequestSelector.Protocol() == aMsg.iSelector.iExt->protocol) &&
		 (iRequestSelector.GetDst().Port() == aMsg.iDstAddr.iAddr->Port()) &&
		 (iRequestSelector.GetSrc().Port() == aMsg.iSrcAddr.iAddr->Port()) &&
		 (iRequestSelector.MaxPortDst() == 
		  aMsg.iDstAddr.iExt->pfqos_port_max) &&
		 (iRequestSelector.MaxPortSrc() == 
		  aMsg.iSrcAddr.iExt->pfqos_port_max))
		 {
		return ETrue;
		}
	else if (((iPending == EPendingDelete && 
			   aMsgType == EPfqosDeleteChannel) ||
			  (iPending == EPendingSetPolicy && 
			   aMsgType == EPfqosConfigChannel))// ||
		   && (iChannelId == aMsg.iChannel.iExt->channel_id))
		{
		return ETrue;
		}

	return EFalse;
	}

TInt CChannel::OpenExisting()
	{
	TRAPD(err, iManager->OpenExistingL(*this, iRequestSelector));
	if (err == KErrNone)
		{
		iPending = EPendingOpenExisting;
		}
	return err;
	}

TInt CChannel::SetQoS(CQoSParameters& aPolicy)
	{
	if (iPending)
		{
		if (iPending == EPendingOpenExisting)
			{
			TRAPD(err, iPolicy.CopyL(aPolicy));
			if (err == KErrNone)
				{
				iPending = EPendingOpenExistingSetQoS;
				}
			return err;
			}
		else
			{
			return KErrInUse;
			}
		}
	TRAPD(err, iPolicy.CopyL(aPolicy));
	if (err != KErrNone)
		{
		return err;
		}
	if (iStatus == EInit)
		{
		TRAP(err, iManager->CreateL(*this, iRequestSelector));
		if (err == KErrNone)
			{
			iPending = EPendingOpen;
			}
		}
	else
		{
		TRAP(err, iManager->SetQoSL(*this));
		if (err == KErrNone)
			{
			iPending = EPendingSetPolicy;
			}
		}
	return err;
	}

TInt CChannel::Join(RSocket& aSocket)
	{
	if (iStatus != EChannelReady)
		{
		return KErrNotReady;
		}
	if (iPending != ENone)
		{
		return KErrInUse;
		}
	TInt err = iRequestSelector.SetAddr(aSocket);
	if (err != KErrNone)
		{
		return err;
		}
	TRAP(err, iManager->JoinL(*this, iRequestSelector));
	if (err == KErrNone)
		{
		iPending = EPendingJoin;
		}
	return err;
	}

TInt CChannel::Leave(RSocket& aSocket)
	{
	if (iStatus != EChannelReady)
		{
		return KErrNotReady;
		}
	if (iPending != ENone)
		{
		return KErrInUse;
		}
	TInt err = iRequestSelector.SetAddr(aSocket);
	if (err != KErrNone)
		{
		return err;
		}
	TRAP(err, iManager->LeaveL(*this, iRequestSelector));
	if (err == KErrNone)
		{
		iPending = EPendingLeave;
		}
	return err;
	}

TInt CChannel::GetCapabilities(TUint& aCapabilities)
	{
	aCapabilities = iCapabilities;
	return KErrNone;
	}

void CChannel::NotifyError(TInt aReason)
	{
	TPendingStatus status = iPending;
	iPending = ENone;

	if (aReason)
		{
		switch (status)
			{
			case EPendingSetPolicy:
				if (iObserver && iEventMask & EQoSEventFailure)
					{
					CQoSFailureEvent event(iPolicy, aReason);
					iObserver->Event(event);
					}
				break;
		
			case EPendingOpen:
				if (iObserver && iEventMask & EQoSEventFailure)
					{
					CQoSFailureEvent event(iPolicy, aReason);
					iObserver->Event(event);
					}
				break;
		
			case EPendingJoin:
				if (iObserver && iEventMask & EQoSEventJoin)
					{
					CQoSJoinEvent event(iRequestSelector, aReason);
					iObserver->Event(event);
					}
				break;
		
			case EPendingLeave:
				if (iObserver && iEventMask & EQoSEventLeave)
					{
					CQoSLeaveEvent event(iRequestSelector, aReason);
					iObserver->Event(event);
					}
				break;
		
			case EPendingOpenExisting:
				if (iObserver && iEventMask & EQoSEventChannel)
					{
					CQoSChannelEvent event(NULL, aReason);
					iObserver->Event(event);
					}
				break;
		
			case EPendingOpenExistingSetQoS:
				TRAPD(err, iManager->CreateL(*this, iRequestSelector));
				//lint -e{961} does not like missing final 'else'
				if (err == KErrNone)
					{
					iPending = EPendingOpen;
					}
				else if (iObserver && iEventMask & EQoSEventFailure)
					{
					CQoSFailureEvent event(iPolicy, err);
					iObserver->Event(event);
					}
				break;
		
			default:
				break;
			}
		}
	}


void CChannel::CreateSelector(TQoSSelector& aSelector, 
	const TPfqosMessage& aMsg)
	{
	TInetAddr src;
	TInetAddr srcmask;
	
	src = *aMsg.iSrcAddr.iAddr;
	src.SetFamily(KAFUnspec);
	src.SetAddress(KInet6AddrNone);
	srcmask.SetAddress(KInet6AddrMask);
	aSelector.SetAddr(src, 
			  srcmask, 
			  *aMsg.iDstAddr.iAddr, 
			  *aMsg.iDstAddr.iPrefix, 
			  aMsg.iSelector.iExt->protocol, 
			  aMsg.iSrcAddr.iExt->pfqos_port_max,
			  aMsg.iDstAddr.iExt->pfqos_port_max);
	aSelector.SetIapId(aMsg.iSelector.iExt->iap_id);
	}

