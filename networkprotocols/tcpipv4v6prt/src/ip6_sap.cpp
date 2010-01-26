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
// ip6_sap.cpp - IPv6 service access point
//

#include "ip6.h"
#include <ip6_hdr.h>
#include "in_net.h"

class CProviderIP6 : public CProviderInet6Network
	{
public:
	CProviderIP6(CProtocolInet6Base* aProtocol);
	virtual TInt DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint aOptions, TUint aOffset);
	virtual TInt SecurityCheck(MProvdSecurityChecker *aChecker);
	};

//
// IP6::NewSAPL
// ************
CServProviderBase *IP6::NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol, TInt /*aId*/)
	{
	LOG(Log::Printf(_L("NewSAPL\t%S SockType=%d"), &aProtocol->ProtocolName(), aSockType));
	if (aSockType != KSockDatagram)
		User::Leave(KErrNotSupported);
	CProviderIP6 *provider = new (ELeave) CProviderIP6(aProtocol);
	CleanupStack::PushL(provider);
	provider->InitL();
	CleanupStack::Pop();
	LOG(Log::Printf(_L("NewSAPL\t%S SAP[%u] OK"), &aProtocol->ProtocolName(), (TInt)provider));
	return provider;
	}

//

CProviderIP6::CProviderIP6(CProtocolInet6Base* aProtocol) : CProviderInet6Network(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderIP6"));
	}

TInt CProviderIP6::DoWrite(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TUint /*aOptions*/, TUint /*aOffset*/)
	{
	iFlow.SetNotify(this);
	if (aInfo.iSrcAddr.Family())
		iFlow.SetLocalAddr(aInfo.iSrcAddr);
	if (aInfo.iDstAddr.Family())
		iFlow.SetRemoteAddr(aInfo.iDstAddr);

	const TInt status = aPacket.Info()->iFlow.Open(iFlow, aPacket.Info());
	if (status == KErrNone)
		{
		// Should compute the IPv6 checksum for the "upper" layer,
		// if requested by the application (advanced api option in
		// IPv6. Not for IPv4.)
		}
	return status;
	}

TInt CProviderIP6::SecurityCheck(MProvdSecurityChecker *aChecker)
	{
	const TInt res = CProviderInet6Network::SecurityCheck(aChecker);
	if (res == KErrNone)
		return CheckPolicy(KPolicyNetworkControl, "TCPIP IP SAP");
	return res;
	}
