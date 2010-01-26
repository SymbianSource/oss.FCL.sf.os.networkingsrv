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
// IPSec policy server client API
// 
//



/**
 @file
 @released
 @publishedPartner
*/

#ifndef __IPSECPOLAPI_H__
#define __IPSECPOLAPI_H__

#include <in_sock.h>

/**
The prameters for Ipsec traffic selector
*/
struct TIpsecSelectorInfo
    {
    TUint iDirection;
    TInetAddr iRemote;      // including port selector, if port non-zero
    TInetAddr iRemoteMask;  // only address part used, as a mask
    TInetAddr iLocal;       // including port selector, if port non-zero
    TInetAddr iLocalMask;   // only address part used, as a mask
    TInt iProtocol;         // used, if non-zero
    TInetAddr iTunnel;      // Used to associate tunnel information with the 
                            // selector in case of tunnel mode policy
    TInt iSaIndex;          // Index into the list of selectors
                            // While retrieving the number of associated SA specs 
                            // This parameter can be used by user to ask for the 
                            // next SA spec from the policy Manager
                            // Also see TIpsecSaSpec::iMoreSasExist
    };

struct TLifeTime
    {
    TInt32 iAllocations;
    TInt64 iBytes;
    TInt64 iAddTime;
    TInt64 iUseTime;
    };

/** 
The SA transform template structure 

This is a flat data structure to enable data exchange
between client and the Policy server.

Clients use this structure for retrieving Ipsec SA transform
specifications from the Ipsec policy server. 

Note:  iMoreSasExist Field is set if the traffic selector
matches more than a single SA transform (specifying nested SAs).
It is used in conjunction with the TIpsecSelectorInfo::iIndex
parameter to retrieve the SA transforms that need to be negotitated
by the IKE daemon.
 */
struct TIpsecSaSpec
    {
	enum { KIpsecMaxIdentityLength = 100 };

    // Identity reference
    TName iName;
    // Extra parameters
    TBool   iTransportMode;             // Tunnel or transport mode
    TBool   iMoreSasExist;              // More SAs exist for selector

    // Identity reference
    TBuf8<KIpsecMaxIdentityLength> iRemoteIdentity;         
    TBuf8<KIpsecMaxIdentityLength> iLocalIdentity;          
    
    // SA selection fields
    TUint8 iType;               // SA type (AH or ESP)
    TUint8 iAalg;               // Authentication algorithm id
    TUint16 iAalgLen;           // Authentication algorithm key length
    TUint8 iEalg;               // Encryption algorithm id
    TUint16 iEalgLen;           // Encryption algorithm Key length
    TUint8 iReplayWindowLength; // Replay Window length (equal or greater)
    TUint8 iPfs:1;              // SA must have same value of PFS
    TUint8 iSrcSpecific:1;		// SA's source specific value

    // These are only used in specifying the life time requirements
    // for the acquire message and are thus preformatted to be used
    // directly as a component of the TPfkeyMessage.
    TLifeTime iHard;
    TLifeTime iSoft;
    };

/**
The currently supported zone scopes
*/
static const TInt KScopeNone    = -1;   //< Zone information is not in use
static const TInt KScopeNetwork = 16;   //< Network scope applicable to IPv4 addresses

/**
Zone information definition for passing selector and
tunnel end-point zone information in the API calls.
*/
struct TZoneInfo
    {
    TInt iScope;    //< The scope of the zone (link-local, site-local etc.)
    TInt iId;       //< The ID of the zone
    };

/** Combined zone information for source and destination */
struct TZoneInfoSet
    {
    TZoneInfo iSelectorZone;    //< The zone associated with selectors (the zone of the traffic that the selectors match)
    TZoneInfo iEndPointZone;    //< The zone associated with (tunnel) end points
    };

typedef TPckgBuf<TZoneInfoSet> TZoneInfoSetPckg;

static const TZoneInfoSet KDefaultZoneInfo = {{KScopeNone, 0}, {KScopeNone, 0}};


/**
 * Policy handle that API users get from the LoadPolicy method and need
 * to use in the UnloadPolicy method.
 */
struct TPolicyHandle
    {
    TUint32    iHandle;
    };

typedef TPckgBuf<TPolicyHandle> TPolicyHandlePckg;

static const TInt KMaxInfoSize = 1024; // The same as supported by the policy parser

/**
 * Information about a conflicting policy
 */
typedef TBuf<KMaxInfoSize> TPolicyNameInfo;

/*
 * Additional processing instructions (flags) used to guide the policy
 * loading process for IPSec policies used in with IPSec VPNs.
 * The flags can be combined with the binary OR operator.
 */
