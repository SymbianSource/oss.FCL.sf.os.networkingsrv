// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// The CStateMachineAgentBase base class implementation
// CStateMachineAgentBase is an Agent that owns a State Machine
// 
//

/**
 @file CStateMachineAgentBase.cpp
*/

#include "CStateMachineAgentBase.h"
#include "AgentPanic.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

/**
State Machine Agent Panic
@internalComponent
*/
_LIT(KStateMachineAgentPanic, "StateMachineAgent");

/**
Panic - programming error!

@internalComponent
@param aPanic the reason for panicking
*/
GLDEF_C void StateMachineAgentPanic(Agent::TStateMachineAgentPanic aPanic)
	{
	LOG( NifmanLog::Printf(_L("CStateMachineAgentBase Panic %d"), aPanic); )
	User::Panic(KStateMachineAgentPanic, aPanic);
	}


//
//                                //
//  Construction and Destruction  //
//                                //
//

EXPORT_C CStateMachineAgentBase::~CStateMachineAgentBase()
/**
Destructor.
Deletes objects used by CStateMachineAgentBase
*/
	{

	if (iStateMachine)
		delete iStateMachine;
	
	if (iNotifyCb)
		delete iNotifyCb;
	}

EXPORT_C CStateMachineAgentBase::CStateMachineAgentBase()
: iNotifyCbOp(EUndefined), iNotifyCbError(KErrNone), iDisconnectReason(KErrNone)
/**
Default Constructor
*/
	{ }


EXPORT_C void CStateMachineAgentBase::ConstructL()
/**
2nd Phase of construction. Calls CAgentBase::ConstructL().

iStateMachine should be instantiated by the derived class.

Leaves if memory allocation fails or the base class constructor leaves.

@see CAgentBase::ConstructL()
*/
	{

	// construct the database and dialog processor
	CAgentBase::ConstructL();

	// create the callback for completing the connect
	TCallBack callback(NotifyCbComplete, this);
	iNotifyCb = new (ELeave) CAsyncCallBack(callback, CActive::EPriorityStandard);
	}


//
//                                                     //
//  Partial implementation of CNifAgentBase interface  //
//                                                     //
//

EXPORT_C void CStateMachineAgentBase::Connect(TAgentConnectType aType)
/**
Request that the State Machine establish a connection.

@param aType the type of connection to make 
(Outgoing, Reconnect, Callback, None, Incoming)
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tConnect(%d)"), this, aType); )

	switch(aType)
		{
		case EAgentStartDialOut:
		case EAgentStartDialIn:
			TRAPD(err, CreateAndStartStateMachineL());
			if (err!=KErrNone)
				{
				ConnectionComplete(err);
				}
			break;

		case EAgentReconnect:
			__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnReconnect));
			iStateMachine->ConnectionContinuation(CAgentSMBase::EReconnect);
			break;

		case EAgentStartCallBack:
			__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnCallBack));
			iStateMachine->ConnectionContinuation(CAgentSMBase::ECallBack);
			break;

		default:
			StateMachineAgentPanic(Agent::EUnknownStartType);
			break;
		}
	}

void CStateMachineAgentBase::CreateAndStartStateMachineL()
/**
Create the appropriate state machine and start the connection

@exception leaves if one of the database access or 
the creation of the state machine leaves
*/
	{

	__ASSERT_DEBUG(!iStateMachine, StateMachineAgentPanic(Agent::ENonNullStateMachineOnCreate));
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));
	iStateMachine = CreateAgentSML(*this, iDlgPrc, *iDatabase, iDatabase->GetConnectionDirection());

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnConnect));
	iStateMachine->StartConnect();
	}

EXPORT_C void CStateMachineAgentBase::Connect(TAgentConnectType aType, CStoreableOverrideSettings* aOverrideSettings)
/**
Request that the State Machine establish a connection using Overrides.

@param aType the type of connection to make (Outgoing, Reconnect, Callback, None, Incoming)
@param aOverrideSettings the database overrides to use for this connection
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tConnect(%d) with overrides(%x)"), this, aType, aOverrideSettings); )

	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TRAPD(err, SetOverridesL(aOverrideSettings));
	if (err!=KErrNone)
		{
		ConnectionComplete(err);
		}
	else
		{
		Connect(aType);
		}
	}

EXPORT_C void CStateMachineAgentBase::CancelConnect()
/**
Cancel a previous request to Connect.
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tCancelConnect()"), this); )

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnCancelConnect));

	// cancel any pending ServiceStarted() or ConnectComplete() callbacks
	if(iNotifyCb->IsActive() && (iNotifyCbOp==EConnectComplete || iNotifyCbOp==EServiceStarted))
		{
		iNotifyCb->Cancel();

		LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tCancelled NotifyCb() for op=%d"), this, iNotifyCbOp); )
		}

	iStateMachine->CancelConnect();
	}

EXPORT_C void CStateMachineAgentBase::Disconnect(TInt aReason)
/**
Request that the State Machine tears down a connection.

@param aReason the reason for the request to disconnect
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tDisconnect(%d)"), this, aReason); )

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnDisconnect));
	
	CAgentBase::CancelAuthenticate();

	iDisconnectReason = aReason;
	iStateMachine->ConnectionContinuation(CAgentSMBase::EDisconnect);
	}

EXPORT_C TInt CStateMachineAgentBase::GetExcessData(TDes8& aBuffer)
/**
Get Excess Data from the State Machine.

@param aBuffer the buffer in which to store the excess data
@todo Is this used?
@return
*/
	{

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnGetExcessData));
	
	return iStateMachine->GetExcessData(aBuffer);
	}

