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
// This file provides the implementation of the NetUps Statemachine.
// @internalAll
// @prototype
// 
//

#include "e32def.h"			//defines __ASSERT_DEBUG
#include "e32cmn.h"

#include "netupsassert.h"

#include "netupsstatemachine.h"
#include "netupsstate.h"
#include "netupsstatedef.h"

#include "netupsserviceid.h"

#include "netupsimpl.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CUpsStateMachine* CUpsStateMachine::NewL(CNetUpsImpl& aNetUpsImpl, TInt32 aServiceId)
	{
	__FLOG_STATIC1(KNetUpsSubsys, KNetUpsComponent, _L("CUpsStateMachine::NewL(), aServiceId = %d"), aServiceId);		

	CUpsStateMachine* self = NULL;
	if (aServiceId != EIpServiceId)
		{
		__FLOG_STATIC0(KNetUpsSubsys, KNetUpsComponent, _L("CUpsStateMachine::NewL() Service id not supported"));
		User::Leave(KErrNotSupported);
		}
	else
		{
		self = new (ELeave) CUpsStateMachine(aNetUpsImpl, aServiceId);

		CleanupStack::PushL(self);
		self->ConstructL();
		CleanupStack::Pop(self);
		}
		
	return self;
	}

CUpsStateMachine::CUpsStateMachine(CNetUpsImpl& aNetUpsImpl, TInt32 aServiceId) : iNetUpsImpl(aNetUpsImpl), iServiceId(aServiceId) 
	{
	__FLOG_2(_L("CUpsStateMachine %08x:\t CUpsStateMachine()"), this, aServiceId);			
	}

void CUpsStateMachine::ConstructL()
	{
	__FLOG_2(_L("CUpsStateMachine %08x:\t ConstructL()"), this, iNetUpsImpl.LifeTimeMode());		

	// Note the order in which the states are instantiated must match the 
	// order in which they are defined in the enumeration TNetUpsState - or a panic will occur.

	iState.Append(CState::NewL(ENull, *this));	
	switch(iNetUpsImpl.LifeTimeMode())
		{
		case CNetUpsImpl::EProcessLifeTimeMode:
			{
			iState.Append(CState::NewL(EProcLife_NonSession, *this));			
			iState.Append(CState::NewL(EProcLife_Transit_SessionYes, *this)); // a transient state is entered when the UPS Server responds with either SessionYes or SessionNo and there are 1 or more UPS requests outstanding to other subsessions which are associated with the same process.			
			iState.Append(CState::NewL(EProcLife_SessionYes, *this));
			iState.Append(CState::NewL(EProcLife_Transit_SessionNo, *this));
			iState.Append(CState::NewL(EProcLife_SessionNo, *this));
			break;
			}
		case CNetUpsImpl::ENetworkLifeTimeMode:
			{
			iState.Append(CState::NewL(ENetLife_NonSession, *this));			
			iState.Append(CState::NewL(ENetLife_SessionNo_Transit_WithoutConnections, *this));			
			iState.Append(CState::NewL(ENetLife_SessionNo_WithOutConnections, *this));
			iState.Append(CState::NewL(ENetLife_SessionNo_Transit_WithConnections, *this));
			iState.Append(CState::NewL(ENetLife_SessionNo_WithConnections, *this));
			iState.Append(CState::NewL(ENetLife_Transit_SessionYes, *this));
			iState.Append(CState::NewL(ENetLife_SessionYes, *this));
			break;				
			}
		default:
			{
			User::Leave(KErrNotSupported);			
			}
		}

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CUpsStateMachine %08x:\t ConstructL()"), this);				
	}

