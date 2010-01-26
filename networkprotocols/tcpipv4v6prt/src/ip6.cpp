// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ip6.cpp - IPv6 protocol
//

#include "inet6log.h"
#include <ip6_hdr.h>
#include <ip4_hdr.h>
#include <icmp6_hdr.h>
#include <udp_hdr.h>
#include <in_pkt.h>
#include <in_chk.h>
#include <in6_if.h>	// only for KIpBroadcastOnLink
#include "ip6.h"
#include <ip6_hook.h>
#include <ip6_iprt.h>
#include "ip6_rth.h"
#include "ip6_doh.h"
#include "ip6_frag.h"
#include "in_flow.h"
#include "iface.h"
#include "in_net.h"
#include <ext_hdr.h>
#include <inet6err.h>
#include "loop6.h"
#include "res.h"	// only for KProtocolInet6Res
#include "addr46.h"
#include "tcpip_ini.h"
#include <comms-infras/nifif_internal.h>

// The preprocessor symbol: ARP
// ----------------------------
// Add code for doing IPv4 Address Resolution Protocol (ARP) on
// IPv4 interfaces that specify "NeedNd". (also needed in ïface.cpp)

#ifdef ARP
#include <arp_hdr.h>
#endif

static const TLitC8<sizeof(TInt)> KInetOptionDisable = {sizeof(TInt), {0}};

//
//	****************
//	TUpperLayerSnoop
//	****************
//	A simple class to map the first 4 bytes of the packet into
//	ports or icmp type/code. This is used to snoop the upper
//	layer information. (The use of "real" upper layer header
//	classes directly would complicate things: each protocol
//	would need to have a separate mapping)
//
class TUpperLayerSnoop
	{
	public:
		//
		// Basic
		//
		inline static TInt MinHeaderLength() {return 4; }
		inline static TInt MaxHeaderLength() {return 4; }

		union
			{

			//
			// The same mapping will do for both TCP and UDP,
			// as only port fields are really accessed.
			//
			TInet6HeaderUDP udp;
			TInet6HeaderICMP icmp;
			};
	};

//
//
//	CProtocolIP6
//	************
//	The implementation and even the class definition of CProtocolIP6 is
//	totally internal to ip6.cpp. No other module needs to know any of the
//	internals. Thus, all declarations of the CProtocolIP6 are included here
//
//
class CHookEntry;
class THookList
	{
	friend class CProtocolIP;
	friend class CProtocolIP4;
	friend class CProtocolIP6;
	void AddL(CProtocolBase *aProtocol);
	void AddL(CIp6Hook *aHook, TInt aPriority);
	TInt AddByOrderListL(CIp6Hook *aHook,  const TDesC &aName, const TDesC &aOrdering, const TInt aPriority);
	TInt Remove(const CProtocolBase *const aProtocol);
	TInt Remove(const CIp6Hook *const aHook);
	TInt Remove(const TAny *const any);
	TInt RemoveAll();
	void StartSending(CProtocolBase *aSource);
	void StartSending(CProtocolBase *aIface, CProtocolBase *aSrc);
	void Error(TInt aError, CProtocolBase *aSource);
	void Error(TInt aError, CProtocolBase *aIface, CProtocolBase *aSrc);
	void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf);
	void InterfaceDetached(const TDesC &aName, CNifIfBase *aIf);
private:
	void Delink(CHookEntry *p, CHookEntry *h);
	void Link(CHookEntry *p, CHookEntry *h);
	CHookEntry *iHead;
	TUint iChainId;
	};


//
//	TBoundHookEntry
//	---------------
//		Bookkeeping entry for the protocols that are bound via
//		esock.ini directive to the IP6 instance
//
class TBoundHookEntry
	{
public:
	CProtocolBaseUnbind *iProtocol;	// Bound protocol, always non-NULL!
	CNifIfBase *iInterface;			// Interface, non-NULL for a special "interface protocol"
	};

//
//	TIcmpThrottle
//	-------------
//
class TIcmpThrottle
	{
public:
	TIcmpThrottle() :
//		iMask((TUint32)(~((1 << 26) - 1))),	// mask matching about 67 seconds
		iMask((TUint32)(~((1 << 23) - 1))),	// mask matching about 8 seconds
		iStamp(0)
		{}
	TInt Suppress();
	inline void SetMax(TInt aMax) { iMax = aMax; }
private:
	const TUint32 iMask;
	TUint32 iStamp;
	TInt iMax, iCount;
	};

TInt TIcmpThrottle::Suppress()
	{
	TTime now;
	//
	// The idea is to have a low overhead throttle test by
	// defining the interval as a 2**N of microseconds
	// (2**23 ~ 8 seconds), and just using the remaining
	// high order bits as a time stamp, which when changed
	// allows more to send... (this will allow a burst of
	// of iMax messages, in the beginning of time interval!)
	//
	now.UniversalTime();
#ifdef I64LOW
	TUint ref = (TUint)I64LOW(now.Int64()) & iMask;
#else
	TUint ref = now.Int64().Low() & iMask;
#endif
	if (ref != iStamp)
		{
		iStamp = ref;
		iCount = 0;
		}
	return ++iCount > iMax;
	}

// TPacketContextItem
// ******************
class TPacketContextItem
	{
public:
	TUint32 iKey, iValue;
	};

class CProtocolIP : public CProtocolInet6Network, public MNetworkServiceExtension, public MPacketContext
	{
	friend class IP6;
public:
	CProtocolIP(CIfManager *aInterfacer, TInt aProtocol);
	~CProtocolIP();
	void InitL(TDesC& aTag);
	CServProviderBase* NewSAPL(TUint aSockType);
	void BindToL(CProtocolBase *protocol);
	void BindL(CProtocolBase *protocol, TUint id);
	void StartL(void);
	void Process(RMBufChain &aPacket, CProtocolBase* aInterface);
	void Unbind(CProtocolBase *protocol, TUint id = 0);
	void Identify(TServerProtocolDesc *) const;
	TInt GetOption(TUint level,TUint name,TDes8 &option,CProtocolBase* aSourceProtocol);
	TInt SetOption(TUint level, TUint aName,const TDesC8 &option,CProtocolBase* aSourceProtocol);
	//
	// Active users tracking is done by the interface manager,
	// just pass the following directly.
	//
	void IncUsers() { iInterfacer->IncUsers(); }
	void DecUsers() { iInterfacer->DecUsers(); }


	void Error(TInt aError, CProtocolBase* aSrc);
	void StartSending(CProtocolBase* aSrc);

	//
	// Network service (MNetworkService)
	//
	CProtocolInet6Binder *Protocol() const;
	MInterfaceManager *Interfacer() const;
	TInt Send(RMBufChain &aPacket, CProtocolBase* aSrc = NULL);
	void Icmp4Send(RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC = 0);
	void Icmp6Send(RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC = 0);
	void IcmpWrap(RMBufChain &aPacket, const TIcmpTypeCode aIcmp, const TUint32 aParameter = 0, const TInt aMC = 0);

	TBool Fragment(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TInt aMtu, RMBufPktQ &aFragments);

	CHostResolvProvdBase *NewHostResolverL();
	CServiceResolvProvdBase *NewServiceResolverL();
	CNetDBProvdBase *NewNetDatabaseL();

	//
	// Network service extension (additional methods MNetworkServiceExtension)
	//
	void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf);
	void InterfaceDetached(const TDesC &aName, CNifIfBase *aIf);


	//
	// The Flow Manager section (MFlowManager)
	//
	CFlowContext *NewFlowL(const void *aOwner, TUint aProtocol);
	CFlowContext *NewFlowL(const void *aOwner, CFlowContext &aFlow);
	TInt SetChanged() const;
	TInt FlowSetupHooks(CFlowInternalContext &aFlow);
	void FlowStartRefresh(CFlowInternalContext &aFlow);
	TInt GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, const CFlowContext &aFlow) const;
	TInt SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow);
	//
	// The Packet Context (MPacketContext)
	//
	TInt SetHookValue(const TUint32 aId, const TUint32 aValue);
	TUint32 HookValue(const TUint32 aId) const;

	TBool DoSwitchL(RMBufHookPacket &aPacket);
	void PostProcess(RMBufChain &aPacket, CProtocolBase *aSrc);

private:
	void DoProcess();
	void IcmpEcho(RMBufPacketBase &aPacket, RMBufRecvInfo *aInfo);
	void IcmpSend(TInt aProtocol, RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC);
	TPtrC HookOrdering(const TDesC &aOrderKey) const;
	static TInt RecvCallBack(TAny* aProtocol);
protected:
	TInt IcmpHandlerL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo, CProtocolBase *aFinal);
	void UnbindAll(TAny *aProtocol);
	TInt DoBuild(RMBufPacketBase &aPacket,  RMBufPktInfo &aInfo, TPacketHead &aHead);
	void DoSendOnePacket(RMBufSendPacket &aPacket);
	void DoProcessOnePacketL(RMBufHookPacket &aPacket);
	CIfManager *const iInterfacer;
	// Resolver instance
	CProtocolBase *iResolver;
	// IPv4 "slave" instance, if NON-NULL (catched in BindToL,
	// not "reference counted"!).
	CProtocolIP *iSlavedIP4;
	//
	// Protocol "identity", the following members contain either
	//	(KProtocolInetIp,KProtocolInetIcmp) for IPv4
	// or
	//	(KProtocolInet6Ip, KProtocolInet6Icmp) for IPv4
	// Both of these could always be deduced from the iProtocol of
	// TServerProtocolDesc, but this way makes some shared code
	// simpler to write.
	const TInt iProtocol, iIcmpProtocol;
	//
	// A brute force protocol switch array (will possibly get revised,
	// later if needed).This array is directly indexed by the next header
	// value of the IP packet.
	//
	// Every element of the array will always have a valid pointer value
	// assigned (iNullProtocol). At run time, there is never need to verify
	// against a NULL pointer.
	//
	// Also, a pointer in iSwitch does not mean that CProtocolIP6 class
	// owns that instance, the objects pointed by this array are NOT
	// deleted when IP6 instance is destroyed.
	//
	MNifIfUser *iNifUser;	// Defined when protocol is registered with the interfacer

	THookList iSwitch[KIp6Hook_ANY+1];
	TUint SwitchSize() {return sizeof(iSwitch) / sizeof(iSwitch[0]); }
	CHookEntry *iNullHook;
	CFragmentHeaderHook *iFragmentHeader;
	CNifIfBase	*iLoopback4;	// "Hardcoded" IPv4 loopback interface
	CNifIfBase	*iLoopback6;	// "Hardcoded" IPv6 loopback interface
	//
	// State information used by the ICMP "throttle"
	//
	TIcmpThrottle iIcmpThrottle;
	// iBindCount counts how many non-NULL protocols are currently
	// stored in iSwitch. This variably is purely for debugging
	// purposes. It *should* be zero when this protocol instance is
	// destroyed!
	//
	TInt iBindCount;
	//
	// List of hooks that are interested in peeking at the outbound packets
	//
	THookList iOutbound;
	//
	// List of hooks that are intrested in packets that got received by
	// the stack, but which apparently are not addresses to this host
	// (e.g. packets that may need forwarding).
	//
	THookList iForwardHooks;
	//
	// List of hooks that want to peek complete packets just before they
	// are sent to the interface. These hooks must be aware of the iFlow
	// in the info block, and are responsible to actully passing the packet
	// to the interface. These hooks receive complete packets through the
	// Send (Outbound) and Process (Inboud) interfaces.
	//
	THookList iPostInbound;
	THookList iPostOutbound;
	CIp6Hook *iPostTerminator;
	//
	// Some hooks may be installed by using the 'bindto'-directive of ESOCK.INI
	// on IP6 instance. This will call BindtoL of IP6 to call Bind()+Open() to the
	// indicated protocol. Need to keep track of these protocols and issue matching
	// Close() calls in case IP6 is deleted. (NOTE: protocol bound in this way
	// cannot "die" until IP6 releases them). iBoundHooks tracks these.
	//
	CArrayFixFlat<TBoundHookEntry> *iBoundHooks;
	//
	// Identification for the IPv4 header field
	//
	TInt iId;
	//
	// Packet forwarding section
	//
	TUint iForwardFlowSize;			// Max Number of entries in the array
	TUint iForwardFlowCount;			// Current number of forward flow contexts.
	RFlowContext *iForwardFlow;		// Array of RFlowContext handles
	TUint iForwardHits;				// Packets forwarded without resetting a flow
	TUint iForwardMiss;				// Packets forwarded with resetting a flow
	const TUint iSwitchSize;             
	//
	// Work space for maintaining inbound packet context (during DoSwitch())
	// (fixed allocation now, implement something else later if needed)
	//
	TInt iPacketContextCount;		// Number of used entries
	// .. the max number of slots is automaticly determined by the
	// .. size of the array iPacketContext (see SetHookValue).
	TPacketContextItem iPacketContext[8];
	TUint32 iPacketId;				// Packet sequence number (used as packet id).
	//
	// The queue for the received packets from the NIF's.
	//
	RMBufAsyncPktQ iRecvQ;
	};

class CProtocolIP6 : public CProtocolIP
	{
	friend class IP6;
public:
	CProtocolIP6(CIfManager *aInterfacer);
	~CProtocolIP6();
	void InitL(TDesC& aTag);
private:
	CRoutingHeaderHook *iRoutingHeader;
	CDestinationOptionsHook *iDestinationOptions;
	CHopOptionsHook *iHopOptions;
	};


class CProtocolIP4 : public CProtocolIP
	{
	friend class IP6;
public:
	CProtocolIP4(CIfManager *aInterfacer);
	~CProtocolIP4();
	void InitL(TDesC& aTag);
	};

//
//	IP6 Class Implementation
//
void IP6::Identify(TServerProtocolDesc &aEntry, TInt aVersion)
	{
	if (aVersion == STATIC_CAST(TInt, KProtocolInetIp))
		{
		aEntry.iName=_S("ip");
		aEntry.iAddrFamily=KAfInet;
		aEntry.iProtocol=KProtocolInetIp;
		// message size is absolute upper limit, not guaranteed to work
		aEntry.iMessageSize=0xffff-TInet6HeaderIP4::MinHeaderLength();
		}
	else
		{
		aEntry.iName=_S("ip6");
		aEntry.iAddrFamily=KAfInet6;
		aEntry.iProtocol=KProtocolInet6Ip;
		// message size is absolute upper limit, not guaranteed to work
		aEntry.iMessageSize=0xffff-TInet6HeaderIP::MinHeaderLength();
		}
	aEntry.iSockType=KSockDatagram;
	aEntry.iVersion=TVersion(KInet6MajorVersionNumber, KInet6MinorVersionNumber, KInet6BuildVersionNumber);
	aEntry.iByteOrder=EBigEndian;
	aEntry.iServiceInfo=KIP6ServiceInfo;
	aEntry.iNamingServices=KIP6NameServiceInfo;
	aEntry.iSecurity=KSocketNoSecurity;
	aEntry.iServiceTypeInfo=KIP6ServiceTypeInfo;
	aEntry.iNumSockets=KUnlimitedSockets;
	}

CProtocolBase *IP6::NewL(CIfManager *aInterfacer, TInt aVersion)
	{
	CProtocolBase *prt;
	if (aVersion == STATIC_CAST(TInt, KProtocolInetIp))
		prt = new (ELeave) CProtocolIP4(aInterfacer);
	else
		prt = new (ELeave) CProtocolIP6(aInterfacer);
	return prt;
	}

