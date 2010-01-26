
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
// Contains methods for all the PtrHolder classes used by the EtelMM
// subsessions.
// 
//

// From core API
#include "Et_clsvr.h"
#include "ETELEXT.H"

// Multimode header files
#include "ETELMM.H"
#include "mmlist.h"
#include "mmretrieve.h"

#include "mm_hold.h"


/************************************************************************/
//
//  RMobilePhoneStore
//
/************************************************************************/

RMobilePhoneStore::RMobilePhoneStore()
	:iStorePtrHolder(NULL)
	{
	}

EXPORT_C void RMobilePhoneStore::BaseConstruct(CMobilePhoneStorePtrHolder* aPtrHolder)
	{
	iStorePtrHolder = aPtrHolder;
	}

EXPORT_C void RMobilePhoneStore::Destruct()
	{
	delete iStorePtrHolder;
	iStorePtrHolder = NULL;
	}

RMobilePhoneStore::TMobilePhoneStoreEntryV1::TMobilePhoneStoreEntryV1() 
:	iIndex(KIndexNotUsed)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

void RMobilePhoneStore::TMobilePhoneStoreEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the generic attributes of a store entry from a stream 
 *
 * \param aStream The read stream containing the store entry
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iIndex = aStream.ReadInt32L();
	}

void RMobilePhoneStore::TMobilePhoneStoreEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the generic attributes of a store entry into a stream 
 *
 * \param aStream The write stream that will contain the store entry
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteInt32L(iIndex);
	}

EXPORT_C RMobilePhoneStore::TMobilePhoneStoreInfoV1::TMobilePhoneStoreInfoV1()
:	iType(EPhoneStoreTypeUnknown), 
	iTotalEntries(-1), 
	iUsedEntries(-1), 
	iCaps(0)
	{
	iExtensionId=KETelMobilePhoneStoreV1;
	}

EXPORT_C void RMobilePhoneStore::GetInfo(TRequestStatus& aReqStatus, TDes8& aInfo) const
/**
 * This method returns the information related to the phone store
 *
 * \retval aInfo A descriptor that will contain the phone store information
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	Get(EMobilePhoneStoreGetInfo, aReqStatus, aInfo);
	}

EXPORT_C void RMobilePhoneStore::Read(TRequestStatus& aReqStatus, TDes8& aEntry) const
/**
 * This method reads an entry from a phone store
 * It is implemented by base class so same method can be called by all derived classes
 * The specialised entry type will be packaged into a descriptor in order to use this method
 *
 * \retval aEntry A descriptor that will contain the phone store entry
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	Get(EMobilePhoneStoreRead, aReqStatus, aEntry);
	}

EXPORT_C void RMobilePhoneStore::Write(TRequestStatus& aReqStatus, TDes8& aEntry)  const
/**
 * This method writes an entry into a phone store
 * It is implemented by base class so same method can be called by all derived classes
 * The specialised entry type will be packaged into a descriptor in order to use this method
 *
 * \param aEntry A descriptor that contains the phone store entry to write
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	Get(EMobilePhoneStoreWrite, aReqStatus, aEntry);
	}

EXPORT_C void RMobilePhoneStore::Delete(TRequestStatus& aReqStatus, TInt aIndex)  const
/**
 * This method deletes an entry from a phone store
 * It is implemented by base class so same method can be called by all derived classes
 *
 * \param aIndex Specifies which entry should be deleted
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iStorePtrHolder->iDeleteIndex=aIndex;
	TPtrC8& ptr1=iStorePtrHolder->SetC(CMobilePhoneStorePtrHolder::ESlot1Delete,iStorePtrHolder->iDeleteIndex);

	Set(EMobilePhoneStoreDelete, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhoneStore::DeleteAll(TRequestStatus& aReqStatus)  const
/**
 * This method deletes all entries in a phone store
 * It is implemented by base class so same method can be called by all derived classes
 */
	{
	Blank(EMobilePhoneStoreDeleteAll, aReqStatus);
	}

