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

#include <es_ini.h>
//xxx
#include <testif.h>
#include <f32file.h>
#include <s32file.h>
#include "testmodule.h"
#include "iface.h"
#include "channel.h"


CTestModule* CTestModule::NewL()
    {
    CTestModule* module = new (ELeave) CTestModule();
    CleanupStack::PushL(module);
    module->ConstructL();
    CleanupStack::Pop();
    return module;
    }

CTestModule::CTestModule()
    {
    LOG(Log::Printf(_L("CTestModule::CTestModule()\r\n")));
    iFlowCount = 0;

    RFs fs;
    RFile file;
    fs.Connect();
    file.Create(fs, KTestModuleLoaded, EFileStreamText);
    file.Close();
    fs.Close();
    }

void CTestModule::ConstructL()
    {
    iIfManager = CNifManager::NewL();
    iChannelManager = CChannelMan::NewL();
    }

CTestModule::~CTestModule()
    {
    LOG(Log::Printf(_L("CTestModule::~CTestModule()\r\n")));
    __ASSERT_ALWAYS(iFlowCount == 0, User::Panic(_L("CTestModule::~CTestModule()"), 0));
    delete iChannelManager;
    delete iIfManager;

    RFs fs;
    RFile file;
    fs.Connect();
    file.Create(fs, KTestModuleUnloaded, EFileStreamText);
    file.Close();
    fs.Close();
    }

void CTestModule::InterfaceAttached(const TDesC& , CNifIfBase* aIf)
    {
    __ASSERT_ALWAYS(aIf != NULL, 
                    User::Panic(_L("CTestModule::InterfaceAttached()"), 0));
    CNif* nif = iIfManager->FindInterface(aIf);
    if (!nif)
	    {
	    TRAPD(err, nif = iIfManager->CreateNifL(aIf, this));
        if (err != KErrNone)
            {
            LOG(Log::Printf(_L("iIfManager->CreateNifL error: %d"), err));
            }
	    }
    }

void CTestModule::InterfaceDetached(const TDesC& , CNifIfBase* aIf)
    {
    __ASSERT_ALWAYS(aIf != NULL, User::Panic(_L("CTestModule::InterfaceDetached()"), 0));
    CNif* nif = iIfManager->FindInterface(aIf);
    if (nif)
	delete nif;
    }

void CTestModule::Unbind(CProtocolBase* , TUint )
    {
    iIfManager->Unbind();
    }

void CTestModule::InitModuleL(MEventInterface& aEventInterface, CExtension* )
    {
    iNotify = &aEventInterface;
    }

void CTestModule::OpenL(CFlowContext& aFlow, CNifIfBase* aIf)
    {
    __ASSERT_ALWAYS(aIf != NULL, User::Panic(_L("CTestModule::Open"), 0));
    CNif* aNif = iIfManager->FindInterface(aIf);
    User::LeaveIfNull(aNif);
    CFlowData *flowdata = FindFlow(&aFlow);
    if (flowdata)
	User::Leave(KErrAlreadyExists);
    flowdata = AddFlowL(aFlow, *aNif);
    }

void CTestModule::Close(CFlowContext& aFlow)
    {
    CFlowData* flowdata = FindFlow(&aFlow);
    if (!flowdata)
	return;
    CChannel* channel = iChannelManager->FindChannel(flowdata->ChannelId());
    if (channel)
	{
	channel->RemoveFlow(*flowdata);
	iIfManager->CancelPendingRequests(channel->ChannelId());
	}
    iIfManager->CancelPendingRequests(flowdata);
    RemoveFlow(&aFlow);
    }

void CTestModule::Release(CFlowContext& aFlow)
    {
    CFlowData* flowdata = FindFlow(&aFlow);
    __ASSERT_ALWAYS(flowdata, User::Panic(_L("CTestModule::Release"), 0));
    CChannel* channel = iChannelManager->FindChannel(flowdata->ChannelId());
    if (channel)
	{
	channel->RemoveFlow(*flowdata);
	iIfManager->CancelPendingRequests(channel->ChannelId());
	}
    iIfManager->CancelPendingRequests(flowdata);
    }

