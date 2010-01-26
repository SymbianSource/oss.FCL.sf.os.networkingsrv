// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// ETel Multimode API
// 
//

#include "mmlist.h"
#include "ETELEXT.H"

/********************************************************************/
//
// CMobilePhoneListBase
// Base class of thin-template idiom for ETel list classes
//
/********************************************************************/

CMobilePhoneListBase::CMobilePhoneListBase(TInt aLength, TInt aGranularity) : 
	iList(aLength,aGranularity), 
	iMaxNumber(KMaxEntriesNotSet)
	{
	}

CMobilePhoneListBase::~CMobilePhoneListBase()
	{
	}

EXPORT_C CBufBase* CMobilePhoneListBase::StoreLC()
/**
 * This method stores the streamed contents of the list into a CBufFlat
 *
 * \return CBufBase* Pointer to the allocated CBufFlat
 */
	{
	CBufFlat* buf=CBufFlat::NewL(4);
	CleanupStack::PushL(buf);
	RBufWriteStream strm(*buf);
	strm << *this;
	strm.CommitL();
	return buf;
	}

EXPORT_C void CMobilePhoneListBase::StoreL(TDes8& aDes)
/**
 * This method stores the streamed contents of the list into a descriptor
 *
 * \param aDes Descriptor in which to store the list
 */
	{
	RDesWriteStream strm(aDes);
	strm << *this;
	strm.CommitL();
	}

EXPORT_C void CMobilePhoneListBase::RestoreL(const TDesC8& aBuf)
/**
 * This method retrieves the contents of the list from a descriptor
 *
 * \param aBuf Descriptor from which to read the list
 */
	{
	RDesReadStream strm(aBuf);			// turn it into a stream
	strm >> *this;						// re-construct arrays 
	}

EXPORT_C const TAny* CMobilePhoneListBase::GetEntryL(TInt aIndex) const
/**
 * This method retrieves the list entry at the specified index
 *
 * \param aIndex Index of the desired entry within the list
 * \return TAny* Pointer to the list entry
 */
	{
	if (aIndex < 0 || aIndex >= iList.Count())
		User::Leave(EListIndexOutOfRange);
	return iList.At(aIndex);
	}

EXPORT_C void CMobilePhoneListBase::AddEntryL(const TAny* aPtr)
/**
 * This method adds a new entry to the end of the list
 *
 * \param aPtr Pointer to the new entry
 */
	{
	if (iMaxNumber != KMaxEntriesNotSet && iList.Count() >= iMaxNumber)
		User::Leave(EListMaxNumberReached);
	iList.AppendL(aPtr);
	}

EXPORT_C TInt CMobilePhoneListBase::Enumerate() const
/**
 * This method returns the number of entries in the list
 *
 * \return TInt Will contain the entry count
 */
	{
	return iList.Count();
	}

EXPORT_C TInt CMobilePhoneListBase::MaxNumberEntries() const
/**
 * This method returns the maximum number of entries that can be stored in this list
 *
 * \return TInt Will contain the maximum number of entries
 */
	{
	return iMaxNumber;
	}

EXPORT_C void CMobilePhoneListBase::SetMaxNumberEntries(TInt aMax)
/**
 * This method sets the maximum number of entries that can be stored in this list
 *
 * \param aMax Supplies the maximum number of entries
 */
	{
	__ASSERT_ALWAYS(aMax >=0, PanicClient(EEtelPanicIndexOutOfRange));
	__ASSERT_ALWAYS(aMax >= iList.Count(), PanicClient(EEtelPanicIndexOutOfRange));
	iMaxNumber=aMax;
	}

EXPORT_C void CMobilePhoneListBase::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the list contents from a stream 
 *
 * \param aStream The read stream containing the list
 */
	{
	iMaxNumber = aStream.ReadInt32L();
	iList.Reset();
	TInt count=aStream.ReadInt32L();
	for (TInt ii=0;ii<count;++ii)
		InternalizeEntryL(iList.ExtendL(),aStream);
	}

EXPORT_C void CMobilePhoneListBase::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the list contents into a stream 
 *
 * \param aStream The write stream that will contain the list
 */
	{
	aStream.WriteInt32L(iMaxNumber);
	TInt count=iList.Count();
	aStream.WriteInt32L(count);
	for (TInt ii=0;ii<count;++ii)
		ExternalizeEntryL(iList.At(ii),aStream);
	}


/********************************************************************/
//
// CMobilePhoneEditableListBase
// Base class of thin-template idiom for ETel list classes
//
/********************************************************************/

CMobilePhoneEditableListBase::CMobilePhoneEditableListBase(TInt aLength, TInt aGranularity)
: CMobilePhoneListBase(aLength,aGranularity)
	{
	}

CMobilePhoneEditableListBase::~CMobilePhoneEditableListBase()
	{}

