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
// dstcache.cpp - Destination cache implementation
// Destination cache implementation
//



/**
 @file dstcache.cpp
*/

#include <in6_dstcache.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <in6_dstcache_internal.h>
#endif

#ifdef NONSHARABLE_CLASS
	NONSHARABLE_CLASS(CDestinationCache);
#endif

class CDestinationCache : public MDestinationCache, public CBase
	{
public:
	CDestinationCache(TInt aKeyMode);
//	~CDestinationCache();  // No explicit destructor needed, as far as I see
	
	void ConstructL();

	virtual void StoreL(const TInetAddr &aAddr, TCacheInfo &aInfo);
	
	virtual const TCacheInfo *Find(const TInetAddr &aAddr);
	
	virtual void RemoveL(const TInetAddr &aAddr);
	
	virtual void SetL(const TInetAddr &aAddr, TUint aParIndex, TUint32 aValue);

	virtual void Cleanup();
	
	virtual inline void RemoveAll()
	{ iStorage.RemoveAll(); }
	
	virtual inline void SetLifetime(TUint aLifetime) { iLifetime = aLifetime; }

	virtual inline void SetMaxSize(TUint aMaxSize) { iMaxSize = aMaxSize; }
	
	friend TBool IfIsExpired(const TCacheInfo &aCinfo, void *aCachePtr);
	
	virtual TBool Match(const TInetAddr& aAddrA, const TInetAddr& aAddrB) const;

private:
	TBool IsExpired(const TCacheInfo &aCinfo) const;

	TCacheHash	iStorage;	//< Hash table that stores the destination cache entries
	TUint		iLifetime;	//< Lifetime of a destination cache entry (seconds)
	TUint		iMaxSize;	//< Maximum memory used for cache entries (bytes)
	TUint		iMemUsed;	//< Current total memory used by cache entries

	// Key Mode selected when instantiating this class
	THashKeyIp6::TKeyMode	iKeyMode;
	};


EXPORT_C MDestinationCache *MDestinationCache::CreateDstCache(TInt aKeyMode)
	{
	CDestinationCache *dcache = new CDestinationCache(aKeyMode);
	if (dcache)
		{
		TInt err = KErrNone;
		// coverity[leave_without_push]
		// if ConstructL() leaves, the leave code is trapped in "err" and proper cleanup is done by freeing the memory alocated to "dcache"
		TRAP(err, dcache->ConstructL());
		if (err != KErrNone)
			{
			delete dcache;
			dcache = NULL;
			}
		}
	return dcache;
	}


CDestinationCache::CDestinationCache(TInt aKeyMode) :	CBase(),
														iStorage(KCacheHashSize),
														iLifetime(KDstCacheLifetime),
														iMaxSize(KDstCacheMaxSize)
	{
	switch (aKeyMode)
		{
	case 2:
		iKeyMode = THashKeyIp6::EPerNet;
		break;

	default:
		iKeyMode = THashKeyIp6::EPerHost;
		break;
		
		// EPerIface not yet implemented		
		}
	}
	
	
void CDestinationCache::ConstructL()
	{
	iStorage.ConstructL();
	}


void CDestinationCache::StoreL(const TInetAddr &aAddr, TCacheInfo &aInfo)
	{
	aInfo.iStoreTime.UniversalTime();
	THashKeyIp6 k(iKeyMode, aAddr);
	
	// Check that hash size does not exceed the given limit.
	// If it is about to do so, clean up expired entries and try again.
	// If it still fails, StoreL fails.
	if (iMemUsed >= iMaxSize)
		{
		Cleanup();  // This reduces iMemUsed if there was expired entries
		if (iMemUsed >= iMaxSize)
			{
			User::Leave(KErrNoMemory);  // TODO: change error code
			}
		}
	
	// Check if there is a non-expired entry in the cache. If incoming aInfo has any
	// non-null entries, they replace the earlier cache entries. Entries that were
	// not updated with this call remain unmodified in the cache.
	// However, the store time is refreshed.
	TCacheInfo *oldent = iStorage.Find(k);
	if (oldent && !IsExpired(*oldent))
		{
		for (TUint i = 0; i < KNumCacheMetrics; i++)
			{
			if (aInfo.iMetrics[i] != 0)
				oldent->iMetrics[i] = aInfo.iMetrics[i];
			}
		oldent->iStoreTime.UniversalTime();

		return;  // Hash-store method is not needed this time.
		}
	
	TUint mem = iStorage.StoreL(k, aInfo);
	iMemUsed += mem;
	}
	
	
