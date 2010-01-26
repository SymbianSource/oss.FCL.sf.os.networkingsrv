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
// IPSecPolicyManHandler.h - IPSec Policy Manager Handler
//



/**
 @internalComponent
*/
#ifndef __IPSECPOLICYMANHANDLER__
#define __IPSECPOLICYMANHANDLER__

#include <e32base.h>
#include <e32std.h>
#include <f32file.h>
#include <es_sock.h>
#include <in_sock.h>

#include "ipsecpolapi.h"
#include "ipsecpol.h"
#include "autoloadlistitem.h"
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "spdb.h"
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT

//
// Direction codes
//
//
enum direction
{
    EInbound = 1,
    EOutbound
};

//
// iBypassOrDropMode flags
//
//
const TInt KDropMode       = 0;          // Drop mode
const TInt KInboundBypass  = (1 << 0);   // Inbound bypass mode
const TInt KOutboundBypass = (1 << 1);   // Outbound bypass mode

//
// Forward declarations
//
//
class CSecpolReader;
class CIPSecPolicyManagerServer;
class CPolicySelector;
class CIPSecPolicyManagerSession;
class CSelectorList;
class CIpSecurityPiece;
class TIpsecSelectorInfo;
class TIpsecSaSpec;
class CAutoloadListItem;
class CSecurityPolicy;
class TSecurityAssocSpec;

//
// Active Policy list entry
//
struct TActivePolicyListEntry
    {
    TPolicyHandle iPolicyHandle;
    HBufC8* iPolicyBuf;
    TBool iActiveState;       // EFalse = loaded, not active; ETrue = active
    TInt iBypassOrDropMode;   // See flags below
    TPolicyType iPolicyType;
    };
typedef CArrayFixFlat<TActivePolicyListEntry*> CActivePolicyList;

//
//
//
struct TManualAutoloadHandlePair
    {
    TPolicyHandle iManualPreloadHandle;
    TPolicyHandle iManualPostLoadHandle;
    };

