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
// ip6.h - IPv6 protocol
//



/**
 @internalComponent
*/
#ifndef __IP6_H__
#define __IP6_H__

#include <es_prot.h>
#include <in_sock.h>
#include <es_prot_internal.h>

//	***
//	IP6
//	***
//	A minimal static interface for setting up the IP6/IP4
//	dual stack protocol instances
//
class CIfManager;
class CProtocolInet6Base;
class IP6
	{
public:
	// Implemented in PRT...
	static CProtocolBase *NewL(CIfManager *aFamily, TInt aVersion = KProtocolInet6Ip);
	static void Identify(TServerProtocolDesc &aEntry, TInt aVersion = KProtocolInet6Ip);
	// Implemented in SAP (and called from PRT NewSAPL)
	static CServProviderBase *NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol, TInt aId);
	};

//

const TInt  KIP6MaxSockets			= 0; //KUnlimitedSockets;
const TUint KIP6ServiceTypeInfo		= ESocketSupport | ETransport | EPreferMBufChains | ENeedMBufs | EUseCanSend;  // | ESocketSupport
const TUint KIP6ServiceInfo			= KSIConnectionLess | KSIMessageBased | KSIBroadcast | KSIPeekData | KSIGracefulClose | KSIRequiresOwnerInfo;
const TUint KIP6NameServiceInfo		= KNSNameResolution | KNSRequiresConnectionStartup; // | KNSServiceResolution | KNSInfoDatabase;
//

#endif