EXPORT_C void RMobilePhoneStore::NotifyStoreEvent(TRequestStatus& aReqStatus, TUint32& aEvent, TInt& aIndex) const
/**
 * This notification completes if an event occurs to that phone store
 * Events can include entries added, deleted, updated or store becoming full or empty
 *
 * \retval aEvent Will contain the event
 * \retval aIndex Will contain the store index in the case of entry added/deleted event
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iStorePtrHolder->Set(CMobilePhoneStorePtrHolder::ESlot1NotifyStoreEvent,aEvent);
	TPtr8& ptr2=iStorePtrHolder->Set(CMobilePhoneStorePtrHolder::ESlot2NotifyStoreEvent,aIndex);
	Get(EMobilePhoneStoreNotifyStoreEvent, aReqStatus, ptr1, ptr2);
	}

/************************************************************************/
//
//  RMobileSmsStore
//
/************************************************************************/

EXPORT_C RMobileSmsStore::RMobileSmsStore()
	{
	}

EXPORT_C TInt RMobileSmsStore::Open(RMobileSmsMessaging& aMessaging, const TDesC& aStoreName)
/**
 * This method opens a RMobileSmsStore subsession from RMobileSmsMessaging.
 */
	{
	RSessionBase* session=&aMessaging.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aMessaging.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));
	TRAPD(ret,ConstructL());
	if (ret)
		return ret;
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aStoreName)));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RMobileSmsStore::Close()
/**
 * This method closes a RMobileSmsStore subsession
 */
 	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RMobileSmsStore::ConstructL()
	{
	__ASSERT_ALWAYS(iStorePtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	CMobilePhoneStorePtrHolder* ptrHolder = CSmsStorePtrHolder::NewL(CSmsStorePtrHolder::EMaxNumberSmsStorePtrSlots,CSmsStorePtrHolder::EMaxNumberSmsStorePtrCSlots);
	RMobilePhoneStore::BaseConstruct(ptrHolder);
	}

RMobileSmsStore::TMobileSmsEntryV1::TMobileSmsEntryV1()
: iMsgStatus(EStoredMessageUnknownStatus)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobileSmsStore::TMobileSmsEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the SMS entry from a stream 
 *
 * \param aStream The read stream containing the SMS fixed size entry
 */
	{
	TMobilePhoneStoreEntryV1::InternalizeL(aStream);
	iMsgStatus = STATIC_CAST(TMobileSmsStoreStatus,aStream.ReadUint32L());
	}

void RMobileSmsStore::TMobileSmsEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the SMS entry into a stream 
 *
 * \param aStream The write stream that will contain the SMS entry
 */
	{
	TMobilePhoneStoreEntryV1::ExternalizeL(aStream);
	aStream.WriteUint32L(iMsgStatus);
	}

EXPORT_C RMobileSmsStore::TMobileGsmSmsEntryV1::TMobileGsmSmsEntryV1()
	{
	iExtensionId=KETelMobileGsmSmsEntryV1;
	}

void RMobileSmsStore::TMobileGsmSmsEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the SMS entry from a stream 
 *
 * \param aStream The read stream containing the SMS fixed size entry
 */
	{
	TMobileSmsEntryV1::InternalizeL(aStream);
	aStream >> iServiceCentre;
	aStream >> iMsgData;
	}

void RMobileSmsStore::TMobileGsmSmsEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the SMS entry into a stream 
 *
 * \param aStream The write stream that will contain the SMS entry
 */
	{
	TMobileSmsEntryV1::ExternalizeL(aStream);
	aStream << iServiceCentre;
	aStream << iMsgData;
	}

/************************************************************************/
//
//  RMobileNamStore
//
/************************************************************************/


EXPORT_C RMobileNamStore::RMobileNamStore()
	{
	}


EXPORT_C void RMobileNamStore::ConstructL()
	{
	__ASSERT_ALWAYS(iStorePtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	CMobilePhoneStorePtrHolder* ptrHolder = CNamStorePtrHolder::NewL(CNamStorePtrHolder::EMaxNumberNamStorePtrSlots,CNamStorePtrHolder::EMaxNumberNamStorePtrCSlots);
	RMobilePhoneStore::BaseConstruct(ptrHolder);
	}

EXPORT_C TInt RMobileNamStore::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileNamStore subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelNamStore);	
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,&name));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RMobileNamStore::Close()
/**
 * This method closes a RMobileNamStore subsession
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C RMobileNamStore::TMobileNamStoreInfoV1::TMobileNamStoreInfoV1() 
:	iNamCount(0), 
	iActiveNam(0)
	{
	iExtensionId=KETelMobileNamStoreV1;
	}

EXPORT_C void RMobileNamStore::SetActiveNam(TRequestStatus& aReqStatus, TInt aNamId) const
/**
 * This method sets a new value for the active NAM
 *
 * \param aNamId Specifies which NAM to activate
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	CNamStorePtrHolder* namStorePtrHolder = STATIC_CAST(CNamStorePtrHolder*,iStorePtrHolder);
	
	namStorePtrHolder->iSetActiveNamNamId = aNamId;
	TPtrC8& ptr1=namStorePtrHolder->SetC(CNamStorePtrHolder::ESlot1SetActiveNam,namStorePtrHolder->iSetActiveNamNamId);

	Set(EMobileNamStoreSetActiveNam,aReqStatus,ptr1);
	}

EXPORT_C RMobileNamStore::TMobileNamEntryV1::TMobileNamEntryV1() 
: 	iNamId(0), 
	iParamIdentifier(0)
	{
	}
	
void RMobileNamStore::TMobileNamEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the NAM entry from a stream 
 *
 * \param aStream The read stream containing the NAM entry
 */
	{
	TMobilePhoneStoreEntryV1::InternalizeL(aStream);
	iNamId = aStream.ReadInt32L();
	iParamIdentifier = aStream.ReadUint32L();
	aStream >> iData;
	}

