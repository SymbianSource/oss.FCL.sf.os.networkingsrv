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
// \file ETELMMCS.H
// Multimode ETel API v1.0
// Header file for Multimode API IPC request definitions
// 
//

#ifndef __ETELMMCS_H__
#define __ETELMMCS_H__

#include "Et_clsvr.h"

/**
Multimode ETel - Client-Server IPC request definitions
@internalComponent
*/

#define IPC_MULTIMODE_EXT (3000)


// 3000 to 3499 reserved for RMobilePhone requests
// 3500 to 4000 reserved for RMobilePhone cancel requests
#define EMobilePhoneIpcOffset				(IPC_MULTIMODE_EXT) 

// 4000 to 4099 reserved for RMobileLine requests
// 4500 to 4599 reserved for RMobileLine cancel requests
#define EMobileLineIpcOffset				(4000) 

// 4100 to 4199 reserved for RMobileCall requests
// 4600 to 4699 reserved for RMobileCall cancel requests
#define EMobileCallIpcOffset				(4100) 

// 4200 to 4299 reserved for RMobileSmsMessaging requests
// 4700 to 4799 reserved for RMobileSmsMessaging cancel requests
#define EMobileSmsMessagingIpcOffset		(4200) 

// 4300 to 4399 reserved for RMobileBroadcastMessaging requests
// 4800 to 4899 reserved for RMobileBroadcastMessaging cancel requests
#define EMobileBroadcastMessagingIpcOffset	(4300) 

// 4400 to 4449 reserved for RMobileUssdMessaging requests
// 4900 to 4949 reserved for RMobileUssdMessaging cancel requests
#define EMobileUssdMessagingIpcOffset		(4400) 

// 4450 to 4499 reserved for RMobilePhoneStore requests
// 4950 to 4999 reserved for RMobilePhoneStore cancel requests
#define EMobilePhoneStoreIpcOffset			(4450) 

// 5000 to 5049 reserved for RMobileSmsStore requests
// 5500 to 5549 reserved for RMobileSmsStore cancel requests
#define EMobileSmsStoreIpcOffset			(5000) 

// 5050 to 5099 reserved for RMobileNamStore requests
// 5550 to 5599 reserved for RMobileNamStore cancel requests
#define EMobileNamStoreIpcOffset			(5050) 

// 5100 to 5149 reserved for RMobileONStore requests
// 5600 to 5649 reserved for RMobileONStore cancel requests
#define EMobileONStoreIpcOffset				(5100) 

// 5150 to 5199 reserved for RMobilePhoneBookStore requests
// 5650 to 5699 reserved for RMobilePhoneBookStore cancel requests
#define EMobilePhoneBookStoreIpcOffset		(5150) 

// 5200 to 5249 reserved for RMobileConferenceCall requests
// 5700 to 5749 reserved for RMobileConferenceCall cancel requests
#define EMobileConferenceCallIpcOffset		(5200) 

