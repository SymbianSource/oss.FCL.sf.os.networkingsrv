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
// This file contains all the method definitions for RMobilePhone.  It also contains the
// entry point for the Etelmm Dll.
// 
//

// From core API
#include "ETELEXT.H"

// Multimode header files
#include "ETELMMCS.H"
#include "ETELMM.H"
#include "mmlist.h"
#include "mm_hold.h"

/*GLDEF_C TInt E32Dll(TDllReason)
//
// DLL entry point
//
	{
	return KErrNone;
	}
*/
/************************************************************************/
//
//  TMobileAddress
//
/************************************************************************/

EXPORT_C RMobilePhone::TMobileAddress::TMobileAddress() 
	: iTypeOfNumber(EUnknownNumber),
	  iNumberPlan(EUnknownNumberingPlan)
	{
	}

void RMobilePhone::TMobileAddress::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the mobile address from a stream 
 *
 * \param aStream The read stream containing the mobile address
 */
	{
	iTypeOfNumber=STATIC_CAST(TMobileTON, aStream.ReadUint32L());
	iNumberPlan=STATIC_CAST(TMobileNPI, aStream.ReadUint32L());
	aStream >> iTelNumber;
	}

void RMobilePhone::TMobileAddress::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the mobile address into a stream 
 *
 * \param aStream The write stream that will contain the mobile address
 */
	{
	aStream.WriteUint32L(iTypeOfNumber);
	aStream.WriteUint32L(iNumberPlan);
	aStream << iTelNumber;
	}


/************************************************************************/
//
//  RMobilePhone
//
/************************************************************************/

EXPORT_C RMobilePhone::RMobilePhone()
	: iMmPtrHolder(NULL)
	{
	}

