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
// This file specifies the states which can be used to form a
// Net Ups State Machine.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSSTATE_H
#define NETUPSSTATE_H

#include <e32def.h>		  					// defines TInt
#include <e32std.h>							// defines ThreadId, TProcessId

#include <comms-infras/ss_activities.h>

#include <comms-infras/ss_nodemessages.h> 	// defines ESock::TCFMessage
#include "netupstypes.h"		// defines TDestinationName, TNetUpsDecision

#include "netupstypes.h"					// defines TRequestId
#include "netupskeys.h"
#include "netupsdatabaseentry.h"
#include "netupsprocessentry.h"				// defines the database entry
#include "netupsthreadentry.h"				// defines the database sub entry
#include "netupsstatedef.h"					// defines the states for each state machine

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
class CNetUpsImpl;
class CUpsStateMachine;
NONSHARABLE_CLASS(CState)
	{
public:
	static CState* NewL(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);
	virtual ~CState();

	const TNetUpsState& State() const;
	CUpsStateMachine& 	UpsStateMachine();
	static void PerformStateTransition(TEvent aEvent, CProcessEntry& aProcessEntry);

	virtual void ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);		

	virtual void IncrementConnectionCountL(TCommsIdKey& aCommsIdKey); 
	virtual void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);

	virtual void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey,  MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	virtual void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey,  MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
				
	virtual void HandleProcessTermination(TProcessKey& aProcessKey);
	virtual void HandleThreadTermination(TThreadKey& aThreadKey);

protected:
	CState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);
private:
	void ConstructL();
private:
	TNetUpsState 		iNetUpsState;
	CUpsStateMachine& 	iUpsStateMachine;

	__FLOG_DECLARATION_MEMBER;		
	};

NONSHARABLE_CLASS(CNullState) : public CState
	{
public:	
	void ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);		

	CNullState(CUpsStateMachine& aStateMachine);
	};

NONSHARABLE_CLASS(CProcLifeTime_NonSessionState) : public CState
	{
public:	
	void ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);			
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);	
	void HandleThreadTermination(TThreadKey& aThreadKey);

	CProcLifeTime_NonSessionState(CUpsStateMachine& aStateMachine);
	};

NONSHARABLE_CLASS(CProcLifeTime_SessionState) : public CState
	{
public:
	void HandleProcessTermination(TProcessKey& aProcessKey);

	CProcLifeTime_SessionState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);
	};

NONSHARABLE_CLASS(CProcLifeTime_TransitTo_SessionState) : public CProcLifeTime_SessionState
	{
public:
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
	
	CProcLifeTime_TransitTo_SessionState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);
	};

NONSHARABLE_CLASS(CNetLifeTime_NonSessionState) : public CState
	{
public:	
	void ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);
	void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
	void HandleThreadTermination(TThreadKey& aThreadKey);

	CNetLifeTime_NonSessionState(CUpsStateMachine& aStateMachine);
	};	

NONSHARABLE_CLASS(CNetLifeTime_SessionYesState) : public CState
	{
public:	
	void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);

	CNetLifeTime_SessionYesState(CUpsStateMachine& aStateMachine);
protected:
	CNetLifeTime_SessionYesState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);	
	};	

NONSHARABLE_CLASS(CNetLifeTime_TransitTo_SessionYesState) : public CNetLifeTime_SessionYesState
	{
public:						
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey,  MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);

	CNetLifeTime_TransitTo_SessionYesState(CUpsStateMachine& aStateMachine);
	};

NONSHARABLE_CLASS(CNetLifeTime_SessionNo_WithOutConnectionsState) : public CState
	{
public:	
	void HandleProcessTermination(TProcessKey& aProcessKey);
	void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);

	CNetLifeTime_SessionNo_WithOutConnectionsState(CUpsStateMachine& aStateMachine);
protected:
	CNetLifeTime_SessionNo_WithOutConnectionsState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);
	};
	
NONSHARABLE_CLASS(CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState) : public CNetLifeTime_SessionNo_WithOutConnectionsState
	{
public:
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleThreadTermination(TThreadKey& aThreadKey);		
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
	
	CNetLifeTimeSession_TransitTo_SessionNoWithOutConnectionsState(CUpsStateMachine& aStateMachine);
	};	

NONSHARABLE_CLASS(CNetLifeTime_SessionNo_WithConnectionsState) : public CState
	{
public:	
	void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);				

	CNetLifeTime_SessionNo_WithConnectionsState(CUpsStateMachine& aStateMachine);
protected:
	CNetLifeTime_SessionNo_WithConnectionsState(TNetUpsState aNetUpsState, CUpsStateMachine& aStateMachine);	
	};
	
NONSHARABLE_CLASS(CNetLifeTime_TransitTo_SessionNo_WithConnectionsState) : public CNetLifeTime_SessionNo_WithConnectionsState 
	{
public:
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);

	CNetLifeTime_TransitTo_SessionNo_WithConnectionsState(CUpsStateMachine& aStateMachine);
	};	

} // end namespace NetUps

#endif // NETUPSSTATE_H