/**
RMobilePhone requests
@internalComponent
*/
enum
	{
	EMobilePhoneGetMultimodeCaps=EMobilePhoneIpcOffset,
	EMobilePhoneGetPhoneStoreInfo,
	EMobilePhoneGetIccAccessCaps,
	EMobilePhoneNotifyIccAccessCapsChange,
	EMobilePhoneGetCustomerServiceProfile,
	EMobilePhoneGetServiceTable,
	EMobilePhoneGetBatteryCaps,
	EMobilePhoneGetBatteryInfo,
	EMobilePhoneNotifyBatteryInfoChange,
	EMobilePhoneGetSignalCaps,
	EMobilePhoneGetSignalStrength,
	EMobilePhoneNotifySignalStrengthChange,
	EMobilePhoneGetIndicatorCaps,
	EMobilePhoneGetIndicator,
	EMobilePhoneNotifyIndicatorChange,
	EMobilePhoneGetIdentityCaps,
	EMobilePhoneGetPhoneId,
	EMobilePhoneGetSubscriberId,
	EMobilePhoneGetDTMFCaps,
	EMobilePhoneNotifyDTMFCapsChange,
	EMobilePhoneSendDTMFTones,
	EMobilePhoneStartDTMFTone,
	EMobilePhoneStopDTMFTone,
	EMobilePhoneNotifyStopInDTMFString,
	EMobilePhoneContinueDTMFStringSending,
	EMobilePhoneGetNetworkCaps,
	EMobilePhoneGetNetworkRegistrationStatus,
	EMobilePhoneNotifyNetworkRegistrationStatusChange,
	EMobilePhoneGetCurrentMode,
	EMobilePhoneNotifyModeChange,
	EMobilePhoneGetCurrentNetwork,
	EMobilePhoneNotifyCurrentNetworkChange,
	EMobilePhoneGetHomeNetwork,
	EMobilePhoneGetDetectedNetworksPhase1,
	EMobilePhoneGetDetectedNetworksPhase2,
	EMobilePhoneGetNetworkSelectionSetting,
	EMobilePhoneSetNetworkSelectionSetting,
	EMobilePhoneNotifyNetworkSelectionSettingChange,
	EMobilePhoneSelectNetwork,
	EMobilePhoneGetNITZInfo,
	EMobilePhoneNotifyNITZInfoChange,
	EMobilePhoneGetDefaultPrivacy,
	EMobilePhoneSetDefaultPrivacy,
	EMobilePhoneNotifyDefaultPrivacyChange,
	EMobilePhoneGetCallServiceCaps,
	EMobilePhoneNotifyCallServiceCapsChange,
	EMobilePhoneProgramFeatureCode,
	EMobilePhoneGetFeatureCode,
	EMobilePhoneSendNetworkServiceRequest,
	EMobilePhoneGetCallForwardingStatusPhase1,
	EMobilePhoneGetCallForwardingStatusPhase2,
	EMobilePhoneSetCallForwardingStatus,
	EMobilePhoneNotifyCallForwardingStatusChange,
	EMobilePhoneNotifyCallForwardingActive,
	EMobilePhoneGetIdentityServiceStatus,
	EMobilePhoneGetBarringStatusPhase1,
	EMobilePhoneGetBarringStatusPhase2,
	EMobilePhoneSetCallBarringStatus,
	EMobilePhoneNotifyCallBarringStatusChange,
	EMobilePhoneSetCallBarringPassword,
	EMobilePhoneGetWaitingStatusPhase1,
	EMobilePhoneGetWaitingStatusPhase2,
	EMobilePhoneSetCallWaitingStatus,
	EMobilePhoneNotifyCallWaitingStatusChange,
	EMobilePhoneGetCCBSStatus,
	EMobilePhoneNotifyCCBSStatusChange,
	EMobilePhoneDeactivateAllCCBS,
	EMobilePhoneGetCompletionRequestsPhase1,
	EMobilePhoneGetCompletionRequestsPhase2,
	EMobilePhoneGetAlternatingCallCaps,
	EMobilePhoneNotifyAlternatingCallCapsChange,
	EMobilePhoneGetAlternatingCallMode,
	EMobilePhoneSetAlternatingCallMode,
	EMobilePhoneNotifyAlternatingCallModeChange,
	EMobilePhoneGetALSLine,
	EMobilePhoneSetALSLine,
	EMobilePhoneNotifyALSLineChange,
	EMobilePhoneGetCostCaps,
	EMobilePhoneNotifyCostCapsChange,
	EMobilePhoneClearCostMeter,
	EMobilePhoneSetMaxCostMeter,
	EMobilePhoneSetPuct,
	EMobilePhoneGetCostInfo,
	EMobilePhoneNotifyCostInfoChange,
	EMobilePhoneGetSecurityCaps,
	EMobilePhoneNotifySecurityCapsChange,
	EMobilePhoneGetLockInfo,
	EMobilePhoneNotifyLockInfoChange,
	EMobilePhoneSetLockSetting,
	EMobilePhoneChangeSecurityCode,
	EMobilePhoneNotifySecurityEvent,
	EMobilePhoneVerifySecurityCode,
	EMobilePhoneAbortSecurityCode,
	EMobilePhoneNotifyMessageWaiting,
	EMobilePhoneGetFdnStatus,
	EMobilePhoneSetFdnSetting,
	EMobilePhoneNotifyFdnStatusChange,
	EMobilePhoneGetMulticallParams,
	EMobilePhoneSetMulticallParams,
	EMobilePhoneNotifyMulticallParamsChange,
	EMobilePhoneGetIncomingCallType,
	EMobilePhoneSetIncomingCallType,
	EMobilePhoneNotifyIncomingCallTypeChange,
	EMobilePhoneGetUUSSetting,
	EMobilePhoneSetUUSSetting,
	EMobilePhoneNotifyUUSSettingChange,
	EMobilePhoneGetIccMessageWaitingIndicators,
	EMobilePhoneSetIccMessageWaitingIndicators,
	EMobilePhoneNotifyIccMessageWaitingIndicatorsChange
	};	

