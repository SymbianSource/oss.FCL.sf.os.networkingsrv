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

/**
 @file
 @internalComponent 
*/

#ifndef __MSCBCPC_H__
#define __MSCBCPC_H__

#ifndef __PPPBASE_H__
#include "PPPBASE.H"
#endif

class CPppMsCbcpClient;

class PppNcpMsCbcpFactory
	{
public:
	static CPppMsCbcpClient * NewL(	CPppLcp * aLcp,
									TMSCBCPAction aAction,
									const TUint8 * aCallbackInfo,
									const TInt aCallbackInfoLen );
	static void Delete( CPppMsCbcpClient * );
	};

#endif

