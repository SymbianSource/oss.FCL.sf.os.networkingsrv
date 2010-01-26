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
// flow.cpp - IPv6/IPv4 flow information
//

#include <es_mbuf.h>
#include "flow.h"
//
//	COptionValue
//	************
//	(Only used internally in flow.cpp, DO NOT EXPORT! It has somewhat
//	"delicate" constructor requiring the extra allocation been done,
//	not checked... -- sma
//
class COptionValue : public CBase
	{
public:
	COptionValue(TInt aLevel, TInt aName, const TDesC8 &aOption) : iLevel(aLevel), iName(aName), iLength(aOption.Length())
		{
		TPtr8((TUint8 *)this + sizeof(COptionValue), iLength).Copy(aOption);
		}
	inline TPtrC8 Value() { return TPtr8((TUint8 *)this + sizeof(COptionValue), iLength, iLength); }
	COptionValue *iNext;		// Next value or NULL
	const TUint iLevel;			// Option level
	const TUint iName;			// Option name
	const TInt iLength;			// Option value length
	//
	// This is followed by the actual octets of the
	// option value. COptionValue class *CANNOT* be
	// subclassed!!!!
	};

// ************
// RFlowContext
// ************


EXPORT_C TInt RFlowContext::Open(MFlowManager *aManager, TUint aProtocol)
	/**
	* Creates a new empty and unconnected flow context and initialises the handle 
	* to point this context.
	*
	* The handle, which is used to create the flow context
	* through this method, will become the owner of the flow context.
	*
	* @param aManager
	*		Flow manager
	* @param aProtocol
	*		The upper layer protocol associated with the flow.
	*		This is also the value for the IP NextHeader/Protocol field, unless
	*		a hook adds some extension headers. This can also be set later using
	*		RFlowContext::SetProtocol method.
	* @return
	*		KErrNone, if the context was successfully created and attached to the handle.
	*/
	{
	iFlow = 0;
	TRAPD(err, iFlow = aManager->NewFlowL(this, aProtocol));
	return err;
	}

EXPORT_C TInt RFlowContext::ReOpen()
	/**
	* Allocates a new flow context to the handle using the parameters from
	* the existing flow context.
	*
	* If the handle is the only reference to the current context, the context is 
	* deleted after the new context has been created.
	*
	* This function is not for general use.
	*
	* @return
	*		KErrNone on success.
	*/
	{
	CFlowContext *old = iFlow;
	if (old)
		{
		iFlow = NULL;
		TRAPD(err, iFlow = old->iMgr->NewFlowL(this, *old));
		old->Close();
		return err;
		}
	else
		return KErrBadHandle;
	}

EXPORT_C TInt RFlowContext::Clone(const RFlowContext &aFlow)
	/**
	* Creates a new flow with the flow parameters copied from another flow.
	*
	* @param aFlow
	*		a flow to be duplicated
	*
	* @return
	*		System wide error codes
	*/
	{
	//
	// This sequence of actions below tries to do the cloning in
	// such a way, that it is safe even if the this == aFlow (in which
	// case this is same as plain "ReOpen()").
	//
	CFlowContext *old = iFlow;
	iFlow = aFlow.iFlow;
	iFlow->Open();
	TInt ret = ReOpen();
	if (old)
		old->Close();
	return ret;
	}


EXPORT_C TInt RFlowContext::Connect()
	/**
	* Forces a (re)connect on the flow.
	*
	* Connect activates the connection phase of the flow context.
	* Any previous connection is disconnected from the flow.
	*
	* @return
	*		the status of the flow. Any non-zero positive
	*		value indicates "pending" state: a connection
	*		is being setup but not yet ready (for example,
	*		a netdial process may have been activated, but
	*		it is still waiting for the user input).
	*/
	{
	if (iFlow)
		{
		iFlow->Connect();
		return iFlow->iStatus;
		}
	else
		return KErrBadHandle;
	}