/**
@internalComponent
*/
enum
	{
	EMobilePhoneGetPhoneStoreInfoCancel = EMobilePhoneGetPhoneStoreInfo + EMobileCancelOffset,
	EMobilePhoneNotifyIccAccessCapsChangeCancel = EMobilePhoneNotifyIccAccessCapsChange + EMobileCancelOffset,
	EMobilePhoneGetCustomerServiceProfileCancel = EMobilePhoneGetCustomerServiceProfile + EMobileCancelOffset,
	EMobilePhoneGetServiceTableCancel = EMobilePhoneGetServiceTable + EMobileCancelOffset,
	EMobilePhoneGetBatteryInfoCancel = EMobilePhoneGetBatteryInfo + EMobileCancelOffset,
	EMobilePhoneNotifyBatteryInfoChangeCancel = EMobilePhoneNotifyBatteryInfoChange + EMobileCancelOffset,
	EMobilePhoneGetSignalStrengthCancel = EMobilePhoneGetSignalStrength + EMobileCancelOffset,
	EMobilePhoneNotifySignalStrengthChangeCancel = EMobilePhoneNotifySignalStrengthChange + EMobileCancelOffset,
	EMobilePhoneGetIndicatorCancel = EMobilePhoneGetIndicator + EMobileCancelOffset,
	EMobilePhoneNotifyIndicatorChangeCancel = EMobilePhoneNotifyIndicatorChange + EMobileCancelOffset,
	EMobilePhoneGetPhoneIdCancel = EMobilePhoneGetPhoneId + EMobileCancelOffset,
	EMobilePhoneGetSubscriberIdCancel = EMobilePhoneGetSubscriberId + EMobileCancelOffset,
	EMobilePhoneNotifyDTMFCapsChangeCancel = EMobilePhoneNotifyDTMFCapsChange + EMobileCancelOffset,
	EMobilePhoneSendDTMFTonesCancel = EMobilePhoneSendDTMFTones + EMobileCancelOffset,
	EMobilePhoneNotifyStopInDTMFStringCancel = EMobilePhoneNotifyStopInDTMFString + EMobileCancelOffset,
	EMobilePhoneGetNetworkRegistrationStatusCancel = EMobilePhoneGetNetworkRegistrationStatus + EMobileCancelOffset,
	EMobilePhoneNotifyNetworkRegistrationStatusChangeCancel = EMobilePhoneNotifyNetworkRegistrationStatusChange + EMobileCancelOffset,
	EMobilePhoneNotifyModeChangeCancel = EMobilePhoneNotifyModeChange + EMobileCancelOffset,
	EMobilePhoneGetCurrentNetworkCancel = EMobilePhoneGetCurrentNetwork + EMobileCancelOffset,
	EMobilePhoneNotifyCurrentNetworkChangeCancel = EMobilePhoneNotifyCurrentNetworkChange + EMobileCancelOffset,
	EMobilePhoneGetHomeNetworkCancel = EMobilePhoneGetHomeNetwork + EMobileCancelOffset,
	EMobilePhoneGetDetectedNetworksCancel = EMobilePhoneGetDetectedNetworksPhase1 + EMobileCancelOffset,
	EMobilePhoneSetNetworkSelectionSettingCancel = EMobilePhoneSetNetworkSelectionSetting + EMobileCancelOffset,
	EMobilePhoneNotifyNetworkSelectionSettingChangeCancel = EMobilePhoneNotifyNetworkSelectionSettingChange + EMobileCancelOffset,
	EMobilePhoneSelectNetworkCancel = EMobilePhoneSelectNetwork + EMobileCancelOffset,
	EMobilePhoneNotifyNITZInfoChangeCancel = EMobilePhoneNotifyNITZInfoChange + EMobileCancelOffset,
	EMobilePhoneSetDefaultPrivacyCancel = EMobilePhoneSetDefaultPrivacy + EMobileCancelOffset,
	EMobilePhoneNotifyDefaultPrivacyChangeCancel = EMobilePhoneNotifyDefaultPrivacyChange + EMobileCancelOffset,
	EMobilePhoneNotifyCallServiceCapsChangeCancel = EMobilePhoneNotifyCallServiceCapsChange + EMobileCancelOffset,
	EMobilePhoneProgramFeatureCodeCancel = EMobilePhoneProgramFeatureCode + EMobileCancelOffset,
	EMobilePhoneGetFeatureCodeCancel = EMobilePhoneGetFeatureCode + EMobileCancelOffset,
	EMobilePhoneSendNetworkServiceRequestCancel = EMobilePhoneSendNetworkServiceRequest + EMobileCancelOffset,
	EMobilePhoneGetCallForwardingStatusCancel = EMobilePhoneGetCallForwardingStatusPhase1 + EMobileCancelOffset,
	EMobilePhoneSetCallForwardingStatusCancel = EMobilePhoneSetCallForwardingStatus + EMobileCancelOffset,
	EMobilePhoneNotifyCallForwardingStatusChangeCancel = EMobilePhoneNotifyCallForwardingStatusChange + EMobileCancelOffset,
	EMobilePhoneNotifyCallForwardingActiveCancel = EMobilePhoneNotifyCallForwardingActive + EMobileCancelOffset,
	EMobilePhoneGetIdentityServiceStatusCancel = EMobilePhoneGetIdentityServiceStatus + EMobileCancelOffset,
	EMobilePhoneGetBarringStatusCancel = EMobilePhoneGetBarringStatusPhase1 + EMobileCancelOffset,
	EMobilePhoneSetCallBarringStatusCancel = EMobilePhoneSetCallBarringStatus + EMobileCancelOffset,
	EMobilePhoneNotifyCallBarringStatusChangeCancel = EMobilePhoneNotifyCallBarringStatusChange + EMobileCancelOffset,
	EMobilePhoneSetCallBarringPasswordCancel = EMobilePhoneSetCallBarringPassword + EMobileCancelOffset,
	EMobilePhoneGetWaitingStatusCancel = EMobilePhoneGetWaitingStatusPhase1 + EMobileCancelOffset,
	EMobilePhoneSetCallWaitingStatusCancel = EMobilePhoneSetCallWaitingStatus + EMobileCancelOffset,
	EMobilePhoneNotifyCallWaitingStatusChangeCancel = EMobilePhoneNotifyCallWaitingStatusChange + EMobileCancelOffset,
	EMobilePhoneNotifyCCBSStatusChangeCancel = EMobilePhoneNotifyCCBSStatusChange + EMobileCancelOffset,
	EMobilePhoneDeactivateAllCCBSCancel = EMobilePhoneDeactivateAllCCBS + EMobileCancelOffset,
	EMobilePhoneGetCompletionRequestsCancel = EMobilePhoneGetCompletionRequestsPhase1 + EMobileCancelOffset,
	EMobilePhoneGetCCBSStatusCancel = EMobilePhoneGetCCBSStatus + EMobileCancelOffset,
	EMobilePhoneNotifyAlternatingCallCapsChangeCancel = EMobilePhoneNotifyAlternatingCallCapsChange + EMobileCancelOffset,
	EMobilePhoneSetAlternatingCallModeCancel = EMobilePhoneSetAlternatingCallMode + EMobileCancelOffset,
	EMobilePhoneNotifyAlternatingCallModeChangeCancel = EMobilePhoneNotifyAlternatingCallModeChange + EMobileCancelOffset,
	EMobilePhoneSetALSLineCancel = EMobilePhoneSetALSLine + EMobileCancelOffset,
	EMobilePhoneNotifyALSLineChangeCancel = EMobilePhoneNotifyALSLineChange + EMobileCancelOffset,
	EMobilePhoneNotifyCostCapsChangeCancel = EMobilePhoneNotifyCostCapsChange + EMobileCancelOffset,
	EMobilePhoneClearCostMeterCancel = 	EMobilePhoneClearCostMeter + EMobileCancelOffset,
	EMobilePhoneSetMaxCostMeterCancel = EMobilePhoneSetMaxCostMeter + EMobileCancelOffset,
	EMobilePhoneSetPuctCancel = EMobilePhoneSetPuct + EMobileCancelOffset,
	EMobilePhoneGetCostInfoCancel = EMobilePhoneGetCostInfo + EMobileCancelOffset,
	EMobilePhoneNotifyCostInfoChangeCancel = EMobilePhoneNotifyCostInfoChange + EMobileCancelOffset,
	EMobilePhoneNotifySecurityCapsChangeCancel = EMobilePhoneNotifySecurityCapsChange + EMobileCancelOffset,
	EMobilePhoneNotifyLockInfoChangeCancel = EMobilePhoneNotifyLockInfoChange + EMobileCancelOffset,
	EMobilePhoneSetLockSettingCancel = EMobilePhoneSetLockSetting + EMobileCancelOffset,
	EMobilePhoneChangeSecurityCodeCancel = EMobilePhoneChangeSecurityCode + EMobileCancelOffset,
	EMobilePhoneNotifySecurityEventCancel = EMobilePhoneNotifySecurityEvent + EMobileCancelOffset,
	EMobilePhoneVerifySecurityCodeCancel = EMobilePhoneVerifySecurityCode + EMobileCancelOffset,
	EMobilePhoneNotifyMessageWaitingCancel = EMobilePhoneNotifyMessageWaiting + EMobileCancelOffset,
	EMobilePhoneSetFdnSettingCancel = EMobilePhoneSetFdnSetting + EMobileCancelOffset,
	EMobilePhoneNotifyFdnStatusChangeCancel = EMobilePhoneNotifyFdnStatusChange + EMobileCancelOffset,
	EMobilePhoneGetMulticallParamsCancel = EMobilePhoneGetMulticallParams + EMobileCancelOffset,
	EMobilePhoneSetMulticallParamsCancel = EMobilePhoneSetMulticallParams + EMobileCancelOffset,
	EMobilePhoneNotifyMulticallParamsChangeCancel = EMobilePhoneNotifyMulticallParamsChange + EMobileCancelOffset,
	EMobilePhoneGetIncomingCallTypeCancel = EMobilePhoneGetIncomingCallType + EMobileCancelOffset,
	EMobilePhoneSetIncomingCallTypeCancel = EMobilePhoneSetIncomingCallType + EMobileCancelOffset,
	EMobilePhoneNotifyIncomingCallTypeChangeCancel = EMobilePhoneNotifyIncomingCallTypeChange + EMobileCancelOffset,
	EMobilePhoneGetUUSSettingCancel = EMobilePhoneGetUUSSetting + EMobileCancelOffset,
	EMobilePhoneSetUUSSettingCancel = EMobilePhoneSetUUSSetting  + EMobileCancelOffset,
	EMobilePhoneNotifyUUSSettingChangeCancel = EMobilePhoneNotifyUUSSettingChange + EMobileCancelOffset,
	EMobilePhoneGetIccMessageWaitingIndicatorsCancel = EMobilePhoneGetIccMessageWaitingIndicators + EMobileCancelOffset,
	EMobilePhoneSetIccMessageWaitingIndicatorsCancel = EMobilePhoneSetIccMessageWaitingIndicators + EMobileCancelOffset,
	EMobilePhoneNotifyIccMessageWaitingIndicatorsChangeCancel = EMobilePhoneNotifyIccMessageWaitingIndicatorsChange + EMobileCancelOffset
	};

