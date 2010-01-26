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
// engine.cpp - protocol analyzer engine
//

#include <es_sock.h>
#include <e32base.h>

#include <nifmbuf.h>

#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <icmp6_hdr.h>
#include <tcp_hdr.h>
#include <udp_hdr.h>
#include <ext_hdr.h>

#include "iprotor.h"
#include "engine.h"

CRotorEngine::CRotorEngine(CRotorAppView *aAppView):CTimer(EPriorityStandard)
{
	iAppView = aAppView;
	CActiveScheduler::Add(this);
}


CRotorEngine::~CRotorEngine()
{
	if (IsActive())
		Cancel();

	if (iReceiver)
		delete iReceiver;
	if (iIPv6Receiver)
		delete iIPv6Receiver;
	if (iDumper)
		delete iDumper;
	if (iBraker)
		delete iBraker;

	iSockServ.Close();
#if EPOC_SDK >= 0x07010000
	// Here would go the functionality that you would need
	// to close the opened link or links
#else
	xnetdial.Stop();
	xnetdial.Close();
#endif
}


void CRotorEngine::ConstructL(const TPreferences& aPref)
{
	//Base class 2nd phase constructor
	CTimer::ConstructL();

	iShowPackets=EFalse;	//Tells the engine to avoid using the console

	//General Options
	iPref = aPref;

	//iProtocol=IP;	//numbered like the list in options dialog
	//iNumBlades=DEFAULT_NUM_BLADES;
	iFactor=1;
	iPacketReceived=EFalse;
//	iViewIPHdr=ETrue;
	
#if EPOC_SDK >= 0x07010000
	// Here would go the functionality that you would need
	// to open a link or links
#else
	if (xnetdial.Open()!=KErrNone)
	{
		iAppView->Status(R_ROTOR_NET_OPEN_ERROR);	
	}
	else
	{
		if (xnetdial.DisableTimers()!=KErrNone)
			iAppView->Status(R_ROTOR_NET_DISABLE_ERROR);
	}
#endif

	TInt err = iSockServ.Connect();	//KESockDefaultMessageSlots
	TProtocolDesc protInfo;
	if (err != KErrNone)
	{
		iAppView->Write(_L("Socket Server Error (Connect)"));
		User::Leave(err);
	}

	err = iSockServ.FindProtocol(_L("probe"), protInfo);
	if (err!=KErrNone)
		{
		iAppView->Write(_L("Protocol Probe (hook) Not available"));
		iProbeActive = EFalse;
		}
	else
		iProbeActive = ETrue;

	err = iSockServ.FindProtocol(_L("ip"),protInfo);
	if (err!=KErrNone)
	{
		iAppView->Write(_L("Protocol IPv4 Not available"));
		iIPv4Active = EFalse;
		iPref.iDumpIPv4 = EFalse;
	}
	else
	{
		//iPref.iDumpIPv4 = ETrue;
		iIPv4Active = ETrue;
	}
	err = iSockServ.FindProtocol(_L("ip6"),protInfo);
	if (err!=KErrNone)
	{
		iAppView->Write(_L("Protocol IPv6 Not available"));
		iIPv6Active = EFalse;
		iPref.iDumpIPv6 = EFalse;
	}
	else
	{
		//iPref.iDumpIPv6 = ETrue;
		iIPv6Active = ETrue;
	}
	err = iSockServ.FindProtocol(_L("secpol"),protInfo);
	if (err!=KErrNone)
	{
		iAppView->Write(_L("Protocol IPSEC Not available"));
		iIPSECActive = EFalse;
		iPref.iDumpIPSEC = EFalse;
	}
	else
	{
		//iPref.iDumpIPSEC = ETrue;
		iIPSECActive = ETrue;
	}

	//IPv4 Monitoring data
	//IP
	iMonIPv4Info.iIPVersion=ETrue;
	iMonIPv4Info.iIPHdrLen=ETrue;
	iMonIPv4Info.iIPTOS=ETrue;
	iMonIPv4Info.iIPTotalLen=ETrue;
	iMonIPv4Info.iIPId=ETrue;
	iMonIPv4Info.iIPFlags=ETrue;
	iMonIPv4Info.iIPOffset=ETrue;
	iMonIPv4Info.iIPTTL=ETrue;
	iMonIPv4Info.iIPProtocol=ETrue;
	iMonIPv4Info.iIPChksum=ETrue;
	iMonIPv4Info.iIPSrcAddr=ETrue;
	iMonIPv4Info.iIPDstAddr=ETrue;

	//ICMP
	iMonIPv4Info.iICMPType=ETrue;
	iMonIPv4Info.iICMPCode=ETrue;
	iMonIPv4Info.iICMPChksum=ETrue;

	//TCP
	iMonIPv4Info.iTCPSrcPort=ETrue;
	iMonIPv4Info.iTCPDstPort=ETrue;
	iMonIPv4Info.iTCPSeq=ETrue;
	iMonIPv4Info.iTCPAckNum=ETrue;
	iMonIPv4Info.iTCPHdrLen=ETrue;
	iMonIPv4Info.iTCPFlags=ETrue;
	iMonIPv4Info.iTCPHdrWinSize=ETrue;
	iMonIPv4Info.iTCPChksum=ETrue;
	iMonIPv4Info.iTCPHdrUrgPtr=ETrue;

	//UDP
	iMonIPv4Info.iUDPSrcPort=ETrue;
	iMonIPv4Info.iUDPDstPort=ETrue;
	iMonIPv4Info.iUDPLen=ETrue;
	iMonIPv4Info.iUDPChksum=ETrue;

	//AH
	iMonIPv4Info.iAHProtocol=ETrue;
	iMonIPv4Info.iAHHdrLen=ETrue;
	iMonIPv4Info.iAHSPI=ETrue;
	iMonIPv4Info.iAHSeq=ETrue;

	//ESP
	iMonIPv4Info.iESPSPI=ETrue;
	iMonIPv4Info.iESPSeq=ETrue;


	//IPv6 Monitoring data
	//IP
	
	iMonIPv6Info.iIPVersion=ETrue;
	iMonIPv6Info.iIPTraffic=ETrue;
	iMonIPv6Info.iIPFlowLabel=ETrue;
	iMonIPv6Info.iIPPayloadLen=ETrue;
	iMonIPv6Info.iIPNextHdr=ETrue;
	iMonIPv6Info.iIPHopLimit=ETrue;
	iMonIPv6Info.iIPSrcAddr=ETrue;
	iMonIPv6Info.iIPDstAddr=ETrue;

	//ICMP
	iMonIPv6Info.iICMPType=ETrue;
	iMonIPv6Info.iICMPCode=ETrue;
	iMonIPv6Info.iICMPChksum=ETrue;
	iMonIPv6Info.iICMPParameter=ETrue;

	//TCP
	iMonIPv6Info.iTCPSrcPort=ETrue;
	iMonIPv6Info.iTCPDstPort=ETrue;
	iMonIPv6Info.iTCPSeq=ETrue;
	iMonIPv6Info.iTCPAckNum=ETrue;
	iMonIPv6Info.iTCPHdrLen=ETrue;
	iMonIPv6Info.iTCPFlags=ETrue;
	iMonIPv6Info.iTCPHdrWinSize=ETrue;
	iMonIPv6Info.iTCPChksum=ETrue;
	iMonIPv6Info.iTCPHdrUrgPtr=ETrue;
	iMonIPv6Info.iTCPOptions=ETrue;

	//UDP
	iMonIPv6Info.iUDPSrcPort=ETrue;
	iMonIPv6Info.iUDPDstPort=ETrue;
	iMonIPv6Info.iUDPLen=ETrue;
	iMonIPv6Info.iUDPChksum=ETrue;

	//HopByHop Hdr
	iMonIPv6Info.iHOPNextHdr=ETrue;
	iMonIPv6Info.iHOPHdrExtLen=ETrue;
	iMonIPv6Info.iHOPOptionType=ETrue;
	iMonIPv6Info.iHOPOptionLen=ETrue;

	//Dest Opt Hdr
	iMonIPv6Info.iDSTNextHdr=ETrue;
	iMonIPv6Info.iDSTHdrExtLen=ETrue;
	iMonIPv6Info.iDSTHomeAddr=ETrue;
	iMonIPv6Info.iDSTBindingUpdate=ETrue;
	iMonIPv6Info.iDSTBindingRequest=ETrue;
	iMonIPv6Info.iDSTBindingAck=ETrue;
	iMonIPv6Info.iDSTPad=ETrue;
	iMonIPv6Info.iDSTUnknown=ETrue;

	//Routing Header
	iMonIPv6Info.iRTNextHdr=ETrue;
	iMonIPv6Info.iRTHdrExtLen=ETrue;
	iMonIPv6Info.iRTRoutingType=ETrue;
	iMonIPv6Info.iRTSegLeft=ETrue;
	iMonIPv6Info.iRTSLBitMap=ETrue;
	iMonIPv6Info.iRTAddresses=ETrue;

	//Fragment Hdr
	iMonIPv6Info.iFRAGNextHdr=ETrue;
	iMonIPv6Info.iFRAGFragOffset=ETrue;
	iMonIPv6Info.iFRAGMFlag=ETrue;
	iMonIPv6Info.iFRAGId=ETrue;

	//AH
	iMonIPv6Info.iAHProtocol=ETrue;
	iMonIPv6Info.iAHHdrLen=ETrue;
	iMonIPv6Info.iAHSPI=ETrue;
	iMonIPv6Info.iAHSeq=ETrue;

	//ESP
	iMonIPv6Info.iESPSPI=ETrue;
	iMonIPv6Info.iESPSeq=ETrue;

	//iRunning=EFalse;

}

