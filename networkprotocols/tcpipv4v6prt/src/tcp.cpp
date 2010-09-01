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
// tcp.cpp - TCP protocol for IPv6/IPv4
//

#include "tcp.h"
#include <icmp6_hdr.h>
#include <ip4_hdr.h>
#include "tcpip_ini.h"

// Copied from ip6.cpp. Should move to some common definition file?
static const TLitC8<sizeof(TInt)> KInetOptionDisable = {sizeof(TInt), {0}};

//
// TCP Protocol Description
//

void CProtocolTCP6::Describe(TServerProtocolDesc &aDesc)
	{
	aDesc.iName		  = _S("tcp");
	aDesc.iAddrFamily	  = KAfInet;
	aDesc.iSockType	  = KSockStream;
	aDesc.iProtocol	  = KProtocolInetTcp;
	aDesc.iVersion	  = TVersion(KInet6MajorVersionNumber,
		KInet6MinorVersionNumber,
		KInet6BuildVersionNumber);
	aDesc.iByteOrder	  = EBigEndian;
	aDesc.iServiceInfo	  = KSIStreamBased | KSIInOrder | KSIReliable |
		KSIGracefulClose | KSIPeekData | KSIUrgentData |
		KSIRequiresOwnerInfo;
	aDesc.iNamingServices	  = KNSNameResolution | KNSRequiresConnectionStartup;
	aDesc.iSecurity	  = KSocketNoSecurity;
	aDesc.iMessageSize	  = KSocketMessageSizeIsStream;
	aDesc.iServiceTypeInfo  = ESocketSupport | ETransport | EPreferMBufChains | ENeedMBufs | EUseCanSend;
	aDesc.iNumSockets	  = KUnlimitedSockets;
	}



CProtocolTCP6::CProtocolTCP6()
	{
	__DECLARE_NAME(_S("CProtocolTCP6"));
	}


CProtocolTCP6::~CProtocolTCP6()
	{
	LOG(Log::Printf(_L("\ttcp Deleted")));
	}

CServProviderBase* CProtocolTCP6::NewSAPL(TUint aSockType)
	{
	LOG(Log::Printf(_L("NewSAPL\ttcp SockType=%d)"), aSockType));
	if (aSockType!=KSockStream)
		User::Leave(KErrNotSupported);
	CProviderTCP6 *sap = new (ELeave) CProviderTCP6(this);
	CleanupStack::PushL(sap);
	sap->InitL();
	CleanupStack::Pop();
	LOG(Log::Printf(_L("NewSAPL\ttcp SAP[%u] OK"), (TInt)sap));
	return sap;
	}


void CProtocolTCP6::InitL(TDesC& aTag)
	{
	CProtocolInet6Transport::InitL(aTag);

	iRandomIncrement = 0;
	UserHal::TickPeriod((TTimeIntervalMicroSeconds32&)iClockGranularity);
	}


