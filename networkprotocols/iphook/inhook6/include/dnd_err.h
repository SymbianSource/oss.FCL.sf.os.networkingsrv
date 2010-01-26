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
// dnd_err.h - name resolver error constants
// IPv6/IPv4 TCP/IP Errors to RHostResolver actions
//



/**
 @file dnd_err.h
 @publishedAll
 @released
*/
#ifndef __DND_ERR_H__
#define __DND_ERR_H__

// This file contains the error values that might be passed on to the applications by the DND

const TInt KErrDndNameNotFound		= -5120;	// Returned when no data found for GetByName
const TInt KErrDndAddrNotFound		= -5121;	// Returned when no data found for GetByAddr

const TInt KErrDndNoServers			= -5122;	// No DNS server addresses available (timeout)
const TInt KErrDndNoRoute			= -5123;	// Send timeout for the query (probably no route for server)

const TInt KErrDndCache				= -5124;	// Corrupted data in cache (= bad DNS reply from server)

// Errors mapped from the DNS reply. These are normally handled internally
// by the resolver code, and rarely, if ever, actually reach the RHostResolver
const TInt KErrDndFormat			= -5125;	// Wrong format
const TInt KErrDndServerFailure		= -5126;
const TInt KErrDndBadName			= -5127;	// Bad name
const TInt KErrDndNotImplemented	= -5128;
const TInt KErrDndRefused			= -5129;	// Server refused	

// Errors while processing response
const TInt KErrDndBadQuery			= -5130;	// Bad queryfrom application (invalid domain name, etc.), not processed
const TInt KErrDndNoRecord			= -5131;	// No record found of the desired type and class. 
const TInt KErrDndNameTooBig		= -5132;	// Buffer overflow with name
const TInt KErrDndUnknown			= -5133;	// Misc error - must be something wrong with the 
												// packet or the NS
const TInt KErrDndServerUnusable	= -5134;	// The server is unusable for the attempted query
												// (for example, not allowing recursion).

#endif
