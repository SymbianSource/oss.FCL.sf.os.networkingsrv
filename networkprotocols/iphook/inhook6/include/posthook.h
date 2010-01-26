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
// posthook.h - Base class for post hooks
// Base class for post hooks.
//



/**
 @file posthook.h
 @publishedPartner
 @released
*/

#ifndef __POSTHOOK_H
#define __POSTHOOK_H

#include <e32std.h>
#include <es_sock.h>

#include "in_bind.h"

// *****************
// CProtocolPostHook
// *****************

class CProtocolPosthook : public CIp6Hook
	/**
	* Base class for the hooks between IP and NIF.
	*
	* This is the base class for hooks that live between the IP and
	* NIF layer (post-processing and pre-processing hooks). However,
	* this is also suitable for the base class of any hook. The class
	* provides automatic "network service detection", and supports both
	* <b>binding to</b> and <b>binding from</b> IP6.
	*
	* The implementations of CProtocolPosthook::BindL and
	* CProtocolPosthook::Unbind handle the "chaining" feature of the
	* post processing hooks. The stack uses a sequence of BindL and
	* Unbind calls when inserting or removing a protocol.
	*
	* Outbound (MIp6Hook::BindPostHook()):
@verbatim
 U ---> IP6          F ---> NIF
 ..Send.. \         /
           \   ....P ->
            \                      |
             H1 --> H2 --> ... --> T
             ...Send...Send...Send
@endverbatim
	* The MNetworkService::Send passes the packet (P) to the CProtocolBase::Send
	* of the first post processing hook (H1). It is the responsibility of
	* H1 to pass the packet to the next post-processing hook (H2). The stack
	* maintains an internal chain terminator (T), which will pass the packet
	* to the NIF using the flow context (F) attached to the packet. The
	* Send does the packet forwarding to the next linked hook automaticly.
	* 
	* The information block associated with the packet is defined
	* by RMBufSendPacket , which always includes a reference to the
	* flow context (F). If a hook before the terminator (T) does
	* not pass the packet forward in chain, then it must also
	* take care of releasing the flow context
	* (RMBufSendInfo::iFlow.Close()).
	*
	* The flow context is the only way for the terminator (T) to
	* know the target NIF. If the packet has not flow context, the
	* terminator drops the packet. The flow context is detached
	* from the packet before it is passed to the NIF.
	*
	* Inbound (MIp6Hook::BindPreHook()):
@verbatim
   NIF --> IP6                     IP6 ---> U
  ..Process..\                      |
              \   ......P ->        |
               \     src=NIF        |
               H1 --> H2 --> .. --> T
            ..Process..Process..Process
@endverbatim
	* The MNetworkService::Process passes the incoming packet
	* to the CProtocolBase::Process of the first pre-processing hook
	* (H1). It is the responsibility of the H1 to pass the packet
	* to the next pre-processing hook (H2). The stack maintains
	* an internal terminator (T), which will pass the packet
	* upwards in the protocol stack. The CProtocolPosthook::Process
	* does the packet forwarding to the next linked hook automaticly.
	*
	* In the Process chain, the source (CProtocolBase *) is actually
	* a pointer to the NIF object (CNifIfBase). Upon arriving to the
	* terminator (T), the source must still be a valid and known to
	* stack as a NIF (successfully introduced as documented in
	* @ref nif_binding
	* at some earlier point). Otherwise packet is not accepted.
	* The information block is defined by the RMBufPktInfo class.
	* The information block "changes into" RMBufRecvInfo after the
	* terminator (T).
	*
	* CProtocolPosthook can also be used as a generic base for any hook.
	* Then the following protocol ids for the BindL will become reserved:
	*
	* - MIp6Hook::BindPostHook()  (for outbound posthook chaining)
	* - MIp6Hook::BindPreHook() (for inbound posthook chaining)
	* - #KProtocolInet6Ip (for the network intance)
	* - #KProtocolInetIp (for network instance, deprecated)
	*
	* ::BindToL implementation handles #KProtocolInet6Ip
	* and #KProtocolInetIp targets. This does not do automatic CProtcolBase::BindL
	* to the target (see ::DoBindToL). The hook binds, if needed, should be done
	* in the ::NetworkAttachedL function.
	*
	* @publishedPartner
	* @released
	*/
	{
protected:
	IMPORT_C virtual ~CProtocolPosthook();
public:
	IMPORT_C virtual void BindToL(CProtocolBase* aProtocol);
	IMPORT_C virtual void BindL(CProtocolBase* aProtocol, TUint aId);
	IMPORT_C virtual void Unbind(CProtocolBase* aProtocol, TUint aId);
	IMPORT_C virtual TInt Send(RMBufChain &aPacket, CProtocolBase* aSrc);
	IMPORT_C virtual void Process(RMBufChain &aPacket, CProtocolBase* aSrc);
	TInt ApplyL(RMBufHookPacket & /*aPacket*/, RMBufRecvInfo & /*aInfo*/) 
		/** Default ApplyL.
		* The default ApplyL does nothing to the packets.
		* @note
		*	If the only hook binding is post or pre-preprocessing, the ApplyL
		*	is never called.
		*
		* @return #KIp6Hook_PASS
		*/
		{ return KIp6Hook_PASS; }
	/**
	* Network layer has been attached.
	*
	* This function is called when the network layer is detected and
	* attached to this protocol object (::NetworkService() returns non-null).
	* The implementation in the derived class can now
	* do the hook specific binds and unbinds using:
@code
	// request inbound packets to my Process()
	NetworkService()->BindL(this, MIp6Hook::BindPreHook());
	// request outbound packets to my Send().
	NetworkService()->BindL(this, MIp6Hook::BindPostHook());
	...
	// stop getting inbound packets to my Process()
	NetworkService()->Unbind(this, MIp6Hook::BindPreHook());
	// stop getting outbound packets to my Send()
	NetworkService()->Unbind(this, MIp6Hook::BindPostHook());
	// ..or, to rip off all my hooks
	NetworkService()->Unbind(this);
@endcode
	*/
	virtual void NetworkAttachedL() = 0;
	/**
	* Network layer is being detached.
	*
	* This function is called when the hook is losing
	* the connection to the network instance (the network
	* instance has issued Unbind request, because it is
	* shutting down).
	*
	* Derived class does not need to implement this, unless it
	* caches network layer dependent data internally. Otherwise,
	* it must implement the function and cleanup all such data.
	*
	* During the call ::NetworkService returns the
	* service instance that is going away. However, it should not
	* be used for any binding or unbinding. the detach process does
	* the unbindings automaticly.
	*
	* @note
	*	The desctuctor can be called while network is attached.
	*	CProtocolPosthook destructor will unbind this object
	*	automaticly from the network (cancel all binds), and
	*	then detaches withouth calling NetworkDetached. Thus,
	*	the destructor of the derived class must do the cleanup
	*	of cached data, but it does not need to worry about the
	*	binds to the network.
	*/
	virtual void NetworkDetached() {};
	/**
	* Gets the network service.
	*
	* This returns the network service, if any is currently attached.
	* The network is attached after ::NetworkAttachedL call until
	* the next ::NetworkDetached or destruction. Otherwise, network
	* is not attached and return is always NULL.
	*
	* @return The network service or NULL.
	*/
	inline MNetworkService *NetworkService() const { return iNetwork; }
protected:
	IMPORT_C TInt DoBindToL(CProtocolBase *aProtocol);
private:
	/** The attached network layer (IP layer).
	* The CProtocolPosthook::DoBindToL, CProtocolPosthook::BindL
	* and CProtocolPosthook::Unbind implementations maintain this
	* pointer.
	*/
	MNetworkService *iNetwork;
	/** Outbound posthook chain.
	* The next protocol in list. The CProtocolPosthook::BindL and
	* CProtocolPosthook::Unbind implementions maintain this chain
	* based on the calls coming from the network layer. The id
	* reference in the chaining calls is MIp6Hook::BindPostHook().
	*/
	CProtocolBase *iPostHook;
	/** Inbound posthook chain.
	* The next protocol in list. The CProtocolPosthook::BindL and
	* CProtocolPosthook::Unbind implementions maintain this chain
	* based on the calls coming from the network layer. The id
	* reference in the chaining calls is MIp6Hook::BindPreHook().
	*/
	CProtocolBase *iInboundHook;
	/**
	* The network attachment type.
	* This base class supports both "bind" and "bindto" attachments
	* to the network layer.
	*
	* - if == 1, <tt>bindto= ip6</tt> is in the "hook" ESK file.
	*	#iNetwork has been set by CProtocolPosthook::DoBindToL.
	* - if == 0, <tt>bindto= hook</tt> is in the [ip6] section of
	*	the TCPIP6.ESK. #iNetwork has been set by the
	*	CProtocolPosthook::BindL.
	*
	* The value is significant only if #iNetwork is non-NULL.
	*/
	TUint iBindToNet:1;
	};

#endif