void RMobileNamStore::TMobileNamEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the NAM entry into a stream 
 *
 * \param aStream The write stream that will contain the NAM entry
 */
	{
	TMobilePhoneStoreEntryV1::ExternalizeL(aStream);
	aStream.WriteInt32L(iNamId);
	aStream.WriteUint32L(iParamIdentifier);
	aStream << iData;
	}

EXPORT_C void RMobileNamStore::StoreAllL(TRequestStatus& aReqStatus, TInt aNamId, CMobilePhoneNamList* aNamList) const
/**
 * This method stores a new version of the NAM list
 *
 * \param aNamList Pointer to the list containing the NAM entries to store
 * \exception Will leave if the CBufFlat to hold the streamed contents can not be allocated
 */
	{
	__ASSERT_ALWAYS(aNamList!=NULL,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	CNamStorePtrHolder* namStorePtrHolder = STATIC_CAST(CNamStorePtrHolder*,iStorePtrHolder);

	delete namStorePtrHolder->iNamBuf;
	namStorePtrHolder->iNamBuf=NULL;

	namStorePtrHolder->iNamBuf=aNamList->StoreLC();
	CleanupStack::Pop();

	(namStorePtrHolder->iNamPtr).Set((namStorePtrHolder->iNamBuf)->Ptr(0));

	namStorePtrHolder->iStoreAllNamId = aNamId;
	TPtrC8& ptr1=namStorePtrHolder->SetC(CNamStorePtrHolder::ESlot1NamListStoreAll,namStorePtrHolder->iStoreAllNamId);

	Set(EMobileNamStoreStoreAll, aReqStatus, ptr1, namStorePtrHolder->iNamPtr);
	}

/************************************************************************/
//
//  RMobileONStore
//
/************************************************************************/


EXPORT_C RMobileONStore::RMobileONStore()
	{

	}

EXPORT_C void RMobileONStore::ConstructL()
	{
	__ASSERT_ALWAYS(iStorePtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	CMobilePhoneStorePtrHolder* ptrHolder = CONStorePtrHolder::NewL(CONStorePtrHolder::EMaxNumberONStorePtrSlots, CONStorePtrHolder::EMaxNumberONStorePtrCSlots);
	RMobilePhoneStore::BaseConstruct(ptrHolder);
	}

EXPORT_C TInt RMobileONStore::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileONStore subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelOwnNumberStore);	
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,&name));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RMobileONStore::Close()
/**
 * This method closes a RMobileONStore subsession
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}


EXPORT_C RMobileONStore::TMobileONStoreInfoV1::TMobileONStoreInfoV1() 
: 	iNumberLen(0), 
	iTextLen(0)
	{
	iExtensionId=KETelMobileONStoreV1;
	}

EXPORT_C RMobileONStore::TMobileONEntryV1::TMobileONEntryV1() 
: 	iMode(RMobilePhone::ENetworkModeUnknown), 
	iService(RMobilePhone::EServiceUnspecified)
	{
	}

void RMobileONStore::TMobileONEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the Own Number entry from a stream 
 *
 * \param aStream The read stream containing the Own Number entry
 */
	{
	TMobilePhoneStoreEntryV1::InternalizeL(aStream);
	iMode = STATIC_CAST(RMobilePhone::TMobilePhoneNetworkMode,aStream.ReadUint32L());
	aStream >> iText;
	aStream >> iNumber;
	iService = STATIC_CAST(RMobilePhone::TMobileService, aStream.ReadUint32L());
	}