void CDestinationCache::SetL(const TInetAddr &aAddr, TUint aParIndex, TUint32 aValue)
	{
	TCacheInfo *infoptr;

	// Overwrite existing value if it exists, otherwise create a new entry in cache
	THashKeyIp6 k(iKeyMode, aAddr);
	infoptr = iStorage.Find(k);

	if (infoptr && !IsExpired(*infoptr))
		{
		infoptr->iMetrics[aParIndex] = aValue;
		infoptr->iStoreTime.UniversalTime();
		}
	else
		{
		TCacheInfo newinfo;
		newinfo.ClearAll();
		newinfo.iMetrics[aParIndex] = aValue;
		StoreL(aAddr, newinfo);
		}
	}
	

const TCacheInfo *CDestinationCache::Find(const TInetAddr &aAddr)
	{
	THashKeyIp6 k(iKeyMode, aAddr);
	TCacheInfo *cinfo = iStorage.Find(k);
	
	// Check whether the cache entry is recent enough. If not, return NULL.
	// However, we don't delete an expired entry from cache. It remains as a placeholder
	// for future equivalent entries.
	if (!cinfo || IsExpired(*cinfo))
		{
		return NULL;
		}
	
	return cinfo;
	}
	

void CDestinationCache::RemoveL(const TInetAddr &aAddr)
	{
	THashKeyIp6 k(iKeyMode, aAddr);
	TUint mem = iStorage.RemoveL(k);
	iMemUsed -= mem;
	}


// Same as CDestinationCache::IsExpired(), but this is to be used with RemoveIf() call.
TBool IfIsExpired(const TCacheInfo &aCinfo, void *aCachePtr)
	{
	if (!aCachePtr)
		{
		return EFalse;
		}
	return ((CDestinationCache *)aCachePtr)->IsExpired(aCinfo);
	}


void CDestinationCache::Cleanup()
	{
	iMemUsed -= iStorage.RemoveIf(IfIsExpired, this);
	}


TBool CDestinationCache::IsExpired(const TCacheInfo &aCinfo) const
	{
	TTime now;
	TTimeIntervalSeconds secs;
	
	now.UniversalTime();
	if (now.SecondsFrom(aCinfo.iStoreTime, secs) != KErrNone)
		{
		// Overflow or something. Consider it as expired entry.
		return ETrue;
		}
	
	if ((TUint)secs.Int() > iLifetime)
		{
		// Expired
		return ETrue;
		}
	
	return EFalse;
	}
	

TBool CDestinationCache::Match(const TInetAddr& aAddrA, const TInetAddr& aAddrB) const
	{
	THashKeyIp6 ka(iKeyMode, aAddrA);
	THashKeyIp6 kb(iKeyMode, aAddrB);
	
	return ka.IsEqual(kb);
	}


// --- THashTable methods ---

template <class K, class V>
THashTable<K,V>::THashTable(TUint aSize) : iTable(NULL), iSize(aSize)
	{
	}


template <class K, class V>
THashTable<K,V>::~THashTable()
	{
	RemoveAll();
	delete [] iTable;
	}


template <class K, class V>
void THashTable<K,V>::ConstructL()
	{
	iTable = new TChain<K,V>[iSize];
	if (!iTable)
		{
		User::Leave(KErrNoMemory);
		}
	
	for (TUint i = 0; i < iSize; i++)
		{
		iTable[i].iNext = NULL;
		}
	}
	
	
template <class K, class V>
TUint THashTable<K,V>::StoreL(MHashKey& aKey, V& aValue)
	{	
	if (!iTable)
		{
		User::Leave(KErrGeneral);
		}

	TUint memused = 0;
	TInt idx = aKey.ToInt() % iSize;
	TChain<K,V> *ptr = &iTable[idx], *newitem = NULL;
	while (ptr->iNext)
		{
		if (ptr->iNext->iKey.IsEqual(aKey))
			{
			// The conventional mode would be:
			//     User::Leave(KErrAlreadyExists)
			// ...however, due to the lazy garbage collection mode adopted, we overwrite
			// the existing entry
			newitem = ptr->iNext;
			break;
			}
		ptr = ptr->iNext;
		}

	// If the key wasn't stored earlier, allocate space for new data item.
	// Allocations from heap is not always a very popular idea, but these events are
	// expected to occur quite rarely.
	if (!newitem)
		{
		newitem = new(ELeave) TChain<K,V>;

		if (!newitem)
			{
			User::Leave(KErrNoMemory);
			}
			
		newitem->iKey = *((K*) &aKey);
		newitem->iNext = NULL;
		ptr->iNext = newitem;
		memused = sizeof(TChain<K,V>);
		}
		
	newitem->iValue = aValue;
	
	/* Dynamically allocated memory newitem has been assigned to ptr->iNext and managed through the list.
	So CleanupStack::PopAndDestroy() will deallocate that memory. But, Coverity has misinterpreted it an issue.*/
	// coverity [SYMBIAN.CLEANUP STACK]
	// coverity [memory_leak]
	return memused;
	}
	
	
