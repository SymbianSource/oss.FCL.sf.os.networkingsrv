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
// protocol_module.h - Generic glue beween family and protocol(s)
// 
//

#ifndef __PROTOCOL_MODULE_H
#define __PROTOCOL_MODULE_H

#include <es_prot.h>

/**
* @file protocol_module.h
* Generic glue between family and protocol(s).
*
* This header does not have own implementation (no protocol_module.cpp
* exist). The implementation must be defined by your own protocol
* implementation.
*/

class CProtocolBase;

class ProtocolModule
	/**
	* A help class for building the plugin example projects.
	*
	* This class defines the minimal API between the protocol
	* family implementation (CProtocolFamilyBase) and protocol
	* implementations (CProtocolBase).
	* This class must be implemented in your protocol module. The
	* generic family implementation converts the socket server
	* requests into calls to these static functions.
	*
	* This is targeted for simple protocol modules, which implement
	* only one or few protocols. It allows the use of a generic
	* CProtocolFamilyBase implementation. The generic family
	* implementation does the "symbian DLL wrappings".
	*
	* A complete protocol module contains three major components:
	*
	* -# protocol family implementation (link in the generic code)
	* -# protocol implementation (CProtocolBase). Write your own version (must).
	* -# service provider implementations (CServProviderBase). Write your own version,
	*	 if the protocol(s) have a socket API.
	*
	* By using this API, a single generic family implemtation can
	* be linked into many protocols modules.
	*
	* @note
	*	This could be used in any protocol implementation,
	*	and not just for a hook in TCP/IP stack.
	*
	*/
	{
public:
	/**
	* Return the number of protocols.
	*
	* This must return the number of protocols that are
	* impelemented by your protocol module. The value
	* determines the number of elements in the TServerProtocolDesc
	* array, that will be returned to the socket server from the
	* CProtocolFamilyGeneric::ProtocolList implementation.
	*
	* The number is constant for each PRT module. The function
	* is only called from the ProtocolList().
	*
	* @return	The number of implemented protocols.
	*/
	static TInt NumProtocols();
	/**
	* Describe a protocol.
	*
	* Like NumProtocols(), this is also called only from the
	* CProtocolFamilyGeneric::ProtocolList implementation.
	* The allocated TServerProtocolDesc array is initialized
	* by calling this function once for each element.
	*
	* If only one protocol is implemented (NumProtocols() returns 1),
	* then this is called once with aIndex = 0. If multiple protocols
	* are implemented, the function must use aIndex to decide which
	* description is to be returned.
	*
	* @retval	aDesc	The protocol description.
	* @param	aIndex	The protocol to describe [0 .. NumProtocols()-1].
	*/
	static void Describe(TServerProtocolDesc &aDesc, const TInt aIndex = 0);
	/**
	* Create the protocol instance.
	*
	* This is called directly from CProtocolFamilyGeneric::NewProtocolL(),
	* and all the semantics of the CProtocolFamilyBase::NewProtocolL are
	* in effect.
	*
	* The implementation must create one instance of a protocol based on
	* aSockType and aProtocol. These should match one of the descriptions
	* returned earlier (TProtocolDesc::iSockType and TProtocolDesc::iProtocol)
	* by Describe().
	*
	* @param	aSocketType	The socket type.
	* @param	aProtocol	The protocl number
	* @return	The protocol instance
	*
	* The function must leave, if protocol cannot be created. Returning
	* NULL is not allowed.
	*/
	static CProtocolBase *NewProtocolL(TUint aSockType, TUint aProtocol);
	};

#endif