void CRotorEngine::GetPreferences(TPreferences &aPref) const
{
	aPref = iPref;
}

void CRotorEngine::DefaultPreferences(TPreferences &aPref)
{
	aPref.iDumpIPv4 = ETrue;
	aPref.iDumpIPv6 = EFalse;
	aPref.iDumpIPSEC = EFalse;
	aPref.iProtocol = IP;	//numbered like the list in options dialog
	aPref.iPort = 0;	//Any port
	aPref.iViewIPHdr = ETrue;
	aPref.iNumBlades = DEFAULT_NUM_BLADES;
}

//Issues next RunL execution
void CRotorEngine::IssueRequest()
{
	if (iFactor<0)
		iFactor=0.5;
	//After((TInt)(iFactor*SECOND));	//Also sets the object as Active
	//TInt time=iFactor*SECOND;
	//TTimeIntervalMicroSeconds32 interval(time);
	After((TInt)iFactor*SECOND);	//Also sets the object as Active
}

/*
//Issues last RunL execution
void CPingSender::IssueLastRequest()
{
	After(iPingModel->iLastSecWait*SECOND);	//Also sets the object as Active
}
*/


void CRotorEngine::PacketReceived()
{
	iPacketReceived=ETrue;
	//iRecvPackets++;
	iStatInfo.iTotalPackets++;
	iPartialPackets++;		//Used only for speed calculation. Is reset when Rotor is not moving.
	IncreaseSpeed();
	iBraker->ReIssueRequest();

}



void CRotorEngine::FirstRun()
{
	iFactor=MAX_FACTOR;		//Factor reset
	After(1);		//Also sets the object as Active.
	if (!IsActive())
		SetActive();
	//IssueRequest();	//First RunL
	
}

// will send all the packets. One packet each Time
void CRotorEngine::RunL()
{
	/*
	if (!iReceiver)
		iAppView->Write(_L("-iReceiver DEAD"));

	if (!iBraker)
		iAppView->Write(_L("-iBraker DEAD"));

	if (!iBraker->IsActive())
		iAppView->Write(_L("-iBraker NOT ACTIVE"));

	if (!iBraker->IsAdded())
		iAppView->Write(_L("-iBraker NOT ADDED"));


	if (iBraker->iStatus!=KRequestPending)
		iAppView->Write(_L("-iBraker NOT Pending"));


	if (!iReceiver->IsActive())
		iAppView->Write(_L("-iReceiver NOT ACTIVE"));

	if (!iReceiver->IsAdded())
		iAppView->Write(_L("-iReceiver NOT ADDED"));
*/
	/*
	if (iReceiver->iStatus!=KRequestPending)
		iAppView->Write(_L("-iReceiver NOT Pending"));
	*/

	if (iFactor<MAX_FACTOR)
	{
		iAppView->UpdateRotor();
		UpdateSpeedL();
	}
	IssueRequest();	//First RunL
}


//Cancel Packet Sending
void CRotorEngine::DoCancel()
{
	CTimer::DoCancel();
	//Deque();	//Dequeues the active object of the Scheduler
}


void CRotorEngine::SetNetworkUp() const
{
#if EPOC_SDK >= 0x07010000
	// Here would go the functionality you would
	// need to open a link or links
#else
	TBuf<50> msg;
	TNifProgress progress;
	xnetdial.Progress(progress);

	TInt err=progress.iError;
	if (err!=KErrNone)
	{
		msg.Format(_L("Network error: "));
		iAppView->ShowError(msg, err);
		iAppView->UpdateNetwork(EFalse);
		Stop();
		return;
	}

	
	if (progress.iStage!=EConnectionOpen)	//If net not UP
	{
#if EPOC_SDK >= 0x06010000
		err = xnetdial.StartOutgoing();
		xnetdial.DisableTimers();
#elif EPOC_SDK
		err=xnetdial.StartDialOut();
#else
		err=xnetdial.StartDialOut();
#endif
		if ((err!=KErrNone) && (err!=KErrAlreadyExists))
		{
			msg.Format(_L("Network error: "));
			iAppView->ShowError(msg, err);
			iAppView->UpdateNetwork(EFalse);
			Stop();
			return;
		}
	}
	else
		iAppView->UpdateNetwork(ETrue);
#endif
}


CRotorReceiver *CRotorEngine::StartReceiver(const TDesC &aName)
	{
	CRotorReceiver *receiver = new CRotorReceiver(iAppView, this);	//Receives incoming packets
	if (receiver == NULL)
		return NULL;
	TRAPD(err, receiver->ConstructL(iPref.iProtocol, aName));
	if (err != KErrNone)
		{
		delete receiver;
		return NULL;
		}
	receiver->Start();
	return receiver;
	}

