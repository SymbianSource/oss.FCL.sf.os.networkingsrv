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
// in_pkt.h - packet handling routines
// Generic packet handling utility for mapping packet handling to the RMBufChain.
//



/**
 @file in_pkt.h
 @publishedAll
 @released
*/

#ifndef __IN_PKT_H__
#define __IN_PKT_H__

#include <nifmbuf.h>
#include "ip6_hdr.h"	// ..should eventually be <inet/ip6_hdr.h>? -- msa
#include "ip4_hdr.h"

#define TPACKETHEAD_FRAGMENT	1	//< Enable iFragment in TPacketHead

/**
 TScopeType is only provided so that "magic" constants can be
 avoided in the source code. However, the max value cannot be changed
 to anything from 0xF. The scope type is assumed to be 4 bits long
 in many occasions.

 The value of the scope type is directly bound the the IPv6 Scope
 level - 1. This can be done, as IPv6 Scope level 0 is not legal
 (or usable) in any context within the stack.
 This allows our non-standard network scope (= 0x10) to
 be coded internally in 4 bits (as 0xF).

 @publishedAll
 @released
 @since v7.0s
*/
enum TScopeType
	{
	EScopeType_IF	= 0x0,	//< (= #KIp6AddrScopeNodeLocal - 1), id is interface index
	EScopeType_IAP	= 0x1,	//< (= #KIp6AddrScopeLinkLocal - 1). id is IAP number
	EScopeType_GLOBAL = 0xD,//< (= #KIp6AddrScopeGlobal - 1). id is global scope id
	//
	// no symbols defined for types 2..14 (they are also valid)
	//
	EScopeType_NET	= 0xF	//< (= #KIp6AddrScopeNetwork - 1), id is network number (must be the last entry)
	};

//
//	TIpHeader
//	*********
class TIpHeader
	/**
	A simple help class that uses a union to merge handling of either an IPv4 or 
	an IPv6 header. 
	@since v7.0
	@publishedAll
	@released
	*/
	{
public:
	/**
	Gets the minimum header length.

	IPv6 header is longer than minimum IPv4 header, thus
	returned value is for IPv4. This function only defined
	because it is required when this class is used as template
	parameter in TInet6Packet.
	
	@return Minimum IPv4 header length
	*/
	inline static TInt MinHeaderLength() {return TInet6HeaderIP4::MinHeaderLength(); }
	/**
	Gets the maximum header length.

	IPv6 header always shorter than maximum IPv4 header, thus
	returned value is for IPv4. This function is only defined
	because "header mapping" classes are expected to have it.
	
	@return Maximum IPv4 header length
	*/
	inline static TInt MaxHeaderLength() {return TInet6HeaderIP4::MaxHeaderLength(); }

	union
		{
		TInet6HeaderIP4 ip4;
		TInet6HeaderIP ip6;
		};
	};


//	RMBufPacketPeek
//	***************
class RMBufPacketPeek : public RMBufChain
	/**
	Extends RMBufChain to add functions to read packet data as a descriptor 
	and as an IP header.

	The RMBufChain is assumed to contain the raw packet, without
	the info block prepended (e.g. if this class is used for RMBufPacketBase
	derived handle, it must be in "unpacked" state).
	
	@since v7.0
	@publishedAll
	@released
	*/
	{
public:
	IMPORT_C TPtr8 Access(TInt aSize, TUint aOffset = 0);
	IMPORT_C TIpHeader *GetIpHeader();
	};

