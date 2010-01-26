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
 
#include <e32base.h>
#include <in_iface.h>
#include "iface.h"
#include "async_request.h"

CNif* CNif::NewL(CNifIfBase* aInterface, CTestModule* aModule)
    {
    CNif* aNif = new (ELeave) CNif(aModule);
    CleanupStack::PushL(aNif);
    aNif->ConstructL(aInterface);
    CleanupStack::Pop();
    return aNif;
    }

void CNif::ConstructL(CNifIfBase* aInterface)
    {
    TCallBack sender(SenderCallBack, this);
    iTransmitter = new (ELeave) CAsyncCallBack(sender, CActive::EPriorityStandard);
    TCallBack callback(CallBack, this);
    iCallback = new (ELeave) CAsyncCallBack(callback, CActive::EPriorityStandard);
    iNif = aInterface;
    }

CNif::CNif(CTestModule* aModule) : iModule(aModule)
    {
    iPending.SetOffset(_FOFF(CRequestBase, iLink));
    iNif = NULL;
    iCurrentRequest = NULL;
    iBlocked = EFalse;
    }

CNif::~CNif()
    {
    TDblQueIter<CRequestBase> i(iPending);
    CRequestBase* request;
    while ((request=i++) != NULL)
	delete request;

    delete iCurrentRequest;
    delete iCallback;
    iTransmitter->Cancel();
    delete iTransmitter;
 
    if (iInQueue)
	iNext.Deque();
    }

void CNif::AddRequest(CRequestBase& aRequest)
    {
    iPending.AddLast(aRequest);
    iCallback->CallBack();
    }

void CNif::CancelPendingRequest(CFlowData* aFlowData)
    {
    TDblQueIter<CRequestBase> iter(iPending);
    CRequestBase* request;
    while ((request = iter++) != NULL)
	request->Cancel(aFlowData);
    }

void CNif::CancelPendingRequest(TInt aChannelId)
    {
    TDblQueIter<CRequestBase> iter(iPending);
    CRequestBase* request;
    while ((request = iter++) != NULL)
	request->Cancel(aChannelId);
    }

TInt CNif::CallBack(TAny* aProvider)
    {
    ((CNif *)aProvider)->RunPendingRequests();
    return KErrNone;
    }

void CNif::RunPendingRequests()
    {
    TDblQueIter<CRequestBase> iter(iPending);
    CRequestBase* request;
    while ((request = iter++) != NULL && iCurrentRequest == NULL)
	{
	iCurrentRequest = request;
	request->Run();
	}
    }

void CNif::CloseRequest(CRequestBase* aRequest)
    {
    if (iCurrentRequest != aRequest)
	return;
    iCurrentRequest = NULL;
    iCallback->CallBack();
    }

void CNif::Unbind()
    {

    }

TInt CNif::Send(RMBufChain& /*buf*/, CProtocolBase*)
    {
    TInt ret = 1; 
    iTransmitter->CallBack();
    return ret;
    }

TInt CNif::SenderCallBack(TAny* aProvider)
    {
    ((CNif *)aProvider)->DoSend();
    return KErrNone;
    }

void CNif::DoSend()
    {
    RMBufChain buf;
    while (1)
	{
	RMBufSendPacket packet;
	packet.Assign(buf);
	TInt ret = 0;
	if (ret <= 0)
	    {
	    iBlocked = ETrue;
	    return;
	    }
	}
    }


//
CNifManager* CNifManager::NewL()
    {
    CNifManager* manager = new (ELeave) CNifManager();
    CleanupStack::PushL(manager);
    manager->ConstructL();
    CleanupStack::Pop();
    return manager;
    }

CNifManager::~CNifManager()
    {
    TDblQueIter<CNif> iter(iNifs);
    CNif* nif;
    while((nif=iter++)!=NULL)
	delete nif;
    }


CNifManager::CNifManager()
    {
    iNifs.SetOffset(_FOFF(CNif, iNext));
    }

void CNifManager::ConstructL()
    {
    }

CNif* CNifManager::CreateNifL(CNifIfBase* aInterface, CTestModule* aModule)
    {
    CNif* nif = CNif::NewL(aInterface, aModule);
    iNifs.AddLast(*nif);
    nif->SetQueued();
    return nif;
    }

CNif* CNifManager::FindInterface(const CNifIfBase *aIface)
    {
    TDblQueIter<CNif> iter(iNifs);
    CNif* nif;
    while ((nif = iter++) != NULL)
	if (nif->Interface() == aIface)
	    return nif;
    return NULL;
    }

// delete all pending requests related to this flow
void CNifManager::CancelPendingRequests(CFlowData* aFlowData)
    {
    __ASSERT_ALWAYS(aFlowData!=NULL, User::Panic(_L("CNifManager::CancelPendingRequests"), 0));
    TDblQueIter<CNif> iter(iNifs);
    CNif *nif;
    while ((nif = iter++) != NULL)
	nif->CancelPendingRequest(aFlowData);
    }

// delete all pending requests related to this flow
void CNifManager::CancelPendingRequests(TInt aChannelId)
    {
    __ASSERT_ALWAYS(aChannelId>=0, User::Panic(_L("CNifManager::CancelPendingRequests"), 0));
    TDblQueIter<CNif> iter(iNifs);
    CNif *nif;
    while ((nif = iter++) != NULL)
	nif->CancelPendingRequest(aChannelId);
    }

void CNifManager::Unbind()
    {
    TDblQueIter<CNif> iter(iNifs);
    CNif *nif;
    while ((nif = iter++) != NULL)
	nif->Unbind();
    }