void CRotorEngine::StartL()
{

	iStatInfo.iTotalPackets=0;
	iStatInfo.iIPv4Packets=0;
	iStatInfo.iIPv6Packets=0;
	iStatInfo.iTCPPackets=0;
	iStatInfo.iUDPPackets=0;
	iStatInfo.iICMPPackets=0;
	iStatInfo.iExtPackets=0;
	

	FirstRun();
	SetNetworkUp();
	TInt err;

	if (iPref.iDumpIPv4 || iPref.iDumpIPv6)
	{
		if (iProbeActive)
			iReceiver = StartReceiver(_L("probe"));
		// If probe is not available, start ip6 and ip on raw sockets...
		if (!iReceiver)
			{
			if (iPref.iDumpIPv6)
				iIPv6Receiver = StartReceiver(_L("ip6"));
			if (iPref.iDumpIPv4)
				iReceiver = StartReceiver(_L("ip"));
			}
		if (iReceiver == NULL && iIPv6Receiver == NULL)
			{
			iPref.iDumpIPv4 = EFalse;
			iPref.iDumpIPv6 = EFalse;
			}
	}

	if (iPref.iDumpIPSEC)
	{
		iDumper= new (ELeave) CRotorReceiver(iAppView,this);	//Receives incoming packets
		TRAP(err,iDumper->ConstructL(iPref.iProtocol, _L("secpol")));
		if (err==KErrNone)
			iDumper->Start();
		else
		{
			delete iDumper;
			iDumper = NULL;
			iPref.iDumpIPSEC = EFalse;
		}

	}

	if (!iPref.iDumpIPv4 && !iPref.iDumpIPv6 && !iPref.iDumpIPSEC)
	{
		//iRunning = EFalse;
		iAppView->Write(_L("No traffic to Dump! (Use Options to Set it)"));
		User::Leave(1);
		// NOTREACHED
		return;
	}
	iBraker= new (ELeave) CRotorBraker(this);	//Brakes the rotor gradually
	iBraker->ConstructL();
	iBraker->Start();

	//iRunning=ETrue;
}

void CRotorEngine::Stop()
{
	iFactor=1;
	//iRecvPackets=0;
	iStatInfo.iTotalPackets=0;
	if (IsActive())
		Cancel();
	
	//Stops hearing from all sockets
	delete iReceiver;
	iReceiver=NULL;
	delete iIPv6Receiver;
	iIPv6Receiver=NULL;
	delete iDumper;
	iDumper=NULL;
	delete iBraker;
	iBraker=NULL;

	//iRunning=EFalse;	
}

void CRotorEngine::IncreaseSpeed()
{
	if (iFactor>=MAX_FACTOR)
	{
		iInitTime.UniversalTime();	//Sets the time for the First packet
		iPartialPackets=0;
	}
	if (iFactor>MIN_FACTOR)
		iFactor-=STEP;
}

void CRotorEngine::DecreaseSpeedL()
{
	//TBuf<50> aux;
	if (iFactor<MAX_FACTOR)
	{
		iFactor+=2*STEP;
		//aux.Format(_L("Decreasing speed: %.3f\n"),iFactor);
	}
	else
	{
		iAppView->UpdateSpeedL(0.0);
		iFactor=1;
	}
	//iAppView->Write(aux);
}

//Calculates and write the speed
void CRotorEngine::UpdateSpeedL()
{
	/*
	if (iRecvPackets==0)
	{	//To measure the speed
		iInitTime.UniversalTime();	//Sets the time for the First packet
	}
	*/
	TTime now;
	now.UniversalTime();

	TTimeIntervalMicroSeconds interval;
	interval=now.MicroSecondsFrom(iInitTime);	//time in us.
	TInt64 num=interval.Int64();
	num = num / SECOND;	//in seconds
#ifdef I64LOW
	TReal realtime=I64LOW(num);
#else
	TReal realtime=num.Low();
#endif
	if (realtime>0)
		iAppView->UpdateSpeedL(iPartialPackets/realtime);
	else
		iAppView->UpdateSpeedL(0.0);
}



//
//	CRotorListener	waits to receive packets from the socket
//


//constructor
CRotorListener::CRotorListener(CRotorAppView *aAppView,CRotorEngine *aEngine)
 : CActive(EPriorityStandard), iAppView(aAppView), iEngine(aEngine), iBuf(0,0)
{
	CActiveScheduler::Add(this);
}

void CRotorListener::ConstructL()
{
	iReceivedData = HBufC8::NewL(MAX_READ_DATA);
	
	TPtr8 aux(iReceivedData->Des());
	iBuf.Set(aux);
}
//destructor
CRotorListener::~CRotorListener()
{
	//Stop();	//In case is still running
	if (IsActive())
		Cancel();
	iSocket.Close();
	delete iReceivedData;

}
	

void CRotorListener::FirstRun()
{
	iSocket.Recv(iBuf, 0, iStatus);
	IssueRequest();	//Prepares to receive it in RunL()
}


void CRotorListener::Start()
{
	FirstRun();
}

void CRotorListener::Stop() const
{
	/*
	if (IsAdded())
		Deque();	//Dequeues the active object of the Scheduler. Calls DoCancel()	
	*/
}

//Issues next RunL execution
void CRotorListener::IssueRequest()
{
	if (!IsActive())
		SetActive();	//Sets the object as Active.
						//RunL will be executed when iStatus!=KRequestPending
}


//Cancel Packet Sending

void CRotorListener::DoCancel()
{
	iStatus=KErrNone;
	//iSocket.CancelRecv();	//Cancels an outstanding request
}

//aPacket contains the full packet
const TUint8 *CRotorListener::FindTransportProtocol(const TDesC8& aPacket) const
{
	if (aPacket.Length() < TInet6HeaderIP::MinHeaderLength())
		return NULL;
	TInet6HeaderIP* const hdr = (TInet6HeaderIP*)aPacket.Ptr(); //lint !e826 // the length is checked above

	TBool end=EFalse;
	TInt nextHdr = 0;
	const TUint8 *packet = NULL;
	if (hdr->Version() == 4)
		{
		// IPv4 packet
		nextHdr = ((TInet6HeaderIP4 *)hdr)->Protocol();
		packet = ((TInet6HeaderIP4 *)hdr)->EndPtr();
		if (((TInet6HeaderIP4 *)hdr)->FragmentOffset() != 0)
			return NULL;	// Only first fragment can have transport header
		}
	else if (hdr->Version() == 6)
		{
		// IPv6 packet
		nextHdr = hdr->NextHeader();
		packet = hdr->EndPtr();
		}
	else
		{
		// Neither IPv4 nor IPv6 => No transport protocol either!
		return NULL;
		}
	while (!end && (packet < aPacket.Ptr() + aPacket.Length()))
	{
		switch (nextHdr)	//Next header field
		{
		case KProtocolInetTcp:
			if (aPacket.Length() - (packet - aPacket.Ptr()) < TInet6HeaderTCP::MinHeaderLength())
				return NULL;
			return packet;

		case KProtocolInetUdp:
			if (aPacket.Length() - (packet - aPacket.Ptr()) < TInet6HeaderUDP::MinHeaderLength())
				return NULL;
			return packet;

		case KIPv6NoNextHdr:		//No more headers. NO TCP or UDP found
			end=ETrue;
			break;

		//Next Header (Extension headers)
		case KIPv6FragmentHdr:	//Fragment Header (fix size = 8 bytes)
			if (aPacket.Length() - (packet - aPacket.Ptr()) < TInet6HeaderFragment::MinHeaderLength())
				return NULL;
			if (((TInet6HeaderFragment *)packet)->FragmentOffset() != 0) //lint !e826 // the length is checked above
				end = ETrue;	// Only the first fragment has transport hdr
			packet += 8;
			break;
		case KIPv6RoutingHdr:	//Routing Header size in 64-bit words
			packet += packet[1]*8 + 8;	//Length doesn't include first 64 bits
			break;
		case KIPv6AuthenticationHdr:	//Authentication Header size in 32-bit words
			packet += packet[1]*4 + 8;	//Payload length doesn't include first 64 bits
			break;
		default:
			end = ETrue;			// Unknown header, cannot locate transport!!
			break;
			//packet += packet[1] + 8;	//Next header + 2 because of the Next header and header length field
										//The length doesn't include the first 8 octets of data
		}
		nextHdr=packet[0];
	}
	return NULL;
}

