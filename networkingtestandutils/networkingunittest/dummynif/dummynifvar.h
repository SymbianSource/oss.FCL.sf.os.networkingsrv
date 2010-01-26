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
// This file forms the interface that other modules use to access the dummy nif - so definitions of addressing 
// used by the nif etc
// 
//

#if (!defined __DUMMYNIFVAR_H__)
#define __DUMMYNIFVAR_H__

/**
some port number not likely to be used by anyone else
@internalTechnology
*/
const TUint KDummyNifCmdPort = 0xAB;

/**
some things you might want to do with the dummy nif
@internalTechnology
*/
enum TDummyNifCommands
{
	KForceReconnect = 0xFE,
	KSendNotification = 0xFD,
	KForceDisconnect = 0xFC,
	KForceFinishedSelection = 0xFB
};

/**
@internalTechnology
*/
// address ranges for different nifs (all variants of dummy)
const TUint KDummyNifLocalAddressBase  = 0x0A010100;

/**
@internalTechnology
*/
const TUint KHungryNifLocalAddressBase = 0x0A020100;

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