void CProtocolTCP6::StartL()
	{
	CProtocolInet6Transport::StartL();

	iMSS = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_MSS,
			KTcpDefaultMSS, KTcpMinimumMSS, 65535, ETrue);

	iRecvBuf = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RECV_BUF,
			KTcpDefaultRcvWnd, KTcpMinimumWindow, KTcpMaximumWindow, ETrue); 
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	iRecvBufFromIniFile = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RECV_BUF,
            KTcpDefaultRcvWnd, KTcpMinimumWindow, KTcpMaximumWindow, ETrue); 
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	
	iSendBuf = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_SEND_BUF,
			KTcpDefaultSndWnd, KTcpMinimumWindow, KTcpMaximumWindow, ETrue);

	// Argh: ini is in millisecs, but CProtocolTCP6 member is in microsecs.
	iMinRTO = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_MIN_RTO,
			KTcpMinRTO/1000, iClockGranularity/1000, KTcpMaxRTO/1000, ETrue) * 1000;

	iMaxRTO = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_MAX_RTO,
			KTcpMaxRTO/1000, iMinRTO/1000, KTcpMaxRTO/1000, ETrue) * 1000;

	iInitialRTO = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_INITIAL_RTO,
			KTcpInitialRTO/1000, iMinRTO/1000, iMaxRTO/1000, ETrue) * 1000;
			
	iSrttSmooth = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_SRTT_SMOOTH,
			KTcpSrttSmooth, 1, KMaxTInt, ETrue);
			
	iMdevSmooth = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RTTVAR_SMOOTH,
			KTcpMdevSmooth, 1, KMaxTInt, ETrue);

	iRTO_K = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RTO_K,
			KTcpRTO_K, 1, KMaxTInt, ETrue);

	iRTO_G = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RTO_G,
			iClockGranularity/1000, iClockGranularity/1000, KMaxTInt, ETrue) * 1000;

	iMaxBurst = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_MAX_BURST,
			KTcpMaxTransmit, 1, KMaxTInt, ETrue);

	iAckDelay = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_ACK_DELAY,
			KTcpAckDelay/1000, iClockGranularity/1000, KMaxTInt/1000, ETrue) * 1000;

	iSynRetries = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_SYN_RETRIES,
			KTcpSynRetries, 0, KMaxTInt, ETrue);

	iRetries1 = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RETRIES1,
			KTcpMaxRetries1, 0, KMaxTInt, ETrue);

	iRetries2 = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RETRIES2,
			KTcpMaxRetries2, iRetries1, KMaxTInt, ETrue);

	iProbeStyle = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_PROBE_STYLE,	2, 0, 2, EFalse);

	iMsl2Delay = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_MSL2,
			KTcpMsl2Delay/1000, 1, KMaxTInt/1000, ETrue) * 1000;

	iReordering = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_REORDERING,
			KTcpReordering, 0, KMaxTInt, ETrue);

	iInitialCwnd = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_INITIAL_CWND,
			KTcpInitialCwnd, 0, KMaxTInt, ETrue);

	iLtxWindow = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_LTX_WINDOW,
			KTcpLtxWindow, 0, KMaxTInt, ETrue);

	iKeepAliveIntv = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_KEEPALIVE_INTV,
			KTcpKeepAliveIntv, 0, KMaxTInt, ETrue);

	iNumKeepAlives = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_NUM_KEEPALIVES,
			KTcpNumKeepAlives, 0, KMaxTInt, ETrue);

	// keepalive retransmission timer cannot be longer than 30 min, because timers are in microseconds.
	iKeepAliveRxmt = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_KEEPALIVE_RXMT,
			KTcpKeepAliveRxmt, 0, 1800, ETrue);

	iFinPersistency = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_FIN_PERSISTENCY,
			KTcpFinPersistency, 0, iRetries2, ETrue);
	
	// ECN settings: 0 = disabled, 1 = enabled with ECT(1), 2 = enabled with ECT(0)
	iEcn = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_ECN, 0, 0, 2, EFalse);

	iSpuriousRtoResponse = GetIniValue(TCPIP_INI_TCP,
			TCPIP_INI_TCP_SPURIOUS_RTO_RESPONSE, 1, 1, 3, EFalse);

	iWinScale = (TInt8)GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_WINSCALE, 0, -1, 7, ETrue);

	// Flags enabled by default
	iTimeStamps = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_TIMESTAMPS, 1, 0, 1);
	iSack = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_SACK, 1, 0, 1);
	iRFC2414 = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RFC2414, 1, 0, 1);
	iDSack = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_DSACK, 1, 0, 1);
	iFRTO = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_FRTO, 1, 0, 1);

	// Flags disabled by default
	iLocalTimeWait = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_LOCAL_TIMEWAIT, 0, 0, 1);
	iStrictNagle = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_STRICT_NAGLE, 0, 0, 1);
	iPushAck = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_PUSH_ACK, 0, 0, 1);
	iAlignOpt = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_ALIGNOPT, 0, 0, 1);

	// Note: 'dstcache' is in [ip] section, because it is not necessarily TCP specific
	iDstCache = GetIniValue(TCPIP_INI_IP, TCPIP_INI_DSTCACHE, 0, 0, 1);
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	iTcpMaxRecvWin = GetIniValue(TCPIP_INI_TCP, TCPIP_INI_TCP_RECV_MAX_WND,
				KTcpDefaultRcvMaxWnd, KTcpMinimumWindow, KTcpMaximumWindow, ETrue);
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	}