//Full packet. Returns True is packet filtered (not show) EFalse otherwise
TBool CRotorListener::Filter(const TDesC8 &aPacket) const
{
	if (iEngine->iPref.iPort==KInetPortAny)
		return EFalse;	//Packet not filtered because any port

	const TUint8 *transHdr=FindTransportProtocol(aPacket);

	if (transHdr == NULL) 
		return EFalse;	//No transport header to be filtered. Try to show in case so not filtered
		
	//found
	if (iEngine->iPref.iProtocol==TCP)
	{
		const TInet6HeaderTCP *TCPhdr= (TInet6HeaderTCP*)transHdr; //lint !e826 // the length is checked in FindTransportProtocol
		if (TCPhdr->SrcPort()==iEngine->iPref.iPort)
			return EFalse;		//Monitored Port so not filtered
	}
	else	//UDP
		{
		const TInet6HeaderUDP *UDPhdr= (TInet6HeaderUDP*)transHdr; //lint !e826 // the length is checked in FindTransportProtocol
		if (UDPhdr->SrcPort()==iEngine->iPref.iPort)
			return EFalse;	//Monitored Port so not filtered
	}
	return ETrue;	//Not interesting so filtered
}

void CRotorListener::MonitorIpPacket(const TDesC8 &aPacket)
	{
	TPtrC8 pkt(aPacket);
	const TInet6HeaderIP *const hdr = (TInet6HeaderIP *)pkt.Ptr(); //lint !e826 // the length is checked in RunL
	const SMonIpInfo *info;
	TUint nexthdr = 0;
	TUint offset = 0;

	if (Filter(aPacket))
		return;

	if (hdr->Version() == 4)
		{
		info = iEngine->MonIPv4Info();
		nexthdr = KProtocolInetIpip;
		}
	else if (hdr->Version() == 6)
		{
		info = iEngine->MonIPv6Info();
		nexthdr = KProtocolInet6Ipip;
		}
	else
		{
		// Not an IP packet
		return;
		}

	iAppView->Write(_L("\n"));
	while ((TInt)offset < pkt.Length() && nexthdr)
		{
		pkt.Set(pkt.Right(pkt.Length() - offset));
		switch (nexthdr)
			{
		case KProtocolInetIpip:
			offset = MonitorIPv4(pkt, *info, nexthdr);
			break;
		case KProtocolInet6Ipip:
			offset = MonitorIPv6(pkt, *info, nexthdr);
			break;
		case KProtocolInet6Icmp:
			offset = MonitorICMPv6(pkt, *info, nexthdr);
			break;
		case KProtocolInetTcp:
			offset = MonitorTCP(pkt, *info, nexthdr);
			break;
		case KProtocolInetUdp:
			offset = MonitorUDP(pkt, *info, nexthdr);
			break;
		case KIPv6HopByHopHdr:
			offset = MonitorHopByHopHeader(pkt, *info, nexthdr);
			break;
		case KIPv6DestOptHdr:
			offset = MonitorDestOptHeader(pkt, *info, nexthdr);
			break;
		case KIPv6RoutingHdr:
			offset = MonitorRoutingHeader(pkt, *info, nexthdr);
			break;
		case KIPv6FragmentHdr:
			offset = MonitorFragmentHeader(pkt, *info, nexthdr);
			break;
		case KProtocolInetIcmp:
			offset = MonitorICMPv4(pkt, *info, nexthdr);
			break;
		case KProtocolInetEsp:
			offset = MonitorESP(pkt, *info, nexthdr);
			break;
		case KProtocolInetAh:
			offset = MonitorAH(pkt, *info, nexthdr);
			break;
		default:	//Others types are Unknown
			TBuf<80> buf;
			buf.Format(_L("--Unknown header (%d)--\n"), nexthdr);
			iAppView->Write(buf);
			nexthdr = 0;
			break;
			}
		}
	}


TUint CRotorListener::MonitorIPv4(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderIP4::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderIP4 *const hdr = (TInet6HeaderIP4 *)aPacket.Ptr(); //lint !e826 // the length is checked above
	
	iEngine->iStatInfo.iIPv4Packets++;

	if (iEngine->iShowPackets)
	{
		if (iEngine->iPref.iViewIPHdr)
		{
			TBuf<400> buf;
			TInetAddr addr;
			TBuf<IPV4_ADDR_SIZE> textaddr;

			buf.Format(_L("--IPv4--\n"));

			if (aMonitor.iIPVersion)
				buf.AppendFormat(_L("Version=%u  "), hdr->Version());

			if (aMonitor.iIPHdrLen)
				buf.AppendFormat(_L("HdrLen=%u  "), hdr->HeaderLength()); //already in bytes

			if (aMonitor.iIPTOS)
				buf.AppendFormat(_L("TOS=%u  "), hdr->TOS());

			if (aMonitor.iIPTotalLen)
				buf.AppendFormat(_L("Length=%u  "), hdr->TotalLength());

			if (aMonitor.iIPId)
				buf.AppendFormat(_L("id=%u  "), hdr->Identification());  

			if (aMonitor.iIPFlags)
				{
				buf.AppendFormat(_L("DF=%d  "), hdr->DF() != 0);  
				buf.AppendFormat(_L("MF=%d  "), hdr->MF() != 0);  
				}

			if (aMonitor.iIPOffset)
				buf.AppendFormat(_L("Offset=%u  "), hdr->FragmentOffset());

			if (aMonitor.iIPTTL)
				buf.AppendFormat(_L("TTL=%u  "), hdr->Ttl());

			if (aMonitor.iIPProtocol)
				buf.AppendFormat(_L("Protocol=%u  "), hdr->Protocol());

			if (aMonitor.iIPChksum)
				buf.AppendFormat(_L("Checksum=%u  "), hdr->Checksum());
	
			if (aMonitor.iIPSrcAddr)
				{
				addr.SetAddress(hdr->SrcAddr());
				addr.Output(textaddr);
				buf.AppendFormat(_L("Src="));
				buf.AppendFormat(textaddr);
				buf.AppendFormat(_L(" "));
				}

			if (aMonitor.iIPDstAddr)
				{
				addr.SetAddress(hdr->DstAddr());
				addr.Output(textaddr);
				buf.AppendFormat(_L("Dst="));
				buf.AppendFormat(textaddr);
				buf.AppendFormat(_L(" "));
				}

			buf.AppendFormat(_L("\n"));
			iAppView->Write(buf);
		} //view hdr
	}//show packets
	aNext = hdr->FragmentOffset() == 0 ? hdr->Protocol() : 0;
	return hdr->HeaderLength();
}

TUint CRotorListener::MonitorICMPv4(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderICMP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderICMP *const hdr = (TInet6HeaderICMP *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iICMPPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--ICMPv4--\n"));
		if (aMonitor.iICMPType)
			buf.AppendFormat(_L("Type=%u  "), hdr->Type());

		if (aMonitor.iICMPCode)
			buf.AppendFormat(_L("Code=%u  "), hdr->Code());

		if (aMonitor.iICMPChksum)
			buf.AppendFormat(_L("Checksum=%u  "), hdr->Checksum()); 

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
		return ICMPv4Specific(aPacket, aMonitor, aNext);
	}
	aNext = 0;
	return aPacket.Length();
}