TUint32 CUpsStateMachine::GetIndex(TNetUpsState aNetUpsState)
	{		
	__FLOG_2(_L("CUpsStateMachine %08x:\t GetIndex() aNetUpsState = %d"), this, aNetUpsState );		

	TUint32 index = 0;
	if (aNetUpsState != ENull)
		{
		switch(iNetUpsImpl.LifeTimeMode())
			{
			case CNetUpsImpl::EProcessLifeTimeMode:
				index = aNetUpsState  - EProcLife_NonSession + 1;
				//__FLOG_3(_L("CUpsStateMachine %08x:\t GetIndex() aNetUpsState = %d, index = %d, mode = iNetUpsImpl.LifeTimeMode()"), this, index, iNetUpsImpl.LifeTimeMode() );		
				break;
			case CNetUpsImpl::ENetworkLifeTimeMode:
				index = aNetUpsState -  ENetLife_NonSession + 1;
				//__FLOG_3(_L("CUpsStateMachine %08x:\t GetIndex() aNetUpsState = %d, index = %d, mode = iNetUpsImpl.LifeTimeMode()"), this, index, iNetUpsImpl.LifeTimeMode() );		

				break;
			default:				
				__FLOG_2(_L("CUpsStateMachine %08x:\t GetIndex() aNetUpsState = %d, mode = iNetUpsImpl.LifeTimeMode()"), this, iNetUpsImpl.LifeTimeMode() );		
				User::Panic(KNetUpsPanic, KPanicSessionTypeOutOfRange);
				break;
			}
		}
	return index;	
	}


CUpsStateMachine::~CUpsStateMachine()
	{
	__FLOG_1(_L("CUpsStateMachine %08x:\t ~CUpsStateMachine()"), this);		

	for (TInt i = iState.Count() - 1; i >= 0; i--)
			{
			delete iState.operator[](i);
			__FLOG_2(_L("CUpsStateMachine %08x:\t ~CUpsStateMachine(), iState[i] = %08x"), this, iState[i]);		
			iState.Remove(i);
			}
	iState.Reset();
	iState.Close();		

	__FLOG_CLOSE;	
	}

TUint32 CUpsStateMachine::GetIndex(CProcessEntry& aProcessEntry)
{
	TNetUpsState netUpsState = CUpsStateMachine::GetState(aProcessEntry);
	TInt32 index = static_cast<TInt32>(GetIndex(netUpsState));

	__FLOG_3(_L("CUpsStateMachine %08x:\t GetIndex(), index = %d, netUpsState = %d"),
			 this, index, netUpsState); 
		
	__ASSERT_DEBUG((index < iState.Count()), User::Panic(KNetUpsPanic, KPanicStateMachineIndexOutOfRange));
	__ASSERT_DEBUG((iState.operator[](index)->State() == netUpsState), User::Panic(KNetUpsPanic, KPanicInvalidStateMachineIndex));

	return index;
}

void CUpsStateMachine::ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	{
	__FLOG_8(_L("CUpsStateMachine %08x:\t ProcessPolicyCheckRequestL(), processId = %d, threadId = %d, serviceId  = %d, platSecCheckResult  = %d, commsId  = %d, originator  = %08x, request id = %d"),
				 this, aPolicyCheckRequestData.iProcessId.Id(),
				 aPolicyCheckRequestData.iThreadId.Id(),		aPolicyCheckRequestData.iServiceId,
				 aPolicyCheckRequestData.iPlatSecCheckResult, 	&aPolicyCheckRequestData.iCommsId.Node(),
				 &(aPolicyCheckRequestData.iPolicyCheckRequestOriginator), aRequestId); 

	TUint32 index = GetIndex(aThreadKey.iProcessEntry);
	(iState.operator[](index))->ProcessPolicyCheckRequestL(aThreadKey, aPolicyCheckRequestData, aRequestId);
	}

void CUpsStateMachine::ProcessPolicyCheckRequestL(TProcessKey& aProcessKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId)
	{
	__FLOG_8(_L("CUpsStateMachine %08x:\t ProcessPolicyCheckRequestL(), processId = %d, threadId = %d, serviceId  = %d, platSecCheckResult  = %d, commsId  = %08x, originator  = %08x, request id = %d"),
				 this, aPolicyCheckRequestData.iProcessId.Id(),
				 aPolicyCheckRequestData.iThreadId.Id(),		aPolicyCheckRequestData.iServiceId,
				 aPolicyCheckRequestData.iPlatSecCheckResult, 	&aPolicyCheckRequestData.iCommsId.Node(),
				 &(aPolicyCheckRequestData.iPolicyCheckRequestOriginator), aRequestId); 

	// Suspect that this method is not required, when threadEntry is not known, we are in session lifetime.
	(void) aProcessKey;
	(void) aPolicyCheckRequestData;
	(void) aRequestId;
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);
	}

