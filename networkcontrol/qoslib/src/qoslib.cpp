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

#include <f32file.h>
#include <es_ini.h>


#include "qosliblog.h"

#include "qoslib.h"
#include "pfqosparser.h"
#include "pfqos.h"
#include "qos_ini.h"
#include "qoslib_glob.h"
#include "pfqos_stream.h"


//
// DLL entry point
//
GLDEF_C TInt E32Dll()
	{
	return(KErrNone);
	}

//
CRequest* CRequest::NewL(CQoSRequestBase* aOwner, TUint aBufSize)
	{
	CRequest* request = new (ELeave) CRequest(aOwner);
	CleanupStack::PushL(request);
	request->ConstructL(aBufSize);
	CleanupStack::Pop();
	return request;
	}

CRequest::~CRequest()
	{
	delete iMsg;
	}

CRequest::CRequest(CQoSRequestBase* aOwner) : iOwner(aOwner)
	{
	iMsg = NULL;
	}

void CRequest::ConstructL(TUint aBufSize)
	{
	iMsg = CPfqosStream::NewL(aBufSize);
	};


//
class CSender : public CActive
	{
	public:
	static CSender* NewL(CQoSMan* aManager, TInt aPriority=EPriorityStandard);
	~CSender();
	void Send(CRequest* aRequest);
	void ClearPendingRequest(CQoSRequestBase* aRequest);

	protected:
	CSender(CQoSMan* aManager, TInt aPriority);
	void ConstructL();
	void RunL();
	void DoCancel();

	private:
	CQoSMan* iManager;
	CRequest* iCurrentRequest;
	TSglQue<CRequest> iRequests;
	};

CSender* CSender::NewL(CQoSMan* aManager, TInt aPriority)
	{
	CSender* sender = new (ELeave) CSender(aManager, aPriority);
	CleanupStack::PushL(sender);
	sender->ConstructL();
	CleanupStack::Pop();
	return sender;
	}

CSender::CSender(CQoSMan* aManager, TInt aPriority) : CActive(aPriority), iManager(aManager)
	{
	CActiveScheduler::Add(this);
	iRequests.SetOffset(_FOFF(CRequest, iLink));
	iCurrentRequest=NULL;
	}

void CSender::ConstructL()
	{
	}

CSender::~CSender()
	{
	if (IsActive())
		Cancel();

	delete iCurrentRequest;

	while (!iRequests.IsEmpty())
		{
		CRequest* request = iRequests.First();
		iRequests.Remove(*request);
		delete request;
		}
	iRequests.Reset();
	}

void CSender::Send(CRequest* aRequest)
	{
	if (IsActive())
		iRequests.AddLast(*aRequest);
	else
		{
		iCurrentRequest = aRequest;
		iCurrentRequest->iMsg->Send(iManager->Socket(), iStatus);
		SetActive();
		}
	}

void CSender::ClearPendingRequest(CQoSRequestBase* aRequest)
	{
	if (iCurrentRequest != NULL)
		{
		if (iCurrentRequest->iOwner == aRequest)
			iCurrentRequest->iOwner = NULL;
		}
	}

void CSender::RunL()
	{
	TInt err = iStatus.Int();
	if (err && iCurrentRequest->iOwner)
		iCurrentRequest->iOwner->NotifyError(err);

	delete iCurrentRequest;
	iCurrentRequest = NULL;

	if (!iRequests.IsEmpty())
		{
		CRequest* request = iRequests.First();
		iRequests.Remove(*request);
		iCurrentRequest = request;
		iCurrentRequest->iMsg->Send(iManager->Socket(), iStatus);
		SetActive();
		}
	}

void CSender::DoCancel()
	{
	iManager->Socket().CancelWrite();
	}


//
CQoSMan* CQoSMan::NewL(TInt aPriority)
	{
	CQoSMan* manager;
	manager = QoSManGlobals::Get();
	if (!manager)
		{
		manager = new (ELeave) CQoSMan(aPriority);
		CleanupStack::PushL(manager);
		manager->ConstructL();
		CleanupStack::Pop();
		QoSManGlobals::Set(manager);
		}
	manager->Open();
	return manager;
	}