TUint CRotorListener::ICMPv4Specific(const TDesC8 &aPacket, const SMonIpInfo & /*aMonitor*/, TUint &aNext)
{
	if (aPacket.Length() < Max(TInet6HeaderICMP::MinHeaderLength(), TInet6HeaderICMP_Echo::MinHeaderLength()))
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderICMP *const hdr = (TInet6HeaderICMP *)aPacket.Ptr(); //lint !e826 // the length is checked above
	const TInet6HeaderICMP_Echo *const echoHdr = (TInet6HeaderICMP_Echo *)aPacket.Ptr(); //lint !e826 // the length is checked above

	if (aPacket.Length() > TInet6HeaderIP::MinHeaderLength() + 8)
		{
		// Note: 8 is for error reports
		const TInet6HeaderIP *const ipHdr = (TInet6HeaderIP *)(aPacket.Ptr() + 8); //lint !e826 // the length is checked above
		// ...by default, assume as error packet (experimental!)
		aNext = ipHdr->Version() == 4 ? KProtocolInetIpip :
				ipHdr->Version() == 6 ? KProtocolInet6Ipip : 0;
		}
	else
		aNext = 0;

	TBuf<64> buf;
	const TInt code=hdr->Code();
	switch(hdr->Type())
	{
		case KInet4ICMP_EchoReply:
			aNext = 0; // "Final" header
			buf.Format(_L("Echo Reply: "));
			buf.AppendFormat(_L("Id=%d "),echoHdr->Identifier());
			buf.AppendFormat(_L("Seq=%d "),echoHdr->Sequence());
			break;
		case KInet4ICMP_Unreachable: 
			switch (code)
			{
				case 0: buf.Format(_L("Network Unreachable"));
					break;
				case 1: buf.Format(_L("Host Unreachable"));
					break;
				case 2: buf.Format(_L("Protocol Unreachable"));
					break;
				case 3: buf.Format(_L("Port Unreachable"));
					break;
				case 4: buf.Format(_L("Message too long. Fragmentation needed"));
					break;
				case 5: buf.Format(_L("Source Route Failed"));
					break;
				case 6: buf.Format(_L("Destination Network Unknown"));
					break;
				case 7: buf.Format(_L("Destination Host Unknown"));
					break;
				case 8: buf.Format(_L("Source host isolated"));
					break;
				case 9: buf.Format(_L("Destination Network Administatively prohibited"));
					break;
				case 10: buf.Format(_L("Destination Host Administatively prohibited"));
					break;
				case 11: buf.Format(_L("Network Unreachable for TOS"));
					break;
				case 12: buf.Format(_L("Host Unreachable for TOS"));
					break;
				case 13: buf.Format(_L("Communication Administatively prohibited"));
					break;
				case 14: buf.Format(_L("Host Precedence violation"));
					break;
				case 15: buf.Format(_L("Precedence cutoff in effect"));
					break;
				default: buf.Format(_L("Unknown code for Destination Unreachable"));
			}
			break;
		case KInet4ICMP_SourceQuench: buf.Format(_L("Source Quench"));
			break;
		case KInet4ICMP_Redirect: 
			switch (code)
			{
				case 0: buf.Format(_L("Redirect for network"));
					break;
				case 1: buf.Format(_L("Redirect for Host"));
					break;
				case 2: buf.Format(_L("Redirect for TOS and Network"));
					break;
				case 3: buf.Format(_L("Redirect for TOS and Host"));
					break;
				default: buf.Format(_L("Unknown code for ICMP Redirect"));
			}
			break;
		case KInet4ICMP_Echo: 
			aNext = 0; // "Final" header
			buf.Format(_L("Echo Request: "));
			buf.AppendFormat(_L("id= %d "),echoHdr->Identifier());
			buf.AppendFormat(_L("Seq= %d "),echoHdr->Sequence());
			break;
		case 9: buf.Format(_L("Router advertisement"));
			aNext = 0; // "Final" header
			break;
		case 10: buf.Format(_L("Router solicitation"));
			aNext = 0; // "Final" header
			break;
		case KInet4ICMP_TimeExceeded: 
			switch (code)
			{
				case 0: buf.Format(_L("TTL 0 during Transit"));
					break;
				case 1: buf.Format(_L("TTL 0 during Reassembly"));
					break;
				default: buf.Format(_L("Unknown Code for Time exceeded type"));
			}
			break;
		case KInet4ICMP_ParameterProblem: buf.Format(_L("Parameter Problem"));
			break;
		case KInet4ICMP_TimeStamp: buf.Format(_L("Timestamp Request"));
			aNext = 0; // "Final" header
			break;
		case KInet4ICMP_TimeStampReply: buf.Format(_L("Timestamp Reply"));
			aNext = 0; // "Final" header
			break;
		case 15: buf.Format(_L("Information Request"));
			aNext = 0; // "Final" header
			break;
		case 16: buf.Format(_L("Information Reply"));
			aNext = 0; // "Final" header
			break;
		case 17: buf.Format(_L("Adress Mask Request"));
			aNext = 0; // "Final" header
			break;
		case 18: buf.Format(_L("Adress Mask Reply"));
			aNext = 0; // "Final" header
			break;
		default: buf.Format(_L("Unknown ICMP Type"));
			aNext = 0; // "Final" header
			break;
	}
	buf.AppendFormat(_L("\n"));
	iAppView->Write(buf);
	return aNext ? 8 : aPacket.Length();
}


TUint CRotorListener::MonitorTCP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderTCP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderTCP *const hdr= (TInet6HeaderTCP *)aPacket.Ptr(); //lint !e826 // the length is checked above
	iEngine->iStatInfo.iTCPPackets++;
	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--TCP--\n"));

		if (aMonitor.iTCPSrcPort)
			buf.AppendFormat(_L("Src Port=%u  "), hdr->SrcPort());
		if (aMonitor.iTCPDstPort)
			buf.AppendFormat(_L("Dst Port=%u  "), hdr->DstPort());
		if (aMonitor.iTCPSeq)
			buf.AppendFormat(_L("Seq=%u  "), hdr->Sequence().Uint32());
		if (aMonitor.iTCPAckNum)
			buf.AppendFormat(_L("ack=%u  "), hdr->Acknowledgment().Uint32());
		if (aMonitor.iTCPHdrLen)
			buf.AppendFormat(_L("hdr len=%u  "), hdr->HeaderLength());
		if (aMonitor.iTCPFlags)
			buf.AppendFormat(_L("flags=%6b  "), hdr->Control());
		if (aMonitor.iTCPHdrWinSize)
			buf.AppendFormat(_L("win=%u  "), hdr->Window());
		if (aMonitor.iTCPChksum)
			buf.AppendFormat(_L("chksum=%u  "), hdr->Checksum());
		if (aMonitor.iTCPHdrUrgPtr)
			buf.AppendFormat(_L("urg=%u  "), hdr->Urgent());
		if (aMonitor.iTCPOptions && hdr->HeaderLength() > 20)
			buf.AppendFormat(_L("optlen=%u  "), hdr->Options().Length());	//Include padding

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
		if (aPacket.Length() > hdr->HeaderLength())
			ViewData(aPacket.Ptr() + hdr->HeaderLength(), aPacket.Length() - hdr->HeaderLength());
	}
	aNext = 0;
	return aPacket.Length();
}