//	TPacketHead
//	***********
class TPacketHead
	/**
	Storage for some precomputed information for an outbound packet flow.

	The outbound TPacketHead is part of the flow context (CFlowContext).

	The CFlowContext::Connect initializes the content from the parameters
	of the flow (TFlowInfo) and runs the connection process.. The connection
	process (MIp6Hook::OpenL and MFlowHook::ReadyL phases) completes the
	information. After this, as long as the flow is connected, the content
	is mostly frozen and <b>must not be modified by anyone</b>.
 
    When there is a need to change any flow information, the changes must
	be done to the flow parameters (and not to TPacketHead). The change of
	flow parameters also sets the CFlowContext::iChanged flag, and this
	eventually causes a new CFlowContext::Connect, which re-initializes
	the TPacketHead with the new information.

	For each field in the TPacketHead, the hook writer must follow the
	basic rule (only for fields that it intends to change):

	- if some field is changed in MIp6Hook::OpenL, then the previous
	value should be restored in the MFlowHook::ReadyL.
	- an exeception: the hook must omit the restore, if the
	previous value was unspecified value (for example, the source
	address).
	- the content of #iPacket (and #iOffset) are special: they cannot
	be modified in the MIp6Hook::OpenL phase. A hook can
	modify them only in the MFlowHook::ReadyL phase. And, if the hook
	is adding an IP header for tunneling, it must save the current content
	of these fields in the ReadyL function, and then clear out the fields
	(it must make the iPacket empty and zero iOffset). The hook must add
	the saved iPacket content below the added tunnel header in
	MFlowHook::ApplyL .

	@since v7.0
	@publishedAll
	@released
	*/
	{
public:
	IMPORT_C TBool ExtHdrGet(TInt aType, TInt& aOfs, TInt& aLen);
	IMPORT_C TBool ExtHdrGetOrPrependL(TInt aType, TInt& aOfs, TInt& aLen);
	IMPORT_C TBool ExtHdrGetOrAppendL(TInt aType, TInt& aOfs, TInt& aLen);
	IMPORT_C void AddDestinationOptionL(const TPtrC8& aOption, TUint8 aAlign=0, TUint8 aModulo=4);
	IMPORT_C void AddDestinationOptionL(const TUint8* aOption, TUint8 aLen, TUint8 aAlign=0, TUint8 aModulo=4);

public:
	/**
	"Virtual" IP header. The IPv6 header stucture is used, but the same
	format is <b>also</b> used for the IPv4 destinations (Version() == 4,
	even though the header format is still IPv6!)
	
	This header is initialized in the beginning of the OpenL phase
	as follows:
	@li	Version = 0
	@li	Traffic Class, copied from the flow iOptions.iTrafficClass
	@li	Flow Label = 0
	@li	Payload Length = 0 (dummy field, not used)
	@li	Next Header, copied from the flow iProtocol
	@li	Hop Limit, copied from the flow iOptions.iHopLimit
	@li	Src Address, copied from the flow Local Address (usually unspecified)
	@li	Dst Address, copied from the flow Remote Address
	
	At beginning of the ReadyL phase (= at end of OpenL), the destination
	address (and iDstId) are used to find a route on the interface. Depending
	on whether this address is IPv4 (mapped) or IPv6, the Version field is set
	accordingly to either 4 or 6.

	After succesfull completion of the ReadyL, this used for *each* packet
	which needs an IP header to be generated on send. The Version() determines
	whether IPv4 or IPv6 frame is to be generated (this is the initial
	header in the packet, *before* running outbound ApplyL hooks):
	
	@verbatim
	                   IPv6            IPv4
	   Version         == 6            ==4
	   Traffic Class   used as is      used as TOS
	   Flow Label      used as is      ignored
	   Payload Length  ignored         ignored
	   Next Header     used as is      used as Protocol
	   Hop Limit       used as is      used as TTL
	   Src Address     used as is      used as IPv4 mapped
	   Dst Address     used as is      used as IPv4 mapped
	@endverbatim
	*/
	TInet6HeaderIP ip6;
	/**
	Contains the scope id associated with the destination address
	which is stored in #ip6 Dst Address. This id and address must
	always be considered as a unit. Logically, any change changes
	both values.

	iDstId is initialized from the flow context TFlowInfo::iRemote.Scope() at
	beginning of the flow connect phase. If application does not define
	this scope id, then the system will attempt to choose a default value
	at beginning of the connect phase. If the default cannot be determined,
	the flow is put into pending state (and no connect happens).

	@par MIp6Hook::OpenL
	On entry to the OpenL, the iDstId is always non-zero and destination
	address is specified. If a hook changes the destination address in
	OpenL method, it must provide the correct id value
	which goes with the new destination. If it cannot do this, it
	must either abort the connect by leaving with an error state, or it
	can leave with PENDING (> 0) status to signal there is no route
	for the new destination.
	If the stack cannot find suitable interface for the destination, then
	it aborts the connect phase, and the flow is placed into holding state.

	@note
		Only a tunneling hook can safely change the destination
		address (a use of routing header can also be a kind of
		tunneling).
	
	@par MFlowHook::ReadyL
	If the hook changed the destination address (or id) in the OpenL,
	the ReadyL must restore the original values back.

	*/
	TUint32 iDstId;
	/**
	Contains the scope id associated with the source address
	which is stored in #ip6 Src address. This is defined when the source
	address is defined, and otherwise undefined.

	iSrcId is initialized from TFlowInfo::iLocal.Scope() at beginning of the
	flow connect phase. If application defines the source address,
	but does not specify this scope id, then the system chooses
	the id based on the interface defined by the source address.
	If scope and address are both specified, they must match the
	selected interface.

	@par MIp6Hook::OpenL
	On entry to the OpenL, the iSrcId (and source address) may be
	undefined (#iSourceSet = 0). If defined (iSourceSet = 1), then
	both address and iSrcId are defined (iSrcId != 0). A hook may
	force a reselection of the source just by zeroing the
	iSourceSet.

	@par MFlowHook::ReadyL
	If the hook changed the source address (or id) in the OpenL,
	the ReadyL must restore the original values back, but only
	if the original value was defined (#iSourceSet = 1 in OpenL).
	*/
	TUint32 iSrcId;
	/**
	The source address has been set.

	This bit indicates whether the value stored in #ip6 src field
	and #iSrcId is to be used as a source address as is.

	Initialized from TFlowInfo::iLocalSet, which tells whether user
	specified tbe source address or not (e.g used RSocket Bind method).
	The stack checks the value after each MIp6Hook::OpenL call, and
	if the flag is set, the source in ip6 is used as is. If the flag
	is zero, then the stack performs the normal source address selection
	based on the current destination address (#iSrcId and destination
	address).

	@par MIp6Hook::OpenL
	On entry, this flag is always set and source address is defined.
	A hook may clear this flag, if it wants the
	stack choose the source address based on current destination.
	The clearing operation is normally needed only by a tunneling
	hook.

	@note
		If the hook specifies the source address, it must be either
		a valid source address for the interface or unspecified
		address.

	@par MFlowHook::ReadyL
	Upon entry to the ReadyL, the source address is always fully
	known (the hook can assume that #iSrcId and the #ip6 source
	addresses are valid).
	If the source address was set before the OpenL, then this
	must restore the original value (along with the #iSrcId
	and source address).
	*/
	TUint iSourceSet:1;
#ifdef TPACKETHEAD_FRAGMENT
	/**
	The fragment processing alredy done.
	
	This bit is meaningful only in OpenL phase. If already set,
	then some ealier hook has requested that the packet must
	be fragmented to fit the mtu.
	
	A tunneling hook can set this bit in OpenL, if it needs
	the fragmenting to happen before the ApplyL is called (e.g.
	the fragments are tunneled instead of fragmenting the
	tunneling).
	
	This bit can only be set or left as is. It cannot be cleared
	once set.
	*/
	TUint iFragment:1;
#endif
	/**
	Selector info, the upper layer protocol.

	iProtocol has the same value as ip6.NextHeader() when iPacket is empty,
	and otherwise it is the same as NextHeader() of the last extension
	header in the iPacket.

	The values of the other selector fields: #iIcmpType, #iIcmpCode
	#iSrcPort and #iDstPort depend on iProtocol. Whenever iProtocol
	is changed, the other fields must be updated accordingly.

	@par MIp6Hook::OpenL
	Because iPacket cannot be modified during the OpenL phase, the
	content of this field and the Next Header (protocol) field in
	the #ip6 pseudoheader must always be the same. This field should
	be considered as <b>read-only</b>, unless the hook intends to
	apply IP-in-IP tunneling, in which case the hook <b>must</b>
	change the value to the appropriate tunneling protocol
	(#KProtocolInet6Ipip or #KProtocolInetIpip).

    @par MFlowHook::ReadyL
	Only a tunneling hook needs to restore the value here to match
	the original upper layer protocol. See #iPacket for
	more detailed information.
	*/
	TUint8 iProtocol;
	/**
	Selector field whose value depends on #iProtocol. 
 
	If this field does not have meaning with the protocol,
	the field content should be set to ZERO.
	*/
	TUint8 iIcmpType;
	/**
	Selector field whose value depends on #iProtocol. 
 
	If this field does not have meaning with the protocol,
	the field content should be set to ZERO.
	*/
	TUint8 iIcmpCode;
	/**
	Selector field whose value depends on #iProtocol. 
 
	If this field does not have meaning with the protocol,
	the field content should be set to ZERO.
	*/
	TUint16 iSrcPort;
	/**
	Selector field whose value depends on #iProtocol. 
 
	If this field does not have meaning with the protocol,
	the field content should be set to ZERO.
	*/
	TUint16 iDstPort;
	/**
	The amount of pre-computed IPv6 extension headers in iPacket which
	are copied to the beginning of each outgoing packet

	If iOffset > 0, then #iPacket includes that much of extension
	headers that are copied in front of each packet.
	*/
	TInt iOffset;
	/**
	Pre-computed extension headers for all packets in this flow.
	
	These can only be added in the ReadyL phase. If any of the
	ReadyL's adds extension headers into this, it must take care
	of maintaining the correct Next Header in the virtual IP header
	(and the original upper layer protocol must be placed in the
	next header of the last extension header added.
	
	Stack copies the content of this to each outgoing packet, just below
	the IP header, before running the ApplyL functions of the outbound
	flow hooks.

	@par MIp6Hook::OpenL
	The iPacket <b>must not</b> be modified during the OpenL phase.

	@par MFlowHook::ReadyL
	A non-tunneling hook may add extension headers into the current
	iPacket. A tunneling hook has more complex requirements:
	it must save the current iPacket and #iOffset and initialize
	iOffset = 0, and iPacket as empty.

    @par MFlowHook::ApplyL
	When a tunneling hook adds the tunneling IP header, it
	must also copy the saved iPacket below the added IP header.
	*/
	RMBufPacketPeek iPacket;
	/**
	The received packet which caused an ICMP error reply to be sent.

	This is only used for ICMP error repply flows, and should be
	ignored by others -- mainly for IPSEC hook. The packet, if
	present, is in unpacked state.
	*/
	RMBufPacketBase iIcmp;
	/**
	The current destination interface.

 	This is ONLY used during connect/OpenL phase.

	The value is maintained by the stack, and is intended as
	read-only information for the hooks that have a use for
	it (for example, IPSEC implementing VPN specific policies).

	A hook must not modify this value (the stack will recompute
	the value after each OpenL, based on the possibly changed
	address parameters in the TPacketHead)

	@par MIp6Hook::OpenL
	<b>read-only</b>
	@par MFlowHook::ReadyL
	<b>read-only</b>
	*/
 	TUint32 iInterfaceIndex;
	};

