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
// ip6_hook.h - hook interface for IPv6 extension header handlers
// This module defines the interface between the main IPv6 protocol
// handler (ip6.cpp) and header handlers. A hook need only the basic
// IPv6 header definitions (ip6_hdr.h), it does not need to be aware
// of IPv6 protocol specific classes: it should not include ip6.h!
// This describes a pure interface, there is no respective
// implementation module (ip6_hook.cpp does not exist).
//



/**
 @file ip6_hook.h
 @publishedPartner
 @released
*/

#ifndef __IP6_HOOK_H__
#define __IP6_HOOK_H__

#include <es_prot.h>
#include <nifmbuf.h>	// The interface uses RMBuf with Info
#include "inet6err.h"
#include "flow.h"
#include "ip6_iprt.h"	// ..for CProtocolBaseUnbind class.
#include "apibase.h"

/** @name IP v6 constants 
* @since v7.0
*/
//@{

/**
* A reference constant used to compute protocol ID values for the CProtocolBase::BindL() 
* method of the IP protocol.
*/
const TInt KIp6Hook_ANY = 256;

/**
* A special return value from MIp6Hook::ApplyL().
*
* This signals that the hook didn't handle the extension header,
* which will be passed to the next registered hook
*/
const TInt KIp6Hook_PASS = 0;

/**
* A special return value from MIp6Hook::ApplyL().
* 
* This signals that the hook processed the extension header and
* that the main loop should restart a new hook processing using
* the function's aInfo.iProtocol field.
*/
const TInt KIp6Hook_DONE = 1;

/**
* A flag that must be <b>cleared</b> by MIp6Hook::ApplyL() processing
* in the aInfo.iFlags parameter, if the hook changes the destination or
* source address in the aInfo.
* 
* It causes address and forwarding checks to be run by the main loop.
* 
* Note that this requires the hook to also signal that the extension
* header has been processed by returning KIp6Hook_DONE. 
*/
const TUint KIpAddressVerified	= 0x080000;
//
//	*NOTE*	This flag is added as a stopgap solution for
//			the IPSEC problem, which results if decrypted packet
//			is echoed pack due to some error condition (like
//			Port Unreachable). It may be removed when a final
//			solution for the problem is found (and if no other
//			uses exist). -- msa
/**
* Prevent sending of this packet as ICMP error report.
*
* A flag that if set causes an unconditional drop of a packet,
* if the packet is passed to the ICMP send functionality in
* the IP layer. (see MNetworkService::Icmp4Send or
* MNetworkService::Icmp6Send
*/
const TUint KIpNeverIcmpError	= 0x100000;
// *NOTE*
//	Reuse the same bit as KIpAddressVerified. This
//	is ok, because verified bit is used only for incoming
//	packets, and the keep up is used only for outgoing packets.
//
/**
* KIpKeepInterfaceUp is an internal flag which set or reset
* depending on the state of the KSoKeepInterfaceUp option.
*/
const TUint KIpKeepInterfaceUp	= 0x080000;

/**
* If this bit is set in RMBufPktInfo flags, the ECN ECT bits in
* IP packet should be zeroed.
*/
const TUint KIpNoEcnEct			= 0x200000;
//@}

