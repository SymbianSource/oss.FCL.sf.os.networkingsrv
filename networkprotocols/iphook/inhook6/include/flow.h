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
// flow.h - IPv6/IPv4 flow information
// IPv6/IPv4 flow information.
//



/**
 @file flow.h
 @publishedPartner
 @released
*/

#ifndef __FLOW_H__
#define __FLOW_H__

#define SYMBIAN_NETWORKING_UPS

#include <es_prot.h>
#include <nifmbuf.h>
#include "in_pkt.h"
#include "apibase.h"

//
//	TFlowStatus
//
/**
* Type of the flow status (some symbolic enum names).
* @since v7.0
* @publishedPartner
* @released
*/
enum TFlowStatus
	{
	/**
	* (< 0) Flow is in error state.
	*
	* All other system wide error codes also indicate a flow error state. 
	* To recover, the flow must be reconnected.
	*/
	EFlow_DOWN = KErrNotReady,
	/**
	* (= 0) Flow is ready to send data.
	*/
	EFlow_READY = 0,
	/**
	* (= 1) Flow is temporarily blocked.
	*
	* This is used when the flow is waiting for the connection setup. 
	* When there is a possibility that flow could change into EFLow_READY
	* state, the notifier is notified with MProviderNotify::CanSend().
	*/
	EFlow_PENDING = 1,
	/**
	* (= 2) Flow is temporarily blocked.
	*
	* This is used when the flow is blocked due to congestion (e.g. lower level 
	* buffers are full). When congestion clears and if there is a possibility
	* that flow could change into EFLow_READY state, the notifier is notified
	* with MProviderNotify::CanSend().
	*/
	EFlow_HOLD = 2
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	,
    EFlow_NOTCONFIGURE = 3
#endif //SYMBIAN_TCPIPDHCP_UPDATE    
	};

//	MProviderNotify
//	***************
class MProviderNotify : public MInetBase
	/**
	* Receiver of the notifications of the status changes in the flow.
	*
	* Note that the two functions CanSend() and Error() are a subset of MSocketNotify. This allows 
	* a Service Access Point to implement the interface just by calling the equivalent 
	* MSocketNotify functions. 
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* Flags that the flow might be ready to change into the EFlow_READY status
	*
	* The flow is (or has been) in EFlow_PENDING or EFlow_HOLD state. The flow
	* owner can now check if flow really can be changed into EFlow_READY state.
	* The state is updated by calling RFlowContext::Status (or some other functions
	* that implicitly refresh the flow state by calling internally the
	* CFlowContext::RefreshFlow function).
	*
	* Even if this function is called, the flow can still be in error or blocked
	* state. This is because, for example, when congestion clears, all flows
	* waiting for that event are notified, but some earlier notified flow may
	* already have refilled all the buffers.
	*/
	virtual void CanSend() = 0;
	/**
	* The flow has entered an error state.
	*
	* Error state in a flow is permanent and requires a new connect (CFlowContext::Connect)
	* to be cleared. A new connect occurs implicitly at FlowContext::Status (or some
	* at some other functions), if flows connection parameters have been modified
	* since the last connect (CFlowContext::iChanged is 1).
	*
	* @param aError Flow error code
	* @param anOperationMask A bitmask of MSocketNotify::TOperationBitmasks values 
	* specifying which pending operations are affected by the error up-call.
	*/
	virtual void Error(TInt aError, TUint anOperationMask=MSocketNotify::EErrorAllOperations) = 0;
	virtual void NoBearer(const TDesC8& aConnectionParams) = 0;
	virtual void Bearer(const TDesC8 &aConnectionInfo) = 0;
	virtual TInt CheckPolicy(const TSecurityPolicy&, const char *) 
		{
		return KErrNone;
		};
	};


//
//	RFlowContext
//	************
//
class MFlowManager;
class CFlowContext;
class CNifIfBase;

