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
 
#include "channel.h"
#include "tc.h"
#include "async_request.h"
#include "iface.h"
#include "log.h"

CChannel* CChannel::NewL(CFlowData& aFlow, TInt aChannelId)
    {
    __ASSERT_ALWAYS(aFlow.Nif() != NULL, User::Panic(_L("CChannel::NewL()"), 0));
    CChannel* channel = new(ELeave) CChannel(*aFlow.Nif(), aChannelId);
    CleanupStack::PushL(channel);
    channel->ConstructL(aFlow);
    CleanupStack::Pop();
    return channel;
    }

CChannel::CChannel(CNif& aNif, TInt aChannelId) : 
  iNif(&aNif), iChannelId(aChannelId)//, iNbrOfFlows((TUint)0)
    {
    LOG(Log::Printf(_L("CChannel::CChannel() [ChannelId=%d]\r\n"),iChannelId));
    iFlows.SetOffset(_FOFF(RFlow, iLink));
    iNbrOfFlows = 0;
    }

CChannel::~CChannel()
    {
    LOG(Log::Printf(_L("CChannel::~CChannel() [ChannelId=%d]\r\n"),iChannelId));
    iLink.Deque();
    TDblQueIter<RFlow> i(iFlows);
    RFlow* flow;
    while((flow=i++)!=NULL)
	{
	flow->iFlow->SetChannelId(-1);
	delete flow;
	}
    }

void CChannel::ConstructL(CFlowData& aFlow)
    {
    AddFlowL(aFlow);
    }

void CChannel::AddFlowL(CFlowData& aFlow)
    {
    RFlow* flow = new (ELeave) RFlow(aFlow);
    iFlows.AddLast(*flow);
    aFlow.SetChannelId(iChannelId);
    iNbrOfFlows++;
    }

void CChannel::RemoveFlow(CFlowData& aFlow)
    {
    RFlow* flow = FindFlow(aFlow);
    __ASSERT_ALWAYS(flow, User::Panic(_L("CChannel::RemoveFlow()"), 0));
    delete flow;
    aFlow.SetChannelId(-1);
    iNbrOfFlows--;
    }

RFlow* CChannel::FindFlow(const CFlowData& aFlow)
    {
    TDblQueIter<RFlow> i(iFlows);
    RFlow* flow;
    while((flow=i++)!=NULL)
	if (flow->iFlow == &aFlow)
	    return flow;
    return NULL;
    }

//
CChannelMan* CChannelMan::NewL()
    {
    CChannelMan* manager = new(ELeave) CChannelMan();
    CleanupStack::PushL(manager);
    manager->ConstructL();
    CleanupStack::Pop();
    return manager;
    }

CChannelMan::~CChannelMan()
    {
    LOG(Log::Printf(_L("CChannelMan::~CChannelMan()\r\n")));
    TDblQueIter<CChannel> i(iChannels);
    CChannel* channel;
    while((channel=i++) != NULL)
	delete channel;
    }

CChannelMan::CChannelMan()
    {
    LOG(Log::Printf(_L("CChannelMan::CChannelMan()\r\n")));
    iChannels.SetOffset(_FOFF(CChannel, iLink));
    }

void CChannelMan::ConstructL()
    {
    }

CChannel* CChannelMan::NewChannelL(CFlowData& aFlow, TInt aChannelId)
    {
    CChannel* channel = CChannel::NewL(aFlow, aChannelId);
    iChannels.AddLast(*channel);
    return channel;
    }

CChannel* CChannelMan::FindChannel(TInt aChannelId)
    {
    TDblQueIter<CChannel> i(iChannels);
    CChannel* channel;
    while((channel=i++) != NULL)
	if (channel->ChannelId() == aChannelId)
	    return channel;
    return NULL;
    }

