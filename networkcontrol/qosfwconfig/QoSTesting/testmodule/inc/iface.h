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
 
#ifndef __TESTIFACE_H__
#define __TESTIFACE_H__


#include <comms-infras/nifif.h>

#include <e32base.h>

class CRequestBase;
class CTestModule;
class CFlowData;

class CNif : public CBase
    {
    public:
    static CNif* NewL(CNifIfBase* aInterface, CTestModule* aModule);
    ~CNif();

    TInt Send(RMBufChain& buf, CProtocolBase* aSourceProtocol=NULL);
    void AddRequest(CRequestBase& aRequest);
    void CancelPendingRequest(CFlowData* aFlowData);
    void CancelPendingRequest(TInt aChannelId);
    void CloseRequest(CRequestBase* aRequest);
    void Unbind();

    inline CNifIfBase* Interface() const;
    inline void SetQueued();

    protected:
    CNif(CTestModule* aModule);
    void ConstructL(CNifIfBase* aInterface);

    private:
    void RunPendingRequests();

    TDblQueLink iNext;
    TBool iInQueue;
    CNifIfBase* iNif;
    CTestModule* iModule;

    CAsyncCallBack* iCallback;
    static TInt CallBack(TAny* aProvider);

    TDblQue<CRequestBase> iPending;
    CRequestBase* iCurrentRequest;

    CAsyncCallBack* iTransmitter;
    static TInt SenderCallBack(TAny* aProvider);
    void DoSend();
    TBool iBlocked;

    friend class CNifManager;
    };

inline CNifIfBase* CNif::Interface() const
    { return iNif; }

inline void CNif::SetQueued()
    { iInQueue = ETrue; };

class CNifManager : public CBase
    {
    public:
    static CNifManager* NewL();
    ~CNifManager();

    CNif* CreateNifL(CNifIfBase* aInterface, CTestModule* aModule);
    CNif* FindInterface(const CNifIfBase *aIface);
    void CancelPendingRequests(CFlowData* aFlowData);
    void CancelPendingRequests(TInt aChannelId);
    void Unbind();

    protected:
    CNifManager();
    void ConstructL();

    private:
    TDblQue<CNif> iNifs;
    };

#endif