//	*WARNING*:
//		Because RFlowContext is used as a member of RMBufSendInfo, which may
//		get moved around, the RFlowContext handle *MUST* not contain anything
//		that breaks if it is copied bit-by-bit from one memory area to another!
//		(for example don't even think of adding link fields and linking
//		RFlowContext's into a list!) -- msa
//
class RFlowContext
	/**
	* A handle to a flow context.
	*
	* The main purpose of this class is to provide automatic reference counting 
	* for flow context users.
	*
	* A typical use for a RFlowContext object is to:
	* @li	allocate a context to a handle through the flow manager (Open())
	* @li	set appropriate flow parameters (the upper layer protocol associated with 
	* 		the flow, destination and source addresses)
	* @li 	use the flow to send packets out
	* @li 	release the context (Close()) from the handle. 
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	inline RFlowContext() : iFlow(0)
		/**
		* Default constructor.
		*
		* This creates a handle without any assigned flow context.
		*/
		{}
	IMPORT_C TInt Open(MFlowManager *aManager, const TSockAddr &aDst, const TSockAddr &aSrc, TUint aProtocol = 0, TUint aIcmpType = 0, TUint aIcmpCode = 0);
	IMPORT_C TInt Open(MFlowManager *aManager, TUint aProtocol = 0);
	IMPORT_C TInt Open(RFlowContext &aContext, RMBufPktInfo *aInfo = NULL);
	IMPORT_C TInt Clone(const RFlowContext &aFlow);
	IMPORT_C TInt ReOpen();
	IMPORT_C TInt Connect();

	inline TBool IsOpen()
		/**
		* Tests if context is attached
		*/
		{return iFlow != NULL;}

	IMPORT_C void SetRemoteAddr(const TSockAddr &aAddr);
	IMPORT_C void SetLocalAddr(const TSockAddr &aAddr);
	IMPORT_C void SetProtocol(TUint aProtocol);
	IMPORT_C void SetIcmpType(TUint aType, TUint aCode = 0);
	IMPORT_C MProviderNotify *SetNotify(MProviderNotify *aProvider);
	IMPORT_C TInt Status();
	IMPORT_C void Grab(RFlowContext &aContext);
	IMPORT_C void Copy(RFlowContext &aContext);
	IMPORT_C void Close();
	IMPORT_C CNifIfBase *Interface() const;
	
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	IMPORT_C TBool IsNdResolutionPending(); //RFC 4861 - Section 7.2.2
#endif //SYMBIAN_TCPIPDHCP_UPDATE
	inline CFlowContext *FlowContext() const
		/** Gets a pointer to the flow context object (CFlowContext)
		* attached to the handle.
		*
		* This can be used for more detailed control of the flow parameters.
		*
		* The returned pointer has limited validity,
		* and it should not be stored in any members of permanent objects.
		* In addition, care must be taken not to call any operation that could
		* destroy the flow context as a side effect.
		*
		* If there is a need for longer validity of the retrieved pointer,
		* the CFlowContext::Open and CFlowContext::Close methods can be used
		* to protect it.
		*
		* @return
		*		The flow context object.
		*/
		{ return iFlow; }
private:
	/** The flow context. */
	CFlowContext *iFlow;
	};



//	******************************
//	RMBufSendInfo, RMBufSendPacket
//	******************************
class RMBufSendInfo : public RMBufPktInfo
	/**
	* Information for outgoing packets.
	*
	* This extends the packet information class to record the flow context. 
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	/** Flow context for the packet. */
	RFlowContext iFlow;	
	};

/** @since v5.0 */
typedef class RMBufInfoPacketBase<RMBufSendInfo> RMBufSendPacket;

