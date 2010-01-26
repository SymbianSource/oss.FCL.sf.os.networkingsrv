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
// qoslib_glob.h - Qoslib global (TLS) data
// Qoslib global (TLS) data
//



/**
 @internalComponent
*/
#ifndef __QOSLIB_GLOB_H__
#define __QOSLIB_GLOB_H__

#include "qoslib.h"
#include "pfqosparser.h"
#include "pfqos_stream.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

//
const TInt KQoSSignaledOK		= 0x0001; // Signaling ok
const TInt KQoSSignaledFailed	= 0x0002; // Signaling failed
const TInt KQoSProvisionedOK	= 0x0004; // Provisioning ok
const TInt KQoSProvisionedFailed	= 0x0008; // Provisioning failed
const TInt KQoSOK				= 0x0010; // A scheduler module should return this when ok 
const TInt KQoSFailed			= 0x0020; // A scheduler module should return this when request fails
const TInt KQoSRequestPending	= 0x0040; // Request is pending and the result will be given using 
										  // Event-method (see below)

const TInt KQoSDefaultBufSize = 8000;

// Forward declaration
class CQoSMan;
// 
class CQoSRequestBase : public CBase
{
	friend class CQoSMan;
public:
	~CQoSRequestBase();

	virtual void ProcessEvent(TPfqosMessage& aMsg)=0;
	virtual void NotifyError(TInt aReason)=0;
	virtual TBool MatchReply(const TPfqosMessage& aMsg, TUint8 aMsgType)=0;
	virtual void ProcessReply(TPfqosMessage& aMsg)=0;
	TInt NotifyEvent(MQoSObserver& aObserver, TUint aMask);
	TInt CancelNotifyEvent(MQoSObserver& aObserver);

protected:
	void ParseExtensions(TPfqosMessage& aMsg, CQoSParameters& aPolicy);
	TInt GetExtensionType(const TDesC8& aData, TInt& aType);

protected:
	TSglQueLink iNext;
	TUint iEventMask;
	MQoSObserver* iObserver;
	CQoSParameters iPolicy;
	CQoSMan* iManager;
};

class TSelectorItem : public TQoSSelector
{
public:
	TSglQueLink iNext;
};

enum EQoSChannelStatus
{
	EInit,
	EChannelCreated,
	EChannelReady
};


class CChannel : public CQoSRequestBase
{
public:
	enum TPendingStatus
	{
		ENone,
		EPendingOpen,
		EPendingOpenExisting,
		EPendingOpenExistingSetQoS,
		EPendingSetPolicy,
		EPendingJoin,
		EPendingLeave,
		EPendingDelete
	};

	static CChannel* NewL(CQoSMan* aManager, RSocket& aSocket, CQoSParameters* aSpec);
	~CChannel();
	void ProcessEvent(TPfqosMessage& aMsg);
	void ProcessReply(TPfqosMessage& aMsg);
	TBool Match(TInt aChannelId);
	TBool MatchReply(const TPfqosMessage& aMsg, TUint8 aMsgType);
	TInt OpenExisting();
	TInt SetQoS(CQoSParameters& aPolicy);
	TInt Join(RSocket& aSocket);
	TInt Leave(RSocket& aSocket);
	TInt GetCapabilities(TUint& aCapabilities);
	void NotifyError(TInt aReason);

	inline CQoSParameters& GetPolicy();
	inline TUint Capabilities() const;
	inline EQoSChannelStatus Status() const;
	inline TInt ChannelId() const;
	TInt Close();

private:
	CChannel(CQoSMan* aManager);
//	CChannel(CQoSMan* aManager, CQoSParameters* aSpec);
	void ConstructL(RSocket& aSocket, CQoSParameters* aSpec);
//	void ConstructL(RSocket& aSocket);
	void CreateSelector(TQoSSelector& aSelector, const TPfqosMessage& aMsg);

private:
	EQoSChannelStatus iStatus;
	TInt iChannelId;
	TQoSSelector iRequestSelector;
	TQoSSelector iPendingRequestSelector;
	TPendingStatus iPending;
	TUint iCapabilities;
};