template <class K, class V>
V* THashTable<K,V>::Find(MHashKey& aKey)
	{
	if (!iTable) return NULL;

	TInt idx = aKey.ToInt() % iSize;
	TChain<K,V> *ptr = &iTable[idx];
	while (ptr->iNext)
		{
		if (ptr->iNext->iKey.IsEqual(aKey))
			{
			return &ptr->iNext->iValue;
			}
		ptr = ptr->iNext;
		}
		
	// Not found
	return NULL;
	}
	
	
template <class K, class V>
TUint THashTable<K,V>::RemoveL(MHashKey& aKey)
	{
	if (!iTable)
		{
		User::Leave(KErrGeneral);
		}

	TInt idx = aKey.ToInt() % iSize;
	TChain<K,V> *ptr = &iTable[idx];
	while (ptr->iNext)
		{
		if (ptr->iNext->iKey.IsEqual(aKey))
			{
			TChain<K,V> *removed = ptr->iNext;
			ptr->iNext = removed->iNext;
			delete removed;
			return sizeof(TChain<K,V>);
			}
		}
		
	// Key was not found
	User::Leave(KErrNotFound);
	return 0;
	}
	
	
template <class K, class V>
TUint THashTable<K,V>::Length()
	{
	if (!iTable) return 0;

	TUint length = 0;
	
	for (TInt idx = 0; idx < iSize; idx++)
		{
		TChain<K,V> *ptr = &iTable[idx];
		while (ptr->iNext)
			{
			length++;
			ptr = ptr->iNext;
			}
		}
	return length;
	}


template <class K, class V>
TUint THashTable<K,V>::RemoveIf(	TBool (*aRemoveCriteria)(const V&, void *),
									void *aDataObject)
	{
	if (!iTable)
		{
		return 0;
		}
	
	TUint memfreed = 0;
	TChain<K,V> *ptr = NULL;
	for (TUint idx = 0; idx < iSize; idx++)
		{
		ptr = &iTable[idx];
		while (ptr->iNext)
			{
			if (aRemoveCriteria(ptr->iNext->iValue, aDataObject))
				{
				TChain<K,V> *removed = ptr->iNext;
				ptr->iNext = removed->iNext;
				delete removed;
				memfreed += sizeof(TChain<K,V>);				
				}
			ptr = ptr->iNext;
			}
		}

	return memfreed;
	}


template <class K, class V>
void THashTable<K,V>::RemoveAll()
	{
	if (iTable != NULL)
		{
		TChain<K,V> *ptr = NULL, *last = NULL;
		for (TUint i = 0; i < iSize; i++)
			{
			// Remember that the first chain entry in &iTable[i] does not contain data.
			// The data items start from the linked entries.
			ptr = &iTable[i];
			while (ptr)
				{
				ptr = ptr->iNext;
				if (last) delete last;
				last = ptr;
				}
			}
		}	
	}

	
THashKeyIp6::THashKeyIp6(TKeyMode aKeyMode, const TInetAddr& aAddr)
	{
	// TODO: retrieve key mode, and assign only a prefix instead of full address.
	// Simple prefix length is not enough, because I'd like to use the same cache for
	// both IPv4 and IPv6
	iKeyMode = aKeyMode;
	iAddress = aAddr.Ip6Address();
	iScopeId = aAddr.Scope();
	}


TUint THashKeyIp6::ToInt()
	{
	if (iKeyMode == EPerNet)
		{
		if (iAddress.IsV4Mapped())
			{
			// IPv4: Network is considered a /24
			return iAddress.u.iAddr32[3] >> 8;
			}
		else
			{
			// IPv6: Network is considered a /64.
			// Return the least significant 32 bits from the network part.
			return iAddress.u.iAddr32[1];
			}
		}
	else  // EPerHost and others (EPerIface not yet implemented)
		{
		// The least significant 32 bits is used as the int index
		return iAddress.u.iAddr32[3];
		}
	}
	
	
TBool THashKeyIp6::IsEqual(const MHashKey& aKey)
	{
	// casting is always risky business, but the user is required
	// to use this method properly
	const TIp6Addr addr2 = ((const THashKeyIp6&)aKey).Ip6Address();
	
	// If scope id is different, match will always fail
	// (until per interface mode is implemented)
	if (iScopeId != ((const THashKeyIp6&)aKey).ScopeId())
		{
		return EFalse;
		}

	if (iKeyMode == EPerNet)
		{
		TInt match = iAddress.Match(addr2);
		if (iAddress.IsV4Mapped() && match >= 96 + 24)
			{
			return ETrue;
			}
		else if (match >= 64)
			{
			return ETrue;
			}
		return EFalse;
		}
	else  // EPerHost and others (EPerIface not yet implemented)
		{
		return iAddress.IsEqual(addr2);
		}
	}