//
//	THookList
//		The implementation of this is totally internal to this IP6
//		protocol module. Thus, all declaratations and implementation
//		is here. No outside module needs to know how this works!
//
class CHookEntry : public CBase
	{
	friend class THookList;
	friend class CProtocolIP;
	friend class CProtocolIP4;
	friend class CProtocolIP6;
	CHookEntry(CProtocolBase *aProtocol, CHookEntry *aNext) : iType(0), iNext(aNext)
		/** Construct upper layer protocol entry. */
		{ iProtocol = aProtocol; }
	CHookEntry(CIp6Hook *aHook, CHookEntry *aNext, TInt aPriority) : iType(aPriority), iNext(aNext)
		/** Construct a hook entry. */
		{ iHook = aHook; }
	inline TBool IsHook() const { return iType != 0; }
	inline TBool IsProtocol() const { return iType == 0; }
private:
	TInt iType;
	union
		{
			CProtocolBase *iProtocol;	//< if iType == 0,
			CIp6Hook *iHook;			//< if iType != 0, for inbound packet hook
		};
	class CHookEntry *iNext;
	};


void THookList::AddL(CProtocolBase *aProtocol)
	/**
	* Adds an upper layer protocol handler.
	*
	* Override the current protocol handler with a new
	* protocol binding. The previous binding (if any) is not
	* removed, but is hidden behind this new one until it unbinds.
	*
	* The new element is added in front of the old one.
	*
	* @param aProtocol	The protocol
	*/
	{
	CHookEntry *p = NULL;
	CHookEntry *h, **head = &iHead;
	//
	// Skip over possible hook mode handlers
	//
	while ((h = *head) != NULL && h->IsHook())
		{
		p = h;
		head = &h->iNext;
		}
	//
	// 'h' points the current first protocol (if it exists), just insert
	// this new protocol in front of it and "hide" the previous one
	//
	// coverity[alloc_fn] coverity[assign]	
	*head = new (ELeave) CHookEntry(aProtocol, h);
	Link(p, *head);
	// coverity[memory_leak]  
	}

void THookList::AddL(CIp6Hook *aHook, TInt aPriority)
	/**
	* Add a new hook handler into the list.
	*
	* @param aHook		The hook
	* @param aPriority	The priority (must be > 0)
	*/
	{
	if (aPriority > 0)
		{
		CHookEntry *p = NULL;
		CHookEntry *h, **head = &iHead;
		while ((h = *head) != NULL && h->iType > 0 && h->iType > aPriority)
			{
			p = h;
			head = &h->iNext;
			}
		// coverity[alloc_fn] coverity[assign]	
		*head = new (ELeave) CHookEntry(aHook, h, aPriority);
		Link(p, *head);
		}
	// coverity[memory_leak]  
	}

// THookList::AddByOrderList
// *************************
// Add protocol to list based on the ordering.
//
// The priority is computed by (bigger is better):
//
// (10000 - hook position in the list) * 256 + aPriority
//
// If the protocol is mentioned more than once in the aOrdering,
// and if list is not chained, then the protocol will be added
// multiple times.
//
// Returns the change in iBindCount (usually +1).
//
// *WARNING*
//	If protocol is addedd multiple times and the latter AddL
//	leaves, then the iBindCount will be incorrect. [to keep
//	count correct in such case is too much trouble and failing
//	to add is pretty serious problem which most likely shuts
//	down the stack anyway... -- msa]
//
//
TInt THookList::AddByOrderListL(CIp6Hook *aHook,  const TDesC &aName, const TDesC &aOrdering, const TInt aPriority)
	{
	TInt star = 0;
	TInt slot = 10000 << 8;

	// Only one "BindL" call per protocol/list allowed. Remove
	// all previous bindings before starting to process this
	// new set.
	TInt bindcount = -Remove((TAny *)aHook);

	TLex start(aOrdering);
	for (TInt done = 0;;)
		{
		slot -= 256;
		if (start.Eos())
			{
			if (!done)
				{
				// Protocol has not been specially mentioned in the ordering.
				// Add it into the '*'-position (or, if there is none specified
				// assume star at the end of the list).
				AddL(aHook, aPriority + (star ? star : slot));
				bindcount++;
				}
			break;
			}
		// *NOTE/WARNING* This is a simple parsing,
		// no extra white space is accepted!
		start.Mark();
		while (!start.Eos() && start.Peek() != ',')
			start.Inc();
		TPtrC hook = start.MarkedToken();
		if (!start.Eos())
			start.Inc();	// Skip ','

		if (hook.Compare(_L("*")) == 0)
			star = slot;
		else if (hook.CompareF(aName) == 0)
			{
			// Located the specific priority entry
			AddL(aHook, slot + aPriority);
			done = 1;
			bindcount++;
			if (iChainId)
				// If hook list is chained, protocol can only be
				// added once!
				break;
			}
		}
	return bindcount;
	}

//
// THookList::Delink
// *****************
// The hook 'h' is has been removed from a list, where it
// was located after 'p'. If this is chained list, then
// updatate the bindings.
//
// p == NULL, if h was the first hook
// p->iNext is already updated to point the hook
// the follows h (or NULL, if none).
//
void THookList::Delink(CHookEntry *p, CHookEntry *h)
	{
	if (iChainId)
		{
		//
		// If h was not the first in the list, then
		// need to remove the chaining to h from the
		// previous hook.
		if (p)
			p->iHook->Unbind(h->iHook, iChainId);
		//
		// If h was not the last in the list, then
		// need to remove the chaining from 'h'
		// to the next hook
		if (h->iNext)
			h->iHook->Unbind(h->iNext->iHook, iChainId);
		//
		// If h was not the first hook, and there was
		// another hook following h, then must link
		// the previous hook (p) to the following.
		if (p && h->iNext)
			{
			TRAP_IGNORE(p->iHook->BindL(h->iNext->iHook, iChainId));
			}
		}
	}

// THookList::Link
// ***************
// The hook 'h' has been added to the list after 'p'
//
// p == NULL, if has was added to the front
//
void THookList::Link(CHookEntry *p, CHookEntry *h)
	{
	if (iChainId)
		{
		TInt err;
		//
		// If h was added after p and there
		// is a hook after h, then the chaining from
		// p must be unbound.
		if (p && h->iNext)
			p->iHook->Unbind(h->iNext->iHook, iChainId);
		//
		// If h is now followed by another hook, then must link
		// h to this.
		if (h->iNext)
			{
			TRAP(err, h->iHook->BindL(h->iNext->iHook, iChainId));
			}
		//
		// If h was not the first hook, then must link the previous
		// hook to h.
		if (p)
			{
			TRAP(err, p->iHook->BindL(h->iHook, iChainId));
			}
		}
	}


//
// THookList::Remove
//	Remove one specific protocol binding from the list.
//	Returns 1, if found and removed
//	Returns 0, if not found
//
TInt THookList::Remove(const CProtocolBase *const aProtocol)
	{
	CHookEntry **head = &iHead;
	CHookEntry *p = NULL;
	for (CHookEntry *h; (h = *head) != NULL; p = h, head = &h->iNext)
		if (h->IsProtocol() && h->iProtocol == aProtocol)
			{
			*head = h->iNext;
			Delink(p, h);
			delete h;
			return 1;
			}
	return 0;
	}

//
// CHookList::Remove
//	Remove one specific hook binding from the list
//	Returns 1, if found and removed
//	Returns 0, if not found
//
TInt THookList::Remove(const CIp6Hook *const aHook)
	{
	CHookEntry **head = &iHead;
	CHookEntry *p = NULL;
	for (CHookEntry *h; (h = *head)->IsHook(); p = h, head = &h->iNext)
		if (h->iHook == aHook)
			{
			*head = h->iNext;
			Delink(p, h);
			delete h;
			return 1;
			}
	return 0;
	}

//
// THookList::Remove
//	Remove all bindings to the specified protocol or hook.
//	Returns the number of of bindings removed.
//
TInt THookList::Remove(const TAny *const any)
	{
	TInt count = 0;
	CHookEntry **head = &iHead;
	CHookEntry *p = NULL;
	for (CHookEntry *h; (h = *head) != NULL; )
		if (h->iHook == any)
			{
			*head = h->iNext;
			Delink(p, h);
			delete h;
			++count;
			}
		else
			{
			p = h;
			head = &h->iNext;
			}
	return count;
	}

// THookList::RemoveAll
//	Remove all entries from the list and
//	return the number of removed entries
//
TInt THookList::RemoveAll()
	{
	TInt count = 0;

	CHookEntry *h;

	while ((h = iHead) != NULL)
		{
		iHead = h->iNext;
		Delink(NULL, h);
		delete h;
		++count;
		}
	return count;
	}

//
//	THookList::StartSending/Error
//
//		Deliver upcall for a target protocol
//		(Hooks are not informed currently)

void THookList::StartSending(CProtocolBase *aSrc)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsProtocol() && h->iProtocol != aSrc)
			{
			h->iProtocol->StartSending(NULL);
			break;
			}
	}

void THookList::Error(TInt aError, CProtocolBase *aSrc)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsProtocol() && h->iProtocol != aSrc)
			{
			h->iProtocol->Error(aError, NULL);
			break;
			}
	}

void THookList::InterfaceAttached(const TDesC &aName, CNifIfBase *aIf)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsHook())
			h->iHook->InterfaceAttached(aName, aIf);
	}

void THookList::InterfaceDetached(const TDesC &aName, CNifIfBase *aIf)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsHook())
			h->iHook->InterfaceDetached(aName, aIf);
	}

void THookList::StartSending(CProtocolBase *aIface, CProtocolBase *aSrc)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsHook() && h->iProtocol != aSrc)
			h->iHook->StartSending(aIface);
	}

void THookList::Error(TInt aError, CProtocolBase *aIface, CProtocolBase *aSrc)
	{
	for (CHookEntry *h = iHead; h; h = h->iNext)
		if (h->IsHook() && h->iProtocol != aSrc)
			h->iHook->Error(aError, aIface);
	}

//
//	CProtocolIP6Null
//
//	CProtocolIP6Null is purely internal to the ip6.cpp
//	implementation. It is used as a dummy protocol instance
//	which will receive all packets which have unbound protocol
//	in their next header field. This is *NOT* a visible protocol
//	for the socket manager.
//
//	*HOWEVER* This is somewhat dubious, drags in some stuff from
//	CProtocolBase. Should use some intermediate class?
//
class CProtocolIP6Null  : public CProtocolBase
	{
public:
	CProtocolIP6Null(MNetworkService *aIp) : iIp(aIp) {}
	void Identify(TServerProtocolDesc *) const {Panic(EInet6Panic_NotSupported);}
	void Process(RMBufChain &aPacket,CProtocolBase *)
		{
		RMBufRecvPacket packet;
		packet.Assign(aPacket);
		RMBufRecvInfo *const info = packet.Unpack();
		if (//
			// Drop unhandled ICMP errors silently
			//
			info->iIcmp == 0 &&
			//
			// Drop packets with No Next Header silently
			//
			info->iProtocol != STATIC_CAST(TInt, KProtocolInet6NoNextHeader) &&
			//
			// Drop all ICMP's when there is no upper
			// layer ICMP protocol to receive them.
			//
			info->iProtocol != STATIC_CAST(TInt, KProtocolInetIcmp) &&
			info->iProtocol != STATIC_CAST(TInt, KProtocolInet6Icmp))
			{
			switch (info->iVersion)
				{
				case 4:
					iIp->Icmp4Send(packet, KInet4ICMP_Unreachable, 2 /* Protocol Unreach */);
					return;
				case 6:
					iIp->Icmp6Send(packet, KInet6ICMP_ParameterProblem, 1, info->iPrevNextHdr);
					return;
				default:
					break;
				}
			}
		//
		// Drop silently
		packet.Free();
		}
	void StartSending(CProtocolBase *) {};
	void Error(TInt, CProtocolBase *) {};
private:
	MNetworkService *iIp;
	};



//
//	CProtocolPostTerminator
//
//	CProtocolPostTerminator is purely internal to the ip6.cpp
//	implementation. It is used as a terminal protocol instance
//	for post hook lists. This is *NOT* a visible protocol
//	for the socket manager.
//
//	*HOWEVER* This is somewhat dubious, drags in some stuff from
//	CProtocolBase. Should use some intermediate class?
//
class CProtocolPostTerminator : public CIp6Hook
	{
public:
	CProtocolPostTerminator(CProtocolIP *aIp) : iIp(aIp) {}

	void Process(RMBufChain &aPacket,CProtocolBase *aInterface)
		{
		iIp->PostProcess(aPacket, aInterface);
		}

	TInt Send(RMBufChain &aPacket, CProtocolBase *)
		{
		RMBufSendInfo *const info = RMBufSendPacket::PeekInfoInChain(aPacket);
		if (info)
			{
			CFlowContext *const flow = info->iFlow.FlowContext();
			if (flow)
				return flow->Send(aPacket);
			}
		aPacket.Free();
		return 1;
		}
	TInt ApplyL(RMBufHookPacket &, RMBufRecvInfo &) { return KIp6Hook_PASS; }
	void StartSending(CProtocolBase *) {}
	void Error(TInt, CProtocolBase *) {}
private:
	CProtocolIP *iIp;
	};

//
//
//	Dual IP6/IP4 Stack methods
//	**************************
//	Both IP6 and IP4 have same methods. The method implementations of
//	CProtocol{IP,IP6,IP4) are grouped together, when different implementations
//	are required. Only CProtocolIP method is present when one shared
//	implementations is sufficient.
//
//
//	*NOTE*
//		The current configuration is that all of the work is handled
//		by the IPv6 instance (iNetwork), and the IPv4 instance is only
//		required as a connection point to the link layer. However, it
//		is left fully functional in case there is some future need to
//		separate the processing again and have IPv4 and IPv6 with their
//		own iSwitch and hooks. (the current idea is that when hook
//		binds a protocol, it gets both IPv4 and IPv6 for that protocol
//		-- msa
//
//	Constructors
//	************
//
CProtocolIP::CProtocolIP(CIfManager *aInterfacer, TInt aProtocol) :
	iInterfacer(aInterfacer),
	iProtocol(aProtocol),
	iIcmpProtocol(aProtocol == STATIC_CAST(TInt, KProtocolInet6Ip) ? KProtocolInet6Icmp : KProtocolInetIcmp),
	iSwitchSize(SwitchSize())
	{
	iNetwork = this;
	}

CProtocolIP6::CProtocolIP6(CIfManager *aInterfacer) : CProtocolIP(aInterfacer, KProtocolInet6Ip)
	{
	LOG(Log::Printf(_L("\tip6 new")));
	}

CProtocolIP4::CProtocolIP4(CIfManager *aInterfacer) : CProtocolIP(aInterfacer, KProtocolInetIp)
	{
	LOG(Log::Printf(_L("\tip new")));
	}

