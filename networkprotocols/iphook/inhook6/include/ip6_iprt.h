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
// ip6_iprt.h - IPv6 protocol/interface definition
// Define CProtocolBaseUnbind and CProtocolInterfaceBase.
// This header defines the minimal extensions to the CProtocolBase
// which are required from any protocol that is bound to IP layer
// of the TCPIP6. (for a more complex bindings, "ip6_hook.h" is
// required instead of this)
//



/**
 @file ip6_iprt.h
 @publishedPartner
 @released
*/

#ifndef __IP6_IPRT_H__
#define __IP6_IPRT_H__

#include <es_prot.h>

//
//	CProtocolBaseUnbind
//	*******************
/**
*	CProtocolBase sub-class that adds a required function to unbind a protocol.
*
*	This class exists only because the CProtocolBase is missing
*	an essential method: Unbind. All protocols that implement
*	BindL, should also support Unbind!
*
*	It is assumed that all protocols and hooks, which connect
*	to TCPIP stack, must support this method. Strictly, supporting
*	Unbind is really needed only if the protocol makes use of
*	BindL (e.g. the protocol is bound to other protocols).
*
* @publishedPartner
* @released
* @since v7.0
*/
class CProtocolBaseUnbind : public CProtocolBase
	{
public:
	/**
	* Unbinds from a specified protocol.
	*
	* Unbind is a reverse of the BindL(). Unbind does nothing
	* if there is no matching bind. It is safe to call Unbind
	* "just to be sure". The rationale for Unbind is explained
	* using the UDP protocol as an example.
	*
	* When UDP protocol is active, it needs to receive IP packets
	* with protocol=17 from the IP layer. To achieve this, the UDP
	* will call IP protocol instance with BindL(UDP, 17). This registers
	* UDP instance as a receiver of the UDP packets. IP records
	* the UDP instance pointer and uses UDP->Process() method to
	* pass received packets to the UDP protocol instance.
	*
	* Before UDP protocol instance is destroyed, the UDP
	* registration in IP layer must be cancelled. Otherwise the
	* IP layer would be using a dangling pointer after UDP destruction.
	* The registration is cancelled by use of Unbind.
	*
	* The similar reasoning applies to any hook that registers with
	* the IP layer with BindL.
    *
	* @param aProtocol
	*	Protocol instance being unregistered.
	* @param aId
	*	Identify the binding that is to be unregistered. The
	*	exact interpretation of this depends on protocol implementation,
	*	but generally, the aId value used in BindL can be used to
	*	unregister that specific binding. In addition to that, the
	*	TCPIP IP layer supports generic unbind: if aId is ZERO, then
	*	all bindings made by aProtocol are canceled and cleared.
	*/
	virtual void Unbind(CProtocolBase *aProtocol, TUint aId = 0) = 0;
	};

//	CProtocolInterfaceBase
//	**********************
class CNifIfBase;
/**
*	A special interface for a protocol module that itself provides
*	access to a network interface.
*
*	When IP6 is bound to a protocol, such protocol is loaded whenever
*	IP is active. If the TServerProtocolDesc::iServiceTypeInfo has
*	EInterface (of TProtocolServiceInfo) flag set, the IP6 assumes that
*	the protocol is derived from CProtocolIneterfaceBase and that it
*	supports the GetBinderL method.
*
*	The GetBinderL method is then called using the current protocol name
*	(ip or ip6) as a parameter. If GetBinderL succeeds by returning a NIF
*	pointer, the stack installs this an interface to the system. This can
*	be used to create "fake" or real interfaces to the stack outside the
*	NIFMAN control.
*
*	<b>WARNING</b>: TCPIP is just stealing the EInterface flag for this
*	purpose. The original use of this flag is unknown, and it does not
*	seem to be used for anything in current socket server (as of writing
*	this).
*
* @publishedPartner
* @released
* @since v7.0
*/
class CProtocolInterfaceBase : public CProtocolBaseUnbind
	{
public:
	/**
	* Gets the network inteface provided by the protocol module.
	*
	* Network layer calls this method once after a protocol that
	* has been tagged as "interface protocol" by use of EInterface
	* flag in the TServerProtocolDesc::iServiceTypeInfo field.
	*
	* The returned NIF (if ANY) is installed a network interface
	* to the IP stack.
	*
	* @param aName
	*	Name of the binding protocol ("ip" or "ip6").
	* @return
	* @li = non-NULL,
	*	The network interface (NIF).
	* @li = NULL
	*	No interface is installed, but the protocol is left running
	*	as if EInterface was not set.
	* @leave
	*	Close the protocol (protocol is not left running).
	*/
	virtual CNifIfBase* GetBinderL(const TDesC& aName) = 0;
	};

#endif
