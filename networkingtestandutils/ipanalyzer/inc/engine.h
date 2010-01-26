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
// engine.h - protocol analyzer engine header
// The monitoring of IPv6 header includes some recursivity when 
// calling to monitor the next packet but it shouldn't be a problem
// However, in case of problrems it could easily turn into a loop
//



/**
 @internalComponent
*/

#ifndef __ENGINE_H
#define __ENGINE_H

#include <es_sock.h>
#include <in_sock.h>

#if EPOC_SDK >= 0x07010000
// here would go the includes that you would
// need for opening a link or links
#elif EPOC_SDK >= 0x06010000
#include <agentclient.h>
#include <connectprog.h>
#else
#include <netdial.h>
#endif

#define SECOND 1000000

#define MIN_FACTOR 0.05
#define MAX_FACTOR 1
#define STEP	0.05
#define MINI_STEP	0.01

#define	KProtocolInetEsp	50	//Protocols numbers. Not defined in any system include
#define	KProtocolInetAh    51

#define IPV4_ADDR_SIZE	16	//IPV4 size in char
#define IPV6_ADDR_SIZE	40	//IPV6 size in char

//Protocols no defined in .h's
#define KIPv6HopByHopHdr	0
#define KIPv6RoutingHdr	43
#define KIPv6FragmentHdr	44
#define	KIPv6ESPHdr	50
#define	KIPv6AuthenticationHdr	51
#define KIPv6DestOptHdr	60
#define KIPv6NoNextHdr	59

//ICMP codes not defined in .h's
#define KInet6ICMP_HopLimitExceeded		0
#define KInet6ICMP_FragReassExceeded	1

#define KInet6ICMP_NoRoute				0
#define KInet6ICMP_AdminProhibition		1
#define KInet6ICMP_NotNeighbour			2
#define KInet6ICMP_AddrUnreach			3
#define KInet6ICMP_PortUnreach			4

#define KInet6ICMP_ErrHdrField			0
#define KInet6ICMP_NextHdrUnknown		1
#define KInet6ICMP_OptionUnkown			2

#define MAX_READ_DATA	512	


struct TPreferences
{
	TBool iDumpIPv4;	//To dump dropped IPv4 packets
	TBool iDumpIPv6;	//To dump dropped IPv6 packets
	TBool iDumpIPSEC;	//To dump dropped ipsec packet
	TInt iProtocol;	//icmp, ip, tcp, udp
	TUint iPort;	//To be used with tcp or udp
	TBool iViewIPHdr;
	TInt iNumBlades;
};

#if 0
struct SMonIPv4Info 
{
	TBool iIPVersion;
	TBool iIPHdrLen;
	TBool iIPTOS;
	TBool iIPTotalLen;
	TBool iIPId;
	TBool iIPFlags;
	TBool iIPOffset;
	TBool iIPTTL;
	TBool iIPProtocol;
	TBool iIPChksum;
	TBool iIPSrcAddr;
	TBool iIPDstAddr;

	TBool iICMPType;
	TBool iICMPCode;
	TBool iICMPChksum;

	TBool iTCPSrcPort;
	TBool iTCPDstPort;
	TBool iTCPSeq;
	TBool iTCPAckNum;
	TBool iTCPHdrLen;
	TBool iTCPFlags;
	TBool iTCPHdrWinSize;
	TBool iTCPChksum;
	TBool iTCPHdrUrgPtr;

	TBool iUDPSrcPort;
	TBool iUDPDstPort;
	TBool iUDPLen;
	TBool iUDPChksum;

	TBool iAHProtocol;
	TBool iAHHdrLen;
	TBool iAHSPI;
	TBool iAHSeq;

	TBool iESPSPI;
	TBool iESPSeq;

};
#else
#	define	SMonIPv4Info SMonIpInfo
#	define	SMonIPv6Info SMonIpInfo
#endif
struct SMonIpInfo 
{