//
//	Destructors
//	***********
//
CProtocolIP::~CProtocolIP()
	{
	iNetwork = NULL;	// Prevent Close() call from the base class destructor!
	iSlavedIP4 = NULL;	// Not relevant any more!

	while (iForwardFlowSize > 0)
		iForwardFlow[--iForwardFlowSize].Close();	// Release flow (if allocated)
	delete[] iForwardFlow;
	iForwardFlow = NULL;

	iInterfacer->Unregister(this);// No more calls to this from interface manager
	//
	// Release all hook lists
	//
	CHookEntry *h;
	for (TUint i = 0; i < iSwitchSize; i++)
		{
		// Note: cannot use iSwitch[i].RemoveAll() because the
		// iNullHook is a member of every list, and it can be
		// released only once!
		//
		while ((h = iSwitch[i].iHead) != iNullHook)
			{
			iSwitch[i].iHead = iSwitch[i].iHead->iNext;
			delete h;
			}
		}
	(void)iOutbound.RemoveAll();
	(void)iForwardHooks.RemoveAll();
	(void)iPostInbound.RemoveAll();
	(void)iPostOutbound.RemoveAll();
	//
	// Issue a Close() to all protocols attaced in BindToL
	//
	if (iBoundHooks)
		{
		for (TInt i = iBoundHooks->Count(); i > 0;)
			{
			TBoundHookEntry *bound = &iBoundHooks->At(--i);
			if (bound->iInterface)
				{
				iNifUser->IfUserInterfaceDown(KErrServerTerminated, bound->iInterface);
				bound->iInterface->Close();
				}
			//
			// "Unbind problem": All protocol classes which are bound
			// from INET6 must support the Unbind,
			// [iProtocol cannot be NULL, let crash if so... -- msa]
			bound->iProtocol->Unbind(this);
			bound->iProtocol->Close();
			}
		delete iBoundHooks;
		}
	if (iResolver)
		iResolver->Close();

	if (iLoopback4)
		{
		iNifUser->IfUserInterfaceDown(KErrServerTerminated, iLoopback4);
		iLoopback4->Close();
		iLoopback4 = 0;
		}
	if (iLoopback6)
		{
		iNifUser->IfUserInterfaceDown(KErrServerTerminated, iLoopback6);
		iLoopback6->Close();
		iLoopback6 = 0;
		}

	delete iPostTerminator;
	delete iFragmentHeader;
	
	if (iNullHook)
		{
		delete iNullHook->iProtocol;
		delete iNullHook;
		}
	}

CProtocolIP6::~CProtocolIP6()
	{
	LOG(Log::Printf(_L("\t%S destruct - start"), &ProtocolName()));
	//
	// Cleanup
	//
	// .. need to do unbinds, because the destructors
	// of the internal protocols don't do it properly
	// for now -- msa
	UnbindAll(iRoutingHeader);
	UnbindAll(iDestinationOptions);
	UnbindAll(iHopOptions);

	delete iRoutingHeader;
	delete iDestinationOptions;
	delete iHopOptions;
	LOG(Log::Printf(_L("\t%S destruct - done"), &ProtocolName()));
	}

CProtocolIP4::~CProtocolIP4()
	{
	LOG(Log::Printf(_L("\t%S destruct"), &ProtocolName()));
	}

//	CProtocolIP::HookOrdering
//	*************************
//	Retrieve hook ordering for the specific hooklist.
//	(Currently from TCPIP.INI)
//
//	Returns a descriptor for the ordering
//	(can be a "NULL" descriptor, if not found)
//
TPtrC CProtocolIP::HookOrdering(const TDesC &aOrderKey) const
	{
	TPtrC ordering;
	if (!iInterfacer->FindVar(TCPIP_INI_HOOK, aOrderKey, ordering))
		return TPtrC(0,0);
	return ordering;
	}

//
//	CProtocolIP::BindL
//	******************
//	(shared by both)
//	The aId has a special format and interpretation:
//
//	aId >= KIp6Hook_ANY
//		The binding protocol wants to bind "as a hook" for extension
//		header identified by (aid - KIp6Hook_ANY). Such protocol must
//		implement the MExtensionHook interface. If the result after
//		subtract is KIp6Hook_ANY, the request is for generic hook,
//		which is called after all extension headers have
//		been handled, but before passing the packet to the next layer.
//
//	0 < aId < KIpHook_ANY
//		The binding protocol want to bind as a normal upper layer protocol
//		for the IP protocol indicated by aId.
//
void CProtocolIP::BindL(CProtocolBase *aProtocol, TUint aId)
	{
	TServerProtocolDesc info;
	aProtocol->Identify(&info);
#ifdef _LOG
	Log::Printf(_L("BindL\t%S called for %S[%u] id=%d"), &ProtocolName(), &info.iName, (TInt)aProtocol, aId);
#endif

	if (aId == 0)
		Panic(EInet6Panic_BadBind);	// Cannot bind 0 as protocol!
	else if (aId == KProtocolInet6Ip && info.iProtocol == KProtocolInet6Ip)
		{
		// ip6 bindto ip
		//
		// *SPECIAL KLUDGE* -- msa
		//	The current protocol is the "dumb" IPv4 instance and the magic
		//	IPv6 instance is being bound to it: by setting the iNetwork
		//	the IPv4 iSwitch is not used, and all received packets are
		//	handled by the IPv6 iSwitch!
		//  [some typecasting occurring, iNetwork is really
		//   MNetworkService * -- msa]
		iNetwork = (CProtocolIP *)aProtocol;
		LOG(Log::Printf(_L("\t\tAssigned %S[%u] as master IPv6, total binds = %d"), &info.iName, (TInt)aProtocol, iBindCount+1));
		}
	else if (aId == KProtocolInetIp && info.iProtocol == KProtocolInetIp)
		{
		// ip bindto ip6
		iSlavedIP4 = (CProtocolIP *)aProtocol;
		LOG(Log::Printf(_L("\t\tAssigned %S[%u] as slaved IPv4, total binds = %d"), &info.iName, (TInt)aProtocol, iBindCount+1));
		}
	else if (aId < STATIC_CAST(TUint, KIp6Hook_ANY))	// Note: cannot bind ANY as a protocol!
		{
		iSwitch[aId].AddL(aProtocol);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as final protocol %d handler, total binds = %d"), &info.iName, (TInt)aProtocol, aId, iBindCount+1));
		}
	else if ((aId -= KIp6Hook_ANY) <= STATIC_CAST(TUint, KIp6Hook_ANY))
									// Note: can bind 0 as a hook!
		{
		iBindCount += iSwitch[aId].AddByOrderListL((CIp6Hook *)aProtocol, info.iName, HookOrdering(TCPIP_INI_HOOK_INANY), 1);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as protocol %d hook, total binds = %d"), &info.iName, (TInt)aProtocol, aId, iBindCount));
		return;
		}
	else if ((aId -= KIp6Hook_ANY) < STATIC_CAST(TUint, KIp6Hook_ANY))
		//
		// Setup as outbound hook with priority = aId (0 < aId < KIpHook_ANY)
		//
		{
		iBindCount += iOutbound.AddByOrderListL((CIp6Hook *)aProtocol, info.iName, HookOrdering(TCPIP_INI_HOOK_FLOW), aId);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as outbound flow hook, pri=%d, total binds = %d"), &info.iName, (TInt)aProtocol, aId, iBindCount));
		return;
		}
	else if ((aId -= KIp6Hook_ANY) == 0)
		{
		iBindCount += iPostOutbound.AddByOrderListL((CIp6Hook *)aProtocol, info.iName, HookOrdering(TCPIP_INI_HOOK_OUTBOUND), 1);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as post outbound hook, total binds = %d"), &info.iName, (TInt)aProtocol, iBindCount));
		return;
		}
	else if (--aId == 0)
		{
		iBindCount += iPostInbound.AddByOrderListL((CIp6Hook *)aProtocol, info.iName, HookOrdering(TCPIP_INI_HOOK_INBOUND), 1);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as post inbound hook, total binds = %d"), &info.iName, (TInt)aProtocol, iBindCount));
		return;
		}
	else if (--aId == 0)
		{
		iBindCount += iForwardHooks.AddByOrderListL((CIp6Hook *)aProtocol, info.iName, HookOrdering(TCPIP_INI_HOOK_FORWARD), 1);
		LOG(Log::Printf(_L("\t\tBinding %S[%u] as forwarding hook, total binds = %d"), &info.iName, (TInt)aProtocol, iBindCount));
		return;
		}
	else
		Panic(EInet6Panic_BadBind);
	iBindCount++;
	return;
	}

//	CProtocolIP::UnbindAll
//	**********************
//	Remove all bind references to the protocol. This method
//	must be "non-virtual", so that it can be used in
//	destructors.
void CProtocolIP::UnbindAll(TAny *aProtocol)
	{
	// Unbinding IPv4 instance (this must have been "ip bindto ip6" configuration)
	if (aProtocol == iSlavedIP4)
		{
		iSlavedIP4 = NULL;
		--iBindCount;
		}
	for (TUint i = 0; i < SwitchSize(); ++i)
		iBindCount -= iSwitch[i].Remove(aProtocol);
	iBindCount -= iOutbound.Remove(aProtocol);
	iBindCount -= iForwardHooks.Remove(aProtocol);
	iBindCount -= iPostInbound.Remove(aProtocol);
	iBindCount -= iPostOutbound.Remove(aProtocol);
	}

//
//	CProtocolIP::Unbind
//	*******************
//	(shared by both)
//
//	A protocol wishes not to receive any more packets from the
//	IP level.
//
void CProtocolIP::Unbind(CProtocolBase *aProtocol, TUint aId)
	{
	LOG(Log::Printf(_L("Unbind\t%S from [%u] id=%d"), &ProtocolName(), (TUint)aProtocol, aId));
	if (aId == 0)
		UnbindAll((TAny *)aProtocol);
	else if (aId < STATIC_CAST(TUint, KIp6Hook_ANY))
		iBindCount -= iSwitch[aId].Remove(aProtocol);
	else if ((aId -= KIp6Hook_ANY) <= STATIC_CAST(TUint, KIp6Hook_ANY))
		iBindCount -= iSwitch[aId].Remove((CIp6Hook *)aProtocol);
	else if ((aId -= KIp6Hook_ANY) < STATIC_CAST(TUint, KIp6Hook_ANY))
		iBindCount -= iOutbound.Remove((CIp6Hook *)aProtocol);
	else if ((aId -= KIp6Hook_ANY) == 0)
		iBindCount -= iPostOutbound.Remove(aProtocol);
	else if (--aId == 0)
		iBindCount -= iPostInbound.Remove(aProtocol);
	else if (--aId == 0)
		iBindCount -= iForwardHooks.Remove(aProtocol);
	//
	// iNetwork can be NULL, if we are getting here from
	// ~CPrototolIP()... -- msa
	//
	if (iNetwork && aProtocol == iNetwork->Protocol())
		{
		--iBindCount;
		iNetwork = this;
		}
	LOG(Log::Printf(_L("Unbind\t%S complete, total binds %d"), &ProtocolName(), iBindCount));
	}

//
//	CProtocolIP::NewSAPL
//	********************
//	(shared by both)
//
CServProviderBase* CProtocolIP::NewSAPL(TUint aSockType)
	{
	return IP6::NewSAPL(aSockType, this, iProtocol);
	}


CProtocolInet6Binder *CProtocolIP::Protocol() const
	{
	return (CProtocolInet6Binder *)this;
	}

MInterfaceManager *CProtocolIP::Interfacer() const
	{
	return iInterfacer;
	}

CHostResolvProvdBase *CProtocolIP::NewHostResolverL()
	{
	if (iResolver == NULL)
		User::Leave(KErrNotSupported);
	return iResolver->NewHostResolverL();
	}

CServiceResolvProvdBase *CProtocolIP::NewServiceResolverL()
	{
	if (iResolver == NULL)
		User::Leave(KErrNotSupported);
	return iResolver->NewServiceResolverL();
	}

CNetDBProvdBase *CProtocolIP::NewNetDatabaseL()
	{
	if (iResolver == NULL)
		User::Leave(KErrNotSupported);
	return iResolver->NewNetDatabaseL();
	}

//
//	CProtocolIP::InitL
//	******************
//
void CProtocolIP::InitL(TDesC& aTag)
	{
	TCallBack recvCb(RecvCallBack, this);
	iRecvQ.InitL(recvCb);

	CProtocolInet6Network::InitL(aTag);
	//
	// Setup the protocol switch
	//
	CProtocolIP6Null* protIP6Null = new (ELeave) CProtocolIP6Null(this);
	CleanupStack::PushL(protIP6Null);
	iNullHook = new (ELeave) CHookEntry(protIP6Null, NULL);
	CleanupStack::Pop();

	iBindCount = 0;
	for (TUint i = 0; i < iSwitchSize; i++)
		iSwitch[i].iHead = iNullHook;

	// Pick up some random start value for the Identification
	TTime time;
	TInt64 seed;
	time.UniversalTime();
	seed = time.Int64();
	iId = Math::Rand(seed);

	iPostTerminator = new (ELeave) CProtocolPostTerminator(this);
	iPostInbound.iChainId = MIp6Hook::BindPostHook()+1;
	iPostOutbound.iChainId = MIp6Hook::BindPostHook();
	iPostInbound.AddL(iPostTerminator);
	iPostOutbound.AddL(iPostTerminator);
	iFragmentHeader = CFragmentHeaderHook::NewL(this);
	iFragmentHeader->ConstructL();
	}

//
//	CProtocolIP6::InitL
//	*******************
//
void CProtocolIP6::InitL(TDesC& aTag)
	{
	CProtocolIP::InitL(aTag);
	// Get configured value for the ICMP limiter.
	iIcmpThrottle.SetMax(iInterfacer->GetIniValue(TCPIP_INI_IP, TCPIP_INI_ICMP_LIMIT, KTcpipIni_IcmpLimit, 0, KMaxTInt));
	// Enable packet forwarding if allowed by configuration
	// (only needed for the IP6 instance)
	iForwardFlowSize = (TUint)iInterfacer->GetIniValue(TCPIP_INI_IP, TCPIP_INI_FORWARD, KTcpipIni_Forward, 0, KMaxTInt);
	if (iForwardFlowSize > 0)
		{
		iForwardFlow = new RFlowContext[iForwardFlowSize];
		if (iForwardFlow == NULL)
			{
			LOG(Log::Printf(_L("No memory for forward = %d, disabled"), (TInt)iForwardFlowSize));
			iForwardFlowSize = 0;
			}
		}

	iRoutingHeader = new (ELeave) CRoutingHeaderHook(this);
	iRoutingHeader->ConstructL();	// Should call BindL

	iHopOptions = new (ELeave) CHopOptionsHook(this);
	iHopOptions->ConstructL();
	iDestinationOptions = new (ELeave) CDestinationOptionsHook(this);
	iDestinationOptions->ConstructL();

	//Bind myself to handle IPIP-detunneling
	BindL(this, KProtocolInet6Ipip);
	// ...need also IPv4 binding here
	BindL(this, KProtocolInetIpip);
	iNifUser = iInterfacer->Register(this);
	}

//
//	CProtocolIP4::InitL
//	*******************
//
void CProtocolIP4::InitL(TDesC& aTag)
	{
	CProtocolIP::InitL(aTag);
	iIcmpThrottle.SetMax(iInterfacer->GetIniValue(TCPIP_INI_IP, TCPIP_INI_ICMP_LIMIT, KTcpipIni_IcmpLimit, 0, KMaxTInt));
	iNifUser = iInterfacer->Register(this);
	}

