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
// This file forms the interface that other modules use to access the dummy nif - so definitions of addressing 
// used by the nif etc
// 
//

#if (!defined __DUMMYNIFVAR_H__)
#define __DUMMYNIFVAR_H__

#include <in_sock.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_sock_partner.h>
#endif

/**
some port number not likely to be used by anyone else
@internalTechnology
*/
const TUint KDummyNifCmdPort = 0xAB;

/**
some things you might want to do with the dummy nif
@internalTechnology

NOTE: keep these in sync with the non-Flow version of Dummy Nif
*/
enum TDummyNifCommands
{
	KForceReconnect = 0xFE,
	KSendNotification = 0xFD,
	KForceDisconnect = 0xFC,
	KForceFinishedSelection = 0xFB,
	KForceBinderError = 0xFA,
	KColourDataByLinkTierAccessPointId = 0xF9
};

/**
@internalTechnology
*/
// address ranges for different nifs (all variants of dummy)
const TUint32 KDummyNifLocalAddressBase				= INET_ADDR(10,1,1,0);

/**
@internalTechnology
*/
const TUint32 KHungryNifLocalAddressBase			= INET_ADDR(10,2,1,0);
 


/**
@internalTechnology
*/
// Class C host id that the Dummy/Hungry Nifs will never allocate as a local address
// (10.1.1.4 and 10.2.1.4).  These addresses are guaranteed not to match the (randomly
// allocated) local address of the Dummy/Hungry Nifs and hence will avoid the situation
// where the TCP/IP stack does a "short-cut" loopback of packets that avoid the Nifs.
const TUint32 KDummyHungryNifReservedHostId		= 4;

// return value from CNifIfLink::Send()
const TInt KDummyNifSendOkay = 1;


//
// DummyNif specific Control options (Level KCOLInterface)
//

/** Base constant for options that set Nifman Idle Timeouts. Used only to define other options. 
@internalTechnology */
const TInt KTestSoDummyNifSetNifmanTimeout(125); 

/** Set Short (LastSessionClosed) Nifman Idle timeout. Level KCOLInterface.
@internalTechnology */
const TInt KTestSoDummyNifSetLastSessionClosedTimeout( (KTestSoDummyNifSetNifmanTimeout + 1) | KConnReadUserDataBit);

/** Set Medium (LastSocketClosed) Nifman Idle Timeout. Level KCOLInterface.
@internalTechnology */
const TInt KTestSoDummyNifSetLastSocketClosedTimeout(  (KTestSoDummyNifSetNifmanTimeout + 2) | KConnReadUserDataBit);

/** Set Long (LastSocketActivity) Nifman Idle Timeout. Level KCOLInterface.
@internalTechnology */
const TInt KTestSoDummyNifSetLastSocketActivityTimeout((KTestSoDummyNifSetNifmanTimeout + 3) | KConnReadUserDataBit);

#endif
