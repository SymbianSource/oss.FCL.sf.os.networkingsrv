// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

#include "policy.h"
#include "ipip6.h"

#include <in_chk.h>

/*
 *	CPolicySpec
 */
CPolicySpec::CPolicySpec(const MPacketSpec& aPacketSpec, const MActionSpec& aAction)
	:iMatchSpec(&aPacketSpec), iActionSpec(&aAction)
	{
	}

CPolicySpec::~CPolicySpec()
	{
	// Cant be const as the members are allocated by someone else
	delete const_cast<MPacketSpec*>(iMatchSpec);
	delete const_cast<MActionSpec*>(iActionSpec);
	}

// Takes a packet spec & gives back the action we need to take
TBool CPolicySpec::Match(const MPacketSpec& aSpec, MActionSpec& aAction) const
	{
	(void)aAction;
	(void)aSpec;
	return ETrue;
	}

/*
 *	CPolicyHolder
 */
CPolicyHolder::~CPolicyHolder()
	{
	iPolicyList.ResetAndDestroy();
	}

void CPolicyHolder::AddSpecL(const MPacketSpec& aPacket, const MActionSpec& aAction)
	{
	if (FindAction(aPacket) == KErrNotFound)
		{
		// Create a policy
		CPolicySpec* policy = new(ELeave) CPolicySpec(aPacket, aAction);
		User::LeaveIfError(iPolicyList.Append(policy));
		return;
		}
	User::Leave(KErrAlreadyExists);
	}

void CPolicyHolder::DelSpec(const TInt index)
	{
	(void)index;
	}

// This hook example does not take into consideration what would happen if more 
// than 1 policy matches a single packethead
const MActionSpec * CPolicyHolder::MatchSpec(const MPacketSpec& aPacket, TBool aIsIncoming) const
	{
	TInt index = KErrNotFound;
	if (aIsIncoming)
		{
		index = FindPolicy(aPacket);
		}
	else 
		{
		index = FindAction(aPacket);
		}

	if (index != KErrNotFound)
		{
		return iPolicyList[index]->iActionSpec;
		}

	else 
		{
		return NULL;
		}
	}

//Takes a packetspec and returns the index in the list
TInt CPolicyHolder::FindAction(const MPacketSpec& aPacket) const
	{
	for (TInt index = 0; index < iPolicyList.Count(); index++)
		{
		CPolicySpec* policy = iPolicyList[index];
		if ((policy->iMatchSpec)->Match(aPacket))
			{
			return index;
			}
		}
	return KErrNotFound;
	}

TInt CPolicyHolder::FindPolicy(const MPacketSpec& aPacket) const
	{
	for (TInt index = 0; index < iPolicyList.Count(); index++)
		{
		CPolicySpec* policy = iPolicyList[index];
		if ((policy->iActionSpec)->Match(aPacket))
			{
			return index;
			}
		}
	return KErrNotFound;
	}

/*
 *	CIPIPPacketSpec
 */
CIPIPPacketSpec* CIPIPPacketSpec::NewLC(const TInetAddr& aSrc, const TInetAddr& aDst)
	{
	CIPIPPacketSpec* self = new(ELeave) CIPIPPacketSpec(aSrc, aDst);
	CleanupStack::PushL(self);
	return self;
	}
	
CIPIPPacketSpec::CIPIPPacketSpec(const TInetAddr& aSrc, const TInetAddr& aDest)
	:iPacketSpec(aSrc, aDest)
	{}

CIPIPPacketSpec::~CIPIPPacketSpec()
	{}

TBool CIPIPPacketSpec::Match(const MPacketSpec& aOtherSpec) const
	{
	const CIPIPPacketSpec* other = (CIPIPPacketSpec*)&aOtherSpec;
	return Match(other->iPacketSpec.iSrcAddr, other->iPacketSpec.iDestAddr);
	}

TBool CIPIPPacketSpec::Match(const TInetAddr& aSrc, const TInetAddr& aDst) const
	{
	(void)aSrc;
	return ((iPacketSpec.iDestAddr.CmpAddr(aDst)) /*&& (iPacketSpec.iSrcAddr.CmpAddr(aSrc))*/ );
	}

/*
 *	CIpIpActionSpec
 */
