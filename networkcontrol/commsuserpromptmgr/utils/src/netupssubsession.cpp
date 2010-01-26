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
// This file provides the implementation of the methods for NetUps SubSession
// @internalAll
// @prototype
// 
//

#include <e32base.h>		// define Active Scheduler
#include <e32property.h>	// defines properties

#include <ups/upsclient.h>
#include <ups/upstypes.h>

#include "netupsassert.h"
#include "netupskeys.h"

#include "netupsimpl.h"
#include "netupsstatemachine.h"
#include "netupssubsession.h"
#include "netupsprocessentry.h"
#include "netupsthreadentry.h"
#include "netupsthreadmonitor.h"
#include "netupspolicycheckrequestqueue.h"

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CSubSession* CSubSession::NewL(UserPromptService::RUpsSession& aUpsSession, CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry&	aThreadEntry)
	{
	CSubSession* self = new (ELeave) CSubSession(aDatabaseEntry, aProcessEntry, aThreadEntry);

	CleanupStack::PushL(self);
	self->ConstructL(aUpsSession);
	CleanupStack::Pop(self);

	CActiveScheduler::Add(self);
	return self;	
	}
	
CSubSession::CSubSession(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry&	aThreadEntry)
  : CActive(EPriorityStandard), iDatabaseEntry(aDatabaseEntry), iProcessEntry(aProcessEntry), iThreadEntry(aThreadEntry),
    iDecision(EUpsDecNo), iCommsId(NULL), iPolicyCheckRequestOriginator(NULL), iReadyForDeletion(EFalse)
    {
#ifdef _DEBUG    
	iState = EPCRPIdle;
#endif // _DEBUG
	}

CSubSession::~CSubSession()
	{
	if (IsActive())
		{
		Cancel();	
		}

	iUpsSubSession.Close();	
	iUpsOpaqueData.Close();
	
#ifdef _DEBUG
	iSimulatedDelay.Close();
#endif // _DEBUG

	__FLOG_1(_L("CSubSession %08x:\t~CSubSession()"), this);	
	__FLOG_CLOSE;	
	}

TInt CSubSession::Authorise(const CPolicyCheckRequestData& aPolicyCheckRequestData)
	{
	__FLOG_6(_L("CSubSession %08x:\tAuthoriseL() processId = %d, threadId = %d, serviceId = %d, platsec result = %d, Originator  = %08x"),
		this, aPolicyCheckRequestData.iProcessId.Id(), aPolicyCheckRequestData.iThreadId.Id(), (TInt32) aPolicyCheckRequestData.iServiceId, aPolicyCheckRequestData.iPlatSecCheckResult, &aPolicyCheckRequestData.iPolicyCheckRequestOriginator);

	TInt rc = KErrNone;

	_LIT(KAp,		  "AccessPoint = \"");	
	_LIT(KTerminator, "\"");

	TInt length = aPolicyCheckRequestData.iDestinationName.Length() + 
				  aPolicyCheckRequestData.iAccessPointName.Length();
	if (length > EMaxUnformattedLength)
		{
		__FLOG_4(_L("CSubSession %08x:\t AuthoriseL() = %d, iDestinationName.Length() = %d, iAccessPointName.Length() = %d, length = %d"), CActive::IsActive(), aPolicyCheckRequestData.iDestinationName.Length(), aPolicyCheckRequestData.iAccessPointName.Length(), length);
		rc = KErrOverflow;
		}
	else
		{
		iPolicyCheckRequestOriginator 	= &const_cast<MPolicyCheckRequestOriginator&>(aPolicyCheckRequestData.iPolicyCheckRequestOriginator);
		iCommsId 						= &aPolicyCheckRequestData.iCommsId;	

		ASSERT(iUpsOpaqueData.Length() == 0);
		iUpsOpaqueData.Append(KAp);
		iUpsOpaqueData.Append(aPolicyCheckRequestData.iAccessPointName);
		iUpsOpaqueData.Append(KTerminator);			
		
		iUpsSubSession.Authorise(aPolicyCheckRequestData.iPlatSecCheckResult,  
				 TUid::Uid(aPolicyCheckRequestData.iServiceId),
				 aPolicyCheckRequestData.iDestinationName,
				 iUpsOpaqueData, 
				 iDecision, 
				 iStatus);

#ifdef _DEBUG
		iState =  EPCRPSimulateDelay;
#endif // _DEBUG

		SetActive();		
		}
	
	return rc;
	}

