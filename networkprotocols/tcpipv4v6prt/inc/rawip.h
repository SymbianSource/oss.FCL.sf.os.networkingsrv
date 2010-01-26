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
// rawip.h - Raw IP protocol
//



/**
 @internalComponent
*/
#ifndef __RAWIP_H__
#define __RAWIP_H__

#include <es_prot.h>
#include <in_sock.h>
#include <es_prot_internal.h>

const TUint KProtocolInetRawIp	= 0xF02;

//
//	*****
//	RAWIP
//	*****
//
class CProtocolInet6Base;
class RAWIP
	{
public:
	// Implemented in PRT...
	static CProtocolBase *NewL();
	static void Identify(TServerProtocolDesc &aEntry);
	// Implemented in SAP (and called from PRT NewSAPL)
	static CServProviderBase *NewSAPL(TUint aSockType, CProtocolInet6Base *aProtocol);
	};

const TInt  KRAWIPMaxSockets		= KUnlimitedSockets;
const TInt  KRAWIPMaxDatagramSize	= 0xffff-128;	// ...just pick some number...
const TUint KRAWIPServiceTypeInfo	= ESocketSupport | ETransport | EPreferMBufChains | ENeedMBufs | EUseCanSend;
const TUint KRAWIPServiceInfo		= KSIConnectionLess | KSIMessageBased | KSIBroadcast | KSIPeekData | KSIGracefulClose | KSIRequiresOwnerInfo;
const TUint KRAWIPNameServiceInfo	= KNSNameResolution | KNSRequiresConnectionStartup; // | KNSServiceResolution | KNSInfoDatabase;
//

#endif