EXPORT_C void RMobilePhone::ConstructL()
	{
	RPhone::ConstructL();
	__ASSERT_ALWAYS(iMmPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iMmPtrHolder = CMobilePhonePtrHolder::NewL(CMobilePhonePtrHolder::EMaxNumberPhonePtrSlots,CMobilePhonePtrHolder::EMaxNumberPhonePtrCSlots);
	}

EXPORT_C void RMobilePhone::Destruct()
	{
	RPhone::Destruct();
	delete iMmPtrHolder;
	iMmPtrHolder = NULL;
	}

/************************************************************************/
//
//  TMultimodeType
//
/************************************************************************/

RMobilePhone::TMultimodeType::TMultimodeType()
	{}

EXPORT_C TInt RMobilePhone::TMultimodeType::ExtensionId() const
/**
 * This method returns the multimode API extension number of the class
 *
 * \return TInt An integer that will indicate the version of the type
 */
	{
	return iExtensionId;
	}

void RMobilePhone::TMultimodeType::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the multimode type from a stream 
 *
 * \param aStream The read stream containing the multimode type
 */
	{
	iExtensionId=aStream.ReadInt32L();	
	}

void RMobilePhone::TMultimodeType::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the multimode type into a stream 
 *
 * \param aStream The write stream that will contain the multimode type
 */
	{
	aStream.WriteInt32L(iExtensionId);
	}


/***********************************************************************************/
//
// MobilePhoneCapability functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetMultimodeAPIVersion(TInt& aVersion) const
/**
 * This method returns the current version of the multimode ETel API
 *
 * \param aVersion Will contain the current version of the multimode ETel API
 * \return KErrNone
 */
	{
	aVersion=KETelExtMultimodeV1;
	return KErrNone;
	}

EXPORT_C TInt RMobilePhone::GetMultimodeCaps(TUint32& aCaps) const
/**
 * This method returns the multimode capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the multimode capabilities
 * \return KErrNone
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetMultimodeCaps, ptr1);
	}

EXPORT_C void RMobilePhone::GetPhoneStoreInfo(TRequestStatus& aReqStatus, TDes8& aInfo, const TDesC& aStoreName) const
/**
 * This method returns the information related to a particular phone store
 *
 * \param aStoreName Specifies the name of the store, for which information is required
 * \retval aInfo A descriptor that will contain the phone store information
 */
	{
	SetAndGet(EMobilePhoneGetPhoneStoreInfo,aReqStatus, aInfo, aStoreName);
	}

/***********************************************************************************/
//
// MobilePhoneSimAccess functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetIccAccessCaps(TUint32& aCaps) const
/**
 * This method returns the ICC (Integrated circuit card) access capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the ICC access capabilities
 * \return KErrNone
 * \exception KErrNotSupported if ICC access is never supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetIccAccessCaps, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyIccAccessCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the ICC access capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new ICC access capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyIccAccessCapsChange,aCaps);

	Get(EMobilePhoneNotifyIccAccessCapsChange,aReqStatus,ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneCspFileV1::TMobilePhoneCspFileV1()
	: iCallOfferingServices(0),iCallRestrictionServices(0),iOtherSuppServices(0),
	iCallCompletionServices(0),iTeleservices(0),iCphsTeleservices(0),iCphsFeatures(0),
	iNumberIdentServices(0),iPhase2PlusServices(0),iValueAddedServices(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetCustomerServiceProfile(TRequestStatus& aReqStatus, TDes8& aCsp) const
/**
 * This method returns the Customer Service Profile (CSP) stored on the SIM
 *
 * \retval aCsp A descriptor that will contain the CSP information
 */
	{
	Get(EMobilePhoneGetCustomerServiceProfile, aReqStatus, aCsp);
	}

EXPORT_C RMobilePhone::TMobilePhoneServiceTableV1::TMobilePhoneServiceTableV1()
	: iServices1To8(0),iServices9To16(0),iServices17To24(0),iServices25To32(0),
	iServices33To40(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetServiceTable(TRequestStatus& aReqStatus, TMobilePhoneServiceTable aTable, TDes8& aSst) const
/**
 * This method returns the Service Table (SIM or CDMA) stored on the ICC
 *
 * \param aTable Specifies whether the SIM or CDMA service table is to be retrieved
 * \retval aTable A descriptor that will contain the service table information
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iServiceTable = aTable;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1GetServiceTable, iMmPtrHolder->iServiceTable);

	SetAndGet(EMobilePhoneGetServiceTable, aReqStatus, ptr1, aSst);
	}

/***********************************************************************************/
//
// MobilePhonePower functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetBatteryCaps(TUint32& aCaps) const
/**
 * This method returns the battery capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the battery capabilities
 * \return KErrNone
 * \exception KErrNotSupported if battery information is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetBatteryCaps, ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneBatteryInfoV1::TMobilePhoneBatteryInfoV1() :
		iStatus(EPowerStatusUnknown), iChargeLevel(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetBatteryInfo(TRequestStatus& aReqStatus, TMobilePhoneBatteryInfoV1& aInfo) const
/**
 * This method returns the current battery information of the phone
 *
 * \retval aInfo Will contain the battery information
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetBatteryInfo,aInfo);

	Get(EMobilePhoneGetBatteryInfo,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyBatteryInfoChange(TRequestStatus& aReqStatus, TMobilePhoneBatteryInfoV1& aInfo) const
/**
 * This notification completes if the battery information changes
 *
 * \retval aInfo Will contain the new battery information
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyBatteryInfoChange,aInfo);

	Get(EMobilePhoneNotifyBatteryInfoChange,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// MobilePhoneSignal functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetSignalCaps(TUint32& aCaps) const
/**
 * This method returns the signal strength capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the signal strength capabilities
 * \return KErrNone
 * \exception KErrNotSupported if signal strength information is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetSignalCaps, ptr1);
	}

EXPORT_C void RMobilePhone::GetSignalStrength(TRequestStatus& aReqStatus, TInt32& aSignalStrength, TInt8& aBar) const
/**
 * This method returns the current signal strength of the phone
 *
 * \retval aSignalStrength Will contain the signal strength, expressed in dBm
 * \retval aBar Will contain the number of bars of signal strength to display
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetSignalStrength,aSignalStrength);
	TPtr8& ptr2=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2GetSignalStrength,aBar);

	Get(EMobilePhoneGetSignalStrength,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifySignalStrengthChange(TRequestStatus& aReqStatus, TInt32& aSignalStrength, TInt8& aBar) const
/**
 * This notification completes if the signal strength of the phone changes
 *
 * \retval aSignalStrength Will contain the new signal strength, expressed in dBm
 * \retval aBar Will contain the new number of bars of signal strength to display
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifySignalStrengthChange,aSignalStrength);
	TPtr8& ptr2=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2NotifySignalStrengthChange,aBar);

	Get(EMobilePhoneNotifySignalStrengthChange, aReqStatus, ptr1, ptr2);
	}

/***********************************************************************************/
//
// MobilePhoneIndicator functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetIndicatorCaps(TUint32& aActionCaps, TUint32& aIndCaps) const
/**
 * This method returns the indicator capabilities of the phone
 *
 * \retval aActionCaps An integer that will contain the bit-wise sum of TMobilePhoneIndicatorCaps flags
 * \retval aIndCaps An integer that will contain the bit-wise sum of supported TMobilePhoneIndicators flags
 * \return KErrNone
 * \exception KErrNotSupported if indicators are not supported
 */
	{
	TPckg<TUint32> ptr1(aActionCaps);
	TPckg<TUint32> ptr2(aIndCaps);
	return Get(EMobilePhoneGetIndicatorCaps, ptr1, ptr2);
	}

EXPORT_C void RMobilePhone::GetIndicator(TRequestStatus& aReqStatus, TUint32& aIndicator) const
/**
 * This method returns the current value of the supported indicators from the phone
 *
 * \retval aIndicator Will contain bit-wise sum of the current values of each indicator
 * \exception KErrNotSupported if indicators are not supported
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetIndicator,aIndicator);

	Get(EMobilePhoneGetIndicator,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyIndicatorChange(TRequestStatus& aReqStatus, TUint32& aIndicator) const
/**
 * This notification completes if any of the supported indicators change state
 *
 * \retval aIndicator Will contain bit-wise sum of the new values of each indicator
 * \exception KErrNotSupported if indicators are not supported
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyIndicatorChange,aIndicator);

	Get(EMobilePhoneNotifyIndicatorChange, aReqStatus, ptr1);
	}

/***********************************************************************************/
//
// MobilePhoneIdentity functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetIdentityCaps(TUint32& aCaps) const
/**
 * This method returns the identity capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the identity capabilities
 * \return KErrNone
 * \exception KErrNotSupported if phone and subscriber identity information is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetIdentityCaps, ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneIdentityV1::TMobilePhoneIdentityV1()
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetPhoneId(TRequestStatus& aReqStatus, TMobilePhoneIdentityV1& aId) const
/**
 * This method returns the identity of the phone
 *
 * \retval aId Will contain the identity, which can consist of manufacturer, model, revision and serial numbers
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetPhoneId,aId);

	Get(EMobilePhoneGetPhoneId, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::GetSubscriberId(TRequestStatus& aReqStatus, TMobilePhoneSubscriberId& aId) const
/**
 * This method returns the identity of the subscriber (IMSI)
 *
 * \retval aId Will contain the identity
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetSubscriberId,aId);

	Get(EMobilePhoneGetSubscriberId, aReqStatus, ptr1);
	}

/***********************************************************************************/
//
// MobilePhoneDTMF functional unit
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetDTMFCaps(TUint32& aCaps) const
/**
 * This method returns the DTMF capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the DTMF capabilities
 * \return KErrNone
 * \exception KErrNotSupported if DTMF is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetDTMFCaps, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyDTMFCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the DTMF capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new DTMF capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyDTMFCapsChange, aCaps);

	Get(EMobilePhoneNotifyDTMFCapsChange, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::SendDTMFTones(TRequestStatus& aReqStatus, const TDesC& aTones) const
/**
 * This method sends a series of DTMF tones across a connected and active voice call
 *
 * \param aTones Supplies the tones to send
 */
	{
	Set(EMobilePhoneSendDTMFTones, aReqStatus, aTones);
	}

EXPORT_C TInt RMobilePhone::StartDTMFTone(TChar aTone) const
/**
 * This method starts the transmission of a single DTMF tone across a connected and active voice call
 *
 * \param aTone Supplies the tone to send
 */
	{
	TPckgC<TChar> ptr1(aTone);
	return Set(EMobilePhoneStartDTMFTone,ptr1);
	}

EXPORT_C TInt RMobilePhone::StopDTMFTone() const
/**
 * This method stops the transmission of a single DTMF tone across a connected and active voice call
 */
	{
	return Blank(EMobilePhoneStopDTMFTone);
	}

EXPORT_C void RMobilePhone::NotifyStopInDTMFString(TRequestStatus& aRequestStatus) const
/**
 * This notification completes if a stop character is found within a DTMF string
 */
	{
	Blank(EMobilePhoneNotifyStopInDTMFString,aRequestStatus);
	}

EXPORT_C TInt RMobilePhone::ContinueDTMFStringSending(TBool aContinue) const
/**
 * This method either continues or cancels the sending of a string of DTMF tones
 * It assumes that a stop character has previously been found within the string
 *
 * \param aContinue A boolean that specifies whether the sending will continue or stop
 */
	{
	TPckg<TBool> ptr1(aContinue);
	return Set(EMobilePhoneContinueDTMFStringSending,ptr1);
	}

/***********************************************************************************/
//
// MobilePhoneNetwork functional unit
//
/***********************************************************************************/

EXPORT_C RMobilePhone::TMobilePhoneNetworkInfoV1::TMobilePhoneNetworkInfoV1()
:	iMode(ENetworkModeUnknown),
	iStatus(ENetworkStatusUnknown),
	iBandInfo(EBandUnknown)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobilePhone::TMobilePhoneNetworkInfoV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the mobile network information from a stream 
 *
 * \param aStream The read stream containing the mobile network information
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iMode=STATIC_CAST(TMobilePhoneNetworkMode, aStream.ReadUint32L());
	iStatus=STATIC_CAST(TMobilePhoneNetworkStatus, aStream.ReadUint32L());
	iBandInfo=STATIC_CAST(TMobilePhoneNetworkBandInfo, aStream.ReadUint32L());
	aStream >> iCountryCode;
	aStream >> iAnalogSID;
	aStream >> iNetworkId;
	aStream >> iDisplayTag;
	aStream >> iShortName;
	aStream >> iLongName;
	}

void RMobilePhone::TMobilePhoneNetworkInfoV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the mobile network information into a stream 
 *
 * \param aStream The write stream that will contain the mobile network information
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteUint32L(iMode);
	aStream.WriteUint32L(iStatus);
	aStream.WriteUint32L(iBandInfo);
	aStream << iCountryCode;
	aStream << iAnalogSID;
	aStream << iNetworkId;
	aStream << iDisplayTag;
	aStream << iShortName;
	aStream << iLongName;
	}

