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
// ip6_frag.cpp - hook for IPv6 fragment header
//

#include "frag.h"
#include <icmp6_hdr.h>	// for ICMP Type symbols
#include <ext_hdr.h>	// for IPv6 fragment header
#include "ip6_frag.h"
#include <in_pkt.h>
#include <in_chk.h>
#include <timeout.h>
#include "tcpip_ini.h"
#include "inet6log.h"

//	TIp6FragmentHeader
//	******************
//	This header is at the beginning of each fragment in the
//	fragment queue. This attempts to be the same size as
//	the real IPv6 fragment header (8 octets), but the code
//	should work with any size of this [use of this specific
//	size avoids some TrimStart() calls in some cases.]
//
class TIp6FragmentHeader
	{
public:
	TUint iOffset;		// Offset of this fragment (bytes)
	TUint iLength;		// Lenght of this fragment (bytes)
	};

//
//	RIp6Fragment
//	************
//	The IP6 fragment data structure is an RMBufChain containing
//	- TIp6FragmentHeader
//	- followed by fragment content
//	This is the interface to the RMBufFragQ class, a collection
//	of required methods for accessing the fragment information and
//	a join operation.
//
class RIp6Fragment : public RMBufFrag
	{
public:
	// Internal utility to the other methods. Assumes that the fragment
	// start is properly aligned to be cast into class pointer.
	inline TIp6FragmentHeader *Header() const { return (TIp6FragmentHeader *)(First()->Ptr()); }
	// Return offset of the fragment (bytes)
	TUint Offset() const {return Header()->iOffset;}
	// Return length of the fragment (bytes)
	// (Does not include the fragment header, using
	// this->Length() will give larger value that
	// includes the header)
	TUint FragmentLength() const {return Header()->iLength;}
	// Join another fragment to this one
	void Join(RIp6Fragment& aFrag)
		{
		TInt overlap = Offset() + FragmentLength() - aFrag.Offset();
		if (overlap < 0)
			{
			aFrag.Free();	// Should panic? This should not happen!
			User::Panic(_L("DEBUG"), 0);
			}
		else
			{
			Header()->iLength += aFrag.FragmentLength() - overlap;
			aFrag.TrimStart(sizeof(TIp6FragmentHeader) + overlap);
			Append(aFrag);
			}
		}
	};


//
//	CFragmentHandler
//	****************
//
class CAssembly;
class CFragmentHandler : public CFragmentHeaderHook
	{
public:
	CFragmentHandler(MNetworkService *aNetwork) : CFragmentHeaderHook(aNetwork) {}
	~CFragmentHandler();
	inline MNetworkService *Network() { return iNetwork; }
	CAssembly *LookupL(const RMBufRecvInfo &aInfo, TUint32 aId, TUint aVersion);

	void ConstructL();
	void Cancel(CAssembly *aAssembly);
	void CancelAll();
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
	void Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ);

	MTimeoutManager *iTimeoutManager;
	CAssembly *iCache;
	//
	TInt iCount;	// Current incomplete assemblies
	TInt iMaxCount;	// Maximum number of assemblies
	TInt iTotal;	// Total amount of RMBuf space allocated to assemblies (bytes)
	TInt iMaxTotal;	// Maximum allowed amount of RMBuf space
private:
	TInt Ip6ApplyL(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, const TInet6HeaderFragment &aHdr);
	TInt Ip4ApplyL(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, const TInet6HeaderIP4 &aHdr);
	void Ip6Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ, TInet6HeaderIP &aHdr);
	void Ip4Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ, TInet6HeaderIP4 &aHdr);

	//
	// iFagmentId is only used for IPv6. For IPv4, the Id is already in
	// in each IP packet.
	//
	TInt iFragmentId;
	};

//
//	CAssembly
//	*********
//	The data structure for a partially assembled packet
//
class CAssembly : public CBase
	{
	friend class CFragmentHandler;
	friend class CFragmentLinkage;

	CAssembly(CFragmentHandler &aHandler, const TIp6Addr &aSrc, const TIp6Addr &aDst, TInt aId, TInt aVersion);
	~CAssembly();

	TInt AddFragmentL(RMBufRecvPacket &aPacket, TInt aTrim, TInt aLength, TInt aOffset, TUint32 aFh0);
	TInt Add(RMBufRecvPacket &aPacket, TInt aTrim, TInt aLength, TInt aOffset, TUint32 aFh0 = 0);
	TInt CompletePacket(TInt aRestoreFH = 0);
	void Timeout();
public:
	//
	// Linkage and management
	//
	CFragmentHandler &iHandler;
	CAssembly *iNext;		// Linkage of the assemblies in the cache

	RTimeout iTimeout;		// Timer hook
#ifndef _LOG
protected:
#endif
	const TUint8 iVersion;	// Need to have, because TAHI wants FH in IPv6 TimeExceeded ICMP
	TUint iIsDead:1;		// =1, if this assembly is "dead" (matching fragments are dropped)
	const TUint32 iId;		// Fragment identification
	const TIp6Addr iSrcAddr;
	const TIp6Addr iDstAddr;

	TUint32 iFH0;			// Saved first 32 bits of the first FH (for TAHI)

	TInt32 iLength;			// Total length of the fragmentable part, when
							// known (e.g. last fragment received).
							// [As 32 bit int is used, there shouuld be no
							// overflow or wrap around problems because
							// offset field is only 16 bits (in bytes)]
	TInt iTotal;			// The amount of "iTotal" from this assembly.
	//
	// The unfragmentable part of the first fragment (if received)
	//
	RMBufRecvPacket iHead;
	RMBufFragQ<RIp6Fragment> iQueue;
	};

