// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Source for Base classes in States and State Machines
// 
//

/**
 @file
*/

#include "CAgentSMBase.h"
#include "AgentPanic.h"
#include <connectprog.h>

//
// Agent State Base
//

EXPORT_C CAgentStateBase::CAgentStateBase(MAgentStateMachineEnv& aSMObserver) 
	: CActive(EPriorityStandard),iSMObserver(&aSMObserver)
/**
Constructor
*/
	{
	CActiveScheduler::Add(this);
	}

EXPORT_C CAgentStateBase::~CAgentStateBase()
/**
Destructor
*/
	{
	Cancel();
	}
	
EXPORT_C void CAgentStateBase::JumpToRunl(TInt aError)
/**
 * Jump directly to the RunL of the object
 * @note Typically used in the case of errors, or when asynchronous requests do not need to be issued by this state
 */
	{
	SetActive();
	TRequestStatus* statusPtr=&iStatus;
	User::RequestComplete(statusPtr,aError);
	}

//
// Agent State Machine Base
//

EXPORT_C CAgentSMBase::CAgentSMBase(MAgentNotify& aControllerObserver, CDialogProcessor* aDlgPrc, CCommsDbAccess& aDbAccess) 
	: CActive(EPriorityStandard),iControllerObserver(&aControllerObserver),iDlgPrc(aDlgPrc),
	iDb(&aDbAccess),iContinueConnection(ETrue),iSMPhase(EConnecting)
/**
Constructor

@param aControllerObserver
@param aDlgPrc
@param aDbAccess
*/
	{
	CActiveScheduler::Add(this);
	}

EXPORT_C CAgentSMBase::~CAgentSMBase()
/**
Destructor
*/
	{
	Cancel();
	delete iState;
	}

void CAgentSMBase::ProcessState()
	{
	CAgentStateBase* nextiState=NULL;
	TRAPD(ret,nextiState=iState->NextStateL(iContinueConnection));
	iContinueConnection=ETrue;
	if(ret==KErrNone)	// Next state instantiated succesfully
		{
		__ASSERT_DEBUG(nextiState, StateMachineAgentPanic(Agent::ENullStateOnProcessState));
		delete iState;
		iState=nextiState;
		iStatus=KRequestPending;
		iState->StartState();
		if (!IsActive())
			SetActive();
		}
	else
		CompleteState(ret);
	}

void CAgentSMBase::StartConnect()
	{
	iStatus = KRequestPending;
	iState->StartState();
	SetActive();
	}

void CAgentSMBase::CancelConnect()
	{
	iContinueConnection=EFalse;
	Cancel();
	}

EXPORT_C void CAgentSMBase::ConnectionContinuation(TSMContinueConnectType aConnectionAction)
	{
	switch(aConnectionAction)
		{
		case ECallBack :
			iCallBack=ETrue;
			iSMPhase=EConnecting;
			break;
		case EReconnect :
			iIsReconnect=ETrue;
			iCallBack=EFalse;
			iSMPhase=EConnecting;
			break;
		case EDisconnect :
			iContinueConnection=EFalse;
			iSMPhase=EDisconnecting;
			break;
		default :
			AgentPanic(Agent::EIllegalActionType);
			break;
		}
	ProcessState();
	}

EXPORT_C void CAgentSMBase::PreventConnectionRetries()
	{
	iControllerObserver->PreventConnectionRetries();
	}

EXPORT_C void CAgentSMBase::ServiceStarted()
	{
	if(!iIsReconnect&&!iCallBack)
		{
		iControllerObserver->ServiceStarted();
		}
	}

void CAgentSMBase::ConnectCompleteReset()
	{
	iSMPhase=EConnected;
	Cancel();
	iIsReconnect=EFalse;
	}

EXPORT_C void CAgentSMBase::ConnectionComplete(TInt aProgress,TInt aError)
	{
	ConnectCompleteReset();
	iControllerObserver->ConnectionComplete(aProgress, aError);
	}

EXPORT_C void CAgentSMBase::ConnectionComplete(TInt aError)
	{
	ConnectCompleteReset();
	iControllerObserver->ConnectionComplete(aError);
	}

EXPORT_C void CAgentSMBase::DisconnectComplete()
	{
	Cancel();
	iSMPhase=EDisconnected;
	iCallBack=EFalse;
	iControllerObserver->DisconnectComplete();
	}

EXPORT_C void CAgentSMBase::UpdateProgress(TInt aProgress,TInt aError)
	{
	iControllerObserver->UpdateProgress(aProgress, aError);
	}

EXPORT_C TInt CAgentSMBase::Notification(TAgentToNifEventType aEvent, TAny* aInfo)
	{
	return iControllerObserver->Notification(aEvent, aInfo);
	}

EXPORT_C void CAgentSMBase::GetLastError(TInt& aError)
/**
Gets the latest error code returned during a connection operation.

@param aError
@return KErrNone, or another error code that might have been 
returned by a connection operation.
*/
	{
	aError=KErrNone;
	}

EXPORT_C TInt CAgentSMBase::IncomingConnectionReceived()
	{
	return iControllerObserver->IncomingConnectionReceived();
	}

EXPORT_C void CAgentSMBase::RunL()
	{
	if(iStatus==KErrNone)
		{
		ProcessState();
		}
	else if(iSMPhase==EConnecting)
		{
		ConnectionComplete(iStatus.Int());
		}
	else if(iSMPhase==EDisconnecting)
		{
		DisconnectComplete();
		}
	}

EXPORT_C void CAgentSMBase::DoCancel()
	{
	iState->Cancel();
	if(iStatus==KRequestPending)
		{
		CompleteState(KErrCancel);
		}
	}

EXPORT_C CDialogProcessor* CAgentSMBase::DlgPrc()
	{
	return iDlgPrc;
	}

EXPORT_C CCommsDbAccess* CAgentSMBase::Db()
	{
	return iDb;
	}

EXPORT_C void CAgentSMBase::CompleteState(TInt aError)
	{
	if (!IsActive())
		SetActive();
	TRequestStatus* status=&iStatus;
	User::RequestComplete(status,aError);
	}