/**
RMobileLine requests
@internalComponent
*/
enum
	{
	EMobileLineGetMobileLineStatus=EMobileLineIpcOffset,
	EMobileLineNotifyMobileLineStatusChange
	};
/**
@internalComponent
*/
enum
	{
	EMobileLineNotifyMobileLineStatusChangeCancel = EMobileLineNotifyMobileLineStatusChange + EMobileCancelOffset
	};

/**
RMobileCall requests
@internalComponent
*/
enum
	{
	EMobileCallGetMobileDataCallCaps=EMobileCallIpcOffset,
	EMobileCallNotifyMobileDataCallCapsChange,
	EMobileCallGetMobileDataCallRLPRange,
	EMobileCallSetDynamicHscsdParams,
	EMobileCallSwitchAlternatingCall,
	EMobileCallNotifyAlternatingCallSwitch,
	EMobileCallGetMobileCallCaps,
	EMobileCallNotifyMobileCallCapsChange,
	EMobileCallHold,
	EMobileCallResume,
	EMobileCallSwap,
	EMobileCallDeflect,
	EMobileCallTransfer,
	EMobileCallGoOneToOne,
	EMobileCallNotifyCallEvent,
	EMobileCallGetMobileCallStatus,
	EMobileCallNotifyMobileCallStatusChange,
	EMobileCallDialEmergencyCall,
	EMobileCallSetPrivacy,
	EMobileCallNotifyPrivacyConfirmation,
	EMobileCallSetTrafficChannel,
	EMobileCallNotifyTrafficChannelConfirmation,
	EMobileCallGetMobileCallInfo,
	EMobileCallNotifyRemotePartyInfoChange,
	EMobileCallGetCurrentHscsdInfo,
	EMobileCallNotifyHscsdInfoChange,
	EMobileCallNotifyVoiceFallback,
	EMobileCallActivateCCBS,
	EMobileCallRejectCCBS,
	EMobileCallAcceptCCBSRecall,
	EMobileCallDeactivateCCBS,
	EMobileCallGetUUSCaps,
	EMobileCallNotifyUUSCapsChange,
	EMobileCallActivateUUS,
	EMobileCallSendUUI,
	EMobileCallReceiveUUI,
	EMobileCallHangupWithUUI,
	EMobileCallAnswerWithUUI,
	};