void CTestModule::OpenChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& , MQoSNegotiateEvent& aNotify, CFlowContext& aFlow)
    {
    CFlowData *flowdata = FindFlow(&aFlow);
    _LIT(OpenChannelLiteral, "CTestModule::OpenChannel()");
    __ASSERT_ALWAYS(flowdata != NULL, User::Panic(OpenChannelLiteral, 0));
    TInt ret = (flowdata->Nif() == NULL) ? KErrNotFound : KErrNone;
    CChannel* channel = NULL;
    if (ret == KErrNone)
	{
	TRAP(ret, channel = iChannelManager->NewChannelL(*flowdata, aChannelId));
	}

    if (ret == KErrNone)
	{
	CRequestBase* request = NULL;
	TRAP(ret, request = CRequestBase::NewL(channel->ChannelId(), flowdata->Nif(), &aNotify));
	if (ret == KErrNone)
	    flowdata->Nif()->AddRequest(*request);
	}
    if (ret != KErrNone)
	aNotify.RequestComplete(ret, &aParams);
    }


void CTestModule::CloseChannel(TInt aChannelId)
    {
    _LIT(CloseChannelLiteral, "CTestModule::CloseChannel()");
    CChannel* channel = iChannelManager->FindChannel(aChannelId);
    __ASSERT_ALWAYS(channel != NULL, User::Panic(CloseChannelLiteral, 0));
    iIfManager->CancelPendingRequests(channel->ChannelId());
    delete channel;
    }

void CTestModule::NegotiateChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& , MQoSNegotiateEvent& aNotify)
    {
    _LIT(NegotiateChannelLiteral, "CTestModule::NegotiateChannel()");
    CChannel* channel = iChannelManager->FindChannel(aChannelId);
    __ASSERT_ALWAYS(channel != NULL, User::Panic(NegotiateChannelLiteral, 0));
    CRequestBase* request = NULL;
    TRAPD(ret, CRequestBase::NewL(channel->ChannelId(), NULL, &aNotify));
    if (ret == KErrNone)
	channel->Nif()->AddRequest(*request);
    else
	aNotify.RequestComplete(ret, &aParams);
    }

void CTestModule::Join(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify)
    {
    _LIT(JoinLiteral, "CTestModule::Join()");
    CChannel* channel = iChannelManager->FindChannel(aChannelId);
    __ASSERT_ALWAYS(channel != NULL, User::Panic(JoinLiteral, 0));
    CFlowData *flowdata = FindFlow(&aFlow);
    __ASSERT_ALWAYS(flowdata != NULL, User::Panic(JoinLiteral, 0));
    CRequestBase* request = NULL;
    TRAPD(ret, request = CRequestBase::NewL(channel->ChannelId(), flowdata->Nif(), &aNotify));
    if (ret == KErrNone)
	channel->Nif()->AddRequest(*request);
    else
	aNotify.RequestComplete(ret, NULL);
    }

void CTestModule::Leave(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify)
    {
    _LIT(LeaveLiteral, "CTestModule::Leave()");
    CChannel* channel = iChannelManager->FindChannel(aChannelId);
    __ASSERT_ALWAYS(channel != NULL, User::Panic(LeaveLiteral, 0));
    CFlowData *flowdata = FindFlow(&aFlow);
    __ASSERT_ALWAYS(flowdata != NULL, User::Panic(LeaveLiteral, 0));
    CRequestBase* request = NULL;
    TRAPD(ret, request = CRequestBase::NewL(channel->ChannelId(), flowdata->Nif(), &aNotify));
    if (ret == KErrNone)
	channel->Nif()->AddRequest(*request);
    else
	aNotify.RequestComplete(ret, NULL);
    }

