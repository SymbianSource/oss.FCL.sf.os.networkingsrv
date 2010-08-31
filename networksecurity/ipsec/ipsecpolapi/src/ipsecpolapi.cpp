// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Policy server - client interface implementation
// 
//

#include <e32math.h>

#include "ipsecpol.h"
#include "ipsecpolapi.h"
#include "clistatic.h"
#include <e32base.h>

EXPORT_C  RIpsecPolicyServ::RIpsecPolicyServ()
    {
    }

EXPORT_C  RIpsecPolicyServ::~RIpsecPolicyServ()
    {
    }

EXPORT_C TVersion RIpsecPolicyServ::Version() const
    {
    return(TVersion(KPolicyServMajorVersion,KPolicyServMinorVersion,KPolicyServBuildVersion));
    }
    
EXPORT_C TInt RIpsecPolicyServ::Connect()
//
// Connect to the server attempting to start it if necessary
//
    {
	TInt retry=2;
	for (;;)
		{
		TInt r=CreateSession(KIpsecPolicyServerName,
                             Version(),
                             KDefaultMessageSlots);
        
		if (r!=KErrNotFound && r!=KErrServerTerminated)
			return r;
		if (--retry==0)
			return r;
		r = Launcher::LaunchServer(KIpsecPolicyServerName, KIpsecPolicyServerImg,
                                     KServerUid3, KMyServerInitHeapSize,
                                     KMyServerMaxHeapSize, KMyServerStackSize);

		if (r!=KErrNone && r!=KErrAlreadyExists)
			return r;
		}
	}
	

EXPORT_C void RIpsecPolicyServ::LoadPolicy(const TDesC8& aPolicy,
                                           TPolicyHandlePckg& aPolicyHandle,
                                           TRequestStatus& aStatus)
/**
@capability NetworkControl Only privileged apps can affect IPSec policies
*/
    {
    SendReceive (EIpsecPolicyLoadPolicy,
                 TIpcArgs(&aPolicy, &aPolicyHandle, &KNullDesC8, KAddIkeBypassSelectors),
                 aStatus);
    }


EXPORT_C void RIpsecPolicyServ::LoadPolicy(const TDesC8& aPolicy,
                             TPolicyHandlePckg& aPolicyHandle,
                             TRequestStatus& aStatus,
                             const TZoneInfoSetPckg& aSelectorZones,
                             TUint aProcessingFlags)
/**
@capability NetworkControl Only privileged apps can affect IPSec policies
*/
    {
    SendReceive (EIpsecPolicyLoadPolicy,
                 TIpcArgs(&aPolicy, &aPolicyHandle, &aSelectorZones,aProcessingFlags),
                 aStatus);
    }

EXPORT_C void RIpsecPolicyServ::CancelLoad()
    {
    SendReceive(EIpsecPolicyCancelLoad,TIpcArgs());
    }
   
EXPORT_C void RIpsecPolicyServ::ActivatePolicy(const TPolicyHandle& aPolicyHandle,
                                               TRequestStatus& aStatus)
/**
@capability NetworkControl Only privileged apps can affect IPSec policies
*/
    {
    SendReceive(EIpsecPolicyActivatePolicy, TIpcArgs(aPolicyHandle.iHandle), aStatus);
    }

EXPORT_C void RIpsecPolicyServ::CancelActivate()
    {
    SendReceive(EIpsecPolicyCancelActivate,TIpcArgs());
    }

EXPORT_C TInt RIpsecPolicyServ::GetDebugInfo(TDes& aDebugInfo, TUint aInfoFlags)

    {
    return SendReceive(EIpsecPolicyGetLastConflictInfo, TIpcArgs(&aDebugInfo, aInfoFlags));
    }
    
EXPORT_C void RIpsecPolicyServ::UnloadPolicy(const TPolicyHandle& aPolicyHandle, 
                                             TRequestStatus& aStatus)
/**
@capability NetworkControl Only privileged apps can affect IPSec policies
*/ 
    {
    SendReceive(EIpsecPolicyUnloadPolicy, TIpcArgs(aPolicyHandle.iHandle), aStatus);
    }

EXPORT_C void RIpsecPolicyServ::CancelUnload()
    {
    SendReceive(EIpsecPolicyCancelUnload, TIpcArgs());
    }

EXPORT_C void RIpsecPolicyServ::MatchSelector(const TDesC8& aProposal,
                                        TDes8& aMatchingSASpec,
                                        TRequestStatus& aStatus)
    {
    SendReceive(EIpsecPolicyMatchSelector, TIpcArgs(&aProposal, &aMatchingSASpec), aStatus);
    }
    
EXPORT_C void RIpsecPolicyServ::CancelMatch()
    {
    SendReceive(EIpsecPolicyCancelMatch, TIpcArgs());
    }

EXPORT_C void RIpsecPolicyServ::AvailableSelectors(const TDesC8& aGateway, CArrayFixFlat<TIpsecSelectorInfo>* aSelectors, TRequestStatus& aStatus)
	{
	TInt count = 0;
	EnumerateSelectors(aGateway, count);
	
	aSelectors->ResizeL(count);
	
	TPtr8 selectors((TUint8*)&aSelectors->At(0), count * aSelectors->Length());
	SendReceive(EIpsecPolicyAvailableSelectors, TIpcArgs(&selectors), aStatus);
	}
	
void RIpsecPolicyServ::EnumerateSelectors(const TDesC8& aGateway, TInt& aCount)
	{
	TPckg<TInt> selectorCount(aCount);
    SendReceive(EIpsecPolicyEnumerateSelectors, TIpcArgs(&aGateway, &selectorCount));
	}