class TInet6PacketBase
	/**
	* Thin base class for the TInet6Packet.
	*/
	{
public:
	enum TAlign
		{
		EAlign1 = 0,	//< Align to byte (no align requirement)
		EAlign2 = 1,	//< Align to 2 byte unit (even address)
		EAlign4 = 3,	//< Align to 4 byte unit
		EAlign8 = 7		//< Align to 8 byte unit
		};

	/**
	Constructor.

	@param aAlign	The align requirement.
	*/
	TInet6PacketBase(TAlign aAlign) : iLength(0), iAlign(aAlign) {}

	/**
	Length of the mapped region.

	The real mapped length as computed by the Access function.
	If access returned non-NULL, the following is always TRUE:

	@li	aMin <= iLength
	*/
	TInt iLength;

	IMPORT_C TUint8 *Access(RMBufChain &aPacket, TInt aOffset, TInt aSize, TInt aMin);

	inline void SetAlign(TAlign aAlign)
		/**
		* Changes the align requirement.
		*
		* @param aAlign The new align requirement.
		*/
		{
		iAlign = aAlign;
		}
protected:
	/**
	The align requirement.
	*/
	TAlign iAlign;
	};

// TInet6Packet template
// *********************
template <class T>
class TInet6Packet : public TInet6PacketBase
	/**
	Encapsulates an IPv6 packet header as a section of an RMBufChain.

	The T template parameter should represent a packet header type. It should 
	support static functions MaxHeaderLength() and MinHeaderLength() that return 
	TInt values for maximum and minimum header lengths respectively.

	@publishedAll
	@released
	@since v7.0
	*/
	{
public:
	TInet6Packet(TAlign aAlign = EAlign4) : TInet6PacketBase(aAlign), iHdr(NULL)
		/**
		Default constructor.

		Construct an empty mapping. To be usable, the Set() function
		must be used.
		*/
		{}
	TInet6Packet(RMBufChain &aPacket) : TInet6PacketBase(EAlign4)
		/**
		Constructor specifying a RMBufChain object.

		Verify and arrange it so that a class T can be mapped
		to a contiguous octets from the beginning of the RMBufChain
		content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		@param	aPacket
			Packet containing the header T at offset = 0
		*/
		{
		iHdr = (T *)Access(aPacket, 0, T::MaxHeaderLength(), T::MinHeaderLength());
		}

	TInet6Packet(RMBufChain &aPacket, TInt aOffset, TAlign aAlign = EAlign4) : TInet6PacketBase(aAlign)
		/**
		Constructor specifying a RMBufChain object and an offset.

		Verify and arrange it so that a class T can be mapped
		to a contiguous octets starting at specified offset of
		the RMBufChain content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		@param aPacket
			Packet containing the header T at aOffset
		@param aOffset
			Offset of the header to be mapped.
		@param aAlign
			The alignement requirement.
		*/
		{
		iHdr = (T *)Access(aPacket, aOffset, T::MaxHeaderLength(), T::MinHeaderLength());
		}

	void Set(RMBufChain &aPacket, TInt aOffset, TInt aSize)
		/**
		Sets the packet header from a specified RMBufChain object.

		Verify and arrange it so that a aSize octets can be mapped
		to a contiguous octets starting at specified offset of
		the RMBufChain content, and set iHdr to point this area.

		If this is not possible, iHdr is initialized to NULL.

		Note that this differs from the contructors: the required
		size is a parameter, and not determined by the T::MinHeaderLength().
		However, the "T* iHdr" is set to point the start of the requested
		area. It's a responsibility of the user of this method to know
		whether using this pointer is safe with the specified size parameter.

		@param	aPacket
			Packet containing the header T at aOffset
		@param	aOffset
			Offset (position) of the header to be mapped
		@param	aSize
			Length of required contiguous memory
		*/
		{
		iHdr = (T *)Access(aPacket, aOffset, aSize, aSize);
		}

	inline T& operator()()
		{
		return *iHdr;
		}
	/**
	The pointer to the mapped region (if non-NULL). If NULL,
	then there is no mapping, and iLength == 0.
	*/
	T *iHdr;
	};


