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
// This file specifies the generic statemachine container class 
// which encapsulates the states for a specific state machine.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSSTATEMACHINE_H
#define NETUPSSTATEMACNINE_H

#include <e32def.h>		  					// defines TInt
#include <e32std.h>
#include <e32cmn.h>							// defines RPointerArray
#include <e32base.h>						// defines CBase

#include <comms-infras/ss_activities.h>
#include <comms-infras/ss_nodemessages.h>	// defines TCFMessage

#include "netupstypes.h"					// defines MPolicyCheckRequestOriginator
#include "netupskeys.h"						// defines keys used to access the netups database

#include "netupsdatabaseentry.h"			// defines the Database Entry		
#include "netupsprocessentry.h"				// defines the Process Entry
#include "netupsthreadentry.h"				// defines the Thread Entry
#include "netupsstatedef.h"					// defines the NetUps Process and Network Lifetime States

#include <comms-infras/commsdebugutility.h> 		// defines the comms debug logging utility

namespace NetUps
{
class CNetUpsImpl;
class CState;

NONSHARABLE_CLASS(CUpsStateMachine) : public CBase 
	{
public:
	static CUpsStateMachine* NewL(CNetUpsImpl& aNetUpsImpl, TInt32 ServiceId);
	~CUpsStateMachine();

	void ProcessPolicyCheckRequestL(TThreadKey& aThreadKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);
	void ProcessPolicyCheckRequestL(TProcessKey& aProcessKey, const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId);
	void IncrementConnectionCountL(TCommsIdKey& aCommsIdKey); 
	void DecrementConnectionCountL(TCommsIdKey& aCommsIdKey);
	void HandleUPSRequestCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision); 
	void HandleUPSErrorOnCompletionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
	void HandleProcessTermination(TProcessKey& aProcessKey);
	void HandleThreadTermination(TThreadKey& aThreadKey);
		
	TInt32		 ServiceId();
	CNetUpsImpl& NetUpsImpl();
	
	TUint32 GetIndex(TNetUpsState netUpsState);
private:
	void ConstructL();
	CUpsStateMachine(CNetUpsImpl& aNetUpsImpl, TInt32 aServiceId);		

	// methods which are declared but not defined.
	CUpsStateMachine();
	CUpsStateMachine(const CUpsStateMachine&);
	void operator=(const CUpsStateMachine&);
	bool operator==(const CUpsStateMachine&);	

	TUint32 GetIndex(CProcessEntry& aProcessEntry);
	static TNetUpsState GetState(CProcessEntry& aProcessEntry);	
private:
	CNetUpsImpl& 			iNetUpsImpl;
	TInt32 					iServiceId;
	RPointerArray<CState> 	iState;

	__FLOG_DECLARATION_MEMBER;		
	};
	
}  // end of name space NetUps 

#endif // NETUPSSTATEMACHINE_H