//Show data in Hex Format. Size in bytes
void CRotorListener::ViewData(const TUint8 *aData, TUint aSize)
{
	TBuf<1024> buf;
	//TBool odd=EFalse;

	buf.Zero();
	TInt width=iAppView->ScreenSize().iWidth;
/*
	if (aSize % 2 != 0)	//In case size is odd
	{		
		odd=ETrue;
		aSize--;
	}
*/
/*	
	for (TInt i=0; i<aLength;i++)
	{
		if (i%4==0)
			buf->Des().AppendFormat(_L(" "));
		buf->Des().AppendFormat(_L("%02.2x"),aArray[i]);	//key Data byte2byte
	}
*/
	TInt chars=0;
	for (TUint i=0; i < aSize ; i++)
	{
		if (i%4==0)
		{
			if (chars > (width - 20))	//to have data aligned
			{
				buf.AppendFormat(_L("\n"));
				chars=0;
			}
			else if (i!=0)
				buf.AppendFormat(_L(" "));	//space every 4 bytes
		}
		buf.AppendFormat(_L("%02.2x"),aData[i]);	//key Data byte2byte
		chars+=2;

		if (buf.Length() > 1016)	//buffer full
		{
			iAppView->Write(buf);
			buf.Zero();
		}
	}
/*
	for (TUint i=0; i < aSize ; i+=2)
	{
		buf.AppendFormat(_L("%04x "),(TUint16)aData[i]);	//5 chars: 4 hex + 1 blank
		chars+=5;
		if (chars > (width-5))	//to have data aligned
		{
			buf.AppendFormat(_L("\n"));
			chars=0;
		}

		if (buf.Length() > 1016)	//buffer full
		{
			iAppView->Write(buf);
			buf.Zero();
		}
	}
*/
/*
	if (odd)	//Prints the odd byte
		buf.AppendFormat(_L("%x  "),aData[aSize]);	//last byte
*/
	if (buf.Length() > 0)	//Write missing data
	{
		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
	}

}

TUint CRotorListener::MonitorUDP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderUDP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderUDP *const hdr = (TInet6HeaderUDP *) aPacket.Ptr(); //lint !e826 // the length is checked above
	iEngine->iStatInfo.iUDPPackets++;
	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--UDP--\n"));
		if (aMonitor.iUDPSrcPort)
			buf.AppendFormat(_L("Src Port=%u  "), hdr->SrcPort());
		if (aMonitor.iUDPDstPort)
			buf.AppendFormat(_L("Dst Port=%u  "), hdr->DstPort());
		if (aMonitor.iUDPLen)
			buf.AppendFormat(_L("Len=%u  "), hdr->Length());
		if (aMonitor.iUDPChksum)
			buf.AppendFormat(_L("chksum=%u  "), hdr->Checksum());

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
		if (aPacket.Length() > hdr->HeaderLength())
			ViewData(aPacket.Ptr() + hdr->HeaderLength(), aPacket.Length() - hdr->HeaderLength());
	}
	aNext = 0;
	return aPacket.Length();
}

TUint CRotorListener::MonitorESP(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderESP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderESP *const hdr = (TInet6HeaderESP *)aPacket.Ptr(); //lint !e826 // the length is checked above
	
	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--ESP--\n"));

		if (aMonitor.iESPSPI)
			buf.AppendFormat(_L("SPI=%u  "), ByteOrder::Swap32(hdr->SPI()));
		if (aMonitor.iESPSeq)
			buf.AppendFormat(_L("seq=%u  "), hdr->Sequence());

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
	}
	aNext = 0;
	return aPacket.Length();
}

TUint CRotorListener::MonitorAH(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderAH::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderAH *const hdr = (TInet6HeaderAH *)aPacket.Ptr(); //lint !e826 // the length is checked above

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--AH--\n"));

		if (aMonitor.iAHProtocol)
			buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());
		if (aMonitor.iAHHdrLen)
			buf.AppendFormat(_L("Hdr Len=%u  "), hdr->HeaderLength());
		if (aMonitor.iAHSPI)
			buf.AppendFormat(_L("SPI=%u  "), ByteOrder::Swap32(hdr->SPI()));
		if (aMonitor.iAHSeq)
			buf.AppendFormat(_L("seq=%u  "), hdr->Sequence());

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
	}
	aNext = hdr->NextHeader();
	return hdr->HeaderLength();
}

TUint CRotorListener::MonitorIPv6(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderIP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderIP *const hdr = (TInet6HeaderIP *)aPacket.Ptr(); //lint !e826 // the length is checked above

	if (!Filter(aPacket))	// TCP/UDP Port filtering
	{
		iEngine->iStatInfo.iIPv6Packets++;
		if (iEngine->iShowPackets)
		{	
			if (iEngine->iPref.iViewIPHdr)
			{
				TBuf<400> buf;
				TInetAddr addr;
				TBuf<IPV6_ADDR_SIZE> textaddr;

				buf.Format(_L("--IPv6--\n"));

				if (aMonitor.iIPVersion)
					buf.AppendFormat(_L("Version=%u  "), hdr->Version());
	
				if (aMonitor.iIPTraffic)
					buf.AppendFormat(_L("Traffic Class=%u  "), hdr->TrafficClass());

				if (aMonitor.iIPFlowLabel)
					buf.AppendFormat(_L("Flow=%u  "), hdr->FlowLabel());  

				if (aMonitor.iIPPayloadLen)
					buf.AppendFormat(_L("Length=%u  "), hdr->PayloadLength());

				if (aMonitor.iIPNextHdr)
					buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());  

				if (aMonitor.iIPHopLimit)
				{
					buf.AppendFormat(_L("Hop Lim=%u  "), hdr->HopLimit());  
				}

	
				if (aMonitor.iIPSrcAddr)
				{
					addr.SetAddress(hdr->SrcAddr());
					addr.Output(textaddr);
					buf.AppendFormat(_L("Src="));
					buf.AppendFormat(textaddr);
				}
		
				if (aMonitor.iIPDstAddr)
				{
					addr.SetAddress(hdr->DstAddr());
					addr.Output(textaddr);
					buf.AppendFormat(_L(" Dst="));
					buf.AppendFormat(textaddr);
				}

				buf.AppendFormat(_L("\n"));
				iAppView->Write(buf);
			}//view hdr 
		}//show packets
	}//filter
	aNext = hdr->NextHeader();
	return hdr->HeaderLength();
}


TUint CRotorListener::MonitorICMPv6(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderICMP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderICMP *const hdr = (TInet6HeaderICMP *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iICMPPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--ICMPv6--\n"));

		if (aMonitor.iICMPType)
			buf.AppendFormat(_L("Type=%u  "), hdr->Type());

		if (aMonitor.iICMPCode)
			buf.AppendFormat(_L("Code=%u  "), hdr->Code());

		if (aMonitor.iICMPChksum)
			buf.AppendFormat(_L("Checksum=%u  "), hdr->Checksum()); 

		//if (info->iICMPParameter)
		//	buf.AppendFormat(_L("Parameter=%u  "), hdr->Parameter()); 

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
		return ICMPv6Specific(aPacket, aMonitor, aNext);	//Specific Info
	}
	aNext = 0;
	return hdr->HeaderLength();
	//No more packets after this one
}


