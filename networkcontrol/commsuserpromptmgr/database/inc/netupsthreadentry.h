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
// This file provides the Net Ups Database SubEntry.
// @internalAll
// @prototype
// 
//

#ifndef NETUPSTHREADENTRY_H
#define NETUPSTHREADENTRY_H

#include <e32base.h>						// defines CBase
#include <e32std.h>							// defines TThreadId

#include <comms-infras/ss_activities.h>

#include <ups/upsclient.h>					// defines the UPS SubSession			

#include <comms-infras/commsdebugutility.h> // defines the comms debug logging utility

#include "netupsconnectionentry.h"		// defines a structure which specifies the number of connections associated with a Comms Id.

namespace NetUps
{
class CDatabaseEntry;
class CProcessEntry;
class CThreadMonitor;
class CSubSession;
class CPolicyCheckRequestQueue;

NONSHARABLE_CLASS(CThreadEntry) : public CBase
	{
public:
	static CThreadEntry* NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, const TThreadId& aThreadId, UserPromptService::RUpsSession& aUpsSession);
	virtual ~CThreadEntry();
public:
	TInt32							 ConnectionCount();
	TInt32							 ConnectionCount(const Messages::TNodeId& aCommsId);	
	void							 IncrementConnectionCount(const Messages::TNodeId& aCommsId);
	void							 IncrementConnectionCountL(const Messages::TNodeId& aCommsId);
	void							 DecrementConnectionCount(const Messages::TNodeId& aCommsId);
	void							 AddCommsIdL(const Messages::TNodeId& aCommsId);
	TBool							 RemoveCommsId(const Messages::TNodeId& aCommsId);
	const TThreadId&				 ThreadId() const;
	void							 SetIsDead(TBool aDead);
	TBool							 IsDead() const;
	void 							 SetThreadMonitor(CThreadMonitor* aThreadMonitor);	
	CThreadMonitor* 				 ThreadMonitor() const; // client code tests for Null pointer
	void							 SetSubSession(CSubSession* aSubSession);
	CSubSession*					 SubSession() const;	// client code tests for Null pointer
	CPolicyCheckRequestQueue& 		 RequestQueue() const;
	RPointerArray<CConnectionEntry>& ConnectionEntry();
protected: // class may be specialised by NetUps clients if required
	CThreadEntry(const TThreadId&);
private:
	TBool FindConnectionEntry(const Messages::TNodeId& aCommsId, TInt32& index);
	void ConstructL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, UserPromptService::RUpsSession& aUpsSession);
private:
	TBool							iIsDead;
	TThreadId						iThreadId;	
	CThreadMonitor* 				iThreadMonitor;	// object life cycle may be shorter than CThreadEntry
	CSubSession*					iSubSession;	// object life cycle may be shorter than CThreadEntry	
	CPolicyCheckRequestQueue* 		iQueue;			// object life cycle may be shorter than CThreadEntry	
	RPointerArray<CConnectionEntry> iConnectionEntry;
private:
	__FLOG_DECLARATION_MEMBER;		
	};

} // end of namespace NetUps

#endif // NETUPSTHREADENTRY_H
