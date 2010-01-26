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
// icmp.cpp - dual IPv4/IPv6 ICMP protocol
// This implements two separate protocol modules (icmp and
// icmp6). Unlike other transport layers (tcp and udp), ICMP has
// different protocol identifier in IPv4 and IPv4. Thus, two separate
// protocol instances is a requirement.
//

#include "inet6log.h"
#include "icmp6.h"
#include "in_net.h"
#include <icmp6_hdr.h>
#include <in_pkt.h>

_LIT(KIcmpName, "icmp");
_LIT(KIcmp6Name, "icmp6");

//
//	CProtocolICMP
//	*************
//	The implementation and methods of the CProtocolICMP4/6 is totally internal
//	to this module. No other module needs to be aware of this.
//	Thus the class definition is included here.
//
class CProtocolICMP : public CProtocolInet6Network
	{
public:
	CProtocolICMP(TInt aProtocol);
	virtual ~CProtocolICMP();
	virtual CServProviderBase *NewSAPL(TUint aProtocol);
	virtual void Process(RMBufChain &aPacket,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt Send(RMBufChain &aPacket,CProtocolBase* aSourceProtocol=NULL);
	virtual void Identify(TServerProtocolDesc *) const;
protected:
	const TInt iProtocol;
	};

//	*****
//	ICMP6
//	*****
CProtocolBase *ICMP6::NewL(TInt aVersion)
	{
	return new (ELeave) CProtocolICMP(aVersion);
	}

void ICMP6::Identify(TServerProtocolDesc &aEntry, TInt aVersion)
	{
	if (aVersion == STATIC_CAST(TInt, KProtocolInetIcmp))
		{
		aEntry.iName = KIcmpName;
		aEntry.iProtocol=KProtocolInetIcmp;
		}
	else
		{
		aEntry.iName = KIcmp6Name;
		aEntry.iProtocol=KProtocolInet6Icmp;
		}
	aEntry.iAddrFamily=KAfInet;
	aEntry.iSockType=KSockDatagram;
	aEntry.iVersion=TVersion(KInet6MajorVersionNumber, KInet6MinorVersionNumber, KInet6BuildVersionNumber);
	aEntry.iByteOrder=EBigEndian;
	aEntry.iServiceInfo=KICMP6ServiceInfo;
	aEntry.iNamingServices=KICMP6NameServiceInfo;
	aEntry.iSecurity=KSocketNoSecurity;
	aEntry.iMessageSize=KICMP6MaxDatagramSize;
	aEntry.iServiceTypeInfo=KICMP6ServiceTypeInfo;
	aEntry.iNumSockets=KICMP6MaxSockets;
	}

//

//
//	CProtocolICMP* constructors and destructors
//	*******************************************

CProtocolICMP::CProtocolICMP(TInt aId) : iProtocol(aId)
	{
	__DECLARE_NAME(_S("CProtocolICMP"));
	}

CProtocolICMP::~CProtocolICMP()
	{
	}

//
// CProtocolICMP::NewSAPL
//	Create a new instance of a CServProviderBase (SAP) for the
//	socket manager. The caller is responsible for the bookkeeping
//	and destruction of this created object!
//
CServProviderBase* CProtocolICMP::NewSAPL(TUint aSockType)
	{
	return ICMP6::NewSAPL(aSockType, this, iProtocol);
	}

void CProtocolICMP::Identify(TServerProtocolDesc *aInfo) const
	{
	ICMP6::Identify(*aInfo, iProtocol);
	}

//
// CProtocolICMP::Send()
//
//	Pass the packet as is to the IP layer. This method is
//	supposed to be used by the ICMP Service provider modules
//	to forward their packets down the stack.
//
//	ICMP makes no checks on validity or format of the packet
//
TInt CProtocolICMP::Send(RMBufChain &aPacket,CProtocolBase* /*aSourceProtocol*/)
	{
	return iNetwork->Send(aPacket);
	}


void CProtocolICMP::Process(RMBufChain &aPacket,CProtocolBase * /*aSourceProtocol*/)
	{
	RMBufRecvPacket packet;
	packet.Assign(aPacket);

	for (;;) // ONLY FOR NEAT BREAK EXITS -- NOT A LOOP
		{
		RMBufRecvInfo *const info = packet.Unpack();
		if (info == NULL)
			break;

		LOG(Log::Printf(_L("\t%S Process(%d bytes)"), &ProtocolName(), info->iLength));
		if (info->iIcmp)
			{
			// This packet is actually an ICMP error report to an ICMP sent by
			// this host. The iOffset points to "inner" ICMP, but tradionally
			// ICMP applications expect the outer RAW ICMP's, thus, change
			// the iOffset and other fields accordingly
			info->iOffset = info->iOffsetIp - 8 /* 8 = ICMP header size*/;
			info->iProtocol = info->iIcmp;
			// *NOTE*
			//	The following assumes that outer IP tunnels have been removed
			//	from the packet. Then, it can be assumed that the IP header
			//	related to ICMP error is at the beginning of the packet (offset=0).
			info->iOffsetIp = 0;
			const MInterface *const mi = NetworkService()->Interfacer()->Interface(info->iInterfaceIndex);
			if (mi == NULL)
				break;
			TIpHeader *const ip = ((RMBufPacketPeek &)packet).GetIpHeader();
			if (ip == NULL)
				break;
			TInetAddr &src = TInetAddr::Cast(info->iSrcAddr);
			TInetAddr &dst = TInetAddr::Cast(info->iDstAddr);
			if (ip->ip4.Version() == 4)
				{
				src.SetV4MappedAddress(ip->ip4.SrcAddr());
				dst.SetV4MappedAddress(ip->ip4.DstAddr());
				}
			else
				{
				src.SetAddress(ip->ip6.SrcAddr());
				dst.SetAddress(ip->ip6.DstAddr());
				}
			const TIp6Addr &src_ip = src.Ip6Address();
			const TIp6Addr &dst_ip = dst.Ip6Address();
			src.SetScope(mi->Scope((TScopeType)(src_ip.Scope()-1)));
			dst.SetScope(mi->Scope((TScopeType)(dst_ip.Scope()-1)));
			}

		// Check "payload length", and don't even try delivering truncated ICMP packets.
		if (info->iLength - info->iOffset < TInet6HeaderICMP::MinHeaderLength())
			break;

		Deliver(packet);
		// *ALWAYS BREAK OUT FROM LOOP*
		break;
		}
	packet.Free();
	}