//
//	CFragmentLinkage
//	****************
//	Glue to bind timeout callback from the timeout manager into Timeout() call
//	on the CAssembly.
//
//	*NOTE*
//		This kludgery is all static and compile time, and only used in the constructor
//		of CFragmentAssembly following this.
//

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KAssemblyTimeoutOffset 12
__ASSERT_COMPILE(KAssemblyTimeoutOffset == _FOFF(CAssembly, iTimeout));
#else
#define KAssemblyTimeoutOffset _FOFF(CAssembly, iTimeout)
#endif

class CFragmentLinkage : public TimeoutLinkage<CAssembly, KAssemblyTimeoutOffset>
	{
public:
	static void Timeout(RTimeout &aLink, const TTime & /*aNow*/, TAny * /*aPtr*/)
		{
		Object(aLink)->Timeout();
		}
	};

#ifdef _LOG
//
//	LogPrintId
//	**********
//	Purely for LOG output, should not be compiled in the final release
//
static void LogPrintId(const TDesC &aText, const CAssembly &a)
	{
	TInetAddr src, dst;
	TBuf<70> sbuf;
	TBuf<70> dbuf;

	src.SetAddress(a.iSrcAddr);
	dst.SetAddress(a.iDstAddr);

	src.OutputWithScope(sbuf);
	dst.OutputWithScope(dbuf);
	Log::Printf(_L("%S [src=%S dst=%S id=%d] (%d)"), &aText, &sbuf, &dbuf, a.iId, a.iHandler.iCount);
	}
#endif

//
//	CFragmentHandler::Cancel
//	************************
//	Cancel a specific assembly (delete it)
//
void CFragmentHandler::Cancel(CAssembly *aAssembly)
	{
	for (CAssembly **h = &iCache; *h != NULL; h = &(*h)->iNext)
		if (aAssembly == *h)
			{
			*h = aAssembly->iNext;
			delete aAssembly;
			break;
			}
	// Should panic, if a was not found from the list! -- msa
	}

//
//	CFragmentHandler::CancelAll
//	***************************
//	Cancel all packets waiting for assembly (delete all)
//
void CFragmentHandler::CancelAll()
	{
	CAssembly *a;
	while ((a = iCache) != NULL)
		{
		iCache = a->iNext;
		delete a;
		}
	}

//
//	ApplyL
//	******
//	Called when an IP packet contain IPv6 fragment header.
//	(does not care whether outer IP header is v4 or v6, both work!)
//
TInt CFragmentHandler::ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo)
	{
	for (;;)	// *NOT REAL LOOP, JUST A CONSTRUCT TO ENABLE USE OF 'break!
		{
		if (aInfo.iProtocol != STATIC_CAST(TInt, KProtocolInet6Fragment))
			break;	// Incorrect call, should only get IPv6 fragments here!
		TInet6Packet<TInet6HeaderFragment> fh(aPacket, aInfo.iOffset);
		if (fh.iHdr == NULL)
			break;	// Drop! (packet too short)

		if (aInfo.iIcmp == 0)
			return Ip6ApplyL(aPacket, aInfo, *fh.iHdr);

		if (aInfo.iIcmp != KProtocolInet6Icmp)
			break;	// Only IPv6 complaints should get this far, drop!

		const TInt offset = aInfo.iOffset - aInfo.iOffsetIp;	// Relative offset within problem packet
		if (aInfo.iType == KInet6ICMP_ParameterProblem &&	// A parameter problem...
			offset <= (TInt)aInfo.iParameter &&				// after start of this header?
			STATIC_CAST(TUint, offset + sizeof(TInet6HeaderFragment)) > STATIC_CAST(TUint, aInfo.iParameter))		// and before end of this header?
				break;		// Drop! (someone doesn't like my fragments!)
		//
		// Error is not Fragment Header specific, pass it on in the chain
		// Skip over header, pass error processing to the next header
		aInfo.iPrevNextHdr = (TUint16)aInfo.iOffset;	// Fragment next header is at +0
		aInfo.iProtocol = fh.iHdr->NextHeader();
		aInfo.iOffset += sizeof(TInet6HeaderFragment);
		return KIp6Hook_DONE;
		// WAS NOT LOOP, BE SURE TO EXIT IT ALWAYS!
		}
	aPacket.Free();
	return -1;
	}

//
//	Fragment
//	********
//	Called when an outgoing IP packet needs to be fragmented
//
void CFragmentHandler::Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ)
	{
	//
	// Decide on which fragmenting from the IP header.
	//
	TIpHeader *ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
	if (ip)
		if (ip->ip4.Version() == 4)
			Ip4Fragment(aPacket, aMtu, aFragQ, ip->ip4);
		else if (ip->ip4.Version() == 6)
			Ip6Fragment(aPacket, aMtu, aFragQ, ip->ip6);
	}


