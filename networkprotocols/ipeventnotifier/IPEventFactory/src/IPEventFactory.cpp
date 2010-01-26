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
 @file IPEventFactory.CPP
 @internalComponent
*/

#include "IPEventFactory.h"
#include <networking/ipeventtypesids.h>
#include <ss_glob.h>
#include <in_sock.h>
#include <comms-infras/idquerynetmsg.h>
#include <ecom/implementationproxy.h>
#include <ecom/ecom.h>
#include "HookLog.h"

// Define the interface UIDs
const TImplementationProxy ImplementationTable[] =
	{
	IMPLEMENTATION_PROXY_ENTRY(IPEvent::KFactoryImplementationUid, CIPEventNotifierFactory::NewL)
	};

/**
 * ECOM Implementation Factory
 * @internalTechnology
 */
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

	return ImplementationTable;
	}
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
using namespace ESock;
#endif

CIPEventNotifierFactory* CIPEventNotifierFactory::NewL(TAny* aConstructionParameters)
	{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	return new (ELeave)CIPEventNotifierFactory( TUid::Uid(IPEvent::KFactoryImplementationUid), *(reinterpret_cast<CProtocolFamilyFactoryContainer*>(aConstructionParameters)));
#else
	return new (ELeave)CIPEventNotifierFactory(IPEvent::KFactoryImplementationUid, *(reinterpret_cast<CProtocolFamilyFactoryContainer*>(aConstructionParameters)));
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	}
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
CIPEventNotifierFactory::CIPEventNotifierFactory(TUid aFactoryId, CProtocolFamilyFactoryContainer& aParentContainer) :
#else
CIPEventNotifierFactory::CIPEventNotifierFactory(TUint aFactoryId, CProtocolFamilyFactoryContainer& aParentContainer) :
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CProtocolFamilyFactoryBase( aFactoryId, aParentContainer )
	{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY    
	// LOG_NODE_CREATE(KHookLogFolder, CIPEventNotifierFactory) - this needs attention since EC120 disabled this macro for UDEB, maybe use FACTORY_LOG_NODE_CREATE macro (which isn't used anywhere else)
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY    
	}

using namespace NetMessages;

/**
 * This sends a message to the IP Event Notifier, asking it to open a session or telling it to close a session.
 *
 * Passed in message is a CTypeIdQuery, that can mean one of two things:
 *
 *   - If its iHandle is null, open a session on the interface named by its iOid. Its iHandle is set to the session handle so the caller knows the handle.
 *      The caller can then listen for events on this pub/sub address key (with the pub/sub address UId of the event type of interest)
 *
 *   - If its iHandle is set to something, close the session specified by that handle.
 */
TInt CIPEventNotifierFactory::DoReceiveMessage( CMessage& aNetMessage )
	{
	TInt nRet = KErrNotSupported;
	//is it the right type?
	if ( aNetMessage.IsTypeOf( STypeId::CreateSTypeId( KInterfaceUid, ETypeIdQueryId ) ) )
		{
		CTypeIdQuery& aMsg = static_cast<CTypeIdQuery&>(aNetMessage);
		//message for us?
		if ( aMsg.iUid == IPEvent::KFactoryImplementationUid && aMsg.iTypeId == IPEvent::KProtocolId )
			{//yes
			CProtocolBase* pProtocol = FindProtocol( IPEvent::KProtocolId );
			if ( pProtocol )
				{
				nRet = pProtocol->GetOption( KSolInetIp, aMsg.iHandle ? KHandleRelease : KHandleAttach, aMsg.iOid );
				aMsg.iHandle = *(reinterpret_cast<const TInt*>(aMsg.iOid.Ptr()));
				}
			}
		}
	return nRet;
	}