//
//	RMBufRecvInfo
//	*************
class RMBufRecvInfo : public RMBufPktInfo
/**
* Information for incoming packets.
* 
* This extends the packet information class to record progress of
* processing the received IPv4 or IPv6 packet.
*
* The RMBufChain contains a full IPv6 (or IPv4) packet
* starting from the beginning. The position of the current
* (upper layer or extension header) is indicated by the
* #iOffset field.
*
* @li #iProtocol
*	is always the internet protocol number 	corresponding
*	to the header incidated by iOffset.
*	
* @li #iLength
*	is the length of the FULL packet, starting from the
*	IP header. The upper layer length (that includes
*	the uppler layer header) is always: (iLength - iOffset) >= 0.
*	Note that this can be ZERO!
*
* @li #iIcmp == 0, for normal packet
*
* @li #iIcmp =! 0, for ICMP error report
*
* The source and destination address are loaded from the ip
* header associated with the indicated upper layer header. In
* case of ICMP error report, the addresses are loaded from the
* packet *inside* the ICMP report.
*
* The addresses are always in IPv6 format. If the packet
* was IPv4 packet, the addresses are presented in IPv4 mapped format.
*
* @warning
*	The fact that address is IPv4 mapped DOES NOT mean that
*	the packet is IPv4. It could as well be an IPv6
*	packet with mapped addresses!
*
* @warning
*	RMBufRecvInfo is assuming that it fits
*	into the single RMBuf block.
*
* @publishedPartner
* @released
* @since v7.0
*/
	{
public:
	TInt CheckL(TInt aLength) const
	/**
	* Tests that the specified length field does not exceed the
	* remaining space in the buffer.
	*
	* Verify that there is enough data after in the packet
	* after the #iOffset octets. This simply tests whether
	* (aLength > #iLength - #iOffset), and leaves
	* with #KErrInet6ShortPacket, if true.
	*
	* @param aLength				Length to test.
	* @return aLength				(for convenience)
	* @leave KErrInet6ShortPacket	Insufficient space.
	*/
		{
		if (aLength > iLength - iOffset)
			User::Leave(KErrInet6ShortPacket);
		return aLength;
		}
	/** Index of the logical source network interface. */
  	TUint32 iInterfaceIndex;
	/** Index of the physical original network interface. */
 	TUint32 iOriginalIndex;
	//
	// IP Information
	//
	/**
	* An offset that indicates the beginning of the current header
	* being processed.
	* 
	* Inbound hooks must update this if they consume a header
	* within the packet.
	* 	
	* It initially points to the first header after the IP header.
	* Offset to the header being processed.
	*/
	TInt iOffset;
	/**
	* Offset to the related IP header.
	* 
	* This is usually zero, but is non-zero for ICMP error reports, and could be 
	* non-zero for tunneled packets.
	*/
	TUint16 iOffsetIp;
	/** Offset of the previous Next Header field.
	* 
	* If a hook consumes an extension header and advances #iOffset to the
	* next header, it must also set this to point to the Next Header
	* field of the former header.
	*
	* This is initialized to refer the next header field of the IP header.
	*
	* This can be used by header handlers which remove the
	* header from the packet. For example, IPSEC does this for AH and
	* ESP headers. IPSEC must be able to correct the protocol/next header
	* field of the previous header.
	*/
	TUint16 iPrevNextHdr;
	/**
	* IP Version (4 or 6) of the related IP header.
	*/
	TUint8 iVersion;
	/**
	* ICMP packet flag.
	* 	
	* This determines the interpretation of the information fields:
	* iType, iCode, and iParameter.
	*
	* @li
	*	iIcmp == 0,	The buffer contains normal upper layer packet,
	*	the header starting from the indicated iOffset.
	*	The values of the iType, iCode and iParameter are undefined..
	*
	* @li
	*	iIcmp != 0,	The buffer contains an ICMP error report for
	*	the upper layer protocol, the returned upper layer header
	*	starting from the indicated iOffset. The #iOffsetIp indicates
	*	the start of the problem packet.
	*
	* Valid values are: 0, #KProtocolInetIcmp, or #KProtocolInet6Icmp.
	*/
	TUint8 iIcmp;
	/**
	* ICMP Type (0..255).
	* 
	* This applies to both ICMPv4 and ICMPv6.
	*
	* (only defined if the field iIcmp != 0)
	*/
	TUint8 iType;
	/**
	* ICMP Code (0..255).
	* 
	* This applies to both ICMPv4 and ICMPv6.
	*
	* (only defined if the field iIcmp != 0)
	*/
	TUint8 iCode;
	/**
	* The last 32 bits from the ICMP header.
	*
	* (only defined if the field iIcmp != 0)
	*/
	TUint32 iParameter;
	};

typedef class RMBufInfoPacketBase<RMBufRecvInfo> RMBufRecvPacket;


class MPacketContext : public MInetBase
/**
* Provides a packet context as a number of (key, value) pairs.
*
* The rules for construction of the key are:
* @li
*	The low eight bits of the key contain always the protocol number
*	(extension header) being implemented by the hook.
* @li
*	If the protocol is destination option or hop-by-hop option,
*	the implemented option types are communicated to the default
*	handler by adding a non-zero value to the packet context with
*	a key computed as "(option-type << 8) | protocol".
*
* @publishedPartner
* @released
* @since v7.0
*/
	{
public:
	/**
	* Sets a (key,value) pair.
	* 
	* If a setting already exists for the key, the value is just replaced.
	*
	* @param aId	Key
	* @param aValue	Value associated with the key
	* @return
	* @li	KErrNone, if value stored successfully.
	* @li	KErrNoMemory, if there was no room for the new value
	*/
	virtual TInt SetHookValue(const TUint32 aId, const TUint32 aValue) = 0;
	/**
	* Gets the value associated with the specified key.
	*
	* Return the current value associated with aId. If aId does not
	* exist, ZERO is returned [=> there is no way to differentiate
	* between non-existing value and a value that is explicitly set
	* to zero. Implementation may interpret setting value to ZERO
	* as request to delete the association, if it exists].
	*
	* @param aId	Key
	* @return	The value, or 0 if no value was found for the key..
	*/
	virtual TUint32 HookValue(const TUint32 aId) const = 0;
	};

