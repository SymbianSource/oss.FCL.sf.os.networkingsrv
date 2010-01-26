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
// in6_dstcache.h - Destination cache (for transport protocols)
//












/**
 @file in6_dstcache.h
 @publishedPartner
 @released
*/

#ifndef __DSTCACHE_H__
#define __DSTCACHE_H__

#include <e32def.h>
#include <in_sock.h>
#include <apibase.h>

class TCacheInfo;

/**
* Interface that a hash key must implement. These functions are needed to determine the
* location of a data item in the hash table, and to indicate when two keys are equal.
*
* @publishedPartner
* @released
*/
class MHashKey
	{
public:
	/**
	* Generate a unsigned integer value from the key. It can be used, e.g. as an index
	* to the hash table. The integer generation must be deterministic.
	*/
	virtual TUint ToInt() = 0;
	
	/**
	* Return ETrue if this key is considered equal to the another key. Equality can be
	* a non-trivial concept, as can be seen in the implementation of THashKeyIp6 that
	* implements this virtual class.
	*/
	virtual TBool IsEqual(const MHashKey& aKey) = 0;
	};



/**
* This will be accessed using MInterfaceManager and the IMPORT_API_L mechanism.
* StoreL, etc. will use THashTable. Depending on the granularity parameter, the
* IP address is used as a hash key either directly or only by a portion of
* address prefix. The granularity prefix will be an ini parameter, for example:
*
* <i>dstcache= [0="do not use cache", 1="cache entry per each address",
* 2="cache entry per address prefix",
* 3="cache entry per network interface" (not yet implemented)]</i>
*
* In addition, another ini parameter, "dst_lifetime" will be defined. This gives
* the default lifetime for a cache entry (in seconds). 10 minutes could be a
* good default value, or should it be shorter (5 min?)?
*
* Additionally "dst_maxsize" gives the maximum size of the cache
* hash in bytes. 2048 would be a good default value.
*
* @publishedPartner
* @released
*/
class MDestinationCache : public MInetBase
	{
public:
	/**
	Store a cache entry with given destination address and data.
	The space for data object is
	allocated from heap and the object is copied there. If there was an existing
	cache object with the same key, it is overwritten without further warnings.
	If the cache is full and no expired objects can be removed, leaves with an
	error.
	
	@param aAddr	Destination address used by the other end.
	@param aInfo	Parameter values that should be associated with this address.
					iStoreTime is set in this function, so the caller does not have to
					set it.
	*/
	virtual void StoreL(const TInetAddr &aAddr, TCacheInfo &aInfo) = 0;

	/**
	Old interface for source backwards compatibility. This one has flawed design, because
	it does not specify scope ID. One should use TInetAddr - version instead.
	
	@deprecated
	*/
	inline void StoreL(const TIp6Addr &aAddr, TCacheInfo &aInfo);

	/**
	Tries to find data with given address. Returns NULL, if the data was not
	found, or it was expired.
	*/
	virtual const TCacheInfo *Find(const TInetAddr &aAddr) = 0;
	
	/**
	Old interface for source backwards compatibility. This one has flawed design, because
	it does not specify scope ID. One should use TInetAddr - version instead.
	
	@deprecated
	*/
	inline const TCacheInfo *Find(const TIp6Addr &aAddr);
	
	/**
	Removes given entry from destination cache.
	
	@param aAddr	Address that identifies the cache entry.
	*/
	virtual void RemoveL(const TInetAddr &aAddr) = 0;
	
	/**
	Modifies a single parameter of a cache entry, while maintaining the values
	of the other parameters. Leaves with error if the given cache entry is
	not found. The iStoreEntry is also updated to current time.
	
	<b>Example:</b>
	@code
	dstcache->SetL(address, EPathMTU, 536);
	@endcode
	
	@param aAddr		Destination address used by the other end.
	@param aParIndex	Index of paramter variable in TCacheInfo array.
	@param aValue		Value to be stored for the parameter.
	*/
	virtual void SetL(const TInetAddr &aAddr, TUint aParIndex, TUint32 aValue) = 0;

	/**
	Iterate through the hashtable and remove expired entries from it. RemoveIf()
	method in THashTable can be used for this purpose.
	*/
	virtual void Cleanup() = 0;

	/**
	Removes all entries from the destination cache.
	*/
	virtual void RemoveAll() = 0;

	/**
	Set the lifetime for cache entries in seconds.
	*/
	virtual void SetLifetime(TUint aLifetime) = 0;
	
	/**
	Set maximum size of the destination cache in bytes. If there are more than this
	much non-expired cache entries, new hash items are not added.
	*/
	virtual void SetMaxSize(TUint aMaxSize) = 0;
	
	/**
	Creates a destination cache instance into heap.
	
	@param aKeyMode	Indicates by an integer whether there will be separate cache
					entry per each address (=1), or common cache entry for addresses
					with same prefix (=2). The integer is equal to what is read from
					tcpip6.ini file for 'dstcache'.
	
	@return	Pointer to the initialized destination cache, or NULL if
	 				cache initialization failed.
	*/
	IMPORT_C static MDestinationCache *CreateDstCache(TInt aKeyMode);
	
	/**
	Checks whether the two addresses are mapped to the same destination cache entry.
	This takes the selected caching mode (per address or per network) into account.
	
	@return		ETrue if the addresses share same cache entry, EFalse if they use
				different entry.
	*/
	virtual TBool Match(const TInetAddr& aAddrA, const TInetAddr& aAddrB) const = 0;
	
	/**
	Causes the actual implementation destructor to be called.
	*/
	virtual ~MDestinationCache() { }
	};


