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
// This file provides the implementation of the methods for CNetUpsImpl
// @internalAll
// @prototype
// 
//

#include <e32std.h>							// defines User
#include <e32cmn.h>							// defines RPointerArray
#include <e32def.h>							// defines ASSERT
#include <e32base.h>						// defines cleanup stack

#include "netupsassert.h"					// defines Net Ups Asserts

#include "netupsimpl.h"

#include "netupsdatabaseentry.h"			// defines the generic NET UPS database
#include "netupsthreadentry.h"				// defines the NET UPS database thread entry
#include "netupssubsession.h"				// defines the NET UPS Subsession
#include "netupsstatemachine.h" 			// defines generic NET UPS state machine
#include "netupsstatedef.h"					// defines internal UPS States
#include "netupspolicycheckrequestqueue.h"	// defines the policy check request queue
#include "netupskeys.h"						// defines the keys which access an entry in the Net UPS database

#include <comms-infras/commsdebugutility.h> // defines the comms debug logging utility

using namespace UserPromptService;
using namespace ESock;

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/
	
CNetUpsImpl* CNetUpsImpl::NewL() 
	{
	CNetUpsImpl* self = new (ELeave) CNetUpsImpl;
		
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	return self;
	}

CNetUpsImpl::CNetUpsImpl() : iLifeTimeMode(EProcessLifeTimeMode), iRefCount(0) //, iRequestId(0)
	{
	}

void CNetUpsImpl::ConstructL() 
	{
	TInt rc = iUpsSession.Connect();
	(void) User::LeaveIfError(rc);

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CNetUpsImpl %08x:\tConstructL()"), this);		
	}
	
CNetUpsImpl::~CNetUpsImpl()
	{
	__FLOG_1(_L("CNetUpsImpl %08x:\t~CNetUpsImpl()"), this);		

	for (TInt i = iDatabaseEntry.Count() - 1; i >= 0; --i)
		{
		delete iDatabaseEntry[i];
		}
	iDatabaseEntry.Reset();	
	iDatabaseEntry.Close();

	for (TInt j = iStateMachine.Count() - 1; j >= 0; --j) 
		{
		delete iStateMachine[j];
		}
	iStateMachine.Reset();
	iStateMachine.Close();

	iUpsSession.Close();

	__FLOG_CLOSE;	
	}

void CNetUpsImpl::SetLifeTimeMode(TLifeTimeMode aLifeTimeMode)
	{
	__FLOG_2(_L("CNetUpsImpl %08x:\tSetLifeTimeMode(), aLifeTimeMode = %d"), this, aLifeTimeMode);		
	iLifeTimeMode = aLifeTimeMode;	
	}

CNetUpsImpl::TLifeTimeMode CNetUpsImpl::LifeTimeMode()
	{
	return iLifeTimeMode;
	}

void CNetUpsImpl::ProcessPolicyCheckRequestL(const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId& aRequestId)
	{
	__FLOG_1(_L("CNetUpsImpl %08x:\tProcessPolicyCheckRequestL()"), this);		

	CUpsStateMachine& upsStateMachine = IdentifyStateMachineL(aPolicyCheckRequestData.iServiceId);
	
	CDatabaseEntry* databaseEntry	= NULL;
	CProcessEntry* 	processEntry	= NULL;
	CThreadEntry*  	threadEntry  	= NULL;		

	IdentifyDatabaseEntryL(aPolicyCheckRequestData, databaseEntry, processEntry, threadEntry); 
	__FLOG_1(_L("\tdatabaseEntry = %08x"), databaseEntry);		
	__ASSERT_DEBUG((databaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl));
	__FLOG_1(_L("\tprocessEntry = %08x"), processEntry);		
	__ASSERT_DEBUG((processEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl1));
	// threadEntry will be NULL if in process lifetime.

	aRequestId = AllocateRequestId();

	TNetUpsState netUpsState = processEntry->NetUpsState();	
	if (MapInternalStateToSessionMode(netUpsState) == ESessionMode)
		{
		// Already in Session Yes or Session No, no need to query UPS Server
		TNetUpsDecision netUpsDecision = ESessionNo;
		DetermineUpsDecision(netUpsState, netUpsDecision);
		if ((netUpsDecision == ESessionYes) && (iLifeTimeMode == ENetworkLifeTimeMode))
			{
			threadEntry->IncrementConnectionCountL(aPolicyCheckRequestData.iCommsId);
			}

		TInt rc = aPolicyCheckRequestData.iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(netUpsDecision, aPolicyCheckRequestData.iCommsId);			
		__FLOG_2(_L("CNetUpsImpl %08x:\tProcessPolicyCheckRequestL(), error = %d"), this , rc );
		}
	else
		{
		__FLOG_1(_L("\tthreadEntry = %08x"), threadEntry);		
		__ASSERT_DEBUG((threadEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl2));		
		TThreadKey threadKey(*databaseEntry, *processEntry, *threadEntry);
		upsStateMachine.ProcessPolicyCheckRequestL(threadKey, aPolicyCheckRequestData, aRequestId);
		}
	}


