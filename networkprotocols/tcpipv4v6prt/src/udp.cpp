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
// udp.cpp - UDP protocol for IPv6/IPv4
//

#include "udp.h"
#include <icmp6_hdr.h>
#include <ip4_hdr.h>
#include <in6_if.h>
#include "tcpip_ini.h"
#include "inet6log.h"

#define SYMBIAN_NETWORKING_UPS

//
// UDP Protocol Description
//

void CProtocolUDP6::Describe(TServerProtocolDesc &aDesc)
	{
	aDesc.iName		  = _S("udp");
	aDesc.iAddrFamily	  = KAfInet;
	aDesc.iSockType	  = KSockDatagram;
	aDesc.iProtocol	  = KProtocolInetUdp;
	aDesc.iVersion	  = TVersion(KInet6MajorVersionNumber,
								KInet6MinorVersionNumber,
								KInet6BuildVersionNumber);
	aDesc.iByteOrder	  = EBigEndian;
	aDesc.iServiceInfo	  = KSIConnectionLess | KSIDatagram |
							KSIGracefulClose | KSIPeekData |
							KSIRequiresOwnerInfo;
	aDesc.iNamingServices	  = KNSNameResolution | KNSRequiresConnectionStartup;
	aDesc.iSecurity	  = KSocketNoSecurity;
	aDesc.iMessageSize	  = 65536-128; /*KSocketMessageSizeUndefined;*/
	aDesc.iServiceTypeInfo  = ESocketSupport | ETransport |
							EPreferMBufChains | ENeedMBufs |
							EUseCanSend;
	aDesc.iNumSockets	  = KUnlimitedSockets;
	}



CProtocolUDP6::CProtocolUDP6()
	{
	__DECLARE_NAME(_S("CProtocolUDP6"));
	}


CProtocolUDP6::~CProtocolUDP6()
	{
	}


CServProviderBase* CProtocolUDP6::NewSAPL(TUint aSockType)
	{
	LOG(Log::Printf(_L("NewSAPL\tudp SockType=%d"), aSockType));
	if (aSockType!=KSockDatagram)
		User::Leave(KErrNotSupported);
	CProviderUDP6 *sap = new (ELeave) CProviderUDP6(this);
	CleanupStack::PushL(sap);
	sap->InitL();
	CleanupStack::Pop();
	LOG(Log::Printf(_L("NewSAPL\tudp SAP[%u] OK"), (TUint)sap));
	return sap;
	}

void CProtocolUDP6::InitL(TDesC& aTag)
	{
	CProtocolInet6Transport::InitL(aTag);
	}


void CProtocolUDP6::StartL()
	{
	CProtocolInet6Transport::StartL();

	iRecvBuf = GetIniValue(TCPIP_INI_UDP, TCPIP_INI_UDP_RECV_BUF, KUdpDefaultRecvBuf, 1, KMaxTInt);

	// Flags on by default
	iWaitNif = GetIniValue(TCPIP_INI_UDP, TCPIP_INI_UDP_WAIT_NIF, 1, 0, 1);
	}


void CProtocolUDP6::Identify(TServerProtocolDesc *aInfo) const
	{
	Describe(*aInfo);
	}

TInt CProtocolUDP6::Send(RMBufChain& aPacket,CProtocolBase* /*aSourceProtocol=NULL*/)
	{
	LOG(Log::Printf(_L("\tudp Send(%d bytes)"), RMBufSendPacket::PeekInfoInChain(aPacket)->iLength));
	return iNetwork->Send(aPacket);
	}


