// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// The implementation of the hungry nif (based on the dummy nif)
// 
//

#include "hungrynif.h"
#include <es_mbuf.h>

/**
 * IPv4 interface binder class
 */

CHungryIf4::CHungryIf4(CHungryIfLink& aLink) : CDummyIf4(aLink)
	{
	CDummyIfLog::Printf(_L("CHungryIf4::CHungryIf4()"));

	// generate my local ip address (ip4) - will overwrite that generated in CDummyIf
	iLocalAddressBase = KHungryNifLocalAddressBase; // also used later in control method
	iLocalAddress = iLocalAddressBase + ((TUint32)this)%255;

	iIfName.Format(_L("hungrynif[0x%08x]"), this);
	}

void CHungryIf4::Recv(RMBufChain& aPdu)
	{

	// the whole idea of this nif is that it swallows packets for them never 
	// to be seen again, hence do nothing here :-)

	// except... want the stack to take ownership of the packet so we don't get a 
	// panic, break pckt first by changing the checksum so it doesn't make it thru
	// the whole stack - presumably the stack destroys it
	//
	// Later Note: it is also possible to just do aPdu.Free() (as in IPv6 binder).
	// However, this may change the timing as it would be much faster and may
	// cause sensitive tests to be re-worked if they fail as a result.

	TInet6HeaderIP4* ip4 = (TInet6HeaderIP4*) aPdu.First()->Next()->Ptr();

	if ((TUint)ip4->Protocol() == KProtocolInetUdp)
		{
		TInet6HeaderUDP* udp = (TInet6HeaderUDP*) ip4->EndPtr();

		CDummyIfLog::Printf(_L("CHungryIf4::Recv(...): UDP length %d, src port %d, dst port %d"),
			udp->Length(), udp->SrcPort(), udp->DstPort());
		}
	else
		CDummyIfLog::Printf(_L("CHungryIf4::Recv(...): IPv4 length %d, protocol %d"),
			ip4->TotalLength(), ip4->Protocol());



	// zero should be fine... assuming not zero already!
	TInt checksum = ip4->Checksum();
	if (checksum != 0)
		ip4->SetChecksum(0);
	else
		ip4->SetChecksum(1); // some other value
	iProtocol->Process(aPdu, (CProtocolBase*)this);
	}

/*
 * The link class
 */

CHungryIfLink::CHungryIfLink(CNifIfFactory& aFactory)
	: CDummyIfLink(aFactory)
	{}

CNifIfBase* CHungryIfLink::GetBinderL(const TDesC& aName)
	{
	CDummyIfLog::Printf(_L("CHungryIfLink::GetBinderL(%S)"), &aName);
	_LIT(KDescIp6, "ip6");
	if (aName.CompareF(KDescIp6) == 0)
		{
		iNifIf6 = new(ELeave) CHungryIf6(*this);
		return iNifIf6;
		}
	else
		{	// ip4
		iNifIf4 = new(ELeave) CHungryIf4(*this);
		return iNifIf4;
		}
	}