TInt CNetUpsImpl::CancelRequest(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, const TRequestId& aRequestId)
	{		
//	__FLOG_4(_L("CNetUpsImpl %08x:\tCancelRequest(), aServiceId = %d, aCallersNodeId = %08x, requestId = %d"), this, aServiceId, aCallersNodeId, aRequestId);		
	__FLOG_3(_L("CNetUpsImpl %08x:\tCancelRequest(), aServiceId = %d, requestId = %d"), this, aServiceId, aRequestId);		


	CThreadEntry* threadEntry = NULL;
	TInt rc =  IdentifyDatabaseEntry(aServiceId, aCallersNodeId, threadEntry);
	
	if ((rc == KErrNone) && (threadEntry->SubSession()->IsActive()))
		{
		rc = threadEntry->RequestQueue().CancelRequest(aCallersNodeId, aRequestId);
		}
			
	return rc;
	}

CUpsStateMachine& CNetUpsImpl::IdentifyStateMachineL(TInt32 aServiceId)
	{
	__FLOG_2(_L("CNetUpsImpl %08x:\tIdentifyStateMachineL(), aServiceId = %d"), this, aServiceId);		
	CUpsStateMachine* upsStateMachine = NULL;
	const TInt count = iStateMachine.Count();
	for (TInt i = 0; i < count; ++i)
		{
		const TInt32 serviceId = iStateMachine[i]->ServiceId();
		if (serviceId == aServiceId)
			{
			upsStateMachine = iStateMachine[i];
			break;			
			}	
		}

	if (upsStateMachine == NULL)
		{
		upsStateMachine = CUpsStateMachine::NewL(*this, aServiceId);
		CleanupStack::PushL(upsStateMachine);
		iStateMachine.AppendL(upsStateMachine);
		CleanupStack::Pop(upsStateMachine);
		}		
	return *upsStateMachine;		
	}
		