void RMobileONStore::TMobileONEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the Own Number entry into a stream 
 *
 * \param aStream The write stream that will contain the Own Number entry
 */
	{
	TMobilePhoneStoreEntryV1::ExternalizeL(aStream);
	aStream.WriteUint32L(iMode);
	aStream << iText;
	aStream << iNumber;
	aStream.WriteUint32L(iService);
	}

EXPORT_C void RMobileONStore::StoreAllL(TRequestStatus& aReqStatus, CMobilePhoneONList* aONList) const
/**
 * This method stores a new version of the Own Number list onto the SIM
 *
 * \param aONList Pointer to the list containing the Own Number entries to store
 * \exception Will leave if the CBufFlat to hold the streamed contents can not be allocated
 */
	{
	__ASSERT_ALWAYS(aONList!=NULL,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	CONStorePtrHolder* onStorePtrHolder = STATIC_CAST(CONStorePtrHolder*, iStorePtrHolder);
	
	delete onStorePtrHolder->iONBuf;
	onStorePtrHolder->iONBuf=NULL;

	onStorePtrHolder->iONBuf=aONList->StoreLC();
	CleanupStack::Pop();

	(onStorePtrHolder->iONPtr).Set((onStorePtrHolder->iONBuf)->Ptr(0));

	Set(EMobileONStoreStoreAll, aReqStatus, onStorePtrHolder->iONPtr);	
	}


/************************************************************************/
//
//  RMobileENStore
//
/************************************************************************/


EXPORT_C RMobileENStore::RMobileENStore() 
	{
	}

EXPORT_C void RMobileENStore::ConstructL()
	{
	__ASSERT_ALWAYS(iStorePtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	CMobilePhoneStorePtrHolder* ptrHolder = CMobilePhoneStorePtrHolder::NewL(CMobilePhoneStorePtrHolder::EMaxNumberPhoneStorePtrSlots, CMobilePhoneStorePtrHolder::EMaxNumberPhoneStorePtrCSlots);
	RMobilePhoneStore::BaseConstruct(ptrHolder);
	}

EXPORT_C TInt RMobileENStore::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileENStore subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelEmergencyNumberStore);	
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,&name));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;
	}

EXPORT_C void RMobileENStore::Close()
/**
 * This method closes a RMobileENStore subsession
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C RMobileENStore::TMobileENEntryV1::TMobileENEntryV1() 
: 	iNetworkSpecific(EFalse), 
	iMode(RMobilePhone::ENetworkModeUnknown)
	{
	}

void RMobileENStore::TMobileENEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the Emergency Number entry from a stream 
 *
 * \param aStream The read stream containing the Emergency Number entry
 */
	{
	TMobilePhoneStoreEntryV1::InternalizeL(aStream);
	
	iNetworkSpecific = STATIC_CAST(TBool,aStream.ReadInt32L());
	iMode = STATIC_CAST(RMobilePhone::TMobilePhoneNetworkMode, aStream.ReadUint32L());
	aStream >> iCountryCode;
	aStream >> iIdentity;
	aStream >> iNumber;
	aStream >> iAlphaId;
	iCallType = aStream.ReadInt32L();
	}

void RMobileENStore::TMobileENEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the Emergency Number entry into a stream 
 *
 * \param aStream The write stream that will contain the Emergency Number entry
 */
	{
	TMobilePhoneStoreEntryV1::ExternalizeL(aStream);

	aStream.WriteInt32L(iNetworkSpecific);
	aStream.WriteUint32L(iMode);
	aStream << iCountryCode;
	aStream << iIdentity;
	aStream << iNumber;	
	aStream << iAlphaId;
	aStream.WriteInt32L(iCallType);
	}