// TFlowInfo
// *********
class TFlowInfo
	/**
	* Collects the information which defines a flow 
	* (The Upper Layer Flow Information). This is a
	* member of CFlowContext.
	*
	* @since v7.0s
	*/
	{
public:
	/**
	* Current remote addess as set by upper layer.
	*
	* Must always be specified before a flow can be connecte or
	* used for sending packets.
	*
	* See RFlowContext::SetRemoteAddr, CFlowContext::RemoteAddr
	*/
	TInetAddr iRemote;
	/**
	* Current local address (system or application selected).
	*
	* See RFlowContext::SetLocalAddr, CFlowContext::LocalAddr
	*/
	TInetAddr iLocal;
	/**
	* The protocol associated with the flow.
	*
	* See RFlowContext::SetProtocol, CFlowContext::Protocol
	*/
	TUint8 iProtocol;
	/**
	* ICMP type, when protocol is ICMP (or similar).
	*
	* See RFlowContext::SetIcmpType, CFlowContext::GetIcmpTypeCode
	*/
	TUint8 iIcmpType;
	/**
	* ICMP code, when protocol is ICMP (or similar).
	*
	* See RFlowContext::SetIcmpType, CFlowContext::GetIcmpTypeCode
	*/
	TUint8 iIcmpCode;
	/**
	* Set when upper layer set the local address.
	*
	* When set, the stack assumes the upper layer has specified
	* the source address of the flow. When not set, the stack
	* chooses the source address.
	*
	* This flag is cleared or set by the RFlowContext::SetLocalAddr().
	* The flag is cleared when address is unspecified and set otherwise.
	* Initial value is unset, if SetLocalAddr is never called.
	*
	* See also CFlowContext::IsLocalSet
	*/
	TUint iLocalSet:1;
	/**
	* Set when interface errors should not affect the flow.
	*
	* When an interface goes down (or reports an error), all flows
	* that are currently connected (routed) to this interface, are
	* also set into error error state (effectively, causing a
	* socket error to the applications).
	*
	* When this flag is set, flow is not set to the error state. However,
	* if interface is going down, the flow is put into hold/pending
	* state (until another or same interface becomes again available).
	*
	* See also the socket option: #KSoNoInterfaceError
	*/
	TUint iNoInterfaceError:1;
	/**
	* Set when this flow should not try to bring up the interface.
	*
	* When a connect is attempted on a flow and it fails due to
	* missing routes (no suitable interfaces up or configured yet), the
	* stack signals the NIFMAN (NoBearer notify function).
	*
	* When this flag is set, NIFMAN is not notified and the flow is
	* just placed into hold/pending state to wait for possible interface
	* or route to appear.
	*
	* @note
	*	NoBearer does not exisit in pre 7.0s systems. In such
	*	systems the stack itself activates the "netdial process" in
	*	this situation.
	*/
	TUint iNoInterfaceUp:1;
	/**
	* Set when flow is used for packet forwarding.
	*
	* This flag, when set, disables the source address checking.
	* Normally the stack works in "strong model" and requires that
	* a packet has a valid source address on the interface.
	* Forwarded packets have other than local source address and
	* the check must be disabled.
	*
	* This can only be set internally or from the hooks. There is no
	* application level socket option to set this.
	*/
	TUint iForwardingFlow:1;
	// Note! Cannot use TScopeType below, because it would make the
	// bitfield into signed and fail on tests like:
	//		x.iLockType == EScopeType_NET
	// even if x.iLockType has value EScopeType_NET!!! -- msa
	/**
	* Locked scope-1 (0..15) [TScopeType].
	*
	* This valid only when iLockId is non-zero.
	*/
	TUint iLockType:4;
	/**
	* Current Locking Id.
	*
	* Value ZERO is unlocked. Non-Zero value is a zone id in the scope
	* specified by iLockType.
	*/
	TUint32 iLockId;
	};


//	************
//	CFlowContext
//	************
//	A base class of the Flow Context, cannot be instantiated as is
//
class COptionValue;
class MFlowHook;
class MInterfaceManager;

class CFlowContext : public CBase
	/**
	* The flow context instance.
	*
	* The CFlowContext has several public methods, but not all of them are safe
	* (or even legal) to use in all situations.
	*
	* CFlowContext is a reference counted object. Whenever a pointer is
	* stored for long term duration, the Open() must be called, and when
	* pointer is no more used, a close must be called. This object is only
	* deleted indirectly via use of Close(). The delete operator is never
	* used explicitly from outside.
	*
	* Base class for a flow context.
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
	friend class MFlowManager;
	friend class RFlowContext;
protected:
 	IMPORT_C CFlowContext(const void *aOwner, MFlowManager *aManager);
 	IMPORT_C CFlowContext(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow);
	// Destructor should not be exported, it should be private! -- msa
	IMPORT_C virtual ~CFlowContext();

#ifdef SYMBIAN_NETWORKING_UPS
	/**
	Indicate whether the flow has a provider above it.
	@return ETrue if it has, else EFalse.
	*/
	inline TBool HasProvider() const;
	
	
	inline void *GetProviderApiL(const TDesC8& aApiName, TUint* aVersion);
#endif //SYMBIAN_NETWORKING_UPS

