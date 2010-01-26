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
// This file provides the interface specification for the Net Ups Process Entry.
// @internalAll
// @prototype
// 
//

#ifndef NETUPSPROCESSENTRY_H
#define NETUPSPROCESSENTRY_H

#include <e32base.h>									// defines CBase
#include <e32std.h>										// defines TProcessId

#include "netupsstatedef.h"								// defines TNetUpsState

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
class CThreadEntry;
class CProcessMonitor;
class CUpsStateMachine;

NONSHARABLE_CLASS(CProcessEntry) : public CBase
{
public:
	static CProcessEntry* NewL(CDatabaseEntry& aDatabaseEntry, const TProcessId& aProcessId, CUpsStateMachine& aUpsStateMachine);
	virtual ~CProcessEntry();

	void							SetNetUpsState(TNetUpsState aNetUpsState);
	TNetUpsState					NetUpsState() const;
	RPointerArray<CThreadEntry>& 	ThreadEntry();
	void							SetProcessMonitor(CProcessMonitor* aProcessMonitor);
	CProcessMonitor* 				ProcessMonitor() const; // client code tests for Null Pointer
	const TProcessId&						ProcessId() const;
	CUpsStateMachine&				UpsStateMachine() const;
protected: // class may be specialised by NetUps clients if required
	CProcessEntry(const TProcessId& aProcessId, CUpsStateMachine& aUpsStateMachine);		
private:
	void ConstructL(CDatabaseEntry& aDatabaseEntry);
private:
	TNetUpsState					iNetUpsState;
	RPointerArray<CThreadEntry> 	iThreadEntry;
	CProcessMonitor* 				iProcessMonitor;// object life cycle may be shorter than CThreadEntry
	TProcessId						iProcessId; 
	CUpsStateMachine&				iUpsStateMachine;
private:
	__FLOG_DECLARATION_MEMBER;		
	}; 
	
} // end of namespace

#endif // NETUPSPROCESSENTRY_H

