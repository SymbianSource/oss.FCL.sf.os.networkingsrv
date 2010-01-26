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
// provider_module.h - generic glue beween protocol and provider
//

#ifndef __PROVIDER_MODULE_H
#define __PROVIDER_MODULE_H

#include <es_prot.h>

/**
* @file provider_module.h
* Generic glue between protocol and provider.
*
* This is just an example, one way of doing this.
*
* This header does not have own implementation (no provider_module.cpp
* exist). The implementation must be defined by your own protocol
* implementation (in cpp that implements the real CServProviderBase
* derived class).
* @internalComponent
*/

class CProtocolBase;
class MNetworkService;
class RMBufPktInfo;
class RMBufChain;

class MProviderForPrt
	/**
	* The API from SAP for PRT.
	*
	* Example.
	*/
	{
public:
	/**
	* Tests if SAP accepts the packet.
	*
	* IsReceiving is used by the protocol module to find out if a SAP
	* will accept the packet. This is to avoid unnecessary copying of
	* the packet for SAP's that don't need it.
	*
	* The reasons for not accepting depend on the SAP. May be accepting only
	* certain types of packets, or the receive queue may be over the limit.
	*
	* @param aInfo	The packet information.
	*
	* @return	TRUE, if will be accepted, and FALSE otherwise.
	*/
	virtual TBool IsReceiving(const RMBufPktInfo &aInfo) = 0;

	/**
	* Delivers a packet for the provider.
	*
	* The packet is in "packed state". The provider is
	* assumed to "own" this packet, the protocol module
	* has duplicated the packet, if it needs to preserve
	* the original.
	*
	* @param aPacket	The packet.
	*/
	virtual void Deliver(RMBufChain& aPacket) = 0;
	};

class MProtocolForSap
	/**
	* The API from PRT for SAP.
	*
	* Example.
	*/
	{
public:
	/**
	* Returns the network service.
	*
	* @return The network service
	*/
	virtual MNetworkService *NetworkService() = 0;
	/**
	* Registers the provider with the protocol.
	*
	* This function must be called if the SAP wants to receive
	* packets from the protocol instance.
	*
	* @param aProvider	The provider
	*/
	virtual void Register(MProviderForPrt &aProvider) = 0;
	/**
	* Deregisters the provider from the protocol.
	*
	* This function must be called at least from the destructor
	* of the provider, if it has been registered. This removes
	* the reference to the provider from the protocol module.
	*/
	virtual void Unregister(const MProviderForPrt &aProvider) = 0;
	};


class ProviderModule
	/**
	* A help class for building the plugin example projects.
	*
	* This class defines the minimal API between the protocol
	* implementation (CProtocolBase) and service provider (SAP)
	* implementations (CServProviderBase).
	* This class must be implemented in your provider module. The
	* protocol implementation converts the socket server
	* into a calls to these static functions.
	*
	* A complete protocol module contains three major components:
	*
	* -# protocol family implementation (link in the generic code)
	* -# protocol implementation (CProtocolBase). Write your own version (must).
	* -# service provider implementations (CServProviderBase). Write your own version,
	*	 if the protocol(s) have a socket API.
	*/
	{
public:
	/**
	* Create new service provider instance (SAP).
	*
	* This is called directly from CProtocolBase::NewSAPL(),
	* and all the semantics of the CProtocolBase::NewSAPL are
	* in effect.
	*
	* The implementation must create one instance of a SAP
	* aProtocol.
	*
	* @param aType		The socket type
	* @param aProtcol	The protocol
	*.
	* @return	The provider
	*
	* The function must leave, if provider cannot be created. Returning
	* NULL is not allowed.
	*/
	static CServProviderBase* NewSAPL(TUint aType, MProtocolForSap &aProtocol);
	};
#endif
