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
// iface.h - IPv6/IPv4 interface and route manager
//



/**
 @internalComponent
*/
#ifndef __IFACE_H__
#define __IFACE_H__

// The preprocessor symbol: ARP
// ----------------------------
// Add code for doing IPv4 Address Resolution Protocol (ARP) on
// IPv4 interfaces that specify "NeedNd". (also needed in ip6.cpp)
//
#undef ARP
#define ARP	1		// include IPv4 ARP code always for now.

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC 5006 definitions
#define RDNSS_REFRESH_TIMEOUT 30
#define RDNSS_NAMESERVER1 1
#define RDNSS_NAMESERVER2 2
#define RDNSSMINLEN 24 // Minimum Length is 3 for a single Address i.e (8 * 3 = 24 Octets)
#endif //SYMBIAN_TCPIPDHCP_UPDATE

#include <e32base.h>
#include <ip6_hook.h>
#include <in_bind.h>
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
#include <icmp6_hdr.h>
#endif //SYMBIAN_TCPIPDHCP_UPDATE
//
// The special returns from StartSending and Error calls are
//
//	The generic rules are as follows:
//
//		returns > 0,	ready to send data out (was not possible previously)
//		returns = 0,	no further action required
//		returns < 0,	interface is down
//
const TInt KIfaceTransition_UP		= 2;	// Interface changed from PENDING/DOWN to READY
const TInt KIfaceTransition_READY	= 1;	// Interface changed from HOLD to READY
const TInt KIfaceTransition_NONE	= 0;	// No change in the interface state (only PENDING, READY or HOLD)
const TInt KIfaceTransition_DOWN	= KErrNotReady;	// (never compare return value against this directly!), only for < 0.

class TIcmpTypeCode
	/**
	* A help class to package both IPv4 and IPv6 ICMP type and code.
	*
	* This is just a parameter to MNetworkServiceExtension::IcmpWrap method.
	*/
	{
public:
	TIcmpTypeCode(TUint8 a4type, TUint8 a4code, TUint8 a6type, TUint8 a6code)
		: i4type(a4type), i4code(a4code), i6type(a6type), i6code(a6code) {}
	const TUint8 i4type;
	const TUint8 i4code;
	const TUint8 i6type;
	const TUint8 i6code;
	};


//	MNetworkServiceExtension
//	************************
//	Extends the MNetworkService with additional private methods
//	which are only used betwen the interface manager and IP
//	protocol instance.
class CNifIfBase;
class MNetworkServiceExtension : public MNetworkService
	{
public:
	// InterfaceAtteched is called just after the CNifIfBase pointer has
	// been stored into the internal interface manager instance (CIp6Interface),
	// aIf->Open() has been called.
	virtual void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf) = 0;
	// InterfaceDetached is called just before the CNifIfBase pointer is going
	// going to be removed from the internal interface managere instance
	// (CIp6Interface) and before the aIf->Close() is being called.
	virtual void InterfaceDetached(const TDesC &aName, CNifIfBase *aIf) = 0;
	// Wrap a packet into ICMP error reply
	virtual void IcmpWrap(RMBufChain &aPacket, const TIcmpTypeCode aIcmp, const TUint32 aParameter = 0, const TInt aMC = 0) = 0;
	// Fragment packet to specificied MTU
	virtual TBool Fragment(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TInt aMtu, RMBufPktQ &aFragments) = 0;
	};

//
//	CIfManager
//	**********
//
class MNifIfUser;
class MFlowManager;
class CFlowContext;
class RTimeout;
class CIfManager : public CBase, public MInterfaceManager
	{
	friend class CProtocolFamilyInet6;
	//
	// Construct and destruct (only used by the INET6 Family object)
	//
	static CIfManager *NewL();
protected:
	virtual ~CIfManager() {}

public:
	virtual CFlowContext *NewFlowL(const void *aOwner, MFlowManager *aManager, TUint aProtocol) = 0;
	virtual CFlowContext *NewFlowL(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow) = 0;
	virtual TInt SetChanged() const = 0;
	//
	//
	virtual TInt StartSending(CNifIfBase *aIface) = 0;
	virtual TInt Error(TInt aError, CNifIfBase *aIface) = 0;
	//
	// Protocol registering (iNifUsers list)
	//
	virtual MNifIfUser *Register(MNetworkServiceExtension *aService) = 0;	// Makes protocol visible to interfaces
	virtual void Unregister(MNetworkServiceExtension *aService) = 0;		// Removes protocol (called from protocol destructor)
	//
	//  IcmpError() is only intended to be used from the
	//	IcmpError() method of the IP protocol instance. The
	//	packet in the aHead is in "unpacked" state, and it
	//	begins with the returned IP header.
	//	Returns
	//	> 0,if packet has been released (not a normal
	//		situation, as this would prevent it reaching
	//		the upper layers)
	//	= 0,if ICMP noted and can be passed to the
	//		appropriate upper layer protocol
	//	> 0,NOT USED CURRENTLY! (treat as = 0)
	//	The first reason for this method is to get the
	//	path MTU mechanism implemented.
	//
	virtual TInt IcmpError(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo) = 0;
	virtual TInt IcmpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo) = 0;
#ifdef ARP
	virtual TInt ArpHandler(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo) = 0;
