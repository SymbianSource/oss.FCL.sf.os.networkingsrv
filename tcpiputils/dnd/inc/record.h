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
// record.h - the queries and replies from the server
//

#ifndef __RECORD_H__
#define __RECORD_H__

/**
@file record.h
Describes the stored answers (in DNS Cache)
@internalComponent	Domain Name Resolver
*/

#include "message.h"

#ifdef EXCLUDE_SYMBIAN_DNS_PUNYCODE
#undef SYMBIAN_DNS_PUNYCODE
#endif //EXCLUDE_SYMBIAN_DNS_PUNYCODE

const TUint KDndMaxTTL = 604800;	//< Maximum TTL for an RR in seconds == 1 week.
const TUint KDndDefaultTTL = 120;	//< Default TTL, if reply has no RR that matches the query

class CDndEngine;
class CDndNode;
class TDndRecordLRU;

class TDnsReply
	{
public:
	TInt iOffset;			//< Offset of the first answer
	HBufC8 *iBuf;			//< The reply from the DNS server as is
	};

// Represents the query and reply from the server
class CDndRecord : public CBase
	{
	friend class TDndRecordList;
	friend class CDndCache;		// for accessing iLRU!

	~CDndRecord();
public:
	CDndRecord(TDndRecordLRU &aLRU, CDndNode &aOwner, const EDnsQType aQType, const EDnsQClass aClass);
	// Returns the expiration time
	const TTime &ExpireTime() const;
	// Invalidate the record.
	void Invalidate();
	// Record a reference into LRU list
	void HitLRU();
	// Mark record as expired.
	inline void MarkExpired() { iExpired = 1; }
	/*
	// Mark record as "valid", set error code and TTL.
	//
	// @param aErrorCode
	//	The error code to set. This must be negative, if there is no
	//	reply. Also, if the code is KErrNotFound, the function actually
	//	invalidates the record.
	// @param aTTL
	//	The time to live in seconds, counted from current time.
	*/
	void FillErrorCode(const TInt aErrorCode, const TUint aTTL = KDndMaxTTL);
	// Mark record as "valid", containing the received reply.
	void FillData(const TMsgBuf &aReply, const TInt aAnswerOffset, const TUint32 aServer, TInt aErrorCode, TUint aTTL);
	/**
	// Return current error code of the record
	//
	// @return Error Code,
	// @li = 0 (KErrNone), record has a reply stored
	// @li = KErrNotFound, record is Invalid, no reply stored
	// @li < 0, some other error condition, no reply stored
	*/
	inline TInt ErrorCode() const { return iErrorCode;}	
	/**
	// Return a reference to the reply associated with the record.
	//
	// Only valid if ErrorCode() returns KErrNone!
	*/
	inline const TMsgBuf &Reply() const { return TMsgBuf::Cast(*iReply.iBuf); }
	/**
	// Return a reference to the reply associated with the record type cast into header class
	//
	// Only valid if ErrorCode() returns KErrNone!
	*/
	inline const TInet6HeaderDNS &Header() const { return *((TInet6HeaderDNS *)iReply.iBuf->Ptr()); }
	// Return the offset to the answer section of the reply
	inline TInt AnswerOffset() const { return iReply.iOffset; }
	// Return the id of the server from which the reply came
	inline TUint32 Server() const { return iServer; }

	// Increment record lock count (prevent remove via LRU handling)
	inline void Lock() { ++iLocks; }
	// Assign a "worker" to record, if not already present.
	TBool AssignWork(const void *aWorker);
	// Decrement record lock count (allow remove via LRU when 0)
	void Unlock(const void *aWorker);
	// Return TRUE, if lock count > 0
	inline TBool IsLocked() const { return iLocks != 0; }
	// Return Query Type
	inline EDnsQType QType() const { return iType; }
	// Return Query Class
	inline EDnsQClass QClass() const { return iClass; }
	// Delete the record from cache
	void Delete();
	
#ifdef SYMBIAN_DNS_PUNYCODE	
	// Print out the record content (debug only)
	void Print(CDndEngine &aControl,TBool aIdnEnabled=EFalse) const;	
#else
	// Print out the record content (debug only)
	void Print(CDndEngine &aControl) const;
#endif //SYMBIAN_DNS_PUNYCODE

	static const TInt iOffsetLRU;	//< offset of iLRU

protected:	
	static const TInt iOffset;		//< offset of iDlink

private:
	CDndNode &iOwner;		//< Back link to the node owning this record
	TDndRecordLRU &iList;	//< Back link to the LRU list head
	const EDnsQType iType;	//< Record Type
	const EDnsQClass iClass;//< Record Class

	TDblQueLink iDlink;		//< Links records under single label
	TTime iTTL;				//< Time to live for this cache record
	TUint32 iServer;		//< The id of originating server
	TDnsReply iReply;		//< The cached reply or replies.
	TInt iErrorCode;		//< KErrNone, if record stored, < 0 otherwise
	TUint16 iLocks;			//< Number of locks by TDndReqData objects
	TUint iHasTTL:1;		//< The iTTL on the record has been set.
	TUint iExpired:1;		//< The record has expired, but is locked.
	TDblQueLink iLRU;		//< LRU links
	const void *iWorker;	//< Non-NULL, if querying process is active
	};


// A Least Recently Used (LRU) list of records
class TDndRecordLRU : public TDblQue<CDndRecord>
	{
	friend class CDndRecord;
public:
	TDndRecordLRU();
	// Remove extra records from LRU
	void Cleanup(const TUint aMaxRecords);
private:
	TUint iCount;			//< Count of records in LRU list
	};

// A list of records under one node
class TDndRecordList : public TDblQue<CDndRecord>
	{
	friend class CDndNode;

	TDndRecordList();

	// Searches the list for the Record of type aType and class aClass
	CDndRecord *Find(const EDnsQType aType, const EDnsQClass aClass, const TTime &aReqTime);

	// Add a new record into list
	void Add(CDndRecord &aRecord);
	// Delete record from list (and cache)
	void Delete(CDndRecord *aRecord);
	// Call CDndRecord::Print for each member of the list.
	void Print(CDndEngine &aControl);
	};


#endif