void CNetUpsImpl::IdentifyDatabaseEntryL(const TPolicyCheckRequestData& aPolicyCheckRequestData, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry) 
/** Searches the database to locate the entities associated with the policy check request. If the entities
are not found then they are instantiated.

@param aPolicyCheckRequestData The policy check request which represents the search key.
@param aDatabaseEntry			An output parameter which contains a pointer to the database entry associated with the search key. It must be NULL on entry to this method.	
@param aProcessEntry			An output parameter which contains a pointer to the process entry associated with the search key. It must be NULL on entry to this method.
@param aThreadEntry				An output parameter which contains a pointer to the threadEntry associated with the search key, or NULL if there is no associated threadEntry.
								The latter is applicable when the processEntry is in process life time mode.
								This parameter must be NULL on entry to this method.
*/
	{
	__FLOG_4(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntriesL(), aDatabaseEntry = %08x, aProcessEntry= %08x, aThreadEntry = %08x"), this, aDatabaseEntry, aProcessEntry, aThreadEntry);		

	TBool serviceIdFound;
	TBool processEntryFound;
	TBool threadEntryFound; 
	serviceIdFound 		= EFalse;
	processEntryFound 	= EFalse;
	threadEntryFound 	= EFalse;
	TBool threadEntryRequired 	= ETrue;			// if ETrue, thread entry must be present, otherwise must be created

	__ASSERT_DEBUG((aDatabaseEntry == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));				
	__ASSERT_DEBUG((aProcessEntry  == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));					
	__ASSERT_DEBUG((aThreadEntry  == NULL),  User::Panic(KNetUpsPanic,  KPanicNonNullPointerToBeOverwritten));					

	const TInt32 databaseEntryCount = iDatabaseEntry.Count();
	for (TInt i = 0; i < databaseEntryCount; ++i)
		{
		__ASSERT_DEBUG((iDatabaseEntry[i] != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl3));				
		if (iDatabaseEntry[i]->ServiceId() == aPolicyCheckRequestData.iServiceId)
			{
			serviceIdFound = ETrue;
			
			aDatabaseEntry = iDatabaseEntry[i];	
			__ASSERT_DEBUG((aDatabaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl4));				
			
			__FLOG_2(_L("\ti = %d, databaseEntry = %08x"), i, aDatabaseEntry);	
			const TInt32 processEntryCount = aDatabaseEntry->ProcessEntry().Count();
			for (TInt j = 0; (j < processEntryCount && threadEntryFound == EFalse); ++j)
				{
				aProcessEntry = (aDatabaseEntry->ProcessEntry())[j];
				__FLOG_2(_L("\tj= %d, processEntry = %08x"), j, aProcessEntry);		
				__ASSERT_DEBUG((aProcessEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl5));				
				if (aProcessEntry->ProcessId() == aPolicyCheckRequestData.iProcessId)
					{
					processEntryFound = ETrue;
					if ((MapInternalStateToSessionMode(aProcessEntry->NetUpsState()) == ENonSessionMode) ||
						(iLifeTimeMode == ENetworkLifeTimeMode))
						{
						const TInt32 threadEntryCount = aProcessEntry->ThreadEntry().Count();
						for(TInt k = 0; k < threadEntryCount; ++k)
							{
							aThreadEntry = (aProcessEntry->ThreadEntry())[k];
							__FLOG_2(_L("\tk = %d, threadEntry = %08x"), k, aThreadEntry);			
							__ASSERT_DEBUG((aThreadEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl6));				
							if (aThreadEntry->ThreadId() == aPolicyCheckRequestData.iThreadId)
								{
								threadEntryFound = ETrue;
								break;
								}
							}	
						}
					else
						{
						threadEntryRequired = EFalse;
						break;
						}	
						
					}
				}
			}			
		}
		
	if (serviceIdFound == EFalse)
		{
		aDatabaseEntry = CDatabaseEntry::NewL(aPolicyCheckRequestData.iServiceId);
		CleanupStack::PushL(aDatabaseEntry);
		__FLOG_2(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntriesL() databaseEntry = %08x"), this, aDatabaseEntry);		
		iDatabaseEntry.AppendL(aDatabaseEntry);
		CleanupStack::Pop(aDatabaseEntry);
		}

	if (processEntryFound == EFalse)
		{
		CUpsStateMachine& stateMachine = IdentifyStateMachineL(aPolicyCheckRequestData.iServiceId);

		
		CProcessEntry* processEntry = CProcessEntry::NewL(*aDatabaseEntry, aPolicyCheckRequestData.iProcessId, stateMachine );
		CleanupStack::PushL(processEntry);
		__FLOG_2(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntriesL() processEntry = %08x"), this, processEntry);		
		aDatabaseEntry->ProcessEntry().AppendL(processEntry);
		CleanupStack::Pop(processEntry);
		aProcessEntry = processEntry;
		}

	if ((threadEntryFound == EFalse) && (threadEntryRequired != EFalse))
		{
		CThreadEntry* threadEntry = CThreadEntry::NewL(*aDatabaseEntry, *aProcessEntry, aPolicyCheckRequestData.iThreadId, iUpsSession);
		CleanupStack::PushL(threadEntry);
		__FLOG_2(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntriesL() threadEntry = %08x"), this, threadEntry);						
		aProcessEntry->ThreadEntry().AppendL(threadEntry);
		CleanupStack::Pop(threadEntry);
		aThreadEntry = threadEntry;				
		}

	// By this point, databaseEntry and processEntry should be assigned to a Non NULL value		
	__ASSERT_DEBUG((aDatabaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl7));				
	__ASSERT_DEBUG((aProcessEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl8));					
	}

TInt CNetUpsImpl::IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, CThreadEntry*& aThreadEntry)
/** Searches the database to locate the thread entry corresponding to the Service Id and CommsId specified. 
@param aServiceId		A Service Id which forms the 1st part of the query key.	
@param aCommsId			A Comms Id which forms the 2nd part of the query key.
@param aThreadEntry		An output parameter which contains a pointer to the thread entry associated with the search key.	The Client must initialise this parameter to NULL to indicate that the referenced memory is unassigned.
*/
	{
	//__FLOG_4(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntry(), aServiceId = %d, aCallersNodeId = %08x, aThreadEntry = %08x"), this, aServiceId, aCallersNodeId, aThreadEntry);		
	__FLOG_3(_L("CNetUpsImpl %08x:\tIdentifyDatabaseEntry(), aServiceId = %d, aThreadEntry = %08x"), this, aServiceId, aThreadEntry);		


	CDatabaseEntry* databaseEntry = NULL;
	CProcessEntry*  processEntry  = NULL;
	TInt32 rc = IdentifyDatabaseEntry(aServiceId, aCallersNodeId, databaseEntry, processEntry, aThreadEntry);

	if ((rc == KErrNone) && (aThreadEntry == NULL))  // the previous function will identify the databaseEntry and
												  	 // processEntry but not a threadEntry if the processEntry
												  	 // is in process session life time mode.
		{
		rc = KErrNotFound;
		}
	
	return rc;
	}

TInt CNetUpsImpl::IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCommsId, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry)
/** Searches the database to locate the entities corresponding to the Service Id and CommsId specified. 



@param aServiceId		A Service Id which forms the 1st part of the query key.	
@param aCommsId			A Comms Id which forms the 2nd part of the query key.
@param aDatabaseEntry	An output parameter which contains a pointer to the database entry associated with the search key. The Client must initialise this parameter to NULL to indicate that the referenced memory is unassigned. 
@param aProcessEntry	An output parameter which contains a pointer to the process entry associated with the search key.  The Client must initialise this parameter to NULL to indicate that the referenced memory is unassigned.
@param aThreadEntry		An output parameter which contains a pointer to the thread entry associated with the search key.	The Client must initialise this parameter to NULL to indicate that the referenced memory is unassigned.
						This argument will remain NULL if the threadEntry does not exist.
*/
	{
	//__FLOG_6(_L("CNetUpsImpl %08x\t IdentifyDatabaseEntriesL(), aServiceId, aCommsId, aDatabaseEntry = %d,\t aProcessEntry= %d,\t aThreadEntry = %d"), this, aServiceId, aCommsId, aDatabaseEntry, aProcessEntry, aThreadEntry);		
	__FLOG_5(_L("CNetUpsImpl %08x\t IdentifyDatabaseEntriesL(), aServiceId, aCommsId, aDatabaseEntry = %d,\t aProcessEntry= %d,\t aThreadEntry = %d"), this, aServiceId, aDatabaseEntry, aProcessEntry, aThreadEntry);		

	__ASSERT_DEBUG((aDatabaseEntry == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));				
	__ASSERT_DEBUG((aProcessEntry  == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));					
	__ASSERT_DEBUG((aThreadEntry  == NULL),  User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));					

	TInt rc = KErrNotFound;
	
	const TInt32 databaseEntryCount = iDatabaseEntry.Count();
	for (TInt i = 0; ((i < databaseEntryCount) && (rc == KErrNotFound)); ++i)
		{
		__ASSERT_DEBUG((iDatabaseEntry[i] != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl9));				
		if (aServiceId == iDatabaseEntry[i]->ServiceId())
			{
			CDatabaseEntry* databaseEntry = iDatabaseEntry[i];
			__FLOG_2(_L("\ti = %d, databaseEntry = %08x"), i, databaseEntry);			
			const TInt32 processEntryCount = databaseEntry->ProcessEntry().Count();
			for (TInt j = 0; ((j < processEntryCount) && (rc == KErrNotFound)); ++j)
				{
				CProcessEntry* processEntry = (databaseEntry->ProcessEntry())[j];
				__FLOG_2(_L("\tj = %d, processEntry = %08x"), j, processEntry);			
				__ASSERT_DEBUG((processEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl10));				
				aDatabaseEntry = databaseEntry;
				aProcessEntry = processEntry;


				const TInt32 threadEntryCount = processEntry->ThreadEntry().Count();
				for(TInt32 k = 0; ((k < threadEntryCount) && (rc == KErrNotFound)); ++k)
					{
					CThreadEntry* threadEntry = (processEntry->ThreadEntry())[k];
					__FLOG_2(_L("\tk = %d, threadEntry = %08x"), k, threadEntry);			
					__ASSERT_DEBUG((threadEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl11));					
					const TInt32 commsIdCount = threadEntry->ConnectionEntry().Count();
					for(TInt32 l = 0; l < commsIdCount; ++l)
						{
						const Messages::TNodeId& commsId = (threadEntry->ConnectionEntry())[l]->CommsId();
						//__FLOG_2(_L("CNetUpsImpl\t i = %d, commsId = %d"), i, commsId.Printable());			
						__FLOG_1(_L("\tl = %d"), l);			

						if (commsId == aCommsId)
							{
							aThreadEntry = threadEntry;
							rc = KErrNone;
							break;						
							}
						}
					}
				}
			}
		}		
			
	return rc;	
	}

void CNetUpsImpl::IncrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId& aCallersNodeId)
	{
	//__FLOG_3(_L("CNetUpsImpl %08x:\tIncrementConnectionCountL(), aServiceId = %d, aCallersNodeId = %08x"), this, aServiceId, aCallersNodeId);		
	__FLOG_2(_L("CNetUpsImpl %08x:\tIncrementConnectionCountL(), aServiceId = %d"), this, aServiceId);		

  	if (iLifeTimeMode != EProcessLifeTimeMode)
 		{ 
 		CThreadEntry* threadEntry = NULL;
		TInt rc = IdentifyDatabaseEntry(aServiceId, aCallersNodeId, threadEntry);

		if (rc == KErrNone)
			{
			threadEntry->IncrementConnectionCountL(aCallersNodeId);
			}
	
		User::LeaveIfError(rc);
 		}
	}

