/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#ifndef T_DNSPROXY_CONST_H_
#define T_DNSPROXY_CONST_H_

#include <e32base.h> 
#include <es_sock.h>
#include <e32cmn.h> 
#include <in_sock.h>
#include <commdbconnpref.h>
#include <comms-infras/es_parameterbundle.h>



#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif

const TInt KSockBufferLength=256;

_LIT(KDnsProxyMainTestWrapper, "DnsProxyMainTestWrapper");

/// Commands
_LIT(KNew,					"New");
_LIT(KTestGlobal,			"TestGlobal");
_LIT(KTestLocal,			"TestLocal");
_LIT(KUrl,					"url");
_LIT(KAddr,					"addr");
_LIT(KDomainName,			"domainname");
_LIT(KUpdateDBFlag,			"updatedbflag");
_LIT(KQueryType,			"querytype");
_LIT(KIAP1,					"iap1");
_LIT(KIAP2,					"iap2");
_LIT(KUSEIAP,				"useiap");
_LIT(KCHANGEIFACE,			"changeifaceinfo");
_LIT(KQueryResp,			"queryresp");
_LIT(KTestConnection,		"TestConnection");
_LIT(KExpectation,			"expectation");

#endif /*T_IPSECCONST_H_*/
