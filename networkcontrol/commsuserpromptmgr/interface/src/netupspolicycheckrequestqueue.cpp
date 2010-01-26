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
// policycheckrequestqueue.cpp
// This file provides the implementation of the inline methods for the
// CPolicyCheckRequestQueue (implemented as a single linked list),
// TQueuedPolicyCheckRequest (the representation of its constituent
// nodes) and TPolicyCheckRequestData_NonRef ( a container for 
// policy check request attributes).
// @internalAll
// @prototype
// 
//

#include <e32base.h>						// Definition for CActiveScheduler::Add()

#include <comms-infras/ss_activities.h>

#include "netupstypes.h"					// defines the request id.

#include "netupsassert.h"
#include "netupspolicycheckrequestqueue.h"
#include "netupssubsession.h"				// Defines NetUpsSubSession

#include <comms-infras/commsdebugutility.h> // defines the comms debug logging utility

namespace NetUps
{
__FLOG_STMT(_LIT8(KNetUpsSubsys, 	"esock");)   
__FLOG_STMT(_LIT8(KNetUpsComponent, "NetUps");) /*esockloader*/

CPolicyCheckRequestData* CPolicyCheckRequestData::NewL(const TPolicyCheckRequestData& aPolicyCheckRequestData)
	{
	CPolicyCheckRequestData*  self = new (ELeave) CPolicyCheckRequestData(aPolicyCheckRequestData);

	CleanupStack::PushL(self);
	self->ConstructL(aPolicyCheckRequestData.iDestinationName, aPolicyCheckRequestData.iAccessPointName);
	CleanupStack::Pop(self);	

	return self;	
	}

CPolicyCheckRequestData::~CPolicyCheckRequestData()
	{
	iDestinationName.Close();
	iAccessPointName.Close();	

	__FLOG_1(_L("CPolicyCheckRequestData %08x:\t ~CQueuedPolicyCheckRequest()"), this);
	__FLOG_CLOSE;	
	}
		
void CPolicyCheckRequestData::ConstructL(const TDesC& 	aDestinationName,
										 const TDesC& 	aAccessPointName)
	{
	iDestinationName.CreateL(aDestinationName);
	iAccessPointName.CreateL(aAccessPointName);

	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);			
	//__FLOG_7(_L("CPolicyCheckRequestData %08x:\t ConstructL() iProcessId.Id() = %d, iThreadId.Id() = %d, iServiceId = %d, iPlatSecCheckResult = %d, iCommsId = %08x, iPolicyCheckRequestOriginator  = %08x"), this, iProcessId.Id(), iThreadId.Id(), iServiceId, iPlatSecCheckResult, iCommsId, iPolicyCheckRequestOriginator);
	__FLOG_6(_L("CPolicyCheckRequestData %08x:\t ConstructL() iProcessId.Id() = %d, iThreadId.Id() = %d, iServiceId = %d, iPlatSecCheckResult = %d, iPolicyCheckRequestOriginator  = %08x"), this, iProcessId.Id(), iThreadId.Id(), iServiceId, iPlatSecCheckResult, &iPolicyCheckRequestOriginator);

	}

CQueuedPolicyCheckRequest* CQueuedPolicyCheckRequest::NewL(const TPolicyCheckRequestData& aPolicyCheckRequestData, const TRequestId& aRequestId)
	{
	CQueuedPolicyCheckRequest*  self = new (ELeave) CQueuedPolicyCheckRequest(aRequestId);

	CleanupStack::PushL(self);
	self->ConstructL(aPolicyCheckRequestData);
	CleanupStack::Pop(self);	

	return self;
	}


void CQueuedPolicyCheckRequest::ConstructL(const TPolicyCheckRequestData& aPolicyCheckRequestData)
	{
	iPolicyCheckRequestData = CPolicyCheckRequestData::NewL(aPolicyCheckRequestData);
	}

CQueuedPolicyCheckRequest::CQueuedPolicyCheckRequest(const TRequestId& aRequestId)
  : iRequestId(aRequestId)
	{
	}

CQueuedPolicyCheckRequest::~CQueuedPolicyCheckRequest()
	{
	delete iPolicyCheckRequestData;
	}

