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
// posthook.cpp - Base class for post hooks (implementation)
//

#include <e32std.h>
#include <e32base.h>

#include "posthook.h"

EXPORT_C CProtocolPosthook::~CProtocolPosthook()
	/**
	* Destructor
	*
	* If this object is still attached to the network layer,
	* the attachement and all bindings to the network layer
	* from this protocol object are cancelled.
	*/
	{
	if (iNetwork != NULL)
		{
		CProtocolInet6Binder *const prt = iNetwork->Protocol();
		iNetwork = NULL;
		prt->Unbind(this);
		if (iBindToNet)
			prt->Close();
		}
	}

EXPORT_C void CProtocolPosthook::BindL(CProtocolBase* aProtocol, TUint aId)
	/**
	* Handles network layer binds.
	*
	* This function handles the network layer detection and posthook
	* chaining binds. The processing is based on the following
	* "well known" id values:
	*
	* - MIp6Hook::BindPostHook() (for outbound posthook chaining)
	* - MIp6Hook::BindPreHook() (for inbound posthook chaining)
	* - #KProtocolInet6Ip (aProtocol must be network instance)
	* - #KProtocolInetIp (aProtocol must be network instance, deprecated)
	*
	* If the aId does not match any of the above, the BindL is
	* silently ignored (just return happens). This is quite OK,
	* there is no need to generate an error in such case.
	*
	* This network layer detection is only used in a configuration, where
	* this protocol is bound from the network layer (ip6). Such configuration
	* will keep this protocol module installed whenever the network is loaded.
@verbatim
[yourposthook]
filename= yourposthook.prt
index= 1
bindfrom= ip6
@endverbatim
	*
	* @note
	*	This protocol can be loaded even if network is not. If the
	*	protocol provides a SAP functionality, an application can
	*	open a socket to this protocol without the network layer
	*	being started.
	*
	* The derived class <b>only</b> needs to override the default
	* implementation if it appears as a target of "bindto" directive
	* in a ESK file of another protocol (except the network layer),
	* <b>and if</b> the implementation needs to store a reference
	* to this protocol. In such case the BindL could be, for example:
@code
	CProtocolBase *iAnother;

	CProtocolDerivedHook::BindL(CProtocolBase *aProtocol, TUint aId)
		{
		if (aId == your_special_binding_protocol)
			{
			// handle your bind
			iAnother = aProtocol;
			}
		// The default should always be called,
		CProtocolPosthook::BindL(aProtocol, aId);
		}
@endcode
	* @param aProtocol	The protocol binding to this.
	* @param aId		The bind id.
	*/
	{
	if (iNetwork == NULL &&	// Only the first network bind is accepted here
		(aId == KProtocolInet6Ip || aId == KProtocolInetIp))
		{
		// Make sure the aProtocol is really the network layer: require
		// that the aId the is same as the one defined in protocol
		// description.
		TServerProtocolDesc info;
		aProtocol->Identify(&info);
		if (info.iProtocol == aId)
			{
			//
			// Hook is loaded via a bindto from network layer
			//
			iNetwork = ((CProtocolInet6Binder *)aProtocol)->NetworkService();
			NetworkAttachedL();
			return;
			}
		}
	if (aId == MIp6Hook::BindPostHook())
		{
		// Maintain outbound post hook chaining
		iPostHook = aProtocol;
		return;
		}
	if (aId == MIp6Hook::BindPreHook())
		{
		// Maintain inbound post hook chaining
		iInboundHook = aProtocol;
		return;
		}
	}