/**
@internalComponent
*/
enum
	{
	EMobileCallNotifyMobileDataCallCapsChangeCancel = EMobileCallNotifyMobileDataCallCapsChange + EMobileCancelOffset,
	EMobileCallGetMobileDataCallRLPRangeCancel = EMobileCallGetMobileDataCallRLPRange + EMobileCancelOffset,
	EMobileCallSetDynamicHscsdParamsCancel = EMobileCallSetDynamicHscsdParams + EMobileCancelOffset,
	EMobileCallSwitchAlternatingCallCancel = EMobileCallSwitchAlternatingCall + EMobileCancelOffset,
	EMobileCallNotifyAlternatingCallSwitchCancel = EMobileCallNotifyAlternatingCallSwitch + EMobileCancelOffset,
	EMobileCallNotifyMobileCallCapsChangeCancel = EMobileCallNotifyMobileCallCapsChange + EMobileCancelOffset,
	EMobileCallHoldCancel = EMobileCallHold + EMobileCancelOffset,
	EMobileCallResumeCancel = EMobileCallResume + EMobileCancelOffset,
	EMobileCallSwapCancel = EMobileCallSwap + EMobileCancelOffset,
	EMobileCallDeflectCancel = EMobileCallDeflect + EMobileCancelOffset,
	EMobileCallTransferCancel = EMobileCallTransfer + EMobileCancelOffset,
	EMobileCallGoOneToOneCancel = EMobileCallGoOneToOne + EMobileCancelOffset,
	EMobileCallNotifyCallEventCancel = EMobileCallNotifyCallEvent + EMobileCancelOffset,
	EMobileCallNotifyMobileCallStatusChangeCancel = EMobileCallNotifyMobileCallStatusChange + EMobileCancelOffset,
	EMobileCallDialEmergencyCallCancel = 	EMobileCallDialEmergencyCall + EMobileCancelOffset,
	EMobileCallNotifyPrivacyConfirmationCancel = 	EMobileCallNotifyPrivacyConfirmation + EMobileCancelOffset,
	EMobileCallNotifyTrafficChannelConfirmationCancel = EMobileCallNotifyTrafficChannelConfirmation + EMobileCancelOffset,
	EMobileCallGetMobileCallInfoCancel = EMobileCallGetMobileCallInfo + EMobileCancelOffset,
	EMobileCallNotifyRemotePartyInfoChangeCancel = EMobileCallNotifyRemotePartyInfoChange + EMobileCancelOffset,
	EMobileCallNotifyHscsdInfoChangeCancel = EMobileCallNotifyHscsdInfoChange + EMobileCancelOffset,
	EMobileCallNotifyVoiceFallbackCancel = EMobileCallNotifyVoiceFallback + EMobileCancelOffset,
	EMobileCallActivateCCBSCancel = EMobileCallActivateCCBS + EMobileCancelOffset,
	EMobileCallAcceptCCBSRecallCancel = EMobileCallAcceptCCBSRecall + EMobileCancelOffset,
	EMobileCallDeactivateCCBSCancel = EMobileCallDeactivateCCBS + EMobileCancelOffset,
	EMobileCallNotifyUUSCapsChangeCancel = EMobileCallNotifyUUSCapsChange + EMobileCancelOffset,
	EMobileCallActivateUUSCancel = EMobileCallActivateUUS + EMobileCancelOffset,
	EMobileCallSendUUICancel = EMobileCallSendUUI + EMobileCancelOffset,
	EMobileCallReceiveUUICancel = EMobileCallReceiveUUI + EMobileCancelOffset,
	EMobileCallHangupWithUUICancel = EMobileCallHangupWithUUI + EMobileCancelOffset,
	EMobileCallAnswerWithUUICancel = EMobileCallAnswerWithUUI + EMobileCancelOffset
	};

