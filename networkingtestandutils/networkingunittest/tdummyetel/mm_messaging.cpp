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
// This file contains the method definitions for all EtelMM subsessions that
// provide messaging functionality.
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
//  RMobileSmsMessaging
//
/************************************************************************/

EXPORT_C RMobileSmsMessaging::RMobileSmsMessaging() :
	iSmsMessagingPtrHolder(NULL)
	{
	}

EXPORT_C TInt RMobileSmsMessaging::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileSmsMessaging subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelSmsMessaging);	
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

EXPORT_C void RMobileSmsMessaging::Close()
/**
 * This method closes a RMobileSmsMessaging subsession
 */
 	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RMobileSmsMessaging::ConstructL()
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iSmsMessagingPtrHolder = CSmsMessagingPtrHolder::NewL(CSmsMessagingPtrHolder::EMaxNumberSmsPtrSlots,CSmsMessagingPtrHolder::EMaxNumberSmsPtrCSlots);
	}

EXPORT_C void RMobileSmsMessaging::Destruct()
	{
	delete iSmsMessagingPtrHolder;
	iSmsMessagingPtrHolder = NULL;
	}

RMobileSmsMessaging::TMobileSmsAttributesV1::TMobileSmsAttributesV1() 
:	iFlags(0), 
	iDataFormat(EFormatUnspecified)
	{
	}

EXPORT_C RMobileSmsMessaging::TMobileSmsReceiveAttributesV1::TMobileSmsReceiveAttributesV1() 
:	iStatus(EMtMessageUnknownStatus),
	iStoreIndex(0)
	{
	iExtensionId=KETelMobileSmsReceiveAttributesV1;
	}

EXPORT_C RMobileSmsMessaging::TMobileSmsSendAttributesV1::TMobileSmsSendAttributesV1() 
:	iMsgRef(0),
	iMore(EFalse)
	{
	iExtensionId=KETelMobileSmsSendAttributesV1;
	}

