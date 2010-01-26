// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IPSecPolManServer.cpp - IPSec Policy Manager server main module
//


#include "ipsecpol.h"
#include "ipsec_version.h"
#include "ipsecpolmanserver.h"
#include "ipsecpolmansession.h"
#include "ipsecpolmanhandler.h"
#include "ipsecpolapi.h"




//platform security changes for IPSec Policy Manager

//the number of platform security ranges
const TUint CIPSecPolicyManagerServer::ipsecRangeCount = 2;

//platform security ranges
const TInt CIPSecPolicyManagerServer::ipsecRanges[ipsecRangeCount] = 
    {
    0, //range is 0-8 inclusive
        9 //9, KMaxInt
    };
//policy elements defined for IpSec PolMan
const CPolicyServer::TPolicyElement CIPSecPolicyManagerServer::ipsecElements[] = 
    {
    //this is the policy enforced by IpSec Policy Manager
        {_INIT_SECURITY_POLICY_C1(ECapabilityNetworkControl), CPolicyServer::EFailClient}, 
    };

//security policies defined
const TUint8 CIPSecPolicyManagerServer::elementIndex[ipsecRangeCount] = 
    {
    0,0
    };

const CPolicyServer::TPolicy CIPSecPolicyManagerServer::ipsecPolicy =
    {
    0, //specifies all connect attempts should be checked for NetworkControl capability
        ipsecRangeCount,
        ipsecRanges,
        elementIndex,
        ipsecElements
    };



//
//  Constructor that takes sets a security policy when called
//
CIPSecPolicyManagerServer::CIPSecPolicyManagerServer(void):CPolicyServer(EPriorityStandard,ipsecPolicy)
    {
    iSessionCount = 0;
    }

//
//  
//
CIPSecPolicyManagerServer::~CIPSecPolicyManagerServer(void)
    {
    delete iIPSecPolicyManagerHandler;
    iIPSecPolicyManagerHandler = 0;
    }

//
// Create CIPSecPolicyManagerServer object
// 
//
CIPSecPolicyManagerServer* CIPSecPolicyManagerServer::NewL(void)
    {
    CIPSecPolicyManagerServer* self = CIPSecPolicyManagerServer::NewLC();
    CleanupStack::Pop(); // self
    return self;
    }

CIPSecPolicyManagerServer* CIPSecPolicyManagerServer::NewLC(void)
    {
    CIPSecPolicyManagerServer* self;
    self = new(ELeave) CIPSecPolicyManagerServer;
    CleanupStack::PushL(self);
    self->StartL(KIpsecPolicyServerName);
    return self;
    }

//
//  
//
TInt CIPSecPolicyManagerServer::RunError(TInt aError)
    {
    // delete allocated resources
    if (iIPSecPolicyManagerHandler != NULL)
        {
        iIPSecPolicyManagerHandler->ReleaseResources();
        }
    
    // Complete the outstanding message
    Message().Complete(aError);
    // ready to roll again
    ReStart(); // really means just continue reading client requests
    
    return KErrNone; // Active scheduler Error() method NOT called          
    }

//
// Stop IPSec Policy Manager service if no session exist 
//  
void CIPSecPolicyManagerServer::StopIPSecPolicyManagerServer(void)
    {
	if (iSessionCount == 0)
		{
		if (iIPSecPolicyManagerHandler != NULL)
			{
			if(iIPSecPolicyManagerHandler->iActivePolicyList->Count() == 0)
			  {
			  delete iIPSecPolicyManagerHandler;
			  iIPSecPolicyManagerHandler = 0;
			  CActiveScheduler::Stop();
			  }
			 }
			else
			  CActiveScheduler::Stop(); 
		}
    }

//
// A client has issued Connect request and kernel calls this function
// that creates a server side session. 
//  

CSession2* CIPSecPolicyManagerServer::NewSessionL(const TVersion& ,
                                                  const RMessage2& ) const
    {
    CSession2* session = CIPSecPolicyManagerSession::NewL(CONST_CAST(CIPSecPolicyManagerServer*, this));
    return session;
    }