EXPORT_C TInt RFlowContext::Open(MFlowManager *aManager, const TSockAddr &aDst,	const TSockAddr &aSrc, TUint aProtocol, TUint aIcmpType, TUint aIcmpCode)
	/**
	* Just a convenience function, which combines Open, SetRemoteAddr, SetLocalAddr, SetIcmpInfo and Connect.
	*
	* @param aManager	Flow Manager
	* @param aDst		Flow's destination address (and port)
	* @param aSrc		Flow's source address (and port)
	* @param aProtocol	The upper layer protocol associated with the flow
	*					(and the value for the IP  NextHeader/Protocol field,
	*					unless some hook adds extension headers)
	* @param aIcmpType	ICMP type (only sensible when protocol is ICMP)
	* @param aIcmpCode	ICMP code (only sensible when protocol is ICMP)
	*
	* @return
	*		current value of CFlowContext::iStatus.
	*/
	{
	TInt ret = Open(aManager, aProtocol);
	if (ret != KErrNone)
		return ret;
	else if (iFlow)
		{
		SetRemoteAddr(aDst);
		SetLocalAddr(aSrc);
		SetIcmpType(aIcmpType, aIcmpCode);
		iFlow->Connect();
		return iFlow->iStatus;
		}
	else
		return KErrBadHandle;
	}

EXPORT_C TInt RFlowContext::Open(RFlowContext &aFlow, RMBufPktInfo *aInfo)
	/**
	* Attaches the flow to a specified packet.
	*
	* This function is used only to attach a copy reference to the flow to an outgoing 
	* packet (RMBufSendInfo::iFlow).

	* This method is very specific and currently used only for attaching a copy reference
	* of the flow to an outgoing packet (iFlow member of the RMBufPacketInfo).
	*
	* The function checks aContext and if there have been no changes to the flow
	* parameters (as recorded by CFlowContext::iChanged) since the last connection,
	* the flow is assumed to be validly connected. Note that this means that if
	* the referenced flow is in an error state and if no flow parameters have been 
	* changed, this attach/open also fails. No reconnect attempt is made in that 
	* case.
	*
	* If the flow parameters have been changed since the last connect, the flow 
	* is reconnected. If there are additional references to the flow context, an 
	* implicit re-open (RFlowContext::ReOpen) is executed for the handle and the
	* flow parameters are copied  from the old flow context.
	* After this, the new flow is connected. Note that 
	* if a new cloned context is created, and if the context had a provider notify 
	* pointer (see RFlowContext::SetNotify()) set through this handle, the pointer
	* is automatically transferred to the new flow context, and removed from the old context. 
	*
	* If, after above processing, the referenced flow is ready to receive outgoing packets,
	* the flow context associated with the referenced handle is attached to the current handle.
	*
	* This design allows simple processing for the upper layers, as they don't need to worry
	* if the old flow is used by some packet still on the way out. Additionally, delaying the
	* "cloning" until actual attach/connect, may avoid unnecessary copying.
	*
	* The opened handle is not the owner of the flow context. This will only create an additional
	* reference to an existing flow context owned by someone else (usually aFlow).
	*
	* @param aFlow
	*		the "source flow" to be attached to the current handle
	* @retval aInfo
	*		An optional information block.
	*		If specified, then on a successful attach, the info members iProtocol,
	*		iSrcAddr and iDstAddr are initialized from the respective selector
	*		fields of the flow.
	* @return
	*		The return value of the Open is in general same as what would be returned
	*		by the Status call. However, when attach causes the actual connection of the flow,
	*		there exists error return codes which do not happen with Status call.
	*/
	{
	// FFS: It should be considered that this method is replaced with something like
	// Attach(aFlow) function in the RMBufSendInfo itself. That would be much cleaner
	// solution.
	iFlow = NULL;
	CFlowContext *flow = aFlow.iFlow;
	if (!flow)
		return KErrBadHandle;

	if (flow->iChanged)
		{
		//
		// The flow parameters have been changed
		//
		if (flow->iRefs > 0)
			{
			//
			// Need to allocate a new flow

			TInt err = aFlow.ReOpen();
			if (err != KErrNone)
				return err;
			flow = aFlow.iFlow;
			}
		// Assume: ConnectFlow will clean out all previous connection state! -- msa
		flow->Connect();
		}
	//
	// ...should this Refresh be done here or not? -- msa
	//
	if (flow->iStatus > EFlow_READY)
		flow->RefreshFlow();
	if (flow->iStatus == EFlow_READY)
		{
		//
		// Attach the flow to this handle
		//
		flow->Open();
		iFlow = flow;
		}
	//
	// Return current address information
	// (even though it in some cases might be random)
	//
	if (aInfo)
		{
		aInfo->iDstAddr = flow->RemoteAddr();
		aInfo->iSrcAddr = flow->LocalAddr();
		aInfo->iProtocol = flow->Protocol();
		}
	return flow->iStatus;
	}