//	TPacketPoker
//	************
class TPacketPoker
	/**
	Provides a utility for linear scanning of a chain of RMBuf objects (an RMBufChain).

	An object of this type maintains a current point in the RMBufChain. This point 
	can only move forward, and a leave occurs if the point advances beyond the 
	end of the chain.

	Any pointers and aligns arranged before the current point, remain valid: for 
	example, you can save a reference and advance the pointer, and the reference 
	remains usable.
 
	If instead you need to go to a single specified offset, then use
	RMBufChain::Goto() or RMBufPacketPeek::Access().

	@post
	A Generic implementation assert: 
	after construct, iTail == 0 iff iCurrent == 0 (all scanned), or
	in other words: as long as there are bytes after current point,
	iTail will be non-zero (and More() returns ETrue).
	All methods maintain this invariant or leave, if impossible.

	Some other utility methods, not directly related to scanning, are also included. 
	@since v7.0
	@publishedAll
	@released
	*/
	{
public:
	IMPORT_C TPacketPoker(RMBufChain &aChain);

	inline void SkipL(TInt aSize)
		/**
		Moves the current point forward a specified number of bytes.

		@param aSize Number of bytes to move forward
		@leave KErrEof
			if the request cannot be satisfied.
		*/
		{ if (aSize < iTail) { iTail -= aSize; iOffset += aSize; } else OverL(aSize); }

	inline TUint8 *Ptr() const
		/**
		Raw pointer to the current point (can be invalid, if iTail = 0).
	
		@note Internal "unsafe" method
		*/
		{return iCurrent->Ptr() + iOffset; }

	inline TUint8 *ReferenceL(TInt aSize = 1)
		/**
		Gets a pointer to the current point, such that
		at least the specified minimum number of bytes can be read.

		@param aSize
			Specified minimum number of bytes to be read through
			the returned pointer.
		@return Raw data pointer
		@leave KErrEof
			if the request cannot be satisfied.
		*/
		{ if (iTail >= aSize) return Ptr(); else return AdjustL(aSize); }

	inline TUint8 *ReferenceAndSkipL(TInt aSize)
		/**
		Gets a pointer to the current point, such that at least the
		specified minimum number of bytes can be read,
		and moves the point the specified number of bytes forward.

		@param aSize
			Specified minimum number of bytes to be read through the returned 
			pointer, and the number of bytes to move forward
		@return
			Raw data pointer
		@leave KErrEof
			if the request cannot be satisfied.
		*/
		{ TUint8 *x = ReferenceL(aSize); SkipL(aSize); return x; }

	inline TInt Remainder() const
		/**
		Gets the length of the contiguous space after the current point.	

		@return Length after the current point
		*/
		{ return iTail; }

	inline TBool AtBegin() const
		/**
		Tests whether the current point is at the beginning of an RMBuf.

		@return ETrue if current point is at the beginning
		*/
		{ return iOffset == 0; }

	inline TBool More() const
		/**
		Tests whether there is more data to scan.

		@return ETrue if there is more data to scan
		*/
		{ return iTail > 0; }

	IMPORT_C static TBool IsExtensionHeader(TInt aProtocolId);
private:
	IMPORT_C void OverL(TInt aSize);
	IMPORT_C TUint8 *AdjustL(TInt aSize);
	/** The RMBuf of the current point. */
	RMBuf *iCurrent;
	/** The offset of the current point in the RMBuf. */
	TInt iOffset;
	/** Remaining bytes starting from the current point in the RMBuf. */
	TInt iTail;
	};

#endif
