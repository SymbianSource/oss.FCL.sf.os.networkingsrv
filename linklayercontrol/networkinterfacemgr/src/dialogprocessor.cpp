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
//

/**
 @file
*/

#include "CAgentSMBase.h"
#include "dialogprocessor.h"
#include "AgentPanic.h"
#include "Ni_Log.h"


NONSHARABLE_CLASS(CDialogDestroyPCTNotification) : public CActive
/**
Declaration of this private class - forward reference in dialogprocessor.h
*/
	{
public:
	CDialogDestroyPCTNotification(RGenConAgentDialogServer* aDialogSvr,
                                  CDialogProcessor* aDlgPrc,
                                  TInt aPriority = CActive::EPriorityStandard);
	virtual ~CDialogDestroyPCTNotification();
	void RequestNotification();
private:

	/** From CActive */
	virtual void DoCancel();
	virtual void RunL();
private:
	RGenConAgentDialogServer* iDialogSvr;
	CDialogProcessor* iDlgPrc;
	};

//
// MDialogProcessorObserver
//

EXPORT_C void MDialogProcessorObserver::MDPOSelectComplete(TInt /*aError*/, const TConnectionSettings& /*aSettings*/)
	{ AgentPanic(Agent::EDialogProcessorSelectObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOSelectModemAndLocationComplete(TInt /*aError*/, const TConnectionSettings& /*aSettings*/)
	{ AgentPanic(Agent::EDialogProcessorSelectModemAndLocationObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOWarnComplete(TInt /*aError*/, TBool /*aResponse*/)
	{ AgentPanic(Agent::EDialogProcessorWarnObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOLoginComplete(TInt /*aError*/)
	{ AgentPanic(Agent::EDialogProcessorLoginObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOAuthenticateComplete(TInt /*aError*/)
	{ AgentPanic(Agent::EDialogProcessorAuthObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOReconnectComplete(TInt /*aError*/)
	{ AgentPanic(Agent::EDialogProcessorReconnectObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOReadPctComplete(TInt /*aError*/)
	{ AgentPanic(Agent::EDialogProcessorReadPctObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPODestroyPctComplete(TInt /*aError*/)
	{ AgentPanic(Agent::EDialogProcessorDestroyPctObserverNotImplemented); }

EXPORT_C void MDialogProcessorObserver::MDPOQoSWarningComplete(TInt /*aError*/, TBool /*aResponse*/)
	{ AgentPanic(Agent::EDialogProcessorWarnQoSObserverNotImplemented); }

//
// CDialogProcessor definitions
//

EXPORT_C CDialogProcessor* CDialogProcessor::NewL(TInt aPriority)
	{
	CDialogProcessor* self = new(ELeave) CDialogProcessor(aPriority);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(); // self
	return self;
	}

CDialogProcessor::CDialogProcessor(TInt aPriority) 
	: CActive(aPriority)
	{}

void CDialogProcessor::ConstructL()
	{
	CActiveScheduler::Add(this);
	TInt ret = iDlgServ.Connect();
	LOG(NifmanLog::Printf(_L("CDialogProcessor::ConstructL - Connect returned %d"),ret); )
	User::LeaveIfError(ret);
	iDestroyPctNotification = new(ELeave) CDialogDestroyPCTNotification(&iDlgServ,this);
	iState = ENoState;
	}

EXPORT_C CDialogProcessor::~CDialogProcessor()
	{
	CancelEverything();
	delete iDestroyPctNotification;
	iDlgServ.Close();
	}

EXPORT_C void CDialogProcessor::SelectConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs)
/**
Request connection dialog
*/
	{
	iDlgServ.IapConnection(iSettings.iIAPId, aPrefs, iStatus);

	SetActive(aObserver,ESelectConnection);
	}

EXPORT_C void CDialogProcessor::SelectConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs, TInt aLastError)

/**
Request connection dialog
*/
	{
	iDlgServ.IapConnection(iSettings.iIAPId, aPrefs, aLastError, iStatus);

	SetActive(aObserver,ESelectConnection);
	}

EXPORT_C void CDialogProcessor::SelectModemAndLocation(MDialogProcessorObserver& aObserver)
/**
Request Modem Dialog
*/
	{
	iDlgServ.ModemAndLocationSelection(iSettings.iBearerId,iSettings.iLocationId,iStatus);
	SetActive(aObserver,ESelectModemAndLocation);
	}

EXPORT_C void CDialogProcessor::Authenticate(MDialogProcessorObserver& aObserver, TDes& aUsername,TDes& aPassword,TBool aIsReconnect)
/**
Request authentication dialog
*/
	{
	iDlgServ.Authenticate(aUsername,aPassword,aIsReconnect,iStatus);
	SetActive(aObserver,EAuthentication);
	}

EXPORT_C void CDialogProcessor::Reconnect(MDialogProcessorObserver& aObserver)
/**
Request reconnect dialog
*/
	{
	iDlgServ.Reconnect(iReconResponse,iStatus);
	SetActive(aObserver,EReconnect);
	}

EXPORT_C void CDialogProcessor::WarnNewConnection(MDialogProcessorObserver& aObserver, const TConnectionPrefs& aPrefs, const TDesC* aNewIapName, const TIspConnectionNames*, TInt aLastError)
/**
Warn of new connection dialog
*/
	{
	__ASSERT_DEBUG(aNewIapName, AgentPanic(Agent::ENullWarnParameter));
	iDlgServ.WarnNewIapConnection(aPrefs, aLastError, *aNewIapName, iWarnNewConnectResponse, iStatus);

	SetActive(aObserver,EWarnNewConnection);
	}

EXPORT_C void CDialogProcessor::Login(MDialogProcessorObserver& aObserver, TDes& aUsername, TDes& aPassword,TBool aIsReconnect)
/**
Request login dialog
*/
	{
	iDlgServ.Login(aUsername,aPassword,aIsReconnect,iStatus);
	SetActive(aObserver,ELogin);
	}

EXPORT_C void CDialogProcessor::DestroyPctNotification(MDialogProcessorObserver& aObserver)
/**
Request notification on destruction of PCT. 
*/
	{
	iDestroyPctNotification->RequestNotification();
	iPctDestructionObserver = &aObserver;
	}

EXPORT_C void CDialogProcessor::ReadPct(MDialogProcessorObserver& aObserver, TDes& aData)
/**
Request read from PCT and return read string in aData
*/
	{
	iDlgServ.ReadPct(aData,iStatus);
	SetActive(aObserver,EReadPct);
	}

EXPORT_C void CDialogProcessor::QoSWarning(MDialogProcessorObserver& aObserver)
/**
Warn QoS has fallen below minimum values
*/
	{
	iDlgServ.QoSWarning(iQoSWarningResponse, iStatus);
	SetActive(aObserver,EQoSWarning);
	}

EXPORT_C TInt CDialogProcessor::OpenPct()
/**
Request open of PCT
*/
	{
	return iDlgServ.OpenPct();
	}

EXPORT_C TInt CDialogProcessor::WritePct(const TDesC& aData)
/**
Request write aData to PCT
*/
	{
	return iDlgServ.WritePct(aData);
	}

EXPORT_C void CDialogProcessor::ClosePct()
/**
Request close of PCT
*/
	{
	iDlgServ.ClosePct();
	}	

EXPORT_C void CDialogProcessor::CancelEverything()
/**
Cancel the active object waiting on the destroy pct notification 
and then everything else outstanding
*/
	{
	if (iDestroyPctNotification)
		{
		iDestroyPctNotification->Cancel();
		// Null the observer pointer, the observer is owned by other objects!
		iPctDestructionObserver = NULL;
		}

	Cancel();
	}	

void CDialogProcessor::CompleteDestroyPctNotification(TInt aStatus)
/**
Complete notification on destruction of PCT
*/
	{
	if (iPctDestructionObserver)
		{
		iPctDestructionObserver->MDPODestroyPctComplete(aStatus);
		// Null the observer pointer, the observer is owned by other objects!
		iPctDestructionObserver = NULL;
		}
	}

void CDialogProcessor::SetActive(MDialogProcessorObserver& aObserver, TDPState aState)
	{
	__ASSERT_DEBUG(!iCurrentObserver, AgentPanic(Agent::EObserverNotNull));

	iCurrentObserver = &aObserver;
	iState = aState;
	CActive::SetActive();
	}

void CDialogProcessor::RunL()
	{
	__ASSERT_DEBUG(iCurrentObserver, AgentPanic(Agent::EObserverNull));

	const TDPState state = iState;
	iState = ENoState;
	MDialogProcessorObserver* currentObserver = iCurrentObserver;
	iCurrentObserver = NULL;

	switch (state)
		{
	case ESelectConnection:
		currentObserver->MDPOSelectComplete(iStatus.Int(),iSettings);
		break;
	case EWarnNewConnection:
		currentObserver->MDPOWarnComplete(iStatus.Int(),iWarnNewConnectResponse);
		break;
	case ESelectModemAndLocation:
		currentObserver->MDPOSelectModemAndLocationComplete(iStatus.Int(),iSettings);
		break;
	case ELogin:
		currentObserver->MDPOLoginComplete(iStatus.Int());
		break;
	case EAuthentication:
		currentObserver->MDPOAuthenticateComplete(iStatus.Int());
		break;
	case EReconnect:
		if(iReconResponse)
			currentObserver->MDPOReconnectComplete(KErrNone);
		else
			currentObserver->MDPOReconnectComplete(KErrCancel);
		break;
	case EReadPct:
		currentObserver->MDPOReadPctComplete(iStatus.Int());
		break;
	case EQoSWarning:
		currentObserver->MDPOQoSWarningComplete(iStatus.Int(),iQoSWarningResponse);
		break;
	default:
		User::Leave(KErrGeneral);
		break;
		}
	}

void CDialogProcessor::DoCancel()
/**
Only called if CDialogProcessor is active (Could be inactive while iDestroyPctNotification
is still active so should call CancelEverything() instead if pct around)
*/
	{ 
	__ASSERT_DEBUG(!iPctDestructionObserver, AgentPanic(Agent::EDestroyNotificationNotCancelled));

	switch (iState)
		{
	case ESelectConnection:
		iDlgServ.CancelIapConnection();
		break;
	case ESelectModemAndLocation:
		iDlgServ.CancelModemAndLocationSelection();
		break;
	case EWarnNewConnection:
		iDlgServ.CancelWarnNewIapConnection();
		break;
	case ELogin:
		iDlgServ.CancelLogin();
		break;
	case EAuthentication:
		iDlgServ.CancelAuthenticate();
		break;
	case EReconnect:
		iDlgServ.CancelReconnect();
		break;
	case EReadPct:
		iDlgServ.CancelReadPct();
		break;
	case EQoSWarning:
		iDlgServ.CancelQoSWarning();
		break;
	default:
		AgentPanic(Agent::EIllegalActionType);
		break;
		}

	iState = ENoState;
	iCurrentObserver = NULL;
	}

//
// CDialogDestroyPCTNotification
//

CDialogDestroyPCTNotification::CDialogDestroyPCTNotification(RGenConAgentDialogServer* aDialogSvr,
															 CDialogProcessor* aDlgPrc,
															 TInt aPriority)
	: CActive(aPriority), iDialogSvr(aDialogSvr), iDlgPrc(aDlgPrc)
	{
	CActiveScheduler::Add(this);
	}

CDialogDestroyPCTNotification::~CDialogDestroyPCTNotification()
	{
	Cancel();
	}

void CDialogDestroyPCTNotification::RequestNotification()
/**
Request notification when the PCT is destroyed
*/
	{
	iDialogSvr->DestroyPctNotification(iStatus);
	SetActive();
	}

void CDialogDestroyPCTNotification::DoCancel()
/**
This will cancel any oustanding requests and cause the destroy notification to complete - which will
get caught by the CActive::Cancel()
*/
	{
	iDialogSvr->CancelDestroyPctNotification();
	}

void CDialogDestroyPCTNotification::RunL()
/**
Received notification
*/
	{
	iDlgPrc->CompleteDestroyPctNotification(iStatus.Int());
	}