void CSubSession::RunL()
	{
	__FLOG_3(_L("CSubSession %08x:\tRunL(): IsActive() = %d, iStatus.Int() = %d"), this, CActive::IsActive(), iStatus.Int());	

#ifdef _DEBUG
	switch (iState)
		{
		case EPCRPSimulateRequest:
#endif
			{
			__FLOG_5(_L("CSubSession %08x:\tRunL() iDecision = %d, iCommsId = %08x, iPolicyCheckRequestOriginator = %08x, iReadyForDeletion = %d"), this, iDecision, iCommsId, iPolicyCheckRequestOriginator, iReadyForDeletion);	

			__ASSERT_DEBUG((iPolicyCheckRequestOriginator != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSubSession));
			__ASSERT_DEBUG((iCommsId 					  != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetUpsSubSession1));

			TNetUpsDecision netUpsDecision = static_cast<TNetUpsDecision>(iDecision);
			TCommsIdKey commsIdKey(iDatabaseEntry, iProcessEntry, iThreadEntry, *iCommsId);

#ifdef _DEBUG
			// Logic to put the NetUps into its error paths; don't include in production build
			ModifyIStatusValue(netUpsDecision);
#endif
			if (iStatus == KErrNone)
				{
				iProcessEntry.UpsStateMachine().HandleUPSRequestCompletionL(commsIdKey, *iPolicyCheckRequestOriginator, netUpsDecision); 	
				// if HandleUPSRequestCompletionL() leaves, then the error is managed in CSubSession::RunError(TInt aError) below
				}
			else	
				{
				// no state change has occured, at the most a threadEntry, processEntry, and databaseEntry have been created which
				// can be just left for the moment - could consider removing them later.
				iProcessEntry.UpsStateMachine().HandleUPSErrorOnCompletionL(commsIdKey, *iPolicyCheckRequestOriginator, iStatus.Int()); 	
				}

			// Reset the temporary variables
			ResetAuthorisationRequestParameters();
			
			DeleteCurrentRequestFromQueue();
			
			if (SubSessionReadyForDeletion() == EFalse)
				{
				PostNextRequestFromQueue();
				}
			else
				{
				delete this;
				}		
#ifdef _DEBUG
			break;	
#endif
			}
#ifdef _DEBUG
		case EPCRPSimulateDelay:
			{
			iSimulatedDelay.After(iStatus,EDelay);
			iState = EPCRPSimulateRequest;
			SetActive();	
					
			break;				
			}
		default:
			User::Panic(KNetUpsPanic, KPanicInvalidLogic);
			break;
		}
#endif // _DEBUG
	}

void CSubSession::DoCancel()
	{
	__FLOG_1(_L("CSubSession %08x:\tDoCancel()"), this);	

	iUpsSubSession.CancelPrompt();

#ifdef _DEBUG
	iSimulatedDelay.Cancel();
#endif

	if (iCommsId)
		{
		iThreadEntry.RemoveCommsId(*iCommsId);		
		} 

	ResetAuthorisationRequestParameters();
	
#ifdef _DEBUG
	User::After(ECancelDelay);
#endif	
	}

void CSubSession::DeleteCurrentRequestFromQueue()
	{
	// Determine whether the last entry on the queue should be flushed.
	// The logic here needs to be reviewed as there are various cases
	// when the thread entry exists and when it does not. The up shot is
	// that is is probably better to remove the entry of the queueu during
	// the HandleUpsRequestCompletion, to keep the logic all in the one place.
	// However, for now:			
	
	CNetUpsImpl::TSessionMode 	sessionMode  = CNetUpsImpl::MapInternalStateToSessionMode(iProcessEntry.NetUpsState());
	CNetUpsImpl::TLifeTimeMode 	lifeTimeMode = iProcessEntry.UpsStateMachine().NetUpsImpl().LifeTimeMode();

	switch(sessionMode)
		{			
		case CNetUpsImpl::ENonSessionMode:
			// finished with current entry, delete it.
			iThreadEntry.RequestQueue().DeleteCurrentEntry();
			break;
		case CNetUpsImpl::ESessionMode:
			switch(lifeTimeMode)
				{
				case CNetUpsImpl::EProcessLifeTimeMode:
					// thread entry gone, queue already deleted.
					break;
				case CNetUpsImpl::ENetworkLifeTimeMode:
					// thread entry remains, delete last queue entry
					iThreadEntry.RequestQueue().DeleteCurrentEntry();
					break;
				default:
					User::Panic(KNetUpsPanic, KPanicInvalidLogic);
					break;
				}
			break;
		default:
			User::Panic(KNetUpsPanic, KPanicInvalidLogic);
			break;
		}			
	}