/************************************************************************/
//
//  RMobilePhoneBookStore
//
/************************************************************************/


EXPORT_C RMobilePhoneBookStore::RMobilePhoneBookStore()
	{
	}

EXPORT_C void RMobilePhoneBookStore::ConstructL()
	{
	__ASSERT_ALWAYS(iStorePtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	CMobilePhoneStorePtrHolder* ptrHolder = CPhoneBookStorePtrHolder::NewL(CPhoneBookStorePtrHolder::EMaxNumberPhoneBookStorePtrSlots, CPhoneBookStorePtrHolder::EMaxNumberPhoneBookStorePtrCSlots);
	RMobilePhoneStore::BaseConstruct(ptrHolder);
	}

EXPORT_C TInt RMobilePhoneBookStore::Open(RMobilePhone& aPhone, const TDesC& aStore)
/**
 * This method opens a RMobilePhoneBookStore subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=0,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TIpcArgs args;
	args.Set(0,REINTERPRET_CAST(TAny*,CONST_CAST(TDesC*,&aStore)));
	args.Set(1,TIpcArgs::ENothing);
	args.Set(2,REINTERPRET_CAST(TAny*,subSessionHandle));
	SetSessionHandle(*session);
	ret = CreateSubSession(*session,EEtelOpenFromSubSession,args);
	if (ret)
		Destruct();
	return ret;	
	}

EXPORT_C void RMobilePhoneBookStore::Close()
/**
 * This method closes a RMobilePhoneBookStore subsession
 */
	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C RMobilePhoneBookStore::TMobilePhoneBookInfoV1::TMobilePhoneBookInfoV1() 
:	iMaxNumLength(-1), 
	iMaxTextLength(-1),
	iLocation(ELocationUnknown),
	iChangeCounter(0)
	{
	iExtensionId=KETelMobilePhonebookStoreV1;
	}

EXPORT_C void RMobilePhoneBookStore::Read(TRequestStatus& aReqStatus, TInt aIndex, TInt aNumEntries, TDes8& aPBData) const
/**
 * This method reads an entry from a phonebook store.
 * The reading will start at the entry specified by aIndex and will stop either after 
 * aNumEntries have been read or no more whole phonebook entries can fit in the size of 
 * the supplied aPBData parameter.
 *
 * \param  aNumEntries Specifies the number of entries to be read.
 * \param  aIndex Specifies the starting index.
 * \retval aPBData A descriptor that will contain the phonebook data in TLV format.
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	CPhoneBookStorePtrHolder* pbStorePtrHolder = STATIC_CAST(CPhoneBookStorePtrHolder*,iStorePtrHolder);

	pbStorePtrHolder->iReadPhoneBookEntry.iIndex = aIndex;
	pbStorePtrHolder->iReadPhoneBookEntry.iNumEntries = aNumEntries;

	TPtrC8& ptr1=pbStorePtrHolder->SetC(CPhoneBookStorePtrHolder::ESlot1PhoneBookStoreRead, pbStorePtrHolder->iReadPhoneBookEntry);
	SetAndGet(EMobilePhoneBookStoreRead, aReqStatus, ptr1, aPBData);
	}

EXPORT_C void RMobilePhoneBookStore::Write(TRequestStatus& aReqStatus, TDes8& aPBData, TInt& aIndex) const
/**
 * This method writes one phonebook entry to the store.
 *
 * \param aIndex Specifies the slot where the entry will be written to.
 * \param aPBData A descriptor that will contain the phonebook data in TLV format.
 */
	{
	__ASSERT_ALWAYS(iStorePtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	CPhoneBookStorePtrHolder* pbStorePtrHolder = STATIC_CAST(CPhoneBookStorePtrHolder*,iStorePtrHolder);

	TPtr8& ptr1=pbStorePtrHolder->Set(CPhoneBookStorePtrHolder::ESlot1PhoneBookStoreWrite, aIndex);

	SetAndGet(EMobilePhoneBookStoreWrite, aReqStatus, aPBData, ptr1);
	}