void CNetUpsImpl::DecrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId& aCallersNodeId)
	{
	//__FLOG_3(_L("CNetUpsImpl %08x:\tDecrementConnectionCountL(), aServiceId = %d, aCallersNodeId = %08x"), this, aServiceId, aCallersNodeId);		
	__FLOG_2(_L("CNetUpsImpl %08x:\tDecrementConnectionCountL(), aServiceId = %d"), this, aServiceId);		

	if (iLifeTimeMode != EProcessLifeTimeMode)
		{
		CUpsStateMachine& upsStateMachine = IdentifyStateMachineL(aServiceId);

		CDatabaseEntry* databaseEntry 	= NULL;
		CProcessEntry*  processEntry  	= NULL;
		CThreadEntry*	threadEntry		= NULL;

	 	TInt rc = IdentifyDatabaseEntry(aServiceId, aCallersNodeId, databaseEntry, processEntry, threadEntry);
		if (rc == KErrNone)
			{
			__ASSERT_DEBUG((databaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl12));
			__ASSERT_DEBUG((processEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl13));

			if (threadEntry != NULL)
				{
				TCommsIdKey commsIdKey(*databaseEntry, *processEntry, *threadEntry, aCallersNodeId);		
				upsStateMachine.DecrementConnectionCountL(commsIdKey);	
				}
			else
				{
				User::Leave(KErrNotSupported);
				}
	
			}		
		}
	}

