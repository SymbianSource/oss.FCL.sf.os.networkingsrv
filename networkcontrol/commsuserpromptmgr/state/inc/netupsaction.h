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
// This file specifies the various actions which are performed by the
// Net UPS.
// @internalComponent
// @prototype
// 
//

#ifndef NETUPSACTION_H
#define NETUPSACTION_H

#include <e32def.h>		  					// defines TInt
#include <e32std.h>							// defines TProcessId, TThreadId

#include <comms-infras/ss_activities.h>
#include <comms-infras/ss_nodemessages.h>	// defines TCFMessage

#include "netupstypes.h"					// defines MPolicyCheckRequestOriginator
#include "netupskeys.h"						// defines keys used to access the netups database
#include "netupsimpl.h"						// defines Net Ups Interface
#include "netupsprocessentry.h"				// defines the database entry
#include "netupsthreadentry.h"				// defines the database sub entry
#include "netupsstate.h"

namespace NetUps
{
namespace NetUpsFunctions
{
void HandleUPSRequestL(CState& aState, TThreadKey& aThreadKey , const TPolicyCheckRequestData& aPolicyCheckRequestData, TRequestId aRequestId); 

void HandleUpsRequestCompletion_ProcessLifeTimeNonSessionL(CState &aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
void HandleUpsRequestCompletion_EnteringProcessSessionLifeTimeL(CState &aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
void HandleUpsRequestCompletion_NetworkLifeTimeNonSessionL(CState &aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);
void HandleUpsRequestCompletion_EnteringNetworkSessionLifeTimeL(CState &aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TNetUpsDecision aNetUpsDecision);

void HandleUPSErrorOnCompletion_ProcessLifeTimeNonSessionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
void HandleUPSErrorOnCompletion_NetworkLifeTimeNonSessionL(TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
void HandleUPSErrorOnCompletion_EnteringProcessSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);
void HandleUPSErrorOnCompletion_EnteringNetworkSessionLifeTimeL(CState& aState, TCommsIdKey& aCommsIdKey, MPolicyCheckRequestOriginator& aPolicyCheckRequestOriginator, TInt aError);

void HandleProcessTermination(TProcessKey& aProcessKey);

void HandleThreadTermination_DeleteThreadEntry(TThreadKey& aThreadKey);

void IncrementConnectionCountL(TCommsIdKey& aCommsIdKey); 

void DecrementConnectionCount_NetworkLifeTime_SessionL(TCommsIdKey& aCommsIdKey);
void DecrementConnectionCount_NetworkLifeTime_NonSessionL(TCommsIdKey& aCommsIdKey);

void DeleteThreadEntry(TThreadKey& aThreadKey);
void DeleteProcessEntry(TProcessKey& aProcessKey, TBool aRetainProcessMonitor = EFalse);

void StartProcessMonitor(TProcessKey& aProcessKey);
void CleanUpThreadEntries_ProcessLifeTime(TThreadKey& aThreadKey);
void CleanUpThreadEntries_NetworkLifeTime(CState& aState, TCommsIdKey& aCommsIdKey, TNetUpsDecision aNetUpsDecision);

void ReplyOutStandingRequests(TThreadKey& aThreadKey);
void ReplyOutStandingRequests(TThreadKey& aThreadKey, TNetUpsDecision aNetUpsDecision);

void PerformTransitionFromNetworkLifeTimeNonSession(CState& aState, TEvent aEvent, TCommsIdKey& aCommsIdKey, TBool aAllActiveObjectsCompleted);
}

}

#endif // NETUPSACTION_H
