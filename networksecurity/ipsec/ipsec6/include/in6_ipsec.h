// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// in6_ipsec.h - definitions related to IPSEC sockets
// Define additional socket control api constants
//



/**
 @file in6_ipsec.h
 @internalTechnology
 @released
*/

#ifndef __IN6_IPSEC_H__
#define __IN6_IPSEC_H__

/**
* The level of the IPSEC socket options.
*/

#include <e32std.h>

const TUint KSolIpsecControl = 0x10000847;		// *note* this is the UID of IPSEC6.PRT

/**
* Get/Set the End Point address.
*
* The option data is TNameRecord. This definition allows easy use
* of RHostResolver results as a parameter to IPSEC end point.
*
* - iFlags are ignored
* - iName is the name of the end point
* - iAddr is the address contained in the end point (port is ignored).
*/
const TUint KSoIpsecEndpoint = 1;

#endif

