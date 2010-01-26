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
// This file provides the implementation of the NetUps Statemachine's
// states.
// @internalAll
// @prototype
// 
//

#include "netupsstatemachine.h"
#include "netupsstate.h"
#include "netupsstatedef.h"

#include "netupsassert.h"
#include "netupsaction.h"
#include "netupsimpl.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CState* CState::NewL(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine)
	{
	CState* self = NULL;

	switch(aNetUpsState)
		{
		case ENull:
			self = new (ELeave) CNullState(aStateMachine);
			break; 
		case EProcLife_NonSession:
			self = new (ELeave ) CProcLifeTime_NonSessionState(aStateMachine);
			break; 
		case EProcLife_Transit_SessionNo: 
		case EProcLife_Transit_SessionYes: 
			self = new (ELeave) CProcLifeTime_TransitTo_SessionState(aNetUpsState, aStateMachine);
			break; 
		case EProcLife_SessionYes:
		case EProcLife_SessionNo:
			self = new (ELeave) CProcLifeTime_SessionState(aNetUpsState, aStateMachine);
			break; 
		case ENetLife_NonSession:
			self = new (ELeave) CNetLifeTime_NonSessionState(aStateMachine);
			break; 
		case ENetLife_SessionNo_Transit_WithoutConnections: 
			self = new CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState(aStateMachine);
			break; 
		case ENetLife_SessionNo_WithOutConnections:
			self = new (ELeave) CNetLifeTime_SessionNo_WithOutConnectionsState(aStateMachine);
			break; 
		case ENetLife_SessionNo_Transit_WithConnections:
			self = new (ELeave) CNetLifeTime_TransitTo_SessionNo_WithConnectionsState(aStateMachine);
			break; 
		case ENetLife_SessionNo_WithConnections:
			self = new (ELeave) CNetLifeTime_SessionNo_WithConnectionsState(aStateMachine);
			break; 
		case ENetLife_Transit_SessionYes:		
			self = new (ELeave) CNetLifeTime_TransitTo_SessionYesState(aStateMachine);
			break; 
		case ENetLife_SessionYes:
			self = new (ELeave) CNetLifeTime_SessionYesState(aStateMachine);
			break; 
		default:
			// Log invalid index
			User::Panic(KNetUpsPanic, KPanicStateMachineIndexOutOfRange);
			break; 
		}

	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);

	return self;
	}

CState::CState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : iNetUpsState(aNetUpsState), iUpsStateMachine(aStateMachine)
	{
	}

CState::~CState()
{
__FLOG_1(_L("CState %08x:\t ~CState()"), this);				
__FLOG_CLOSE;		
}

void CState::ConstructL()
	{	
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_2(_L("CState %08x:\t ConstructL() iNetUpsState = %d"), this, iNetUpsState);	
	}