const TInt KAddIkeBypassSelectors =  (1 << 0);   //< Add IKE bypass selectors to the policy
const TInt KAddIpsecBypassSelectors =  (1 << 1); //< Add an bypass selector for traffic identified with a TIpsecSelectorInfo structure
const TInt KAddDhcpBypassSelectors =  (1 << 2);  //< Add DHCP bypass selectors to the policy
/*
 * A flag uset in GetDebugInfo call.
 *
 */
const TInt KConflictingPolicyInfo =  (1 << 0);   //< Return information about a conflicting policy
const TInt KParsingErrorInfo =  (1 << 1);        //< Return information about a policy parsing error

/**
 * IPSec Policy API methods return the following error codes
 * in the TRequestStatus parameter.
 * NOTE! The error code values below MUST be kept in sync with
 * the corresponding error code values defined together by
 * vpnapi/data/vpnerr.rss and vpnapi/data/vpnerr.ra
 */
enum TErrors
    {
    EOpenSocketError            = -5135,
    EBindSocketError            = -5136,
    EOpenAlgorithmsFileError    = -5137,
    ESecpolSocketSetOptError    = -5138,
    EUnknownPolicyHandle        = -5139,
    EParsingError               = -5140,
    EWriteSocketError           = -5141,
    ESecpolReaderError          = -5142,
    ENoSelectorFound            = -5143,
    ENoMemory                   = -5144,
    EInboundOutboundConflict    = -5145,
    ESelectorConflict           = -5146,
    ENoConflictInfoFound        = -5147,
    ENoGatewayFound				= -5148
    };

typedef TPckg<TInetAddr> TInetAddrPckg;

/**
 * RIpsecPolicyServ API is used by clients to:
 *    Users who load and unload policies
 *    KMD, that needs to find out if SA proposal can be accepted
 *
 */
