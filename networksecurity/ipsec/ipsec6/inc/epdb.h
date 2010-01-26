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
// epdb.h - IPSEC Endpoint Database
// @internalComponent	for IPSEC
//



/**
 @internalComponent
*/
#ifndef __EPDB_H__
#define __EPDB_H__

#include <e32std.h>
#include "circular.h"

class TInetAddr;
class TIp6Addr;
class TIpAddress;
class CEndPoint;

// REndPoints
// **********
class REndPoints : RCircularList
	/**
	* Contains the Endpoint (internet addresses) database.
	*/
	{
	friend class RIpAddress;
public:
	 // The following is only available in DEBUG build
	 void LogPrint(const TDesC &aFormat) const;
	};


class RIpAddress
	/**
	* The handle for the End Point address.
	*
	* The name of the end point can only contain ASCII character (or to be
	* exact, codes in range 0..255)
	*/
	{
public:
	inline RIpAddress() : iAddr(NULL) {}
	RIpAddress(const RIpAddress &aAddr);
	RIpAddress &operator=(const RIpAddress &aAddr);
	~RIpAddress();

	TInt Open(REndPoints &aMgr, const TDesC &aName);
	TInt Open(REndPoints &aMgr, const TDesC &aName, const TIpAddress &aAddr, TInt aOptional = 0);
	TInt Open(REndPoints &aMgr, const TIpAddress &aAddr);
	TInt Open(REndPoints &aMgr, const TInetAddr &aAddr);
	TInt Set(const TIp6Addr &aAddr, TUint32 aScopeId);
	TInt Set(const TInetAddr &aAddr);
	void Close();
	const TIpAddress& operator()() const;
	TBool IsNamed() const;
	const TDesC8 &Name() const;
private:
	CEndPoint *iAddr;
	};

#endif