#endif
	virtual TInt IsForMePacket(RMBufRecvInfo &aInfo) const = 0;
	virtual void SetTimer(RTimeout &aHandle, TUint32 aDelay) = 0;
	virtual TInt GetIniValue(const TDesC &aSection, const TDesC &aName, TInt aDefault = 0, TInt aMin = 0, TInt aMax = 1) = 0;
	};


static const TInt KLoopbackMcastMetric = 0xffff;
    //< Metric set for multicast routes on loopback interface

#ifdef _LOG
// Internal help function for logging only
//extern void PktLog(const TDesC &aFormat, const RMBufPktInfo &aInfo, const TDesC &aName);
extern void PktLog(const TDesC &aFormat, const RMBufPktInfo &aInfo, TUint aIndex, const TDesC &aName);
#endif

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC-5006 Changes
typedef TUint32 TRdnssLifetime;

class TRdnssOptionData
//Class to read RDNSS option data from TInet6OptionICMP_DnsInformationV1
/*
@internalTechnology
*/
    {
public:
    
    TRdnssLifetime iStoredRdnssLifeTime; // (life time + system time) at which iRDNSS address becomes invalid   
    TInetAddr iRDNSSaddress;             //128 bit DNS address
    };


class TRdnssSortData
//Class to Sort RDNSS Server List data
/*
@internalTechnology
*/
    {
public:    
    TRdnssLifetime iStoredRdnssLifeTime; // (life time + system time) at which iRDNSS address becomes invalid   
    TInt iRdnssServerListIndex;          //index corresponds to iRdnssArrayList entries
    };

//Class to Manage RDNSS option data from TInet6OptionICMP_DnsInformationV1
class CManageRdnssServerList: public CBase
/*
@ internalTechnology
*/
    {
public:
    static CManageRdnssServerList *NewL();
    ~CManageRdnssServerList();
    
public:
    // Functions to Process RDNSS Option and Manage RDNSS server List
    void RdnssProcessOptionData(TInet6OptionICMP_DnsInformationV1 aRdnssOption, TUint8 aNumRdnssAddr);
    TBool RdnssParseOptionHdr(TInet6OptionICMP_DnsInformationV1 aRdnssOption, TUint8& aNumRdnssAddr );  
    void RdnssServerListUpdate(TInet6OptionICMP_DnsInformationV1 aRdnssIcmpOption, TUint8 aNumRdnssAddr);
    TBool RdnssServerListSync(TInetAddr& aNameSer1, TInetAddr& aNameSer2);
    
    // Functions to syncrhonise NameServer Entries
    void RdnssNameServerUpdate(TInetAddr& aNameSer, TUint8 aNameSerIndex);
    void RdnssNameServerSync(TInt aRdnssIndex, TInetAddr& aNameSer1, TInetAddr& aNameSer2 ); 
    void RdnssNameServerReset(TInetAddr& aNameSer, TInt& aDdnsflag);
        
    //RArray Wrapper Functions
    inline TInt CountRdnssEntry() {  return iRdnssArrayList.Count(); }
    inline TInt CountRdnssLifetimeEntry() { return iRdnssLifetimeArrayList.Count(); }
    TBool InsertRdnssEntryL(TRdnssOptionData& aRdnssEntry, TInt aIndex);    
    void DeleteRdnssEntry(TInt aRdnssArrayIndex);
    void DeleteRdnssLifetimeEntry(TInt aRdnssArrayIndex);
    
    //Miscallaneous Utitility Functions
    TRdnssOptionData& GetRdnssEntryRef(TInt aIndex);
    TRdnssSortData& GetRdnssLifetimeEntryRef(TInt aIndex);
    TBool RdnssEntryExists(const TIp6Addr& aRdnssAddr, TInt& aIndex);   
    TInt& GetRdnssFlag();
    void SetStoredLifeTime(TInt aIndex, TRdnssOptionData aRdnssData);    
    void RdnssExpireLeastEntry(TRdnssOptionData aRdnssRcvData);
    void RdnssServerListDelete();
    void RdnssServerListSort();
    void RdnssServerListCopyL();    
    void PrintRdnssServerList(TUint8 aIndex);
    void PrintRdnssLifetimeList(TUint8 aIndex);    
    void RdnssLifetimeListDelete();
    
    // Return the number of seconds between the given time (aStamp) and
    // the time when interface was activated (iTimeStamp). aStamp must
    // always be same of after iTimeStamp.      
    TRdnssLifetime Elapsed(const TTime &aStamp) const;
    TRdnssLifetime GetRemainingLifeTime(TInt aRdnssEntryindex);
    TRdnssLifetime ElapsedLifeTime(TRdnssOptionData aRdnssEntry);
    
private:
    CManageRdnssServerList();
    void ConstructL();                            // Two Phase construction
    
private:    
    TTime iCurrentTimeStamp;                      // TimeStamp at which RDNSS data is stored
    TInt  iRouterLifeTime;                        // Router lifetime
    TInt  iRdnssFlag;                              // RDNSS Flag associated with Resolver repository1 and Repository2
    RArray<TRdnssOptionData> iRdnssArrayList;     // RDNSS Server List [index 0 set to iNameserver1, index 1 set to iNameServer2]
    RArray<TRdnssSortData>   iRdnssLifetimeArrayList;// RDNSS Dummy Server List for sorting Lifetime
 };
#endif // SYMBIAN_TCPIPDHCP_UPDATE

#endif
