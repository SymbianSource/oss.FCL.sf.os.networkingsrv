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
// record.cpp - the queries and replies from the server
// This file contains the implementations for the CDndRecord and TDndRecordList objects. 
//

#include "record.h"
#include "node.h"
#include <networking/dnd_err.h>
#include "inet6log.h"
#include "engine.h"

const TInt CDndRecord::iOffset = _FOFF(CDndRecord, iDlink);
const TInt CDndRecord::iOffsetLRU = _FOFF(CDndRecord, iLRU);

// Default constructor
CDndRecord::CDndRecord(TDndRecordLRU &aLRU, CDndNode &aOwner, const EDnsQType aType, const EDnsQClass aClass)
 : iOwner(aOwner), iList(aLRU), iType(aType), iClass(aClass), iErrorCode(KErrNotFound)
	{
	//
	// Maintain LRU list state
	//
	iList.AddFirst(*this);
	iList.iCount += 1;
	}

CDndRecord::~CDndRecord()
	{
	//
	// Maintain LRU list state
	//
	iLRU.Deque();	// All records are always in LRU
	iList.iCount -= 1;
	}

// CndRecord::Unlock
// *****************
/**
//	Unlocks the record (decrement reference count).
//
//	When the reference count reaches ZERO, and if record has no TTL set or
//	has expired, the record is removed.
//
//	If iHasTTL == 0, then RR has been created, but no query results
//	have been received. If the querier quits before any answers become
//	available, there is no point in keeping the empty record in cache.
//
//	If iExpired == 1, then RR has been detected as expired, but could
//	not be released due to being locked.
//
// @param aWorker	Unlocking instance
*/
void CDndRecord::Unlock(const void *aWorker)
	{
	ASSERT(iLocks > 0);
	// Cancel "being worked on" state, if this unlocking
	// instance had registered as working on request.
	if (iWorker == aWorker)
		iWorker = NULL;

	if (--iLocks == 0)
		{
		ASSERT(iWorker == NULL);
		// The last user exited. If the record has no data (initial state)
		// or has been marked as expired, then delete the record.
		if (iHasTTL == 0 || iExpired == 1)
			Delete();
		}
	}


// CndRecord::AssignWork
// *********************
/**
// Assign a "worker" to record, if not already present.
//
// The caller (= aWorker) wants to start the DNS querying process for
// this record.
//
// @return
//	@li ETrue, if worker has been assigned (and queries can be started)
//	@li EFalse, if some other worker has already been assigned (no queries need to be started)
*/
TBool CDndRecord::AssignWork(const void *aWorker)
	{
	ASSERT(iLocks > 0);
	if (iWorker == NULL)
		{
		iWorker = aWorker;
		return ETrue;
		}
	// Return also true, if this same worker is already
	// assigned, and otherwise false (= someone else is working on this).
	return iWorker == aWorker;
	}


// CDndRecord::HitLRU
// ******************
/**
//	Moves the record to first position in the LRU list.
*/
void CDndRecord::HitLRU()
	{
	//
	// Maintain LRU list state
	//
	iLRU.Deque();
	iList.AddFirst(*this);
	}

// CDndRecord::Delete
// ******************
void CDndRecord::Delete()
	{
	iOwner.DeleteRecord(this);
	}

// CDndRecord::Expiretime
// **********************
/**
// @returns	a reference to expiration time of this record
*/
const TTime &CDndRecord::ExpireTime() const
	{
	return iTTL;
	}


// CDndRecord::Invalidate
// **********************
/**
// "Invalid record" is just a record that contains no actual
// data (yet). It is the initial state of the newly created
// records.
*/
void CDndRecord::Invalidate()
	{
#ifdef _LOG
	if (iErrorCode != KErrNotFound)
		{
		TBuf<KDnsMaxName> name;
		Reply().GetName(sizeof(TInet6HeaderDNS), name);
		Log::Printf(_L("%S: QType: %d, QClass: %d [%d] Invalidated\r\n"), &name, iType, iClass, (TInt)(Header().ID()));
		}
#endif
	delete iReply.iBuf;
	iReply.iBuf = NULL;
	iHasTTL = 0;
	iErrorCode = KErrNotFound;
	}

// CDndRecord::FillErrorCode
// *************************
/**
// @param	aErrorCode	then new error code for the record
// @param	aTTL
//		the "Time To Live" time for the record in seconds. This
//		converted into real expiration time by adding it into
//		current time.
*/
void CDndRecord::FillErrorCode(const TInt aErrorCode, const TUint aTTL)
	{
	if (aErrorCode == KErrNotFound)
		Invalidate();
	else
		{
		// Transfer the TTL to absolute time.
		const TTimeIntervalSeconds interval = aTTL;
		iTTL.UniversalTime();
		iTTL += interval;
		iErrorCode = aErrorCode;
		iHasTTL = 1;
		}

	}


