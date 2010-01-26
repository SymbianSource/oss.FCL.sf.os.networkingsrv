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
// This file provides the implementation for the process entry.
// @internalComponent
// @prototype
// 
//

#include "e32base.h"				// defines CleanupStack, Active Scheduler

#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"
#include "netupsthreadentry.h"
#include "netupsprocessmonitor.h"
#include "netupsstatedef.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility


namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CProcessEntry* CProcessEntry::NewL(CDatabaseEntry& aDatabaseEntry, const TProcessId& aProcessId, CUpsStateMachine& aUpsStateMachine)
	{
	CProcessEntry* self = new (ELeave) CProcessEntry(aProcessId, aUpsStateMachine);

	CleanupStack::PushL(self);
	self->ConstructL(aDatabaseEntry);
	CleanupStack::Pop(self);

	return self;
	}

CProcessEntry::~CProcessEntry()
	{
	__FLOG_1(_L("CProcessEntry %08x:\t~CProcessEntry()"), this);


	for (TInt j = iThreadEntry.Count() - 1; j >= 0; --j)
		{
		CThreadEntry* threadEntry = iThreadEntry[j];
		__FLOG_3(_L("CProcessEntry %08x:\t~CProcessEntry(), j = %d, threadEntry = %08x"), this, j, threadEntry);
		delete threadEntry;		
		}
		
	iThreadEntry.Reset();
	iThreadEntry.Close();

	delete iProcessMonitor;

	__FLOG_CLOSE;	
	}

void CProcessEntry::ConstructL(CDatabaseEntry& aDatabaseEntry)
	{
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_3(_L("CProcessEntry %08x:\tConstructL(), iNetUpsState = %d, process id = %d"), this, iNetUpsState, iProcessId.Id());

	iProcessMonitor = CProcessMonitor::NewL(aDatabaseEntry, *this);
	}

CProcessEntry::CProcessEntry(const TProcessId& aProcessId, CUpsStateMachine& aUpsStateMachine)
  : iNetUpsState(ENull), iProcessMonitor(NULL), iProcessId(aProcessId), iUpsStateMachine(aUpsStateMachine)
	{
	}

void CProcessEntry::SetNetUpsState(TNetUpsState aNetUpsState)
	{
	iNetUpsState = aNetUpsState;
	}

TNetUpsState CProcessEntry::NetUpsState() const
	{
	return iNetUpsState;	
	}

CUpsStateMachine& CProcessEntry::UpsStateMachine() const
	{
	return iUpsStateMachine;
	}

RPointerArray<CThreadEntry>& CProcessEntry::ThreadEntry()
	{
	return iThreadEntry;
	}

void CProcessEntry::SetProcessMonitor(CProcessMonitor* aProcessMonitor)
	{
	iProcessMonitor = aProcessMonitor;	
	__FLOG_4(_L("CProcessEntry %08x:\t SetProcessMonitor(), iNetUpsState = %d, iProcessMonitor = %08x, process id = %d"), this, iNetUpsState, iProcessMonitor, iProcessId.Id());

	}

CProcessMonitor* CProcessEntry::ProcessMonitor() const
	{
	return iProcessMonitor;
	}

const TProcessId&	CProcessEntry::ProcessId() const
	{
	return iProcessId;	
	}

} // end of namespace

