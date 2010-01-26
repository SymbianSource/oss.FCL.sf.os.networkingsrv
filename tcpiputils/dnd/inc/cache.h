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
// cache.h - the record cache
//

#ifndef __CACHE_H__
#define __CACHE_H__
/**
@file cache.h
DNS cache
@internalComponent	Domain Name Resolver
*/

#include "record.h"		// ...for TDndRecordLRU (and indirectly class, type symbols and TInetAddr)

const TInt KDndMaxRecords = 100;	//< Maximum mumber of records allowed in the cache.

class CDndNameSpace;
class CDndEngine;

/**
// The cache maintains a collection of (question, answer) pairs.
//
// The question (= the search key for the cache) has the
// following components:
//
// @li namespace id
// @li domain-name (dotted notation)
// @li DNS query type (EDnsQType)
// @li DNS query class (EDnsQClass)
//
// The answer is contained within CDndRecord class. Among other
// things, it contains the full original reply from some DNS server
// for the question.
//
// The cache is implemented as a forest of trees, one tree
// for each name space. Each component of the domain-name
// becomes a node in the tree. A node at any non-leaf level can have
// zero or more answers attached (for. example, the domain name "nokia.fi"
// can have both "A" (address) and "NS" (name server) answers
// associated it.
//
// The leaf nodes must have at least one answer. When
// the last answer is expired or otherwise removed from a leaf node,
// the node itself is also removed. A tree containing no answers
// consists just of the root node (name space node).
//
// For example, if in the below example, the "A" answer associated with
// query (namespace=\\<global\\>, domain-name="www.nokia.com", query=A, class=IN)
// is removed, then also the nodes for "www" and "nokia" are pruned away.
@verbatim

                      cache
                  .............
              <global>          <local>
                / |              /  \
              fi com            |    |
             /    |  \         muu  foo
            /     |    \        |  /   \ 
        nokia  symbian nokia  ..A.A....AAAA... (lru)
       / |  \     |      |   .
    ..A..NS www  www    www .
           . |    |      | .
            .A....A......A
@endverbatim
// The tree implementation is not directly visible outside the cache
// implementation. The CDndCache::FindL returns only a pointer to the
// answer (CDndRecord).
//
// There is a limit to the number of answers (KDndMaxRecords) that
// can be stored. If this limit is reached, the <b>least recently used</b>
// answers are deleted, if possible. An answer cannot be deleted
// is it is currently being used (locked) by some resolver.
*/
class CDndCache : public CBase
	{
	// See implementation for methods comments!
public:
	CDndCache();
	void ConstructL();
	~CDndCache();

	TInt GetServerAddress(const TUint32 aNameSpace, TInetAddr &aAddr);
	void SetServerAddress(const TUint32 aNameSpace, const TInetAddr &aAddr, const TInt aState);
	CDndRecord *FindL(const TUint32 aNameSpace, const TDesC8 &aName, const EDnsQType aType, const EDnsQClass aClass, 
						const TTime &aReqTime);		
	void Dump(CDndEngine &aControl);
	void Flush();
private:
	CDndNameSpace *GetNameSpace(const TUint32 aNameSpace);
	// The roots of the name-spaces
	CDndNameSpace *iRootList;
	// The LRU list of cache records
	TDndRecordLRU iRecordList;
	};

#endif
