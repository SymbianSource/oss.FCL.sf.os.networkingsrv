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
// This file forms the interface that other modules use to access the dummy ppp nif - so definitions of addressing 
// used by the nif etc
// 
//

#if (!defined __DUMMYPPPNIFVAR_H__)
#define __DUMMYPPPNIFVAR_H__

// some port number not likely to be used by anyone else
const TUint KDummyPppNifCmdPort = 0xAB;

enum TDummyPPPNifCommands
{
	KForceDisconnect = 0xFC
};

// address ranges for different nifs (all variants of dummy)
const TUint KDummyPppNifLocalAddressBase  = 0x0A010100;

#endif