void CProtocolUDP6::Process(RMBufChain& aPacket, CProtocolBase* /*aSourceProtocol*/)
	{
	RMBufRecvPacket datagram;
	datagram.Assign(aPacket);
	RMBufRecvInfo* info = datagram.Unpack();
	TInet6PacketUDP pkt(datagram, info->iOffset);
	TInet6Packet<TInet6HeaderIP4> ip4;
	TInet6Packet<TInet6HeaderIP>  ip6;
	TInt err = KErrNone;
	TInt version(0);
	
	CProviderUDP6 *sap = NULL;
	TInetAddr icmpSender;

#ifdef _LOG
	TBuf<50> src, dst;
	LOG(Log::Printf(_L("\tudp Process(%d bytes)"), datagram.Length()));
#endif

	ASSERT(info->iLength == datagram.Length());

	//
	// Preliminary checking for both UDP and ICMP
	//
	switch (info->iProtocol)
		{
	case KProtocolInetUdp:
		// Sanity checking
		if (!pkt.iHdr)
			{
			LOG(Log::Printf(_L("\tudp Process() Runt packet discarded")));
			datagram.Free();
			return;
			}

#ifdef _LOGx
		TInetAddr(info->iSrcAddr).OutputWithScope(src); TInetAddr(info->iDstAddr).OutputWithScope(dst);
		LOG(Log::Printf(_L("\tudp Process(%d bytes) {%S,%d} -> {%S,%d}"),
			datagram.Length(), &src, pkt.iHdr->SrcPort(), &dst, pkt.iHdr->DstPort()));
#endif

		// More sanity checking.
		if (pkt.iHdr->SrcPort() == KInetPortNone || pkt.iHdr->DstPort() == KInetPortNone)
			{
			LOG(Log::Printf(_L("\tudp Process() Zero port. Packet discarded.")));
			datagram.Free();
			return;
			}
		break;

	default:
		LOG(Log::Printf(_L("\tudp Process() Unknown protocol. Packet discarded.")));
		datagram.Free();
		return;
		}

	switch (info->iIcmp)
		{
	case 0:

		// Check source address
		if (!TInetAddr::Cast(info->iSrcAddr).IsUnicast() && !TInetAddr::Cast(info->iSrcAddr).IsUnspecified())
			{
			LOG(Log::Printf(_L("\tudp Process() Invalid source address. Packet discarded.")));
			datagram.Free();
			break;
			}

		// Sanity checking
		if (info->iLength != info->iOffset + pkt.iHdr->Length())
			{
			LOG(Log::Printf(_L("\tudp Process() Size mismatch. Packet discarded.")));
			datagram.Free();
			break;
			}

		// Process UDP checksum. Check for no checksum (legal in UDP/IPv4)
		if (pkt.iHdr->Checksum() ? !pkt.VerifyChecksum(datagram, info, info->iOffset) : info->iVersion != 4)
			{
			LOG(Log::Printf(_L("\tudp Process() Bad checksum. Packet discarded.")));
			datagram.Free();
			return;
			}

		//LOG(Log::Printf(_L("Verified UDP checksum %04x."), pkt.iHdr->Checksum()));

		// Get port numbers from header
		info->iSrcAddr.SetPort(pkt.iHdr->SrcPort());
		info->iDstAddr.SetPort(pkt.iHdr->DstPort());

#ifdef _LOG
		TInetAddr(info->iSrcAddr).OutputWithScope(src); TInetAddr(info->iDstAddr).OutputWithScope(dst);
		LOG(Log::Printf(_L("\tudp Process() UDP datagram {%S,%d} -> {%S,%d}"),
			&src, info->iSrcAddr.Port(), &dst, info->iDstAddr.Port()));
#endif

		// Advance iOffset over the UDP header
		info->iOffset += pkt.iHdr->HeaderLength();

		//
		// Replicate packets to all receivers.
		// If there is only a single receiver,
		// no extra packet copies will be made.
		//
		CProviderUDP6 *s;
		version = info->iVersion == 4 ? KAfInet : KAfInet6;

		// Locate hash bucket and traverse hash chain
		s = (CProviderUDP6*)CProtocolInet6Base::LocateProvider(info->iDstAddr.Port());
		// Locate best match if exists
	    if (s)
	    	{
	    	if (!s->iNextSAP) // only one possible receiver in hash entry
	    		{
				// Locate best match
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
				sap = (CProviderUDP6*)LocateSap(EMatchServerUnspecAddr,
					version, info->iDstAddr, info->iSrcAddr, s);	    		
#else
				sap = (CProviderUDP6*)LocateSap(EMatchServerUnspecAddr,
					version, info->iDstAddr, info->iSrcAddr, s,info->iInterfaceIndex);	    		
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
	    		}
	    	else // Multiple possible receivers in hash entry
	    		{
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
				sap = (CProviderUDP6*)LocateSap(EMatchServerSpecAddr, //Since this is UDP we are only interested in the dst Ip addr
					version, info->iDstAddr, info->iSrcAddr, s);
#else
				sap = (CProviderUDP6*)LocateSap(EMatchServerSpecAddr, //Since this is UDP we are only interested in the dst Ip addr
					version, info->iDstAddr, info->iSrcAddr, s,info->iInterfaceIndex);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
			
				if (!sap) // no explicit connections
					{
					for (; s != NULL; s = (CProviderUDP6*)s->iNextSAP)
						{
						// Locate next match
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
						s = (CProviderUDP6*)LocateSap(EMatchServerUnspecAddr,
							version, info->iDstAddr, info->iSrcAddr, s);
#else
						s = (CProviderUDP6*)LocateSap(EMatchServerUnspecAddr,
							version, info->iDstAddr, info->iSrcAddr, s,info->iInterfaceIndex);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
						// No match?
						if (s == NULL)
							break;

#ifdef SYMBIAN_NETWORKING_UPS
						if (!s->HasNetworkServices() && (!s->ConnectionInfoSet())&& (info->iFlags & KIpLoopbackPacket) == 0)
#else
						if (!s->HasNetworkServices() && (info->iFlags & KIpLoopbackPacket) == 0)
#endif
							{
							LOG(Log::Printf(_L("\tudp SAP[%u] Not allowed to receive external packets"), (TInt)s));
							continue;
							}

						// More than one receiver found? Copy the packet.
						if (sap != NULL)
							{
							RMBufRecvPacket copy;
							TRAPD(err, datagram.CopyL(copy);datagram.CopyInfoL(copy));
							if (err == KErrNone)
								{
								copy.Pack();
								sap->Process(copy);
								}
							copy.Free();
							}
						// Remember last match
						sap = s;
						}
					}
	    		}
	    	}
		// No match? Respond with ICMP
		if (sap == NULL)
			{
            /* From RFC1122:
			If a datagram arrives addressed to a UDP port for which
			there is no pending LISTEN call, UDP SHOULD send an ICMP
			Port Unreachable message.*/
			LOG(LogProviders(info->iDstAddr.Port()));

			// Include UDP header in ICMP payload
			info->iOffset -= pkt.iHdr->HeaderLength();

			if (info->iVersion == 4)
				iNetwork->Icmp4Send(datagram, KInet4ICMP_Unreachable, 3 /* port unreachable*/);
			else
				iNetwork->Icmp6Send(datagram, KInet6ICMP_Unreachable, 4 /* port unreachable*/);
			}
		else
			{
#ifdef SYMBIAN_NETWORKING_UPS
			if (!sap->HasNetworkServices() && (!sap->ConnectionInfoSet()) && (info->iFlags & KIpLoopbackPacket) == 0)
#else
			if (!sap->HasNetworkServices() && (info->iFlags & KIpLoopbackPacket) == 0)
#endif
				{
				LOG(Log::Printf(_L("\tudp SAP[%u] Not allowed to receive external packets"), (TInt)sap));
				datagram.Free();
				break;
				}
			const TInt iface = info->iOriginalIndex;

			// Punt the packet to the SAP
			datagram.Pack();
			sap->Process(datagram);

			// Reset idle timers
			Interfacer()->PacketAccepted(iface);
			}
		break;

	case KProtocolInetIcmp:
	case KProtocolInet6Icmp:

		// Get port numbers from header
		info->iSrcAddr.SetPort(pkt.iHdr->SrcPort());
		info->iDstAddr.SetPort(pkt.iHdr->DstPort());

#ifdef _LOG
		TInetAddr(info->iSrcAddr).OutputWithScope(src); TInetAddr(info->iDstAddr).OutputWithScope(dst);
		LOG(Log::Printf(_L("\tudp Process() ICMP reply to UDP datagram {%S,%d} -> {%S,%d}"),
			&src, info->iSrcAddr.Port(), &dst, info->iDstAddr.Port()));
		LOG(Log::Printf(_L("\tudp Process() ICMP type=%d code=%d param=%d received."),
			info->iType, info->iCode, info->iParameter));
#endif

		sap = (CProviderUDP6*)LocateSap(EMatchServerUnspecAddr,
			info->iVersion == 4 ? KAfInet : KAfInet6,
			info->iSrcAddr, info->iDstAddr);
		if (sap == NULL)
			{
			LOG(Log::Printf(_L("\tudp Process() No socket. Discarding ICMP message.")));
			datagram.Free();
			break;
			}

		err = KErrUnknown;
		if (info->iIcmp == KProtocolInetIcmp)
			{
			//
			// ICMPv4 message processing
			//
			ip4.Set(datagram, 0, TInet6HeaderIP4::MinHeaderLength());
			if (!ip4.iHdr)
				{
				LOG(Log::Printf(_L("\tudp Process() Invalid IPv4 header in ICMP.")));
				datagram.Free();
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

				case 4:
					err = KErrTooBig;
					break;

				case 0:
				case 5:
				case 6:
				case 8:
				case 9:
				case 11:
					err = KErrNetUnreach;
					break;

				case 1:
				case 7:
				case 10:
				case 12:
					err = KErrHostUnreach;
					break;

				default:
					break;
					}
				break;

			case KInet4ICMP_SourceQuench:
				break;

			case KInet4ICMP_TimeExceeded:
				err = KErrTimedOut;
				break;

			case KInet4ICMP_ParameterProblem:
				err = KErrArgument;
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
			ip6.Set(datagram, 0, TInet6HeaderIP::MinHeaderLength());
			if (!ip6.iHdr)
				{
				LOG(Log::Printf(_L("\tudp  Process() Invalid IPv6 header in ICMP.")));
				datagram.Free();
				break;
				}
			icmpSender.SetAddress(ip6.iHdr->SrcAddr());
			switch (info->iType)
				{
			case KInet6ICMP_Unreachable:
/*
   Code           0 - no route to destination
                  1 - communication with destination administratively prohibited
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
				err = KErrTooBig;
				break;

			case KInet6ICMP_TimeExceeded:
/*
   Code           0 - hop limit exceeded in transit
                  1 - fragment reassembly time exceeded
				*/
				err = KErrTimedOut;
				break;

			case KInet6ICMP_ParameterProblem:
/*
   Code           0 - erroneous header field encountered
                  1 - unrecognized Next Header type encountered
                  2 - unrecognized IPv6 option encountered
				*/
				err = KErrArgument;
				break;

			default:
				break;
				}
			}

		// Store and report
		sap->IcmpError(err, MSocketNotify::EErrorSend|MSocketNotify::EErrorRecv,
			info->iType, info->iCode,
			TInetAddr::Cast(info->iSrcAddr),
			TInetAddr::Cast(info->iDstAddr),
			TInetAddr::Cast(icmpSender));
		datagram.Free();
		break;

	default:
		datagram.Free();
		break;
		}
	}