//
//	CProtocolIP::BindToL
//	********************
//	(shared by both)
//
void CProtocolIP::BindToL(CProtocolBase *aProtocol)
	{
#ifdef _LOG
	TServerProtocolDesc log_info;
	aProtocol->Identify(&log_info);
	Log::Printf(_L("BindToL\t%S binding to %S[%u] start"), &ProtocolName(), &log_info.iName, (TInt)aProtocol);
#endif
	//
	// Experimentally, allow binding of IP instance
	// to other protocols as a way of activating them
	// IP instance makes no assumputions about the
	// protocols bound in this way.
	//
	TInt err;	
	aProtocol->Open();
	TRAP(err, aProtocol->BindL(this, iProtocol));
	if (err == KErrNone)
		{
		TBoundHookEntry binding;
		binding.iProtocol = (CProtocolBaseUnbind *)aProtocol;
		binding.iInterface = NULL;
		if (iBoundHooks || (iBoundHooks = new CArrayFixFlat<TBoundHookEntry>(4)) != NULL)
			{
			TServerProtocolDesc info;
			aProtocol->Identify(&info);
			if (info.iServiceTypeInfo & EInterface)
				{
				// The bound "protocol" is actually some kind of
				// interface, get the interface and pass it to the
				// interfacer!
				TServerProtocolDesc my_info;
				Identify(&my_info);
				TRAP(err, binding.iInterface = ((CProtocolInterfaceBase *)aProtocol)->GetBinderL(my_info.iName));
				if (err == KErrNone && binding.iInterface)
					{
					binding.iInterface->Open();
					TRAP(err, iNifUser->IfUserNewInterfaceL(binding.iInterface, 0));
					}
				}
			if (err == KErrNone)
				{
				TRAP(err, iBoundHooks->AppendL(binding));
				if (err == KErrNone)
					{
					//
					// If all is ok, detect if a "special" protocol providing
					// the host resolver services is being bound. If so, save
					// (or replace) a reference also into iResolver.
					//
					// [Instead of this, could one just check some bits in
					// iNamingServices and detect a protocol that way? -- msa
					//
					if (info.iProtocol == KProtocolInet6Res)
						{
						if (iResolver)
							iResolver->Close();
						iResolver = aProtocol;
						iResolver->Open();
						}
					else if (info.iProtocol == KProtocolInetIp)
						//
						// this ip6 bindto ip
						//
						// iSlavedIP4 is only needed for providing
						// backward compatibility for old raw sockets
						// which open to 'ip' protocol and expect to
						// get IPv4 only with IP header swapped!
						iSlavedIP4 = (CProtocolIP4 *)aProtocol;
					else if (info.iProtocol == KProtocolInet6Ip)
						// this ip bindto ip6
						iNetwork = (CProtocolIP *)aProtocol;
					LOG(Log::Printf(_L("BindToL\t%S binding to %S[%u] recorded"), &ProtocolName(), &log_info.iName, (TInt)aProtocol));
					return;			// BindToL completed!
					}
				}
			//
			// Failed for some reason, cleanup
			//
			if (binding.iInterface)
				{
				iNifUser->IfUserInterfaceDown(err, binding.iInterface);
				binding.iInterface->Close();
				}
			}
		else
			err = KErrNoMemory;
		// Cancel BindL
		binding.iProtocol->Unbind(this, iProtocol);
		}
	aProtocol->Close();
#ifdef _LOG
	Log::Printf(_L("BindToL\t%S binding to %S[%u] failed with %d"), &ProtocolName(), &log_info.iName, (TInt)aProtocol, err);
#endif
	User::Leave(err);
	}


//
//	CProtocolIP::StartL
//	********************
//	(shared)
//
void CProtocolIP::StartL(void)
	{
	CProtocolInet6Network::StartL();

	if (iProtocol == STATIC_CAST(TInt, KProtocolInet6Ip))
		{
		if (iLoopback6 == NULL)
			{
			//
			// Insert IPv6 Loopback Interface and Route
			//
			_LIT(loop6, "loop6");

			iLoopback6 = CIfLoop6::NewL(loop6);
			iLoopback6->Open();
			iNifUser->IfUserNewInterfaceL(iLoopback6, 0);
			iInterfacer->AddRouteL(KInet6AddrLoop, 128, loop6, 1);
			}
		if (iLoopback4 == NULL)
			{
			//
			// Insert IPv4 Loopback Interface and Route
			//
			_LIT(loop4, "loop4");

			iLoopback4 = CIfLoop6::NewL(loop4);
			iLoopback4->Open();
			iNifUser->IfUserNewInterfaceL(iLoopback4, 0);
			TInetAddr loopnet;
			loopnet.SetV4MappedAddress((127 << 24) + 1);
			iInterfacer->AddRouteL(loopnet.Ip6Address(),128 - 32 + 8, loop4, 1);
			}
		}
	}

//
//	CProtocolIP::Identify
//	*********************

void CProtocolIP::Identify(TServerProtocolDesc *aInfo) const
	{
	IP6::Identify(*aInfo, iProtocol);
	}

//
//	CProtocolIP::GetOption
//	**********************
//	(shared)

TInt CProtocolIP::GetOption(TUint aLevel, TUint aName, TDes8& aOption, CProtocolBase* /*aSourceProtocol=NULL*/)
	{
	if(aLevel == KNifOptLevel)
		{
		if(aName == KNifOptGetNifIfUser)
			{
			TNifIfUser ifuser;
			ifuser() = iNifUser;
			aOption.Copy(ifuser);
			return KErrNone;
			}
		return KErrNotSupported;
		}
	return iInterfacer->GetOption(aLevel, aName, aOption);
	}

//
//	CProtocolIP::SetOption
//	**********************
//	(shared)
//
TInt CProtocolIP::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption, CProtocolBase*)
	{
	return iInterfacer->SetOption(aLevel, aName, aOption);
	}

//
//	MergeHeadAndPayload
//	*******************
//	A static, shared utility function for DoBuild methods
//
static TInt MergeHeadAndPayload(RMBufPacketBase &aPacket, TPacketHead &aHead, TInt aHdrLen)
	{
	TInt err;
	//
	// Prepend the head information to the payload part
	//
	if (aHead.iOffset > 0)
		{
		RMBufChain work;
		err = aHead.iPacket.Copy(work);
		if (err == KErrNone)
			aPacket.Prepend(work);
		work.Free();
		}
	if (aPacket.IsEmpty())
		err = aPacket.Alloc(aHdrLen);
	else
		err = aPacket.Prepend(aHdrLen);
	return err;
	}

// DoBuildIp4
// **********
static TInt DoBuildIp4(RMBufPacketBase &aPacket, RMBufPktInfo &aInfo, TPacketHead &aHead, TInt aId)
	{
	aInfo.iProtocol = KProtocolInetIp;
	//
	// Prepend the head information to the payload part
	//
	//	*NOTE*
	//		The following will create more RMBuf's than
	//		is absolutely necessary. Look into this and
	//		optimize at some point!! -- msa
	//
	const TInt ip4len = TInet6HeaderIP4::MinHeaderLength();	// No IP options for now!
	const TInt err = MergeHeadAndPayload(aPacket, aHead, ip4len);
	if (err == KErrNone)
		{
		TInet6Checksum<TInet6HeaderIP4> ip(aPacket);	// Access the packet as IP header.
		if (ip.iHdr)
			{
			aInfo.iLength += ip4len + aHead.iOffset;	// ..above merge does not do this!
			//
			// Prepare the final packet and info (before hooks).
			// Note: iOffset does not include the IPv4 header length
			// Note: err is already KErrNone
			// Note 3: Certain packets must not have ECN ECT bits set. tcp_sap.cpp sets
			// KIpNoEcnEct for those packets.
			ip.iHdr->Init((aInfo.iFlags & KIpNoEcnEct) ?
					(aHead.ip6.TrafficClass()&0xfc) : aHead.ip6.TrafficClass());
			ip.iHdr->SetProtocol(aHead.ip6.NextHeader());
			ip.iHdr->SetTtl(aHead.ip6.HopLimit());
			// Raw Address load. Someone else must have already checked that
			// both src and dst are IPv4 mapped addresses at the end of the
			// ReadyL phase! This just does a blind copy... -- msa
			ip.iHdr->DstAddrRef() = aHead.ip6.DstAddr().u.iAddr32[3];
			ip.iHdr->SrcAddrRef() = aHead.ip6.SrcAddr().u.iAddr32[3];
			// The info->iLength is assumed to be correctly maintained!
			ip.iHdr->SetTotalLength(aInfo.iLength);
			ip.iHdr->SetIdentification(aId);
			//
			// Somewhat ad hoc thing: if DontFragment flag is set, then
			// set the DF bit to the IPv4 header... -- msa
			//
			if (aInfo.iFlags & KIpDontFragment)
				// ..this "set" generates more code than is necessary, as
				// a simple "bitset" to the proper byte would do... --msa
				ip.iHdr->SetFlags((TUint8)(ip.iHdr->Flags() | KInet4IP_DF));
			ip.ComputeChecksum();
			}
		else
			return KErrGeneral;
		}
	return err;
	}

// DoBuildIp6
// **********
static TInt DoBuildIp6(RMBufPacketBase &aPacket, RMBufPktInfo &aInfo, TPacketHead &aHead)
	{
	aInfo.iProtocol = KProtocolInet6Ip;
	const TInt err = MergeHeadAndPayload(aPacket, aHead, sizeof(TInet6HeaderIP));
	if (err == KErrNone)
		{
		TInet6Packet<TInet6HeaderIP> ip(aPacket);	// Access the packet as IP header.
		if (ip.iHdr)
			{
			//
			// Prepare the final packet and info (before hooks).
			// Note: iOffset does not include the IPv6 header length
			// Note: err is already KErrNone!
			*ip.iHdr = aHead.ip6;

			// Note: Certain packets must not have ECN ECT bits set. tcp_sap.cpp sets
			// KIpNoEcnEct for those packets.
			if (aInfo.iFlags & KIpNoEcnEct)
				{
				ip.iHdr->SetTrafficClass(ip.iHdr->TrafficClass() & 0xfc);
				}

			// Note! info iLength is assumed to be correctly maintained!
			aInfo.iLength += aHead.iOffset;			// ..above merge does not do this!
			ip.iHdr->SetPayloadLength(aInfo.iLength);
			aInfo.iLength += sizeof(TInet6HeaderIP);
			}
		else
			return KErrGeneral;		// Bad packet
		}
	return err;
	}

//	CProtocolIP::DoBuild
//	********************
//	Build packet for IPv4 or IPv6
//
TInt CProtocolIP::DoBuild(RMBufPacketBase &aPacket, RMBufPktInfo &aInfo, TPacketHead &aHead)
	{
	if (aInfo.iFlags & KIpHeaderIncluded)
		{
		// Set aInfo.iProtocol based on the included header		
		TInet6Packet<TInet6HeaderIP4> ip(aPacket);	// Access the packet as IP header.
		if (ip.iHdr)
			{
			switch (ip.iHdr->Version())
				{
				case 4:
					aInfo.iProtocol = KProtocolInetIp;
					return KErrNone;
				case 6:
					aInfo.iProtocol = KProtocolInet6Ip;
					return KErrNone;
				default:
					break;
				}
			}
		}
	else switch (aHead.ip6.Version())
		{
		case 4:
			return DoBuildIp4(aPacket, aInfo, aHead, ++iId);
		case 6:
			return DoBuildIp6(aPacket, aInfo, aHead);
		default:
			break;
		}
	return KErrNotSupported;
	}


// CProtocolIP::Fragment
// *********************
TBool CProtocolIP::Fragment(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo, TInt aMtu, RMBufPktQ &aFragments)
	{
	if ((aInfo.iFlags & KIpDontFragment) == 0 && iFragmentHeader)
		{
		//
		// Remove the flow context from the packet
		// (thus fragmenter does not need to worry about it)
		//
		RFlowContext orig;
		orig.Grab(aInfo.iFlow);
		iFragmentHeader->Fragment(aPacket, aMtu, aFragments);
		aPacket.Free();				// Just in case...
		if (aFragments.IsEmpty()) // Fragmentation failed
			{
			orig.Close();
			return EFalse;
			}
		//
		// Attach a reference to the original flow context into each fragment.
		//
		TMBufPktQIter iter(aFragments);
		for (iter.SetToFirst(); iter.More(); iter++)
			{
			const RMBufChain& frag = (RMBufChain &)iter.Current();
			// PeekInfoInChain should really have "const" in parameter...
			RMBufSendInfo *info = (RMBufSendInfo *)RMBufPacketBase::PeekInfoInChain((RMBufChain &)frag);
			info->iFlow.Copy(orig);
			}
		orig.Close();
		return ETrue;
		}
	else
		{
		// Attempt to generate ICMP Error when packet is to be dropped
		// due unallowed fragmenting
		aInfo.iFlow.Close();				// Detach flow from info!
		aPacket.Pack();
		IcmpWrap(aPacket, TIcmpTypeCode(KInet4ICMP_Unreachable, 4, KInet6ICMP_PacketTooBig, 0), aMtu);
		ASSERT(aPacket.IsEmpty());			// IcmpWrap is guaranteed to either take the aPacket or to free it.
		return EFalse;
		}
	}

//	CProtocolIP::DoSendOnePacket
//	****************************
void CProtocolIP::DoSendOnePacket(RMBufSendPacket &aPacket)
	{
	RMBufSendInfo *info = aPacket.Unpack();
	CFlowInternalContext *flow;
	RMBufPktQ fragments;

	if (!info->iFlow.IsOpen())
		{
		//
		// No flow attached, try opening a flow matching the info.
		// *WARNING* If anyone (like 6to4) relies on this, remember.. this is a
		// load to the system (each packet is allocated own flow).
		// Something better should be done -- msa
		if (info->iFlow.Open(iNetwork, info->iDstAddr, info->iSrcAddr, info->iProtocol) != KErrNone)
			goto packet_drop;
		}
	flow = (CFlowInternalContext *)info->iFlow.FlowContext();	// Always non-NULL!

	// inilize port information to zero for the NIF
	// (A hook may change these, if it co-operates with the NIF)
	info->iSrcAddr.SetPort(0);
	info->iDstAddr.SetPort(0);

	// Note: DoBuild() initiliazes info->iProtocol either to KProtocolInet6Ip or KProtocolInetIp
	// depending on the intial IP header (IPv6 or IPv4).
	//
	// info->iProtocol can be used by the NIF to decide how the packet will be framed and
	// it should be properly set.
	//
	// If any hook in ApplyHooks adds tunnels, they must also change the the info->iProtocol
	// to match the new outermost IP header.
	//
	if (DoBuild(aPacket, *info, flow->iHead) != KErrNone)
		goto packet_drop;		//
	if (flow->ApplyHooks(aPacket, *info, fragments, *this) != KErrNone)
		goto packet_drop;
	if (fragments.IsEmpty())
		{
		aPacket.Pack();
		iPostOutbound.iHead->iHook->Send(aPacket);
		}
	else
		{
		// Send out the fragments
		while (fragments.Remove(aPacket))
			{
			iPostOutbound.iHead->iHook->Send(aPacket);
			}
		}
	return;
packet_drop:
	if (aPacket.IsEmpty())
		{
		// Packet has been "eaten" by someone -- not availble any more
		LOG(Log::Printf(_L("\tDROPPED SEND -- Packet dropped/processed by hook/internally")));
		}
	else
		{
		// Default close/free, if packet has not been "eaten" by some already
#ifdef _LOG
		TBuf<70> tmp_src;
		TBuf<70> tmp_dst;
		TInetAddr::Cast(info->iSrcAddr).OutputWithScope(tmp_src);
		TInetAddr::Cast(info->iDstAddr).OutputWithScope(tmp_dst);
		Log::Printf(_L("\tDROPPED SEND src=[%S] dst=[%S] proto=%d"), &tmp_src, &tmp_dst, info->iProtocol);
#endif
		info->iFlow.Close();			// Detach flow from packet
		aPacket.Free();					// Release packet
		}

	// If packet got fragmented, release the fragments.
	while (fragments.Remove(aPacket))
		{
		info = aPacket.Unpack();
		info->iFlow.Close();
		aPacket.Free();
		}
	}