class RMBufHookPacket : public RMBufRecvPacket
/**
* Extends the received packet buffer class for hook processing.
* 
* The extension provides a packet context (MPacketContext) for
* the duration of the hook processing. 
* 
* This extension has been created to solve the following problem:
*
*	-#	The default option header handlers need to return ICMP
*		error message on some unimplemented options,
*	-#	The basic design idea of the stack is that functionality
*		can be dynamically added. Thus, if a dynamically loaded
*		module adds support for some new option type, the default
*		handler should not report error for such options.
*
* The rules of the context use are:
*
*	-#	While the incoming packet is processed with hooks, the IP
*		layer maintains a packet specific context, which can store
*		values (32 bits) associated with a key (32 bits).
*	-#	The low 8 bits of the key is defined to be the protocol number
*		of the header, and interpretation of the rest of the key bits
*		is up to protocol/header specific definitions.
*	-#	For destination and hop by hop headers, to solve the problem,
*		the additional specification is used: the default handlers will
*		look a value from packet context with the following key:
*			- (optiontype << 8) | (protocol)
*			.
*		and if the returned value is non-ZERO, the default handler
*		will assume someone implemented the option in question and
*		does not generate an error.
*
* @note
*	The packet context is only available during "hook processing".
*	It is not available for upper layer prototocols!
*
* @publishedPartner
* @released
* @since v7.0
*/
	{
public:
	/**
	* Constructor
	* @param aContext Packet context
    */
	inline RMBufHookPacket(MPacketContext *const aContext) : iContext(aContext) {}
	inline TInt SetHookValue(const TUint32 aId, const TUint32 aValue)
	/**
	* Sets a (key,value) pair.
	* 
	* If a setting already exists for the key, the value is just replaced.
	*
	* @param aId Key
	* @param aValue  Value associated with the key
	* @return  KErrNone, if the value was stored,
	* @return  KErrNoMemory, if there was no room to store the value
	*/
		{ return iContext->SetHookValue(aId, aValue); }
	
	inline TInt HookValue(const TUint32 aId) const
	/**
	* Gets the value associated with the specified key. 
	* 
	* @param aId	Key
	* @return		The value, or 0 if no value was found for the key 
	*
	* Note: There is no way to distinquish between 'no stored value'
	* and 'stored value = 0'.
	*/
		{ return iContext->HookValue(aId); }
private:
	// The packet context handler. This is always defined while the
	// packet is being processed by the hooks.
	MPacketContext *const iContext;
	};

