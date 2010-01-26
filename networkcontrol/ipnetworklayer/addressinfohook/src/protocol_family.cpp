// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// A generic protocol family implementation.
// This implements the generic protocol independent
// protocol family (CProtocolFamilyGeneric) and the
// DLL "main program".
// The API towards the actual protocol implementation
// is defined by the ProtocolModule class.
// This is useful for simple protocols and saves them
// from the need of writing this standard DLL wrapping.
// Complex protocols need to write own implementation
// of this directly (and not use the ProtocolModule
// class).
// This could be used in any protocol implementation,
// and not just for a hook in TCP/IP stack.
// 
//

/**
 @file protocol_family.cpp
 @note
 @internalTechnology
 @prototype
*/

#include <es_prot.h>

#include "protocol_family.h"
#include "protocol_module.h"

// -------------------------------------------------------------------
// DLL wrappings
//
// Entrypoint
//


extern "C" { IMPORT_C CProtocolFamilyBase* CreateProtocolFamilyL (void); }
EXPORT_C CProtocolFamilyBase* CreateProtocolFamilyL()
	/**
	* Create family instance.
	*
	* This is the one and only exported function from
	* a polymorphic DLL. The second number of the UID
	* (usually 0x10003d38 for protocols) defines the
	* type of the returned instance. For the protocol
	* modules (.PRT), this is CProtocolFamilyBase.
	*
	* @return The protocol family.
	*/
	{
	return new (ELeave) CProtocolFamilyGeneric;
	}

//----------------------------------------------------------------------
// The actual protocol family implementation
//

CProtocolFamilyGeneric::CProtocolFamilyGeneric()
	/** Constructor. */
	{
	__DECLARE_NAME(_S("CProtocolFamilyGeneric"));
	}

CProtocolFamilyGeneric::~CProtocolFamilyGeneric()
	{
	/** Destructor. */
	}

TInt CProtocolFamilyGeneric::Install()
	/**
	* Install protocol module.
	*
	* This is the "contruct" of the protocol family
	* instance. The simple generic protocol family
	* has nothing to do. Always succeeds.
	*/
	{
	return KErrNone;
	}

TInt CProtocolFamilyGeneric::Remove()
	/**
	* Remove protocol module.
	*
	* This is optional, and the simple generic implementation has
	* nothing to do. Returning other than KErrNone should prevent
	* socket server from removing the module.
	*
	* @return Status.
	*/
	{
	return KErrNone;
	}

TUint CProtocolFamilyGeneric::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
	/**
	* Gets list of supported protocols.
	*
	* Uses ProtocolModule class to find out how many protocols (ProtocolModule::Describe)
	* this module implements, allocates and fills in (ProtocolModule::Describe)
	* the protocol description list.
	*
	* @retval	aProtocolList	The protocol description list.
	* @return	The number of protocols.
	*
	* Even, though the function is not named as ending with L, this must leave
	* if the allocation of the list fails or if some other problem occurs.
	*
	* @note
	*	Newer OS versions may support also 0 or error returns.
	*
	*/

	// This function should be a leaving fn
	// apparently it is OK for it to leave
	const TInt num_protocols = ProtocolModule::NumProtocols();
	if (num_protocols > 0)
		{
		TServerProtocolDesc *const p = new (ELeave) TServerProtocolDesc[num_protocols]; // Esock catches this leave
		for (TInt i = 0; i < num_protocols; ++i)
			ProtocolModule::Describe(p[i], i);
		aProtocolList = p;
		return num_protocols;
		}
	User::Leave(KErrNotSupported);
	return NULL;	// Just to silence compiler warning.
	}

CProtocolBase* CProtocolFamilyGeneric::NewProtocolL(TUint aSockType, TUint aProtocol)
	{
	/**
	* Creates a new protocol object.
	*
	* The implementation delegated as is to the ProtocolModule::NewProtocolL.
	*
	* @param aSockType	The socket type
	*	(#KSockStream, #KSockDatagram, #KSockSeqPacket or #KSockRaw=4)
	*	(TServerProtocolDesc::iSockType).
	* @param aProtocol	The protocol number (TServerProtocolDesc::iProtocol)
	*/
	return ProtocolModule::NewProtocolL(aSockType, aProtocol);
	}
