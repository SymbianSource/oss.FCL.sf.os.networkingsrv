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
 
#include "iface.h"
#include "qoserr.h"
#include "async_request.h"

CRequestBase* CRequestBase::NewL(CFlowData* aFlow, CNif* aNif, MQoSNegotiateEvent* aNotify)
    {
    CRequestBase* request = new (ELeave) CRequestBase(aFlow, aNif, aNotify);
    CleanupStack::PushL(request);
    request->ConstructL();
    CleanupStack::Pop();
    return request;
    }

CRequestBase* CRequestBase::NewL(TInt aChannelId, CNif* aNif, MQoSNegotiateEvent* aNotify)
    {
    CRequestBase* request = new (ELeave) CRequestBase(aChannelId, aNif, aNotify);
    CleanupStack::PushL(request);
    request->ConstructL();
    CleanupStack::Pop();
    return request;
    }

CRequestBase::CRequestBase(CFlowData* aFlow, CNif* aNif, MQoSNegotiateEvent* aNotify) 
  : 	 iNotify(aNotify), iNif(aNif), iFlow(aFlow)
    {
    iChannelId = -1;
	
    }

CRequestBase::CRequestBase(TInt aChannelId, CNif* aNif, MQoSNegotiateEvent* aNotify) 
  : 	iNotify(aNotify),iNif(aNif),iChannelId(aChannelId)
    {
    iFlow = NULL;
    }

void CRequestBase::ConstructL()
    {
    TCallBack callback(TimeoutCallBack, this);
    }

CRequestBase::~CRequestBase()
    {
    iLink.Deque();
    iNif->CloseRequest(this);
    }

void CRequestBase::Run()
    {
    }

void CRequestBase::Cancel(CFlowData* aFlowData)
    {
    __ASSERT_ALWAYS(aFlowData != NULL, User::Panic(_L("CRequestBase::Cancel"), 0));
    if (iFlow == aFlowData)
	{
	delete this;
	}
    }

void CRequestBase::Cancel(TInt aChannelId)
    {
    __ASSERT_ALWAYS(aChannelId >= 0, User::Panic(_L("CRequestBase::Cancel"), 0));
    if (iChannelId == aChannelId)
	{
	delete this;
	}
    }

void CRequestBase::NotifyEvent()
    {
    TQoSParameters params;
    iNotify->RequestComplete(KErrNone, &params);
    }

TInt CRequestBase::TimeoutCallBack(TAny* aProvider)
    {
    CRequestBase* request= (CRequestBase*)aProvider;
    request->NotifyEvent();
    delete request;
    return KErrNone;
    }
