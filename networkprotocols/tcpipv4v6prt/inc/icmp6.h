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
// icmp6.h - ICMPv6 protocol
//



/**
 @internalComponent
*/
#ifndef __ICMP6_H__
#define __ICMP6_H__

#include <es_prot.h>
#include <in_sock.h>
#include <icmp6_hdr.h>	// Not used by this header--remove?
#include <es_prot_internal.h>

//
//	*****
//	ICMP6
//	*****
//
class CProtocolInet6Base;
class ICMP6
	{
public:
	// Implemented in PRT...
	static CProtocolBase *NewL(TInt aVersion = KProtocolInet6Icmp);
	static void Identify(TServerProtocolDesc &aEntry, TInt aVersion = KProtocolInet6Icmp);
	// Implemented in SAP (and called from PRT NewSAPL)
	static CServProviderBase *NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol, TInt aId);
	};

const TInt  KICMP6MaxSockets		= KUnlimitedSockets;
const TInt  KICMP6MaxDatagramSize	= 0xffff-128;	// ...just pick some number...
const TUint KICMP6ServiceTypeInfo	= ESocketSupport | ETransport | EPreferMBufChains | ENeedMBufs | EUseCanSend;
const TUint KICMP6ServiceInfo		= KSIConnectionLess | KSIMessageBased | KSIBroadcast | KSIPeekData | KSIGracefulClose | KSIRequiresOwnerInfo;
const TUint KICMP6NameServiceInfo	= KNSNameResolution | KNSRequiresConnectionStartup; // | KNSServiceResolution | KNSInfoDatabase;
//

#endif