EXPORT_C TInt RMobilePhone::GetNetworkCaps(TUint32& aCaps) const
/**
 * This method returns the network capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the network capabilities
 * \return KErrNone
 * \exception KErrNotSupported if network access/information is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetNetworkCaps, ptr1);
	}

EXPORT_C TInt RMobilePhone::GetCurrentMode(TMobilePhoneNetworkMode& aNetworkMode) const
/**
 * This method returns the current mode of the phone
 *
 * \retval aNetworkMode Will contain the mode (GSM, WCDMA, CDMA, TDMA, AMPS)
 */
	{
	TPckg<TMobilePhoneNetworkMode> ptr1(aNetworkMode);
	return Get(EMobilePhoneGetCurrentMode, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyModeChange(TRequestStatus& aReqStatus, TMobilePhoneNetworkMode& aNetworkMode) const
/**
 * This notification completes if the mode of the phone changes
 *
 * \retval aNetworkMode Will contain the new mode (GSM, WCDMA, CDMA, TDMA, AMPS)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyModeChange, aNetworkMode);

	Get(EMobilePhoneNotifyModeChange,aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::GetHomeNetwork(TRequestStatus& aReqStatus, TDes8& aNetworkInfo) const
/**
 * This method returns information on the subscriber's home network
 *
 * \retval aNetworkInfo Will contain the home network information
 */
	{
	Get(EMobilePhoneGetHomeNetwork, aReqStatus, aNetworkInfo);
	}

EXPORT_C RMobilePhone::TMobilePhoneLocationAreaV1::TMobilePhoneLocationAreaV1() 
:	iAreaKnown(EFalse), iLocationAreaCode(0), iCellId(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetCurrentNetwork(TRequestStatus& aReqStatus, TDes8& aNetworkInfo, TMobilePhoneLocationAreaV1& aArea) const
/**
 * This method returns information on the phone's current serving network
 *
 * \retval aNetworkInfo Will contain the information related to the current network
 * \retval aArea Will contain the phone's current location area
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr2=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetCurrentNetwork, aArea);

	Get(EMobilePhoneGetCurrentNetwork, aReqStatus, aNetworkInfo, ptr2);
	}

EXPORT_C void RMobilePhone::NotifyCurrentNetworkChange(TRequestStatus& aReqStatus, TDes8& aNetworkInfo, TMobilePhoneLocationAreaV1& aArea) const
/**
 * This notification completes if the serving network of the phone changes
 *
 * \retval aNetworkInfo Will contain the information related to the new network
 * \retval aArea Will contain the phone's new location area
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr2=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCurrentNetworkChange, aArea);

	Get(EMobilePhoneNotifyCurrentNetworkChange, aReqStatus, aNetworkInfo, ptr2);
	}

EXPORT_C void RMobilePhone::GetNetworkRegistrationStatus(TRequestStatus& aReqStatus, TMobilePhoneRegistrationStatus& aStatus) const
/**
 * This method returns the phone's current network registration status
 *
 * \retval aStatus Will contain the network registration status
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetNetworkRegistrationStatus, aStatus);

	Get(EMobilePhoneGetNetworkRegistrationStatus, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyNetworkRegistrationStatusChange(TRequestStatus& aReqStatus, TMobilePhoneRegistrationStatus& aStatus) const
/**
 * This notification completes if the network registration status of the phone changes
 *
 * \retval aStatus Will contain the new network registration status
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyNetworkRegistrationStatusChange, aStatus);

	Get(EMobilePhoneNotifyNetworkRegistrationStatusChange, aReqStatus, ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneNetworkSelectionV1::TMobilePhoneNetworkSelectionV1()
	: iMethod(ENetworkSelectionUnknown), 
	  iBandClass(ENetworkBandClassUnknown), 
	  iOperationMode(ENetworkOperationUnknown)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobilePhone::GetNetworkSelectionSetting(TDes8& aSetting) const
/**
 * This method returns the phone's current network selection setting
 *
 * \retval aSetting Will contain the network selection setting
 */
	{
	return Get(EMobilePhoneGetNetworkSelectionSetting, aSetting);
	}

EXPORT_C void RMobilePhone::SetNetworkSelectionSetting(TRequestStatus& aReqStatus, const TDes8& aSetting) const
/**
 * This method sets a new value for the phone's network selection setting
 *
 * \param aSetting Supplies the new network selection setting
 */
	{
	Set(EMobilePhoneSetNetworkSelectionSetting,aReqStatus,aSetting);
	}

EXPORT_C void RMobilePhone::NotifyNetworkSelectionSettingChange(TRequestStatus& aReqStatus, TDes8& aSetting) const
/**
 * This notification completes if the network selection setting of the phone changes
 *
 * \retval aSetting Will contain the new setting
 */
	{
	Get(EMobilePhoneNotifyNetworkSelectionSettingChange, aReqStatus, aSetting);
	}

EXPORT_C void RMobilePhone::SelectNetwork(TRequestStatus& aReqStatus, TBool aIsManual, const TMobilePhoneNetworkManualSelection& aManualSelection) const
/**
 * This method instructs the phone to initiate network selection
 *
 * \param aIsManual Specifies whether phone should manual or automatic network selection method
 * \param aManualSelection If aIsManual=ETrue, then this parameter contain the user's manually selected network
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iIsManual = aIsManual;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SelectNetwork, iMmPtrHolder->iIsManual);
	TPtrC8& ptr2=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SelectNetwork, aManualSelection);

	Set(EMobilePhoneSelectNetwork,aReqStatus, ptr1, ptr2);
	}

EXPORT_C RMobilePhone::TMobilePhoneNITZ::TMobilePhoneNITZ() 
	: iNitzFieldsUsed(0), iTimeZone(0), iDST(0)
	{
	}

EXPORT_C RMobilePhone::TMobilePhoneNITZ::TMobilePhoneNITZ(TInt aYear, TMonth aMonth, TInt aDay, TInt aHour, TInt aMinute, TInt aSecond, TInt aMicroSecond) 
	: TDateTime(aYear,aMonth,aDay,aHour,aMinute,aSecond,aMicroSecond), iNitzFieldsUsed(0), iTimeZone(0), iDST(0)
	{
	}

EXPORT_C TInt RMobilePhone::GetNITZInfo(TMobilePhoneNITZ& aNITZInfo) const
/**
 * This method returns the current snapshot of network time & date information
 *
 * \retval aNITZInfo Will contain the time & date information
 */
	{
	TPckg<TMobilePhoneNITZ> ptr1(aNITZInfo);
	return Get(EMobilePhoneGetNITZInfo,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyNITZInfoChange(TRequestStatus& aReqStatus, TMobilePhoneNITZ& aNITZInfo) const
/**
 * This notification completes if the time & date information sent by the network changes
 *
 * \retval aNITZInfo Will contain the new time & date information
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyNITZInfoChange,aNITZInfo);
	Get(EMobilePhoneNotifyNITZInfoChange,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// MobilePrivacy functional unit
//
/***********************************************************************************/
EXPORT_C void RMobilePhone::NotifyDefaultPrivacyChange(TRequestStatus& aReqStatus, TMobilePhonePrivacy& aSetting) const
/**
 * This notification completes if the default voice privacy setting of the phone changes
 *
 * \retval aSetting An enum that will contain the new privacy setting
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyDefaultPrivacyChange, aSetting);

	Get(EMobilePhoneNotifyDefaultPrivacyChange, aReqStatus, ptr1);
	}

/***********************************************************************************/
//
// TSY Capabilities for supplementary call services
//
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetCallServiceCaps(TUint32& aCaps) const
/**
 * This method returns the supplementary call service capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the call service caps
 * \return KErrNone
 * \exception KErrNotSupported if user network access is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetCallServiceCaps, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyCallServiceCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the call service capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new call service capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCallServiceCapsChange, aCaps);

	Get(EMobilePhoneNotifyCallServiceCapsChange, aReqStatus, ptr1);
	}

/***********************************************************************************/
//
// MobilePhoneUserNetworkAccess functional unit
//
/***********************************************************************************/

EXPORT_C void RMobilePhone::ProgramFeatureCode(TRequestStatus& aReqStatus, const TDesC& aFCString, TMobilePhoneNetworkService aService, TMobilePhoneServiceAction aAction) const
/**
 * This method programs a feature code string against a network service action
 *
 * \param aFCString Supplies the feature code string to be programmed
 * \param aService Specifies which service is applicable to the feature code string
 * \param aAction Specifies which action is applicable to the feature code string
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iMmPtrHolder->iProgramFeatureCode.iService = aService;
	iMmPtrHolder->iProgramFeatureCode.iAction = aAction;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1ProgramFeatureCode,iMmPtrHolder->iProgramFeatureCode);

	Set(EMobilePhoneProgramFeatureCode,aReqStatus, ptr1, aFCString);
	}

EXPORT_C void RMobilePhone::GetFeatureCode(TRequestStatus& aReqStatus, TDes& aFCString, TMobilePhoneNetworkService aService, TMobilePhoneServiceAction aAction) const
/**
 * This method returns the feature code string programmed against a network service action
 *
 * \param aService Specifies which service is applicable to the feature code string
 * \param aAction Specifies which action is applicable to the feature code string
 * \retval aFCString Will contain the feature code string programmed
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iMmPtrHolder->iGetFeatureCode.iService = aService;
	iMmPtrHolder->iGetFeatureCode.iAction = aAction;
	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetFeatureCode,iMmPtrHolder->iGetFeatureCode);

	Get(EMobilePhoneGetFeatureCode,aReqStatus, ptr1, aFCString);
	}

EXPORT_C void RMobilePhone::SendNetworkServiceRequest(TRequestStatus& aReqStatus, const TDesC& aServiceString) const
/**
 * This method sends a request to the network in the form of a character and digit string
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 * \param aServiceString Supplies the supplementary service or feature code string to be sent
 */
	{
	Set(EMobilePhoneSendNetworkServiceRequest, aReqStatus, aServiceString);
	}

/***********************************************************************************/
//
// MobilePhoneCallForwarding functional unit
// 
/***********************************************************************************/

EXPORT_C RMobilePhone::TMobilePhoneCFInfoEntryV1::TMobilePhoneCFInfoEntryV1() 
:	iCondition(ECallForwardingUnspecified), 
	iServiceGroup(EServiceUnspecified),
	iStatus(ECallForwardingStatusUnknown),
	iTimeout(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobilePhone::TMobilePhoneCFInfoEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the call forwarding information from a stream 
 *
 * \param aStream The read stream containing the call forwarding information
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iCondition=STATIC_CAST(TMobilePhoneCFCondition, aStream.ReadUint32L());
	iServiceGroup=STATIC_CAST(TMobileService, aStream.ReadUint32L());
	iStatus=STATIC_CAST(TMobilePhoneCFStatus, aStream.ReadUint32L());
	aStream >> iNumber;
	iTimeout=aStream.ReadInt32L();
	}

void RMobilePhone::TMobilePhoneCFInfoEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the call forwarding information into a stream 
 *
 * \param aStream The write stream that will contain the call forwarding information
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteUint32L(iCondition);
	aStream.WriteUint32L(iServiceGroup);
	aStream.WriteUint32L(iStatus);
	aStream << iNumber;
	aStream.WriteInt32L(iTimeout);
	}

EXPORT_C void RMobilePhone::NotifyCallForwardingStatusChange(TRequestStatus& aReqStatus, TMobilePhoneCFCondition& aCondition) const
/**
 * This notification completes if the status of a call forwarding service changes
 *
 * \retval aCondition Will contain the name of the changed service (CFU, CFB, CFNRc, CFNRy)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCallForwardingStatusChange,aCondition);

	Get(EMobilePhoneNotifyCallForwardingStatusChange,aReqStatus, ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneCFChangeV1::TMobilePhoneCFChangeV1() 
:	iServiceGroup(EServiceUnspecified),
	iAction(EServiceActionUnspecified), 
	iTimeout(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::SetCallForwardingStatus(TRequestStatus& aReqStatus, TMobilePhoneCFCondition aCondition, const TMobilePhoneCFChangeV1& aInfo) const
/**
 * This method sets the call forwarding status for incoming calls across all lines
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 * \param aCondition Specifies which call forwarding service (CFU, CFB, CFNRy, CFNRc) is being set
 * \param aInfo Supplies the new status and/or registered information of the call forwarding service, as applied to all basic services (i.e. all lines)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	iMmPtrHolder->iSetCFCondition = aCondition;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetCallForwardingStatus,iMmPtrHolder->iSetCFCondition);

	TPtrC8& ptr2=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SetCallForwardingStatus,aInfo);

	Set(EMobilePhoneSetCallForwardingStatus, aReqStatus, ptr1, ptr2);
	}

EXPORT_C void RMobilePhone::NotifyCallForwardingActive(TRequestStatus& aReqStatus, TMobileService& aServiceGroup, TMobilePhoneCFActive& aActiveType) const
/**
 * This notification completes if a call is made on this line while call forwarding is active on it
 *
 * \retval aActiveType Will indicate whether unconditional (CFU) or one of the conditional (CFB, CFNRy, CFNRc) services is active
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1PhoneNotifyCallForwardingActive, aServiceGroup);
	TPtr8& ptr2=iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2PhoneNotifyCallForwardingActive, aActiveType);

	Get(EMobilePhoneNotifyCallForwardingActive, aReqStatus, ptr1, ptr2);
	}

/***********************************************************************************/
//
// Mobile Identity Service functional unit
// 
/***********************************************************************************/

EXPORT_C void RMobilePhone::GetIdentityServiceStatus(TRequestStatus& aReqStatus, const TMobilePhoneIdService aService, TMobilePhoneIdServiceStatus& aStatus, TMobileInfoLocation aLocation) const
/**
 * This method returns the current status of the specified identity service
 *
 * \param aService Specifies which identity service (CLIP, CLIR, COLP, COLR etc.) is being interrogated
 * \retval aStatus Will contain the current status of the service
 * \param aLocation Specifies whether the information should be retrieved from phone cache or network
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iGetIdentityServiceStatus.iLocation = aLocation;
	iMmPtrHolder->iGetIdentityServiceStatus.iService = aService;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1GetIdentityServiceStatus,iMmPtrHolder->iGetIdentityServiceStatus);
	TPtr8& ptr2 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2GetIdentityServiceStatus,aStatus);

	SetAndGet(EMobilePhoneGetIdentityServiceStatus,aReqStatus,ptr1,ptr2);
	}

/***********************************************************************************/
//
// Mobile Call Barring functional unit
// 
/***********************************************************************************/

EXPORT_C RMobilePhone::TMobilePhoneCBInfoEntryV1::TMobilePhoneCBInfoEntryV1() 
:	iCondition(EBarUnspecified), 
	iServiceGroup(EServiceUnspecified), 
	iStatus(ECallBarringStatusUnknown)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

void RMobilePhone::TMobilePhoneCBInfoEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the call barring inforamation from a stream 
 *
 * \param aStream The read stream containing the call barring inforamation
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iCondition = STATIC_CAST(TMobilePhoneCBCondition,aStream.ReadUint32L());
	iServiceGroup = STATIC_CAST(TMobileService,aStream.ReadUint32L());
	iStatus = STATIC_CAST(TMobilePhoneCBStatus,aStream.ReadUint32L());
	}

void RMobilePhone::TMobilePhoneCBInfoEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the call barring information into a stream 
 *
 * \param aStream The write stream that will contain the call barring information
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteUint32L(iCondition);
	aStream.WriteUint32L(iServiceGroup);
	aStream.WriteUint32L(iStatus);
	}

EXPORT_C RMobilePhone::TMobilePhoneCBChangeV1::TMobilePhoneCBChangeV1() 
:	iServiceGroup(EServiceUnspecified),
	iAction(EServiceActionUnspecified)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::SetCallBarringStatus(TRequestStatus& aReqStatus, TMobilePhoneCBCondition aCondition, const TMobilePhoneCBChangeV1& aInfo) const
/**
 * This method sets the call barring status for calls across all lines
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 * \param aCondition Specifies which call barring program (BAOC, BIC etc) is being set
 * \param aInfo Supplies the new status of the call barring service, as applied to all basic services (i.e. all lines)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetCBStatusCondition = aCondition;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetCallBarringStatus,iMmPtrHolder->iSetCBStatusCondition);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SetCallBarringStatus,aInfo);

	Set(EMobilePhoneSetCallBarringStatus,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifyCallBarringStatusChange(TRequestStatus& aReqStatus, TMobilePhoneCBCondition& aCondition) const
/**
 * This notification completes if the status of a call barring program changes
 *
 * \retval aCondition Will contain the name of the changed barring program (BAOC, BIC etc.)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCallBarringStatusChange,aCondition);

	Get(EMobilePhoneNotifyCallBarringStatusChange,aReqStatus,ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhonePasswordChangeV1::TMobilePhonePasswordChangeV1()
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::SetCallBarringPassword(TRequestStatus& aReqStatus, const TMobilePhonePasswordChangeV1& aPassword) const
/**
 * This method sets the call barring password that is part of the subscription for any call barring program
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 * \param aPassword Supplies the old and new call barring passwords
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetCallBarringPassword,aPassword);

	Set(EMobilePhoneSetCallBarringPassword,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// Mobile Call Waiting functional unit
// 
/***********************************************************************************/

void RMobilePhone::TMobilePhoneCWInfoEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the call waiting inforamation from a stream 
 *
 * \param aStream The read stream containing the call waiting inforamation
 */
	{
	TMultimodeType::InternalizeL(aStream);
	iServiceGroup = STATIC_CAST(TMobileService,aStream.ReadUint32L());
	iStatus = STATIC_CAST(TMobilePhoneCWStatus,aStream.ReadUint32L());
	}

void RMobilePhone::TMobilePhoneCWInfoEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the call waiting information into a stream 
 *
 * \param aStream The write stream that will contain the call waiting information
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream.WriteUint32L(iServiceGroup);
	aStream.WriteUint32L(iStatus);
	}

EXPORT_C RMobilePhone::TMobilePhoneCWInfoEntryV1::TMobilePhoneCWInfoEntryV1() 
:	iServiceGroup(EServiceUnspecified), 
	iStatus(ECallWaitingStatusUnknown)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::SetCallWaitingStatus(TRequestStatus& aReqStatus, TMobileService aServiceGroup, TMobilePhoneServiceAction aAction) const
/**
 * This method sets the status of the call waiting service for all lines
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 * \param aAction Supplies the new status of the call waiting service, as applied to all basic services
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetCallWaitingStatusGroup = aServiceGroup;
	iMmPtrHolder->iSetCallWaitingStatusAction = aAction;
	
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetCallWaitingStatus,iMmPtrHolder->iSetCallWaitingStatusGroup);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SetCallWaitingStatus,iMmPtrHolder->iSetCallWaitingStatusAction);

	Set(EMobilePhoneSetCallWaitingStatus,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifyCallWaitingStatusChange(TRequestStatus& aReqStatus, TDes8& aCWStatus) const
/**
 * This notification completes if the status of the call waiting service changes
 *
 * \retval aCWStatus Will contain the new status of the call waiting service
 */
	{
	Get(EMobilePhoneNotifyCallWaitingStatusChange,aReqStatus,aCWStatus);
	}


/***********************************************************************************/
//
// Mobile Call Completion functional unit
// 
/***********************************************************************************/
	
EXPORT_C void RMobilePhone::GetCCBSStatus(TRequestStatus& aReqStatus, TMobilePhoneCCBSStatus& aCcbsStatus, TMobileInfoLocation aLocation) const
/**
 * This method returns the current status of the CCBS service
 *
 * \retval aCcbsStatus Will contain the current status of the service
 * \param aLocation Specifies whether the information should be retrieved from phone cache or network
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iGetCCBSStatusLocation = aLocation;
	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetCCBSStatus,aCcbsStatus);
	TPtr8& ptr2 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2GetCCBSStatus,iMmPtrHolder->iGetCCBSStatusLocation);

	Get(EMobilePhoneGetCCBSStatus,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifyCCBSStatusChange(TRequestStatus& aReqStatus, TMobilePhoneCCBSStatus& aCcbsStatus) const
/**
 * This notification completes if the status of the CCBS service changes
 *
 * \retval aCcbsStatus Will contain the new status of the CCBS service
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCCBSStatusChange,aCcbsStatus);

	Get(EMobilePhoneNotifyCCBSStatusChange,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::DeactivateAllCCBS(TRequestStatus& aReqStatus) const
/**
 * This method deactivates all currently active CCBS requests
 * It will complete either when the network has confirmed the request or after the phone has sent it
 * depending upon whether network confirmation is supported by serving network
 *
 */
	{
	Blank(EMobilePhoneDeactivateAllCCBS,aReqStatus);
	}

void RMobilePhone::TMobilePhoneCCBSEntryV1::InternalizeL(RReadStream& aStream)
/**
 * This method internalizes the CCBS inforamation from a stream 
 *
 * \param aStream The read stream containing the CCBS inforamation
 */
	{
	TMultimodeType::InternalizeL(aStream);
	aStream >> iCallName;
	iServiceGroup = STATIC_CAST(TMobileService,aStream.ReadUint32L());
	iCcbsIndex = STATIC_CAST(TInt,aStream.ReadUint32L());
	aStream >> iDestination;
	}

void RMobilePhone::TMobilePhoneCCBSEntryV1::ExternalizeL(RWriteStream& aStream) const
/**
 * This method externalizes the CCBS information into a stream 
 *
 * \param aStream The write stream that will contain the CCBS information
 */
	{
	TMultimodeType::ExternalizeL(aStream);
	aStream << iCallName;
	aStream.WriteInt32L(iServiceGroup);
	aStream.WriteInt32L(iCcbsIndex);
	aStream << iDestination;
	}

EXPORT_C RMobilePhone::TMobilePhoneCCBSEntryV1::TMobilePhoneCCBSEntryV1() 
:	iCcbsIndex(0), iServiceGroup(EServiceUnspecified)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

/***********************************************************************************/
//
// Mobile Alternating Call functional unit
// 
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetAlternatingCallCaps(TUint32& aCaps) const
/**
 * This method returns the alternating call capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the alternating call capabilities
 * \return KErrNone
 * \exception KErrNotSupported if alternating calls are not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetAlternatingCallCaps,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyAlternatingCallCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the alternating call capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new alternating call capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyAlternatingCallCapsChange,aCaps);

	Get(EMobilePhoneNotifyAlternatingCallCapsChange,aReqStatus,ptr1);
	}

EXPORT_C TInt RMobilePhone::GetAlternatingCallMode(TMobilePhoneAlternatingCallMode& aMode, TMobileService& aFirstService) const
/**
 * This method returns the current alternating call mode
 *
 * \retval aMode Will contain the current alternating call mode
 * \retval aFirstService Will contain the service that will be first in an alternating call (voice, data, fax)
 */
	{
	TPckg<TMobilePhoneAlternatingCallMode> ptr1(aMode);
	TPckg<TMobileService> ptr2(aFirstService);

	return Get(EMobilePhoneGetAlternatingCallMode,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::SetAlternatingCallMode(TRequestStatus& aReqStatus, TMobilePhoneAlternatingCallMode aMode, TMobileService aFirstService) const
/**
 * This method sets a new value for alternating call mode
 *
 * \param aMode Specifies the new alternating call mode
 * \param aFirstService Specifies the service that will be first in an alternating call (voice, data, fax)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetAlternatingCallModeMode = aMode;
	iMmPtrHolder->iSetAlternatingCallModeService = aFirstService;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetAlternatingCallMode,iMmPtrHolder->iSetAlternatingCallModeMode);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SetAlternatingCallMode,iMmPtrHolder->iSetAlternatingCallModeService);
	
	Set(EMobilePhoneSetAlternatingCallMode,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifyAlternatingCallModeChange(TRequestStatus& aReqStatus, TMobilePhoneAlternatingCallMode& aMode,TMobileService& aFirstService) const
/**
 * This notification completes if the alternating call mode of the phone changes
 *
 * \retval aMode Will contain the new alternating call mode
 * \retval aFirstService Will contain the service that will be first in an alternating call (voice, data, fax)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyAlternatingCallModeChange,aMode);
	TPtr8& ptr2 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot2NotifyAlternatingCallModeChange,aFirstService);

	Get(EMobilePhoneNotifyAlternatingCallModeChange,aReqStatus,ptr1,ptr2);
	}

/***********************************************************************************/
//
// Mobile Alternate Line Service functional unit
// 
/***********************************************************************************/


EXPORT_C TInt RMobilePhone::GetALSLine(TMobilePhoneALSLine& aALSLine) const
/**
 * This method returns the current ALS line selected
 *
 * \retval aALSLine Will contain the ALS line selected
 */
	{
	TPckg<TMobilePhoneALSLine> ptr1(aALSLine);
	return Get(EMobilePhoneGetALSLine,ptr1);
	}

EXPORT_C void RMobilePhone::SetALSLine(TRequestStatus& aReqStatus, TMobilePhoneALSLine aALSLine) const
/**
 * This method sets a new values for the ALS line selected
 *
 * \param aALSLine Specifies the new ALS line selected
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetALSLine = aALSLine;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetALSLine,iMmPtrHolder->iSetALSLine);

	Set(EMobilePhoneSetALSLine,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyALSLineChange(TRequestStatus& aReqStatus, TMobilePhoneALSLine& aALSLine) const
/**
 * This notification completes if the ALS line selected changes
 *
 * \retval aALSLine Will contain the new ALS line selected
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyALSLineChange,aALSLine);

	Get(EMobilePhoneNotifyALSLineChange,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// Mobile Cost functional unit
// 
/***********************************************************************************/


EXPORT_C TInt RMobilePhone::GetCostCaps(TUint32& aCaps) const
/**
 * This method returns the call cost information capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the call cost information capabilities
 * \return KErrNone
 * \exception KErrNotSupported if call cost information is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetCostCaps,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyCostCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the call cost information capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new call cost information capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyCostCapsChange,aCaps);

	Get(EMobilePhoneNotifyCostCapsChange,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::ClearCostMeter(TRequestStatus& aReqStatus, TMobilePhoneCostMeters aMeter) const
/**
 * This method clears the Accumulated Cost Meter (ACM) on the SIM
 * It will complete either when the SIM confirms that ACM is cleared or after the phone has
 * been denied access due to the requirement for PIN2 entry first
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iClearCostMeter = aMeter;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1ClearCostMeter,iMmPtrHolder->iClearCostMeter);

	Set(EMobilePhoneClearCostMeter,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::SetMaxCostMeter(TRequestStatus& aReqStatus, TUint aUnits) const
/**
 * This method sets a new value for the Max Accumulated Cost Meter (ACMmax) on the SIM
 * It will complete either when the SIM confirms that ACMmax is set or after the phone has
 * been denied access due to the requirement for PIN2 entry first
 *
 * \param aUnits Specifies the number of units to set ACMmax to
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetMaxCostMeterUnits = aUnits;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetMaxCostMeter,iMmPtrHolder->iSetMaxCostMeterUnits);

	Set(EMobilePhoneSetMaxCostMeter,aReqStatus,ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhonePuctV1::TMobilePhonePuctV1() 
: iPricePerUnit(0.0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

	
EXPORT_C void RMobilePhone::SetPuct(TRequestStatus& aReqStatus, const TDesC8& aPuct) const
/**
 * This method sets a new value for the Price Per Unit & Currency Table (PUCT) on the SIM
 * It will complete either when the SIM confirms that PUCT is set or after the phone has
 * been denied access due to the requirement for PIN2 entry first
 *
 * \param aPuct Supplies the new PUCT setting
 */
	{
	Set(EMobilePhoneSetPuct,aReqStatus,aPuct);
	}

EXPORT_C RMobilePhone::TMobilePhoneCostInfoV1::TMobilePhoneCostInfoV1() 
:	iService(ECostServiceUnknown), 
	iCCM(0), 
	iACM(0), 
	iACMmax(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}
	
EXPORT_C void RMobilePhone::GetCostInfo(TRequestStatus& aReqStatus, TDes8& aCostInfo) const
/**
 * This method returns the current snapshot of the phone's call cost information
 *
 * \retval aCostInfo Will contain the cost information
 */
	{
	Get(EMobilePhoneGetCostInfo,aReqStatus,aCostInfo);
	}

EXPORT_C void RMobilePhone::NotifyCostInfoChange(TRequestStatus& aReqStatus, TDes8& aCostInfo) const
/**
 * This notification completes if the call cost information changes
 *
 * \retval aCostInfo Will contain the new call cost information
 */
	{
	Get(EMobilePhoneNotifyCostInfoChange,aReqStatus,aCostInfo);
	}


/***********************************************************************************/
//
// Mobile Security functional unit
// 
/***********************************************************************************/

EXPORT_C TInt RMobilePhone::GetSecurityCaps(TUint32& aCaps) const
/**
 * This method returns the security capabilities of the phone
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the security capabilities
 * \return KErrNone
 * \exception KErrNotSupported if security is not supported
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobilePhoneGetSecurityCaps,ptr1);
	}

EXPORT_C void RMobilePhone::NotifySecurityCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the security capabilities of the phone change
 *
 * \retval aCaps An integer that will contain the new security capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifySecurityCapsChange,aCaps);

	Get(EMobilePhoneNotifySecurityCapsChange,aReqStatus,ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneLockInfoV1::TMobilePhoneLockInfoV1() 
:	iStatus(EStatusLocked), 
	iSetting(ELockSetEnabled)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetLockInfo(TRequestStatus& aReqStatus, TMobilePhoneLock aLock, TDes8& aLockInfo) const
/**
 * This method returns the current status and setting of the specified lock
 *
 * \param aLock Specifies which lock is being interrogated
 * \retval aLockInfo Will contain the lock's current status and setting
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iLock = aLock;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1GetLockInfo,iMmPtrHolder->iLock);

	SetAndGet(EMobilePhoneGetLockInfo,aReqStatus,ptr1,aLockInfo);
	}

EXPORT_C void RMobilePhone::NotifyLockInfoChange(TRequestStatus& aReqStatus, TMobilePhoneLock& aLock, TDes8& aLockInfo) const
/**
 * This notification completes if the status or information of a lock changes
 *
 * \retval aLockInfo Will contain the new lock status and information
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyLockInfoChange,aLock);
	
	Get(EMobilePhoneNotifyLockInfoChange,aReqStatus,ptr1,aLockInfo);
	}

EXPORT_C void RMobilePhone::SetLockSetting(TRequestStatus& aReqStatus, TMobilePhoneLock aLock, TMobilePhoneLockSetting aSetting) const
/**
 * This method sets a new value for the setting of the specified lock
 *
 * \param aLock Specifies which lock is being set
 * \param aSetting Supplies the lock's new setting
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetLockSettingLock = aLock;
	iMmPtrHolder->iSetLockSettingSetting = aSetting;

	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetLockSetting,iMmPtrHolder->iSetLockSettingLock);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2SetLockSetting,iMmPtrHolder->iSetLockSettingSetting);

	Set(EMobilePhoneSetLockSetting,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::ChangeSecurityCode(TRequestStatus& aReqStatus, TMobilePhoneSecurityCode aType, const TMobilePhonePasswordChangeV1& aChange) const
/**
 * This method changes the value of the specified security code
 *
 * \param aType Specifies which security code is being changed
 * \param aChange Supplies the old and new values for the security code
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iChangeSecurityCodeType = aType;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1ChangeSecurityCode,iMmPtrHolder->iChangeSecurityCodeType);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2ChangeSecurityCode,aChange);

	Set(EMobilePhoneChangeSecurityCode,aReqStatus,ptr1,ptr2);
	}

EXPORT_C void RMobilePhone::NotifySecurityEvent(TRequestStatus& aReqStatus, TMobilePhoneSecurityEvent& aEvent) const
/**
 * This notification completes if the phone recognises that a security event has occurred
 *
 * \retval aEvent Will contain the security event
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifySecurityEvent,aEvent);

	Get(EMobilePhoneNotifySecurityEvent,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::VerifySecurityCode(TRequestStatus& aReqStatus, TMobilePhoneSecurityCode aType, const TMobilePassword& aCode,
		const TMobilePassword& aUnblockCode) const
/**
 * This method verifies the user's code agains the specified stored security code
 *
 * \param aType Specifies which security code is being verified
 * \param aCode Supplies the user's code
 * \param aUnblockCode Supplies the user's unblock code which may be needed if the user is actually unblocking a security code (e.g. PIN1 or PIN2)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iVerifySecurityCodeType = aType;
	iMmPtrHolder->iVerifySecurityCodeData.iCode = aCode;
	iMmPtrHolder->iVerifySecurityCodeData.iUnblockCode = aUnblockCode;
	
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1VerifySecurityCode,iMmPtrHolder->iVerifySecurityCodeType);
	TPtrC8& ptr2 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot2VerifySecurityCode,iMmPtrHolder->iVerifySecurityCodeData);

	Set(EMobilePhoneVerifySecurityCode,aReqStatus,ptr1,ptr2);	
	}

EXPORT_C TInt RMobilePhone::AbortSecurityCode(TMobilePhoneSecurityCode aType) const
/**
 * This method informs the phone that the user has cancelled the request for a security code to be entered
 */
	{
	TPckg<TMobilePhoneSecurityCode> ptr1(aType);
	return Set(EMobilePhoneAbortSecurityCode,ptr1);
	}

EXPORT_C RMobilePhone::TMobilePhoneMulticallSettingsV1::TMobilePhoneMulticallSettingsV1()
	: iUserMaxBearers(-1),iServiceProviderMaxBearers(-1),iNetworkSupportedMaxBearers(-1),
	iUESupportedMaxBearers(-1)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::GetMulticallParams(TRequestStatus& aReqStatus, TDes8& aMulticallParams) const
/** 
 * This method retrieves the maximum number of simultaneous CS service bearers defined by 
 * the user, defined by the service provider, supported by the network and supported by the phone.
 *
 * \param aMulticallParams Will contain the multicall bearer settings
 */
	{
	Get(EMobilePhoneGetMulticallParams, aReqStatus, aMulticallParams);	
	}

EXPORT_C void RMobilePhone::SetMulticallParams(TRequestStatus& aReqStatus, TInt aUserMaxBearers) const
/**
 * This method is used by client application to set the maximum number of 
 * simultaneous CS bearers specified by the user (iUserMaxBearers)
 *
 * \param aUserMaxBearers Supplies the new user specified value
 */	
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iUserMaxBearers = aUserMaxBearers;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetMulticallParams, iMmPtrHolder->iUserMaxBearers);

	Set(EMobilePhoneSetMulticallParams,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyMulticallParamsChange(TRequestStatus& aReqStatus, TDes8& aMulticallParams) const
/**
 * This notification completes if the multicall parameters of the phone change
 *
 * \retval aMulticallParams Will contain the new Multicall parameters set by the network
 */
	{
	Get(EMobilePhoneNotifyMulticallParamsChange, aReqStatus, aMulticallParams);
	}

/***********************************************************************************/
//
// Mobile Message Waiting functional unit
// 
/***********************************************************************************/

EXPORT_C RMobilePhone::TMobilePhoneMessageWaitingV1::TMobilePhoneMessageWaitingV1()
: iVoiceMsgs(0), iAuxVoiceMsgs(0), iDataMsgs(0), iFaxMsgs(0), iEmailMsgs(0), iOtherMsgs(0)
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C void RMobilePhone::NotifyMessageWaiting(TRequestStatus& aReqStatus, TInt& aCount) const
/**
 * This notification completes if the phone receives a "message waiting" message from the network
 *
 * \retval aCount Will contain the number of voicemail messages waiting
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyMessageWaiting,aCount);

	Get(EMobilePhoneNotifyMessageWaiting,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::GetIccMessageWaitingIndicators(TRequestStatus& aReqStatus, TDes8& aMsgIndicators) const
/**
 * This method retrieves the set of message waiting indicators from the current ICC
 *
 * \retval aMsgIndicators Will contain the type and number of waiting messages
 */
	{
	Get(EMobilePhoneGetIccMessageWaitingIndicators,aReqStatus,aMsgIndicators);
	}

EXPORT_C void RMobilePhone::SetIccMessageWaitingIndicators(TRequestStatus& aReqStatus, const TDesC8& aMsgIndicators) const
/**
 * This method sets the message waiting indicators on the current ICC
 *
 * \retval aMsgIndicators Will contain the type and number of waiting messages
 */
	{
	Set(EMobilePhoneSetIccMessageWaitingIndicators,aReqStatus,aMsgIndicators);
	}

EXPORT_C void RMobilePhone::NotifyIccMessageWaitingIndicatorsChange(TRequestStatus& aReqStatus, TDes8& aMsgIndicators) const
/**
 * This notification completes if the message waiting indicators change on the current ICC
 *
 * \retval aMsgIndicators Will contain the type and number of waiting messages
 */
	{
	Get(EMobilePhoneNotifyIccMessageWaitingIndicatorsChange,aReqStatus,aMsgIndicators);
	}

/***********************************************************************************/
//
// Mobile Fixed Dialling Numbers functional unit
// 
/***********************************************************************************/


EXPORT_C TInt RMobilePhone::GetFdnStatus(TMobilePhoneFdnStatus& aFdnStatus) const
/**
 * This method returns the current status of the Fixed Dialling Number (FDN) service
 *
 * \retval aFdnStatus Will contain the current status of FDN
 */
	{
	TPckg<TMobilePhoneFdnStatus> ptr1(aFdnStatus);

	return Get(EMobilePhoneGetFdnStatus,ptr1);
	}

EXPORT_C void RMobilePhone::SetFdnSetting(TRequestStatus& aReqStatus, TMobilePhoneFdnSetting aFdnSetting) const
/**
 * This method sets a new value for the status of the Fixed Dialling Number (FDN) service
 *
 * \retval aFdnSetting Supplies the new status of FDN
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetFdnSetting = aFdnSetting;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetFdnSetting,iMmPtrHolder->iSetFdnSetting);
	
	Set(EMobilePhoneSetFdnSetting,aReqStatus,ptr1);
	}

EXPORT_C void RMobilePhone::NotifyFdnStatusChange(TRequestStatus& aReqStatus, TMobilePhoneFdnStatus& aFdnStatus) const
/**
 * This notification completes if the status of the FDN service changes
 *
 * \retval aFdnStatus Will contain the new FDN status
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyFdnStatusChange,aFdnStatus);

	Get(EMobilePhoneNotifyFdnStatusChange,aReqStatus,ptr1);
	}

/************************************************************************************/
//
// Single Numbering Scheme functional unit
//
/************************************************************************************/


EXPORT_C void RMobilePhone::GetIncomingCallType(TRequestStatus& aReqStatus, TMobilePhoneIncomingCallType& aCallType, TDes8& aDataParams) const
/**
 * This method retrieves the current setting of the incoming call type
 *
 * \retval aType Will contain the incoming call types supported by the phone
 * 
 * \param aDataParams Will contain the data bearer service settings if aType
 *  indicates that incoming calls will include a data bearer element
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetIncomingCallType, aCallType);

	Get(EMobilePhoneGetIncomingCallType, aReqStatus, ptr1, aDataParams);
	}

EXPORT_C void RMobilePhone::SetIncomingCallType(TRequestStatus& aReqStatus, TMobilePhoneIncomingCallType aCallType, TDes8& aDataParams) const
/**
 * This method sets the incoming call type.
 *
 * \param aCallType Supplies the new settings for the incoming call types
 * \param aDataParam Suppies the new data settings in the case of a data bearer
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iCallType = aCallType;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetIncomingCallType, iMmPtrHolder->iCallType);

	Set(EMobilePhoneSetIncomingCallType, aReqStatus, ptr1, aDataParams);
	}

EXPORT_C void RMobilePhone::NotifyIncomingCallTypeChange(TRequestStatus& aReqStatus, TMobilePhoneIncomingCallType& aCallType, TDes8& aDataParams) const
/**
 * This method notifies the client if the setting of the incoming call type changes. 
 *
 * \retval aCallType Will contain the new incoming call type.
 * 
 * \param aDataParams Will contain the data bearer service settings if aCallType
 *  indicates that incoming calls will include a data bearer element
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyIncomingCallTypeChange, aCallType);

	Get(EMobilePhoneNotifyIncomingCallTypeChange, aReqStatus, ptr1, aDataParams);
	}

/************************************************************************************/
//
// User-To-User Signalling functional unit
//
/************************************************************************************/


EXPORT_C void RMobilePhone::GetUUSSetting(TRequestStatus& aReqStatus, TMobilePhoneUUSSetting& aSetting) const
/**
 * This method retrieves the current User-User Signalling settings of the phone
 *
 * \retval aSetting An integer that will contain the current UUS settings
 * \return KErrNone
 * \exception KErrNotSupported if UUS functionality is not supported by the phone/TSY
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));
	
	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1GetUUSSetting, aSetting);
	Get(EMobilePhoneGetUUSSetting, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::SetUUSSetting(TRequestStatus& aReqStatus, TMobilePhoneUUSSetting aSetting) const
/**
 * This method sets the User-User Signalling settings of the phone
 *
 * \param aSetting Supplies the new UUS settings
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iSetUUSSetting = aSetting;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobilePhonePtrHolder::ESlot1SetUUSSetting, iMmPtrHolder->iSetUUSSetting);
	
	Set(EMobilePhoneSetUUSSetting, aReqStatus, ptr1);
	}

EXPORT_C void RMobilePhone::NotifyUUSSettingChange(TRequestStatus& aReqStatus, TMobilePhoneUUSSetting& aSetting) const
/**
 * This notification completes if the User-User Signalling settings of the phone change
 *
 * \retval aCaps An integer that will contain the new UUS settings
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobilePhonePtrHolder::ESlot1NotifyUUSSettingChange,aSetting);

	Get(EMobilePhoneNotifyUUSSettingChange, aReqStatus, ptr1);
	}

/***********************************************************************************/