//	CAssembly
//	*********
//
CAssembly::CAssembly(CFragmentHandler &aHandler, const TIp6Addr &aSrc, const TIp6Addr &aDst, TInt aId, TInt aVersion)
 : iHandler(aHandler), iTimeout(CFragmentLinkage::Timeout), iVersion((TUint8)aVersion), iId(aId),
	iSrcAddr(aSrc), iDstAddr(aDst), iLength(65536)
	{
	}

//	CAssembly::~CAssebly()
//	**********************
//
CAssembly::~CAssembly()
	{
	iHandler.iTotal -= iTotal;
	--iHandler.iCount;

	LOG(LogPrintId(_L("CAssebly::~CAssebly():"), *this));

	iTimeout.Cancel();
	iQueue.Free();
	iHead.Free();
	}


LOCAL_C TInt CompareStructures()
	{
	return sizeof(TIp6FragmentHeader) - sizeof(TInet6HeaderFragment);
	}
//
//	CAssembly::CompletePacket
//	*************************
//	Merge the iHead and first fragment into a complete packet
//	(including the IPv4/IPv6 header stuff!)
//
//	Returns,
//	== KErrNone, if packet in iHead built
//	!= KErrNone, if no packet available
//
//	*NOTE*
//		this is also used when reassembly timeout hits, thus the
//		"complete" can also be an incomplete packet (including
//		only the first fragment).
//
TInt CAssembly::CompletePacket(TInt aRestoreFH)
	{
	if (iHead.IsEmpty())
		return -1;

	RMBufRecvInfo *const info = iHead.Info();

	RIp6Fragment first;
	iQueue.Remove(first);

	TInt length = first.Header()->iLength;
	ASSERT(first.Header()->iOffset == 0);

	if (aRestoreFH)
		{
		// *********************************************************
		// TAHI tester wants the returned ICMP to include the FH on
		// time exceeded message. This is for that...
		// *********************************************************
		const TInt adjust = CompareStructures();
		// As the value of adjust is known by compile time, compiler should eliminate
		// unnecessary code below (although, it may give a warnings about constants
		// being compared--this is indication that code elimination should work!)
		if (adjust > 0)
			first.TrimStart(adjust);
		else if (adjust < 0)
			{
			TInt err = first.Prepend(-adjust);
			if (err!=KErrNone)
				{
				first.Free();
				return -1;
				}
			}
		TInet6HeaderFragment fake_fh;
		*(TUint32 *)&fake_fh = iFH0;	// Saved 1st word of original FH
		fake_fh.SetId(iId);				// ..and this SetId cover all of FH
										// (there is no need to zero anything else)
		first.CopyIn(TPtrC8((TUint8 *)&fake_fh, sizeof(TInet6HeaderFragment)), 0);
		// .. ugh, KProtocolInet6Fragment is TInt, casting to TUint8* might
		// cause "endian problems", thus this copy to true TUint8 variable... -- msa
		const TUint8 proto = KProtocolInet6Fragment;
		iHead.CopyIn(TPtrC8((TUint8 *)&proto, 1), info->iPrevNextHdr);
		length += sizeof(TInet6HeaderFragment);
		iLength = length;
		}
	else
		first.TrimStart(sizeof(TIp6FragmentHeader));
	if (length > iLength)
		{
		//
		// Just to deal with some overlapping fragment, which extends
		// beyond the last fragment... (iLength is initialized to
		// 65536 and is only less, when last fragment has arrived!)
		length = iLength;
		first.TrimEnd(length);
		}

	iHead.Append(first);
	//
	// Fix info and IP header
	//
	//
	// Only the iLength needs to be updated, all other fields in the info
	// must already have the correct values!
	//
	info->iLength = info->iOffset + length;

	TInet6Packet<TIpHeader> ip(iHead, info->iOffsetIp);
	if (ip.iHdr)
		{
		TInt ver = ip.iHdr->ip4.Version();
		if (ver == 4)
			{
			if (ip.iLength >= ip.iHdr->ip4.HeaderLength() &&
				info->iLength < 65536)
				{
				// ..just to be tidy, make sure M=0 (nobody really cares) -- msa
				ip.iHdr->ip4.SetFlags((TUint8)(ip.iHdr->ip4.Flags() & ~KInet4IP_MF));
				ip.iHdr->ip4.SetTotalLength(info->iLength);
				return KErrNone;
				}
			}
		else if (ver == 6)
			{
			if (ip.iLength >= TInet6HeaderIP::MinHeaderLength() &&
				  STATIC_CAST(TUint, info->iLength) < STATIC_CAST(TUint, 65536 + sizeof(TInet6HeaderIP)))
				{
				ip.iHdr->ip6.SetPayloadLength(info->iLength - sizeof(TInet6HeaderIP));
				return KErrNone;
				}
			}
		}
	return -1;
	}

//
//	CAssembly::Add
//	**********************
//
TInt CAssembly::Add(RMBufRecvPacket &aPacket, TInt aTrim, TInt aOffset, TInt aLength, TUint32 aFH0)
	{
		if (iIsDead == 0)
			{
			TInt ret = 0; // [ = 0, only to silence compiler]
			TRAPD(err, ret = AddFragmentL(aPacket, aTrim, aOffset, aLength, aFH0));
			if (err == KErrNone)
				return ret;
			//
			// AddFragmentL left, change the Assembly into "dead" state
			// (and make sure all unnecessary resources are released)
			//
			iIsDead = 1;
			iHead.Free();
			iQueue.Free();
			iHandler.iTotal -= iTotal;
			iTotal = 0;
			}
		ASSERT(iTotal == 0);
		//
		// The hook return code = -1 (=> packet handled and released)
		//
		aPacket.Free();
		return -1;
	}