class RIpsecPolicyServ : public RSessionBase
    {
public:

    IMPORT_C RIpsecPolicyServ();
	
	IMPORT_C ~RIpsecPolicyServ();

    IMPORT_C TVersion Version() const;
    
    IMPORT_C TInt Connect();
    
    /** 
     *  Loads the specified policy to the IPSec Policy Manager as such,
     *  without any modifications. The ActivatePolicy method must be
     *  called to merge the policy with other active policies and load
     *  the combined policy to the IPSec Protocol Module (where it
     *  forms the SPD).
     * 
     *  @param aPolicy
     *      a descriptor containing the Policy
     *      
     *  @param aPolicyHandle
     *      a TPckgBuf containing a TPolicyHandle
	 *
	 *	@param aStatus
	 *		On completion, will contain an error code, see the system-wide error codes.
     */
    IMPORT_C void LoadPolicy(const TDesC8& aPolicy, 
                             TPolicyHandlePckg& aPolicyHandle,
                             TRequestStatus& aStatus);

    /** 
     *  Loads the policy as a zone-specific policy to the IPSec Policy
     *  Manager. The ActivatePolicy method must be called to merge the
     *  policy with other active policies and load the combined policy
     *  to the IPSec Protocol Module (where it forms the SPD).
     *
     *  The specified selector zone ID is added to each policy selector
     *  before the policy is merged with other loaded policies. Any
     *  existing zone IDs in the policy are overwritten. The use of the
     *  selector zone IDs in policy loading allows the loading of
     *  multiple policies even with otherwise overlapping selector
     *  address spaces. The zone ID is added also to plain port and 
     *  protocol selectors that originally do not define destination
     *  addresses.
     *
     *  In addition, the method allows the caller to specify a tunnel
     *  end-point zone ID that is added to each tunnel end-point
     *  definition in the policy before the policy is merged with other
     *  loaded policies. Any existing tunnel end-point zone IDs in the
     *  policy are overwritten.
     *
     *  The use of zone IDs in the tunnel end point addresses allows the proper
     *  routing of the tunneled IP packets even in the presence of
     *  several interfaces in the system whose routing table would
     *  otherwise match a certain tunnel end-point address.
     *
     *  Finally, the method allows the user to specify additional
     *  processing instructions to be applied during the policy loading
     *  process. The following processing instructions are supported:
     *  <ul>
     *  <li>KAddIkeBypassSelectors</li>
     *  <li>KAddDhcpBypassSelectors</li>
     *  </ul>
     *  Both of these instructions are typically used in the context of
     *  VPN IPSec policies.
     *  If the KAddIkeBypassSelectors flag is defined, the IPSec Policy
     *  Manager adds IKE bypass selectors for each tunnel end-point defined
     *  in the policy before it is merged with other loaded policies.
     *  The IKE bypass selectors are needed to allow the Key Management
     *  Module (KMD) to negotiate IPSec SAs with VPN gateways during
     *  the VPN tunnel establishment phase.
     *  If the KAddDhcpBypassSelectors flag is defined, the IPSec Policy
     *  Manager adds DHCP bypass selectors to the policy before it is
     *  merged with other loaded policies. The bypass selectors are
     *  associated with the tunnel-end point zone that corresponds to
     *  the real IAP and network. The DHCP bypass selectors can be used
     *  to avoid blocking DHCP traffic to the real interface (e.g. a
     *  WLAN interface) associated with a VPN interface when a VPN IAP
     *  associated with a LAN-type IAP is activated and the related
     *  IPSec policy is loaded. The DHCP traffic must succeed so that
     *  the LAN-type interface can gets IP address and other related
     *  parameters through DHCP.
     *  
     *  @param aPolicy
     *      a descriptor containing the Policy
     *
     *  @param aSelectorZone
     *      zone information to be associated with each selector
     *      
     *  @param aProcessingFlags
     *      additional processing instructions (flags)
     *      
     *  @param aPolicyHandle
     *      a TPckgBuf containing a TPolicyHandle
	 *
	 *	@param aStatus
	 *		On completion, will contain an error code, see the system-wide error codes.
     *
     */
    IMPORT_C void LoadPolicy(const TDesC8& aPolicy,
                             TPolicyHandlePckg& aPolicyHandle,
                             TRequestStatus& aStatus,
                             const TZoneInfoSetPckg& aSelectorZones,
                             TUint aProcessingFlags = KAddIkeBypassSelectors);

    /**
     * Cancels an ongoing policy load operation.
     */     
    IMPORT_C void CancelLoad();

    /**
     * Activates the specified loaded policy. The activation causes the
     * policy to be merged with other active policies and the loading
     * of the combined policy to the IPSec Protocol Module (where it
     * forms the SPD).
	 * 
	 *  @param aPolicyHandle
     *      a descriptor containing a handle to Policy
     *
     *	@param aStatus
	 *		On completion, will contain an error code, see the system-wide error codes.
     *
     */
    IMPORT_C void ActivatePolicy(const TPolicyHandle& aPolicyHandle,
                                 TRequestStatus& aStatus);
    
    /**
     * Cancels an ongoing activate police operation.
     */     
    IMPORT_C void CancelActivate();

    /**
     * Returns information about the policy that caused policy
     * activation to fail or info about parsing error.
	 *  @param aDebugInfo
     *      a descriptor the error message returned to the user.
     *
     *  @param aInfoFlags
	 *		A combination of flags that determine the information to be returned
	 *
     */
    IMPORT_C TInt GetDebugInfo(TDes& aDebugInfo, TUint aInfoFlags = KConflictingPolicyInfo);
    
    /**
     *  - Deactivate and unload the specified policy. The method causes
     *  the remaining policies to be re-merged and loaded to the IPSec
     *  Protocol module.
     *  
     *  @param aPolicyHandle 
     *      a descriptor containing a handle to Policy
	 *
	 *	@param aStatus
	 *		On completion, will contain an error code, see the Ipsec policy and 
	 *		system wide error codes.
     *
     */
    IMPORT_C void UnloadPolicy(const TPolicyHandle& aPolicyHandle, TRequestStatus& aStatus);

    /**
     * Cancels an ongoing policy unload operation.
     */     
    IMPORT_C void CancelUnload();
    
    /**
     *  Check ISAKMP Phase2 Porposal
     *  Given the information to be matched in selector, 
     *  checks whether there is a matching selector in the active policy.
     *  This function is called multiple times in order to retrieve all 
     *  the SA specifications that are associated with a selector.
     *
     *  @param aSelector
     *      the traffic selector proposal to be matched
     *  @param aMatchingSaSpec
     *      If the selector matches, this contains the SA spec upon return
     *  @param aStatus
	 *		On completion, will contain an error code, see the Ipsec policy and 
	 *		system wide error codes.
     */ 
    IMPORT_C void MatchSelector(const TDesC8& aSelector,
                                TDes8& aMatchingSaSpec,
                                TRequestStatus& aStatus);
    
    /**
     * Cancels an ongoing match operation.
     */     
    IMPORT_C void CancelMatch();
    
    /**
     *  Given the information to be matched in gateway, 
     *  the API checks whether there is a matching gateway in the active policy.
     *  This function is called in order to retrieve all 
     *  the available selectors that are associated with the gateway.
     *
     *  @param aGateway
     *      the gateway proposal to be matched is packaged in TInetAddrPckg.
     *  @param aSelector
     *      On completion, the available selectors information in aSelectors. 
     *  @param aStatus
	 *		On completion, will contain an error code, see the Ipsec policy and 
	 *		system wide error codes.
     */ 
    IMPORT_C void AvailableSelectors(const TDesC8& aGateway, CArrayFixFlat<TIpsecSelectorInfo>* aSelectors, TRequestStatus& aStatus);
private:
	void EnumerateSelectors(const TDesC8& aGateway, TInt& aCount);

private:
	TAny* iReserverd[4];
    };

#endif // __IPSECPOLAPI_H__
