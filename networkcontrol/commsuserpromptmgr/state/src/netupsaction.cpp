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
// This file provides the implementation of the NetUps Utility Functions.
// Most / all of thes methods seem destinated to migrate into the netupsstate.cpp file.
// @internalAll
// @prototype
// 
//


#include "e32cmn.h"

#include <comms-infras/ss_activities.h>

#include "netupsaction.h"

#include "netupsassert.h"
#include "netupstypes.h"			// defines TNetUpsDecision
#include "netupsstatemachine.h"		// defines the NetUps State Machine

#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"
#include "netupsthreadentry.h"
#include "netupsprocessmonitor.h"
#include "netupsthreadmonitor.h"
#include "netupssubsession.h"

#include "netupspolicycheckrequestqueue.h"

#include "netupsstatedef.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

using namespace ESock;

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

	namespace NetUpsFunctions 
	{

	void HandleUPSRequestL(CState& aState, TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	// Process Life Time Non Session
	// Network Life Time Non Session 
	// CNullState
		{
		__FLOG_STATIC7(KNetUpsSubsys, KNetUpsComponent, _L("::HandleUPSRequestL(), processId = %d, threadId = %d, serviceId  = %d, platSecCheckResult  = %d, NodeId  = %08x, originator  = %08x, requestId = %d"),
					 aPolicyCheckRequestData.iProcessId.Id(),
					 aPolicyCheckRequestData.iThreadId.Id(),		aPolicyCheckRequestData.iServiceId,
					 aPolicyCheckRequestData.iPlatSecCheckResult, 	&aPolicyCheckRequestData.iCommsId.Node(),
					 &(aPolicyCheckRequestData.iPolicyCheckRequestOriginator), aRequestId); 

		aThreadKey.iThreadEntry.AddCommsIdL(aPolicyCheckRequestData.iCommsId);
		TRAPD(err, aThreadKey.iThreadEntry.RequestQueue().SendRequestL(aPolicyCheckRequestData, aRequestId));
		if (err != KErrNone)
			{
			// 1st back out the CommsId which we have just added, provided there are no
			// connections currently associated with it.
			aThreadKey.iThreadEntry.RemoveCommsId(aPolicyCheckRequestData.iCommsId);
			// 2nd, leave with original error, presumably KErrNoMemory
			User::Leave(err);
			}

		aState.PerformStateTransition(EPolicyCheckRequest, aThreadKey.iProcessEntry);
		}

	void HandleUpsRequestCompletion_ProcessLifeTimeNonSessionL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
		{ // Tested
		// process life time, non session
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_ProcessLifeTimeNonSessionL(), processId = %d, threadId = %d, nodeId = %08x, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aNetUpsDecision );
					
		switch(aNetUpsDecision) 
			{			
			case ENo:
				//  if request fails, remove the record of the commsid (cosmetic).
				aCommsIdKey.iThreadEntry.RemoveCommsId(aCommsIdKey.iCommsId);
				// intended fall through
			case EYes:
				{
				__ASSERT_DEBUG((aCommsIdKey.iThreadEntry.ThreadMonitor() != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsAction));					
				if (aCommsIdKey.iThreadEntry.ThreadMonitor()->IsActive() == EFalse)
					{
					aCommsIdKey.iThreadEntry.ThreadMonitor()->Start();						
					}
				TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, aCommsIdKey.iCommsId);
				__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_ProcessLifeTimeNonSessionL(), error = %d"), rc);
				break;	
				}
			case ESessionYes:
			case ESessionNo:
				{
				HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(aState, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
				break;
				}
			default:
				{
				HandleUPSErrorOnCompletion_ProcessLifeTimeNonSessionL(aCommsIdKey, aPolicyCheckRequestOriginator, KErrCorrupt);
				break;
				}
			}
		}

	void HandleUPSErrorOnCompletion_ProcessLifeTimeNonSessionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
		{
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(), processId = %d, threadId = %d, nodeId = %08x, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aError ); 

		// Remove the entry on 1st time failure	
		aCommsIdKey.iThreadEntry.RemoveCommsId(aCommsIdKey.iCommsId);
		
		TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aError, aCommsIdKey.iCommsId);			
		__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUPSErrorOnCompletion_ProcessLifeTimeNonSessionL(), error = %d, rc = %d"), aError, rc );
		}

	void HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
		{ // Tested
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(), processId = %d, threadId = %d, nodeId = %08x, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aNetUpsDecision ); 

		// network life time, non session		
		switch(aNetUpsDecision) 
			{			
			case EYes:
				{
				aCommsIdKey.iThreadEntry.IncrementConnectionCount(aCommsIdKey.iCommsId);

				__ASSERT_DEBUG((aCommsIdKey.iThreadEntry.ThreadMonitor() != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsAction1));					
				if (aCommsIdKey.iThreadEntry.ThreadMonitor()->IsActive() == EFalse)
					{
					aCommsIdKey.iThreadEntry.ThreadMonitor()->Start();
					}
				TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, aCommsIdKey.iCommsId);
				__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(), error = %d"), rc );
				break;
				}
			case ENo:
				{
				//  if request fails, remove the record of the commsid
				aCommsIdKey.iThreadEntry.RemoveCommsId(aCommsIdKey.iCommsId);		
				
				__ASSERT_DEBUG((aCommsIdKey.iThreadEntry.ThreadMonitor() != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsAction2));					
				if (aCommsIdKey.iThreadEntry.ThreadMonitor()->IsActive() == EFalse)
					{
					aCommsIdKey.iThreadEntry.ThreadMonitor()->Start();
					}
				TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, aCommsIdKey.iCommsId);
				__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(), error = %d"), rc );
				break;	
				}
			case ESessionYes:
			case ESessionNo:
				{
				HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(aState, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
				break;
				}
			default:
				{
				HandleUPSErrorOnCompletion_NetworkLifeTimeNonSessionL(aCommsIdKey, aPolicyCheckRequestOriginator, KErrCorrupt);
				break;
				}
			}
		}

	void HandleUPSErrorOnCompletion_NetworkLifeTimeNonSessionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
		{
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringNetworkLifeTimeL(), processId = %d, threadId = %d, commsId = %08x, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aError ); 

		// Remove the entry on 1st time failure
		aCommsIdKey.iThreadEntry.RemoveCommsId(aCommsIdKey.iCommsId);

		TInt rc =  aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aError, aCommsIdKey.iCommsId);			
		__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUPSErrorOnCompletion_NetworkLifeTimeNonSessionL(), error = %d, rc = %d"), aError, rc );
		}


	void HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
		{ // Tested
		// process life time, entering session mode

		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(), processId = %d, threadId = %d, commsId = %d, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aNetUpsDecision ); 

		if (CNetUpsImpl::MapInternalStateToSessionMode(aState.State()) == CNetUpsImpl::ESessionMode)
			{
			// if already in session mode, reply with session decision
			CNetUpsImpl::DetermineUpsDecision(aState.State(), aNetUpsDecision);					
			}
		else	
			{
			// otherwise not yet in session mode, so use the ups decision which has just returned.
			TEvent event;		
			aNetUpsDecision == ESessionYes ? event = EResponseSessionYes : event = EResponseSessionNo;	
			aState.PerformStateTransition(event, aCommsIdKey.iProcessEntry);
			}
		TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, aCommsIdKey.iCommsId);								
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(), error = %d"), rc );

		CleanUpThreadEntries_ProcessLifeTime(aCommsIdKey);
						
		if ((aCommsIdKey.iProcessEntry.ThreadEntry()).Count() == 0)
			{
			StartProcessMonitor(aCommsIdKey);
			// if thread entry count = 0, must be ready to move from Transit_Session_Yes to Session_Yes
			aState.PerformStateTransition(ETransitionForward, aCommsIdKey.iProcessEntry);
			}
		}		
	
	void HandleUPSErrorOnCompletion_EnteringProcessSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt /*aError*/)
		{				
		// Covers the case that we are already in session life time, but outstanding UPS requests are completing
		// Simply check if ready to transition to process life time.

		// 	Determine the response to be given for all subsessions associated with this session.
		TNetUpsDecision netUpsDecision;
		CNetUpsImpl::DetermineUpsDecision(aState.State(), netUpsDecision);					
		TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(netUpsDecision, aCommsIdKey.iCommsId);								
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUPSErrorOnCompletion_EnteringProcessSessionLifeTimeL(), error = %d"), rc );
		
		CleanUpThreadEntries_ProcessLifeTime(aCommsIdKey);
							
		if ((aCommsIdKey.iProcessEntry.ThreadEntry()).Count() == 0)
			{
			StartProcessMonitor(aCommsIdKey);
			// if thread entry count = 0, must be ready to move from Transit_Session_Yes to Session_Yes
			aState.PerformStateTransition(ETransitionForward, aCommsIdKey.iProcessEntry);
			}
		}	

	void CleanUpThreadEntries_ProcessLifeTime(TThreadKey& aThreadKey)
		{ // Tested
		__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent, _L("::CleanUpThreadEntries_ProcessLifeTime() processId = %d, threadId = %d"),
				 		aThreadKey.iProcessEntry.ProcessId().Id(), aThreadKey.iThreadEntry.ThreadId().Id() ); 

		// 1st detach this subsession so it can be deleted inside CSubSession::RunL() 
		// which is the top of this calling stack.
		__ASSERT_DEBUG((aThreadKey.iThreadEntry.SubSession() != NULL), User::Panic(KNetUpsPanic, KPanicInvalidLogic));
		aThreadKey.iThreadEntry.SubSession()->SetReadyForDeletion();
		aThreadKey.iThreadEntry.SetSubSession(NULL);			

		// now clean up everything other than the current entry on the queue, 
		// which is currently being processed and will be deleted by the subsession.
		TNetUpsDecision netUpsDecision;
		aThreadKey.iProcessEntry.UpsStateMachine().NetUpsImpl().DetermineUpsDecision(aThreadKey.iProcessEntry.NetUpsState(), netUpsDecision);
		ReplyOutStandingRequests(aThreadKey, netUpsDecision);

		// Code to bottom is used in method below, consider cloning
		TInt count = aThreadKey.iProcessEntry.ThreadEntry().Count();
		for (TInt i = count - 1; i >= 0; --i)
			{
			TBool readyToDelete = ETrue;
			CThreadEntry* threadEntry = aThreadKey.iProcessEntry.ThreadEntry()[i];
			if (threadEntry->ThreadMonitor() != NULL)
				{
				// Note: An active object remains active until immediately before its RunL is executed.
				if (aThreadKey.iProcessEntry.ThreadEntry()[i]->ThreadMonitor()->IsActive() != EFalse)
					{
					threadEntry->ThreadMonitor()->Cancel();
					}
				delete threadEntry->ThreadMonitor();
				threadEntry->SetThreadMonitor(NULL);
				}
						
			if (threadEntry->SubSession() != NULL)
				{
				if (threadEntry->SubSession()->IsActive() == EFalse)
					{
					delete threadEntry->SubSession();
					threadEntry->SetSubSession(NULL);
					}
				else
					{
					readyToDelete = EFalse;
					TThreadKey threadKey(aThreadKey.iDatabaseEntry, aThreadKey.iProcessEntry, *threadEntry);
					ReplyOutStandingRequests(threadKey, netUpsDecision);
					}				
				}
	
			if (readyToDelete)
				{		
				delete threadEntry;
				aThreadKey.iProcessEntry.ThreadEntry().Remove(i);						
				}									
			}
		}

	void HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
		{ // Tested
		// network life time, non session mode
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(), processId = %d, threadId = %d, commsId = %d, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aNetUpsDecision ); 
			
		if (CNetUpsImpl::MapInternalStateToSessionMode(aState.State()) == CNetUpsImpl::ESessionMode)
			{
			// if already in session mode, reply with session decision
			CNetUpsImpl::DetermineUpsDecision(aState.State(), aNetUpsDecision);		
			}

		if (aNetUpsDecision == ESessionYes)
			{
			aCommsIdKey.iThreadEntry.IncrementConnectionCount(aCommsIdKey.iCommsId);
			}		

		TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, aCommsIdKey.iCommsId);					
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(), error = %d"), rc );

		CleanUpThreadEntries_NetworkLifeTime(aState, aCommsIdKey, aNetUpsDecision);
		}			

	void HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
		{
		// network life time, non session mode
#ifndef _DEBUG
		(void) aError;
#endif
		__FLOG_STATIC5(KNetUpsSubsys, KNetUpsComponent,_L("::CleanUpThreadEntries_NetworkLifeTime(), processId = %d, threadId = %d, commsId = %d, aPolicyCheckRequestOriginator = %08x, netUpsDecision = %d"),
					 	aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 	aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 	&aCommsIdKey.iCommsId.Node(),
					 	&aPolicyCheckRequestOriginator,
					 	aError ); 

		// 	Determine the response to be given for all subsessions associated with this session.
		TNetUpsDecision netUpsDecision = ENo;
		CNetUpsImpl::DetermineUpsDecision(aState.State(), netUpsDecision);					
		TInt rc = aPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(netUpsDecision, aCommsIdKey.iCommsId);								
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(), error = %d"), rc );

		CleanUpThreadEntries_NetworkLifeTime(aState, aCommsIdKey, netUpsDecision);					
		}

	void CleanUpThreadEntries_NetworkLifeTime(CState& aState, TCommsIdKey& aCommsIdKey, TNetUpsDecision aNetUpsDecision)
		{ // Tested
		// 1st detach this subsession so it can be deleted inside CSubSession::RunL() 
		// which is the top of this calling stack.

		__FLOG_STATIC4(KNetUpsSubsys, KNetUpsComponent,_L("::CleanUpThreadEntries_NetworkLifeTime(), processId = %d, threadId = %d, commsId = %d, netUpsDecision = %d"),
				 aCommsIdKey.iProcessEntry.ProcessId().Id(),
				 aCommsIdKey.iThreadEntry.ThreadId().Id(),
				 &aCommsIdKey.iCommsId.Node(),
				 aNetUpsDecision ); 

		__ASSERT_DEBUG((aCommsIdKey.iThreadEntry.SubSession() != NULL), User::Panic(KNetUpsPanic, KPanicInvalidLogic));
		aCommsIdKey.iThreadEntry.SubSession()->SetReadyForDeletion();
		aCommsIdKey.iThreadEntry.SetSubSession(NULL);			

		ReplyOutStandingRequests(aCommsIdKey, aNetUpsDecision);

		TBool allActiveObjectsCompleted = ETrue;
		TInt count = aCommsIdKey.iProcessEntry.ThreadEntry().Count();
		for (TInt i = 0; i < count; i++)
			{
			CThreadEntry* threadEntry = aCommsIdKey.iProcessEntry.ThreadEntry()[i];
			if (threadEntry->ThreadMonitor() != NULL)
				{
				// Note: An active object remains active until immediately before its RunL is executed.				
				if (threadEntry->ThreadMonitor()->IsActive() != EFalse)
					{
					threadEntry->ThreadMonitor()->Cancel();
					}
				delete threadEntry->ThreadMonitor();
				threadEntry->SetThreadMonitor(NULL);
				}
			
			if (threadEntry->SubSession() != NULL)
				{
				// Note: An active object remains active until immediately before its RunL is executed.				
				if (threadEntry->SubSession()->IsActive() == EFalse)
					{
					delete threadEntry->SubSession();
					threadEntry->SetSubSession(NULL);
					}
				else
					{
					allActiveObjectsCompleted = EFalse;
					TThreadKey threadKey(aCommsIdKey.iDatabaseEntry, aCommsIdKey.iProcessEntry, *threadEntry);
					ReplyOutStandingRequests(threadKey, aNetUpsDecision);
					}	
				}
			}

		__ASSERT_DEBUG(((aNetUpsDecision == ESessionYes) || (aNetUpsDecision == ESessionNo)), User::Panic(KNetUpsPanic, KPanicInvalidLogic));
		if (aNetUpsDecision == ESessionYes)
			{				
			PerformTransitionFromNetworkLifeTimeNonSession(aState, EResponseSessionYes, aCommsIdKey, allActiveObjectsCompleted);			
			}
		else if (aNetUpsDecision == ESessionNo)
			{
			TInt count = aCommsIdKey.iProcessEntry.ThreadEntry().Count();
			TBool allCountsZero = ETrue;
			for (TInt i = 0; i < count; i++)
				{
				if ( (aCommsIdKey.iProcessEntry.ThreadEntry())[i]->ConnectionCount() != 0 )
					{
					allCountsZero = EFalse;
					break;
					}
				}
						
			if (allCountsZero)
				{
				PerformTransitionFromNetworkLifeTimeNonSession(aState, EResponseSessionNo_WithoutConnections, aCommsIdKey, allActiveObjectsCompleted);			
				}	
			else
				{
				PerformTransitionFromNetworkLifeTimeNonSession(aState, EResponseSessionNo_WithConnections, aCommsIdKey, allActiveObjectsCompleted);			
				}			
			}
		}

	void HandleProcessTermination(TProcessKey& aProcessKey)
		{ // Not Tested
		// Needed for process life time - session Yes / session No
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("::HandleProcessTermination() processId = %d"), aProcessKey.iProcessEntry.ProcessId().Id()); 
		TBool retainProcessMonitor = ETrue;
		DeleteProcessEntry(aProcessKey, retainProcessMonitor);
		}

	void HandleThreadTermination_DeleteThreadEntry(TThreadKey& aThreadKey)
		{ // Tested
		// Could add reason for thread termination, but end result is the same.

		// Handles the following states:
		// Process Life Time Non Session - No Movement
		// Network Life Time Non Session - No Movement		
		// The thread monitors should be cancelled before entry into any other state.
		TNetUpsState state = aThreadKey.iProcessEntry.NetUpsState();
		__ASSERT_DEBUG(((state == EProcLife_NonSession) || (state == ENetLife_NonSession)), User::Panic(KNetUpsPanic, KPanicInvalidLogic));
		
		__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent, _L("::HandleThreadTermination_DeleteThreadEntry() processId = %d, threadId = %d"),
				 		aThreadKey.iProcessEntry.ProcessId().Id(), aThreadKey.iThreadEntry.ThreadId().Id() ); 

		CThreadEntry& threadEntry = aThreadKey.iThreadEntry;
		threadEntry.SetThreadMonitor(NULL); // Set the thread monitor to NULL, as  currently called
											// from threadMonitor RunL, which deletes itself at end of RunL.

		if (aThreadKey.iThreadEntry.SubSession() != NULL) 
			{
			// cancel all requests and reply back with KErrDied to the originator.
			threadEntry.RequestQueue().CancelAllRequests(KErrDied);
			DeleteThreadEntry(aThreadKey);						
			if (aThreadKey.iProcessEntry.ThreadEntry().Count() == 0)		
				{
				DeleteProcessEntry(aThreadKey);
				}				
			}
		}

	void IncrementConnectionCountL(TCommsIdKey&)
		{
		__FLOG_STATIC0(KNetUpsSubsys, KNetUpsComponent, _L("::IncrementConnectionCountL"));
		User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
		}				

	void DecrementConnectionCount_NetworkLifeTime_SessionL(TCommsIdKey& aCommsIdKey)
		{ // Tested
		// Network Lifetime - Session No - MCPR Count
		// Network Lifetime - Session Yes

		// Remove the comms id 1st
		//  
		__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("::DecrementConnectionCount_NetworkLifeTime_SessionL(), processId = %d, threadId = %d, commsId = %d, aError = %d"),
					 aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 &aCommsIdKey.iCommsId.Node()); 

		TNetUpsState state = aCommsIdKey.iProcessEntry.NetUpsState();

		__ASSERT_DEBUG(((state == ENetLife_Transit_SessionYes) 					||
						(state == ENetLife_SessionYes) 							||
						(state == ENetLife_SessionNo_Transit_WithConnections) 	||
						(state == ENetLife_SessionNo_WithConnections)),
						User::Panic(KNetUpsPanic, KPanicInvalidLogic));

		DecrementConnectionCount_NetworkLifeTime_NonSessionL(aCommsIdKey);
									
		if (aCommsIdKey.iProcessEntry.ThreadEntry().Count() == 0)
			{
			TProcessKey processKey(aCommsIdKey.iDatabaseEntry, aCommsIdKey.iProcessEntry);
			DeleteProcessEntry(processKey);				
			}			
		}				
	
	void DecrementConnectionCount_NetworkLifeTime_NonSessionL(TCommsIdKey& aCommsIdKey)
		{ // Tested
		__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("::DecrementConnectionCount_NetworkLifeTime_NonSessionL(), processId = %d, threadId = %d, commsId = %d, aError = %d"),
					 aCommsIdKey.iProcessEntry.ProcessId().Id(),
					 aCommsIdKey.iThreadEntry.ThreadId().Id(),
					 &aCommsIdKey.iCommsId.Node()); 

		aCommsIdKey.iThreadEntry.DecrementConnectionCount(aCommsIdKey.iCommsId);

		TInt32 connectionCount = aCommsIdKey.iThreadEntry.ConnectionCount();
		if (connectionCount == 0) 
			{
			TThreadKey threadKey(aCommsIdKey.iDatabaseEntry, aCommsIdKey.iProcessEntry, aCommsIdKey.iThreadEntry);
			DeleteThreadEntry(threadKey);	// results in an active thread monitor being cancelled then deleted.			
			}			
		}				

	void StartProcessMonitor(TProcessKey& aProcessKey)
		{ // Tested
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("::StartProcessMonitor() processId = %d"), aProcessKey.iProcessEntry.ProcessId().Id()); 
		__ASSERT_DEBUG((aProcessKey.iProcessEntry.ProcessMonitor() != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsAction3));					
		__ASSERT_DEBUG((aProcessKey.iProcessEntry.ProcessMonitor()->IsActive() == EFalse), User::Panic(KNetUpsPanic, KPanicInvalidLogic));
		aProcessKey.iProcessEntry.ProcessMonitor()->Start();			
		}

	void DeleteProcessEntry(TProcessKey& aProcessKey, TBool aRetainProcessMonitor)
		{ //Not Tested
		__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("::DeleteProcessEntry() processId = %d"), aProcessKey.iProcessEntry.ProcessId().Id()); 
		
		if (aRetainProcessMonitor != EFalse)
			{
			aProcessKey.iProcessEntry.SetProcessMonitor(NULL);
			// running within the calling stack of CProcessMonitor::RunL(), so delete the
			// process monitor when at the end of its RunL method.
			}

		for (TInt i = aProcessKey.iDatabaseEntry.ProcessEntry().Count() - 1; i >=0; --i )
			{
			if ((aProcessKey.iDatabaseEntry.ProcessEntry())[i]->ProcessId() == aProcessKey.iProcessEntry.ProcessId())
				{
				delete (aProcessKey.iDatabaseEntry.ProcessEntry())[i]; 
				aProcessKey.iDatabaseEntry.ProcessEntry()[i] = 0; 
				aProcessKey.iDatabaseEntry.ProcessEntry().Remove(i); // do we need to consider compression ?
				break;
				}	
			}					
		}

	void DeleteThreadEntry(TThreadKey& aThreadKey)
		{// Tested
		__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent, _L("::DeleteThreadEntry() processId = %d, threadId = %d"),
				 		aThreadKey.iProcessEntry.ProcessId().Id(), aThreadKey.iThreadEntry.ThreadId().Id() ); 


		for (TInt i = aThreadKey.iProcessEntry.ThreadEntry().Count() - 1; i >=0; --i )
			{
			if ((aThreadKey.iProcessEntry.ThreadEntry())[i]->ThreadId() == aThreadKey.iThreadEntry.ThreadId())
				{
				delete (aThreadKey.iProcessEntry.ThreadEntry())[i]; 
				aThreadKey.iProcessEntry.ThreadEntry()[i] = 0; 
				aThreadKey.iProcessEntry.ThreadEntry().Remove(i); // do we need to consider compression ?
				break;
				}	
			}	
		}

	void ReplyOutStandingRequests(TThreadKey& aThreadKey)
		{
		CNetUpsImpl& netUpsImpl  = aThreadKey.iProcessEntry.UpsStateMachine().NetUpsImpl();
		TNetUpsState netUpsState = aThreadKey.iProcessEntry.NetUpsState();

		TNetUpsDecision netUpsDecision = ENo;
		if (netUpsImpl.MapInternalStateToSessionMode(netUpsState) == CNetUpsImpl::ESessionMode)
			{
			aThreadKey.iProcessEntry.UpsStateMachine().NetUpsImpl().DetermineUpsDecision(netUpsState, netUpsDecision);
			}
			
		ReplyOutStandingRequests(aThreadKey, netUpsDecision);
		}

	void ReplyOutStandingRequests(TThreadKey& aThreadKey, TNetUpsDecision aNetUpsDecision)
		{
		while (aThreadKey.iThreadEntry.RequestQueue().OutStandingRequestToProcess() != EFalse)
			{

			CPolicyCheckRequestData& policyCheckRequestData = aThreadKey.iThreadEntry.RequestQueue().GetOutStandingRequestToProcess();

			if ((aNetUpsDecision == ESessionYes) || (aNetUpsDecision == EYes))
				{
				aThreadKey.iThreadEntry.IncrementConnectionCountL(policyCheckRequestData.iCommsId);
				}
			
			TInt rc = policyCheckRequestData.iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aNetUpsDecision, 
																									  policyCheckRequestData.iCommsId);
			__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent,_L("::ReplyOutStandingRequests(), error = %d"), rc );
			aThreadKey.iThreadEntry.RequestQueue().DeleteOutStandingRequest();					
			}	
		}

	void PerformTransitionFromNetworkLifeTimeNonSession(CState& aState, TEvent aEvent, TCommsIdKey& aCommsIdKey, TBool aAllActiveObjectsCompleted)
		{
		switch(aEvent)
			{
			case EResponseSessionYes:
				aState.PerformStateTransition(EResponseSessionYes, aCommsIdKey.iProcessEntry);
				break;
			case EResponseSessionNo_WithoutConnections:
				aState.PerformStateTransition(EResponseSessionNo_WithoutConnections, aCommsIdKey.iProcessEntry);
				break;
			case EResponseSessionNo_WithConnections:
				aState.PerformStateTransition(EResponseSessionNo_WithConnections, aCommsIdKey.iProcessEntry);
				break;
			default:
				User::Panic(KNetUpsPanic, KPanicInvalidLogic);					
				break;
			}

		if (aAllActiveObjectsCompleted != EFalse)
			{
			switch(aEvent)
				{
				case EResponseSessionNo_WithoutConnections:
					StartProcessMonitor(aCommsIdKey);
					// drop through to perform state transition
				case EResponseSessionYes:
					// drop through to perform state transition
				case EResponseSessionNo_WithConnections:
					aState.PerformStateTransition(ETransitionForward, aCommsIdKey.iProcessEntry);	
					break;
				default:
					User::Panic(KNetUpsPanic, KPanicInvalidLogic);					
					break;
				}
			}
		}
	
	} // end of namespace NetUpsFunctions
} // end of namespace NetUps