//	CAssembly::AddFragmentL
//	***********************
//  This should only be called from Add, which must trap the leaves and 
//
//	returns
//		standard inbound hook return codes
//	leaves
//		when assembly should be declared "dead" (seed "Add")
//
TInt CAssembly::AddFragmentL(RMBufRecvPacket &aPacket, TInt aTrim, TInt aOffset, TInt aLength, TUint32 aFH0)
	{
	RMBufRecvInfo *info = aPacket.Info();
	if (aOffset == 0)
		{
		//
		// Processing the first fragment (offset == 0!)
		//

		if (!iHead.IsEmpty())
			aTrim += info->iOffset;
		else
			{
			const TInt unfrag = info->iOffset;	// = Unfragmentable length (bytes)
			iTotal += unfrag;
			iHandler.iTotal += unfrag;

			iHead.Assign(aPacket);
			iHead.SplitL(unfrag, aPacket);
			iHead.SetInfo(info);
			aPacket.SetInfo(NULL);
			//
			// Patch in the next header field
			//
			iHead.CopyIn(TPtrC8((TUint8 *)&info->iProtocol, 1), info->iPrevNextHdr);
			//
			// This fragment is assigned as the first fragment. Save the FH0 for it.
			// (if there are multiple "first fragments", the first received is used)
			// (Only needed for IPv6 and to generate TAHI-compatible ICMP Time Exceeded)
			// *NOTE* The full first word is saved because, the reserved bits must be
			// kept also, if someone is using them -- msa
			iFH0 = aFH0;
			}
		}
	else
		aTrim += info->iOffset;
	//
	// Convert the header in the packet (if present) into
	// internal fragment header
	//
	aTrim -= sizeof(TIp6FragmentHeader);
	if (aTrim > 0)
		// Cast needed to avoid TrimStart() looking for info block!!!
		((RMBufChain &)aPacket).TrimStart(aTrim);
	else if (aTrim < 0)
		aPacket.PrependL(-aTrim);
	// Make sure the fragment header is properly aligned in the First() buffer.
	aPacket.Align(sizeof(TIp6FragmentHeader));
	TIp6FragmentHeader *frag = ((RIp6Fragment &)aPacket).Header();
	frag->iLength = aLength;
	frag->iOffset = aOffset;
	//
	// Add fragment to the queue. This assumes that the RMBufChain
	// is removed from the aPacket (IsEmpty() becomes True).
	//
	iQueue.Add((RIp6Fragment &)aPacket);
	ASSERT(aPacket.IsEmpty());

	if (iQueue.First().FragmentLength() >= (TUint)iLength)
		{
#ifdef _LOG
		TBuf<80> buf;
		buf.Format(_L("CAssembly::AddFragmentL(): [%d..%d (%d)] COMPLETED"), aOffset, aOffset+aLength-1, aLength);
		LogPrintId(buf, *this);
#endif
		// Packet has been fully assembled, restore the
		// complete packet into aPacket
		//
		if (CompletePacket() == KErrNone)
			{
			aPacket.Assign(iHead);
			if (info != iHead.Info())
				{
				//
				// Need to copy the Information from the iHead
				// (can't just do "*info = *iHead.Info()", because
				// that would copy the RMBuf base class parts too!!
				//
				// ...this looks a bit scary, does this always work? Should
				// look for a cleaner way to do this? -- msa
				const TInt off = sizeof(RMBufCell);
				const TInt len = sizeof(*info) - off;
				TPtr8((TUint8 *)info + off, len).Copy(TPtrC8((TUint8 *)iHead.Info() + off, len));
				}
			else
				{
				//
				// This only happens when the only fragment is full packet!
				//
				iHead.SetInfo(NULL);
				aPacket.SetInfo(info);
				}
			iHandler.Cancel(this);		// Deletes this!
			return KIp6Hook_DONE;
			}
		aPacket.Free();					// (mainly for info block!)
		iHandler.Cancel(this);			// Deletes this!
		return -1;
		}
	else
		{
		aPacket.FreeInfo();
#ifdef _LOG
		TBuf<80> buf;
		buf.Format(_L("CAssembly::AddFragmentL(): [%d..%d (%d)]"), aOffset, aOffset+aLength-1, aLength);
		LogPrintId(buf, *this);
#endif
		// Maintain total number of allocated bytes (roughly, this "overestimates"
		// if fragments overlap!).
		//
		iTotal += aLength;
		iHandler.iTotal += aLength;
		if (iHandler.iTotal > iHandler.iMaxTotal)
			{
#ifdef _LOG
			// ...borrow the 'buf' from above LOG block!
			buf.Format(_L("CAssembly::AddFragmentL(): %d went over max (assembly total=%d)"), iHandler.iTotal, iTotal);
			LogPrintId(buf, *this);
#endif
			User::Leave(KErrOverflow); // make assebly "dead" by leaving
			}
		return -1;
		}
	}


