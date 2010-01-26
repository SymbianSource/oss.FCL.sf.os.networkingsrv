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
//

#include <e32std.h>
#include <es_prot_internal.h>
#include "ipseccrypto.h"

//
// Local declarations, not needed by anyone else but this module
//

class CProtocolFamilyCryptoEay : public CProtocolFamilyBase
	{
    public:
        CProtocolFamilyCryptoEay();
        ~CProtocolFamilyCryptoEay();
    public: 
        TInt Install();
        TInt Remove();
        CProtocolBase *NewProtocolL(TUint aSockType, TUint aProtocol);
        TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	};

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase *Install(); }
EXPORT_C CProtocolFamilyBase *Install()
	{
    return new CProtocolFamilyCryptoEay;
	}

//
// Protocol Stub
//

CProtocolEay::CProtocolEay()
	{
    __DECLARE_NAME(_S("CProtocolIpsecCrypto"));
	}

//
// This FillinInfo is total guesswork (as this is not a "real"
// protocol, most (all?) of these are totally useless. The
// key is to choose neutral values that won't cause the
// socket manager to assume any special functionality of this
// protocol.
//
// (declared as "pure" static to avoid cluttering the
// CProtocolEay with unnecessary methods)
//
static void FillinInfo(TServerProtocolDesc &anEntry)
	{
    anEntry.iName=_S("ipseccrypto");
    anEntry.iAddrFamily=KAfCrypto;
    anEntry.iSockType=KSockRaw;
    anEntry.iProtocol=KProtocolCrypto;
    anEntry.iVersion=TVersion(1, 0, 0);
    anEntry.iByteOrder=ELittleEndian;
    anEntry.iServiceInfo=KSIStreamBased;
    anEntry.iNamingServices = 0;
    anEntry.iSecurity=KSocketNoSecurity;
    anEntry.iMessageSize = KSocketMessageSizeIsStream;
    anEntry.iServiceTypeInfo=ENeedMBufs;
    anEntry.iNumSockets=KUnlimitedSockets;
	}

void CProtocolEay::Identify(TServerProtocolDesc *aInfo) const
	{
    FillinInfo(*aInfo);
	}


//
// Protocol Family Stub
//

CProtocolFamilyCryptoEay::CProtocolFamilyCryptoEay()
	{
    __DECLARE_NAME(_S("CProtocolFamilyCryptoIpsecCrypto"));
	}


CProtocolFamilyCryptoEay::~CProtocolFamilyCryptoEay()
	{
	}


TInt CProtocolFamilyCryptoEay::Install()
	{
    return KErrNone;
	}


TInt CProtocolFamilyCryptoEay::Remove()
	{
    return KErrNone;
	}


TUint CProtocolFamilyCryptoEay::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
    // This function should be a leaving fn
    // apparently it is OK for it to leave
    TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[1];

    FillinInfo(p[0]);
    // There should be no need for this below...
    // TRAPD(res, Nif::CheckInstalledMBufManagerL();)
    aProtocolList = p;
    return 1;
	}


CProtocolBase* CProtocolFamilyCryptoEay::NewProtocolL(TUint /*aSockType*/, TUint aProtocol)
	{
    CProtocolBase *inprt = NULL;
    if (aProtocol == KProtocolCrypto)
        inprt = new (ELeave) CProtocolEay;
    return inprt;
	}

