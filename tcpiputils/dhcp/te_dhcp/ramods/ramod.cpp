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
//

/**
 @file ramod.cpp
 @internalComponent
*/


#include <in_chk.h>
#include <icmp6_hdr.h>
#include <in_sock.h>
#include <in_bind.h>
#include "in6_opt.h"

#include "ramod.h"
#include "HookLog.h"


Cramod* Cramod::NewL()
	{
	Cramod* self = new(ELeave) Cramod();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
	}



void Cramod::ConstructL()
	{
	}


Cramod::Cramod()
	{
	}



Cramod::~Cramod()
	{
	}



void Cramod::BindL(CProtocolBase* aProtocol, TUint aId)
	{
	// Do sanity checks
	//
	if ((aId != KProtocolInet6Ip) || (aProtocol == this))
		{
		User::Leave(KErrArgument);
		}

	TUint ourId;
		{
		TServerProtocolDesc info;
		Identify(&info);
		ourId = info.iProtocol;
		}

	if (aId == ourId)
		{
		User::Leave(KErrArgument);
		}

	if ( iProtocolIPv6 != NULL )
		{
		if ( iProtocolIPv6 == aProtocol )
			{
			// We don't need to bind to the same protocol twice.
			//
			return;
			}
		else
			{
			// We don't want to bind to a different protocol either...
			//
			User::Leave(KErrAlreadyExists);
			}
		}

	iProtocolIPv6 = (CProtocolInet6Binder*) aProtocol;

	RegisterHooksL();
	}



void Cramod::Unbind(CProtocolBase* aProtocol, TUint /* aId */)
	{
	if (iProtocolIPv6 != aProtocol)
		{
		return;
		}

	UnregisterHooks();

	iProtocolIPv6 = 0;
	}
	



void Cramod::RegisterHooksL(void)
/**
 * Registers the hook to catch IP stack events
 */
	{
	if(!iProtocolIPv6)
		{
		User::Leave(KErrNotReady);
		}

	// bind to IP for incoming packets
	iProtocolIPv6->BindL((CProtocolBase*)this, BindHookFor(KProtocolInet6Icmp));
	}


	
void Cramod::UnregisterHooks(void)
/**
 * Detaches this hook from the running of the stack.
 *  This is so IPEN doesn't slow down the stack while it isn't
 *   needed by any clients.
 *
 *  e.g. without this mechanism, every incoming packet would pass through
 *   Cramod::ApplyL pointlessly.
 *
 */
	{
	if(!iProtocolIPv6)
		{
		return;
		}
	// If IPv6 exists, unbind the hook. Covers both in/out hooks (I hope)
	iProtocolIPv6->Unbind(this);
	}

	

void Cramod::FillIdentification(TServerProtocolDesc& anEntry)
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: reference to the structure to be filled in
 */
	{
	anEntry.iName=_S("ramod");
	anEntry.iAddrFamily=KAfInet; //KAfExain;
	anEntry.iSockType=KSockDatagram;
	anEntry.iProtocol=KMyProtocolId;
	anEntry.iVersion=TVersion(1, 0, 0);
	anEntry.iByteOrder=EBigEndian;
	anEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
	anEntry.iNamingServices=0;
	anEntry.iSecurity=KSocketNoSecurity;
	anEntry.iMessageSize=0xffff;
	anEntry.iServiceTypeInfo=0; // 1 for ability to create sockets
	anEntry.iNumSockets=KUnlimitedSockets;
	}


void Cramod::Identify(TServerProtocolDesc* aProtocolDesc) const
/**
 * Fills in an existing protocol description structure with details of this protocol.
 *
 * @param aProtocolDesc: pointer to the structure to be filled in
 */
	{
	FillIdentification(*aProtocolDesc);
	}



/**
 * Incoming packet
 */
TInt Cramod::ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo)
	{

	LOG	(
		_LIT(KHookNotifyStr,"Cramod::ApplyL hit with protocol: %d");
		HookLog::Printf( KHookNotifyStr , aInfo.iProtocol );
		)

	if(aInfo.iProtocol == static_cast<TInt>(KProtocolInet6Icmp))
		{

		LOG	(
			_LIT(KHookNotifyStr," ICMPv6 packet detected");
			HookLog::Printf( KHookNotifyStr );
			)

		/**
		 * Could switch on ICMP type here, in case the hook needs to catch
		 *   other ICMP packet types..
		 */
		TInet6Checksum<TInet6HeaderICMP_RouterAdv> icmp(aPacket,aInfo.iOffset);

		if(icmp.iHdr)
			{
			if (icmp.iHdr->Type() == KInet6ICMP_RouterAdv &&
				icmp.VerifyChecksum( aPacket,
							  aInfo.iProtocol == static_cast<TInt>(KProtocolInet6Icmp) ? &aInfo : NULL,
							  aInfo.iOffset) )
				{
#ifdef RAMOD1
				// RA received from LAN, modfiy the M and O flag here, clear 'M' flag and set 'O' flag
				icmp.iHdr->SetFlags(64);
				icmp.ComputeChecksum( aPacket,
							aInfo.iProtocol == static_cast<TInt> (KProtocolInet6Icmp) ? &aInfo : NULL,
							aInfo.iOffset );
#elif RAMOD2
				// RA received from LAN, modfiy the M and O flag here, set both 'M' and 'O' flags as false
				icmp.iHdr->SetFlags(0);
				icmp.ComputeChecksum( aPacket,
							aInfo.iProtocol == static_cast<TInt> (KProtocolInet6Icmp) ? &aInfo : NULL,
							aInfo.iOffset );

#elif RAMOD3
				// RA received, modfiy the M and O flag here, invalidate checksum by changing flag, Don't compute checksum, packets will be dropped
				icmp.iHdr->SetFlags(0);
#endif
				}
			}

		}
			
	return KIp6Hook_PASS;   // ensure we don't affect the control flow
	}