void CAssembly::Timeout()
	{
	//
	// Reconstruct start of IPv6 packet from the unfrag part
	//
	if (iVersion == 4)
		{
		if (CompletePacket() == KErrNone)
			iHandler.Network()->Icmp4Send(iHead, KInet4ICMP_TimeExceeded, 1, 0);
		}
	else
		{
		if (CompletePacket(1) == KErrNone)	// Request restore of FH (for TAHI)
			iHandler.Network()->Icmp6Send(iHead, KInet6ICMP_TimeExceeded, 1, 0);
		}
	LOG(LogPrintId(_L("CAssembly::Timeout()"), *this));
	iHandler.Cancel(this);	// <-- Deletes this!!!
	}

CAssembly *CFragmentHandler::LookupL(const RMBufRecvInfo &aInfo, TUint32 aId, TUint aVersion)
	{
	CAssembly *a = NULL;
	CAssembly *d = NULL;
	const TIp6Addr &src = TInetAddr::Cast(aInfo.iSrcAddr).Ip6Address();
	const TIp6Addr &dst = TInetAddr::Cast(aInfo.iDstAddr).Ip6Address();
	for (a = iCache; ; a = a->iNext)
		{
		if (a == NULL)
			{
			// If the quota for incomplete assemblies is all used
			if (iCount >= iMaxCount)
				{
				LOG(Log::Printf(_L("CFragmentHandler::LookupL() MaxCount %d reached"), iCount));
				if (d == NULL)
					User::Leave(KErrOverflow); // ... should assign some specific reason code?
				Cancel(d);
				LOG(Log::Printf(_L("CFragmentHandler::LookupL() deleted dead assembly")));
				}
			a = new (ELeave) CAssembly(*this, src, dst, aId, aVersion);
			a->iNext = iCache;
			iCache = a;
			a->iTimeout.Set(iTimeoutManager, 60);	// 60 seconds or bust!
			iCount++;
			LOG(LogPrintId(_L("CFragmentHandler::LookupL(): NEW"), *a));
			break;
			}
		else if (a->iId == aId && a->iSrcAddr.IsEqual(src) && a->iDstAddr.IsEqual(dst))
			break;
		else if (a->iIsDead)
			d = a;	// remember "oldest dead" assembly
		}
	return a;
	}

//
//	CFragmentHeaderHook::NewL
//
CFragmentHeaderHook *CFragmentHeaderHook::NewL(MNetworkService *aNetwork)
	{
	return new (ELeave) CFragmentHandler(aNetwork);
	}

void CFragmentHandler::ConstructL()
	{
	iTimeoutManager = TimeoutFactory::NewL();
	iNetwork->BindL((CProtocolBase *)this, BindHookFor(KProtocolInet6Fragment));


	//
	// Setup DOS protections
	// - maximum number of incomplete assemblies (iMaxCount)
	// - maximum amount of RMBUF space (iMaxTotal)
	//
	LOG(_LIT(KFormat, "\t[%S] %S = %d"));
	TInt value;
	if (iNetwork->Interfacer()->FindVar(TCPIP_INI_IP, TCPIP_INI_FRAG_COUNT, value))
		iMaxCount = value;
	else
		iMaxCount = KTcpipIni_FragCount;
	LOG(Log::Printf(KFormat, &TCPIP_INI_IP, &TCPIP_INI_FRAG_COUNT, iMaxCount));

	if (iNetwork->Interfacer()->FindVar(TCPIP_INI_IP, TCPIP_INI_FRAG_TOTAL, value))
		iMaxTotal = value;
	else
		iMaxTotal = KTcpipIni_FragTotal;
	LOG(Log::Printf(KFormat, &TCPIP_INI_IP, &TCPIP_INI_FRAG_TOTAL, iMaxTotal));
	}

//
//	CFragmentHandler::~CFragmentHandler()
//	*************************************
//
CFragmentHandler::~CFragmentHandler()
	{
	CancelAll();
	delete iTimeoutManager;
	}