TUint CRotorListener::ICMPv6Specific(const TDesC8 &aPacket, const SMonIpInfo &/*aMonitor*/, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderICMP::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	TBuf<100> buf;
	const TInet6HeaderICMP_Echo *echoHdr;
	const TInet6HeaderICMP* hdr = (TInet6HeaderICMP *)aPacket.Ptr(); //lint !e826 // the length is checked above
	TInt8 code=hdr->Code();

	switch(hdr->Type())
	{
		//Errors 0-127
		case KInet6ICMP_Unreachable: 
			buf.Format(_L("type= "));
			buf.AppendFormat(_L("Dest. Unreach. "));
			//buf.Format(_L("type=%d "),type);
			switch (code)
			{
			case KInet6ICMP_NoRoute:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("No Route "));
				break;
			case KInet6ICMP_AdminProhibition:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Admin. Prohibition "));
				break;
			case KInet6ICMP_NotNeighbour:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Not a Neighbour "));
				break;
			case KInet6ICMP_AddrUnreach:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Addr. Unreachable "));
				break;
			case KInet6ICMP_PortUnreach:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Port Unreachable "));
				break;
			default: buf.AppendFormat(_L("code=%d "),code);
			}
			break;
		case KInet6ICMP_PacketTooBig:
			buf.Format(_L("type= "));
			buf.AppendFormat(_L("Pack. Too big "));
			break;
		case KInet6ICMP_TimeExceeded:
			buf.Format(_L("type= "));
			buf.AppendFormat(_L("Time exceeded "));
			switch (code)
			{
			case KInet6ICMP_HopLimitExceeded:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Hop Limit "));
				break;
			case KInet6ICMP_FragReassExceeded:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Frag. Reassembly "));
				break;
			default: buf.AppendFormat(_L("code=%d "),code);
			}
			break;
		case KInet6ICMP_ParameterProblem:
			buf.Format(_L("type= "));
			buf.AppendFormat(_L("Parameter problem "));
			switch (code)
			{
			case KInet6ICMP_ErrHdrField:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Bad header filed"));
				break;
			case KInet6ICMP_NextHdrUnknown:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Unknown Next Header "));
				break;
			case KInet6ICMP_OptionUnkown:
				buf.AppendFormat(_L("code= "));
				buf.AppendFormat(_L("Unknown Option"));
				break;
			default: buf.AppendFormat(_L("code=%d "),code);
			}
			break;

		//Information 128-255
		case KInet6ICMP_EchoRequest:
			if (aPacket.Length() < TInet6HeaderICMP_Echo::MinHeaderLength())
				{
				aNext = KProtocolUnknown;
				return 0;
				}
			echoHdr=(const TInet6HeaderICMP_Echo *)hdr; //lint !e826 // the length is checked above
			buf.AppendFormat(_L("Echo Request: "));
			buf.AppendFormat(_L("Id=%d "),echoHdr->Identifier());
			buf.AppendFormat(_L("Seq=%d "),echoHdr->Sequence());
			break;
		case KInet6ICMP_EchoReply:
			if (aPacket.Length() < TInet6HeaderICMP_Echo::MinHeaderLength())
				{
				aNext = KProtocolUnknown;
				return 0;
				}
			echoHdr=(const TInet6HeaderICMP_Echo *)hdr; //lint !e826 // the length is checked above
			buf.AppendFormat(_L("Echo Reply: "));
			buf.AppendFormat(_L("Id=%d "),echoHdr->Identifier());
			buf.AppendFormat(_L("Seq=%d "),echoHdr->Sequence());
			break;

		case KInet6ICMP_GroupQuery:
			buf.AppendFormat(_L("Multicast Listener Query"));
			break;

		case KInet6ICMP_GroupReport:
			buf.AppendFormat(_L("Multicast Listener Report"));
			break;

		case KInet6ICMP_GroupDone:
			buf.AppendFormat(_L("Multicast Listener Done"));
			break;

		case KInet6ICMP_RouterSol:
			buf.AppendFormat(_L("Router Solicitation"));
			break;

		case KInet6ICMP_RouterAdv:
			buf.AppendFormat(_L("Router Advertisement"));
			break;

		case KInet6ICMP_NeighborSol:
			buf.AppendFormat(_L("Neighbor Solicitation"));
			break;

		case KInet6ICMP_NeighborAdv:
			buf.AppendFormat(_L("Neighbor Advertisement"));
			break;

		case KInet6ICMP_Redirect:
			buf.AppendFormat(_L("Redirect"));
			break;

		default:
			buf.Format(_L("Unknown ICMP Type"));
			break;
	}
	buf.AppendFormat(_L("\n"));
	iAppView->Write(buf);
	aNext = 0;
	return aPacket.Length();
}



TUint CRotorListener::MonitorHopByHopHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderHopByHop::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderHopByHop *const hdr = (TInet6HeaderHopByHop *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iExtPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--IPv6 HopByHop Header--\n"));
		if (aMonitor.iHOPNextHdr)
			buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());

		if (aMonitor.iHOPHdrExtLen)
			buf.AppendFormat(_L("Hdr Len=%u  "), hdr->HeaderLength());

		if (aMonitor.iHOPOptionType)
		{
			if (hdr->OptionType()==194)
				buf.AppendFormat(_L("Jumbo Payload"));
			else
				buf.AppendFormat(_L("type=%d "),hdr->OptionType());
		}
	
		if (aMonitor.iHOPOptionLen)
			buf.AppendFormat(_L("length=%u  "), hdr->OptionDataLen());

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
	}
	aNext = hdr->NextHeader();
	return hdr->HeaderLength();
}


TUint CRotorListener::MonitorDestOptHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6Options::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6Options *const hdr = (TInet6Options *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iExtPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--IPv6 Destination Options--\n"));

		if (aMonitor.iDSTNextHdr)
			buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());

		if (aMonitor.iDSTHdrExtLen)
			buf.AppendFormat(_L("HdrExtLen=%u  "), hdr->HdrExtLen());	//Real value in 8-octet units

		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);

		if (aMonitor.iDSTHomeAddr			|| 
			aMonitor.iDSTBindingUpdate		||
			aMonitor.iDSTBindingRequest	||
			aMonitor.iDSTBindingAck		||
			aMonitor.iDSTPad				|| 
			aMonitor.iDSTUnknown) {
			const TUint8 *ptr = aPacket.Ptr() + TInet6Options::O_Options;
			TInt len = hdr->HeaderLength() - TInet6Options::O_Options;
			while (len > 0) {
				TInt type = *ptr;
				TInt optlen = 1;

				if (type != KDstOptionPad1)
					optlen = 2 + *(ptr + 1);
				
				if (optlen > len)
					break;
				
				buf.SetLength(0);
				switch (type) {
				case KDstOptionPad1:
				case KDstOptionPadN:
					if (aMonitor.iDSTPad)
						buf.Format(_L("  Pad%u"), optlen);
					break;
				case KDstOptionHomeAddress:
					if (aMonitor.iDSTHomeAddr) {
						TInetAddr addr((const TIp6Addr &)*(ptr + 2), 0);
						buf.Format(_L("  Home Address="));
						addr.Output(buf);
						buf.AppendFormat(_L(", Length=%u"), optlen);
					}
					break;

				default:
					if (aMonitor.iDSTUnknown)
						buf.Format(_L("  Type=%u, Length=%u"), type, optlen);
					break;
				}
				if (buf.Length() > 0) {
					buf.AppendFormat(_L("\n"));
					iAppView->Write(buf);
				}

				len -= optlen;
				ptr += optlen;
			}
		}	
	}
	aNext = hdr->NextHeader();
	return hdr->HeaderLength();
}



TUint CRotorListener::MonitorRoutingHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderRouting::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderRouting *const hdr = (TInet6HeaderRouting *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iExtPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--IPv6 Routing Header--\n"));

		if (aMonitor.iRTNextHdr)
			buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());

		if (aMonitor.iRTHdrExtLen)
			buf.AppendFormat(_L("HdrExtLen=%u  "), hdr->HdrExtLen());	//Real value in 8-octet (64 bits) units

		if (aMonitor.iRTRoutingType)
			buf.AppendFormat(_L("Routing type=%d "),hdr->RoutingType());
	
		if (aMonitor.iRTSegLeft)
			buf.AppendFormat(_L("Segments Left=%u  "), hdr->SegmentsLeft());
	
		/*	deleted from RFC?
		if (info->iRTSLBitMap)
			buf.AppendFormat(_L("S/L BitMap=%24b  "), hdr->StrictLooseBitMap());	//24 bits binary view
		*/
		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
		if (aMonitor.iRTAddresses && hdr->RoutingType() == 0) {
			const TUint8 *ptr = aPacket.Ptr() + TInet6HeaderRouting::O_Address;
			for (TInt i=0; i<hdr->HdrExtLen()/2; i++) {
				TInetAddr addr((const TIp6Addr &)*ptr, 0);
				buf.Format(_L("%1d "), i+1);
				addr.Output(buf);
				buf.AppendFormat(_L("\n"));
				iAppView->Write(buf);
				ptr += sizeof(TIp6Addr);
			}
		}
	}
	aNext = hdr->NextHeader();
	return hdr->HeaderLength();
}