/**
RMobileSmsMessaging requests
@internalComponent
*/
enum
	{
	EMobileSmsMessagingGetCaps=EMobileSmsMessagingIpcOffset,
	EMobileSmsMessagingGetReceiveMode,
	EMobileSmsMessagingSetReceiveMode,
	EMobileSmsMessagingNotifyReceiveModeChange,
	EMobileSmsMessagingAckSmsStored,
	EMobileSmsMessagingNackSmsStored,
	EMobileSmsMessagingResumeSmsReception,
	EMobileSmsMessagingSendMessage,
	EMobileSmsMessagingSetMoSmsBearer,
	EMobileSmsMessagingGetMoSmsBearer,
	EMobileSmsMessagingNotifyMoSmsBearerChange,
	EMobileSmsMessagingReceiveMessage,
	EMobileSmsMessagingEnumerateMessageStores,
	EMobileSmsMessagingGetMessageStoreInfo,
	EMobileSmsMessagingGetSmspListPhase1,
	EMobileSmsMessagingGetSmspListPhase2,
	EMobileSmsMessagingStoreSmspList,
	EMobileSmsMessagingNotifySmspListChange,
	};
/**
@internalComponent
*/
enum
	{
	EMobileSmsMessagingSetReceiveModeCancel = EMobileSmsMessagingSetReceiveMode + EMobileCancelOffset,
	EMobileSmsMessagingNotifyReceiveModeChangeCancel = EMobileSmsMessagingNotifyReceiveModeChange + EMobileCancelOffset,
	EMobileSmsMessagingAckSmsStoredCancel = EMobileSmsMessagingAckSmsStored + EMobileCancelOffset,
	EMobileSmsMessagingNackSmsStoredCancel = EMobileSmsMessagingNackSmsStored + EMobileCancelOffset,
	EMobileSmsMessagingResumeSmsReceptionCancel = EMobileSmsMessagingResumeSmsReception + EMobileCancelOffset,
	EMobileSmsMessagingSendMessageCancel = EMobileSmsMessagingSendMessage + EMobileCancelOffset,
	EMobileSmsMessagingSetMoSmsBearerCancel = EMobileSmsMessagingSetMoSmsBearer + EMobileCancelOffset,
	EMobileSmsMessagingNotifyMoSmsBearerChangeCancel = EMobileSmsMessagingNotifyMoSmsBearerChange + EMobileCancelOffset,
	EMobileSmsMessagingReceiveMessageCancel = EMobileSmsMessagingReceiveMessage + EMobileCancelOffset,
	EMobileSmsMessagingGetMessageStoreInfoCancel = EMobileSmsMessagingGetMessageStoreInfo + EMobileCancelOffset,
	EMobileSmsMessagingGetSmspListCancel = EMobileSmsMessagingGetSmspListPhase1 + EMobileCancelOffset,
	EMobileSmsMessagingStoreSmspListCancel = EMobileSmsMessagingStoreSmspList + EMobileCancelOffset,
	EMobileSmsMessagingNotifySmspListChangeCancel = EMobileSmsMessagingNotifySmspListChange + EMobileCancelOffset
	};

