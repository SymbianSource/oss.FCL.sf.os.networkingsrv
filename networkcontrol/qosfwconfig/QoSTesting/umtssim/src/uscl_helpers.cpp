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
// uscl_helpers.cpp - implementations of some classes meant to make simulator user's life easier
//
 
#include "uscl_helpers.h"
#include "uscl_pcktcs.h"


// *** CPacketServiceStatus *** helper object to manage asynchronous status queries
EXPORT_C CPacketServiceStatus* CPacketServiceStatus::NewL(RPacketService* aPacketService)
    {
	CPacketServiceStatus* self = new (ELeave) CPacketServiceStatus(aPacketService);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketServiceStatus::CPacketServiceStatus(RPacketService* aPacketService)
	: CActive(0), iPacketService(aPacketService), iCallback(NULL)
    {
	// nothing to do here
    }

EXPORT_C CPacketServiceStatus::~CPacketServiceStatus()
    {
	// nothing to do here
    }

void CPacketServiceStatus::ConstructL()
    {
	CActiveScheduler::Add(this);
    }

EXPORT_C void CPacketServiceStatus::MakeRequest(TStatusCB aCallback, TAny* aParam)
    {
	if(IsActive()) {
    // what to do?
	} else {
    iCallback = aCallback;
    iCallbackParam = aParam;
    iPacketService->NotifyStatusChange(iStatus, iPacketServiceStatus);
    SetActive();
	}
    }

EXPORT_C void CPacketServiceStatus::CancelRequest()
    {
	if(!IsActive()) {
    // what to do?
	} else {
    Cancel(); // from CActive, will call DoCancel()
	}
    }

void CPacketServiceStatus::DoCancel()
    {
	//iPacketService->NotifyStatusChangeCancel();
	iPacketService->CancelAsyncRequest(EPacketNotifyStatusChange);
    }

void CPacketServiceStatus::RunL()
    {
	if(iStatus.Int() == KErrNone) {
    if(iCallback) iCallback(iPacketServiceStatus, iCallbackParam);
	} else if(iStatus.Int() != KErrCancel) {
    User::Invariant(); // Err, error ;)
	}
    }



// *** CControlNotifyAll *** Active object taking care of asynchronous event queries
EXPORT_C CControlNotifyAll* CControlNotifyAll::NewL(RControl* aControl)
    {
	CControlNotifyAll* self = new (ELeave) CControlNotifyAll(aControl);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CControlNotifyAll::CControlNotifyAll(RControl* aControl)
	: CActive(0), iControl(aControl), iCallback(NULL)
    {
	// nothing to do here
    }

EXPORT_C CControlNotifyAll::~CControlNotifyAll()
    {
	// nothing to do here
    }

void CControlNotifyAll::ConstructL()
    {
	CActiveScheduler::Add(this);
    }

EXPORT_C void CControlNotifyAll::MakeRequest(TNotifyAllCB aCallback, TAny* aParam)
    {
	if(IsActive()) {
    // what to do?
	} else {
    iCallback = aCallback;
    iCallbackParam = aParam;
    iControl->NotifyAll(iStatus, iMessage);
    SetActive();
	}
    }

EXPORT_C void CControlNotifyAll::CancelRequest()
    {
	if(!IsActive()) {
    // what to do?
	} else {
    Cancel(); // from CActive, will call DoCancel()
	}
    }

void CControlNotifyAll::DoCancel()
    {
	iControl->CancelAsyncRequest(EControlNotifyAll);
    }

void CControlNotifyAll::RunL()
    {
	if(iStatus.Int() == KErrNone) {
    if(iCallback) iCallback(iMessage, iCallbackParam);
	} else if(iStatus.Int() != KErrCancel) {
    User::Invariant(); // Err, error ;)
	}
    }



#if 0

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    EXPORT_C CAsyncRequestHandler<T,U,RF,CF>* CAsyncRequestHandler<T,U,RF,CF>::NewL(U* aSubSession)
    {
	CAsyncRequestHandler<T,U,RF,CF>* self = new (ELeave) CAsyncRequestHandler<T,U,RF,CF>(aSubSession);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    CAsyncRequestHandler<T,U,RF,CF>::CAsyncRequestHandler(U* aSubSession)
        : CActive(0), iSubSession(aSubSession), iCallback(NULL)
    {
	// nothing to do here
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    EXPORT_C CAsyncRequestHandler<T,U,RF,CF>::~CAsyncRequestHandler()
    {
	// nothing to do here
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    void CAsyncRequestHandler<T,U,RF,CF>::ConstructL()
    {
	CActiveScheduler::Add(this);
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    EXPORT_C void CAsyncRequestHandler<T,U,RF,CF>::MakeRequest(TFuncTypes<T,U>::TCBFunc aCallback, TAny* aParam)
    {
	if(IsActive()) {
    // what to do?
	} else {
    iCallback = aCallback;
    iCallbackParam = aParam;
    iSubSession->*RF(iStatus, iReturnValue);
    SetActive();
	}
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    EXPORT_C void CAsyncRequestHandler<T,U,RF,CF>::CancelRequest()
    {
	if(!IsActive()) {
    // what to do?
	} else {
    Cancel(); // from CActive, will call DoCancel()
	}
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    void CAsyncRequestHandler<T,U,RF,CF>::DoCancel()
    {
	iSubSession->*CF();
    }

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF>
    void CAsyncRequestHandler<T,U,RF,CF>::RunL()
    {
	if(iStatus.Int() == KErrNone) {
    if(iCallback) iCallback(iReturnValue, iCallbackParam);
	} else if(iStatus.Int() != KErrCancel) {
    User::Invariant(); // Err, error ;)
	}
    }

#endif
