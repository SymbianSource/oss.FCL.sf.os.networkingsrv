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
// exain_init.cpp - Inbound plugin example protocol initialization
//

#include "exain.h"
#include "protocol_module.h"

CProtocolExain::CProtocolExain()
	/**
	* Constructor.
	*
	* (nothing to do in this example)
	*/
	{
	}

CProtocolExain::~CProtocolExain()
	/**
	* Desctructor.
	*
	* (nothing to do in this example).
	*/
	{
	}

void CProtocolExain::NetworkAttachedL()
	/**
	* The TCP/IP stack has been attached to this plugin.
	*
	* The CProtocolPosthook impelements the basic BindL/BindToL and Unbind
	* processing. The NetworkAttached is called when the TCP/IP
	* is connected with this protocol module.
	*
	* This function installs a hook for IPv4 ICMP protocol. The function
	* ApplyL will be called for each packet that includes ICMP header.
	*/
	{
/** @code */
	NetworkService()->BindL(this, BindHookFor(KProtocolInetIcmp));
/** @endcode */
	}

void CProtocolExain::Identify(TServerProtocolDesc *aDesc) const
	/**
	* Returns description of this protocol.
	*
	* The description is required by this and also by CProtocolFamilyBase::ProtocolList,
	* which in the example enviroment gets translated into a call to the
	* CProtocolModule::Describe. Identify can use the same function.
	*/
	{
/** @code */
	Describe(*aDesc);
/** @endcode */
	}


void CProtocolExain::Describe(TServerProtocolDesc& anEntry)
	/**
	* Return the description of the protocol.
	*
	* Becauses this example does not provide the socket service (SAP),
	* most of the fields in the description can be set to zero. Only
	* the following need to be initialized to something specific:
	*
	* - iName		The name of the protocol ("exain").
	* - iAddrFamily	The address family of the protocol (KAfExain).
	* - iProtocol	The protocol number (KProtocolExain).
	*/
	{
/** @code */
	anEntry.iName=_S("Exain");
	anEntry.iAddrFamily=KAfExain;
	anEntry.iSockType=0;
	anEntry.iProtocol=KProtocolExain;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=0;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=0;
	anEntry.iMessageSize=0;
	anEntry.iServiceTypeInfo=0;
	anEntry.iNumSockets=0;
/** @endcode */
	}

//
// Example environment wrappings
// -----------------------------
//


TInt ProtocolModule::NumProtocols()
	{
	return 1;
	}

void ProtocolModule::Describe(TServerProtocolDesc& anEntry, const TInt /*aIndex*/)
	{
	CProtocolExain::Describe(anEntry);
	}


CProtocolBase *ProtocolModule::NewProtocolL(TUint /*aSockType*/, TUint aProtocol)
	{
	if (aProtocol != KProtocolExain)
		User::Leave(KErrNotSupported);
	return new (ELeave) CProtocolExain;
	}


