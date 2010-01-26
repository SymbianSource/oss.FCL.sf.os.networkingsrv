// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef _USCL_HELPERS_H__
#define _USCL_HELPERS_H__

#include "uscl_packet.h"
#include "uscl_control.h"


//**********************************
// Helper classes
//**********************************

// Helpers for RPacketService

// *** CPacketServiceStatus *** Active object taking care of asynchronous status queries

class CPacketServiceStatus : public CActive
    {
    public:
      // types
      typedef void (*TStatusCB)(const RPacketService::TStatus&, TAny *);
    public:
      IMPORT_C static CPacketServiceStatus* NewL(RPacketService* aPacketService);
      IMPORT_C ~CPacketServiceStatus();

    private:
      CPacketServiceStatus(RPacketService* aPacketService);
      void ConstructL();

    public:
      // request handling
      IMPORT_C void MakeRequest(TStatusCB aCallback, TAny* aParam);
      IMPORT_C void CancelRequest();

      // CActive
      void RunL();
      void DoCancel();
    private:
      RPacketService* iPacketService;
      RPacketService::TStatus iPacketServiceStatus;
      TStatusCB iCallback;
      TAny* iCallbackParam;
    };



// Helpers for RControl

// *** CControlNotifyAll *** Active object taking care of asynchronous event queries

class CControlNotifyAll : public CActive
    {
    public:
      // types
      typedef void (*TNotifyAllCB)(const TDesC&, TAny *);
    public:
      IMPORT_C static CControlNotifyAll* NewL(RControl* aControl);
      IMPORT_C ~CControlNotifyAll();

    private:
      CControlNotifyAll(RControl* aControl);
      void ConstructL();

    public:
      // request handling
      IMPORT_C void MakeRequest(TNotifyAllCB aCallback, TAny* aParam);
      IMPORT_C void CancelRequest();

      // CActive
      void RunL();
      void DoCancel();
    private:
      RControl* iControl;
      TBuf<200> iMessage;
      TNotifyAllCB iCallback;
      TAny* iCallbackParam;
    };


// The following may some day save some coder's life, but doesn't work yet :E
#if 0

template<class T, class U> struct TFuncTypes
    {
      typedef void (U::* TReqFunc)(TRequestStatus&, T&) const;
      typedef void (U::* TCancelFunc)() const;
      typedef void (*TCBFunc)(T&, TAny*);
    };

template<class T, class U, TFuncTypes<T,U>::TReqFunc RF, TFuncTypes<T,U>::TCancelFunc CF> class CAsyncRequestHandler : public CActive
    {
    public:
      IMPORT_C static CAsyncRequestHandler* NewL(U* aSubSession);
      IMPORT_C ~CAsyncRequestHandler();

    private:
      CAsyncRequestHandler(U*);
      void ConstructL();

    public:
      IMPORT_C void MakeRequest(TFuncTypes<T,U>::TCBFunc aCallback, TAny* aParam);
      IMPORT_C void CancelRequest();

      // CActive
      void RunL();
      void DoCancel();

    private:
      U* iSubSession;
      T iReturnValue;
      TFuncTypes<T,U>::TCBFunc iCallback;
      TAny* iCallbackParam;
    };


typedef CAsyncRequestHandler<RPacketService::TStatus,
							 RPacketService,
							 &RPacketService::NotifyStatusChange,
							 &RPacketService::NotifyStatusChangeCancel>
    CPacketServiceStatus;
typedef CAsyncRequestHandler<TBuf<65>,
							 RControl,
							 &RControl::NotifyAll,
							 &RControl::NotifyAllCancel>
    CControlNotifyAll;
#endif


#endif
