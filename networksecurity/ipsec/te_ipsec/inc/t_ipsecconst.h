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
#ifndef T_IPSECCONST_H_
#define T_IPSECCONST_H_

#include <e32base.h> 
#include <es_sock.h>
#include <e32cmn.h> 
#include <in_sock.h>
#include <commdbconnpref.h>
#include <comms-infras/es_parameterbundle.h>
//#include <networking/ipscprdscpparam.h>


#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif

const TInt KSockBufferLength=7;

_LIT(KIPSecCryptoTestWrapper, "IPSecCryptoTestWrapper");
_LIT(KIPSecDSCPTestWrapper, "IPSecDSCPTestWrapper");
_LIT(KIPSecIKEv2TestWrapper, "IPSecIKEv2TestWrapper");
_LIT(KIPSecMultipleSATestWrapper, "IPSecMultipleSATestWrapper");

/// Commands
_LIT(KNew,						"New");
_LIT(KSetDSCP,					"SetDSCP");
_LIT(KTestIKEv2,				"TestIKEv2");
_LIT(KNegativeTestIKEv2,		"NegativeTestIKEv2");
_LIT(KEchoClient,				"EchoClient");
_LIT(KTestMultipleSA,			"TestMultipleSA");
_LIT(KNegativeTestMultipleSA,	"NegativeTestMultipleSA");
_LIT(KGetCryptoStrength,		"GetCryptoStrength");

/// Command parameters
_LIT(KObjectValue,			"object_value");
_LIT(KTOSorTrafficClass, 	"TosOrTrafficClass");
_LIT(KPort,			"port");
_LIT(KDestAddr,		"destaddr");
_LIT(KLocaladdr,	"localaddr");
_LIT(KPriority,				"priority");
_LIT(KDrive,				"drive");
_LIT(KMsvServerPattern, 	"!MsvServer*");



#endif /*T_IPSECCONST_H_*/