const TRequestId& CQueuedPolicyCheckRequest::RequestId()
	{
	return iRequestId;	
	}

CPolicyCheckRequestQueue* CPolicyCheckRequestQueue::NewL(CSubSession& aSubsession)
	{
	CPolicyCheckRequestQueue*  self = new (ELeave) CPolicyCheckRequestQueue(aSubsession);

	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);	

	return self;
	}

CPolicyCheckRequestQueue::~CPolicyCheckRequestQueue()
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\t ~CPolicyCheckRequestQueue()"), this);		

	// If the destructor is being called, the SubSession should be inactive - however, cannot check this
	// as it may have already been deleted.
	
	// Under normal circumstances, the assumption is that the client code has emptied the queue before
	// deleting the queue. However is this always true ? What happens if the NetUps is deleted with
	// outstanding messages on the queue ?
	__FLOG_2(_L("CPolicyCheckRequestQueue %08x:\t QueueL() iRequestQueue.IsEmpty() = %d"), this, iRequestQueue.IsEmpty());		
	//__ASSERT_DEBUG((iRequestQueue.IsEmpty() != EFalse), User::Panic(KNetUpsPanic, KPanicInvalidLogic));

	while (iRequestQueue.IsEmpty() == EFalse)
		{
		CQueuedPolicyCheckRequest* queuedPolicyCheckRequest = iRequestQueue.First();
		__ASSERT_DEBUG((queuedPolicyCheckRequest != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetupsPolicyCheckRequestQueue));
		iRequestQueue.Remove(*queuedPolicyCheckRequest);
		delete queuedPolicyCheckRequest;
		}

	__FLOG_CLOSE;	
	}

void CPolicyCheckRequestQueue::SendRequestL(const TPolicyCheckRequestData& aPolicyCheckRequestData, const TRequestId& aRequestId)  
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\tSendRequestL()"), this);		

	CQueuedPolicyCheckRequest* queuedPolicyCheckRequest = CQueuedPolicyCheckRequest::NewL(aPolicyCheckRequestData, aRequestId);
	iRequestQueue.AddLast(*queuedPolicyCheckRequest);

	PostFirstRequestOnQueueToUpsServer();
	}

void CPolicyCheckRequestQueue::PostFirstRequestOnQueueToUpsServer()  
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\tPostFirstRequestOnQueueToUpsServer()"), this);		

	while ((iRequestQueue.IsEmpty() == EFalse) && (iSubSession.IsActive() == EFalse))
		{
		CPolicyCheckRequestData& policyCheckData = GetNextRequest(); 
		__FLOG_3(_L("CPolicyCheckData %08x:\t post next request, policyCheckData = %08x, iSubSession = %08x"), this, &policyCheckData, &iSubSession);		
		TInt err = iSubSession.Authorise(policyCheckData);
		if (err != KErrNone)
			{
			const Messages::TNodeId& commsId = policyCheckData.iCommsId;
			policyCheckData.iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(KErrCancel, commsId);
			DeleteCurrentEntry();			
			}
		}
	}

void CPolicyCheckRequestQueue::ConstructL()
	{
	__FLOG_OPEN(KNetUpsSubsys, KNetUpsComponent);			
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\t ConstructL()"), this);		
	}
 
TInt CPolicyCheckRequestQueue::CancelRequest(const Messages::TNodeId& aCommsId, const TRequestId& aRequestId)
	{
	__FLOG_2(_L("CPolicyCheckRequestQueue %08x:\tCancelRequest(), requestId = %d"), this, aRequestId);		

	TInt rc = KErrNotFound;

	TSglQueIter<CQueuedPolicyCheckRequest>  requestQueueIterator(iRequestQueue);
	CQueuedPolicyCheckRequest* policyCheckRequest= NULL;
	
	while ((policyCheckRequest = requestQueueIterator++) != NULL)
		{
		if (policyCheckRequest->RequestId() ==  aRequestId)
			{
			__FLOG_2(_L("CPolicyCheckRequestQueue %08x:\t CancelRequest() policyCheckRequest = %08x"), this, policyCheckRequest);		
			if (iRequestQueue.IsFirst(policyCheckRequest) && iSubSession.IsActive())
				{
				iSubSession.Cancel();
				}
			
			// notify the client of the cancellation
			policyCheckRequest->PolicyCheckRequestData().iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(KErrCancel, aCommsId);

			// delete the current entry
			iRequestQueue.Remove(*policyCheckRequest);
			delete policyCheckRequest;
			policyCheckRequest	= NULL;

			PostFirstRequestOnQueueToUpsServer();
			
			rc = KErrNone;
			
			break;
			}
		}
	return rc;	
	}

