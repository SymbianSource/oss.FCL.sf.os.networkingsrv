// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @internalComponent
*/
#ifndef __FLOWHOOK_H__
#define __FLOWHOOK_H__

#include <e32std.h>
#include <e32base.h>
#include <flow.h>		// ..at least for MFlowHook

// Forward declarations
class CPolicySelector;
class CInterface;
class RModule;
class CQoSConn;

class TQoSPolicy
	{
public:
	TQoSPolicy() : iPolicy(NULL), iDefault(NULL), iOverride(NULL), iModules(NULL) {}

public:
	const CPolicySelector* iPolicy;
	const CPolicySelector* iDefault;
	const CPolicySelector* iOverride;
	// Should also be const, but some other declarations prevent this for the time being...
	CModuleSelector* iModules;
	};

class CQoSSessionBase;
class CProtocolQoS;
class CInternalQoSChannel;


class CFlowHook : public CBase, public MFlowHook
	{
public:
	CFlowHook(CProtocolQoS &aProtocol, CFlowContext &aContext);
	void Open();
	TInt ReadyL(TPacketHead &);
	TInt ApplyL(RMBufSendPacket &, RMBufSendInfo &);
	void Close();
	void ReleasePolicy(const CPolicySelector* aSel);
	void UpdateQoS();
	void SetQoS(const TQoSParameters& aSpec);

	void ClearPendingRequest(TInt aError);
	TInt StartPendingRequest(CQoSSessionBase* iSession);

	void CloseQoS();
	void RestartQoS();	// CloseQoS + trigger immeadiate ReadyL

	inline void SetUid(const TUidType& aUid);
	inline const TCheckedUid& Uid() const;
	inline TQoSPolicy& Policies();
	inline CInterface* Interface();
	inline CFlowContext& Context();
	inline CProtocolQoS& Protocol();
	inline const TQoSParameters& QoSParameters() const;

	inline RPointerArray<RModule>& ModuleList() { return iModuleList;}
	void Block();
	void UnBlock();
	inline void SetQoSChannel(CQoSConn* aChannel);
	inline CQoSConn* Channel();
	inline TBool ChannelJoined();
	inline void SetChannelJoined(TBool aJoined);

	void FillFlowInfo(TPfqosMessage& aMsg, pfqos_address& aSrc, pfqos_address& aDst, pfqos_selector& aSel, TInetAddr &aMask);
private:
	virtual ~CFlowHook();	// reference counted object, only Close() can issue the delete.

	void LoadModulesL();
	void OpenModulesL();
	TInt Negotiate();
	void AdjustForHeaderMode(TQoSParameters& aQoS);

	CProtocolQoS&			iProtocol;	// immutable after construction
	CFlowContext&			iContext;	// immutable after construction

	TCheckedUid				iUid;	//?? Why TCheckedUid instead of plain TUidType?
	TQoSParameters			iQoS;
	CInterface*				iInterface;
	TQoSPolicy				iQoSPolicies;
	TInt					iRefCount;
	TInt					iFlowStatus;
	CQoSSessionBase*		iSession;
	TUint					iChannelJoined:1;		// = 1, when flow has joined the channel (only if iChannel)
	TUint					iHasControlModule:1;	// = 1, if flow has control module at iModuleList[0]
	TUint					iQoSChanged:1;			// = 1, if flow specific QoS needs negotiation
	CQoSConn*				iChannel;
	RPointerArray<RModule>	iModuleList;
public:	// Only for CProtocolQoS really...
	TDblQueLink				iHookLink;
	};

#include "flowhook.inl"

#endif