	TBool iIPVersion;
	union
		{
		TBool iIPTraffic;	// IPv6
		TBool iIPTOS;		// IPv4
		};
	TBool iIPFlowLabel;
	union
		{
		TBool iIPTotalLen;	// IPv4
		TBool iIPPayloadLen;// IPv6
		};
	union
		{
		TBool iIPNextHdr;	// IPv6
		TBool iIPProtocol;	// IPv4
		};
	union
		{
		TBool iIPHopLimit;	// IPv6
		TBool iIPTTL;		// IPv4
		};

	TBool iIPSrcAddr;
	TBool iIPDstAddr;

	TBool iIPHdrLen;	// IPv4
	TBool iIPId;		// IPv4
	TBool iIPFlags;		// IPv4
	TBool iIPOffset;	// IPv4
	TBool iIPChksum;	// IPv4

	TBool iICMPType;
	TBool iICMPCode;
	TBool iICMPChksum;
	TBool iICMPParameter;	//Particular Info depending on ICMP Code and Type

	TBool iTCPSrcPort;
	TBool iTCPDstPort;
	TBool iTCPSeq;
	TBool iTCPAckNum;
	TBool iTCPHdrLen;
	TBool iTCPFlags;
	TBool iTCPHdrWinSize;
	TBool iTCPChksum;
	TBool iTCPHdrUrgPtr;
	TBool iTCPOptions;

	TBool iUDPSrcPort;
	TBool iUDPDstPort;
	TBool iUDPLen;
	TBool iUDPChksum;

	TBool iHOPNextHdr;
	TBool iHOPHdrExtLen;
	TBool iHOPOptionType;
	TBool iHOPOptionLen;

	TBool iDSTNextHdr;
	TBool iDSTHdrExtLen;
	TBool iDSTHomeAddr;
	TBool iDSTBindingUpdate;
	TBool iDSTBindingRequest;
	TBool iDSTBindingAck;
	TBool iDSTPad;
	TBool iDSTUnknown;

	TBool iRTNextHdr;
	TBool iRTHdrExtLen;
	TBool iRTRoutingType;
	TBool iRTSegLeft;
	TBool iRTSLBitMap;
	TBool iRTAddresses;

	TBool iFRAGNextHdr;
	TBool iFRAGFragOffset;
	TBool iFRAGMFlag;
	TBool iFRAGId;

	TBool iAHProtocol;
	TBool iAHHdrLen;
	TBool iAHSPI;
	TBool iAHSeq;

	TBool iESPSPI;
	TBool iESPSeq;

};


struct SMonStatInfo 
{
	TUint iTotalPackets;
	TUint iIPv4Packets;
	TUint iIPv6Packets;
	TUint iTCPPackets;
	TUint iUDPPackets;
	TUint iICMPPackets;
	TUint iExtPackets;
};

class CRotorAppView;
class CRotorReceiver;
class CRotorIPv6Receiver;
class CRotorDumper;
class CRotorBraker;

class CRotorEngine : public CTimer
{
public:
	//constructor
	CRotorEngine(CRotorAppView *aAppView);

	//destructor
	~CRotorEngine();
	
	//second phase constructor
	void ConstructL(const TPreferences& aPref);
	void GetPreferences(TPreferences &aPref) const;
	static void DefaultPreferences(TPreferences &aPref);
	CRotorReceiver *StartReceiver(const TDesC &aName);
	void FirstRun();
	void StartL();
	void Stop();
	void IncreaseSpeed();
	void DecreaseSpeedL();
	void PacketReceived();

	inline SMonIPv4Info *MonIPv4Info()
		{return &iMonIPv4Info;}
	inline SMonIPv6Info *MonIPv6Info()
		{return &iMonIPv6Info;}
	inline RSocketServ &SocketServ()
	{return iSockServ;}

protected:
	//void UpdateRotor();
	void UpdateSpeedL();

	//Issues next RunL execution
	void IssueRequest();

	// will send all the packets
	void RunL();

