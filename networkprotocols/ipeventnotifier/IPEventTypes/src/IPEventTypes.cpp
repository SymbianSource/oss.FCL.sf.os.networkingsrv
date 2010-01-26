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
 @file ipeventtypes.cpp
 @internalComponent
*/


#include <e32cons.h>
//#include <in_sock.h>
//#include <nifman.h>
#include <e32property.h>
#include <e32hal.h>
#include <e32math.h>
#include <comms-infras/metadata.h>
#include <comms-infras/metatype.h>
#include <ecom/implementationproxy.h>
#include <ecom/ecom.h>


#include "IPEventTypes.h"


using namespace Meta;
using namespace IPEvent;


REGISTER_TYPEID( CIPEventType, IPEvent::KEventImplementationUid, Meta::KNetMetaTypeAny )

SMetaDataECom* CIPEventType::InstantiateL(TAny* aParams)
	{
	TInt type = reinterpret_cast<TInt>(aParams);
	SMetaDataECom* impl = NULL;
	switch( type )
		{
		case EMFlagReceived:
			impl = new(ELeave)CMFlagReceived();
			break;
		case EIPReady:
			impl = new(ELeave)CIPReady();
			break;
		case ELinklocalAddressKnown:
			impl = new(ELeave)CLinklocalAddressKnown();
			break;
		default:
			User::Leave(KErrNotSupported);
			break;
		};
	return impl;
	}


START_ATTRIBUTE_TABLE( CMFlagReceived, IPEvent::KEventImplementationUid, EMFlagReceived )
	REGISTER_ATTRIBUTE( CMFlagReceived, iMFlag, TMetaNumber )
	REGISTER_ATTRIBUTE( CMFlagReceived, iOFlag, TMetaNumber )
END_ATTRIBUTE_TABLE_BASE(CIPEventType,IPEvent::KEventImplementationUid)


START_ATTRIBUTE_TABLE( CIPReady, IPEvent::KEventImplementationUid, EIPReady )
	REGISTER_ATTRIBUTE( CIPReady, iIPAddress, TMeta < TInetAddr > )
	REGISTER_ATTRIBUTE( CIPReady, iAddressValid, TMetaNumber )
END_ATTRIBUTE_TABLE_BASE(CIPEventType,IPEvent::KEventImplementationUid)


START_ATTRIBUTE_TABLE( CLinklocalAddressKnown, IPEvent::KEventImplementationUid, ELinklocalAddressKnown )
	REGISTER_ATTRIBUTE( CLinklocalAddressKnown, iIPAddress, TMeta < TInetAddr > )
END_ATTRIBUTE_TABLE_BASE(CIPEventType,IPEvent::KEventImplementationUid)

/**
 * Define the instantiator table
 */
const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY(IPEvent::KEventImplementationUid, CIPEventType::InstantiateL)
	};


/**
 * ECOM Implementation Factories
 * @internalTechnology
 */

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

	return ImplementationTable;
	}