// Help function for address loading, returns
//  = 0, if address did not change
//  = 1, if address has changed
//
static TInt ChangedAddress(TInetAddr &aAddr, const TInetAddr &aNewAddr)
	{
	// "Normalize" new address
	// - convert IPv4 addresess into IPv4 mapped
	// - anything other then KAfInet6 will be set as None
	TInetAddr tmp(aNewAddr);
	if (tmp.Family() == KAfInet)
		{
		tmp.ConvertToV4Mapped();
		tmp.SetScope(0);	// [is already part of "convert" in newer insocks]
		}
	else if (tmp.Family() != KAfInet6)
		tmp.SetAddress(KInet6AddrNone);

	if (tmp.CmpAddr(aAddr) &&				// address and port same?
		tmp.FlowLabel() == aAddr.FlowLabel())
		{
		// If the scope id in new addres is zero, then the
		// scope id is not part the address compare.
		// (this will optimize unconnected sockets where application
		// gives destination without scope -- eliminates unnecessary
		// flow connect operations.
		if (tmp.Scope() == aAddr.Scope() || tmp.Scope() == 0)
			return 0;	// No Change to the previous value
		}

	// Address is being changed
	aAddr = tmp;
	return 1;
	}


/**
* @defgroup	setselectors	Flow selector fields
*
* The selector information currently includes the following:
* @li	local address and port
* @li	remote addresses and port
* @li	IP protocol number
* @li	ICMP (or some other protocol) type and code.
*
* When a method is used to set any of the selector fields, the new and current value are
* compared, and if there is a change, the CFlowContext::iChanged is set.
*
* Although the code does not verify it, changing any of the selector fields should
* be done only by the owner of the flow context.
*
* If the handle is not attached to a flow, all methods will just silently return
* without doing anything.
*
* @{
*/

EXPORT_C void RFlowContext::SetRemoteAddr(const TSockAddr &aAddr)
	/**
	* Sets the flow's remote address and port.
	*
	* @param aAddr
	*	An address and port of the selector.
	*	If an address is given in IPv4 format (KAfInet), it will be automaticly
	*	converted into IPv4 mapped format (KAfInet6) in the flow context.
	*	Note that the flow  itself does not record whether an IPv4 address was specified. 
	*/
	{
	if (iFlow && ChangedAddress(iFlow->iInfo.iRemote, TInetAddr::Cast(aAddr)))
		{
		if (!iFlow->IsLocalSet())
			//
			// If the local address is unspecified by the application,
			// changing the remote address must also reset the local address
			// back to the unspecified state, so that the system will reselect
			// a new local address which will match the new remote address!
			//
			// Due to IPSEC, even changing port may cause destination
			// change, if policy specifies tunneling for certain ports
			// [this None setting may be unnecessary, need to check -- msa]
			iFlow->iInfo.iLocal.SetAddress(KInet6AddrNone);
		iFlow->iChanged = 1;
		}
	}

EXPORT_C void RFlowContext::SetLocalAddr(const TSockAddr &aAddr)
	/**
	* Sets the flow's local address and port
	*
	* @param aAddr
	*	An address and port of the selector.
	*
	*	If an address is given in IPv4 format (KAfInet), it will be converted
	*	into IPv4 mapped format (KAfInet6) in the flow context. If address
	*	is unspecified, the CFlowContext::iLocalSet is cleared, and set otherwise.
	*	See also CFlowContext::IsLocalSet().
	*
	*	Note that the flow  itself does not record whether an IPv4 address
	*	was originally specified in KAfInet or KAfInet6 format.
	*/
	{
	if (iFlow && ChangedAddress(iFlow->iInfo.iLocal, TInetAddr::Cast(aAddr)))
		{
		iFlow->iInfo.iLocalSet = !(iFlow->iInfo.iLocal.IsUnspecified());
		iFlow->iChanged = 1;
		}
	}