void CPolicyCheckRequestQueue::CancelAllRequests(TInt aError)
	{
	__FLOG_2(_L("CPolicyCheckRequestQueue %08x:\tCancelAllRequests(), aError = %d"), this, aError);		

	if (iSubSession.IsActive() != EFalse)
		{
		iSubSession.Cancel();		
		}

	while (iRequestQueue.IsEmpty() == EFalse)
		{
		CPolicyCheckRequestData& policyCheckData = GetNextRequest(); 
		__FLOG_1(_L("Cancel next request, policyCheckData = %08x"), &policyCheckData);		
		policyCheckData.iPolicyCheckRequestOriginator.ProcessPolicyCheckResponse(aError, policyCheckData.iCommsId);
		DeleteCurrentEntry();			
		}
	}

void CPolicyCheckRequestQueue::DeleteCurrentEntry()
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\tDeleteCurrentEntry()"), this);		

	CQueuedPolicyCheckRequest* queuedPolicyCheckRequest = iRequestQueue.First();
	__ASSERT_DEBUG((queuedPolicyCheckRequest != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetupsPolicyCheckRequestQueue1));

	iRequestQueue.Remove(*queuedPolicyCheckRequest);
	delete queuedPolicyCheckRequest;	
	}

TBool CPolicyCheckRequestQueue::IsEmpty() const
	{
	return iRequestQueue.IsEmpty();
	}

TBool CPolicyCheckRequestQueue::OutStandingRequestToProcess()
	{
	TBool rc = EFalse;
	if ((iRequestQueue.IsEmpty() == EFalse) && (iRequestQueue.First() != iRequestQueue.Last())) 
		{
		rc = ETrue;
		}
	return rc;	
	}

CPolicyCheckRequestData& CPolicyCheckRequestQueue::GetOutStandingRequestToProcess()
	{
	__ASSERT_DEBUG( ((iRequestQueue.IsEmpty() == EFalse) && (iRequestQueue.First() != iRequestQueue.Last())),
	 User::Panic(KNetUpsPanic, KPanicInvalidLogic)); 
	return (iRequestQueue.Last())->PolicyCheckRequestData();
	}


void CPolicyCheckRequestQueue::DeleteOutStandingRequest()
	{
	__ASSERT_DEBUG( ((iRequestQueue.IsEmpty() == EFalse) && (iRequestQueue.First() != iRequestQueue.Last())),
	 User::Panic(KNetUpsPanic, KPanicInvalidLogic)); 

	CQueuedPolicyCheckRequest* queuedPolicyCheckRequest = iRequestQueue.Last();

	iRequestQueue.Remove(*queuedPolicyCheckRequest);
	delete queuedPolicyCheckRequest;
	}

CPolicyCheckRequestData& CPolicyCheckRequestQueue::GetNextRequest()
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\tGetNextRequest(), iSubSession = %08x"), this);		

	CQueuedPolicyCheckRequest* queuedPolicyCheckRequest = iRequestQueue.First();  
	__ASSERT_DEBUG((queuedPolicyCheckRequest != NULL), User::Panic(KNetUpsPanic, KPanicNullPointer_NetupsPolicyCheckRequestQueue2));

	return queuedPolicyCheckRequest->PolicyCheckRequestData();
	}

CPolicyCheckRequestQueue::CPolicyCheckRequestQueue(CSubSession& aSubSession) : iSubSession(aSubSession)		
	{
	__FLOG_1(_L("CPolicyCheckRequestQueue %08x:\tCPolicyCheckRequestQueue(), iSubSession = %08x"), this);		

	iRequestQueue.SetOffset(_FOFF(CQueuedPolicyCheckRequest,iLink));			
	}	

} // end of namespace netups

