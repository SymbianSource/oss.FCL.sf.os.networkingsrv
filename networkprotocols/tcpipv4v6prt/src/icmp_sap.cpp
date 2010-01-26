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
// icmp_sap.cpp - ICMP service access point
//

#include "icmp6.h"
#include <icmp6_hdr.h>
#include <in_pkt.h>
#include <in_chk.h>
#include "in_net.h"
//

class CProviderICMP6 : public CProviderInet6Network
	{
public:
	CProviderICMP6(CProtocolInet6Base* aProtocol, TInt aProtocolId);
	virtual TInt DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint aOptions, TUint aOffset);
	virtual TInt SetLocalName(TSockAddr &aAddr);
	virtual TInt SecurityCheck(MProvdSecurityChecker *aChecker);
	};

CServProviderBase *ICMP6::NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol, TInt aId)
	{
	LOG(Log::Printf(_L("NewSAPL\t%S SockType=%d"), &aProtocol->ProtocolName(), aSockType));
	if (aSockType != KSockDatagram)
		User::Leave(KErrNotSupported);
	CProviderICMP6 *provider = new (ELeave) CProviderICMP6(aProtocol, aId);
	CleanupStack::PushL(provider);
	provider->InitL();
	CleanupStack::Pop();
	LOG(Log::Printf(_L("NewSAPL\t%S SAP[%u] OK"), &aProtocol->ProtocolName(), (TInt)provider));
	return provider;
	}

//

CProviderICMP6::CProviderICMP6(CProtocolInet6Base* aProtocol, TInt aProtocolId) :
	CProviderInet6Network(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderICMP6"));

	iProtocolId = aProtocolId;	// either ICMPv4 or ICMPv6 id
	}

TInt CProviderICMP6::DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint /*aOptions*/, TUint aOffset)
	{
	TInet6Checksum<TInet6HeaderICMP> icmp(aPacket, aOffset);
	if (icmp.iHdr == NULL)
		return KErrInet6ShortPacket;

	iFlow.SetIcmpType(icmp.iHdr->Type(), icmp.iHdr->Code());
	iFlow.SetNotify(this);
	if (aInfo.iSrcAddr.Family())
		iFlow.SetLocalAddr(aInfo.iSrcAddr);
	if (aInfo.iDstAddr.Family())
		iFlow.SetRemoteAddr(aInfo.iDstAddr);

	const TInt status = aInfo.iFlow.Open(iFlow, &aInfo);
	if (status == KErrNone)
		{
		// Compute the checksum here. This means that currently
		// the application cannot compute the checksum into
		// ICMP header. This may be a drawback for some situations
		// and perhaps some flag is introduced later -- msa
		//
		// IPv4 ICMP checksum does not use "pseudoheader" => pass NULL for info when IPv4!
		icmp.ComputeChecksum(aPacket, iProtocolId == STATIC_CAST(TInt, KProtocolInetIcmp) ? NULL : &aInfo, aOffset);
		}
	return status;
	}

//
// Need to override the network SetLocalName because by default
// the network deliver filters by the port number (compared to
// the protocol number).
//
TInt CProviderICMP6::SetLocalName(TSockAddr &aAddr)
	{
	// To prevent accidental non-zero values of port messing things up,
	// make sure the local port field is zero before doing the bind.
	//
	aAddr.SetPort(0);
	return CProviderInet6Network::SetLocalName(aAddr);
	}
	
// For now, require network control from ICMP sockets, because they
// also see all incoming ICMP's, including any bounced packets from
// other applications.
TInt CProviderICMP6::SecurityCheck(MProvdSecurityChecker *aChecker)
	{
	const TInt res = CProviderInet6Network::SecurityCheck(aChecker);
	if (res == KErrNone)
		return CheckPolicy(KPolicyNetworkControl, "TCPIP ICMP SAP");
	return res;
	}