//	******************
//	Reassembly section
//	******************
//	This a normal extension header receiver hook which is called when the
//	processing of headers reaches the Fragmentation Header.
//
TInt CFragmentHandler::Ip6ApplyL(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, const TInet6HeaderFragment &aHdr)
	{
	//
	// Extract fragment information off from the RMBufs
	// (after this, the fragment header can be disposed as needed)
	//
	aInfo.iProtocol = aHdr.NextHeader();
	const TUint32 fh0 = *(TUint32 *)&aHdr;		// needed for ICMP Time Exceeded.
	const TInt length = aInfo.iLength - aInfo.iOffset - sizeof(TInet6HeaderFragment);
	const TInt offset = aHdr.FragmentOffset();
	const TInt last = (aHdr.MFlag() == 0);

	CAssembly *a = LookupL(aInfo, aHdr.Id(), aInfo.iVersion);

	if (length <= 0 || ((length & 0x7) && !last))
		{
		// Zero length fragment, or the length of non-last fragment payload is not multiple of 8.
		// (always reply with ICMPv6, because IPv6 fragment header was used. If the packet was
		// actually an IPv4 packet, just adjust use different the parameter offset).
		//
		LOG(Log::Printf(_L("CFragmentHandler::IP6ApplyL(...): Payload Length=%d"), length));
		Network()->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 0,
			aInfo.iVersion == 4 ? TInet6HeaderIP4::O_TotalLength : TInet6HeaderIP::O_PayloadLength);
		Cancel(a);
		// Note: Icmp6Send takes the packet -- no packet Free required!
		return -1;
		}
	if (offset + length > 65535)
		{
		//
		// This fragment would result too long payload for the final IPv6 packet
		//
		// *** above test is not correct! It should also take into account the
		//	   length of the unfragmentable part (if that includes extension
		//	   headers in addition to IPv6 header). However, this information
		//	   is not available until the first fragment is received. Thus, it
		//	   is possible that illegal fragments get entered into the fragment
		//	   queue! Needs some checking!! -- msa
		//
		LOG(Log::Printf(_L("CFragmentHandler::IP6ApplyL(...): offset(%d) + length(%d) > 65535"), offset, length));
		Network()->Icmp6Send(aPacket, KInet6ICMP_ParameterProblem, 0,
							aInfo.iOffset + TInet6HeaderFragment::O_FragmentOffset);
		Cancel(a);
		// Note: Icmp6Send takes the packet -- no packet Free required!
		return -1;
		}
	if (last)
		{
		//
		// If this is the last fragment, the total length is now known
		//
		a->iLength = offset + length;
		}

	return a->Add(aPacket, sizeof(TInet6HeaderFragment), offset, length, fh0);
	}

//	*******************
//	Fragmenting section
//	*******************
//	When entered, aPacket contains full IPv6 packet, and
//	it has been already tested that this does not fit the
//	Path MTU!
//	If called with a packet that fits the MTU, this generates a
//	single fragment (just inserts the IPv6 fragment header into
//	the packet with offset==0 and M=1)
//
void CFragmentHandler::Ip6Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ, TInet6HeaderIP &aHdr)
	{
	//
	// Check extension headers that need to be included into
	// unfragmentable part
	// It is somewhat fuzzy what should be included in addition
	// of Hop-by-hop and Routing header. Currently including
	// destination options, if they are before the routing header
	// -- msa

	// unfrag_next
	//	point always to the last next header field of the
	//	unfragmentable part.
	TUint8 *unfrag_next = ((TUint8 *)&aHdr) + TInet6HeaderIP::O_NextHeader;
	//
	// unfrag_size
	//	size of the unfragmentable part.
	TInt unfrag_size = sizeof(TInet6HeaderIP);

	TInt offset = sizeof(TInet6HeaderIP);
	for (TInt header = (TInt)*unfrag_next;;)
		{
		TInet6Packet<TInet6HeaderExtension> ext(aPacket, offset);
		if (ext.iHdr == NULL)
			// If the next header (8 octets) cannot be mapped from
			// the packet it cannot be a valid extension header anyways.
			// This search can be terminated with what we have now.
			break;
		offset += ext.iHdr->HeaderLength();

		if (header == STATIC_CAST(TInt, KProtocolInet6HopOptions))
			{
			// The mapped header is Hop-by-hop options. This is
			// included into unfragmentable part, but must look
			// forward if there is a routing header. Just remember
			// this part
			unfrag_size = offset;
			unfrag_next = (TUint8 *)ext.iHdr;	// point to Next Hdr field.
			}
		else if (header != STATIC_CAST(TInt, KProtocolInet6DestinationOptions))
			{
			// Terminate search (we got either unknown extension
			// or routing header). If the latter, then include it
			// into unfragmentable part.
			if (header == STATIC_CAST(TInt, KProtocolInet6RoutingHeader))
				{
				unfrag_size = offset;
				unfrag_next = (TUint8 *)ext.iHdr;	// point to Next Hdr field.
				}
			break;
			}
		// Look for more
		header = ext.iHdr->NextHeader();
		}
	//
	// The NextHeader of the last header (either IPv6 or an extension
	// need to be changed into KProtocolInet6Fragment. And, the old
	// value is placed into next header of every generated fragment
	// header.
	TInt next_header = (TInt)*unfrag_next;
	*unfrag_next = KProtocolInet6Fragment;
	//
	// Remove the fragmentable part off the packet, leaving
	// only the unfragmentable part into the original packet
	// (and info block!)
	//
	RMBufChain fragmentable;	// Fragmentable part
	RMBufSendPacket fragment;
	RMBufChain tailpart;
	RMBufPktInfo *info;
	TInet6Packet<TInet6HeaderFragment> fh;
	TInt remainder, chunk;

	TInt err = aPacket.Split(unfrag_size, fragmentable);
	if (err == KErrNone)
		err = aPacket.Append(sizeof(TInet6HeaderFragment));
	if (err != KErrNone)
		goto drop_all;		// -- probably out of memory (RMBUF's)

	remainder = aPacket.Info()->iLength - unfrag_size;
	//
	// Prepare access for the fragment header
	//
	fh.Set(aPacket, unfrag_size, sizeof(TInet6HeaderFragment));
	unfrag_size += sizeof(TInet6HeaderFragment);
	if (err != KErrNone ||		// Memory allocation error, or...
		fh.iHdr == NULL ||		// Cannot access fragment header, or..
		aMtu < (unfrag_size+8))
		goto drop_all;			// Mission impossible! Path MTU is less than
								// required unfragmentable size + minimal payload!
	fh.iHdr->ZeroAll();
	fh.iHdr->SetNextHeader(next_header);
	fh.iHdr->SetId(++iFragmentId);
	fh.iHdr->SetMFlag(1);
	//
	// Fragment payload size is multiple of 8
	//
	chunk = (aMtu - unfrag_size) & ~0x7;
	offset = 0;
	//
	// Patch in fragment payload size into IPv6 header, this will
	// be the same for all but last fragment, so it can be set outside
	// the loop...
	// (making a big assumption that the aHdr pointer is still valid!!
	// the current operations done to the aPacket are safe, but beware
	// when changing this code...) -- msa
	//
	aHdr.SetPayloadLength(unfrag_size + chunk - sizeof(TInet6HeaderIP));
	while (remainder > chunk)
		{
		//
		// Setup unfragmentable part
		//
		TRAP(err,
			aPacket.CopyL(fragment);
			aPacket.CopyInfoL(fragment);
			);
		if (err != KErrNone)
			goto drop_all;
		info = fragment.Info();
		info->iLength = chunk + unfrag_size;
		//
		// Extract next fragment payload
		//
		err = fragmentable.Split(chunk, tailpart);
		if (err != KErrNone)
			goto drop_all;
		fragment.Append(fragmentable);
		fragment.Pack();
		aFragQ.Append(fragment);
		fragmentable.Assign(tailpart);
		offset += chunk;
		remainder -= chunk;
		fh.iHdr->SetFragmentOffset(offset);
		}
	//
	// Make aPacket as the last fragment
	//
	fh.iHdr->SetMFlag(0);
	aHdr.SetPayloadLength(unfrag_size + remainder - sizeof(TInet6HeaderIP));
	aPacket.Append(fragmentable);
	info = aPacket.Info();
	info->iLength = unfrag_size + remainder;
	aPacket.Pack();
	aFragQ.Append(aPacket);
	return;

drop_all:
	aFragQ.Free();
	fragmentable.Free();
	fragment.Free();
	tailpart.Free();
	aPacket.Free();
	LOG(Log::Printf(_L("CFragmentHandler::Ip6Fragment(...) FAILED")));
	}