//
// Policy Manager Handler Class
//
class CIPSecPolicyManagerHandler : public CBase
    {
public:
    static CIPSecPolicyManagerHandler* NewL(CIPSecPolicyManagerServer* aServer);
    void ConstructL();
    
    ~CIPSecPolicyManagerHandler();

    //
    // Policy Manager API related methods
    //
    TInt ProcessLoadPolicyL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession,
        TPolicyType aPolType);

    TInt ProcessLoadPoliciesL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt ProcessActivatePolicyL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt ProcessActivateAutoloadPolicyL(
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt ProcessUnloadPolicyL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt ProcessUnloadPoliciesL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt UnloadPolicyByHandleL(
        TUint32 aPolicyHandle,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt GetIPSecSAInfoL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);

    TInt GetLastConflictInfoL(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession);
    
#ifdef TESTFLAG
        
    TInt RequestEvent(
        const RMessage2& aMsg,
        const CIPSecPolicyManagerSession* aIPSecPolicyManagerSession); 
       
#endif 

    void ErrorHandlingL(TInt aMainCode, TInt aDetailCode);

    void ReleaseResources();

    void ParseCurrentPolicyL();

    void UpdateSelectorsAndTunnels();

    void ConvertFromObjectsToStringWithSectionsL(TInt aFunction,
                                                 TInt aBypassDropMode);

    void ConvertFromObjectsToStringWithoutSectionsL();

    void StorePolicyToActiveListL(TPolicyType aPolType,
                                  TInt aBypassDropMode);

    void ParseAllPolicyFilesL();

    void MakeUniqueSANamesL();

    void SortSelectors();

    void ConvertFromObjectsToStringL();

    void SendAlgorithmsAndPolicyToIPSecL(const TDesC& aSocket);

    void SendNullFileToIPSecL(const TDesC& aSocket);

    void ReturnPolicyFileHandleL(const RMessage2& aMsg);

    void ApiCallCompleted();

    TInt DeletePolicyFromList();

    TInt SearchPolicyFromListAndActivate();
    
    TInt GetAvailableSelectors(const RMessage2& aMsg);
    TInt GetSelectorsCount(const RMessage2& aMsg);

    //
    // Miscallenious utilities related to building policy data  
    //
    TInt WriteTunnelModeIkeNegotiationStringsL(HBufC8*& aPolBfr);

    void BuildTunnelModeIkeString(
        TDes8& aString,
        TInt aDirection,
        TInt aPort,
        TInetAddr& aGWAddr);

    TInt WriteTransportModeIkeNegotiationStrings(HBufC8*& aPolBfr);

    void BuildTransportModeIkeString(
        TDes8& aString,
        TInt aPort,
        TInetAddr& aRemote,
        TInetAddr& aRemoteMask);

    TInt BuildDhcpProtocolString(HBufC8*& aPolBfr);

    TInt BuildMip4BypassSelectors(HBufC8*& aPolBfr);

    TInt BuildComparisonWord(CSelectorList* aSelList);

    TInt SetSequenceNumbers(CSelectorList* aSelList);

    TInt CalculateMaskLength(TInetAddr& aMask);

    TInt MaskLength(TUint32 aAddr);

    TInt MaskLength(const TIp6Addr &aAddr);

    void DeleteExtraInboundOutboundSelectors();

    TInt AddInboundOutboundSelectorPair();

    void InboundOutboundSelectors();

    CPolicySelector* FindMatchingSelector();

    void FillSAInfoObject(CPolicySelector* aPolicySelector, TInt aIndex);
    
    TBool IsBypassEverythingElse(const CPolicySelector& aPolicySelector) const;

    TBool IsDropEverythingElse(const CPolicySelector& aPolicySelector) const;

    TBool IsEqualInterface(const CPolicySelector& aP1,
                           const CPolicySelector& aP2) const;

    TBool IsEqualRemoteAddress(const CPolicySelector& aP1,
                               const CPolicySelector& aP2) const;

    TBool IsEqualLocalAddress(const CPolicySelector& aP1,
                              const CPolicySelector& aP2) const;

#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
    TBool IsEqualSaSpec(CSecpolBundleItem* aS1, 
                        CSecpolBundleItem* aS2) const;
#else
    TBool IsEqualSaSpec(TSecurityAssocSpec* aS1, 
                        TSecurityAssocSpec* aS2) const;
#endif     
    
    void FillSelectorInfoObject();

    //
    // Policy conflict checking related methods
    //
    TBool CalculateCombinedPolicyBypassDropMode();
    
    void CheckSelectorConflictsL();

    void TakeNextActivePolicyL(TInt aIndex);

    void CompareSelectorsL(CPolicySelector *aPolicySelector);

    TInt CompareSAParameters(
        CPolicySelector* aPolicySelectorNew,
        CPolicySelector* aPolicySelectorOld);

    TBool CheckAddressOverlapping(
        TUint32 aNet1IpAddress,    // Net1 low address
        TUint32 aNet1Mask,         // Net1 mask
        TUint32 aNet2IpAddress,    // Net2 low address
        TUint32 aNet2Mask);        // Net2 mask

    TInt GetRangeHighAddress(
        TUint32& aNetIpAddressHigh,
        TUint32 aNetIpAddressLow,
        TUint32 aNetMask);

    void ConflictTestForPortsAndProtocolL();

    TInt ComparePortProtocol(
        CPolicySelector *aPolicySelector1,
        CPolicySelector *aPolicySelector2);

    void BuildConflictInfoL();

    // Autoload related methods
    void ReadAutoloadConfigDataL();

    // Perform autoload policies
    void AutoloadPoliciesL(
        const TZoneInfoSet& aZoneInfoSet,
        HBufC8* aPolicyBuffer,
        TAutoloadFlags aAutoloadType = EAutoloadNone);

    // Get the autoload flag
    inline TBool IsPreloadNeeded()
        {
        return iIsPreloadNeeded;
        }

    // Set autoload flag
    inline void SetAutoloadStatus(TAutoloadFlags aAutoloadFlag)
        {
        iAutoloadFlag = aAutoloadFlag;
        }

    // Check if autoload policy is active
    TBool IsAutoloadPolicyActive();

    // Set the autoload policy flag
    void SetAutoloadPolicyActive(TBool aIsAutoloadPolicyActive);

    // Store autoload policy handle
    void StorePreloadPolicyHandle();

    // Return autoload policy handle
    TUint32 GetAutoloadPolicyHandle();

    // Find an autoload policy handle based on the parent policy handle
    CAutoloadListItem* FindScopedAutoloadPolicyPair(
        TUint32 aParentPolicyHandle);

    // Add an autoload policy handle based on the parent policy handle
    void AddScopedAutoloadPolicyPairL(
        TUint32 aPreloadPolicyHandle,
        TUint32 aPostloadPolicyHandle,
        TUint32 aParentPolicyHandle);

    //delete the scoped autoload policy pair
    void DeleteScopedAutoloadPolicyPair(TUint32 aParentPolicyHandle);

    //calculate the bypass mode
    void CalculateBypassOrDropMode(TBool aForLoadPolicy);

    //determines whether the last manual autoload policy has been unloaded
    TBool IsLastManualLoadPolicy(TUint32 aPolicyHandle);

private:
    // Read the algorithms.conf file
    void ReadAlgorithmsFileL();

    void ReadNextAutoloadPolicyL(
        HBufC8*& aAutoloadPolicyBuffer,
        TPtrC& aPolicyFileName);

    void UpdatePolicySelectorScopeId(CPolicySelector& aPolicySelector,
                                     TInt aScopeId);

    void UpdatePolicyBundleScopeId(CPolicySelector& aPolicySelector,
                                   TInt aScopeId,
                                   TInt aGwScopeId,
                                   TBool& aIsTunnelMode);

    TInt CalculatePolicyBypassDropMode(CSecurityPolicy& aSp) const;

public:

    // List containing all loaded policies
    CActivePolicyList* iActivePolicyList;

    // Handle into the Socket server
    RSocketServ iSS;
    
private:
    TPolicyHandle iNextPolicyHandle;
    TPolicyHandle iCurrentPolicyHandle;

    TPckg<TIpsecSaSpec>* iPckgSAInfo;
    TIpsecSaSpec* iSAInfo;

    TInt iVPNNetId;
    TInt iGwNetId;

    TInt iFunction;

    HBufC8* iPolicyHBufC8;
    HBufC8* iPolBfr;
    HBufC8* iAlgorithmsHBufC8;

    // Info section data of active policy
    HBufC8* iLastConflictInfo;

    // A string copied from ipsecpolparser
    HBufC8* iLastParsingErrorInfo;
    
#ifdef TESTFLAG
    
    HBufC* iStringBuf;
    
    TAutoloadHandles* iHandles;
    
#endif

    TIpsecSelectorInfo* iSelectorInfo;

    // Used to parse current policy
    CIpSecurityPiece* iPieceData;

    // Used to parse active policy list entries
    CIpSecurityPiece* iPieceData2;

    // Bypass/Drop mode of the combined policy
    TInt iBypassOrDropMode;

    const CIPSecPolicyManagerSession *iSession;

    CSecpolReader* iSecpolReader6;
    
    RSocket iSock;
    
    RFs iFs;
    
    RFile iAlgFile;

    TBool iSecpolSocketOpen;
    TBool iAlgorithmsFileOpen;

    // Autoload member variables
    TBool iIsPreloadNeeded;
    TManualAutoloadHandlePair iManualAutoloadHandlePair;

    // Autoload flag
    TAutoloadFlags iAutoloadFlag;

    TBool iIsAutoloadPolicyActive;
    HBufC8* iPreloadPolicy;
    HBufC8* iBeforeManualLoadPolicy;
    HBufC8* iAfterManualLoadPolicy;
    HBufC8* iBeforeScopedLoadPolicy;
    HBufC8* iAfterScopedLoadPolicy;

    TPolicyHandle iPreloadPolicyHandle;

    // Array storing the autoload-parent
    RPointerArray<CAutoloadListItem> iScopedAutoloadPolicyPairs;
    
    // Used to retrieve the gateway address passed by client in RIpsecPolicyServ::AvailableSelectors API
    TInetAddr iTunnel;
    
    CArrayFixFlat<TIpsecSelectorInfo>* iSelectorInfoArray;
    };

#endif