EXPORT_C void RFlowContext::SetProtocol(TUint aProtocol)
	/**
	* Sets the flow's protocol.
	* @param aProtocol
	*	The upper layer protocol associated with the flow (and the value for the IP
	*	NextHeader/Protocol field, unless some hook adds extension headers)
	*/
	{
	if (iFlow && iFlow->iInfo.iProtocol != aProtocol)
		{
		iFlow->iInfo.iProtocol = (TUint8)aProtocol;
		if (!iFlow->iInfo.iLocalSet)
			//
			// Need to reset source address to unselected also here...
			//
			// Due to IPSEC, even changing protocol, may cause destination
			// change, if policy specifies tunneling for certain protocols
			// [this None setting may be unnecessary, need to check -- msa]
			iFlow->iInfo.iLocal.SetAddress(KInet6AddrNone);

		iFlow->iChanged = 1;
		}
	}

EXPORT_C void RFlowContext::SetIcmpType(TUint aType, TUint aCode)
	/**
	* Sets the flow's ICMP type and code. Omitted aCode will set ICMP code in the
	* flow as zero.
	*
	* Note: Although this method is designed for ICMP, there are other protocols
	* which do not use ports, but some kind to type/code instead. Despite
	* it's name, this function can be used.
	*
	* @param aType		ICMP type
	* @param aCode		ICMP code
	*/
	{
	if (iFlow && (iFlow->iInfo.iIcmpType != aType || iFlow->iInfo.iIcmpCode != aCode))
		{
		iFlow->iInfo.iIcmpType = (TUint8)aType;
		iFlow->iInfo.iIcmpCode = (TUint8)aCode;
		if (!iFlow->iInfo.iLocalSet)
			//
			// Need to reset source address to unselected also here...
			//
			// Due to IPSEC, changing anything that affects selector,
			// may cause destination change (due tunnel), and thus
			// a new source address..
			// [this None setting may be unnecessary, need to check -- msa]
			iFlow->iInfo.iLocal.SetAddress(KInet6AddrNone);
		iFlow->iChanged = 1;
		}
	}


/** @} */	// End setselectors



EXPORT_C MProviderNotify * RFlowContext::SetNotify(MProviderNotify *aProvider)
	/**
	* Requesting notifications of events on the flow.
	*
	* A provider may request notification callbacks from events affecting the flow
	* by attaching a MProviderNotify instance to a flow context.
	* A flow can only have one such notifier at any time.
	*
	* Only the owner of the flow context is allowed to set this.
	*
	* The receiver of the notifications must implement the MProviderNotify mixin class:
	*
	* MProviderNotify::CanSend() is called when the flow might be ready to change
	* into the EFlow_READY status (this is only called when flow status is HOLD
	* or PENDING).
	*
	* MProviderNotify::Error is called whenever the flow is set into error state.
	* The aMask parameter follows the MSocketNotify (in <es_prot.h>) definitions.
	*
	* These two methods are a subset of the MSocketNotify,
	* and the intention is that, by default the SAP implementation using the flow
	* can just call the equivalent MSocketNotify method from these notifies.
	*
	* @param aProvider
	*		an object to receive the CanSend() or Error() upcalls,
	*		or NULL if cancel notifications
	* @return
	*		the pointer of the previous notifier, if any, is returned
	*		(or, the context was not attached, the method returns the
	*		aProvider).
	*/
	{
	// FFS: The notifier call should be suppressed while
	// iChanged is set, only the CanSend() notify?
	if (iFlow && iFlow->iOwner == this)
		{
		MProviderNotify *n = iFlow->iProvider;
		iFlow->iProvider = aProvider;
		return n;
		}
	else
		return aProvider;
	}

EXPORT_C void RFlowContext::Close()
	/**
	* Closes an open handle.
	*
	* This must be called when the handle is no longer required.
	* Close detaches the actual flow context from this handle.
	* If this handle was the last reference to the context, the context is
	* destroyed.
 	*/
	{
	if (iFlow)
		{
		if (iFlow->iOwner == this)
			iFlow->iProvider = NULL;
		// Do a "safe" close, by removing the flow context
		// reference from the handle before closing.
		CFlowContext &flow = *iFlow;
		iFlow = NULL;
		flow.Close();
		}
	}