//	**********************
//	IPv4 Fragment handling
//	**********************

//	******************
//	Reassembly section
//	******************
//	This a called when an IPv4 fragment is received. On entry
//	aHead.ip4 already contains a full copy of the IPv4 header
//	and aHead.iOffset == ip4.HeaderLength()
//
//	*NOTE*
//		On return, the IPv4 header checksum is not recomputed
//		(should it? maybe for possible ICMP error message? -- msa)
//
TInt CFragmentHandler::Ip4ApplyL(RMBufRecvPacket &aPacket, RMBufRecvInfo &aInfo, const TInet6HeaderIP4 &aHdr)
	{
	//
	// Extract fragment information off from the RMBufs
	// (after this, the fragment header can be disposed as needed)
	//
	aInfo.iProtocol = aHdr.Protocol();				// Put the protocol into info
	const TInt length = aInfo.iLength - aInfo.iOffset;	// Actual payload length
	const TInt offset = (aHdr.FragmentOffset()) << 3;
	const TInt last = (aHdr.MF() == 0);
	//
	// Locate or create an assemble matching this fragment
	//
	CAssembly *a = LookupL(aInfo, aHdr.Identification() | (aInfo.iProtocol << 16), 4);

	if (length <= 0 || ((length & 0x7) && !last))
		{
		// Zero length fragment or,
		// the length of non-last fragment payload is not multiple of 8.
		//
		Network()->Icmp4Send(aPacket, KInet4ICMP_ParameterProblem, 0,
			TInet6HeaderIP4::O_TotalLength);
		Cancel(a);
		// Note: Icmp4Send takes the packet -- no packet Free required!
		return -1;
		}
	if (offset + length > 65535 - TInet6HeaderIP4::MinHeaderLength())
		{
		//
		// This fragment would result too long payload for the final IPv4 packet
		// (NOTE: above is not full proof, if the first fragment has options!
		//	The intent is just to drop obviously wrong fragments before doing
		//  anything more with them... -- msa)
		//
		Network()->Icmp4Send(aPacket, KInet4ICMP_ParameterProblem, 0,
			TInet6HeaderIP4::O_FragmentOffset);
		Cancel(a);
		// Note: Icmp4Send takes the packet -- no packet Free required!
		return -1;
		}

	if (last)
		a->iLength = offset + length;	// Last fragment, real payload length is now known

	return a->Add(aPacket, 0, offset, length);
	}