class CPolicy : public CQoSRequestBase
{
public:
	enum TPendingStatus
	{
		ENone,
		EPendingAdd,
		EPendingUpdate,
		EPendingDelete,
		EPendingGet,
		EPendingLoadFile,
		EPendingUnloadFile
	};

	static CPolicy* NewL(CQoSMan* aManager, const TQoSSelector& aSelector);
	~CPolicy();
	void SetQoSL(CQoSParameters& aPolicy);
	void GetQoSL();
	void DeleteL();
	void LoadFileL(const TDesC& aName);
	void UnloadFileL(const TDesC& aName);
	TBool Match(const TQoSSelector& aSelector);
	void ProcessReply(TPfqosMessage& aMsg);
	TBool MatchReply(const TPfqosMessage& aMsg, TUint8 aMsgType);
	void ProcessEvent(TPfqosMessage& aMsg);
	void NotifyError(TInt aReason);
	void Close();

protected:
	CPolicy(CQoSMan* aManager, const TQoSSelector& aSelector);

private:
	TPendingStatus iPending;
	TBool iPolicyCreated;
	TQoSSelector iSelector;
	TUint iCapabilities;
};

//
class CRequest : public CBase
{
public:
	static CRequest* NewL(CQoSRequestBase* aOwner, TUint aBufSize);
	~CRequest();

protected:
	CRequest(CQoSRequestBase* aOwner);
	void ConstructL(TUint aBufSize);

public:
	CPfqosStream* iMsg;
	CQoSRequestBase* iOwner;
	TSglQueLink iLink;
};

class CSender;
class CQoSMan : public CActive
{
public:
	static CQoSMan* NewL(TInt aPriority=0);
	~CQoSMan();
	
	// qos channel methods
	CChannel* OpenQoSChannelL(RSocket& aSocket);
	void RemoveQoSChannel(CChannel* aChannel);
	void SetQoSL(CChannel& aChannel);
	void CreateL(CChannel& aChannel, const TQoSSelector& aSelector);
	void OpenExistingL(CChannel& aChannel, const TQoSSelector& aSelector);
	void JoinL(CChannel& aChannel, const TQoSSelector& aSelector);
	void LeaveL(CChannel& aChannel, const TQoSSelector& aSelector);

	// qos policy methods
	CPolicy* OpenQoSPolicyL(const TQoSSelector& aSelector);
	void RemoveQoSPolicy(CPolicy* aChannel);
	CPolicy* FindPolicy(const TQoSSelector& aSelector);

	void ClearPendingRequest(CQoSRequestBase* aRequest);
	void Send(CRequest* aRequest);
	inline RSocket& Socket();
	inline const TCheckedUid& Uid() const;
	inline void Open();
	void Close();

protected:
	CQoSMan(TInt aPriority=0);
	void ConstructL();

private:
	void Notify(TPfqosMessage& aMsg);
	void Flush();
	CChannel* Match(TPfqosMessage& aMsg);
	CChannel* MatchChannelReply(TPfqosMessage& aMsg, TUint8 aMsgType);
	CPolicy* MatchPolicyReply(TPfqosMessage& aMsg, TUint8 aMsgType);
	void ExecEvent(TPfqosMessage& aMsg);
	void ExecReply(TPfqosMessage& aMsg);
	void RunL();
	void DoCancel();

private:
	TInt iRefCount;
	TBool iNotifyPending;
	TBool iShutdown;
	TPtr8 iRecBuf;
	HBufC8* iBuf;
	RSocketServ iSocketServer;
	RSocket iSocket;
	CSender* iSender;
	TCheckedUid iUid;
	TSglQue<CChannel> iChannels;
	TSglQue<CPolicy> iStaticPolicies;
};

//
class QoSManGlobals
	{
public:
	inline static CQoSMan* Get();
	inline static void Set(CQoSMan* aGlobals);
	};

#include "qoslib_glob.inl"

#endif
