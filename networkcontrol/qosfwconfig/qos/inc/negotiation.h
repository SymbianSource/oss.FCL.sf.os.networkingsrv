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
#ifndef __NEGOTIATION_H__
#define __NEGOTIATION_H__

#include <e32std.h>
#include <e32base.h>

#include "module_if.h"		// ..MQoSNegotiateEvent is used as a base class of CNegotiateItem
#include "qosparameters.h"	// ..TQoSParameters is a member variable!
#include "pfqoslib.h"		// ..TPfqosMessage is a member variable!

class CNegotiateItem;
class CInternalQoSChannel;
class RModule;
class MQoSNegotiateEvent;
class CFlowHook;
class TExtensionData;

// Base class for QoS sessions
class CQoSSessionBase : public CBase
	{
public:
	CQoSSessionBase(CFlowHook& aHook, TInt aChannelId);
	~CQoSSessionBase();
	virtual void Run();
	virtual void SubRequestComplete(TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension, CNegotiateItem* aItem);

protected:
	void DeliverEvent(TUint16 aEvent);
	void FatalError(TInt aErrorCode);
	CInternalQoSChannel* Channel() const;

	virtual TInt DoCall(RModule &aModule, MQoSNegotiateEvent& aCallback) = 0;
	virtual void RequestComplete() = 0;
private:
	void Proceed();

protected:
	CFlowHook& iHook;						// Flow hook owner of this session
	RPointerArray<RModule>& iModuleList;	// The modules to be run (from the flow hook)
	const TInt iChannelId;					// > 0, if session associates with a channel

	TInt iError;							// Error code
	TBool iFatalError;						// If set, stop negotiation
	TUint16 iValue;							// Return value
	TInt iCurrent;							// The current module to be run (0..)
	TInt iProceed;							// Non-zero when control in Proceed()
	TQoSParameters iNegotiated;				// 
	TPfqosMessage iMsg;						// PF_QOS reply msg
	TSglQue<CNegotiateItem> iPending;		// Pending request queue
	};

// Negotiate session
class CNegotiateSession : public CQoSSessionBase
	{
public:
	CNegotiateSession(CFlowHook& aHook);
	~CNegotiateSession();

private:
	TInt DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback);
	void RequestComplete();
	};


// Create channel session
class CCreateChannelSession : public CQoSSessionBase
	{
public:
	CCreateChannelSession(CFlowHook& aHook, TInt aChannelId);
	~CCreateChannelSession();

private:
	TInt DoCall(RModule &aModule, MQoSNegotiateEvent& aCallback);
	void RequestComplete();
	};


// Negotiate channel session
class CNegotiateChannelSession : public CQoSSessionBase
	{
public:
	CNegotiateChannelSession(CFlowHook& aHook, TInt aChannelId);
	~CNegotiateChannelSession();

private:
	TInt DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback);
	void RequestComplete();
	};

// Join session
class CJoinSession : public CQoSSessionBase
	{
public:
	CJoinSession(CFlowHook& aHook, TInt aChannelId);
	~CJoinSession();

private:
	TInt DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback);
	void RequestComplete();
	};

// Leave session
class CLeaveSession : public CQoSSessionBase
	{
public:
	CLeaveSession(CFlowHook& aHook, TInt aChannelId);
	~CLeaveSession();

private:
	TInt DoCall(RModule& aModule, MQoSNegotiateEvent& aCallback);
	void RequestComplete();
	};


class CNegotiateItem : public CBase, public MQoSNegotiateEvent
	// BEWARE:
	//	Once you start the negotiation and pass a reference of MQosNegotiateEvent
	//	to the module, there is no way to cancel this request. This object MUST exist
	//	until the RequestComplete is called!	
	{
public:
	~CNegotiateItem();
	CNegotiateItem(CQoSSessionBase* aSession, TUint aFlags);
	void Kill();
	void RequestComplete(TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension);
	inline TUint Flags() const { return iFlags; };

	TSglQueLink iLink;
private:
	TUint				iFlags;		// Module flags
	CQoSSessionBase*	iSession;	// Session (or NULL, when ZOMBIE)
	};

#endif
