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
// hostname.h - hostnames of the local node
//



/**
 @internalComponent	Domain Name Resolver
*/

#ifndef _HOSTNAME_H_
#define _HOSTNAME_H_

#include <es_sock.h>

class TLocalHostMap;
class CLocalHostName;

class THostNames
	/**
	* Cache for the local host names.
	*
	* Maintains mapping of between configured local host names and
	* ids.
	*
	* There can be only one hostname for each id. The same name
	* can be mapped to multiple ids. Mapping a new name the same id,
	* overwrites the previous mapping.
	*/
	{
public:
	// Constructor, empty cache.
	THostNames() : iNameList(NULL), iMap(NULL) {}
	// Destructor.
	~THostNames();

	// Defines mapping between the name and id.
	TInt Map(TUint32 aId, const TDesC &aName);
	// Removes mapping for the id.
	void Unmap(TUint32 aId);
	// Returns the name associated with the id.
	const TDesC &Find(TUint32 aId) const;
	// Reset hostname mapping to initial state (possibly non-empty cache).
	TInt Reset(const TDesC &aLocalHost);
	// Refresh hostname mapping for the specific network.
	TInt Refresh(TUint32 aId);
private:
	void ReferenceRemoved(const CLocalHostName *aName);
	void Cleanup();
	void GetCommDbDataL(TUint32 aId);

	CLocalHostName *iNameList;			//< List of registered hostnames.
	CArrayFixFlat<TLocalHostMap> *iMap;	//< Mapping of <networkid,hostname>
	};

#endif
