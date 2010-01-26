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
 
#ifndef __TESTMODULE_H__
#define __TESTMODULE_H__


#include <e32base.h>
#include <e32std.h>
#include "module_if.h"
#include "flow.h"
#include <in_iface.h>
#include <in_bind.h>
#include "tc.h"
#include <pfqos.h>
#include "log.h"



const TUint KTestModule = 0x169;
//const TUint KModuleCapabilites = KQoSModuleSerialize|KQoSFatalFailure;
const TUint KModuleCapabilites = KQoSModuleBindToInterface|KQoSModuleSerialize|KQoSFatalFailure;

const TUint KSolTest = 0x501;
const TUint KSoTest = 0x501;

// UniqueId to identify flows
class TFlowData
    {
    public:
    CFlowData* iHandle;
    };

const TUint KMajorVersionNumber=0;
const TUint KMinorVersionNumber=1;
const TUint KBuildVersionNumber=1;


class CNif;
class CPdpContext;
class CChannelMan;
class CNifManager;

class CTestModule : public CModuleBase
    {
    public:
    static CTestModule* NewL();
    ~CTestModule();

    void InitModuleL(MEventInterface& aEventInterface, CExtension* aData);
    void OpenL(CFlowContext &aFlow, CNifIfBase* aIf);
    void Close(CFlowContext &aFlow);
    void Negotiate(CFlowContext &aFlow, const TQoSParameters& aParams, MQoSNegotiateEvent& aNotify);
    void Release(CFlowContext &aFlow);
    void OpenChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& aPolicy, MQoSNegotiateEvent& aNotify, CFlowContext& aFlow);
    void CloseChannel(TInt aChannelId);
    void NegotiateChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& aPolicy, MQoSNegotiateEvent& aNotify);
    void Join(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify);
    void Leave(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify);
    TInt Configure(TUint aLevel, TUint aName, TDes8& aOption, TAny* aSource=0);
    void Identify(TServerProtocolDesc* aProtocolDesc)const;
    static void Identify(TServerProtocolDesc& aDesc);
    TInt Send(RMBufChain& buf, CProtocolBase* aSourceProtocol=NULL);
    void InterfaceAttached(const TDesC& aName, CNifIfBase *aIf);
    void InterfaceDetached(const TDesC& aName, CNifIfBase *aIf);
    void Unbind(CProtocolBase* aProtocol, TUint aId = 0);

    CFlowData* FindFlow(const CFlowContext* aFlow);

    inline CNifManager* IfManager();
    inline CChannelMan* ChannelManager();
    inline MEventInterface* QoSEvent();

    protected:
    CTestModule();
    void ConstructL();

    protected:
    MEventInterface *iNotify;

    CFlowData* AddFlowL(CFlowContext& aFlow, CNif& aNif);
    void RemoveFlow(CFlowContext* aFlow);

    private:
    TUint iFlowCount;
    CNifManager* iIfManager;
    CChannelMan* iChannelManager;
    };


// Inline methods
inline CNifManager* CTestModule::IfManager()
    { return iIfManager; };

inline CChannelMan* CTestModule::ChannelManager()
    { return iChannelManager; };

inline MEventInterface* CTestModule::QoSEvent()
    { return iNotify; }

typedef TDblQueIter<CNif> TNifIter;

#endif
