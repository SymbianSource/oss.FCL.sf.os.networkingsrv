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
// dummynif6.cpp
// 
//

#include <e32hal.h>	// UserHal::MachineInfo()
#include <comms-infras/nifif.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <in_iface.h>
#include <comms-infras/commsdebugutility.h>
#include <connectprog.h>

#include <in_chk.h>
#include <in_sock.h>
#include <in6_if.h>	// KSoIface*, KIf*, TSoInet6IfConfig
#include "dummynif.h"
#include "hungrynif.h"


/*
 * The IPv6 interface binder class
 */

CHungryIf6::CHungryIf6(CHungryIfLink& aLink)
	: CDummyIf6(aLink)
	{
	CDummyIfLog::Printf(_L("CHungryIf6::CHungryIf6()"));

	iIfName.Format(_L("hungrynif6[0x%08x]"), this);
	}

void CHungryIf6::Recv(RMBufChain& aPdu)
	{
	// get the IP and UDP header from the RMBufChain
	TInet6HeaderIP* ip6 = (TInet6HeaderIP*) aPdu.First()->Next()->Ptr();
	TInet6HeaderUDP* udp = NULL;

	if ((TUint)ip6->NextHeader() == KProtocolInetUdp)
		{
		// get the udp header as well - assume only udp traffic here
		udp = (TInet6HeaderUDP*) ip6->EndPtr();

		CDummyIfLog::Printf(_L("CHungryIf6::Recv(...): UDP length %d, src port %d, dst port %d"),
			udp->Length(), udp->SrcPort(), udp->DstPort());
		}
	else
		{
		CDummyIfLog::Printf(_L("CHungryIf6::Recv(...): IPv6 length %d, next header %d"),
			ip6->PayloadLength(), ip6->NextHeader());
		}

	// just discard the packet, as this NIF is supposed to do.

	aPdu.Free();
	return;
}