void CNetUpsImpl::DecrementConnectionCountL(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId)
	{
	if (iLifeTimeMode != EProcessLifeTimeMode)
		{
		CUpsStateMachine& upsStateMachine = IdentifyStateMachineL(aServiceId);

		CDatabaseEntry* databaseEntry 	= NULL;
		CProcessEntry*  processEntry  	= NULL;
		CThreadEntry*	threadEntry		= NULL;

	 	TInt rc = IdentifyDatabaseEntry(aServiceId, aCallersNodeId, aProcessId, aThreadId, databaseEntry, processEntry, threadEntry);
		if (rc == KErrNone)
			{
			__ASSERT_DEBUG((databaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl14));
			__ASSERT_DEBUG((processEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl15));

			if (threadEntry != NULL)
				{
				TCommsIdKey commsIdKey(*databaseEntry, *processEntry, *threadEntry, aCallersNodeId);		
				upsStateMachine.DecrementConnectionCountL(commsIdKey);	
				}
			else
				{
				User::Leave(KErrNotSupported);
				}
	
			}		
		}	
	}

TInt CNetUpsImpl::IdentifyDatabaseEntry(TInt32 aServiceId, const Messages::TNodeId&  aCallersNodeId, TProcessId& aProcessId, TThreadId& aThreadId, CDatabaseEntry*& aDatabaseEntry, CProcessEntry*& aProcessEntry, CThreadEntry*& aThreadEntry)	
	{
	TBool serviceIdFound;
	TBool processEntryFound;
	TBool threadEntryFound; 
	serviceIdFound 		= EFalse;
	processEntryFound 	= EFalse;
	threadEntryFound 	= EFalse;
	TInt rc = KErrNotFound;
	
	__ASSERT_DEBUG((aDatabaseEntry == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));				
	__ASSERT_DEBUG((aProcessEntry  == NULL), User::Panic(KNetUpsPanic, KPanicNonNullPointerToBeOverwritten));					
	__ASSERT_DEBUG((aThreadEntry  == NULL),  User::Panic(KNetUpsPanic,  KPanicNonNullPointerToBeOverwritten));					

	const TInt32 databaseEntryCount = iDatabaseEntry.Count();
	for (TInt i = 0; (i < databaseEntryCount && serviceIdFound == EFalse); ++i)
		{
		__ASSERT_DEBUG((iDatabaseEntry[i] != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl16));				
		if (iDatabaseEntry[i]->ServiceId() == aServiceId)
			{
			serviceIdFound = ETrue;
			
			aDatabaseEntry = iDatabaseEntry[i];	
			__ASSERT_DEBUG((aDatabaseEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl17));				
			
			__FLOG_2(_L("\ti = %d, databaseEntry = %08x"), i, aDatabaseEntry);	
			const TInt32 processEntryCount = aDatabaseEntry->ProcessEntry().Count();
			for (TInt j = 0; (j < processEntryCount && processEntryFound == EFalse); ++j)
				{
				aProcessEntry = (aDatabaseEntry->ProcessEntry())[j];
				__FLOG_2(_L("\tj= %d, processEntry = %08x"), j, aProcessEntry);		
				__ASSERT_DEBUG((aProcessEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl18));				
				if (aProcessEntry->ProcessId() == aProcessId)
					{
					processEntryFound = ETrue;
					if ((MapInternalStateToSessionMode(aProcessEntry->NetUpsState()) == ENonSessionMode) ||
						(iLifeTimeMode == ENetworkLifeTimeMode))
						{
						const TInt32 threadEntryCount = aProcessEntry->ThreadEntry().Count();
						for(TInt k = 0; (k < threadEntryCount && threadEntryFound == EFalse); ++k)
							{
							aThreadEntry = (aProcessEntry->ThreadEntry())[k];
							__FLOG_2(_L("\tk = %d, threadEntry = %08x"), k, aThreadEntry);			
							__ASSERT_DEBUG((aThreadEntry != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSimpl19));				
							if (aThreadEntry->ThreadId() == aThreadId)
								{
								threadEntryFound = ETrue;
								
								const TInt32 commsIdCount = aThreadEntry->ConnectionEntry().Count();
								for(TInt32 l = 0; l < commsIdCount; ++l)
									{
									const Messages::TNodeId& commsId = (aThreadEntry->ConnectionEntry())[l]->CommsId();
									//__FLOG_2(_L("CNetUpsImpl\t i = %d, commsId = %d"), i, commsId.Printable());			
									__FLOG_1(_L("\tl = %d"), l);			

									if (commsId == aCallersNodeId)
										{
										rc = KErrNone;
										break;
										}
									}
								}
							}	
						}						
					}
				}
			}			
		}
			
	return rc;
	}