EXPORT_C TInt RFlowContext::Status()
	/**
	* Gets the status of the flow.
	*
	* This function attempts to return the current "effective" status
	* of the flow based on current settings. Thus, if the flows parameters
	* have been changed since the last connect, the function does an implicit
	* (re)connect first.
	*
	* For documentation on values, see CFlowContext::Status.
	*
	* @return
	*		KErrBadHandle, if handle is not attached to a flow context. Other returns
	*		are defined by CFlowContext::Status.
	*/
	{
	if (!iFlow)
		return KErrBadHandle;

	// If the flow is in error state (< 0), the Status() alone will not
	// clear it (changed bit is not effective!).
	if (iFlow->iStatus >= 0 && iFlow->iChanged)
		{
		//
		// The flow parameters have been changed
		//
		if (iFlow->iRefs > 0)
			{
			//
			// Need to allocate a new flow

			TInt err = ReOpen();
			if (err != KErrNone)
				return err;
			}
		// Assume: ConnectFlow will clean out all previous connection state! -- msa
		iFlow->Connect();
		}
	return iFlow->Status();
	}

EXPORT_C CNifIfBase * RFlowContext::Interface() const
	/**
	* Gets a pointer to the network interface object at the end of the flow.
	*
	* @return	Network interface object (or NULL).
	*/
	{
	return iFlow ? iFlow->Interface() : NULL;
	}

EXPORT_C void RFlowContext::Grab(RFlowContext &aContext)
	/**
	* Moves the flow context from one handle to another.
	*
	* This operation does not increase or decrease the references.
	* The grabbed handle will become unattached.
	*
	* This function is not for general use.
	* @param aContext
	*		Handle from which to move the context
	*/
	{
	iFlow = aContext.iFlow;
	// FFS: Should check that the aContext is not the owner and disallow grab in such
	// situation (or disable notifier callback, if set). If a aContext
	// is the owner of the CFlowContext and notifier is set, then this pointer
	// is left "dangling"! Currently only used to detach from the packet, which
	// is never the owner? Suggested code (disable notifier):
	if (iFlow && iFlow->iOwner == &aContext)
		iFlow->iProvider = NULL;
	aContext.iFlow = NULL;
	}

EXPORT_C void RFlowContext::Copy(RFlowContext &aContext)
	/**
	* Copies the specified flow context to the handle and updates the
	* flow's reference count.
	* The call never fails.
	*
	* @param	aContext
	*	Handle to the flow context to copy
	*/
	{
	iFlow = aContext.iFlow;
	if (iFlow)
		iFlow->Open();
	}

#ifdef SYMBIAN_TCPIPDHCP_UPDATE
//RFC 4861 Section 7.2.2
//Checks whether Neighbour Discovery is currently pending
EXPORT_C TBool RFlowContext::IsNdResolutionPending()
	{
	TBool bFlag = EFalse;
	if (iFlow)
		{
		bFlag = iFlow->IsNdPacketPendingResolution();  			
		}
	return bFlag;	
	}
#endif //SYMBIAN_TCPIPDHCP_UPDATE

// ************
// CFlowContext (methods)
// ************
//
EXPORT_C CFlowContext::CFlowContext(const void *aOwner, MFlowManager *aManager) : iOwner(aOwner), iMgr(aManager), iStatus(KErrNotReady)
	/**
	* Constructor.
	*
	* The contructor (returned pointer) counts as a reference. The user does
	* not need to issue a separate Open() to update the reference count. When
	* the flow pointer is not needed, it must be released by a Close() call
	* (and not by a delete operator).
	* The Close() deletes the CFlowContext automaticly, when the last reference
	* is removed. The delete must not be used.
	*
	* @note
	*	The CFlowContext should be handled through the RFlowContext class, which
	*	takes care of the Open() and Close() calls correctly.
	*
	* @param aOwner
	*	Flow owner identifier. This is untyped, but can be used for 
	*	comparisions against other owner identifier.
	* @param aManager
	*	The flow manager that created this object
	*/
	{
	}