//
//	MIp6Hook
//
class MIp6Hook : public MInetBase
/** Abstract IP hook interface.
* 
* A protocol which binds to the stack as a hook
* must implement this interface.
*
* @warning
*	Even though this is a mixin class, all protocol hooks
*	<b>MUST</b> be derived from CIp6Hook, which includes MIp6Hook.
*	The stack assumes that all hooks use CIp6Hook derived
*	classes. This is only for hooks, an upper layer protocol
*	can be anything derived from CProtocolBase. However, the
*	recommended base class for the upper layer is CProtocolInet6Binder.
*	The recommended generic base class for any hook is CProtocolPosthook,
*	which inherits from CIp6Hook.
*
* An IPv6 packet can have several layers of extension headers
* to be peeled off, before the actual transport layer is reached.
* This process must be done by the installed hooks following this
* API. The stack includes default hooks for the obligatory extension
* headers (destination, hop-by-hop and routing headers). A hook which
* implements an extension header for the protocol number N, attaches
* itself to the stack by calling
@code
	NetworkService()->BindL(this, BindHookFor(N));
@endcode
* and starts receiving a call to the ApplyL() function for
* each protocol header N. It is possible that one packet contains
* multiple instances of the header N, and ApplyL is called
* for each of them. If multiple hooks register for the same
* protocol, they are called sequentially until any of them
* handles the header or rejects the packet.
*
* A hook can also register to be called just before the packet
* is going to be passed to the upper layer by calling:
@code
	NetworkService()->BindL(this, BindHookAll());
@endcode
* Any number of hooks can register this way and the ApplyL()
* is called sequentially until any of them rejects the packet.
* If all of them pass the packet, the packet goes to the
* upper layer protocol.
*
* A hook can also process outgoing packets. This type of hook
* should implement OpenL(), SetFlowOption() and GetFlowOption().
* The outbound processing is totally different from the inbound
* processing, and is based on the flow architecture and MFlowHook.
* Registerning for outbound flow processing is
@code
	NetworkService()->BindL(this, BindFlowHook());
@endcode
* If a hook is interested in packets which the stack would
* forward, if forwarding is enabled, there is a binding code
* for that too:
@code
	NetworkService()->BindL(this, BindForwardHook());
@endcode
* The ApplyL() of the forwarding hook is called for each received
* packet, which does not have a destination address of this node
* (either unicast or multicast group).
*
* Finally, a hook can be a post processing hook, which sees raw
* packets as they come from the network interfaces (inbound) or are
* going out to the interface (outbound).
* The base class for a post processing hook should be CProtocolPosthook.
* The registering for post processing hook is:
@code
	NetworkService()->BindL(this, BindPostHook()); // for outbound
	NetworkService()->BindL(this, BindPreHook());  // for inbound.
@endcode
*
* All hooks can monitor which interfaces are attached to the stack.
* This requires implementing InterfaceAttached() and InterfaceDetached(). 
*
* @publishedPartner
* @released
* @since v7.0
*
* @dontinclude mip6hook.cpp
* Example using a hook class
* @skip class CHookExample
* @until //-
* and registering hooks with the stack to handle extension header
* in all incoming and outgoing packets:
* @skip ::NetworkAttachedL
* @until //-
*/
	{
public:
	inline static TUint BindHookFor(TUint8 aProtocol)
		/**
		* Gets the ID value for binding as an inbound hook for the
		* specified protocol.
		* 
		* @param aProtocol Protocol number (0..255) for which to get ID
		* @return 		   The ID value
		*/
		{ return KIp6Hook_ANY + aProtocol; }
	inline static TUint BindHookAll()
		/**
		* Gets the ID value for binding as an inbound hook for all protocols.	
		* 
		* @return	The ID value
		*/
		{ return 2*KIp6Hook_ANY; }
	inline static TUint BindFlowHook(TUint8 aPriority = 1)
		/**
		* Gets the ID value for binding as an outbound flow hook
		* with the specified priority.
		* 
		* The priority determines the calling order for OpenL() sequence.
		* 
		* @param aPriority Priority value (allowed range [1..255])
		* @return 		   The ID value
		*/
		{ return 2*KIp6Hook_ANY + Max(Min(KIp6Hook_ANY-1, aPriority), 1); }
	inline static TUint BindPostHook()
		/**
		* Gets the ID value for binding as an outbound packet
		* post-processor (below the IP layer).
		*
		* See CProtocolPosthook.
		* 
		* @return	The ID value
		*/
		{ return 3*KIp6Hook_ANY; }
	inline static TUint BindPreHook()
		/**
	 	* Gets the ID value for binding as an inbound packet
		* pre-processor (below IP layer)
		* 
		* See CProtocolPosthook.
		* 
		* @return	The ID value
		*/
		{ return BindPostHook()+1; }
	inline static TUint BindForwardHook()
		/**
	 	* Gets the ID value for binding as a forwarding hook
		* 
		* @return	The ID value
		*/
		{ return BindPreHook()+1; }
	/**
	* Processing of incoming packet.
	*
	* Depending on the how the hook binds to the stack, the stack calls
	* this function from different places during the inbound packet
	* processing path:
	*
	* @li
	*	to implement new (or just to monitor occurrence of) header, do a bind
	*	with BindHookFor(protocol) and the stack calls this function whenever
	*	a header of the protocol is encountered within the packet (some headers
	*	can appear more than once per packet). The RMBufRecvPacket::iProtocol
	*	contains the protocol.
	* @li
	*	to watch all packets for upper layer protocols, do a bind with
	*	BindHookAll() and the stack calls this function for every packet
	*	about to be passed on to the upper layer protocol (identified
	*	by RMBufRecvPacket::iProtocol).
	* @li
	*	to watch all packets which stack would forward (or drop if forwarding
	*	is disabled), do a bind with BindForwardHook(), and the stack calls
	*	this function whenever a packet would be forwarded.
	*
	* The same hook can request all of the above callbacks. However, then
	* the function may have some difficulties in determining the type
	* of call from the packet and associated information.
	*
	* In addition to normal packet parsing (RMBufRecvInfo::iIcmp == 0),
	* the ApplyL is also called when processing a returned packet within
	* the ICMP error message (RMBufRecvInfo::iIcmp != 0).
	*
	* The function receives the packet and information about the state of
	* its processing.
	*
	* The hook has three choices of returns as follows:
	* 
	* @li < 0:
	*	The hook dropped or passed the packet elsewhere.
	*	The main loop goes to the next packet
	* 
	* @li #KIp6Hook_PASS (= 0):
	*	The hook has completed, and the header is still in the packet, 
	*	and has possibly been modified. The main loop continues processing
	*	this header with the next hook or protocol
	* 
	* @li #KIp6Hook_DONE (= 1):
	*	The hook has completed, the header has been handled. The 
	*	hook is responsible for updating the iOffset and other fields
	*	to skip over the processed header.
	*	The main loop will restart to process the new protocol.
	* 
	* In the case of a ICMPv6 Parameter Problem message, the value of the
	* aInfo.iParameter is an offset to the problematic value relative to
	* the start of the original packet. To check whether the parameter
	* problem applies to the current header, the code must test whether
	* the offset falls between 
@verbatim
	0 <= (iParameter + aInfo.iOffsetIp - aInfo.iOffset) < header_length
@endverbatim
	*
	* @param aPacket
	*	The received packet. On return, the packet as modified by the hook.
	* @param aInfo
	*	The packet information. On return, the information as modified by
	*	the hook.
	* @return
	*	Return code, as described above. 
	* @leave error
	*	The packet is dropped and buffers are released.
	*
	* Example: @ref doc_example_1
	* @dontinclude mip6hook.cpp
	* @skip class TExtensionHeader
	* @until //-
	* Only this hoook knows how to handle it. The stack needs the help of this
	* hook for normal packets, and also for processing the returned packet
	* inside the ICMP error reports.
	* @skip ::ApplyL
	* @until //-
	*/
	virtual TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo) = 0;

	/**
	* Opening a hook for a flow.
	*
	* The OpenL is called once at flow opening phase, if the hook has
	* registered a flow hook using BindFlowHook() id.
	* OpenL must decide whether the flow needs any processing by this hook.
	* If yes, it must return with a non-NULL pointer to an instance of
	* MFlowHook. The returned handler is attached to the flow until it
	* closes,  which event is informed to the hook by the MFlowHook::Close
	* method.
	*
	* @param aHead
	*	Contains the address information of the flow.
	*	A hook can update this information in the Open phase, if required.
	* @param aFlow The flow for which the hook is being activated
	* @return
	*	MFlowHook pointer (!= NULL), if the hook attaches to the flow using this handler.
	*	Returning NULL means that the hook has no interest on this flow.
	*
	* @leave error (< 0).
	*	The flow setup is aborted and the indicated error is passed to
	*	the application.
	* @leave EFlow_PENDING
	*	(leave with anything > 0). The flow setup is aborted and flow
	*	is treated as if no route for the destination was available
	*	(flow is put into pending state).
	*	This may activate additional interface setups.
	*
	* @note
	*	This function has a default implmentation, which returns NULL.
	*
	* Example: @ref doc_example_1
	* Attach to every outbound flow:
	* @dontinclude mip6hook.cpp
	* @skip ::OpenL
	* @until //-
	*
	* But, then we need to suply the MFlowHook methods as well.
	* @skip ::ReadyL
	* @until //-
	*
	* and 
	*
	* @skip ::ApplyL
	* @until //-
	*/
	virtual MFlowHook *OpenL(TPacketHead &aHead, CFlowContext *aFlow)
		{
		(void)aHead; (void)aFlow;	// silence compiler warnings
		return NULL;
		}
	/**
	* Implements additional flow options in the hook.
	*
	* When a hook registers for outbound packets, it will also get these calls whenever
	* the upper layer uses the GetOption to a flow.
	*
	* @note
	*	 This function has a default implementation, which returns KErrNotSupported.
	* @note
	*	The flow does not need to be open when this call occurs. If hook implements
	*	any options, it should use the CFlowContext::RetrieveOption for the current
	*	value of the option.
	*
	* @param aLevel		The option level code
	* @param aName		The option name code
	* @retval aOption   The option value (if KErrNone)
	* @param aFlow		The flow
	* @return error code (KErrNotSupported) or KErrNone
	*
	* Example: @ref doc_example_1
	* The current example port and protocol number can be read by a socket option
	* by any application code. Assuming socket is an opened RSocket (for example,
	* an UDP socket), then
	*
	* @code
	TPckgBuf<TUint> opt;
	RSocket socket;
	if (socket.GetOpt(KSoHookExample_PROTOCOL, KSolHookExample, opt) == KErrNone)
		{
		...
		protocol = opt();
		...
		}
	@endcode
	*
	* enters the GetFlowOption function in the example hook:
	* @dontinclude mip6hook.cpp
	* @skip ::GetFlowOption
	* @until //-
	*/
	virtual TInt GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, const CFlowContext &aFlow) const
		{
		(void)aLevel; (void)aName; (void)aOption; (void)aFlow;	// silence compiler warnings
		return KErrNotSupported;
		}
	/**
	* Implements additional flow options in the hook.
	*
	* When a hook registers for outbound packets, it will also get these calls whenever
	* the upper layer uses the SetOption to a flow.
	*
	* @note
	*	 This function has a default implementation, which returns KErrNotSupported.
	* @note
	*	The flow does not need to be open when this call occurs. The hook should not store
	*	the pointer of the flow. Instead, it should use the CFlowContext::StoreOption to
	*	remember the option values.
	*
	* @param aLevel		The option level code
	* @param aName		The option name code
	* @param aOption	The option value
	* @param aFlow		The flow
	* @return error code (KErrNotSupported) or KErrNone
	*
	* Example: @ref doc_example_1
	* The example port and protocol number can be changed by a socket option.
	* Assuming socket is an opened RSocket (for example, an UDP socket), then
	* @code
	TPckgBuf<TUint> opt;
	RSocket socket;
	opt() = 18;
	if (socket.SetOpt(KSoHookExample_PROTOCOL, KSolHookExample, opt) == KErrNone)
		{
		// Succesfully changed the protocol number!
		}
	@endcode
	*
	* enters the SetFlowOption function in the example hook:
	* @dontinclude mip6hook.cpp
	* @skip ::SetFlowOption
	* @until //-
	*/
	virtual TInt SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow)
		{
		(void)aLevel; (void)aName; (void)aOption; (void)aFlow;	// silence compiler warnings
		return KErrNotSupported;
		}
	/**
	* Monitoring attached interfaces.
	*
	* A hook can monitor what interfaces are attached
	* to the stack by overriding the MIp6Hook::InterfaceAttached and
	* MIp6Hook::InterfaceDetached.
	*
	* The InterfaceAttached is called just after the CNifIfBase
	* pointer has  been stored into the internal interface
	* instance and CNifIfBase::Open() has been called.
	*
    * @note
	*	It is possible to receive InteraceDetached
	*	without a matching InterfaceAttached, because interfaces can
	*	be up before the hook is active.
	*
	* @param aName   The name of the interface within the stack
	* @param aIf	 The interface
	*/
	virtual void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf) {(void)aName; (void)aIf;}
	/**
	* Monitoring attached interfaces.
	*
	* A hook can monitor what interfaces are attached
	* to the stack by overriding the MIp6Hook::InterfaceAttached and
	* MIp6Hook::InterfaceDetached.
	*
	* The InterfaceDetached is called just before the CNifIfBase
	* pointer is  going to be removed from the internal interface
	* instance and before calling the CNifIfBase::Close().
	*
    * @note
	*	It is possible to receive InteraceDetached
	*	without a matching InterfaceAttached, because interfaces can
	*	be up before the hook is active.
	*
	* @param aName   The name of the interface within the stack
	* @param aIf	 The interface
	*/
	virtual void InterfaceDetached(const TDesC & aName, CNifIfBase *aIf) {(void)aName; (void)aIf;}
	};



class CIp6Hook: public CProtocolBaseUnbind, public MIp6Hook
/**
* The base class of all hook protocols.
*
* See MIp6Hook and CProtocolBaseUnbind.
*
* @publishedPartner
* @released
* @since v7.0
*/
	{
public:
	/**
	* Processes an incoming packet.
	* 
	* @see MIp6Hook::ApplyL().
	* @param aPacket 	Packet to process
	* @param aInfo 		Packet information
	* @return 			System-wide error code
	*/
	virtual TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo) = 0;

public:
	//
	// Silence compiler
	//
	/**
     * dummy
     */
	void Identify(struct TServerProtocolDesc *) const {}	// should put something here!!
	/**
     * The inbound hooks don't need this really
     */
	void Unbind(CProtocolBase *, TUint) {}		// The inbound hooks don't need this really.
	};

#endif