// CDndRecord::FillData
// ********************
/**
// Store the reply into record and compute the TTL for the record
// from the TTL's of contained RR's (which match the query). If the
// reply has no matching RR's, then the KDndDefaultTTL is used.
//
// The record is marked as "valid" with error code KErrNone.
//
// @param	aReply
//		contains the reply for the query represented by this record
// @param	aAnswerOffset
//		indication the starting position of the answer section in the
//		reply.
// @param	aServer
//		id of the server from which reply comes (note that for the Multicast DNS, this
//		id identifies the multicast group, and replies from different hosts are tagged
//		with the same id).
// @param	aErrorCode
//		The error code
// @param	aTTL
//		The default time to live, if RR's don't specify otherwise.
*/
void CDndRecord::FillData(const TMsgBuf &aReply, const TInt aAnswerOffset, const TUint32 aServer, TInt aErrorCode, TUint aTTL)
	{
	TUint32 max_ttl = KDndMaxTTL;

	if (aErrorCode == KErrNone)
		{
		// Compute TTL of the record from the minimum TTL
		// of the RR's that match exactly the iQType and iQClass
		TDndRR rr(aReply);
		TInt next = 0;
		TInt answerCount = ((TInet6HeaderDNS *)(aReply.Ptr()))->ANCOUNT();
		for (;;++next)
			{
			const TInt ret = rr.FindRR(aAnswerOffset, answerCount, iType, iClass, next);

			if (ret == KErrDndCache)
				// The reply is corrupt, do not store into cache, do
				// not modify the currently cached value, if any exists.
				return;
			if (ret < 0)
				break;	// No more rr's
			if (EDnsQClass(rr.iClass) != iClass || EDnsQType(rr.iType) != iType)
				break;	// No more matching RR's (FindRR returns first all matching ones)
			if (max_ttl > rr.iTTL)
				max_ttl = rr.iTTL;
			}
		if (next == 0)
			{
			// No RR's that matched the query or there was no anser RR's.
			// Use the supplied default TTL.
			max_ttl = aTTL;
			}
		}
	else
		max_ttl = aTTL;	// Just use the default for any error codes.

	delete iReply.iBuf;
	iReply.iBuf = HBufC8::New(aReply.Length());
	if (iReply.iBuf)
		{
		TPtr8 buf = iReply.iBuf->Des();
		buf = aReply;
		iReply.iOffset = aAnswerOffset;
		iServer = aServer;
		FillErrorCode(aErrorCode, max_ttl);
		}
	else
		{
		// This is internal memory allocation problem,
		// use just default TTL
		FillErrorCode(KErrNoMemory, KDndDefaultTTL);
		}
	}

#ifdef _LOG
// CDndRecord::Print
// *****************
/**
// (This method is currently only used in DEBUG compile)
//
// @param aControl	provides the actual "output device"
*/
#ifdef SYMBIAN_DNS_PUNYCODE
void CDndRecord::Print(CDndEngine &aControl, TBool aIdnEnabled) const
#else
void CDndRecord::Print(CDndEngine &aControl) const
#endif //SYMBIAN_DNS_PUNYCODE
	{
	TInt err;
	TBuf<KDnsMaxName> buf;
	if (iErrorCode == KErrNotFound)
		{
		aControl.ShowText(_L("Invalid Record."));
		return;
		}
#ifdef SYMBIAN_DNS_PUNYCODE
		Reply().GetName(sizeof(TInet6HeaderDNS), buf, aIdnEnabled);
#else
		Reply().GetName(sizeof(TInet6HeaderDNS), buf);
#endif //SYMBIAN_DNS_PUNYCODE

	const TInt answerCount = Header().ANCOUNT();

	aControl.ShowTextf(_L("%S: QType: %d, QClass: %d, Err: %d [%d %d/%d/%d AA=%d TC=%d RA=%d RCODE=%d]"),
		&buf,
		iType, iClass, iErrorCode,
		(TInt)(Header().ID()),
		answerCount, (TInt)Header().NSCOUNT(), (TInt)Header().ARCOUNT(),
		(TInt)(Header().AA() != 0),
		(TInt)(Header().TC() != 0),
		(TInt)(Header().RA() != 0),
		(TInt)(Header().RCODE()));

	TRAP(err, iTTL.FormatL(buf, _L(" | Expires: %F%D.%M.%Y %H:%T:%S")));
	aControl.ShowText(buf);

	TDndRR rr(TMsgBuf::Cast(*iReply.iBuf));
#ifdef SYMBIAN_DNS_PUNYCODE
	rr.iIdnEnabled = aIdnEnabled;
#endif //SYMBIAN_DNS_PUNYCODE
	TInt next = 0;
	for (;;++next)
		{
		if (rr.FindRR(iReply.iOffset, answerCount, iType, iClass, next) < 0)
			break;	// No more rr's

		TInetAddr addr;
		rr.GetResponse(buf, addr);
		TBuf<50> tmp;
		addr.OutputWithScope(tmp);
		aControl.ShowTextf(_L("   + RR: %S Type: %d, Class: %d, TTL: %d [%S]"), &buf, (int)rr.iType, (int)rr.iClass, (int)rr.iTTL,
			&tmp);
		}
	}
