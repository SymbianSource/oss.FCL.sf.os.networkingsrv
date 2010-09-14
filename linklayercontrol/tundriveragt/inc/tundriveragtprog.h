/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file for tundriver agent
* 
*
*/

/**
 @file tundriveragtprog.h
 @internalTechnology
*/

#if !defined (__TUNDRIVERAGTPROG_H__)
#define __TUNDRIVERAGTPROG_H__

#include <nifvar.h>

/**
TunDriver Agent Progress
*/
enum TTunDriverAgentProgress
	{
	ETunDriverAgtIdle             = KMinAgtProgress,
	ETunDriverAgtConnecting,      // for future reference
	ETunDriverAgtConnected        = KConnectionOpen,
	ETunDriverAgtDisconnecting    = KConnectionStartingClose,
	ETunDriverAgtDisconnected     = KConnectionClosed
	 
	};
#endif // __TUNDRIVERAGTPROG_H__