public:
	IMPORT_C void Close();

	inline void Open()
		/**
		* Increments a reference count on the context
		*/
		{ iRefs++; }
	IMPORT_C TInt Status();
	IMPORT_C void SetStatus(TInt aStatus);
	IMPORT_C TInt StoreOption(TUint aLevel, TUint aName, const TDesC8 &aOption);
	IMPORT_C TInt RetrieveOption(TUint aLevel, TUint aName, TDes8 &aOption) const;

	inline TPacketHead &Head()
		/**
		* Gets access to precomputed information for the outbound packet flow.
		*
		* @return Precomputed information for the outbound packet flow
		*/
		{ return iHead; }

	/**
	* @defgroup	getselectors	Retrieve current selector fields
	*
	* @{
	*/

 	inline TUint LocalPort() const
		/**
		* Gets the flow's local port.
		* @return current local port
		*/
		{ return iInfo.iLocal.Port(); }

	inline TUint RemotePort() const
		/**
		* Gets the flow's remote port.
		* @return current remote port
		*/
		{ return iInfo.iRemote.Port(); }

	inline TUint Protocol() const
		/**
		* Gets the flow protocol.
		* @return current protocol
		*/
		{ return iInfo.iProtocol; }

	inline const TInetAddr &LocalAddr() const
		/**
		* Gets the flow's local address.
		*
		* The local address may have been selected by the system, if
		* the transport layer didn't set it (left it unspecified or
		* explicitly set unspecified address to it).
		*
		* @note
		*	All addresses in the flow context are kept in IPv6 format
		*	(using IPv4 mapped addresses for IPv4 flows).
		*
		* @return Local address.
		*/
		{ return iInfo.iLocal; }

	inline TBool IsLocalSet() const
		/**
		* Gets the flow's local address status.
		*
		* The local address can be selected by the stack, or application
		* can have set it into a specific value. When this address status
		* have value ETrue, the stack assumes that the local address has
		* been chosen by the application or upper layer protocol. When
		* status is EFalse, the stack selects the source
		* address for the flow when the flow is connected
		* (CFlowContext::Connect).
		*
		* The RFlowContext::SetLocalAddress will set this status to EFalse,
		* if the address is unspecified address, and to ETrue otherwise.
		*
		* @note
		*	If an application requires using the unspecified source address
		*	in packets (IPv4 <tt>0.0.0.0</tt> or IPv6 <tt>::</tt>), it must use
		*	the socket option #KSoNoSourceAddressSelect (level #KSolInetIp)
		*	<em>after</em> it has performed the RSocket::Bind() to uspecified
		*	address.
		*
		* Some upper layer protocols may also set this status, after the
		* connection is established (TCP).
		*
		* @return ETrue, if local addr is set
	 	*/
		{ return iInfo.iLocalSet != 0; }

	inline const TInetAddr &RemoteAddr() const
		/**
		* Gets the flow's remote address.
		*
		* @return Remote address.
		*/
		{ return iInfo.iRemote; }
		
	inline void GetIcmpTypeCode(TUint8 &aType, TUint8 &aCode) const
		/**
		* Gets the flow's ICMP type and code.
		*
		* @retval aType	Icmp (or other) type
		* @retval aCode	Icmp (or other) code
		*/
		{ aType = iInfo.iIcmpType; aCode = iInfo.iIcmpCode; }

	inline TScopeType LockType() const
		/**
		* Gets the flow's locking type [0..15].
		*
		* The locking type tells the type of the lock id. The type has no
		* meaning if the lock id has zero value (= flow is not locked)..
		*
		* @return current type of LockId (IF=0, IAP=1, NET=15).
		*/
		{ return (TScopeType)iInfo.iLockType; }

	inline TUint32 LockId() const
		/**
		* Gets the flows lock id.
		*
		* A flow can be locked to a specified scope. When locked (non-zero),
		* the flow can only be connected to an interface within the locked
		* scope.
		*
		* The id value is a plain 32 bit number and the domain of this
		* value is defined by the locking type (CFlowContext::LockType).
		*
		* @return current lock id (IAP, NET or IF). Not locked, if ZERO
		*/
		{ return iInfo.iLockId; }

	/** @} */

	/**
	* @defgroup packetsize	Accessing parameters of the packet size
	*
	* In all, return
	*	@li	< 0, indicates an error or value not known
	*	@li	= 0, (interpretation not fixed)
	*	@li	> 0, the indicated value
	*
	* Some assertations that should be true
	*	@li	PathMtu() > HeaderSize()
	*	@li	HeaderSize() >= sizeof(TInet6HeaderIP)
	*	@li	InterfaceSMtu() >= PathMTU()
	*	@li	InterfaceRMtu() > sizeof(TInet6HeaderIP)
	*
	* @{
	*/

	inline TInt PathMtu() const
		/**
		* Gets the Path MTU of the flow.
		*
		* @return
		* @li	< 0,    indicates an error or value not yet known (for example,
		*				if accessed before the flow is connected or interface is up)
		* @li	= 0,    value not known
		*
		* @li	> 0,     a real value
		*/
		{ return iPathMtu; }

	inline TInt HeaderSize() const
		/**
		* Gets the amount of the protocol overhead in the packet from all of the lower layers.
		*
		* The value is defined only for a flow that has been connected.
		* (CFlowContext::Connect).
		*
		* HeaderSize () > 0 (at least IPv4 or IPv6 header size)
		*
		* @return
		* @li	< 0,    indicates an error or value not yet known (for example,
		*				if accessed before the flow is connected or interface is up)
		* @li	= 0,    value not known
		* @li	> 0,	a real value
		*/
		{ return iHdrSize; }
	
	/**
	* Gets the raw send MTU of the attached interface.
	*
	* The value is defined only for a flow that has been connected.
	* (CFlowContext::Connect).
	*
	* @return
	* @li	< 0,    indicates an error or value not yet known (for example,
	*				if accessed before the flow is connected or interface is up)
	* @li	= 0,    value not known
	* @li	> 0,	a real value
	*/
	virtual TInt InterfaceSMtu() const = 0;
	/**
	* Gets the raw receive MTU of the interface.
	*
	* The value is defined only for a flow that has been connected.
	* (CFlowContext::Connect).
	*
	* @return
	* @li	< 0,    indicates an error or value not yet known (for example,
	*				if accessed before the flow is connected or interface is up)
	* @li	= 0,    value not known
	* @li	> 0,	a real value
	*/
	virtual TInt InterfaceRMtu() const = 0;
	/** @} */

	/**
	* Gets an option from the flow context.
	*
	* A set of options can be read from the flow context.
	*
	* In addition to internally supported options,
	* any registered outbound hook can add support for additional options
	* (see MIp6Hook::GetFlowOption documentation).
	*
	* The function is called part of the normal option processing.
	*
	* @param aLevel The option level
	* @param aName The option name
	* @param aOption The option value
	* @return
	*		KErrNone, or KErrNotSuppoted if option cannot be read from the flow context.
	*/
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8 &aOption) const = 0;

	/**
	* Sets an option to the flow context.
	*
	* A set of options can be set to the flow context.
	*.
	* In addition to internally supported options,
	* any registered outbound hook can add support for additional options
	* (see MIp6Hook::SetFlowOption documentation).
	*
	* The function is called part of the normal option processing.
	*
	* @param aLevel The option level
	* @param aName The option name
	* @param aOption The option value
	* @return
	*		KErrNone, or KErrNotSuppoted if option cannot be set from the flow context.
	*/
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8 &aOption) = 0;

	/**
	* Gets the currently connected interface.
	*
	* @return
	*		The currently connected interface,
	*		if the flow is properly connected, otherwise NULL.
	*		The returned pointer has limited validity, and it should not be stored
	*		in any members of permanent objects.
	*/
	virtual CNifIfBase *Interface() const = 0;

	/**
	* Gets the interface manager of the TCP/IP stack..
	*
	* @return
	*		The interface manager.
	*/
	virtual MInterfaceManager *Interfacer() const = 0;

	/**
	* Sends a packet to the attached interface.
	*
	* This is a low level function intended for the IP layer itself.
	* Send bypasses all the installed hooks and path mtu processing.
	* The raw data in the packet is passed to the interface as is.
	* (for the normal way of sending a packet, see MNetworkService::Send).
	*
	* If the flow is not connected, the Send drops the packet and the
	* return is KErrNotReady.
	*
	* The destination address of the information block may get changed
	* into link layer destination address, if the stack is handling the
	* neighbour discovery on the link (ARP for IPv4, ICMPv6 Neighbor
	* discovery for IPv6).
	*
	* If the destination is a multicast address, and if that address
	* is also joined by some application(s), then a copy of the packet
	* is sent to the inbound direction (MNetworkService::Process),
	* unless disabled by #KSoIp6MulticastLoop socket option.
	*
	* @param aPacket
	*		The data packet (assumed to be RMBufInfoPktBase in "packed" state)
	* @param aSource
	*		The source protocol instance (passed as is to the interface). Optional,
	*		and usually NULL.
	* @return
	*		is defined similarly as the equivalent methods of the interfaces and
	*		protocols, as follows:
	* @li	< 0,
	*		an error: the packet is not sent, but is dropped by Send().
	* @li	= 0,
	*		indicates that the interface received the packet, but is also signaling
	*		that its reluctance to receive more packets. All flows attached to this
	*		interface are automatically set into EFlow_HOLD state.
	* @li	> 0,
	*		indicates that the interface received the packet and is willing to
	*		receive more after this.
	*/
	virtual TInt Send(RMBufChain &aPacket, CProtocolBase* aSource = NULL) = 0;

	/** Attaches a flow to a route and an interface. */
	virtual void Connect() = 0;
	/** Disconnects the flow, and remove all hooks. */
	virtual void Disconnect() = 0;
	/** Recomputes the current flow status. */
	virtual void RefreshFlow() = 0;
	/**
	* Sets "changed" state to flow(s).
	*
	* Sets the iChanged flag that indicates that connect information has changed.
	* 
	* When any component of the system determines that a flow or set
	* of flows require an open phase (reconnect), the component can use the
	* this function to force a reconnect of the flow on the next outgoing packet.
	*
	* @param aScope determines what flows are affected:
	* @li	0: set iChanged on current flow
	* @li	1: set iChanged on all flows with same route entry,
	* @li	2: set iChanged on all flows with same interface,
	* @li	> 2: set iChanged on all existing flows
	*
	* @return
	*		Number of flows affected (regardless of their previous iChanged state)
	*/
	virtual TInt SetChanged(const TInt aScope = 0) = 0;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	/* RFC 4861: Sec 7.2.2 Verifies any pending ND packets exists on a route during ND
	 */
	virtual TBool IsNdPacketPendingResolution() {return EFalse; } ; // Base implementation