#endif

TDndRecordLRU::TDndRecordLRU() : TDblQue<CDndRecord>(CDndRecord::iOffsetLRU)
	{
	}

// TDndRecordLRU::Cleanup
// **********************
/**
//	Limit the number of records in the LRU to the
//	specified aMaxRecords. Deletes excess records
//	from the end of the list.
//
//	Locked records are not deleted and are left
//	in the list.
//
// @param	aMaxRecords
//		the maximum number of records that should be
//		on the list. This is "advisory", if there are
//		locked records, Cleanup may not be able to
//		comply fully with the request.
*/
void TDndRecordLRU::Cleanup(const TUint aMaxRecords)
	{
	LOG(Log::Printf(_L("LRU Cleanup(max=%d) records in cache=%d"), aMaxRecords, iCount));
	TUint locked = 0;
	while (locked < iCount && iCount > aMaxRecords)
		{
		CDndRecord *const record = Last();
		if (record->IsLocked())
			{
			// Oops... the last record is locked, cannot remove!
			// Shunt to first, and try next one...
			record->HitLRU();
			locked += 1;
			// A special kludge: if aMaxRecords == 0, all records really
			// should be deleted. Can't delete locked ones now, so just
			// mark them as 'expired', so that new queries don't use them.
			if (aMaxRecords == 0)
				record->MarkExpired();
			}
		else
			{
			// This Delete *WILL* decrease iCount!
			record->Delete();
			}
		}
	LOG(Log::Printf(_L("LRU Cleanup finished, records in cache=%d (at least %d locked)"), iCount, locked));
	}


TDndRecordList::TDndRecordList() : TDblQue<CDndRecord>(CDndRecord::iOffset)
	{}

#ifdef _LOG
// TDndRecordList::Print
// *********************
/**
// (This method is currently only used in DEBUG compile)
//
// @param aControl	provides the actual "output device"
*/
void TDndRecordList::Print(CDndEngine &aControl) 
	{
	if (IsEmpty())
		aControl.ShowText(_L("RecordList: Empty List"));
	else
		{
		aControl.ShowText(_L("RecordList: "));
		CDndRecord *record;
		TDblQueIter<CDndRecord> iter(*this);
		while((record = iter++) != NULL)
			{
#ifdef SYMBIAN_DNS_PUNYCODE
			record->Print(aControl); // by default the name shall be displayed in ASCII format. 
#else
			record->Print(aControl);
#endif // SYMBIAN_DNS_PUNYCODE
			}
		}
	}
#endif

// TDndRecordList::Find
// ********************
/**
// Locate a record of matching type and class.
//
// Search the record list for a matching type and class. If
// an expired record is found, empty and reuse it.
//
// Note: this does not release expired record with no users. Just
// let the normal LRU cleanup handle them.
//
// @param	aType of the record to be located
// @param	aClass of the record to be located
// @param	aReqTime of the query (current time or very close to it)
// @returns
//	@li	NULL, if no matching record exists
//	@li non-NULL record pointer, if a matching record exists
*/
CDndRecord * TDndRecordList::Find(const EDnsQType aType, const EDnsQClass aClass, const TTime &aReqTime)
	{
	if (IsEmpty())
		return NULL;
	
	CDndRecord *record;
	TDblQueIter<CDndRecord> iter(*this);
	while ((record = iter++) != NULL)
		{
		if (record->iExpired == 0 && record->iType == aType && record->iClass == aClass)
			{
			if (record->iHasTTL && record->ExpireTime() < aReqTime)
				{
				// The record has expired,
				if (record->IsLocked())
					{
					// The record is in use by someone, cannot reuse it.
					// Mark it as "expired", so that it will be automaticly
					// deleted when the last current user releases it.
					record->iExpired = 1;
					// A new duplicate RR will be created.
					continue;
					}
				// The record has expired, but nobody is using it for now.
				// Just reset to empty and use the existing RR.
				record->Invalidate();
				}
			break;
			}
		}
	return record;
	}

// TDndRecordList::Add
// *******************
/**
// @param	aRecord to be insersted into list.
*/
void TDndRecordList::Add(CDndRecord &aRecord)
	{
	AddFirst(aRecord);
	}

// TDndRecordList::Delete
// **********************
/**
//	@param	aRecord to be deleted (must be a member of this list)
*/
void TDndRecordList::Delete(CDndRecord *aRecord)
	{
	aRecord->iDlink.Deque();
	delete aRecord;
	}