//	CProtocolIP::InterfaceAttached
//	******************************
//	Notification from the interface manager about new interface
//	instance being bound to the stack. The network layer does not
//	self need this for anything.
void CProtocolIP::InterfaceAttached(const TDesC &aName, CNifIfBase *aIf)
	{
	// There are two instances, one for IPv6 and one for IPv4.
	// Currently all processing is concentrated into iNetwork
	// instance (== IPv6) and the IPv4 instance is just a
	// "pass-through".
 	if (iNetwork != this && iNetwork)
		{
		((CProtocolIP *)iNetwork->Protocol())->InterfaceAttached(aName, aIf);
 		return;
		}
	iOutbound.InterfaceAttached(aName, aIf);
	iForwardHooks.InterfaceAttached(aName, aIf);
	iPostOutbound.InterfaceAttached(aName, aIf);
	iPostInbound.InterfaceAttached(aName, aIf);
	for (TUint i = 0; i < iSwitchSize; i++)
		iSwitch[i].InterfaceAttached(aName, aIf);
	}

//	CProtocolIP::InterfaceDetached
//	******************************
//	Notification from the interface manager about old interface
//	instance being unbound to the stack. The network layer does not
//	self need this for anything.
void CProtocolIP::InterfaceDetached(const TDesC &aName, CNifIfBase *aIf)
	{
	// There are two instances, one for IPv6 and one for IPv4.
	// Currently all processing is concentrated into iNetwork
	// instance (== IPv6) and the IPv4 instance is just a
	// "pass-through".
 	if (iNetwork != this && iNetwork)
		{
		((CProtocolIP *)iNetwork->Protocol())->InterfaceDetached(aName, aIf);
 		return;
		}
	iOutbound.InterfaceDetached(aName, aIf);
	iForwardHooks.InterfaceDetached(aName, aIf);
	iPostOutbound.InterfaceDetached(aName, aIf);
	iPostInbound.InterfaceDetached(aName, aIf);
	for (TUint i = 0; i < iSwitchSize; i++)
		iSwitch[i].InterfaceDetached(aName, aIf);
	}


//
//	CProtocolIP::StartSending
//	*************************
//	Upcalls from the interfaces
//
void CProtocolIP::StartSending(CProtocolBase *aIface)
	{
	LOG(Log::Printf(_L("StartSending\t%S for NIF[%u] - Start"), &ProtocolName(), (TInt)aIface));

	// There are two instances, one for IPv6 and one for IPv4.
	// Currently all processing is concentrated into iNetwork
	// instance (== IPv6) and the IPv4 instance is just a
	// "pass-through".
 	if (iNetwork != this && iNetwork)
		{
		iNetwork->Protocol()->StartSending(aIface);
 		return;
		}

	// If the parameter is NULL, then this a forwarded
	// call from some bound protocol and this only occurs
	// for UP transitions. Otherwise this is a real StartSending
	// from the interface and the transition state is decided
	// by the interface manager.
	//
	if (aIface == NULL ||
		iInterfacer->StartSending((CNifIfBase*)aIface) == KIfaceTransition_UP)
		{
		//
		// Notify All bound prototocols
		//
		for (TUint i = 0; i < iSwitchSize; i++)
			iSwitch[i].StartSending(this);
		}
	//
	// Notify post hook (if any)
	//
	if (aIface)
		// iPostInbound too?
		iPostOutbound.StartSending(aIface, this);
	LOG(Log::Printf(_L("StartSending\t%S for NIF[%u] - Done"), &ProtocolName(), (TInt)aIface));
	}

//
//	CProtocolIP::Error
//	******************
//	Upcalls from the interfaces
//
void CProtocolIP::Error(TInt aError, CProtocolBase* aIface)
	{
	LOG(Log::Printf(_L("Error\t%S Error=%d for NIF[%u] - start"), &ProtocolName(), aError, (TInt)aIface));

	// There are two instances, one for IPv6 and one for IPv4.
	// Currently all processing is concentrated into iNetwork
	// instance (== IPv6) and the IPv4 instance is just a
	// "pass-through".
 	if (iNetwork != this && iNetwork)
		{
 		iNetwork->Protocol()->Error(aError, aIface);
 		return;
		}

	if (aIface == NULL ||
		iInterfacer->Error(aError, (CNifIfBase *)aIface) != KIfaceTransition_NONE)
		{
		//
		// Notify All bound prototocols
		//
		for (TUint i = 0; i < iSwitchSize; i++)
			iSwitch[i].Error(aError, this);
		}
	//
	// Notify post hook (if any)
	//
	if (aIface)
		// iPostInbound too?
		iPostOutbound.Error(aError, aIface, this);
	LOG(Log::Printf(_L("Error\t%S Error=%d for NIF[%u] - completed"), &ProtocolName(), aError, (TInt)aIface));
	}


#ifdef _LOG
//
//	VerifyPacket
//	************
//	This checks the consistency of the various information fields.
//	Intended for DEBUG use to check the correctness of the Hook returns
//
static void VerifyPacket(RMBufRecvPacket &aPacket, const RMBufRecvInfo &aInfo)
	{
	// - there must be something to process!
	ASSERT(!aPacket.IsEmpty());
	// - hook must maintain correct iLength!
	ASSERT(aInfo.iLength == aPacket.Length());
	// - the related IP header must always be before the current header
	ASSERT(aInfo.iOffsetIp < aInfo.iOffset);
	// - the previous next header field is always before the current header, but after
	//   the related IP header
	ASSERT(aInfo.iPrevNextHdr > aInfo.iOffsetIp);
	ASSERT(aInfo.iPrevNextHdr < aInfo.iOffset);
	// - if the packet is on ICMP error path (IcmpHandler), then iOffsetIp should
	//   point to the IP header of the returned error packet, and must always be
	//   preceded by ICMP header (and thus >= 8)
	ASSERT(aInfo.iIcmp == 0 || aInfo.iOffsetIp >= 8);
	// - iOffset is always <= iLength!
	ASSERT(aInfo.iOffset <= aInfo.iLength);
	// - legal values for the next header field are [0..255] ( --> iProtocol)
	ASSERT(aInfo.iProtocol < 256);
	// - hook can freely mangle the packet RMBufs, but the returned packet
	//   must always have the *original* information block!
	ASSERT(&aInfo == aPacket.Info());
	// - info addresses must always be in IPv6 format
	ASSERT(aInfo.iSrcAddr.Family() == KAfInet6);
	ASSERT(aInfo.iDstAddr.Family() == KAfInet6);
	}
#endif

//
//	ClassifyIcmp
//	************
//	This is an internal help utility to classify ICMP messages by
//	their type into three categories by following returns
//
typedef enum
	{
	EIcmpType_NONE,		// Not an ICMP, protocol is neither ICMPv4 nor ICMPv6
	EIcmpType_ECHO,		// ICMP echo request
	EIcmpType_ERROR,	// ICMP error report
	EIcmpType_OTHER		// Other ICMP, none of the above
	} TIcmpType;
//
static TIcmpType ClassifyIcmp(const TInt aProtocol, const TInt aType)
	{
	if (aProtocol == STATIC_CAST(TInt, KProtocolInet6Icmp))
		{
		if (aType == KInet6ICMP_EchoRequest)		// .. == 128
			return EIcmpType_ECHO;
		else if (aType > KInet6ICMP_EchoRequest)	// .. > 128
			return EIcmpType_OTHER;						// ...are non-Error ICMP's
		else
			return EIcmpType_ERROR;
		}
	else if (aProtocol == STATIC_CAST(TInt, KProtocolInetIcmp))
		{
		switch (aType)
			{
			case KInet4ICMP_Unreachable:		// Destination unreachable
			case KInet4ICMP_SourceQuench:		// Source Quench
			case KInet4ICMP_Redirect:			// Redirect request
			case KInet4ICMP_TimeExceeded:		// Time Exceeded
			case KInet4ICMP_ParameterProblem:	// Parameter Problem
				return EIcmpType_ERROR;
			case KInet4ICMP_Echo:				// Echo
				return EIcmpType_ECHO;
			default:
				return EIcmpType_OTHER;
			}
		}
	return EIcmpType_NONE;
	}

//
// Compute simple hash value from TInetAddr
//
static TUint HashIt(const TInetAddr &aAddr)
	{
	TUint hash = aAddr.Port() & 0xffff;
	const TIp6Addr &addr = aAddr.Ip6Address();
	hash += addr.u.iAddr16[0];
	hash += addr.u.iAddr16[1];
	hash += addr.u.iAddr16[2];
	hash += addr.u.iAddr16[3];
	hash += addr.u.iAddr16[4];
	hash += addr.u.iAddr16[5];
	hash += addr.u.iAddr16[6];
	hash += addr.u.iAddr16[7];
	hash += aAddr.Scope();
	return hash;
	}



// CProtocolIP::SetHookValue
// *************************
TInt CProtocolIP::SetHookValue(const TUint32 aId, const TUint32 aValue)
	{
	// The packet id (identified iwth aId==0) cannot be set (read-only)
	if (aId == 0)
		return KErrArgument;

	TInt j = iPacketContextCount; // Next free slot, if any
	for (TInt i = 0;; ++i)
		{
		if (i == iPacketContextCount)
			{
			// Key not found.
			if (aValue == 0)
				return KErrNone; // No need to store 0 values!
			if (j == i) // == iPacketContextCount
				{
				// need to allocate new slot
				const TInt size = sizeof(iPacketContext) / sizeof(iPacketContext[0]);
				if (j == size)
					return KErrNoMemory; // No room!
				iPacketContextCount += 1;
				}
			break;
			}
		if (iPacketContext[i].iKey ==  aId)
			{
			j = i; // Same key, replace with new value
			break;
			}
		if (iPacketContext[i].iValue == 0)
			j = i; // If value == 0, then entry can be reused
		}

	iPacketContext[j].iKey = aId;
	iPacketContext[j].iValue = aValue;
	return KErrNone;
	}

// CProtocolIP::HookValue
// **********************
TUint32 CProtocolIP::HookValue(const TUint32 aId) const
	{
	// aId==0 is a special key for the Packet Id (read-only)

	if (aId == 0)
		return iPacketId;

	for (TInt i = 0; i < iPacketContextCount; ++i)
		if (iPacketContext[i].iKey == aId)
			return iPacketContext[i].iValue;
	return 0;
	}

//
//	CProtocolIP::DoSwitchL()
//	************************
//
TBool CProtocolIP::DoSwitchL(RMBufHookPacket &aPacket)
	/**
	* Processes one layer of IP header with extension headers.
	*
	* @param aPacket The packet (unpacked state)
	*
	* @return
	* 	@li	FALSE, if packet has been processed (either dropped or accepted)
	*	@li TRUE, if detunneling happened (packet contained another IP header)
	*
	* The leave happens if a called ApplyL leaves.
	* The caller must trap the leave and release the packet.
	*/
	{
packet_redo:

	// If a generic hook modifies the packet (returns KIp6Hook_DONE), then
	// this cancels the currently selected transport protocol (target) and
	// the protocol specific hooks for the new protocol are executed. After
	// this, all generic hooks are executed again.
	//
	// If none of the protocol specific hooks modifies the packet (all return
	// KIp6Hook_PASS), then without the exclusion mechanism, the original
	// generic hook, which restarted the process, would be called again with
	// exactly the same packet for which it already reported DONE. To spare
	// the hook from responsibility of detecting this situation, the
	// exclusion is implemented here (exclude_this), and the protocol is
	// not called.
	CHookEntry *exclude_this = NULL;

	CHookEntry *h;
	TInt ret = 0;
	RMBufRecvInfo *const info = aPacket.Info();
	info->iFlags &= ~KIpAddressVerified;	// Verify addressess at least at beginning!
	for (;;)
		{
		//
		// *NOTE*
		//	If iProtocol == 0, this is a Hob-by-Hop header. To get it executed, skip
		//	the address check and fall to the switch loop without clearing the
		//	address flag. After header is handled and skipped, the control comes back
		//	here and address is now checked. (This can happen only initially, HBH is
		//	is not accepted from the hooks at later stages)
		//
		if (info->iProtocol && (info->iFlags & KIpAddressVerified) == 0)
			{
			info->iFlags |= KIpAddressVerified;	// .. it will be after following.

			if (info->iVersion == 4)
				{
				//
				// IPv4 specific address tests (for now, just blindly assume
				// that addresses are in IPv4 mapped format, and fetch the last
				// 4 bytes of the IPv6 address... (NETWORK ORDER!)
				//
				const TUint32 src32 = TInetAddr::Cast(info->iSrcAddr).Ip6Address().u.iAddr32[3];
	
				if (((TUint8 &)src32) >= 224)	// Do not accept multicast or other weirdos as src.
					goto packet_drop;
				}
			else
				{
				// IPv6 specific address tests
				//
				// Note: ND Duplicate Address Detection requires that src=Unspecified must pass through..
				if (TIp46Addr::Cast(TInetAddr::Cast(info->iSrcAddr).Ip6Address()).IsMulticast())
					goto packet_drop;//Do not accept Multicast as a source address!
				}

			ret = iInterfacer->IsForMePacket(*info);
			if (ret < 0)
				goto packet_drop;
			else if (ret == 0)
				break;	// Not for me, forward?
			}
		//
		//
		// Call Extension Header Handlers and Hooks
		//
		CProtocolBase *target = NULL;
		for (h = iSwitch[info->iProtocol].iHead;;)
			{
			ASSERT(h != NULL);	// ..there is ALWAYS the NULL protocol with IsProtocol() == TRUE!
			if (info->iOffset > info->iLength)
				goto packet_drop;

			if (h->IsProtocol())
				{
				if (target)
					{
					if (target == this)
						{
						//
						// Detunneling detected. IP is registered as upper layer
						// only for IP-in-IP tunneling protocols.
						//
						aPacket.TrimStart(info->iOffset);	// Trim to the inner IP header.
						info->iOffset = 0;
						return TRUE;
						}

					if (aPacket.IsEmpty())
						goto packet_drop;	// Cannot have EMPTY packet at this stage!

					ASSERT(info->iLength == aPacket.Length());
					//
					// Deliver packets to raw SAP's (if any). Demux packets based
					// on IP version to appropriate IP protocol instance (if we
					// have the IP6 + IP4slave configuration).
					//
					if (info->iVersion == 4 && iSlavedIP4)
						{
						if (iSlavedIP4->iSapCount > 0)
							iSlavedIP4->Deliver(aPacket);
						}
					else if (iSapCount > 0)
						Deliver(aPacket);

					if (info->iProtocol == STATIC_CAST(TInt, KProtocolInet6Icmp) || info->iProtocol == STATIC_CAST(TInt, KProtocolInetIcmp))
						{
						// Call internal ICMP handler
						if (IcmpHandlerL(aPacket, *info, target) < 0)
							goto packet_done;
						}
					aPacket.Pack();
					target->Process(aPacket);
					goto packet_done;
					}
				//
				// Target protocol found (Phase 1 done)
				//
				target = h->iProtocol;	// Always != NULL!!
				//
				// Now, process all generic hook(s), if any registered
				//
				h = iSwitch[KIp6Hook_ANY].iHead;
				}
			else
				{
				if (h != exclude_this)
					{
					ret = h->iHook->ApplyL(aPacket, *info);
					if (ret < 0)
						goto packet_done;
					LOG(VerifyPacket(aPacket, *info));	// Hook sanity checks...
					if (ret == KIp6Hook_DONE)
						{
						if (target)
							exclude_this = h;		// -- prevent repeated call for the same packet
						else
							exclude_this = NULL;	// -- remove exclusion (if any)
						if (info->iProtocol == 0)
							{
							// HOP-by-HOP options can only appear as first after IP header
							// ?What about IPv4 case? -- msa
							Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, /*code*/ 1, info->iPrevNextHdr);
							goto packet_done;
							}
						break;	// == Next header (need to recheck MyAddress!)
						}
					ASSERT(ret == KIp6Hook_PASS);
					}
				h = h->iNext;
				}
			}
		}