#endif //SYMBIAN_TCPIPDHCP_UPDATE
	inline void NoBearer(const TDesC8& aConnectionParams);
	inline void Bearer(const TDesC8 &aConnectionInfo);
	inline TInt CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic);

protected:
	/** The owner of the flow (untyped ID data). */
	const void *const iOwner;
	/** The flow manager that created this object. */
	MFlowManager *const iMgr;

	/** Contains the object reference count.
	* 
	* For a single reference, this is 0.
	*/
	TInt iRefs;
	/** The flow's status.
	* 
	* @li = 0: up and running
	* @li > 0: pending
	* @li < 0: error.
	*/
	TInt iStatus;
public:	
	/** The Upper Layer Flow Information */
	TFlowInfo iInfo;
protected:
	/**	
	* Set when flow needs a reconnect (selector information changed).
	*
	* When this is set (1), RFlowContext::Status (and some other methods) will
	* automaticly call CFlowContext::Connect for the flow. Primary reason for
	* this to be set is that the flow selector information has been changed
	* (@ref setselectors).
	*
	* This can be set explicitly by the CFlowContext::SetChanged function (or
	* MFlowManager::SetChanged).
	*/
	TUint iChanged:1;
public:
	/**
	* Flag that indicates that NIF HOLD return should not block the flow.
	*
	* After a packet send a NIF can return a value that indicates that no
	* more packets are to be sent to it, until it allows it again via
	* the CProtocolBase::StartSending call. Normally, such indication
	* sets the flow into HOLD state.
	*
	* When this flag is set, flow is not put into HOLD. This flag should
	* only be used by a hook that have other means of enforcing the flow
	* control (for example QOS).
	*/
	TUint iIgnoreFlowControl:1;
	/** Precomputed packet header information. */
	TPacketHead iHead;
	/**
	* The current Path MTU
	*
	* Set from the path MTU of the connected interface. May change
	* dynamically due to ICMP "packet too big" or other events.
	*/
	TUint iPathMtu;
	/**
	* The header overhead by IP layer and hooks
	*
	* The iHdrSize is initialized to 0 at the beginning of the MIp6Hook::OpenL
	* phase.
	* The final value at the end of the OpenL phase is saved, and this value
	* will be the initial value at the beginning of the MFlowHook::ReadyL phase.
	*
	* The final value at the end of the ReadyL phase must be the total amount
	* of header space required by the layers below the transport (upper layer
	* protocol). The space available for the upper layer header and payload
	* is: iPathMtu - iHdrSize.
	*
	* The hook can add the header space requirement in OpenL or ReadyL
	* method. If it does it in OpenL, it does not need to touch the
	* iHdrSize in ReadyL method (for example, IPSEC only knows the
	* exact required header space at ReadyL phase).
	*
	* If a hook uses the TPacketHead::iPacket member to store precomputed
	* headers, which are automaticly appended to each packet, it must include
	* the amount into iHdrSize (it must carefully compute the change of length
	* in iPacket, if it adds new data there).
	*
	* The stack includes implicitly the header space for INNERMOST IP header
	* (which is also the final IP header, if no tunneling is present). Any hook,
	* that does tunneling, must include the OUTER IP header requirements into
	* the iHdrSize (a tunneling hook is ADDING the outer header!).
	*/
	TUint iHdrSize;