EXPORT_C void CProtocolPosthook::Unbind(CProtocolBase *aProtocol, TUint aId)
	/**
	* Handles network layer unbinds.
	*
	* This function handles the network layer unbind and posthook
	* chaining unbinds. The processing is based on the following
	* "well known" bind id values:
	*
	* - MIp6Hook::BindPostHook()  (for outbound posthook chaining)
	* - MIp6Hook::BindPreHook() (for inbound posthook chaining)
	* - #KProtocolInet6Ip (for the network intance)
	* - #KProtocolInetIp (for network instance, deprecated)
	* - 0 (unbind all)
	*
	* If the aId does not match any of the above, the Unbind is
	* silently ignored (just return happens). This is quite OK,
	* there is no need to generate an error in such case.
	*
	* The derived class needs to override the default implementation
	* only if it is bound from another protocol (except the network layer)
	* and if class also overrode the CProtocolPosthook::BindL and saved
	* a pointer to this protocol. For example:
@code
	CProtocolDerivedHook::Unbind(CProtocolBase *aProtocol, TUint aId)
		{
		if (iAnother == aProtocol &&
			(aId == 0 || aId == your_special_binding_protocol))
			{
			// handle your unbind
			iAnotherProtocol = NULL;
			}
		// The default should always be called,
		CProtocolPosthook::Unbind(aProtocol, aId);
		}
@endcode	
	* @param aProtocol	The protocol unbinding from this.
	* @param aId		The bind id.
	*/
	{
	if (!iBindToNet &&
		iNetwork != NULL &&
		iNetwork->Protocol() == aProtocol && 
		(aId == 0 || aId == KProtocolInet6Ip || aId == KProtocolInetIp))
		{
		NetworkDetached();
		iNetwork = 0;
		}
	if (iPostHook == aProtocol && (aId == 0 || aId == MIp6Hook::BindPostHook()))
		iPostHook = 0;
	if (iInboundHook == aProtocol && (aId == 0 || aId == MIp6Hook::BindPostHook()+1))
		iInboundHook = 0;
	}

// CProtocolPosthook::DoBindToL
// ****************************

EXPORT_C TInt CProtocolPosthook::DoBindToL(CProtocolBase *aProtocol)
	/**
	* Handle network layer bindto.
	*
	* Performs the BindToL processing for the network detection.
	* This network layer detection is only used in a configuration,
	* where the "bindto" list of this protocol includes the network
	* layer (ip6). In this configuration, the network layer
	* stays installed whenever this protocol is loaded.
	*
	* @note
	*	Network can be loaded without this protocol being loaded.
	*
	* @param aProtocol	The protocol to bind to.
	* @return KErrNone or the id of the protocol.
	*  - KErrNone (= 0),
	*	 BindTo processed (it was the network bind, NetworkAttachedL has
	*	 been called.)
	*  - protocol id ( != 0) from the aProtocol, which was not the network
	*
	* @note
	*	The protocol id is TUint. However, this method is defined
	*	to return TInt  The function should be considered to return
	*	TUint and any non-zero value means that the protocol was not
	*	the network. [returning TInt instead of TUint is a mistake].
	*
	* This function exists for derived classes that override the default
	* BindToL for their own processing. See ::BindToL for example.
	*/
	{
	TServerProtocolDesc info;
	aProtocol->Identify(&info);
	const TUint id = info.iProtocol;
	if (iNetwork == NULL && (id == KProtocolInetIp || id == KProtocolInet6Ip))
		{
		iNetwork = ((CProtocolInet6Binder *)aProtocol)->NetworkService();
		aProtocol->Open();
		iBindToNet = 1;	// Network layer bind via BindTo.
		// Note: when the hook binds to network, NetworkDetached is never
		// called, because network can never exit before this hook, the
		// event never happens while this hook is running.
		NetworkAttachedL();
		return KErrNone;
		}
	return id;
	}

// CProtocolPosthook::BindToL
// **************************
EXPORT_C void CProtocolPosthook::BindToL(CProtocolBase *aProtocol)
	/**
	* The default implementation.
	*
	* This implemenation only handles the network bind and leaves
	* with #KErrNotSupported if the protocol is not the network
	* instance.
	*
	* If the "bindto" list of this protocol can have other protocols
	* than the network (ip6), then the derived class must implement the
	* support for it. For example
@code
	CProtocolDerivedHook::BindToL(CProtocolBase *aProtocol)
		{
		const TInt id = DoBindToL(aProtocol);
		if (id == (TInt)your_special_protocol)
			{
			//
			if (iBindTo)
				iBindTo->Close();
			iBindTo = aProtocol;
			iBindTo->Open();
			// Destructor must issue Close, if iBindTo is non-NULL
			}
		else if (id != KErrNone)
			User::Leave(KErrNotSupported);
		}
@endcode
	*/
	{
	if (DoBindToL(aProtocol) != KErrNone)
		User::Leave(KErrNotSupported);
	}