CIpIpActionSpec* CIpIpActionSpec::NewLC(const TInetAddr& aSrc, const TInetAddr& aDst)
	{
	CIpIpActionSpec* self = new(ELeave) CIpIpActionSpec(aSrc, aDst);
	CleanupStack::PushL(self);
	return self;
	}
	
CIpIpActionSpec::CIpIpActionSpec(const TInetAddr& aSrc, const TInetAddr& aDest)
	:iActionSpec(aSrc, aDest)
	{}

CIpIpActionSpec::~CIpIpActionSpec()
	{}

// matching the external addresses here
TBool CIpIpActionSpec::Match(const MPacketSpec& aOtherSpec) const
	{
	const CIPIPPacketSpec* other = (CIPIPPacketSpec*)&aOtherSpec;
	return(( iActionSpec.iDestAddr.Match(other->iPacketSpec.iSrcAddr) && 
		iActionSpec.iSrcAddr.Match(other->iPacketSpec.iDestAddr)));
	}

// For flow hooks
TInt CIpIpActionSpec::ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo)
	{
	TIpHeader *ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
	if (!ip)
		return KErrGeneral;

	const TInt inner_ip4 = (ip->ip4.Version() == 4);

	if (iActionSpec.iDestAddr.IsV4Mapped())
		{
		const TInt hlen = TInet6HeaderIP4::MinHeaderLength();
		aPacket.PrependL(hlen);
		TInet6Checksum<TInet6HeaderIP4> outer(aPacket);
		if (outer.iHdr == NULL)
			{
			return KErrGeneral;
			}
		aInfo.iLength += hlen;
		aInfo.iProtocol = KProtocolInetIp;		// Outer is IPv4
		outer.iHdr->Init();
		outer.iHdr->SetHeaderLength(hlen);
		outer.iHdr->SetTotalLength(aInfo.iLength);
		outer.iHdr->SetIdentification(AllocId());
		outer.iHdr->SetDstAddr(iActionSpec.iDestAddr.Address());
		outer.iHdr->SetSrcAddr(iActionSpec.iSrcAddr.Address());

		if (inner_ip4)
			{
			outer.iHdr->SetTOS(ip->ip4.TOS());
			outer.iHdr->SetTtl(ip->ip4.Ttl());
			outer.iHdr->SetFlags((TUint8)(ip->ip4.Flags() & KInet4IP_DF));
			outer.iHdr->SetProtocol(KProtocolInetIpip);
			outer.ComputeChecksum();
			}
		else 
			{
			Panic(EIpipPanic_Ip6NotSupported);
			}
		}
	else 
		{
		Panic(EIpipPanic_Ip6NotSupported);
		}

	return KErrNone;
	}

// For Incoming hooks
TInt CIpIpActionSpec::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
	{
	aPacket.TrimStart(aInfo.iOffset);	
	aInfo.iOffsetIp = 0;	

	if (aPacket.IsEmpty())
	   return KErrGeneral;
							
	TInet6Packet<TIpHeader> ip(aPacket);
	if (ip.iHdr == NULL)
		return KErrGeneral;

	aInfo.iFlags &= ~KIpAddressVerified;
	aInfo.iVersion = (TUint8)ip.iHdr->ip4.Version();

	if (aInfo.iVersion == 4)
		{
		const TInt hlen = ip.iHdr->ip4.HeaderLength();
		aInfo.iOffset = aInfo.iOffsetIp + hlen;
		if (ip.iLength < aInfo.iOffset)
			return KErrGeneral;
		if (TChecksum::ComplementedFold(TChecksum::Calculate((TUint16 *)ip.iHdr, hlen)) != 0)
			return EIpipPanic_CorruptPacketIn;	

		aInfo.iPrevNextHdr = (TUint16)(aInfo.iOffsetIp + TInet6HeaderIP4::O_Protocol);
		aInfo.iProtocol = ip.iHdr->ip4.Protocol();
		(TInetAddr::Cast(aInfo.iSrcAddr)).SetV4MappedAddress(ip.iHdr->ip4.SrcAddr());
		(TInetAddr::Cast(aInfo.iDstAddr)).SetV4MappedAddress(ip.iHdr->ip4.DstAddr());
		}

	return KIp6Hook_DONE; //aInfo.iProtocol;
	}