void CState::ProcessPolicyCheckRequestL(TThreadKey& , const TPolicyCheckRequestData&, TRequestId)
	{
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

void CState::IncrementConnectionCountL(TCommsIdKey&)
	{
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

void CState::DecrementConnectionCountL(TCommsIdKey&)
	{
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

void CState::HandleUPSRequestCompletionL(TCommsIdKey&, MPolicyCheckRequestOriginator&, TNetUpsDecision)
	{
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

void CState::HandleUPSErrorOnCompletionL(TCommsIdKey&, MPolicyCheckRequestOriginator&, TInt)
	{
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

const TNetUpsState& CState::State() const
	{
	return iNetUpsState;
	}

CUpsStateMachine& CState::UpsStateMachine()
	{
	return iUpsStateMachine;
	}

void CState::PerformStateTransition(TEvent aEvent, CProcessEntry& aProcessEntry)
	{

	// probably cleaner to provide an implementation of this method in each subclass.
	// keep this way for present so the state logic is co-located.
	// alternatively provide a table driven implementation - one row for each state, 
	// 1 column for each event, each column contains the next state to transit into,
	// or an error code which causes a panic.
	
	#define DEBUG_DECISION ETrue  // optionally profile unwanted events
	
	switch (aProcessEntry.NetUpsState())
		{
		case ENull:
			{
			__ASSERT_DEBUG((aEvent == EPolicyCheckRequest), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
			CNetUpsImpl::TLifeTimeMode lifeTimeMode = aProcessEntry.UpsStateMachine().NetUpsImpl().LifeTimeMode();
			__ASSERT_DEBUG((lifeTimeMode <=  CNetUpsImpl::ENetworkLifeTimeMode), User::Panic(KNetUpsPanic, KPanicSessionTypeOutOfRange));
			lifeTimeMode == CNetUpsImpl::EProcessLifeTimeMode ? aProcessEntry.SetNetUpsState(EProcLife_NonSession) :
																aProcessEntry.SetNetUpsState(ENetLife_NonSession);	
			break;
			}	

		case EProcLife_NonSession:
			if (aEvent == EResponseSessionYes)
				{
				aProcessEntry.SetNetUpsState(EProcLife_Transit_SessionYes);
				}
			else if (aEvent == EResponseSessionNo)
				{
				aProcessEntry.SetNetUpsState(EProcLife_Transit_SessionNo);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG(((DEBUG_DECISION) || (aEvent == EPolicyCheckRequest)), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}	
			break;			
		case EProcLife_Transit_SessionYes:
			if (aEvent == ETransitionForward)
				{
				aProcessEntry.SetNetUpsState(EProcLife_SessionYes);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;	
		case EProcLife_SessionYes:
			__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
			__ASSERT_DEBUG((ETrue), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
			break;
		case EProcLife_Transit_SessionNo:
			if (aEvent == ETransitionForward)
				{
				aProcessEntry.SetNetUpsState(EProcLife_SessionNo);
				}
			else
				{
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;
		case EProcLife_SessionNo:
			__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
			__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
			break;	
		case ENetLife_NonSession:
			if (aEvent == EResponseSessionYes)
				{
				aProcessEntry.SetNetUpsState(ENetLife_Transit_SessionYes);
				}
			else if (aEvent == 	EResponseSessionNo_WithConnections)
				{
				aProcessEntry.SetNetUpsState(ENetLife_SessionNo_Transit_WithConnections);			
				}
			else if (aEvent == EResponseSessionNo_WithoutConnections)
				{
				aProcessEntry.SetNetUpsState(ENetLife_SessionNo_Transit_WithoutConnections);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG(((DEBUG_DECISION) || (aEvent == EPolicyCheckRequest)), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}	
			break;
		case ENetLife_SessionNo_Transit_WithoutConnections:
			if (aEvent == ETransitionForward)
				{
				aProcessEntry.SetNetUpsState(ENetLife_SessionNo_WithOutConnections);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;
		case ENetLife_SessionNo_WithOutConnections: 
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;
		case ENetLife_SessionNo_Transit_WithConnections:
			if (aEvent == ETransitionForward)
				{
				aProcessEntry.SetNetUpsState(ENetLife_SessionNo_WithConnections);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((ETrue), User::Panic(KNetUpsPanic, KPanicInvalidEvent));	
				}
			break;	
		case ENetLife_SessionNo_WithConnections:
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;	
		case ENetLife_Transit_SessionYes:
			if (aEvent == ETransitionForward)
				{
				aProcessEntry.SetNetUpsState(ENetLife_SessionYes);
				}
			else
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));	
				}
			break;
		case ENetLife_SessionYes:
				{
				__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
				__ASSERT_DEBUG((DEBUG_DECISION), User::Panic(KNetUpsPanic, KPanicInvalidEvent));
				}
			break;	
		default:
			User::Panic(KNetUpsPanic, KPanicInvalidStateMachineIndex);		
			break;
		}

		__FLOG_STATIC3(KNetUpsSubsys, KNetUpsComponent, _L("CState:\t PerformStateTransition() event = %d, processId = %d, netUpsState = %d"), aEvent, aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState());	
	}
				
void CState::HandleProcessTermination(TProcessKey&)
	{
	}

void CState::HandleThreadTermination(TThreadKey&)
	{	
	}

CNullState::CNullState(CUpsStateMachine& aStateMachine) : CState(ENull, aStateMachine)
	{	
	}

void CNullState::ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	{
	NetUpsFunctions::HandleUPSRequestL(*this, aThreadKey, aPolicyCheckRequestData, aRequestId);
	}

CProcLifeTime_NonSessionState::CProcLifeTime_NonSessionState(CUpsStateMachine& aStateMachine) : CState(EProcLife_NonSession, aStateMachine)
	{
	}

void CProcLifeTime_NonSessionState::HandleThreadTermination(TThreadKey& aThreadKey)
	{
	NetUpsFunctions::HandleThreadTermination_DeleteThreadEntry(aThreadKey);
	}

void CProcLifeTime_NonSessionState::ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	{
	NetUpsFunctions::HandleUPSRequestL(*this, aThreadKey, aPolicyCheckRequestData, aRequestId);
	}
	
void CProcLifeTime_NonSessionState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_ProcessLifeTimeNonSessionL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

void CProcLifeTime_NonSessionState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator,TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_ProcessLifeTimeNonSessionL(aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}

CProcLifeTime_SessionState::CProcLifeTime_SessionState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : CState(aNetUpsState, aStateMachine)
	{
	}

void CProcLifeTime_SessionState::HandleProcessTermination(TProcessKey& aProcessKey)
	{
	NetUpsFunctions::HandleProcessTermination(aProcessKey);
	}

CProcLifeTime_TransitTo_SessionState::CProcLifeTime_TransitTo_SessionState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : CProcLifeTime_SessionState(aNetUpsState, aStateMachine)
	{
	}

void CProcLifeTime_TransitTo_SessionState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

void CProcLifeTime_TransitTo_SessionState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_EnteringProcessSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}

CNetLifeTime_NonSessionState::CNetLifeTime_NonSessionState(CUpsStateMachine& aStateMachine) : CState(ENetLife_NonSession, aStateMachine)
	{
	}

void CNetLifeTime_NonSessionState::ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	{
	NetUpsFunctions::HandleUPSRequestL(*this, aThreadKey, aPolicyCheckRequestData, aRequestId);
	}

void CNetLifeTime_NonSessionState::DecrementConnectionCountL(TCommsIdKey& aCommsIdKey )
	{
	NetUpsFunctions::DecrementConnectionCount_NetworkLifeTime_NonSessionL(aCommsIdKey);
	}

void CNetLifeTime_NonSessionState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

void CNetLifeTime_NonSessionState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_NetworkLifeTimeNonSessionL(aCommsIdKey, aPolicyCheckRequestOriginator, aError);	
	}

void CNetLifeTime_NonSessionState::HandleThreadTermination(TThreadKey& aThreadKey)
	{
	NetUpsFunctions::HandleThreadTermination_DeleteThreadEntry(aThreadKey);
	}

CNetLifeTime_SessionYesState::CNetLifeTime_SessionYesState(CUpsStateMachine& aStateMachine) : CState(ENetLife_SessionYes, aStateMachine)
	{
	}

CNetLifeTime_SessionYesState::CNetLifeTime_SessionYesState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : CState(aNetUpsState, aStateMachine)
	{	
	}

void CNetLifeTime_SessionYesState::DecrementConnectionCountL(TCommsIdKey& aCommsIdKey)
	{
	NetUpsFunctions::DecrementConnectionCount_NetworkLifeTime_SessionL(aCommsIdKey);
	}

CNetLifeTime_TransitTo_SessionYesState::CNetLifeTime_TransitTo_SessionYesState(CUpsStateMachine& aStateMachine) : CNetLifeTime_SessionYesState(ENetLife_Transit_SessionYes, aStateMachine)
	{
	}

void CNetLifeTime_TransitTo_SessionYesState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}
				
void CNetLifeTime_TransitTo_SessionYesState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

CNetLifeTime_SessionNo_WithOutConnectionsState::CNetLifeTime_SessionNo_WithOutConnectionsState(CUpsStateMachine& aStateMachine) : CState(ENetLife_SessionNo_WithOutConnections, aStateMachine)
	{	
	}

CNetLifeTime_SessionNo_WithOutConnectionsState::CNetLifeTime_SessionNo_WithOutConnectionsState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : CState(aNetUpsState, aStateMachine)
	{	
	}

void CNetLifeTime_SessionNo_WithOutConnectionsState::HandleProcessTermination(TProcessKey& aProcessKey)
	{
	NetUpsFunctions::HandleProcessTermination(aProcessKey);
	}

void CNetLifeTime_SessionNo_WithOutConnectionsState::DecrementConnectionCountL(TCommsIdKey&)
	{
	// This method should never be called. However the client calls DecrementConnectionCount when
	// when processing "provider down" even if there are no connections associated with the provider.
	}

CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState::CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState(CUpsStateMachine& aStateMachine) : CNetLifeTime_SessionNo_WithOutConnectionsState(ENetLife_SessionNo_Transit_WithoutConnections, aStateMachine )
	{	
	}

void CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL( *this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);	
	}

void CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}

void CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState::HandleThreadTermination(TThreadKey& aThreadKey)
	{
	NetUpsFunctions::HandleThreadTermination_DeleteThreadEntry(aThreadKey);
	}

CNetLifeTime_SessionNo_WithConnectionsState::CNetLifeTime_SessionNo_WithConnectionsState(CUpsStateMachine& aStateMachine) : CState(ENetLife_SessionNo_WithConnections, aStateMachine )
	{	
	}

CNetLifeTime_SessionNo_WithConnectionsState::CNetLifeTime_SessionNo_WithConnectionsState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine) : CState(aNetUpsState, aStateMachine) 	
	{	
	}

void CNetLifeTime_SessionNo_WithConnectionsState::DecrementConnectionCountL(TCommsIdKey& aCommsIdKey)
	{
	NetUpsFunctions::DecrementConnectionCount_NetworkLifeTime_SessionL(aCommsIdKey);
	}

CNetLifeTime_TransitTo_SessionNo_WithConnectionsState::CNetLifeTime_TransitTo_SessionNo_WithConnectionsState(CUpsStateMachine& aStateMachine) : CNetLifeTime_SessionNo_WithConnectionsState(ENetLife_SessionNo_Transit_WithConnections, aStateMachine )
	{
	}

void CNetLifeTime_TransitTo_SessionNo_WithConnectionsState::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision)
	{
	NetUpsFunctions::HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

void CNetLifeTime_TransitTo_SessionNo_WithConnectionsState::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	NetUpsFunctions::HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(*this, aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}

} // end of namespace