void CProtocolTCP6::Identify(TServerProtocolDesc *aInfo) const
	{
	Describe(*aInfo);
	}

TInt CProtocolTCP6::Send(RMBufChain& aPacket,CProtocolBase* /*aSourceProtocol=NULL*/)
	{
#ifdef _LOGx
	RMBufSendPacket seg;
	seg.Assign(aPacket);
	RMBufSendInfo *info = seg.Unpack();
	LOG(LogPacket('>', seg, info));
	seg.Pack();
	aPacket.Assign(seg);
#endif
	return iNetwork->Send(aPacket);
	}

void CProtocolTCP6::Process(RMBufChain& aPacket, CProtocolBase* /*aSourceProtocol*/)
	{
	RMBufRecvPacket seg;
	seg.Assign(aPacket);
	//  RMBufRecvPacket& seg = (RMBufRecvPacket&)aPacket;
	RMBufRecvInfo* info = seg.Unpack();

#ifdef _LOG
	TBuf<50> src, dst;
	LOG(Log::Printf(_L("\ttcp Process(%d bytes)"), seg.Length()));
#endif

	ASSERT(info->iLength == seg.Length());

	TTcpPacket pkt;
	TInet6Packet<TInet6HeaderIP4> ip4;
	TInet6Packet<TInet6HeaderIP>  ip6;
	TInt err = KErrNone, errMask = MSocketNotify::EErrorAllOperations;
	CProviderTCP6 *sap;
	TInetAddr icmpSender;

	switch (info->iIcmp)
		{
	case 0:

		// Check source and destination addresses
		if (!(TInetAddr::Cast(info->iDstAddr).IsUnicast() && TInetAddr::Cast(info->iSrcAddr).IsUnicast()))
			{
			LOG(Log::Printf(_L("\ttcpProcess() Invalid source or destination address. Packet discarded")));
			seg.Free();
			break;
			}

		pkt.Set(seg, info->iOffset, KTcpMinHeaderLength);

		// Verify header length.
		if (!pkt.iHdr || info->iLength < pkt.iHdr->HeaderLength())
			{
			LOG(Log::Printf(_L("\ttcp Process() Invalid header. Packet discarded")));
			seg.Free();
			return;
			}

		// Get port numbers from header
		info->iSrcAddr.SetPort(pkt.iHdr->SrcPort());
		info->iDstAddr.SetPort(pkt.iHdr->DstPort());

#ifdef _LOG
		LogPacket('<', seg, info, info->iOffset);
		pkt.Set(seg, info->iOffset, pkt.iHdr->HeaderLength()); // LogPacket() may have realigned the header.

		if (!pkt.iHdr)
			{
			LOG(Log::Printf(_L("\ttcp Process() header alignment failed. Packet discarded")));
			seg.Free();
			return;
			}
#endif

		// Verify TCP checksum
		if (!pkt.VerifyChecksum(seg, info, info->iOffset))
			{
			LOG(Log::Printf(_L("\ttcp Process() Bad checksum. Packet discarded")));
			seg.Free();
			break;
			}

#ifdef _LOGx
		TInetAddr(info->iSrcAddr).OutputWithScope(src); TInetAddr(info->iDstAddr).Output(dst);
		LOG(Log::Printf(_L("\ttcp Process(%d bytes): {%S,%d} -> {%S,%d}"),
			seg.Length(), &src, info->iSrcAddr.Port(), &dst, info->iDstAddr.Port()));
#endif

		// More sanity checking.
		if (info->iSrcAddr.Port() == KInetPortNone ||
			info->iDstAddr.Port() == KInetPortNone)
			{
			LOG(Log::Printf(_L("\ttcp Process() Illegal port number. Packet discarded")));
			seg.Free();
			break;
			}

		sap = (CProviderTCP6*)LocateSap(EMatchServerUnspecAddr,
			info->iVersion == 4 ? KAfInet : KAfInet6,
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
			info->iDstAddr, info->iSrcAddr);
#else
			info->iDstAddr, info->iSrcAddr, NULL, info->iInterfaceIndex);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
		if (sap == NULL)
			{
			LOG(Log::Printf(_L("\ttcp Process() No socket. Packet discarded")));
			if (!pkt.iHdr->RST())
				{
				//
				// Send RST.
				//
				if (NetworkService()->Interfacer()->LocalScope(TInetAddr::Cast(info->iDstAddr).Ip6Address(),
					info->iInterfaceIndex, EScopeType_IF))
					{
					if (pkt.iHdr->ACK())
						{
						SendControlSegment(NULL, info->iDstAddr, info->iSrcAddr, KTcpCtlRST,
							pkt.iHdr->Acknowledgment(), 0);
						}
					else
						{
						//TTcpSeqNum seq = pkt.iHdr->Sequence() + aPacket.Length() - pkt.iHdr->HeaderLength();
						TTcpSeqNum seq = pkt.iHdr->Sequence() + seg.Length() -
							pkt.iHdr->HeaderLength() - info->iOffset;

						// Adjust for SYN and FIN
						if (pkt.iHdr->SYN() || pkt.iHdr->FIN())
							seq++;

						SendControlSegment(NULL, info->iDstAddr, info->iSrcAddr, KTcpCtlRST|KTcpCtlACK, 0, seq);
						}
					}
				}
			seg.Free();
			break;
			}

		// Bump it to the SAP.
		seg.Pack();
		sap->Process(seg);
		break;

	case KProtocolInetIcmp:
	case KProtocolInet6Icmp:

        /* From RFC1122:
        4.2.3.9  ICMP Messages
 
            TCP MUST act on an ICMP error message passed up from the IP
            layer, directing it to the connection that created the
            error.  The necessary demultiplexing information can be
            found in the IP header contained within the ICMP message.
 
            o    Source Quench
 
                 TCP MUST react to a Source Quench by slowing
                 transmission on the connection.  The RECOMMENDED
                 procedure is for a Source Quench to trigger a "slow
                 start," as if a retransmission timeout had occurred.
 
            o    Destination Unreachable -- codes 0, 1, 5
 
                 Since these Unreachable messages indicate soft error
                 conditions, TCP MUST NOT abort the connection, and it
                 SHOULD make the information available to the
                 application.
 
                 DISCUSSION:
                      TCP could report the soft error condition directly
                      to the application layer with an upcall to the
                      ERROR_REPORT routine, or it could merely note the
                      message and report it to the application only when
                      and if the TCP connection times out.
 
            o    Destination Unreachable -- codes 2-4
 
                 These are hard error conditions, so TCP SHOULD abort
                 the connection.
 
            o    Time Exceeded -- codes 0, 1
 
                 This should be handled the same way as Destination
                 Unreachable codes 0, 1, 5 (see above).
 
            o    Parameter Problem
 
                 This should be handled the same way as Destination
		Unreachable codes 0, 1, 5 (see above).*/

		//
		// Map the first 8 bytes of TCP segment header
		// (8 bytes is all there is in an ICMPv4 packet)
		//
		// WARNING!!!   Any attempt to access fields other than
		//              SrcPort, DstPort, or Sequence WILL CAUSE
		//              BAD THINGS TO HAPPEN!
		//
		//
		pkt.Set(seg, info->iOffset, 8);
		if (!pkt.iHdr)
			{
			LOG(Log::Printf(_L("\ttcp Process() Invalid TCP header in ICMP")));
			seg.Free();
			break;
			}

		// Get port numbers from header
		info->iSrcAddr.SetPort(pkt.iHdr->SrcPort());
		info->iDstAddr.SetPort(pkt.iHdr->DstPort());

#ifdef _LOG
		TInetAddr(info->iSrcAddr).OutputWithScope(src); TInetAddr(info->iDstAddr).OutputWithScope(dst);
		LOG(Log::Printf(_L("\ttcp Process() ICMP reply to TCP segment {%S,%d} -> {%S,%d}"),
			&src, info->iSrcAddr.Port(), &dst, info->iDstAddr.Port()));
		LOG(Log::Printf(_L("\ttcp Process() ICMP type=%d code=%d param=%d received"),
			info->iType, info->iCode, info->iParameter));
#endif

		sap = (CProviderTCP6*)LocateSap(EMatchServerUnspecAddr,
			info->iVersion == 4 ? KAfInet : KAfInet6,
			info->iSrcAddr, info->iDstAddr);
		if (sap == NULL)
			{
			LOG(Log::Printf(_L("\ttcp Process() No socket. Discarding ICMP message")));
			seg.Free();
			break;
			}

		if (info->iIcmp == KProtocolInetIcmp)
			{
			//
			// ICMPv4 message processing
			//
			ip4.Set(seg, 0, TInet6HeaderIP4::MinHeaderLength());
			if (!ip4.iHdr)
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] Invalid IPv4 header in ICMP"), (TInt)sap));
				seg.Free();
				break;
				}
			icmpSender.SetAddress(ip4.iHdr->SrcAddr());
			switch (info->iType)
				{
			case KInet4ICMP_Unreachable:
/*
   Code           0 = net unreachable
                  1 = host unreachable
                  2 = protocol unreachable
                  3 = port unreachable
                  4 = fragmentation needed and DF set
                  5 = source route failed
                  6 = destination network unknown
                  7 = destination host unknown
                  8 = source host isolated
                  9 = communication with destination network administratively prohibited
                 10 = communication with destination host administratively prohibited
                 11 = network unreachable for type of service
                 12 = host unreachable for type of service
				*/
				switch (info->iCode)
					{
				case 2:
					err = KErrNoProtocolOpt;
					break;

				case 3:
					err = KErrCouldNotConnect;
					break;

				case 0:                          // Transient condition
					errMask = 0;
					// Fall through

				case 5: case 6: case 8: case 9:  // Persistent conditions
					err = KErrNetUnreach;
					break;

				case 1:                          // Transient condition
					errMask = 0;
					// Fall through

				case 7: case 10: case 12:        // Persistent conditions
					err = KErrHostUnreach;
					break;

				default:
					break;
					}
				break;

			case KInet4ICMP_SourceQuench:        // Do a slow start
				sap->SourceQuench();
				break;

			case KInet4ICMP_TimeExceeded:
				err = KErrNetUnreach;
				errMask = 0;
				break;

			case KInet4ICMP_ParameterProblem:
				err = KErrArgument;
				errMask = 0;
				break;

			default:
				break;
				}
			}
		else
			{
			//
			// ICMPv6 message processing
			//
			ip6.Set(seg, 0, TInet6HeaderIP::MinHeaderLength());
			if (!ip6.iHdr)
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] Invalid IPv6 header in ICMP"), (TInt)sap));
				seg.Free();
				break;
				}
			icmpSender.SetAddress(ip6.iHdr->SrcAddr());
			switch (info->iType)
				{
			case KInet6ICMP_Unreachable:
/*
   Code           0 - no route to destination
                  1 - communication with destination
                        administratively prohibited
                  2 - (not assigned)
                  3 - address unreachable
                  4 - port unreachable
				*/
				switch (info->iCode)
					{
				case 3:
					err = KErrHostUnreach;
					break;

				case 4:
					err = KErrCouldNotConnect;
					break;

				default:
					err = KErrNetUnreach;
					break;
					}
				break;

			case KInet6ICMP_PacketTooBig:
				//
				// Treat this as a soft error. The flow should
				// automatically adjust to changes in path MTU.
				//
				err = KErrTooBig;
				errMask = 0;
				break;

			case KInet6ICMP_TimeExceeded:
/*
   Code           0 - hop limit exceeded in transit
                  1 - fragment reassembly time exceeded
				*/
				err = KErrNetUnreach;
				errMask = 0;
				break;

			case KInet6ICMP_ParameterProblem:
/*
   Code           0 - erroneous header field encountered
                  1 - unrecognized Next Header type encountered
                  2 - unrecognized IPv6 option encountered
				*/
				err = KErrArgument;
				errMask = 0;
				break;

			default:
				break;
				}
			}

		if (err != KErrNone)
			{
			//
			// TCP only treats ICMP errors as hard errors if it's
			// setting up a new connection. We make ICMP spoofing
			// a little bit more difficult by checking the included
			// TCP sequence number. If the sequence number does not
			// match, we change the error to a soft error.
			//
			if (errMask && pkt.iHdr->Sequence() != sap->iSND.UNA)
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] TCP sequence mismatch. Converting to soft error"), (TInt)sap));
				errMask = 0;
				}

			LOG(Log::Printf(_L("\ttcp SAP[%u] Reporting ICMP error %d, mask %d.\r\n"), (TInt)sap, err, errMask));
			}

		// Store and report
		sap->IcmpError(err, errMask,
			info->iType, info->iCode,
			TInetAddr::Cast(info->iSrcAddr),
			TInetAddr::Cast(info->iDstAddr),
			TInetAddr::Cast(icmpSender));
		seg.Free();
		break;

	default:
		break;
		}
	}