CNetUpsImpl::TSessionMode CNetUpsImpl::MapInternalStateToSessionMode(TNetUpsState aNetUpsState)
	{
	__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("CNetUpsImpl::MapInternalStateToSessionMode(), aNetUpsState = %d"), aNetUpsState);		

	__ASSERT_DEBUG( (aNetUpsState == ENull) ||
				    ((aNetUpsState >= EProcLife_NonSession) && (aNetUpsState <= EProcLife_SessionNo)) ||
				    ((aNetUpsState >= ENetLife_NonSession)  && (aNetUpsState <= ENetLife_SessionYes)) ,
			 	  	User::Panic(KNetUpsPanic, KPanicInvalidState));

	TSessionMode rc = ENonSessionMode;

	switch(aNetUpsState)
		{
		case EProcLife_Transit_SessionYes:
		case EProcLife_Transit_SessionNo:
		case EProcLife_SessionYes:		
		case EProcLife_SessionNo:
	    case ENetLife_SessionNo_Transit_WithoutConnections:
	    case ENetLife_SessionNo_WithOutConnections:
	    case ENetLife_SessionNo_Transit_WithConnections:
	    case ENetLife_SessionNo_WithConnections:
	    case ENetLife_Transit_SessionYes:    	    
	    case ENetLife_SessionYes:
		    rc = ESessionMode;
	    	break;
		case ENull:
		case EProcLife_NonSession:
		case ENetLife_NonSession:
			break;
		default:
			User::Panic(KNetUpsPanic, KPanicUnhandledState);
			break;		
		}

	    
	return rc;
	}