EXPORT_C void CMobilePhoneEditableListBase::DeleteEntryL(TInt aIndex)
/**
 * This method deletes the list entry at the specified index
 *
 * \param aIndex Index of the entry to delete within the list
 */
	{
	if (aIndex < 0 || aIndex >= iList.Count())
		User::Leave(EListIndexOutOfRange);
	iList.Delete(aIndex);
	}

EXPORT_C void CMobilePhoneEditableListBase::InsertEntryL(TInt aIndex, const TAny* aPtr)
/**
 * This method inserts a new list entry at the specified index
 *
 * \param aIndex Index of the point at which to insert the new entry
 * \param aPtr Pointer to the new entry
 */
	{
	if (aIndex < 0 || aIndex >= iList.Count())
		User::Leave(EListIndexOutOfRange);
	if (iMaxNumber != KMaxEntriesNotSet && iList.Count() >= iMaxNumber)
		User::Leave(EListMaxNumberReached);
	iList.InsertL(aIndex, aPtr);
	}


/********************************************************************/
//
// CMobilePhoneNetworkList
// A concrete instantion of an ETel list - holding RMobilePhone::TMobilePhoneNetworkInfoV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneNetworkList* CMobilePhoneNetworkList::NewL()
/**
 * This method creates a list that will contain RMobilePhone::TMobilePhoneNetworkInfoV1 objects
 *
 */
	{
	CMobilePhoneNetworkList* r=new(ELeave) CMobilePhoneNetworkList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneNetworkList::CMobilePhoneNetworkList()
	: CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneNetworkInfoV1>()
	{}

EXPORT_C CMobilePhoneNetworkList::~CMobilePhoneNetworkList()
	{}

void CMobilePhoneNetworkList::ConstructL()
	{}

/********************************************************************/
//
// CMobilePhoneCFList
// A concrete instantion of an ETel list - holding RMobilePhone::TMobilePhoneCFInfoEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneCFList* CMobilePhoneCFList::NewL()
/**
 * This method creates a list that will contain RMobilePhone::TMobilePhoneCFInfoEntryV1 objects
 *
 */
	{
	CMobilePhoneCFList* r=new(ELeave) CMobilePhoneCFList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneCFList::CMobilePhoneCFList()
	: CMobilePhoneReadOnlyList<RMobilePhone::TMobilePhoneCFInfoEntryV1>()
	{}

EXPORT_C CMobilePhoneCFList::~CMobilePhoneCFList()
	{}

void CMobilePhoneCFList::ConstructL()
	{}

/********************************************************************/
//
// CMobilePhoneCBList
// A concrete instantion of an ETel list - holding RMobilePhone::TMobilePhoneCBInfoEntryV1 objects
//
/********************************************************************/


EXPORT_C CMobilePhoneCBList* CMobilePhoneCBList::NewL()
/**
 * This method creates a list that will contain RMobilePhone::TMobilePhoneCBInfoEntryV1 objects
 *
 */
	{
	CMobilePhoneCBList* r=new(ELeave) CMobilePhoneCBList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

EXPORT_C CMobilePhoneCBList::~CMobilePhoneCBList()
	{
	}

CMobilePhoneCBList::CMobilePhoneCBList()
	{
	}

void CMobilePhoneCBList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneCWList
// A concrete instantion of an ETel list - holding RMobilePhone::TMobilePhoneCWInfoEntryV1 objects
//
/********************************************************************/


EXPORT_C CMobilePhoneCWList* CMobilePhoneCWList::NewL()
/**
 * This method creates a list that will contain RMobilePhone::TMobilePhoneCWInfoEntryV1 objects
 *
 */
	{
	CMobilePhoneCWList* r=new(ELeave) CMobilePhoneCWList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

EXPORT_C CMobilePhoneCWList::~CMobilePhoneCWList()
	{
	}

CMobilePhoneCWList::CMobilePhoneCWList()
	{
	}

void CMobilePhoneCWList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneCCBSList
// A concrete instantion of an ETel list - holding RMobilePhone::TMobilePhoneCCBSEntryV1 objects
//
/********************************************************************/


EXPORT_C CMobilePhoneCcbsList* CMobilePhoneCcbsList::NewL()
/**
 * This method creates a list that will contain RMobilePhone::TMobilePhoneCCBSEntryV1 objects
 *
 */
	{
	CMobilePhoneCcbsList* r=new(ELeave) CMobilePhoneCcbsList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}

EXPORT_C CMobilePhoneCcbsList::~CMobilePhoneCcbsList()
	{
	}

CMobilePhoneCcbsList::CMobilePhoneCcbsList()
	{
	}

void CMobilePhoneCcbsList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneGsmSmsList
// A concrete instantion of an ETel list - holding RMobileSmsStore::TMobileGsmSmsEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneGsmSmsList* CMobilePhoneGsmSmsList::NewL()
/**
 * This method creates a list that will contain RMobileSmsStore::TMobileGsmSmsEntryV1 objects
 *
 */
	{
	CMobilePhoneGsmSmsList* r=new(ELeave) CMobilePhoneGsmSmsList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneGsmSmsList::CMobilePhoneGsmSmsList()
	: CMobilePhoneReadOnlyList<RMobileSmsStore::TMobileGsmSmsEntryV1>()
	{}

EXPORT_C CMobilePhoneGsmSmsList::~CMobilePhoneGsmSmsList()
	{}

void CMobilePhoneGsmSmsList::ConstructL()
	{}
	

/********************************************************************/
//
// CMobilePhoneSmspList
// A concrete instantion of an ETel list - holding RMobileSmsMessaging::TMobileSmspEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneSmspList* CMobilePhoneSmspList::NewL()
/**
 * This method creates a list that will contain RMobileSmsMessaging::TMobileSmspEntryV1 objects
 *
 */
	{
	CMobilePhoneSmspList* r=new(ELeave) CMobilePhoneSmspList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneSmspList::CMobilePhoneSmspList()
	: CMobilePhoneEditableList<RMobileSmsMessaging::TMobileSmspEntryV1>()
	{}

EXPORT_C CMobilePhoneSmspList::~CMobilePhoneSmspList()
	{}

void CMobilePhoneSmspList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneBroadcastIdList
// A concrete instantion of an ETel list - holding RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneBroadcastIdList* CMobilePhoneBroadcastIdList::NewL()
/**
 * This method creates a list that will contain RMobileBroadcastMessaging::TMobileBroadcastCbmiEntryV1 objects
 *
 */
	{
	CMobilePhoneBroadcastIdList* r=new(ELeave) CMobilePhoneBroadcastIdList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneBroadcastIdList::CMobilePhoneBroadcastIdList()
	: CMobilePhoneEditableList<RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1>()
	{}

EXPORT_C CMobilePhoneBroadcastIdList::~CMobilePhoneBroadcastIdList()
	{}

void CMobilePhoneBroadcastIdList::ConstructL()
	{}

EXPORT_C void CMobilePhoneBroadcastIdList::AddRangeEntryL(const RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1& aStart, 
											  const RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1& aEnd)
/**
 * This method adds a range of contiguous CBMI entries into the list
 * If aStart=0 and aEnd=100 then 100 entries with CBMI values 0 to 99 will be added
 *
 * \param aStart The first CBMI value to add
 * \param aEnd The last CBMI value to add
 */
	{
	if (aEnd.iId < aStart.iId)
		User::Leave(EBadRange);

	TInt numToAdd;
	numToAdd = (aEnd.iId - aStart.iId);

	if (iMaxNumber != KMaxEntriesNotSet && numToAdd + iList.Count() >= iMaxNumber)
		User::Leave(EListMaxNumberReached);

	RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1 entry;

	TInt count=0;
	while ( count<numToAdd )
		{
		entry.iId=(TUint16)((aStart.iId)+count);
		AddEntryL(entry);
		++count;
		}
	}

/********************************************************************/
//
// CMobilePhoneNamList
// A concrete instantion of an ETel list - holding RMobileNamStore::TMobileNamEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneNamList* CMobilePhoneNamList::NewL()
/**
 * This method creates a list that will contain RMobileNamStore::TMobileNamEntryV1 objects
 *
 */
	{
	CMobilePhoneNamList* r=new(ELeave) CMobilePhoneNamList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneNamList::CMobilePhoneNamList()
	: CMobilePhoneEditableList<RMobileNamStore::TMobileNamEntryV1>()
	{}

EXPORT_C CMobilePhoneNamList::~CMobilePhoneNamList()
	{}

void CMobilePhoneNamList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneONList
// A concrete instantion of an ETel list - holding RMobileONStore::TMobileONEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneONList* CMobilePhoneONList::NewL()
/**
 * This method creates a list that will contain RMobileONStore::TMobileONEntryV1 objects
 *
 */
	{
	CMobilePhoneONList* r=new(ELeave) CMobilePhoneONList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneONList::CMobilePhoneONList()
	: CMobilePhoneEditableList<RMobileONStore::TMobileONEntryV1>()
	{}

EXPORT_C CMobilePhoneONList::~CMobilePhoneONList()
	{}

void CMobilePhoneONList::ConstructL()
	{}


/********************************************************************/
//
// CMobilePhoneENList
// A concrete instantion of an ETel list - holding RMobileENStore::TMobileENEntryV1 objects
//
/********************************************************************/

EXPORT_C CMobilePhoneENList* CMobilePhoneENList::NewL()
/**
 * This method creates a list that will contain RMobileENStore::TMobileENEntryV1 objects
 *
 */
	{
	CMobilePhoneENList* r=new(ELeave) CMobilePhoneENList();
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}
	
CMobilePhoneENList::CMobilePhoneENList()
	: CMobilePhoneReadOnlyList<RMobileENStore::TMobileENEntryV1>()
	{}

EXPORT_C CMobilePhoneENList::~CMobilePhoneENList()
	{}

void CMobilePhoneENList::ConstructL()
	{}