void CUpsStateMachine::IncrementConnectionCountL(TCommsIdKey& aCommsIdKey)
	{
	__FLOG_4(_L("CUpsStateMachine %08x:\t IncrementConnectionCountL(), processId = %d, threadId = %d, commsId = %d"),
				 this, aCommsIdKey.iProcessEntry.ProcessId().Id(),
				 aCommsIdKey.iThreadEntry.ThreadId().Id(),
				 &aCommsIdKey.iCommsId.Node() ); 

	(void) aCommsIdKey;
	
	// Suspect that this method is not required, connection counts are incremented without the request going via the state machine.

	/*
	TUint32 index = GetIndex(aCommsIdKey.iProcessEntry);
	(iState.operator[](index))->IncrementConnectionCountL(aCommsIdKey);
	*/
	User::Panic(KNetUpsPanic, KPanicMethodNotSupported);

	}

void CUpsStateMachine::DecrementConnectionCountL(TCommsIdKey& aCommsIdKey)
	{
	__FLOG_4(_L("CUpsStateMachine %08x:\t DecrementConnectionCountL(), processId = %d, threadId = %d, commsId = %d"),
				 this, aCommsIdKey.iProcessEntry.ProcessId().Id(),
				 aCommsIdKey.iThreadEntry.ThreadId().Id(),
				 &aCommsIdKey.iCommsId.Node() ); 

	TUint32 index = GetIndex(aCommsIdKey.iProcessEntry);
	(iState.operator[](index))->DecrementConnectionCountL(aCommsIdKey);
	}
		
void CUpsStateMachine::HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator , TNetUpsDecision aNetUpsDecision)
	{
	__FLOG_5(_L("CUpsStateMachine %08x:\t HandleUPSRequestCompletionL(), processId = %d, threadId = %d, commsId = %d, netUpsDecision = %d"),
				 this, aCommsIdKey.iProcessEntry.ProcessId().Id(),
				 aCommsIdKey.iThreadEntry.ThreadId().Id(),
				 &aCommsIdKey.iCommsId.Node(),
				 aNetUpsDecision ); 

	TUint32 index = GetIndex(aCommsIdKey.iProcessEntry);
	(iState.operator[](index))->HandleUPSRequestCompletionL(aCommsIdKey, aPolicyCheckRequestOriginator, aNetUpsDecision);
	}

void CUpsStateMachine::HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError)
	{
	__FLOG_5(_L("CUpsStateMachine %08x:\t HandleUPSErrorOnCompletionL(), processId = %d, threadId = %d, commsId = %d, aError = %d"),
				 this, aCommsIdKey.iProcessEntry.ProcessId().Id(),
				 aCommsIdKey.iThreadEntry.ThreadId().Id(),
				 &aCommsIdKey.iCommsId.Node(),
				 aError ); 

	TUint32 index = GetIndex(aCommsIdKey.iProcessEntry);
	(iState.operator[](index))->HandleUPSErrorOnCompletionL(aCommsIdKey, aPolicyCheckRequestOriginator, aError);
	}

void CUpsStateMachine::HandleProcessTermination(TProcessKey& aProcessKey)
	{
	__FLOG_2(_L("CUpsStateMachine %08x:\t HandleProcessTermination(), processId %d"),
				 this, aProcessKey.iProcessEntry.ProcessId().Id()); 

	TUint32 index = GetIndex(aProcessKey.iProcessEntry);
	(iState.operator[](index))->HandleProcessTermination(aProcessKey);
	}

void CUpsStateMachine::HandleThreadTermination(TThreadKey& aThreadKey)
	{
	__FLOG_3(_L("CUpsStateMachine\t HandleThreadTermination() processId = %d, threadId = %d"),
				 this, aThreadKey.iProcessEntry.ProcessId().Id(), aThreadKey.iThreadEntry.ThreadId().Id() ); 

	TUint32 index = GetIndex(aThreadKey.iProcessEntry);
	(iState.operator[](index))->HandleThreadTermination(aThreadKey);
	}
		
TInt32	CUpsStateMachine::ServiceId() 
	{
	return iServiceId;
	}

TNetUpsState CUpsStateMachine::GetState(CProcessEntry& aProcessEntry)
	{
	__FLOG_STATIC2(KNetUpsSubsys, KNetUpsComponent, _L("CUpsStateMachine\t GetState() processId = %d, netUpsState = %d"),
				 	aProcessEntry.ProcessId().Id(), aProcessEntry.NetUpsState()); 

	return aProcessEntry.NetUpsState();
	}

CNetUpsImpl& CUpsStateMachine::NetUpsImpl()
	{
	return iNetUpsImpl;	
	}

} // end of namespace