void CNetUpsImpl::DetermineUpsDecision(TNetUpsState aNetUpsState, TNetUpsDecision& aNetUpsDecision)
	{
	__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("CNetUpsImpl::DetermineUpsDecision, aNetUpsState = %d"), aNetUpsState);		
	
	__ASSERT_DEBUG( (aNetUpsState == ENull) ||
				    ((aNetUpsState >= EProcLife_NonSession) && (aNetUpsState <= EProcLife_SessionNo)) ||
				    ((aNetUpsState >= ENetLife_NonSession)  && (aNetUpsState <= ENetLife_SessionYes)) ,
			 	  	User::Panic(KNetUpsPanic, KPanicInvalidState));
	
	switch(aNetUpsState)
		{		
		case EProcLife_Transit_SessionYes:
		case EProcLife_SessionYes:
		case ENetLife_Transit_SessionYes:	
		case ENetLife_SessionYes:
			aNetUpsDecision = ESessionYes;
			break;
		case EProcLife_Transit_SessionNo:
		case EProcLife_SessionNo:
		case ENetLife_SessionNo_Transit_WithoutConnections:
		case ENetLife_SessionNo_WithOutConnections:
		case ENetLife_SessionNo_Transit_WithConnections:
		case ENetLife_SessionNo_WithConnections:
			aNetUpsDecision = ESessionNo;
			break;
		default:
			User::Panic(KNetUpsPanic, KPanicUnhandledState);
			break;
		}	    
	
	__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("aNetUpsDecision = %d"), aNetUpsDecision);		
	}
	
void CNetUpsImpl::IncrementRefCount(void)
	{
	++iRefCount;
	__FLOG_2(_L("CNetUpsImpl %08x:\tIncrementRefCount(), iRefCount = %d"), this, iRefCount);		
	}

void CNetUpsImpl::DecrementRefCount(void)
	{
	if (--iRefCount < 0)
		{
		User::Panic(KNetUpsPanic, KPanicAttemptToDecrementPastZero);
		}
	__FLOG_2(_L("CNetUpsImpl %08x:\tDecrementRefCount(), iRefCount = %d"), this, iRefCount);			
	}
	
TInt32 CNetUpsImpl::RefCount(void) 
	{
	return iRefCount;
	}
	
TRequestId	CNetUpsImpl::AllocateRequestId()
	{
	return ++iRequestId;
	}
			
} // end of namespace