	//Cancel Packet Sending
	void DoCancel();

private:
	void SetNetworkUp() const;

public:
	// General option. Public to make it easier to access from the dialog
	TPreferences iPref;

	TBool iProbeActive;	// IP Probe Hook availabe
	TBool iIPv4Active;	//IPv4 packets dump available
	TBool iIPv6Active;	//IPv6 packets dump available
	TBool iIPSECActive;	//IPSEC packets dump available
	SMonStatInfo iStatInfo;	
	TBool iShowPackets;	//Tells the engine to avoid using the console
	CRotorAppView *iAppView;

private:
	RSocketServ iSockServ;
	CRotorReceiver *iReceiver;		//AO listening IPv4
	CRotorReceiver *iIPv6Receiver;	//AO listening IPv6
	CRotorReceiver *iDumper;		//AO listening IPSEC socket
	CRotorBraker *iBraker;			//AO that brakes the rotor
	TReal iFactor;
	TBool iPacketReceived;
	//TUint iRecvPackets;
	TReal iPartialPackets;
	TTime iInitTime;
	SMonIPv4Info iMonIPv4Info;
	SMonIPv6Info iMonIPv6Info;
	
	//TBool iRunning;

#if EPOC_SDK >= 0x07010000
	// Here would go the member variable you would use
	// to open a link or links
#elif EPOC_SDK >= 0x06010000
	RGenericAgent xnetdial;	// Use GenConn for R6.1+
#else
	RNetDial xnetdial;	// To set the net UP
#endif
};



class CRotorListener : public CActive
{
public:
	//constructor
	CRotorListener(CRotorAppView *aAppView, CRotorEngine *aEngine);

	//destructor
	~CRotorListener();
	
	//second phase constructor
	void ConstructL();
	void FirstRun();
	void Start();
	

protected:
	//void UpdateRotor();

	//Issues next RunL execution
	void IssueRequest();


	// will send all the packets
	//void RunL();

	void Stop() const;

	//Cancel Packet Sending
	void DoCancel();

	TBool Filter(const TDesC8 &aPacket) const;

	void ViewData(const TUint8 *aData,TUint aSize);

	void MonitorIpPacket(const TDesC8 &aPacket);

	TUint MonitorIPv4(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorIPv6(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorICMPv4(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorICMPv6(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint ICMPv4Specific(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint ICMPv6Specific(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);	
	TUint MonitorTCP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorUDP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorESP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorAH(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorHopByHopHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorDestOptHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorRoutingHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	TUint MonitorFragmentHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext);
	
	//aPacket contains the full packet
	const TUint8 *FindTransportProtocol(const TDesC8& aPacket) const;
	void ErrorL(const TDesC& string,TInt error);

protected:
	CRotorAppView *const iAppView;
	CRotorEngine *const iEngine;
	//RSocketServ iSockServ;
	RSocket iSocket;
	HBufC8* iReceivedData;
	TPtr8 iBuf;
	TInt iErrPackets;
	TRequestStatus iWriteStatus;
};


//
//	CRotorReceiver: Receives IPv4 packets to move the rotor
//


class CRotorReceiver : public CRotorListener
{
friend class CRotorDumper; //To let this class use the monitoring functions
public:
	//constructor
	CRotorReceiver(CRotorAppView *aAppView, CRotorEngine *aEngine);
	
	//second phase constructor
	void ConstructL(TUint aProtocol, const TDesC &aName);

protected:
	void RunL();
private:
	TInt iSwapIpHeader;
};

class CRotorBraker : public CTimer
{
public:
	//constructor
	CRotorBraker(CRotorEngine *aEngine);

	//destructor
	~CRotorBraker();

	//second phase constructor
	void ConstructL();
	
	void Start();
	void ReIssueRequest();

protected:
	
	//Issues next RunL execution
	void IssueRequest();

	void FirstRun();

	// will send all the packets
	void RunL();

	void Stop() const;

	//Cancel Packet Sending
	void DoCancel();

private:
	CRotorEngine *iEngine;
};

#endif
