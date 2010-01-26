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
// netupsconnectionentry.pp
// This file defines the CConnectionEntry a structure which
// specifies the number of connections associated with a CommsId. 
// @internalAll
// 
//
	
#include <e32const.h>
#include "netupsconnectionentry.h"
#include "netupsassert.h"

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CConnectionEntry* CConnectionEntry::NewL(const Messages::TNodeId& aCommsId, TInt32 aCount)
	{
	CConnectionEntry* self = new (ELeave) CConnectionEntry(aCommsId, aCount);
	
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	
	return self;		
	}

void CConnectionEntry::ConstructL()
	{
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CConnectionEntry %08x:\tCConnectionEntry()"), this);	
	}
	
CConnectionEntry::~CConnectionEntry()
	{
	__FLOG_1(_L("CConnectionEntry %08x:\t~CConnectionEntry()"), this);
	__FLOG_CLOSE;
	}

void CConnectionEntry::IncrementCount()
	{
	iCount++;
	}
	
void CConnectionEntry::DecrementCount()
	{
	__ASSERT_DEBUG(iCount > 0, User::Panic(KNetUpsPanic, KPanicAttemptToDecrementPastZero));
	--iCount;	
	}

}

