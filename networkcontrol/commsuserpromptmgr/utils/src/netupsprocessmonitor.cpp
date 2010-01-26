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
// netupprocessmonitor.cpp
// This file provides the implementation of the methods for NetUps Process Monitor
// @internalAll
// @prototype
// 
//

#include <e32std.h>
#include <e32base.h>

#include "netupskeys.h"
#include "netupsprocessmonitor.h"
#include "netupsstatemachine.h"
#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"

#include "netupsassert.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CProcessMonitor* CProcessMonitor::NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry)
	{
	CProcessMonitor* self = new (ELeave) CProcessMonitor(aDatabaseEntry, aProcessEntry);

	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	CActiveScheduler::Add(self); 

	return self;	
	}

void CProcessMonitor::ConstructL()
	{
	(void) User::LeaveIfError(iProcess.Open(iProcessEntry.ProcessId()));	

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CProcessMonitor %08x:\tConstructL()"), this);	
	}

CProcessMonitor::CProcessMonitor(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry) : CActive(EPriorityStandard), iDatabaseEntry(aDatabaseEntry), iProcessEntry(aProcessEntry) 
	{
	}

CProcessMonitor::~CProcessMonitor()
	{
	if (IsActive())
		{
		Cancel();
		}
	iProcess.Close();

	__FLOG_1(_L("CProcessMonitor %08x:\t~CProcessMonitor()"), this);	
	__FLOG_CLOSE;	
	}

void CProcessMonitor::Start()
	{
	__FLOG_1(_L("CProcessMonitor %08x:\tStart()"), this);	

	iProcess.Logon(iStatus);
	SetActive();
	}

void CProcessMonitor::RunL()
	{
	__FLOG_1(_L("CProcessMonitor %08x:\tRunL()"), this);	

	TProcessKey processKey(iDatabaseEntry,iProcessEntry);
	iProcessEntry.UpsStateMachine().HandleProcessTermination(processKey); 
	// Consider passing the iStatus as an error code
	// Log iStatus, consider error path - what happens if KErrGeneral, or other

	delete this;
	}

void CProcessMonitor::RunErrorL()
	{
	__FLOG_1(_L("CProcessMonitor %08x:\t RunErrorL()"), this);	

	// Handles Error in RunL(). We don't allocate any memory in the
	// RunL, so this method should never be called.
	
	}

void CProcessMonitor::DoCancel()
	{
	__FLOG_1(_L("CProcessMonitor %08x:\tDoCancel()"), this);	

	TInt rc = iProcess.LogonCancel(iStatus);
	// Log Error if != KErrNone, as should not call this method
	__FLOG_3(_L("CProcessMonitor %08x:\tDoCancel() iStatus = %d, rc = %d"), this, iStatus.Int(), rc);	
	__ASSERT_DEBUG((rc == KErrNone), User::Panic(KNetUpsPanic, KPanicProcessLogonCancelInvalid));
	}
} // end of namespace NetUps