//
// This routine generates and transmits a TCP control segment.
//
TInt CProtocolTCP6::SendControlSegment(RFlowContext *aFlow,
	const TSockAddr& aSrcAddr, const TSockAddr& aDstAddr,
	TUint8 aFlags, TTcpSeqNum aSeq, TTcpSeqNum aAck,
	TUint32 aWnd, TUint32 aUP)
	{
	LOG(Log::Printf(_L("\ttcp SendControlSegment")));

	RMBufSendPacket seg;
	RMBufSendInfo *info = NULL;
	TInt err;

	// Reserve space for IP+TCP headers and info.
	err = seg.Alloc(TInet6HeaderIP::MaxHeaderLength() + KTcpMinHeaderLength, iBufAllocator);
	if (err != KErrNone)
		return err;
	info = seg.NewInfo();
	if (!info)
		{
		seg.Free();
		return KErrNoMBufs;
		}

	// Leave space for the IP header.
	seg.TrimStart(TInet6HeaderIP::MaxHeaderLength());

	// Fill in info struct
	info->iProtocol = KProtocolInetTcp;
	info->iSrcAddr = aSrcAddr;
	info->iDstAddr = aDstAddr;
	info->iLength = KTcpMinHeaderLength;

	// Open flow context
	if (aFlow != NULL && aFlow->IsOpen())
		{
		err = info->iFlow.Open(*aFlow, info);
		}
	else
		{
		if ((err = info->iFlow.Open(NetworkService(), info->iDstAddr, info->iSrcAddr, info->iProtocol))
				== KErrNone)
			{
			info->iFlow.FlowContext()->SetOption(KSolInetIp, KSoKeepInterfaceUp, KInetOptionDisable);
			}
		}

	if (err != KErrNone)
		{
		// If at first you don't succeed... Well, life is sometimes unforgiving.
		info->iFlow.Close();
		seg.Free();
		return err;
		}

	//
	// Fill in TCP header. Note that the header length has already
	// been set and the checksum will be calculated later.
	//
	TTcpPacket pkt(seg);
	pkt.iHdr->SetHeaderLength(KTcpMinHeaderLength);
	pkt.iHdr->SetSrcPort(info->iSrcAddr.Port());
	pkt.iHdr->SetDstPort(info->iDstAddr.Port());
	pkt.iHdr->SetSequence(aSeq);
	pkt.iHdr->SetAcknowledgment(aAck);
	pkt.iHdr->SetControl(aFlags);
	pkt.iHdr->SetWindow(aWnd);
	pkt.iHdr->SetUrgent(aUP);

	ASSERT(info->iLength == seg.Length());

	//
	// Compute checksum and send the segment.
	//
	pkt.ComputeChecksum(seg, info);
	LOG(LogPacket('>', seg, info));
	seg.Pack();
	Send(seg);

	return KErrNone;
	}