/**
RMobileBroadcastMessaging requests
@internalComponent
*/
enum
	{
	EMobileBroadcastMessagingGetCaps=EMobileBroadcastMessagingIpcOffset,
	EMobileBroadcastMessagingReceiveMessage,
	EMobileBroadcastMessagingGetFilterSetting,
	EMobileBroadcastMessagingSetFilterSetting,
	EMobileBroadcastMessagingNotifyFilterSettingChange,
	EMobileBroadcastMessagingGetLanguageFilter,
	EMobileBroadcastMessagingSetLanguageFilter,
	EMobileBroadcastMessagingNotifyLanguageFilterChange,
	EMobileBroadcastMessagingGetIdListPhase1,
	EMobileBroadcastMessagingGetIdListPhase2,
	EMobileBroadcastMessagingStoreIdList,
	EMobileBroadcastMessagingNotifyIdListChange,
	};
/**
@internalComponent
*/
enum
	{
	EMobileBroadcastMessagingReceiveMessageCancel = EMobileBroadcastMessagingReceiveMessage + EMobileCancelOffset,
	EMobileBroadcastMessagingSetFilterSettingCancel = EMobileBroadcastMessagingSetFilterSetting + EMobileCancelOffset,
	EMobileBroadcastMessagingNotifyFilterSettingChangeCancel = EMobileBroadcastMessagingNotifyFilterSettingChange + EMobileCancelOffset,
	EMobileBroadcastMessagingGetLanguageFilterCancel = EMobileBroadcastMessagingGetLanguageFilter + EMobileCancelOffset,
	EMobileBroadcastMessagingSetLanguageFilterCancel = EMobileBroadcastMessagingSetLanguageFilter + EMobileCancelOffset,
	EMobileBroadcastMessagingNotifyLanguageFilterChangeCancel = EMobileBroadcastMessagingNotifyLanguageFilterChange + EMobileCancelOffset,
	EMobileBroadcastMessagingGetIdListCancel = EMobileBroadcastMessagingGetIdListPhase1 + EMobileCancelOffset,
	EMobileBroadcastMessagingStoreIdListCancel = EMobileBroadcastMessagingStoreIdList + EMobileCancelOffset,
	EMobileBroadcastMessagingNotifyIdListChangeCancel = EMobileBroadcastMessagingNotifyIdListChange + EMobileCancelOffset
	};

/**
RMobileUssdMessaging requests
@internalComponent
*/

enum
	{
	EMobileUssdMessagingGetCaps=EMobileUssdMessagingIpcOffset,
	EMobileUssdMessagingReceiveMessage,
	EMobileUssdMessagingSendMessage,
	};