void CQoSMan::ConstructL()
	{
	_LIT(KDescPfqos, "pfqos");
	User::LeaveIfError(iSocketServer.Connect());
	User::LeaveIfError(iSocket.Open(iSocketServer, KDescPfqos));

	iSender = CSender::NewL(this);
	iBuf = HBufC8::NewL(KQoSDefaultBufSize);
	TPtr8 tmp(iBuf->Des());
	iRecBuf.Set(tmp);
	iUid.Set(RProcess().Type());
	iSocket.Recv(iRecBuf, 0, iStatus);
	SetActive();
	}

CQoSMan::CQoSMan(TInt aPriority) : CActive(aPriority), iRecBuf(0,0)
	{
	CActiveScheduler::Add(this);
	iRefCount = 0;
	iChannels.SetOffset(_FOFF(CChannel, iNext));
	iStaticPolicies.SetOffset(_FOFF(CPolicy,iNext));
	iNotifyPending = EFalse;
	iShutdown = EFalse;
	}

void CQoSMan::Close()
	{
	if (--iRefCount <= 0)
		{
		if (iNotifyPending)
			iShutdown = ETrue;
		else
			delete this;
		}
	}

CQoSMan::~CQoSMan()
	{
	while (!iChannels.IsEmpty())
		{
		CChannel* channel = iChannels.First();
		iChannels.Remove(*channel);
		delete channel;
		}
	iChannels.Reset();

	while (!iStaticPolicies.IsEmpty())
		{
		CPolicy* policy = iStaticPolicies.First();
		iStaticPolicies.Remove(*policy);
		delete policy;
		}
	iStaticPolicies.Reset();

	delete iSender;

	if (IsActive())
		Cancel();

	iSocket.Close();
	iSocketServer.Close();
	delete iBuf;
	QoSManGlobals::Set(NULL);
	}

CChannel* CQoSMan::OpenQoSChannelL(RSocket& aSocket)
	{
	CChannel* channel = CChannel::NewL(this, aSocket, NULL);
	iChannels.AddLast(*channel);
	return channel;
	}

void CQoSMan::RemoveQoSChannel(CChannel* aChannel)
	{
	TSglQueIter<CChannel> iter(iChannels);
	CChannel* channel;
	while ((channel = iter++) != NULL)
		{
		if (channel == aChannel)
			{
			iChannels.Remove(*aChannel);
			break;
			}
		}
	}

void CQoSMan::SetQoSL(CChannel& aChannel)
	{
	CRequest* request = CRequest::NewL(&aChannel, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosConfigChannel);
	request->iMsg->AddQoSParameters(aChannel.GetPolicy().iQoS);
	request->iMsg->AddChannel(aChannel.ChannelId());

	TQoSExtensionQueueIter iter(aChannel.GetPolicy().Extensions());
	CExtensionBase *extension;
	while ((extension=iter++) != NULL)
		request->iMsg->AddExtensionPolicy(extension->Data());
	iSender->Send(request);
	}

