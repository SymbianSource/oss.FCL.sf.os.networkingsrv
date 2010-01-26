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
// Contains method implementations for RMobileCall subsession
// 
//

#include "ETELMMCS.H"
#include "ETELEXT.H"
#include "ETELMM.H"
#include "mm_hold.h"

/************************************************************************/
//
//  RMobileCall
//
/************************************************************************/

EXPORT_C RMobileCall::RMobileCall()
	: iMmPtrHolder(NULL)
	{
	}

EXPORT_C void RMobileCall::ConstructL()
	{
	RCall::ConstructL();
	__ASSERT_ALWAYS(iMmPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iMmPtrHolder = CMobileCallPtrHolder::NewL(CMobileCallPtrHolder::EMaxNumberCallPtrSlots,CMobileCallPtrHolder::EMaxNumberCallPtrCSlots);
	}

EXPORT_C void RMobileCall::Destruct()
	{
	RCall::Destruct();
	delete iMmPtrHolder;
	iMmPtrHolder = NULL;
	}

EXPORT_C RMobileCall::TMobileCallParamsV1::TMobileCallParamsV1() 
:	RCall::TCallParams(),
	iIdRestrict(EIdRestrictDefault), 
	iAutoRedial(EFalse)
	{
	iCug.iExplicitInvoke = EFalse;
	iCug.iCugIndex = 0;
	iCug.iSuppressOA = EFalse;
	iCug.iSuppressPrefCug = EFalse;
	iExtensionId = KETelMobileCallParamsV1; //overwrite iExtensionId setup in RCall::TCallParams c'tor
	}

/***********************************************************************************/
//
// Mobile Call Data Functional Unit
//
/***********************************************************************************/

EXPORT_C RMobileCall::TMobileCallCugV1::TMobileCallCugV1() :
	iExplicitInvoke(EFalse), 
	iCugIndex(0), 
	iSuppressOA(EFalse), 
	iSuppressPrefCug(EFalse)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C RMobileCall::TMobileCallDataCapsV1::TMobileCallDataCapsV1() : 
	iSpeedCaps(0), 
	iProtocolCaps(0), 
	iServiceCaps(0), 
	iQoSCaps(0), 
	iHscsdSupport(EFalse),
	iMClass(0), 
	iMaxRxTimeSlots(0), 
	iMaxTxTimeSlots(0), 
	iTotalRxTxTimeSlots(0), 
	iCodingCaps(0),
	iAsymmetryCaps(0),
	iUserInitUpgrade(EFalse),
	iRLPVersionCaps(0),
	iV42bisCaps(0)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileCall::GetMobileDataCallCaps(TDes8& aCaps) const
/**
 * This method returns the mobile call's circuit switched data capabilities
 *
 * \retval aCaps A descriptor that will contain the circuit switched data capabilities
 * \return KErrNone
 * \exception KErrNotSupported if call does not support circuit switched data
 */
	{
	return Get(EMobileCallGetMobileDataCallCaps,aCaps);
	}

EXPORT_C void RMobileCall::NotifyMobileDataCallCapsChange(TRequestStatus& aReqStatus, TDes8& aCaps) const
/**
 * This notification completes when the mobile call's circuit switched data capabilities change
 *
 * \retval aCaps A descriptor that will contain the new circuit switched data capabilities
 */
	{
	Get(EMobileCallNotifyMobileDataCallCapsChange,aReqStatus,aCaps);
	}

EXPORT_C RMobileCall::TMobileDataRLPRangesV1::TMobileDataRLPRangesV1() :
	iIWSMax(0),
	iIWSMin(0),
	iMWSMax(0),
	iMWSMin(0),
	iT1Max(0),
	iT1Min(0),
	iN2Max(0),
	iN2Min(0),
	iT4Max(0),
	iT4Min(0)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C void RMobileCall::GetMobileDataCallRLPRange(TRequestStatus& aReqStatus, TInt aRLPVersion, TDes8& aRLPRange) const
/**
 * This method method returns the minimum and maximum RLP parameter ranges supported by the 
 * phone for the specified RLP version.
 *
 * \retval aRLPRange A descriptor that will contain the RLP parameter ranges
 * \param aRLPVersion Will contain the RLP version in the aRLPVersion parameter 
 * \exception KErrNotSupported if the phone does not support the RLP version supplied
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iRLPVersion = aRLPVersion;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot1GetMobileDataCallRLPRange, iMmPtrHolder->iRLPVersion);

	SetAndGet(EMobileCallGetMobileDataCallRLPRange, aReqStatus, ptr1, aRLPRange);
	}

EXPORT_C RMobileCall::TMobileDataCallParamsV1::TMobileDataCallParamsV1() 
:	TMobileCallParamsV1(),
	iService(EServiceUnspecified),
	iSpeed(ESpeedUnspecified), 
	iProtocol(EProtocolUnspecified), 
	iQoS(EQoSUnspecified),
	iRLPVersion(ERLPNotRequested),
	iModemToMSWindowSize(0),
	iMSToModemWindowSize(0),
	iAckTimer(0),
	iRetransmissionAttempts(0),
	iResequencingPeriod(0),
	iV42bisReq(EV42bisNeitherDirection),
	iV42bisCodewordsNum(0),
	iV42bisMaxStringLength(0)
/**
 * Constructor - initialises all T-class member data
 */
	{
	iExtensionId = KETelMobileDataCallParamsV1; //overwrite iExtensionId setup in TMobileCallParamsV1 c'tor
	}

EXPORT_C RMobileCall::TMobileHscsdCallParamsV1::TMobileHscsdCallParamsV1() 
:	TMobileDataCallParamsV1(),
	iWantedAiur(EAiurBpsUnspecified),
	iWantedRxTimeSlots(0), 
	iMaxTimeSlots(0), 
	iCodings(0),
	iAsymmetry(EAsymmetryNoPreference),
	iUserInitUpgrade(EFalse)
/**
 * Constructor - initialises all T-class member data
 */
	{
	iExtensionId = KETelMobileHscsdCallParamsV1; //overwrite iExtensionId setup in TMobileDataCallParamsV1 c'tor
	}
	
EXPORT_C void RMobileCall::SetDynamicHscsdParams(TRequestStatus& aReqStatus, TMobileCallAiur aAiur, TInt aRxTimeslots) const
/**
 * This method sets new values for the dynamic parameters of a mobile HSCSD call
 * It will complete after the phone has requested the new parameters
 *
 * \param aAiur Wanted air interface user rate
 * \param aRxTimeslots Wanted number of receive (downlink) timeslots
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iAiur = aAiur;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot1SetDynamicHscsdParams, iMmPtrHolder->iAiur);

	iMmPtrHolder->iRxTimeslots = aRxTimeslots;
	TPtrC8& ptr2=iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot2SetDynamicHscsdParams, iMmPtrHolder->iRxTimeslots);

	Set(EMobileCallSetDynamicHscsdParams,aReqStatus,ptr1,ptr2);
	}

EXPORT_C RMobileCall::TMobileCallHscsdInfoV1::TMobileCallHscsdInfoV1() : 
	iAiur(EAiurBpsUnspecified), 
	iRxTimeSlots(0), 
	iTxTimeSlots(0), 
	iCodings(ETchCodingUnspecified)
/**
 * Constructor - initialises all T-class member data
 */
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileCall::GetCurrentHscsdInfo(TDes8& aHSCSDInfo) const
/**
 * This method returns the current values of the HSCSD parameters of an active mobile HSCSD call 
 *
 * \retval aHSCSDInfo A descriptor that will contain the retrieved HSCSD information
 * \return KErrNone if HSCSD information successfully retrieved
 * \exception KErrEtelCallNotActive if call is not an active HSCSD call
 * \exception KErrNotSupported if call does not support HSCSD
 */
	{
	return Get(EMobileCallGetCurrentHscsdInfo,aHSCSDInfo);
	}

EXPORT_C void RMobileCall::NotifyHscsdInfoChange(TRequestStatus& aReqStatus, TDes8& aHSCSDInfo) const
/**
 * This notification completes when the mobile call's HSCSD parameters change
 *
 * \retval aHSCSDInfo A descriptor that will contain the new HSCSD information
 */
	{
	Get(EMobileCallNotifyHscsdInfoChange,aReqStatus,aHSCSDInfo);			
	}

/***********************************************************************************/
//
// Voice Fallback for Multimedia Calls functional unit
//
/***********************************************************************************/

EXPORT_C void RMobileCall::NotifyVoiceFallback(TRequestStatus& aReqStatus, TName& aCallName) const
/**
 * This method notifies the client if a Multimedia call falls back to a voice call 
 * due to inability in network or terminating end to comply with the Multimedia request
 *
 * \retval aCallName Will contain the name of a new voice call object created by the TSY
 */
	{
	Get(EMobileCallNotifyVoiceFallback, aReqStatus, aCallName);
	}

/***********************************************************************************/
//
// MobileCall Alternating Call functional unit
//
/***********************************************************************************/


EXPORT_C void RMobileCall::SwitchAlternatingCall(TRequestStatus& aReqStatus) const
/**
 * This method switches an alternating call to its opposite call mode
 * It will complete once the call switch has either succeeded or failed
 */
	{
	Blank(EMobileCallSwitchAlternatingCall,aReqStatus);
	}

EXPORT_C void RMobileCall::NotifyAlternatingCallSwitch(TRequestStatus& aReqStatus) const
/**
 * This notification completes when an alternating call successfully switches call mode
 */
	{
	Blank(EMobileCallNotifyAlternatingCallSwitch,aReqStatus);
	}

/***********************************************************************************/
//
// MobileCallControl functional unit
//
/***********************************************************************************/

EXPORT_C RMobileCall::TMobileCallCapsV1::TMobileCallCapsV1() : 
	iCallControlCaps(0),
	iCallEventCaps(0)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileCall::GetMobileCallCaps(TDes8& aCaps) const
/**
 * This method returns the mobile call's call control & event capabilities
 *
 * \retval aCaps A descriptor that will contain the call control & event capabilities
 * \return KErrNone
 */
	{
	return Get(EMobileCallGetMobileCallCaps, aCaps);
	}

EXPORT_C void RMobileCall::NotifyMobileCallCapsChange(TRequestStatus& aReqStatus, TDes8& aCaps) const
/**
 * This notification completes when the mobile call's call control & event capabilities change
 *
 * \retval aCaps A descriptor that will contain the new call control & event capabilities
 */
	{
	Get(EMobileCallNotifyMobileCallCapsChange, aReqStatus, aCaps);
	}

EXPORT_C TInt RMobileCall::GetMobileCallStatus(TMobileCallStatus& aStatus) const
/**
 * This method returns the current status of a mobile call 
 *
 * \retval aStatus Will contain the retrieved call state
 * \return KErrNone
 */
	{
	TPckg<TMobileCallStatus> ptr1(aStatus);
	return Get(EMobileCallGetMobileCallStatus, ptr1);
	}

EXPORT_C void RMobileCall::NotifyMobileCallStatusChange(TRequestStatus& aReqStatus, TMobileCallStatus& aStatus) const
/**
 * This notification completes when the mobile call's state changes
 *
 * \retval aStatus Will contain the new call state
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobileCallPtrHolder::ESlot1NotifyMobileCallStatusChange, aStatus);

	Get(EMobileCallNotifyMobileCallStatusChange, aReqStatus, ptr1);
	}

EXPORT_C void RMobileCall::Hold(TRequestStatus& aReqStatus) const
/**
 * This method puts a call on hold
 * It will complete when the network confirms that the call is in the hold state
 */
	{
	Blank(EMobileCallHold,aReqStatus);
	}

EXPORT_C void RMobileCall::Resume(TRequestStatus& aReqStatus) const
/**
 * This method resumes a call that is on hold
 * It will complete when the network confirms that the call is in the active state
 */
	{
	Blank(EMobileCallResume, aReqStatus);
	}

EXPORT_C void RMobileCall::Swap(TRequestStatus& aReqStatus) const
/**
 * This method swaps a connected call to its opposite state, either active or on hold
 * It will complete when the network confirms that the call has changed state
 */
	{
	Blank(EMobileCallSwap, aReqStatus);
	}

EXPORT_C void RMobileCall::Deflect(TRequestStatus& aReqStatus, TMobileCallDeflect aDeflectType, const RMobilePhone::TMobileAddress& aDestination) const
/**
 * This method deflects an incoming call to another destination
 * It will complete when the network confirms that the deflection was either successful or failed
 *
 * \param aDestination The number to deflect the call to
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));


	iMmPtrHolder->iDeflectType = aDeflectType;
	TPtrC8& ptr1=iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot1Deflect, iMmPtrHolder->iDeflectType);
	TPtrC8& ptr2=iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot2Deflect, aDestination);

	Set(EMobileCallDeflect, aReqStatus, ptr1, ptr2);
	}

EXPORT_C void RMobileCall::Transfer(TRequestStatus& aReqStatus) const
/**
 * This method transfers an ongoing call to a third party, while this party drops out of the call
 * It will complete when the network confirms that the transfer was either successful or failed
 */
	{
	Blank(EMobileCallTransfer,aReqStatus);
	}

EXPORT_C void RMobileCall::GoOneToOne(TRequestStatus& aReqStatus) const
/**
 * This method enables this single call to become a private conversation within a multiparty call
 * It will complete when the network confirms that the "one-to-one" is either successful or failed
 */
	{
	Blank(EMobileCallGoOneToOne,aReqStatus);
	}

EXPORT_C void RMobileCall::NotifyCallEvent(TRequestStatus& aReqStatus, TMobileCallEvent& aEvent) const
/**
 * This notification completes when a mobile call event occurs
 *
 * \retval aEvent Will contain the call event
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobileCallPtrHolder::ESlot1NotifyCallEvent, aEvent);

	Get(EMobileCallNotifyCallEvent, aReqStatus, ptr1);
	}

/***********************************************************************************/
//
// MobileCallPrivacy functional unit
//
/***********************************************************************************/


EXPORT_C TInt RMobileCall::SetPrivacy(RMobilePhone::TMobilePhonePrivacy aPrivacySetting) const
/**
 * This method sets a new value for the privacy setting of a mobile call
 * It will complete after the phone has requested the new setting
 *
 * \param aPrivacySetting Wanted privacy setting (either on or off)
 * \return KErrNone if request processed successfully
 * \exception KErrMMEtelWrongMode if request made during a mode that does not support it
 */
	{
	TPckg<RMobilePhone::TMobilePhonePrivacy> ptr1(aPrivacySetting);
	return Set(EMobileCallSetPrivacy, ptr1);	
	}

EXPORT_C void RMobileCall::NotifyPrivacyConfirmation(TRequestStatus& aReqStatus, RMobilePhone::TMobilePhonePrivacy& aPrivacySetting) const
/**
 * This notification completes when the network confirms whether privacy is on or off for this call
 *
 * \retval aPrivacySetting Confirmed privacy setting (either on or off)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileCallPtrHolder::ESlot1NotifyPrivacyConfirmation,aPrivacySetting);

	Get(EMobileCallNotifyPrivacyConfirmation,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// MobileCallTrafficChannel Functional Unit
//
/***********************************************************************************/



EXPORT_C TInt RMobileCall::SetTrafficChannel(TMobileCallTch aTchRequest) const
/**
 * This method sets a new value for the traffic channel setting of a mobile call
 * It will complete after the phone has requested the new setting
 *
 * \param aTchRequest Wanted traffic channel (either analog or digital)
 * \return KErrNone if request processed successfully
 * \exception KErrMMEtelWrongMode if request made during a mode that does not support it
 */
	{
	TPckg<TMobileCallTch> ptr1(aTchRequest);
	return Set(EMobileCallSetTrafficChannel,ptr1);
	}

EXPORT_C void RMobileCall::NotifyTrafficChannelConfirmation(TRequestStatus& aReqStatus, TMobileCallTch& aTchType) const
/**
 * This notification completes when the network confirms what type of traffic channel has been allocated for this call
 *
 * \retval aTchType Confirmed traffic channel type (either analog or digital)
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileCallPtrHolder::ESlot1NotifyTrafficChannelConfirmation,aTchType);

	Get(EMobileCallNotifyTrafficChannelConfirmation,aReqStatus,ptr1);
	}

/***********************************************************************************/
//
// MobileCallInformation functional unit
//
/***********************************************************************************/

EXPORT_C RMobileCall::TMobileCallInfoV1::TMobileCallInfoV1() 
:   iValid(0), 
	iService(RMobilePhone::EServiceUnspecified), 
	iStatus(EStatusUnknown), 
	iCallId(-1), 
	iExitCode(0), 
	iEmergency(EFalse),
	iForwarded(EFalse),
	iPrivacy(RMobilePhone::EPrivacyUnspecified),
	iAlternatingCall(RMobilePhone::EAlternatingModeUnspecified),
	iDuration(0),
	iTch(RMobileCall::ETchUnknown)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileCall::GetMobileCallInfo(TDes8& aCallInfo) const
/**
 * This method retrieves the current snapshort of all information related to this call
 *
 * \retval aCallInfo A descriptor containing the large block of call information
 * \return KErrNone
 * \exception KErrNotFound if call information is not available
 */
	{
	return Get(EMobileCallGetMobileCallInfo, aCallInfo);
	}

EXPORT_C RMobileCall::TMobileCallRemotePartyInfoV1::TMobileCallRemotePartyInfoV1() 
	: iRemoteIdStatus(ERemoteIdentityUnknown), iDirection(EDirectionUnknown)
	{
	iExtensionId = KETelExtMultimodeV1;
	}

EXPORT_C void RMobileCall::NotifyRemotePartyInfoChange(TRequestStatus& aReqStatus, TDes8& aRemotePartyInfo) const
/**
 * This notification completes if there are any changes in the information related to the remote party of this call
 *
 * \retval aRemotePartyInfo A descriptor containing the new remote party information
 */
	{
	Get(EMobileCallNotifyRemotePartyInfoChange, aReqStatus, aRemotePartyInfo);
	}

/***********************************************************************************/
//
// MobileCallEmergency functional unit
// 
/***********************************************************************************/

EXPORT_C void RMobileCall::DialEmergencyCall(TRequestStatus& aReqStatus, const TDesC& aNumber) const
/**
 * This method dials an emergency call using the number specified in the aNumber parameter
 * It will complete after the network confirms the call is connected
 *
 * \param aNumber Supplies the emergency number
 */
	{
	Set(EMobileCallDialEmergencyCall, aReqStatus, aNumber);
	}

/***********************************************************************************/
//
// MobileCallCompletion functional unit
// 
/***********************************************************************************/


EXPORT_C void RMobileCall::ActivateCCBS(TRequestStatus& aReqStatus) const
/**
 * This method activates a CCBS request on a call that has failed due to remote user busy
 * It will complete once the activate request has been confirmed by the network
 */
	{
	Blank(EMobileCallActivateCCBS,aReqStatus);
	}

EXPORT_C TInt RMobileCall::RejectCCBS() const
/**
 * This method rejects the possibility of activating a CCBS request
 *
 * \return KErrNone
 * \exception KErrNotReady if call is not expecting a CCBS rejection
 */
	{
	return Blank(EMobileCallRejectCCBS);
	}

EXPORT_C void RMobileCall::AcceptCCBSRecall(TRequestStatus& aReqStatus) const
/**
 * This method accepts the CCBS recall so that the MO call is set-up to the remote party
 * It will complete once the call is connected to the remote party
 */
	{
	Blank(EMobileCallAcceptCCBSRecall,aReqStatus);
	}

EXPORT_C void RMobileCall::DeactivateCCBS(TRequestStatus& aReqStatus) const
/**
 * This method deactivates a CCBS request that is still waiting for the remote party to become free
 * It will complete once the deactivate request has been confirmed by the network
 */
	{
	Blank(EMobileCallDeactivateCCBS,aReqStatus);
	}

/************************************************************************************/
//
// User-To-User Signalling functional unit
//
/************************************************************************************/

EXPORT_C RMobileCall::TMobileCallUUSRequestV1::TMobileCallUUSRequestV1()
	{
	iExtensionId=KETelExtMultimodeV1;
	}

EXPORT_C TInt RMobileCall::GetUUSCaps(TUint32& aCaps) const
/**
 * This method returns the current instance of the User-User Signalling capabilities of the call
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the UUS capabilities
 * \return KErrNone
 * \exception KErrNotSupported if UUS functionality is not supported by the phone/TSY
 */
	{
	TPckg<TUint32> ptr1(aCaps);
	return Get(EMobileCallGetUUSCaps, ptr1);
	}

EXPORT_C void RMobileCall::NotifyUUSCapsChange(TRequestStatus& aReqStatus, TUint32& aCaps) const
/**
 * This notification completes if the UUS capabilities of the call change
 *
 * \retval aCaps An integer that will contain the bit-wise sum of the new UUS capabilities
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1 = iMmPtrHolder->Set(CMobileCallPtrHolder::ESlot1NotifyUUSCapsChange, aCaps);

	Get(EMobileCallNotifyUUSCapsChange, aReqStatus, ptr1);
	}

EXPORT_C void RMobileCall::ActivateUUS(TRequestStatus& aReqStatus, const TDesC8& aUUSRequest) const
/** 
 * This method activates the UUS Services either before or during a call
 *
 * \param aUUSRequest Contains the UUS service activation requests and any UUI data
 */
	{
	Set(EMobileCallActivateUUS, aReqStatus, aUUSRequest);
	}

EXPORT_C void RMobileCall::SendUUI(TRequestStatus& aReqStatus, TBool aMore, const TMobileCallUUI& aUUI) const
/** 
 * This method sends a UUI message for UUS2 and UUS3 services to the remote user either during 
 * the call set-up or during the actual call.
 *
 * \param aUUI Supplies the UUI message data
 * \param aMore If aMore=ETrue, then there is another UUI message to follow
 * \exception KErrGsmCCAccessInformationDiscarded if congestion forces the network to drop the UUI
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	iMmPtrHolder->iMoreUUI = aMore;
	TPtrC8& ptr1 = iMmPtrHolder->SetC(CMobileCallPtrHolder::ESlot2SendUUI, iMmPtrHolder->iMoreUUI);

	Set(EMobileCallSendUUI, aReqStatus, ptr1, aUUI);
	}

EXPORT_C void RMobileCall::ReceiveUUI(TRequestStatus& aReqStatus, TMobileCallUUI& aUUI) const
/** 
 * This method receives an incoming UUI message from the remote side.
 *
 * \retval aUUI Will contain the UUI message data
 */
	{
	Get(EMobileCallReceiveUUI, aReqStatus, aUUI);
	}

EXPORT_C void RMobileCall::HangupWithUUI(TRequestStatus& aReqStatus, const TMobileCallUUI& aUUI) const
/**
 *  This overloaded HangUp method sends a UUS1 data at the call release
 *
 * \param aUUI Supplies the UUI message data
 */
	{
	Set(EMobileCallHangupWithUUI,aReqStatus,aUUI);
	}

EXPORT_C void RMobileCall::AnswerIncomingCallWithUUI(TRequestStatus& aReqStatus, const TDesC8& aCallParams, const TMobileCallUUI& aUUI) const
/**
 *  This method answers an incoming call and sends UUI to the calling party at the same time
 *
 * \param aUUI Supplies the UUI message data
 */
	{
	Set(EMobileCallAnswerWithUUI,aReqStatus,aCallParams,aUUI);
	}