//
//	CrunchOptions
//	-------------
//		Only options marked as "copy" are copied into every fragment header.
//
//		This function takes options from the original packet and removes
//		all except the copiable options (process in place).
//
//		Returns the length of the "crunched" IPv4 header
//
static TInt CrunchOptions(TInet6HeaderIP4 &aHdr)
	{
	TInt length = TInet6HeaderIP4::MinHeaderLength();
	TUint8 *opt = ((TUint8 *)&aHdr) + length;
	TUint8 *dst = opt;
	TInt count = aHdr.HeaderLength() - length;
	while (count > 0)
		{
		if (*opt < 2)
			{
			// Q: Is it legal to have "Copy" on PAD? If so, this
			// needs some fixing yet! -- msa
			//
			++opt;	// END/PAD bytes, skip
			count -= 1;
			}
		else
			{
			int optlen = opt[1];
			if (optlen > count || optlen < 2)
				// An illegal option length, just bail out.
				// (The packet is broken anyway)
				break;
			count -= optlen;
			if (*opt & 0x80)
				{
				//
				// Copy the option
				//
				length += optlen;
				while (--optlen >= 0)
					*dst++ = *opt++;
				}
			else
				//
				// Crunch this
				//
				opt += optlen;
			}
		}
	//
	// The IPv4 header length is multiple of 4, and thus options length
	// must be padded, if not already aligned.. (Pad with END marker)
	//
	while (length & 0x3)
		{
		*dst++ = 0;	// EMD
		length++;
		}
	aHdr.SetHeaderLength(length);
	return length;
	}

//	*******************
//	Fragmenting section
//	*******************
//	When entered, aPacket contains full IPv4 packet, and
//	it has been already tested that this does not fit the
//	Path MTU!
void CFragmentHandler::Ip4Fragment(RMBufPacketBase &aPacket, TInt aMtu, RMBufPktQ &aFragQ, TInet6HeaderIP4 &aHdr)
	{
	RMBufPacketBase fragmentable;	// Fragmentable part
	RMBufChain tailpart;			// Just work space
	TInt hlen, chunk, remainder, offset;
	TInet6HeaderIP4 ip4;
	TPtr8 ip4ptr((TUint8 *)&ip4, sizeof(ip4), sizeof(ip4));
	TInt err = KErrNone;
	
	offset = aHdr.FragmentOffset() << 3;
	hlen = aHdr.HeaderLength();

	chunk = (aMtu - hlen) & ~7;
	if (chunk <= 0 || aHdr.DF())
		goto drop_all;	// Unreasonable Mtu... or fragmenting not allowed
	//
	// Split off the first fragment...
	//
	err = aPacket.Split(hlen + chunk, fragmentable);
	if (err != KErrNone)
		goto drop_all;
	aHdr.SetTotalLength(hlen + chunk);
	offset += chunk;
	remainder = aPacket.Info()->iLength - hlen - chunk;
	aPacket.Info()->iLength = hlen + chunk;
	//
	// Setup a fragment header including the options that
	// need to be copied for all the remaining fragments
	//
	ip4ptr = TPtrC8((TUint8 *)&aHdr, hlen);
	hlen = CrunchOptions(ip4);
	ip4ptr.SetLength(hlen);
	//
	// Generate "middle fragments". All of these will
	// have the more (MF) as 1.
	//
	chunk = (aMtu - hlen) & ~7;
	ip4.SetTotalLength(hlen + chunk);
	ip4.SetFlags(KInet4IP_MF);
	while (remainder > chunk)
		{
		//
		// Extract next fragment payload
		//
		err = fragmentable.Split(chunk, tailpart);
		if (err == KErrNone)
			err = fragmentable.Prepend(hlen);
		if (err != KErrNone)
			goto drop_all;
		ip4.SetFragmentOffset((TUint16)(offset >> 3));
		ip4.SetChecksum(0);
		ip4.SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)&ip4, hlen)));
		fragmentable.CopyIn(ip4ptr);
		TRAP(err, aPacket.CopyInfoL(fragmentable));
		if (err != KErrNone)
			goto drop_all;
		fragmentable.Info()->iLength = chunk + hlen;
		fragmentable.Pack();
		aFragQ.Append(fragmentable);
		fragmentable.Assign(tailpart);
		offset += chunk;
		remainder -= chunk;
		}
	//
	// Generate the last fragment
	//
	err = fragmentable.Prepend(hlen);
	if (err != KErrNone)
		goto drop_all;
	ip4.SetFragmentOffset((TUint16)(offset >> 3));
	ip4.SetTotalLength(hlen + remainder);
	ip4.SetFlags((TUint8)(aHdr.Flags()));	// need to copy More flag from the original packet!
	ip4.SetChecksum(0);
	ip4.SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)&ip4, hlen)));
	fragmentable.CopyIn(ip4ptr);
	TRAP(err, aPacket.CopyInfoL(fragmentable));
	if (err != KErrNone)
		goto drop_all;
	fragmentable.Info()->iLength = hlen + remainder;
	fragmentable.Pack();
	aFragQ.Append(fragmentable);

	//
	// Complete the header of the first fragment
	//
	aHdr.SetFlags(KInet4IP_MF);

	aHdr.SetChecksum(0);
	aHdr.SetChecksum(TChecksum::ComplementedFold(TChecksum::Calculate((TUint16*)&aHdr, aHdr.HeaderLength())));
	aPacket.Pack();
	aFragQ.Prepend(aPacket);
	//
	// All fragments successfully built.
	//
	return;
	//
	// Cleanup everything
	//
drop_all:
	aFragQ.Free();
	fragmentable.Free();
	tailpart.Free();
	aPacket.Free();
	LOG(Log::Printf(_L("CFragmentHandler::Ip4Fragment(...) FAILED")));
	}