EXPORT_C TInt CProtocolPosthook::Send(RMBufChain &aPacket, CProtocolBase* aSrc)
	/**
	* Send packet to the next protocol.
	*
	* Send passes the outbound packet to the next post-processing hook
	* (#iPostHook).
	* The derived post-processing hook can ímplement own Send, After
	* processing the packet, it would normally call CProtocolPosthook::Send
	* to pass the packet forward (note that the information block is
	* RMBufSendInfo and the attached flow context (RFlowContext)
	* <b>must be closed</b>, if the packet is deleted):
@code
	TInt CProtocolDerivedHook::Send(RMBufChain &aPacket, CProtocolBase *aSrc)
		{
		...
		// Do own packet processing

		if (packet_is_dropped)
			{
			RMBufSendInfo *info = RMBufSendPacket::PeekInfoInChain(aPacket);
			if (info)
				info->iFlow.Close();
			aPacket.Free();
			return 1;
			}
		else
			return CProtocolPosthook::Send(aPacket, aSrc);
		}
@endcode
	* Hook will receive and pass forward the outbound packets only if
	* it has a binding to the network layer as a post-processinghook:
@code
	NetworkService()->BindL(this, MIp6Hook::BindPostHook());
@endcode
	*
	* @note
	*	The network layer currently sends packets with aSrc == NULL.
	*	This may change in future (the network instance is used
	*	instead).
	*
	* If hook is configured to receive packets to Send function from some
	* other protocol than the network layer, then it can use the aSrc to
	* detect this and <b>NOT</B> call this default implementation for such
	* packets (unless it intends to inject them to the outbound packet
	* stream).
	*
	* Hook will receive outbound packets from the network layer only if
	* it binds as post-processing hook.
	*
	* @param aPacket	The outbound packet.
	* @param aSrc		The originating protocol (or NULL).
	*
	* @pre	RMBufSendPacket::PeekInfoInChain(aPacket).iFlow.IsOpen()
	*/
	{
	if (iPostHook)
		return iPostHook->Send(aPacket, aSrc);

	//
	// Nobody to forward, consume the packet
	//
	RMBufSendInfo *info = RMBufSendPacket::PeekInfoInChain(aPacket);
	if (info)
		info->iFlow.Close();
	aPacket.Free();
	return 1;
	}

EXPORT_C void CProtocolPosthook::Process(RMBufChain &aPacket, CProtocolBase* aSrc)
	/**
	* Process incoming packet.
	*
	* Process passes the inbound packet to the next pre-processing hook
	* (#iInboundHook).
	* The derived pre-processing hook can implement own Process. After processing
	* the packet, it would normally call CProtocolPosthook::Process to pass
	* the packet forward (note that the information block is the basic RMBufPktInfo).
@code
	TInt CProtocolDerivedHook::Process(RMBufChain &aPacket, CProtocolBase *aSrc)
		{
		// Do own packet processing.

		RMBufPktInfo *info = RMBufPacketBase::PeekInfoInChain(aPacket);
		...
		if (packet_is_dropped)
			aPacket.Free();
		else
			CProtocolPosthook::Process(aPacket, aSrc);
		}
@endcode
	* Hook will receive and pass forward inbound packets only if it has
	* bound as pre-processing hook:
@code
	NetworkService()->BindL(this, MIp6Hook::BindPreHook());
@endcode
	*
	* @note
	*	For inbound packets, the aSrc is the NIF object (derived from
	*	CNifIfBase and not from CProtocolBase.).
	* @note
	*	The implementation <b>SHOULD NOT</b> change the aSrc. But, if it
	*	does change the source, the new source must also be a NIF pointer,
	*	and that NIF must also be known to the TCP/IP stack.
	*
	* @param aPacket	The inbound packet
	* @param aSrc		The originating NIF
	*/
	{
	if (iInboundHook)
		iInboundHook->Process(aPacket, aSrc);
	else
		aPacket.Free();
	}