EXPORT_C CFlowContext::CFlowContext(const void *aOwner, MFlowManager *aManager, CFlowContext &aFlow)
  : iOwner(aOwner), iMgr(aManager), iStatus(KErrNotReady)
	/**
	* Constructor, copying the parameters of an existing flow.
	*
	* The contructed flow must be connected before it can be used
	* (regardless of the state of the copied flow).
	* @param aOwner
	*	Flow owner identifier. This is untyped, but can be used for 
	*	comparisions against other owner identifier.
	* @param aManager
	*	The flow manager that created this object
	* @param aFlow
	*	The flow from which to copy parameters
	*/
	{
	//
	// Copy/transfer upper layer specific information
	// into the new flow
	//
	iInfo = aFlow.iInfo;
	//
	// If the source flow has the same owner, then transfer the
	// notifier and option storage into the new flow context
	//
	if (iOwner == aFlow.iOwner)
		{
		iProvider = aFlow.iProvider;
		aFlow.iProvider = NULL;
		iStorage = aFlow.iStorage;
		aFlow.iStorage = NULL;
		}
	}


// Destructor should not be exported, it should be private! -- msa
EXPORT_C CFlowContext::~CFlowContext()
	/** Destructor. */
	{
	while (iStorage)
		{
		COptionValue *o = iStorage;
		iStorage = o->iNext;
		delete o;
		}
	//
	// Release associated RMBUF's, if left over by someone
	//
	iHead.iPacket.Free();
	iHead.iIcmp.Free();
	}



EXPORT_C void CFlowContext::Close()
	/**
	* Decrements a reference count on the context.
	*
	* The function deletes the CFlowContext object when the last reference is removed.
	*/
	{
	if (--iRefs < 0)
		delete this;
	}

EXPORT_C TInt CFlowContext::Status()
	/**
	* Refresh and return current status.
	*
	* This function is for upper layer users (flow owner) only, because it includes
	* conditional side effects through a call to the CFlowContext::RefreshFlow,
	* if the status not ready.
	*
	* A hook must not use this method for flows that it doesn't own itself..
	*
	* @return
	*		the general rules about the returned value are:
	* @li	< 0, the flow is in unrecoverable error state. No further output can be done
	*		over it and the only way to recover from this is to cause a reconnect.
	* @li	= 0, the flow is ready for output, data can be sent over it. (#EFlow_READY)
	* @li	> 0, the flow is temporarily blocked and waiting for event. When the event occurs,
	*		the SAP is notified if it has setup for it (see SetNotify method).
	* @li	A few specific values have been defined as follows:
	* @par	Flow_PENDING
	*		Not ready for data yet. This occurs normally at the connection
	*		startup, when the connection to outside is being established (netdial),
	*		but it can also occur at later stage (for example, when IPSEC needs to
	*		negotiate a new key).
	* @par EFlow_HOLD
	*		This state is set when the interface reports congestion
	*		(cannot receive more data for a while).
	*/
	{
	//
	// The rationale here goes something...
	// in normal situation the iStatus == EFlow_READY and this non-virtual
	// method is very light to execute. For the exceptional situation a
	// heavier virtual RefreshFlow() is called to check if flow state can
	// be changed (this may include initializing the source address).
	// "Normal" exceptions are
	//	EFlow_PENGING ( > EFlow_READY)
	//		usually means that the interface is not yet fully and/or the source
	//		address to be use cannot yet be decided. The refresh will verify this
	//		and update the source if situation has changed.
	//	EFlow_HOLD
	//		the interface has reported that it is unable to receive more packet
	//		packets
	//
	//	For this to work, it requires
	//		1) when interface goes into HOLD, all affected flows must be marked with
	//		HOLD state
	//		2) if a SAP keeps opened flow handle, it MUST use Status() call before
	//		sending of each packet.
	//
	//	Refresh is only called for PENDING states. Once the flow enters error
	//	(< EFlow_READY), it cannot be used for sending data!
	//
	if (iStatus > EFlow_READY)
		RefreshFlow();
	return iStatus;
	}

