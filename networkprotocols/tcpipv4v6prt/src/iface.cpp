// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// iface.cpp - IPv6/IPv4 interface and route manager
// Implementations of flows, routes and interfaces for IPv6.
// CIp6Manager
// |iInterfaceList 
// /         iFlowList\    /       iRouteList\  |
// V    iRoute           \ V    iInterface      \V
// CIp6Flow ------------> CIp6Route -----------> CIp6Interface  ---> NIF
// |iNext                  |iNext       iAddress/|iNext
// |                       |         CIp6Address |
// |                       |         (list) |    |
// V                       V                     |
// CIp6Flow               CIp6Route                 V
// |                       |                  CIp6Interface
// ÌAddress/|
// CIp6Address
// A sketch of the Interface state transitions from StartSending/Error/Send calls
// and the resulting return values for StartSending (UP, READY). Not
// shown, but NONE is returned when StartSending is called in READY
// (unless address is changed, in which case UP is returned regardless
// of previous state).
// / Error( <0 )         / Error( <0 )      |
// |                     |                   V
// PENDING ------------> READY --------------> DOWN
// StartSending   | ^  Error( <0 )      |
// ( == UP)    | |                  /
// Send| |( == READY)     /
// return| |StartSending  /
// V |            /
// HOLD --------->
// Error (<0)
// Define WEAK_ES, if you don't want STRONG ES model for
// the host.
//



/**
 @file iface.cpp
 @verbatim
 @endverbatim
 @verbatim
 @endverbatim
*/
#undef WEAK_ES
//#define WEAK_ES

// Support for IPv6 DNS Configuration based on Router Advertisement


#define SYMBIAN_NETWORKING_UPS

//
// In Epoc R6 nifman.h has been split, CNifIfBase definition has been moved
// into <comms-infras/nifif.h>.
//
#include <e32hal.h>
#include <e32math.h>
#include <nifman.h>
#include <comms-infras/nifif.h> // ..for CNifIfBase in Epoc R6 and later

#include <in6_opt.h>
#include <in_sock.h>
#include "inet6log.h"
#include "iface.h"
#include <in_iface.h>     // IPv6 driver API specifications
#include <icmp6_hdr.h>
#include <in_chk.h>
#include <ip6_hook.h>
#include "in_flow.h"
#include <timeout.h>
#include <inet6err.h>
#include "addr46.h"
#ifdef ARP
#include <arp_hdr.h>
#endif

#include <es_ini.h>

#ifdef SYMBIAN_NETWORKING_UPS
#include "in_trans.h"
#endif

#include "tcpip_ini.h"
#include "networkinfo.h"
#include <in6_event.h>
#include <in6_dstcache.h>

#include "in6_version.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <in_sock_internal.h>
#include <in6_dstcache_internal.h>
#endif


// Temporay backward portability definition, until
// KErrLinkConfigChanged is standard from some SDK
// version forward. For now, if a special version
// is installed, define LINK_CONFIG_CHANGED in MMP
// file. -- msa
#ifdef LINK_CONFIG_CHANGED
#   include <agenterrors.h>
#else
#   define  KErrLinkConfigChanged (-3060)
#endif

#include <comms-infras/nifif_internal.h>
#include <nifman_internal.h>

//
// DAEMON_USE_PROCESSES must be set in MMP file if required by SDK. It
// determines whether daemons are to be run on threads or on real
// processes. Only force here that processes are always used when
// compiling for target device.
//

/**
* The basic timer unit.
*
* To enable compile time optimization, the unit is
* defined as preprocessor constant. The value indicates
* the fraction of the second to be used as a basic unit
* of the timer. This can be from 1 to 1000000 (from 1
* second to 1 microsecond).
*/
#define TIMER_UNIT 100
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
const TInt KRDNSSGranularity = 4; // Shall hold not more than 4 RDNSS Address
#endif //SYMBIAN_TCPIPDHCP_UPDATE

//
//lint -save -e708 stupid lint info
/**
* @name Well known multicast and other addresses of the neighbour discovery.
*
* @{
*/
/** Multicast to all receivers on this node. */
const TIp6Addr KInet6AddrNodeLocal  = {{{0xff,0x01,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}};
/** Multicast to all hosts on the link. */
const TIp6Addr KInet6AddrAllNodes   = {{{0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1}}};
/** Multicast to all routers on the link  */
const TIp6Addr KInet6AddrAllRouters = {{{0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,2}}};
/** @} */
//lint -restore

// speed optimisations
#ifdef __ARMCC__
#pragma push
#pragma arm
#endif

class TInetNdConfig
    /**
    * Neighbor Discovery (RFC-2461) Protocol Constants.
    *
    * The constants are defined as members of TInetNdConfig, because
    * in future it is possible that a link layer specific variations
    * will be defined and some control option is provided to access
    * them.
    *
    * (with some extras from RFC-2462).
    */
    {
public:
    // - router constants
    TUint iMaxInitialRtrAdvertInterval; //< seconds
    TUint iMaxInitialRtrAdvertisements; //< transmissions
    TUint iMaxFinalRtrAdvertisements;   //< transmissions
    TUint iMinDelayBetweenRas;          //< seconds
    // - host constants
    TUint iMaxRtrSolicitationDelay;     //< seconds
    TUint iRtrSolicitationInterval;     //< seconds
    TUint iMaxRouterSolicitations;      //< transmissions
    // - node constants
    TUint iMaxMulticastSolicit;         //< transmissions
    TUint iMaxUnicastSolicit;           //< transmissions
    TUint iMaxAnycastDelayTime;         //< seconds
    TUint iMaxNeighborAdvertisement;    //< transmissions
    TUint iReachableTime;               //< milliseconds
    TUint iRetransTimer;                //< milliseconds
    TUint iDelayFirstProbeTime;         //< seconds
    TReal iMinRandomFactor;             //
    TReal iMaxRandomFactor;

    // RFC-2462 additions
    TUint iDupAddrDetectTransmits;      //< transmissions

    // IPv4 (and IPv6?) (draft-ietf-zeroconf-ipv4-linklocal-05)
    TUint iMaxAddrRegenerations;        //< addresses generated
    TUint iDupAddrDefendTime;           //< seconds

    // IPv4 Linklocal Address specifications (draft-ietf-zeroconf-ipv4-linklocal-05)
    TUint iIPv4DupAddrDetectTransmits;  //< transmissions
    TUint iIPv4DupAddrAnnouncements;    //< announcements
    TUint iIPv4RetransTimer;            //< seconds (for Dup and Announce)

    // Router Reachability probing (draft-ietf-ipv6-router-selection-02.txt)
    TUint iRateLimitProbingTime;        //< seconds
    };

/** The current default values (from RFC-2461). */
const TInetNdConfig KInetNdConfig =
    {
    //- router constants
    /* MaxInitialRtrAdvertInterval */   16, // seconds
    /* MaxInitialRtrAdvertisements */   3,  // transmissions
    /* MaxFinalRtrAdvertisements */     3,  // transmissions
    /* MinDelayBetweenRas */            3,  // seconds
    // - host constants
    /* MaxRtrSolicitationDelay */       1,  // second
    /* RtrSolicitationInterval */       4,  // seconds
    /* MaxRouterSolicitations */        3,  // transmissions
    // - node constants
    /* MaxMulticastSolicit */           3,  // transmissions
    /* MaxUnicastSolicit */             3,  // transmissions
    /* MaxAnycastDelayTime */           1,  // second
    /* MaxNeighborAdvertisement */      3,  // transmissions
    /* ReachableTime */             30000,  // milliseconds
    /* RetransTimer */               1000,  // milliseconds
    /* DelayFirstProbeTime */           5,  // seconds
    /* MinRandomFactor */               0.5,
    /* MaxRandomFactor */               1.5,

    // - RFC-2462 additions
    /* DupAddrDetectTransmits */        1,  // transmissions

    // IPv4 (and IPv6?) (draft-ietf-zeroconf-ipv4-linklocal-05)
    /* MaxAddrRegenerations */          10, // max addresses generated
    /* DupAddrDefendTime */             10, // seconds

    // IPv4 Linklocal Address specifications (draft-ietf-zeroconf-ipv4-linklocal-07)
    /* IPv4DupAddrDetectTransmits */    3,  // transmissions
    /* IPv4DupAddrAnnouncements */      2,  // announcements
    /* IPv4RetransTimer */              1,  // seconds (for Dup and Announce)

    // Router Reachability probing (draft-ietf-ipv6-router-selection-02.txt)
    /* iRateLimitProbingTime */         60  // seconds
    };


/**
* @name Route Preference constants
*
* @{
*/
/** Route preference values. */
enum TRoutePreference
    {
    ERoutePreference_MEDIUM = 0,    //< Prf = 0
    ERoutePreference_HIGH   = 1,    //< Prf = 1
    ERoutePreference_INVALID= 2,    //< Prf = -0
    ERoutePreference_LOW    = 3     //< Prt = -1
    };

/** Translate TRoutePrefence value to route metric. */
const TInt KPreferenceMetric[4] =
    {
    1,  // 0 (medium) and the default for metric in all created routes.
    0,  // 1 (high)
    0,  // 2 (invalid)
    2,  // 3 (low)
    };
/** @} */

//
// TIcmpNdHeader
// *************
class TIcmpNdHeader
    /** 
    * Collection of the ICMP Messages relating to the
    * Neigbor Discovery (RFC 2461).
    */
    {
public:
    //
    // Basic
    //
    inline static TInt MinHeaderLength() {return 8; }
    inline static TInt MaxHeaderLength() {return 40; }  // Not much useful

    union
        {
        TInet6HeaderICMP iIcmp;
        TInet6HeaderICMP_RouterSol iRS;
        TInet6HeaderICMP_RouterAdv iRA;     
        TInet6HeaderICMP_NeighborSol iNS;
        TInet6HeaderICMP_NeighborAdv iNA;
        TInet6HeaderICMP_Redirect iRD;
        };
    };
//
// TIcmpNdOption
// *************
class TIcmpNdOption
    /**
    * Collection of the ICMP options relating to the
    * Neighbor Disovery (RFC 2461).
    */
    {
public:
    inline static TInt MinHeaderLength() {return 8; }
    inline static TInt MaxHeaderLength() {return 40; }  // Not much useful

    union
        {
        TInet6OptionICMP_LinkLayer iLink;
        TInet6OptionICMP_Prefix iPrefix;
        TInet6OptionICMP_Mtu iMtu;
#if 1
        // Experimental: draft-ietf-ipv6-router-selection-02.txt
        // Default Router Preferences, More-Specific Routes, and Load Sharing
        TInet6OptionICMP_RouteInformation iRouteInformation;
#endif
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
        // IPv6 DNS Configuration based on Router Advertisement: RFC-5006
        TInet6OptionICMP_DnsInformationV1 iDnsInformation;
#else
        // Experimental: draft-jeong-dnsop-ipv6-discovery-03.txt
        // IPv6 DNS Configuration based on Router Advertisement
        TInet6OptionICMP_DnsInformation iDnsInformation;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
        };
    };

// TRouteAddress
// *************
class TRouteAddress
    /**
    * Internal class to hold an address.
    *
    * Internal help class which can hold any address of the TSockAddr, but
    * does not inlude the port field or TBuf8 descriptor. Mainly required
    * to get rid of the TSockAddr constructor, which prevents it's use
    * inside union structure.
    *
    * If family is KAfInet6, then address is IPv6 or IPv4 address; otherwise
    * the address is assumed to be a link layer address.
    *
    * Only a raw addresses are handled. Port, Scope Id and Flow label are
    * not included.
    */
    {
public:
    inline TUint Family() const { return iFamily; }
    inline TPtrC8 Address() const
        /**
        * Get the raw address bytes.
        * @return descriptor for the raw address bytes.
        */
        {
        return TPtrC8(iBuf, iLength);
        }
    inline const TIp6Addr &Ip6Address() const
        /**
        * Get the raw IPv6 address.
        *
        * @return IPv6 address.
        */
        {
        return (TIp6Addr &)iBuf[0];
        }
    void SetAddress(const TIp6Addr &aAddr);
    void SetAddress(const TSockAddr &aAddr);
    void GetAddress(TSockAddr &aAddr) const;
    TBool Match(const TSockAddr& aAddr) const;
private:
    TUint iFamily;      //< Address family (0 = KAFUnspec)
    TUint iLength;      //< The length of the stored address
    // ..and enough space for any possible address used in TSockAddr
    TUint8 iBuf[KMaxSockAddrSize];//< Address bytes.
    };

void TRouteAddress::SetAddress(const TIp6Addr &aAddr)
    /**
    * Set address from raw IPv6 address.
    *
    * @param aAddr The Address.
    */
    {
    ASSERT(sizeof(iBuf) >= sizeof(TIp6Addr));
    *(TIp6Addr *)iBuf = aAddr;
    iFamily = KAfInet6;
    iLength = sizeof(TIp6Addr);
    }

void TRouteAddress::SetAddress(const TSockAddr &aAddr)
    /**
    * Set address from a TSockAddr.
    *
    * @param aAddr The Address
    */
    {
    TPtr8 ptr(iBuf, sizeof(iBuf));
    ptr = TLinkAddr::Cast(aAddr).Address();
    
    // Unfortunately, IPv6 addresses have to be treated specially,
    // the iLength must reflect the plain IPv6 address, and not the
    // TInetAddr user length. Otherwise, setting IPv6
    // from TInetAddr and TIp6Addr will not result matching
    // entries... (icky! Perhaps needs some other fix..)
    // Someone is bound to trip over this!! -- msa
    // [...to use TInetAddr Userlen() is *NOT* a solution. Comparisons
    // should only involve IPv6 address and not include scope/flow etc.]
    iFamily = aAddr.Family();
    iLength = iFamily == KAfInet6 ? sizeof(TIp6Addr) : ptr.Length();
    }

void TRouteAddress::GetAddress(TSockAddr &aAddr) const
    /**
    * Gets stored address into TSockAddr
    *
    * @retval aAddr The address.
    */
    {
    // Have to undo the trickery in SetAddress (yet another yechh!) -- msa
    if (iFamily == KAfInet6)
        TInetAddr::Cast(aAddr).SetAddress(Ip6Address());
    else
        {
        aAddr.SetFamily(iFamily);
        TLinkAddr::Cast(aAddr).SetAddress(Address());
        }
    }

TBool TRouteAddress::Match(const TSockAddr& aAddr) const
    /**
    * Tests if the stored address matches another address.
    *
    * @param aAddr Another address.
    *
    * @return ETrue, if addresses are same; otherwise EFalse.
    */
    {
    if (iFamily != aAddr.Family())
        return FALSE;
    if (iFamily == KAFUnspec)
        return TRUE;
    if (iFamily == KAfInet6)
        return Ip6Address().IsEqual(TInetAddr::Cast(aAddr).Ip6Address());
    else
        return Address() == TLinkAddr::Cast(aAddr).Address();
    }


// TSolicitedNodeAddr
// *******************
class TSolicitedNodeAddr : public TIp6Addr
    /**
    * Generates Solicited Node Multicast address.
    *
    * An class whose sole purpose is to construct an intialized
    * TIp6Address, which holds a solicited node multicast address
    */
    {
public:
    TSolicitedNodeAddr(const TIp6Addr &aAddress)
        {
        const union { TUint8 a[4]; TUint32 b; } mc_node = {{0xff, 0x02, 0, 0}};
        const union {TUint8 a[4]; TUint32 b;} one = { {0, 0, 0, 1} };
        const union {TUint8 a[4]; TUint32 b;} ff = { {0xff, 0, 0, 0} };

        u.iAddr32[0] = mc_node.b;
        u.iAddr32[1] = 0;
        u.iAddr32[2] = one.b;
        u.iAddr32[3] = ff.b | aAddress.u.iAddr32[3];
        }
    };

// Lifetime definitions
// ********************
// (values are seconds)
//
typedef TUint32 TLifetime;
const TUint32 KLifetimeForever = KMaxTUint32;

//
// The implementations of
//      CIp6Interface
//      CIp6Route
//      CIp6Flow
//      CIp6NifUser
//      CIp6Daemon
// are internal to this module and thus the class declaration do not need to
// be visible to any outsider.
//
// *NOTE*
//      The public/private/protected and friend designations are total mess
//      and should be cleaned up, if nothing else, then make all public, as
//      these classes cross reference each other too much... -- msa
//
class CIp6Flow;
class CIp6Route;
class CIp6Interface;
class CIp6NifUser;
class CIp6Daemon;
class MNifIfUser;
class CNifIfBase;
class MTimeoutManager;

//
//  CIp6Manager
//  ***********
//
class CIp6Manager : public CIfManager, public MNetworkInfo
    , public MProvdSecurityChecker
    {
    // ... lots of "friends", look into this later -- msa
    friend class CIp6Flow;
    friend class CIp6Interface;
    friend class CIp6Route;
    friend class CIp6NifUser;
    friend class CIp6ManagerTimeoutLinkage;
    //
    // Construct and InitL are only used from CIfManager::NewL()
    //
    friend class CIfManager;

    CIp6Manager();
    void InitL();
    //
    virtual ~CIp6Manager();
    TBool LoadConfigurationFile();
public:
    // Access to the configuration file (tcpip.ini)
    TBool FindVar(const TDesC &aSection,const TDesC &aVarName,TPtrC &aResult);
    TBool FindVar(const TDesC &aSection,const TDesC &aVarName,TInt &aResult);
    //
    // Implement virtual methods required by the CIfManager
    //
    inline TInt FlowCount() { return iFlows; }
    inline TInt UserCount() { return iUsers; }
    inline TInt NifCount() { return iNifCount; }

    virtual void AddRouteL(const TIp6Addr &aAddr, TInt aPrefix, const TDesC &aName,
        TUint aFlags = KRouteAdd_ONLINK, const TSockAddr *const aGateway = NULL, const TUint32 *const aLifetime = NULL);
    virtual TInt CheckRoute(const TIp6Addr &aAddr, const TUint32 aScopeid, TIp6Addr &aSrc) const; 
    virtual TUint32 LocalScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const;
    virtual TUint32 RemoteScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const;
    virtual TUint32 IsForMeAddress(const TIp6Addr &aAddr, const TUint32 aInterfaceIndex) const;
    virtual TInt IsForMePacket(RMBufRecvInfo &aInfo) const;

    virtual const MInterface* Interface(const CNifIfBase *const aIf) const;
    virtual const MInterface* Interface(const TDesC &aName) const;
    virtual const MInterface* Interface(const TUint32 aInterfaceIndex) const;

    // Get* methods are new versions of InterfaceInfo and RouteInfo.
    // Instead of iterating through by returning one
    // entry per each call, Get* methods return an array of entries in aOption buffer when
    // returning. I.e., Get* methods return an atomic snapshot of the current status.
    virtual TUint InterfaceInfo(TUint aIndex, TSoInetInterfaceInfo &aInfo) const;
    virtual TUint RouteInfo(TUint aIndex, TSoInetRouteInfo &aInfo) const;

    // Doxy descriptions for the Get*() methods can be found in MNetworkInfo definition.
    // These implement MNetworkInfo, thus Doxygen shows the same comments here
    virtual TInt GetInterfaces(TDes8& aOption) const;
    virtual TInt GetAddresses(TDes8& aOption) const;
    virtual TInt GetRoutes(TDes8& aOption) const;

      // Options processing
    virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const;
    virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption);
    TInt CheckPolicy(const TSecurityPolicy& /*aPolicy*/, const char */*aDiagnostic*/) { return KErrNone; }
    virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption, MProvdSecurityChecker &aChecker) const;
    virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption, MProvdSecurityChecker &aChecker);

    // "Users" housekeeping. 
    virtual void IncUsers();
    virtual void DecUsers();
    //
    virtual TInt PacketAccepted(const TUint32 aInterfaceIndex);

    // Flows
    virtual CFlowContext *NewFlowL(const void *aOwner, MFlowManager *aManager, TUint aProtocol);
    virtual CFlowContext *NewFlowL(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow);
    virtual TInt SetChanged() const;
    //
    //
    virtual TInt StartSending(CNifIfBase *aIface);
    virtual TInt Error(TInt aError, CNifIfBase *aIface);
    //
    // Protocol registering (iNifUsers list)
    //
    virtual MNifIfUser *Register(MNetworkServiceExtension *aProtocol);  // Makes protocol visible to interfaces
    virtual void Unregister(MNetworkServiceExtension *aProtocol);   // Removes protocol (called from protocol destructor)
    // ICMP stuff
    TInt IcmpError(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);
    TInt IcmpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);

#ifdef ARP
    virtual TInt ArpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo);
#endif
    //
    // Accessing the main components uniformly independent of the class
    // (when linkages between classes change, just change these to reflect
    // the change, and the rest of the code should work unchanged)
    //
    inline CIp6Manager &Interfacer() { return *this; };

    TInt GetDstCachePathMtu(const TIp6Addr& aDstAddress, TUint32 aScopeId) const;
    void *GetApiL(const TDesC8& aApiName, TUint* aVersion);
    
    // Returns the event manager instance used by the stack
    inline MEventService *EventManager() const { return iEventManager; }

    // Wrap a packet into ICMP error reply
    void IcmpSend(RMBufChain &aPacket, const TIcmpTypeCode aIcmp, const TUint32 aParameter = 0, const TInt aMC = 0);

private:
#   ifdef WEAK_ES
    TUint32 IsForMe(const TIp6Addr &aAddr, const CIp6Interface *const aSrcIf,
        const TUint32 aScopeId, const TScopeType aType) const;
#   else
    TUint32 IsForMe(const TIp6Addr &aAddr, const CIp6Interface *const aSrcIf) const;
#   endif
    //
    //
    CIp6Interface *FindInterface(const CNifIfBase *aInterface) const;
    CIp6Interface *FindInterface(const TAny *aId) const;
    CIp6Interface *FindInterface(const TInetAddr &aAddr) const;
    CIp6Interface *FindInterface(const TUint32 aIndex) const;
    CIp6Interface *FindInterface(const TDesC &aName) const;
    CIp6Interface *FindInterface(const TUint32 aIndex, const TScopeType aLevel) const;
    //
    // Internal Route manipulation
    //
    CIp6Route *FindRoute
        (const TIp6Addr &aDst, const TUint32 aDstId, const TUint aDstType,
         const TIp6Addr &aSrc = KInet6AddrNone, const TUint32 aSrcId = 0) const;
    void ProbeDestination
        (const TIp6Addr &aDst, const TUint32 aDstId, const TUint aDstType,
         const TIp6Addr &aSrc = KInet6AddrNone, const TUint32 aSrcId = 0) const;
    //
    // "HoldingRoute()" returns the dummy "default" route entry that gets all
    // the flows that wait for interface setup (like dialup).
    //
    CIp6Route *HoldingRoute() const { return iHoldingRoute; }
    //
    // Moving flow()s to holding route
    //
    void MoveToHolding(CIp6Flow &aFlow) const;  // Move the specific aFlow
    void MoveToHolding(CIp6Route &aRoute) const;    // Move all flows from aRoute

    //
    // ScanHoldings() scans the flows in the special holding route and
    // checks if any of them could now be assigned to a real route (and
    // does so, if yes).
    void ScanHoldings();
    //
    // Get interface by name (and create a new entry, if not found)
    //
    CIp6Interface *GetInterfaceByNameL(const TDesC &aName);
    //
    // Unconditional removal of the interface
    //
    void RemoveInterface(CIp6Interface *aIf);
    //
    // Modify Inet Interface information (SetOption part!)
    //
    TInt InetInterfaceOption(TUint aName, const TSoInet6InterfaceInfo &aInfo);
    //
    // Query Interface Information
    //
    TInt InterfaceQueryOption(TUint aName, TSoInetIfQuery &aQuery, const TInt aLength) const;
    //
    // A gateway from Set/Get Option to interface
    //
    TInt InterfaceOption(TUint aLevel, TUint aName, TDes8 &aOption) const;

    // Called when SetOption for KSoIpv4LinkLocal has been issued.
    TInt SetIpv4LinkLocalOption(const TSoInetIpv4LinkLocalInfo &aOption);
    
    //
    // Multicast Join/Leave Group processing
    //
    TInt MulticastOption(TUint aName, const TIp6Mreq &aRequest);
    //
    // Automatic Daemon control (start/stop)
    //
    void StartDaemons();
    void StopDaemons();
    void Timeout(const TTime &aStamp);  // Timer expiration event handler
    static TUint TimerUnits(const TUint aDelay, const TUint aUnit = 1);
    void SetTimer(RTimeout &aHandle, TUint32 aDelay);

    inline void SetTimerWithUnits(RTimeout &aHandle, TUint32 aDelay)
        {
        iTimeoutManager->Set(aHandle, aDelay);
        }

    //
    // Set/reset a timer event on the current object
    //
    inline void SetTimer(TUint32 aDelay) { SetTimer(iTimeout, aDelay); }
    // CancelTimer/IsTimerActive are just syntactic sugar, because
    // of the SetTimer: if one uses SetTimer and hides iTimeout, then
    // all uses of iTimeout should be "hidden" too!
    inline void CancelTimer() { iTimeout.Cancel(); }
    inline TBool IsTimerActive() { return iTimeout.IsActive(); }

    //
    // Get tcpip.ini values
    //
    TInt GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aDefault = 0, TInt aMin = 0, TInt aMax = 1);

    CIp6Route *iHoldingRoute;           //< Always Exists! The place for pending flows
    CIp6Interface *iInterfaceList;      //< All interfaces
    CIp6Daemon *iDaemons;               //< Daemons created in InitL
    TInt iLinkLocalTTL;                 //< Default TTL/Hoplimit for unicast link local destinations
    TUint8 iMaxTTL;                     //< Default TTL/Hoplimit
    TUint8 iRA_OptRoute;                //< Assigned value for KInet6OptionICMP_RouteInformation (until fixed by IANA)
#ifndef SYMBIAN_TCPIPDHCP_UPDATE     
    TUint8 iRA_OptDns;                  //< Assigned value for KInet6OptionICMP_DnsInformation (until fixed by IANA)
#endif //SYMBIAN_TCPIPDHCP_UPDATE
    // Default value for the flow iNoInterfaceError flag
    TUint iNoInterfaceError:1;
    // Default value for the flow iKeepInterfaceUp flag
    TUint iKeepInterfaceUp:1;

    // Configure IPv4 link local addresses, if non-zero.
    // Determines the default interface-specific setting in CIp6Interface
    // (see there for usable values).
    TUint iIpv4Linklocal:3;

    // Disable "ID defense mode", if non-zero
    TUint iNoDefendId:1;
    // Enable ND probing for addresses for which there is no route
    TUint iProbeAddress:1;

    // = 1, if holding queue should be scanned
    TUint iScanHolding:1;
    /**
    // Number of seconds to wait, before killing the daemons (DND etc)
    // after the last *counted* user (SAP, NIF) exits.
    */
    TUint iShutdownDelay;
    // A maximum timelimit for holding flows (seconds).
    TUint iMaxHoldingTime;
    /**
    // iMaxTickInterval is precomputed at initialize and holds the
    // longest time interval in seconds that can be expressed
    // with tick counts (when used as time stamps and compared)
    // (= number of seconds corresponding KMaxTInt ticks)
    */
    TUint iMaxTickInterval;
    // for Random sequence...
    TInt64 iSeed;
    //
    // iNifUser array is filled at init with
    // allocated objects of CIp6NifUser. A Register()
    // call from a protocol will fill in itself to
    // the appropriate slot (overriding any previous
    // register).
    //
    // All this because I don't know for sure what NIF wants,
    // but it does appear to assume that there is one-to-one
    // mapping between a protocol instance and MIfNifUser.
    // -- msa
    enum
        {
        E_IPv4 = 0,     // for a protocol supporting KAfInet
        E_IPv6 = 1,     // for a protocol supporting KAfInet6
        E_IPmax
        };
    CIp6NifUser *iNifUser[E_IPmax];

    TInt iUsers;                        //< Count of active users
    TInt iNifCount;                     //< Count of NIF references
    TInt iFlows;                        //< Count of Flow contexts
    //
    TUint iInterfaceIndex;              //< Last assigned interface index (or zero)
    TUint iRouteIndex;                  //< Last assigned route index (or zero)
    MTimeoutManager *iTimeoutManager;   //< Provide Timer Services for the Interface Manager
    CESockIniData *iConfig;             //< Configuration data
    TInt iConfigErr;                    //< Non-zero, if configuration file is not available

    MEventService *iEventManager;       //< For providing interface and route events to the plugins.
    MDestinationCache *iDestinationCache;  //< Destination cache (for transport protocol params).
    
public: // GCC doesn't compile Linkage, if this is private! -- msa
    RTimeout iTimeout;                  //< Hook to the timer service (MTimeoutManager)
    };

//
//  CIp6ManagerTimeoutLinkage
//  *************************
//  *NOTE*
//      This kludgery is all static and compile time, and only used in the constructor
//      of CIp6Interface.
//

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KIp6ManagerTimeoutOffset 104
__ASSERT_COMPILE(KIp6ManagerTimeoutOffset == _FOFF(CIp6Manager, iTimeout));
#else
#define KIp6ManagerTimeoutOffset _FOFF(CIp6Manager, iTimeout)
#endif

class CIp6ManagerTimeoutLinkage : public TimeoutLinkage<CIp6Manager, KIp6ManagerTimeoutOffset>
    /**
    * Glue to bind timeout callback from the timeout manager into Timeout() call
    * on the CIp6Route
    */
    {
public:
    static void Timeout(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
        {
        LOG(Log::Printf(_L("<>\tCIp6Manager Timeout")));
        Object(aLink)->Timeout(aNow);
        }
    };

//
//  CIp6NifUser
//  ***********
//
#ifdef _LOG
_LIT(KIPv4, "IPv4");
_LIT(KIPv6, "IPv6");
#endif

class CIp6NifUser : public CBase, public MNifIfUser
    {
    friend class CIp6Manager;
    friend class CIp6Interface;
    friend class CIp6Flow;
    friend class CIp6Route;
    CIp6NifUser(CIp6Manager &aManager, TBool aIPv4) : iManager(aManager), iIPv4(aIPv4) {}
    inline TBool IsIPv4() const {return iIPv4; }
#ifdef _LOG
    const TDesC &LogName() const { return IsIPv4() ? KIPv4() : KIPv6(); }
#endif
public:
    //
    // Interface interface
    //
    void IfUserBindFailure(TInt aResult, TAny* aId);
    void IfUserNewInterfaceL(CNifIfBase* aIf, TAny* aId);
    void IfUserInterfaceDown(TInt aResult, CNifIfBase* aIf);
    void IfUserOpenNetworkLayer();
    void IfUserCloseNetworkLayer();
    CProtocolBase* IfUserProtocol();
    TBool IfUserIsNetworkLayerActive();
    TBool IfUserIsNetworkLayerActive(CNifIfBase *);
    //
    // Accessing the main components uniformly independent of the class
    // (when linkages between classes change, just change these to reflect
    // the change, and the rest of the code should work unchanged)
    //
    inline CIp6Manager &Interfacer() const { return iManager; };
private:
    CIp6Manager &iManager;
    const TBool iIPv4;      // True for IPv4, False otherwise
    MNetworkServiceExtension *iNetwork;
    };


// ***************
// TIp6AddressInfo
// ***************
//
class CIp6Address;
class TIp6AddressInfo
    {
public:
    // Match returns TRUE, if aAddr matches the ID/hostnumber.
    TBool Match(const TIp6Addr &aAddr) const;
    
    // Match returns TRUE if aAddr ID matches exactly (prefix is
    // not used to mask address bits).
    TBool MatchExactly(const TIp6Addr &aAddr) const;

    CIp6Address *iNext;
    /**
    // iId and iPrefix define the ID/hostnumber portion of
    // the address, aligned to end of the iId field.
    //
    // iPrefix is *usually* the length of the prefix to be
    // used with this id. Technically, iPrefix is the number
    // of bits in the iId, that DO NOT BELONG to the stored id.
    // The legal values are [0..128]. *Note* Storing address
    // with iPrefix=128 will make that id part match any
    // address (id length == 0!) -- careful with it!
    //
    // This is designed for IPv6 addresses, but the processing
    // is "tweaked" so that the same code works also for IPv4
    // as follows:
    //
    // @li IPv4 loopback net (127.x.x.x) is coded as
    //   route = ELoopback, 127.0.0.0/8, address = ::/128
    //   => Address match depends only on ELoopback prefix
    //
    // @li IPv4 address
    //   route = ELoopback, ::ffff:ipv4/128,
    //   address = ::ffff:ipv4/0. IPv4 is treated as single
    //   unit (not split into prefix and id)
    //
    // @li IPv6 loopback is coded
    //    route = ELoopback, ::1/128, address= ::/128
    //    => Address match depends only on ELoopback prefix
    */
    TIp6Addr iId;       //< The Id value (aligned to the end)
    TUint8 iPrefix;     //< Number of bits to skip before id.
    //
    // Duplicate Address Detection
    //
    TUint8 iNS;         //< Number of NS sent for DAD
    /**
    // Generated address counter. If value is non-zero,
    // then this address has been randomly generated and
    // on duplicate address collision, it is legal to
    // regenerate another address. iGenerated counts
    // the number of address generations.
    */
    TUint8 iGenerated;  //< Number of address generations
    //
    // Address type
    //
    enum TAddressType
        {
        EProxy      = 2,    //< Do DAD, is not for me (forward)
        EAnycast    = 1,    //< Don't do DAD, is for me address
        ENormal     = 0,    //< Do DAD, is for me
        };
private:
    TUint iType:2;
    TBool iPrimary;
    //
    // Address state
    //
    enum TState
        {
        ENoAddress  = 0,    //< 0 0 - unassigned initial state (no address present)
        EDuplicate  = 1,    //< 0 1 - address is duplicate
        EAssigned   = 2,    //< 1 0 - address fully available
        ETentative  = 3     //< 1 1 - address is tentative (DAD in progress)
        };
    TUint iState:2;
public:
    /**
    // A flag to mark an internally generated IPv4 link-local address.
    //
    // There can only be at most one of these per interface.
    */
    TUint iIpv4LinkLocal:1;
    //
    //
    inline TInt AddressType() const { return (TInt)iType; }
    inline TInt AddressState() const { return (TInt)iState; }
    inline TBool IsSet() const { return iState != ENoAddress; };
    inline TBool IsTentative() const { return iState == ETentative; }
    inline TBool IsAssigned() const { return iState == EAssigned; }
    inline TBool IsDuplicate() const { return iState == EDuplicate; }
    inline TBool IsAnycast() const { return iType == EAnycast; }
    inline TBool IsProxy() const { return iType == EProxy; }
    inline TBool IsNormal() const { return iType == ENormal; }
    inline TBool IsPrimary() const { return iPrimary; }
    inline void SetInitial(const TInt aTentative) { iState = aTentative ? ETentative : EAssigned; }
    inline void SetDuplicate() { iState = EDuplicate; }
    inline void SetNoAddress() { iState = ENoAddress; }
    inline void SetType(const TInt aType)
        {
        iType = (TUint)aType;
        // ..anycast address is always assigned (NO DAD performed)
        if (aType == EAnycast) iState = EAssigned;
        }
    inline void SetPrimary(const TBool aPrimary ) { iPrimary = aPrimary; }
#ifdef _LOG
    const TDesC &LogAddressType() const;
    const TDesC &LogAddressState() const;
#endif
    //
    // Address Lifetimes (mainly for temporary address management)
    // (privacy extension for IPv6, RFC 3041)
    //
    TTime iCreated;     //< Creation Time (real time)
    TLifetime iVLT;     //< Valid lifetime (relative to iCRT)
    TLifetime iPLT;     //< Preferred lifetime (relative to iCRT)
    };

#ifdef _LOG
const TDesC &TIp6AddressInfo::LogAddressType() const
    {
    _LIT(KProxy,            "proxy ");
    _LIT(KAnycast,          "anycast ");
    _LIT(KNormal,           "");
    _LIT(KNormalPrimary,    "primary ");
    _LIT(KInvalid,          "invalid ");
    switch (iType)
        {
        case EProxy: return KProxy;
        case EAnycast: return KAnycast;
        case ENormal:
            {
            if( IsPrimary() )
                {
                return KNormalPrimary;
                }
            else
                {
                return KNormal;
                }
            }
        default: break;
        }
    return KInvalid;
    }

const TDesC &TIp6AddressInfo::LogAddressState() const
    {
    _LIT(KNoAddress,    "none");
    _LIT(KDuplicate,    "duplicate");
    _LIT(KAssigned,     "assigned");
    _LIT(KTentative,    "tentative");
    switch (iState)
        {
        case EDuplicate: return KDuplicate;
        case EAssigned: return KAssigned;
        case ETentative: return KTentative;
        default: break;
        }
    return KNoAddress;
    }

#endif
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC 5006 Changes
//
// CManageRdnssServerList::NewL()
// ****************************
// First Phase construction
CManageRdnssServerList* CManageRdnssServerList::NewL()
    {
    // Construct instance of type CMangeRdnssServerList
    CManageRdnssServerList* me = new (ELeave)CManageRdnssServerList();
    CleanupStack::PushL(me);
    me->ConstructL();
    CleanupStack::Pop();
    return me;
    }


// CManageRdnssServerList::ConstructL
// ****************************
// 
void CManageRdnssServerList::ConstructL()
    {
    
    }


// CManageRdnssServerList::CManageRdnssServerList
// ****************************
// Sets Creation time for RDNSS entry
CManageRdnssServerList::CManageRdnssServerList():iRdnssArrayList(KRDNSSGranularity,_FOFF(TRdnssOptionData,iRDNSSaddress)),iRdnssLifetimeArrayList(KRDNSSGranularity) // set the granularity size to 4
    {
    //Construct iRdnssArrayList of size kRDNSSGranularity=4
    //Set the initial current time stamp
    
    iCurrentTimeStamp.UniversalTime();
    }


// CManageRdnssServerList::~CManageRdnssServerList
// ****************************
// Destructor to clean up iRdnssArrayList
CManageRdnssServerList::~CManageRdnssServerList()
    {
    iRdnssArrayList.Close();
    iRdnssLifetimeArrayList.Close();
    }


// CManageRdnssServerList::InsertRdnssEntryL
// ****************************
// Inserts RDNSS entry into RDNSS server list
// Array shall hold 4 DNS entries, 
// Returns ETrue if successful, if more than 4 received returns EFalse.
TBool CManageRdnssServerList::InsertRdnssEntryL(TRdnssOptionData& aRdnssEntry, TInt aIndex)
    {
    TInt numRdnssEntry = CountRdnssEntry();
    if( numRdnssEntry < KRDNSSGranularity )// Shall hold only 4 entries
        {
        iRdnssArrayList.InsertL(aRdnssEntry, aIndex);
        return ETrue;
        }
    return EFalse;
    }


// CManageRdnssServerList::GetRdnssEntryRef
// ****************************
// Gets a reference to RDNSS entry
TRdnssOptionData& CManageRdnssServerList::GetRdnssEntryRef(TInt aIndex)
    {
    return iRdnssArrayList[aIndex];     
    }


// CManageRdnssServerList::DeleteRdnssEntry
// ****************************
// Deletes RDNSS Entry for corresponding index
void CManageRdnssServerList::DeleteRdnssEntry(TInt aRdnssArrayIndex)
    {   
    // Removes entry from the array iRdnssServerList
    iRdnssArrayList.Remove(aRdnssArrayIndex);
    }


// CManageRdnssServerList::SetStoredLifeTime
// ****************************
// Sets StoredLifetime for a RDNSS Entry
void CManageRdnssServerList::SetStoredLifeTime(TInt aIndex, TRdnssOptionData aRdnssData)
    {   
    // Sets field Lifetime at the aIndex
    iRdnssArrayList[aIndex].iStoredRdnssLifeTime = aRdnssData.iStoredRdnssLifeTime; 
    }


// CManageRdnssServerList::Elapsed
// ****************************
// Returns the elapsed time with respect to created timestamp of RDNSS Entry
TLifetime CManageRdnssServerList:: Elapsed(const TTime &aStamp)const
    {
    TTimeIntervalSeconds elapsed;
    aStamp.SecondsFrom(iCurrentTimeStamp, elapsed);
    // Return 0, if time is earlier than time stamp (clock turned back?)
    return (TLifetime) (elapsed.Int() < 0 ? 0 : elapsed.Int());
    }


// CManageRdnssServerList::PrintRdnssServerList
// ****************************
// Prints RDNSS Entries
void CManageRdnssServerList::PrintRdnssServerList(TUint8 aIndex)
    {
    TRdnssOptionData rdnssEntry = GetRdnssEntryRef(aIndex);
    TBuf<70> tmpsrc;
    TInetAddr inetAddr(rdnssEntry.iRDNSSaddress);
    TInetAddr::Cast(inetAddr).OutputWithScope(tmpsrc);      
    LOG(Log::Printf(_L("\t [Index:%d],[RDNSS =%S],[Lifetime=%d]"),aIndex, &tmpsrc,rdnssEntry.iStoredRdnssLifeTime));
    }


// CManageRdnssServerList::GetRdnssFlag
// ****************************
// Returns the RDNSS repository Flag
TInt& CManageRdnssServerList::GetRdnssFlag()
    {
    // iRdnsFlag shall be updated corresponding to each NameServer Entry.
    // iRdnsFlag shall be 0x01 for first NameServer entry.
    // iRdnsFlag shall be 0x03 for first and second NameServer entry.
    // iRdnsFlag shall be 0x04 when Namserver entries are reset to KAFUnspec.
    
    return iRdnssFlag;
    }


// CManageRdnssServerList::GetRemainingLifeTime
// ****************************
// Returns remaining life time for existing 'rdnssEntry' entry
TLifetime CManageRdnssServerList::GetRemainingLifeTime(TInt aRdnssEntryindex)
    {
    TRdnssOptionData rdnssEntry = GetRdnssEntryRef(aRdnssEntryindex);
    TRdnssLifetime elapsedLifetime = ElapsedLifeTime(rdnssEntry);
#ifdef _DEBUG
    TBuf<70> tmpsrc;
    TInetAddr inetAddr(rdnssEntry.iRDNSSaddress);
    TInetAddr::Cast(inetAddr).OutputWithScope(tmpsrc);
    LOG(Log::Printf(_L("\t [[RDNSS =%S]][Elapsed Lifetime=%d,]"),&tmpsrc,elapsedLifetime));
#endif  
    return elapsedLifetime;   
    }


// CManageRdnssServerList::ElapsedLifeTime
// ****************************
// Compute the remaining life time for existing 'i' entry
TLifetime CManageRdnssServerList::ElapsedLifeTime(TRdnssOptionData aRdnssEntry)
    {
    // If iStoredRdnssLifetime is greater than current time, ie. entry is not expired, returns difference of
    // (iStoredRdnssLifetime - curret_time).
    // If current_time is greater than iStoredRdnssLifetime, then entry is expired, returns 0.
    
    TTime stamp;
    stamp.UniversalTime();
    const TLifetime current_time = Elapsed(stamp);
                
    TLifetime elapsedLifetime = (aRdnssEntry.iStoredRdnssLifeTime > current_time)?
                                                (aRdnssEntry.iStoredRdnssLifeTime - current_time):0;
    return elapsedLifetime;
    }


// CManageRdnssServerList::RdnssEntryExists
// ****************************
// Verify RDNSS Entry Exists in RDNSS Server List
TBool CManageRdnssServerList::RdnssEntryExists(const TIp6Addr& aAddress, TInt& aArrIndex )
    {
    // Returns ETrue, with (aArrIndex)index of the (aAddress)RDNSS adress matched in iRdnssArrayList.
    // Returns EFalse, if there exists no match of (aAddress)RDNSS address in iRdnssArrayList.
    
    TRdnssOptionData rdnssEntry;
    TInt numRdnssEntry = CountRdnssEntry();
    for(TInt rdnssIndex=0;rdnssIndex<numRdnssEntry;rdnssIndex++)
        {   
        rdnssEntry = GetRdnssEntryRef(rdnssIndex);
        TInetAddr inetAddr(rdnssEntry.iRDNSSaddress);
        if(aAddress.IsEqual(TInetAddr::Cast(inetAddr).Ip6Address()))
            {
            aArrIndex = rdnssIndex;
            return ETrue;
            }
        else
            {
            continue;   
            }
        }
    return EFalse;
    }


// RdnssOrderFunc
// ****************************
// Function used to Order the List in Descending order
TInt RdnssOrderFunc( const TRdnssSortData &a, const TRdnssSortData &b)
    {
    if(a.iStoredRdnssLifeTime != b.iStoredRdnssLifeTime)
        {
        return a.iStoredRdnssLifeTime >b.iStoredRdnssLifeTime ? 1:-1;
        }
    else
        return 0;
    }


// CManageRdnssServerList::RdnssServerListSort
// ****************************
// Sorts on a temporary copied RDNSS Server List i.e iRdnssLifetimeArrayList on iStoredRdnssLifeTime Entry
void CManageRdnssServerList::RdnssServerListSort()
    {
    TLinearOrder<TRdnssSortData> order(RdnssOrderFunc);
    iRdnssLifetimeArrayList.Sort(order);
    }


// CManageRdnssServerList::RdnssServerListCopyL
// ****************************
// Copies contents from iRdnssArrayList to iRdnssLifetimeArrayList
void CManageRdnssServerList::RdnssServerListCopyL()
    {
    // Since sort on iRdnssArrayList alters the position related to NameServer entries,
    // hence we copy the lifetime values of iRdnssArrayList to iRdnssLifetimeArrayList
    // and then perform sort to determine the least lifetime entry.
    
    TInt rdnssCount = CountRdnssEntry(); // Get Number of Elements in iRdnssArrayList
    TRdnssOptionData rdnssEntry;
    TRdnssSortData lifetimeArrayList;
    
    for(TInt rdnssIndex=0;rdnssIndex<rdnssCount;rdnssIndex++)
        {
        rdnssEntry = GetRdnssEntryRef(rdnssIndex);
        // Insert index position of iRdnssArrayList[] into iRdnssLifetimeArrayList[]      
        lifetimeArrayList.iRdnssServerListIndex = rdnssIndex;
        lifetimeArrayList.iStoredRdnssLifeTime  = rdnssEntry.iStoredRdnssLifeTime;
        // Insert copied entries into iRdnssLifetimeArrayList
        iRdnssLifetimeArrayList.InsertL(lifetimeArrayList,rdnssIndex);
        }
    }


// CManageRdnssServerList::GetRdnssLifetimeEntryRef
// ****************************
// Gets a reference to Lifetime entry in  RDNSS Lifetime list
TRdnssSortData& CManageRdnssServerList::GetRdnssLifetimeEntryRef(TInt aIndex)
    {
    return iRdnssLifetimeArrayList[aIndex];      
    }


// CManageRdnssServerList::DeleteRdnssLifetimeEntry
// ****************************
// Delete Lifetime Entry for corresponding index in RDNSS Lifetime list
void CManageRdnssServerList::DeleteRdnssLifetimeEntry(TInt aRdnssArrayIndex)
    {
    // Removes entry from the array iRdnssLifetimeArrayList
    iRdnssLifetimeArrayList.Remove(aRdnssArrayIndex); 
    }


// CManageRdnssServerList::RdnssLifetimeListDelete
// ****************************
// Delete All Lifetime Entries in Rdnss Lifetime List 
void CManageRdnssServerList::RdnssLifetimeListDelete()
    {
    TInt rdnssCount;
    rdnssCount=CountRdnssLifetimeEntry();
    for(;rdnssCount!=0;rdnssCount--)
        {
        DeleteRdnssLifetimeEntry(0); // 0 - Remove all Entries in RdnssDummyArrayList        
        
#ifdef _DEBUG
        LOG(Log::Printf(_L("\t ...RdnssLifetimeListDelete.[Deleted Entry: %d]"),rdnssCount));
#endif      
        }   
    }


// CManageRdnssServerList::PrintRdnssLifetimeList
// ****************************
// Prints Lifetime Entries associated with iRdnssServerList Table 
void CManageRdnssServerList::PrintRdnssLifetimeList(TUint8 aIndex)
    {
    TRdnssSortData rdnssEntry = GetRdnssLifetimeEntryRef(aIndex);
#ifdef _DEBUG       
    LOG(Log::Printf(_L("\t [Index:%d],[RDNSSIndex =%d],[Lifetime=%d]"),aIndex, rdnssEntry.iRdnssServerListIndex,rdnssEntry.iStoredRdnssLifeTime));
#endif    
    }


// CManageRdnssServerList::RdnssExpireLeastEntry
// ****************************
// Expire an RDNSS Entry with Least Lifetime in RDNSS Server List
void CManageRdnssServerList::RdnssExpireLeastEntry(TRdnssOptionData aRdnssRcvData)
    {
    // Copy the contents from iRdnssArrayList to iRdnssLifetimeArrayList and perform sort on later
    // to find Worse Lifetime Entry corresponding to StoredLifetime. 
    
    RdnssServerListCopyL();    
    
    //Sort iRdnssLifetimeArrayList using RArray Sort.
    RdnssServerListSort();    
    
    //Get the Least Lifetime entry available at Index 0
    TRdnssSortData rdnssData = GetRdnssLifetimeEntryRef(0);    
    TInt rdnssServerListIndex = rdnssData.iRdnssServerListIndex;      
    DeleteRdnssEntry(rdnssServerListIndex);    
    LOG(Log::Printf(_L("\t Deleted an Entry and Insert Successful")));
    InsertRdnssEntryL(aRdnssRcvData, 0);
#ifdef _DEBUG    
    for(TUint8 index=0;index<CountRdnssLifetimeEntry();index++)
        {   
        PrintRdnssLifetimeList(index);      
        }    
#endif
    // Flush all the entries in iRdnssLifetimeArrayList, since its no more required
    RdnssLifetimeListDelete();
    }


// CManageRdnssServerList::RdnssServerListDelete
// ****************************
// Expire All RDNSS Entries in RDNSS Server List since router lifetime is 0
void CManageRdnssServerList::RdnssServerListDelete()
    {
    TInt rdnssCount;
    rdnssCount=CountRdnssEntry();
    for(;rdnssCount!=0;rdnssCount--)
        {
        DeleteRdnssEntry(0); // 0 , since its last entry in iRdnssServerList        
#ifdef _DEBUG
        LOG(Log::Printf(_L("\t ...RdnssServerListDelete.[Deleted Entry: %d]"),rdnssCount));
#endif      
        }   
    }


// CManageRdnssServerList::RdnssServerListSync
// ****************************
// Syncrhonises RDNSS server list 
TBool CManageRdnssServerList::RdnssServerListSync(TInetAddr& aNameSer1, TInetAddr& aNameSer2)
    {
    // Verfifies elapsed Lifetime is equal to 0 for each RDNSS Entry in iRdnssArrayList.
    // If elapsed Lifetime is zero, then delete corresponding entry in iRdnssArrayList.
    // If entry corresponds to RDNSS NameServer Repository[0 or 1], 
    // Synchronise NameServer Repository Entries in iRdnssArrayList and update iNameSer1 and iNameSer2 entries in CIp6Interface
    
    TRdnssLifetime elapsedLifetime;
    TRdnssOptionData rdnssEntry;
    TBool sendRdnssRS = EFalse;    
        
    for(TInt rdnssIndex=0;rdnssIndex<CountRdnssEntry();++rdnssIndex)
        {   
            rdnssEntry = GetRdnssEntryRef(rdnssIndex);
            elapsedLifetime = ElapsedLifeTime(rdnssEntry);
#ifdef _DEBUG
            TBuf<70> tmpsrc;
            TInetAddr inetAddr(rdnssEntry.iRDNSSaddress);
            TInetAddr::Cast(inetAddr).OutputWithScope(tmpsrc);
            LOG(Log::Printf(_L("\t [[RDNSS =%S]][Elapsed Lifetime=%d,]"),&tmpsrc,elapsedLifetime));
#endif      
            // RDNSS entry is expired since its not updated by RA. 
            if(elapsedLifetime==0)
                {
                // Verify whether expired entry is a preferred entry matching index 0 or index 1 corresponding to iNameSer1/iNameSer2,
                // If so reset the dns_flag. 
                 if((rdnssIndex == 0) || (rdnssIndex == 1)) 
                     {                                       
                     RdnssNameServerSync(rdnssIndex,aNameSer1,aNameSer2);                               
                     }
                 else
                     {
                    //Expired Entry is not a Preferred Entry, remove the corresponding entry from iRdnssArrayList[i].    
                     DeleteRdnssEntry(rdnssIndex);                                       
                     LOG(Log::Printf(_L("\t ...RDNSS Entry Deleted[Index=%d,]"),rdnssIndex));                                        
                     }              
                }
            // RDNSS entry is about to expire, need to refresh RDNSS entry by initiating a RS
            // Since CIP6Interface::Timeout handler expires for every 30 seconds.
            if(elapsedLifetime<=RDNSS_REFRESH_TIMEOUT)
                {                
                return sendRdnssRS = ETrue;
                }
        } //End of for()  
    return sendRdnssRS;
    }


// CManageRdnssServerList::RdnssNameServerUpdate
// ****************************
// Update NameServer Repository iNameSer1/iNameSer2 in CIp6Interface from RDNSS server list
void CManageRdnssServerList::RdnssNameServerUpdate(TInetAddr& aNameSer, TUint8 aNameServerIndex)
    {    
    // If RdnssArrayList exist, just update aNameSer(iNameSer1/iNameSer2 entries in CIp6Interface) with appropriate dns address.
    // If RDNSSArrayList doesn't exist, Need to reset aNameSer(iNameSer1/iNameSer2 in CIp6Interface) to KAFUnspec.
    
    if( CountRdnssEntry()>aNameServerIndex )     
        {   
        TRdnssOptionData rdnssEntry = GetRdnssEntryRef(aNameServerIndex);
        
        // aNameSerIndex for iNameSer1 ==> 1.
        // aNameServerIndex for iNameSer2 ==> 2.
        
        // If NameServer entry doesnt exist, configure iNameSer1/iNameSer2 and mark "dns_changed" respect to repository being set..
        if( (iRdnssFlag&(aNameServerIndex+1))==0) 
            {
            aNameSer.SetAddress(rdnssEntry.iRDNSSaddress.Ip6Address());
            iRdnssFlag |= (aNameServerIndex + 1);          
            }
        // If Nameserver entry exists, needs to be refreshed due to fresh update of iRdnssArrayList.
        else 
            {
            aNameSer.SetAddress(rdnssEntry.iRDNSSaddress.Ip6Address());
            }
        LOG(Log::Printf(_L("\t Updating NameServer Repository: NameSerIndex=%d, RdnssFlag=%d Lifetime=%d"),aNameServerIndex,iRdnssFlag,rdnssEntry.iStoredRdnssLifeTime));
        }
    else
        {
        // Dont reset iRdnsFlag, since there is a single dns address yet to be expired.
        // RdnssNameServerSync() shall delete repository entry, provided elapsed time is 0 and also shall reset iRdnssFlag appropriately.
        LOG(Log::Printf(_L("\t Reset NameServer Repository: NameSerIndex=%d"),aNameServerIndex));
        aNameSer.Init(KAFUnspec);        
        }
    }


// CManageRdnssServerList::RdnssNameServerReset
// ****************************
// Reset Rdnss Namserver Repository Upon expiry of lifetime to KAFUnspec.
void CManageRdnssServerList::RdnssNameServerReset(TInetAddr& aNameSer, TInt& aDdnsflag )
    {
    aNameSer.Init(KAFUnspec);
    // mark "dns changed" respect to repository being deleted.
    aDdnsflag = aDdnsflag & 4;
    }


// CManageRdnssServerList::RdnssNameServerSync
// ****************************
// Syncrhonise NameServer Repository entries in RDNSS server list.
void CManageRdnssServerList::RdnssNameServerSync(TInt aRdnssIndex, TInetAddr& aNameSer1, TInetAddr& aNameSer2)
    {
    // Since NameServer Entry is expired, remove the corresponding entry from iRdnssArrayList.
    // Reset iNameSer1/iNameSer2 to KAFUnsepc
    
    TInetAddr& nameSer = aRdnssIndex==0?aNameSer1:aNameSer2;
    RdnssNameServerReset(nameSer,GetRdnssFlag());
    DeleteRdnssEntry(aRdnssIndex);
    LOG(Log::Printf(_L("\t ...RDNSS Repository Entry Deleted[Index=%d,]"),aRdnssIndex));
    }


// CManageRdnssServerList::RdnssServerListUpdate
// ****************************
// Append RDNSS Entry in RDNSS Server List or update existing entry.
void CManageRdnssServerList::RdnssServerListUpdate(TInet6OptionICMP_DnsInformationV1 aRdnssIcmpOption, TUint8 aNumRdnssAddr)
    {
    // Find Whether Received Entry Exists
    // If found existing Entry, update stored lifetime.
    // Else create a new Entry.
    // If no room to accomodate a new Entry, find Worse Entry and delete.
    // Upon successful deletion,Store new Entry.    
    
    TRdnssOptionData rdnssRcvData;
    TInt rdnssOptionPktSize;
        
    for(TUint8 rdnssAddrCount=0; rdnssAddrCount< aNumRdnssAddr;++rdnssAddrCount) 
        {
        //Fetch the first RDNSS address from RDNSS option
        rdnssOptionPktSize = aRdnssIcmpOption.HeaderLength()+((rdnssAddrCount+1)*RDNSSADDRSIZE);
        
        const TIp6Addr &addr  = aRdnssIcmpOption.GetNextAddress(rdnssOptionPktSize-RDNSSADDRSIZE);
        const TRdnssLifetime lifetime = aRdnssIcmpOption.Lifetime();
        rdnssRcvData.iRDNSSaddress.SetAddress(addr);
        
        TInt rdnssArrayIndex;
        
        //Find this entry exist in RDNSS Server List
        if(!RdnssEntryExists(addr, rdnssArrayIndex))
            {
            //Entry doesnt exist,Try to Insert it infront of the iRdnssArrayList            
            //Before updating the lifetime, convert into seconds to determine it is expired.
            TTime stamp;
            stamp.UniversalTime();
            const TRdnssLifetime current_time = Elapsed(stamp);     
            rdnssRcvData.iStoredRdnssLifeTime = lifetime + current_time ; // (It should be current system time + lifetime ...in seconds )
            LOG(Log::Printf(_L("\t Received Entry with Lifetime: %d"),rdnssRcvData.iStoredRdnssLifeTime));          
                            
            if(!InsertRdnssEntryL(rdnssRcvData, 0))
                {   
                LOG(Log::Printf(_L("\t Insert Unsuccessful,Received More than 4 ENTRIES")));
                //If insert is unsuccessful, then try to find room for new entry.               
                RdnssExpireLeastEntry(rdnssRcvData);
                }   
            }
            else // Found an entry, need to update existing entry with suitable lifetime 
                {
                // Delete an entry , if lifetime is zero.
                if( lifetime==0 )
                    {
                    // Entry is expired, remove the corresponding entry from iRdnssArrayList[i].
                    DeleteRdnssEntry(rdnssArrayIndex);
                    continue;
                    }
                else
                //Update the entry 'i' with received lifetime, if remaininglifetime is greater than 0.
                    {
                    TTime stamp;
                    stamp.UniversalTime();
                    const TRdnssLifetime current_time = Elapsed(stamp);
                    // (It should be current system time + lifetime ...in seconds )
                    rdnssRcvData.iStoredRdnssLifeTime = lifetime + current_time;
                    SetStoredLifeTime(rdnssArrayIndex,rdnssRcvData);
                    }
                }
        }//end of For
    
    }


// CManageRdnssServerList::RdnssProcessOptionData
// ****************************
// Process received RDNSS Option Data from Router Adevertisement.
void CManageRdnssServerList::RdnssProcessOptionData(TInet6OptionICMP_DnsInformationV1 aRdnssOption, TUint8 aNumRdnssAddr )
    {
    RdnssServerListUpdate(aRdnssOption,aNumRdnssAddr);
#ifdef _DEBUG
    LOG(Log::Printf(_L("\tIF RDNSS TABLE PRINTED")));   
    for(TUint8 index =0;index<CountRdnssEntry();index++)
        {   
        PrintRdnssServerList(index);
        }
#endif  
    }


// CManageRdnssServerList::RdnssParseOptionHdr
// ****************************
// Parse received RDNSS Option Header from RA.
// Returns True, if option length is > RDNSSMINLEN, else returns False.
TBool CManageRdnssServerList::RdnssParseOptionHdr(TInet6OptionICMP_DnsInformationV1 aRdnssOption, TUint8& aNumRdnssAddr )
    {
    const TUint8 length = aRdnssOption.Length();    
    //Find the total length field since it is units of 8 octets and discard if less than RDNSSMINLEN.
    TUint8 opt_len = (length)* RDNSSOPTION_HDRLENGTH;                       
    if(opt_len < RDNSSMINLEN)
        {
        return EFalse;
        }           
    else
        {
        aNumRdnssAddr = (length-1)/2;
        return ETrue;   
        }   
    }

//RFC 5006 Changes  for RDNSS_OPTION
#endif //SYMBIAN_TCPIPDHCP_UPDATE


// ***********
// CIp6Address
// ***********
// Holds additional id's assigned to this interface, to be used for
// generated ids as described in privacy extension RFC-3041.
//
class CIp6Address : public CBase
    {
public:
    TIp6AddressInfo iInfo;
    };

// *************
// CIp6Interface
// *************

class CIp6Interface : public CBase, public MInterface
    {
    friend class CIp6Flow;
    friend class CIp6Manager;
    friend class CIp6NifUser;
    friend class CIp6Route;
    // *********
    // *WARNING*
    // *********
    // When adding members/fields into this class, do remember
    // to check the "reset information" in Reset() method,
    // if value needs to be cleared between interfaces reusing
    // this same structure! -- msa
    //
public:
    CIp6Interface(CIp6Manager &aMgr, TUint aIndex, const TDesC &aName);
    ~CIp6Interface();

    TUint32 Index() const;
    const TDesC &Name() const;
    TUint32 Scope(const TScopeType aType) const;

    void Reset(const TInt aKeepNif = 0);// set instance back to initial state.

    void Timeout(const TTime &aStamp);  // Timer expiration event handler


    CIp6Route *SelectSource(TIp6Addr &aSrc, const TIp6Addr & aDst) const;
    void SetPrefix(const TIp6Addr &aPrefix, const TUint aLength, const TInt aForce, const TLifetime aLifetime = KLifetimeForever, const TLifetime aPeferred = KLifetimeForever);

    //
    // Return the number of seconds between the given time (aStamp) and
    // the time when interface was activated (iTimeStamp). aStamp must
    // always be same of after iTimeStamp.
    TLifetime Elapsed(const TTime &aStamp) const;

    //
    // Methods for the Id part of the Ip6 addresses
    //
    void UpdateIdRoutes(const TIp6AddressInfo &aId, const TLifetime aLifetime);
    TInt SetId(TIp6AddressInfo &aId, const TIp6Addr &aAddr, const TInt aPrefix, const TInt aAddressType);
    TInt AddId(const TSockAddr& aId);   // returns 0 = not changed, 1 = changed
    TInt AddId(const TIp6Addr &aId, const TInt aPrefix, const TInt aAddressType = TIp6AddressInfo::ENormal, const TBool aForcePrimary = EFalse);    // returns 0 = not changed, 1 = changed
    TIp6AddressInfo* GetId(const TIp6Addr &aAddr) const;
    TInt RemId(const TIp6AddressInfo *const aId);
    // SetMtu sets the send MTU. Currently called from RouterAdvert handler
    // and it might be dubious thing to unconditionally change the interface
    // send Mtu this way (could perhaps constrain it by the interface reported
    // value). CHECK THIS LATER! -- msa
    void SetMtu(TInt aMtu, TInt aMin);
    // StartSending handles the StartSending from the interface
    TInt StartSending();
    inline TInt IsNetdial() const {return iName.Length() == 0; }
    inline TInt NeedsND() const { return iFeatures & KIfNeedsND; }
    // ...can send RS only if IPv6 enabled and supports multicast
    inline TInt CanSendRS() const { return iIsIPv6 && (iFeatures & KIfCanMulticast); }
    //
    // IsMyAddress returns non-NULL, if aAddr matches any of the
    // current src addresses for this interface. The returned ptr
    // indicates the ID that matched.
    //
    // Normally proxy and anycast addresses are not "my addresses", but
    // neigbour discovery needs to treat them as own, thus allow aAll != 0
    // to include them into "my address"..
    //
    TIp6AddressInfo *IsMyAddress(const TIp6Addr &aAddr, const TInt aAll = 0) const;
    // IsForMeAddress returns TRUE, if aAddr is for me (almost
    // same as IsMyAddr, but additionally returns true for multicast
    // and anycast addresses.
    TBool IsForMeAddress(const TIp6Addr &aAddr) const;
    //
    // IsMyId returns non-NULL, if aAddr matches any of the
    // id's for the interface (also tentative ones!)
    TIp6AddressInfo *IsMyId(const TIp6Addr &aAddr) const;
    //
    // IsMyPrefix returns non-NULL, if aAddr matches any of
    // the MYPREFIX entries in the route list.
    CIp6Route *IsMyPrefix(const TIp6Addr &aAddr, const TIp6AddressInfo &aId) const;
    //
    // Update flow counts (iFlows). Change can positive
    // or negative.
    //
    void UpdateFlowCount(TInt aChange);
    // Send a packet to the interface
    TInt Send(RMBufChain& aPacket, CProtocolBase* aSourceProtocol=NULL);
    TInt UpdateMulticast(const TIp6Addr &aMulticast, TLifetime const aLifetime = KLifetimeForever);
    void GetDefGateway(const TBool aIsIPv4, TInetAddr &aAddr) const;
    CIp6Route *GetRoute(const TIp6Addr &aAddr, TInt aPrefix, TUint aFlags, const TSockAddr *const aGateway = NULL, const TLifetime *const aLifetime = NULL);
    void RemoveRoute(CIp6Route *aRoute);
    void MoveToFront(CIp6Route *aRoute);    // Move the route to the first in the list
    void NotifyFlows(TInt aState, TBool aForce = EFalse) const; // External change in interface/driver
    void NotifyFlowsPmtu(const TUint aPmtu) const;  // Notify attached flows about changed Path MTU 
    TInt SetChanged(const TInt aScope = 0) const;// Set iChanged on attached flows
    CIp6Route *SelectNextHop(const TIp6Addr &aDst, const TIp6Addr &aSrc, CIp6Route *aRoute);

    CIp6Route *FindNeighbor(const TIp6Addr &aDst) const;
    CIp6Route *FindRoute(const TIp6Addr &aDst, CIp6Route *aRoute) const;

    //
    // Accessing the main components uniformly independent of the class
    // (when linkages between classes change, just change these to reflect
    // the change, and the rest of the code should work unchanged)
    //
    inline CIp6Manager &Interfacer() const { return iInterfacer; };
    //
    // Set/reset a timer event on the current object
    //
    void SetTimer(TUint32 aDelay) { Interfacer().SetTimer(iTimeout, aDelay); }
    // CancelTimer/IsTimerActive are just syntactic sugar, because
    // of the SetTimer: if one uses SetTimer and hides iTimeout, then
    // all uses of iTimeout should be "hidden" too!
    inline void CancelTimer() { iTimeout.Cancel(); }
    inline TBool IsTimerActive() { return iTimeout.IsActive(); }
    TInt HaveIp4LinkLocal();
    TBool HasIpv4LinkLocalAddr() const  { return FindIpv4LinkLocalAddr() ? ETrue : EFalse; }
    TInt SetIpv4LinkLocal(TUint aFlag);
    const TIp6AddressInfo* FindIpv4LinkLocalAddr() const;
    
    // Values given by HaveIp4LinkLocal(), equal to possible tcpip6.ini configuration settings.
    enum EV4LLEnums
        {
        EV4LLDisabled = 0,              //< Do not use IPv4 link-local addresses in any case.
        EV4LLAlways,                    //< Use IPv4 link-local address whenever possible.
        EV4LLConditional,               //< Use IPv4 link-local address if Nif does not have configured address.
        EV4LLConfigDaemonControlled,    //< Do not use IPv4 link-local address if we succeed in acquiring an IP from a server (e.g., DHCP).  If a server is not present or unavailable, automatically configure link-local address.
        EV4LLUnknown                    //< Status of IPv4 link-local setting is unknown (ini file haven't been read yet).
        };
    CIp6Route *StartProbeND(const TIp6Addr &aSrc, const TIp6Addr &aDst);

private:
    // DoBind is called when NifIfBase instance becomes available
    TInt DoBind(CIp6NifUser *aNifUser, CNifIfBase *aIf);
    TInt RandomAddress(TIp6Addr &aAddr, TUint aPrefix, TUint aN);
    void DuplicateAddress(TIp6AddressInfo *aId, TBool &aDefendIPAddress, const TBool aGratuitousArp = EFalse);
    TInt ConfigureAddress(const TIp6Addr &aAddr, const TUint aMaskLength, const TBool aForcePrimary = EFalse);
    TInt ConfigureLinkLocal(TUint32 aConfAddr);
    TIp6AddressInfo* FindInternalIpv4LinkLocalAddr();
    void UpdateNameServers(const TInetAddr &ns1, const TInetAddr &ns2, const TInt aOverride = 0);
    TInt Update4(TInt aTransition); // Try IPv4 specific setup
    TInt Update6(TInt aTransition); // Try IPv6 specific setup
    TInt SendNeighbors(TInt aMessageType, CIp6Route *aDestination, const TIp6Addr &aTarget, const TIp6Addr *const aSrc = NULL);
    TInt IcmpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, TInet6Packet<TIcmpNdHeader>  &aNd);
#ifdef ARP
    TInt ArpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, TInet6Packet<TInet6HeaderArp> &aArp);
#endif
    void Ip4RedirectHandler(const RMBufRecvPacket &aPacket, const RMBufRecvInfo &aInfo);

    void SetReachableTime();
    void SetRetransTimer();
    void RouterChanged(CIp6Route *const aRouter);
    void SetAddressAndScope(TSockAddr &aAddr, const TSockAddr &aSrc) const;

    void NotifyAddressEvent(TUint aEventType, const TIp6Addr &aPrefix,
        const TUint aLength,
        const CIp6Route *aPrefixEntry, const TIp6AddressInfo &aAddress) const;

    // Send notification about changed route to event manager
    void NotifyRouteEvent(TUint aEventType, const CIp6Route *aRoute, const TLifetime aLifetime = 0) const;

    void NotifyInterfaceEvent(TUint aEventType) const;
    
    void NotifyMulticastEvent(TUint aEventType, const TIp6Addr &aMulticast, const TLifetime aLifetime) const;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    //Do DAD for the Global address
    void PerformDADForGlobalAddress(const TIp6Addr &aPrefix,const TUint aLength);
#endif	//SYMBIAN_TCPIPDHCP_UPDATE

    CIp6Manager &iInterfacer;               //
    const TName iName;                      //< The name of the interface.
    TInt iState;                            //< Interface state: PENDING, READY, HOLD or DOWN.
    TTime iTimeStamp;                       //< Base Time Reference for address lifetimes.
    TUint iSequence;                        //< Incremented once for each address deleting event.
    TIp6AddressInfo iAddress;               //< Assigned addresses.
    CIp6Route *iRouteList;                  //< All routes.
    TInt iSMtu;                             //< Send MTU (cached value, could also just ask it always from the interface)
    TInt iRMtu;                             //< Receive MTU (cached value, could also just ask it always from the interface)
    TInt iSpeedMetric;                      //< (cached value from the interface)
    TUint iFeatures;                        //< (cached value from the interface)
    TInetAddr iNameSer1;                    //< 1. Name server address (if defined)
    TInetAddr iNameSer2;                    //< 2. Name server address (if defined)
    TInt iPMtu;                             //< Path MTU for this interface
    TUint iRouters;                         //< Current number of routers
    TUint8 iRetryRS:8;                      //< ...only used in startup for RS sols.
    TUint8 iHopLimit:8;                     //< Current Default Hoplimit on this link
    TUint iIsIPv6:1;                        //< Interface configured for IPv6, if set
    TUint iIsIPv4:1;                        //< Interface configured for IPv4, if set
    TUint iIsSuspended:1;                   //< TRUE if interface is suspended.
    // Use of link-local IPv4 addresses. 0: link-locals disabled, 1: use if no IPv4 address is read from Nif,
    // 2: always attach link-local address to interface, 3: use link-local address if no IPv4 address is read from Nif or configuration daemon
    TUint iIpv4Linklocal:3;

    /** Set IS ROUTER flag to neighbour advertisement message.
    // Note! This is a low level functionality flag, and does not have any
    // other semantics, like enabling general router functionality.
    */
    TUint iIsRouter:1;

    TInetNdConfig iND;      //< Current Neighbor Discovery parameters (base values)
    TUint iReachableTime;   //< User::TickCount units, computed from base values
    TUint iRetransTimer;    //< Timer units, computed from base values
    /**
    // Hardware address of the interface.
    // @li
    //   if iHwAddr.Family() is KAFUnspec (= 0), then link layer does not support
    //   link layer addresses
    // @li
    //   if iHwAddr.Family() is not KAFUnspec, then this contains the current
    //   link layer address of this interface (if known). Also, the Family
    //   is the assumed address family of the link layer addresses of the
    //   other nodes on this link.
    */
    TLinkAddr iHwAddr;
    CNifIfBase *iNifIf;     //< Interface instance
    /**
    // Held packets when interface has blocked (iState == EFlow_HOLD).
    // This queue should normally be ALWAYS empty. It only gets used
    // when some component of the system does not honour the "flow
    // blocking" singal (return 0 from Send).
    //
    // Currently, IP fragmenter is such component due to posthooks.
    // It would have hard time handling the situation (because it
    // cannot be sure which interface the packets actually end up!)
    */
    RMBufPktQ iHoldQueue;
    //
    // CIp6Manager Work Space
    //
    TInt iFlows;            //< Number of flows leading to this interface from routes
    CIp6Interface *iNext;   //< Interface List Link (head in CIp6Manager)
    //
    // iNifUser always points to one of the MNifIfUser instances within
    // CIp6Manager. It is initialized in when interface is created and
    // only updated in DoBind().
    // [the need for this needs to be re-examined -- msa]
    //
    CIp6NifUser *iNifUser;
    // The Scope Identifiers assigned to this interface
    TInetScopeIds iScope;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    CManageRdnssServerList *iRdnssList; //RFC-5006      
    // Global flag, used while composing global address from prefix of RA (with 'A' flag set)
    TBool iGlobalflag;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
public: // GCC doesn't compile CIp6InterfaceLinkage, if this is private! -- msa
    RTimeout iTimeout;          //< Hook to the timer service (MTimeoutManager)
    };

//
//  CIp6InterfaceTimeoutLinkage
//  ***********************
//  Glue to bind timeout callback from the timeout manager into Timeout() call
//  on the CIp6Route
//
//  *NOTE*
//      This kludgery is all static and compile time, and only used in the constructor
//      of CIp6Interface.
//

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KIp6InterfaceTimeoutOffset 696
__ASSERT_COMPILE(KIp6InterfaceTimeoutOffset == _FOFF(CIp6Interface, iTimeout));
#else
#define KIp6InterfaceTimeoutOffset _FOFF(CIp6Interface, iTimeout)
#endif

class CIp6InterfaceTimeoutLinkage : public TimeoutLinkage<CIp6Interface, KIp6InterfaceTimeoutOffset>
    {
public:
    static void Timeout(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
        {
        LOG(Log::Printf(_L("<>\tIF Timeout")));
        Object(aLink)->Timeout(aNow);
        }
    };

//
//  *********
//  CIp6Route
//  *********
//      Map address to specific gateway and interface
//

// The main type of the route is expressed with 2 bits. However, in some
// cases system may need routes that are distinguished in GetRoute as
// distinct entries, but work the same everywhere else. Only one bit
// is reserved for that purpose now... (and used by ERedirect)
const TUint KRouteAdd_EXTENSIONMASK = (1 << 2);
const TUint KRouteAdd_SHIFT = 3;

class CIp6Route : public CBase
    {
    friend class CIp6Flow;
    friend class CIp6Manager;
    friend class CIp6Interface;
    friend class CIp6NifUser;
    friend class CIp6RouteTimeoutLinkage;
public:
    CIp6Route(TUint aIndex, CIp6Manager &aMgr, const TIp6Addr &aAddr, TInt aPrefix, CIp6Interface &aInterface);
    ~CIp6Route();
    TInt Match(const TIp6Addr &aAddr) const;
    void Attach(CIp6Flow &aFlow);   //
    void Attach(CIp6Route &aRoute); // "Steal" all flows from another route
    void Detach(CIp6Flow &aFlow);
    void NotifyFlows(TInt aStatus);
    TInt SetChanged(const TInt aScope = 0) const;       // Set iChanged on attached flows

    enum TState
        {
        //
        // The first 4 states must *exactly* match the route type
        // value of the flags parameter in AddRouteL method!
        //
        // New CIp6Route can be created only into these 4 states
        //
        EIncomplete = KRouteAdd_NEIGHBOR,   //< == 0 (MUST BE ZERO)
        ELoopback = KRouteAdd_MYPREFIX,     //< == 1
        EOnlink = KRouteAdd_ONLINK,         //< == 2
        EGateway = KRouteAdd_GATEWAY,       //< == 3
        /**
        // ERedirect is a special variant of a Gateway generated by
        // the ICMP Redirects (mostly works exactly as a gateway)
        */
        ERedirect = KRouteAdd_EXTENSIONMASK | KRouteAdd_GATEWAY,
        // EAnycast is a special variant of a Loopback enty
        EAnycast = KRouteAdd_EXTENSIONMASK | KRouteAdd_MYPREFIX,
        // The remaining states are only entered from EIncomplete
        // if Neighbor discovery is applicable for the interface
        // (all of these must have the low 2 bits zero, to
        // make all of them as host routes (Type() == 0).
        //
        EReachable = 1 << KRouteAdd_SHIFT,
        EStale =  2 << KRouteAdd_SHIFT,
        EDelay = 3 << KRouteAdd_SHIFT,
        EProbe = 4 << KRouteAdd_SHIFT,

        // A unique state for the fixed HoldingRoute
        EHolding = 7 << KRouteAdd_SHIFT
        };

    TUint ExtendedType() const
        {
        return iState & (KRouteAdd_EXTENSIONMASK | KRouteAdd_TYPEMASK);
        }

    TUint Type() const
        {
        return iState & KRouteAdd_TYPEMASK;
        }
    TBool IsHoldingRoute() const
        {
        return iState == EHolding;
        }
    inline TBool IsOnlink() const
        /** Matching address(es) is/are Onlink */
        {
        return Type() == KRouteAdd_ONLINK;
        }
    inline TBool IsGateway() const
        /** Matching addresses should be sent to the gateway. */
        {
        return Type() == KRouteAdd_GATEWAY;
        }
    inline TBool IsHostRoute() const
        /** Matching address is onlink with specified link layer address. */
        {
        return Type() == KRouteAdd_NEIGHBOR;
        }
    inline TBool IsMyPrefix() const
        {
        return iState == ELoopback && !iIsMulticast; // Note: Does no match EAnycast!
        }
    void Timeout(const TInt aExpired = 0);
    //
    // Accessing the main components uniformly independent of the class
    // (when linkages between classes change, just change these to reflect
    // the change, and the rest of the code should work unchanged)
    //
    inline CIp6Manager &Interfacer() const { return iInterfacer; };
    //
    // Set/reset a timer event on the current object
    //
    void SetTimer(TUint32 aDelay) { Interfacer().SetTimer(iTimeout, aDelay); }
    // CancelTimer/IsTimerActive are just syntactic sugar, because
    // of the SetTimer: if one uses SetTimer and hides iTimeout, then
    // all uses of iTimeout should be "hidden" too!
    inline void CancelTimer() { iTimeout.Cancel(); }
    inline TBool IsTimerActive() { return iTimeout.IsActive(); }
#ifdef _LOG
    const TDesC &LogRouteType() const;
    //
    // Generate a log message from current route state
    //
    void LogRoute(const TLifetime aLifetime) const;
#endif

private:
    const TUint iIndex;         //< Assigned Route index (always > 0)
    CIp6Manager &iInterfacer;   //< The interface Manager
    //
    // The "address/prefix"
    //
    TIp6Addr iPrefix;           //< Address prefix for this route
    TUint8 iLength;             //< Length of the prefix (bits)
    TState iState:8;            //< Current State
    TUint iRetry:8;             //< Tracks various retransmissions (depend on iState)
    TUint iIsMulticast:1;       //< True if prefix is multicast address (precomputed for optim.)
    TUint iIsRouter:1;          //< = 0, no other route may point to this (iRouter)
    TUint iIsProbing:1;         //< = 1, if is probing route (not found by FindRoute)
    TInt iMetric;               //< Metric value of the route (smaller is better)
    TUint iTimeStamp;           //< Interpretation depends on iState
    RMBufChain iPacket;         //< A packet waiting for ND completion, if any.
public: // GCC doesn't compile CIp6RouteLinkage, if this is private! -- msa
    RTimeout iTimeout;          //< Hook to the timer service (MTimeoutManager)
private:
    TInt Send(RMBufChain& aPacket, CProtocolBase* aSourceProtocol=NULL, TInt aMulticastLoop = 0);
    void StartND(const TIp6Addr &aSrc);
    TInt Update(TInt aFlags, const TSockAddr *aGateway, const TLifetime *const aLifetime);
    void UpdatePrefix(const TLifetime aLifetime, const TLifetime aPreferred);
    
    void FillRouteInfo(TInetRouteInfo &rinfo, TLifetime aRefTime) const;
    void FillNeighbourInfo(TInetNeighbourInfo &nginfo, TLifetime aRefTime) const;

    struct TIp6LifetimeField
        {
        TLifetime iStored;      //< Stored Lifetime (relative to interface startup)
        TLifetime iPreferred;   //< "deprecated lifetime" in seconds
        TUint iDeprecated:1;    //< = 1, when in "deprecated state"
        TUint iCount;           //< Only used for multicast entries (for now)
    };
    
    union
        {
        //
        // Type() != ELoopback
        //
        // Note: Cannot use TSockAddr derived class, as it would generate a
        // constructor and cannot use it here (inside union) -- msa 
        //
        TRouteAddress iAddress;
        //
        // Type() == ELoopback
        //
        TIp6LifetimeField iLifetime;
        };
    CIp6Route *iRouter;             //< Link to the Router entry, when it exists
    //
    // Linking of all routes on the interface
    //
    CIp6Interface &iInterface;      //< Interface definition
    CIp6Route *iNext;               //< Route List Link (head in CIp6Interface)
    CIp6Flow *iFlowList;            //< Book keeping of flows using this route
    };

//
//  CIp6RouteTimeoutLinkage
//  ***********************
//  Glue to bind timeout callback from the timeout manager into Timeout() call
//  on the CIp6Route
//
//  *NOTE*
//      This kludgery is all static and compile time, and only used in the constructor
//      of CIp6Route following this.
//

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KIp6RouteTimeoutOffset 44
__ASSERT_COMPILE(KIp6RouteTimeoutOffset == _FOFF(CIp6Route, iTimeout));
#else
#define KIp6RouteTimeoutOffset _FOFF(CIp6Route, iTimeout)
#endif

class CIp6RouteTimeoutLinkage : public TimeoutLinkage<CIp6Route, KIp6RouteTimeoutOffset>
    {
public:
    static void Timeout(RTimeout &aLink, const TTime & /*aNow*/, TAny * /*aPtr*/)
        {
        LOG(Log::Printf(_L("<>\tROUTE Timeout")));
        Object(aLink)->Timeout(1);  // aExpired==1 to signal true expiration
        }
    };

//
//  TFlowOptions
//  ************
//  A dubious collection of fields that are mainly set
//  by set options.
//
class TFlowOptions
    {
public:
    TInt16 iHopLimit;           //< Hoplimit/TTL for non-multicast packets
    TInt16 iMulticastHops;      //< Hoplimit/TTL for multicast packets
    TUint8 iTrafficClass;       //
    TUint iDF:1;
    TUint iMulticastLoop:1;     //< 0=Don't loopback multicasts, 1=do loopback
    // The interface flow count controls the NIF OpenRoute/CloseRoute calls,
    // iKeepInterfaceUp controls whether this flow affects that count (the
    // default is 0).
    TUint iKeepInterfaceUp:1;   //< 0=Don't count, 1= count flow against inteface flow count

    // Note! Cannot use TScopeType below, because it would make the
    // bitfield into signed and fail on tests like:
    //      x.iLockType == EScopeType_NET
    // even if x.iLockType has value EScopeType_NET!!! -- msa
    /**
    * Locked scope-1 (0..15) [TScopeType].
    * Initialized from upper layer value at the beginning of the
    * Connect. Cleared after use and must be "reactivated" by
    * the hook, if it needs it. [No API this exists now]
    */
    TUint iLockType:4;
    /**
    * Current Locking Id.
    * Initialized from upper layer value at the beginning of the
    * Connect. Cleared after use and must be "reactivated" by
    * the hook, if it needs it. [No API this exists now]
    */
    TUint32 iLockId;
    };

//  *********
//  TListLink
//  *********
class TListLink
    /**
    * A base class of a simple double linked list.
    *
    * This implmentain does not require the 'offset' like TDblQueBase.
    * Also, NULL is never used, links are always non-NULL and if list
    * is empty (or element is not part of any list),
    * the links point to self.
    *
    * At this level, there is no difference between list head and
    * an element.
    */
    {
public:
    inline TListLink()
        /**
        * Constructor.
        *
        * TListLink is automaticly created as "detached", linked to self.
        */
        {
        iPrev = this;
        iNext = this;
        }
    inline ~TListLink()
        /**
        * Desctructor.
        *
        * TListLink(s) can be declared as a member of any class
        * and there is no need to worry about instance being a
        * member of lists when it is destroyed. This destructor
        * automaticly removes the instance from a list if inserted.
        */
        {
        Detach();
        }
    inline void MoveTo(TListLink &aList)
        /**
        * Move element to another list.
        *
        * MoveTo removes this element from previous list (if any)
        * and inserts it to a new list, in front of the specified
        * element (aList).
        */
        {
        // Does not work if moving to self!
        ASSERT(this != &aList);
        if (this == &aList)
            return;

        // Remove element from the old queue
        iPrev->iNext = iNext;
        iNext->iPrev = iPrev;
        // Add to aList
        iNext = &aList;
        iPrev = aList.iPrev;
        iPrev->iNext = this;
        iNext->iPrev = this;
        }

    inline void Detach()
        /** Detach this element from a list (if any). */
        {
        iPrev->iNext = iNext;
        iNext->iPrev = iPrev;
        iNext = this;
        iPrev = this;
        }

protected:
    TListLink *iPrev;
    TListLink *iNext;
    };


//  ********
//  CIp6Flow
//  ********
//
class TFlowNotifyList;
class CIp6Flow : public CFlowInternalContext
    {
    friend class CIp6Manager;
    friend class CIp6Route;
    friend class CIp6Interface;
    friend class CIp6NifUser;
    friend class TFlowNotifyList;
public:
    CIp6Flow(const void *aOwner, MFlowManager *aManager, CIp6Manager &aInterfacer, TUint aProtocol);
    CIp6Flow(const void *aOwner, MFlowManager *aManager, CIp6Manager &aInterfacer, CFlowContext &aFlow);
    virtual ~CIp6Flow();
    virtual MInterfaceManager *Interfacer() const
        { return &iInterfacer; }
    virtual CNifIfBase *Interface() const;
    virtual TInt Send(RMBufChain& aPacket, CProtocolBase* aSourceProtocol=NULL);
    virtual void RefreshFlow();
    virtual void Connect();
    virtual TInt RouteFlow(TPacketHead &aHead);
    virtual void Disconnect();
    virtual TInt InterfaceSMtu() const;
    virtual TInt InterfaceRMtu() const;
    virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const;
    virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption);

    void Notify(TFlowNotifyList &aList, const TInt aState);
    TInt SetChanged(const TInt aScope = 0); // Set iChanged
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    virtual TBool IsNdPacketPendingResolution();//RFC 4861 Changes
#endif //SYMBIAN_TCPIPDHCP_UPDATE
private:
    TInt VerifyAddress(const TPacketHead &aHead, const CIp6Interface &aIf) const;
    void SelectNextHop();

#ifdef SYMBIAN_NETWORKING_UPS
    TBool UPSPromptingPossible();
    TBool ApplyStaticSecurityCheck();
#endif
    CIp6Manager &iInterfacer;       // (i) Interface Manager
    CIp6Route *iRoute;              // (i) Attached route
    CIp6Flow *iNext;                // (i) Other flows on same route
    TFlowOptions iOptions;          // (m)
    TUint iSequence;                // (-) Used to detect expired source address.
    TUint iTimeStamp;               // (-) Currently, only used when in HoldingRoute
    TListLink iNotifyList;
#ifdef SYMBIAN_NETWORKING_UPS
    TBool iUpsAuthorisationRequired;
    TBool iUpsAuthorisationPending;
#endif
    };


#ifdef _LOG

//
// Internal utility to return a symbolic scope level name
// (only for DEBUG compile)
static const TDesC &LogScopeName(TInt aScope)
    {
    _LIT(KScope_0, "IF");       // 1 interface (node local)
    _LIT(KScope_1, "IAP");      // 2 link local
    _LIT(KScope_2, "SC3");      // 3
    _LIT(KScope_3, "SC4");      // 4
    _LIT(KScope_4, "SITE");     // 5 site local
    _LIT(KScope_5, "SC6");      // 6
    _LIT(KScope_6, "SC7");      // 7
    _LIT(KScope_7, "ORG");      // 8 organization
    _LIT(KScope_8, "SC9");      // 9
    _LIT(KScope_9, "SC10");     // 10
    _LIT(KScope_10, "SC11");    // 11
    _LIT(KScope_11, "SC12");    // 12
    _LIT(KScope_12, "SC13");    // 13
    _LIT(KScope_13, "GBL");     // 14 global
    _LIT(KScope_14, "SC15");    // 15
    _LIT(KScope_15, "NET");     // -- network

    // Some tricky type casting is required to get rid of the
    // writable static data problem at target linking...
#   define CAST(x) &reinterpret_cast<const TDesC &>(x)
    static const TDesC *const map[] = 
        {
        CAST(KScope_0), CAST(KScope_1), CAST(KScope_2), CAST(KScope_3),
        CAST(KScope_4), CAST(KScope_5), CAST(KScope_6), CAST(KScope_7),
        CAST(KScope_8), CAST(KScope_9), CAST(KScope_10), CAST(KScope_11),
        CAST(KScope_12), CAST(KScope_13), CAST(KScope_14), CAST(KScope_15)
        };
#   undef CAST


    return *map[aScope & 0xF];
    }


//
// Internal utility for formatting address/prefix. Only for DEBUG compile
//
class TLogAddressPrefix : public TBuf<70>
    {
public:
    TLogAddressPrefix() {}
    /** Format plain address%scope. */
    TLogAddressPrefix(const TIp6Addr &aAddr);
    /** Format address%scope/prefix. */
    TLogAddressPrefix(const TIp6Addr &aAddr, const TInt aPrefix);
    /** Format address%scope#port */
    TLogAddressPrefix(const TInetAddr &aAddr);
    /** Format plain address%scope. */
    void Set(const TIp6Addr &aAddr);
    /** Format address%scope/prefix. */
    void Set(const TIp6Addr &aAddr, const TInt aPrefix);
    /** Format address%scope#port */
    void Set(const TInetAddr &aAddr);
    };

TLogAddressPrefix::TLogAddressPrefix(const TIp6Addr &aAddr)
    {
    Set(aAddr);
    }

TLogAddressPrefix::TLogAddressPrefix(const TIp6Addr &aAddr, const TInt aPrefix)
    {
    Set(aAddr, aPrefix);
    }

TLogAddressPrefix::TLogAddressPrefix(const TInetAddr &aAddr)
    {
    Set(aAddr);
    }

void TLogAddressPrefix::Set(const TIp6Addr &aAddr)
    {
    const TInetAddr addr(aAddr, 0);
    addr.OutputWithScope(*this);
    }

void TLogAddressPrefix::Set(const TIp6Addr &aAddr, const TInt aPrefix)
    {
    Set(aAddr);
    _LIT(KFormat, "/%d");
    // Note: the overflow check is omitted on purpose (because
    // leaving information out silently would cause more confusion).
    // The supplied buffer should always be sufficient.
    AppendFormat(KFormat, aPrefix - (aAddr.IsV4Mapped() ? 96 : 0));
    }

void TLogAddressPrefix::Set(const TInetAddr &aAddr)
    {
    if (aAddr.Family() == KAfInet || aAddr.Family() == KAfInet6 || aAddr.Family() == KAFUnspec)
        aAddr.OutputWithScope(*this);
    else
        {
        // Assume some type of link layer address, dump octets as xx:xx:...
        SetLength(0);
        const TPtrC8 ptr = TLinkAddr::Cast(aAddr).Address();
        const TInt N = ptr.Length();
        if (N > 0)
            {
            AppendNum((TInt)ptr[0], EHex);
            for (TInt i = 1; i < N; ++i)
                {
                Append(':');
                AppendNum((TInt)ptr[i], EHex);
                }
            }
        }
    if (aAddr.Port())
        {
        _LIT(KFormat, "#%d");
        AppendFormat(KFormat, aAddr.Port());
        }
    }

void PktLog(const TDesC &aFormat, const RMBufPktInfo &aInfo, TUint aIndex, const TDesC &aName)
    {
    TLogAddressPrefix src(TInetAddr::Cast(aInfo.iSrcAddr));
    TLogAddressPrefix dst(TInetAddr::Cast(aInfo.iDstAddr));
    Log::Printf(aFormat, aIndex, &aName, aInfo.iProtocol, &src, &dst, aInfo.iLength);
    }

#endif


class TFlowNotifyList : public TListLink
    {
public:
    void Insert(CIp6Flow &aFlow);
    void Deliver(TInt aStatus);
    };


void TFlowNotifyList::Insert(CIp6Flow &aFlow)
    {
    aFlow.iNotifyList.MoveTo(*this);
    }

void TFlowNotifyList::Deliver(TInt aStatus)
    {
    TListLink *p;
    while ((p = iNext) != this)
        {
        p->Detach();
        CIp6Flow *const f = (CIp6Flow *)((TUint8 *)p - _FOFF(CIp6Flow, iNotifyList));
#ifdef _LOG
            {
            TLogAddressPrefix src(f->iInfo.iLocal);
            TLogAddressPrefix dst(f->iInfo.iRemote);
            Log::Printf(_L("\t\tFlow[%u] Deliver(%d -> %d) prot=%d src=[%S], dst=[%S]"),
                (TInt)f, aStatus, f->iStatus, (TInt)f->iInfo.iProtocol, &src, &dst);
            }
#endif
        f->SetStatus(aStatus);
        }
    }



//
// Internal help utility to retrieve tick period as unsigned int
//
static TUint TickPeriod()
    {
    TTimeIntervalMicroSeconds32 period;
    UserHal::TickPeriod(period);
    return (TUint)period.Int();
    }

static TUint ElapsedUnits(const TTime &aMark, const TTime &aLater)
    /**
    * Internal help utitility to compute the difference between two timestamps.
    *
    * @param aMark  The earlier time stamp
    * @param aLater The later time stamp
    *
    * @return
    *   (aLater - aMark) in timer units, or KMaxTUint if
    *   the value does not fit in TUint.
    */
    {
    const TInt64 elapsed = aLater.MicroSecondsFrom(aMark).Int64() / (1000000 / TIMER_UNIT);
#ifdef I64HIGH
    return I64HIGH(elapsed) != 0 ? KMaxTUint : I64LOW(elapsed);
#else
    return elapsed.High() != 0 ? KMaxTUint : elapsed.Low();
#endif
    }


// **********
// CIp6Daemon
// **********
class CIp6Daemon : public CBase
    /**
    * Keep track of active daemons related to the protocol stack
    */
    {
public:
    ~CIp6Daemon();
    void Start(const TDesC &aProcessName, const TDesC &aFileName);
    void Kill();
    CIp6Daemon *iNext;
    RProcess iProcess;
    TUint iStarted:1;   //< =1, if handle is attached to another process/thread
    };

// ***********
// ~CIp6Daemon
// ***********
CIp6Daemon::~CIp6Daemon()
    /** Also automaticly kills the attached process, if any running. */
    {
    Kill();
    }

// ****************
// CIp6Daemon::Kill
// ****************
void CIp6Daemon::Kill()
    /** Kill thread/process, if running and disconnect handle from the thread/process. */
    {
    if (iStarted)
        {
        iStarted = 0;
        if (iProcess.ExitType() == EExitPending)
            iProcess.Kill(KErrServerTerminated);
        iProcess.Close();
        }
    }

// *****************
// CIp6Daemon::Start
// *****************
//
void CIp6Daemon::Start(const TDesC &aProcessName, const TDesC &aFileName)
    /** Start the named EXE as a daemon. */
    {
    ASSERT(!iStarted);

    TInt res = iProcess.Create(aFileName, _L(""));
    if (res == KErrNone)
        {
        iStarted = 1;
        iProcess.Resume();
        }

#ifdef _LOG
    if (iStarted)
        Log::Printf(_L("CIp6Daemon::Start(%S, %S) OK"), &aProcessName, &aFileName);
    else
        Log::Printf(_L("CIp6Daemon::Start(%S, %S) *** FAILED *** with error %d"), &aProcessName, &aFileName, res);
#else
    (void)aProcessName; // prevent warning message
#endif
    }

// TIp6AddressInfo::Match
// **********************
TBool TIp6AddressInfo::Match(const TIp6Addr &aAddr) const
    /**
    * Match ID part.
    *
    * @param aAddr The id to compare with.
    *
    * Match returns TRUE, if aAddr matches the
    * ID/hostnumber (tailored after similar code in
    * TIp6Addr::Match() )
    */
    {
    ASSERT(iPrefix <= 128);
    if (iPrefix > 127)  // (actually, == 128)
        return TRUE;

    TInt i = 3;
    while (iId.u.iAddr32[i] == aAddr.u.iAddr32[i])
        if (i == 0)
            return TRUE;    // Obviously true, regardless of the iPrefix
        else
            --i;
    // Optimize for 64 bit id?
    // if (iPrefix >= 64 && i < 2) return TRUE;

    i = i * 2 + 1;
    if (iId.u.iAddr16[i] == aAddr.u.iAddr16[i])
        --i;

    i = i * 2 + 1;
    if (iId.u.iAddr8[i] == aAddr.u.iAddr8[i])
        --i;

    // i = index of the byte containing a difference, the
    // number of unmatched bits is (i+1) * 8 - "matched bits
    // in the current byte". ...count them below
    //
    TUint8 diff = (TUint8)(iId.u.iAddr8[i] ^ aAddr.u.iAddr8[i]);
    for (i = (i + 1) << 3; !(diff & 0x1); diff >>= 1)
        --i;
    // Matched full id part?
    return iPrefix >= i;
    }

// TIp6AddressInfo::MatchExactly
// **********************
TBool TIp6AddressInfo::MatchExactly(const TIp6Addr &aAddr) const
    /**
    * Match ID part only.
    *
    * @param aAddr The id to compare with.
    *
    * Match returns TRUE, if aAddr matches the
    * ID/hostnumber (tailored after similar code in
    * TIp6Addr::Match() )
    */
    {
    ASSERT(iPrefix <= 128);

    // If any address bits are ignored, return false as the address
     // cannot be an exact match.
     if( iPrefix != 0 )
         {
         return EFalse;
         }
  
    // Determine the size of the address.
    TUint addrBits = sizeof( iId.u )/ sizeof( TUint );
    TUint addrWords = addrBits;
    if( iId.IsV4Mapped() )
        {
        addrWords = 1;
        }
    
        
    // Check every address word.
    for( TUint i = ( addrBits ) - addrWords; i < addrBits; ++i )
        {
        if( aAddr.u.iAddr32[i] != iId.u.iAddr32[i] )
            {
            return EFalse;
            }
        }
        
    return ETrue;
    }


// **********************************************
// CIp6Flow, the flow termination implementations
// **********************************************

// 
// Interaface
// **********
// Return connected interface
CNifIfBase *CIp6Flow::Interface() const
    {
    //
    // Is this flow actually connected?
    //
    if (iRoute)
        return iRoute->iInterface.iNifIf;
    else
        return NULL;
    }


//
//  CIp6Flow::CIp6Flow
//  ******************
//  *BEWARE*
//      iManager link is set here, but there is no path from
//      the manager to this until it is connected (after which
//      it can be found via routes). If manager is destroyed,
//      it is possible to have dangling pointers from
//      unconnected flows!
//
//      However, the current assumption is that all flows are
//      allocated within this same protocol library and all
//      them must have been deleted before the family object
//      and the associated CIp6Manager gets deleted.
//
//      The iFlows count in the CIp6Manager is maintained only
//      to catch programming errors (and a Panic is issued).

CIp6Flow::CIp6Flow(const void *aOwner, MFlowManager *aManager, CIp6Manager &aInterfacer, TUint aProtocol)
: CFlowInternalContext(aOwner, aManager), iInterfacer(aInterfacer)
    {
    iInterfacer.iFlows++;
    iInfo.iProtocol = (TUint8)aProtocol;
    // Init with default "interface error handling" policy
    iInfo.iNoInterfaceError = iInterfacer.iNoInterfaceError;
    // By default, lock flows to any network
    iInfo.iLockId = 0;
    iInfo.iLockType = EScopeType_NET;
    // Init default for flow counting on interfaces policy
    iOptions.iKeepInterfaceUp = iInterfacer.iKeepInterfaceUp;
    //
    // Initialize non-zero defaults for options
    //
    iOptions.iHopLimit = -1;
    iOptions.iMulticastHops = -1;
    iOptions.iMulticastLoop = 1;
    LOG(Log::Printf(_L("\t\tFlow[%u] New: protocol=%d (%d flows now)"), this, (TInt)aProtocol, iInterfacer.iFlows));
    }

CIp6Flow::CIp6Flow(const void *aOwner, MFlowManager *aManager, CIp6Manager &aInterfacer, CFlowContext &aFlow)
: CFlowInternalContext(aOwner, aManager, aFlow), iInterfacer(aInterfacer)
    {
    iInterfacer.iFlows++;
    iInfo.iProtocol = (TUint8)aFlow.Protocol();
    // Assumes below that the aFlow is also CIp6Flow!
    iOptions = ((CIp6Flow &)aFlow).iOptions;
    LOG(Log::Printf(_L("\t\tFlow[%u] New: protocol=%d cloning from Flow[%u] (%d flows now)"),
        this, (TInt)iInfo.iProtocol, (TInt)&aFlow, iInterfacer.iFlows));
    }

CIp6Flow::~CIp6Flow()
    {
    //
    // Detach flow from a route if connected
    //
    if (iRoute)
        iRoute->Detach(*this);
    ASSERT(iInterfacer.iFlows > 0);
    iInterfacer.iFlows--;
    LOG(Log::Printf(_L("\t\tFlow[%u] Deleted (%d flows remaining)"), this, iInterfacer.iFlows));
    }


// CIp6Flow::Notify
// ****************
//
// *NOTE* The assumption here is that if the aState is negative, it
//        the error state of the interface (thus the test for
//        iNoInterfaceError is correct).
//
void CIp6Flow::Notify(TFlowNotifyList &aList, const TInt aState)
    {
    if (aState >= 0 || iInfo.iNoInterfaceError == 0)
        aList.Insert(*this);
    }

// CIp6Flow::SetChanged
// ********************
TInt CIp6Flow::SetChanged(const TInt aScope)
    {
    if (aScope > 0)
        return iRoute ? iRoute->SetChanged(aScope-1) : 0;
    iChanged = 1;
    // *NOTE* It might be convenient to call SetStatus(EFlow_READY),
    // if iState is PENDING (> 0) to wake up the SAP. However, this
    // might cause problems, because SetStatus() may cause a destruction
    // of the flow context (and possibly other structures), and if this
    // is called from CIp6Route instance or higher, then all traversing
    // loops would need to be protected against destruction of any object
    // on the list while processing the list... -- msa
    return 1;
    }

//
//  CIp6Flow::Send
//  **************
TInt CIp6Flow::Send(RMBufChain& aPacket, CProtocolBase* aSrc)
    /**
    * Send a packet to the attached interface.
    *
    * @param aPacket    The packet.
    * @param aSrc       The source (mostly ignored).
    *
    * @return
    *   @li < 0, no interface of some other missing component
    *   @li = 0, packet sent, but interface does not want more after this
    *   @li = 1, packet sent, and interface is willing to accept more
    *
    *   The packet "ownership" is always transfered, regardless of the return type
    *   (the aPacket should be empty after this!)
    *
    *   On entry, the packet must have a info structure of RMBufSendInfo.
    *   This routine will release the flow handle, if any attached.
    */
    {
    RFlowContext flow;
    TInt ret = KErrNotReady;

    RMBufSendInfo *const info = RMBufSendPacket::PeekInfoInChain(aPacket);
    if (info)
        {
        flow.Grab(info->iFlow);
        ASSERT(flow.FlowContext() == this);
        if (iRoute)
            {
            if (iSequence != iRoute->iInterface.iSequence)
                {
                // The valid address list has been changed since
                // the last packet, verify that the current source
                // address is still legal.
                // *NOTE* after ReadyL processing the iHead represents
                // the "upper layer view" of the addresses (for example,
                // there might be home addresses loaded with mobile-ip).
                // However, here the test need to be done on the ultimate
                // final addresses being used for each packet, and those
                // *SHOULD* *ALWAYS* be in the iStart, which is saved
                // after OpenL() phase! The test here must be done to
                // the final address!!! -- msa
                //
                ret = VerifyAddress(iStart, iRoute->iInterface);
                if (ret != KErrNone)
                    {
                    // Invalid address, shutdown the flow
                    SetStatus(ret);
                    goto drop_out;
                    }
                // Prevent further tests until next address expiration
                iSequence = iRoute->iInterface.iSequence;
                }
                            
            // Because the flow has been detached from the packet, this
            // is the last point where we can set the correct value for
            // the KIpKeepInterfaceUp bit (which controls the counters
            // for the NIF shutdown).
            if (iOptions.iKeepInterfaceUp)
                info->iFlags |= KIpKeepInterfaceUp;     // Set
            else
                info->iFlags &= ~KIpKeepInterfaceUp;    // Clear
            ret = iRoute->Send(aPacket, aSrc, iOptions.iMulticastLoop);
            }
drop_out:
        flow.Close();   // <-- May delete THIS? (Probably, if above KErrInet6AddressExpired occurred!)
        }
    aPacket.Free();     // NOOP, if already done or assigned to elsewhere
    return ret;
    }

TInt CIp6Flow::VerifyAddress(const TPacketHead &aHead, const CIp6Interface &aIf) const
    /**
    * Verify that the current source address is valid for the interface.
    *
    * For forwarding flows only, the source address scope is verified. The forwarding
    * flows are allowed to use "invalid source addresses". The source address of other
    * flows must be either unspecified address or a configured and assigned address on
    * the interface.
    *
    * @return
    *   @li KErrNone, if valid
    *   @li KErrInet6SourceAddress, if address is out of scope
    *   @li KErrInet6AddressExpired, if address is missing.
    */
    {
    const TIp6Addr &src = aHead.ip6.SrcAddr();

    //
    // Verify the source address in a aHead
    //
    const TUint scope = (TUint)(src.Scope() - 1);
    if (!TIp46Addr::Cast(src).IsUnspecified())
        {
        if (scope > EScopeType_NET || aHead.iSrcId != aIf.iScope[scope])
            {
            // Trying to use out of scope source address
            return KErrInet6SourceAddress;
            }
#ifndef WEAK_ES
        // In strong ES model, don't allow sending packets
        // with wrong source address (except unspecified address
        // in some rare cases, and if we are forwarding data).
        if (iInfo.iForwardingFlow == 0 && aIf.IsMyAddress(src) == NULL)
            {
            return KErrInet6AddressExpired;
            }
#endif
        }
    return KErrNone;
    }

//
// CIp6Flow::RefreshFlow
// *********************
void CIp6Flow::RefreshFlow()
    /**
    * Recompute the current flow status.
    *
    * This function is called when a flow in needed and the status is > 0 (pending or hold).
    * This checks whether the flow can be changed into ready state, and it (re)runs the
    * MIp6Hook::ReadyL phase for the hooks.
    */
    {
    if (iChanged)
        {
        LOG(Log::Printf(_L("\t\tFlow[%u] Changed, reconnect required"), this));
        return;
        }
    if (!iRoute)
        {
        iStatus = KErrNotFound;     // No route available/attached!
        LOG(Log::Printf(_L("\t\tFlow[%u] No route"), this));
        return;
        }

    CIp6Interface &iface = iRoute->iInterface;
    if (iface.iState != EFlow_READY)
        {
        LOG(Log::Printf(_L("\t\tFlow[%u] %S is not ready (%d)"), this, &iface.iName, iface.iState));
        iStatus = iface.iState == EFlow_HOLD ? EFlow_HOLD : EFlow_PENDING;
        return;
        }

    Reset();
    const TIp6Addr &dst = iHead.ip6.DstAddr();
    // For log prints, have destination address as a string.
    LOG(TLogAddressPrefix log_dst(dst));
    ASSERT(iHead.iSourceSet);
    iStatus = VerifyAddress(iHead, iface);
    if (iStatus < 0)
        return;
    iSequence = iRoute->iInterface.iSequence;   // Address in synch for now!

    iTimeStamp = 0; // [for why, look at comments in MoveToHolding!! -- msa]
    //
    // This destination will be needed, thus activate ND, if destination is not yet
    // known! (note: at this point the ultimate src address of the packet is already
    // known.
    CIp6Route *const host_route = iRoute->iRouter ? iRoute->iRouter : iRoute;
    if (host_route->iState == CIp6Route::EIncomplete)
        {
        LOG(Log::Printf(_L("\t\tFlow[%u] IF %u [%S] Next hop address not known, start ND [%S]"), this, iface.iScope[0], &iface.iName, &log_dst));
        host_route->StartND(iHead.ip6.SrcAddr());
        }
    //
    // The Path MTU is *ALWAYS* maintained to have some sensible
    // value. Use it as is.
    //
    // 1) Check whether destination cache has a stored path mtu entry
    //      cachemtu == 0, if dstcache is not enabled or entry was not found in the cache
    // 2) If not, use value stored with interface
    const TUint cachemtu = iInterfacer.GetDstCachePathMtu(dst, iHead.iDstId);
    if (cachemtu && (iPathMtu == 0 || iPathMtu > cachemtu))
        iPathMtu = cachemtu;
    else if (iPathMtu == 0 || (TInt)iPathMtu > iface.iPMtu)
        iPathMtu = iface.iPMtu; // Just copy current Path MTU from the interface

    if (iPathMtu > 0)
        {
        // Choose framing purely on whether destination is is
        // IPv4 mapped (=> do IPv4) or not (=> do IPv6).
        iHead.ip6.SetVersion(dst.IsV4Mapped() ? 4 : 6); 
        if (iHead.ip6.HopLimit() != iface.iHopLimit)
            {
            // The current hoplimit differs from the interface default. This
            // can happen for following reasons:
            // - hoplimit has not yet been initialize (still has value 0)
            // - destination is multicast (the default is different)
            // - hoplimit has been overriden by socket option
            // - hoplimit on interface has changed due to RA
            //
            // The following assigns a value to hoplimit. This will get
            // unnecessarily executed for each refresh, if hoplimit is
            // set by socket option or destination is multicast. However,
            // this should not cause too much overhead, as refresh is not
            // supposed to happen frequently on every packet!
            //
            // Doing it in this way (comparing the limit to to interface), has
            // the advantage that if RA changes the default for the interface,
            // it will take effect on flows at next refresh (otherwise, RA
            // processing would need to force "iChanged" on all flows to get
            // it effective) -- still may have to do it that way, but first
            // this solution is tested if it will be sufficient -- msa
            //
            if (TIp46Addr::Cast(dst).IsMulticast())
                iHead.ip6.SetHopLimit(iOptions.iMulticastHops < 0 ? 1 : iOptions.iMulticastHops);
            else if (iOptions.iHopLimit >= 0)   // override by socket option?
                iHead.ip6.SetHopLimit(iOptions.iHopLimit);
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
            // According to RFC 4861, Echo Reply needs to respond with CurHopLimit set earlier by RA
            //else if (TIp46Addr::Cast(dst).IsLinkLocal() && iface.Interfacer().iLinkLocalTTL >= 0)
                //iHead.ip6.SetHopLimit(iface.Interfacer().iLinkLocalTTL);
#else
            else if (TIp46Addr::Cast(dst).IsLinkLocal() && iface.Interfacer().iLinkLocalTTL >= 0)
            iHead.ip6.SetHopLimit(iface.Interfacer().iLinkLocalTTL);
#endif //SYMBIAN_TCPIPDHCP_UPDATE	
            else
                iHead.ip6.SetHopLimit(iface.iHopLimit);
            }
        //
        // Run the ReadyL phase for the flow
        //

        RefreshHooks();                 // Set status from hooks
        iMgr->FlowStartRefresh(*this);  // (this needs be done for inner header, if tunnels!)

        //
        // If we already have a packet, we should make it possible to delay this.
        // EFlow_PENDING is only meant for NIF startup and EFlow_HOLD for temporarily blokcs.
        // Thus, for future this should something like EFlow_ROUTEHOLD = 3
        // but to minimize regression with current hooks we use HOLD for now.
        //
        if (!iRoute->iPacket.IsEmpty())
            SetStatus(EFlow_HOLD);
        //
        // If the upper layer has not specified the source address
        // explicitly (iLocalSet TRUE), then the current source address
        // from the iHead.ip6.SrcAddr() is copied for the upper layer.
        // This is done *after* the hooks so that they have a chance to
        // change it (for example, mobile IP with Care/Home Address).
        //
        // If a hook adds tunnel(s), it is the responsiblity of the hook
        // to change the source in iHead to the inner source while
        // doing the ReadyL() processing (upper layer will see the
        // inner source!)
        //
        if (!iInfo.iLocalSet)
            iInfo.iLocal.SetAddress(iHead.ip6.SrcAddr());
        //
        // Final scope id's are unconditionally set
        // (for now)
        iInfo.iLocal.SetScope(iHead.iSrcId);
        iInfo.iRemote.SetScope(iHead.iDstId);
        }
    else
        {
        iStatus = KErrInet6NoPathMtu;
        LOG(Log::Printf(_L("\t\tFlow[%u] IF %u [%S] has no MTU"), this, iface.iScope[0], &iface.iName));
        }
    }


#ifdef SYMBIAN_TCPIPDHCP_UPDATE
// CIp6Flow::IsNdPacketPendingResolution ( RFC 4861 )
// Returns ETrue if empty else returns EFalse
// *********************
TBool CIp6Flow::IsNdPacketPendingResolution()
    {
    // CIP6Route Class holds information related to pending Neighbour Discovery packet waiting for Address resolution on that route
    // This method returns ETrue if there are no pending packets
    TBool ndPktExists = EFalse;
    if (iRoute) //If Route Exists
        {
         if(!iRoute->iPacket.IsEmpty()) // Pending ND Packet exists
             {
             ndPktExists = ETrue;
             return ndPktExists;
             }
         else   // Pending ND Packet doesnt exist
             {
             ndPktExists = EFalse;
             return ndPktExists;
             }
        }
    return ndPktExists;
    }
#endif //SYMBIAN_TCPIPDHCP_UPDATE


// CIp6Flow::RouteFlow
// *******************
TInt CIp6Flow::RouteFlow(TPacketHead &aHead)
    /**
    * Route a flow to an interface based on a TPacketHead
    *
    * Using the current connection parameters in the given TPacketHead (aHead),
    * try to locate an interface for the flow. If found, complete TPacketHead
    * information from the selected interface. Verify the validity of the
    * source address, if it is defined (TPacketHead::iSourceSet == 1), or try
    * to select the source address otherwise.
    *
    * In addition to initial call in Connect(), the RouteFlow is called
    * after each MIp6Hook::OpenL, if any the addressing information has been
    * changed by the hook.
    *
    * @param    aHead   The connection parameters.
    *
    * @return A value for the flow status, as follows:
    *   @li EFlow_READY, if routing succeeded and route attached
    *   @li EFlow_PENDING, if route or source address cannot be selected
    *   @li KErrInet6NoDestination, if destination address is missing
    *   @li KErrInet6SourceAddress, if source address is out of scope
    *   @li KErrInet6AddressExpired, if source address is not valid for the interface.
    *   @li KErrInet6NoRoute, if route or source address cannot be selected and on demand setup is not desired.
    *
    * In case the route is not found, the function starts a neighbor probe for the
    * destination address on all interfaces in the current scope (only if this
    * feature is enabled via the configuration parameter or one or more interfaces
    * have link local source addresses).
    *
    */ 
    {
    const TIp6Addr &dst = aHead.ip6.DstAddr();
    TUint dstType = (TUint)(dst.Scope()-1);
    if (dst.IsUnspecified() || dstType > EScopeType_NET)
        return KErrInet6NoDestination;

    CIp6Route *rt = NULL;
    for (;;) /* NOT A LOOP, JUST FOR BREAK EXITS */
        {
        // For "forwarding flows", the source address is set, but must not affect the
        // route selection (because it would fail due to address not being a valid
        // address for this host!). Need two temp locations to hold the source info.
        const TInt use_source = aHead.iSourceSet && iInfo.iForwardingFlow == 0;
        const TIp6Addr &src_tmp = use_source ? aHead.ip6.SrcAddr() : KInet6AddrNone;
        const TUint32 src_id = use_source ? aHead.iSrcId : 0;

        if (aHead.iDstId == 0)
            {
            // Application has not specified scope. Find route within the locked scope.
            rt = iInterfacer.FindRoute(dst, iOptions.iLockId, iOptions.iLockType, src_tmp, src_id);
            if (rt != NULL)
                {
                aHead.iDstId = rt->iInterface.iScope[dstType];
                break;  // --> Success, route found
                }
            // Special kludge, because the socket server locks unnecessarily to IAP level
            // by default: if the locking is to IAP, then try finding an alternate route
            // by the network id only.
            if (iOptions.iLockType == EScopeType_IAP)
                {
                CIp6Interface *const ifp = iInterfacer.FindInterface(iOptions.iLockId, (TScopeType)iOptions.iLockType);
                if (ifp != NULL)
                    {
                    // At least one interface with matching IAP exists, try to find a route using the
                    // network scope id from this interface.
                    rt = iInterfacer.FindRoute(dst, ifp->iScope[EScopeType_NET], EScopeType_NET, src_tmp, src_id);
                    if (rt != NULL)
                        {
                        aHead.iDstId = rt->iInterface.iScope[dstType];
                        break;  // --> Success, route found
                        }
                    // should probing scope be also widened to network?
                    }
                }
            // For probing, limit it to locked scope, if any
            dstType = iOptions.iLockType;
            aHead.iDstId = iOptions.iLockId;
            }
        else
            {
            // If there is a locking to smaller scope than the requested destination scope,
            // then try to find a route within the locked scope.
            if (iOptions.iLockId && dstType > iOptions.iLockType)
                {
                rt = iInterfacer.FindRoute(dst, iOptions.iLockId, iOptions.iLockType, src_tmp, src_id);
                if (rt != NULL && rt->iInterface.iScope[dstType] == aHead.iDstId)
                    break;  // --> Success, route found
                }
            // If that route is not found or if the found interface does not match the required
            // destination scope, then assume application wants to ignore the locking scope => search
            // route by destination scope only.
            rt = iInterfacer.FindRoute(dst, aHead.iDstId, dstType, src_tmp, src_id);
            if (rt != NULL)
                break;  // --> Success, route found
            }
        //
        // No route found, do the probing if enabled and return no route.
        //
        iInterfacer.ProbeDestination(dst, aHead.iDstId, dstType, src_tmp, src_id);
        return iInfo.iNoInterfaceUp ? KErrInet6NoRoute : EFlow_PENDING; // --> Fail, route not found or pending
        }
    //
    // The route (and interface) has been located
    //
    rt->Attach(*this);

    // Lock is applied only once (must be explicitly re-enabled, if new lock is to be applied)
    iOptions.iLockId = 0;
    const CIp6Interface &iface = rt->iInterface;

    aHead.iInterfaceIndex = iface.iScope[0];
    //
    TIp6Addr &src = aHead.ip6.SrcAddr();
    // If source address is already specified, then skip the
    // automatic address selection. TPacketHead::iSourceSet is
    // initialized from the upper layer iLocalSet, and can be
    // modified by the hooks in OpenL phase.
    if (aHead.iSourceSet)
        {
        if (aHead.iSrcId == 0)
            aHead.iSrcId = iface.iScope[(src.Scope()-1)&0xF];
        return VerifyAddress(aHead, iface);
        }
    else if (iface.SelectSource(src, dst) == NULL)
        {
        const TIp6AddressInfo* address = iface.FindIpv4LinkLocalAddr();
        
        // If no appropriate routable address exists, fallback to any link local
        // address regardless of prefix (this is required by the ZEROCONF RFC and
        // a link local address can be used to reach any neighbour on-link using
        // ARP).
        if( address && address->IsAssigned() )
            {
            #ifdef _LOG
                TInetAddr addr;
                addr.SetAddress( address->iId );
                TBuf<39> addrStr;
                addr.Output( addrStr );
                
                Log::Printf(_L("CIp6Flow::RouteFlow - Unable to select source address for flow 0x%X based on destination - defaulting to link local address %S"), this, &addrStr);
            #endif
        
            src = address->iId;
            }
        else
            {
            return EFlow_PENDING;
            }
        }
    //
    // Fix source address and scope
    //
    aHead.iSourceSet = 1;
    aHead.iSrcId = iface.iScope[(src.Scope()-1)&0xF];
    return EFlow_READY;
    }



//
// CIp6Flow::Connect
// *****************
void CIp6Flow::Connect()
    /**
    * Attach a flow to route and interface.
    *
    * At the start, iHead will contain the parameters affecting the
    * flow connection (addresses, ports, etc.) in IPv6 variant of the
    * iHead.
    *
    * Runs the "Open Phase" of flow hook mechanism for this flow. This
    * will attach the interested outbound flow hooks to this flow. 
    */
    {

    // Run the open phase
retry_connect:
    // Clean up all previous crud
    iStatus = EFlow_READY;  // Make sure Disconnect doesn't call the caller back!
    Disconnect();
    iChanged = 0;
    //
    // Initialize the flow iHead (TPacketHead) from the upper layer information
    //
    const TInetAddr & localAddr = LocalAddr();
    const TInetAddr & remoteAddr = RemoteAddr();
    iHead.ip6.Init();                   // Set Version=6, hoplimit=0
    iHead.ip6.SetVersion(0);            // We really don't know yet!
    iHead.ip6.SetSrcAddr(localAddr.Ip6Address());
    iHead.ip6.SetDstAddr(remoteAddr.Ip6Address());
    // Load Selector Fields
    iHead.iProtocol = (TUint8)Protocol();
    iHead.iSrcPort = (TUint16)LocalPort();
    iHead.iDstPort = (TUint16)RemotePort();
    GetIcmpTypeCode(iHead.iIcmpType, iHead.iIcmpCode);
    //
    iHead.ip6.SetNextHeader(iHead.iProtocol);

    iHead.ip6.SetTrafficClass(iOptions.iTrafficClass);
    iHead.ip6.SetFlowLabel(remoteAddr.FlowLabel());

    iHead.iSrcId = localAddr.Scope();
    iHead.iSourceSet = iInfo.iLocalSet;
    iHead.iDstId = remoteAddr.Scope();

    // Initialize locking (if any)
    iOptions.iLockId = iInfo.iLockId;
    iOptions.iLockType = iInfo.iLockType;

#ifdef SYMBIAN_NETWORKING_UPS
    TBool isScoped = EFalse;
#endif //SYMBIAN_NETWORKING_UPS

    for (;;) /* JUST FOR BREAK-EXITS, NOT REALLY A LOOP! */
        {
#ifdef SYMBIAN_NETWORKING_UPS
        // User Prompt Service (UPS) support.
        //
        // Generate NoBearer() upcalls on all flows that have an upper provider and have not
        // yet had the scope set by ESOCK.  Generate NoBearer() even if enough information
        // is available to route them with a straight Bearer() upcall.  This is to allow
        // ESOCK to apply additional authorisation on any usage of a socket by a process.
        //
        // The check against upper provider is to exempt internal flows that aren't associated
        // with an ESOCK socket - for example, those used for Neighbour Discovery or ICMP.
        //

        TBool upsPromptingPossible = UPSPromptingPossible();      
        if (upsPromptingPossible)
            {
            if ((iOptions.iLockId == 0 && (HasProvider() || iHead.iDstId == 0)) || iUpsAuthorisationRequired)
                {
                //
                // Accept only unambiguous loopback addresses if scope is not locked.
                //
                                
                //
                // iUpsAuthorisationRequired is True if iOptions.iLockId has been set by
                // SetOption KSoInterfaceIndex. This allows a gratituous (scoped) nobearer
                // up call to be made which results in a request being made to the Ups Server.
                //
                const TInt scope_level = iHead.ip6.DstAddr().Scope();

                if (scope_level != KIp6AddrScopeNodeLocal)  // not a loopback address
                    {
                    // TODO 1116 NW: find out the correct means of preventing NoBearer() on 
                    // destination addresses which are one of our interface addresses.
                    // Pity this is going to take up cpu time for the check.
                    //
                    // iInterfacer.IsForMeAddress(iHead.ip6.DstAddr(), 0) seems inappropriate
                    // as it doesn't do scope matching and works with interface index second argument,
                    // which is meaningless in this context.
                    // 
                    // iInterfacer.FindInterface(RemoteAddr()) seems more appropriate, but not
                    // entirely complete as it doesn't do ELoopback route matching.  So it will match
                    // addresses.  If the address of an interface doesn't quite match, but it has a
                    // route does match, then it won't pick tis up.  Perhaps it is good enough.  Perhaps
                    // we don't need to do anything here at all!
                    
                    // Sending to a local interface address is treated as a loopback, so avoid issuing
                    // NoBearer() in this case - drop through to Bearer().

                    // The following 2 lines have been removed as iInterfacer does not always return the correct interface
                    // const CIp6Interface* ifp = iInterfacer.FindInterface(RemoteAddr());
                    // if (ifp == NULL)
                    
                        {
                        if (iHead.iDstId != 0 || scope_level == KIp6AddrScopeGlobal)
                            {
                            // If we *could* have gone through and attempted to connect the flow,
                            // were it not for UPS, then mark this with a flag which will be
                            // communicated to ESock in the NoBearer() upcall.  Effectively, we
                            // are issuing what we think is a gratuitous NoBearer() just for ESock
                            // to get a look in before the flow is connected.  ESock will
                            // issue a SetOption(KSoConnectionInfo) with NetworkId KNetworkIdFromAddress
                            // instead of what it thinks is the correct NetworkId (which is not going to
                            // be based on the scope from the user).
                            isScoped = ETrue;
                            }
                        break;
                        }
                    }
                } 
            
            // ESock can issue a SetOption(KSoConnectionInfo) with NetworkId KNetworkIdFromAddress after
            // NoBearer() on flows where either the scope id has been set by the user, or the address is
            // unambiguous.  This indicates that the flow is now able to be connected using the scope specified,
            // rather than ESock giving the scope explicitly.  In effect, ESock is saying that it has "blessed"
            // the flow for connection.  In other words, "attempt to attach the flow to a route, but determine
            // the NetworkId from the socket address, as you've already indicated in the NoBearer() that you
            // have enough information to do so".
            //
            // KNetworkIdFromAddress is just a magic way of signalling this method, as issuing a KSoConnectionInfo
            // with NetworkId == 0 is filtered out at a higher level.  Reset iLockId to zero as we've received
            // the signal that we need.
            //
            
            if (iOptions.iLockId == KNetworkIdFromAddress)
                {
                iOptions.iLockId = 0;
                }
            }
        else
            {
            if (iHead.iDstId == 0 && iOptions.iLockId == 0)
                {
                //
                // Accept only unambiguous addresses if scope is not locked. -MikaL
                //
                const TInt scope_level = iHead.ip6.DstAddr().Scope();
                if (scope_level != KIp6AddrScopeGlobal &&   // not IPv6 global address
                    scope_level != KIp6AddrScopeNodeLocal)  // not a loopback address
                    break;
                }                           
            }
#else
        if (iHead.iDstId == 0 && iOptions.iLockId == 0)
            {
            //
            // Accept only unambiguous addresses if scope is not locked. -MikaL
            //
            const TInt scope_level = iHead.ip6.DstAddr().Scope();
            if (scope_level != KIp6AddrScopeGlobal &&   // not IPv6 global address
                scope_level != KIp6AddrScopeNodeLocal)  // not a loopback address
                break;
            }
#endif

        iStatus = RouteFlow(iHead);
        if (iStatus != 0)
            break;
        iHdrSize = 0;
        iStatus = iMgr->FlowSetupHooks(*this);

        if (iStatus != 0)
            break;          // Pending or fatal state, cannot proceed.

#ifndef SYMBIAN_NETWORKING_UPS
        // Decide on what destinations require network services capabily:
        // - if it is my own assigned address allow without capability (IsMyPrefix test)
        // - otherwise, allow only node local destinations (loopbacks)
            if (!ApplyStaticSecurityCheck())
                {
                return;             
                }
#else
        // Decide on what destinations require network services capabily:
        // - if it is my own assigned address allow without capability (IsMyPrefix test)
        // - otherwise, allow only node local destinations (loopbacks)
        if (!upsPromptingPossible)
            {
            if (!ApplyStaticSecurityCheck())
                {
                return;             
                }
            }
                
#endif
        //
        // The final interface has been located
        //
        Start();
        SelectNextHop();
        if (iStatus != 0)
            return;
        ASSERT(iRoute != NULL);
#ifdef _LOG
            {
            TLogAddressPrefix src(iInfo.iLocal);
            TLogAddressPrefix dst(iInfo.iRemote);
            Log::Printf(_L("\t\tFlow[%u] Connect: prot=%d src=[%S], dst=[%S] attached to IF %u [%S]"),
                this, (TInt)iInfo.iProtocol, &src, &dst, iRoute->iInterface.iScope[0], &iRoute->iInterface.iName);
            }
#endif
        TPckgBuf<TSoIfConnectionInfo> netinfo;
        netinfo().iIAPId = iRoute->iInterface.iScope[EScopeType_IAP];
        netinfo().iNetworkId = iRoute->iInterface.iScope[EScopeType_NET];
        Bearer(netinfo);
        RefreshFlow();
        return;
        }
    //
    // Cannot locate any interface for the flow.
    // Punt the flow directly into holding state
    //
    iInterfacer.MoveToHolding(*this);
    ASSERT(iRoute != NULL);
#ifdef _LOG
        {
        TLogAddressPrefix src(iInfo.iLocal);
        TLogAddressPrefix dst(iInfo.iRemote);
        Log::Printf(_L("\t\tFlow[%u] Connect: prot=%d src=[%S], dst=[%S] into holding (%d)"),
            this, (TInt)iInfo.iProtocol, &src, &dst, iStatus);
        }
#endif
    if (iStatus > 0)
        {
        // The next hop could not be selected because no suitable
        // interface or route matched the requirements. Need to
        // activate new interfaces (or just keep waiting for them)

        if (iInfo.iNoInterfaceUp == 0)
            {
            // Because NoBearer may cause immediate call to SetOption with
            // connection info, there are the following problems:
            //
            // 1) The connection info SetOption should wake up the flow, and
            //    needs to call SetStatus(EFlow_READY). If flow is pending,
            //    it will cause a CanSend. This must be prevented => Thus,
            //    for NoBearer call, iStatus is temporarily set to READY.
            // 2) Must detect whether NoBearer called SetOption, and if it
            //    did, this code must retry the connect. => Thus, remember
            //    current locking and see if it has changed during the
            //    NoBearer.
            // 
            const TInt old_status = iStatus;
            const TUint old_type = iInfo.iLockType;
            const TUint32 old_id = iInfo.iLockId;
            iStatus = EFlow_READY;

            _LIT8(KProtoIPv6, "protocol=ip6");
            _LIT8(KProtoIPv4, "protocol=ip");

#ifdef SYMBIAN_NETWORKING_UPS
            const TInt KMaxArgLen = 19;     // sufficient for longest string length ("protocol=ip6 scoped")
            TBuf8<KMaxArgLen> arg(iHead.ip6.DstAddr().IsV4Mapped() ? KProtoIPv4() : KProtoIPv6());
            if (isScoped)
                {
                _LIT8(KScoped, " scoped");
                arg.Append(KScoped());
                }
                    
            if (iUpsAuthorisationRequired)
                {
                // A NoBearer upcall is being made after a set option KSoInterfaceIndex
                // which will result in a request to the UPS Server.
                // Set a flag to indicate that a UPS request is pending.
                // This flag will be reset when a KSoConnectionInfo is received in
                // response to the NoBearer upcall.
                iUpsAuthorisationPending = ETrue;                   
                }
            NoBearer(arg);
#else
            NoBearer(iHead.ip6.DstAddr().IsV4Mapped() ? KProtoIPv4() : KProtoIPv6());
#endif
            if (iInfo.iLockType != old_type || iInfo.iLockId != old_id)
                goto retry_connect;
            //
            // NoBearer didn't call or change locking, restore
            // original pending status and exit.
            //
            iStatus = old_status;
            }
        }
    }

#ifdef SYMBIAN_NETWORKING_UPS
TBool CIp6Flow::UPSPromptingPossible()
    {
    TBool upsPromptingPossible = EFalse;
    
    if (HasProvider())
        {
        TUint version = 0;
        _LIT8(KProviderBindings, "MProviderBindings");
        MProviderBindings* providerBindings = 0;
        TRAPD(err, providerBindings  = (MProviderBindings*) GetProviderApiL(KProviderBindings, &version));              
        if ((providerBindings != NULL) && (err == KErrNone ))
            {
            upsPromptingPossible = providerBindings->HasSocket();                               
            }
        }
    return upsPromptingPossible;    
    }
#endif

TBool CIp6Flow::ApplyStaticSecurityCheck()
    {
    TBool rc = ETrue;
    
    ASSERT(iRoute);
    if (!iRoute->IsMyPrefix() && iHead.ip6.DstAddr().Scope() > KIp6AddrScopeNodeLocal)
        {
        iStatus = CheckPolicy(KPolicyNetworkServices, "TCPIP Connect");
        if (iStatus != 0)
            rc = EFalse;                            
        }
    return rc;      
    }
// CIp6Flow::Disconnect
// ********************
void CIp6Flow::Disconnect()
    /**
    * Disconnect flow (remove hooks).
    */
    {
    iChanged = 1;
    iHead.iPacket.Free();
    RemoveHooks();
    //
    // Setting EFlow_READY should cause a wakeup for the
    // SAP, if the flow status is pending.
    // *WARNING* As Disconnect() is called in various
    // contexts, there is a danger for infinite
    // recursion, if this is called carelessly for
    // PENDING flows...  -- msa
    SetStatus(EFlow_READY);
    }

//
// CIp6Flow::InterfaceSMtu/InterfaceRMtu
// *************************************
// Return interface semd MTUs, if connected.
TInt CIp6Flow::InterfaceSMtu() const
    {
    return iRoute ? iRoute->iInterface.iSMtu : KErrNotReady;
    }

// Return interface receive MTUs, if connected.
TInt CIp6Flow::InterfaceRMtu() const
    {
    return iRoute ? iRoute->iInterface.iRMtu : KErrNotReady;
    }

//
// Local utility functions
//
static TInt GetIntValue(const TDesC8 &aOption, TInt aDefault)
    {
    if (aOption.Length() < (TInt)sizeof(TInt))
        return aDefault;
    // note: here it is assumed that the Ptr() is properly
    // aligned, but for debug check it! -- msa)
    ASSERT((((TUint)aOption.Ptr()) & 0x3) == 0);
    return *((TInt *)aOption.Ptr());
    }

static void SetIntValue(TDes8 &aOption, TInt aValue)
    {
    aOption = TPtrC8((TUint8 *)&aValue, sizeof(aValue));
    }
//
// CIp6Flow::GetOption
// *******************
TInt CIp6Flow::GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const
    {
    if (aLevel == KSolInetIp)
        {
        switch(aName)
            {
        case KSoIpTOS:
            SetIntValue(aOption, iOptions.iTrafficClass);
            return KErrNone;
        case KSoIpEcn:
            SetIntValue(aOption, iOptions.iTrafficClass & 3);
            return KErrNone;
        case KSoIpTTL:              // Old IPv4 option
        case KSoIp6UnicastHops:     // New IPv6/4 option
            SetIntValue(aOption,
                // ...return explicitly set value, if defined by socket option
                iOptions.iHopLimit >= 0 ? iOptions.iHopLimit :
                // ...return current value from interface, if connected flow
                iRoute ? iRoute->iInterface.iHopLimit :
                // ...return system default otherwise
                iInterfacer.iMaxTTL);
            return KErrNone;
        case KSoIp6InterfaceUnicastHops:    // ignores any socket option which would override the current setting
            SetIntValue(aOption,
                // ...return current value from interface, if connected flow
                iRoute ? iRoute->iInterface.iHopLimit :
                // ...return system default otherwise
                iInterfacer.iMaxTTL);
            return KErrNone;
        case KSoIp6MulticastHops:
            // If no socket specific option has been set, then return the default 1.
            SetIntValue(aOption, iOptions.iMulticastHops < 0 ? 1 : iOptions.iMulticastHops);
            return KErrNone;
        case KSoIp6MulticastLoop:
            SetIntValue(aOption, iOptions.iMulticastLoop);
            return KErrNone;
        case KSoNoInterfaceError:
            SetIntValue(aOption, iInfo.iNoInterfaceError);
            return KErrNone;
        case KSoInterfaceIndex:
            SetIntValue(aOption, (TInt)(
                (iInfo.iLockType == 0) ? iInfo.iLockId :
                (iRoute && !iRoute->IsHoldingRoute()) ? iRoute->iInterface.iScope[0] :
                0));
            return KErrNone;
        case KSoKeepInterfaceUp:
            SetIntValue(aOption, iOptions.iKeepInterfaceUp);
            return KErrNone;
        case KSoNextHop:
            // Return information about the current route/next hop selection.
            if (aOption.Length() != sizeof(TInetRouteInfo))
                return KErrArgument;
            if (iStatus == EFlow_READY && iRoute)
                {
                TInetRouteInfo &opt = *(TInetRouteInfo*)aOption.Ptr();
                TTime stamp;
                stamp.UniversalTime();
                const CIp6Route *route = iRoute->iRouter ? iRoute->iRouter : iRoute;
                route->FillRouteInfo(opt, route->iInterface.Elapsed(stamp));
                return KErrNone;
                }
            return KErrNotReady;    // The flow is not connected.
        default:
            break;
            }
        }
    return iMgr->GetFlowOption(aLevel, aName, aOption, *this);
    }
// CIp6Flow::SetOption
// *******************
TInt CIp6Flow::SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption)
    {
    if (aLevel == KSolInetIp)
        {
        // ...so far all implemented options use only int
        // parameter. Just save code space and prefetch value,
        // whether it is needed or not (GetIntValue should be safe
        // to call in all cases).
        const TInt val = GetIntValue(aOption, 0);
        switch (aName)
            {
        // IPv4 TOS and IPv6 TrafficClass fields are now split to the TOS part and
        // to the Explicit Congestion Notification (ECN) part (the least significant two
        // bits). TCP SAP may prevent modifying the ECN bits with the TOS option. With UDP
        // the option works in the traditional way.
        case KSoIpTOS:
            if (val < 0 || val > 255)
                return KErrArgument;    // Invalid option value

            if (val != iOptions.iTrafficClass)
                {
                iOptions.iTrafficClass = (TUint8)val;
                iChanged = 1;
                }
            return KErrNone;
        case KSoIpEcn:
            if (val < 0 || val > 3)
                return KErrArgument;
            if (val != (iOptions.iTrafficClass & 3))
                {
                TUint8 tosfield = (TUint8)val;
                tosfield |= iOptions.iTrafficClass & 0xfc;
                iOptions.iTrafficClass = tosfield;
                iChanged = 1;
                }
            return KErrNone;
        case KSoIpTTL:              // Old IPv4 option
        case KSoIp6UnicastHops:     // New IPv6/4 option
            if (val < -1 || val > 255)
                return KErrArgument;
            if (val != iOptions.iHopLimit)
                {
                iOptions.iHopLimit = (TInt16)val;
                iChanged = 1;
                }
            return KErrNone;
        case KSoIp6MulticastHops:
            if (val < -1 || val > 255)
                return KErrArgument;
            if (val != iOptions.iMulticastHops)
                {
                iOptions.iMulticastHops = (TInt16)val;
                iChanged = 1;
                }
            return KErrNone;
        case KSoIp6MulticastLoop:
            // For now, assume this option doesn't require iChanged processing!
            if (val == 0)
                iOptions.iMulticastLoop = 0;    // disable loopback of multicast
            else if (val == 1)
                iOptions.iMulticastLoop = 1;    // enable loopback of multicast
            else
                return KErrArgument;
            //break;
            return KErrNone;  // was 'break'? why? fixed to return 9.1.2002/msa
        case KSoNoInterfaceError:
            // Control whether flow gets interface errors or not
            if (val == 0)
                iInfo.iNoInterfaceError = 0;
            else if (val == 1)
                iInfo.iNoInterfaceError = 1;
            else
                return KErrArgument;
            return KErrNone;
        case KSoInterfaceIndex:
            // Fix flow to a specific interface, if non-zero
            if (iInfo.iLockType != 0 || iInfo.iLockId != (TUint)val)
                {
                iInfo.iLockType = EScopeType_IF;
                iInfo.iLockId = (TUint)val;
                iChanged = 1;
#ifdef SYMBIAN_NETWORKING_UPS
                //
                // iInfo.iLockId == 0 is tested as a condition which allows 
                // entry into the logical branch in CIp6Flow::Connect() that
                // triggers a NoBearer up call. A TNoBearer message results in
                // a call to the UPS Server. Setting iUpsAuthorisationRequired
                // ensures this branch is executed.
                // 
                iUpsAuthorisationRequired = ETrue;
#endif
                }
            return KErrNone;
        case KSoKeepInterfaceUp:
            // Just treat any non-zero val as "1"
            if ((int)iOptions.iKeepInterfaceUp == (val != 0))
                return KErrNone; // Value not changed
            iOptions.iKeepInterfaceUp = (val != 0);
            // Update the flow count on the interface
            if (iRoute)
                iRoute->iInterface.UpdateFlowCount((val != 0) ? 1 : -1);
            return KErrNone;
        case KSoNoSourceAddressSelect:
            iInfo.iLocalSet = 1;
            return KErrNone;
        case KSoNextHop:
            // Force next hop selection. The option argument is ignored.
            Start();
            SelectNextHop();
            return iStatus <= 0 ? iStatus : KErrNotReady;
        default:
            break;
            }
        }
    else if (aLevel == STATIC_CAST(TUint, KSOLProvider))
        {
        const TUint optlen = aOption.Length();
        const TUint8 *optptr = aOption.Ptr();

        if (aName == (TUint)KSoConnectionInfo)
            {
            if (optlen >= sizeof(TSoIfConnectionInfo))
                {
                const TSoIfConnectionInfo &opt = *(TSoIfConnectionInfo*)optptr;
                TUint new_type;
                TUint new_lock;
                if (opt.iIAPId)
                    {
                    // Locking on IAP
                    new_type = EScopeType_IAP;
                    new_lock = opt.iIAPId;
                    }
                else
                    {
                    // Locking on NetworkId
                    new_type = EScopeType_NET;
                    new_lock = opt.iNetworkId;
                    }
                if (iInfo.iLockId != 0 && iInfo.iLockType == EScopeType_IF)
                    {
                    // If the flow has already been explicitly locked to an interface
                    // using the socket option KSoInterfaceIndex, then assume application
                    // really wants to override the default connection settings that would
                    // be coming from the socket server.
                    LOG(Log::Printf(_L("\t\tFlow[%u] Set ConnectionInfo: IF=%d already interface locked, %S=%d ignored"),
                        this, (TInt)iInfo.iLockId, &LogScopeName(new_type), (TInt)new_lock));

#ifdef SYMBIAN_NETWORKING_UPS
                if (iUpsAuthorisationRequired && iUpsAuthorisationPending)
                    {
                    // This branch is called as a direct result of a NoBearer call which has
                    // resulted from CIp6Flow::Connect() being called after SetOption(KSoInterfaceIndex).
                    // Now reset the flags which allowed a NoBearer upcall to be made.

                    iUpsAuthorisationRequired = EFalse;
                    iUpsAuthorisationPending  = EFalse;

                    // The following three lines cater for the following stack trace:
                    //  CIp6Flow::Connect()
                    //      NoBearer("scoped")
                    //          SetOption(KSoConnectionInfo(KNetworkIdFromAddress))
                    // When we eventually return to CIp6Flow::Connect(), we need to ensure that it
                    // retries the flow connect.
                    if (new_type == EScopeType_NET && new_lock == KNetworkIdFromAddress)
                        {
                        iInfo.iLockType = new_type;
                        iInfo.iLockId = new_lock;
                        }

                    SetStatus(EFlow_READY);                 
                    }
#endif

                    return KErrNone;
                    }
                LOG(Log::Printf(_L("\t\tFlow[%u] Set ConnectionInfo: %S=%d -> %S=%d"),
                    this, &LogScopeName(iInfo.iLockType), (TInt)iInfo.iLockId, &LogScopeName(new_type), (TInt)new_lock));
                if (iInfo.iLockType != new_type || iInfo.iLockId != new_lock)
                    {
                    iInfo.iLockType = new_type;
                    iInfo.iLockId = new_lock;
                    iChanged = 1;
                    // Wake up the flow, if pending
                    SetStatus(EFlow_READY);
                    }
                return KErrNone;
                }
            LOG(Log::Printf(_L("\t\tFlow[%u] Set ConnectionInfo: Bad TSoIfConnection"), this));
            return KErrArgument;
            }
        }
    return iMgr->SetFlowOption(aLevel, aName, aOption, *this);
    }

//
// ************************
// CIp6Route Implementation
// ************************
//
//
// CIp6Route
// *********
// Construct & Destruct
//
CIp6Route::CIp6Route(TUint aIndex, CIp6Manager &aMgr, const TIp6Addr &aAddr, TInt aPrefix, CIp6Interface &aInterface)
:   iIndex(aIndex),
    iInterfacer(aMgr),
    iPrefix(aAddr),
    iLength((TUint8)aPrefix),
    iMetric(KPreferenceMetric[ERoutePreference_MEDIUM]),
    iTimeout(CIp6RouteTimeoutLinkage::Timeout),
    iInterface(aInterface)
    {
    }

CIp6Route::~CIp6Route()
    {
    LOG(Log::Write(_L("~CIp6Route()")));
    CancelTimer();  // ..if any pending
    iPacket.Free();     // (if any)
    //
    // Just detach all flows attached to this route
    // (Should have a "route down" error status? -- msa)
    //
    TInt changed = 0;
    CIp6Flow *f;
    while ((f = iFlowList) != NULL)
        {
        iFlowList = f->iNext;
        f->iRoute = NULL;
        f->iNext = NULL;
        f->SetStatus(EFlow_DOWN);
        changed -= f->iOptions.iKeepInterfaceUp;
        }
    iInterface.UpdateFlowCount(changed);
    }

//
// CIp6Route::Attach
// *****************
/**
// Attach flow to a specific route (it has already
// been decided elsewhere that this route is the
// correct one; this function is just pure housekeeping)
*/
void CIp6Route::Attach(CIp6Flow &aFlow)
    {
    //
    // A flow can only be attached to one route at time.
    // If different from this, then detach first.
    //
    if (aFlow.iRoute != this)
        {
        if (aFlow.iRoute)
            aFlow.iRoute->Detach(aFlow);
        //
        // Add to the flow list of the route
        //
        aFlow.iRoute = this;
        aFlow.iNext = iFlowList;
        iFlowList = &aFlow;
        iInterface.UpdateFlowCount(aFlow.iOptions.iKeepInterfaceUp);
        }
    }

#ifdef _LOG
const TDesC &CIp6Route::LogRouteType() const
    {
    _LIT(KIncomplete,   "Incomplete");
    _LIT(KReachable,    "Reachable");
    _LIT(KStale,        "Stale");
    _LIT(KDelay,        "Delay");
    _LIT(KProbe,        "Probe");

    _LIT(KMyPrefix,     "MyPrefix");
    _LIT(KOnlink,       "Onlink");
    _LIT(KGateway,      "Gateway");
    _LIT(KAnycast,      "Anycast");
    _LIT(KRedirect,     "Redirect");
    _LIT(KMulticast,    "Multicast");
    _LIT(KHolding,      "Holding");
    _LIT(KInvalid,      "Invalid");

    switch (iState)
        {
        case EIncomplete: return KIncomplete;
        case ELoopback: if (iIsMulticast) return KMulticast; else return KMyPrefix;
        case EOnlink: return KOnlink;
        case EGateway: return KGateway;
        case ERedirect: return KRedirect;
        case EAnycast: return KAnycast;
        case EReachable: return KReachable;
        case EStale: return KStale;
        case EDelay: return KDelay;
        case EProbe: return KProbe;
        case EHolding: return KHolding;
        default: break;
        }
    return KInvalid;
    }

void CIp6Route::LogRoute(const TLifetime aLifetime) const
    {
    // index interface route/prefix type address ...
    _LIT(KFormat0, "\tIF %u [%S] ROUTE %u %S %S %S metric=%d");
    _LIT(KFormat1, "\tIF %u [%S] ROUTE %u %S %S %S metric=%d LT=%u");
    _LIT(KFormat2, "\tIF %u [%S] ROUTE %u %S %S %S metric=%d REMOVED");

    const TDesC &format =
        aLifetime == KLifetimeForever ? KFormat0() : aLifetime != 0 ? KFormat1() : KFormat2();

    TLogAddressPrefix tmp(iPrefix, iLength);
    TLogAddressPrefix gw;
    if (Type() != CIp6Route::ELoopback)
        {
        TInetAddr addr;
        iAddress.GetAddress(addr);
        gw.Set(addr);
        }
    Log::Printf(format,
        iInterface.iScope[0],
        &iInterface.iName,
        iIndex,
        &tmp,
        &LogRouteType(),
        &gw,
        (TInt)iMetric,
        aLifetime);
    }
#endif
//
// CIp6Route::Attach
// *****************
/**
// Detach all flows from one route and add them to this route
//
// @li *NOTE*
//  As flows are set into PENDING state, this makes only
//  sense as a method of the "holding route".
*/
void CIp6Route::Attach(CIp6Route &aRoute)
    {
    CIp6Flow *f;
    TInt count = 0;

    while ((f = aRoute.iFlowList) != NULL)
        {
        aRoute.iFlowList = f->iNext;
        f->iNext = iFlowList;
        iFlowList = f;
        f->iRoute = this;
        // reminder: SetStatus for pending (> 0) will not change the
        // flow status, if it already is in error state (the error
        // state is preserved) -- msa
        f->SetStatus(EFlow_PENDING);
        //++count;
        count += f->iOptions.iKeepInterfaceUp;
        }
    iInterface.UpdateFlowCount(count);
    aRoute.iInterface.UpdateFlowCount(-count);
    }

//
// CIp6Route::Detach
// *****************
// Disconnect flow from route. Just pure house keeping
void CIp6Route::Detach(CIp6Flow &aFlow)
    {
    if (this == aFlow.iRoute)
        {
        CIp6Flow **h, *f;
        //
        // Some concern: Will this list normally be short enough for
        // this simple scan or should a double linked list with
        // deque applied?  -- msa
        //
        aFlow.iRoute = NULL;
        for (h = &iFlowList; (f = *h) != NULL; h = &(f->iNext))
            if (f == &aFlow)
                {
                *h = f->iNext;
                iInterface.UpdateFlowCount(-(int)aFlow.iOptions.iKeepInterfaceUp);
                return;                 // Succesful/Normal result.
                }
        User::Panic(_L("DEBUG"), 0);    // Should never happen!
        }
    else
        {
        ASSERT(aFlow.iRoute == NULL);
        }
    }

//
// CIp6Route::NofifyFlows
// **********************
// Notify all flows attached to this route
void CIp6Route::NotifyFlows(TInt aState)
    {
    TFlowNotifyList list;
    for (CIp6Flow *f = iFlowList; f; f = f->iNext)
        f->Notify(list, aState);
    list.Deliver(aState);
    }

// CIp6Route::SetChanged
// *********************
// Set iChanged for all flows on this route
TInt CIp6Route::SetChanged(const TInt aScope) const
    {
    if (aScope > 0)
        return iInterface.SetChanged(aScope-1);
    TInt count = 0;
    for (CIp6Flow *f = iFlowList; f; f = f->iNext)
        count += f->SetChanged(0);
    return count;
    }

//
//  CIp6Route::Send
//  ***************
/**
//  Send a packet to the attached interface,
//  @param aPacket  The packet to send (with info block).
//  @param aSrc     The source (just passed through, not significant)
//  @param aMulticastLoop
//      If non-zero and destination is multicast, then
//      pass copy of the packet to receivers on this node.
//  @return
//  @li < 0, no interface of some other missing component
//  @li = 0, packet sent, but interface does not want more after this
//  @li = 1, packet sent, and interface is willing to accept more
//
//  The packet "ownership" is always transfered, regardless of the return type,
//  aPacket should be empty after this.
//
//  On entry, the packet must have a info structure of RMBufSendInfo.
*/
TInt CIp6Route::Send(RMBufChain& aPacket, CProtocolBase* aSrc, TInt aMulticastLoop)
    {
    TInt ret = KErrNotReady;
    for (;;)            // Just to provide multiple exits (break) below
        {
        RMBufSendInfo *const info = RMBufSendPacket::PeekInfoInChain(aPacket);
        if (!info)
            break;

        // "Dereference" automatically when sent to a gateway route
        if (iRouter)
            {
            if (info->iFlags & KIpDontRoute)
                break;  // "routing disabled", just drop the packet!
            return iRouter->Send(aPacket, aSrc, aMulticastLoop);
            }

        if (Type() == ELoopback)    // Use Type() to cover all loopback routes (like Anycast)
            {
            // Tag the packet as "loopback", so that inbound processing can easily
            // recognize such packets (used in capability checks).
            info->iFlags |= KIpLoopbackPacket;
            // Set and clear KIpBroadcastOnLink depending on if destination is multicast.
            // (this catches net broadcast and joined multicast groups).
            if (iIsMulticast)
                info->iFlags |= KIpBroadcastOnLink;
            else
                info->iFlags &= ~KIpBroadcastOnLink;
    
            // Do not require iNifIf on ELoopBack destinations
            const TIp6Addr &dst = TInetAddr::Cast(info->iDstAddr).Ip6Address();
            //
            // Assume if route is multicast, then dest is also!
            //
            // *NOTE*
            //  This branch only gets multicast routes for joined multicast
            //  groups (Type() == ELoopback). Beware: there are also other
            //  types of routes with multicast address prefix!
            //
            if (!iIsMulticast || dst.Scope() < KIp6AddrScopeLinkLocal)
                {
                // Destination is not multicast or has less than link local scope.
                // This is plain loopback in the stack internally.
                // No other special handling required.
                iInterface.iNifUser->iNetwork->Process(aPacket, (CProtocolBase *)iInterface.iNifIf);
                ret = 1;        // Return 1 to indicate "more packets can be sent"
                break;
                }
            // Destination is multicast and has wider than node local scope...
            if (aMulticastLoop)
                {
                // The packet must be both looped back and also sent out to interface,
                // a copy of the packet is needed...
                RMBufPacketBase clone;
                TRAP(ret, clone.CopyPackedL(aPacket));
                if (ret != KErrNone)
                    {
                    clone.Free();   // just in case...
                    break;
                    }
                iInterface.iNifUser->iNetwork->Process(clone, (CProtocolBase *)iInterface.iNifIf);
                ret = KErrNotReady; // (restoring the "default" return status)
                }
            // Continue processing the multicast packet as any other packet, sending it
            // to the interface..
            }
        else
            {
            if (iState == EIncomplete)
                {
                // The neighbor discovery is still in progress, need to queue
                // at least ONE packet to wait for it's completion. Swap the
                // content of the queue (iPacket) and current packet (aPacket)
                //
                // Also notify HOLD to hint flows that they shouldn't send anything
                // before the ND completion...
                //
                NotifyFlows(EFlow_HOLD); 
                RMBufChain tmp(iPacket);
                iPacket = aPacket;
                aPacket = tmp;
                ret = 1;
                break;
                }
            else if (iInterface.NeedsND() &&
                (iState == EStale ||
                 (iState == EReachable &&
                // iReachableTime needs to be represented in TickCount units -- msa
                (User::TickCount() - iTimeStamp) > iInterface.iReachableTime)))
                {
                // Start the delay timer
                iState = EDelay;
                iRetry = 1;
                // This a bit dangerous call, as iRoute->Timeout() also includes code to
                // do route destruction! Should be damn sure that this call doesn't
                // trigger that!!! -- msa
                Timeout();
                }
            if (iAddress.Family())
                {
                // Copy address information into info destination address
                iAddress.GetAddress(info->iDstAddr);
                }
            // Clear KIpBroadcastOnLink. It will be set in the Send of CIp6Interface
            // if destination is IPv4 or IPv6 multicast address.
            info->iFlags &= ~KIpBroadcastOnLink;
            }
        // If destination is still IP address and IPv4 mapped, then
        // pass it to the interface as plain IPv4 address (KAfInet)!
        // (even, if ND interface, this is still relevant for multicast
        // destinations)
        if ((TInetAddr::Cast(info->iDstAddr).IsV4Mapped()))
            (TInetAddr::Cast(info->iDstAddr)).ConvertToV4();

        return iInterface.Send(aPacket, aSrc);
        }
    aPacket.Free(); // NOOP, if packet ownership taken by someone.
    return ret;
    }

//
// CIp6Route::StartND
// ******************
// Start ND process.
// @param aSrc The source to be used.
void CIp6Route::StartND(const TIp6Addr &aSrc)
    {
    // There should be no need to test for EIncomplete. However, as some
    // ND messages are also used in Point-to-Point links (RA), it is
    // possible that EIncomplete routes get created for non-ND interfaces.
    // Thus, NeedsND() is checked, and if not set, the route is directly
    // changed into EReachable state,
    ASSERT(iState == EIncomplete);

    if (!iInterface.NeedsND())
        {
        iState = EReachable;
        return;
        }
    //
    // On EIncomple route the timer is always active, if
    // ND is in progress. When ND terminates, fail or success,
    // the route state will not be EIncomplete!
    //
    if (!IsTimerActive())
        {
        // ...when incomplete, triggering SRC address
        // is stored in the iAddress field of the route.
        iAddress.SetAddress(aSrc);  // ..thus, store the src
        iRetry = iInterface.iND.iMaxMulticastSolicit;
        Timeout();
        }
    }

//
// CIp6Route::Update
// *****************
/**
// Update route entry.
// @param   aFlags  How to update
// @param   The gateway (gateway routes) or link layer address (host routes).
// @param   The lifetime.
// @returns
//  @li 0, if state change cannot release any flows
//  @li 1, if state change potentially released flows
*/
TInt CIp6Route::Update(TInt aFlags, const TSockAddr *aAddress, const TLifetime *const aLifetime)
    {
//  const TUint family = aAddress ? aAddress->Family() : KAFUnspec;
    const TSockAddr *const address = (aAddress && aAddress->Family() != KAFUnspec) ? aAddress : NULL;
    TInt notify = 0;
    switch (iState)
        {
        case ERedirect:
        case EGateway:
            if (address)
                {
                iAddress.SetAddress(*address);

                // Changed address of a gateway entry (includes
                // case of when gateway entry is created). Must
                // now "refresh" the iRouter entry!
                // Should verify that Family is KAfInet6? -- msa
                CIp6Route *router = iInterface.FindNeighbor(iAddress.Ip6Address());
                if (router && router->iIsRouter)
                    iRouter = router;
                else
                    {
                    // If route was ERedirect, it should never get here (and if it
                    // gets, it should be deleted--however, "Update" cannot delete
                    // the self (this). [should be ok, ERedirect is only installed from
                    // ND handler, and it should make sure to install the host
                    // route (ISROUTER=1) before creating the redirection! --msa]
                    ASSERT(iState != ERedirect);
                    iRouter = NULL;
                    }
                notify = 1;
                }
            // *FALL THROUGH*
        case ELoopback:
        case EOnlink:
        default:
            if (aLifetime)
                {
                //
                // Reset the lifetime of this route (aLifetime should be > 0, but
                // just as a safety measure, treat 0 as 1)
                CancelTimer();
                ASSERT(iRetry == 0);
                iRetry = 0;     // should be unnecessary..
                ASSERT(*aLifetime > 0);
                if (*aLifetime < KLifetimeForever)
                    // NEED FIXING/LOOKING INTO: There is theoretical possibility that
                    // timeout expires directly and deletes the route now... Should
                    // prevent that somehow!  -- msa
                    SetTimer(*aLifetime ? *aLifetime : 1);
                }
            return notify;
        //
        // NEIGHBOR DISCOVERY SECTION
        // --------------------------
        case EIncomplete:
            // TimeStamp can be set on every update, because the reachability
            // algorithm kicks in only when entry is changhed into some other
            // state. Setting it here guarantees that every IsHostRoute() has
            // some reasonable value for the time stamp (all such routes are
            // *ALWAYS* created as first into "EIncomplete" state). TimeStamp
            // can then be used in the cleanup process to detect old unused
            // entries (see CIp6Interface::Timeout()). [in other states,
            // updating time stamp depends on the situation and cannot be done
            // unconditionally -- msa]
            iTimeStamp = User::TickCount();
#if 0
            if (address == NULL)
                {
                // This is dubious, might cause a fail of some ND
                // conformance test. However, this is needed, because PPP
                // link needs to get it's IS ROUTER bit set, even if it
                // has no link layer addresses..
                // [if link layer address is missing, should ND enabled
                // interface just return here, and NOT do the ISROUTER
                // stuff? -- msa]
                if (iInterface.iHwAddr.Family() == KAFUnspec)
                    {
                    // If interface does not use link layer addresses,
                    // just implicitly change into reachable state...
                    iState = EReachable;
                    }
                break;
                }
            CancelTimer();          // if any active!
            iAddress.SetAddress(*address);
#else
            if (address == NULL)
                {
                // No address available (no target linklayer address)
                if ((aFlags & KRouteAdd_UPDATEONLY) != 0 &&
                    // UPDATEONLY is set for NA, but not for REDIRECT related
                    // target routes (which have ISROUTER set!). Redirect routes
                    // must process the "router bit", even if no link address
                    // is present. NA must drop the packet and NOT process
                    // "router bit".
                    iInterface.iHwAddr.Family() != KAFUnspec)
                    // The link layer requires addresses. If the NA didn't have the
                    // link layer address, drop the packet (RFC 2461, 7.2.5).
                    return notify;
                // ..otherwise, proceed normally to update ISROUTER/ISHOST
                // (but DO LEAVE THE STATE AS INCOMPLETE!!!)
                break;
                }
            else
                iAddress.SetAddress(*address);
            CancelTimer();          // if any active!
#endif
            iState = (aFlags & KRouteAdd_SOLICITED) ? EReachable : EStale;
            if (iIsProbing)
                {
                iIsProbing = 0;
                Interfacer().iScanHolding = 1;
                }
            notify = 1;
            //
            // Send queued packets (only one for now), if any
            //
            if (!iPacket.IsEmpty())
                {
                // Send should not reque this packet, but just in case,
                // use a tmp instead of calling Send(iPacket).
                RMBufChain tmp;
                tmp.Assign(iPacket);
                Send(tmp);
                tmp.Free(); // In case it failed.
                }
            break;  // ..to the IS ROUTER processing
        case EReachable:
        case EStale:
        case EDelay:
        case EProbe:
            // At this point, the state is one of the following and nothing else
            //  EReachable (no timer)
            //  EStale (no timer)
            //  EDelay (timer active)
            //  EProbe (timer active)
            //
            if (address && !iAddress.Match(*address))
                {
                if (aFlags & KRouteAdd_OVERRIDE)
                    {
                    iAddress.SetAddress(*address);
                    iState = EStale;
                    CancelTimer();  // Could have been in Probe or Delay!
                    }
                else
                    {
                    // RFC 2461 7.2.5 does not require SOLICITED to be set for setting
                    // STALE, but in APPENDIX C it looks like it should be required?
                    //
                    if (iState == EReachable /* && (aFlags & KRouteAdd_SOLICITED)*/)
                        {
                        iState = EStale;
                        // no need to cancel timer, EReachable doesn't have one!
                        LOG(LogRoute(KLifetimeForever));
                        }
                    return 0;
                    }
                }
            if (aFlags & KRouteAdd_SOLICITED)
                {
                iState = EReachable;
                CancelTimer();      // Could have been in Probe or Delay!
                iTimeStamp = User::TickCount();
                LOG(if (!notify) LogRoute(KLifetimeForever));
                }
            break;
        }
    //
    // Maintain IS ROUTER status (iIsRouter + iRoute pointers)
    //
    // Cannot have both ISROUTER and ISHOST set!
    ASSERT((aFlags & (KRouteAdd_ISHOST|KRouteAdd_ISROUTER)) != (KRouteAdd_ISHOST|KRouteAdd_ISROUTER));
    ASSERT(Type() == KRouteAdd_NEIGHBOR);   // Only Host Routes should get here
    if (iIsRouter && (aFlags & KRouteAdd_ISHOST) != 0)
        {
        // IS-ROUTER changed from TRUE to FALSE
        iIsRouter = 0;
        iInterface.RouterChanged(this);
        }
    else if (!iIsRouter && (aFlags & KRouteAdd_ISROUTER) != 0)
        {
        // IS-ROUTER changed from FALSE to TRUE
        iIsRouter = 1;
        iInterface.RouterChanged(this);
        notify = 1;
        }
    return notify;
    }

// CIp6Route::Timeout
// ******************
// A timeout expired for this route.

//
// *WARNING*
//  This method is used both for starting the timers and also
//  as the timeout handler. When called to start the timer, iRetry
//  should always be non-zero, and this method MUST NEVER destroy
//  the route instance in such case...!!
//
//      On real timeout, aExpired == 1
//      On start timeout, aExpired == 0
//
// *NOTE*
//  In current version SetTimeout(0) will never call directly
//  expire (Timeout). Thus, it is also safe to use that
//  within this method, if necessary. (one does not need to
//  worry about intance being deleted away in middle of this
//  method).
//
void CIp6Route::Timeout(const TInt aExpired)
    {
    //
    // What was I doing?
    //
    switch (iState)
        {
        case EIncomplete:
            if (iRetry == 0)
                break;
            //
            // RFC-2461 7.2.2 says "...If the source address of the packet prompting the
            // solicitation is the same as one of the addresses assigned to the outgoing
            // interface, that address SHOULD be placed in the IP Source Address of the
            // outgoing solicitation."
            //
            // Assume this src address is stored in iAddress at this point. Must be
            // my own address (not verified here).
                {
#ifdef _LOG
                TLogAddressPrefix tmpsrc(iAddress.Ip6Address());
                TLogAddressPrefix tmpdst(iPrefix);
                Log::Printf(_L("\tIF %u [%S] ROUTE %u Send NS from [%S] for target [%S]"), iInterface.iScope[0], &iInterface.iName, iIndex, &tmpsrc, &tmpdst);
#endif
                iRetry -= 1;
                // unspecified address as dst will use solicited node based on target
                iInterface.SendNeighbors(KInet6ICMP_NeighborSol, NULL, iPrefix, &iAddress.Ip6Address());
                if (iIsProbing && iRetry == 0)
                    {
                    // When probing, set the last timeout specially. This will prevent
                    // reactivating the probing and rate limits it.
                    SetTimer(iInterface.iND.iRateLimitProbingTime);
                    }
                else
                    Interfacer().SetTimerWithUnits(iTimeout, iInterface.iRetransTimer);
                }
            return;
        case EDelay:
            if (iRetry)
                {
                // A request to start the delay
                iRetry = 0;
                SetTimer(iInterface.iND.iDelayFirstProbeTime);
                return;
                }
            // The requested delay has completed...
            iRetry = iInterface.iND.iMaxUnicastSolicit;
            iState = EProbe;
            LOG(LogRoute(KLifetimeForever));
            /* FALL THFROUGH */
        case EProbe:
                {
                if (iRetry == 0)
                    {
                    break;
                    }
                else if (iRetry == 1)
                    {
                    // try broadcast first otherwise it will break the route
                    iInterface.SendNeighbors(KInet6ICMP_NeighborSol, NULL, iPrefix);
                    Interfacer().SetTimerWithUnits(iTimeout, iInterface.iRetransTimer);
                    return;
                    }
                else
                    {
                    iRetry -= 1;
                    iInterface.SendNeighbors(KInet6ICMP_NeighborSol, this, iPrefix);
                    Interfacer().SetTimerWithUnits(iTimeout, iInterface.iRetransTimer);
                    return;
                    }
                }

        case ELoopback:
            // *Note* It is assumed for now, that iPreferred == 0
            // for all ELoopback routes that represent joined multicast
            // groups (=> timer expiration will delete group). Some
            // other logic may be designed later -- msa
#ifdef _LOG
            {
                TLogAddressPrefix tmp(iPrefix, iLength);
                Log::Printf(_L("\tIF %u [%S] ROUTE %u Timeout prefix [%S]"), iInterface.iScope[0], &iInterface.iName, iIndex, &tmp);
            }
#endif
            if (iLifetime.iPreferred > 0)
                {
                // Prefix is changing from preferred to deprecated
                // (pick the value into temporary and zero iPreferred
                // before setting the timeout, just in case timeout
                // expires immediate -- Ìt shouldn't because of delay
                // is > 0, but as it costs nothing to be safe.. -- msa
                const TLifetime value = iLifetime.iPreferred;
                iLifetime.iPreferred = 0;
                iLifetime.iDeprecated = 1;
                SetTimer(value);
                LOG(LogRoute(value));
                return;
                }
            //
            // Prefix lifetime has expired
            //
            iInterface.iSequence++;
            break;
        case EOnlink:
        case EGateway:
        case EReachable:
        case EStale:
        default:
            //
            // The default processing with iRetry==0 is to delete the route
            //
            if (iRetry == 0)
                break;
            iRetry = 0;
            return;

        case EHolding:
            //
            // Holding route timeout handling is different from others:
            // It should poll the held flows and "expire" too old ones!
            //
            const TUint max_time = Interfacer().iMaxHoldingTime;
            //
            // if iMaxHoldingTime == 0, no "expire" happens
            //
            if (max_time > 0)
                {
                const TUint tick_now = User::TickCount();

                TUint tick_limit; // max_time converted into ticks (conversion code block below)
                    {
                    TReal interval;
                    (void)Math::Round(interval, (max_time * 1000000.0) / TickPeriod(), 0);
                    (void)Math::Int((TInt32 &)tick_limit, interval);
                    }
                LOG(Log::Printf(_L("HoldingRoute: max=%d [s], tick_limit = %d, tick_now = %d"), max_time, tick_limit, tick_now));

                TUint longest_hold = 0;
                TUint flows_left = 0; // just 0 = no flows, 1= flows left holding
                TFlowNotifyList list;
                for (CIp6Flow *f = iFlowList; f != NULL; f = f->iNext)
                    {
                    const TUint hold_time = tick_now - f->iTimeStamp;
                    if (hold_time >= tick_limit)
                        list.Insert(*f);
                    else
                        {
                        // Flow still has time to wait left. Keep
                        // track of the longest unexpired hold (for
                        // setting the next timer...)
                        //
                        flows_left = 1;
                        if (hold_time > longest_hold)
                            longest_hold = hold_time;
                        }
                    }
                list.Deliver(KErrInet6NoRoute);
                //
                // (Re)Enable polling timer, if flows still attached
                // [note: longest_hold can be 0, if we just added the first flow]
                if (flows_left)
                    {
                    // ... it might be better to precompute integer approximation of ticks per second
                    // to be used where exact timing is not so essential (instead of this floating
                    // arithmetic). then it would be just integer divisition with truncate:
                    //     max_time = (tick_limit - longest_hold + ticks_per_second + 1) / ticks_per_second;
                    // and
                    //     tick_limit = ticks_per_second * max_time;
                    // -- msa
                    //
                    TReal interval;
                    (void)Math::Round(interval, 1.0 * (tick_limit - longest_hold) * TickPeriod() / 1000000.0, 0);
                    (void)Math::Int((TInt32 &)max_time, interval);
                    LOG(Log::Printf(_L("HoldingRoute: next after %d ticks [= %d+1s]"), tick_limit-longest_hold, max_time));
                    SetTimer(max_time + 1); // add +1 to guarantee > 0, and that surely enough time has passed
                    }
                }
            return;
        }
    //
    // Gets here only if this route should be removed (if possible)
    // (iRetry == 0)
    //
    // If there is any packet waiting for transmission, then report
    // ICMP host/address unreachable for it (currently iPacket is
    // only used for neighbor cache routes). If iPacket is used
    // in some other route types, a test for the route type could
    // be inserted here...
    Interfacer().IcmpSend(iPacket, TIcmpTypeCode(KInet4ICMP_Unreachable, 1, KInet6ICMP_Unreachable, 3));

    if (aExpired)
        iInterface.RemoveRoute(this);   // "this" is DELETED! Beware!
    else
        // Be tricky, and destruct with a delay...
        SetTimer(1);
    }

//
// ****************************
// CIp6Interface Implementation
// ****************************
//

CIp6Interface::CIp6Interface(CIp6Manager &aMgr, TUint aIndex, const TDesC &aName) :
    iInterfacer(aMgr), iName(aName), iTimeout(CIp6InterfaceTimeoutLinkage::Timeout)
    {
    __DECLARE_NAME(_S("CIp6Interface"));
    iScope[0] = aIndex;
    LOG(Log::Printf(_L("\tIF %u [%S] New"), iScope[0], &iName));
    }

// CIp6Interface::Index, Name and Scope
// ************************************
// The basic information about the interface
//
TUint32 CIp6Interface::Index() const
    {
    return iScope[0];
    }

const TDesC & CIp6Interface::Name() const
    {
    return iName;
    }

TUint32 CIp6Interface::Scope(const TScopeType aType) const
    {
    return ((TUint)aType > EScopeType_NET) ? 0 : iScope[aType];
    }

//
// CIp6Interface::UpdateFlowCount
// ******************************
//
void CIp6Interface::UpdateFlowCount(TInt aChange)
    {
    if (aChange == 0)
        return;             // No change!
    iFlows += aChange;
    LOG(Log::Printf(_L("\tIF %u [%S] Attached flows changed from %d to %d"), iScope[0], &iName, iFlows - aChange, iFlows));
    ASSERT(iFlows >= 0);

    if (!iNifIf || !iNifIf->Notify())
        return;             // No interface attached or Nifman
                            // doesn't care--nothing more to do.
    if (iFlows == 0)
        {
        //
        // The flow count for this interface has gone to Zero
        // Notify the NIFMAN that the interface can be closed,
        // if it wants to do so
        //
        LOG(Log::Printf(_L("\tIF %u [%S] CloseRoute"), iScope[0], &iName));
        iNifIf->Notify()->CloseRoute();
        }
    else if (iFlows == aChange)
        {
        //
        // The flow just changed from 0 to something non-zero
        // Notify NIFMAN that interface is back in use.
        LOG(Log::Printf(_L("\tIF %u [%S] OpenRoute"), iScope[0], &iName, aChange));
        iNifIf->Notify()->OpenRoute();
        }
    }

// CIp6Interface::Send
// *******************
// Send a packet to the interface
TInt CIp6Interface::Send(RMBufChain& aPacket, CProtocolBase* aSrc)
    {
    TInt ret = KErrNotReady;

    for (;iNifIf;)  // ** NOT REAL LOOP, ONLY FOR CONVENIENT ERROR EXITS! **
        {
        if (iState == EFlow_HOLD)
            {
            iHoldQueue.Append(aPacket);
            LOG(Log::Printf(_L("\tIF %u [%S] Send holding packets"), iScope[0], &iName));

            return 0;       // Try to request "no more packets"
            }
        // Because Send will consume the packet, must pick up the parameters
        // for the packet activity notification before it!
        RMBufSendInfo *const info = RMBufSendPacket::PeekInfoInChain(aPacket);
        if (info == NULL)
            break;  // ...should really never happen, but just in case!
        const TUint bytes = (TUint)info->iLength;
        const TBool reset_timer = (info->iFlags & KIpKeepInterfaceUp) != 0;
        
        // If destination at this point is still IPv4 or IPv6 multicast address
        // set the KIpBroadcastOnLink flag. Note, that the flag is not cleared
        // becuase this bit may have been correctly set earlier for broadcast
        // addresses (which are not recognized as multicast here!).
        if (TInetAddr::Cast(info->iDstAddr).IsMulticast())
            info->iFlags |= KIpBroadcastOnLink;
        LOG(PktLog(_L("\tIF %u [%S] SEND prot=%d src=%S dst=%S len=%d"), *info, iScope[0], iName));
        ret = iNifIf->Send(aPacket, aSrc);
        if (ret <= 0)
            {
            // The Send returns are "officially" only
            //  1 = Packet Accepted ok
            //  0 = Packet Accepted, don't send more before StartSending
            // However, some may return < 0 to signal an error (and not
            // follow it up with start sending). So, the following logic
            // is implemented:
            //  Returned 1, no change in interface state or flow [does not enter this branch of code]
            //  Returned 0, interface and flows are set to HOLD
            //  Returned < 0, no change in interface state, error is
            //  reported to the flows!
            LOG(Log::Printf(_L("\tIF %u [%S] NIF Send returned HOLD (%d)"), iScope[0], &iName, ret));
            TInt state = ret;
            if (state == 0)
                iState = state = EFlow_HOLD;    // Convert "0" to HOLD state
            NotifyFlows(state);
            }
        if (iNifIf->Notify())
            (void)iNifIf->Notify()->PacketActivity(EOutgoing, bytes, reset_timer);

        // If the packet ownership is not taken at this point, someone is not
        // working as specified (Process() or Send()).
        ASSERT(aPacket.IsEmpty());
        break;  // ** MUST TERMINATE THE "FAKE FOR"-LOOP ALWAYS!
        }
    aPacket.Free(); // If nobody took it, release it!
    return ret;
    }

//
// CIp6Interface::UpdateMulticast
// ******************************
/**
//  Maintain multicast membership status at low level (add/remove the
//  route and update the interface; must not do MLD!)
//
// @param   aMulticast speficies the multicast group to join or leave
// @param   aLifetime, = 0 => leave group, != 0, join group. (the life time
// of multicast group is controlled by join/leaves, and the actual non-zero
// value is not used for anything else except as a flag).
//
// @returns
// @li  KErrNone, if join or leave succeeds,
// @li  KErrNotFound, if leave for non-existent group
// @li  KErrNoMemory, if group cannot be joined due to memory allocation failures
// @li  any other error, if NIF rejects the join .
*/
TInt CIp6Interface::UpdateMulticast(const TIp6Addr &aMulticast, const TLifetime aLifetime)
    {
    //
    // Construct Join/leave option buffer
    //
    TPckgBuf<TIp6Mreq> opt;
    opt().iAddr = aMulticast;
    opt().iInterface = iScope[0];
    //
    // NIF needs to be notified only if it exists, and if the scope
    // is larger than node-local. NIF can use this notify
    // to maintain multicast filters for the interface,
    // if it supports such feature.
    const TBool notify_nif = (iNifIf != NULL) && (aMulticast.Scope() > KIp6AddrScopeNodeLocal);
    //
    // Get the multicast route entry representing the group
    //
    CIp6Route *route = GetRoute(aMulticast, 128, KRouteAdd_MYPREFIX|KRouteAdd_UPDATEONLY);
    if (route == NULL)
        {
        //
        // The multicast group does not exist yet.
        //
        if (aLifetime == 0)
            return KErrNotFound;    // not joined to this group, cannot leave.

        if (notify_nif)
            {
            const TInt err = iNifIf->Control(KSolInetIp, KSoIp6JoinGroup, opt);
            if (err < 0 && err != KErrNotSupported)
                return err;             // Interface explicitly rejects the join.
            }
        route = GetRoute(aMulticast, 128, KRouteAdd_MYPREFIX);
        if (route == NULL)
            return KErrNoMemory;
        // note: above GetRoute just created a special multicast "myprefix"
        // route entry. This is exactly same as joining to the multicast
        // group. Could consider generating the NotifyMulticastEvent from
        // the "address route creation event", and not sending the route
        // event in such case at all (implementation of joined multicast
        // groups as "multicast my-address" entries is internal implementation
        // issue). -- msa
        NotifyMulticastEvent(EventTypeAdd, aMulticast, aLifetime);
        return KErrNone;
        }

    // Join or Leave to an existing group. The iLifetime.iCount
    // counts the *ADDITIONAL* users after the first one. With one
    // user, the iCount == 0!

    ASSERT(route->iIsMulticast);

    if (aLifetime)
        {
        //
        // Additional join to an existing multicast group
        //
        route->iLifetime.iCount++;
        }
    else if (route->iLifetime.iCount == 0)
        {
        //
        // Last user left the group
        //
        NotifyMulticastEvent(EventTypeDelete, aMulticast, aLifetime);
        // note: generates remove of "multicast myprefix" entry, above
        // multicast notify could be generated from that.. -- msa
        RemoveRoute(route);
        if (notify_nif)
            (void)iNifIf->Control(KSolInetIp, KSoIp6LeaveGroup, opt);
        }
    else
        {
        //
        // Non-last user left the group
        //
        route->iLifetime.iCount--;
        }
    return KErrNone;
    }

// CIp6Interface::DoBind
// *********************
/**
// Finalizes the connection between the network and interface, assuming
// the CNifIfBase instance is connected to the interface
//
// @note
//      Actually combined "close/rebind/open" method, which closes any
//      existing bindings, and sets the new binding (if provided).
//      There should probably be separate Open/Close methods.
*/
TInt CIp6Interface::DoBind(CIp6NifUser *aNifUser, CNifIfBase *aIf)
    {
    ASSERT(aNifUser != NULL);

    iNifUser = aNifUser;    // Do this always!

    if (iNifIf == aIf)
        return KErrNone;    // Do nothing, already bound to this (or both NULL)!

    Reset();                // ...back to initial state!
    if (aIf)
        {
        //
        // A new NIF interface to bind
        //
        aIf->Open();
        iNifIf = aIf;

        if (aNifUser->iNetwork)
            {
            TRAPD(err, aIf->BindL(aNifUser->iNetwork->Protocol()));
            if (err != KErrNone)
                {
                //
                // Bind failure
                //
                iNifIf = NULL;
                aIf->Close();
                }
            else
                {
                // ...notify network layer, in case any hook is interested...
                // [The above BindL may have caused StartSending and other
                // large actions to happen, so just to be safe, need to
                // check that iNetwork is still attached... -- msa]
                if (iNifUser->iNetwork)
                    iNifUser->iNetwork->InterfaceAttached(iName, iNifIf);
/*              if (iFlows > 0 && aIf->Notify())
                aIf->Notify()->OpenRoute();*/
                }
            return err;
            }
        else
            {
            // Interface instance exists, but there is no network to
            // connect. What to do? Should be left into pending state
            // until the network registers and then call the BindL?
            // -- msa
            User::Panic(_L("DEBUG"), 0);
            }
        }
    return KErrNotReady;
    }


//
// MakeFullAddress
// ***************
static void MakeFullAddress(TIp6Addr &aPrefix, TUint aLength, const TUint8 *aId, TInt aIdLen)
    /**
    * Combine hardware id and prefix into single address.
    *
    * If ID is longer than available room in address, the extra bits
    * from the start of the ID are ignored.
    *
    * If ID is shorter than available room in address, the unspecified
    * bits in the ID part will be ZERO.
    *
    * @retval aPrefix The prefix part of the address, and final address on return.
    * @param aLength The prefix length in BITS.
    * @param aId The Id value
    * @param aIdLen The id length in full bytes.
    */
    {
    TInetAddr msk;
    msk.PrefixMask(aLength);

    // Initialize properly aligned ID
    // (copy id to the end of TIp6Addr and zero remaining)
    TIp6Addr id;
    TInt i = sizeof(id.u.iAddr8);
    do
        {
        id.u.iAddr8[--i] = --aIdLen >= 0 ? aId[aIdLen] : (TUint8) 0;
        }while(i > 0);
    // Added explicit cast (TUint8) above to suppress WINS compiler warning

    // Merge id with prefix
    const TIp6Addr &m = msk.Ip6Address();

/*    for (TInt j = 4; --j >= 0;)
        aPrefix.u.iAddr32[j] = (aPrefix.u.iAddr32[j] & m.u.iAddr32[j]) | (id.u.iAddr32[j] & (~m.u.iAddr32[j]));
*/  
    // We don't loop. Just do the calculation.
    aPrefix.u.iAddr32[3] = (aPrefix.u.iAddr32[3] & m.u.iAddr32[3]) | (id.u.iAddr32[3] & (~m.u.iAddr32[3]));
    aPrefix.u.iAddr32[2] = (aPrefix.u.iAddr32[2] & m.u.iAddr32[2]) | (id.u.iAddr32[2] & (~m.u.iAddr32[2]));
    aPrefix.u.iAddr32[1] = (aPrefix.u.iAddr32[1] & m.u.iAddr32[1]) | (id.u.iAddr32[1] & (~m.u.iAddr32[1]));
    aPrefix.u.iAddr32[0] = (aPrefix.u.iAddr32[0] & m.u.iAddr32[0]) | (id.u.iAddr32[0] & (~m.u.iAddr32[0]));
    }


//
// CIp6Interface::Elapsed
// **********************
TLifetime CIp6Interface::Elapsed(const TTime &aStamp) const
    {
    TTimeIntervalSeconds elapsed;
    aStamp.SecondsFrom(iTimeStamp, elapsed);
    TInt elapsedInt = elapsed.Int();
    // Return 0, if time is earlier than time stamp (clock turned back?)
    return (TLifetime) (elapsedInt < 0 ? 0 : elapsedInt);
    }

//
// CIp6Interface::SetPrefix()
// **************************
//
// *NOTE* to delete a prefix, aLifeTime == 0 (and set aForce non-ZERO, if 2h safeguard
// is to be disabled)
//
void CIp6Interface::SetPrefix(const TIp6Addr &aPrefix, const TUint aLength, const TInt aForce, const TLifetime aLifetime, const TLifetime aPreferred)
    {
    ASSERT(aLength <= 128);
#ifdef SYMBIAN_TCPIPDHCP_UPDATE      
    #ifdef _DEBUG
        LOG(Log::Printf(_L("<>\tCIp6Interface::SetPrefix() lifetime = (%d)"),aLifetime));
    #endif
#endif // SYMBIAN_TCPIPDHCP_UPDATE          
    if (TIp46Addr::Cast(aPrefix).IsMulticast() ||
        // ...basicly, all prefixes on the interface should have the same
        // length (128 - idlength). However, make an exception for special
        // interfaces with with idlength = 0 (like 6to4). [Note: idlength
        // is not directly stored, but computed as (128-iAddress.iPrefix)]
        (iAddress.iPrefix != 128 && iAddress.iPrefix != aLength && aLength != 128))
        return;     // silently ignore all attemps to put in funny prefixes!

    CIp6Route *prefix = GetRoute(aPrefix, aLength,
        // Set UPDATEONLY, if lifetime is ZERO (don't create!)
        KRouteAdd_MYPREFIX | (aLifetime > 0 ? 0 : KRouteAdd_UPDATEONLY)
        );
    if (prefix == NULL)
        return;     // OOPS, out of memory or something...

    // Lifetime is maintained relative to the iTimeStamp of the interface
    TTime stamp;
    stamp.UniversalTime();
    const TLifetime current_time = Elapsed(stamp);
    // Compute old remaining lifetime (StoredLifetime in RFC 2462/5.5.3)

    TLifetime storedLifetime = (prefix->iLifetime.iStored > current_time) ?
        prefix->iLifetime.iStored - current_time : 0 /* 0 = expired*/;

    // The logic we are counting on here: if storedLifetime above has something set,
    // this must have been an earlier existing prefix, hence EventTypeModify.
    // Otherwise it is a new prefix.
    TUint eventtype = (storedLifetime ? EventTypeModify : EventTypeAdd);

    const TUint two_hours = 7200; //2 * 60 * 60;  in seconds
    if (aForce || aLifetime > two_hours || aLifetime > storedLifetime)
        {
        storedLifetime = aLifetime;
        }
    else if (storedLifetime >= two_hours || aLifetime >= storedLifetime)
        {
        storedLifetime = two_hours;
        }
    //
    // If after above, the  storedLifetime > 0, then this prefix should
    // still remain.
    if (storedLifetime > 0)
        {

        // Set new stored time, but watch out for overflow

        if (storedLifetime > KLifetimeForever - current_time)
            prefix->iLifetime.iStored = KLifetimeForever;
        else
            prefix->iLifetime.iStored = current_time + storedLifetime;
        //
        // The iPreferred contains the duration of the "deprecated"
        // time before true expiration occurs (in seconds)
        //
        //   0 <= iPreferred <= storedLifetime
        //
        if (aPreferred < storedLifetime)
            prefix->iLifetime.iPreferred = storedLifetime - aPreferred;
        else
            prefix->iLifetime.iPreferred = 0;
        prefix->iLifetime.iDeprecated = 0;

        // Set active timer only if the lifetime is less than "forever".
        const TLifetime life = storedLifetime - prefix->iLifetime.iPreferred;
        if (life < KLifetimeForever)
            prefix->SetTimer(storedLifetime - prefix->iLifetime.iPreferred);
        else
            {
            prefix->CancelTimer();
            }
#ifdef SYMBIAN_TCPIPDHCP_UPDATE        
        if(iGlobalflag)
            {
            //If the global flag is set(this will be set when the 'A' flag is set in the prefix of RA)
            //then do DAD for the Global address
            #ifdef _DEBUG
                LOG(Log::Printf(_L("<>\tCIp6Interface::SetPrefix() glbal flag is set")));
            #endif
            PerformDADForGlobalAddress(aPrefix,aLength);
            iGlobalflag=EFalse;
            }
#endif // SYMBIAN_TCPIPDHCP_UPDATE
        // Send notification about new prefix to the event service
        NotifyAddressEvent(eventtype, aPrefix, aLength, prefix, iAddress);
        }
    else
        {
        // Send notfification about deleted prefix to the event service
        NotifyAddressEvent(EventTypeDelete, prefix->iPrefix, prefix->iLength,
                prefix, iAddress);

        iSequence++;
        RemoveRoute(prefix);
        }
    }


void CIp6Interface::NotifyAddressEvent( TUint aEventType,
                                        const TIp6Addr &aPrefix,
                                        const TUint aLength,
                                        const CIp6Route *aPrefixEntry,
                                        const TIp6AddressInfo &aAddress ) const
    /**
    * Send notification about changed address to event manager.
    * @param aEventType     The event type code (see in_bind.h).
    * @param aPrefix        Prefix part of the address.
    * @param aLength        Prefix length.
    * @param aPrefixEntry   Pointer to the KRouteAdd_MYPREFIX entry in routing table
    *                       NULL indicates this is was an addition of ID part of the
    *                       address.
    * @param aAddress       Information about the ID part of the address.
    */
    {
    CIp6Manager *const mgr = &Interfacer();
    
    // If there is no event manager, or if there are no registered listeners, we can exit
    // the function right away
    if (!mgr->EventManager())
        {
        return;
        }

    if (mgr->EventManager()->IsEmpty(EClassAddress))
        {
        return;
        }
  
    TInetAddressInfo info;
    info.iAddress = aPrefix;
    if (aPrefixEntry)
        {
        MakeFullAddress(info.iAddress, aLength,
            iAddress.iId.u.iAddr8, sizeof(iAddress.iId.u.iAddr8));
        }
    
    TScopeType st = (TScopeType) (aPrefix.Scope() - 1);
    info.iScopeId = Scope(st);
    info.iPrefixLen = (TUint8) aLength;
    info.iInterface = Index();
    
    TTime stamp;
    stamp.UniversalTime();
    const TLifetime current_time = ElapsedUnits(aAddress.iCreated, stamp);

    TLifetime plt, vlt;

    if (aAddress.iPLT != KLifetimeForever)
        {
        plt = (aAddress.iPLT > current_time) ?
            aAddress.iPLT - current_time : 0 /* 0 = expired*/;
        }
    else
        {
        plt = KLifetimeForever;
        }

    if (aAddress.iVLT != KLifetimeForever)
        {
        vlt = (aAddress.iVLT > current_time) ?
            aAddress.iVLT - current_time : 0 /* 0 = expired*/;
        }
    else
        {
        vlt = KLifetimeForever;
        }

    info.iPrefLifetime = plt / TIMER_UNIT;
    info.iValidLifetime = vlt / TIMER_UNIT;
    info.iGenerations = aAddress.iGenerated;
    info.iNS = aAddress.iNS;
    info.iState = (TUint8) aAddress.AddressState();
    info.iType = (TUint8) aAddress.AddressType();
    info.iFlags = 0;

    if (aPrefixEntry == NULL)
        {
        info.iFlags |= TInetAddressInfo::EF_Id;
        if (plt == 0)
            {
            info.iFlags |= TInetAddressInfo::EF_Deprecated;
            }
        }
    else
        {
        info.iFlags |= TInetAddressInfo::EF_Prefix;
        if (aPrefixEntry->iLifetime.iDeprecated)
            {
            info.iFlags |= TInetAddressInfo::EF_Deprecated;
            }
        }

    mgr->EventManager()->Notify(EClassAddress, aEventType, &info);  
    }

//
// CIp6Interface::SetReachableTime
// *******************************
/**
// Compute the in use value for the reachable time. The
// value is stored in TickCount units!
//
// The iND is assumed to containt the base value in milliseconds
*/
void CIp6Interface::SetReachableTime()
    {
    const TUint tick = TickPeriod();

    TReal factor = iND.iMinRandomFactor + Math::FRand(Interfacer().iSeed) * (iND.iMaxRandomFactor - iND.iMinRandomFactor);
    (void)Math::Round(factor, (iND.iReachableTime * factor * 1000.0) / tick, 0);
    (void)Math::Int((TInt32 &)iReachableTime, factor);
    LOG(Log::Printf(_L("\tIF %u [%S] ReachableTime base=%d [ms], new time = %d [tics = %ds]"), iScope[0], &iName, iND.iReachableTime, iReachableTime, (TInt)((iReachableTime * tick) / 1000000)));
    }

// CIp6Interface::SetRetransTimer
// ******************************
/**
// Compute the in use value for the retrans timer. The value
// is stored in internal timer units.
//
// The iND is assumed to contain the base value in milliseconds
*/
void CIp6Interface::SetRetransTimer()
    {
    iRetransTimer = CIp6Manager::TimerUnits(iND.iRetransTimer, 1000);
    if (iRetransTimer == 0)
        iRetransTimer = 1;  // Never allow ZERO!
    LOG(Log::Printf(_L("\tIF %u [%S] RetransTimer base=%d, value = %u/%u s"),
        iScope[0], &iName, iND.iRetransTimer, iRetransTimer, TIMER_UNIT));
    }

//
// CIp6Interface::SelectSource
// ***************************
/**
// Select and set the source address matching the
// specified destination address.
//
// @retval  aSrc    The selected source address.
// @param   aDst    The destination.
//
// @return
//  @li route pointer (MYPREFIX),   if source address is fully specified
//  @li NULL, if source address is not know or incomplete
*/
CIp6Route *CIp6Interface::SelectSource(TIp6Addr &aSrc, const TIp6Addr &aDst) const
    {
    CIp6Route *best_match = NULL;
    TInt best_score = KMinTInt;
    const TUint scope = aDst.Scope();       // Prefetch destination scope
    const TBool is_ip4 = aDst.IsV4Mapped(); // Prefetch destination type

    // If destination is my own address, the source address will
    // be the destination address. Prepare for detecting this
    // by prefetching the matching ID part, if any exists.
    //
    const TIp6AddressInfo *myid = IsMyId(aDst);
    if (myid && !myid->IsAssigned())
        myid = NULL;
    //
    // Choose the prefix part
    //
    for (CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
        {
        if (!rt->IsMyPrefix())
            continue;   // Not a "my prefix" entry, get next.
        //
        // The route entry represents a prefix entry
        //
        const TInt match = rt->iPrefix.Match(aDst) - rt->iLength;
        if (myid && match >= 0)
            {
            // A quick hack to prevent choosing proxy or anycast
            // address as a source address (they are currently
            // entered as 128 bit adressess and "aDst ~ myid ~
            // prefix"... -- msa
            if (!myid->IsNormal())
                continue;
            // The ID part of the destination has already matched,
            // and now a full MYPREFIX matched => the destination
            // is my own address on this interface,
            // return aDst as a source!
            aSrc = aDst;
            return rt;
            }
        // For other than own addresses, consider the prefix only
        // if the primary id length and this prefix have a compatible
        // length... [somewhat kludgy way to prevent 2002:7f::/24 from
        // being chosen over 2002:ip4::ip4/128 .. --msa]
        // (make an exception for full 128 bit addresses)
        if (rt->iLength < 128 && rt->iLength != iAddress.iPrefix)
            continue;
        // IPv4 addresses are currently stored as full 128 bit
        // addresses, the IPv4 mapped test is only needed for them.
        if (is_ip4 != (rt->iLength == 128 && rt->iPrefix.IsV4Mapped()))
            continue;   // Mismatched IPv4 / IPv6!

        // Prefer matches that cover full prefix, thus use the difference
        // between matched bits and prefix length as a criteria, with
        // additional criteria that if the match is longer than prefix,
        // the comparison value will be the prefix length.
        // -- msa
        TInt weight = match >= 0 ? rt->iLength : match;
        if (rt->iLifetime.iDeprecated)
            // A deprecated prefix. Consider it, but decrease it's comparison
            // value by 128, so that it won't be selected if there is even one
            // non-deprecated choice available.
            weight -= 128;

        const TUint src_scope = rt->iPrefix.Scope();
        if (src_scope < scope)
            weight -= 256;  // use src with smaller scope only as last ditch.
            
        if (weight > best_score || (weight == best_score && src_scope == scope))
            {
            best_score = weight;
            best_match = rt;
            }
        }
    if (!best_match)
        return NULL;    // Cannot find source address (no prefix!)

    // Kludge: if the prefix is 128 bits, use it as is for source address!
    // However, there *SHOULD* be a corresponding address entry with
    // id->iPrefix==0
    if (best_match->iLength == 128)
        {
        for (const TIp6AddressInfo *id = &iAddress; ;id = &id->iNext->iInfo)
            {
            if (id->iPrefix == 0 && id->iId.IsEqual(best_match->iPrefix))
                {
                if (!id->IsAssigned())
                    return NULL;
                aSrc = best_match->iPrefix;
                return best_match;
                }
            if (id->iNext == NULL)
                break;
            }
        return NULL;
        }
    //
    // Choose the id part
    //
    myid = NULL;
    for (const TIp6AddressInfo *id = &iAddress; ;id = &id->iNext->iInfo)
        {
        if (id->IsAssigned() && id->IsNormal())
            {
            if (best_match->iLength <= id->iPrefix)
                {
                //
                // iGenerated is non-zero if id is randomly generated.
                // This may be used in privacy address.
                // Test
                //  iGenerated == 0, prefer random id
                //  iGenerated != 0, prefer non-random id.
                //
                if (myid == NULL)
                    myid = id;
                else if (myid->iGenerated != 0)
                    myid = id;
                }
            }
        if (id->iNext == NULL)
            break;
        }
    if (myid)
        {
        aSrc = best_match->iPrefix;
        MakeFullAddress(aSrc, best_match->iLength, myid->iId.u.iAddr8, sizeof(myid->iId.u.iAddr8));
        return best_match;
        }
    return NULL;
    }

// CIp6Interface::UpdateIdRoutes
// *****************************
/**
// Maintains internal, address related route entries.
//
// Somewhat "ad hoc" code: maintain "solicited" node
// multicast addresses on the Route list for all id's.
// (the ad hoc part is in how this feature is activated
// by condition: 0 < aPrefix < 128, and only for IPv6.
//
// Also, maintain host loopback routes for configured
// alias addresses.
*/
void CIp6Interface::UpdateIdRoutes(const TIp6AddressInfo &aId, const TLifetime aLifetime)
    {
    if (!aId.IsSet())
        return; // Nothing to do with unspecified address.
    if (aId.iPrefix == 0 && !aId.IsProxy())
        {
        const TUint flags = aId.IsAnycast() ? CIp6Route::EAnycast : CIp6Route::ELoopback;
        (void)GetRoute(aId.iId, 128, flags, NULL, &aLifetime);
        }

    // IPv6, each own id needs to recognize the corresponding solicited
    // node multicast destination...
    if (!aId.iId.IsV4Mapped() && aId.iPrefix < 128)
        {
        // ..or, should require that the Id part is at least
        // 24 bits long, before solicited node is generated
        // (iPrefix <= 104) -- msa
        // Delete or Create entry (depending on aLifetime)
        UpdateMulticast(TSolicitedNodeAddr(aId.iId), aLifetime);        
        }
        
    // If this ID is being removed make sure all routes using this ID as a source
    // address or prefix are also removed or else SelectSource will be confused if
    // it tries to reuse a route which no longer has a corresponding source address.
    if( aLifetime == 0 )
        {
        CIp6Manager &mgr = Interfacer();
        CIp6Route *rt;

        for (CIp6Route **h = &iRouteList; ; )
            {
            rt = *h;
            
            if( !rt )
                {
                // Stop.
                break;
                }
            else
                {
                if( aId.MatchExactly( rt->iAddress.Ip6Address() ) || aId.MatchExactly( rt->iPrefix ) )
                    {
                    // Remove the the route from the current position.
                    *h = rt->iNext;
                        
                    LOG(rt->LogRoute(0));

                    //
                    // Delete matching route, if lifetime is ZERO
                    //
                    if (rt->iIsRouter)
                        {
                        rt->iIsRouter = 0;
                        RouterChanged(rt);
                        }
                    //
                    // If any flows are attached to the route that is being removed,
                    // move them all into the holding route with PENDING status.
                    //
                    // Note: holding is *ALWAYS* non-NULL. The only time holding
                    // can be NULL, is when it is being created by InitL(), and in
                    // that case GetRoute() *NEVER* gets into this branch! -- msa
                    //
                    mgr.MoveToHolding(*rt);

                    // Send notification about removed route to event manager
                    NotifyRouteEvent(EventTypeDelete, rt);

                    delete rt;
                    }
                else
                    {
                    h = &rt->iNext;
                    }
                }
            }
        }
    }


void CIp6Interface::NotifyMulticastEvent(TUint aEventType, const TIp6Addr &aMulticast, const TLifetime aLifetime) const
{
    CIp6Manager *const mgr = &Interfacer();

    // If there is no event manager, or if there are no registered listeners, we can exit
    // the function right away
    if (!mgr->EventManager())
        return;

    if (mgr->EventManager()->IsEmpty(EClassMulticast))
        return;
  
    TInetMulticastInfo info;
    info.iMulticastGroup = aMulticast;
    info.iInterface = iScope[0];
    info.iLifetime = aLifetime;

    mgr->EventManager()->Notify(EClassMulticast, aEventType, &info);
}


//
// CIp6Interface::SetId
// ********************
/**
// @retval  aId The address/id to be modified.
// @param   aAddr The new address.
// @param   aPrefix The length of the prefix part.
// @param   aAddressType Type of the address.
// @return
// @li 0, if ID was not changed
// @li 1, if ID changed
//
//  Although, the main point is the id-part, it is assumed that the
//  value stored as ID is also *ALWAYS* a valid full address for this
//  interface (some code may depend on it!)
*/
TInt CIp6Interface::SetId(TIp6AddressInfo &aId, const TIp6Addr &aAddr, const TInt aPrefix, const TInt aAddressType)
    {
    // Should this also check whether address type is same?
    // Changing just type does not work with this code!
    // -- msa 24.10.2003
    if (aPrefix < 0 || aPrefix > 128)
        return 0;       // Invalid length, do nothing!
    if (TIp46Addr::Cast(aAddr).IsMulticast())
        return 0;       // A multicast address cannot be my own.
    if (aId.IsSet() && aPrefix == aId.iPrefix && aAddr.IsEqual(aId.iId))
        {
		// Id has not changed but expecting some change in any of 
		// the other interface fields. So raising a Interface change event
		// for the subscribers to keep themselves updated
		
        // Send notification to the event service
        NotifyInterfaceEvent(EventTypeModify);
        return 1;
        }

    UpdateIdRoutes(aId, 0);     // Remove old route (if needed)
    aId.iId = aAddr;
    aId.iPrefix = (TUint8)aPrefix;
    aId.SetInitial(NeedsND());
    aId.SetType(aAddressType);
    aId.iNS = 0;
    aId.iCreated.UniversalTime();
    aId.iVLT = KLifetimeForever;
    aId.iPLT = KLifetimeForever;
    UpdateIdRoutes(aId, KLifetimeForever);  // Add new route (if needed)

    // Send notification about the new address to the event service
    NotifyAddressEvent(EventTypeAdd, aAddr, aPrefix, NULL, aId);

    // ..should activate Timeout for DAD detection!? -- mas

    return 1;   // Id has been changed
    }

TInt CIp6Interface::AddId(const TSockAddr& aId)
    {
    // Should get the true length of the id part. The code below works only
    // for id with length of full bytes... -- msa
    const TInt prefix = 128 - (*(TSockAddr *)&aId).GetUserLen() * 8;

    // Setting id of length ZERO (prefix == 128) does nothing
    // and returns "nothing changed"...
    if (prefix >= 0 /*&& prefix < 128*/)
        {
        TIp6Addr local(KInet6AddrLinkLocal);
        //
        // Add "ONLINK" route for all link locals
        //
        (void)GetRoute(local, 10, KRouteAdd_ONLINK);

        MakeFullAddress(local, prefix, aId.Ptr(), aId.Length());
        // Need to set ID before prefix (SetPrefix does some checks that
        // require a known id length on the interface...)
        TInt ret = AddId(local, prefix);
        // AddId is only used for IPv6 interfaces, add the link local "prefix"
        // here. [totally add hoc rule: set the link local prefix only if
        // idlen > 0!]
        SetPrefix(local, prefix, 1);
        return ret;
        }
    return 0;
    }
//
// CIp6Interface::AddId
// ********************
/**
// This will define the primary ID, if not yet specified, or adds
// a new id.
*/
TInt CIp6Interface::AddId(const TIp6Addr &aId, const TInt aPrefix, const TInt aAddressType, const TBool aForcePrimary)
    {
    ASSERT(!aId.IsUnspecified());
    //
    // First would need to check if any of the id's match for this address
    //
    TIp6AddressInfo *prevID = NULL, *id;
    for (id = &iAddress; ;prevID = id, id = &id->iNext->iInfo)
        {
        // Compare id's as full addresses.
        // Note: here comparing just address is correct. Address type
        // can only be one of the following: normal, proxy, anycast, etc. 
        if (id->IsSet() && id->iId.IsEqual(aId))
            {
            if( aForcePrimary && !id->IsPrimary() )
                {
                // We need to move this address into the primary slot.
                CIp6Address *oldPrimary = new CIp6Address;
                if (oldPrimary == NULL)
                    return 0;
                oldPrimary->iInfo = iAddress;
                oldPrimary->iInfo.SetPrimary( EFalse );
                
                iAddress = *id;
                iAddress.SetPrimary( ETrue );
                iAddress.iIpv4LinkLocal = EFalse; // reset this flag in case a link local formerly occupied this slot
                iAddress.iNext = oldPrimary;
                
                if( prevID )
                    {
                    prevID->iNext = id->iNext;
                    }
                delete id;

                id = &iAddress;
                }
                
            break;
            }
        if (id->iNext == NULL)
            {
            // None matched
            //
            if (!iAddress.IsSet())
                {
                // Primary ID slot is still empty, use it!
                id = &iAddress;
                iAddress.SetPrimary( ETrue );

                break;
                }
                
            //
            // Primary id slot is used, need to create a new entry
            //
            if( aForcePrimary )
                {
                // We need to move this address into the primary slot.
                CIp6Address *oldPrimary = new CIp6Address;
                if (oldPrimary == NULL)
                    return 0;
                oldPrimary->iInfo = iAddress;
                oldPrimary->iInfo.SetPrimary( EFalse );
                
                iAddress.SetPrimary( ETrue );
                iAddress.iIpv4LinkLocal = EFalse; // reset this flag in case a link local formerly occupied this slot
                iAddress.iNext = oldPrimary;

                id = &iAddress;
                }
            else
                {
                CIp6Address *p = new CIp6Address;
                if (p == NULL)
                    return 0;
                p->iInfo.iNext = iAddress.iNext;

                iAddress.iNext = p;

                id = &p->iInfo;
                }

            break;
            }
        }
    return SetId(*id, aId, aPrefix, aAddressType);
    }

// CIp6Interface::GetId
// ********************
// Locate Id block by address
TIp6AddressInfo* CIp6Interface::GetId(const TIp6Addr &aAddr) const
    {
    for (const TIp6AddressInfo *id = &iAddress; ;id = &id->iNext->iInfo)
        {
        // Compare id's as full addresses
        if (id->IsSet() && aAddr.IsEqual(id->iId))
            // Throw away 'const' -- hopefully this does not cause any
            // compiler problems... -- msa
            return (TIp6AddressInfo *)id;
        if (id->iNext == NULL)
            break;  // None found!
        }
    return NULL;
    }

//
// CIp6Interface::RemId
// ********************
// Remove specified Id.
TInt CIp6Interface::RemId(const TIp6AddressInfo *const aId)
    {
    if (aId == NULL) // For convenience, allow call with NULL ptr.
        return KErrNotFound;

    UpdateIdRoutes(*aId, 0);        // Remove old route (if needed)
    ++iSequence;                    // Always increment (does not hurt, even if no deletion actually happens)

    // Note: event is generated, even if no matching address is found
    NotifyAddressEvent(EventTypeDelete, aId->iId, aId->iPrefix, NULL, *aId);

    if (aId == &iAddress)
        {
        //
        // Removing the primary Id is a special case
        //
        if (iAddress.IsTentative())
            iAddress.SetDuplicate();
        else
            iAddress.SetNoAddress();
        return KErrNone;
        }

    CIp6Address **h, *p;
    for (h = &iAddress.iNext; (p = *h) != NULL; h = &p->iInfo.iNext)
        if (aId == &p->iInfo)
            {
            *h = p->iInfo.iNext;
            delete p;
            return KErrNone;
            }
    return KErrNotFound;
    }


//
// GetIp4Config
// ************
/**
// A simple code that initializes the TSoInetIfConfig structure
// properly and performs the query to the interface. Not a general
// method, but just way to minimize code size (used from different
// places)
*/
static TInt GetIp4Config(CNifIfBase *aIf, TPckgBuf<TSoInetIfConfig> &cfg)
    {
    TSoInetIfConfig *const c = &cfg();

    c->iFamily = KAfInet;

    // Ip4 interfaces are picky about the family constant,
    // and TInetAddr initialize into family KAfInet6.
    // The following will turn them into KAfInet
    // -- msa
    c->iConfig.iAddress.SetAddress(0);
    c->iConfig.iNetMask.SetAddress(0);
    c->iConfig.iDefGate.SetAddress(0);
    c->iConfig.iBrdAddr.SetAddress(~0U);
    c->iConfig.iNameSer1.SetAddress(0);
    c->iConfig.iNameSer2.SetAddress(0);
    return aIf ? aIf->Control(KSOLInterface, KSoIfConfig, cfg) : KErrNotFound;
    }


// CIp6Interface::IsMyId
// *********************
TIp6AddressInfo *CIp6Interface::IsMyId(const TIp6Addr &aAddr) const
    {
    //
    // Find longest matching and usable id (the length of the id is "128 - iPrefix").
    //
    const TIp6AddressInfo *best_id = NULL;
    for (const TIp6AddressInfo *id = &iAddress; ;id = &id->iNext->iInfo)
        {
        if ((best_id == NULL || best_id->iPrefix < id->iPrefix) &&
            id->IsSet() &&
            id->Match(aAddr))
            {
            best_id = id;
            }
        if (id->iNext == NULL)
            break;
        }
    // Throw away 'const'
    return (TIp6AddressInfo *)best_id;
    }

// CIp6Interface::IsMyPrefix
// *************************
CIp6Route *CIp6Interface::IsMyPrefix(const TIp6Addr &aAddr, const TIp6AddressInfo &aId) const
    {
    for (const CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
        {
        if (!rt->IsMyPrefix())
            continue;
        // The prefix is examined, only if
        //  aId.iPrefix == 128 (0x80,  special, id length is 0, any prefix will do), or
        //  aId.iPrefix == 0 (0x00, id is full address, just pick any matching prefix)
        //  aId.iPrefix == rt->iLength (otherwise, id and prefix must have matching lengths).
        if ((aId.iPrefix & 0x7f) != 0 && aId.iPrefix != rt->iLength)
            continue;   // id must be zero length or match the prefix length.
        if (rt->iPrefix.Match(aAddr) >= rt->iLength)
            return (CIp6Route *)rt;
        }
    return NULL;
    }

//
// CIp6Interface::IsMyAddress
// **************************
/**
// IsMyAddress returns non-NULL, if aAddr matches any of the
// current addresses for this interface
*/
TIp6AddressInfo *CIp6Interface::IsMyAddress(const TIp6Addr &aAddr, const TInt aAll) const
    {
    //
    // First would need to check if any of the id's match for this address
    // (and only properly assigned id can be "my address")
    //
    TIp6AddressInfo *id = IsMyId(aAddr);
    if (id == NULL || !id->IsAssigned())
        return NULL;
    //
    // proxy/anycast address are not normal "my addresses"
    // (they cannot be used as a source address)
    //
    if (aAll == 0 && !id->IsNormal())
        return NULL;
    //
    // If id part is 128 bits, then no further tests are required,
    // and otherwise need to check that address matches some prefix.
    //
    if (id->iPrefix == 0 || IsMyPrefix(aAddr, *id))
        return id;  // This is my address!
    //
    // None of the prefixes match
    //
    return NULL;
    }
//
// CIp6Interface::IsForMeAddress
// *****************************
TBool CIp6Interface::IsForMeAddress(const TIp6Addr &aAddr) const
    {
    //
    // IsForMeAddress is TRUE. if the address is IsMyAddress or matches
    // any of the "multicast" addresses configured for the interface
    // (for IPv4 "multicast" includes the broadcast addresses). This
    // is logically two different passes over the iRouteList, but as
    // this method is expected to be used a lot, the both loops have
    // been merged here... -- msa


    // First check if any of the id's match for this address. If none
    // matches, then only the "multicast" addresses need to be
    // tested.
    const TIp6AddressInfo *id = IsMyId(aAddr);
    if (id == NULL || !id->IsAssigned() || id->IsProxy())
        // Not yet assigned or is a proxy address (not really for me)
        id = NULL;
    else if (id->iPrefix == 0)
        // Full configured address matched, no need for further tests
        return TRUE;

    // Examine ELoopback entries in the route list for match
    for (const CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
        {
        if (rt->iState != CIp6Route::ELoopback)
            continue;           // Not my prefix or multicast...
        if (rt->iIsMulticast)
            {
            // "Multicast" entries are always specified as 128 bit
            // prefixes => use IsEqual, which is faster than Match!
            if (aAddr.IsEqual(rt->iPrefix))
                return TRUE;    // Matched fully a multicast, it's for me!
            }
        else if (id)
            {
            // Prefixes need to be compared only if Id matched!
            if (rt->iPrefix.Match(aAddr) >= rt->iLength)
                return TRUE;    // Matched a prefix and id, it's my address!
            }
        }
    return FALSE;   // No match, not for me!
    }

//
// CIp6Interface::SetMtu
// *********************
/**
// SetMtu *defines* the current send MTU for the link, and
// updates the Path MTU in case it is obviously affected.
*/
void CIp6Interface::SetMtu(TInt aMtu, TInt aMin)
    {
    iSMtu = aMtu;
    //
    // This may not be the correct solution, but assume
    // this method is not called often (and usually only
    // on startup), thus assume that this can be used to
    // initiate the Path MTU discovery and set the path
    // MTU same local link mtu [currently the only way
    // to get larger than minimum path mtu] -- msa
    //
    // *NOTE*
    //      Flows are not notified of this! [hopefully
    //      the caller will do something about it].
    if (aMtu >= aMin && (iPMtu < aMin || iPMtu > aMtu))
        iPMtu = aMtu;
    LOG(Log::Printf(_L("\tIF %u [%S] Proposed MTu=%d, current Send MTU=%d, Recv MTU=%d, Path MTU=%d"), iScope[0], &iName, aMtu, iSMtu, iRMtu, iPMtu));
    }

// MaskLength
// **********
// Local utility, compute consecutive leftmost 1-bits from 32 bit integer
//
// Not optimized for speed or anything...
//
static TInt MaskLength(TUint32 aAddr)
    {
    TInt count = 0;
    // obviously, this is "brute force" counting
    while (aAddr & 0x80000000)
        {
        count++;
        aAddr <<= 1;
        }
    return count;
    }
static TInt MaskLength(const TIp6Addr &aAddr)
    {
    TInt count = 0;
    TUint loopCount = sizeof(aAddr.u.iAddr8) / sizeof(aAddr.u.iAddr8[0]);
    for (TUint i = 0; i < loopCount; ++i)
        if (aAddr.u.iAddr8[i] == 0xFF)
            count += 8;
        else
            {
            count += MaskLength(aAddr.u.iAddr8[i] << 24);
            break;
            }
    return count;
    }

// CIp6Interface::Update6
// **********************
// Configure interface for IPv6 if it supports KSoIfInfo6 
TInt CIp6Interface::Update6(TInt aTransition)
    {
    if (iIsIPv6)
        return aTransition;     // Do not redo configuration!

    // Error returns from the following query is an indication
    // that the driver does not support IPv6.
    TPckgBuf<TSoIfInfo6> ifProp;
    ASSERT(iNifIf != NULL);
    if (iNifIf->Control(KSOLInterface, KSoIfInfo6, ifProp) != KErrNone)
        return aTransition; // No IPv6 support, exit

    iFeatures = ifProp().iFeatures;
    iSpeedMetric = ifProp().iSpeedMetric;
    SetMtu(ifProp().iMtu, KInet6MinMtu);
    iRMtu = ifProp().iRMtu;

    TPckgBuf<TSoInet6IfConfig> cfg;
    cfg().iFamily = KAfInet6;
    if (iNifIf->Control(KSOLInterface, KSoIfConfig, cfg) != KErrNone)
        return aTransition; // No IPv6 support, exit

    iIsIPv6 = 1;    // Ok, configure for IPv6
    aTransition = KIfaceTransition_UP;

    if (iFeatures & KIfCanMulticast)
        {
        CIp6Route *route;
        // If interface indicates multicast capability, then add a default
        // multicast route for it (to allow join group to select this
        // interface!)
        route = GetRoute(KInet6AddrAllNodes, 8, KRouteAdd_ONLINK);
        if (route && (iFeatures & KIfIsLoopback))
          {
            // Multicast routes on loopback interfaces should have poor metric.
            // If a "real" network interface comes up, it should be favoured over loopback.
            route->iMetric = KLoopbackMcastMetric;
          }
        }

    if (cfg().iLocalId.Family() != KAFUnspec)
        (void)AddId(cfg().iLocalId);

    if (cfg().iRemoteId.Family() != KAFUnspec)
        {
        // Assume the interface is giving implicitly an address
        // of some other host on the link (probably a Point-to-Point
        // link, and this is the other end of the link). Just construct
        // a link local address for it and setup a host route.
        //
        TIp6Addr remote(KInet6AddrLinkLocal);
        MakeFullAddress(remote, 10, cfg().iRemoteId.Ptr(), cfg().iRemoteId.Length());
        (void)GetRoute(remote, 128, KRouteAdd_ONLINK);
        }

    // Initialize name servers from configuration
    UpdateNameServers(cfg().iNameSer1, cfg().iNameSer2);

    //
    // Add permanent multicast groups
    //
    (void)GetRoute(KInet6AddrNodeLocal, 128, KRouteAdd_MYPREFIX);
#if 0
    (void)UpdateMulticast(KInet6AddrAllNodes);
#else
    if (iScope[1])  // Interface has link local scope id?
        (void)UpdateMulticast(KInet6AddrAllNodes);
#endif
    return aTransition;
    }

// CIp6Interface::ConfigureAddress
// *******************************
/**
// Internal utility which configures an first/additional IPv4
// address + netmask for the interface
//
// @param aAddr
//  The IPv4 address in IPv4-mapped format
// @param aMaskLength
//  The netmask length (bits counted for IPv6, thus netmask
//  is configured only if <tt>96 < aMaskLength <= 128</tt>.
//
// @return
//  @li = 0, if no change in configuration
//  @li = 1, if configuration changed
*/
TInt CIp6Interface::ConfigureAddress(const TIp6Addr &aAddr, const TUint aMaskLength, const TBool aForcePrimary)
    {
    ASSERT(aMaskLength <= 128);
    if (aMaskLength > 128)
        return 0;

#ifdef _LOG
    TLogAddressPrefix tmp(aAddr, aMaskLength);
    Log::Printf(_L("\tIF %u [%S] ConfigureAddress([%S])"), iScope[0], &iName, &tmp);
#endif

    // Address can be configured only if there is an address...
    if (aAddr.u.iAddr32[3] == 0)
        return 0;

    //
    // Setup up my own address
    // -----------------------
    // Convert TInetAddr iAddress into ipv4 compat address
    // and set it up as a prefix and id

    if (AddId(aAddr, 0, TIp6AddressInfo::ENormal, aForcePrimary) == 0)
        // No change.
        return 0;

    // Setup up netmask (if defined)
    // -----------------------------
    if (aMaskLength > 96)
        {
        //
        // Add ONLINK route for the net
        //
        (void)GetRoute(aAddr, aMaskLength, KRouteAdd_ONLINK);
        //
        // Make network broadcast address as a "multicast group" into the routes.
        // my_net is the network prefix combined with all-ones host part (= broadcast address)
        TIp46Addr my_net(0xffffffffU >> (aMaskLength-96));
        my_net.u.iAddr32[3] |= aAddr.u.iAddr32[3];
        CIp6Route *const rt = GetRoute(my_net, 128, KRouteAdd_MYPREFIX);
        if (rt)
            rt->iIsMulticast = 1;   // mark it as "multicast"!
        }
    return 1;
    }


// CIp6Interface::FindInternalIpv4LinkLocalAddr
// ****************************************
/**
// Find the one and only internally generated IPv4 link-local, if present.
//
// @return the TIp6AddressInfo, if such address exists; and NULL otherwise.
*/
TIp6AddressInfo* CIp6Interface::FindInternalIpv4LinkLocalAddr()
    {
    // Call does not get ownership of object.
    return const_cast<TIp6AddressInfo *>( FindIpv4LinkLocalAddr() );
    }


// CIp6Interface::RandomAddress
// ****************************
// Generate pseudorandom IPv4 link local address.
TInt CIp6Interface::RandomAddress(TIp6Addr &aAddr, TUint aPrefix, TUint aN)
    {
    // Use current hardware address as a seed and generate N'th variant.
    // Optimized for space, not speed (wasting CPU on generating
    // the N-1 numbers needlessly, but saving the need to store
    // the seed in CIp6Interface...).
    //
    // *NOTE 1* In current use aN is stored in TUint8 of
    // TIp6AddressInfo::iGenerated => after 256 addressesses,
    // the same sequence starts, and the loop below does not
    // grow into infinity... (in practice aN = 0 or 1)
    // *NOTE 2* To avoid the loop, one would need to store
    // two seeds into CIp6Interface, one for IPv4 and one for
    // IPv6.
    //
    // The use of hw as seed gives the effect that host tends to
    // get the same link local address, if possible
    TInt64 seed = (TInt64 &)iHwAddr[8];
    TReal r;
    TUint i = 0;
    do
        r = Math::FRand(seed);
    while (++i <= aN);  // pick N'th pseudo-random number


    if (aAddr.IsV4Mapped())
        {
        if (aAddr.Scope() != KIp6AddrScopeLinkLocal)
            return 0;   // IPv4 address can only be generated if Link Local

        TReal random_addr_float;
        TInt32 random_addr = 0;

        (void)Math::Round(random_addr_float, r * (INET_ADDR(169,254,254,255) - INET_ADDR(169,254,1,0)), 0);
        (void)Math::Int(random_addr, random_addr_float);
        TIp46Addr addr(INET_ADDR(169,254,1,0) + random_addr);

        aAddr = addr;
        return 1;
        }
    //
    // For IPv6, a placeholder for now -- just use the current seed as a source for the ID part
    // (this part is not used until privacy, RFC-3041 is implemented)
    //
    MakeFullAddress(aAddr, aPrefix, (TUint8 *)&seed, sizeof(seed));
    return 1;
    }

// CIp6Interface::DuplicateAddress
// *******************************
// The specified address has been detected as duplicate.
void CIp6Interface::DuplicateAddress(TIp6AddressInfo *aId, TBool &aDefendIPAddress, TBool aGratuitousArp)
    {
    if (aId == NULL)
        return;
    TIp6Addr addr = aId->iId;
    for (;;)
        {
        TTime stamp;
        stamp.UniversalTime();

        if (!aId->IsTentative())
            {
            //
            // Messy situation, a collision on established address. The
            // following logic is applied: if address is younger than
            // DupAddrDefendTime seconds, give it up. Otherwise defend
            // address by sending an announcement. However, to prevent looping
            // on this, reset creation time of the address to now.
            //
            TLifetime now = ElapsedUnits(aId->iCreated, stamp);
            if (now > CIp6Manager::TimerUnits(iND.iDupAddrDefendTime))
                {
                if(aGratuitousArp)
                {
                aDefendIPAddress = ETrue;
                }
                else 
                {
                // Old established address, try to keep it: reset
                // creation time and send an announcement for it...
                aId->iCreated = stamp;
                (void)SendNeighbors(KInet6ICMP_NeighborSol, NULL, aId->iId);
                }
                return;
                }
            }
        //
        // A tentative address or an established address which we
        // going to give up...
        // 
        if (aId->iGenerated == 0)
            break;  // Not automatically generated or has been generated
                    // too many times already!
        if (!RandomAddress(addr, aId->iPrefix, aId->iGenerated))
            break;  // failed for some reason
        //
        // A new address has been generated
        //
        aId->iGenerated++;
        SetId(*aId, addr, aId->iPrefix, aId->AddressType());
        if (aId->iGenerated >= iND.iMaxAddrRegenerations)
            // If we have exceeded the limitation of regenerations,
            // then put the creation time 60sec into future, and thus
            // delay the activation of this address (probes start
            // at least 60s delayed).
            aId->iCreated += TTimeIntervalSeconds(60);
        //
        // Start the DAD process
        //
        Timeout(stamp);
        return;
        }
    //
    // No new address, remove the duplicate
    //
    RemId(aId);
    }

// CIp6Interface::ConfigureLinkLocal
// *********************************
/**
// Internal utility for automatic configuring of the linklocal IPv4 address.
//
// @param aConfAddr IPv4 address received from CNifIfBase::Control(). If 0, no IPv4 address
//                  was configured on Nif, and linklocal address will be enabled on settings
//                  2 (EV4LLConditional) and 1 (EV4LLAlways).
//
// @return
// @li  = 0, if no change in configuration
// @li  = 1, if configuration changed
*/
TInt CIp6Interface::ConfigureLinkLocal(TUint32 aConfAddr)
    {
    // IPv4 link local specification applies only for
    // interfaces that support Neighbour Discovery).
    if (!NeedsND())
        return 0;
        
    // Always support IPv4 LL on all ND interfaces, by
    // always installing the IPv4 LL onlink route.
    const TInt prefix = 96+16; // Ipv4-mapped format, need to add 96
    TIp46Addr addr(KInetAddrLinkLocalNet);
    if (GetRoute(addr, prefix, KRouteAdd_ONLINK) == NULL)
        {
        // If this route cannot be created or does not exist,
        // there is no point in doing anything else here.
        return 0;
        }

    // Check if automatic configuration has already been done,
    TIp6AddressInfo *const exists = FindInternalIpv4LinkLocalAddr();

    // Currently, detecting duplicates is only defined for
    // interfaces that support ARP, and this is only possible if
    // there are link layer addresses.
    // => LinkLocals can only be generated on interface
    //    which has addresses!
    const TInt flag = HaveIp4LinkLocal();
    if (flag == EV4LLDisabled ||
        (flag == EV4LLConditional && aConfAddr) ||
        iHwAddr.Family() == KAFUnspec)
        {
        // The automatically configured IPv4 should not exist - remove
        // if it does. RemId can be called with NULL, and returns
        // KErrNone, if address was actually removed.
        TInt retVal = RemId(exists);
        
        #ifdef _LOG
            if( retVal == KErrNone )
                {
                TBuf<39> addrStr;
                
                TInetAddr( addr, 0 ).Output( addrStr );
                
                Log::Printf( _L( "CIp6Interface::ConfigureLinkLocal - Link local address %S removed" ), &addrStr );
                }
        #endif
            
        return retVal;
        }

    if (exists)
        {
        // Just reset the lifetimes, in case it was in deprecated status
        exists->iPLT = KLifetimeForever;
        exists->iVLT = KLifetimeForever;
        return 0;
        }
    //
    // Address does not exist yet - make it unless we are trying to reuse
    // an old address.
    //
    if( RandomAddress(addr, prefix, 0) && ConfigureAddress( addr, prefix ) )
        {
        // If address generated, must find the address and
        // mark it as generated.
        TIp6AddressInfo *const id = GetId(addr);
        if (id)
            {
            id->iIpv4LinkLocal = 1;     // Mark it as Internally Generated IPv4 LL.
            if (id->iGenerated == 0)
                id->iGenerated = 1;
            // Add a random constant to the creation time, so that DAD starts after a random
            // delay. [timers 1sec accuracy is a bit problem here -- msa]
            ASSERT(iND.iIPv4RetransTimer < 2000);   // ensures non-negative adjust below.
            id->iCreated += TTimeIntervalMicroSeconds32((TInt)(Math::FRand(Interfacer().iSeed) * iND.iIPv4RetransTimer * 1000000.0));

            #ifdef _LOG
                TBuf<39> addrStr;
                
                TInetAddr( addr, 0 ).Output( addrStr );
                
                Log::Printf( _L( "CIp6Interface::ConfigureLinkLocal - Link local address %S configured" ), &addrStr );
            #endif
            }
        return 1;
        }
    return 0;
    }

CIp6Route *CIp6Interface::StartProbeND(const TIp6Addr &aSrc, const TIp6Addr &aDst)
    /**
    * Probe for an address on the link.
    *
    * Starts a probing neighbour discovery on a destination address.
    * This can be used to force ND on any address.
    *
    * @param    aSrc    Source address to be used in probing
    * @param    aDst    Destination to probe
    * @return
    *   Host route entry, if probing started (or was already active).
    *   Or, NULL not started.
    */
    {
    CIp6Route *const n = GetRoute(aDst, 128, KRouteAdd_PROBINGONLY);
    if (n && n->iIsProbing)
        {
#ifdef _LOG
        TLogAddressPrefix dst(aDst);
        TLogAddressPrefix src(aSrc);
        Log::Printf(_L("\tIF %u [%S] StartProbeND(src=%S, dst=%S)"), iScope[0], &iName, &src, &dst);
#endif
        n->StartND(aSrc);
        return n;
        }
    return NULL;
    }



// CIp6Interface::UpdateNameServers
// ********************************
/**
// Internal utility to load the namer server addresses consistently
// @param ns1 The name server address 1.
// @param ns2 The name server address 2.
// @param aOverride
//  @li == 0 => addresses are only changed if unspecified previously
//  @li != 0 => new specified address always overwrites previous setting
*/
void CIp6Interface::UpdateNameServers(const TInetAddr &ns1, const TInetAddr &ns2, const TInt aOverride)
    {
#ifdef _LOG
    TLogAddressPrefix old_ns(ns1);
    TLogAddressPrefix new_ns(ns2);
    Log::Printf(_L("\tIF %u [%S] UpdateNameServers(ns1=%S, ns2=%S, override=%d)"), iScope[0], &iName, &old_ns, &new_ns, aOverride);
    old_ns.Set(iNameSer1);
#endif
    //
    // 1. name server address
    //
    if (ns1.Family() != KAFUnspec && (aOverride || iNameSer1.Family() == KAFUnspec))
        {
        if (ns1.IsUnspecified())
            iNameSer1.Init(KAFUnspec);
        else
            iNameSer1 = ns1;
        }
#ifdef _LOG
    new_ns.Set(iNameSer1);
    Log::Printf(_L("\tIF %u [%S]   ns1: old=%S new=%S"), iScope[0], &iName, &old_ns, &new_ns);
    old_ns.Set(iNameSer2);
#endif
    //
    // 2. name server address
    //
    if (ns2.Family() != KAFUnspec && (aOverride || iNameSer2.Family() == KAFUnspec))
        {
        if (ns2.IsUnspecified())
            iNameSer2.Init(KAFUnspec);
        else
            iNameSer2 = ns2;
        }
#ifdef _LOG
    new_ns.Set(iNameSer2);
    Log::Printf(_L("\tIF %u [%S]   ns2: old=%S new=%S"), iScope[0], &iName, &old_ns, &new_ns);
#endif
    }

//
// CIp6Interface::Update4
// **********************
// Configure interface for IPv4 if it supports KSoIfInfo and KSoIfConfig
TInt CIp6Interface::Update4(TInt aTransition)
    {
    if (iIsIPv4)
        return aTransition; // Do not redo configuration, if it has been already done!

    TPckgBuf<TSoIfInfo> info_buf;
    ASSERT(iNifIf != NULL);
    TInt err = iNifIf->Control(KSOLInterface, KSoIfInfo, info_buf);
    if (err != KErrNone)
        return aTransition; // No IPv4 support (no change)
    //
    // Basic minimal configuration
    //

    const TSoIfInfo &info = info_buf();
    iFeatures = info.iFeatures;
    iSpeedMetric = info.iSpeedMetric;
    SetMtu(info.iMtu, KInetMinMtu);
    iRMtu = info.iMtu;  // In IPv4 there is no separate slot for
                        // receive and send MTU (assume they are same)

    // Need to magically setup routing for the IPv4 interfaces,
    // just ask the interface parameters and make best effort...
    TPckgBuf<TSoInetIfConfig> cfg;
    if ((err = GetIp4Config(iNifIf, cfg)) != KErrNone)
        return aTransition; // No IPv4 support

    // For all IPv4 Interfaces, setup 255.255.255.255 address
    static const TIp6Addr broadcast = {{{0,0,0,0,0,0,0,0,0,0,0xff,0xff,255,255,255,255}}};
    CIp6Route *const rt = GetRoute(broadcast, 128, KRouteAdd_MYPREFIX);
    if (rt)
        rt->iIsMulticast = 1;   // mark it as "multicast"

    // For all IPv4 Interfaces: join to 224.0.0.1 multicast group (all hosts)
    static const TIp6Addr mc_hosts = {{{0,0,0,0,0,0,0,0,0,0,0xff,0xff,224,0,0,1}}};
    (void)UpdateMulticast(mc_hosts);
    if ((iFeatures & (KIfCanMulticast|KIfIsLoopback)) == KIfCanMulticast)
        {
        // Add default IPv4 multicast route (but not on loopbacks!)
        // (this puts all multicast as "ONLINK", instead of possibly
        // punting them to the default gateway!)
        (void)GetRoute(mc_hosts, 100, KRouteAdd_ONLINK);
        }

    //  cfg().iConfig values:
    //
    const TInetIfConfig  &cf = cfg().iConfig;
    const TUint32 addr = cf.iAddress.Address();
    const TIp46Addr my_addr(addr);

    // Initialize name servers from configuration
    UpdateNameServers(cf.iNameSer1, cf.iNameSer2);

    // Configure "configured" address, if any, and configure ZEROCONF link local
    // address (if not already done).  Only the EV4LLAlways and EV4LLConditional
    // with no static IP address options cause link local creation at this time.
    // A configuration daemon may create a link local at its discretion (e.g.,
    // if DHCP discovery fails) if the EV4LLConfigDaemonControlled option
    // is enabled.
    TInt changed = ConfigureAddress(my_addr, 96+MaskLength(cf.iNetMask.Address()));
    const TInt flag = HaveIp4LinkLocal();
    if( flag != EV4LLConfigDaemonControlled ) // daemon with EV4LLConfigDaemonControlled interface option will use KSoInetCreateIPv4LLOnInterface socket option to configure a link local if desired
        {
        changed |= ConfigureLinkLocal( addr );
        }
    if (changed == 0)
        return aTransition; // -- no change in update4

    iIsIPv4 = 1;    // Freeze current configuration and mark IF as IPv4 capable

    const TUint32 defgate = cf.iDefGate.Address();
    if (defgate)
        {
        const TIp46Addr tmp(defgate);   // tmp required to get it compiled with gcc!
        if (defgate == addr)
            {
            // *NOTE* Apparently some GPRS phones have a PPP server which gives
            // my own address as a default gateway, do not add gateway route
            // in such case (it might confuse next hop selection).
            //
            // *NOTE* This branch is only entered when the interface reports
            // "broken/bad" configuration information (unless we define the
            // condition "gateway == my address" to mean exactly this: install
            // default IPv4 onlink route to the link).
            (void)GetRoute(tmp, 96, KRouteAdd_ONLINK);
            }
        else
            {
            const TInetAddr gateway(tmp, 0);
            (void)GetRoute(tmp, 128, KRouteAdd_ISROUTER);
            (void)GetRoute(tmp, 96, KRouteAdd_GATEWAY, &gateway);
            }
        }
#if 0   // iBrdAddr appears to hold the other end address??

    //  TInetAddr iBrdAddr. Store net broadcast address into
    // iPrefix[0]... (so it will be recognized as own...)
    cf.iBrdAddr.ConvertToV4Mapped();
    iPrefix[0] = TIp6Prefix(cf.iBrdAddr.Ip6Address(), 128);
#endif

    return KIfaceTransition_UP;
    }

// CIp6Interface::SendNeighbors
// ****************************
const TInt KSendNeighbors_NO_OVERRIDE = 0x100;  // Do not set OVERRIDE bit in NA
/**
// Send Neighbor Discovery packets (IPv6 ND or IPv4 ARP).
//
// Internal help method which is used to send send one of the
// following
//  @li Neighbor Solicitation
//  @li Neighbor Advertisement
//  @li Router Solicitation
//
// to the interface
//
// @param aMessageType
//  The low 8 bits are the ICMP6 type code of the message to be sent
//  The higher bits can be used as flags for details
// @param aDest
//  The (host) route to be used in sending. This is used for unicast
//  ND traffic. If NULL, then packet will have multicast destination.
// @param aTarget
//  The target address (used in NS/NA).
// @param aSource
//  The source address to use. If not given (NULL or unspecified), the source is
//  selected by the destination (for ND) or by the aTarget (for ARP) address.
// @return
//  @li == KErrNone, if send apparently succeeded
//  @li != KErrNone, for problems detected (out of memory mostly)
//
// WARNING:
//  This code is "hand tailored" to work exactly and *ONLY* with the listed
//  types of ICMP messages. If a support for a new type of ICMP is to be
//  added, the code must be reviewed very carefully! -- msa
*/
TInt CIp6Interface::SendNeighbors(TInt aMessageType, CIp6Route *aDest, const TIp6Addr &aTarget, const TIp6Addr *const aSource)
    {
#ifdef SYMBIAN_TCPIPDHCP_UPDATE  
    #ifdef _DEBUG
        LOG(Log::Printf(_L("<>\tCIp6Interface::SendNeighbors()")));
    #endif 
#endif // SYMBIAN_TCPIPDHCP_UPDATE  
    const TUint8 icmp_type = (TUint8)aMessageType;
    RMBufSendPacket packet;
    RMBufSendInfo *info = NULL;
    TInt err = KErrNone;
    TIp6Addr dst, src;

#ifdef ARP
    // If the target is IPv4 address, then translate the IPv6 ND request to
    // ARP message.
    if (aTarget.IsV4Mapped())
        {
        // The ARP kludge needs only to map the Neighbor Solicitation
        // into ARP Request (this really should not get called with
        // anything else).
        if (icmp_type != KInet6ICMP_NeighborSol)
            return KErrNone;

        //
        // Source address is actually used in the ARP packet and
        // must thus match the ARP target. Thus, select it by
        // target.
        if (aSource && (aSource->u.iAddr32[3] == 0 || IsMyAddress(*aSource)))
            src = *aSource;
        else if (SelectSource(src, aTarget) == NULL)
            src = KInet6AddrNone;

        const TUint arp_length = TInet6HeaderArp::MinHeaderLength() +
            (iHwAddr.GetUserLen() + 4) * 2;
        TRAP(err, info = packet.CreateL(arp_length));
        if (err != KErrNone || info == NULL)
            return err;
        for (;;)
            {
            TInet6Packet <TInet6HeaderArp> arp;
            arp.Set(packet, 0, arp_length);
            if (arp.iHdr == NULL)
                break;
            arp.iHdr->SetPrAddrLen(4);
            arp.iHdr->SetHwAddrLen(iHwAddr.GetUserLen());
            // Assume the required ARP Hardware type is returned in the port
            // field of the hardware address of the interface (either this,
            // or the interface snoops ARP and fixes the value for this
            // field.. -- msa)
            arp.iHdr->SetHardwareType(iHwAddr.Port());
            arp.iHdr->SetProtocolType(KArpProtocolType_IP);
            arp.iHdr->SetOperation(EArpOperation_REQUEST);
            arp.iHdr->SenderHwAddr().Copy(iHwAddr.Address());
            arp.iHdr->TargetHwAddr().FillZ();
            // Assume src & target are IPv4 mapped address...
            arp.iHdr->SenderPrAddr().Copy(TPtrC8(&src.u.iAddr8[12], 4));
            arp.iHdr->TargetPrAddr().Copy(TPtrC8(&aTarget.u.iAddr8[12], 4));
            info->iProtocol = KProtocolArp;
            info->iFlags = 0;
            // Note: if aDest is non-NULL, then this iDstAddr will be
            // will be replaced in aDest->Send() with the link layer
            // address... -- msa
            TInetAddr::Cast(info->iDstAddr).SetAddress(KInetAddrBroadcast);
            TInetAddr::Cast(info->iSrcAddr).SetAddress(0);  // Don't care
            packet.Pack();
            // draft-ietf-zeroconf-ipv4-linklocal-05.txt says that whenever
            // the sender is ipv4 link local, then the replies (and requests)
            // must always be sent to the broadcast address [IMHO, this is
            // a bit dubious rule, but if it is so specified, comply... -- msa]
            if (aDest && src.Scope() != KIp6AddrScopeLinkLocal)
                aDest->Send(packet);
            else if (iState == EFlow_READY)
                {
                // Send only if ready, to avoid queuing ARP packets into hold queue.
                Send(packet, NULL);
                }
            break;
            }
        packet.Free();
        return KErrNone;
        }
#endif
    //
    // If destination is Unspecified, use solicited node address generated from
    // the target address (as first default, may be changed later below)
    //
    if (aDest)
        dst = aDest->iPrefix;
    else
        dst = (const TIp6Addr)TSolicitedNodeAddr(aTarget);
    //
    // Try to pick aSrc address, if not specified by the caller
    // Leave it unspecified, if no valid source addresses.
    //
    if (aSource && (aSource->IsUnspecified() || IsMyAddress(*aSource)))
        // However, a source address must be a valid address on this interface
        // or unspecified (it cannot be a proxy or anycast). Thus, IsMyAddress
        // test in above. [This situation occurs when node is acting as
        // a router/proxy and is trying to find destination cache for a
        // forwarded packet, by normal rules the source is taken from the
        // packet.
        // Could perhaps do this test before calling SendNeighbors? --msa]
        src = *aSource;
    else if (SelectSource(src, dst) == NULL)
        src = KInet6AddrNone;
    // This allocates too much space for the RS, but as class
    // used in TInet6Checksum is NA, the mapping would fail
    // for too short RMBufChain... icky! -- msa
    // [but, as one RMBuf is always needed anyway, it doesn't
    // cause any real extra allocations either...]
    TUint icmp_length = TInet6HeaderICMP_NeighborAdv::MinHeaderLength();
    for (;;)    // for handy error exits via breaks...
        {
        TInt link_layer = 0;    // The length of the SLL/TLL option in bytes
        if (iHwAddr.Family() != KAFUnspec && !src.IsUnspecified())
            {
            // Got Link Layer Address, include it into the solicitation
            link_layer = ((iHwAddr.GetUserLen() + 2) + 7) & ~0x7;
            icmp_length += link_layer;
            }

        TRAP(err, info = packet.CreateL(icmp_length));
        if (err != KErrNone || info == NULL)
            break;

        ASSERT((TUint)info->iLength == icmp_length);

        TInet6Checksum<TInet6HeaderICMP_NeighborAdv> icmp(packet);
        if (icmp.iHdr == NULL)
            break;      // Shouldn't happen!
        //
        // Build the ICMP Message
        //
        icmp.iHdr->SetType(icmp_type);
        icmp.iHdr->SetCode(0);
        icmp.iHdr->SetParameter(0); // (for NA, this may contain flags, see below)
        switch (icmp_type)
            {
            case KInet6ICMP_NeighborAdv:
#ifdef SYMBIAN_TCPIPDHCP_UPDATE                  
                #ifdef _DEBUG
                    LOG(Log::Printf(_L("<>\tCIp6Interface::SendNeighbors() KInet6ICMP_NeighborAdv is called")));
                #endif
#endif // SYMBIAN_TCPIPDHCP_UPDATE                      
                // Make some guesses for S and O bits of NA (these
                // depend on how this SendNeighbors method is called
                // in the current implementation!) -- msa
                //
                if (aDest)
                    {
                    // Assume NA to a specific destionation
                    // is always SOLICITED!
                    icmp.iHdr->SetS(1);
                    }
                else
                    {
                    // Otherwise dest is allways for all nodes
                    dst = KInet6AddrAllNodes;
                    }
                if ((KSendNeighbors_NO_OVERRIDE & aMessageType) == 0)
                    {
                    // By default all NA's are for own address, so O=1, if we
                    // have link_layer addr (unless disabled by the caller).
                    // (NA's proxy addresses must not have O set, for example)
                    icmp.iHdr->SetO(link_layer);
                    }
                icmp.iHdr->SetR(iIsRouter);
                // *FALL TRHOUGH TO NS*/
            case KInet6ICMP_NeighborSol:
#ifdef SYMBIAN_TCPIPDHCP_UPDATE                  
                #ifdef _DEBUG
                    LOG(Log::Printf(_L("<>\tCIp6Interface::SendNeighbors() KInet6ICMP_NeighborSol is called")));
                #endif
#endif // SYMBIAN_TCPIPDHCP_UPDATE                      
                icmp.iHdr->Target() = aTarget;  // NS & NA have same format for this
                break;
            case KInet6ICMP_RouterSol:
                if (aDest == NULL)
                    // Unspecific destination is always to all ROUTERS
                    dst = KInet6AddrAllRouters;
                // Argh! because we allocated too much space for the
                // buffer, the info->iLength is now incorrect for RS.
                // Need to fix.. another yech! -- msa
                icmp_length -=
                    TInet6HeaderICMP_NeighborAdv::MinHeaderLength() -
                    TInet6HeaderICMP_RouterSol::MinHeaderLength();
                packet.TrimEnd(icmp_length);
                break;
            default:
                ASSERT(0);
                break;
            }
        //
        // Add Target (for NA) or Source (for NS and RS) Link-layer address option (if possible)
        //
        if (link_layer > 0)
            {
            const TPtr8 ptr(((RMBufPacketPeek &)packet).Access(link_layer, info->iLength - link_layer));
            if (ptr.Length() < link_layer)
                break;
            TInet6OptionICMP_LinkLayer *addr = (TInet6OptionICMP_LinkLayer *)ptr.Ptr();
            addr->SetType(icmp_type == KInet6ICMP_NeighborAdv ? KInet6OptionICMP_TargetLink : KInet6OptionICMP_SourceLink);
            addr->SetLength(link_layer >> 3);    // Option length is in units of 8 octets!
            addr->Address().Copy(iHwAddr.Address());
            }
        //
        // Complete the Info structure for ICMP Checksum computation
        //
        info->iProtocol = KProtocolInet6Icmp;
        TInetAddr::Cast(info->iDstAddr).SetAddress(dst);
        TInetAddr::Cast(info->iSrcAddr).SetAddress(src);
        TInetAddr::Cast(info->iDstAddr).SetScope(iScope[dst.Scope()-1]);    // scopeid from current interface

        // Create unconnected flow context to the packet (info)
        if (info->iFlow.Open(iNifUser->iNetwork, info->iProtocol) != KErrNone)
            break;
        CIp6Flow *flow = (CIp6Flow *)info->iFlow.FlowContext();
        if (!flow)
            break;
        // Setup the connection information and connect
        info->iFlow.SetRemoteAddr(info->iDstAddr);
        info->iFlow.SetLocalAddr(info->iSrcAddr);
        info->iFlow.SetIcmpType(icmp_type, 0);
        flow->iInfo.iLocalSet = 1;              // Disable source address select (even for unspecified)
        flow->iInfo.iLockId = iScope[0];        // Accept connect only to this interface.
        flow->iInfo.iLockType = EScopeType_IF;  // Accept connect only to this interface.
        flow->iOptions.iMulticastHops = 255;    // ND wants hoplimit = 255 for multicast
        flow->iOptions.iHopLimit = 255;         // ND wants hoplimit = 255 for unicast
        flow->iOptions.iMulticastLoop = 0;      // We don't want to see own packets!
        flow->iOptions.iKeepInterfaceUp = 0;    // We don't keep IF up just for ND traffic.
        info->iFlow.Connect();
        if (flow->iStatus != KErrNone)
            {
            LOG(Log::Printf(_L("\tIF  %d [%S] SendNeighbors connect failed (%d)"), iScope[0], &iName, (TInt)flow->iStatus));
            info->iFlow.Close();
            break;      // Cannot get a flow opened..
            }
        // The "assert" below might seem logical, but it is not, if ND is
        // to IPSEC VPN interface (VPN != Real interface for flow).
        // -- thus, remove it!
        // __ASSERT_DEBUG(iNifIf == info->iFlow.Interface(), User::Panic(_L("DEBUG"), 0));

        info->iFlags = 0;
        icmp.ComputeChecksum(packet, info);
        packet.Pack();
        (void)iNifUser->iNetwork->Send(packet, NULL);
        LOG(Log::Printf(_L("<>\tCIp6Interface::SendNeighbors() KInet6ICMP_NeighborSol is connected")));
        return KErrNone;
        //  
        }   // <-- Never get here, not a real loop!
    packet.Free();
    LOG(Log::Printf(_L("\tIF %u [%S] SendNeighbors send failed (%d)"), iScope[0], &iName, err));
    return err < 0 ? err : KErrNoMemory;
    }


//
// CIp6Interface::StartSending
// ***************************
// Interface specific StartSending method. Generic code (and IPv6 stuff)
TInt CIp6Interface::StartSending()
    {
    if (!iNifIf)
        return KErrGeneral;         // Should never happen!?

    // Getting a StartSending from a device means that the driver
    // is ready for the first or more input. Thus, by default set
    // the interface state initially to EFlow_READY. The code
    // after this may decide to set some other state later.
    //
    // Similarly, decide on initial return value for the
    // transition state
    TInt transition = (iState > 0) ? KIfaceTransition_READY : KIfaceTransition_NONE;
    iState = EFlow_READY;

    //
    // If there are any packets in hold queue, then flush out
    // as many as possible now...
    //
    if (!iHoldQueue.IsEmpty())
        {
        RMBufChain packet;
        TInt count = 0;
        while (iHoldQueue.Remove(packet))
            {
            count++;
            // Use the standard Send() method! This has a code to re-insert the packet
            // into hold queue (but, it will not fire as long as iState is not
            // EFlow_HOLD!)
            (void)Send(packet);
            if (iState > 0)
                {
                // The interface went back to hold, ignore start sending
                LOG(Log::Printf(_L("\tIF %u [%S] NIF signals HOLD after sending %d packets from hold queue (%d)"),
                    iScope[0], &iName, count, (TInt)iHoldQueue.IsEmpty()));
                return KIfaceTransition_NONE;
                }
            }
        LOG(Log::Printf(_L("\tIF %u [%S] Flushed hold queue (%d) successfully"), iScope[0], &iName, count));
        }

    // Configure Network and IAP identifiers
    TPckgBuf<TSoIfConnectionInfo> netinfo;
    if (iNifIf->Control(KSOLInterface, KSoIfGetConnectionInfo, netinfo) == KErrNone)
        {
        // NIF supports Network Information
        LOG(Log::Printf(_L("\tIF %u [%S] has IAP=%d, NET=%d"),
            iScope[0], &iName, (TInt)netinfo().iIAPId, (TInt)netinfo().iNetworkId));
        }
    else
        {
        // NIF does not support Network Information, pick some dummies
        netinfo().iIAPId = ~iScope[0];
        netinfo().iNetworkId = KDefaultNetworkId;
        LOG(Log::Printf(_L("\tIF %u [%S] has no ConnectionInfo, defaulting IAP=%d, NET=%d"),
            iScope[0], &iName, (TInt)netinfo().iIAPId, (TInt)netinfo().iNetworkId));
        }
    //
    // Initialize the scope vector from netinfo
    //
    iScope[1] = netinfo().iIAPId; // - Link Local Scope (2)
    iScope[2] = netinfo().iIAPId; // - Subnet-local Scope (3)
    // Remaining slots will get the network id
    for (TInt i = 3; i <= EScopeType_NET; ++i)
        iScope[i] = netinfo().iNetworkId;

    //
    // Refresh the hardware address of the interface on each StartSending
    // (if link layer addresses are supported by the interface)
    //
    TPckgBuf<TSoIfHardwareAddr> hwaddr;
    if (iNifIf->Control(KSOLInterface, KSoIfHardwareAddr, hwaddr) == KErrNone)
        iHwAddr = TLinkAddr::Cast(hwaddr().iHardwareAddr);
    else
        iHwAddr.SetFamily(KAFUnspec);

    transition = Update4(transition);
    transition = Update6(transition);

    if (transition == KIfaceTransition_UP)
        {
        //
        // Save the UP transition time into the iAddress.iPreferredLifetime
        // and activate router finding and duplicate address detection.
        //
        //
        // Before sending the solicitations, choose a delay [0..MAX_RTR_SOLICITATION_DELAY]
        //
        const TInt delay = (TInt)(Math::FRand(Interfacer().iSeed) * iND.iMaxRtrSolicitationDelay * 1000000.0);
        iAddress.iCreated += TTimeIntervalMicroSeconds32(delay);
        LOG(Log::Printf(_L("\tIF %u [%S] Next event delay=%d [us]"), iScope[0], &iName, delay));
        iAddress.iNS = 0;
        iRetryRS = 0;
        Interfacer().SetTimerWithUnits(iTimeout, CIp6Manager::TimerUnits(delay, 1000000));

        // This is treated as a change-type event, the add event is considered to occur
        // in DoBind (i.e. when InterfaceAttached is called)
        NotifyInterfaceEvent(EventTypeModify);
        }

    return transition;
    }


void CIp6Interface::NotifyInterfaceEvent(TUint aEventType) const
{
    CIp6Manager *const mgr = &Interfacer();

    // If there is no event manager, or if there are no registered listeners, we can exit
    // the function right away
    if (!mgr->EventManager())
        return;

    if (mgr->EventManager()->IsEmpty(EClassInterface))
        return;
  
    TInetInterfaceInfo info;

    info.iIndex = iScope[0];
    info.iHwAddr = iHwAddr;
    info.iName = iName;
    info.iFeatures = iFeatures;
    info.iSMtu = iSMtu;
    info.iRMtu = iRMtu;
    info.iSpeedMetric = iSpeedMetric;

    // Copied and edited from interfaceinfo()
    if (iNifIf == NULL)
        info.iState = EIfDown; // no interface or address not known or was duplicate
    else if (iState == EFlow_READY)
        info.iState = EIfUp;
    else if (iState == EFlow_PENDING)
        info.iState = EIfPending;
    else if (iState == EFlow_HOLD)
        info.iState = EIfBusy;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    else if (iState == EFlow_NOTCONFIGURE)
        info.iState = EIfNotConfigured;
#endif //SYMBIAN_TCPIPDHCP_UPDATE    
    else
        info.iState = EIfDown;

    mgr->EventManager()->Notify(EClassInterface, aEventType, &info);
}


//
// CIp6Interface::Timeout()
// ************************
//
void CIp6Interface::Timeout(const TTime &aStamp)
    {
    // Somewhat twisted logic for sending RS and NS, but not wanting to run
    // separate timers for both, and as they have different transmit intervals,
    // the decision whether to send RS/NS or not, is tricky... -- msa
    //
    // The process starts when interface does the UP transition (see StartSending),
    // at that point the start time of the process is saved into iAddress.iCreated
    //
    // If (transmitted_packets * transmit_interval <= elapsed)
    //     a packet can be sent;
    //
    // Timeout can be called more often than packets are sent, but only after sufficient
    // amount of time has passed, the actual sending occurs. [If there is a configuration
    // error, and either transmit_interval is ZERO, then those packets are sent back to
    // back without delay (causing *recursive* calls to this Timeout()!)].
    //
#ifdef SYMBIAN_TCPIPDHCP_UPDATE  
    #ifdef _DEBUG
        LOG(Log::Printf(_L("<>\tCIp6Interface::Timeout()")));
    #endif
#endif // SYMBIAN_TCPIPDHCP_UPDATE  
    TUint next_event = KMaxTUint;
    //
    // Go through all Address blocks
    //
    TIp6AddressInfo *privacy = NULL;
    TInt have_address = 0;
    for (TIp6AddressInfo *id = &iAddress;;)
        {
        if (id->IsSet())
            {
            TUint elapsed_units = ElapsedUnits(id->iCreated, aStamp);
#ifdef _LOG
            TLogAddressPrefix tmp(id->iId, id->iPrefix == 0 ? 128 : id->iPrefix);
            Log::Printf(_L("\tIF %u [%S] ADDRESS %S %S%S age=%u [1/%d s] PLT=%u VLT=%u"),
                iScope[0], &iName, &tmp, &id->LogAddressType(), &id->LogAddressState(), elapsed_units, TIMER_UNIT, id->iPLT, id->iVLT);
#endif
            // Some timers and values are different for IPv4 link local
            const TInt is_ip4_local = id->iId.IsV4Mapped() && id->iId.Scope() == KIp6AddrScopeLinkLocal;
            const TUint retrans_timer = is_ip4_local ? CIp6Manager::TimerUnits(iND.iIPv4RetransTimer) : iRetransTimer;
            const TUint dup_transmits = is_ip4_local ? iND.iIPv4DupAddrDetectTransmits : iND.iDupAddrDetectTransmits;
            have_address = 1; // ..although, it may be tentative!
            if (aStamp < id->iCreated)
                {
                // Can only happen when iCRT is set to future (may happen with delay > 0 setting)
                const TUint time = ElapsedUnits(aStamp, id->iCreated);
                if (next_event > time)
                    next_event = time;
                }
            else
                {
                //
                // goto's used, sorry. It's just simpler this way (without replicating code)
                //
                TUint time = id->iNS * retrans_timer;
                if (id->IsTentative())
                    {
                    if (id->iNS == 0)
                        {
                        // This branch is to "fix" sluggish RunL() calls. When a timeout is scheduled
                        // the call to RunL gets sometimes delayed and CRT < Current Time. If this is
                        // the first DAD NS, adjust CRT to the current real time.
                        id->iCreated = aStamp;
                        elapsed_units = 0;
                        }
                    //
                    // Duplicate Address Detection
                    //
                    // Time to send another NS?
                    if (time <= elapsed_units)
                        {
                        if (id->iNS < dup_transmits)
                            {
                            id->iNS++;
                            LOG(Log::Printf(_L("\tIF %u [%S] Sending %d. NS for %S"), iScope[0], &iName, id->iNS, &tmp));
                            // note: source address is forced to be NONE!
                            (void)SendNeighbors(KInet6ICMP_NeighborSol, NULL, id->iId, &KInet6AddrNone);
                            time = retrans_timer;   // schedule next event at retrans timer
                            }
                        else
                            {
                            id->SetInitial(0);      // Done!
                            
                            // Generate an event indicating DAD is complete (iState has changed)
                            NotifyAddressEvent(EventTypeModify, id->iId, id->iPrefix, NULL, *id);
                            
                            // Because missing source address punts flows to holding,
                            // need to do the ScanHoldings, instead of notifying just
                            // flows on current interface... -- msa (see RefreshFlow)
                            Interfacer().ScanHoldings();
                            goto announce_test; // Address ready, do the announce test!
                            }
                        }
                    else
                        time -= elapsed_units;  // compute the remaining wait time.
                    if (next_event > time)
                        next_event = time;
                    goto expiration_test;// Address not ready yet, skip over announcement test!
                    }

announce_test:  // Need to send IPv4 (link local) announcements?
                if (is_ip4_local && NeedsND())
                    {
                    //
                    // For IPv4 link local addresses, send 2 extra "announcements" after address has been accepted
                    //
                    if (id->iNS < dup_transmits + iND.iIPv4DupAddrAnnouncements)
                        {
                        // Time to send another announcement?
                        if (time <= elapsed_units)
                            {
                            id->iNS++;
                            LOG(Log::Printf(_L("\tIF %u [%S] Sending %d. NS (announce) for %S"), iScope[0], &iName, id->iNS, &tmp));
                            (void)SendNeighbors(KInet6ICMP_NeighborSol, NULL, id->iId, &id->iId);
                            time = retrans_timer;
                            }
                        else
                            time -= elapsed_units;
                        if (next_event > time)
                            next_event = time;
                        }
                    }

                // Just remember one id/address block for which lifetime processing
                // is required (don't want to it inside the loop, because it may
                // require removal and/or addition of new entries, and the loop would
                // need to be more complex... -- msa)
expiration_test:
                if (id->iPLT < elapsed_units)
                    privacy = id;
                else if (id->iPLT != KLifetimeForever)
                    {
                    const TUint time = id->iPLT - elapsed_units;
                    if (time < next_event)
                        next_event = time;
                    }

                if (id->iVLT <= elapsed_units)
                    privacy = id;
                else if (id->iVLT != KLifetimeForever)
                    {
                    const TUint time = id->iVLT - elapsed_units;
                    if (time < next_event)
                        next_event = time;
                    }
                }
            }
        if (id->iNext == NULL)
            break;
        id = &id->iNext->iInfo;
        }
    if (privacy)
        {
        const TLifetime current_age = ElapsedUnits(privacy->iCreated, aStamp);
        // At most one entry handled at each Timeout(). As these lifetimes should be
        // relatively long, anything missed on current pass, will get handled on the
        // next round!
        if (privacy->iPLT < current_age)
            {
            // If RFC-3041 is being supported and this is randomly generated id
            // for that purpose, one should at least now (but preferrably already
            // earlier) regenerate a new random id! This id may still have valid
            // life left, so it continues to be used with old connections...
            if (privacy->iPrefix == 0)
                {
                SetPrefix(privacy->iId, 128, 1, KLifetimeForever, 0);
                }
            }
        if (privacy->iVLT < current_age)
            RemId(privacy);
        }
    // Should only be sending RS, if interface as at least one address
    // (there is no point in sending RS, if all addresses have become
    // disabled by DAD). Also, needs to be IPv6 enabled interface.
    // [Even if there are routers, must at least send one RS and get
    // one reply after such RS]
    if (have_address && CanSendRS())
        {
        //
        // Try to find if routers are available
        //
        if (iRetryRS < iND.iMaxRouterSolicitations)
            {
            const TUint interval = CIp6Manager::TimerUnits(iND.iRtrSolicitationInterval);
            // The router discovery "borrows" the creation time of the
            // primary address...
            TUint time = iRetryRS * interval;
            if (iAddress.iCreated > aStamp)
                {
                // Creation time in future, need to wait...
                time = ElapsedUnits(aStamp, iAddress.iCreated);
                }
            else
                {
                const TUint elapsed = ElapsedUnits(iAddress.iCreated, aStamp);
                if (time <= elapsed)
                    {
                    iRetryRS++;
                    LOG(Log::Printf(_L("\tIF %u [%S] Sending %d. RS"), iScope[0], &iName, iRetryRS));
                    // note: src address is not specified: it can be either none or some valid address
                    // of the interface!
                    (void)SendNeighbors(KInet6ICMP_RouterSol, NULL, KInet6AddrNone);
                    time = interval;
                    }
                else
                    time -= elapsed;
                }
            if (next_event > time)
                next_event = time;
            }
        }
    if (NeedsND())
        {
        // An experimental code for cleaning out excess
        // unused neighbor cache entries: pick the one
        // with oldest reachable confirmation and if
        // the time elapsed is long enough, delete it.
        //
        // *NOTE* During DAD/RS process, this code is
        // executed for each timeout (probably wasted
        // effort), but DAD/RS procesess are only active
        // for short period of time, so it *should* not
        // matter... -- msa
        const TUint current = User::TickCount();
        TUint oldest_time = 0;
        CIp6Route *oldest_rt = NULL;
        LOG(Log::Printf(_L("\tIF %u [%S] Neighbor cache cleanup check"), iScope[0], &iName));
        for (CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
            {
            if (rt->IsHostRoute() &&
                rt->iIsRouter == 0 &&       // ..do not expire routers...
                rt->iFlowList == NULL &&    // ..only, if no flows!
                // ..below test should work correctly even if TickCount
                // wraps around! [without the wraparound "problem", could
                // just look for the smallest timestamp... -- msa]
                oldest_time < (TUint)(current - rt->iTimeStamp))
                {
                oldest_time = current - rt->iTimeStamp;
                oldest_rt = rt;
                }
            }
        // "long enough" = ~8*iReachableTime
        if (oldest_time / 8 > iReachableTime)
            // no need to test oldest_rt != NULL! if oldest_time is
            // is non-zero, then also oldest_rt != NULL. -- msa
            RemoveRoute(oldest_rt);
        //
        // Just use the *default* reachable time as a basis for
        // repeating this loop [given in milliseconds, needs to
        // be converted into units]
        //
        // *NOTE* This means that a timer is always active for
        // ND interface entries... -- msa
        const TUint schedule = CIp6Manager::TimerUnits(KInetNdConfig.iReachableTime, 1000);
        if (next_event > schedule)
            next_event = schedule;
        }
    
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    // RFC 5006 Changes
    // Since CIP6Interface::Timeout()handles RS initiation every 30 seconds, not necessary for 5006 to do it again
    // Synchronise RDNSS server list and Repository for every 30 seconds 
       if(iRdnssList!= NULL)           
            {
#ifdef _DEBUG
            LOG(Log::Printf(_L("\tIF %u [%S] RDNSS Cache Cleanup"), iScope[0], &iName));
#endif          
            if (iRdnssList->RdnssServerListSync(iNameSer1, iNameSer2))
                {
                (void)SendNeighbors(KInet6ICMP_RouterSol, NULL, KInet6AddrNone);
                LOG(Log::Printf(_L("\tIF %u [%S]RDNSS: Sending RS"), iScope[0], &iName));
                }
            
            // Update Nameserver repository only if iRdnssFlag was reset to zero
            // Even if either iNameSer1 or iNameSer2 is elapsed iRdnssFlag was reset to zero
            // Need to update both iNameSer1/iNameSer2 with KAFUnspec or a valid DNS address from iRdnssArrayList 
            if( (((iRdnssList->GetRdnssFlag())& RDNSS_NAMESERVER1) ==0) || 
                (((iRdnssList->GetRdnssFlag())& RDNSS_NAMESERVER2)== 0) 
                )
                {
                iRdnssList->RdnssNameServerUpdate(iNameSer1,(TUint8)0);
                iRdnssList->RdnssNameServerUpdate(iNameSer2,(TUint8)1);
                LOG(Log::Printf(_L("\tIF RDNSS TABLE After SYNC")));
                TInt arrayCount = iRdnssList->CountRdnssEntry();
                for(TUint8 index =0;index<arrayCount;index++)
                    {   
                     iRdnssList->PrintRdnssServerList(index);
                    }
                }
            if(iRdnssList->GetRdnssFlag()) //Notify only if changed
             NotifyInterfaceEvent(EventTypeModify);     
            }
#endif // SYMBIAN_TCPIPDHCP_UPDATE
       
    if (next_event < KMaxTUint)
        {
        LOG(Log::Printf(_L("\tIF %u [%S] Schedule next timeout after %u/%u s"),
            iScope[0], &iName, next_event, TIMER_UNIT));
        Interfacer().SetTimerWithUnits(iTimeout, next_event);
        }
    else
        {
        LOG(Log::Printf(_L("\tIF %u [%S] Sleeps, no timeout"), iScope[0], &iName));
        CancelTimer();
        }
    }

CIp6Interface::~CIp6Interface()
    {
    // Note: Reset does some extra inits, which
    // are wasted on destructor. Should not cause
    // any problems... -- msa
    Reset();
    LOG(Log::Printf(_L("\tIF %u [%S] Deleted"), iScope[0], &iName));
    }

// CIp6Interface::Reset
// ********************
/**
// Release all attached resources and set the instance into
// initial state. Can be called any time.
//
// EXCEPTION: DO NOT TOUCH 'iNetDial' in Reset!
//
// NOTE: Also used from the class destructor!
*/
void CIp6Interface::Reset(TInt aKeepNif)
    {
    LOG(Log::Printf(_L("\tIF %u [%S] Reset(%d)"), iScope[0], &iName, aKeepNif));
    CancelTimer();  // Prevent any timers fromm firing
    //
    // Empty the hold queue
    //
    iHoldQueue.Free();
    //
    // Remove all routes
    //
    CIp6Route *rh = iRouteList;
    iRouteList = NULL;
    iRouters = 0;   // No routers left either!
    while (rh != NULL)
        {
        CIp6Route *const r = rh;
        rh = r->iNext;
        // Because ALL entries are going to be deleted, there
        // is no need to worry about iRouter pointers.
        NotifyRouteEvent(EventTypeDelete, r);
        delete r;
        }
    ASSERT(iFlows == 0);
    //
    iIsIPv6 = 0;
    iIsIPv4 = 0;
    iIsRouter = 0;
    iIsSuspended = 0;
    iIpv4Linklocal = EV4LLUnknown;
    //
    // Remove address information
    //
    TIp6AddressInfo ai = iAddress;
    iAddress.iNext = NULL;
    iAddress.iId = KInet6AddrNone;
    iAddress.SetNoAddress();
    iAddress.iIpv4LinkLocal = 0;
    for (;;)
        {
        if (ai.IsSet())
            NotifyAddressEvent(EventTypeDelete, ai.iId, ai.iPrefix, NULL, ai);
        CIp6Address *const a = ai.iNext;
        if (a == NULL)
            break;
        ai = a->iInfo;
        delete a;
        }

    // Reset the scope vector: fill all scope levels
    // with unique non-zero id by loading a complement
    // of the interface index into them.
    for (TInt i = 1; i <= EScopeType_NET; ++i)
        iScope[i] = ~iScope[0];

    iTimeStamp.UniversalTime(); // A new "birthday" for the interface
    iState = KErrNone;  // is this needed?? -- msa

    // Reset some other address fields
    iHwAddr.SetFamily(0);
    iNameSer1.Init(0);
    iNameSer2.Init(0);


    iSMtu = 0;
    iRMtu = 0;
    iPMtu = 0;

    // Reset default hoplimit from the configured default
    iHopLimit = Interfacer().iMaxTTL;
    //
    // Reset ND parameters (whether needed or not)
    //
    iND = KInetNdConfig;
    SetReachableTime(); // Initial value
    SetRetransTimer();  // Initial value
    //
    // Remove NIFMAN/Interface associations
    //
    if (aKeepNif == 0 && iNifIf)
        {
        // ...notify network layer, in case any hook is interested.
        // Use temporary safe "nif" variable, because there could
        // some callbacks from the hooks within the InterfaceDetached
        // method(s).
        CNifIfBase *const nif = iNifIf;
        iNifIf = NULL;
        if (iNifUser->iNetwork)
            iNifUser->iNetwork->InterfaceDetached(iName, nif);
        nif->Close();
        }
#ifdef SYMBIAN_TCPIPDHCP_UPDATE		
    //RFC-5006 Changes for RDNSS option
    delete iRdnssList;
    iRdnssList = NULL;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
    }


TInt CIp6Interface::HaveIp4LinkLocal()
    /**
    * Tells whether IPv4 link-local addresses are in use.
    * The possible return values are enumerated in EV4LLEnums.
    */
    {
    // - If the IPv4 Link-local parameter was already set for this interface, use that
    // - If it was not set, apply the following search order when looking for ipv4linklocal
    //   1. [interface name] - specific section in tcpip6.ini
    //   2. [ip] - section in tcpip6.ini
    if (iIpv4Linklocal != EV4LLUnknown)
        return iIpv4Linklocal;
        
    TInt value = 0;
    if (!Interfacer().FindVar(iName, TCPIP_INI_IPV4LINKLOCAL, value))
        iIpv4Linklocal = Interfacer().iIpv4Linklocal;
    else if (value < 0 || value > 3)
        iIpv4Linklocal = Interfacer().iIpv4Linklocal;
    else
        iIpv4Linklocal = value;
        
    return iIpv4Linklocal;
    }


//
// **************************
// CIp6Manager Implementation
// **************************
//
//
CIp6Manager::CIp6Manager() : iTimeout(CIp6ManagerTimeoutLinkage::Timeout)
    {
    }

CIfManager *CIfManager::NewL()
    {
    CIp6Manager *mgr = new (ELeave) CIp6Manager();
    CleanupStack::PushL(mgr);
    mgr->InitL();
    CleanupStack::Pop();
    return mgr;
    };


TInt CIp6Manager::GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aDefault, TInt aMin, TInt aMax)
    /**
    * Get tcpip.ini value.
    *
    * @param aSection The [section] name
    * @param aName The variblae name
    * @param aDefault The value returned, if variable is undefined, invalid or out of range.
    * @param aMin The minimum allowed value
    * @param aMax The maximum allowed value
    *
    * @return The either the aDefault, or value parsed from the ini file.
    */
    {
    LOG(_LIT(KFormat, "\t[%S] %S = %d"));
    LOG(_LIT(KFormatInv, "\t[%S] %S = %d is invalid"));

    TInt value;
    if (!FindVar(aSection, aName, value))
        value = aDefault;
    else if (value < aMin || value > aMax)
        {
        LOG(Log::Printf(KFormatInv, &aSection, &aName, value));
        value = aDefault;
        }
    LOG(Log::Printf(KFormat, &aSection, &aName, value));
    return value;
    }

void CIp6Manager::InitL()
    {
    LOG(Log::Printf(_L("--- tcpip6 starting, version: [%S] ---"), &KInet6Version));

    // Create EventManager instance. May return NULL.
    iEventManager = MEventService::CreateEventManager(KNumClassesTcpIp6);
    
        {
        // ini parameter value determines the destination cache granularity
        // [ separate cache entry either 1=per address or 2=per prefix ]
        const TInt keymode = GetIniValue(TCPIP_INI_IP, TCPIP_INI_DSTCACHE, 0, 0, 2);
        if (keymode)
            {
            // Create Destination Cache instance. May return NULL
            iDestinationCache = MDestinationCache::CreateDstCache(keymode);
            if (iDestinationCache)
                {
                iDestinationCache->SetLifetime(GetIniValue(TCPIP_INI_IP, TCPIP_INI_DST_LIFETIME, KDstCacheLifetime, 1, KMaxTInt));
                iDestinationCache->SetMaxSize(GetIniValue(TCPIP_INI_IP, TCPIP_INI_DST_MAXSIZE, KDstCacheMaxSize, 0, KMaxTInt));
                }
            }
        }
    //
    // Create the NIF "proxies" for the protocols
    //
    for (TInt i = 0; i < (TInt)(sizeof(iNifUser) / sizeof(iNifUser[0])); ++i)
        iNifUser[i] = new (ELeave) CIp6NifUser(*this, i == E_IPv4);

    // Just put something non-zero into iSeed
    // (should use something other than 'this', as it may leak unwanted
    // information out. Also, not random enough, if multiple identical
    // devices are on the same net booting at same time -- fix sometime -- msa)
    const TUint32 seed[2] = {(TUint32)this, ~(TUint32)this};
    iSeed = *(const TInt64*)&seed;
    //
    // Compute maximum interval in seconds, that can be expressed in ticks difference
    //
    TReal interval = (TReal)KMaxTInt * TickPeriod() / 1000000.0;
    (void)Math::Int((TInt32 &)iMaxTickInterval, interval);
    LOG(Log::Printf(_L("\tMaxTickInterval = %d\n"), (int)iMaxTickInterval));

    //
    // Create Timer Service
    //
    iTimeoutManager = TimeoutFactory::NewL(TIMER_UNIT, this, KTcpipIni_TimeoutPriority);
    //
    // Create a special holding route.
    // Load the route with 128 prefix and no address (= illegal destination)
    // => Thus, holding route is never selected by plain FindRoute! -- msa
    AddRouteL(KInet6AddrNone, 128, _L(""));
    //
    // At this stage, the above route should be the *ONLY* one existing
    // iHoldingRoute is just a copy pointer to special route entry.
    // DO not use delete on it!
    //
    iHoldingRoute = iInterfaceList->iRouteList;
    if (iHoldingRoute == NULL)
        // In this case AddRouteL returns and fails to
        // create first entry to the iRouteList only if
        // route allocation fails due to heap. There is
        // no point in going on with stack initialize,
        // just leave (and and shutdown).
        User::Leave(KErrNoMemory);

    ASSERT(iHoldingRoute->iNext == NULL);
    iHoldingRoute->iState = CIp6Route::EHolding;

    //
    // Default TTL/HopLimit
    //
    iMaxTTL = (TUint8)GetIniValue(TCPIP_INI_IP, TCPIP_INI_MAXTTL, KTcpipIni_Maxttl, 0, 255);
    //
    // Default TTL/HopLimit for link local unicast targets
    // (if value is negative, the default is same as for normal unicast)
    //
    iLinkLocalTTL = GetIniValue(TCPIP_INI_IP, TCPIP_INI_LINKLOCALTTL, KTcpipIni_LinkLocalttl, -1, 255);

    // Set default how interface errors should affect the flows
    // (If flag is 1, then interface error just moves the flow to pending
    // state and waits for the same or some other interface to become
    // available again..)
    iNoInterfaceError = (GetIniValue(TCPIP_INI_IP, TCPIP_INI_NOIFERROR, KTcpipIni_Noiferror) != 0);

    // Set default whether a flow should be counted on the interface
    // or not. When counted, positive count on interface will cause
    // the NIF OpenRoute() called, and when count reaches ZERO, the
    // CloseRoute() is called.
    iKeepInterfaceUp = (GetIniValue(TCPIP_INI_IP, TCPIP_INI_KEEP_INTERFACE_UP, KTcpipIni_KeepInterfaceUp) != 0);

    // iMaxHoldingTime sets the approximate time limit for a flow to be waiting
    // for a route or netdial completion in pending state.
    // A value 0 can be used to disable expiration of the hold (= "infinite" time)
    iMaxHoldingTime = (TUint)GetIniValue(TCPIP_INI_IP, TCPIP_INI_MAXHOLDTIME, KTcpipIni_Maxholdtime, 0, iMaxTickInterval);

    // iShutdownDelay defines the time to wait before daemons are killed
    // after last user leaves the stack.
    iShutdownDelay = GetIniValue(TCPIP_INI_IP, TCPIP_INI_SHUTDOWN_DELAY, KTcpipIni_ShutdownDelay, 0, KMaxTInt);

    // iIpv4Linklocal enables automatic IPv4 link local addresses from
    // 169.254.0.0/16
    iIpv4Linklocal = GetIniValue(TCPIP_INI_IP, TCPIP_INI_IPV4LINKLOCAL, KTcpipIni_Ipv4Linklocal, 0, 3);
        
    // Allow disabling a part of "DIID" (if set, do not defend my
    // interface ID's aggressively.
    iNoDefendId = (GetIniValue(TCPIP_INI_IP, TCPIP_INI_NODEFENDID, KTcpipIni_NoDefendId) != 0);

    // Control ND probing. If enabled, stack probes for the destination
    // address on all interfaces in the scope, when destination does not
    // have a route (e.g. if system has no default route).  This setting
    // is ignored if disabled if the source address is an IPv4 link local
    // or else the link local would not function except for neighbours
    // with routes already discovered and cached.  This is necessary for
    // compliance with the ZEROCONF RFC.
    iProbeAddress = (GetIniValue(TCPIP_INI_IP, TCPIP_INI_PROBEADDRESS, KTcpipIni_ProbeAddress) != 0);

    // Control support of the Route Information option. The option is enabled, if the value
    // is non-zero. The value defines the option type to be used.
    iRA_OptRoute = (TUint8)GetIniValue(TCPIP_INI_IP, TCPIP_INI_RA_OPT_ROUTE, 0, 0, 255);
    
#ifndef SYMBIAN_TCPIPDHCP_UPDATE 
    // Control support of the RDNSS option. The option is enabled, if the value
    // is non-zero. The value defines the option type to be used.
    iRA_OptDns = (TUint8)GetIniValue(TCPIP_INI_IP, TCPIP_INI_RA_OPT_RDNSS, 0, 0, 255);
#endif //SYMBIAN_TCPIPDHCP_UPDATE
    }

TBool CIp6Manager::LoadConfigurationFile()
    {
    if (iConfig)
        return TRUE;    // Already loaded!
    // if iConfigErr != 0 (KErrNone), then an attempt for
    // loading configuration has been made and failed,
    // assume it never will succeed and avoid further
    // attemps on each FindVar...
    if (iConfigErr)
        return FALSE;
    LOG(Log::Printf(_L("CIp6Manager::LoadConfigurationFile(): %S"), &TCPIP_INI_DATA));
    TRAP(iConfigErr, iConfig = CESockIniData::NewL(TCPIP_INI_DATA));
    return (iConfig != NULL);
    }

//
// Access to the configuration file (tcpip.ini)
//
TBool CIp6Manager::FindVar(const TDesC &aSection, const TDesC &aVarName, TPtrC &aResult)
    {
    if (LoadConfigurationFile())
        {
        ASSERT(iConfig);    // <-- lint gag
        return iConfig->FindVar(aSection, aVarName, aResult);
        }
    return FALSE;
    }

TBool CIp6Manager::FindVar(const TDesC &aSection, const TDesC &aVarName, TInt &aResult)
    {
    if (LoadConfigurationFile())
        {
        ASSERT(iConfig);    // <-- lint gag
        return iConfig->FindVar(aSection, aVarName, aResult);
        }
    return FALSE;
    }

CIp6Manager::~CIp6Manager()
    {
    LOG(Log::Printf(_L("~CIp6Manager()")));

    CancelTimer();

    StopDaemons();

    // Release all interfaces

    while (iInterfaceList)
        {
        CIp6Interface *iface = iInterfaceList;
        iInterfaceList = iface->iNext;
        delete iface;
        }
    delete iTimeoutManager;
    delete iConfig;
    //
    // Destroy NIF "proxies"
    //
    for (TInt i = 0; i < (TInt)(sizeof(iNifUser) / sizeof(iNifUser[0])); ++i)
        delete iNifUser[i];

    ASSERT(iFlows == 0);

    delete iEventManager;
    delete iDestinationCache;
    LOG(Log::Printf(_L("--- tcpip6 finished, version: [%S] ---"), &KInet6Version));
    }

// *************************
// CIp6Manager::StartDaemons
// *************************
void CIp6Manager::StartDaemons()
    {
    ASSERT(iDaemons == NULL);
    LOG(Log::Printf(_L("CIp6Manager::StartDaemons()")));

    //
    // Create and start Daemons (as specified in TCPIP.INI)
    //
    TPtrC daemons;
    if (FindVar(TCPIP_INI_START, TCPIP_INI_DAEMONS, daemons))
        {
        TLex start(daemons);
        while (!start.Eos())
            {
            start.Mark();
            while (!start.Eos() && start.Peek() != ',')
                start.Inc();

            TPtrC demon = start.MarkedToken();
            if (!start.Eos())
                start.Inc();    // Skip ','
            TPtrC name;
            if (FindVar(demon, TCPIP_INI_FILENAME, name))
                {
                CIp6Daemon *d = new CIp6Daemon;
                if (!d)
                    return; // should probably tell someone about this fail!
                d->iNext = iDaemons;
                iDaemons = d;
                d->Start(demon, name);
                }
            else
                {
                // Should probably report this. It's an TCPIP.INI error
                // to reference a daemon section that does not exist!
                LOG(Log::Printf(_L("CIp6Manager::InitL() '[%S] filename' not found\n"), &demon));
                }
            }
        }
    }

void CIp6Manager::StopDaemons()
    {
    LOG(Log::Printf(_L("CIp6Manager::StopDaemons()")));
    // Kill active daemons
    //
    while (iDaemons)
        {
        CIp6Daemon *const d = iDaemons;
        iDaemons = iDaemons->iNext;
        delete d;
        }
    }

// CIp6Manager::Timeout
// ********************
/**
// The manager Timeout is currently used only for "sluggish
// shutdown", so implentation is very simple...
*/
void CIp6Manager::Timeout(const TTime & /*aStamp*/)
    {
    LOG(Log::Printf(_L("CIp6Manager::Timeout() iUsers=%d"), (int)iUsers));
    if (iUsers == 0)
        StopDaemons();
    }


// CIp6Manager::TimerUnits
// ***********************
//
TUint CIp6Manager::TimerUnits(const TUint aDelay, const TUint aUnit)
    /**
    * Convert a delay time to internal timer units.
    *
    * If the converted result would exceed KMaxTUint, then
    * KMaxTUint is returned.
    *
    * @param aDelay     The timer delay in specified units
    * @param aUnit      The unit definition (fraction of second, [1..1000000])
    * @return The delay in internal units [0..KMaxTUint].
    */
    {
    if (aUnit == TIMER_UNIT)
        return aDelay;

#ifdef MAKE_TINT64
    const TInt64 delay = (MAKE_TINT64(0U, TIMER_UNIT) * MAKE_TINT64(0U, aDelay) + MAKE_TINT64(0U, aUnit - 1)) / MAKE_TINT64(0U, aUnit);
    return I64HIGH(delay) ? KMaxTUint : I64LOW(delay);
#else
    const TInt64 delay((TInt64(0U, TIMER_UNIT) * TInt64(0U, aDelay) + TInt64(0U, aUnit - 1)) / TInt64(0U, aUnit));
    return delay.High() ? KMaxTUint : delay.Low();
#endif
    }


// CIp6Manager::SetTimerSeconds
// ****************************
void CIp6Manager::SetTimer(RTimeout &aHandle, TUint32 aDelay)
    /**
    * Set the timeout event on a handle.
    *
    * @param aHandle    The timeout handle
    * @param aDelay     The timer delay in seconds
    */
    {
#if TIMER_UNIT == 1
    // Timer unit is 1s, aDelay can be used as is.
    iTimeoutManager->Set(aHandle, aDelay);
#else
    // Timer unit not 1s, aDelay must be adjusted.
    static const TUint KMaxDelay = KMaxTUint / TIMER_UNIT;
    iTimeoutManager->Set(aHandle, aDelay > KMaxDelay ? KMaxTUint : aDelay * TIMER_UNIT);
#endif
    }

// CIp6Manager::IncUsers
// *********************
// Increment users
//
void CIp6Manager::IncUsers()
    {
    iUsers++;
    LOG(Log::Printf(_L("\t\tIncUsers: Users=%d Nifs=%d"), iUsers, iNifCount));
    if (iDaemons == NULL)
        StartDaemons();
    }

// CIp6Manager::DecUsers
// *********************
// Decrement users count
//
void CIp6Manager::DecUsers()
    {
    __ASSERT_ALWAYS(iUsers > 0, User::Panic(_L("iUsers"), iUsers));
    if (--iUsers == iNifCount)
        {
        // Only interfaces remain, no real clients
        Nif::NetworkLayerClosed(*iNifUser[E_IPv6]);
        Nif::NetworkLayerClosed(*iNifUser[E_IPv4]);
        }
    LOG(Log::Printf(_L("\t\tDecUsers: Users=%d Nifs=%d"), iUsers, iNifCount));
    if (iUsers == 0)
        {
        //StopDaemons();
        //
        // Sometimes aggressive shutdown causes problems in system
        // components. Use a small delay before really activating
        // the daemon killer...
        SetTimer(iShutdownDelay);
        }
    }

// CIp6Manager::PacketAccepted
// ***************************
//
TInt CIp6Manager::PacketAccepted(const TUint32 aIndex)
    {
    const CIp6Interface *const iface = FindInterface(aIndex);
    if (iface)
        {
        if (iface->iNifIf && iface->iNifIf->Notify())
            return iface->iNifIf->Notify()->PacketActivity(EIncoming, 0, TRUE);
        return KErrNone;
        }
    return KErrNotFound;
    }

//
// CIp6Manager::NewFlow
// ********************
// Create a new flow instance
//
CFlowContext *CIp6Manager::NewFlowL(const void *aOwner, MFlowManager *aManager, TUint aProtocol)
    {
    CFlowContext *flow = new (ELeave) CIp6Flow(aOwner, aManager, *this, aProtocol);
    return flow;
    }

CFlowContext *CIp6Manager::NewFlowL(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow)
    {
    return new (ELeave) CIp6Flow(aOwner, aManager, *this, aFlow);
    }


// CIp6Manager::Register
// *********************
// Attach a protocol to NIF "proxy"
MNifIfUser *CIp6Manager::Register(MNetworkServiceExtension *aNetwork)
    {
    TServerProtocolDesc info;
    CIp6NifUser *ifuser = NULL;

    aNetwork->Protocol()->Identify(&info);
    if (info.iAddrFamily == KAfInet6)
        {
        (ifuser = iNifUser[E_IPv6])->iNetwork = aNetwork;
        }
    else if (info.iAddrFamily == KAfInet)
        {
        (ifuser = iNifUser[E_IPv4])->iNetwork = aNetwork;
        }
    return ifuser;
    }

//
// CIp6Manager::Unregister
// ***********************
// Remove all references to the specified protocol
void CIp6Manager::Unregister(MNetworkServiceExtension *aNetwork)
    {
    for (TInt i = 0; i < (TInt)(sizeof(iNifUser) / sizeof(iNifUser[0])); ++i)
        if (iNifUser[i]->iNetwork == aNetwork)
            iNifUser[i]->iNetwork = NULL;
    }

// CIp6Manager::IcmpSend
// *********************
/**
// Wrap a packet into ICMP error reply and send it out.
//
// Delegate the task to the network instance MNetworkServiceExtension::IcmpWrap method.
//
// @param aPacket
//  The RMBuf chain containing the IP packet in packet state
// @param aIcmp
//  The 32 bit value containing type and code for both IPv4 and IPv6. The type and
//  code to be used are chosen based on the actual IP version of the packet.
// @param aParameter
//  The 32 bit value to be placed as the "parameter" field of the ICMP header.
// @param aMC
//  A flag, when non-Zero, forces sending of ICMP, even if the packet destination
//  was a multicast address (see MNetworkService::Icmp4Send and
//  MNetworkService::Icmp6Send).
*/
void CIp6Manager::IcmpSend(RMBufChain &aPacket, const TIcmpTypeCode aIcmp, const TUint32 aParameter, const TInt aMC)
    {
    // Just use any "network" instance, and assume it works just
    // as well whether IPv4 or IPv6...
    MNetworkServiceExtension *const network = iNifUser[E_IPv6]->iNetwork ? iNifUser[E_IPv6]->iNetwork : iNifUser[E_IPv4]->iNetwork;
    if (network)
        network->IcmpWrap(aPacket, aIcmp, aParameter, aMC);
    //
    // Release packet (if not passed on)
    //
    aPacket.Free();
    return;
    }

//
// CIp6Manager::IcmpError
// **********************
/**
// Gets a peek at all received ICMP error messages before they
// are passed to the upper layers. This is called from the
// IcmpError() method of the IP layer.
//
// @return
// @li < 0, if packet has been released (packet will not
//          go to the upper layer after this),
// @li = 0, the usual return, packet looked and it can be
//          passed to the upper layers
// @li > 0, *NOT USED NOW*, Treat as = 0 as default
*/
TInt CIp6Manager::IcmpError(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
    {
    // note: iParameter is fixed by ICMP protocol as 32 bit entity.
    // Assume that "natural" TUint will always be at least 32 bits
    // (compiler should warn, if this is not true).
    TUint mtu = aInfo.iParameter;
    
    //
    // Currently the only interesting thing here is to detect the
    // MTU affecting ICMP's for both IPv6 and IPv4.
    //
    if (aInfo.iIcmp == KProtocolInet6Icmp)
        {
        // IPv6 ICMP Errors
        //
        // The only insteresting error is KInet6ICMP_PacketTooBig,
        //
        if (aInfo.iType != KInet6ICMP_PacketTooBig)
            return 0;
        if (mtu < STATIC_CAST(TUint,KInet6MinMtu))
            return 0;       // Cannot set it smaller than minimum MTU!
        }
    else if (aInfo.iIcmp == KProtocolInetIcmp)
        {
        // IPv4 ICMP Errors
        //
        if (aInfo.iType == KInet4ICMP_Redirect)
            {
            CIp6Interface *const ifp = FindInterface(aInfo.iInterfaceIndex);
            if (ifp)
                ifp->Ip4RedirectHandler(aPacket, aInfo);
            return 0;
            }
        // The only interesting thing is the Unreachable/Fragmentation needed
        // (RFC-792 + RFC-1192)
        if (aInfo.iType != KInet4ICMP_Unreachable || aInfo.iCode != 4)
            return 0;
        //
        // Check the parameter as per RFC-1192 (additions to RFC-972)
        if (mtu == 0)
            mtu = 576;      // ICMP does not include next hop MTU, use 576
        else
            {
            mtu &= 0xffff;  // Mask off the "unused high order bits"
            if (mtu < STATIC_CAST(TUint,KInetMinMtu))
                return 0;   // Cannot be smaller than 68!
            }           
        }
    else
        return 0;
    //
    // A new minimum mtu value has been received...
    //
    //
    // We assume the info still holds the original data as set in the
    // interface.
    //
    const CIp6Interface *const iface = FindInterface(aInfo.iInterfaceIndex);
    if (iface == NULL)
        return 0;       // Can't locate interface (should not happen
                        // unless interface actually died after this
                        // packet got in).
    //
    // Poor man's Path MTU algorithm. Just maintain single Path MTU
    // for a interface. [first obvious optimization would be to use
    // interface MTU at least for the local addresses]. Enhance as
    // needed later.. -- msa
    //
    if (mtu >= (TUint)iface->iPMtu)
        return 0;       // Too Big error MUST NOT increase existing Path MTU!

    //
    // Path MTU has been decreased, notify all affected flows!
    //
    iface->NotifyFlowsPmtu(mtu);

    // Store path mtu value to destination cache
    if (iDestinationCache)
        {
        TInetAddr dst = TInetAddr::Cast(aInfo.iDstAddr);
        const TCacheInfo *ci = iDestinationCache->Find(dst);

        // valid cached PMTU must not be increased
        if (!ci || ci->iMetrics[TCacheInfo::EPathMTU] > mtu)
            {
            // Nothing useful to be done here with error code.
            TRAP_IGNORE(iDestinationCache->SetL(dst, TCacheInfo::EPathMTU, mtu));
            }
        }

    return 0;
    }


// CIp6Manager::MoveToHolding
// **************************
//
void CIp6Manager::MoveToHolding(CIp6Flow &aFlow) const
    {
    aFlow.iChanged = 1;         // When put on hold, always require a reconnect
    if (iHoldingRoute == NULL)
        return; // Should never happen...
    if (aFlow.iStatus >= 0) // ..don't overwrite error states
        aFlow.iStatus = EFlow_PENDING;

    if (aFlow.iRoute != iHoldingRoute)
        {
        // This is ugly: a flow can be in holding, but at arrival
        // of router advertisement with default route, it will
        // attach to the interface (and off holding). However, if
        // source address is not available, it gets punted back
        // to holding, with unfortunate side effect of resetting
        // the time stamp and preventing expire. A somewhat ugly
        // quick solution: time stamp is set only when it's zero,
        // and RefreshFlow is changed to zero the stamp when it
        // gets past source address selection... -- msa
        if (aFlow.iTimeStamp == 0)
            // NOTE: on rare accounts, TickCount can also be 0,
            // but, it should not break things too much...
            aFlow.iTimeStamp = User::TickCount();
        iHoldingRoute->Attach(aFlow);
        if (!iHoldingRoute->IsTimerActive())
            iHoldingRoute->Timeout();
        }
    }

void CIp6Manager::MoveToHolding(CIp6Route &aRoute) const
    {
    if (iHoldingRoute != &aRoute)
        {
        while (aRoute.iFlowList != NULL)
            MoveToHolding(*aRoute.iFlowList);
        }
    }

// CIp6Manager::ScanHoldings
// *************************
//
void CIp6Manager::ScanHoldings()
    {
    iScanHolding = 0;
    if (!iHoldingRoute)
        return;
    TFlowNotifyList list;
    for (CIp6Flow *f = iHoldingRoute->iFlowList; f != NULL; f = f->iNext)
        list.Insert(*f);
    list.Deliver(EFlow_READY);
    }

//
// CIp6Manager::GetInterfaceByNameL
// ********************************
/**
// Get interface by name (and create a new entry, if not found)
//
// @returns non-NULL always or leaves
*/
CIp6Interface *CIp6Manager::GetInterfaceByNameL(const TDesC &aName)
    {
    //
    // Look if the interface already exists
    //
    CIp6Interface **h, *iface;

    for (h = &iInterfaceList;; h= &iface->iNext)
        {
        if ((iface = *h) == NULL)
            {
            //
            // A new interface, create an instance
            //
            // *NOTE*
            //  1)  assume iInterfaceIndex is an ever increasing sequence
            //      (some worry about wrap around... -- msa)
            //  2)  new interfaces are always added to the *END* of
            //      the list
            //  =>  The interfaces on the list are *ALWAYS* in increasing
            //      order by their iIndex fields!
            //  **  The above FACT is relied on in other methods, such
            //      as InterfaceInfo!
            //
            iface = new (ELeave) CIp6Interface(*this, ++iInterfaceIndex, aName);
            iface->iNext = NULL;
            iface->iNifUser = iNifUser[0];  // Doesn't matter whether IPv4 or IPv6 (it will be changed as needed).
            ASSERT(iNifUser[0] != NULL);
            *h = iface;
            iface->Reset(); // ..some members have non-ZERO defauls, so Reset is also called!
            break;
            }
        else if (aName.Compare(iface->iName) == 0)
            break;      // Interface with specified name already exists
        }
    return iface;
    }

// CIp6Manager::RemoveInterface
// ****************************
// Brutally tear the specified interface down and release all associated resources
void CIp6Manager::RemoveInterface(CIp6Interface *aIf)
    {
    CIp6Interface **h, *iface;

    for (h = &iInterfaceList; (iface = *h) != NULL; h= &iface->iNext)
        if (iface == aIf)
            {
            // Remove from the list and destroy!
            *h = iface->iNext;
            delete iface;
            return;
            }
    // Should NEVER get here!
    ASSERT(0);
    }

// CIp6Manager::FindRoute
// **********************
/**
// Locate route entry who has the longest matching
// prefix with destination. (A default route can be
// expressed with zero length prefix, which will match
// any address)
//
// Only interfaces with matching ScopeId are searched.
*/
CIp6Route *CIp6Manager::FindRoute
        (const TIp6Addr &aDst, const TUint32 aDstId, const TUint aDstType,
         const TIp6Addr &aSrc, const TUint32 aSrcId) const
    {
    //
    // Search Over all interfaces "within scope"
    //
    CIp6Route *route = NULL;

    if (aDstType > EScopeType_NET)
        return NULL;

    if (!TIp46Addr::Cast(aSrc).IsUnspecified())
        {
        // Find route by source and destination (need to do the hard way in
        // case we allow the same source address with multiple interfaces).
        // [easy way would be: find interface with the address and just check
        // route on it -- msa]
        //
        const TUint srcType = (TUint)(aSrc.Scope() - 1);
        if (srcType > EScopeType_NET)
            return NULL;

        for (const CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
            {
            if ((aSrcId == 0 || ifp->iScope[srcType] == 0 || ifp->iScope[srcType] == aSrcId) &&
                (aDstId == 0 || ifp->iScope[aDstType] == 0 || ifp->iScope[aDstType] == aDstId) &&
                ifp->IsMyAddress(aSrc))
                route = ifp->FindRoute(aDst, route);
            }
        }
    else if (aSrcId != 0)
        {
        // When a source address is not specified, but source id is given, then it is
        // assumed that the source ID is the interface index and will limit the route
        // search to a specific interface.
        for (const CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
            if (ifp->iScope[0] == aSrcId)
                {
                route = ifp->FindRoute(aDst, route);
                break;
                }
        }
    else
        {
        // Find route by destination alone
        for (const CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
            {
            if ((aDstId == 0 || ifp->iScope[aDstType] == 0 || ifp->iScope[aDstType] == aDstId))
                route = ifp->FindRoute(aDst, route);
            }
        }
    return route;
    }


// CIp6Manager::ProbeDestination
// *****************************
void CIp6Manager::ProbeDestination
        (const TIp6Addr &aDst, const TUint32 aDstId, const TUint aDstType,
         const TIp6Addr &aSrc, const TUint32 aSrcId) const
    {
    // No probing if destination is not fully defined

    if (!aDstId || aDstType > EScopeType_NET)
        return;

    if (!TIp46Addr::Cast(aSrc).IsUnspecified())
        {
        // The source address limits the valid interfaces for probing.
        //
        const TUint srcType = (TUint)(aSrc.Scope() - 1);
        if (srcType > EScopeType_NET)
            return;

        for (CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
            {
            if (ifp->NeedsND() &&
                ifp->iScope[srcType] == aSrcId &&
                ifp->iScope[aDstType] == aDstId
                )
                {
                TIp6AddressInfo *info = ifp->IsMyAddress(aSrc);
                
                // We override the probe address setting for link local source addresses -
                // they are not routable and therefore useless otherwise.
                if( info && ( iProbeAddress || info->iIpv4LinkLocal ) )
                    {
                    (void)ifp->StartProbeND(aSrc, aDst);
                    }
                }
            }
        }
    else if (aSrcId != 0)
        {
        // When a source address is not specified, but source id is given, then it is
        // assumed that the source ID is the interface index and will limit the
        // search to a specific interface.
        CIp6Interface *const ifp = FindInterface(aSrcId);
        TIp6Addr src;
        if (ifp && ifp->NeedsND())
            {
            if( ifp->SelectSource(src, aDst) )
                {
                if( iProbeAddress )
                    {
                    (void)ifp->StartProbeND(src, aDst);
                    }
                else
                    {
                    TIp6AddressInfo *info = ifp->IsMyId( src ); // do not need to search anycast addresses
                    
                    // We override the probe address setting for link local source addresses -
                    // they are not routable and therefore useless otherwise.
                    if( info && info->iIpv4LinkLocal )
                        {
                        (void)ifp->StartProbeND(src, aDst);
                        }
                    }
                }
            else
                {
                const TIp6AddressInfo* address = ifp->FindIpv4LinkLocalAddr();

                // If no appropriate routable source address exists, start a probe if the interface
                // has an IPv4 link local address.
                if( address && address->IsAssigned() )
                    {
                    (void)ifp->StartProbeND(address->iId, aDst);
                    }
                }
            }
        }
    else
        {
        TBool probeStarted = EFalse;
        
        // Select by destination only.  If no source address can be found, start a probe
        for (CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
            {
            if (ifp->NeedsND() && ifp->iScope[aDstType] == aDstId)
                {
                TIp6Addr src;
                if (ifp->SelectSource(src, aDst))
                    {
                    if( iProbeAddress )
                        {
                        (void)ifp->StartProbeND(src, aDst);
                        
                        probeStarted = ETrue;
                        }
                    else
                        {
                        TIp6AddressInfo *info = ifp->IsMyId( src ); // do not need to search anycast addresses
                        
                        // We override the probe address setting for link local source addresses -
                        // they are not routable and therefore useless otherwise.
                        if( info && info->iIpv4LinkLocal )
                            {
                            (void)ifp->StartProbeND(src, aDst);
                        
                            probeStarted = ETrue;
                            }
                        }
                    }
                }
            }
            
        // If no interface has an appropriate routable source address, start a probe
        // on each interface with an IPv4 link local address.
        if( !probeStarted )
            {
            for (CIp6Interface *ifp = iInterfaceList; ifp != NULL; ifp = ifp->iNext)
                {
                if (ifp->NeedsND() && ifp->iScope[aDstType] == aDstId)
                    {
                    const TIp6AddressInfo* address = ifp->FindIpv4LinkLocalAddr();

                    if( address && address->IsAssigned() )
                        {
                        (void)ifp->StartProbeND(address->iId, aDst);
                        }
                    }
                }
            }
        }
    }


TInt CIp6Manager::GetDstCachePathMtu(const TIp6Addr& aDstAddress, TUint32 aScopeId) const
/**
Returns Path MTU value stored in destination cache with given IP address.

@param aDstAddress  IP destination address to be searched from the destination cache.
@param aScopeId     Scope ID of the destination. RefreshFlow() has selected this.

@return Path MTU if value was stored in destination cache. If destination cache is not available
or given address could not be found, 0 is returned.
*/
    {
    if (!iDestinationCache)
        {
        return 0;
        }
        
    TInetAddr dst(aDstAddress, 0);
    dst.SetScope(aScopeId);
    const TCacheInfo *cinfo = iDestinationCache->Find(dst);
    LOG(TLogAddressPrefix logdst(dst)); // For logging, the destination address as a string.
    if (!cinfo)
        {
        LOG(Log::Printf(_L("\t\tDstCache Path MTU for %S -- No match"), &logdst));
        return 0;
        }
        
    LOG(Log::Printf(_L("\t\tDstCache Path MTU for %S -- Found MTU = %d"),
        &logdst, cinfo->iMetrics[TCacheInfo::EPathMTU]));

    return cinfo->iMetrics[TCacheInfo::EPathMTU];
    }


void *CIp6Manager::GetApiL(const TDesC8& aApiName, TUint* aVersion)
    {
    if (aApiName == _L8("MEventService"))
        {
        if (!iEventManager)
            {
            User::Leave(KErrInetUnsupportedApi);
            }
        return EXPORT_API_L(MEventService, iEventManager, aVersion);
        }

    if (aApiName == _L8("MNetworkInfo"))
        return EXPORT_API_L(MNetworkInfo, this, aVersion);
        
    if (aApiName == _L8("MDestinationCache"))
        {
        // It is possible that the destination cache has not been instantiated
        // (e.g. due to ini param), or its initialization has failed.
        // The leave error below is not the most ideal, perhaps
        if (!iDestinationCache)
            {
            User::Leave(KErrInetUnsupportedApi);
            }
        return EXPORT_API_L(MDestinationCache, iDestinationCache, aVersion);
        }

    User::Leave(KErrInetUnsupportedApi);
    // NOTREACHED
    return NULL;
    }


CIp6Route *CIp6Interface::FindNeighbor(const TIp6Addr &aDst) const
    {
    // Only a neighbor cache entry is accepted!
    for (CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
        if (rt->IsHostRoute() && aDst.IsEqual(rt->iPrefix))
            return rt;
    return NULL;
    }


// CIp6Interface::FindRoute
// ************************
CIp6Route *CIp6Interface::FindRoute(const TIp6Addr &aDst, CIp6Route *aRoute) const
    /**
    * Locate the best route from this interface matching the destination.
    *
    * @param aDst   The destination address
    * @param aRoute The current best route, or NULL if none found yet.
    *
    * @return
    * @li if no better route is found, the return value is aRoute.
    * @li if a better route is found, return value is the new better route.
    */
    {
    // The way sending packets is now implemented, requires that if the destionation
    // is my own address, then the route assigned to the flow must indicate that
    // (must be ELoopback). [This way, the destination doesn't need to be checked
    // for every packet--a simple test for route type is enough].
    //
    // Address is my address, if the ID (which is not a proxy) matches my id AND,
    // if the prefix part fully matches one of the ELoopback routes.
    CIp6Route *rt = NULL; // silly GCC want's this to be silent...

    const TIp6AddressInfo *const id = IsMyId(aDst);
    if (id && !id->IsProxy() && (rt = IsMyPrefix(aDst, *id)) != NULL)
        {
        // ..could test if id IsAssigned(), but only "wrong"
        // thing that happens is, that a loopback route is returned
        // for the flow and no packets with tentative address will
        // go out! (and as "tentative" is very transient state,
        // this should not happen easily)
        //
        // If a destination is my address on this interface,
        // don't even consider any other routes.
        //
        return rt;
        }
    //
    // The route with best weight is selected. The weight value is
    // computed as follows
    //  -2, if no route yet
    //  -1, if route is not "reachable"
    //  prefix length, if route is "reachable"
    //
    // "reachable" an internal concept to this function and is
    // defined as: route is reachable if it is not a gateway route,
    // or if it is a gateway route and a host route to the gateway
    // exist with ISROUTE set!
    //
    // *NOTE* The gateway test intentionally excludes the ERedirect
    // gateways!


    TInt weight;
    // If destination is IPv4 address, the route must
    // match at least 96 bits to be acceptable!
    const TInt min_match = aDst.IsV4Mapped() ? 96 : 0;

    if (aRoute == NULL)
        weight = -2;
    else
        weight = (aRoute->iState != CIp6Route::EGateway || aRoute->iRouter) ? aRoute->iLength : -1;

    for (rt = iRouteList; rt != NULL; rt = rt->iNext)
        {
        if (rt->IsMyPrefix())
            continue;   // Ignore "my prefix" entries. Looking for "true" routes only
        if (rt->iIsProbing)
            continue;   // Ignore "probing only" routes (ND host route).
        if (min_match > rt->iLength)
            continue;   // Ignore too short prefixes (IPv4 needs at least 96).

        const TInt w = (rt->iState != CIp6Route::EGateway || rt->iRouter) ? rt->iLength : -1;
        if (weight > w)
            continue;   // Already better route exists, no need to compare address!
        if (rt->iPrefix.Match(aDst) < rt->iLength)
            continue;           // Does not match (full prefix must match!)
        //
        // *NOTE* if aRoute == NULL, then weight == -2 and thus cannot be equal
        // to w (always > -2) => No need to test if aRoute is NULL!!
        // Metric only affects "reachable" routes (if it affected non-"reachable",
        // system would never try an alternate gateway route with larger metric,
        // even if the better metric route would fail consistently!) -- msa
//lint -e{613}
        if (weight == w && w >= 0 && rt->iMetric > aRoute->iMetric)
            continue;   // Weights are equal, previous route has better metric.

        // *WARNING* The above test causes the behaviour that the last matching
        // route is returned, when none of the routers appear reachable. This
        // is utilized by "SelectNexthop" to implement the Round-Robin attempts
        // to contact routers, when none is reachable...

        aRoute = rt;
        weight = w;
        }
    return aRoute;
    }

// CIp6Interface::GetDefGateway 
// ************************ 
// Get the default gateway address for this interface matching the gateway address type 
//  
void CIp6Interface::GetDefGateway(const TBool aIsIPv4, TInetAddr &aAddr) const 
     { 
     static const TIp6Addr prefix = {{{0,0,0,0,0,0,0,0,0,0,0xff,0xff,0,0,0,0}}}; 
     TUint length = aIsIPv4 ? 96 : 0; 
   
     CIp6Route *located_rt = NULL; 
     for (CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext) 
         { 
         if (!rt->IsGateway()) 
             continue; 
   
         if (length != rt->iLength) 
             continue; // Ignore non-default routes 
   
         if (!aIsIPv4 && (rt->iPrefix.Match(prefix) < length)) 
             continue; // Does not match (full prefix must match!) 
   
         if (located_rt && (rt->iMetric > located_rt->iMetric)) 
             continue; // previous route has better metric 
   
         located_rt = rt; 
         } 
   
     if (located_rt) 
         located_rt->iAddress.GetAddress(aAddr); 
     else 
         aAddr.SetAddress(KInet6AddrNone); 
      
     } 

// CIp6Interface::RouterChanged
// ****************************
void CIp6Interface::RouterChanged(CIp6Route *const aRouter)
    /**
    * Maintain iRouter pointers.
    *
    * A host route entry has been changed to a router or ceased
    * to be a router. Update all gateway routes that would use
    * this router by patching the iRouter entry
    *
    * When called, the iIsRouter value must be already set to
    * the new value!
    */
    {
    ASSERT(aRouter->IsHostRoute());
    if (!aRouter->IsHostRoute())
        return;     // Should never get here (must only be called with host routes)

    if (aRouter->iIsRouter)
        {
        iRouters++;
        // Add/Overwrite pointer to the matching gateway entries
        for (CIp6Route *rt = iRouteList; rt != NULL; rt = rt->iNext)
            {
            // There should be no pointers to this route before this operation!
            ASSERT(rt->iRouter != aRouter);
            if (rt->IsGateway() &&  // Only Gateway routes are affected!
                rt->iAddress.Ip6Address().IsEqual(aRouter->iPrefix))
                rt->iRouter = aRouter;
            }
        // Should check flows in iHoldingRoute, if any could use this router now?
        }
    else
        {
        iRouters--;
        // Remove all references to the this router (if all is working as intended
        // they should all be gateway entries and address should match, but to be
        // safe, just remove all without testing (in NON-debug release)
        //
        CIp6Route **h, *rt;
        for (h = &iRouteList; (rt = *h) != NULL; )
            {
            if (rt->iRouter == aRouter)
                {
                ASSERT(rt->IsGateway() && rt->iAddress.Ip6Address().IsEqual(aRouter->iPrefix));
                rt->iRouter = NULL;
                // Punt all flows, if any, to holding route!
                // (this action may need to be considered yet).
                Interfacer().MoveToHolding(*rt);
                if (rt->iState == CIp6Route::ERedirect)
                    {
                    // If a "redirect" route loses it's target router,
                    // redirect is cancelled -- remove the redirect
                    // route from the route list.
                    *h = rt->iNext;
                    delete rt;
                    continue;
                    }
                }
            h = &rt->iNext;
            }
        }
    }

//
// CIp6Interface::SelectNextHop
// ****************************
// Returns NULL, if no usable next hop found!
CIp6Route *CIp6Interface::SelectNextHop(const TIp6Addr &aDst, const TIp6Addr &aSrc, CIp6Route *aRoute)
    {
    ASSERT(aRoute != NULL);
#ifdef _LOG
    TLogAddressPrefix logtmp(aDst);
    TLogAddressPrefix logprf(aRoute->iPrefix, aRoute->iLength);
#endif

    if (aRoute->IsGateway())
        {
#ifdef _LOG
        TLogAddressPrefix logsrc(aRoute->iAddress.Ip6Address());
//      Log::Printf(_L("\tNEXTHOP [%S] for [%S]  ROUTE %d [%S] to GATEWAY [%S]"),
//          &iName, &logtmp, aRoute->iIndex, &logprf, &logsrc);
        Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] is ROUTE %d [%S] to GATEWAY [%S]"),
            iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf, &logsrc);
#endif
        if (aRoute->iRouter == NULL)
            {
            // None of possible routers have host entries.

            // Move the selected route to the front of the
            // iRouteList to implement the "Round-Robin"
            // requirement of default router selection
            // THIS IS SOMEWHAT TRICKY! It depends on the
            // behaviour of FindRoute() method to return the
            // *LAST* matching route entry in case where no
            // routers are reachable. -- msa
            MoveToFront(aRoute);
            // Locate the neighbour cache entry for the gate (or create one if not exist yet). Do not
            // set or clear ISROUTER for IPv6 gateway! We don't know whether it is a router or not!
            // With IPv4, there ND (=ARP) has no ISROUTER feature, so we just have to assume all
            // IPv4 hosts in gateway routes are implicitly routers.
            const TUint flags = aRoute->iAddress.Ip6Address().IsV4Mapped() ? KRouteAdd_ISROUTER : 0;
            CIp6Route *const n = GetRoute(aRoute->iAddress.Ip6Address(), 128, flags);
            if (n && n->iState == CIp6Route::EIncomplete)
                {
                //
                // Nothing is known about the neighbor yet, start ND
                //
                LOG(Log::Printf(_L("\tIF %u [%S] NEXTHOP ROUTER (ND ROUTE %u) needed for [%S]"), iScope[0], &iName, n->iIndex, &logsrc));
                n->StartND(aSrc);
                }
            // If in above the neighbor cache already existed in some other state thatn EIncomplete,
            // it means that the ISROUTER is not set for this host, and it cannot be used as a router
            // currently. => Cannot use this gateway!
            //
            // Cannot assign the next hop to the gateway, because it is not yet known if
            // it is actually a router! Keep in holding!
            return NULL;
            }
        return aRoute;
        }
    if(!aRoute->IsOnlink())
        {
#ifdef _LOG
        // assume route prefix is in logtmp
        if (aRoute->IsMyPrefix())
            Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] is LOOPBACK ROUTE %u [%S]"), iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf);
        else if (aRoute->iIsMulticast)
            Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] is MULTICAST ROUTE %u [%S]"), iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf);
        else if (aRoute->IsHostRoute())
            Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] is HOST ROUTE %u [%S]"), iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf);
        else
            Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] is NOT ONLINK ROUTE %u [%S]"), iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf);
#endif
        return aRoute;
        }
    // This route is for onlink determination. Finding this
    // means that the destination/next hop is on link, but
    // no Host route for it exists yet. Create the host route.
    //
    // Note: LinkLocal destination is not handled in any special
    // way. Interface supporting link local (fe80::/10) destinations
    // must define the ONLINK route for it!
    LOG(Log::Printf(_L("\tIF %u [%S] NEXTHOP for [%S] ONLINK ROUTE %d [%S]"), iScope[0], &iName, &logtmp, aRoute->iIndex, &logprf));
    //
    // Create implicit host route entry, if ND is required on the interface
    // (For non-ND interfaces, the route is used as is)
    //
    if (NeedsND() && !((TIp46Addr &)aDst).IsMulticast())
        {
        CIp6Route *const n = GetRoute(aDst, 128, KRouteAdd_NEIGHBOR);
        if (n == NULL)
            return NULL;
        if (n->iState == CIp6Route::EIncomplete)
            n->StartND(aSrc);
        LOG(Log::Printf(_L("\tIF %u [%S] NEXTHOP HOSTROUTE (ND ROUTE %u)"), iScope[0], &iName, n->iIndex));
        return n;
        }
    return aRoute;
    }

//
// CIp6Flow::SelectNextHop
// ***********************
void CIp6Flow::SelectNextHop()
    {
    const TIp6Addr &dst = iStart.ip6.DstAddr();
    // For log prints, have destination address as a string.
    LOG(TLogAddressPrefix log_dst(dst));

    // Assume iRoute determines the route and interface. Limit nexthop
    // selection to that route only!
    for (;/* JUST FOR BREAK EXITS */;)
        {
        if (iRoute == NULL)
            {
            LOG(Log::Printf(_L("\t\tFlow[%u] OOPS? Route is NULL for %S"), this, &log_dst));
            break;  // -- no route attached
            }
        CIp6Interface &iface = iRoute->iInterface;

        if (!iStart.iSourceSet)
            {
            LOG(Log::Printf(_L("\t\tFlow[%u] IF %d [%S] has no source address for %S"), this, iface.iScope[0], &iface.iName, &log_dst));
            // If the source address cannot be assigned to this flow,
            // punt the flow back to holding route (do not leave it
            // attached to this interface). The reason is, for example
            // PPP: 
            // - has a separate interface for IPv4 and IPv6
            // - if IPv6 interface installs default route before IPv4
            //   has been configured, then IPv4 flows will try to
            //   attach to the IPv6 interface, but fail due to
            //   source address
            // - however, if they are not moved to the holding route
            //   they are stuck forever into the IPv6, even after the
            //   real IPv4 becomes configured
            // - *NOTE* IPv4 is just a case where a problem was observed,
            //   there may be other situations which would fail with IPv6
            //   in similar ways (choosing wrong interface based on
            //   incomplete configuration), thus no special kludge
            //   just for IPv4 is not designed -- msa
            // The "punt" has unfortunate side effect for ethernet:
            // - when ethernet comes up and a flow is punted to the
            //   holding because DAD has not yet completed, the flow
            //   is not wakened when DAD completes, unless DAD completion
            //   also does a ScanHolding() [see Timeout()/DAD].
            //   [Logically DAD completion should only be concern of
            //   flows attached to the interface, but this punting
            //   changes that... -- msa]
            break;
            }
        if (iStart.iInterfaceIndex != iface.iScope[0])
            {
            LOG(Log::Printf(_L("\t\tFlow[%u] Route lost, interface %u changed to IF %u [%S] for %S"),
                this, iStart.iInterfaceIndex, iface.iScope[0], &iface.iName, &log_dst));
            break;  // -- wrong interface (route was deleted probably)
            }
        CIp6Route *const route = iface.SelectNextHop(dst, iStart.ip6.SrcAddr(), iRoute);
        if (iRoute == route)
            return; // -- route not changed, all OK.
        if (!route)
            {
            LOG(Log::Printf(_L("\t\tFlow[%u] IF %u [%S] has no ND entry for %S"), this, iface.iScope[0], &iface.iName, &log_dst));
            break;  // -- no suitable route for the destination
            }
        //
        // Route changed (into neighbor cache host route)
        //
        route->Attach(*this);
        return;
        }
    iInterfacer.MoveToHolding(*this);
    }

//
// CIp6Interface::GetRoute
// ***********************
/**
// Insert a route entry to direct packets for a
// matching prefix to a specific named interface.
// (Interface is only activated after there a flow is
// activated to this route).
//
// A new route is NOT created if an identical route pointing
// to this same interface exists. In such case, the lifetime
// of the route is just updated (when lifetimes are implemented)
// It is allowed to have multiple routes with same prefix
// pointing to a *different* interface.
//
// NOTE:
//      This "add front" feature is part of the specification
//      and relied by some other parts. Beware of changing it!
//
// @return NULL or the pointer to the updated/created route entry
*/
CIp6Route *CIp6Interface::GetRoute(const TIp6Addr &aAddr, TInt aPrefix, TUint aFlags, const TSockAddr *const aGateway, const TLifetime *const aLifetime)
    {
    CIp6Manager &mgr = Interfacer();
    const TLifetime lifetime = aLifetime ? *aLifetime : KLifetimeForever;
    const TUint rtype = aFlags & (KRouteAdd_EXTENSIONMASK | KRouteAdd_TYPEMASK);

    // Initial state can only be (HOST, ONLINK, GATEWAY, MYPREFIX) with possible
    // extension bits. In debug version, just panic the operation, if extra bits
    // is present in the state. In production, the trunctated value is used as is.
    ASSERT(rtype == (aFlags & KRouteAdd_STATEMASK));

    const TLinkAddr KLinkAddr;
    const TLinkAddr &gate = aGateway ? TLinkAddr::Cast(*aGateway) : KLinkAddr;

    TInt notifytype = EventTypeAdd;

    CIp6Route *rt;
    for (CIp6Route **h = &iRouteList; ; h = &rt->iNext)
        {
        if ((rt = *h) == NULL)
            {
            //
            // 1) Don't create entry with zero lifetime (call was just a Remove
            //    route request in disguise.
            // 2) Don't create entry if the call was "update existing only"
            //
            if (lifetime == 0 || (aFlags & KRouteAdd_UPDATEONLY) != 0)
                return NULL;
            //
            // This a new route, create it here
            //
            rt = new CIp6Route(++mgr.iRouteIndex, mgr, aAddr, aPrefix, *this);
            if (rt == NULL)
                return NULL;
            rt->iState = (CIp6Route::TState)rtype;

            if (TIp46Addr::Cast(aAddr).IsMulticast())
                rt->iIsMulticast = 1;
            rt->iIsProbing = (aFlags & KRouteAdd_PROBINGONLY) != 0;
            break;
            }
        else if (rt->iLength == aPrefix &&
                 rt->ExtendedType() == rtype &&
                 rt->iPrefix.Match(aAddr) >= aPrefix &&
                // Require gateway match only for "true" gateway entries!
                // (otherwise we never find the incomplete hostroutes, or
                // changed link layer addresses, because aGateway does not
                // match in those cases...). Note, specially that ERedirect
                // gateways do not require match on address!
                (rtype != CIp6Route::EGateway || rt->iAddress.Match(gate)))
            {
            // Somewhat dubious, but remove the the route from
            // the current position (and it will be reinserted
            // to the front. This might give priority to the last
            // advertised default route, for example... -- msa
            *h = rt->iNext;
            // Should check the state matches the aFlags?? -- msa

            notifytype = EventTypeModify;
                
            if (lifetime == 0)
                {
                LOG(rt->LogRoute(0));
                //
                // Delete matching route, if lifetime is ZERO
                //
                if (rt->iIsRouter)
                    {
                    rt->iIsRouter = 0;
                    RouterChanged(rt);
                    }
                //
                // If any flows are attached to the route that is being removed,
                // move them all into the holding route with PENDING status.
                //
                // Note: holding is *ALWAYS* non-NULL. The only time holding
                // can be NULL, is when it is being created by InitL(), and in
                // that case GetRoute() *NEVER* gets into this branch! -- msa
                //
                mgr.MoveToHolding(*rt);

                // Send notification about removed route to event manager
                NotifyRouteEvent(EventTypeDelete, rt);

                delete rt;
                return NULL;
                }
            break;
            }
        }
        
    //
    // Attach the route (new or some old) to front of the route list
    //
    rt->iNext = iRouteList;
    iRouteList = rt;

    //
    // Load gateway address, if specified
    //
    if (rt->Update(aFlags, aGateway, aLifetime) || notifytype == EventTypeAdd)
        {
        LOG(rt->LogRoute(lifetime));
        rt->NotifyFlows(EFlow_READY);

        // Send notification about new route to event manager
        NotifyRouteEvent(notifytype, rt, lifetime);
        }

    return rt;
    }

void CIp6Interface::NotifyRouteEvent(TUint aEventType, const CIp6Route *aRoute,
                     const TLifetime /*aLifetime*/) const
/**
Sends notification about event on routing table (add, delete, change) to event manager.
Event manager distributes the event to interested plugins.
This method handles two classes of notifications EClassRoute for events on route information,
and EClassNeighbour for events on changed information in neighbour cache.

@param aEventType   EventTypeAdd, EventTypeDelete, or EventTypeModify.
@param aRoute       Pointer to routing table entry that has changed.
@param aLifetime    Not used. Will be removed.
*/
    {
    CIp6Manager &mgr = Interfacer();
    TUint evclass = aRoute->IsHostRoute() ? EClassNeighbour : EClassRoute;

    // If there is no event manager, or if there are no registered listeners, we can exit
    // the function right away
    if (!mgr.EventManager())
        {
        return;
        }

    if (mgr.EventManager()->IsEmpty(evclass))
        {
        return;
        }

    void *ptr = NULL;   // Either TInetRouteInfo or TInetNeighbourInfo
    TTime stamp;
    stamp.UniversalTime();
    TInetRouteInfo rinfo;
    TInetNeighbourInfo nginfo;
    if (evclass == EClassRoute)
        {
        aRoute->FillRouteInfo(rinfo, Elapsed(stamp));
        ptr = &rinfo;
        }
    else
        {
        aRoute->FillNeighbourInfo(nginfo, Elapsed(stamp));
        ptr = &nginfo;
        }
    
    mgr.EventManager()->Notify(evclass, aEventType, ptr);
    }


// CIp6Manager::AddRouteL
// **********************
// This creates both route and interface (if interface didn't exist before)
void CIp6Manager::AddRouteL
    (const TIp6Addr &aAddr, TInt aPrefix,const TDesC &aName, TUint aFlags,
     const TSockAddr *const aGateway, const TUint32 *const aLifetime)
    {
    CIp6Interface *const iface = GetInterfaceByNameL(aName);
    const CIp6Route *const route = iface->GetRoute(aAddr, aPrefix, aFlags, aGateway, aLifetime);
    if (route)
        {
        if (route->IsMyPrefix())
          // FIXME: Temporary fix to make node-local multicast through loopback interface to work
          iface->AddId(aAddr, aPrefix < 128 ? 128 : 0); // (attempt to make at least one id implicitly defined
                                        // so that a single AddRouteL can be used to create
                                        // a "virtual" or loopback interface.
        ScanHoldings(); // ...routes might have changed by above
        }
    }

// CIp6Manager::CheckRoute
// ***********************
/**
// Find a route by destination address and get a matching source address.
// @return
// @li  KErrNone, if route and source address found
// @li  KErrNotFound, otherwise
*/
TInt CIp6Manager::CheckRoute(const TIp6Addr &aAddr, const TUint32 aScopeId, TIp6Addr &aSrc) const
    {
    const CIp6Route *const route = FindRoute(aAddr, aScopeId, (TUint)(aAddr.Scope()-1));
    if (route && route->iInterface.SelectSource(aSrc, aAddr))
        return KErrNone;    // Route and source address found!
    return KErrNotFound;    // Route or source address not found!
    }

//
// CIp6Interface::RemoveRoute
// **************************
// Remove a specific route
void CIp6Interface::RemoveRoute(CIp6Route *aRoute)
    {
    CIp6Route **h, *rt;
    for (h = &iRouteList; (rt = *h) != NULL; h = &rt->iNext)
        {
        if (rt == aRoute)
            {
            *h = rt->iNext;
            LOG(rt->LogRoute(0));

            if (rt->iIsRouter)
                {
                rt->iIsRouter = 0;
                RouterChanged(rt);  // iIsRouter: "1 -> 0" 
                }
            // Holding route should really never be deleted through this!
            ASSERT(rt != Interfacer().iHoldingRoute);
            // Punt flows into holding for next hop selection (need ScanHolding?)
            if (rt != Interfacer().iHoldingRoute)
                Interfacer().MoveToHolding(*rt);

            // Send notification to event manager
            NotifyRouteEvent(EventTypeDelete, rt);

            delete rt;
            break;
            }
        }
    }

// CIp6Interface::MoveToFront
// **************************
/**
// Move the specific route to the first in the list. The
// route SHOULD be in the list, but if not, nothing happens
//
// (currently only used by select next hop, in rare occasions)
*/
void CIp6Interface::MoveToFront(CIp6Route *aRoute)
    {
    CIp6Route **h, *rt;
    for (h = &iRouteList; (rt = *h) != NULL; h = &rt->iNext)
        {
        if (rt == aRoute)
            {
            *h = rt->iNext;
            rt->iNext = iRouteList;
            iRouteList = rt;
            break;
            }
        }
    }

//
// CIp6Manager::LocalScope
// ***********************
/**
// @returns non-zero scope id, if aAddr is a valid source address
// for packets originating from this node,
// and ZERO otherwise.
*/
TUint32 CIp6Manager::LocalScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const
    {
    const TUint scope = (TUint)(aAddr.Scope()-1);
    if (scope > EScopeType_NET)
        return 0;   // Bad Address

    for (const CIp6Interface *iface = iInterfaceList;iface != NULL; iface = iface->iNext)
        {
        if ((aLock == 0 || iface->iScope[aLockType] == 0 || iface->iScope[aLockType] == aLock) &&
            iface->IsMyAddress(aAddr))
            // iface->iScope[scope] can be ZERO for looback (and similar *wild*
            // interface). Return the default non-zero ID value in such case.
            // (it will match this same network).
            return iface->iScope[scope] ? iface->iScope[scope] : KDefaultNetworkId;
        }
    return 0;
    }

//
// CIp6Manager::RemoteScope
// ************************
// Determine the default scope id for a destination address.
//
// *NOTE* This does not check whether a matching interface actually exists!
//
TUint32 CIp6Manager::RemoteScope(const TIp6Addr &aAddr, const TUint32 aLock, const TScopeType aLockType) const
    {
    const CIp6Route *const rt = FindRoute(aAddr, aLock, aLockType);
    if (rt == NULL)
        return 0;

    // If rt non-null, then the scope index below will be valid (no need to check!)
    const TUint scope_id = rt->iInterface.iScope[aAddr.Scope() - 1];

    // iface can be ZERO for looback (and similar *wild*
    // interface). Return the default non-zero ID value in such case.
    // (it will match this same network).
    return scope_id ? scope_id : KDefaultNetworkId;
    }

// CIp6Manager::IsForMe
// ********************
// (private help utility)
//
// @returns
// @li != 0 (= interface index), if address is for me
// @li == 0, if address is not for me
//
#ifdef WEAK_ES
TUint32 CIp6Manager::IsForMe
    (const TIp6Addr &aAddr, const CIp6Interface *const aSrcIf, const TUint32 aScopeId, const TScopeType aType) const
#else
TUint32 CIp6Manager::IsForMe(const TIp6Addr &aAddr, const CIp6Interface *const aSrcIf) const
#endif
    {
    //
    // Check the original interface first, as this is the
    // most common case.
    //
    if (aSrcIf->IsForMeAddress(aAddr))
        return aSrcIf->iScope[0];

#ifdef WEAK_ES
    //
    // In weak ES model, incoming packet can be accepted, even
    // if destination address is assigned to another interface
    //
    // *NOTE*
    //  the scope check does limit the range! So, this is not
    //  exactly the weak model either.
    //
    // *NOTE*
    //  the scope test is for strict equality! Thus, this WILL not
    //  match addresses on loopback interfaces (unless the
    //  original incoming interface was also a loopback--in
    //  which case it should have already been dealt in above
    //  test for original interface!)
    //
    for (const CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        if (iface->iScope[aType] == aScopeId && iface != aSrcIf && iface->IsForMeAddress(aAddr))
            return iface->iScope[0];
#endif
    return 0;
    }

//
// CIp6Manager::IsForMeAddress
// ***************************
/**
// Returns the interface index, if aAddr selects the current node
// (packets having this address as a destination are intended for me)
//
// @returns
//  @li != 0 (= interface index), if address is for me
//  @li == 0, if address is not for me
*/
TUint32 CIp6Manager::IsForMeAddress(const TIp6Addr &aAddr, const TUint32 aInterfaceIndex) const
    {
    if (aInterfaceIndex == 0)
        {
        for (const CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
            if (iface->IsForMeAddress(aAddr))
                return iface->iScope[0];
        return 0;
        }

    const CIp6Interface *const ifp = FindInterface(aInterfaceIndex);
    if (ifp == NULL)
        return 0;   // Cannot locate interface!
#ifdef WEAK_ES
    const TUint scope = (TUint)(aAddr.Scope() - 1);
    if (scope > EScopeType_NET)
        return 0;   // Invalid address
    return IsForMe(aAddr, ifp, ifp->iScope[scope], (TScopeType)scope);
#else
    return IsForMe(aAddr, ifp);
#endif
    }

// CIp6Manager::IsForMePacket
// **************************
/**
// Complete scopes into the info and check if received packet as described by aInfo is for me.
//
// @return
//  @li -1 if incoming interface or addresses are not valid (drop packet!)
//  @li =0 if packet is not for me
//  @li =1 if packet is for me
*/
TInt CIp6Manager::IsForMePacket(RMBufRecvInfo &aInfo) const
    {
    const CIp6Interface *const ifp = FindInterface(aInfo.iInterfaceIndex);
    if (ifp == 0)
        return -1;  // Cannot locate interface!

    LOG(PktLog(_L("\tIF %u [%S] RECV prot=%d src=%S dst=%S len=%d"), aInfo, ifp->iScope[0], ifp->iName));

    const TIp6Addr &src = TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address();
    const TIp6Addr &dst = TInetAddr::Cast(aInfo.iDstAddr).Ip6Address();

        {
        //
        // Fix source scope id into info block
        //
        const TUint scope = src.Scope() - 1;
        if (scope > EScopeType_NET)
            return -1;  // Invalid source scope
        else if (scope == EScopeType_IF &&
                !TIp46Addr::Cast(src).IsUnspecified() &&
                !ifp->IsMyAddress(src))
            return -1;  // Invalid source address (loopback address from wrong interface!)
        TInetAddr::Cast(aInfo.iSrcAddr).SetScope(ifp->iScope[scope]);
        }
    //
    // Fix destination scope id into info block
    //
    const TUint scope = (TUint)(dst.Scope() - 1);
    if (scope > EScopeType_NET)
        return -1;  // Invalid destination scope
    const TUint dstId = ifp->iScope[scope];
    TInetAddr::Cast(aInfo.iDstAddr).SetScope(dstId);

#ifdef WEAK_ES
    return IsForMe(dst, ifp, dstId, (TScopeType)scope) != 0;
#else
    return IsForMe(dst, ifp) != 0;
#endif
    }

// MagicSetAddress
// ***************
// A static help routine to load TSockAddr address from a prefix
//
static void MagicSetAddress(TSockAddr &aAddr, const TIp6Addr &aSrc, TUint aLength, TSockAddr *aMask = NULL)
    {
    //
    // Make aAddr address family to either IPv4 or IPv6 depending
    // on whether aSrc is V4 mapped address or not.
    //
    if (aSrc.IsV4Mapped())
        {
        ((TInetAddr &)aAddr).SetAddress(
            (aSrc.u.iAddr8[12] << 24) |
            (aSrc.u.iAddr8[13] << 16) |
            (aSrc.u.iAddr8[14] << 8) |
            aSrc.u.iAddr8[15]);
        if (aMask)
            {
            const TInt shift = 32 - (aLength - 96);
            ((TInetAddr *)aMask)->SetAddress(shift > 31 ? 0 : ~0L << shift);
            }
        }
    else
        {
        ((TInetAddr &)aAddr).SetAddress(aSrc);
        if (aMask)
            ((TInetAddr *)aMask)->PrefixMask(aLength);
        }
    }

// MagicGetAddress
// ***************
/**
// Convert Address/Mask pair into Address and prefix, while doing some
// IPv4/IPv6 unifications.
//
// @returns the prefix length (the number of leftmost 1-bits in the mask)
//  @li < 0, (KErrArgument) the address family of addr or mask is not KAfInet6,
//       KAfInet or Unspecified
//  @li = 0, if mask is unspecified (or if leftmost bit is zero)
//  @li = 1..128, the prefix length
//  @li = 129 addr is unspecified family (mask is ignored)
*/
static TInt MagicGetAddress(TIp6Addr &aResult, const TInetAddr &aAddr, const TInetAddr &aMask)
    {
    //
    // Convert aAddr into plain IPv6 address
    //
    if (aAddr.Family() == KAfInet)
        {
        TInetAddr tmp(aAddr);
        tmp.ConvertToV4Mapped();
        aResult = tmp.Ip6Address();
        }
    else if (aAddr.Family() == KAfInet6)
        aResult = aAddr.Ip6Address();
    else if (aAddr.Family() == KAFUnspec)
        return 129;     // Special value to indicate: no address/prefix present
    else
        return KErrArgument;    // Invalid Address
    //
    // Convert aMask into prefix length (ugh!)
    //
    if (aMask.Family() == KAfInet)
        return 96 + MaskLength(aMask.Address());
    else if (aMask.Family() == KAfInet6)
        return MaskLength(aMask.Ip6Address());
    else if (aMask.Family() == KAFUnspec)
        return 0;
    return KErrArgument;        // Invalid aMask!
    }


void CIp6Route::FillRouteInfo(TInetRouteInfo &rinfo, TLifetime aReftime) const
/**
Fill TInetRouteInfo struct based on this route.

@param rinfo    struct to be filled by the route information.
@param aRefTime reference time used in lifetime calculation. It should refer to the time
                the network interface has been up.
*/
    {
    rinfo.iType = 0;
    rinfo.iState = iState;
    rinfo.iMetric = iMetric;
    rinfo.iDstAddr = iPrefix;
    rinfo.iPrefixLen = iLength;
    rinfo.iInterface = iInterface.Index();
    rinfo.iScopeId = iInterface.Scope((TScopeType)(iPrefix.Scope()-1));
    rinfo.iGateway = iAddress.Ip6Address();
    rinfo.iIndex = iIndex;

    if (iLifetime.iPreferred != KLifetimeForever)
      {
        rinfo.iLifetime = (iLifetime.iPreferred > aReftime) ?
                iLifetime.iPreferred - aReftime : 0;
      }
    else
      {
        rinfo.iLifetime = KLifetimeForever;
      }
      
    if (iLifetime.iDeprecated)
        rinfo.iType |= (TUint32)TInetRouteInfo::EDeprecated;
    }


void CIp6Route::FillNeighbourInfo(TInetNeighbourInfo &nginfo, TLifetime aReftime) const
/**
Fill TInetNeighbourInfo struct based on this route.

@param rinfo    struct to be filled by the neighbour information.
@param aRefTime reference time used in lifetime calculation. It should refer to the time
                the network interface has been up.
*/
    {
    nginfo.iIndex = iIndex;
    nginfo.iState = iState;
    nginfo.iMetric = iMetric;
    nginfo.iDstAddr = iPrefix;
    nginfo.iInterface = iInterface.Index();
    nginfo.iScopeId = iInterface.Scope((TScopeType)(iPrefix.Scope()-1));
    nginfo.iHwAddr.Copy(iAddress.Address());

    if (iLifetime.iPreferred != KLifetimeForever)
        {
        nginfo.iLifetime = (iLifetime.iPreferred > aReftime) ?
                iLifetime.iPreferred - aReftime : 0;
        }
    else
        {
        nginfo.iLifetime = KLifetimeForever;
        }
    }
    

void CIp6Interface::SetAddressAndScope(TSockAddr &aAddr, const TSockAddr &aSrc) const
    /**
    * Assign an address (aSrc to aAddr) and supplement it with the
    * scope information from the interface.
    *
    * @retval aAddr The address supplemented with the scope id.
    * @param aSrc   The address to be supplemented.
    *
    * @note aSrc and aAddr can reference the same address.
    */
    {
    // Allow call with aAddr == aSrc. Testing this may not be
    // necessary, but just to be sure, avoid "overlapping"
    // copy operation in such case... -- msa
    if (&aAddr != &aSrc)
        aAddr = aSrc;

    TInetAddr &addr = TInetAddr::Cast(aAddr);
    if (addr.Family() == KAfInet)
        addr.ConvertToV4Mapped();
    if (addr.Family() == KAfInet6)
        {
        //
        // Scope is stored only if the address family is KAfInet or KAfInet6,
        // and if so, the final family is always KAfInet6.
        const TUint scopeType = (TUint)(addr.Ip6Address().Scope() - 1);
        if (scopeType > EScopeType_NET)
            return;
        addr.SetScope(iScope[scopeType]);
        //
        // (minor feature: if scope id would be 0, return IPv4 as KAfInet)
        // [maybe of dubious value, just always return Ipv6 format?]
        if (iScope[scopeType] == 0 && addr.IsV4Mapped())
            addr.ConvertToV4();
        }
    }

// CIp6Manager::InterfaceInfo
// **************************
/**
// Locate the next interface after aIndex and return the
// information and assigned interface index.
//
// @return
//  @li = 0, if no next interface exists
//  @li > 0, interface index, aInfo updated to describe this interface
*/
TUint CIp6Manager::InterfaceInfo(TUint aIndex, TSoInetInterfaceInfo &aInfo) const
    {
    // ..yes, this is silly O(n!) (?) algorithm for scanning the interfaces. Each time
    // this is called, it has find and count all entries that come before the specified
    // aIndex.
    // Also, if between calls, prefixes or id's have been added or removed, the algorithm
    // fails and returns same entries multiple times or skips some that it should have
    // returned. However, this should rarely happen.
    TUint i = 0;
    for (const CIp6Interface *iface = iInterfaceList;iface != NULL; iface = iface->iNext)
        {
        TInt found = 0;
        for (const TIp6AddressInfo *address = &iface->iAddress; ; address = &address->iNext->iInfo)
            {
            TIp6Addr addr(KInet6AddrNone);
            TInt length = 0;
            // "i" represents a virtual index over all *assigned* addresses in all interfaces,
            // with exception that at least one entry is returned for each interface, whether
            // there is assigned addresses or not.
            if (!address->IsSet())
                {
                if (address->iNext)
                    continue;       // Check next address.
                //
                // This is the last address (actually, this is the primary address slot)
                //
                if (found > 0 || ++i <= aIndex)
                    break;          // Done with this interface.
                // No entries returned from this interface,
                // return one dummy with no address
                }
            else if (address->iPrefix == 0)
                {
                //
                // Specified individual alias address
                //
                ++found; 
                if (++i <= aIndex)  // Already processed?
                    {
                    if (address->iNext)
                        continue;   // Look next address.
                    else
                        break;      // Last address, look for next interface
                    }
                // Use address as is...
                addr = address->iId;
                }
            else
                {
                //
                // Address is specified as autoconfigured ID part, find out the next
                // prefix to combine with it.
                //
                // coverity[write_write_order]
                const CIp6Route *route = route = iface->iRouteList;
                for (; route != NULL; route = route->iNext)
                    {
                    if (!route->IsMyPrefix())
                        continue;
                    // The lengths of the prefix and id must be compatible to be
                    // combined.
                    if (route->iLength != address->iPrefix)
                        continue;

                    found++;
                    if (++i > aIndex)
                        break;  // a prefix found!
                    // this prefix was already processed look forward...
                    }
                if (route == NULL)
                    {
                    // No unprocessed prefixes, go to next address, if any
                    if (address->iNext)
                        continue;
                    else
                        {
                        if (found > 0 || ++i <= aIndex)
                            break;          // Done with this interface.
                        // No entries returned from this interface,
                        // return one dummy with no address (or whatever
                        // the current address points to).
                        addr = address->iId;
                        }
                    }
                else
                    {
                    //
                    // Combine prefix and current id part into full address
                    //
                    addr = route->iPrefix;
                    length = route->iLength;
                    MakeFullAddress(addr, length, address->iId.u.iAddr8, sizeof(address->iId.u.iAddr8));
                    }
                }
            //
            // "Interface" located, return information about it
            //
            aInfo.iName = iface->iName;
            if (iface->iNifIf == NULL || !address->IsSet())
                aInfo.iState = EIfDown; // no interface or address not known or was duplicate
            else if (iface->iState == EFlow_READY)
                aInfo.iState = EIfUp;
            else if (iface->iState == EFlow_PENDING)
                aInfo.iState = EIfPending;
            else if (iface->iState == EFlow_HOLD)
                aInfo.iState = EIfBusy;
            else
                aInfo.iState = EIfDown;
            aInfo.iTag = iface->iName;  // dubious, but what else? -- msa
            aInfo.iMtu = iface->iSMtu;
            aInfo.iSpeedMetric = iface->iSpeedMetric;
            aInfo.iFeatures = iface->iFeatures;
            aInfo.iHwAddr = iface->iHwAddr;

            iface->SetAddressAndScope(aInfo.iNameSer1, iface->iNameSer1);
            iface->SetAddressAndScope(aInfo.iNameSer2, iface->iNameSer2);

            TInetAddr none;
            if (addr.IsV4Mapped())
                {
                // IPv4 interface
                none.SetAddress(0);
                //
                // Get some configuration information
                // (ignore errors and assume the 'cfg' is
                // mostly initialized to null addresses in such case)
                //
                TPckgBuf<TSoInetIfConfig> cfg;
                (void)GetIp4Config(iface->iNifIf, cfg);
                const TSoInetIfConfig &cf = cfg();
                ((TInetAddr &)aInfo.iNetMask) = cf.iConfig.iNetMask;
                iface->SetAddressAndScope(aInfo.iBrdAddr, cf.iConfig.iBrdAddr);
                }
            else
                {
                // IPv6 interface
                none.SetAddress(KInet6AddrNone);
                ((TInetAddr &)aInfo.iNetMask) = none;
                ((TInetAddr &)aInfo.iBrdAddr) = none;
                }

            // default gateway
            TInetAddr gw;
            iface->GetDefGateway(addr.IsV4Mapped(), gw);
            iface->SetAddressAndScope(aInfo.iDefGate, gw);

            ((TInetAddr &)aInfo.iAddress) = none;
            MagicSetAddress(aInfo.iAddress, addr, length, &aInfo.iNetMask);
            iface->SetAddressAndScope(aInfo.iAddress, aInfo.iAddress);
            return i;
            }
        }
    return 0;
    }


TInt CIp6Manager::GetInterfaces(TDes8& aOption) const
    {
    TOverlayArray<TInetInterfaceInfo> info(aOption);
    TInt idx = 0, exceed = 0;
    const CIp6Interface *iface = iInterfaceList;

    while (iface)
        {
        // IndexPtr returns NULL, if there is not enough room in descriptor
        if (info.IndexPtr(idx))
            {
            TInetInterfaceInfo *const opt = new (info.IndexPtr(idx)) TInetInterfaceInfo;
            opt->iIndex = iface->iScope[0];
            opt->iName = iface->iName;
            opt->iState = iface->iState;
            opt->iSMtu = iface->iSMtu;
            opt->iRMtu = iface->iRMtu;
            opt->iSpeedMetric = iface->iSpeedMetric;
            opt->iFeatures = iface->iFeatures;
            opt->iHwAddr = iface->iHwAddr;

            idx++;
            }
        else
            {
            exceed++;
            }
        iface = iface->iNext;
        }

    info.SetLength(idx);

    return exceed;
    }


TInt CIp6Manager::GetAddresses(TDes8 &aOption) const
    {
    TOverlayArray<TInetAddressInfo> info(aOption);
    TInt idx = 0, excess = 0, count = 0;
    #ifdef _DEBUG
        TInt ifaceSpecificAddrIdx = 0;
    #endif

    for (const CIp6Interface *iface = iInterfaceList;iface != NULL; iface = iface->iNext)
        {
        #ifdef _DEBUG
            ifaceSpecificAddrIdx = 0;
        #endif
        
        TTime stamp;
        stamp.UniversalTime();
        const TLifetime current_time = iface->Elapsed(stamp);
      
        for (   const TIp6AddressInfo *address = &iface->iAddress;
                address != NULL;
                address = &address->iNext->iInfo)
            {
            TIp6Addr addr(KInet6AddrNone);

        // Own prefixes are appended with an ID part used for the interface and included in
        // address list. They are stored in route table with "myprefix" flag.
        // Route table is maintained separately for each interface, hence the multilevel loop
            const CIp6Route *route = iface->iRouteList;
            for (;;)
                {
                TBool haveprefix = EFalse;

                if (route && !route->IsMyPrefix())
                    {
                    route = route->iNext;
                    continue;
                    }

                // The lengths of the prefix and id must be compatible to be
                // combined.
                if (route && route->iLength != address->iPrefix)
                    {
                    route = route->iNext;
                    continue;
                    }
            
                if (route == NULL || address->iPrefix == 0)
                    {
                    addr = address->iId;
                    }
                else
                    {
                    // Combine prefix and current id part into full address
                    addr = route->iPrefix;
                    haveprefix = ETrue;
                    MakeFullAddress(addr, route->iLength, address->iId.u.iAddr8, sizeof(address->iId.u.iAddr8));
                    }

                if (info.IndexPtr(idx))
                    {
                    #ifdef _DEBUG
                        if( !address->iId.IsUnspecified() )
                            {
                            if( ifaceSpecificAddrIdx == 0 )
                                {
                                ASSERT( address->IsPrimary() );
                                }
                            else
                                {
                                ASSERT( !address->IsPrimary() );
                                }
                            }
                    #endif
                    
                    TInetAddressInfo *const opt = new (info.IndexPtr(idx)) TInetAddressInfo;

                    opt->iInterface = iface->iScope[0];
                    opt->iAddress = addr;
                    opt->iPrefixLen = address->iPrefix;
                    opt->iGenerations = address->iGenerated;
                    opt->iNS = address->iNS;
                    opt->iState = address->AddressState();
                    opt->iType = address->AddressType();
                    opt->iFlags = 0;

            // We distinct the "ID"-entries from "prefix" entries for the user.
            // The actual set of usable addresses is the set of prefix x id combinations.
            // This has more significance when handling address events rather than here,
            // but I'd like to do it for completeness
                    if (!haveprefix)
                        {
                        opt->iFlags |= TInetAddressInfo::EF_Id;
                        }
                    else
                        {
                        opt->iFlags |= TInetAddressInfo::EF_Prefix;
                        }

                    TScopeType st = (TScopeType) (addr.Scope() - 1);
                    opt->iScopeId = iface->Scope(st);

                    TLifetime plt = KLifetimeForever, vlt = KLifetimeForever;

                    // Lifetimes relate to the interface startup time
                    if (haveprefix)
                        {
                        if (route->iLifetime.iPreferred != KLifetimeForever)
                            {
                            plt = (route->iLifetime.iPreferred > current_time) ?
                                        route->iLifetime.iPreferred - current_time : 0;
                            }
                        if (route->iLifetime.iPreferred != KLifetimeForever &&
                                    route->iLifetime.iStored != KLifetimeForever)
                            {
                            vlt = (route->iLifetime.iStored > current_time) ?
                                        route->iLifetime.iStored - current_time : 0;
                            }

                        if (route->iLifetime.iDeprecated)
                            {
                            opt->iFlags |= TInetAddressInfo::EF_Deprecated;
                            }
                        }
                    else
                        {
                        // Address lifetimes are computed in timer units!
                        const TLifetime current_age = ElapsedUnits(address->iCreated, stamp);

                        if (address->iPLT != KLifetimeForever)
                            {
                            plt = (address->iPLT > current_age) ?
                                    address->iPLT - current_age : 0 /* 0 = expired*/;
                            plt /= TIMER_UNIT;
                            }
            
                        if (address->iVLT != KLifetimeForever)
                            {
                            vlt = (address->iVLT > current_age) ?
                                        address->iVLT - current_age : 0 /* 0 = expired*/;
                            vlt /= TIMER_UNIT;
                            }
                    
                        if (plt == 0)
                            {
                            opt->iFlags |= TInetAddressInfo::EF_Deprecated;
                            }
                        }

                    opt->iPrefLifetime = plt;
                    opt->iValidLifetime = vlt;
                    count++;
                    }
                else
                    {
                    excess++;
                    }

                idx++;
                if (!route) break;
                route = route->iNext;

                // Must try one time with route == NULL to output a possible pure Id entry
                }

            if (!address->iNext)
                {
                break;
                }

            #ifdef _DEBUG
                ifaceSpecificAddrIdx++;
            #endif
            }
        }

    info.SetLength(count);

    // Return number of addresses not fit into the option buffer. 0 indicates all of them are
    // listed
    return excess;
    }


TInt CIp6Manager::GetRoutes(TDes8& aOption) const
    {
    TOverlayArray<TInetRouteInfo> info(aOption);
    TInt idx = 0, exceed = 0;

    for (const CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        {
        for (const CIp6Route *route = iface->iRouteList; route != NULL; route = route->iNext)
            {
            // Ignore myprefix entries, these are listed in GetAddresses()
            if (route->IsMyPrefix())
                {
                continue;
                }
        
            if (idx < info.MaxLength())
                {
                TTime stamp;
                stamp.UniversalTime();
                TInetRouteInfo *const rinfo = new (info.IndexPtr(idx)) TInetRouteInfo;
                route->FillRouteInfo(*rinfo, iface->Elapsed(stamp));
                idx++;
                }
            else
                {
                exceed++;
                }
            }
        }

    info.SetLength(idx);

    return exceed;
    }


// CIp6Manager::RouteInfo
// **********************
/**
// Locate the next route after aIndex and return the
// information and assinged route index.
//
// @return
//  @li = 0, if not next route exits
//  @li > 0, route index, aInfo updated to describe this route
*/
TUint CIp6Manager::RouteInfo(TUint aIndex, TSoInetRouteInfo &aInfo) const
    {
    const CIp6Route *rt = NULL;
    const CIp6Interface *ifp;
    for (ifp = iInterfaceList;; ifp = ifp->iNext)
        {
        if (ifp == NULL)
            return 0;       // No more routes;

        for (rt = ifp->iRouteList; rt != NULL; rt = rt->iNext)
            if (rt->iIndex > aIndex &&
                !rt->IsMyPrefix()) // ..additional condition: ingore "prefix" entries in the list!
                goto found_one;
        }
    // At least for now, the route list is *NOT* ordered by
    // the index. Thus, need to find a next higher index by
    // scanning the full list!
found_one:  // ifp != NULL && rt != NULL
    const CIp6Route *next = rt;
    for (;;)
        {
        for (;rt; rt = rt->iNext)
            {
            if (rt->iIndex > aIndex && rt->iIndex < next->iIndex &&
                !rt->IsMyPrefix()) // ..additional condition: ingore "prefix" entries in the list!
                next = rt;
            }
        if ((ifp = ifp->iNext) == NULL)
            break;
        rt = ifp->iRouteList;
        }

    TIp6Addr ifaddr(KInet6AddrNone);

    // The existing values of TRouteState and TRouteType are not
    // quite suitable for the current IPv6 Neighbor Discovery
    // environment. Just do some reasonable effort in mapping
    // the route state into iState & iType.
    //
    // - if route is controlled by ND, set type = ERtIcmpAdd
    //
    aInfo.iType = ERtNormal;
    aInfo.iState = ERtReady;
    switch (next->iState)
        {
        case CIp6Route::EIncomplete:
            aInfo.iType = ERtIcmpAdd;
            aInfo.iState = ERtPending;
            break;
        case CIp6Route::EReachable:
        case CIp6Route::EStale:
        case CIp6Route::EDelay:
        case CIp6Route::EProbe:
            aInfo.iType = ERtIcmpAdd;
            aInfo.iState = ERtReady;
            break;
        default:
            break;
        }
    aInfo.iMetric = next->iMetric;

    (void)next->iInterface.SelectSource(ifaddr, next->iPrefix);

    MagicSetAddress(aInfo.iIfAddr, ifaddr, 0);
    next->iInterface.SetAddressAndScope(aInfo.iIfAddr, aInfo.iIfAddr);
    if (next->iState == CIp6Route::ELoopback)
        aInfo.iGateway.SetFamily(0);
    else
        {
        next->iAddress.GetAddress(aInfo.iGateway);
        next->iInterface.SetAddressAndScope(aInfo.iGateway, aInfo.iGateway);
        }
    MagicSetAddress(aInfo.iDstAddr, next->iPrefix, next->iLength, &aInfo.iNetMask);
    return next->iIndex;
    }

// CIp6Manager::InterfaceQueryOption
// *********************************
// Get information about interface
//
TInt CIp6Manager::InterfaceQueryOption(TUint aName, TSoInetIfQuery &aQuery, const TInt aLength) const
    {
    // aLength holds the availabe space for iZone[] array (backward compatibility kludge for
    // some odd applications that might be using old in_sock.h without the iZone part!
    if (aLength < 0)
        return KErrArgument;    // Option is too short to be anything sensible!

    const CIp6Interface *iface = NULL;

    //
    // the destination address is required in Ip6 format
    // in processing, prefetch...
    //
    TIp46Addr dst(aQuery.iDstAddr);
    //
    // The aName determines how the interface is to be located
    //
    switch (aName)
        {
        case KSoInetIfQueryByDstAddr:
            {
            //
            // *NOTE* This is not satisfactory! It will not give the right answer,
            // if there are hooks that might change the interface based on destination
            // address (or some policy). ...and if policies (IPSEC, QoS) come into
            // picture, we need to have the protocol and port here too!
            // To solve, need almost to create a flow and connect it, but not
            // refresh (no ReadyL() phase, nor interface Activation!)
            //
            const CIp6Route *const route = FindRoute(dst,
                aQuery.iDstAddr.Scope(), (TUint)(aQuery.iDstAddr.Ip6Address().Scope()-1));
            if (route)
                iface = &route->iInterface;
            break;
            }
        case KSoInetIfQueryBySrcAddr:
            iface = FindInterface(aQuery.iSrcAddr);
            break;
        case KSoInetIfQueryByIndex:
            iface = FindInterface(aQuery.iIndex);
            break;
        case KSoInetIfQueryByName:
            iface = FindInterface(aQuery.iName);
            break;
        default:
            return KErrNotSupported;
        }
    if (iface == NULL)
        return KErrNotFound;
    //
    // Fill In the query information
    //
    // Try to select src address based on whatever content of dst has. If
    // fails, then src address will be unspecified.
    if (iface->SelectSource(dst, dst) != NULL)
        aQuery.iSrcAddr.SetAddress(dst);
    else
        aQuery.iSrcAddr.Init(0);
    // Zone ids, copy as much as there is available space in option.
    const TInt N = (aLength > (TInt)sizeof(iface->iScope) ? sizeof(iface->iScope) : aLength) / sizeof(aQuery.iZone[0]);
    for (int i = 0; i < N; ++i)
        aQuery.iZone[i] = iface->iScope[i];
    aQuery.iIndex = iface->iScope[0];       // make sure iIndex is also correct.
    aQuery.iName = iface->iName;            // Interface Name
    aQuery.iIsUp = iface->iNifIf != NULL;   // 1 = if interface has CNifIfBase * attached
    return KErrNone;
    }

// CIp6Manager::InetInterfaceOption
// ********************************
// Modify Inet Interface information (SetOption part)
//
TInt CIp6Manager::InetInterfaceOption(TUint aName, const TSoInet6InterfaceInfo &aInfo)
    {
    #ifdef _LOG
        TBuf<39> addressStr;
        aInfo.iAddress.Output( addressStr );
        TBuf<39> netMaskStr;
        aInfo.iNetMask.Output( netMaskStr );
        TBuf<39> brdAddrStr;
        aInfo.iBrdAddr.Output( brdAddrStr );
        TBuf<39> defGateStr;
        aInfo.iDefGate.Output( defGateStr );
        TBuf<39> nameSer1Str;
        aInfo.iNameSer1.Output( nameSer1Str );
        TBuf<39> nameSer2Str;
        aInfo.iNameSer2.Output( nameSer2Str );

        LOG( Log::Printf( _L( "CIp6Interface::InetInterfaceOption iName(%S), iState(%d), iMtu(%d), iSpeedMetric(%d), iFeatures(%u), iAddress(%S), iNetMask(%S), iBrdAddr(%S), iDefGate(%S), iNameSer1(%S), iNameSer2(%S), iDelete(%u), iAlias(%u), iDoPrefix(%u), iDoId(%u), iDoState(%u), iDoAnycast(%u), iDoProxy(%u)" ),
            &aInfo.iName,
            aInfo.iState,
            aInfo.iMtu,
            aInfo.iSpeedMetric,
            aInfo.iFeatures,
            &addressStr,
            &netMaskStr,
            &brdAddrStr,
            &defGateStr,
            &nameSer1Str,
            &nameSer2Str,
            aInfo.iDelete,
            aInfo.iAlias,
            aInfo.iDoPrefix,
            aInfo.iDoId,
            aInfo.iDoAnycast,
            aInfo.iDoProxy
        ) );
    #endif

    // Locate the interface
    CIp6Interface *iface = FindInterface(aInfo.iName);
    if (iface)
        {
        switch (aName)
            {
            case KSoInetDeleteInterface:
                RemoveInterface(iface);
                return KErrNone;
            case KSoInetConfigInterface:
            case KSoInetChangeInterface:
                break;
            case KSoInetResetInterface:
                // *THIS IMPLEMENATION NEEDS TO BE LOOKED INTO*  -- msa
                {
                LOG(Log::Printf(_L("CIp6Manager::InetInterfaceOption(KSoInetResetInterface, %S)"), &iface->iName));
                if (iface->iState >= EFlow_READY)
                    iface->iState = EFlow_HOLD;

                for(CIp6Route *rt=iface->iRouteList; rt != NULL; rt = rt->iNext)
                    MoveToHolding(*rt);
                iface->Reset(1);        // Reset to initial state, but keep binding to NIF
                }
                return KErrNone;
            case KSoInetStartInterface:
                // *THIS IMPLEMENATION NEEDS TO BE LOOKED INTO*  -- msa
                LOG(Log::Printf(_L("CIp6Manager::InetInterfaceOption(KSoInetStartInterface, %S)"), &iface->iName));
                StartSending(iface->iNifIf);
                return KErrNone;
            case KSoInetCreateIPv4LLOnInterface:
                {
                LOG(Log::Printf(_L("CIp6Manager::InetInterfaceOption(KSoInetCreateIPv4LLOnInterface, %S)"), &iface->iName));
                
                TInt err = KErrNone;
                const TInt flag = iface->HaveIp4LinkLocal();

                if( flag == CIp6Interface::EV4LLConfigDaemonControlled )
                    {
                    // Check for any state change.
                    if (aInfo.iDoState)
                        {
                        if (aInfo.iState == EIfDown)
                            iface->iState = KErrNotReady;
                        else if (aInfo.iState == EIfUp)
                            iface->iState = EFlow_READY;
                        else
                            err = KErrArgument; // no others supported now!
                        }

                    if( err == KErrNone )
                        {
                        // Try to create a link local.
                        TInt retVal = iface->ConfigureLinkLocal( 0 );
                        
                        // Check for any errors.
                        if( retVal == 1 )
                            {
                            // Link local was created.
                            err = KErrNone;

                            // Address configuration has been changed, the process completes
                            // through the Timeout function, activate it.
                            TTime stamp;
                            stamp.UniversalTime();
                            iface->Timeout(stamp);
                            }
                        else
                            {
                            if( iface->HasIpv4LinkLocalAddr() )
                                {
                                // Link local already exists.
                                err = KErrNone;
                                }
                            else if( !iface->iIsIPv4 )
                                {
                                // This interface option is only available when using IPv4.
                                err = KErrNotSupported;
                                }
                            else
                                {
                                // Link local could not be created for unknown reasons.
                                err = KErrUnknown;
                                }
                            }
                        }
                    }
                else
                    {
                    // This interface option is only available when EV4LLConfigDaemonControlled is specified.
                    err = KErrNotSupported;
                    }
                
                return err;
                }
            default:
                return KErrNotSupported;
            }
        }
    else if (aName == STATIC_CAST(TUint,KSoInetConfigInterface))
        {
        TRAPD(err, iface = GetInterfaceByNameL(aInfo.iName));
        if (iface == NULL)
            return err;
        // For a newly created interface, let the address decide the "mode"
        if (aInfo.iAddress.Family() == KAfInet6)
            iface->iNifUser = iNifUser[E_IPv6];
        }
    else
        return KErrNotFound;

    // Execute the option on interface

    TIp6Addr addr;
    TInt prefix = MagicGetAddress(addr, aInfo.iAddress, aInfo.iNetMask);
    if (prefix < 0)
        return prefix;  // Bad address information
    if (aInfo.iMtu > 0)
        iface->iSMtu = iface->iRMtu = iface->iPMtu = aInfo.iMtu;
    if (aInfo.iSpeedMetric > 0)
        iface->iSpeedMetric = aInfo.iSpeedMetric;
    if (aInfo.iDoState)
        {
        if (aInfo.iState == EIfDown)
            iface->iState = KErrNotReady;
        else if (aInfo.iState == EIfUp)
            iface->iState = EFlow_READY;
        else
            return KErrArgument;    // no others supported now!
        }
    iface->UpdateNameServers(aInfo.iNameSer1, aInfo.iNameSer2, 1); // Note, override mode!

    //
    // Do some iDefGate processing (if specified)
    //
    if (!aInfo.iDefGate.IsUnspecified())
        {
        // Family is either KAfInet or KAfInet6 now
        TInetAddr defgate(aInfo.iDefGate);
        if (defgate.Family() == KAfInet)
            defgate.ConvertToV4Mapped();

        static const TLifetime zero = 0;
        const TLifetime *const lifetime = aInfo.iDelete ? &zero : NULL;
        const TInt pb = defgate.IsV4Mapped() ? 96 : 0;
        const TIp6Addr &gw = defgate.Ip6Address();
        if (gw.IsEqual(addr))
            (void)iface->GetRoute(gw, pb, KRouteAdd_ONLINK, NULL, lifetime);
        else
            {
            (void)iface->GetRoute(gw, 128, KRouteAdd_ISROUTER, NULL, lifetime); 
            (void)iface->GetRoute(gw, pb, KRouteAdd_GATEWAY, &defgate, lifetime);
            }
        }

    if (prefix > 128)
        return KErrNone;    // No address info, ignore rest

    //
    // prefix = 0 => request to configure a single address
    // prefix > 0 => request to configure prefix and/or id part
    //
    if (aInfo.iDoId)
        {
        if (TIp46Addr::Cast(addr).IsMulticast())
            {
#if 0
            return KErrArgument;        // Cannot configure multicast address as own address
#else
            //
            // Experimental hack -- treat multicast address configuration as
            // multicast join/leave group event
            //
            return iface->UpdateMulticast(addr,  aInfo.iDelete ? 0 : KLifetimeForever);
#endif
            }

        // *NOTE* Some obscure stuff: You cannot configure a plain
        // id, because this 'addr' is stored as is into the id list
        // and the full address is used for DAD processing.
        if (aInfo.iDelete)
            {
            const TInt err = iface->RemId(iface->GetId(addr));
            if (err != KErrNone)
                return err;
            }
        else if (aInfo.iDoAnycast || aInfo.iDoProxy)
            {
            //
            // Special addresses
            //
            const TInt address_type =
                aInfo.iDoAnycast ? TIp6AddressInfo::EAnycast :
                aInfo.iDoProxy ? TIp6AddressInfo::EProxy :
                TIp6AddressInfo::ENormal;
            (void)iface->AddId(addr, 0, address_type);
            prefix = 0; // "disarm" iDoPrefix processing below (not relevant for proxy/anycast)
            }
        else if (addr.IsV4Mapped())
            {
            // Configuring IPv4 interface with (prefix > 96) or without (prefix == 0)
            // netmask. The "id" processing is adding the recognition of the network broadcast
            // address. For IPv4, "alias" is always assumed implicitly
            iface->ConfigureAddress(addr, prefix, ETrue);
            prefix = 0; // "disarm" iDoPrefix processing below (not relevant for IPv4!)
            }
        else if (aInfo.iAlias || prefix == 0)
            {
            // when prefix==0, the request does not specify mask. Treat
            // this as a request to add a special 128bit address as id
            // (= configuring single address for interface). Do it
            // always as "alias".
            (void)iface->AddId(addr, prefix);
            }
        else
            {
            // Change/Define primary id part
            iface->SetId(iface->iAddress, addr, prefix, TIp6AddressInfo::ENormal);
            }
        //
        // Update lifetime and DAD events
        //
        TTime stamp;
        stamp.UniversalTime();
        iface->Timeout(stamp);
        }
    // If prefix=0, AddId already does SetPrefix...
    if (aInfo.iDoPrefix && prefix > 0)
        {
        // ...works as if RA prefix option with A=1
        const TLifetime lifetime = aInfo.iDelete ? 0 : KLifetimeForever;
        iface->SetPrefix(addr, prefix, 1, lifetime);
        // ...works as if RA prefix option with L=1
        iface->GetRoute(addr, prefix, KRouteAdd_ONLINK, NULL, &lifetime);
        }
    return KErrNone;
    }

//
// CIpManager::InterfaceOption
// ***************************
/**
// This method implements *BOTH* SetOption and GetOption when level KSOLInterface
// The call is translated to a Control(). Somewhat dubious, but thats the way it
// was before...
*/
TInt CIp6Manager::InterfaceOption(TUint aLevel, TUint aName, TDes8 &aOption) const
    {
    // Note: Checking against MaxLength only to be sure that the iName field
    // of TSoIfInfo is accessible. It just blindly assumed that the caller
    // has initialized the content properly (even if Length() is not necessarily
    // correctly set). [To be compatible with the old implementation].
    if ((TUint)aOption.MaxLength() < sizeof(TSoIfInfo))
        return KErrArgument;    // Both Get/Set need the interface name!
    const TSoIfInfo &opt = *(TSoIfInfo *)aOption.Ptr();

    // Locate Interface mathing the specified name
    const CIp6Interface *const iface = FindInterface(opt.iName);
    if (iface)
        return iface->iNifIf ? iface->iNifIf->Control(aLevel, aName, aOption) : KErrNotReady;
    // No such interface
    return KErrBadDriver;
    }



//
// CIp6Manager::MulticastOption
// ****************************
// Process Multicast Options Join and Leave Group
//
TInt CIp6Manager::MulticastOption(TUint aName, const TIp6Mreq &aRequest)
    {
    if (!TIp46Addr::Cast(aRequest.iAddr).IsMulticast())
        return KErrArgument;    // This must be a valid multicast address!
    CIp6Route *route;
    //
    // Locate interface to be used
    //
    CIp6Interface *iface;
    if (aRequest.iInterface == 0)
        {
        route = FindRoute(aRequest.iAddr, 0, 0);
        if (route == NULL)
            return KErrNotFound;
        iface = &route->iInterface;
        }
    else
        {
        // Assuming the request.iInterface is always a true interface index.
        // (and not a scope id depending on the scope of the multicast address)
        iface = FindInterface(aRequest.iInterface);
        if (iface == NULL || (iface->FindRoute(aRequest.iAddr, NULL)) == NULL)
            {
            // Should return something specific: "no multicast enabled interface" or something...
            // or, should this cause dialup popup? -- msa
            return KErrNotFound;
            }
        }
    // Note: the caller must have already verified that aName is either KSoIp6JoinGroup or KSoIp6LeaveGroup
    return iface->UpdateMulticast(aRequest.iAddr, aName == KSoIp6JoinGroup ? KLifetimeForever : 0);
    }

//
// CIp6Manager::GetOption
// **********************
TInt CIp6Manager::GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const
    {
    return GetOption(aLevel, aName, aOption, *(CIp6Manager *)this);
    }

TInt CIp6Manager::GetOption(TUint aLevel, TUint aName, TDes8 &aOption, MProvdSecurityChecker &aChecker) const
    {
    if (aLevel == KSOLInterface)
        {
        const TInt ret = aChecker.CheckPolicy(KPolicyNetworkControl, 0);
        if (ret != KErrNone)
            return ret;
        return InterfaceOption(aLevel, aName, aOption);
        }

    else if (aLevel == KSolInetIfQuery)
        {
        // Returns an array of TInetInterfaceInfo objects
        if (aName == KSoInetInterfaceInfo)
            {
            return GetInterfaces(aOption);
            }

        // Returns an array of TInetAddressInfo objects
        else if (aName == KSoInetAddressInfo)
            {
            return GetAddresses(aOption);
            }

        // Returns an array of TInetRouteInfo objects
        else if (aName == KSoInetRouteInfo)
            {
            return GetRoutes(aOption);
            }

        // Other options return TSoInetIfQuery object
        return InterfaceQueryOption(aName, *(TSoInetIfQuery *)aOption.Ptr(), aOption.Length() - _FOFF(TSoInetIfQuery, iZone[0]));
        }
    return KErrNotSupported;    // No get options supported for now (here)
    }

// CIp6Manager::SetOption
// **********************
TInt CIp6Manager::SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption)
    {
    return SetOption(aLevel, aName, aOption, *this);
    }

TInt CIp6Manager::SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption, MProvdSecurityChecker &aChecker)
    {
    const TUint8 &ref = *aOption.Ptr(); // Prefetch for use in below.

    if (aLevel == KSolInetIp)
        {
        if (aName == KSoIp6JoinGroup || aName == KSoIp6LeaveGroup)
            {
            if (aOption.Length() != sizeof(TIp6Mreq))
                return KErrArgument;
            return MulticastOption(aName, (TIp6Mreq &)ref);
            }
        return KErrNotSupported;
        }

    if (aLevel == KSolInetIfCtrl)
        {
        const TInt ret = aChecker.CheckPolicy(KPolicyNetworkControl, 0);
        if (ret != KErrNone)
            return ret;
        if (aName == KSoIpv4LinkLocal &&
                aOption.Length() == sizeof(TSoInetIpv4LinkLocalInfo))
            return SetIpv4LinkLocalOption((TSoInetIpv4LinkLocalInfo &)ref);
        
        if (aOption.Length() != sizeof(TSoInet6InterfaceInfo))
            return KErrArgument;
        return InetInterfaceOption(aName, (TSoInet6InterfaceInfo &)ref);
        }
    else if (aLevel == KSOLInterface)
        {
        const TInt ret = aChecker.CheckPolicy(KPolicyNetworkControl, 0);
        if (ret != KErrNone)
            return ret;
        // InterfaceOption needs modifiable descriptor, make it so!
        TPtr8 tmp((TUint8 *)&ref, aOption.Length());
        return InterfaceOption(aLevel, aName, tmp);
        }
    else if (aLevel == KSolInetIfQuery)
        {
        const TInt ret = aChecker.CheckPolicy(KPolicyNetworkControl, 0);
        if (ret != KErrNone)
            return ret;
        if (aOption.Length() != sizeof(TSoInetIfQuery))
            return KErrArgument;
        const TSoInetIfQuery &info = (TSoInetIfQuery &)ref;

        // Search interface by index and hand some special options.
        // [These are not really very natural for KSolInetIfQuery
        // and probably should be placed under some other level]
        CIp6Interface *const iface = FindInterface(info.iIndex);
        if (iface == NULL)
            return KErrNotFound;
        switch (aName)
            {
            // Copy the zone id vector from option to the interface.
            case KSoInetIfQuerySetScope:
                {
                TUint loopCount = sizeof(iface->iScope) / sizeof(iface->iScope[0]);
                for (TUint i = 1; i < loopCount; ++i)
                    iface->iScope[i] = info.iZone[i];
                iface->NotifyInterfaceEvent(EventTypeModify);
                return KErrNone;
                }
            // Clear "IS ROUTER" flag on interface (IPv6 Neighbour Advertisement IS_ROUTER flag)
            case KSoInetIfQuerySetHost:
                iface->iIsRouter = 0;
                return KErrNone;
            // Set "IS ROUTER" flag on interface (IPv6 Neighbour Advertisement IS_ROUTER flag)
            case KSoInetIfQuerySetRouter:
                iface->iIsRouter = 1;
                return KErrNone;
            default:
                return KErrNotSupported;
            }
        }
    else if (aLevel == KSolInetRtCtrl)
        {
        const TInt ret = aChecker.CheckPolicy(KPolicyNetworkControl, 0);
        if (ret != KErrNone)
            return ret;
        if (aOption.Length() != sizeof(TSoInetRouteInfo))
            return KErrArgument;

        const TSoInetRouteInfo& opt = (TSoInetRouteInfo &)ref;
        TInetAddr gateway(opt.iGateway);
        if (gateway.Family() == KAfInet)
            gateway.ConvertToV4Mapped();

        // Because the iType of the TSoInetRouteInfo does not match very well
        // with the internal route types, some "guesswork" is required to decide
        // which type of route operated:
        //
        // The KSoInetRtCtrl supports the following:
        // 1) Neighbor Cache entry, if iType == ERtIcmpAdd (prefix should be 128 bits!),
        // 2) Gateway route, if iGateway was an IPv4 or IPv6 address (0 <= prefix <= 128)
        // 3) Otherwise, Onlink route (0 <= prefix <= 128)
        //
        // For neighbor cache entry, the gateway address is the link layer address.
        //
        TUint rtype =
            opt.iType == ERtIcmpAdd ? KRouteAdd_NEIGHBOR :
            gateway.Family() == KAfInet6 ? KRouteAdd_GATEWAY : KRouteAdd_ONLINK;

        //
        // Is interface specified by iIfAddr or iGateway?
        //
        CIp6Interface *iface = NULL;
        if (opt.iIfAddr.Family() != KAFUnspec)
            {
            //
            // Select interface by iIfAddr (IPv4 or IPv6 address)
            //
            iface = FindInterface(opt.iIfAddr);
            }
        else if (rtype == KRouteAdd_GATEWAY)
            {
            // Select interface by iGateway address
            const CIp6Route *const rt = FindRoute(gateway.Ip6Address(), gateway.Scope(),
                (TUint)(gateway.Ip6Address().Scope()-1));
            if (rt != NULL)
                iface = &rt->iInterface;
            }
        // Must have interface...
        if (iface == NULL)
            return KErrNotFound;

        //
        // Convert iDstAddr/iNetMask into prefix
        //
        TIp6Addr prefix_addr;
        const TInt prefix_len = MagicGetAddress(prefix_addr, opt.iDstAddr, opt.iNetMask);
        if (prefix_len < 0 || prefix_len > 128)
            return KErrArgument;        // Add ROUTE makes no sense without a valid prefix info!

        switch (aName)
            {
            case KSoInetDeleteRoute:
                rtype |= KRouteAdd_UPDATEONLY;
                break;
            case KSoInetChangeRoute:
                rtype |= KRouteAdd_UPDATEONLY;
                /* FALLTRHOUGH */
            case KSoInetAddRoute:
                // If this is adding/changing a gateway route, add also IS ROUTER status for the gateway!
                if ((rtype & KRouteAdd_TYPEMASK) == KRouteAdd_GATEWAY)
                    {
                    TIp6Addr src;
                    CIp6Route *const n = iface->GetRoute(gateway.Ip6Address(), 128, KRouteAdd_ISROUTER);
                    if (n && n->iState == CIp6Route::EIncomplete)
                        if (iface->SelectSource(src,n->iPrefix)!=NULL) 
                            n->StartND(src); 
                        else 
                            n->StartND(n->iInterface.iAddress.iId);
                    }
                break;
            default:
                return KErrNotSupported;
            }

        CIp6Route *const route = iface->GetRoute(prefix_addr, prefix_len, rtype, &gateway);
        if (route == NULL)
            return (rtype & KRouteAdd_UPDATEONLY) != 0 ? KErrNotFound : KErrNoMemory;

        if (aName == STATIC_CAST(TUint,KSoInetDeleteRoute))
            iface->RemoveRoute(route);
        else
            route->iMetric = opt.iMetric;

        ScanHoldings();
        return KErrNone;
        }
    return KErrNotSupported;
    }


TInt CIp6Manager::SetIpv4LinkLocalOption(const TSoInetIpv4LinkLocalInfo &aOption)
    {
    CIp6Interface *iface = FindInterface(aOption.iInterface);
    if (iface == NULL)
        return KErrNotFound;
        
    return iface->SetIpv4LinkLocal(aOption.iFlag);
    }
    
    
TInt CIp6Interface::SetIpv4LinkLocal(TUint aFlag)
    /**
    * Sets the IPv4 link-local flag for this interface.
    * Possible parameter values are enumerated in EV4LLEnums.
    *
    * If the LL flag is set to disabled when link local addresses are used,
    * the link-local prefix/id entries are set to deprecated state (without timeout)
    * and they are left into the routing table.
    */
    {
    // Unlike the ini parameter, the socket option has only two valid choices:
    // unconditional disable and unconditional enable
    if (aFlag > 1)
        return KErrArgument;

    iIpv4Linklocal = aFlag;

    for (;;)    // FAKE LOOP, JUST FOR BREAK EXITS!
        {
        if (aFlag == EV4LLAlways)
            {
            // ConfigureLinkLocal() does not get confused even if it was called already
            // earlier, so this should be safe operation
            if (ConfigureLinkLocal(0) == 0)
                break;  // -- no change!
            }
        else if (aFlag == EV4LLDisabled)
            {
            // The code from this point on is only for deprecating IPv4 LL addresses when
            // the user requests to disable LL when they were earlier enabled

            TIp6AddressInfo *const address = FindInternalIpv4LinkLocalAddr();
            if (address == NULL)
                break;  // -- no change (no LL address present)

            // Found IPv4 Link-local address (Id). Mark it as deprecated.
            // The corresponding prefix will also get deprecated in the Timeout()
            // function
            address->iPLT = 0;
            // Further processing for deprecation takes place in timeout handler
            }
        else
            break;  // -- no change
        
        // Address configuration has been changed, the process completes
        // through the Timeout function, activate it.
        TTime stamp;
        stamp.UniversalTime();
        Timeout(stamp);
        break;  // ** ALWAYS EXIT THE FAKE LOOP
        }
    return KErrNone;
    }


// CIp6Interface::FindIpv4LinkLocalAddr
// ****************************************
/**
// Find the one and only internally generated IPv4 link-local, if present.
//
// @return the TIp6AddressInfo, if such address exists; and NULL otherwise.
*/
const TIp6AddressInfo* CIp6Interface::FindIpv4LinkLocalAddr() const
    {
    for (const TIp6AddressInfo *address = &iAddress;;)
        {
        if( address->IsSet() && address->iIpv4LinkLocal )
            {
            // Caller does not get ownership of object.
            return (TIp6AddressInfo *)address;
            }

        if (address->iNext == NULL)
            break;
        address = &address->iNext->iInfo;
        }
    return NULL;
    }


//
// CIp6Interface::NotifyFlows
// **************************
// The interface has changed the state, notify the flows about this
//
void CIp6Interface::NotifyFlows(TInt aState, TBool aForce) const
    {
    // Interface state has changed. Notify all affected flows
    // which have requested to be notified.
    // Cannot just change the flow into ready, because the
    // hooks may have something to add also.
    // (Call RefreshFlows()? Ugh!!.. --msa)
    TFlowNotifyList list;
    for (CIp6Route *rt = iRouteList; rt; rt = rt->iNext)
        for (CIp6Flow *f = rt->iFlowList; f; f = f->iNext)
            {
            // Temp. kludge -- msa
            if (f->iPathMtu == 0 || (TInt)f->iPathMtu > iPMtu)
                f->iPathMtu = iPMtu;    // Minor kludge, fixes the
                                            // problem when interface
                                            // Mtu changes the Path Mtu
                                            // -- msa

            // iIgnoreFlowContorol is set for flows that should
            // not enter automaticly READY/HOLD state by a signal
            // from the interface. A separate (external) module
            // makes the decision for such flows...
            //
            if (aState < 0 || !f->iIgnoreFlowControl || aForce)
                f->Notify(list, aState);
            }
    list.Deliver(aState);
    }


//
// CIp6Interface::NotifyFlowsPmtu
// ******************************
// Special method to define/change the Path MTU of the 
// currently attached flows.
//
// The provider is not notified... should it? -- msa
//
void CIp6Interface::NotifyFlowsPmtu(const TUint aPmtu) const
    {
    for (const CIp6Route *rt = iRouteList; rt; rt = rt->iNext)
        for (CIp6Flow *f = rt->iFlowList; f; f = f->iNext)
            if (f->iPathMtu == 0 || f->iPathMtu > aPmtu)
                f->iPathMtu = aPmtu;
    }


// CIp6Interface::SetChanged
// *************************
// Set iChanged for all flows on this interface
//
TInt CIp6Interface::SetChanged(const TInt aScope) const
    {
    if (aScope > 0)
        return Interfacer().SetChanged();
    TInt count = 0;
    for (CIp6Route *rt = iRouteList; rt; rt = rt->iNext)
        count += rt->SetChanged(0);
    return count;
    }

//
// CIp6Manager::SetChanged
// ***********************
// Set iChanged for all flows
//
TInt CIp6Manager::SetChanged() const
    {
    TInt count = 0;
    for (CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        count += iface->SetChanged(0);
    return count;
    }

//
// CIp6Manager::FindInterface
// **************************
/**
// Based on CNifIfBase pointer, locate the internal
// CIp6Interface description that is connected to the
// this interface. Returns NULL, if none found.
//
// WARNING: (conserns both FindInterface methods)
//      The search key is a pointer to memory block representing some
//      structure and the point of these lookups is to guarantee that
//      this value is still valid. However, there is a potential
//      problem if the object being searched gets released and another
//      object of the same type gets created using the same memory
//      address. In such case FindInterface may return wrong interface!
//      -- msa [needs to be checked whether this is a problem!]
*/
CIp6Interface *CIp6Manager::FindInterface(const CNifIfBase *aInterface) const
    {
    if (aInterface == NULL)
        return NULL;    // Don't search for NULL!
    for (CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        if (iface->iNifIf == aInterface)
            return iface;
    return NULL;
    }
//
// CIp6Manager::FindInterface
/**
//      This is mainly to safely convert aId parameter from MNifIfuser
//      upcall into valid CIp6Interface pointer. (Just makes sure that
//      the instance referred by aId really exists.
*/
CIp6Interface *CIp6Manager::FindInterface(const TAny *aId) const
    {
    for (CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        if (iface == aId)
            return iface;
    return NULL;
    }
//
// CIp6Manager::FindInterface
//     Locate interface by address
CIp6Interface *CIp6Manager::FindInterface(const TInetAddr &aAddr) const
    {
    const TIp46Addr addr(aAddr);
    const TUint32 scope_type = addr.Scope() - 1;
    if (scope_type > EScopeType_NET)
        return NULL;
    const TUint32 scope_id = aAddr.Scope();
    for (CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        {
        const TUint32 if_scope = iface->iScope[scope_type];
        if ((if_scope == 0 || scope_id == 0 || if_scope == scope_id) && iface->IsMyAddress(addr))
            return iface;
        }
    return NULL;
    }

//
// CIp6Manager::FindInterface
//     Locate interface by Interface index
CIp6Interface *CIp6Manager::FindInterface(const TUint32 aIndex) const
    {
    return FindInterface(aIndex, EScopeType_IF);
    }

// CIp6Manager::FindInterface
//     Locate Interface by a specific scope id.
//     Note, that there can be multiple interfaces which match
//     the condition. Only the first located is returned.
CIp6Interface *CIp6Manager::FindInterface(const TUint32 aIndex, const TScopeType aLevel) const
    {
    for (CIp6Interface *iface = iInterfaceList; iface != NULL; iface = iface->iNext)
        if (iface->iScope[aLevel] == aIndex)
            return iface;
    return NULL;
    }

// CIp6Manager::FindInterface
//     Locate Interface mathing the specified name
CIp6Interface *CIp6Manager::FindInterface(const TDesC &aName) const
    {
    for (CIp6Interface *iface = iInterfaceList; iface; iface = iface->iNext)
        if (aName.Compare(iface->iName) == 0)
            return iface;
    return NULL;
    }
    


// CIp6Manager::Interface
// **********************
//
const MInterface* CIp6Manager::Interface(const CNifIfBase *const aIf) const
    {
    return FindInterface(aIf);
    }

const MInterface* CIp6Manager::Interface(const TDesC &aName) const
    {
    return FindInterface(aName);
    }

const MInterface* CIp6Manager::Interface(const TUint32 aInterfaceIndex) const
    {
    return FindInterface(aInterfaceIndex);
    }

//
// CIp6Manager::StartSending
// *************************
//
TInt CIp6Manager::StartSending(CNifIfBase *aIface)
    {
    if (aIface)
        {
        CIp6Interface* iface = FindInterface((CNifIfBase*)aIface);
        if (iface)
            {
            LOG(Log::Printf(_L("\tIF %u [%S] StartSending"), iface->iScope[0], &iface->iName));
            const TInt transition = iface->StartSending();
            //
            // StartSending from an interface implies that it is
            // in READY state (can accept Send()). However this
            // does not directly mean that flows can be opened
            // yet. Update may have decided that information is
            // still missing (prefixes, return PENDING) or detected
            // some configuration error (return < 0). Other than
            // pending, should be notified to flows.
            //
            LOG(Log::Printf(_L("\tIF %u [%S] StartSending, transition=%d"), iface->iScope[0], &iface->iName, transition));
            if (transition != KIfaceTransition_NONE)
                {
                iface->NotifyFlows(transition > 0 ? EFlow_READY : transition);
                // if transition is UP, try waking up the holding flows
                // (try to re-attach pending flows, in case this new interface fits
                // some of them...)
                if (transition == KIfaceTransition_UP)
                    ScanHoldings();
                }
            return transition;
            }
        else
            return KIfaceTransition_DOWN;
        }
    else
        return KIfaceTransition_NONE;
    }
//
// CIp6Manager::Error
// ******************
//
// Comment/msa: It is somewhat unclear what it means when an interface
// calls Error() of the network layer. What is the expected effect?
//
// a) just report it to flows, but leave interface into OK state?
// b) report to flows, put interface into error state until
//    either StartSending() clears it, or interface goes down?
//
// The current implementaion does (b).
//
TInt CIp6Manager::Error(TInt aError, CNifIfBase *aIface)
    {
    if (aIface)
        {
        CIp6Interface* iface = FindInterface((CNifIfBase*)aIface);
        if (iface)
            {
            LOG(Log::Printf(_L("CIp6Manager::Error(%d, %S)"), aError, &iface->iName));

            if (iface->iState >= EFlow_READY)
                iface->iState = aError;
            //
            // Notify flows that want interface errors...
            //
            if (aError < 0)
                {
                iface->NotifyFlows(aError);
                //
                // For the remaining flows, interface going down is not a fatal
                // error. Move all attached flow to the holding route (pending state).
                //
                // Note: if there is no holding route, flows will be terminated
                // by the Reset().
                for (CIp6Route *rt = iface->iRouteList; rt != NULL; rt = rt->iNext)
                    MoveToHolding(*rt);
                iface->Reset(1);    // Reset to initial state, but keep binding to NIF
                }
            }
        //
        // This is somewhat kludgy. Allow Error to used by the interface to set the
        // interface state into non-error state (like pending). If such call happens,
        // return with NONE transition to prevent unnecessary further processing.
        return aError < 0 ? aError : KIfaceTransition_NONE;
        }
    else
        return KIfaceTransition_NONE;
    }


//
// CIp6Manager::IcmpHandler
// ************************
/**
// Gets a peek at all received non-error ICMP's
//
// @return
// @li < 0, if packet has been released (packet will not
//          go to the upper layer after this),
// @li  = 0,    the usual return, packet looked and it can be
//          passed to the upper layers
// @li  > 0,    *NOT USED NOW*, Treat as = 0 as default
*/
TInt CIp6Manager::IcmpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
    {
    if (aInfo.iProtocol != STATIC_CAST(TInt,KProtocolInet6Icmp))
        return 0;       // For now, only ICMP6 is interesting!
    //
    // For validity checking, the hoplimit/TTL is required.. get it..
    //
    const TIpHeader *const ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
    if (!ip)
        return 0;       // should probably drop the packet
    if (255 != ((aInfo.iVersion == 4) ? ip->ip4.Ttl() : ip->ip6.HopLimit()))
        return 0;       // All ND ICMP's must have hoplimit == 255!

    TInet6Packet<TIcmpNdHeader> nd(aPacket, aInfo.iOffset);

    if (nd.iHdr == NULL ||
        nd.iHdr->iIcmp.Code() != 0)
        return 0;       // Not for me!

    CIp6Interface *const srcif = FindInterface(aInfo.iInterfaceIndex);
    if (!srcif)
        return 0;
    return srcif->IcmpHandler(aPacket, aInfo, nd);
    }


//
// CIp6Interface::IcmpHandler
// **************************
/**
// The CIp6Manager::IcmpHandler determined that the basic ICMP is
// valid (for ND), and belongs to the current interface, where
// the actual ICMP processing occurs.
//
// @return
// @li  < 0,    if packet has been released (packet will not
//          go to the upper layer after this),
// @li  = 0,    the usual return, packet looked and it can be
//          passed to the upper layers
// @li  > 0,    *NOT USED NOW*, Treat as = 0 as default
*/
TInt CIp6Interface::IcmpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, TInet6Packet<TIcmpNdHeader>  &aNd)
    {
#ifdef SYMBIAN_TCPIPDHCP_UPDATE      
    #ifdef _DEBUG
        LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler()")));
    #endif
#endif // SYMBIAN_TCPIPDHCP_UPDATE          
    TInt notify = 0;
#ifndef SYMBIAN_TCPIPDHCP_UPDATE
    TInt dns_flag = 0;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
    TInt count, start;

    const TInt icmp_type = aNd.iHdr->iIcmp.Type();
    TLinkAddr source_link, target_link;

    const TIp6Addr &icmp_src_addr = TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address();
    const TIp6Addr &icmp_dst_addr = TInetAddr::Cast(aInfo.iDstAddr).Ip6Address();

#ifdef _LOG
    // src & dst for logging purposes
    TBuf<70> tmpsrc, tmpdst;
    TInetAddr::Cast(aInfo.iSrcAddr).OutputWithScope(tmpsrc);
    TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpdst);
#endif


    // Setup and Check Validity (part of it)
    // *************************************
    //
    // The ICMP source address can only be either a valid unicast address
    // or unspecified address in some cases for RS and NS.
    //
    const TInt icmp_src_unspecified = icmp_src_addr.IsUnspecified();
    if (TIp46Addr::Cast(icmp_src_addr).IsMulticast())
        goto drop_packet;   // Was not unicast or unspecified

    switch (icmp_type)
        {
        case KInet6ICMP_RouterSol:
            LOG(Log::Printf(_L("\tIF %u [%S] Received RS src=[%S] dst=[%S]"), iScope[0], &iName, &tmpsrc, &tmpdst));
            start = aNd.iHdr->iRS.HeaderLength();
            break;
        case KInet6ICMP_RouterAdv:
            //
            // A Router Advertisement ICMP detected
            //
            LOG(Log::Printf(_L("\tIF %u [%S] Received RA src=[%S] dst=[%S]"), iScope[0], &iName, &tmpsrc, &tmpdst));
            if (!icmp_src_addr.IsLinkLocal())   // .. src must be a link local address.
                goto drop_packet;
            start = aNd.iHdr->iRA.HeaderLength();
            break;
        case KInet6ICMP_NeighborSol:
            LOG(Log::Printf(_L("\tIF %u [%S] Received NS src=[%S] dst=[%S]"), iScope[0], &iName, &tmpsrc, &tmpdst));
            start = aNd.iHdr->iNS.HeaderLength();
            // RFC-2461 says to require !Multicast.. but, that would let Unspecified through!
            // Is it an error in RFC or not? (Need to check message length here, becuase of
            // accessing of the target field!)
            if (start > aNd.iLength || !aNd.iHdr->iNS.Target().IsUnicast())
                goto drop_packet;
            // The NS destination address is either the target address or..
            if (!icmp_dst_addr.IsEqual(aNd.iHdr->iNS.Target()))
                {
                // ..it must be the solicited node multicast corresponding
                // the target address (RFC-2461, 4.3)
                TSolicitedNodeAddr solicited(aNd.iHdr->iNS.Target());
                if (!icmp_dst_addr.IsEqual(solicited))
                    goto drop_packet;
                }
            else if (icmp_src_unspecified)
                // ..apparently, src=:: && target==dst is invalid combination
                // (TAHI), and packet must be dropped...
                goto drop_packet;
            break;
        case KInet6ICMP_NeighborAdv:
            LOG(Log::Printf(_L("\tIF %u [%S] Received NA src=[%S] dst=[%S]"), iScope[0], &iName, &tmpsrc, &tmpdst));
            start = aNd.iHdr->iNA.HeaderLength();
            // RFC-2461 says to require target != Multicast.. but, that would let Unspecified through!
            // Is it an error in RFC or not? (Need to check message length here, becuase of
            // accessing of the target field!)
            if (icmp_src_unspecified || start > aNd.iLength || !aNd.iHdr->iNA.Target().IsUnicast())
                goto drop_packet;
            // Note: Solicited bit/Target Link-layer option vs. multicast destination
            // check is performed after the options pass (see there.) [Can do this,
            // because *currently* none of the NA option processing "commits" any
            // changes to the system state.. however, watch it -- msa]
            break;
        case KInet6ICMP_Redirect:
            LOG(Log::Printf(_L("\tIF %u [%S] Received Redirect src=[%S] dst=[%S]"), iScope[0], &iName, &tmpsrc, &tmpdst));
            if (!icmp_src_addr.IsLinkLocal())   // .. src must be a link local address.
                goto drop_packet;
            start = aNd.iHdr->iRD.HeaderLength();
            if (start > aNd.iLength || !aNd.iHdr->iRD.Destination().IsUnicast())
                goto drop_packet;
            break;
        default:
            return 0;           // Not for me
        }
    //
    // Process Options
    // ***************
    // (to be 100% right in everything, one should probably make
    // this section a separate method with two operating modes, and
    // which is called twice in processing the ND ICMP: (1) to check
    // validity of everything, and if all is OK, (2) execute the
    // options. -- msa)
    //
    // Note: *Currently* only RA's may run into this problem, because
    // RA is the only ND ICMP, in which the options are actually
    // "committed" directly in the options processing (Prefix, Mtu).
    //
    start += aInfo.iOffset;
    count = aInfo.iLength - start;
    if (count < 0)
        goto drop_packet;       // Message is too short to be valid ND.
    while (count > 0)
        {
        TIcmpNdOption option;
        TPtr8 opt((TUint8 *)&option, sizeof(option), sizeof(option));

        RMBuf *p;
        TInt offset, len;
        TUint8 *ptr, type;

        if (!aPacket.Goto(start, p, offset, len))
            return 0;       // Drop instead?
        ptr = p->Buffer() + offset;
        type = *ptr++;
        --len;
        while (len == 0)    // Should loop only once, but 'while' just in
                            // case someone wants RMBuf with zero length...
            {
            p = p->Next();
            if (p == NULL)
                return 0;
            len = p->Length();
            ptr = p->Ptr();
            }
        len = *ptr;
        //
        // All included options must have length > 0. However, by coding
        // it this way, there is a problem that all preceding valid options
        // have already been executed and system state may have been changed
        // for those. To fully comply, one should do two passes over the
        // options, first to check the validity and then execute. -- msa 
        if (len < 1)
            goto drop_packet;   // Option length < 1, drop packet!
        len <<= 3;
        // We don't want panic from perfectly legal but unknown to us
        // options that are longer than any of the "known" ones. Thus,
        // constrain SetLength()...
        opt.SetLength(len > opt.MaxLength() ? opt.MaxLength() : len);
        aPacket.CopyOut(opt, start);
        switch (type)
            {
            // source and target link options are only accepted, if the
            // link layer supports addresses, and ignored otherwise.

            case KInet6OptionICMP_SourceLink:   // Source link-layer address
                if (icmp_src_unspecified)
                    goto drop_packet;           // Illegal, if unspecified source!
                if (iHwAddr.Family() != KAFUnspec)
                    {
                    source_link.SetAddress(option.iLink.Address());
                    source_link.SetFamily(iHwAddr.Family());
                    }
                break;
            case KInet6OptionICMP_TargetLink:   // Target link-layer address
                if (iHwAddr.Family() != KAFUnspec)
                    {
                    target_link.SetAddress(option.iLink.Address());
                    target_link.SetFamily(iHwAddr.Family());
                    }
                break;
            case KInet6OptionICMP_Mtu:          // MTU
                if (icmp_type == KInet6ICMP_RouterAdv)
                    SetMtu(option.iMtu.Mtu(), KInet6MinMtu);
                break;
            case KInet6OptionICMP_Prefix:       // Prefix Information
                if (icmp_type == KInet6ICMP_RouterAdv && !option.iPrefix.Prefix().IsLinkLocal())
                    {
                    const TInt length = option.iPrefix.PrefixLength();
                    const TLifetime lifetime = option.iPrefix.ValidLifetime();
                    const TLifetime preferred = option.iPrefix.PreferredLifetime();

                    if (length > 128)
                        goto drop_packet;   // Garbage!
                    if (preferred > lifetime)
                        break;              // Ignore Option with illogical lifetimes
                    if (option.iPrefix.AFlag()) // Can use this for address generation?
                        // ...should set the aForce flag, if RA was protected by IPSEC... -- msa
                        {
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
                        #ifdef _DEBUG
                            LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler() option.iPrefix.AFlag()")));
                        #endif
                        iGlobalflag = ETrue; 
#endif //SYMBIAN_TCPIPDHCP_UPDATE
                        SetPrefix(option.iPrefix.Prefix(), length, !NeedsND(), lifetime, preferred);
                        }
                    // Note: now multicasts and unspecified addresses are accepted.
                    // Is this a "feature" or should it be prevented? -- msa
                    if (option.iPrefix.LFlag())
                        (void)GetRoute(option.iPrefix.Prefix(), length, KRouteAdd_ONLINK, NULL, &lifetime);
                    notify++;   // .. only for new prefixes.
                    }
                break;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC 5006 Changes
            case KInet6OptionICMP_RDNSS:
                // Process RDNSS only if M Flag is set
                if (aNd.iHdr->iRA.M()) 
                    {
                    if(iRdnssList == NULL)
                        {
                        iRdnssList = CManageRdnssServerList::NewL();                                
                        }       
                    if( (aNd.iHdr->iRA.RouterLifetime())!= 0)
                        {   
                        LOG(Log::Printf(_L("\tIF %u [%S] RECEIVED RDNSS OPTION"), iScope[0], &iName));
                        TInet6OptionICMP_DnsInformationV1 rdnssOption = option.iDnsInformation;
                                        
                        TUint8 numRdnssAddr;
                        if(iRdnssList->RdnssParseOptionHdr(rdnssOption,numRdnssAddr))
                            {                               
                            //Update the received RDNSS entry into RDNSServerList
                            iRdnssList->RdnssProcessOptionData(rdnssOption, numRdnssAddr);
                            iRdnssList->RdnssNameServerUpdate(iNameSer1,(TUint8)0);
                            iRdnssList->RdnssNameServerUpdate(iNameSer2,(TUint8)1);
                            }
                        else
                            {
                            delete iRdnssList;
                            iRdnssList = NULL;
                            goto drop_packet;
                            }
                        }
                    else
                        // Router Lifetime is 0, Delete all RDNSS entries
                        {
                        iRdnssList->RdnssServerListDelete();    
                        // Reset Respository iNameServer1 and iNameServer2 to KAFUnspec 
                        iRdnssList->RdnssNameServerReset(iNameSer1,iRdnssList->GetRdnssFlag());
                        iRdnssList->RdnssNameServerReset(iNameSer2,iRdnssList->GetRdnssFlag());                 
                        }
                        
                    }
                break;
#endif // SYMBIAN_TCPIPDHCP_UPDATE
            default:
                // Handle options, for which is is no fixed assigned type by IANA, and which are
                // configured via TCPIP.INI. Option is enabled, if a non-zero type code is configured.
                // Because the 0 value has own case in switch, there is no need to test against
                // ZERO here (unconfigured option type is ZERO and never matches).
                if (icmp_type == KInet6ICMP_RouterAdv)
                    {
                    if (type == Interfacer().iRA_OptRoute)
                        {
                        // Route Information Option 
                        // Experimental: draft-draves-ipngwg-router-selection-01.txt
                        // Default Router Preferences and More-Specific Routes
                        //
                        const TInt preference = option.iRouteInformation.Prf(); // range guaranteed to be [0..3]!
                        if (preference == ERoutePreference_INVALID)
                            break;  // invalid preference value
                        if (option.iRouteInformation.PrefixLength() > 128)
                            goto drop_packet;   // Garbage !
                        const TLifetime lifetime = option.iRouteInformation.RouteLifetime();
                        // Because the option is copied into a temporary space, Prefix() method is
                        // "safe" to use... (see comment on it's definition!). (However, might
                        // consider opt.FillZ before CopyOut to remove possible distracting, but
                        // harmless garbage.
                        CIp6Route *const route = GetRoute(
                            option.iRouteInformation.Prefix(),
                            option.iRouteInformation.PrefixLength(),
                            KRouteAdd_GATEWAY,
                            &aInfo.iSrcAddr,        // The gateway address
                            &lifetime);
                        if (route)
                            route->iMetric = KPreferenceMetric[preference];
                        }
#ifndef SYMBIAN_TCPIPDHCP_UPDATE
                    else if (type == Interfacer().iRA_OptDns)
                             {
                             // Experimental: draft-jeong-dnsop-ipv6-discovery-03.txt
                             // IPv6 DNS Configuration based on Router Advertisement
                             //
                             // *WARNING* Just a "proof of concept", not complete
                             // implementation (more like a "placeholder code", indicates
                             // the point where real implementation should go...)
                             // - preference is ignored
                             // - other than "delete", lifetime is ignored
                             // - only two first addresses processed
                             // - does not do much sanity check (delete and insert same address).
                             const TIp6Addr &addr = option.iDnsInformation.Address();
                             const TLifetime lifetime = option.iDnsInformation.Lifetime();
                             if (lifetime == 0)
                                 {
                                 // Should remove the matching DNS server address
                                 // (if present)
                                 if (iNameSer1.Ip6Address().IsEqual(addr))
                                     {
                                     iNameSer1.Init(KAFUnspec);
                                     dns_flag |= 4;  // mark "dns changed"
                                     }
                                 if (iNameSer2.Ip6Address().IsEqual(addr))
                                     {
                                     iNameSer2.Init(KAFUnspec);
                                     dns_flag |= 4;  // mark "dns changed"
                                     }
                                 }
                             else if (!addr.IsUnspecified())
                                 {
                                 // Add DNS server address
                                 // (ignore prefs, take the first two)
                                 if ((dns_flag&1) == 0)
                                     {
                                     iNameSer1.SetAddress(addr);
                                     dns_flag |= 1;
                                     }
                                 else if ((dns_flag&2) == 0)
                                     {
                                     iNameSer2.SetAddress(addr);
                                     dns_flag |= 2;
                                     }
                                 }
                             }
#endif //SYMBIAN_TCPIPDHCP_UPDATE
                    }
                break;
            case 0: // To avoid getting into default branch with type==0!
                break;
            }
        //
        // Advance to the next option
        //
        count -= len;
        start += len;
        }

    //
    // Execute the actual ND ICMP
    // **************************
    //
    TIp6AddressInfo *my_id;

    switch (icmp_type)
        {
        case KInet6ICMP_RouterAdv:
            //
            // The router is advertising as a default route
            // Add a default route entry (will also handle changes
            // in the lifetime and even destruction, if lifetime == 0.
            //
            // *NOTE* Even if the RouterLifeTime is 0, ISROUTER is set, because
            // this is a valid router, it's just not a default router.
            //
            (void)GetRoute(icmp_src_addr, 128, KRouteAdd_OVERRIDE|KRouteAdd_ISROUTER, &source_link);
                {
                const TInt preference = aNd.iHdr->iRA.Prf(); // range guaranteed to be [0..3]!
                // If prf is invalid, don't install default route!
                const TLifetime lifetime = (preference == ERoutePreference_INVALID) ? 0 : aNd.iHdr->iRA.RouterLifetime();
                // Disable Router Discovery process (sending RS's), if
                // at least one RS has been sent, see RFC 2461 6.3.7),
                // And if this RA had non-zero lifetime.
                if (lifetime && iRetryRS > 0)
                    iRetryRS = KMaxTUint8;  // ...should be large enough!
                CIp6Route *const route = GetRoute(KInet6AddrNone, 0, KRouteAdd_GATEWAY, &aInfo.iSrcAddr, &lifetime);
                if (route)
                    route->iMetric = KPreferenceMetric[preference];
                }
            notify++;
            if (aNd.iHdr->iRA.ReachableTime() && aNd.iHdr->iRA.ReachableTime() != iND.iReachableTime)
                {
                iND.iReachableTime = aNd.iHdr->iRA.ReachableTime();
                SetReachableTime();
                }
            if (aNd.iHdr->iRA.RetransTimer() && aNd.iHdr->iRA.RetransTimer() != iND.iRetransTimer)
                {
                iND.iRetransTimer = aNd.iHdr->iRA.RetransTimer();
                SetRetransTimer();
                }
            if (aNd.iHdr->iRA.CurHopLimit())
                iHopLimit = aNd.iHdr->iRA.CurHopLimit();
            break;
        case KInet6ICMP_NeighborSol:
            {
#ifdef SYMBIAN_TCPIPDHCP_UPDATE              
            #ifdef _DEBUG
                LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler() KInet6ICMP_NeighborSol recieved NS")));
            #endif
#endif //SYMBIAN_TCPIPDHCP_UPDATE                  
            // include proxy into "my address" class
            TIp6Addr &target = aNd.iHdr->iNS.Target();
            my_id = IsMyAddress(target, 1);
            if (my_id == NULL)
                {
                // Target is not my assigned address, ...
                if (!icmp_src_unspecified)
                    break;
                // ..., but the NS was a DAD probe. Check if
                // the plain ID part matches with any of my id's (including tentative ones)...
                //
                // * REQUIRE ID IS USED ONLY BY ONE HOST *
                //
                my_id = IsMyId(target);
                if (my_id == NULL)
                    break;      // Target has no relation with me, ignore NS.
                if (my_id->IsTentative())
                    {
                    //
                    // Someone is doing Duplicate Address Detection on my tentative address!
                    // Decide this a duplicate address collision..
                    //
                    RemId(my_id);       // Kill this ID
                    notify++;
                    break;
                    }
                //
                // Someone is doing Duplicate Address Detection on a address with a prefix
                // which is not used by me currently (because IsMyAddress() failed, but the
                // ID part matches one of my assigned id's. [If I ever acquire the same prefix,
                // there will be a collisions and confusion...].
                //
                // Could try to kill it by sending DAD probe, but that would result immediate
                // packet storm, if two hosts on the net had this strategy... -- msa
                // Instead, fall through to the standard NA code (and "incorrectly" just advertise the
                // address...)
                if (Interfacer().iNoDefendId)
                    break; // "Defending ID" has been disabled, ignore DAD NS
                }
            //
            // Normal/DAD NS for my assigned address
            //
            CIp6Route *route = NULL;
            const TIp6Addr *dst = &KInet6AddrAllNodes;
            if (!icmp_src_unspecified)
                {
                dst = &icmp_src_addr;
                // We need the route entry, whether link layer is known or not
                route = GetRoute(*dst, 128, KRouteAdd_OVERRIDE, &source_link);
                if (!route)
                    break;      // No route, and it couldn't be created for some reason!
                // Things get awkward if there is no cached link layer address
                // to be used in the reply.
                if (route->iState == CIp6Route::EIncomplete)
                    route->StartND(my_id->iId);// Use my link local as ND source!
                }
#ifdef SYMBIAN_TCPIPDHCP_UPDATE              
            //If it is DADNS,the solicitation's Source Address is unspecified then the destination address for the NA should be 
            //the all-nodes multicast address(Refer RFC 4861 sec 4.4)
            else if(icmp_src_unspecified)
                {
                #ifdef _DEBUG
                    LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler() KInet6ICMP_NeighborSol recieved NS (icmp_src_unspecified)")));
                #endif
                //Get the target route entry
                CIp6Route *route1 = GetRoute(aNd.iHdr->iNS.Target(), 64, KRouteAdd_MYPREFIX | KRouteAdd_UPDATEONLY);
                if (route1==NULL)
                    {
                    //if the target route is expired/no route dont send NA
                    #ifdef _DEBUG
                        LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler() KInet6ICMP_NeighborSol recieved NS route1==NULL")));
                    #endif
                    break;      // No route, and it couldn't be created for some reason!
                    }
                // Things get awkward if there is no cached address
                // to be used in the reply.
                if (route1->iState == CIp6Route::EIncomplete)
                    route1->StartND(my_id->iId);// Use my target address as ND source!
                }
#endif // SYMBIAN_TCPIPDHCP_UPDATE  
            const TInt message_type = KInet6ICMP_NeighborAdv | (my_id->IsProxy() ? KSendNeighbors_NO_OVERRIDE : 0);
#ifdef _LOG
            TInetAddr tmp(route ? route->iPrefix : KInet6AddrNone, 0);
            tmp.OutputWithScope(tmpsrc);
            tmp.SetAddress(aNd.iHdr->iNS.Target());
            tmp.OutputWithScope(tmpdst);
            Log::Printf(_L("\tIF %u [%S] Sending NA(%d) dst=[%S] target=[%S]"), iScope[0], &iName, message_type, &tmpsrc, &tmpdst);
#endif
            SendNeighbors(message_type, route, target);
            }
            break;
        case KInet6ICMP_NeighborAdv:
            {
            LOG(Log::Printf(_L("<>\tCIp6Interface::IcmpHandler() KInet6ICMP_NeighborAdv recieved NA")));
            const TInt flags = KRouteAdd_NEIGHBOR | KRouteAdd_UPDATEONLY |
                (aNd.iHdr->iNA.O() ? KRouteAdd_OVERRIDE : 0) |
                (aNd.iHdr->iNA.S() ? KRouteAdd_SOLICITED : 0) |
                (aNd.iHdr->iNA.R() ? KRouteAdd_ISROUTER : KRouteAdd_ISHOST);
            // Additional "validity checks" for NA...
            const TInt dst_is_mc = TIp46Addr::Cast(icmp_dst_addr).IsMulticast();
            if (flags & KRouteAdd_SOLICITED)
                {
                // If destination was multicast, solicited bit cannot be set.
                if (dst_is_mc)
                    goto drop_packet;
                }
            TIp6Addr &target = aNd.iHdr->iNS.Target();
            my_id = IsMyId(aNd.iHdr->iNA.Target());
            if (my_id)
                {
                // Someone is advertising with id part matching my id???
                //
                // * REQUIRE ID IS USED ONLY BY ONE HOST *
                //
                if (my_id->IsTentative())
                    {
                                    
                    // If doing Duplicate Address Detection, assume duplicate, if NA
                    // for my tentative address is received! (RFC-2462, 5.4.4)
                    RemId(my_id);       // Kill this ID
                    notify++;
                    
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
                    //A tentative address that is determined to be a duplicate as described
                    //above MUST NOT be assigned to an interface, and the node SHOULD log a
                    //system management error (RFC-4862,5.4.5)
                    iState = EFlow_NOTCONFIGURE;
                    NotifyInterfaceEvent(EventTypeModify);
                    LOG(Log::Printf(_L("\t Interface is not configured as DAD detects Duplicate Adress")));
#endif //SYMBIAN_TCPIPDHCP_UPDATE
                    break;
                    }
                //
                // Here is a problem: Someone is using address that has the same id part
                // as I have...
                if (IsMyAddress(target, 1))
                    {
                    // ..it's even my current address. Just ignore it (but, there is
                    // another host on link using my address--this is not a good
                    // situation!)
                    break;
                    }
                // The address has my id, but with a prefix which not configured (YET!)
                // for me. Accept advertisement. This works fine as long as the prefix
                // does not get autoconfigured for me (e.g. as long as no router
                // advertises it with A=1 in prefixes).
                }
            if (dst_is_mc && iHwAddr.Family() != target_link.Family())
                // Multicast NA *MUST* have target link, if the link
                // is using addresses. (however, allow them to affect
                // DAD process and test this after that)
                goto drop_packet;
            // Must not create entry, if it does not already exist
            (void)GetRoute(target, 128, flags, &target_link);
            }
            break;
        case KInet6ICMP_Redirect:
            {
            // More validity checks: accept redirects only if they actually come from a router that
            // would be getting the packets sent to the specified Destination address.
#ifdef _LOG
            // reuse tmpsrc for target
            // reuse tmpdst for destination
            TInetAddr tmp;
            tmp.SetAddress(aNd.iHdr->iRD.Destination());
            tmp.OutputWithScope(tmpdst);
            tmp.SetAddress(aNd.iHdr->iRD.Target());
            tmp.OutputWithScope(tmpsrc);
#endif
            // Need to beef up the route with scope?
            const TUint dstType = (TUint)(aNd.iHdr->iRD.Destination().Scope()-1);
            const CIp6Route *const route =
                dstType > EScopeType_NET ? NULL : Interfacer().FindRoute(aNd.iHdr->iRD.Destination(), iScope[dstType], dstType);

            if (route != NULL &&
                &route->iInterface == this &&
                route->IsGateway() &&
                icmp_src_addr.IsEqual(route->iAddress.Ip6Address()))
                {
                LOG(Log::Printf(_L("\tIF %u [%S] Redirect [%S] to [%S] accepted"), iScope[0], &iName, &tmpdst, &tmpsrc));
                TInt flags = KRouteAdd_OVERRIDE;
                TIp6Addr &target = aNd.iHdr->iRD.Target();
                if (!aNd.iHdr->iRD.Destination().IsEqual(target))
                    {
                    if (aNd.iHdr->iRD.Target().IsLinkLocal())
                        flags |= KRouteAdd_ISROUTER;
                    else
                        goto drop_packet;
                    }
                // Create host route for the target
                (void)GetRoute(aNd.iHdr->iRD.Target(), 128, flags, &target_link);
                if (flags & KRouteAdd_ISROUTER)
                    {
                    // Target is a router, need to add redirect route for the destination,
                    // pointing to the gateway (= target).
                    TInetAddr gateway;
                    gateway.SetAddress(target);
                    (void)GetRoute(aNd.iHdr->iRD.Destination(), 128, CIp6Route::ERedirect, &gateway);
                    }
                //
                // Previously, any flow using the rerouted destination address was assigned
                // to the 'route'. Need to kick those flows to recheck their nexthop (e.g.
                // all flows going to the 'destination' must now be assigned to the new
                // redirected route
                // Note: this may be bad thing, if there are many redirects and many unaffected
                // flows are attached to the 'route' (which could be the default route).
                // - more intelligent SetChanged(new_route), wich only affect the flows that have
                //   a better match with the new route (the redirect), or
                // - change of logic and implement true "destination cache" (flow send would have
                //   to look into it for every packet) [or, view flows themselves as "destination
                //   cache"?]
                // -- msa
                route->SetChanged();
                }
            else
                {
                LOG(Log::Printf(_L("\tIF %u [%S] Redirect [%S] to [%S] rejected"), iScope[0], &iName, &tmpdst, &tmpsrc));
                goto drop_packet;
                }
            }
            break;
        default:
            break;
        }
    //
    //
    if (notify)
        {
        NotifyFlows(EFlow_READY);
        Interfacer().ScanHoldings();
        }
    
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
    //RFC-5006 Changes
    if (iRdnssList!=NULL)
        {
        if(iRdnssList->GetRdnssFlag())      
            {
            // DNS information changed
            NotifyInterfaceEvent(EventTypeModify);
            }
        }
#else
    if (dns_flag)
        {
        // DNS information changed
        NotifyInterfaceEvent(EventTypeModify);
        }
#endif // SYMBIAN_TCPIPDHCP_UPDATE

    return 0;
    //
    // Drop packets (should only be used for obviously bad packets)
    //
drop_packet:
    aPacket.Free();
    return -1;
    }


#ifdef ARP
//
// CIp6Manager::ArpHandler
// ***********************
/**
// Receives all ARP packets
//
// Currently always "consumes" packet,
// and return is always < 0
*/
TInt CIp6Manager::ArpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo)
    {
    TInet6Packet<TInet6HeaderArp> arp(aPacket);
    for (;;)
        {
        if (arp.iHdr == NULL)
            break;      // Too short, ignore!
        // Remap with real packet length, and assume all of the
        // ARP packet will fit into single RMBuf. Should be okay,
        // as only IPv4 is being implemented and map will only
        // fail if hwaddr length is > 56 bytes! Computed by
        // RMBuf = 128 bytes, prlen = 4 for IPv4, fixed part is
        // 8 bytes => (128 - 8 - 2*prlen) / 2 = 56. For longer
        // than 56 hwaddr, this ARP stops working!  -- msa
        arp.Set(aPacket, 0, arp.iHdr->HeaderLength());
        if (arp.iHdr == NULL)
            break;      // Too short (corrupt packet, or hwlen > 56)
        if (arp.iHdr->PrAddrLen() != 4 ||
            arp.iHdr->ProtocolType() != KArpProtocolType_IP)
            break;      // Only IPv4 supported.
        CIp6Interface *const srcif = FindInterface(aInfo.iInterfaceIndex);
        if (!srcif)
            break;      // Cannot find interface...
        (void)srcif->ArpHandler(aPacket, aInfo, arp);
        break;      // Alway exit the loop!
        }
    //
    // Packet always consumed, whether processed or not!
    //
    aPacket.Free();
    if (iScanHolding)
        ScanHoldings();
    return -1;
    }

TInt CIp6Interface::ArpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, TInet6Packet<TInet6HeaderArp> &aArp)
    {
    if (iHwAddr.GetUserLen() != aArp.iHdr->HwAddrLen())
        return 0;   // Hardware address length does not match the interface, ignore ARP!
    if (aArp.iHdr->PrAddrLen() != 4)
        return 0;   // Protocol address is IPv4, must be 4 bytes long!
    const TUint operation = aArp.iHdr->Operation();
    if (operation != EArpOperation_REQUEST && operation != EArpOperation_REPLY)
        return 0;   // Only the basic REQUEST/REPLY are supported!

    TIp46Addr sender(0), target(0);
    TPtr8(&sender.u.iAddr8[12], 4).Copy(aArp.iHdr->SenderPrAddr());
    TPtr8(&target.u.iAddr8[12], 4).Copy(aArp.iHdr->TargetPrAddr());

    TLinkAddr sender_link, target_link;
    sender_link.SetFamily(iHwAddr.Family());
    target_link.SetFamily(iHwAddr.Family());
    sender_link.SetAddress(aArp.iHdr->SenderHwAddr());
    target_link.SetAddress(aArp.iHdr->TargetHwAddr());
#ifdef _LOG
        {
        TLogAddressPrefix link(TInetAddr::Cast(sender_link));
        TLogAddressPrefix ip(sender);
        Log::Printf(_L("\tIF %u [%S] ARP (bytes=%d) sender=%S [%S]"), iScope[0], &iName, aInfo.iLength, &ip, &link);
        link.Set(target_link);
        ip.Set(target);
        Log::Printf(_L("\t\tTarget=%S [%S]"), &ip, &link);
        }
#endif
    if (sender_link.Address() == iHwAddr.Address())
        {
        // The sender has my hardware address. This may happen because
        // 1) this is my own ARP echoed back from the link for some reason
        // 2) someone else on the link has the same link layer address
        // In either case, there is not much that can be done, just drop
        // the ARP for now...
        LOG(Log::Printf(_L("\t\tARP sender link is my hwaddr, ARP ignored")));
        return 0;
        }
    //
    // Assume IPv4 addressess are stored as 128 bit ids (e.g. iPrefix == 0)
    // (thus it is sufficient to check the id)
    //
    TIp6AddressInfo *my_id = IsMyId(target);
    if (my_id && my_id->iPrefix != 0)
        my_id = NULL;   // ignore other matches

    // DAD stuff
    // *********
    // ..if sender matches tentative => declare collision on sender_id
    // ..if sender matches my address => declare nasty collision on sender_id
    // ..if sender is 0.0.0.0 and target matches tentative => declare collision on my_id
    //
    TIp6AddressInfo *dup_id = IsMyId(sender);
    if (dup_id && dup_id->iPrefix != 0)
        dup_id = NULL;

    if (my_id && my_id->IsTentative())
        {
        if (sender.u.iAddr32[3] == 0)
            {
            // If sender = None, it couldn't have been found as duplicate!
            ASSERT(dup_id == NULL);
            dup_id = my_id;
            }
        my_id = NULL;   // Anyways, it's not yet my official address, do not reply!
        } 
    TBool gratuitousArpFromOtherHost = EFalse;
    TBool defendIPAddress = EFalse;
    if (dup_id)
        {
        if (aArp.iHdr->SenderHwAddr() !=  iHwAddr.Address() && operation == EArpOperation_REQUEST)
                 gratuitousArpFromOtherHost = ETrue;
        DuplicateAddress(dup_id, defendIPAddress, gratuitousArpFromOtherHost);
        if(!defendIPAddress)    
            return 0;       
        }

    if(!defendIPAddress)
    {
    //
    // Choose Flags, if target==me, force creation of the entry
    //
    const TInt flags = KRouteAdd_OVERRIDE |
        ((my_id != NULL) ?
        (operation == EArpOperation_REPLY ? KRouteAdd_SOLICITED : 0) :
        KRouteAdd_UPDATEONLY);

    // Update neighbor cache only if sender IP address is defined
    if (sender.u.iAddr32[3])
        (void)GetRoute(sender, 128, flags, &sender_link);
    }

    // If target was me and operation is a Request, swap sender and target
    // and fill in my hw address.
    if ((my_id || defendIPAddress) && operation == EArpOperation_REQUEST)
        {
        if(defendIPAddress)
        {
        aArp.iHdr->TargetHwAddr().Copy(iHwAddr.Address());
        }
        else 
        {
        aArp.iHdr->TargetHwAddr().Copy(aArp.iHdr->SenderHwAddr()); 
        }
        aArp.iHdr->TargetPrAddr().Copy(aArp.iHdr->SenderPrAddr());
        aArp.iHdr->SenderHwAddr().Copy(iHwAddr.Address());
        aArp.iHdr->SenderPrAddr().Copy(TPtrC8(&target.u.iAddr8[12], 4));
        aArp.iHdr->SetOperation(EArpOperation_REPLY);
        aInfo.iProtocol = KProtocolArp;

        // draft-ietf-zeroconf-ipv4-linklocal-05.txt says that whenever
        // the sender is ipv4 link local, then the replies (and requests)
        // must always be sent to the broadcast address [IMHO, this is
        // a bit dubious rule, but if it is so specified, comply... -- msa]
        if (target.Scope() == KIp6AddrScopeLinkLocal)
            {
            TInetAddr::Cast(aInfo.iDstAddr).SetAddress(KInetAddrBroadcast);
            aInfo.iFlags = KIpBroadcastOnLink;  // Dst is broadcast.
            }
        else
            {
            aInfo.iDstAddr = sender_link;
            aInfo.iFlags = 0;                   // Dst is unicast hw address.
            }
        if (iNifIf)
            {
            aPacket.Pack();
            iNifIf->Send(aPacket, NULL);
            }
        }
    return 0;
    }

#endif


// CIp6Interface::Ip4RedirectHandler
// *********************************
/**
// Handle IPv4 Redirect ICMP
//
// @param   aPacket the returned ICMP error packet
// @param   aInfo   the associated info (iIcmp != 0)
*/
void CIp6Interface::Ip4RedirectHandler(const RMBufRecvPacket &aPacket, const RMBufRecvInfo &aInfo)
    {
    // Because IPv4 ICMP redirect is catched from the ICMP Error handling
    // path, the info block is already loaded with the data extracted from
    // from ICMP error message as follows:
    //
    // - aInfo.iParemeter == Gateway address (IPv4 in host byteorder)
    // - aInfo.iSrcAddr == should be my address of the original packet
    // - aInfo.iDstAddr == should be the destination of the original packet
    //
    // Handle all redirects (codes 0-3) as host redirects (ignore other codes)
    if (aInfo.iCode > 3)
        return;

    const TIp46Addr gateway(aInfo.iParameter);
    const TIp6Addr &dst_addr = TInetAddr::Cast(aInfo.iDstAddr).Ip6Address();

    //
    // Check various things, whether to actually accept the redirect
    //
    // .. did I send this packet?
    if (!IsMyAddress(TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address()))
        return; // -- original source is not my address, just ignore the redirect

    // ..the gateway must be another node on the link
    CIp6Route *route = FindRoute(gateway, NULL);
    if (route == NULL || !(route->IsOnlink() || route->IsHostRoute()))
        return;

    // ...check that destination would actually have been sent to the
    // ...router that sent the ICMP redirect.
    const TUint dstType = dst_addr.Scope()-1;
    if (dstType > EScopeType_NET)
        return; // -- bad scope value
    route = Interfacer().FindRoute(dst_addr, iScope[dstType], dstType);
    if (route == NULL || &route->iInterface != this || !route->IsGateway())
        return; // -- nope, not routed to a gateway on this interface
    const TIpHeader *const ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
    if (!ip || ip->ip4.Version() != 4)
        return; // -- probably bad packet
    if (!TIp46Addr(ip->ip4.SrcAddr()).IsEqual(route->iAddress.Ip6Address()))
        return; // -- would not be sent to source of the icmp redirect

    //
    // Redirect accepted, do the stuff...
    //
    (void)GetRoute(gateway, 128, KRouteAdd_ISROUTER);   // Must mark the gateway as ROUTER!
    const TInetAddr gw(gateway, 0);
    (void)GetRoute(dst_addr, 128, CIp6Route::ERedirect, &gw);
#ifdef _LOG
        {
        TBuf<70> tmpdst, tmpgw;
        TInetAddr::Cast(aInfo.iDstAddr).OutputWithScope(tmpdst);
        gw.OutputWithScope(tmpgw);
        Log::Printf(_L("\tIF %u [%S] Redirecting IPv4 dst=[%S] to [%S]"), iScope[0], &iName, &tmpdst, &tmpgw);
        }
#endif
    // Previously, any flow using the rerouted destination address was assigned
    // to the 'route'. Need to kick those flows to recheck their nexthop (e.g.
    // all flows going to the 'destination' must now be assigned to the new
    // redirected route
    route->SetChanged();
    }

//
// ********************************
// MIfUser Interface Implementation
// ********************************
//

/**
//  The NIF::BindL failed.
//
//  A relic from ancient time -- not used currently.
//
//  @deprecated
*/
void CIp6NifUser::IfUserBindFailure(TInt aResult, TAny* aId)
    {
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserBindFailure(%d, %d)"), &LogName(), aResult, (TInt)aId));

    // Remove warnings
    (void) aResult;
    (void) aId;
    }

/**
// Introduce a new interface.
//
// @param aIf   The NIF
// @param aId   A relic from history -- not used currently.
*/
void CIp6NifUser::IfUserNewInterfaceL(CNifIfBase* aIf, TAny* aId)
    {
    (void) aId;     // Remove warning

    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserNewInterface(%d, %d)"), &LogName(), (TInt)aIf, (TInt)aId));
    if (!aIf)
        return;     // Should do something? Does this ever happen?
    TNifIfInfo info;
    aIf->Info(info);
    CIp6Interface *iface = iManager.GetInterfaceByNameL(info.iName);
    if (iface->IsNetdial())
        User::Leave(KErrBadDriver); // Interface is giving bad name ('')

    LOG(Log::Printf(_L("CIp6NifUser[%S]::AddInterface(CIp6Interface[%S])"), &LogName(), &iface->iName));
    aIf->Open();    // Prevent aIf from being deleted while in DoBind.
    (void)iface->DoBind(this, aIf);
    aIf->Close();   // *NOTE* This will delete the CNifIfBase instance
                    // if there was no other references to it!
    }
    
/**
// Reports closed interface using negative error code
// Reports status of operable interface using positive status code
//
// @param aResult Error code of closed interface or status of operable interface
// @param aIf The NIF
*/
void CIp6NifUser::IfUserInterfaceDown(TInt aResult, CNifIfBase* aIf)
    {

    CIp6Interface *const iface = iManager.FindInterface(aIf);
    //
    // IfUserInterfaceDown can only be processed, if the stack has an
    // instance of CIp6Interface that is actually connected to the aIf
    // (IfUserNewInterface has been called for it!). The above find
    // returns non-NULL iface only iff  "iface->iNifIf == aIf".
    //
    if (iface == NULL)
        {
        // NIFMAN is reporting interface down, which has no
        // corresponding instance within the stack -- ignore
        // silently... [this may happen if interface has been
        // deleted from the stack, for example via ifconfig
        // socket options]
        LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserInterfaceDown(%d, %d) NOT FOUND"), &LogName(), aResult, (TInt)aIf));
        return;
        }
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserInterfaceDown(%d, %d) is CIp6Interface[%S])"), &LogName(), aResult, (TInt)aIf, &iface->iName));

    const TInt link_change = (aResult == KErrLinkConfigChanged);
    if (!link_change && aResult <= 0)
        {
        // First notify the error state to all flows attached to
        // this interface. This will affect all flows that don't
        // the flag iNoInterfaceError set.
        iface->NotifyFlows(aResult);
        }
    else if (aResult > 0)
        {
        // IfUserInterfaceDown positive values are used to report NIF events to stack.
        switch (aResult)
            {
            case KLinkLayerOpen:
                if (iface->iIsSuspended)
                    {
                    // Flip flows to trigger CanSend()
                    iface->NotifyFlows(EFlow_HOLD, ETrue);
                    iface->NotifyFlows(EFlow_READY, ETrue);
                    iface->iIsSuspended = FALSE;
                    }
            break;
            case KDataTransferTemporarilyBlocked:
                iface->iIsSuspended = TRUE;
            break;
            }
        // Just return here when processing positive values
        return;
        }
    // Note: if there is no holding route, flows will be terminated
    // by the Reset(). Currently, holding *should* always exist.
    for (CIp6Route *rt = iface->iRouteList; rt != NULL; rt = rt->iNext)
        Interfacer().MoveToHolding(*rt);

    if(link_change)
        {
        // In case link is changed, just reset the interface but do not delete it.
        iface->Reset(link_change);
        }
    else
        {
        // Delete interface instance when it goes down.
        Interfacer().RemoveInterface(iface);
        }

    if (iNetwork)
        iNetwork->Protocol()->Error(aResult, NULL);
    }

void CIp6NifUser::IfUserOpenNetworkLayer()
    {
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserOpenNetworkLayer()"), &LogName()));
    __ASSERT_ALWAYS(iNetwork, User::Invariant());
    iNetwork->Protocol()->Open();
    iManager.iNifCount++;
    iManager.IncUsers();
    }

void CIp6NifUser::IfUserCloseNetworkLayer()
    {
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserCloseNetworkLayer()"), &LogName()));
    __ASSERT_ALWAYS(iNetwork, User::Invariant());
    --iManager.iNifCount;
    iManager.DecUsers();
    iNetwork->Protocol()->Close();
    }

CProtocolBase* CIp6NifUser::IfUserProtocol()
    {
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserProtocol() --> %d"), &LogName(), (TInt)iNetwork));
    return iNetwork ? iNetwork->Protocol() : NULL;
    }

TBool CIp6NifUser::IfUserIsNetworkLayerActive()
    {
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserIsNetworkLayerActive() iUsers=%d, iNifCount=%d --> %d"),
        &LogName(), iManager.iUsers, iManager.iNifCount, (iManager.iUsers - iManager.iNifCount) > 0));
    return (iManager.iUsers - iManager.iNifCount) > 0;
    }

TBool CIp6NifUser::IfUserIsNetworkLayerActive(CNifIfBase *aIf)
    {
    if (aIf == NULL)
        {
        LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserIsNetworkLayerActive(NULL) *BUG IN NIFMAN* "), &LogName()));
        return IfUserIsNetworkLayerActive();
        }
    CIp6Interface *const iface = iManager.FindInterface(aIf);
    if (iface == NULL)
        {
        LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserIsNetworkLayerActive(%d) *BUG IN NIFMAN"), &LogName(), (TInt)aIf));
        return IfUserIsNetworkLayerActive();
        }
    LOG(Log::Printf(_L("CIp6NifUser[%S]::IfUserIsNetworkLayerActive(%S) returns %d"),
        &LogName(), &iface->iName, iface->iFlows > 0));
    return iface->iFlows > 0;
    }
#ifdef SYMBIAN_TCPIPDHCP_UPDATE  
/**
// Do DAD for my blobal address
// Ref: RFC 4862
// @param aPrefix The prefix of an IP address,part of prefix information sent by RA
// @param aLength The prefix length,part of prefix information sent by RA
*/
void CIp6Interface::PerformDADForGlobalAddress(const TIp6Addr &aPrefix,const TUint aLength)
    {
    #ifdef _DEBUG
        LOG(Log::Printf(_L("\tCIp6Interface::PerformDADForGlobalAddress is called")));
    #endif
    if (&aPrefix == NULL)
            return;
    
    TInetAddressInfo info;
    info.iAddress = aPrefix;
    MakeFullAddress(info.iAddress, aLength,iAddress.iId.u.iAddr8, sizeof(iAddress.iId.u.iAddr8));
    #ifdef _DEBUG
        LOG(Log::Printf(_L("\tCIp6Interface::PerformDADForGlobalAddress,MakeFullAddress done")));
    #endif
    TIp6AddressInfo *address;
    // check info.iAddress matches any of the id's for the interface (also tentative ones!)
    address = IsMyId(info.iAddress);
    if(address == NULL)
        {
        #ifdef _DEBUG
            LOG(Log::Printf(_L("\tCIp6Interface::PerformDADForGlobalAddress,address is NULL so returning from the method")));
        #endif  
        return;
        }
    if (!address->IsTentative())
        {
        #ifdef _DEBUG
            LOG(Log::Printf(_L("\tCIp6Interface::PerformDADForGlobalAddress,sending NS")));
        #endif  
        SendNeighbors(KInet6ICMP_NeighborSol, NULL,info.iAddress,&KInet6AddrNone);
        }
    else
        {
        //it is not my address ignore it
        #ifdef _DEBUG
            LOG(Log::Printf(_L("\tCIp6Interface::PerformDADForGlobalAddress,address->IsTentative()")));
        #endif
        return;
        }
    }
#endif // SYMBIAN_TCPIPDHCP_UPDATE  

#ifdef __ARMCC__
#pragma pop
#endif
