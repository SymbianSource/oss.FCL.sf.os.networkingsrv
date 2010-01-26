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
 
#ifndef __ASYNC_REQUEST_H__
#define __ASYNC_REQUEST_H__

#include "module_if.h"

enum TRequestType
    {
    ENegotiate,
    EClose,
    EOpenChannel,
    ENegotiateChannel,
    ECloseChannel,
    EJoinChannel,
    ELeaveChannel,
    EDefaultContext
    };

class CFlowData;
class CNif;

// Base class for async requests: separate flow-specific and channel-specific requests?
class CRequestBase : public CBase
    {
    public:
    static CRequestBase* NewL(CFlowData* aFlow, CNif* aNif, MQoSNegotiateEvent* aNotify);
    static CRequestBase* NewL(TInt aChannelId, CNif* aNif, MQoSNegotiateEvent* aNotify);
    ~CRequestBase();
    void Run();
    void Cancel(CFlowData* aFlowData);
    void Cancel(TInt aChannelId);
    void NotifyEvent();

    protected:
    CRequestBase(CFlowData* aFlow, CNif* aNif, MQoSNegotiateEvent* aNotify);
    CRequestBase(TInt aChannelId, CNif* aNif, MQoSNegotiateEvent* aNotify);
    void ConstructL();

    MQoSNegotiateEvent* iNotify;
    CNif* iNif;
    CFlowData* iFlow;
    TInt iChannelId;

    // Timer to emulate QoS negotiation
    static TInt TimeoutCallBack(TAny* aProvider);

    TDblQueLink iLink;
    friend class CNif;
    private:
    TUint iType;
    };


#endif
