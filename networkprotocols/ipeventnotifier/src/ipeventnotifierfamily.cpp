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

/**
 @file ipeventnotifierfamily.cpp
 @internalComponent
*/

 
#include "ipeventnotifierfamily.h"
#include "ipeventnotifier.h"
//this .h should be removed once SocketServer::InitSockmanGlobalsL creates factories first
//TODO why is this header included?? #include <ss_protprov.h>
#include <ss_glob.h>
#include <networking/ipeventtypesids.h>

/*
 * @internalTechnology
 */
IMPORT_C CProtocolFamilyBase* Install(void);

/*
 * @internalTechnology
 */
EXPORT_C CProtocolFamilyBase* Install(void)
	{
	//this should be removed once SocketServer::InitSockmanGlobalsL creates factories first
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	_LIT8(KIPEventFactory, "IpEventFactory");
	CSockManData::InstallFactoryL(ESock::EProtocolFamilyFactory, KIPEventFactory, TUid::Uid(IPEvent::KFactoryImplementationUid));
#else
	_LIT(KIPEventFactory, "IpEventFactory");
	TSockManData::InstallFactoryL(Esock::EProtocolFamilyFactory, KIPEventFactory, IPEvent::KFactoryImplementationUid );
#endif	
	return new (ELeave) CIPEventNotifierProtocolFamily;
	}

/**
 *	CIPEventNotifierProtocolFamily
 */
CIPEventNotifierProtocolFamily::CIPEventNotifierProtocolFamily()
	{
	__DECLARE_NAME(_S("CIPEventNotifierProtocolFamily"));
	}

CIPEventNotifierProtocolFamily::~CIPEventNotifierProtocolFamily()
	{
	}

TInt CIPEventNotifierProtocolFamily::Install()
	{
	return KErrNone;
	}

TInt CIPEventNotifierProtocolFamily::Remove()
	{
	return KErrNone;
	}

TUint CIPEventNotifierProtocolFamily::ProtocolList(TServerProtocolDesc*& aProtocolList)
	{
	TServerProtocolDesc* p = new (ELeave) TServerProtocolDesc[1]; // Esock catches this leave

	CIPEventNotifier::FillIdentification(p[0]);
	aProtocolList = p;
	return 1;
	}

CProtocolBase* CIPEventNotifierProtocolFamily::NewProtocolL(TUint /*aSockType*/,
												   TUint aProtocol)
	{
	__ASSERT_ALWAYS(aProtocol == IPEvent::KProtocolId, User::Leave(KErrNotSupported) );

	CProtocolBase* pProto=CIPEventNotifier::NewL();
	
	return pProto;
	}