EXPORT_C void CFlowContext::SetStatus(TInt aStatus)
	/**
	* Sets (suggests) the flow status value.
	*
	* The function allows a hook to suggest a value for the flow status.
	* The flow does not necessarily accept this value.
	*
	* This function should never be used by the flow owner.
	*
	* The function uses the following rules (in the listed order) to
	* determine the new status value:
	* -# if the new status is an error ( < 0), it will become the flow status
	* and all hooks are removed throuh a call to Disconnect().
	* A hook must be careful here, as this will cause a call to
	* the MFlowHook::Close. Also, the MProviderNotify::Error is called,
	* if the notifier is attached to the flow.
	* -# if the old status is an error ( < 0), it cannot be changed to any
	* non-error status (SetStatus is ignored),
	* -# if the new status is #EFlow_READY and the old flow status was
	* not READY (was > 0), the flow status is NOT changed, but the attached upper
	* layer is notified (MProviderNotify::CanSend). This will eventually cause a
	* flow refresh where the correct value of the status will be determined.
	* -# if the new status is pending/hold/blocked (> 0), it will become the
	* flow status.
	*
	* The idea behind above definition is that the hook can suggest a status
	* from its local view of the flow, and SetStatus makes the "global"
	* decision, and activates necessary callbacks if any is required.
	*
	* @param aStatus
	*	The suggested status of the flow: either a system wide error 
	*	code or a #TFlowStatus value.
	*/
	{
	if (aStatus < 0)
		{
		// New status is Error status, set it
		// and notify SAP. The previous status does not
		// matter.
		iStatus = aStatus;
		Disconnect();		// ...removes the hooks
		if (iProvider)
			iProvider->Error(aStatus);
		}
	else if (iStatus > 0)
		{
		// Old status is "pending/hold/blocked"
		if (aStatus > 0)
			iStatus = aStatus;		// Allow any overrides > 0 (!= EFlow_READY)
		else if (iProvider)
			iProvider->CanSend();
		}
	else if (iStatus == EFlow_READY)
		// New status is either READY or PENDING
		iStatus = aStatus;
	// When none of the above conditions apply, old status
	// is Error state and cannot be overridden by any
	// non-Error status.
	}

EXPORT_C TInt CFlowContext::StoreOption(TUint aLevel, TUint aName, const TDesC8 &aOption)
	/**
	* Stores a flow context-specific option.
	*
	* The function allows outbound flow hook modules to store the options that they 
	* support. Sockets clients can ultimately query these options through RSocket::GetOpt().
	*
	* If the specified (level, name) pair already exists in the storage, the new 
	* value replaces the old value.
	*
	* @param aLevel the option level
	* @param aName the option name
	* @param aOption the option value
	*
	* @return
	*		@li KErrNone on success,
	*		@li KErrNoMemory, if option cannot stored due to memory shortage
	*/
	{
	COptionValue **h, *o;
	const TInt length = aOption.Length();
	for (h = &iStorage; (o = *h) != NULL; h = &o->iNext)
		{
		if (o->iLevel == aLevel && o->iName == aName)
			{
			// Replacing a value of existing option. Just delete
			// the previous allocation (and insert new as if this
			// was not present).
			*h = o->iNext;
			delete o;
			break;
			}
		}
	//
	// Attach the new option to the front of the list
	//
	o = new (length) COptionValue(aLevel, aName, aOption);
	if (o)
		{
		o->iNext = iStorage;
		iStorage = o;
		return KErrNone;
		}
	else
		return KErrNoMemory;
	}

EXPORT_C TInt CFlowContext::RetrieveOption(TUint aLevel, TUint aName, TDes8 &aOption) const
	/**
	* Gets a flow context-specific option.
	*
	* The option must have been previously stored with StoreOption().
	* Retrieve value from flow
	*
	* @param aLevel The option level
	* @param aName The option name
	* @retval aOption The option value
	*
	* @return
	*		@li KErrNone on success,
	*		@li KErrNotFound, if the value is not stored
	*/
	{
	for (COptionValue *o = iStorage; o != NULL; o = o->iNext)
		{
		if (o->iLevel == aLevel && o->iName == aName)
			{
			aOption = o->Value();
			return KErrNone;
			}
		}
	return KErrNotFound;
	}
