
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
// policycheckrequestqueue.h
// This file provides the interface for the policy check request
// queue and its associated elements.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPS_PCR_QUEUE_H
#define NETUPS_PCR_QUEUE_H

#include <e32base.h>		// defines CBase, CActive
#include <e32std.h>			// defines TSglQueLink

#include "netupstypes.h"	

#include <comms-infras/commsdebugutility.h> // defines the comms debug logging utility

namespace NetUps
{
class CSubSession;
class MPolicyCheckRequestOriginator;

NONSHARABLE_CLASS (CPolicyCheckRequestData) : public CBase
	{
public:
	static CPolicyCheckRequestData* NewL(const TPolicyCheckRequestData& PolicyCheckRequestData);
	~CPolicyCheckRequestData();	
private:	
	void ConstructL(const TDesC& 								aDestinationName,
					const TDesC& 								aAccessPointName);
					
	CPolicyCheckRequestData(const TPolicyCheckRequestData& aPolicyCheckRequestData) :
							iProcessId(aPolicyCheckRequestData.iProcessId),
							iThreadId(aPolicyCheckRequestData.iThreadId),
							iServiceId(aPolicyCheckRequestData.iServiceId),
							iPlatSecCheckResult(aPolicyCheckRequestData.iPlatSecCheckResult),
							iCommsId(aPolicyCheckRequestData.iCommsId),
							iPolicyCheckRequestOriginator(aPolicyCheckRequestData.iPolicyCheckRequestOriginator)
							{						
							}			
public:
	TProcessId 										iProcessId;
	TThreadId 										iThreadId;
	TInt32 			  								iServiceId;
	TBool 		   									iPlatSecCheckResult;
	RBuf											iDestinationName;
	RBuf 											iAccessPointName;
	Messages::TNodeId	 								iCommsId;
	MPolicyCheckRequestOriginator&					iPolicyCheckRequestOriginator;	
private:
	__FLOG_DECLARATION_MEMBER;
	};

NONSHARABLE_CLASS (CQueuedPolicyCheckRequest) : public CBase
	{
	friend class CPolicyCheckRequestQueue; // needs iLink
public:
	static CQueuedPolicyCheckRequest* NewL(const TPolicyCheckRequestData& aPolicyCheckRequestData, const TRequestId& aRequestId);
	~CQueuedPolicyCheckRequest();
	CPolicyCheckRequestData& PolicyCheckRequestData() { return *iPolicyCheckRequestData; }
	const TRequestId& RequestId();
private:
	void ConstructL(const TPolicyCheckRequestData& PolicyCheckRequestData);						
	CQueuedPolicyCheckRequest(const TRequestId& aRequestId);
private:
	TRequestId					iRequestId;
	CPolicyCheckRequestData* 	iPolicyCheckRequestData;
	TSglQueLink iLink;
	};

NONSHARABLE_CLASS (CPolicyCheckRequestQueue) : public CBase
	{
public:
	static CPolicyCheckRequestQueue* NewL(CSubSession&);
	~CPolicyCheckRequestQueue();

	TBool IsEmpty() const;
	CPolicyCheckRequestData& GetNextRequest();  // used by the subSession to get 1st entry on queue.
	void DeleteCurrentEntry();

	TBool OutStandingRequestToProcess();		// used by the subSession to get entries other than the 1st entry in order
												// to reply outstanding requests when entering a Session State
												// (other than 1st entry for which are reply has already been received from the UPS Server).
	CPolicyCheckRequestData& GetOutStandingRequestToProcess();
	void DeleteOutStandingRequest();

	void SendRequestL(const TPolicyCheckRequestData& aPolicyCheckRequestData, const TRequestId& aRequestId);
	TInt CancelRequest(const Messages::TNodeId& aCommsId, const TRequestId& aRequestId);
	void CancelAllRequests(TInt aError);
private:
	void ConstructL();
	void DoCancel();
	CPolicyCheckRequestQueue(CSubSession& aSubsession);
	
	void PostFirstRequestOnQueueToUpsServer();			
private:
	TSglQue<CQueuedPolicyCheckRequest> iRequestQueue;	
	CSubSession& iSubSession;

	__FLOG_DECLARATION_MEMBER;		
	};

} // end namespace NetUps

#endif // NETUPS_PCR_QUEUE_H