private:
	/** Receives state change upcalls. Also the owner of the flow. */
	MProviderNotify *iProvider;
	/** Storage for any other options. */
	COptionValue *iStorage;
	};


inline void CFlowContext::NoBearer(const TDesC8& aConnectionParams)
	/** Passes NoBearer call to owner, if present. */
	{
	if (iProvider)
		iProvider->NoBearer(aConnectionParams);
	}
inline void CFlowContext::Bearer(const TDesC8 &aConnectionInfo)
	/** Passes Bearer call to owner, if present. */
	{
	if (iProvider)
		iProvider->Bearer(aConnectionInfo);
	}

inline TInt CFlowContext::CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic)
	{
	return iProvider ? iProvider->CheckPolicy(aPolicy, aDiagnostic) : KErrNone;
	}

#ifdef SYMBIAN_NETWORKING_UPS
inline TBool CFlowContext::HasProvider() const
	{
	return (iProvider != NULL);
	}
	
inline void *CFlowContext::GetProviderApiL(const TDesC8& aApiName, TUint* aVersion)
	{
	if (iProvider == NULL)
		{
		return NULL;
		}
	else
		{
		return iProvider->GetApiL(aApiName, aVersion);		
		}	
	}
	
#endif //SYMBIAN_NETWORKING_UPS