/**
@internalComponent
*/
enum
	{
	EMobileUssdMessagingReceiveMessageCancel = EMobileUssdMessagingReceiveMessage + EMobileCancelOffset,
	EMobileUssdMessagingSendMessageCancel = EMobileUssdMessagingSendMessage + EMobileCancelOffset
	};
/**
RMobilePhoneStore requests
@internalComponent
*/
enum
	{
	EMobilePhoneStoreGetInfo=EMobilePhoneStoreIpcOffset,
	EMobilePhoneStoreRead,
	EMobilePhoneStoreWrite,
	EMobilePhoneStoreDelete,
	EMobilePhoneStoreDeleteAll,
	EMobilePhoneStoreNotifyStoreEvent,
	EMobilePhoneStoreReadAllPhase1,
	EMobilePhoneStoreReadAllPhase2,
	};
/**
@internalComponent
*/
enum
	{
	EMobilePhoneStoreGetInfoCancel = EMobilePhoneStoreGetInfo + EMobileCancelOffset,
	EMobilePhoneStoreReadCancel = EMobilePhoneStoreRead + EMobileCancelOffset,
	EMobilePhoneStoreWriteCancel = EMobilePhoneStoreWrite + EMobileCancelOffset,
	EMobilePhoneStoreDeleteCancel = EMobilePhoneStoreDelete + EMobileCancelOffset,
	EMobilePhoneStoreDeleteAllCancel = EMobilePhoneStoreDeleteAll + EMobileCancelOffset,
	EMobilePhoneStoreNotifyStoreEventCancel = EMobilePhoneStoreNotifyStoreEvent + EMobileCancelOffset,
	EMobilePhoneStoreReadAllCancel = EMobilePhoneStoreReadAllPhase1 + EMobileCancelOffset
	};

/**
RMobileSmsStore requests
RMobileNamStore requests
@internalComponent
*/
enum
	{
	EMobileNamStoreSetActiveNam = EMobileNamStoreIpcOffset,
	EMobileNamStoreStoreAll,
	};
/**
@internalComponent
*/
enum
	{
	EMobileNamStoreSetActiveNamCancel =	EMobileNamStoreSetActiveNam + EMobileCancelOffset,
	EMobileNamStoreStoreAllCancel = EMobileNamStoreStoreAll + EMobileCancelOffset
	};
/**
RMobileONStore requests
@internalComponent
*/

enum 
	{
	EMobileONStoreStoreAll = EMobileONStoreIpcOffset
	};
/**
@internalComponent
*/

enum
	{
	EMobileONStoreStoreAllCancel = EMobileONStoreStoreAll + EMobileCancelOffset
	};
/**
RMobilePhoneBookStore requests
@internalComponent
*/

enum 
	{
	EMobilePhoneBookStoreRead = EMobilePhoneBookStoreIpcOffset,
	EMobilePhoneBookStoreWrite
	};
/**
RMobileConferenceCall requests
@internalComponent
*/

enum
	{
	EMobileConferenceCallGetCaps = EMobileConferenceCallIpcOffset,
	EMobileConferenceCallNotifyCapsChange,
	EMobileConferenceCallCreateConference,
	EMobileConferenceCallAddCall,
	EMobileConferenceCallSwap,
	EMobileConferenceCallHangUp,
	EMobileConferenceCallEnumerateCalls,
	EMobileConferenceCallGetMobileCallInfo,
	EMobileConferenceCallGetConferenceStatus,
	EMobileConferenceCallNotifyConferenceStatusChange,
	EMobileConferenceCallNotifyConferenceEvent,
	};

/**
@internalComponent
*/

enum
	{
	EMobileConferenceCallNotifyCapsChangeCancel = EMobileConferenceCallNotifyCapsChange + EMobileCancelOffset,
	EMobileConferenceCallCreateConferenceCancel = EMobileConferenceCallCreateConference + EMobileCancelOffset,
	EMobileConferenceCallAddCallCancel = EMobileConferenceCallAddCall + EMobileCancelOffset,
	EMobileConferenceCallSwapCancel = EMobileConferenceCallSwap + EMobileCancelOffset,
	EMobileConferenceCallHangUpCancel = EMobileConferenceCallHangUp + EMobileCancelOffset,
	EMobileConferenceCallNotifyConferenceStatusChangeCancel = EMobileConferenceCallNotifyConferenceStatusChange + EMobileCancelOffset,
	EMobileConferenceCallNotifyConferenceEventCancel = EMobileConferenceCallNotifyConferenceEvent + EMobileCancelOffset
	};

#endif
