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
// in_fmly.cpp - internet protocol family
//

#include "icmp6.h"
#include "ip6.h"
#include "udp.h"
#include "tcp.h"
#include "res.h"
#include "iface.h"
#include "in_fmly.h"
#include "rawip.h"

//
// CProtocolFamilyInet6
// ********************
// Internal to this implementation, no need to be defined in headers!
//
class CProtocolFamilyInet6 : public CProtocolFamilyBase
	{
public:
	CProtocolFamilyInet6();
	virtual ~CProtocolFamilyInet6();

	virtual TInt Install();
	virtual TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	virtual CProtocolBase* NewProtocolL(TUint aSockType,TUint aProtocol);
private:
	CIfManager *iInterfacer;	// Interface Manager (created in Install())
	};






// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase *Install(void); }
EXPORT_C CProtocolFamilyBase *Install()
	{
	return new CProtocolFamilyInet6;
	}



void Panic(TInet6Panic aPanic)
	{
	_LIT(KTcpIp6, "TCPIP6");
	User::Panic(KTcpIp6, aPanic);
	}


CProtocolFamilyInet6::CProtocolFamilyInet6()
	{
	}

CProtocolFamilyInet6::~CProtocolFamilyInet6()
	{
	delete iInterfacer;
	}

//
// CProtocolFamilyInet6::Install
// *****************************
// The "non-leaving" Install() is called from ESOCK
//
TInt CProtocolFamilyInet6::Install()
	{
	TRAPD(err, iInterfacer = CIfManager::NewL());
	return err;
	}


TUint CProtocolFamilyInet6::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
	// This function should be a leaving fn
	// apparently it is OK for it to leave
	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[8]; // Esock catches this leave

	IP6::Identify(p[0], KProtocolInet6Ip);
	ICMP6::Identify(p[1], KProtocolInet6Icmp);
	CProtocolUDP6::Describe(p[2]);
	CProtocolTCP6::Describe(p[3]);
	RES::Identify(p[4]);
	//
	// IPv4 versions of dual stack at end for the time being...
	//
	IP6::Identify(p[5], KProtocolInetIp);
	ICMP6::Identify(p[6], KProtocolInetIcmp);
	RAWIP::Identify(p[7]);
	aProtocolList = p;
	return 8;
	}

CProtocolBase* CProtocolFamilyInet6::NewProtocolL(TUint aSockType,TUint aProtocol)
	{
	if (iInterfacer)
		{
		// Is this test for Undefined protocol required? Does SocketServer
		// already do this. Although, how can it know which of the datagram
		// protocols (IP, IP6, ICMP, ICMP6 and UDP) is actually the preferred
		// one...? -- msa
		if (aProtocol == KUndefinedProtocol)
			{
			if (aSockType == KSockDatagram)
				aProtocol = KProtocolInetUdp;
			else if (aSockType == KSockStream)
				aProtocol = KProtocolInetTcp;
			else
				goto no_support;
			}

		switch (aProtocol)
			{
		case KProtocolInet6Ip:	// IPv6 support for the dual stack
		case KProtocolInetIp:	// IPv4 support for the dual stack
			if (aSockType == KSockDatagram)
				return IP6::NewL(iInterfacer, aProtocol);
			break;
		case KProtocolInetRawIp:
			if (aSockType == KSockRaw)
				return RAWIP::NewL();
			break;
		case KProtocolInet6Icmp:
		case KProtocolInetIcmp:
			if (aSockType == KSockDatagram)
				return ICMP6::NewL(aProtocol);
			break;
		case KProtocolInetUdp:
			if (aSockType == KSockDatagram)
				return new (ELeave) CProtocolUDP6;
			break;
		case KProtocolInetTcp:
			if (aSockType == KSockStream)
				return new (ELeave) CProtocolTCP6;
			break;
		case KProtocolInet6Res:
			return RES::NewL(iInterfacer);
		default:
			break;
			}
		}
no_support:
	User::Leave(KErrNotSupported);
	// NOTREACHED
	return 0;	// To keep compiler happy...
	}