EXPORT_C TInt CStateMachineAgentBase::Notification(TNifToAgentEventType aEvent, TAny* aInfo)
/**
Notification from the NIF to the State Machine.

NIFMAN does not interpret this message

@param aEvent 
@param aInfo 
*/
	{

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnNotification));

	return iStateMachine->Notification(aEvent, aInfo);
	}

EXPORT_C void CStateMachineAgentBase::GetLastError(TInt& aError)
/**
Return the last error from the State Machine.

@param aError on return contains the error
*/
	{

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnGetLastError));

	iStateMachine->GetLastError(aError);
	}

EXPORT_C TBool CStateMachineAgentBase::IsReconnect() const
/**
Is this a reconnection? - required for Authenticate()

@return ETrue if this is a reconnection,
         EFalse otherwise
*/
	{

	__ASSERT_DEBUG(iStateMachine, StateMachineAgentPanic(Agent::ENullStateMachineOnIsReconnect));

	return iStateMachine->IsReconnect();
	}

//
// Implementation of MAgentNotify interface
//

EXPORT_C void CStateMachineAgentBase::PreventConnectionRetries()
/**
@todo we need to work out how to do this & whether it should still be supported
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tPreventConnectionRetries()"), this); )
	}

EXPORT_C void CStateMachineAgentBase::ServiceStarted()
/**
ServiceStarted.
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tServiceStarted()"), this); )

	CallNotifyCb(EServiceStarted, KErrNone);
	}

EXPORT_C void CStateMachineAgentBase::ConnectionComplete(TInt aProgress, TInt aError)
/**
ConnectionComplete upcall from the state machine

@param aProgress
@param aError on return contains the error
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tConnectionComplete(%d, %d)"), this, aProgress, aError); )

	UpdateProgress(aProgress,aError);
	ConnectionComplete(aError);
	}

EXPORT_C void CStateMachineAgentBase::ConnectionComplete(TInt aError)
/**
ConnectionComplete upcall from the state machine

@param aError on return contains the error
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tConnectionComplete()"), this); )

	CallNotifyCb(EConnectComplete, aError);
	}

EXPORT_C void CStateMachineAgentBase::DisconnectComplete()
/**
DisconnectComplete upcall from the state machine
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tDisconnectComplete()"), this); )

	delete iStateMachine;
	iStateMachine = NULL;

	CallNotifyCb(EDisconnectComplete, KErrNone);
	}

EXPORT_C void CStateMachineAgentBase::UpdateProgress(TInt aProgress, TInt aError)
/**
UpdateProgress upcall from the state machine

@param aProgress
@param aError on return contains the error
*/
	{
	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	iNotify->AgentProgress(aProgress, aError);
	}

EXPORT_C TInt CStateMachineAgentBase::Notification(TAgentToNifEventType aEvent, TAny* aInfo)
/**
Notification upcall from the state machine

@param aEvent
@param aInfo
*/
	{
	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	return iNotify->Notification(aEvent, aInfo);
	}

EXPORT_C TInt CStateMachineAgentBase::IncomingConnectionReceived()
/**
IncomingConnectionReceived upcall from the state machine

@return
*/
	{
	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	return iNotify->IncomingConnectionReceived();
	}


//
//                                                 //
//  Callback used for notifying NIFMAN             //
//                                                 //
//

void CStateMachineAgentBase::CallNotifyCb(TNotifyOperation aOperation, TInt aError)
/**
CallNotifyCb

@param aOperation the MNifAgtNotify function to call
@param aError the error to pass to Nifman
*/
	{
	__ASSERT_DEBUG(!iNotifyCb->IsActive(), StateMachineAgentPanic(Agent::ENotifyCallbackAlreadyPending));

	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tCallNotifyCb() op=%d, err=%d"), this, aOperation, aError); )

	iNotifyCbOp = aOperation;
	iNotifyCbError = aError;
	iNotifyCb->CallBack();
	}

TInt CStateMachineAgentBase::NotifyCbComplete(TAny* aThisPtr)
/**
NotifyCbComplete

@param aThisPtr pointer to the instance object that triggered the callback
@return KErrNone
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent:\t\tNotifyCbComplete(%x)"), aThisPtr); )

	((CStateMachineAgentBase*)aThisPtr)->DoNotify();
	return KErrNone;
	}

void CStateMachineAgentBase::DoNotify()
/**
Notify NIFMAN that part of the connection process has completed

*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("StateMachAgent %x:\tDoNotify() op=%d, err=%d"), this, iNotifyCbOp, iNotifyCbError); )

	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	TNotifyOperation notifyCbOp = iNotifyCbOp;
	TInt notifyCbError = iNotifyCbError;

	iNotifyCbOp = EUndefined;
	iNotifyCbError = KErrNone;

	switch(notifyCbOp)
		{
		case EConnectComplete:
			{
			iNotify->ConnectComplete(notifyCbError);
			break;
			}
		case EServiceStarted:
			{
			iNotify->ServiceStarted();
			break;
			}
		case EDisconnectComplete:
			{
			iNotify->DisconnectComplete();
			break;
			}
		default:
			StateMachineAgentPanic(Agent::EUndefinedNotifyOperation);
			break;
		}
	}

