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
// inet6err.h - INET6 error codes
// Define TCPIP6 specific error codes.
// A range of unique error codes have been allocated for the IPSEC +
// INET6 related uses (by Martin Amstrong 21.10.1999):
// values:	-5100 .. -5228
// IPSEC will use about 30-50 values from -5228
// INET6 (and other modules) should start allocating codes
// in sequence from the other end of the range (-5100).
//



/**
 @file inet6err.h
 @publishedAll
 @released
*/

#ifndef __INET6ERR_H__
#define __INET6ERR_H__

#include <e32def.h>	// for TInt

/**
* @name IPv6 error constants
* @since v7.0 (unless indicated otherwise)
*/
//@{

/** Flow has no destination address. */
const TInt KErrInet6NoDestination = -5100;
/** Misconfigured driver not giving proper MTU. */
const TInt KErrInet6NoPathMtu = -5101;
/** A truncated IP packet */
const TInt KErrInet6ShortPacket = -5102;
/** Duplicate Address Detected on a device (see RFC-2462). */
const TInt KErrInet6DuplicateAddress = -5103;
/** The Source Address used by the connection has expired. */
const TInt KErrInet6AddressExpired = -5104;
/** No route was available. */
const TInt KErrInet6NoRoute = -5105;
/** Selected interface is beyond source scope (or invalid). @since v7.0s */
const TInt KErrInet6SourceAddress = -5106;

// Importing/Exporting interfaces (MInetBase) 

/** Requested API not known @since v7.0s */
const TInt KErrInetUnsupportedApi = -5107;
/** API is of an incompatible version  @since v7.0s */
const TInt KErrInetUnsupportedApiVersion = -5108;

/** DNS resolver server not available. @since v7.0s */
const TInt KErrInetNoDnsResolver = -5109;

		// -5102
		// -5103
		// ...
		// etc.
/** Start of IPv6 error code range */
const TInt KErrInet6First = -5228;
/**	End of IPv6 error code range */
const TInt KErrInet6Last = -5100;

//@}
#endif