#ifdef SYMBIAN_TCPIPDHCP_UPDATE	
	//Check whether the source address scope is less than the destination address scope,if that is the case through 
	//ICMP error code 2,beyond scope of source address (Refer RFC 4443: sec 3.1)
		if(info->iVersion == 6)
			{
			TInet6Checksum<TInet6HeaderIP4> ip(aPacket, info->iOffsetIp);
			if (!ip.iHdr)
				{
				goto packet_drop;
				}
			TInet6HeaderIP *const ip6 = (TInet6HeaderIP *)ip.iHdr;
			if (ip.iLength < TInet6HeaderIP::MinHeaderLength())
				{
				goto packet_drop;
				}
			if ( ip6->SrcAddr().Scope() < ip6->DstAddr().Scope())
				{
				// Attempting to forward out of scope of the source address (mainly for IPv6)
				aPacket.Pack();
				IcmpWrap(aPacket, TIcmpTypeCode(KInet4ICMP_Unreachable, 0, KInet6ICMP_Unreachable, 2));
				return EFalse;
				}
			}//end of scope check
#endif // SYMBIAN_TCPIPDHCP_UPDATE 
	//
	// *************************
	// Packet forwarding section
	// *************************
	// (a simple and quick sketch, not efficient, including
	// juggling of addresses between flow and info... -- msa)
	//

	// Try forwarding hooks first
	//
	for (h = iForwardHooks.iHead; h != NULL; h = h->iNext)
		{
		ret = h->iHook->ApplyL(aPacket, *info);
		if (ret < 0)
			goto packet_done;

		LOG(VerifyPacket(aPacket, *info));	// Hook sanity checks...
		if (ret == KIp6Hook_DONE)
			goto packet_redo;
		ASSERT(ret == KIp6Hook_PASS);
		}

	if (info->iIcmp == 0 && iForwardFlowSize > 0)
		{
		if (TInetAddr::Cast(info->iDstAddr).IsMulticast())
			goto packet_drop;		// Do not forward multicast packets!
		//
		// Modify some of IP header parameters before attempting to forward
		// (specifically addresses which may have been changed due to routing
		// header and other similar processing)
		//
		TInt hops;
		info->iFlags = KIpDontFragment|KIpHeaderIncluded;
		TInet6Checksum<TInet6HeaderIP4> ip(aPacket, info->iOffsetIp);
		if (!ip.iHdr)
			goto packet_drop;
		
		if (info->iVersion == 4)
			{
			// IPv4 specific forwarding checks
			hops = ip.iHdr->Ttl() - 1;
			ip.iHdr->SetTtl(hops);			// Blindly set, don't care if 0 or negative.
			ip.ComputeChecksum();
			if (!ip.iHdr->DF())
				info->iFlags &= ~KIpDontFragment;	// Allow fragmenting
			}
		else
			{
			// IPv6 specific forwarding checks
			// IPv4 mapping will cover also all of the IPv6 header, if packet is long enough
			TInet6HeaderIP *const ip6 = (TInet6HeaderIP *)ip.iHdr;
			if (ip.iLength < TInet6HeaderIP::MinHeaderLength())
				goto packet_drop;
	
			hops = ip6->HopLimit() - 1;
			ip6->SetHopLimit(hops);			// Blindly set, don't care if 0 or negative.
			// For now, IPv6 only forwards unicast addresses (basicly need to drop
			// all routers addresseses...
			if (!ip6->DstAddr().IsUnicast())
				goto packet_drop;
			}
		//
		// For the flow, do some quick upper layer snooping
		// (also needed to detect ICMP errors and not reply to them)
		// *NOTE*
		//	This snoop is intentionally "shallow", only the outermost
		//	layer is tested. If an ICMP error is hidden under some
		//	extension headers, it is perfectly legal to reply to it
		//	with Time Exceeded (as it is in general case impossible
		//	to skip over the potentially unkown extension headers
		//	here!)
		// *NOTE*
		//	All of this does not make much sense, because it is never
		//	100% sure for IPSEC. Thus, the right thing is not to try!
		//	- only do this lightweight snoop for ICMP when Time
		//	exceeded is about to be sent (its optional in IPv6 anyway)
		//	- accept that any IPSEC policies which affect forwarding,
		//	can only be written based on src/dst addresses. The protocol
		//	can be used, but with knowledge, that it will always be
		//	the outermost header just below the IP header! -- msa
		info->iSrcAddr.SetPort(0);
		info->iDstAddr.SetPort(0);
		// (borrow the info type & code fields for a moment)
		info->iType = 0;
		info->iCode = 0;
			{
			const TUint proto = (TUint)info->iProtocol;
			const TInt is_icmp = proto == KProtocolInetIcmp || proto == KProtocolInet6Icmp;
			const TInt is_ports = proto == KProtocolInetUdp || proto == KProtocolInetTcp;

			if (is_icmp || is_ports)
				{
				TInet6Packet<TUpperLayerSnoop> snoop(aPacket, info->iOffset);
				if (snoop.iHdr == NULL)	// runt packet!
					goto packet_drop;
				if (is_icmp)
					{
					info->iType = snoop.iHdr->icmp.Type();
					info->iCode = snoop.iHdr->icmp.Code();
					}
				else
					{
					info->iDstAddr.SetPort(snoop.iHdr->udp.DstPort());
					info->iSrcAddr.SetPort(snoop.iHdr->udp.SrcPort());
					}
				}
			}
#ifdef _LOG
			{
			TBuf<70> tmp_src;
			TBuf<70> tmp_dst;
			TInetAddr::Cast(info->iSrcAddr).OutputWithScope(tmp_src);
			TInetAddr::Cast(info->iDstAddr).OutputWithScope(tmp_dst);
			Log::Printf(_L("\tFORWARDING src=[%S %d] dst=[%S %d] proto=%d type=%d code=%d hops=%d (%d hit=%u miss=%u)"),
				&tmp_src, info->iSrcAddr.Port(),
				&tmp_dst, info->iDstAddr.Port(),
				info->iProtocol,
				(TInt)info->iType, (TInt)info->iCode,
				hops,
				iForwardFlowCount,
				iForwardHits,
				iForwardMiss);
			}
#endif
		if (hops <= 0)
			{
			//
			// Complain with Time Exceeded ICMP error
			//
			// NOTE:
			//	ClassifyIcmp can be called with non-ICMP protocol, it only returns
			//	EIcmpType_ERROR, if there is a real ICMP protocol!
			//
			if (ClassifyIcmp(info->iProtocol, info->iType) == EIcmpType_ERROR)
				goto packet_drop;		// Avoid sending ICMP error for ICMP Error!
			if (info->iVersion == 4)
				Icmp4Send(aPacket, KInet4ICMP_TimeExceeded, /*code*/ 0, /*Paramater*/ 0);
			else
				Icmp6Send(aPacket, KInet6ICMP_TimeExceeded, /*code*/ 0, /*Paramater*/ 0);
			goto packet_done;
			}
		//
		// Choose the flow
		//
		const TUint hash =
			HashIt(TInetAddr::Cast(info->iDstAddr)) +
			HashIt(TInetAddr::Cast(info->iSrcAddr)) +
			info->iCode +
			info->iType +
			info->iProtocol +
			info->iInterfaceIndex;
		RFlowContext &flow = iForwardFlow[hash % iForwardFlowSize];
		if (!flow.IsOpen())
			{
			if (flow.Open(this, 0) != KErrNone)
				goto packet_drop;	// No flow context and creation failed, just drop.
			// Mark this as forwarding flow.
			flow.FlowContext()->iInfo.iForwardingFlow = 1;
			// Do not keep interfaces up due to forwarding flows.
			flow.FlowContext()->SetOption(KSolInetIp, KSoKeepInterfaceUp, KInetOptionDisable);
			iForwardFlowCount += 1;
			}
		//
		// Set up flow for the packet
		//
		flow.SetRemoteAddr(info->iDstAddr);
		flow.SetLocalAddr(info->iSrcAddr);
		flow.SetProtocol(info->iProtocol);
		flow.SetIcmpType(info->iType, info->iCode);
		//
		// Collect some statistics
		//
		if (((CFlowInternalContext *)flow.FlowContext())->IsChanged())
			iForwardMiss += 1;
		else
			iForwardHits += 1;

		ret = ((RMBufSendInfo *)info)->iFlow.Open(flow);
		if (ret == KErrNone)
			{
			if (info->iOffsetIp > 0)	// Throw away outer IP header layers (in case some detunneling left them in buffer)
				aPacket.TrimStart(info->iOffsetIp);	// (info->iLength updated by TrimStart!)
			aPacket.Pack();
			Send(aPacket);
			return FALSE;
			}
		else if (ret == KErrInet6SourceAddress)
			{
			// Attempting to forward out of scope of the source address (mainly for IPv6)
			aPacket.Pack();
			IcmpWrap(aPacket, TIcmpTypeCode(KInet4ICMP_Unreachable, 0, KInet6ICMP_Unreachable, 2));
			return FALSE;
			}
		}
	// Just FALL to packet drop on flow attach failure
packet_drop:
#ifdef _LOG
		{
		TBuf<70> tmp_src;
		TBuf<70> tmp_dst;
		TInetAddr::Cast(info->iSrcAddr).OutputWithScope(tmp_src);
		TInetAddr::Cast(info->iDstAddr).OutputWithScope(tmp_dst);
		Log::Printf(_L("\tDROPPING src=[%S] dst=[%S] proto=%d"), &tmp_src, &tmp_dst, info->iProtocol);
		}
#endif
	aPacket.Free();
packet_done:
	ASSERT(aPacket.IsEmpty());
	return FALSE;
	}

//
//	CProtocolIP::DoProcessOnePacketL
//	********************************
//	The same code is used for both IPv6 and IPv4 protocol instances and
//	this accepts both type of packets, regardless of the source (it accepts
//	IPv6 packets from IPv4 interface and vice versa).
//
//	The caller must trap the leave and release the packet!
//	The caller is also assumed to free the packet if not consumed by this.
//
void CProtocolIP::DoProcessOnePacketL(RMBufHookPacket &aPacket)
	{
	RMBufRecvInfo *const info = aPacket.Unpack();

#ifdef ARP
	//
	// Assume the packet contains an IPv4 ARP packet, if the
	// iProtocol is set to KProtocolArp.
	//
	if (info->iProtocol == STATIC_CAST(TInt, KProtocolArp))
		{
		(void)iInterfacer->ArpHandler(aPacket, *info);
		return;
		}
#endif

	// Bump the packet id, Zero is not accepted.
	if (++iPacketId == 0)
		iPacketId = 1;

	// RFC-3168 detunneling: when a packet arrives, there
	// is no outer header and the congestion flas is
	// initially OFF.
	TBool ecnCongestion = EFalse;

	do	{
		// Each IP header resets the packet context (when detunneling is
		// done, the previous context data is forgotten. This is needed
		// because the main use of the context is to detect options
		// implemented by hooks, and if a new IP layer is started, the
		// previous option data is not any more relevant).
		//
		// Note: The packet id is not changed when detunneling occurs.
		//
		iPacketContextCount = 0;	// Reset Packet Context

		TInet6Packet<TIpHeader> ip(aPacket);
		TInt total, hlen;

		// RFC-3168: This a temporary copy of the TOS (IPv4) or
		// Traffic Class (IPv6) from the current IP header. 
		TInt tos;

		if (!ip.iHdr)
			break;				// Bad packet, just drop

		info->iOffsetIp = 0;	// First IP header is at start of the buffer.
		info->iIcmp = 0;
		info->iVersion = (TUint8)ip.iHdr->ip4.Version();
		if (info->iVersion == 4)
			{
			hlen = ip.iHdr->ip4.HeaderLength();
			if (hlen < TInet6HeaderIP4::MinHeaderLength())
				break;		// Corrupt IPv4 packet (header too short!)
			total = ip.iHdr->ip4.TotalLength();
			info->iPrevNextHdr = TInet6HeaderIP4::O_Protocol;
			tos = ip.iHdr->ip4.TOS();
			}
		else if (info->iVersion == 6)
			{
			hlen = ip.iHdr->ip6.HeaderLength();
			total = hlen + ip.iHdr->ip6.PayloadLength();
			info->iPrevNextHdr = TInet6HeaderIP::O_NextHeader;
			tos = ip.iHdr->ip6.TrafficClass();
			}
		else
			break;	// Only IPv4 and IPv6 are supported (probably corrupt packet)

		if (ip.iLength < hlen)
			break;	// The packet is not long enough for IPv4 or IPv6 header

		info->iOffset = hlen;

		const TInt extra = info->iLength - total;
		if (extra < 0)
			break;	// The packet is not long enough for the indicated payload
		else if (extra > 0)
			{
			info->iLength -= extra;
			aPacket.TrimEnd(info->iLength);	// (note, info->iLength also reset by TrimEnd!)
			}

		if (info->iFlags & KIpNoEcnEct || (tos & 3) == 0)
			{
			// ECN has been disabled for this packet/header. Clear out
			// congestion status (if set).
			ecnCongestion = EFalse;
			}
		else
			{
			// Current IP has ECN enabled.
			ecnCongestion |= (tos & 3) == 3;	// Set ecnCongestion if CE flag.
			// Proactively, set CE to the saved TOS/TrafficClass. It will only be
			// copied to the packet header, if ecnCongestion is also set.
			tos |= 3;
			}
		if (info->iVersion == 4)
			{
			if (TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)ip.iHdr, hlen)) != 0)
				break;		// Packet fails IPv4 header checksum
			if (ecnCongestion)
				{
				// Set ECN CE to inner header if ECN is enabled and
				// if outer header indicated congestion. Note: IP checksum
				// is not updated (The assumption is that the checksum is not
				// checked after this by anyone, and if the packet ends up
				// being forwarded, the checksum is recomputed there anyway).
				ip.iHdr->ip4.SetTOS(tos);
				}
			// Setup addresses and protocol into info before the
			// fragment assembly (if this is the first fragment, the
			// values will be the final values for the assembled
			// packet).
			TInetAddr::Cast(info->iSrcAddr).SetV4MappedAddress(ip.iHdr->ip4.SrcAddr());
			TInetAddr::Cast(info->iDstAddr).SetV4MappedAddress(ip.iHdr->ip4.DstAddr());
			info->iProtocol = ip.iHdr->ip4.Protocol();
			if (ip.iHdr->ip4.MF() || ip.iHdr->ip4.FragmentOffset())
				{
				if (iFragmentHeader->Ip4ApplyL(aPacket, *info, ip.iHdr->ip4) < 0)
					break;
				LOG(VerifyPacket(aPacket, *info));
				}
			}
		else
			{
			if (ecnCongestion)
				{
				// Set ECN CE to inner header if ECN is enabled and
				// if outer header indicated congestion.
				ip.iHdr->ip6.SetTrafficClass(tos);
				}
			TInetAddr::Cast(info->iSrcAddr).SetAddress(ip.iHdr->ip6.SrcAddr());
			TInetAddr::Cast(info->iSrcAddr).SetFlowLabel(ip.iHdr->ip6.FlowLabel());
			TInetAddr::Cast(info->iDstAddr).SetAddress(ip.iHdr->ip6.DstAddr());
			TInetAddr::Cast(info->iDstAddr).SetFlowLabel(0);
			info->iProtocol = ip.iHdr->ip6.NextHeader();
			}
		//
		// Dispatch the packet (returns TRUE, only if tunneled IP header is uncovered)
		//
		} while (DoSwitchL(aPacket));
	}

