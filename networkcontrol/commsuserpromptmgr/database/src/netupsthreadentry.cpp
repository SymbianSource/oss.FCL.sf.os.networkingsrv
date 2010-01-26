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
// This file provides the implementation for the thread entry.
// @internalComponent
// @prototype
// 
//

#include "e32base.h"				// defines CleanupStack

#include "netupsthreadentry.h"
#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"
#include "netupssubsession.h"
#include "netupsthreadmonitor.h"
#include "netupspolicycheckrequestqueue.h"
#include "netupsassert.h"

#include <ups/upsclient.h>

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CThreadEntry* CThreadEntry::NewL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, const TThreadId& aThreadId, UserPromptService::RUpsSession& aUpsSession)
	{
	CThreadEntry* self = new (ELeave) CThreadEntry(aThreadId);

	CleanupStack::PushL(self);
	self->ConstructL(aDatabaseEntry, aProcessEntry, aUpsSession);
	CleanupStack::Pop(self);

	return self;
	}

CThreadEntry::CThreadEntry(const TThreadId& aThreadId) :  iIsDead(EFalse), iThreadId(aThreadId), iThreadMonitor(NULL)
	{
	}
	
void CThreadEntry::ConstructL(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, UserPromptService::RUpsSession& aUpsSession)
	{
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);

	iSubSession 	= CSubSession::NewL(aUpsSession, aDatabaseEntry, aProcessEntry, *this);
	iQueue 			= CPolicyCheckRequestQueue::NewL(*iSubSession); 
	iThreadMonitor 	= CThreadMonitor::NewL(aDatabaseEntry, aProcessEntry, *this);

	__FLOG_6(_L("CThreadEntry %08x:\tConstructL(), iIsDead = %d, thread id = %d, iThreadMonitor = %08x, iSubSession = %08x, iQueue = %08x"), this, iIsDead, iThreadId.Id(), iThreadMonitor, iSubSession, iQueue);	
	}

CThreadEntry::~CThreadEntry() 
	{
	__FLOG_1(_L("CThreadEntry %08x:\t~CThreadEntry()"), this);

	for (TInt i = iConnectionEntry.Count() - 1; i >=0; --i )
		{
		delete iConnectionEntry[i];
		}	
	iConnectionEntry.Reset();
	iConnectionEntry.Close();

	delete	iThreadMonitor;
	delete	iSubSession;
	delete  iQueue;

	__FLOG_CLOSE;	
	}

TBool CThreadEntry::FindConnectionEntry(const Messages::TNodeId& aCommsId, TInt32& aIndex)
	{
	TBool found = EFalse;
	for (TInt i = iConnectionEntry.Count() - 1; i >=0; --i)
		{
		if (iConnectionEntry[i]->CommsId() == aCommsId)
			{
			aIndex = i;
			found = ETrue;
			break;
			}
		}
		
	return found;	
	}

void CThreadEntry::AddCommsIdL(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\t AddCommsIdL(), aCommsId = %d"), this, aCommsId.Printable());
	__FLOG_1(_L("CThreadEntry %08x:\tAddCommsIdL()"), this);

	TInt32 index = 0;
	TBool found = FindConnectionEntry(aCommsId, index);
		
	if (found == EFalse)
		{
		CConnectionEntry* connectionEntry = CConnectionEntry::NewL(aCommsId, (TInt) 0);
		CleanupStack::PushL(connectionEntry);
		iConnectionEntry.AppendL(connectionEntry);
		CleanupStack::Pop(connectionEntry);
		}		
	}

TBool CThreadEntry::RemoveCommsId(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\tRemoveCommsId(), aCommsId %08x"), this, aCommsId);
	__FLOG_1(_L("CThreadEntry %08x:\tRemoveCommsId()"), this);

	TInt32 index = 0;
	TBool found = FindConnectionEntry(aCommsId, index);

	TBool commsIdRemoved = EFalse;
	if (found && (iConnectionEntry[index]->Count() == 0))
		{
		delete iConnectionEntry[index];
		iConnectionEntry[index] = 0;
		iConnectionEntry.Remove(index);			
		commsIdRemoved = ETrue;
		}

	return commsIdRemoved;
	}

void CThreadEntry::IncrementConnectionCount(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\t IncrementConnectionCount(), aCommsId = %d"), this, aCommsId.Printable());
	__FLOG_1(_L("CThreadEntry %08x:\t IncrementConnectionCount()"), this);

	TInt32 index = 0;
	TBool found = FindConnectionEntry(aCommsId, index);
		
	if (found == EFalse)
		{
		User::Panic(KNetUpsPanic, KPanicInvalidLogic);					
		}	
	else
		{
		iConnectionEntry[index]->IncrementCount();	
		}
	}

