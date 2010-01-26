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
// This file provides the implementation of the methods for NetUps Thread Monitor
// @internalAll
// @prototype
// 
//

#include "e32std.h"
#include "e32base.h"				// Active Scheduler

#include "netupsthreadmonitor.h"

#include "netupsstatemachine.h"
#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"
#include "netupsthreadentry.h"
#include "netupsassert.h"
#include "netupskeys.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

class CThreadEntry;

CThreadMonitor* CThreadMonitor::NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry& aThreadEntry)
	{
	CThreadMonitor* self = new (ELeave) CThreadMonitor(aDatabaseEntry, aProcessEntry, aThreadEntry);

	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	CActiveScheduler::Add(self); 

	return self;	
	}

void CThreadMonitor::ConstructL()
	{
	(void) User::LeaveIfError(iThread.Open(iThreadEntry.ThreadId()));	

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CThreadMonitor %08x:\t ConstructL()"), this);	
	}

CThreadMonitor::CThreadMonitor(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry& aThreadEntry) : CActive(EPriorityStandard), iDatabaseEntry(aDatabaseEntry), iProcessEntry(aProcessEntry), iThreadEntry(aThreadEntry)
	{
	}

CThreadMonitor::~CThreadMonitor()
	{
	if (IsActive())
		{
		Cancel();
		}
	iThread.Close();		

	__FLOG_1(_L("CThreadMonitor %08x:\t~CThreadMonitor()"), this);	
	__FLOG_CLOSE;	
	}

void CThreadMonitor::Start()
	{
	__FLOG_1(_L("CThreadMonitor %08x:\tStart()"), this);	
	Thread().Logon(iStatus);
	SetActive();
	}

void CThreadMonitor::RunL()
	{
	__FLOG_1(_L("CThreadMonitor %08x:\tRunL()"), this);	

	TThreadKey threadKey(iDatabaseEntry, iProcessEntry, iThreadEntry);
	iProcessEntry.UpsStateMachine().HandleThreadTermination(threadKey); 
	// Consider passing the iStatus as an error code
	// Log iStatus, consider error path - what happens if KErrGeneral, or other
	delete this;
	}

void CThreadMonitor::RunErrorL()
	{
	__FLOG_1(_L("CThreadMonitor %08x:\tRunErrorL()"), this);	

	// Handles leaves in CTHreadMonitor::RunL().
	// We don't allocate memory in the RunL(), so this method should never be called.
	}

void CThreadMonitor::DoCancel()
	{
	__FLOG_1(_L("CThreadMonitor %08x:\tDoCancel()"), this);	

	TInt rc = iThread.LogonCancel(iStatus);
	__FLOG_3(_L("CThreadMonitor %08x:\t DoCancel() iStatus = %d, rc = %d"), this, iStatus.Int(), rc);		
	}

const RThread& CThreadMonitor::Thread() const
	{
	return iThread;	
	}

} // end of namespace
