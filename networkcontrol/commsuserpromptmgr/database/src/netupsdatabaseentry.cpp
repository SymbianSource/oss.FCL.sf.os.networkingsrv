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
// This file provides the implementation for Net Ups Database
// @internalAll
// @prototype
// 
//

#include "e32base.h"	// defines CleanupStack

#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CDatabaseEntry::CDatabaseEntry(TInt32 aServiceId) : iServiceId(aServiceId)
	{
	}

CDatabaseEntry* CDatabaseEntry::NewL(TInt32 aServiceId)
	{
	CDatabaseEntry* self = new (ELeave) CDatabaseEntry(aServiceId);

	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	return self;	
	}

void CDatabaseEntry::ConstructL()
	{	
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_2(_L("CDatabaseEntry %08x:\tConstructL(), iServiceId = %d"), this, iServiceId);		
	}

CDatabaseEntry::~CDatabaseEntry()
	{
	__FLOG_2(_L("CDatabaseEntry %08x:\t~CDatabaseEntry(), iServiceId = %d"), this, iServiceId);		

	for (TInt j = iProcessEntry.Count() - 1; j >= 0; --j)
		{
		CProcessEntry* processEntry = iProcessEntry[j];
		__FLOG_3(_L("CDatabaseEntry %08x:\t~CDatabaseEntry(), j = %d, processEntry = %08x"), this, j, processEntry);		
		delete processEntry;		
		}
	iProcessEntry.Reset();
	iProcessEntry.Close();	

	__FLOG_CLOSE;	
	}

TInt32 CDatabaseEntry::ServiceId()
	{
	return iServiceId;	
	}

RPointerArray<CProcessEntry>& CDatabaseEntry::ProcessEntry(void)
	{
	return 	iProcessEntry;
	}

} // end of namespace