void CSubSession::PostNextRequestFromQueue()
	{
#ifdef _DEBUG
	iState = EPCRPIdle;
#endif
	// determine there is an opportunity for the next entry to be forwarded to the UPS server.			
	while ((iThreadEntry.RequestQueue().IsEmpty() == EFalse) && (IsActive() == EFalse))
		{
		CPolicyCheckRequestData&  policyCheckRequestData = iThreadEntry.RequestQueue().GetNextRequest();
		__FLOG_2(_L("CSubSession: %08x:\t RunL(), post next request, policyCheckRequest = %08x"), this, &policyCheckRequestData);		
		TInt err = Authorise(policyCheckRequestData);
		if (err != KErrNone)
			{
			const Messages::TNodeId& commsId = policyCheckRequestData.iCommsId;
			policyCheckRequestData.iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(KErrCancel, commsId);
			iThreadEntry.RequestQueue().DeleteCurrentEntry();
#ifdef _DEBUG
			iState = EPCRPIdle;
#endif
			}
		}	
	}

void CSubSession::ConstructL(UserPromptService::RUpsSession& aUpsSession)
	{
	// 	Interesting opportunity to log the process and thread ids
	//	RProcess process;
	// 	TSecureId secureId = process.SecureId();
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);
	__FLOG_1(_L("CSubSession %08x:\tConstructL()"), this);	
	RThread thread;
	TInt ret = thread.Open(iThreadEntry.ThreadId());
	if (ret != KErrNone)
		{
		__FLOG_2(_L("CSubSession %08x:\tError %d opening thread %d\n"), ret, iThreadEntry.ThreadId().Id());
		User::Leave(ret);
		}
	(void) User::LeaveIfError(iUpsSubSession.Initialise(aUpsSession, thread));
	
	thread.Close();
	
	iUpsOpaqueData.CreateL(EMaxFormattedLength);
	#ifdef _DEBUG
	(void) User::LeaveIfError(iSimulatedDelay.CreateLocal()); // Add temporary delay for Nadeem
#endif

	}

void CSubSession::SetReadyForDeletion()
	{
	__FLOG_1(_L("CSubSession %08x:\tSetReadyForDeletion()"), this);	
	iReadyForDeletion = ETrue;
	}

TBool CSubSession::SubSessionReadyForDeletion()
	{
	return iReadyForDeletion;
	}

void CSubSession::ResetAuthorisationRequestParameters()
	{
	iUpsOpaqueData.Zero();	
	iPolicyCheckRequestOriginator 	= NULL;	
	iCommsId 						= NULL;	
	}

TInt CSubSession::RunError(TInt aError)
	{
	__FLOG_2(_L("CSubSession %08x:\tRunError(), aError = %d"), this, aError);	
	(void) aError;

	// Handles Leaves from CSubSession::RunL.
	// We don't allocate any memory in the RunL, so this method should never be called.
	
	return KErrNone;	
	}

#ifdef _DEBUG
void CSubSession::ModifyIStatusValue(TNetUpsDecision aNetUpsDecision)
	{
	TInt rc = RProperty::Get(KUidNetUpsTestNotifCategory, KNetUpsSubSessionError, iTestErrorCode);
	if (  	(rc == KErrNone) &&
			(iTestErrorCode != KErrNone) && 
		  ( ((aNetUpsDecision == EYes)  && 
		    (iProcessEntry.NetUpsState() == EProcLife_NonSession) || (iProcessEntry.NetUpsState() == ENetLife_NonSession))
		     ||
			 ((aNetUpsDecision == ESessionYes) &&
		     ((iProcessEntry.NetUpsState() == EProcLife_Transit_SessionYes ) || (iProcessEntry.NetUpsState() == EProcLife_Transit_SessionNo ) ||
		      (iProcessEntry.NetUpsState() == ENetLife_SessionNo_Transit_WithoutConnections  ) || (iProcessEntry.NetUpsState() == ENetLife_SessionNo_Transit_WithConnections  ) ||
		      (iProcessEntry.NetUpsState() == ENetLife_Transit_SessionYes  )) )
		      ) )
		{
		iStatus = iTestErrorCode;
		}	
	}
#endif

} // end of namespace