TUint32 CProtocolTCP6::RandomSequence()
	{
	iRandomIncrement += Random(1000000);
	return ((User::TickCount() * iClockGranularity) >> 2) + iRandomIncrement;
	}


#ifdef _LOG
void CProtocolTCP6::LogPacket(char aDir, RMBufChain& aPacket, RMBufPktInfo *info, TInt aOffset)
	{
	RMBufPacketBase pkt;
	pkt.Assign(aPacket);
	TBool packed = EFalse;

	if (info == NULL)
		{
		info = pkt.Unpack();
		packed = ETrue;
		}

	TBuf<0x100> output;
	TBuf<50> src, dst;
	TTcpPacket seg(pkt, aOffset);
	TTcpSeqNum seq = seg.iHdr->Sequence();
	TUint32 len = info->iLength - seg.iHdr->HeaderLength() - aOffset;
	TTcpOptions opt;
	TTime now;
	now.UniversalTime();
#ifdef I64LOW
	TUint32 usec = I64LOW(now.Int64());
#else
	TUint32 usec = now.Int64().GetTInt();
#endif

	TInetAddr(info->iSrcAddr).OutputWithScope(src);
	TInetAddr(info->iDstAddr).OutputWithScope(dst);

	output.Format(_L("\t%6u.%03u "),
		usec / 1000000, (usec / 1000) % 1000);

	if (aDir == '>')
		output.AppendFormat(_L("%S.%u > %S.%u"),
		&src, info->iSrcAddr.Port(), &dst, info->iDstAddr.Port());
	else
		output.AppendFormat(_L("%S.%u < %S.%u"),
		&dst, info->iDstAddr.Port(), &src, info->iSrcAddr.Port());

	_LIT(KHdrSyn, "S");
	_LIT(KHdrFin, "F");
	_LIT(KHdrPsh, "P");
	_LIT(KHdrRst, "R");
	_LIT(KHdrEce, "E");
	_LIT(KHdrCwr, "W");
	_LIT(KHdrDot, ".");
	_LIT(KHdrNot, "");

	output.AppendFormat(_L(" %S%S%S%S%S%S%S %u:%u(%u) wnd=%u"),
		seg.iHdr->SYN() ? &KHdrSyn : &KHdrNot,
		seg.iHdr->FIN() ? &KHdrFin : &KHdrNot,
		seg.iHdr->PSH() ? &KHdrPsh : &KHdrNot,
		seg.iHdr->RST() ? &KHdrRst : &KHdrNot,
		seg.iHdr->ECE() ? &KHdrEce : &KHdrNot,
		seg.iHdr->CWR() ? &KHdrCwr : &KHdrNot,
		!(seg.iHdr->Control() & ~KTcpCtlACK) ? &KHdrDot : &KHdrNot,
		seq.Uint32(), (seq+len).Uint32(), len, seg.iHdr->Window());

	if (seg.iHdr->ACK())
		output.AppendFormat(_L(" ack=%u"), (seg.iHdr->Acknowledgment()).Uint32() );

	if (seg.iHdr->URG())
		output.AppendFormat(_L(" urg=%u"), seg.iHdr->Urgent());

//	output.AppendFormat(_L("\r\n"));
	Log::Write(output);

	if (seg.iHdr->Options(opt) && opt.Length() > 0)
		{
		TUint32 tsVal, tsEcr;
		TInt blockCount = opt.SackBlocks().Count();
		output.Format(_L("\t  options ["));
		if (opt.MSS() >= 0)
			output.AppendFormat(_L(" MSS=%u"), opt.MSS());
		if (opt.SackOk())
			output.AppendFormat(_L(" SackOk"));
		if (opt.TimeStamps(tsVal, tsEcr))
			output.AppendFormat(_L(" TS=%u,%u"), tsVal, tsEcr);
		if (opt.WindowScale())
			output.AppendFormat(_L(" WS=%d"), opt.WindowScale()-1);
			
		if (blockCount)
			{
			SequenceBlockQueueIter iter(opt.SackBlocks());
			SequenceBlock *block;

			output.Append(_L(" SACK="));
			iter.SetToFirst();
			while (block = iter++, block != NULL)
				output.AppendFormat(_L("%u-%u%s"),
				(block->iLeft).Uint32(),
				(block->iRight).Uint32(), --blockCount ? "," : "");
			}
		output.AppendFormat(_L(" ]"));
		Log::Write(output);
		}

	if (packed)
		pkt.Pack();

	aPacket.Assign(pkt);
	}
#endif