//	*********
//	MFlowHook
//	*********
class MFlowHook : public MInetBase
	/**
	* Abstract base class for flow hooks.
	*
	* Flow hook providers implement this class. They register the hook using
	* MIp6Hook::BindFlowHook(), and return an instance from MIp6Hook::OpenL().
	*
	* @note
	*	The same instance of MFlowHook can be returned for multiple flows,
	*	if the logic of the hook does not require unique instance for each flow.
	*
	* The object can be implemented as reference counted object: last reference
	* removed by Close deletes the object.
	*
	* @since v7.0
	* @publishedPartner
	* @released
	*
	* Example:
	* @dontinclude mflowhook.cpp
	* @skip MFlowHook
	* @until //-
	*/
	{
public:
	/**
	* Increment reference count.
	*
	* The Open and Close functions must implement a reference
	* counting system. The Close function must destroy the current
	* instance, when the last reference is removed.
	*
	* Non-NULL return from MIp6Hook::OpenL() counts as one reference, and
	* the stack is guaranteed to call the matching Close exactly once.
	*
	* If a hook creates a new instance for each flow at OpenL, it can
	* leave the reference count as initial ZERO, if it implements a
	* Close, which deletes the object when the count goes negative.
	*
	* If a hook returns an existing instance at OpenL, it must
	* increment the reference count by one.
	*
	* Example:
	* @dontinclude mflowhook.cpp
	* @skip ::Open(
	* @until //-
	*/
	virtual void Open() = 0;
	/**
	* On an interface connecting, asks the hook if a flow is ready.
	*
	* The ReadyL calls propagate interface ready state up the
	* flow. The calls to hooks are made in reverse order;
	* the closest to interface is called first. The call informs
	* this hook that everything is ready from this hook to the interface.
	* Now it this hooks turn to check the ready state of the flow.
	*
	* @param aHead
	*		Address information of the flow.
	* @return
	*		from the ReadyL is the new status of the flow and has the following
	*		implications
	* @li	== 0, hook is ready, proceed to the next one or mark the flow
	*		as READY, if this was the first hook.
	* @li	> 0, hook is not ready, the ready calling is stopped and the
	*		returned value is the (pending) state of the flow.
	*		The hook MUST send a signal later to release this state to
	*		reactivate the ReadyL call chain.
	* @li	< 0, hook detected an unrecoverable error on flow
	*
	* @exception
	*		If the ReadyL leaves, the leave status will become
	*		the flow status  (the leave status must be negative, or KErrGeneral
	*		is substituted for it)
	*
	* Example:
	* @dontinclude mflowhook.cpp
	* @skip ::ReadyL(
	* @until //-
	*/
	virtual TInt ReadyL(TPacketHead &aHead) = 0;
	/**
	* Apply send transformations.
	*
	* The ApplyL is called by IP protocol  for outbound packet. The aPacket
	* is in "unpacked" state (RMBufPacketBase::Unpack).
	*
	* @param aPacket
	*		a complete packet to be processed (if needed) by the hook.
	*		The packet includes the IP header.
	* @param aInfo
	*		information block associated with the packet (a hook must not
	*		break this association!)
	* @return
	* @li	= 0,    (KErrNone) hook processed the packet, proceed with the next.
	* @li	< 0,    (error code) hook discarded the packet for some reason, send is
	*				aborted.
	* @li	> 0,    restart hook processing [the actual utility of this is
	*				still under consideration, maybe removed if no sensible
	*				use found.]
	*
	* @exception
	*		if ApplyL leaves, the packet is dropped.
	*
	* Example:
	* @dontinclude mflowhook.cpp
	* @skip ::ApplyL(
	* @until //-
	*/
	virtual TInt ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo) = 0;
	/**
	* Decrement references and destroy if last.
	*
	* Example:
	* @dontinclude mflowhook.cpp
	* @skip ::Close(
	* @until //-
	*/
	virtual void Close() = 0;
	};