void CThreadEntry::IncrementConnectionCountL(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\t IncrementConnectionCount(), aCommsId = %d"), this, aCommsId.Printable());
	__FLOG_1(_L("CThreadEntry %08x:\tIncrementConnectionCountL()"), this);

	TInt32 index = 0;
	TBool found = FindConnectionEntry(aCommsId, index);
		
	if (found == EFalse)
		{
		CConnectionEntry* connectionEntry = CConnectionEntry::NewL(aCommsId, (TInt) 1);
		CleanupStack::PushL(connectionEntry);
		iConnectionEntry.AppendL(connectionEntry);
		CleanupStack::Pop(connectionEntry);
		}
	else
		{
		iConnectionEntry[index]->IncrementCount();
		}		
	}

void CThreadEntry::DecrementConnectionCount(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\t DecrementConnectionCount(), aCommsId = %d"), this, aCommsId.Printable());
	__FLOG_1(_L("CThreadEntry %08x:\tDecrementConnectionCount()"), this);

	TInt32 index = 0;
	TBool found = FindConnectionEntry(aCommsId, index);

	if (found == EFalse)
		{
		User::Panic(KNetUpsPanic, KPanicInvalidLogic);
		}
	else
		{
		iConnectionEntry[index]->DecrementCount();
		if (iConnectionEntry[index]->Count() == 0)
			{
			delete iConnectionEntry[index];
			iConnectionEntry[index] = 0;
			iConnectionEntry.Remove(index);
			}
		}		
	}

TInt32 CThreadEntry::ConnectionCount()
	{
	__FLOG_1(_L("CThreadEntry %08x:\tConnectionCount()"), this);

	TInt32 count = 0;
	
	for (TInt i = iConnectionEntry.Count() - 1; i >=0; --i )
		{
		count+= iConnectionEntry[i]->Count();
		}	

	__FLOG_2(_L("\tcount = %d"), this, count);
	
	return count;
	}

TInt32	CThreadEntry::ConnectionCount(const Messages::TNodeId& aCommsId)
	{
	//__FLOG_2(_L("CThreadEntry %08x:\t ConnectionCount(), aCommsId = %d"), this, aCommsId.Printable());
	__FLOG_1(_L("CThreadEntry %08x:\t ConnectionCount()"), this);

	TInt32 index = 0;
	TInt32 count = 0;
	TBool found = FindConnectionEntry(aCommsId, index);

	if (found == EFalse)
		{
		User::Panic(KNetUpsPanic, KPanicInvalidLogic);
		}
	else
		{
		count = iConnectionEntry[index]->Count();			
		}	

	return count;
	}

RPointerArray<CConnectionEntry>& CThreadEntry::ConnectionEntry()
	{
	return iConnectionEntry;	
	}

const TThreadId& CThreadEntry::ThreadId() const
	{
	return iThreadId;
	}

void CThreadEntry::SetIsDead(TBool aDead)
	{
	iIsDead = aDead;
	}

TBool CThreadEntry::IsDead() const
	{
	return iIsDead;
	}

void CThreadEntry::SetThreadMonitor(CThreadMonitor* aThreadMonitor)
	{
	iThreadMonitor = aThreadMonitor;	
	__FLOG_6(_L("CThreadEntry %08x:\t SetThreadMonitor(), iIsDead = %d, thread id = %d, iThreadMonitor = %08x, iSubSession = %08x, iQueue = %08x"), this, iIsDead, iThreadId.Id(), iThreadMonitor, iSubSession, iQueue);	
	}

CThreadMonitor* CThreadEntry::ThreadMonitor() const
	{
	return iThreadMonitor;
	}

void CThreadEntry::SetSubSession(CSubSession* aSubSession)
	{
	iSubSession = aSubSession;	
	__FLOG_6(_L("CThreadEntry %08x:\t SetSubSession(), iIsDead = %d, iThreadId.Id() = %d, iThreadMonitor = %08x, iSubSession = %08x, iQueue = %08x"), this, iIsDead, iThreadId.Id(), iThreadMonitor, iSubSession, iQueue);
	}

CSubSession* CThreadEntry::SubSession() const
	{
	return iSubSession;
	}

CPolicyCheckRequestQueue& CThreadEntry::RequestQueue() const
	{
	return *iQueue;
	}

} // end of namespace