/**
* The code could be something like following at the protocol SAP side
*
* <i>When opening a connection (This could be in the end of InitL() in TCP SAP)</i>:

@code
MInterfaceManager *ifacer = Interfacer();
MDestinationCache *dstcache = IMPORT_API_L(ifacer, MDestinationCache);
TIp6Addr& addr = iFlowContext->RemoteAddr().Ip6Address();
TCacheInfo *cache = dstcache->Find(addr);
if (cache)
	{
	iSsthresh = cache->iMetrics[ESsThresh];
	iSRTT = cache->iMetrics[ESRtt];
	}
@endcode

* <i>When closing a connection</i>:

@code
MDestinationCache *dstcache = ...IMPORT_API_L()...;
TIp6Addr& addr = iFlowContext->RemoteAddr().Ip6Address();
TCacheInfo cache;  // Should intialize to zero
cache.iMetrics[ESsThresh] = iSsthresh;
cache.iMetrics[ESRtt] = iSRTT;
dstcache->StoreL(addr, cache);
@endcode

* Questions:
* ----------
* 1) Should we assume that once cache entry is found, all values are valid?
*		- Probably not. Have to reserve NULL for "invalid" or "don't use"
*
* 2) Should we just override the earlier values in cache when a later connection
*	 is closed, or should some smarter smoothing methods be used for storing
*		- Just overriding with store would be a straightforward alternative
*		  to be implemented in the first place, but it may be subject to instability
*		  in stored values.
*
* 3) What kind of "safeguards" should there be when storing or applying values in
*	 cache? (probably rules something like "don't use ssthresh if it is <= 4"
*	 are needed)
*
* 4) Should there be some snapshots stored in the cache during lengthy
*	 TCP connections, or is it enough to store values only when closing
*	 TCP connection?
*/


/* -- Inline methods -- */

inline void MDestinationCache::StoreL(const TIp6Addr &aAddr, TCacheInfo &aInfo)
	{
	StoreL(TInetAddr(aAddr, 0), aInfo);
	}

inline const TCacheInfo *MDestinationCache::Find(const TIp6Addr &aAddr)
	{
	return Find(TInetAddr(aAddr, 0));
	}
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <in6_dstcache_internal.h>
#endif
#endif // __DST_CACHE_H__