//
//	CProtocolIP::DoProcess
//	**********************
//	The same code is used for both IPv6 and IPv4 protocol instances and
//	this accepts both type of packets, regardless of the source (it accepts
//	IPv6 packets from IPv4 interface and vice versa).
//
void CProtocolIP::DoProcess()
	{
	ASSERT(this == iNetwork);

	RMBufHookPacket packet(this);
	LOG(Log::Printf(_L("--- Process Queued Packets")));
	while (iRecvQ.Remove(packet))
		{
		//
		// Process one IP packet
		//
		TRAPD(err, DoProcessOnePacketL(packet));
		err = err;  // Clearing "never used" warning caused by TRAPD
		//
		// Free leftovers, if any
		//
		LOG(if (!packet.IsEmpty()) Log::Printf(_L("\tPacket dropped by LEAVE %d"), err););
		packet.Free();
		}
	LOG(Log::Printf(_L("--- End Processing")));
	}

//
//	CProtocolIP::Process
//	*********************
//	The same code is used for both IPv6 and IPv4 protocol instances and
//	this accepts both type of packets, regardless of the source (it accepts
//	IPv6 packets from IPv4 interface and vice versa).
//
//	However, the hook lists are valid only for 'ip6' instance. After this,
//	all continued process occurs under the 'ip6' instance!
//
void CProtocolIP::Process(RMBufChain &aPacket, CProtocolBase* aInterface)
	{
	LOG(PktLog(_L("--- Packet from NIF[%u]%S: prot=%d src=%S dst=%S len=%d [calling prehooks]"), *RMBufPacketBase::PeekInfoInChain(aPacket), (TUint)aInterface, _L("")));
	// This byte count updating does not really belong to IP stack. It should
	// be done within NIF to be absolutely accurate... -- msa
	CNifIfBase *const nif = (CNifIfBase *)aInterface;
	if (nif && nif->Notify())
		{
		const RMBufRecvInfo *const info = RMBufRecvPacket::PeekInfoInChain(aPacket);
		if (info)
			(void)nif->Notify()->PacketActivity(EIncoming, (TUint)info->iLength, FALSE);
		}
	// Always shunt the remaining processing to the real ip6 instance.
	((CProtocolIP*)iNetwork)->iPostInbound.iHead->iHook->Process(aPacket, aInterface);
	}

//	CProtocolIP::PostProcess
//	************************
//
void CProtocolIP::PostProcess(RMBufChain &aPacket, CProtocolBase *aInterface)
	{
	ASSERT(this == iNetwork);

	RMBufRecvInfo *const info = RMBufRecvPacket::PeekInfoInChain(aPacket);

	const MInterface *const mi = iInterfacer->Interface((CNifIfBase *)aInterface);
	if (mi)
		{
		LOG(PktLog(_L("--- Packet after prehooks IF %u [%S] prot=%d src=%S dst=%S len=%d [Queued]"), *info, mi->Index(), mi->Name()));
		info->iInterfaceIndex = info->iOriginalIndex = mi->Index();
		// Insert the packet into a queue and request a callback. This is to
		// avoid extremely deep calling stacks. Up to this point, the processing
		// has been under the "RunL" of some NIF or driver. This queueing
		// "break" ensures that the actual IP processing of the packet starts
		// with a "fresh and direct" call from active scheduler.
		iRecvQ.Append(aPacket);
		iRecvQ.Wake();
		}
	else
		{
		LOG(PktLog(_L("--- Packet after prehooks Unknown NIF[%u]%S prot=%d src=%S dst=%S len=%d [Dropped]"), *info, (TUint)aInterface, _L("")));
		aPacket.Free(); // Unknown interface, drop silently!
		}
	}

//
// Recv callback handler. Called to kick off handling of
// any datagrams on our recv queue.
//
TInt CProtocolIP::RecvCallBack(TAny* aProtocol)
	{
	((CProtocolIP*)aProtocol)->DoProcess();
	return 0;
	}

//
//	CProtocolIP::Send
//	*****************
//	
TInt CProtocolIP::Send(RMBufChain& aPacket,CProtocolBase * /* aSrc */)
	{
	RMBufSendPacket packet;
	packet.Assign(aPacket);
	DoSendOnePacket(packet);
	return 1;
	}


//
//	************************
//	Internal ICMP processing
//	************************
//	The ICMP is an essential part of the IP stack and part of the ICMP processing
//	is mandatory. For this reason, the ICMP handling is implemented as a part of
//	the IP level implementation here.
//
//	Another reason is that if all of the ICMP is implemented as a separate
//	protocol module, it appears to create some complexities, because IP needs
//	ICMP and ICMP needs IP, thus the natural BIND directives would form a
//	loop. However, making a loop that way would prevent SocketServer from
//	terminating. The current solution is as follows:
//
//	-	Normal ICMP protocol is provided for applications, such as PING.
//		This is bound to the IP level normally. However, this protocol is
//		just a "gateway" for ICMP packets for applications.
//
//	-	Most of the mandatory ICMP functionality is implemented here as
//		a part of the IP protocol instance.
//
//
//	CProtocolIP::IcmpEcho
//	*********************
//	Generate ICMP echo reply to either IPv4 or IPv6 depending on aProtocol
//
void CProtocolIP::IcmpEcho(RMBufPacketBase &aPacket, RMBufRecvInfo *aInfo)
	{
	RMBufSendPacket packet;
	RMBufSendInfo *info;

	LOG(Log::Printf(_L("\tIcmpEcho(%d bytes)"), aInfo->iLength));
	packet.SetInfo((RMBufSendInfo *)aInfo);
	aPacket.SetInfo(NULL);
	packet.Assign(aPacket);
	info = packet.Info();

	//
	// Discard IP headers upto ICMP header
	//
	packet.TrimStart(aInfo->iOffset);
	//
	// The packet already has ICMP Echo Request header, so just map it
	//
	TInet6Checksum<TInet6HeaderICMP_Echo> echo(packet);
	if (echo.iHdr && !iIcmpThrottle.Suppress())
//	if (echo.iHdr)
		{
		// Swap src and dest addresses
		info->iSrcAddr.Swap(aInfo->iDstAddr);
		// If the request destination was not my own assigned address, then do
		// not use it as a source address of the reply. Select the default
		// address instead!
		if (!iInterfacer->LocalScope(TInetAddr::Cast(info->iSrcAddr).Ip6Address(), aInfo->iInterfaceIndex, EScopeType_IF))
			TInetAddr::Cast(info->iSrcAddr).SetAddress(KInet6AddrNone);		
		if (info->iProtocol == STATIC_CAST(TInt, KProtocolInetIcmp))
			echo.iHdr->SetType((TUint8)0);
		else
			echo.iHdr->SetType(KInet6ICMP_EchoReply);
		if (info->iFlow.Open(iNetwork, info->iProtocol) == KErrNone)
			{
			info->iFlow.SetRemoteAddr(info->iDstAddr);
			info->iFlow.SetLocalAddr(info->iSrcAddr);
			info->iFlow.SetIcmpType(echo.iHdr->Type());
			info->iFlow.FlowContext()->SetOption(KSolInetIp, KSoKeepInterfaceUp, KInetOptionDisable);
			info->iFlow.Connect();
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
			// Need to Handle NUT replaces the oldest ND entry with the new arrival packet, when queue overflows. 
			if ( (info->iFlow.Status() == KErrNone ) || 
				 ( (info->iFlow.Status() == EFlow_HOLD) && (info->iFlow.IsNdResolutionPending()) )//[RFC-4861] 
				)
#else
			    if (info->iFlow.Status() == KErrNone)		    
#endif // SYMBIAN_TCPIPDHCP_UPDATE	
	
				{
				// Successfully connected
				// Connection might have changed or redefined the src address. Must update
				// the info block with it.
				info->iSrcAddr = info->iFlow.FlowContext()->LocalAddr();
				info->iFlags = 0;
				// IPv4 ICMP checksum does not use "pseudoheader" => pass NULL for info when IPv4!
				echo.ComputeChecksum(packet, info->iProtocol == STATIC_CAST(TInt, KProtocolInetIcmp) ? NULL : info);
				packet.Pack();
				Send(packet);
				return;
				}
			info->iFlow.Close();
			}
		}
	packet.Free();
	}

//
//	CProtocolIP::IcmpHandlerL
//	*************************
//	A shared ICMP handler for IPv4 and IPv6. Even if the code clearly splits
//	into two totally different parts, it allows easy handling of ICMP6 within
//	IPv4 or ICMP4 within IPv6 (whether such bogosities are legal or not).
//
//	The caller must trap the leave and release the packet!
//
TInt CProtocolIP::IcmpHandlerL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo, CProtocolBase *aFinalTarget)
	{
	CHookEntry *h;
	CProtocolBase *target = NULL;

	TInet6Checksum<TInet6HeaderICMP> icmp(aPacket, aInfo.iOffset);
	if (icmp.iHdr == NULL)
		goto drop_packet;
	//
	// This assert should really be somehow in the inet.h, but for the time
	// being it is here... (e.g. Info blocks *MUST* fit into single RMBuf block!)
	// -- msa
	ASSERT(sizeof(RMBufRecvInfo) <= STATIC_CAST(TUint, KMBufSmallSize));


	if (!icmp.VerifyChecksum(aPacket, aInfo.iProtocol == STATIC_CAST(TInt, KProtocolInet6Icmp) ? &aInfo : NULL, aInfo.iOffset))
		goto drop_packet;
	//
	// Fill in the ICMP portion of the RMBufIcmpInfo
	// (Note, that iIcmp is still ZERO, just preloading the information for easy access)
	//
	aInfo.iType = icmp.iHdr->Type();
	aInfo.iCode = icmp.iHdr->Code();
	aInfo.iParameter = icmp.iHdr->Parameter();

	switch (ClassifyIcmp(aInfo.iProtocol, aInfo.iType))
		{
		case EIcmpType_ECHO:
			IcmpEcho(aPacket, &aInfo);
			return -1;
		case EIcmpType_ERROR:
			break;					// Fall to Error processing
		case EIcmpType_OTHER:
			return iInterfacer->IcmpHandler(aPacket, aInfo);
		default:
			goto drop_packet;		// ...weird...
		}

#if 1
	// The error processing is going to "eat" the ICMP error packet. If we have
	// some application reading ICMP socket, it would not see these packets. To
	// enable this feature, a separate copy must be explicitly made and passed.
	//
	if (aFinalTarget && aFinalTarget != iNullHook->iProtocol)
		{
		RMBufPacketBase copy;
		TRAPD(err, aPacket.CopyL(copy); aPacket.CopyInfoL(copy););
		if (err == KErrNone)
			{
			copy.Pack();
			aFinalTarget->Process(copy, this);
			}
		copy.Free();
		}	
#endif


	//
	//	ICMP Error dispatching loop
	//	---------------------------
	//	(first access the problem packet and setup the info block accordingly)
	//
	aInfo.iIcmp = (TUint8)aInfo.iProtocol;		// Start ICMP error buffer processing
	aInfo.iOffset += sizeof(TInet6HeaderICMP);	// ..points first octet after ICMP header (v4 or v6)
	aInfo.iOffsetIp = (TUint16)aInfo.iOffset;
	if (aInfo.iOffset > aInfo.iLength)
		goto drop_packet;

		{
		// Get interface (needed for scopes)
		const MInterface *const mi = iInterfacer->Interface(aInfo.iInterfaceIndex);
		if (mi == NULL)
			goto drop_packet;

		TInet6Packet<TIpHeader> ip(aPacket, aInfo.iOffsetIp);
		if (ip.iHdr == NULL)
			goto drop_packet;
		aInfo.iVersion = (TUint8)ip.iHdr->ip4.Version();

		TInetAddr &src = TInetAddr::Cast(aInfo.iSrcAddr);
		TInetAddr &dst = TInetAddr::Cast(aInfo.iDstAddr);
		switch (aInfo.iVersion)
			{
			case 6:
				//
				// Set Info addresses from the IPv6 header of the problem packet
				//
				if (ip.iLength < (TInt)sizeof(TInet6HeaderIP))
					goto drop_packet;		// Not useful, trunctated IP header.
				src.SetAddress(ip.iHdr->ip6.SrcAddr());
				src.SetFlowLabel(ip.iHdr->ip6.FlowLabel());
				dst.SetAddress(ip.iHdr->ip6.DstAddr());
				aInfo.iPrevNextHdr = (TUint16)(aInfo.iOffset + TInet6HeaderIP::O_NextHeader);
				aInfo.iOffset += sizeof(TInet6HeaderIP);
				aInfo.iProtocol = ip.iHdr->ip6.NextHeader();	// Always in range [0 .. 255] (TUint8)
				break;
			case 4:
				{
				// Need to do sanity check on IPv4 header length!
				const TInt hlen = ip.iHdr->ip4.HeaderLength();
				if (ip.iLength < hlen || hlen < TInet6HeaderIP4::MinHeaderLength())
					goto drop_packet;
				//
				// Set Info addresses from the IPv4 header of the problem packet
				//
				src.SetV4MappedAddress(ip.iHdr->ip4.SrcAddr());
				dst.SetV4MappedAddress(ip.iHdr->ip4.DstAddr());
				aInfo.iPrevNextHdr = (TUint16)(aInfo.iOffset + TInet6HeaderIP4::O_Protocol);
				aInfo.iOffset += hlen;
				aInfo.iProtocol = ip.iHdr->ip4.Protocol();		// Always in range [0 .. 255] (TUint8)
				}
				break;
			default:
				goto drop_packet;
			}
		// Complete scope id values
		src.SetScope(mi->Scope((TScopeType)(src.Ip6Address().Scope()-1)));
		dst.SetScope(mi->Scope((TScopeType)(dst.Ip6Address().Scope()-1)));
		}
	//
	// Let the interface manager have a peek at the packet
	//
	if (iInterfacer->IcmpError(aPacket, aInfo) < 0)
		return -1;		// Packet was owned/dropped in iInterfacer method!

again:
	for (h = iSwitch[aInfo.iProtocol].iHead;;)
		{
		if (h->IsProtocol())
			{
			if (target)
				{
				if (target == this)
						{
						// A temporary solution for "self-detunnel", which
						// is not prepared to get "iOffset" in the info
						// structure (cannot trust it, because normal Process
						// calls come from the drivers). Need propably do a
						// separate detunneling pseudoprotocol, even though
						// linking self is a "cute trick".. -- msa
						break;		// CANNOT HANDLE THIS IN ANY WAY, JUST DROP!!
						}
				if (aInfo.iLength > aInfo.iOffset)	// Trust iLength properly maintained!!
					{
					aPacket.Pack();
					target->Process(aPacket);
					return -1;
					}
				else
					break;				// All of the packet processed, just drop.
				}
			//
			// Target protocol found (Phase 1 done)
			//
			target = h->iProtocol;	// Always != NULL!!
			//
			// Now, process all generic hook(s), if any registered
			//
			h = iSwitch[KIp6Hook_ANY].iHead;
			}
		else
			{
			//
			// *NOTE*
			//		The error ApplyL method is not supposed to touch the info (iLength),
			//		It only must update the head iOffset field!!! -- msa
			//
			const TInt ret = h->iHook->ApplyL(aPacket, aInfo);
			if (ret < 0)
				{
				ASSERT(aPacket.IsEmpty());
				return -1;	// The hook consumed the packet.
				}
			else
				{
				LOG(VerifyPacket(aPacket, aInfo));// Hook Sanity checks!
				if (ret == KIp6Hook_DONE)
					{
					if (aInfo.iProtocol != 0)
						goto again;			// The hook changed Next header (restart)
					else
						goto drop_packet;	// Hop-by-hop is only valid after IP header!
					}
				}
			h = h->iNext;
			}
		}
	// Plain exit from the above loop is "drop packet" (consume)
drop_packet:
	LOG(Log::Printf(_L("\t%S IcmpHandler: packet dropped"), &ProtocolName()));
	aPacket.Free();
	return -1;
	}