class CFlowInternalContext;

class MFlowManager : public MInetBase
	/**
	* The flow manager interface.
	*
	* The use of MFlowManager is mostly hidden behind the RFlowContext,
	* but the upper layer must be aware of its existence.
	* Currently, the MFlowManager interface is included into the MnetworkService
	* (and is implemented by the IP6 protocol instance),
	* which must be used whenever an instance of MFlowManager is required.
	* @since v7.0
	* @publishedPartner
	* @released
	*/
	{
public:
	/**
	* Increase the "users" counter.
	*
	* The count of current "users" is used in determining whether to
	* keep or release resources. This in turn, may cause the disconnect
	* of a data connection.
	*
	* The stack leaves it up to the upper layers to decide what is counted as a 
	* "user". IncUsers() and DecUsers() should be used to give this information 
	* to the underlying system. By default, each opened socket is counted as
	* one user.
	*/
	virtual void IncUsers() = 0;
	/**
	* Decrease the "users" counter.
	* For details, see IncUsers().
	*/
	virtual void DecUsers() = 0;
	//
	// Use of the following methods is through the
	// RFlowContext handle
	//
	/**
	* Creates a new (empty) instance of a CFlowContext.
	*
	* @param aOwner Identifies the flow's owner (typically an RFlowContext handle)
	* @param aProtocol Protocol ID
	* @return New object
	*/
	virtual CFlowContext *NewFlowL(const void *aOwner, TUint aProtocol) = 0;
	/**
	* Creates a copy of an instance of a CFlowContext.
	*
	* @param aOwner Identifies the flow's owner (typically an RFlowContext handle)
	* @param aFlow Object to copy
	* @return New object
	*/
	virtual CFlowContext *NewFlowL(const void *aOwner, CFlowContext &aFlow) = 0;
	/**
	* Sets the connect information changed flag on all flows.
	* @return Number of flows.
	*/
	virtual TInt SetChanged() const = 0;


//protected:

	/** Internal API between flow and flow manager. @publishedPartner */
	virtual TInt FlowSetupHooks(CFlowInternalContext &aFlow) = 0;
	/** Internal API between flow and flow manager. @publishedPartner */
	virtual void FlowStartRefresh(CFlowInternalContext &aFlow) = 0;
	//
	// Flow option handling
	//
	/** Internal API between flow and flow manager. @publishedPartner */
	virtual TInt GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, const CFlowContext &aFlow) const = 0;
	/** Internal API between flow and flow manager. @publishedPartner */
	virtual TInt SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow) = 0;
	};

#endif
