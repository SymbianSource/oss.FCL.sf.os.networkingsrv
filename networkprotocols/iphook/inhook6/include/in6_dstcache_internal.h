/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Interface Manager Standard Variable Names
* 
*
*/



/**
 @file in6_dstcache_internal.h
 @internalTechnology
 @released
*/

#ifndef __DSTCACHE_INTERNAL_H__
#define __DSTCACHE_INTERNAL_H__

#include <in6_dstcache.h>

/**
* @file in6_dstcache_internal.h
* Destination cache. It will be used mostly by transport
* protocols (primarily TCP) only. The core functionality in iface.cpp does not need
* to care about it at all, except when doing Path MTU discovery, which is also
* stored in the cache.
*
* The cache implementation consists of a hashtable
* that uses destination IP addresses as key values and a struct of stored metrics
* as data. When a (TCP) connection is opened, the SAP checks whether there are
* stored values to be used as initial parameters in the cache. If a matching cache
* entry is found, the (TCP) sender uses those as initial values, otherwise it adopts
* the normal initial values conventionally. Correspondingly, when the connection is
* about to be closed, the current values in
* TCP SAP variables are stored in the cache to be used for future connections.
*
* The destination cache is meant to be protocol-independent so that also other
* transport protocols than TCP (such as a possible SCTP implementation) could
* use the variables in the cache.
*
* The cache entries have lifetime, and the cache entries can be removed
* after timer expires. Instead of actively removing cache entries,
* we implement a "lazy" method: The implementation remembers the time
* of last store event for each cache entry, and checks the validity of cache
* entries upon certain events, for example when accessing the cache item next time,
* and removes the entry if necessary. The cache has limited max size.
*
* @internalTechnology
* @released
*/

/** One node in THashTable.
@internalTechnology
@released
*/
template <class K, class V>
class TChain
	{
public:
	K			iKey;
	V			iValue;
	TChain<K,V>	*iNext;
	};

#ifdef NONSHARABLE_CLASS
	NONSHARABLE_CLASS(THashKeyIp6);
#endif

/**
* A hash key based on IPv6 addresses. There are three operation modes:
* @li	each IP address forms its own equivalence class.
* @li	IP addresses in the same network belong to the same equivalence class
*		Since we often do not
*		have the information about the network masks at the destination, we
*		have to do some kind of assumptions. Therefore, IPv6 addresses are considered
*		to have /64 prefix and IPv4 addresses have /24 prefix.
*		(Note that occasionally having slightly wrong network prefixes here is not
*		too bad a failure, if the general direction is correct).
* @li	Addresses delivered via a particular network interface belong to the same
*		equivalence class. Here we have unsolved implementation issues and
*		other considerations, so presently this remains a TODO item. Motivation of this
*		is the assumption that in the GPRS world the last-hop wireless link is
*		the bottleneck that determines the TCP connection characteristics.
*
* @internalTechnology
* @released
*/
class THashKeyIp6 : public MHashKey
	{
public:
	enum TKeyMode
		{
		EPerHost,		///< Each host has a dedicated entry in hash table.
		EPerNet,		///< Hosts with the same network prefix have a common entry
						///< in the table.
		EPerIface		///< All destinations with same network interface share an
						///< entry in the table (not yet implemeted).
		};


	THashKeyIp6() { };
	THashKeyIp6(TKeyMode aMode, const TInetAddr& aAddr);
	virtual TUint ToInt();
	virtual TBool IsEqual(const MHashKey& aKey);
	inline const TIp6Addr& Ip6Address() const { return iAddress; }
	inline TUint32 ScopeId() const { return iScopeId; }

private:
	TKeyMode	iKeyMode;	///< Key Mode of this key.
	TIp6Addr	iAddress;	///< IP address represented by this key.
	TUint32		iScopeId;	///< Scope ID of the destination entry.
	};
	

/**
* Generic hashtable for storing keys with associated values. K must
* be a subclass of class MHashKey.
*
* @internalTechnology
* @released
*/
template <class K, class V>
class THashTable
	{
public:
	THashTable(TUint aSize);
	~THashTable();

	void ConstructL();

	/**
	* Store key and the associated value to the hashtable. The cache operates in
	* overwrite mode: if key exists, the earlier value is overwritten with new one.
	* A future work item might be to implement the conventional mode, that would
	* raise an exception when the key already exists.
	*
	* @return	Number of bytes allocated from memory with this call. 0 indicates that
	*			there was already an equivalent entry in the cache that was overwritten.
	*			If an error occurred (e.g. out of memory) the call leaves with an
	*			appropriate error code.
	*/
	TUint StoreL(MHashKey& aKey, V& aValue);

	V* Find(MHashKey& aKey);

	/**
	* Deletes a data item from hash and releases the memory allocated.
	*
	* @return	Number of bytes freed from memory.
	*/
	TUint RemoveL(MHashKey& aKey);
	
	/// How many objects are stored in Hash table.
	TUint Length();

	/// How many bytes does the hash table take. This assumes all data objects are
	/// equally sized.
	inline TUint Size() { return Length() * sizeof(TChain<K,V>); }
		
	/**
	* Iterate through hashtable and exectue aRemoveCriteria function for all
	* entries. Delete entries for which the function returns ETrue.
	*
	* @param aRemoveCriteria	Function that returns ETrue if the data item given as
	*							the parameter for the function should be removed.
	*
	* @param aDataObject		Data pointer that is passed to the remove criteria
	*							function. The meaning of data pointer is determined
	*							by the criteria function.
	*							See CDestinationCache::Cleanup for example on how this
	*							function is used.
	*
	* @return Total number of bytes deleted
	*/
	TUint RemoveIf(TBool (*aRemoveCriteria)(const V&, void *), void *aDataObject);
	
	/**
	* Removes all data from the hashtable.
	*/
	void RemoveAll();
	
private:
	TChain<K,V>	*iTable;	///< Array of size specified in constructor.
	TUint		iSize;		///< Size of iTable array.
	};
	

const TUint KNumCacheMetrics = 16;

/** Information that is stored for each entry in destination cache.

This is the data part of THashTable entries.

@internalTechnology
@released
*/
class TCacheInfo
	{
public:
	TTime		iStoreTime;					///< When was this entry stored.
	TUint32		iMetrics[KNumCacheMetrics];	///< Data elements indexed by TCacheIndex.
	
	/// Fills all metrics with zeros.
	inline void ClearAll() { for (TUint i = 0; i < KNumCacheMetrics; i++) iMetrics[i] = 0; }
	
	enum TCacheIndex
		{
		EPathMTU	= 0,	///< Result of Path MTU discovery (Bytes).
		ESsThresh	= 1,	///< Slow start threshold (Bytes).
		ESRtt		= 2,	///< Smoothed RTT estimate (Ms).
		ERto		= 3		///< Retransmission timer estimate (Ms).
							//  (is this needed, since we have srtt?)							
		};
	};

const TUint KApiVer_MDestinationCache = 3;


typedef THashTable<THashKeyIp6, TCacheInfo> TCacheHash;
const TUint KCacheHashSize = 11;

const TUint KDstCacheLifetime 	= 600;	///< Default entry lifetime = 10 min.
const TUint KDstCacheMaxSize	= 2048;	///< Default cache max size = 2 KB.


#endif // __DSTCACHE_INTERNAL_H__