void CProtocolIP::IcmpSend(TInt aProtocol, RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC)
	{
	RMBufRecvInfo *info = aPacket.Info();
	RMBufSendPacket packet;
	RMBufSendInfo *snd = NULL;

	for (;;)
		{
		if (info == NULL || info->iIcmp || (info->iFlags & KIpNeverIcmpError) != 0)
			break;
		if (iIcmpThrottle.Suppress())
			break;		// Drop! Too many sent

		const TIpHeader *const ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
		if (!ip)
			break;

		TInetAddr src, dst;
		switch (ip->ip4.Version())
			{
		case 4:
			src.SetV4MappedAddress(ip->ip4.DstAddr());
			dst.SetV4MappedAddress(ip->ip4.SrcAddr());
			break;
		case 6:
			src.SetAddress(ip->ip6.DstAddr());
			dst.SetAddress(ip->ip6.SrcAddr());
			break;
		default:
			goto cleanup;
			}

		dst.SetScope(iInterfacer->RemoteScope(dst.Ip6Address(), info->iInterfaceIndex, EScopeType_IF));
		// Verify and adjust addresses
		// If the packet was addressed directly to me, use the same address
		// as a source in the complaint.
		const TUint32 src_id = iInterfacer->LocalScope(src.Ip6Address(), info->iInterfaceIndex, EScopeType_IF);
		if (src_id)
			{
			// The packet was addressed directly to me, use the same address
			// as a source in the complaint.
			//
			// However, the src address scope >= dst address scope
			//
			if (dst.Ip6Address().Scope() < src.Ip6Address().Scope())
				break;	// Do not send ICMP Error
			if ((info->iFlags & KIpBroadcastOnLink) != 0 && !aMC)
				break;	// Was broadcast on link, and replies to multicast not allowed.
			src.SetScope(src_id);
			}
		else if (aMC || (src.IsUnicast() &&
						 (info->iFlags & KIpBroadcastOnLink) == 0 &&
						 !iInterfacer->IsForMeAddress(src.Ip6Address(), info->iInterfaceIndex)))
			{
			// This must be some type of forwarding or multicast destination
			// instead, let the flow open decide on it. Note, the caller of
			// this method is responsible for allowing (aMC != 0) generating
			// ICMP's for multicast destinations! (In some cases sending
			// replies to multicast is required by RFC's, can't prevent it
			// at this level! -- msa)
			src.SetAddress(KInet6AddrNone);
			src.SetScope(info->iInterfaceIndex);
			}
		else
			break;

		TRAPD(err, snd = packet.CreateL(8));
		if (err != KErrNone || snd == NULL)
			break;

		// Create unconnected flow context to the packet (info)
		if (snd->iFlow.Open(this, aProtocol) != KErrNone)
			break;
		// Setup the connection information and connect
		snd->iFlow.SetRemoteAddr(dst);
		snd->iFlow.SetLocalAddr(src);
		snd->iFlow.SetIcmpType(aType, aCode);
		//
		// Supply the original packet as a flow parameter
		// (transfer the aPacket as is into the flow context)
		CFlowContext *const flow = snd->iFlow.FlowContext();
		aPacket.SetInfo(NULL);
		flow->iHead.iIcmp.Assign(aPacket);
		flow->iHead.iIcmp.SetInfo(info);
		// Disable "keep interface up" for ICMP error replies
		flow->SetOption(KSolInetIp, KSoKeepInterfaceUp, KInetOptionDisable);
		//
		snd->iFlow.Connect();
		if (snd->iFlow.Status() != KErrNone)
			break;		// Cannot get a flow opened..
		if (flow->iHead.iIcmp.IsEmpty() ||
			(info = (RMBufRecvInfo *)flow->iHead.iIcmp.Info()) == NULL)
			break;		// Oops, apparently some hook ate the packet/info
		ASSERT(info->iLength == flow->iHead.iIcmp.Length());

		const TIpHeader *const ip2 = ((RMBufPacketPeek &)flow->iHead.iIcmp).GetIpHeader();
		if (!ip2)
			break;
		//
		// Trim the returned packet for the ICMP error payload
		//
		switch (ip2->ip4.Version())
			{
		case 4:
			// For IPv4, ICMP error payload includes the original IP header
			// and at most 8 bytes following it.
			if (info->iLength > ip2->ip4.HeaderLength() + 8)
				flow->iHead.iIcmp.TrimEnd(ip2->ip4.HeaderLength() + 8);
				// TrimEnd updates info->iLength!
			break;
		case 6:
				{
				// For IPv6, ICMP error payload inludes as much of the original
				// packet as can be fit into minimum MTU for IPv6
				const TInt available = KInet6MinMtu - flow->HeaderSize() - snd->iLength;
				if (available < 0)
					goto cleanup; // flow headers take too much space! (bad hooks?)
				else if (available < info->iLength)
					flow->iHead.iIcmp.TrimEnd(available);
					// TrimEnd updates info->iLength!!
				}
			break;
		default:
			goto cleanup;
			}
		// Append original packet to the error report
		packet.Append(flow->iHead.iIcmp);
		snd->iLength += info->iLength;
		flow->iHead.iIcmp.Free();

		snd->iSrcAddr = flow->LocalAddr();
		snd->iDstAddr = flow->RemoteAddr();
		snd->iProtocol = aProtocol;

		TInet6Checksum<TInet6HeaderICMP> icmp(packet);
		if (icmp.iHdr == NULL)
			break;

		icmp.iHdr->SetType((TUint8)aType);
		icmp.iHdr->SetCode((TUint8)aCode);
		icmp.iHdr->SetParameter(aParameter);
		icmp.ComputeChecksum(packet, aProtocol == STATIC_CAST(TUint, KProtocolInetIcmp) ? NULL : snd);
		packet.Pack();
		Send(packet);
		return;
		}
cleanup:
	// Release resources that didn't get passed on
	aPacket.Free();
	if (snd != NULL)
		snd->iFlow.Close();
	packet.Free();
	}

//
//	CProtocolIP::Icmp4Send
//	**********************
//	Build IPv4 ICMP message from the received packet.
//
//	The packet is assumed to be in *UNPACKED STATE* (Info block separated!)
//
void CProtocolIP::Icmp4Send(RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC)
	{
	IcmpSend(KProtocolInetIcmp, aPacket, aType, aCode, aParameter, aMC);
	}
//
//	CProtocolIP::Icmp6Send
//	**********************
//	Build IPv6 ICMP message from the received packet.
//
//	The packet is assumed to be in *UNPACKED STATE* (Info block separated!)
//
void CProtocolIP::Icmp6Send(RMBufRecvPacket &aPacket, TInt aType, TInt aCode, TUint32 aParameter, TInt aMC)
	{
	IcmpSend(KProtocolInet6Icmp, aPacket, aType, aCode, aParameter, aMC);
	}

// CProtocolIP::IcmpWrap
// *********************
/**
// Wrap a packet into ICMP error reply and send it out.
//
// The aPacket is assumed to hold a complete IP packet (starting from the beginning)
// in a packed state. The info block is initialized by this method (the current
// content is ignored).
//
// Determine the IP version from the IP header in the packet and choose the ICMP
// type and code accordingly from aIcmp, and then call the actual ICMP sending
// code (either Icmp4Send or Icmp6Send).
//
// On any error or problems, the packet is simply released, and no ICMP message
// will occur.
//
// @param aPacket
//	The RMBuf chain containing the IP packet in packet state
// @param aIcmp
//	The 32 bit value containing type and code for both IPv4 and IPv6. The type and
//	code to be used are chosen based on the actual IP version of the packet.
// @param aParameter
//	The 32 bit value to be placed as the "parameter" field of the ICMP header.
// @param aMC
//	A flag, when non-Zero, forces sending of ICMP, even if the packet destination
//	was a multicast address (see MNetworkService::Icmp4Send and
//	MNetworkService::Icmp6Send).
*/
void CProtocolIP::IcmpWrap(RMBufChain &aPacket, const TIcmpTypeCode aIcmp, const TUint32 aParameter, const TInt aMC)
	{
	if (aPacket.IsEmpty())
		return;	// nothing to do with empty packet!

	// Use "info" as receive info, because this is what the
	// actual ICMP sending code (Icmp4Send/Icmp6Send) expects.
	RMBufRecvPacket packet;
	packet.Assign(aPacket);
	packet.Unpack();
	RMBufRecvInfo *const info = packet.Info();

	if (info)
		{
		// Setup "fake" RMBufRecvInfo (just enough to pass
		// Icmp4Send/Icmp6Send inspections...)
		info->iIcmp = 0;	// Prevent it being from dropped
		info->iOffset = 0;	// not used?
		info->iFlags = 0;
		//
		// *note*
		// Icmp4Send/Icmp6Send are designed to return incoming packet
		// and they use iInterfaceIndex to return the error message
		// to the original interface. However, this method is mostly
		// used for outgoing packets from this host (source address
		// is usually my own address), there is no "originating
		// interface". Use ZERO, and assume ICMP sending methods
		// handle this correctly.
		// 
		// When forwarding is enabled, the packet could actually have
		// come from an other interface. Unfortunately, in current
		// version, the informatio about originating interface has
		// beeen lost at this point. It would be more correct to
		// force the ICMP error back to original interface instead
		// of relying on source address and routing to choose it.
		// [needs to be examined later -- msa]
		info->iInterfaceIndex = 0;

		const TIpHeader *const ip = ((RMBufPacketPeek &)packet).GetIpHeader();
		if (ip)
			{
			if (ip->ip4.Version() == 4)
				Icmp4Send(packet, aIcmp.i4type, aIcmp.i4code, aParameter, aMC);
			else if (ip->ip6.Version() == 6)
				Icmp6Send(packet, aIcmp.i6type, aIcmp.i6code, aParameter, aMC);
			}
		}
	//
	// Release packet (if not passed on)
	//
	packet.Free();
	return;
	}


//
//	**********************
//	Flow Network Interface
//	**********************
//
//	CProtocolIP::NewFlowL
//	**********************
//
CFlowContext *CProtocolIP::NewFlowL(const void *aOwner, TUint aProtocol)
	{
 	if (iNetwork != this && iNetwork)	// There are two instances, etc...
		return iNetwork->NewFlowL(aOwner, aProtocol);
	return iInterfacer->NewFlowL(aOwner, this, aProtocol);	// <-- either leaves or succeeds!
	}

CFlowContext *CProtocolIP::NewFlowL(const void *aOwner, CFlowContext &aFlow)
	{
	//
	// Assume the 'aFlow' provides all the necessary non-zero defaults
	// and no other inits are required.
	//
 	if (iNetwork != this && iNetwork)	// There are two instances, etc...
		return iNetwork->NewFlowL(aOwner, aFlow);
	return iInterfacer->NewFlowL(aOwner, this, aFlow);	// <-- either leaves or succeeds!
	}

//	CProtocolIP::SetChanged
//	***********************
//
TInt CProtocolIP::SetChanged() const
	{
	return iInterfacer->SetChanged();
	}
//
//	CProtocolIP::FlowSetupHooks
//	**************************
//	(common for both IPv6 and IPv4)
//
TInt CProtocolIP::FlowSetupHooks(CFlowInternalContext &aFlow)
	{
 	if (iNetwork != this && iNetwork)	// There are two instances, etc...
		return iNetwork->FlowSetupHooks(aFlow);

	//
	// Attach registered hooks
	//	For each registered outbound hook, ask if it wants to attach to
	//	the current flow or not. If it wants, add a flow hook entry.
	//
	for (CHookEntry *h = iOutbound.iHead; h != NULL; h = h->iNext)
		{
		MFlowHook *hook = NULL;

		const TIp6Addr src = aFlow.Head().ip6.SrcAddr();
		const TUint32 src_id = aFlow.Head().iSrcId;
		const TIp6Addr dst = aFlow.Head().ip6.DstAddr();
		const TUint32 dst_id = aFlow.Head().iDstId;
		const TUint set = aFlow.Head().iSourceSet;
		TInt frag = aFlow.Head().iFragment ? -1 : aFlow.HeaderSize();
#ifdef _LOG
		TServerProtocolDesc info;
		h->iHook->Identify(&info);
#endif
		TRAPD(ret, hook = h->iHook->OpenL(aFlow.Head(), &aFlow););
		if (ret == KErrNone)
			{
			if (frag < 0)
				{
				// Fragging has already been requested before, just make
				// sure the iFragment stays set (hook cannot clear it!)
				ASSERT(aFlow.Head().iFragment == 1);
				aFlow.Head().iFragment = 1;
				}
			else if (aFlow.Head().iFragment == 0)
				// This hook is not requesting fragmentation.
				frag = -1;

			// frag is non-negative only if this hook requested the
			// fragmentaion, and is the first one to do so.
			if (hook)
				{

				LOG(Log::Printf(_L("\t\tFlow[%u] %S attaching MFlowHook[%u] frag=%d"), &aFlow, &info.iName, hook, frag));
				TRAP(ret, aFlow.AddHookL(hook, frag));
				hook->Close();		// Cancel my OpenL reference
				if (ret == KErrNone)
					{
					if (src.IsEqual(aFlow.Head().ip6.SrcAddr()) &&
						src_id == aFlow.Head().iSrcId &&
						dst.IsEqual(aFlow.Head().ip6.DstAddr()) &&
						dst_id == aFlow.Head().iDstId &&
						set == aFlow.Head().iSourceSet)
						continue;	// Unchanged routing info
					if ((ret = aFlow.RouteFlow(aFlow.Head())) == KErrNone)
						continue;
					}
				}
			else if (frag < 0)
				continue;		// Not interested!
			else
				{
				// Hook requested fragmentation, but didn't attach to the flow. This
				// is not allowed!
				ret = KErrGeneral;
				}
			}
		//
		// Error condition, Abort Open/Connect sequence
		//
		aFlow.RemoveHooks();
		LOG(Log::Printf(_L("\t\tFLow[%u] OpenL aborted by %S with reason=%d"), &aFlow, &info.iName, ret));
		return ret;
		}

	return KErrNone;
	}


void CProtocolIP::FlowStartRefresh(CFlowInternalContext &aFlow)
	{
	if (aFlow.iHead.ip6.Version() == 4)
		aFlow.iHdrSize += TInet6HeaderIP4::MinHeaderLength();
	else
		aFlow.iHdrSize += TInet6HeaderIP::MinHeaderLength();
	}

TInt CProtocolIP::GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, const CFlowContext &aFlow) const
	{
 	if ((const MNetworkService*)iNetwork != this && iNetwork)	// There are two instances, etc...
		return iNetwork->GetFlowOption(aLevel, aName, aOption, aFlow);

	//
	// Go through the output hooks and try to Get the options
	//
	TInt ret = KErrNotSupported;
	for (CHookEntry *h = iOutbound.iHead; h != NULL; h = h->iNext) {
		if ((ret = h->iHook->GetFlowOption(aLevel, aName, aOption, aFlow)) != KErrNotSupported)
			break;
	}
	return ret;
	}

TInt CProtocolIP::SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow)
	{
 	if (iNetwork != this && iNetwork)	// There are two instances, etc...
		return iNetwork->SetFlowOption(aLevel, aName, aOption, aFlow);
	//
	// Go through the output hooks and try to Set the options
	//
	TInt ret = KErrNotSupported;
	for (CHookEntry *h = iOutbound.iHead; h != NULL; h = h->iNext) {	
		if ((ret = h->iHook->SetFlowOption(aLevel, aName, aOption, aFlow)) != KErrNotSupported)
			break;
	}
	return ret;
	}
