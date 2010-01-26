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
 
#ifndef __GUQOS_CHANNEL_H__
#define __GUQOS_CHANNEL_H__

#include <e32std.h>
#include <e32base.h>

class CFlowData;
class CNif;


class RFlow
    {
    public:
    RFlow(CFlowData& aFlow) : iFlow(&aFlow){};
    ~RFlow() { iLink.Deque(); };

    CFlowData* iFlow;
    TDblQueLink iLink;
    };

class CCloseChannel;
class CChannel : public CBase
    {
    public:
    static CChannel* NewL(CFlowData& aFlow, TInt aChannelId);
    ~CChannel();

    void AddFlowL(CFlowData& aFlow);
    void RemoveFlow(CFlowData& aFlow);
    RFlow* FindFlow(const CFlowData& aFlow);

    inline TInt ChannelId() const;
    inline TUint NbrOfFlows() const;
    inline CNif* Nif();

    protected:
    CChannel(CNif& aNif, TInt aChannelId);
    void ConstructL(CFlowData& aFlow);

    private:
    TUint iNbrOfFlows;
    CNif* iNif;
    TInt iChannelId;
    TDblQue<RFlow> iFlows;
    TDblQueLink iLink;
    friend class CChannelMan;
    friend class TFlowIter;
    };

class TFlowIter
    {
    public:
    inline TFlowIter(CChannel& aChannel) : iIter(aChannel.iFlows) {};
    inline CFlowData* operator++(TInt)
	  {
	  RFlow* flow = iIter++;
	  if (flow)
	      return flow->iFlow;
	  return NULL;
	  };

    private:
    TDblQueIter<RFlow> iIter;
    };

class CChannelMan : public CBase
    {
    public:
    static CChannelMan* NewL();
    ~CChannelMan();

    CChannel* NewChannelL(CFlowData& aFlow, TInt aChannelId);
    CChannel* FindChannel(TInt aChannelId);

    protected:
    CChannelMan();
    void ConstructL();

    private:
    TDblQue<CChannel> iChannels;
    friend class TChannelIter;
    };


class TChannelIter
    {
    public:
    inline TChannelIter(CChannelMan& aChannelMan) : iIter(aChannelMan.iChannels) {};
    inline CChannel* operator++(TInt)
	  {
	  CChannel* channel= iIter++;
	  return channel;
	  };

    private:
    TDblQueIter<CChannel> iIter;
    };


//

inline TInt CChannel::ChannelId() const
    { return iChannelId; };

inline TUint CChannel::NbrOfFlows() const
    { return iNbrOfFlows; };

inline CNif* CChannel::Nif()
    { return iNif; };

#endif