EXPORT_C RMobileSmsMessaging::TMobileSmsCapsV1::TMobileSmsCapsV1() 
:	iSmsMode(0), 
	iSmsControl(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileSmsMessaging::GetCaps(TDes8& aCaps) const
/**
 * This method returns the SMS messaging capabilities
 *
 * \retval aCaps A descriptor that will contain the SMS messaging capabilities
 * \return KErrNone
 */
	{
	return Get(EMobileSmsMessagingGetCaps, aCaps);
	}

EXPORT_C TInt RMobileSmsMessaging::GetReceiveMode(TMobileSmsReceiveMode& aReceiveMode) const
/**
 * This method will get the TSY-Phone receive mode
 *
 * \retval aReceiveMode Will contains the current receive mode setting
 */
	{
	TPckg<TMobileSmsReceiveMode> ptr1(aReceiveMode);
	return Get(EMobileSmsMessagingGetReceiveMode, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::SetReceiveMode(TRequestStatus& aReqStatus, TMobileSmsReceiveMode aReceiveMode) const
/**
 * This method will set the TSY-Phone receive mode
 *
 * \param aReceiveMode Contains the desired receive mode setting
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	iSmsMessagingPtrHolder->iReceiveMode = aReceiveMode;
	TPtrC8& ptr1=iSmsMessagingPtrHolder->SetC(CSmsMessagingPtrHolder::ESlot1SetReceiveMode,iSmsMessagingPtrHolder->iReceiveMode);

	Set(EMobileSmsMessagingSetReceiveMode, aReqStatus, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::NotifyReceiveModeChange(TRequestStatus& aReqStatus, TMobileSmsReceiveMode& aReceiveMode)
/**
 * This notification completes if the TSY-Phone receive mode changes
 *
 * \retval aReceiveMode Will contain the new receive mode
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iSmsMessagingPtrHolder->Set(CSmsMessagingPtrHolder::ESlot1NotifyReceiveModeChange,aReceiveMode);

	Get(EMobileSmsMessagingNotifyReceiveModeChange, aReqStatus, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::ReceiveMessage(TRequestStatus& aReqStatus, TDes8& aMsgData, TDes8& aMsgAttributes) const
/**
 * This method will complete when a new incoming SMS message has been received
 *
 * \retval aMsgData Will contain the SMS message data
 * \retval aMsgAttributes Will contain the SMS message attributes, which includes the message data format
 */
	{
	Get(EMobileSmsMessagingReceiveMessage, aReqStatus, aMsgData, aMsgAttributes);
	}

EXPORT_C void RMobileSmsMessaging::AckSmsStored(TRequestStatus& aReqStatus, const TDesC8& aMsgData, TBool aFull) const
/**
 * This method acknowledges that a received, unstored SMS has now been decoded and stored
 *
 * \param aMsgData A descriptor that will contain the SMS-DELIVER-REPORT TPDU
 * \param aFull A boolean that will indicate whether the client's SMS storage area is now full or not
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iSmsMessagingPtrHolder->iAckSmsStoredFull = aFull;
	TPtrC8& ptr1=iSmsMessagingPtrHolder->SetC(CSmsMessagingPtrHolder::ESlot1AckSmsStored,iSmsMessagingPtrHolder->iAckSmsStoredFull);

	Set(EMobileSmsMessagingAckSmsStored, aReqStatus, aMsgData ,ptr1);
	}

EXPORT_C void RMobileSmsMessaging::NackSmsStored(TRequestStatus& aReqStatus, const TDesC8& aMsg, TInt aRpCause) const
/**
 * This method rejects a received, unstored SMS because it could not be decoded and/or stored
 *
 * \param aMsg A descriptor that will contain the SMS-DELIVER-REPORT TPDU
 * \param aRpCause The error cause to place in the RP-ERROR
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	iSmsMessagingPtrHolder->iNackSmsStoredCause = aRpCause;
	TPtrC8& ptr1=iSmsMessagingPtrHolder->SetC(CSmsMessagingPtrHolder::ESlot1NackSmsStored,iSmsMessagingPtrHolder->iNackSmsStoredCause);

	Set(EMobileSmsMessagingNackSmsStored, aReqStatus, aMsg, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::ResumeSmsReception(TRequestStatus& aReqStatus) const
/**
 * This method resumes SMS reception if it is suspended due to a previously rejected MT SMS.
 */
	{
	Blank(EMobileSmsMessagingResumeSmsReception, aReqStatus);
	}

EXPORT_C void RMobileSmsMessaging::SendMessage(TRequestStatus& aReqStatus,  const TDesC8& aMsgData, TDes8& aMsgAttributes) const
/**
 * This method sends an outgoing SMS to the network
 *
 * \param aMsgData Supplies the TPDU to send
 * \param aMsgAttributes Supplies the attributes of the outgoing message
 */
	{
	SetAndGet(EMobileSmsMessagingSendMessage, aReqStatus,aMsgData,aMsgAttributes);
	}

EXPORT_C void RMobileSmsMessaging::SetMoSmsBearer(TRequestStatus& aReqStatus, TMobileSmsBearer aBearer) const
/**
 * This method will set the bearer (GSM or GPRS) used for outgoing SMS
 *
 * \param aBearer Contains the desired bearer setting
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	iSmsMessagingPtrHolder->iSmsBearer = aBearer;
	TPtrC8& ptr1=iSmsMessagingPtrHolder->SetC(CSmsMessagingPtrHolder::ESlot1SetMoSmsBearer,iSmsMessagingPtrHolder->iSmsBearer);

	Set(EMobileSmsMessagingSetMoSmsBearer, aReqStatus, ptr1);
	}

EXPORT_C TInt RMobileSmsMessaging::GetMoSmsBearer(TMobileSmsBearer& aBearer) const
/**
 * This method will get the bearer (GSM or GPRS) used for outgoing SMS
 *
 * \retval aBearer Will contains the current bearer setting
 */
	{
	TPckg<TMobileSmsBearer> ptr1(aBearer);
	return Get(EMobileSmsMessagingGetMoSmsBearer, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::NotifyMoSmsBearerChange(TRequestStatus& aReqStatus, TMobileSmsBearer& aBearer)
/**
 * This notification completes if the SMS bearer for outgoing SMS changes
 *
 * \retval aBearer Will contain the new bearer setting
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iSmsMessagingPtrHolder->Set(CSmsMessagingPtrHolder::ESlot1NotifyMoSmsBearerChange,aBearer);

	Get(EMobileSmsMessagingNotifyMoSmsBearerChange, aReqStatus, ptr1);
	}

//
// SMS related storage
//

EXPORT_C TInt RMobileSmsMessaging::EnumerateMessageStores(TInt& aCount) const
/**
 * This method returns the number of SMS message stores supported by phone and SIM
 *
 * \retval aCount Will contain the number of stores
 */
	{
	TPckg<TInt> ptr1(aCount);
	return Get(EMobileSmsMessagingEnumerateMessageStores, ptr1);
	}

EXPORT_C void RMobileSmsMessaging::GetMessageStoreInfo(TRequestStatus& aReqStatus, TInt aIndex, TDes8& aInfo) const
/**
 * This method returns information related to one of the SMS message stores
 *
 * \param aIndex The index of the store, = 0 to aCount-1 where aCount is the returned value from EnumerateMessageStores
 * \retval aInfo A descriptor that will contain the store information
 */
	{
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iSmsMessagingPtrHolder->iGetMessageStoreInfoIndex = aIndex;
	TPtrC8& ptr1=iSmsMessagingPtrHolder->SetC(CSmsMessagingPtrHolder::ESlot1GetMessageStoreInfo,iSmsMessagingPtrHolder->iGetMessageStoreInfoIndex);

	SetAndGet(EMobileSmsMessagingGetMessageStoreInfo, aReqStatus, ptr1, aInfo);
	}

//
// SMS Parameter Storage
//

EXPORT_C RMobileSmsMessaging::TMobileSmspEntryV1::TMobileSmspEntryV1() 
: 	iValidParams(0), 
	iProtocolId(0), 
	iDcs(0), 
	iValidityPeriod(0),
	iReservedFiller(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobileSmsMessaging::TMobileSmspEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the SMS parameter entry from a stream 
 *
 * \param aStream The read stream containing the SMS parameter entry
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iIndex = aStream.ReadInt32L();
	aStream >> iText;
	iValidParams = aStream.ReadUint32L();
	aStream >> iDestination;
	aStream >> iServiceCentre;
	iProtocolId = aStream.ReadUint8L();
	iDcs = aStream.ReadUint8L();
	iValidityPeriod = aStream.ReadUint8L();
	}

void RMobileSmsMessaging::TMobileSmspEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the SMS parameter entry into a stream 
 *
 * \param aStream The write stream that will contain the SMS parameter entry
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteInt32L(iIndex);
	aStream << iText;
	aStream.WriteUint32L(iValidParams);
	aStream << iDestination;
	aStream << iServiceCentre;
	aStream.WriteUint8L(iProtocolId);
	aStream.WriteUint8L(iDcs);
	aStream.WriteUint8L(iValidityPeriod);
	}


EXPORT_C void RMobileSmsMessaging::StoreSmspListL(TRequestStatus& aReqStatus, CMobilePhoneSmspList* aSmspList) const
/**
 * This method stores a new version of the SMSP list onto the SIM
 *
 * \param aSmspList Pointer to the list containing the SMSP entries to store
 * \exception Will leave if the CBufFlat to hold the streamed contents can not be allocated
 */
	{
	__ASSERT_ALWAYS(aSmspList!=NULL,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(iSmsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	delete iSmsMessagingPtrHolder->iSmspBuf;
	iSmsMessagingPtrHolder->iSmspBuf = NULL;

	iSmsMessagingPtrHolder->iSmspBuf=aSmspList->StoreLC();
	CleanupStack::Pop();

	(iSmsMessagingPtrHolder->iSmspPtr).Set((iSmsMessagingPtrHolder->iSmspBuf)->Ptr(0));

	Set(EMobileSmsMessagingStoreSmspList,aReqStatus,iSmsMessagingPtrHolder->iSmspPtr);

	}

EXPORT_C void RMobileSmsMessaging::NotifySmspListChange(TRequestStatus& aReqStatus) const
/**
 * This notification completes if the SMSP list contents change
 * The new contents are not returned but must be retrieved using CRetrieveMobilePhoneBroadcastIdList
 */
	{
	Blank(EMobileSmsMessagingNotifySmspListChange,aReqStatus);
	}


/************************************************************************/
//
//  RMobileBroadcastMessaging
//
/************************************************************************/

EXPORT_C RMobileBroadcastMessaging::RMobileBroadcastMessaging() :
	iCbsMessagingPtrHolder(NULL)
	{
	}

EXPORT_C TInt RMobileBroadcastMessaging::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileBroadcastMessaging subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelBroadcastMessaging);	
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

EXPORT_C void RMobileBroadcastMessaging::Close()
/**
 * This method closes a RMobileBroadcastMessaging subsession
 */
 	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RMobileBroadcastMessaging::ConstructL()
	{
	__ASSERT_ALWAYS(iCbsMessagingPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iCbsMessagingPtrHolder = CCbsMessagingPtrHolder::NewL(CCbsMessagingPtrHolder::EMaxNumberBroadcastPtrSlots,CCbsMessagingPtrHolder::EMaxNumberBroadcastPtrCSlots);
	}

EXPORT_C void RMobileBroadcastMessaging::Destruct()
	{
	delete iCbsMessagingPtrHolder;
	iCbsMessagingPtrHolder = NULL;
	}

EXPORT_C RMobileBroadcastMessaging::TMobileBroadcastCapsV1::TMobileBroadcastCapsV1() : 
	iModeCaps(0), 
	iFilterCaps(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileBroadcastMessaging::GetCaps(TDes8& aCaps) const
/**
 * This method returns the broadcast messaging capabilities
 *
 * \retval aCaps A descriptor that will contain the broadcast messaging capabilities
 * \return KErrNone
 */
	{
	return Get(EMobileBroadcastMessagingGetCaps, aCaps);
	}

EXPORT_C RMobileBroadcastMessaging::TMobileBroadcastAttributesV1::TMobileBroadcastAttributesV1() :
	iFlags(0), 
	iFormat(EFormatUnspecified),
	iServiceCategory(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobileBroadcastMessaging::ReceiveMessage(TRequestStatus& aReqStatus, TDes8& aMsgData, TDes8& aMsgAttributes) const
/**
 * This method will complete when a new incoming broadcast message has been received
 *
 * \retval aMsgData Will contain the broadcast message data
 * \retval aMsgAttributes Will contain the broadcast message attributes, which includes the message data format
 */
	{
	Get(EMobileBroadcastMessagingReceiveMessage, aReqStatus, aMsgData, aMsgAttributes);
	}

EXPORT_C TInt RMobileBroadcastMessaging::GetFilterSetting(TMobilePhoneBroadcastFilter& aSetting) const
/**
 * This method returns the current filter setting
 * The filter specifies which broadcast messages are accepted and which are rejected
 *
 * \retval aSetting Will contain the filter setting
 */
	{
	TPckg<TMobilePhoneBroadcastFilter> ptr1(aSetting);
	return Get(EMobileBroadcastMessagingGetFilterSetting, ptr1);
	}

EXPORT_C void RMobileBroadcastMessaging::SetFilterSetting(TRequestStatus& aReqStatus, TMobilePhoneBroadcastFilter aSetting) const
/**
 * This method sets a new value for the filter setting
 * The filter specifies which broadcast messages are accepted and which are rejected
 *
 * \param aSetting Supplies the new filter setting
 */
	{
	__ASSERT_ALWAYS(iCbsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iCbsMessagingPtrHolder->iSetFilterSetting=aSetting;

	TPtrC8& ptr1=iCbsMessagingPtrHolder->SetC(CCbsMessagingPtrHolder::ESlot1SetFilterSetting, iCbsMessagingPtrHolder->iSetFilterSetting);
	
	Set(EMobileBroadcastMessagingSetFilterSetting, aReqStatus, ptr1);
	}

EXPORT_C void RMobileBroadcastMessaging::NotifyFilterSettingChange(TRequestStatus& aReqStatus, TMobilePhoneBroadcastFilter& aSetting) const
/**
 * This notification completes if the filter setting changes
 * The filter specifies which broadcast messages are accepted and which are rejected
 *
 * \retval aSetting Will contain the new filter setting
 */
	{
	__ASSERT_ALWAYS(iCbsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iCbsMessagingPtrHolder->Set(CCbsMessagingPtrHolder::ESlot1NotifyFilterSettingChange,aSetting);

	Get(EMobileBroadcastMessagingNotifyFilterSettingChange, aReqStatus, ptr1);
	}

EXPORT_C void RMobileBroadcastMessaging::GetLanguageFilter(TRequestStatus& aReqStatus, TDes16& aLangFilter) const
/**
 * This method returns the current language filter contents
 * The language filter specifies which broadcast message languages are accepted and which are rejected
 *
 * \retval aLangFilter A descriptor that will contain the language specifiers, each one is a 16-bit value 
 */
	{
	Get(EMobileBroadcastMessagingGetLanguageFilter,aReqStatus,aLangFilter);
	}

EXPORT_C void RMobileBroadcastMessaging::SetLanguageFilter(TRequestStatus& aReqStatus, const TDesC16& aLangFilter) const
/**
 * This method sets the language filter contents
 * The language filter specifies which broadcast message languages are accepted and which are rejected
 *
 * \retval aLangFilter A descriptor that will supply the language specifiers, each one is a 16-bit value 
 */
	{
	Set(EMobileBroadcastMessagingSetLanguageFilter,aReqStatus,aLangFilter);
	}

EXPORT_C void RMobileBroadcastMessaging::NotifyLanguageFilterChange(TRequestStatus& aReqStatus, TDes16& aLangFilter) const
/**
 * This notification completes if the language filter contents change
 *
 * \retval aLangFilter A descriptor that will contain the new language specifiers, each one is a 16-bit value 
 */
	{
	Get(EMobileBroadcastMessagingNotifyLanguageFilterChange,aReqStatus,aLangFilter);
	}

EXPORT_C RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1::TMobileBroadcastIdEntryV1() :
	iId(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the CBMI entry from a stream 
 *
 * \param aStream The read stream containing the CBMI entry
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iId = aStream.ReadUint16L();
	}

void RMobileBroadcastMessaging::TMobileBroadcastIdEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the CBMI entry into a stream 
 *
 * \param aStream The write stream that will contain the CBMI entry
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteInt16L(iId);
	}

EXPORT_C void RMobileBroadcastMessaging::StoreBroadcastIdListL(TRequestStatus& aReqStatus, CMobilePhoneBroadcastIdList* aBroadcastIdList, TMobileBroadcastIdType aIdType)
/**
 * This method stores a new version of the BroadcastId list onto the SIM
 * The BroadcastId list specifies which broadcast message identifiers are accepted and which are rejected
 *
 * \param aBroadcastIdList Pointer to the list containing the BroadcastId entries to store
 * \exception Will leave if the CBufFlat to hold the streamed contents can not be allocated
 */
	{
	__ASSERT_ALWAYS(aBroadcastIdList!=NULL,PanicClient(EEtelPanicNullHandle));
	__ASSERT_ALWAYS(iCbsMessagingPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	delete iCbsMessagingPtrHolder->iBroadcastIdBuf;
	iCbsMessagingPtrHolder->iBroadcastIdBuf = NULL;

	iCbsMessagingPtrHolder->iBroadcastIdBuf=aBroadcastIdList->StoreLC();
	CleanupStack::Pop();

	(iCbsMessagingPtrHolder->iBroadcastIdPtr).Set((iCbsMessagingPtrHolder->iBroadcastIdBuf)->Ptr(0));

	iCbsMessagingPtrHolder->iIdType = aIdType;
	TPtrC8& ptr2=iCbsMessagingPtrHolder->SetC(CCbsMessagingPtrHolder::ESlot1StoreBroadcastIdListL,iCbsMessagingPtrHolder);

	Set(EMobileBroadcastMessagingStoreIdList,aReqStatus,iCbsMessagingPtrHolder->iBroadcastIdPtr, ptr2);
	}

EXPORT_C void RMobileBroadcastMessaging::NotifyBroadcastIdListChange(TRequestStatus& aReqStatus) const
/**
 * This notification completes if the BroadcastId filter contents change
 * The new contents are not returned but must be retrieved using CRetrieveMobilePhoneBroadcastIdList
 */
	{
	Blank(EMobileBroadcastMessagingNotifyIdListChange,aReqStatus);
	}

/************************************************************************/
//
//  RMobileUssdMessaging
//
/************************************************************************/

EXPORT_C RMobileUssdMessaging::RMobileUssdMessaging()
: iUssdMessagingPtrHolder(NULL)
	{
	}

EXPORT_C TInt RMobileUssdMessaging::Open(RMobilePhone& aPhone)
/**
 * This method opens a RMobileUssdMessaging subsession from RMobilePhone.
 */
	{
	RSessionBase* session=&aPhone.SessionHandle();
	__ASSERT_ALWAYS(session!=NULL,PanicClient(EEtelPanicNullHandle));
	TInt subSessionHandle=aPhone.SubSessionHandle();
	__ASSERT_ALWAYS(subSessionHandle!=0,PanicClient(EEtelPanicNullHandle));

	TRAPD(ret,ConstructL()); 
	if (ret)
		return ret;
	TPtrC name(KETelUssdMessaging);	
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

EXPORT_C void RMobileUssdMessaging::Close()
/**
 * This method closes a RMobileUssdMessaging subsession
 */
 	{
	CloseSubSession(EEtelClose);
	Destruct();
	}

EXPORT_C void RMobileUssdMessaging::ConstructL()
	{
	}

EXPORT_C void RMobileUssdMessaging::Destruct()
	{
	}

EXPORT_C RMobileUssdMessaging::TMobileUssdCapsV1::TMobileUssdCapsV1() : 
	iUssdFormat(0), 
	iUssdTypes(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileUssdMessaging::GetCaps(TDes8& aCaps) const
/**
 * This method returns the USSD messaging capabilities
 *
 * \retval aCaps A descriptor that will contain the USSD messaging capabilities
 * \return KErrNone
 */
	{
	return Get(EMobileUssdMessagingGetCaps, aCaps);
	}

EXPORT_C RMobileUssdMessaging::TMobileUssdAttributesV1::TMobileUssdAttributesV1() :
	iFlags(0), 
	iFormat(EFormatUnspecified), 
	iType(EUssdUnknown), 
	iDcs(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobileUssdMessaging::ReceiveMessage(TRequestStatus& aReqStatus, TDes8& aMsgData, TDes8& aMsgAttributes) const
/**
 * This method will complete when a new incoming USSD message has been received
 *
 * \retval aMsgData Will contain the USSD message data
 * \retval aMsgAttributes Will contain the USSD message attributes, which includes the message data format
 */
	{
	Get(EMobileUssdMessagingReceiveMessage, aReqStatus, aMsgData, aMsgAttributes);
	}

EXPORT_C void RMobileUssdMessaging::SendMessage(TRequestStatus& aReqStatus, const TDesC8& aMsgData, const TDesC8& aMsgAttributes) const
/**
 * This method sends an outgoing USSD to the network
 *
 * \param aMsgData Supplies the USSD message data to send
 * \param aMsgAttributes Supplies the attributes of the outgoing USSD
 */
	{
	Set(EMobileUssdMessagingSendMessage, aReqStatus, aMsgData, aMsgAttributes);
	}

