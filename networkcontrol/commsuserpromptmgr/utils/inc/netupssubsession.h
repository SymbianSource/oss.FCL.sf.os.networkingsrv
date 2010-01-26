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
// This file provides the interface specification for active objects
// which are used to interact with the UPS subsessions.
// @internalAll
// @prototype
// 
//

#ifndef NETUPSSUBSESSION_H
#define NETUPSSUBSESSION_H

#include <e32base.h>	// defines CActive
#include <e32std.h> 	// defines timer for Nadeem's testing
#include <e32cmn.h>		// defines RBuf
#include <e32property.h>// defines properties

#include "netupstypes.h"
#include "netupsproperties.h"
#include "netupsimpl.h"

#include <ups/upsclient.h>
#include <ups/upstypes.h>

namespace NetUps
{
class CDatabaseEntry;
class CProcessEntry;
class CThreadEntry;
class CPolicyCheckRequestData;
class MPolicyCheckRequestOriginator;
class TCommsId;
class UserPromptService::RUpsSubsession;

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility
	
NONSHARABLE_CLASS(CSubSession) : public CActive
	{
public:

#ifdef _DEBUG
	enum TPCRPState
		{
		EPCRPIdle,
		EPCRPSimulateRequest,
		EPCRPSimulateDelay
		};

	enum TDelay
		{
		EDelay = 0*1000000,
		ECancelDelay =  0*1000000
		};
#endif
		
	enum TLength
		{
		EStartPosition 			= 0,
		EMaxUnformattedLength 	= 64,
		EMaxFormattedLength 	= 80
		};
		
	enum TDeletionStatus
		{
		EAlreadyDeleted 		= 0, // ThreadEntry and associated queue are deleted on entering process life time mode - so current queue entry does not need to be deleted in CSubSession::RunL()
		ENotYetDeleted			= 1  // otherwise current queue entry is deleted inside SubSession::RunL()
		};	
			
public:
	static CSubSession* NewL(UserPromptService::RUpsSession& aUpsSession, CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry&	aThreadEntry);
	~CSubSession();

	TInt Authorise(const CPolicyCheckRequestData& aPolicyCheckRequestData);
	void SetReadyForDeletion();

	CProcessEntry& GetProcessEntry();
private:
	void ConstructL(UserPromptService::RUpsSession& aUpsSession);
	CSubSession(CDatabaseEntry& aDatabaseEntry, CProcessEntry& aProcessEntry, CThreadEntry&	aThreadEntry);

	void RunL();
	TInt RunError(TInt aError);
	void DoCancel();
	
	void ResetAuthorisationRequestParameters();
	void DeleteCurrentRequestFromQueue();
	void PostNextRequestFromQueue();
	
	TBool SubSessionReadyForDeletion();
	
#ifdef _DEBUG	
	void ModifyIStatusValue(TNetUpsDecision aNetUpsDecision);
#endif
private:									
	UserPromptService::RUpsSubsession 		iUpsSubSession;
	CDatabaseEntry& 						iDatabaseEntry;
	CProcessEntry& 						 	iProcessEntry;
	CThreadEntry&						 	iThreadEntry;
	TUpsDecision 							iDecision;
	const Messages::TNodeId*					iCommsId;						// pointer does not denote ownership
	MPolicyCheckRequestOriginator*			iPolicyCheckRequestOriginator; 	// pointer does not denote ownership
	TBool									iReadyForDeletion;
	TInt									iTestErrorCode;

	//RBuf									iUpsDestinationName;	
	RBuf8									iUpsOpaqueData;

#ifdef _DEBUG
	TPCRPState								iState;
	RTimer									iSimulatedDelay;
#endif

	__FLOG_DECLARATION_MEMBER;				
	};
	
} // end of namespace NetUps

#endif // CSubSession
