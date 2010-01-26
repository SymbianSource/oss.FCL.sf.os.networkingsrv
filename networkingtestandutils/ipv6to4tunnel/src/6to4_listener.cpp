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
// Name        : 6to4_listener.cpp
// Part of     : 6to4 plugin / 6to4.prt
// Implements 6to4 automatic and configured tunnels, see
// RFC 3056 & RFC 2893
// Version     : 0.2
//




// INCLUDE FILES
#include <inet6log.h>
#include <in6_opt.h>
#include "6to4_listener.h"

// EXTERNAL DATA STRUCTURES
// EXTERNAL FUNCTION PROTOTYPES  
// CONSTANTS
// MACROS
// LOCAL CONSTANTS AND MACROS
// MODULE DATA STRUCTURES
// LOCAL FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// UpdateAddress
// ----------------------------------------------------------------------------
static void UpdateAddress(MInterfaceManager &aIfMgr, const TIp6Addr &aAddr, TBool aDelete)
	{
	// Add or delete address to/from 6to4 interface
	TPckgBuf<TSoInet6InterfaceInfo> opt;
	TSoInet6InterfaceInfo &inf = opt();
	inf.iName = K6to4;
	inf.iDoState = 0;
	inf.iDoId = 1;
	inf.iDoPrefix = 0;
	inf.iDoAnycast = 0;
	inf.iDoProxy = 0;
	inf.iAlias = 0;
	inf.iDelete = aDelete;
	inf.iAddress.SetAddress(aAddr);
	inf.iDefGate.SetFamily(0);
	inf.iNetMask.SetFamily(0);
	inf.iNameSer1.SetFamily(0);
	inf.iNameSer2.SetFamily(0);
	inf.iMtu = 0;
	inf.iSpeedMetric = 0;

	const TInt res = aIfMgr.SetOption(KSolInetIfCtrl, KSoInetConfigInterface, opt);
#ifdef _LOG
	_LIT(KDelete, "Deleted");
	_LIT(KAdded,  "Added");
	TBuf<70> tmp;
	inf.iAddress.Output(tmp);
	Log::Printf(_L("%S 6to4 address %S (result=%d)"), aDelete ? &KDelete() : &KAdded(), &tmp, res);
#else
	(void)res;	// (silence unused warning)
#endif
	}


// ============================ MEMBER FUNCTIONS ==============================

C6to4Listener::C6to4Listener (MNetworkService *const aNetwork, MEventService &aService) :
	 CBase (), iNetwork(aNetwork), iService(aService)
	{
	// Register the listener to get the events when addresses are added/deleted
	// to/from an interface.
	iService.RegisterListener (this, EClassAddress);
	}

C6to4Listener::~C6to4Listener()
	{
	// Unregister the listener to not get the events anymore when addresses
	// are added/deleted to/from an interface.
	iService.RemoveListener (this);
	}

// ----------------------------------------------------------------------------
// C6to4Listener::Notify
// Notification handler. Handles interface address addings and deletions.
// 
// ----------------------------------------------------------------------------
//
void C6to4Listener::Notify (TUint aEventClass, TUint aEventType,
							const void *aData)
	{
	if (aEventClass == EClassAddress)
		{
		TInetAddressInfo *info = (TInetAddressInfo *) aData;

		if (info->iPrefixLen != 0 || !info->iAddress.IsV4Mapped())
			// Only interested in IPv4 addressess
			return;

		const TBool remove = aEventType == EventTypeDelete;
		if (!remove && aEventType != EventTypeAdd)
			return;
		
		// Only Delete or Add events processed

		// Build the ip address for the 6to4 virtual interface
		// and just for automatic tunnels, since the 6to4 address
		// for each interface IPv4 address has to be known by the 
		// system for it to accept the packet.

		TIp6Addr addr;
		addr.u.iAddr8[0] = 0x20;
		addr.u.iAddr8[1] = 0x02;
		addr.u.iAddr8[2] = info->iAddress.u.iAddr8[12];
		addr.u.iAddr8[3] = info->iAddress.u.iAddr8[13];
		addr.u.iAddr8[4] = info->iAddress.u.iAddr8[14];
		addr.u.iAddr8[5] = info->iAddress.u.iAddr8[15];
		addr.u.iAddr8[6] = 0;
		addr.u.iAddr8[7] = 0;
		addr.u.iAddr8[8] = 0;
		addr.u.iAddr8[9] = 0;
		addr.u.iAddr8[10] = 0;
		addr.u.iAddr8[11] = 0;
		addr.u.iAddr8[12] = 0;
		addr.u.iAddr8[13] = 0;
		addr.u.iAddr8[14] = 0;
		addr.u.iAddr8[15] = 1;

		UpdateAddress(*NetworkService ()->Interfacer (), addr, remove);
		}
	}


// ========================== OTHER EXPORTED FUNCTIONS ========================

//  End of File  