void CQoSMan::CreateL(CChannel& aChannel, const TQoSSelector& aSelector)
	{
	CRequest* request = CRequest::NewL(&aChannel, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosCreateChannel);
	request->iMsg->AddSelector((TUint8)aSelector.Protocol(), iUid.UidType(), EPfqosFlowspecPolicy, aSelector.IapId(), EPfqosApplicationPriority, TPtr(0,0));
	request->iMsg->AddSrcAddress(aSelector.GetSrc(), aSelector.GetSrcMask(), (TUint16)aSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(aSelector.GetDst(), aSelector.GetDstMask(), (TUint16)aSelector.MaxPortDst()); 
	request->iMsg->AddChannel(0);
	request->iMsg->AddQoSParameters(aChannel.GetPolicy().iQoS);
	TQoSExtensionQueueIter iter(aChannel.GetPolicy().Extensions());
	CExtensionBase *extension;
	while ((extension=iter++) != NULL)
		request->iMsg->AddExtensionPolicy(extension->Data());
	Send(request);
	}

void CQoSMan::OpenExistingL(CChannel& aChannel, const TQoSSelector& aSelector)
	{
	CRequest* request = CRequest::NewL(&aChannel, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosOpenExistingChannel);
	request->iMsg->AddSelector((TUint8)aSelector.Protocol(), iUid.UidType(), EPfqosFlowspecPolicy, aSelector.IapId(), EPfqosApplicationPriority, TPtr(0,0));
	request->iMsg->AddSrcAddress(aSelector.GetSrc(), aSelector.GetSrcMask(), (TUint16)aSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(aSelector.GetDst(), aSelector.GetDstMask(), (TUint16)aSelector.MaxPortDst()); 
	request->iMsg->AddChannel(0);
	Send(request);
	}

void CQoSMan::JoinL(CChannel& aChannel, const TQoSSelector& aSelector)
	{
	CRequest* request = CRequest::NewL(&aChannel, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosJoin);
	request->iMsg->AddSelector((TUint8)aSelector.Protocol(), iUid.UidType(), EPfqosFlowspecPolicy, aSelector.IapId(), EPfqosApplicationPriority, TPtr(0,0));
	request->iMsg->AddSrcAddress(aSelector.GetSrc(), aSelector.GetSrcMask(), (TUint16)aSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(aSelector.GetDst(), aSelector.GetDstMask(), (TUint16)aSelector.MaxPortDst()); 
	request->iMsg->AddChannel(aChannel.ChannelId());
	Send(request);
	}


void CQoSMan::LeaveL(CChannel& aChannel, const TQoSSelector& aSelector)
	{
	CRequest* request = CRequest::NewL(&aChannel, KQoSDefaultBufSize);
	request->iMsg->Init(EPfqosLeave);
	request->iMsg->AddSelector((TUint8)aSelector.Protocol(), iUid.UidType(), EPfqosFlowspecPolicy, aSelector.IapId(), EPfqosApplicationPriority, TPtr(0,0));
	request->iMsg->AddSrcAddress(aSelector.GetSrc(), aSelector.GetSrcMask(), (TUint16)aSelector.MaxPortSrc()); 
	request->iMsg->AddDstAddress(aSelector.GetDst(), aSelector.GetDstMask(), (TUint16)aSelector.MaxPortDst()); 
	request->iMsg->AddChannel(aChannel.ChannelId());
	Send(request);
	}


//
// Notify informs applications about events received from the kernel. 
// A callback function is called, if application has registered for the event.
// Selector is matched against flows to find out if an application ought to be notified.
//
void CQoSMan::Notify(TPfqosMessage& aMsg)
	{
	if (!aMsg.iBase.iMsg)
		return;

	switch (aMsg.iBase.iMsg->pfqos_msg_type)
		{

	case EPfqosEvent:
		ExecEvent(aMsg);
		return;

	case EPfqosFlush:
		// All policies have been deleted from the kernel. 
		// Inform applications with POLICY_EVENT_QOS_FAILURE.
		Flush();
		break;

	case EPfqosUpdate:
	case EPfqosDelete:
	case EPfqosAdd:
	case EPfqosGet:
	case EPfqosReject:
	case EPfqosDump:
	case EPfqosConfigure:
	case EPfqosJoin:
	case EPfqosLeave:
	case EPfqosCreateChannel:
	case EPfqosOpenExistingChannel:
	case EPfqosDeleteChannel:
	case EPfqosConfigChannel:
	case EPfqosLoadFile:
	case EPfqosUnloadFile:
		ExecReply(aMsg);
		break;
	default:
		break;
		}
	}

// Flush deletes all flowinfo in CQoSMan. If aEvent > 0, inform applications.
void CQoSMan::Flush()
	{
	while (!iStaticPolicies.IsEmpty())
		{
		CPolicy* policy = iStaticPolicies.First();
		if (policy)
			{
			iStaticPolicies.Remove(*policy);
			delete policy;
			}
		}
	iStaticPolicies.Reset();
	}


CChannel* CQoSMan::Match(TPfqosMessage& aMsg)
	{
	if (!aMsg.iChannel.iExt)
		return NULL;

	TSglQueIter<CChannel> iter(iChannels);
	CChannel* channel;
	while ((channel = iter++) != NULL)
		{
		if (channel->Match(aMsg.iChannel.iExt->channel_id))
			return channel;
		}
	return NULL;
	}


CChannel* CQoSMan::MatchChannelReply(TPfqosMessage& aMsg, TUint8 aMsgType)
	{
	if (aMsgType == EPfqosCreateChannel || aMsgType == EPfqosJoin || 
	aMsgType == EPfqosLeave)
	if (aMsg.iSrcAddr.iExt == NULL || aMsg.iDstAddr.iExt == NULL || aMsg.iSelector.iExt == NULL)
		return NULL;

	TSglQueIter<CChannel> iter(iChannels);
	CChannel* channel;

	while ((channel = iter++) != NULL)
		{
		if (channel->MatchReply(aMsg, aMsgType))
			return channel;
		}
	return NULL;
	}

CPolicy* CQoSMan::MatchPolicyReply(TPfqosMessage& aMsg, TUint8 aMsgType)
	{
	TSglQueIter<CPolicy> iter(iStaticPolicies);
	CPolicy* policy;

	while ((policy = iter++) != NULL)
		{
		if (policy->MatchReply(aMsg, aMsgType))
			return policy;
		}
	return NULL;
	}

//
// POLICY_EVENT message has to contain <header, selector, srcaddr, dstaddr, policy_event,(flowspec)>
// flowspec is mandatory for all others but POLICY_EVENT_QOS_FAILURE event.
//
void CQoSMan::ExecEvent(TPfqosMessage& aMsg)
	{
	if (aMsg.iEvent.iExt == NULL || aMsg.iFlowSpec.iExt == NULL)
		return;

	TUint event = aMsg.iEvent.iExt->event_type;
	switch (event)
		{
	case KPfqosEventReceivers:
	case KPfqosEventSenders:
		break;

	default:
		if (aMsg.iChannel.iExt)
			{
			TSglQueIter<CChannel> iter(iChannels);
			CChannel* channel;
			while ((channel = iter++) != NULL)
				{
				if (channel->Match(aMsg.iChannel.iExt->channel_id))
					channel->ProcessEvent(aMsg);
				}
			}
		else
			{
			if (aMsg.iSrcAddr.iExt == NULL || aMsg.iDstAddr.iExt == NULL || aMsg.iSelector.iExt == NULL)
				return;
			TSglQueIter<CPolicy> iter(iStaticPolicies);
			CPolicy* policy;
			while ((policy = iter++) != NULL)
				policy->ProcessEvent(aMsg);
			}
		break;
		}
	}


void CQoSMan::ExecReply(TPfqosMessage& aMsg)
	{
	TUint8 type = aMsg.iBase.iMsg->pfqos_msg_type;
	CChannel* request = MatchChannelReply(aMsg, type);
	if (request)
		request->ProcessReply(aMsg);
	else
		{
		if (aMsg.iSrcAddr.iExt == NULL || aMsg.iDstAddr.iExt == NULL || aMsg.iSelector.iExt == NULL)
			return;
		CPolicy* policy = MatchPolicyReply(aMsg, type);
		if (policy)
			policy->ProcessReply(aMsg);
		}
	}

void CQoSMan::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		TPfqosMessage msg(iRecBuf);
		__ASSERT_DEBUG(msg.iError == KErrNone, User::Panic(_L("CQoSMan*::RunL(): Msg corrupted"),msg.iError));
		// Should log the error
		if (msg.iError == KErrNone)
			{
			iNotifyPending = ETrue;

			LOG(Log::Printf(_L("CQoSMan::RunL() - [%d] "),msg.iBase.iMsg->pfqos_msg_errno));

			Notify(msg);
			iNotifyPending = EFalse;
			}
		}

	if (!iShutdown)
		{
		iRecBuf.Zero();
		iSocket.Recv(iRecBuf, 0, iStatus);
		SetActive();
		}
	else
		delete this;
	}

void CQoSMan::DoCancel()
	{
	iSocket.CancelRecv();
	}

CPolicy* CQoSMan::OpenQoSPolicyL(const TQoSSelector& aSelector)
	{
	CPolicy* policy = CPolicy::NewL(this, aSelector);
	iStaticPolicies.AddLast(*policy);
	return policy;
	}

void CQoSMan::RemoveQoSPolicy(CPolicy* aPolicy)
	{
	TSglQueIter<CPolicy> iter(iStaticPolicies);
	CPolicy* policy;
	while ((policy = iter++) != NULL)
		{
		if (policy == aPolicy)
			{
			iStaticPolicies.Remove(*aPolicy);
			return;
			}
		}
	}

CPolicy* CQoSMan::FindPolicy(const TQoSSelector& aSelector)
	{
	TSglQueIter<CPolicy> iter(iStaticPolicies);
	CPolicy* policy;

	while ((policy=iter++) != NULL)
		{
		if (policy->Match(aSelector))
			return policy;
		}
	return NULL;
	}

void CQoSMan::ClearPendingRequest(CQoSRequestBase* aRequest)
	{
	iSender->ClearPendingRequest(aRequest);
	}

void CQoSMan::Send(CRequest* aRequest)
	{
	iSender->Send(aRequest);
	}

