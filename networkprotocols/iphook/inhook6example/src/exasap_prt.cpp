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
// exasap_prt.cpp - Plugin with SAP example 
//

#include <posthook.h>
#include "protocol_module.h"
#include "provider_module.h"

/**
* @name The exasap identification
*
* The protocol is identified by (address family, protocol number). This
* should be unique among all protocols known to the socket server.
*
* Unfortunately there are no rules or registration for these values, and
* the protocol writer just has to pick a combination that is supposed to
* be unique.
*
* In this example, neither of these values have any significance to the
* implementation. Any values will work.
* @{
*/
/** The address family constant. Use the UID value of this protocol module. */
const TUint KAfExasap = 0x101F6D00;
/** The protocol number. Because the family is unique, 1000 should not confuse anyone. */
const TUint KProtocolExasap	= 1000;
/** @} */


class CProviderEntry : public CBase
	/**
	* Track registered providers.
	*/
	{
public:
	CProviderEntry(MProviderForPrt &aProvider) : iProvider(aProvider) {}

	MProviderForPrt &iProvider;
	CProviderEntry *iNext;
	};


class CProtocolExasap : public CProtocolPosthook, public MProtocolForSap
	/**
	* A protocol plugin for inbound packets.
	*
	* This is a minimal definition for a protocol plugin class
	* (hook), which attaches to the inbound packet path to examine
	* packets for a specific protocol or just before they
	* are passed to the upper layer (choice depends on the nature
	* of the BindL which is done in the NetworkAttachedL function).
	*/
	{
public:
	CProtocolExasap();
	virtual ~CProtocolExasap();

	// CProtocolBase
	virtual void Identify(TServerProtocolDesc *aDesc) const;
	virtual CServProviderBase *NewSAPL(TUint aType);

	// CProtocolPosthook
	virtual void NetworkAttachedL();

	// CIp6Hook::MIp6Hook 
	virtual TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);

	// ProtocolModule glue
	static void Describe(TServerProtocolDesc& anEntry);

	// MProtocolForSap
	virtual MNetworkService *NetworkService();
	virtual void Register(MProviderForPrt &aProvider);
	virtual void Unregister(const MProviderForPrt &aProvider);

private:
	CProviderEntry *iList;
	};

CProtocolExasap::CProtocolExasap()
	/**
	* Constructor.
	*
	* (nothing to do in this example)
	*/
	{
	}

CProtocolExasap::~CProtocolExasap()
	/**
	* Desctructor.
	*
	*/
	{
	// The protocol instance cannot be deleted while there
	// are SAP's active, thus iList must be NULL!
	ASSERT(iList == NULL);
	}


void CProtocolExasap::NetworkAttachedL()
	/**
	* The TCP/IP stack has been attached to this plugin.
	*
	* The CProtocolPosthook impelements the basic BindL/BindToL and Unbind
	* processing. The NetworkAttached is called when the TCP/IP
	* is connected with this protocol module.
	*
	* This function installs a hook for forwarded packets. The function
	* ApplyL will be called for each received packet that enters the
	* forward path (before actual forwarding decision).
	*
	* Could also install any other hooks to pull packets.
	*/
	{
/** @code */
	NetworkService()->BindL(this, BindForwardHook());
/** @endcode */
	}

void CProtocolExasap::Identify(TServerProtocolDesc *aDesc) const
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


void CProtocolExasap::Describe(TServerProtocolDesc& anEntry)
	/**
	* Return the description of the protocol.
	*
	* Becauses this example provides the socket service (SAP),
	* some socket related fields in the description need to be
	* defined for the socket server.
	*
	* - iName		The name of the protocol ("exasap").
	* - iAddrFamily	The address family of the protocol (KAfExasap).
	* - iProtocol	The protocol number (KProtocolExasap).
	* - iSockType	Defined as datagram
	* - iNumSockets	Must allow sockets to be opened
	* - iMessageSize	Max possibe datagram size
	*
	* A socket for this protocol can be opened as:
@code
	RSocket socket;
	// .. by name
	err = socket.Open(socket_server, _L("exasap"));
	// .. by explicit family, type and protocol
	err = socket.Open(socket_server, KAfExasap, KSockDatagram, KProtocolExasap);
@endcode
	*/
	{
/** @code */
	anEntry.iName=_S("exasap");
	anEntry.iAddrFamily=KAfExasap;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KProtocolExasap;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIConnectionLess | KSIMessageBased;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=0;
	anEntry.iMessageSize=0xffff-128;
	anEntry.iServiceTypeInfo=ESocketSupport | ETransport | EPreferMBufChains | ENeedMBufs | EUseCanSend;
	anEntry.iNumSockets=KUnlimitedSockets;
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
	CProtocolExasap::Describe(anEntry);
	}


CProtocolBase *ProtocolModule::NewProtocolL(TUint /*aSockType*/, TUint aProtocol)
	{
	if (aProtocol != KProtocolExasap)
		User::Leave(KErrNotSupported);
	return new (ELeave) CProtocolExasap;
	}

//
// Provider related support
// ------------------------
//

MNetworkService *CProtocolExasap::NetworkService()
	{
	return CProtocolPosthook::NetworkService();
	}

void CProtocolExasap::Register(MProviderForPrt &aProvider)
	{
	CProviderEntry* sap = new CProviderEntry(aProvider);
	if (sap)
		{
		sap->iNext = iList;
		iList = sap;
		}
	}

void CProtocolExasap::Unregister(const MProviderForPrt &aProvider)
	{
	CProviderEntry *sap;
	for (CProviderEntry** h = &iList; (sap = *h) != NULL; h = &sap->iNext)
		{
		if (&sap->iProvider == &aProvider)
			{
			*h = sap->iNext;
			delete sap;
			break;
			}
		}
	}

CServProviderBase *CProtocolExasap::NewSAPL(TUint aType)
	{
	return ProviderModule::NewSAPL(aType, *this);
	}

//
// Packet Processing
// -----------------

TInt CProtocolExasap::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo & aInfo)
	/**
	* Receives a peek at hooked packets.
	*
	* @param aPacket	The packet data
	* @param aInfo		The packet information
	*
	* @return KIp6Hook_PASS
	*/
	{
/** @code */
	for (CProviderEntry* sap = iList; sap != NULL; sap = sap->iNext)
		{
		if (sap->iProvider.IsReceiving(aInfo))
			{
			RMBufRecvPacket copy;
			TRAPD(err, aPacket.CopyL(copy);aPacket.CopyInfoL(copy));
			if (err == KErrNone)
				{
				copy.Pack();
				sap->iProvider.Deliver(copy);
				}
			copy.Free();
			}
		}
	return KIp6Hook_PASS;
/** @endcode */
	}
