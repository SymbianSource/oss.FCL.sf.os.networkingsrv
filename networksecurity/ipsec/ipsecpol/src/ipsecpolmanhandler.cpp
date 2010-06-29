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
// IPSecPolicyManagerHandler.cpp - IPSec Policy Manager Handler
//

#include <es_sock.h>
#include <in_sock.h>
#include <es_ini.h>

#include "ipsecpolmanhandler.h"
#include "algorithmconf.h"
#include "ipsecpolmanserver.h"
#include "ipsecpolmansession.h"
#include "ipsecpolapi.h"
#include "ipsecpol.h"
#include "log_ipsecpol.H"
#include "ipsecpolparser.h"
#include "secpolreader.h"

#define FIRST_ARGUMENT  0
#define SECOND_ARGUMENT 1
#define THIRD_ARGUMENT  2
#define FOURTH_ARGUMENT 3

//
// Create IPSecPolicyManagerHandler object
//
CIPSecPolicyManagerHandler*
CIPSecPolicyManagerHandler::NewL(CIPSecPolicyManagerServer*)
    {
    LOG(Log::Printf(_L("Constructing IPSecPolManHandler\n")));

    CIPSecPolicyManagerHandler* self = 
        new (ELeave) CIPSecPolicyManagerHandler();
    
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

//
// Phase 2 constructor
//
void 
CIPSecPolicyManagerHandler::ConstructL()
    {
    User::LeaveIfError(iFs.Connect());
    User::LeaveIfError(iSS.Connect());

    iActivePolicyList = new (ELeave) CActivePolicyList(1);

    iIsPreloadNeeded = EFalse;

    iPreloadPolicyHandle.iHandle = 0;


#ifdef TESTFLAG

	iStringBuf = NULL;

#endif

    ReadAlgorithmsFileL();
    iSelectorInfoArray = new (ELeave) CArrayFixFlat<TIpsecSelectorInfo> (2);

    }

//
// Destructor
//
CIPSecPolicyManagerHandler::~CIPSecPolicyManagerHandler()
    {
    LOG(Log::Printf(_L("Destructing IPSecPolManHandler\n")));

    // Unload the autoload policy if the load method is Preload,
    if (iPreloadPolicyHandle.iHandle > 0)
        {
        TRAPD(leaveCode,
              UnloadPolicyByHandleL(iPreloadPolicyHandle.iHandle, NULL));

        if (leaveCode != KErrNone)
            {
            LOG(Log::Printf(_L("Unload autoload policy failed\n")));
            }
        }
        
	// Corrected code
    if (iActivePolicyList)
        {
        // Deletes all the items in the list
        for (TInt i = iActivePolicyList->Count() - 1; i >= 0; i--)
            {
            
            if ( iActivePolicyList->At(i) != NULL ) 
            	{	
	            // Unloads still active policies
	            TRAPD(leaveCode,
	              UnloadPolicyByHandleL(iActivePolicyList->At(i)->iPolicyHandle.iHandle, NULL));

	        	if (leaveCode != KErrNone)
	            	{
	            	LOG(Log::Printf(_L("Unload autoload policy failed\n")));
            		}
            	
            	}
            }

        delete iActivePolicyList;
        iActivePolicyList = NULL;
        }

    delete iSecpolReader6;
    iSecpolReader6 = NULL;

    delete iLastConflictInfo;
    iLastConflictInfo = NULL;

    delete iLastParsingErrorInfo;
    iLastParsingErrorInfo = NULL;

    delete iAlgorithmsHBufC8;
    iAlgorithmsHBufC8 = NULL;

    delete iPreloadPolicy;
    iPreloadPolicy = NULL;

    delete iBeforeManualLoadPolicy;
    iBeforeManualLoadPolicy = NULL;

    delete iAfterManualLoadPolicy;
    iAfterManualLoadPolicy = NULL;

    delete iBeforeScopedLoadPolicy;
    iBeforeScopedLoadPolicy = NULL;

    delete iAfterScopedLoadPolicy;
    iAfterScopedLoadPolicy = NULL;

#ifdef TESTFLAG

	delete iStringBuf;
	iStringBuf = NULL;

#endif

    // Release  scoped autoload policy array
    for (TInt i = iScopedAutoloadPolicyPairs.Count() - 1; i >= 0; i--)
        {
        delete iScopedAutoloadPolicyPairs[i];
        iScopedAutoloadPolicyPairs.Remove(i);
        }

    // Close array to avoid memory leaks
    iScopedAutoloadPolicyPairs.Close();

    ReleaseResources();

    iFs.Close();
    iSS.Close();
    
    delete iSelectorInfoArray;
    iSelectorInfoArray = NULL;
    }

//
// Release resources allocated for a call
//
void 
CIPSecPolicyManagerHandler::ReleaseResources()
    {
    delete iPieceData;
    iPieceData = NULL;

    delete iPieceData2;
    iPieceData2 = NULL;

    delete iSAInfo;
    iSAInfo = NULL;

    delete iSelectorInfo;
    iSelectorInfo = NULL;

    delete iPolicyHBufC8;
    iPolicyHBufC8 = NULL;

    delete iPolBfr;
    iPolBfr = NULL;

#ifdef TESTFLAG
    
    delete iHandles;	
    iHandles = NULL;

#endif

    if (iSecpolSocketOpen)
        {
        iSecpolSocketOpen = EFalse;
        iSock.Close();
        }

    if (iAlgorithmsFileOpen)
        {
        iAlgorithmsFileOpen = EFalse;
        iAlgFile.Close();
        }
    }

//
// ErrorHandling - Leave is trapped by server's RunError()
//
void 
CIPSecPolicyManagerHandler::ErrorHandlingL(
    TInt aMainCode,
    TInt aDetailCode)
    {
    LOG(Log::Printf(_L("ErrorHandling: main status: %d  detail status: %d\n"),
                    aMainCode, aDetailCode));
#ifndef __FLOG_ACTIVE
    (void)aDetailCode;
#endif
                    
    User::Leave(aMainCode);
    }

//
//
// ProcessLoadPolicy - Process a LoadPolicy request issued
// by IPSecPolicyManApi
//
//
TInt 
CIPSecPolicyManagerHandler::ProcessLoadPolicyL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession* /* aIPSecPolicyManagerSession */,
    TPolicyType aPolType)
    {
    // Retrieve arguments
    
    // (1): Policy data buffer length
    TInt policyDataLth = aMsg.GetDesLengthL(FIRST_ARGUMENT);
    
    // (2): Policy data buffer contents
    delete iPolicyHBufC8;
    iPolicyHBufC8 = NULL;
    iPolicyHBufC8 = HBufC8::NewL(policyDataLth);
    TPtr8 policyDataDesc(iPolicyHBufC8->Des());
    aMsg.ReadL(FIRST_ARGUMENT, policyDataDesc);

    // (3): Zone info (optional)
    TZoneInfoSet zoneInfoSet;
    if (aMsg.GetDesLength(THIRD_ARGUMENT) <= 0)
        {
        zoneInfoSet = KDefaultZoneInfo;
        iFunction = 0;
        }
    else
        {
        iFunction = aMsg.Int3();
        TPckg<TZoneInfoSet>pckgZoneInfoSet(zoneInfoSet);
        aMsg.ReadL(THIRD_ARGUMENT, pckgZoneInfoSet);
        }
    iVPNNetId = 0;
    if (zoneInfoSet.iSelectorZone.iScope != KScopeNone)
        {
        iVPNNetId = zoneInfoSet.iSelectorZone.iId;
        }
    iGwNetId = 0;
    if (zoneInfoSet.iEndPointZone.iScope != KScopeNone)
        {
        iGwNetId = zoneInfoSet.iEndPointZone.iId;
        }
    LOG(Log::Printf(_L("LoadPolicy request VPN NetId: %d  GW NetId: %d\n"),
                    iVPNNetId, iGwNetId));


    // Parse the policy file from string format to the ipsecpolparser
    // class object format
    ParseCurrentPolicyL();

    // Calculate the Bypass/Drop mode of parsed policy
    TInt policyBypassDropMode(KDropMode);
    CSecurityPolicy* policy = iPieceData->Policies();
    policyBypassDropMode = CalculatePolicyBypassDropMode(*policy);

    // Check if we have a direct Bypass-everything-else Vs Drop-everything-else conflict between the active policy and 
    // the one that is attempted to be loaded. If so, return with error

    TInt activepolicyBypassDropMode; 
    //coverity[var_compare_op]
    //intentional null comparision if there is no policylist do nothing.
    if (iActivePolicyList && iActivePolicyList->Count())
        {
        //coverity[var_compare_op]   
        // It is ok to compare with the first active policy. Every subsequent policy would have been compared against the first one
        activepolicyBypassDropMode = iActivePolicyList->At(0)->iBypassOrDropMode;
        if((policyBypassDropMode == KDropMode && (( activepolicyBypassDropMode & KInboundBypass) || (activepolicyBypassDropMode & KOutboundBypass))) || 
                (((policyBypassDropMode & KInboundBypass) || (policyBypassDropMode & KOutboundBypass)) && activepolicyBypassDropMode == KDropMode )) 
            {
            ErrorHandlingL (ESelectorConflict,0);
            }
        } 
   
    // Add VPNNetId to CPolicySelector and GwNetId to CSecpolBundleItem objects
    UpdateSelectorsAndTunnels();

    // Check if the IP addresses in the selectors of current policy
    // file overlaps with the selectors of the policies in the active
    // policy list. Overlapping is allowed when the scope IDs differ
    // or all parameters in the selectors and corresponding SA
    // match exactly.
    CheckSelectorConflictsL();

    //  Modify the SA names to avoid conflicts caused by equal names
    //  in different policies
    MakeUniqueSANamesL();

    // Convert object format policy pieces to string format
    // and do the operations defined by the API flags
    ConvertFromObjectsToStringWithSectionsL(iFunction,
                                            policyBypassDropMode);

    // Store current policy entry to active policy list
    StorePolicyToActiveListL(aPolType, policyBypassDropMode);
    LOG(Log::Printf(_L("LoadPolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // Return the current policy file handle to the API user
    ReturnPolicyFileHandleL(aMsg);

    // An API call completed
    ApiCallCompleted();
    return KErrNone;
    }

//
//
// ProcessLoadPoliciesL - Process a LoadPolicy request issued
// by IPSecPolicyManApi. This method considers autoload policies as well
//
//
TInt 
CIPSecPolicyManagerHandler::ProcessLoadPoliciesL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession* aSession)
    {
    // Read scope information
    TBool scopedLoad = EFalse;
    TInt preLoadHandle(0);
    TInt postLoadHandle(0);
    TInt parentPolicyHandle(0);
    TInt leaveCode(0);

    TZoneInfoSet zoneInfoSet;
    if (aMsg.GetDesLength(THIRD_ARGUMENT) <= 0)    // Use default zoneInfo
        {
        zoneInfoSet = KDefaultZoneInfo;

        // Processing flags
        iFunction = 0;
        }
    else
        {
        scopedLoad = ETrue;
        iFunction = aMsg.Int3();
        TPckg<TZoneInfoSet>pckgZoneInfoSet(zoneInfoSet);
        aMsg.ReadL(THIRD_ARGUMENT, pckgZoneInfoSet);
        }
    iVPNNetId = 0;
    if (zoneInfoSet.iSelectorZone.iScope != KScopeNone)
        {
        iVPNNetId = zoneInfoSet.iSelectorZone.iId;
        }
    iGwNetId = 0;
    if (zoneInfoSet.iEndPointZone.iScope != KScopeNone)
        {
        iGwNetId = zoneInfoSet.iEndPointZone.iId;
        }

    LOG(Log::Printf(_L("LoadPolicy request VPN NetId: %d  GW NetId: %d\n"),
                    iVPNNetId, iGwNetId));

    if (scopedLoad)
        {
        // Load BeforescopedLoadPolicies before
        if (iBeforeScopedLoadPolicy->Size() != 0)
            {
            TRAP(leaveCode,
                 AutoloadPoliciesL(zoneInfoSet,
                                   iBeforeScopedLoadPolicy,
                                   EAutoloadBeforeScopedLoad));
            if (leaveCode != KErrNone)
                {
                LOG(Log::Printf(_L("Autoload failed for parent policy: %d\n"),
                                aMsg.Int0()));
                }
            else
                {
                preLoadHandle = iCurrentPolicyHandle.iHandle;
                }
            }

        // Load the policy itself
        ProcessLoadPolicyL(aMsg, aSession, EScopedManualLoad);
        parentPolicyHandle = iCurrentPolicyHandle.iHandle;

        // Load BeforescopedLoadPolicies before
        if (iAfterScopedLoadPolicy->Size() != 0 )
            {
            TRAP(leaveCode,
                 AutoloadPoliciesL(zoneInfoSet,
                                   iAfterScopedLoadPolicy,
                                   EAutoloadAfterScopedLoad));

            if (leaveCode != KErrNone)
                {
                LOG(Log::Printf(_L("Autoload failed for parent policy: %d\n"),
                                aMsg.Int0()));
                }
            else
                {
                postLoadHandle = iCurrentPolicyHandle.iHandle;
                }
            }

        if (parentPolicyHandle != 0
            && (preLoadHandle != 0 || postLoadHandle != 0))
            {
            // Add the policy hadle pair to an 'unload list'
            AddScopedAutoloadPolicyPairL(preLoadHandle,
                                         postLoadHandle,
                                         parentPolicyHandle);
            }
        }
    else
        {
        // In case of manual load
        if (iManualAutoloadHandlePair.iManualPreloadHandle.iHandle == 0
            && iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle == 0)
            {
            // Load BeforescopedLoadPolicies before
            if (iBeforeManualLoadPolicy->Size() != 0)
                {
                TRAP(leaveCode,
                     AutoloadPoliciesL(zoneInfoSet,
                                       iBeforeManualLoadPolicy,
                                       EAutoloadBeforeManualLoad));
                if (leaveCode != KErrNone)
                    {
                    LOG(Log::Printf(_L("Autoload failed for parent policy: %d\n"),
                                    aMsg.Int0() ));
                    }
                else
                    {
                    iManualAutoloadHandlePair.iManualPreloadHandle.iHandle =
                        iCurrentPolicyHandle.iHandle;
                    }
                }

            // Load the policy itself
            ProcessLoadPolicyL(aMsg, aSession, EManualLoad);

            // Load AfterManualLoadPolicies
            if (iAfterManualLoadPolicy->Size() != 0)
                {
                TRAP(leaveCode,
                     AutoloadPoliciesL(zoneInfoSet,
                                       iAfterManualLoadPolicy,
                                       EAutoloadAfterManualLoad));
                if (leaveCode != KErrNone)
                    {
                    LOG(Log::Printf(_L("Autoload failed for parent policy: %d\n"),
                                    aMsg.Int0() ));
                    }
                else
                    {
                    iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle =
                        iCurrentPolicyHandle.iHandle;
                    }
                }
            }
        else
            {
            // Load the policy itself
            ProcessLoadPolicyL(aMsg, aSession, EManualLoad);
            }
            
        }

    return KErrNone;
    }

//
//
// ProcessActivatePolicy - Process an ActivatePolicy request issued
// by IPSecPolicyManApi
//
//
TInt 
CIPSecPolicyManagerHandler::ProcessActivatePolicyL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession)
    {
    // Store the parameters to this object
    iSession = aIPSecPolicyManagerSession;
    iCurrentPolicyHandle.iHandle = aMsg.Int0();

    LOG(Log::Printf(_L("ActivatePolicy request, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // Search from the active policy list the policy corresponding
    // to the current policy handle and activate it
    TInt ret = SearchPolicyFromListAndActivate();
    if (ret != KErrNone)
        {
        ErrorHandlingL(ret, 0 );
        }

    // Parse all active policy files from string format
    // to IPSecPolParser class object formats
    ParseAllPolicyFilesL();

    // Calculate the combined policy Bypass/Drop mode
    CalculateCombinedPolicyBypassDropMode();

    // Delete all pure Inbound/Ooutbound selectors
    DeleteExtraInboundOutboundSelectors();

    //  Update the policy selectors by building a comparison word
    //  and setting a sequence number. IpsecPolParser uses them.
    SortSelectors();

    // Convert object format policy pieces to string format.
    // This format is used by IPSec protocol component.
    ConvertFromObjectsToStringWithoutSectionsL();

    // Add an Inbound/outbound selector pair to the end of string format
    // policy file. This occurs only when the bypass mode is defined.
    TInt err = AddInboundOutboundSelectorPair();
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }

    // Send the algorithms table and  the string format policy file to
    // IPSec protocol component using Secpol socket
    SendAlgorithmsAndPolicyToIPSecL(_L("secpol6"));

    LOG(Log::Printf(_L("ActivatePolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // An API call completed
    ApiCallCompleted();
    return KErrNone;
    }

//
//
// ProcessActivateAutoloadPolicy - Process an ActivatePolicy request issued
// by IPSecPolicyManApi
//
//
TInt 
CIPSecPolicyManagerHandler::ProcessActivateAutoloadPolicyL(
    const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession)
    {
    // Store the parameters to this object
    iSession = aIPSecPolicyManagerSession;

    LOG(Log::Printf(_L("ActivateAutoloadPolicy request, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // Search from the active policy list the policy corresponding
    // to the current policy handle and activate it
    TInt ret = SearchPolicyFromListAndActivate();
    if (ret != KErrNone)
        {
        ErrorHandlingL(ret, 0 );
        }

    // Parse all active policy files from string format
    // to IPSecPolParser class object formats
    ParseAllPolicyFilesL();

    // Calculate the combined policy Bypass/Drop mode
    CalculateCombinedPolicyBypassDropMode();

    // Delete all pure Inbound/Ooutbound selectors
    DeleteExtraInboundOutboundSelectors();

    //  Update the policy selectors by building a comparison word
    //  and setting a sequence number. IpsecPolParser uses them.
    SortSelectors();

    // Convert object format policy pieces to string format.
    // This format is used by IPSec protocol component.
    ConvertFromObjectsToStringWithoutSectionsL();

    // Add an Inbound/outbound selector pair to the end of string format
    // policy file. This occurs only when the bypass mode is defined.
    TInt err = AddInboundOutboundSelectorPair();
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }

    // Send the algorithms table and  the string format policy file to
    // IPSec protocol component using Secpol socket
    SendAlgorithmsAndPolicyToIPSecL(_L("secpol6"));

    LOG(Log::Printf(_L("ActivateAutoloadPolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // An API call completed
    ApiCallCompleted();
    return KErrNone;
    }


//
// Process UnloadPolicy - Process an UnloadPolicy request issued
// by IPSecPolicyManApi
//
TInt 
CIPSecPolicyManagerHandler::ProcessUnloadPolicyL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession)
    {
    // Store the parameters to this object
    iSession = aIPSecPolicyManagerSession;

    iCurrentPolicyHandle.iHandle = aMsg.Int0();
    LOG(Log::Printf(_L("UnloadPolicy request, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // Search from the active policy list the policy corresponding
    // to the current policy handle and delete it
    TInt ret = DeletePolicyFromList();
    if (ret != KErrNone)
        {
        ErrorHandlingL(ret, 0 );
        }

    // Send BypassAll file to the IPSec protocol component if
    // the deleted policy was the last one
    if (iActivePolicyList->Count() == 0)
        {
        SendNullFileToIPSecL(_L("secpol6"));
        LOG(Log::Printf(_L("Last policy deleted\n")));
        LOG(Log::Printf(_L("UnLoadPolicy request completed OK, handle: %d\n"),
                        iCurrentPolicyHandle.iHandle));

        // An API call completed
        ApiCallCompleted();
        return KErrNone;
        }

    // Parse all active policy files from string format
    // to IPSecPolParser class object formats
    ParseAllPolicyFilesL();

    // Calculate whether it's drop or bypass mode to be used
    CalculateCombinedPolicyBypassDropMode();

    // Delete pure Inbound/Outbound selectors
    DeleteExtraInboundOutboundSelectors();

    //  Update the policy selectors by building a comparison word
    //  and setting a sequence number
    SortSelectors();

    // Convert object format policy pieces to string format.
    // This format is used by IPSec protocol component.
    ConvertFromObjectsToStringWithoutSectionsL();

    // Add an Inbound/outbound selector pair to the end of string format
    // policy file. This occurs only when the bypass mode is defined.
    TInt err = AddInboundOutboundSelectorPair();
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }

    // Send the algorithms table and  the string format policy file to
    // IPSec protocol component using Secpol socket
    SendAlgorithmsAndPolicyToIPSecL(_L("secpol6"));

    // An API call completed
    LOG(Log::Printf(_L("UnloadPolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));
    ApiCallCompleted();
    return KErrNone;
    }

//
// Convert the object format policy pieces to string format
// and do the operations defined by the function code.
// Section Data, for example, [POLICY], is included
//
void 
CIPSecPolicyManagerHandler::ConvertFromObjectsToStringWithSectionsL(
    TInt aFunction,
    TInt aBypassDropMode)
    {
    // Constant for adding Mip4 bypass selectors to the policy
    // TODO:
    //  This constant should be defined in Policy Manager API
    //  header files 
    const TInt KAddMip4BypassSelectors = (1 << 3);

    // Allocate a buffer for string format policy
    delete iPolBfr;
    iPolBfr = NULL;
    iPolBfr = HBufC8::NewL(KPolicyBufferSizeIncrement);

    // Convert and write  policy data to a policy buffer
    TInt err = TIpSecParser::Write(iPieceData, iPolBfr);
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }

    // Check if given policy contains 'drop_everything_else' rule
    // and add IKE, DHCP and MIPv4 bypass selectors if necessary 
    if (aBypassDropMode == KDropMode)
        {
        // Allow plain IKE negotiation packets. Write  the bypass
        // selectors  to the end of selector list, but they will
        // later be sorted to the beginning of selectors.
        if (aFunction & KAddIkeBypassSelectors)
            {
            TInt err = WriteTunnelModeIkeNegotiationStringsL(iPolBfr);
            if (err != KErrNone)
                {
                ErrorHandlingL(ENoMemory, err);
                }

            err = WriteTransportModeIkeNegotiationStrings(iPolBfr);
            if (err != KErrNone)
                {
                ErrorHandlingL(ENoMemory, err);
                }
            }

        // Allow plain DHCP negotiation packets. Write bypass mode
        // selectors for DHCP ports (67, 68) to the end of selector list.
        if (aFunction & KAddDhcpBypassSelectors)
            {
            TInt err = BuildDhcpProtocolString(iPolBfr);
            if (err != KErrNone)
                {
                ErrorHandlingL(ENoMemory, err);
                }
            }

        // Allow plain MIPv4 (Mobile IP v4) packets. Write  bypass mode
        // selectors for MIPv4 to the end of selector list.
        if (aFunction & KAddMip4BypassSelectors)
            {
            TInt err = BuildMip4BypassSelectors(iPolBfr);
            if (err != KErrNone)
                {
                ErrorHandlingL(ENoMemory, err);
                }
            }
        }
    }

//
// Convert object format policy pieces to string format.
// Section Data is not included
//
void 
CIPSecPolicyManagerHandler::ConvertFromObjectsToStringWithoutSectionsL()
    {
    // Allocate a buffer for string format policy
    delete iPolBfr;
    iPolBfr = NULL;
    iPolBfr = HBufC8::NewL(KPolicyBufferSizeIncrement);

    // Convert and write policy data to a policy buffer
    TInt err = TPolicyParser::Write(iPieceData->Policies(), iPolBfr, ETrue);
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }
    }

//
//  Store  a policy entry to the  policy list
//
void 
CIPSecPolicyManagerHandler::StorePolicyToActiveListL(
    TPolicyType aPolType,
    TInt aBypassOrDropMode)
    {
    // Take the next policy file handle
    iNextPolicyHandle.iHandle++;
    iCurrentPolicyHandle.iHandle = iNextPolicyHandle.iHandle;

    // Update an active policy entry
    TActivePolicyListEntry* entry = new (ELeave) TActivePolicyListEntry;
    CleanupStack::PushL(entry);
    entry->iActiveState = EFalse;
    entry->iBypassOrDropMode = aBypassOrDropMode;
    entry->iPolicyHandle.iHandle = iCurrentPolicyHandle.iHandle;
    entry->iPolicyBuf = iPolBfr;
    entry->iPolicyType = aPolType;
    iPolBfr = NULL;
    CleanupStack::PushL(entry->iPolicyBuf);

    // Store a new entry to active policy list
    iActivePolicyList->AppendL(entry);
    CleanupStack::Pop(2);
    }

//
//  Parse all active policy files from string format
//  to IPSecPolParser class object format
//
void 
CIPSecPolicyManagerHandler::ParseAllPolicyFilesL()
    {
    delete iPieceData;
    iPieceData = NULL;
    iPieceData = new (ELeave) CIpSecurityPiece;
    iPieceData->ConstructL();

    for (TInt i = 0; i < iActivePolicyList->Count(); i++)
        {
        // Check if policy activated
        if (iActivePolicyList->At(i)->iActiveState == EFalse) 
            {
            continue;
            }

        // Store policy to IPSecPolParser
        HBufC8* policyHBufC8 = iActivePolicyList->At(i)->iPolicyBuf;

        // Copy policy to 16bit buffer
        TPtr8 ptr8(policyHBufC8->Des());
        TInt length = ptr8.Length();
        HBufC *policyDataHBufC16 = HBufC::NewL(length);
        CleanupStack::PushL(policyDataHBufC16);
        TPtr ptr(policyDataHBufC16->Des());

        // Parse policy into binary object format  
        ptr.Copy(ptr8);
        TIpSecParser parser(ptr);          
        TInt err = parser.ParseAndIgnoreIKEL(iPieceData);
        
        // Delete policyDataHBufC16;
        CleanupStack::PopAndDestroy();

        if (err != KErrNone)
            {
            ErrorHandlingL(EParsingError, err);
            }
        }
    }

//
//  Sort the selectors in the following sequence:
//  -- Port 500 or 4500 is defined for selector
//  -- Port 67 or 68 is defined
//  -- All other selectors are in original order
//
void 
CIPSecPolicyManagerHandler::SortSelectors()
    {
    CSecurityPolicy* sp = iPieceData->Policies();
    CSelectorList* selectorList = sp->SelectorList();
    BuildComparisonWord(selectorList);
    SetSequenceNumbers(selectorList);
    }

//
// Send the algorithms file and string format policy file to
// IPSec protocol  component using Secpol socket
//
void 
CIPSecPolicyManagerHandler::SendAlgorithmsAndPolicyToIPSecL(
    const TDesC &aSocket)
    {
    TInt err(KErrNone);
    TInt algSize(0), polSize(0), sendBuf(0), octetSize(0);

    TInetAddr addr;
    TRequestStatus stat;

    // Open Secpol socket
    err = iSock.Open(iSS, aSocket);
    if (err != KErrNone)
        {
        ErrorHandlingL(EOpenSocketError, err);
        }
    iSecpolSocketOpen = ETrue;

    // Bind to socket
    addr.SetAddress(KInetAddrNone);
    addr.SetPort(0);
    err = iSock.Bind(addr);
    if (err != KErrNone)
        {
        ErrorHandlingL(EBindSocketError, err);
        }
    iSock.LocalName(addr);


    // Get algorithm table size and police file size
    if (iAlgorithmsHBufC8 != NULL)
        {
        algSize = iAlgorithmsHBufC8->Length();
        }
    polSize = iPolBfr->Des().Length();

    // Allocate a buffer for the algorithms data and policy  data
    HBufC8 *buf = HBufC8::NewLC(algSize + polSize);   // Both files
    TPtr8 algPtr((TUint8 *)buf->Ptr(), algSize, algSize);
    TPtr8 polPtr((TUint8 *)buf->Ptr() + algSize, polSize, polSize);

    if (iAlgorithmsHBufC8 != NULL)
        {
        algPtr.Copy(*iAlgorithmsHBufC8);
        }

    // Store the policy buffer data after algorithm data
    // and free the policy buffer
    polPtr.Copy(iPolBfr->Des());
    delete iPolBfr;
    iPolBfr = NULL;

    // Currently, the policy loading requires the policy data in a
    // single contiguous buffer. If the socket buffer is less than
    // policy file length, then increase the buffer size!

    // *2 because _UNICODE
    octetSize = 2 * (algSize + polSize);
    sendBuf = -1;
    err = iSock.GetOpt(KSOSendBuf, KSOLSocket, sendBuf);
    if (sendBuf < octetSize)
        {
        err = iSock.SetOpt(KSOSendBuf, KSOLSocket, octetSize);
        }
    if (err != KErrNone)
        {
        // The buffer holding the 8-bit data in the file
        CleanupStack::PopAndDestroy();
        ErrorHandlingL(ESecpolSocketSetOptError, err);
        }

    // Set buffer address
    HBufC *data = HBufC::NewL(algSize + polSize);
    TPtr ptr16(data->Des());
    TPtrC8 ptr((TUint8 *)buf->Ptr(), algSize + polSize);

    // For debug
    ptr16.Copy(ptr);
    
#ifdef TESTFLAG 

	// TODO: Copy the concatenated string for testing purposes
	delete iStringBuf;
	iStringBuf = HBufC::NewL(algSize + polSize);
	*iStringBuf = *data;
	
#endif

    // The buffer holding the 8-bit data in the file
    CleanupStack::PopAndDestroy();

    CleanupStack::PushL(data);
    ptr.Set((TUint8 *)ptr16.Ptr(), ptr16.Size());

    // Write the data over socket to IPSec protocol component
    iSock.Write(ptr, stat);
    User::WaitForRequest(stat);

    // The buffer holding the data in the file
    CleanupStack::PopAndDestroy();
    if (stat.Int() != KErrNone)
        {
        ErrorHandlingL(EWriteSocketError, stat.Int());
        }
    iSock.Close();
    iSecpolSocketOpen = EFalse;

    // Start the SecpolReader object that reads the log messages
    // sent by IPSec protocol component
    if (!iSecpolReader6)
        {
        iSecpolReader6 = new (ELeave) CSecpolReader(this);
        err = iSecpolReader6->Construct(_L("secpol6"));
        if (err != KErrNone)
            {
            delete iSecpolReader6;
            iSecpolReader6 = NULL;
            ErrorHandlingL(ESecpolReaderError, err);
            }
        }
    }

//
// Send a "Bypass All" policy to the IPSec protocol  component using Secpol
// socket. It indicates that IPSec accepts all packets.
//
void 
CIPSecPolicyManagerHandler::SendNullFileToIPSecL(const TDesC &aSocket)
    {
    TInt err(KErrNone);
    TInetAddr addr;
    TRequestStatus stat;
    TInt algSize(0), polSize(0);

    _LIT8 (KBypassAllString, " inbound = {}\n outbound = {}\n");
    // Open Secpol socket
    err = iSock.Open(iSS, aSocket);
    if (err != KErrNone)
        {
        ErrorHandlingL(EOpenSocketError, err);
        }
    iSecpolSocketOpen = ETrue;

    // Bind to socket
    addr.SetAddress(KInetAddrNone);
    addr.SetPort(0);
    err = iSock.Bind(addr);
    if (err != KErrNone)
        {
        ErrorHandlingL(EBindSocketError, err);
        }
    iSock.LocalName(addr);

    polSize = KBypassAllString().Length();
	if (iAlgorithmsHBufC8 != NULL)
		{
		algSize = iAlgorithmsHBufC8->Des().Length();
		}

    // Both files
    HBufC8* buf = HBufC8::NewLC(algSize + polSize);
    TPtr8 algPtr((TUint8 *)buf->Ptr(), algSize, algSize);
    TPtr8 polPtr((TUint8 *)buf->Ptr() + algSize, polSize, polSize);

    if (iAlgorithmsHBufC8 != NULL)
        {
        algPtr.Copy(*iAlgorithmsHBufC8);
        }

    // Store the policy buffer data after algorithm data
    // and free the policy buffer
    polPtr.Copy(KBypassAllString);

    // Allocate a buffer for the KBypassAllString
    HBufC* data = HBufC::NewL(algSize + polSize);
    TPtr ptr16(data->Des());
    TPtrC8 ptr((TUint8 *)buf->Ptr(), algSize + polSize);

    // For debug
    ptr16.Copy(ptr);

    // The buffer holding the 8-bit data in the file
    CleanupStack::PopAndDestroy();

    CleanupStack::PushL(data);
    ptr.Set((TUint8 *)ptr16.Ptr(), ptr16.Size());

    // Write the BypassAllString over socket to IPSec protocol component
    iSock.Write(ptr, stat);
    User::WaitForRequest(stat);

    // The buffer holding the data in the file
    CleanupStack::PopAndDestroy();
    iSock.Close();
    iSecpolSocketOpen = EFalse;
    }

//
// Search from the active policy list the policy corresponding
// to the current policy handle and delete it
//
TInt 
CIPSecPolicyManagerHandler::DeletePolicyFromList()
    {
    TInt count(iActivePolicyList->Count());
    for (TInt i = 0; i < count; i++)
        {
        if (iActivePolicyList->At(i)->iPolicyHandle.iHandle ==
            iCurrentPolicyHandle.iHandle)
            {
            delete iActivePolicyList->At(i)->iPolicyBuf;
            iActivePolicyList->At(i)->iPolicyBuf = NULL;
            delete iActivePolicyList->At(i);
            iActivePolicyList->Delete(i);
            return KErrNone;
            }
        }
    return (EUnknownPolicyHandle);
    }

//
// Search from the active policy list the policy corresponding
// to the current policy handle
//
TInt 
CIPSecPolicyManagerHandler::SearchPolicyFromListAndActivate()
    {
    TInt count(iActivePolicyList->Count());
    for (TInt i = 0; i < count; i++)
        {
        if (iActivePolicyList->At(i)->iPolicyHandle.iHandle ==
            iCurrentPolicyHandle.iHandle)
            {
            iActivePolicyList->At(i)->iActiveState = ETrue;
            return KErrNone;
            }
        }
    return (EUnknownPolicyHandle);
    }

//
// Return the new Policy File Handle to the API user
//
void 
CIPSecPolicyManagerHandler::ReturnPolicyFileHandleL(
    const RMessage2& aMsg)
    {
    TPckg<TPolicyHandle> pckgPolicyHandle(iCurrentPolicyHandle);
    aMsg.WriteL(SECOND_ARGUMENT, pckgPolicyHandle);
    }

//
// An API call completed
//
void 
CIPSecPolicyManagerHandler::ApiCallCompleted()
    {
    ReleaseResources();
    }

//
// Read the autoload configuration data
//
void 
CIPSecPolicyManagerHandler::ReadAutoloadConfigDataL()
    {
    TBool findVar(EFalse);
    TInt leaveCode(KErrNone);
    TBuf<32> loadIndexBuf;

    iPreloadPolicy = HBufC8::NewL(1);
    iBeforeManualLoadPolicy = HBufC8::NewL(1);
    iAfterManualLoadPolicy = HBufC8::NewL(1);
    iBeforeScopedLoadPolicy = HBufC8::NewL(1);
    iAfterScopedLoadPolicy = HBufC8::NewL(1);

    // Open ini file
    CESockIniData* preloadIniFile = NULL;

    TRAP(leaveCode, preloadIniFile = CESockIniData::NewL(KPreloadFileName));
    if (leaveCode != KErrNone)
        {
        return;
        }

    CleanupStack::PushL(preloadIniFile);

    TPtrC fileName(NULL, 0);
    TPtrC loadFlag(NULL, 0);

    TInt index(0);
    _LIT(KAutoloadEntry, "Autoload%d");

    loadIndexBuf.Format(KAutoloadEntry, index);
    while (preloadIniFile->FindVar(loadIndexBuf, KLoadFlag, loadFlag))
        {
        findVar = preloadIniFile->FindVar(loadIndexBuf, KFileName, fileName);
        // If there is no such configuration information, no autoload takes place
        if (!findVar)
            {
            goto forward;
            }
        findVar = preloadIniFile->FindVar(loadIndexBuf, KLoadFlag, loadFlag);
        if (!findVar || (loadFlag == KAutoloadNone))
            {
            goto forward;
            }

        // Check the values of the autoload flag and read & store autoload
        // policy file accordingly
        if (loadFlag == KAutoloadPreload)
            {
            ReadNextAutoloadPolicyL(iPreloadPolicy, fileName);
            iIsPreloadNeeded = ETrue;
            }
        if (loadFlag == KAutoloadBeforeManualLoad)
            {
            ReadNextAutoloadPolicyL(iBeforeManualLoadPolicy, fileName);
            }
        if (loadFlag == KAutoloadAfterManualLoad)
            {
            ReadNextAutoloadPolicyL(iAfterManualLoadPolicy, fileName);
            }
        if (loadFlag == KAutoloadBeforeScopedLoad)
            {
            ReadNextAutoloadPolicyL(iBeforeScopedLoadPolicy, fileName);
            }
        if (loadFlag == KAutoloadAfterScopedLoad)
            {
            ReadNextAutoloadPolicyL(iAfterScopedLoadPolicy, fileName);
            }

        forward:
            index++;
            loadIndexBuf.Format(KAutoloadEntry, index);
        }

    // Ini file closed
    CleanupStack::PopAndDestroy(preloadIniFile);
    }

//
// read the next autoload policy file
//
void 
CIPSecPolicyManagerHandler::ReadNextAutoloadPolicyL(
    HBufC8*& aAutoloadPolicyBuffer,
    TPtrC& aPolicyFileName)
    {
    RFile fileAutoload;
    CleanupClosePushL(fileAutoload);
    TInt err = fileAutoload.Open(iFs, aPolicyFileName, EFileRead);

    if (err != KErrNone)
        {
        CleanupStack::PopAndDestroy(); // fileAutoload
        return ;
        }

    TInt polSize;
    err = fileAutoload.Size(polSize);

    if (err != KErrNone)
        {
        CleanupStack::PopAndDestroy(); // fileAutoload
        return ;
        }

    // Read current policy size
    TInt currentPolSize = aAutoloadPolicyBuffer->Length();

    // Create temporary buffer that will concatenate policies and push
    // it to cleanup stack
    HBufC8* tempFileBuf = HBufC8::NewLC(currentPolSize + polSize + 1);

    // Store current policies to temp buf
    *tempFileBuf = *aAutoloadPolicyBuffer;
    TPtr8 policyPtr8((TUint8*)tempFileBuf->Ptr() + currentPolSize,
                     polSize,
                     polSize);

    // Read new autoload polici data to the same temp buffer
    err = fileAutoload.Read(policyPtr8);
    if (err != KErrNone)
        {
        CleanupStack::PopAndDestroy(2); // tempFileBuf,fileAutoload
        return ;
        }

    // Delete old policy buffer
    delete aAutoloadPolicyBuffer;
    aAutoloadPolicyBuffer = NULL;

    // Create new policy buffer
    aAutoloadPolicyBuffer = HBufC8::NewL(currentPolSize + polSize + 1);
    TPtr8 policyBuf(aAutoloadPolicyBuffer->Des());

    // Copy contents of concatenated buffer to new policy buffer
    policyBuf.Copy( (TUint8*)tempFileBuf->Ptr() , currentPolSize + polSize);
    policyBuf.ZeroTerminate();

    // Delete temporary buffer and close policy file
    CleanupStack::PopAndDestroy(2);
    }

//
// Set the flag determining the autoload policy status
//
void 
CIPSecPolicyManagerHandler::SetAutoloadPolicyActive(
    TBool aIsAutoloadPolicyActive)
    {
    iIsAutoloadPolicyActive = aIsAutoloadPolicyActive;
    }

//
// Check is there is an active autoload policy
//
TBool 
CIPSecPolicyManagerHandler::IsAutoloadPolicyActive()
    {
    return iIsAutoloadPolicyActive;
    }

//
// Perform the autoload for policies
//
void 
CIPSecPolicyManagerHandler::AutoloadPoliciesL(
    const TZoneInfoSet& aZoneInfoSet,
    HBufC8* aPolicyBuffer,
    TAutoloadFlags aAutoloadType)
    {
    // Zone Info
    iVPNNetId = 0;
    if (aZoneInfoSet.iSelectorZone.iScope != KScopeNone)
        {
        iVPNNetId = aZoneInfoSet.iSelectorZone.iId;
        }
    iGwNetId = 0;
    if (aZoneInfoSet.iEndPointZone.iScope != KScopeNone)
        {
        iGwNetId = aZoneInfoSet.iEndPointZone.iId;
        }

    LOG(Log::Printf(_L("LoadPolicy request VPN NetId: %d  GW NetId: %d\n"),
                    iVPNNetId, iGwNetId));

    // Autoload policies are loaded into a separate descriptor
    if (aAutoloadType == EAutoloadPreload)
        {
        delete iPolicyHBufC8;
        iPolicyHBufC8 = NULL;
        iPolicyHBufC8 = HBufC8::NewL(iPreloadPolicy->Length());
        TPtr8 autoloadPolicyDataDesc(iPolicyHBufC8->Des());
        autoloadPolicyDataDesc.Copy(iPreloadPolicy->Des());
        }
    else
        {
        delete iPolicyHBufC8;
        iPolicyHBufC8 = NULL;
        iPolicyHBufC8 = HBufC8::NewL(aPolicyBuffer->Length());
        TPtr8 autoloadPolicyDataDesc(iPolicyHBufC8->Des());
        autoloadPolicyDataDesc.Copy(aPolicyBuffer->Des());
        }

    // Parse the policy file from string format to binary object format
    ParseCurrentPolicyL();

    // Calculate the Bypass/Drop mode of parsed policy
    TInt policyBypassDropMode(KDropMode);
    CSecurityPolicy* policy = iPieceData->Policies();
    policyBypassDropMode = CalculatePolicyBypassDropMode(*policy);

    // Add VPNNetId to CPolicySelector and GwNetId to CSecpolBundleItem objects
    UpdateSelectorsAndTunnels();

    // Check if the IP addresses in the selectors of current policy
    // file overlaps with the selectors of the policies in the active
    // policy list. Overlapping is allowed when the scope IDs differ
    // or all parameters in the selectors and corresponding SA
    // match exactly.
    CheckSelectorConflictsL();

    //  Modify the SA names to avoid conflicts caused by equal names
    //  in different policies
    MakeUniqueSANamesL();

    // Convert object format policy pieces to string format
    // and do the operations defined by the API flags
    ConvertFromObjectsToStringWithSectionsL(iFunction,
                                            policyBypassDropMode);

    // Store current policy entry to active policy list
    StorePolicyToActiveListL(EAutoload, policyBypassDropMode);
    LOG(Log::Printf(_L("LoadPolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // An API call completed
    ApiCallCompleted();

    if (SearchPolicyFromListAndActivate() == KErrNone)
        {
        SetAutoloadPolicyActive(ETrue);
        }
    else
        {
        LOG(Log::Printf(_L("LoadPolicy request completed OK, handle: %d\n"),
                        iCurrentPolicyHandle.iHandle));
        }
    }

//
// Store the preload policy handle
//
void 
CIPSecPolicyManagerHandler::StorePreloadPolicyHandle()
    {
    iPreloadPolicyHandle.iHandle = iCurrentPolicyHandle.iHandle;

    // Delete preload buffer because it's not needed anymore
    delete iPreloadPolicy;
    iPreloadPolicy = NULL;
    }

//
// Find the associated autoload policy for a certain policy
//
CAutoloadListItem* 
CIPSecPolicyManagerHandler::FindScopedAutoloadPolicyPair(
    TUint32 aParentPolicyHandle)
    {
    for (TInt i = 0; i < iScopedAutoloadPolicyPairs.Count(); i++)
        {
        if (iScopedAutoloadPolicyPairs[i]->GetParentPolicyHandle() 
            == aParentPolicyHandle)
            {
            return iScopedAutoloadPolicyPairs[i];
            }
        }

    return NULL;
    }

//
// Add the associated autoload policy pair for a certain policy
//
void 
CIPSecPolicyManagerHandler::AddScopedAutoloadPolicyPairL(
    TUint32 aPreloadPolicyHandle,
    TUint32 aPostloadPolicyHandle,
    TUint32 aParentPolicyHandle)
    {
    CAutoloadListItem* autoloadListItem =
        CAutoloadListItem::NewL(aPreloadPolicyHandle,
                                aPostloadPolicyHandle,
                                aParentPolicyHandle);

    if (autoloadListItem != NULL)
        {
		CleanupStack::PushL(autoloadListItem);
        iScopedAutoloadPolicyPairs.AppendL(autoloadListItem);
        CleanupStack::Pop(autoloadListItem);
        }
    }

//
// Delete the associated autoload policy pair for a certain policy
//
void 
CIPSecPolicyManagerHandler::DeleteScopedAutoloadPolicyPair(
    TUint32 aParentPolicyHandle)
    {
    for (TInt i = 0; i < iScopedAutoloadPolicyPairs.Count(); i++)
        {
        if (iScopedAutoloadPolicyPairs[i]->GetParentPolicyHandle() 
            == aParentPolicyHandle)
            {
            delete iScopedAutoloadPolicyPairs[i];
            iScopedAutoloadPolicyPairs.Remove(i);
            }
        }
    }

//
// read the algorithmsfile contents
//
void 
CIPSecPolicyManagerHandler::ReadAlgorithmsFileL()
    {
    RFile algFile;
    TInt algSize(0);
    CleanupClosePushL(algFile);
    TInt err = algFile.Open(iFs, KAlgorithmFile, EFileRead);

    delete iAlgorithmsHBufC8;
    iAlgorithmsHBufC8 = NULL;

    if (err == KErrNone)
        {
        err = algFile.Size(algSize);
        iAlgorithmsHBufC8 = HBufC8::NewL(algSize);
        TPtr8 algorithmsPtr8(iAlgorithmsHBufC8->Des());
        algFile.Read(algorithmsPtr8);
        }
    else
        {
        algSize = KAlgorithmConf().Length();
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT        
        algSize += KNewAlgorithmConf().Length();
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT        
        iAlgorithmsHBufC8 = HBufC8::NewL(algSize);
        TPtr8 algorithmsPtr8(iAlgorithmsHBufC8->Des());
        algorithmsPtr8.Copy(KAlgorithmConf);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT    
        algorithmsPtr8.Append(KNewAlgorithmConf);
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT         
        }

    CleanupStack::PopAndDestroy();
    }

TBool 
CIPSecPolicyManagerHandler::IsLastManualLoadPolicy(TUint32 aPolicyHandle)
    {
    for (TInt i = 0; i < iActivePolicyList->Count(); i++)
        {
        // If there are other manual load policies
        if ((iActivePolicyList->At(i)->iPolicyType == EManualLoad) &&
            iActivePolicyList->At(i)->iPolicyHandle.iHandle != aPolicyHandle)
            {
            return EFalse;
            }
        }
    return ETrue;
    }

//
// Process UnloadPolicy - Process an UnloadPolicy for the autoload policies
// The policy is identified by the policy handle passed on as a parameter
//
TInt 
CIPSecPolicyManagerHandler::UnloadPolicyByHandleL(
    TUint32 aPolicyHandle,
    const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession)
    {
    // Store the parameters to this object
    iSession = aIPSecPolicyManagerSession;
    iCurrentPolicyHandle.iHandle = aPolicyHandle;
    LOG(Log::Printf(_L("UnloadPolicy request, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));

    // Search from the active policy list the policy corresponding
    // to the current policy handle and delete it
    TInt ret = DeletePolicyFromList();
    if (ret != KErrNone)
        {
        ErrorHandlingL(ret, 0 );
        }

    // Send BypassAll file to the IPSec protocol component if
    // the deleted policy was the last one
    if (iActivePolicyList->Count() == 0)
        {
        SendNullFileToIPSecL(_L("secpol6"));
        LOG(Log::Printf(_L("Last policy deleted\n")));
        LOG(Log::Printf(_L("UnLoadPolicy request completed OK, handle: %d\n"),
                        iCurrentPolicyHandle.iHandle));

        // An API call completed
        ApiCallCompleted();
        return KErrNone;
        }

    // Parse all active policy files from string format
    // to IPSecPolParser class object formats
    ParseAllPolicyFilesL();

    // Calculate whether it's drop or bypass mode to be used
    CalculateCombinedPolicyBypassDropMode();

    // Delete all INBOUND/OUTBOUND selector pairs
    DeleteExtraInboundOutboundSelectors();

    //  Update the policy selectors by building a comparison word
    //  and setting a sequence number
    SortSelectors();

    // Convert object format policy pieces to string format.
    // This format is used by IPSec protocol component.
    ConvertFromObjectsToStringWithoutSectionsL();

    // Add an Inbound/outbound selector pair to the end of string format
    // policy file. This occurs only when the bypass mode is defined.
    TInt err = AddInboundOutboundSelectorPair();
    if (err != KErrNone)
        {
        ErrorHandlingL(ENoMemory, err);
        }

    // Send the algorithms table and  the string format policy file to
    // IPSec protocol component using Secpol socket
    SendAlgorithmsAndPolicyToIPSecL(_L("secpol6"));

    // An API call completed
    LOG(Log::Printf(_L("UnloadPolicy request completed OK, handle: %d\n"),
                    iCurrentPolicyHandle.iHandle));
    ApiCallCompleted();
    return KErrNone;
    }

//
// Process UnloadPolicy - Process an UnloadPolicy considering the autoload
// policies. The policy is identified by the policy handle passed on in the
// IPC message
//
TInt 
CIPSecPolicyManagerHandler::ProcessUnloadPoliciesL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession)
    {
    TInt err(KErrNone);
    TInt preloadHandle(0), postLoadHandle(0);
    TInt leaveCode(KErrNone);

    // Find the autoload policy paired with this parent policy and unload it
    CAutoloadListItem* autoLoadItem = FindScopedAutoloadPolicyPair(aMsg.Int0());

    if (autoLoadItem != NULL)
        {
        err = ProcessUnloadPolicyL(aMsg, aIPSecPolicyManagerSession);
        
        preloadHandle = autoLoadItem->GetPreloadPolicyHandle();
        if (preloadHandle > 0)
            {
            TRAP(leaveCode, UnloadPolicyByHandleL(preloadHandle,
                                                  aIPSecPolicyManagerSession));

            if (leaveCode != KErrNone)
                {
                LOG(Log::Printf(_L("UnloadAutoloadPolicy failed for handle: %d\n"), 
                    preloadHandle));
                }
            }

        postLoadHandle = autoLoadItem->GetPostloadPolicyHandle();
        if (postLoadHandle > 0)
            {
            TRAP(leaveCode, UnloadPolicyByHandleL(postLoadHandle,
                                                  aIPSecPolicyManagerSession));

            if (leaveCode != KErrNone)
                {
                LOG(Log::Printf(_L("UnloadAutoloadPolicy failed for handle: %d\n"), 
                postLoadHandle));
                }
            }

        DeleteScopedAutoloadPolicyPair(aMsg.Int0());
        }
    else
        {
        // If this is the last unload for manual policies
        if (IsLastManualLoadPolicy(aMsg.Int0()))
            {
            if (iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle != 0 )
                {
                TRAP(leaveCode,
                     UnloadPolicyByHandleL(iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle,
                                           aIPSecPolicyManagerSession));
                if (leaveCode != KErrNone)
                    {
                    LOG(Log::Printf(_L("UnloadAutoloadPolicy failed for handle: %d\n"), 
                        iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle));
                    }
                iManualAutoloadHandlePair.iManualPostLoadHandle.iHandle = 0;
                }

            err = ProcessUnloadPolicyL(aMsg, aIPSecPolicyManagerSession);
            if (iManualAutoloadHandlePair.iManualPreloadHandle.iHandle != 0 )
                {
                TRAP(leaveCode,
                     UnloadPolicyByHandleL(iManualAutoloadHandlePair.iManualPreloadHandle.iHandle,
                                           aIPSecPolicyManagerSession));
                if (leaveCode != KErrNone)
                    {
                    LOG(Log::Printf(_L("UnloadAutoloadPolicy failed for handle: %d\n"), 
                        iManualAutoloadHandlePair.iManualPreloadHandle.iHandle));
                    }

                iManualAutoloadHandlePair.iManualPreloadHandle.iHandle = 0;
                }
            }
        else
            {
            err = ProcessUnloadPolicyL(aMsg, aIPSecPolicyManagerSession);
            }
        }
        
    return err;
    }

//
// GetIPSecSAInfo - Search a selector object and correponding SAs
// and return the SA info data. The parameters in message
// object are:
// -- TIpsecSelectorInfo. Contains parameters used to
//    search the rigth selector object
// -- TIpsecSaSpec in which the SA Info is to be
//    returned
//
//
TInt 
CIPSecPolicyManagerHandler::GetIPSecSAInfoL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession*   /*aIPSecPolicyManagerSession*/)
    {
    LOG(Log::Printf(_L("MatchSelector (GetIPsecSaInfo) request\n")));

    // Allocate buffer for TIpsecSelectorInfo object and read the object
    // data from the client
    delete iSelectorInfo;
    iSelectorInfo = NULL;
    iSelectorInfo = new(ELeave) TIpsecSelectorInfo;
    TPckg<TIpsecSelectorInfo>pckgSelectorInfo(*iSelectorInfo);
    aMsg.ReadL(FIRST_ARGUMENT , pckgSelectorInfo);

    // Allocate buffer for TIPSecSAInfo object
    delete iSAInfo;
    iSAInfo = NULL;
    iSAInfo = new(ELeave) TIpsecSaSpec;
    TPckg<TIpsecSaSpec>pckgSaSpec(*iSAInfo);
    iPckgSAInfo = &pckgSaSpec;

    // Parse all active policy files from string format to IPSecPolParser 
    // class object format
    ParseAllPolicyFilesL();
    if (iActivePolicyList->Count() == 0)
        {
        ErrorHandlingL(ENoSelectorFound, 0);
        }

    // Find selector object that corresponds to the parameters
    // in TIPSecSelectorInfo object
    CPolicySelector* policySelector = FindMatchingSelector();
    if (policySelector == NULL)
        {
        ErrorHandlingL(ENoSelectorFound, 0);
        }

    // Fill the TIPSecSAInfo object with the data of an SA
    // (TSecurityAssocSpec)
    FillSAInfoObject(policySelector, iSelectorInfo->iSaIndex);
    
    // Write the SAInfo block to the client's address space
    TPckg<TIpsecSaSpec> pckgSAInfoWrite(*iSAInfo);
    aMsg.WriteL(SECOND_ARGUMENT, pckgSAInfoWrite);

    // An API call completed
    LOG(Log::Printf(_L("MatchSelector request completed OK\n")));
    ApiCallCompleted();
    return KErrNone;
    }

//
// GetLastConflictInfo (GetDebugInfo) - Returns information about
// the policy that caused policy activation to fail.
//  ARG 0 = aDebugInfo buffer in API
//  ARG 1 = aInfoFlags
//   KConflictingPolicyInfo - Get the last conflict Info data
//   KParsingErrorInfo      - Get the last parsing error data
//
//
TInt 
CIPSecPolicyManagerHandler::GetLastConflictInfoL(
    const RMessage2& aMsg,
    const CIPSecPolicyManagerSession*   /*aIPSecPolicyManagerSession*/)
    {
    TUint flags = aMsg.Int1();

    //
    // Check what info the API user is getting
    //
    HBufC8* infoBfr = NULL;
    if (flags & KConflictingPolicyInfo)
        {
        infoBfr = iLastConflictInfo;
        LOG(Log::Printf(_L("GetDebugInfo request, conflicting info requested\n")));
        }
    else if (flags & KParsingErrorInfo)
        {
        infoBfr = iLastParsingErrorInfo;
        LOG(Log::Printf(_L("GetDebugInfo request, parsing error info requested\n")));
        }

    if (infoBfr != NULL)
        {
        // Copy info data to 16 bit buffer
        TPtr8 ptr8(infoBfr->Des());
        TInt length(ptr8.Length());
        
        HBufC16* infoBfr16 = HBufC16::NewL(length + 1);
        CleanupStack::PushL(infoBfr16);
        TPtr16 ptr(infoBfr16->Des());
        ptr.FillZ();
        ptr.Copy(ptr8);
        ptr.ZeroTerminate();

        // Write the info buffer content to the client's address space
        LOG(Log::Printf(_L("GetDebugInfo returned: %S\n"), &ptr));
        aMsg.WriteL(FIRST_ARGUMENT, *infoBfr16);
        
        // Release 16-bit info buffer 
        CleanupStack::PopAndDestroy(1);
        }
    else
        {
        ErrorHandlingL(ENoConflictInfoFound, 0);
        }

    // An API call completed
    ApiCallCompleted();
    return KErrNone;
    }

//
//  Convert the policy content to object format using IPSecPolParser
//
//
void 
CIPSecPolicyManagerHandler::ParseCurrentPolicyL()
    {
    delete iPieceData;
    iPieceData = NULL;
    iPieceData = new (ELeave) CIpSecurityPiece;
    iPieceData->ConstructL();

    // Copy policy to 16-bit buffer
    TPtr8 ptr8(iPolicyHBufC8->Des());
    TInt length = ptr8.Length();
    HBufC *policyDataHBufC16 = HBufC::NewL(length + 1);
    CleanupStack::PushL(policyDataHBufC16);
    TPtr ptr(policyDataHBufC16->Des());
    ptr.FillZ();
    ptr.Copy(ptr8);
    ptr.ZeroTerminate();

    // Parse policy from string format into binary object format
    TIpSecParser parser(ptr);          
    TInt err = parser.ParseAndIgnoreIKEL(iPieceData);
    CleanupStack::PopAndDestroy();    

    // If parsing error, allocate a buffer and read the last error
    // string from the ipsecpolparser. The API user will use the
    // GetDebugInfo method to get the string (method is named as
    // GetLastConflictInfo here).
    if (err != KErrNone)
        {
        delete iLastParsingErrorInfo;
        iLastParsingErrorInfo = NULL;

        // The same size as used in ipsecpolparser
        iLastParsingErrorInfo = HBufC8::NewL(200);
        iLastParsingErrorInfo->Des().Copy(iPieceData->iErrorInfo);
        ErrorHandlingL(EParsingError, err);
        }
    }

//
// CIPSecPolicyManagerHandler::UpdateSelectorsAndTunnels()
//
// Include VPNNetId to CPolicySelectors and GwNetId to CSecpolBundleItem
// objects
//
//
void
CIPSecPolicyManagerHandler::UpdateSelectorsAndTunnels()
    {
    // If VPN Network ID is set, store it to policy selectors
    if (iVPNNetId)
        {
        CSecurityPolicy* sp = iPieceData->Policies();
        CSelectorList* selectorList = sp->SelectorList();
        TInt count = selectorList->Count();

        // Iterate through selector (policy rule) list
        for (TInt i = 0; i < count; i++)
            {
            CPolicySelector* ps = selectorList->At(i);

            // Skip 'global', 'bypass/drop_everything_else' and 'interface' selectors
            if (ps->iGlobalSelector
                || (ps->iDirection == KPolicySelector_INTERFACE)
                || IsBypassEverythingElse(*ps) 
                || IsDropEverythingElse(*ps))
                {
                continue;
                }
            
             UpdatePolicySelectorScopeId(*ps, iVPNNetId);
            }
        }

    // If VPN Gateway Network ID is set, store it to policy bundles
    if (iGwNetId)
        {
        CSecurityPolicy* sp = iPieceData->Policies();
        CSelectorList* selectorList = sp->SelectorList();
        TInt count = selectorList->Count();

        // Iterate through selector (policy rule) list
        for (TInt i = 0; i < count; i++)
            {
            TBool isTunnelMode = EFalse;
            CPolicySelector* ps = selectorList->At(i);

            // Skip 'global', 'bypass/drop_everything_else' and 'interface' selectors
            if (ps->iGlobalSelector
                || (ps->iDirection == KPolicySelector_INTERFACE)
                || IsBypassEverythingElse(*ps) 
                || IsDropEverythingElse(*ps))
                {
                continue;
                }

            UpdatePolicyBundleScopeId(*ps, iVPNNetId, iGwNetId, isTunnelMode);

            // If not tunnel mode use VPN Gateway Network Id
            // as ScopeId when updating policy selector scope
            if (!isTunnelMode)
                {
                UpdatePolicySelectorScopeId(*ps, iGwNetId);
                }
            }
        }
    }

//
// CIPSecPolicyManagerHandler::UpdatePolicySelectorScopeId()
//
// Updates PolicySelector with the given ScopeId. The ScopeId can be either
// VPN Network Id or VPN Gateway Network Id.
//
// TODO:
// This method should be included within CPolicySelector class
// CPolicySelector::UpdateRemoteAddressScopeId(TInt aScopeId)
//
//
void
CIPSecPolicyManagerHandler::UpdatePolicySelectorScopeId(
    CPolicySelector& aPolicySelector,
    TInt aScopeId)
    {
    // Check if remote address is specified (other than any)
    if (!aPolicySelector.iRemote.IsUnspecified())
        {
        // Specific address exists, so convert address to IPv6 mapped
        if (aPolicySelector.iRemote.Family() == KAfInet)
            {
            aPolicySelector.iRemote.ConvertToV4Mapped();
            aPolicySelector.iRemoteMask.ConvertToV4Mapped();
            }

        // VPN Network ID shall be used only with addresses in network
        // scope (addresses in other scope levels are handled as global
        // selectors)
        if (aPolicySelector.iRemote.Ip6Address().Scope() == KIp6AddrScopeNetwork)
            {
            aPolicySelector.iRemote.SetScope(aScopeId);
            aPolicySelector.iRemoteMask.SetScope(0xFFFFFFFF);
            }
        }
    else
        {
        // Unspecified remote address so change address family to AfInet
        // and then build null address/mask and set ScopeId
        aPolicySelector.iRemote.SetFamily(KAfInet);
        aPolicySelector.iRemoteMask.SetFamily(KAfInet);
        aPolicySelector.iRemote.ConvertToV4Mapped();
        aPolicySelector.iRemoteMask.ConvertToV4Mapped();

        aPolicySelector.iRemote.SetScope(aScopeId);
        aPolicySelector.iRemoteMask.SetScope(0xFFFFFFFF);
        }
    }

//
// CIPSecPolicyManagerHandler::UpdatePolicyBundleScopeId()
//
// Updates bundle list of given PolicySelector by using the VPN Network Id and
// VPN Gateway Network Id to set the ScopeId.
//
// VPN Gateway Network Id is set as ScopeId to tunnel address. If there are
// nested tunnels(tunnel-in-tunnel), the VPN Gateway Network Id is stored to
// the last gateway and VPN Network Id to all other.
//
// TODO:
// This method should be included within CPolicySelector class
// CPolicySelector::UpdateBundleScopeId(TInt aScopeId, TInt aGwScopeId)
//
//
void
CIPSecPolicyManagerHandler::UpdatePolicyBundleScopeId(
    CPolicySelector& aPolicySelector,
    TInt aScopeId,
    TInt aGwScopeId,
    TBool& aIsTunnelMode)
    {
    // Iterate through action bundles to set ScopeId
    TSecpolBundleIter iter(aPolicySelector.iBundle);
    while (iter)
        {
        CSecpolBundleItem* item = (CSecpolBundleItem *)iter++;
        TBool isLast = (!iter ? ETrue : EFalse);
        TInt scopeId = (isLast ? aGwScopeId : aScopeId);

        // Check if tunnel address is set
        if (!item->iTunnel.IsUnspecified())
            {
            aIsTunnelMode = ETrue;

            // Map tunnel IPv4 address to Ipv6
            if (item->iTunnel.Family() == KAfInet)
                {
                item->iTunnel.ConvertToV4Mapped();
                }

            item->iTunnel.SetScope(scopeId);
            }
        }
    }
    
#ifdef TESTFLAG   
//
//
// CIPSecPolicyManagerHandler::RequestInfo()
//
// This is used for testing purposes. Test client queries information from the
// policy server
//  ARG 0 = aBuf which returns the concatenated string send to ipsec protocol
//			module
//  ARG 1 = aHandles which returns loaded autoload handles 
//
//  
TInt
CIPSecPolicyManagerHandler::RequestEvent(
	const RMessage2& aMsg,
	const CIPSecPolicyManagerSession* /*aIPSecPolicyManagerSession*/ )
	{	

	// In case when there have not yet been any call of
	// SendAlgorithmsAndPolicyToIPSecL(_L("secpol6"));
	// This should not be executed
	if ( !iStringBuf == NULL )
	{
		aMsg.WriteL(FIRST_ARGUMENT, (*iStringBuf));
	}
	
	// Deletes remained handles and creates new structure
	delete iHandles;	
    iHandles = NULL;
    iHandles = new(ELeave) TAutoloadHandles;
    
    // Initialization
    iHandles->iHandle1 = 0;
    iHandles->iHandle2 = 0;
    iHandles->iHandle3 = 0;
    iHandles->iHandle4 = 0;
    iHandles->iHandle5 = 0;
    
	TInt count(0); // Counting autoload handles
	
	// Finds active Autoload policies and copies them in right order to the iHandles
	// structure
    for (TInt i = 0; i < iActivePolicyList->Count() ; i++)
    	{
            
        if ( iActivePolicyList->At(i)->iPolicyType == EAutoload )  // Case autoload
        	{
        	
        	if ( count == 0 )
        		{
        		iHandles->iHandle1 = iActivePolicyList->At(i)->iPolicyHandle.iHandle;
        		}
        	else if ( count == 1)
        		{
        		iHandles->iHandle2 = iActivePolicyList->At(i)->iPolicyHandle.iHandle;
        		}
        	else if ( count == 2)
        		{
        		iHandles->iHandle3 = iActivePolicyList->At(i)->iPolicyHandle.iHandle;
        		}
        	else if ( count == 3)
        		{
        		iHandles->iHandle4 = iActivePolicyList->At(i)->iPolicyHandle.iHandle;
        		}
        	else if ( count == 4)
        		{
        		iHandles->iHandle5 = iActivePolicyList->At(i)->iPolicyHandle.iHandle;
        		} 
        	
        	count++;
        	}
        }
    
    // Creates the IPC package and copies iHandles to it
    TPckg<TAutoloadHandles> handlesPckgWrite(*iHandles);       
    
    // Writes IPC message
    aMsg.WriteL(SECOND_ARGUMENT, handlesPckgWrite);
	
	return KErrNone;
	}

#endif

//
// GetAvailableSelectors - Returns the available selectors information
//
//
TInt CIPSecPolicyManagerHandler::GetAvailableSelectors(const RMessage2& aMsg)
	{
	LOG(Log::Printf(_L("AvailableSelectors (GetAvailableSelectors) request\n")));
    // Write the SelectorInfo block to the client's address space 
   	TInt count = iSelectorInfoArray->Count();
   	TPtrC8 selectorInfoWrite(reinterpret_cast<TUint8*>(&(iSelectorInfoArray->At(0))), count * sizeof(TIpsecSelectorInfo));
   	aMsg.WriteL(FIRST_ARGUMENT, selectorInfoWrite);

    // An API call completed
    LOG(Log::Printf(_L("AvailableSelectors request completed OK\n")));
    ApiCallCompleted();
    return KErrNone;
    }

//
// GetSelectorsCount - Returns the number of selectors matching the
// gateway address passed by the client
//
//   
 TInt CIPSecPolicyManagerHandler::GetSelectorsCount(const RMessage2& aMsg)
 	{
 	LOG(Log::Printf(_L("EnumerateSelectors (GetSelectorsCount) request\n")));
 	
 	// Read the gateway address sent by the client
    TInetAddrPckg inetAddrPckg(iTunnel);
    aMsg.ReadL(FIRST_ARGUMENT , inetAddrPckg);
    
 	// Parse all active policy files from string format to IPSecPolParser 
    // class object format
    ParseAllPolicyFilesL();
    if (iActivePolicyList->Count() == 0)
        {
        ErrorHandlingL(ENoGatewayFound, 0);
        }
    
	// Find the selector object that corresponds
 	// to the gateway address sent by the client
    FillSelectorInfoObject();
    if (!iSelectorInfoArray->Count())
        {
        ErrorHandlingL(ENoGatewayFound, 0);
        }
	
	
	TPckg<TInt> selectorCount(iSelectorInfoArray->Count());
	aMsg.WriteL(SECOND_ARGUMENT, selectorCount);
	
	// An API call completed
    LOG(Log::Printf(_L("EnumerateSelectors request completed OK\n")));
    ApiCallCompleted();
    return KErrNone;
 	}