TUint CRotorListener::MonitorFragmentHeader(const TDesC8 &aPacket, const SMonIpInfo &aMonitor, TUint &aNext)
{
	if (aPacket.Length() < TInet6HeaderFragment::MinHeaderLength())
		{
		aNext = KProtocolUnknown;
		return 0;
		}

	const TInet6HeaderFragment *const hdr = (TInet6HeaderFragment *)aPacket.Ptr(); //lint !e826 // the length is checked above

	iEngine->iStatInfo.iExtPackets++;

	if (iEngine->iShowPackets)
	{
		TBuf<256> buf;
		buf.Format(_L("--IPv6 Fragment Header--\n"));

		if (aMonitor.iFRAGNextHdr)
			buf.AppendFormat(_L("Next Hdr=%u  "), hdr->NextHeader());

		if (aMonitor.iFRAGFragOffset)
			buf.AppendFormat(_L("Offset=%u  "), hdr->FragmentOffset());

		if (aMonitor.iFRAGMFlag)	//Binary 1 bit
			buf.AppendFormat(_L("M=%d "),hdr->MFlag() != 0);
	
		if (aMonitor.iFRAGId)
			buf.AppendFormat(_L("Id=%u  "), hdr->Id());
	
		buf.AppendFormat(_L("\n"));
		iAppView->Write(buf);
	}

	// Can continue dumping only if this is the first fragment!
	aNext =  hdr->FragmentOffset() == 0 ? hdr->NextHeader() : 0;
	return hdr->HeaderLength();
}

void CRotorListener::ErrorL(const TDesC& string,TInt error)
{
	TBuf<150> aux;
	TBuf<150> errtxt;

	aux.Format(string);	
	CEikonEnv::Static()->GetErrorText(errtxt,error);
	aux.AppendFormat(errtxt);
	aux.AppendFormat(_L("\n"));
	
	iAppView->Write(aux);
	if (IsActive())
		Cancel();
	User::Leave(error);	// NOT SURE IF IT'S THE BEST WAY!!!	
}

//
//	CRotorReceiver	waits to receive packets from the socket
//


//constructor
CRotorReceiver::CRotorReceiver(CRotorAppView *aAppView,CRotorEngine *aEngine):CRotorListener(aAppView,aEngine)
{
}
	
//second phase constructor
void CRotorReceiver::ConstructL(TUint aProtocol, const TDesC &aName)
{
	TInt err=KErrNone;

	CRotorListener::ConstructL();

	TInt port=KInetPortAny;

	err=iSocket.Open(iEngine->SocketServ(), aName);

	if (err!=KErrNone)
	{
		ErrorL(_L("Socket Error (Open) "),err);
		return;
	}

	switch (aProtocol)
	{
	case ICMP:
		port=KProtocolInetIcmp;
		break;
	case IP:
		port=KInetPortAny;
		break;
	case TCP:
		port=KProtocolInetTcp;
		break;
	case UDP:
		port=KProtocolInetUdp;
		break;
	case ESP:	//only IPv4
		port=KProtocolInetEsp;
		break;
	case AH:
		port=KProtocolInetAh;	//only IPv4
		break;
	default: 
		ErrorL(_L("Protocol type "),KErrUnknown);
		return;
	}

	TInetAddr anyAddr(KInetAddrAny,port);	//Sniffs all packets
	err=iSocket.Bind(anyAddr);
	if (err!=KErrNone)
	{	
		ErrorL(_L("Socket Error (Bind)"),err);
		return;
	}

	err=iSocket.SetOpt(KSoRawMode,KSolInetIp,1);	//to read the IP header
	if (err!=KErrNone)
	{
		ErrorL(_L("Socket Error (Options)"),err);
		return;
	}
	err=iSocket.SetOpt(KSoHeaderIncluded,KSolInetIp,1);	//to read the IP header
	if (err!=KErrNone)
	{
		ErrorL(_L("Socket Error (Options)"),err);
		return;
	}

	//
	// A special kludge to handle the swapped IP header for the OLD TCPIP
	//
	iSwapIpHeader = aName.Compare(_L("ip")) == 0;
}

// will send all the packets
void CRotorReceiver::RunL()
{
	if (iStatus==KErrNone)
	{
		iEngine->PacketReceived();
		
		if (iSwapIpHeader && (iBuf.Length() < TInet6HeaderIP4::MinHeaderLength()))
			{
			// Does not work at all, if there are any other packets
			// except IPv4...

			TInet6HeaderIP4 *hdr = (TInet6HeaderIP4 *)iBuf.Ptr(); //lint !e826 // the length is checked above
			if (hdr->Version() != 4)
				{
				hdr->Swap();
				if (hdr->Version() != 4)
					hdr->Swap();	// Oops, I suppose not then...
				}
			}

		MonitorIpPacket(iBuf);
		iAppView->UpdateStatisticsL();
		iSocket.Recv(iBuf, 0, iStatus);	//Waits on iStatus to alert RunL when req. complete
		IssueRequest();	//Prepares to receive it in RunL()
	}
	else
	{
		iErrPackets++;
		iSocket.Recv(iBuf, 0, iStatus);	//Waits on iStatus to alert RunL when req. complete
		IssueRequest();	//Prepares to receive it in RunL()
	}

}

//
//	CRotorBraker: Takes care of decreasing the speed of the Rotor gradually
//

CRotorBraker::CRotorBraker(CRotorEngine *aEngine):CTimer(EPriorityStandard)
{
	iEngine=aEngine;
	CActiveScheduler::Add(this);
}


CRotorBraker::~CRotorBraker()
{
	if (IsActive())
		Cancel();
}

void CRotorBraker::ConstructL()
{
	//Base class 2nd phase constructor
	CTimer::ConstructL();
}

void CRotorBraker::Start()
{
	FirstRun();
}

void CRotorBraker::FirstRun()
{
	IssueRequest();	//Also sets the object as Active. 0.1 secs
	//if (!IsActive())
	//	SetActive();
}

// will send all the packets. One packet each Time
void CRotorBraker::RunL()
{
	iEngine->DecreaseSpeedL();
	IssueRequest();	//First RunL
}

void CRotorBraker::Stop() const
{
	iEngine->iAppView->Write(_L("Braker Stop!!!\n"));
	/*
	if (IsAdded())
		Deque();	//Dequeues the active object of the Scheduler. Calls DoCancel()	
		*/
}

//Cancel Packet Sending
void CRotorBraker::DoCancel()
{
	//iEngine->iAppView->Write(_L("Braker DoCancel!!!\n"));
	CTimer::DoCancel();
	//Deque();	//Dequeues the active object of the Scheduler
}

void CRotorBraker::ReIssueRequest()
{
	if (IsActive())
	{
		Cancel();	 //Cancels the current request
		IssueRequest();  
	}
}

void CRotorBraker::IssueRequest()
{
	//After((TInt)(1.5*SECOND));	//Also sets the object as Active
	After((TInt)(1.5*SECOND));	//Also sets the object as Active
}