void CTestModule::Negotiate(CFlowContext &aFlow, const TQoSParameters& aParams, MQoSNegotiateEvent& aNotify)
    {
    _LIT(NegotiateLiteral, "CTestModule::Negotiate");
    CFlowData* flowdata = FindFlow(&aFlow);
    __ASSERT_ALWAYS(flowdata, User::Panic(NegotiateLiteral, 0));
    __ASSERT_ALWAYS(flowdata->Nif(), User::Panic(NegotiateLiteral, 0));
    CRequestBase* request = NULL;
    TRAPD(ret, request = CRequestBase::NewL(flowdata, flowdata->Nif(), &aNotify));
    if (ret == KErrNone)
	flowdata->Nif()->AddRequest(*request);
    else
	aNotify.RequestComplete(ret, &aParams);
    }


TInt CTestModule::Configure(TUint aLevel,TUint aName, TDes8& aOption, TAny* )
    {
    if (aLevel == KSOLQoSModule)
	{
	switch (aName)
	    {
	    case KSoCapabilities:
	      {
	      TInt& opt = *(TInt*)aOption.Ptr();
	      opt = KModuleCapabilites;
	      }
	      return KErrNone;

	    default:
		break;
	    }
	}
    return KErrNotSupported;
    }

TInt CTestModule::Send(RMBufChain& buf, CProtocolBase* )
    {
    RMBufSendPacket packet;
    packet.Assign(buf);
    RMBufSendInfo *const info = packet.Unpack();
    CFlowContext* context = info->iFlow.FlowContext();
    CFlowData* flowdata = FindFlow(context);
    __ASSERT_ALWAYS(flowdata != NULL, User::Panic(_L("CModuleGuqos::Send(): flowdata"), 0));
    packet.Pack();
    return flowdata->Nif()->Send(packet);
    }

void CTestModule::Identify(TServerProtocolDesc* aProtocolDesc) const
    {
    Identify(*aProtocolDesc);
    }

void CTestModule::Identify(TServerProtocolDesc& aDesc)
    {
    aDesc.iName=_S("testmodule");
    aDesc.iAddrFamily=KAfInet;
    aDesc.iSockType=KSockDatagram;
    aDesc.iProtocol=KTestModule;
    aDesc.iVersion=TVersion(KMajorVersionNumber, KMinorVersionNumber, KBuildVersionNumber);
    aDesc.iByteOrder=EBigEndian;
    aDesc.iServiceInfo=0;
    aDesc.iNamingServices=0;
    aDesc.iSecurity=KSocketNoSecurity;
    aDesc.iMessageSize=0xffff;
    aDesc.iServiceTypeInfo=EPreferMBufChains | ENeedMBufs;
    aDesc.iNumSockets=KUnlimitedSockets;
    }

CFlowData* CTestModule::AddFlowL(CFlowContext& aFlow, CNif& aNif)
    {
    CFlowData* flowdata = CFlowData::NewL(&aFlow, &aNif);
    TFlowData data;
    data.iHandle = flowdata;
    TPckg<TFlowData> aOpt(data);
    TInt err = aFlow.StoreOption(KSolTest, KSoTest, aOpt);
    if (err!=KErrNone)
	{
	delete flowdata;
	User::Leave(err);
	}
    iFlowCount++;
    return flowdata;
    }

void CTestModule::RemoveFlow(CFlowContext* aFlow)
    {
    TFlowData data;
    TPckg<TFlowData> aOpt(data);
    TInt ret = aFlow->RetrieveOption(KSolTest, KSoTest, aOpt);
    __ASSERT_ALWAYS(ret == KErrNone, User::Panic(_L("CTestModule::RemoveFlow"), 0));
    CFlowData* flowdata = data.iHandle;
    __ASSERT_ALWAYS(flowdata, User::Panic(_L("CTestModule::RemoveFlow"), 0));
    data.iHandle = NULL;
    ret = aFlow->StoreOption(KSolTest, KSoTest, aOpt);
    __ASSERT_ALWAYS(ret == KErrNone, User::Panic(_L("CTestModule::RemoveFlow"), 0));
    iFlowCount--;
    delete flowdata;
    }

CFlowData* CTestModule::FindFlow(const CFlowContext* aFlow)
    {
    TFlowData data;
    TPckg<TFlowData> aOpt(data);
    TInt ret = aFlow->RetrieveOption(KSolTest, KSoTest, aOpt);
    if (ret != KErrNone)
	return NULL;
    return data.iHandle;
    }

