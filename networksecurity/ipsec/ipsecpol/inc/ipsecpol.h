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
// IpsecPol - Ipsecpol.h
//



/**
 @internalComponent
*/
#ifndef __IPSECPOL_H__
#define __IPSECPOL_H__

#include <e32std.h>

_LIT(KIpsecPolicyServerName,"IPSec Policyserver");
_LIT(KIpsecPolicyServerImg,"ipsecpol");
_LIT(KPreloadFileName, "ipsecpol.ini");
_LIT(KAutoLoadPolicy, "Autoload");
_LIT(KLoadFlag, "Loadflag");
_LIT(KFileName, "FileName");

//algorithms configuration file name
_LIT(KAlgorithmFile,"algorithms.conf");


_LIT(KAutoloadNone, "None");
_LIT(KAutoloadPreload, "Preload");
_LIT(KAutoloadBeforeManualLoad, "BeforeManualLoad");
_LIT(KAutoloadAfterManualLoad, "AfterManualLoad");
_LIT(KAutoloadBeforeScopedLoad, "BeforeScopedLoad");
_LIT(KAutoloadAfterScopedLoad, "AfterScopedLoad");

const TUid KServerUid3={0x01000000};

const TInt KMyServerStackSize=0x2000;           //  8KB
const TInt KMyServerInitHeapSize=0x1000;        //  4KB
const TInt KMyServerMaxHeapSize=0x1000000;      // 16MB

const TInt KPolicyServMajorVersion = 1;
const TInt KPolicyServMinorVersion = 0;
const TInt KPolicyServBuildVersion = 0;

const TInt KMaxMyMessage=100;

enum TPolicyMessages 
    { 
    EIpsecPolicyLoadPolicy,
        EIpsecPolicyCancelLoad,
        EIpsecPolicyActivatePolicy,
        EIpsecPolicyCancelActivate,
        EIpsecPolicyGetLastConflictInfo,
        EIpsecPolicyUnloadPolicy,
        EIpsecPolicyCancelUnload,
        EIpsecPolicyMatchSelector,
        EIpsecPolicyCancelMatch,
        ERequestInfo,
        
        EIpsecPolicyReadPolicyCount,
        EIpsecPolicyReadPolicy,
        EIpsecPolicyCancelRead,
        EIpsecPolicyCancelSelector,
        EIpsecPolicyCancelAll,
        EIpsecPolicyGetSpecsForSelector,
        EIpsecPolicyAddPolicy,
        EIpsecPolicyGetPolicy,
        EIpsecPolicyGetPolicyCount,
        EIpsecPolicyGetPolicyIds,
        EIpsecPolicyGetPolicyHandles,
        EIpsecPolicyGetSelectorCount,
        EIpsecPolicyGetSASpec,
        EIpsecPolicyAddSASpec,
        EIpsecPolicyDeleteSASpec,
        EIpsecPolicyGetSelector,
        EIpsecPolicyAddSelector,
        EIpsecPolicyDeleteSelector,
        EIpsecPolicyUnloadAllPolicies,
        EIpsecPolicyDebugNotificationRequest,
        EIpsecPolicyGetDebugInfo,
        EIpsecPolicyCancelDebug,
        EIpsecPolicyAvailableSelectors,
        EIpsecPolicyEnumerateSelectors,
        EIpsecPolicySetOption,
        
    };
    
enum TAutoloadFlags 
    { 
    EAutoloadNone,
        EAutoloadPreload,
        EAutoloadBeforeManualLoad,
        EAutoloadAfterManualLoad,
        EAutoloadBeforeScopedLoad,
        EAutoloadAfterScopedLoad
    };

enum TPolicyType
    {
    EManualLoad,
        EScopedManualLoad,
        EAutoload
    };

class TServerStart
    {
    public:
        TServerStart(TRequestStatus& aStatus);
        TPtrC AsCommand() const;
        
        TServerStart();
        TInt GetCommand();
        void SignalL();
    private:
        TThreadId iId;
        TRequestStatus* iStatus;
    };

#endif
