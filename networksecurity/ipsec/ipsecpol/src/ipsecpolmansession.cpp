// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPSecPolicyManSession.cpp - IPSec Policy Manager Session
// - This module contains the server side session relating
// functions activated in IPSecPolicyManApi module.
//

#include "ipsecpol.h"
#include "ipsecpolmansession.h"
#include "ipsecpolmanserver.h"
#include "ipsecpolmanhandler.h"
#include "ipsecpolapi.h"
#include "log_ipsecpol.H"


CIPSecPolicyManagerSession* CIPSecPolicyManagerSession::NewL(CIPSecPolicyManagerServer* aServer)
    {
    CIPSecPolicyManagerSession* self;
    self = new (ELeave) CIPSecPolicyManagerSession(aServer);
    aServer->iSessionCount++;
    return self;
    }


CIPSecPolicyManagerSession::CIPSecPolicyManagerSession( CIPSecPolicyManagerServer* aServer)
:iServer(aServer)
    {
    }

CIPSecPolicyManagerSession::~CIPSecPolicyManagerSession(void)
    {
    if (iServer->iSessionCount )
        {
        iServer->iSessionCount --;   
        iServer->StopIPSecPolicyManagerServer();
        }
    }
    /*=====================================================================
    *
    * Process a message received from client
    *
    * ==================================================================*/ 
void CIPSecPolicyManagerSession::ServiceL(const RMessage2& aMessage)
    {
    TInt Status = KErrNone;
    if (iServer->iIPSecPolicyManagerHandler == NULL)
        {
        iServer->iIPSecPolicyManagerHandler = CIPSecPolicyManagerHandler::NewL(iServer);
        //Autoload policy config data is read here
        TRAPD (error, iServer->iIPSecPolicyManagerHandler->ReadAutoloadConfigDataL ()); 
        if (error != KErrNone)
        {
          LOG(Log::Printf(_L("Read autoload config data failed\n")));		
        }
                
        //if this is a preload type autoload policy
        if (iServer->iIPSecPolicyManagerHandler->IsPreloadNeeded())
            {
            TRAPD(leaveCode, iServer->iIPSecPolicyManagerHandler->AutoloadPoliciesL(KDefaultZoneInfo, NULL, EAutoloadPreload ));
            
            if (leaveCode != KErrNone)
                {
                LOG(Log::Printf(_L("Preload autoload policy failed\n")));	
                }
            else
                {
                //activate the preload autoload policy
                iServer->iIPSecPolicyManagerHandler->ProcessActivateAutoloadPolicyL(this);
                iServer->iIPSecPolicyManagerHandler->StorePreloadPolicyHandle();
                }
            
            }
        
        }
    
    switch ( aMessage.Function() )
        {
        
        /*=====================================================================
        *
        * LoadPolicy
        *  p[0] = aIPSecPolicy;
        *  p[1] = aPolicyHandle;
        *  p[2] = aZoneInfoSet;
        *  p[3] = aProcessingFlags;
        * 
        *==================================================================*/ 
        case EIpsecPolicyLoadPolicy:
            
            //decide what kind of load it is
            
            Status = iServer->iIPSecPolicyManagerHandler->ProcessLoadPoliciesL(
                aMessage,
                this);
            break;
            
            /*=====================================================================
            *
            * ActivatePolicy
            *  p[0] = aPolicyHandle;
            * 
            *==================================================================*/ 
        case EIpsecPolicyActivatePolicy:
            
            Status = iServer->iIPSecPolicyManagerHandler->ProcessActivatePolicyL(
                aMessage,
                this);
            break;
            /*=====================================================================
            *
            * GetLastConflictInfo
            *  p[0] = aConflictingPolicyName;
            *  p[1] = aDebugInfo;
            * 
            *==================================================================*/ 
        case EIpsecPolicyGetLastConflictInfo:
            
            Status = iServer->iIPSecPolicyManagerHandler->GetLastConflictInfoL(
                aMessage,
                this);
            break;
            
            /*=====================================================================
            *
            * UnloadPolicy
            *  p[0] = aPolicyHandle;
            * 
            *==================================================================*/ 
        case EIpsecPolicyUnloadPolicy:
            
            
            Status = iServer->iIPSecPolicyManagerHandler->ProcessUnloadPoliciesL(
                aMessage,
                this);
            break;
            
            /*=====================================================================
            *
            * MatchSelector
            *  p[0] = TDesC8&               // TIpsecSelectorInfo 
            *  p[1] = TDes8&                // TIpsecSaSpec      
            * 
            *==================================================================*/ 
        case EIpsecPolicyMatchSelector:
            
            Status = iServer->iIPSecPolicyManagerHandler->GetIPSecSAInfoL(
                aMessage,
                this);
            break;
            /*=====================================================================
            *
            * CancelLoad, CancelUnload, CancelMatch
            * 
            *==================================================================*/ 
        case EIpsecPolicyCancelLoad:
        case EIpsecPolicyCancelActivate:
        case EIpsecPolicyCancelUnload:
        case EIpsecPolicyCancelMatch:
            Status = KErrNone;
            break;
 
#ifdef TESTFLAG
            
            /*=====================================================================
            *
            * Not supported method
            * 
            *==================================================================*/ 
        case ERequestInfo:
        	
        	Status = iServer->iIPSecPolicyManagerHandler->RequestEvent(
                aMessage,
                this);
       		break;
       	
#endif    
        case EIpsecPolicyAvailableSelectors:
        	Status = iServer->iIPSecPolicyManagerHandler->GetAvailableSelectors(aMessage);
            break;
        
        case EIpsecPolicyEnumerateSelectors:
        	Status = iServer->iIPSecPolicyManagerHandler->GetSelectorsCount(aMessage);
            break;
        default:
            Status = KErrNotSupported;
            break;
            
        }
    
    
        /*=====================================================================
        *
        * Send the response to the caller's active object
        * 
        *==================================================================*/ 
        aMessage.Complete(Status);

        
    }
    
