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
// cache.cpp - the record cache
//

// This file contains the implementations of the CDndCache object.
#include "engine.h"
#include "node.h"
#include "cache.h"
#include <networking/dnd_err.h>
#include "inet6log.h"

// Cache root node for a specific name space
class CDndNameSpace : public CDndNode
	{
public:
	CDndNameSpace(TUint32 aNameSpaceId) : CDndNode(NULL, 0, _L8("")), iNameSpaceId(aNameSpaceId), iState(1) {}
	~CDndNameSpace();
	const TUint32 iNameSpaceId;	//< The name space ID
	TInt iState;				//< Address state (initially = 1)
	TInetAddr iAddr;			//< Server Address and port (if any)
	CDndNameSpace *iNext;		//< Next name space root
	};

/**
// The destructor has really nothing functional to do.
// The precondition for destruct is that ALL nodes and
// records under the root node have already been destroyed.
*/
CDndNameSpace::~CDndNameSpace()
	{
	ASSERT(IsEmpty());
	}


// CDndCache::CDndCache
// ********************
CDndCache::CDndCache()
	{}


// CDndCache::ContstructL
// **********************
void CDndCache::ConstructL()
	{
	LOG(Log::Printf(_L("CDndCache::ConstructL() size=%d\r\n"), (TInt)sizeof(*this)));
	}


// CDndCache::~CDndCache
// *********************
/**
// Delete ALL records, and as a side effect all nodes
// (except the roots are freed). And, then delete all
// root nodes.
//
// If there are locked records, which prevent deletion
// of all nodes, then cache destructor has been called
// incorrectly! (=> ~CDndNameSpace() will panic)
*/
CDndCache::~CDndCache()
	{
	iRecordList.Cleanup(0);		// Delete all unlocked records
	//
	// Delete (now supposedly empty) root nodes
	//
	while (iRootList)
		{
		CDndNameSpace *ns = iRootList;
		iRootList = iRootList->iNext;
		delete ns;
		}
	}

// CDndCache::Flush
// ****************
/**
// Empty the cache.
*/
void CDndCache::Flush()
	{
	iRecordList.Cleanup(0);
	}

// CDndCache::GetServerAddress
// ***************************
/**
// Get currently known server address for the name space.
//
// The address state is inially 1 (> 0). It is only changed by the
// CDndCache::SetServerAddress .
//
// If the name-space does not have a root node, the root node
// is created by this call (CDndCache::GetNameSpaceL).
//
// @param	aNameSpace	the name space id
// @retval	aAddr		the server address currently associated with the name space
// @returns	the state of the returned address as:
//	@li	> 0, no address has been set
//	@li	= 0, address set
//	@li < 0, address set with error state, or
//	@li	KErrNoMemory, if needed to create the root node and failed
*/
TInt CDndCache::GetServerAddress(const TUint32 aNameSpace, TInetAddr &aAddr)
	{
	CDndNameSpace *const ns = GetNameSpace(aNameSpace);
	if (ns == NULL)
		return KErrNoMemory;

	aAddr = ns->iAddr;
	return ns->iState;
	}

// CDndCache::SetServerAddress
// ***************************
/**
// Set the server address/state for the name space.
//
// If the name-space does not have a root node, the root node
// is created by this call (CDndCache::GetNameSpaceL).
//
// @param	aNameSpace	the name space id
// @param	aAddr		the server address to be set
// @param	aState		the state of the address
*/
void CDndCache::SetServerAddress(const TUint32 aNameSpace, const TInetAddr &aAddr, const TInt aState)
	{
	CDndNameSpace *const ns = GetNameSpace(aNameSpace);
	if (ns != NULL)
		{
		ns->iAddr = aAddr;
		ns->iState = aState;
		}
	}

// CDndCache::GetNameSpace
// ***********************
/**
// Locate the name space root or try to create it if not found.
//
// @param	aNameSpace	the name-space id
// @returns	the name-space root or NULL, if not found and create
//			failed.
*/
CDndNameSpace *CDndCache::GetNameSpace(const TUint32 aNameSpace)
	{
	CDndNameSpace *ns = iRootList;

	// Does a request namespace already exist?
	for ( ; ns != NULL; ns = ns->iNext)
		if (ns->iNameSpaceId == aNameSpace)
			return ns;

	// Does not exist, a new namespace needed, create a new root
	ns = new CDndNameSpace(aNameSpace);
	if (ns)
		{
		ns->iNext = iRootList;
		iRootList = ns;
		}
	return ns;
	}


// CDndCache::FindL
// ****************
/**
// Find record from the cache.
//
// Finds the record from the cache which 
// matches the name, type and class. If it has expired, it is invalidated
// and returned. If no such record exists, one new record is inserted and
// returned.
//
// If the name-space does not have a root node, the root node
// is created by this call (CDndCache::GetNameSpaceL).
//
// @param	aNameSpace
//		the name-space id. If ZERO, only search is performed
//		across all currently existing name-spaces. No new
//		state is created.
// @param	aName
//		the fully qualified domain-name which owns the record (dotted format)			
// @param	aType
//		the record type
// @param	aClass
//		the record class
// @param	aReqTime
//		the time of the request. This is used to detect records whose TTL
//		is expired
// @returns
//		always a non-NULL pointer to the record matching (aNameSpace,aName,aType,aClass). The FindL
//		will create the necessary node structures, if they do not exist before the call. If the
//		record didn't exist before, the initial state of the new record is "Invalid".
// @exception	KErrNoMemory
//		the main potential reason for FindL to fail (not enough memory to create the objects)
// @exception	KErrDndNoRecord
//		can occur only when called with aNameSpace == 0, and the record does not exist.
*/
CDndRecord * CDndCache::FindL(const TUint32 aNameSpace, const TDesC8 &aName, const EDnsQType aType, const EDnsQClass aClass, 
								const TTime &aReqTime)
	{
	CDndRecord *record = NULL;

	if (aNameSpace == 0)
		{
		// If no name space is specified, then just search for match, never add record
		for (CDndNameSpace *ns = iRootList; ; ns = ns->iNext)
			{
			if (ns == NULL)
				User::Leave(KErrDndNoRecord);
			record = ns->FindL(aName, FALSE, aType, aClass, iRecordList, aReqTime);
			if (record)
				break;
			}
		// record != NULL here, ALWAYS!
		}
	else
		{
		// When namespace is defined, always create the record and other necessary structures
		// if neeeded.
		//
		iRecordList.Cleanup(KDndMaxRecords);
		// Locate or create the name-space root
		CDndNameSpace *const ns = GetNameSpace(aNameSpace);
		if (ns == NULL)
			User::Leave(KErrNoMemory);
		// Locate (or create) the matching record
		record = ns->FindL(aName, TRUE, aType, aClass, iRecordList, aReqTime);
		// record != NULL here ALWAYS!
		}

	record->HitLRU();
	return record;
	}


#ifdef _LOG
// CDndCache::Dump
// ***************
/**
// Print out the entire cache (for DEBUG only)
*/
void CDndCache::Dump(CDndEngine &aControl)
	{
	if (!iRootList)
		aControl.ShowText(_L("No cache in the system"));
	else
		for (CDndNameSpace *ns = iRootList; ns != NULL; ns = ns->iNext)
			ns->Print(aControl);
	}
#endif
