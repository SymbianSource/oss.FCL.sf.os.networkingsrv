// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//


#ifndef __EXTERROR_H__
#define __EXTERROR_H__

/**
Extended Error definitions
@internalComponent
*/
const TInt KErrEtelGsmBase = -4000;

/**
Radio Resource Network Errors - defined by GSM 04.08
@internalComponent
*/
const TInt KErrGsmRadioResourceBase=KErrEtelGsmBase;

const TInt KErrGsmRRUnspecifedAbnormalRelease=KErrGsmRadioResourceBase-1;
const TInt KErrGsmRRChannelUnacceptable=KErrGsmRadioResourceBase-2;
const TInt KErrGsmRRTimerExpired=KErrGsmRadioResourceBase-3;
const TInt KErrGsmRRNoActivityOnRadioPath=KErrGsmRadioResourceBase-4;
const TInt KErrGsmRRPreEmptiveRelease=KErrGsmRadioResourceBase-5;
const TInt KErrGsmRRCallAlreadyCleared=KErrGsmRadioResourceBase-65;
const TInt KErrGsmRRInvalidMessage=KErrGsmRadioResourceBase-95;
const TInt KErrGsmRRNonExistentMessage=KErrGsmRadioResourceBase-97;
const TInt KErrGsmRRIncompatibleMessageWithCallState=KErrGsmRadioResourceBase-98;
const TInt KErrGsmRRInvalidInformationElement=KErrGsmRadioResourceBase-100;
const TInt KErrGsmRRUnspecifiedProtocolError=KErrGsmRadioResourceBase-111;

/**
Mobility Management Network Errors - defined by GSM 04.08
@internalComponent
*/
const TInt KErrGsmMobilityManagementBase=KErrGsmRadioResourceBase-128;

const TInt KErrGsmMMUnallocatedTmsi=KErrGsmMobilityManagementBase-1;
const TInt KErrGsmMMImsiUnknownInHlr=KErrGsmMobilityManagementBase-2;
const TInt KErrGsmMMIllegalMs=KErrGsmMobilityManagementBase-3;
const TInt KErrGsmMMImsiUnknownInVlr=KErrGsmMobilityManagementBase-4;
const TInt KErrGsmMMImeiNotAccepted=KErrGsmMobilityManagementBase-5;
const TInt KErrGsmMMIllegalMe=KErrGsmMobilityManagementBase-6;
const TInt KErrGsmMMPlmnNotAllowed=KErrGsmMobilityManagementBase-11;
const TInt KErrGsmMMLocationAreaNotAllowed=KErrGsmMobilityManagementBase-12;
const TInt KErrGsmMMRoamingNotAllowedInThisLocationArea=KErrGsmMobilityManagementBase-13;
const TInt KErrGsmMMNetworkFailure=KErrGsmMobilityManagementBase-17;
const TInt KErrGsmMMCongestion=KErrGsmMobilityManagementBase-22;
const TInt KErrGsmMMServiceOptionNotSupported=KErrGsmMobilityManagementBase-32;
const TInt KErrGsmMMServiceOptionNotSubscribed=KErrGsmMobilityManagementBase-33;
const TInt KErrGsmMMServiceOptionTemporaryOutOfOrder=KErrGsmMobilityManagementBase-34;
const TInt KErrGsmMMCallCanNotBeIdentified=KErrGsmMobilityManagementBase-38;
const TInt KErrGsmMMSemanticErrorInMessage=KErrGsmMobilityManagementBase-95;
const TInt KErrGsmMMMandatoryInformationElementError=KErrGsmMobilityManagementBase-96;
const TInt KErrGsmMMNonExistentMessageType=KErrGsmMobilityManagementBase-97;
const TInt KErrGsmMMIncompatibleMessageWithProtocolState=KErrGsmMobilityManagementBase-98;
const TInt KErrGsmMMNonExistentInformationElement=KErrGsmMobilityManagementBase-99;
const TInt KErrGsmMMConditionalIEError=KErrGsmMobilityManagementBase-100;
const TInt KErrGsmMMIncompatibleMessageWithCallState=KErrGsmMobilityManagementBase-101;

/**
Call Control Network Errors - defined by GSM 04.08
@internalComponent
*/
const TInt KErrGsmCallControlBase=KErrGsmMobilityManagementBase-128;

const TInt KErrGsmCCUnassignedNumber=KErrGsmCallControlBase-1;
const TInt KErrGsmCCNoRouteToTransitNetwork=KErrGsmCallControlBase-2;
const TInt KErrGsmCCNoRouteToDestination=KErrGsmCallControlBase-3;
const TInt KErrGsmCCChannelUnacceptable=KErrGsmCallControlBase-6;
const TInt KErrGsmCCOperatorDeterminedBarring=KErrGsmCallControlBase-8;
const TInt KErrGsmCCUserBusy=KErrGsmCallControlBase-17;
const TInt KErrGsmCCUserNotResponding=KErrGsmCallControlBase-18;
const TInt KErrGsmCCUserAlertingNoAnswer=KErrGsmCallControlBase-19;
const TInt KErrGsmCCCallRejected=KErrGsmCallControlBase-21;
const TInt KErrGsmCCNumberChanged=KErrGsmCallControlBase-22;
const TInt KErrGsmCCNonSelectedUserClearing=KErrGsmCallControlBase-26;
const TInt KErrGsmCCDestinationOutOfOrder=KErrGsmCallControlBase-27;
const TInt KErrGsmCCInvalidNumberFormat=KErrGsmCallControlBase-28;
const TInt KErrGsmCCFacilityRejected=KErrGsmCallControlBase-29;
const TInt KErrGsmCCResponseToStatusEnquiry=KErrGsmCallControlBase-30;
const TInt KErrGsmCCNormalUnspecified=KErrGsmCallControlBase-31;
const TInt KErrGsmCCNoChannelAvailable=KErrGsmCallControlBase-34;
const TInt KErrGsmCCNetworkOutOfOrder=KErrGsmCallControlBase-38;
const TInt KErrGsmCCTemporaryFailure=KErrGsmCallControlBase-41;
const TInt KErrGsmCCSwitchingEquipmentCongestion=KErrGsmCallControlBase-42;
const TInt KErrGsmCCAccessInformationDiscarded=KErrGsmCallControlBase-43;
const TInt KErrGsmCCRequestedChannelNotAvailable=KErrGsmCallControlBase-44;
const TInt KErrGsmCCResourceNotAvailable=KErrGsmCallControlBase-47;
const TInt KErrGsmCCQualityOfServiceNotAvailable=KErrGsmCallControlBase-49;
const TInt KErrGsmCCRequestedFacilityNotSubscribed=KErrGsmCallControlBase-50;
const TInt KErrGsmCCIncomingCallsBarredInCug=KErrGsmCallControlBase-55;
const TInt KErrGsmCCBearerCapabilityNotAuthorised=KErrGsmCallControlBase-57;
const TInt KErrGsmCCBearerCapabilityNotCurrentlyAvailable=KErrGsmCallControlBase-58;
const TInt KErrGsmCCServiceNotAvailable=KErrGsmCallControlBase-63;
const TInt KErrGsmCCBearerServiceNotImplemented=KErrGsmCallControlBase-65;
const TInt KErrGsmCCChannelTypeNotImplemented=KErrGsmCallControlBase-66;
const TInt KErrGsmCCAcmGreaterThanAcmMax=KErrGsmCallControlBase-68;
const TInt KErrGsmCCRequestedFacilityNotImplemented=KErrGsmCallControlBase-69;
const TInt KErrGsmCCOnlyRestrictedDigitalInformationBCAvailable=KErrGsmCallControlBase-70;
const TInt KErrGsmCCServiceNotImplemented=KErrGsmCallControlBase-79;
const TInt KErrGsmCCInvalidCallReferenceValue=KErrGsmCallControlBase-81;
const TInt KErrGsmCCChannelDoesNotExist=KErrGsmCallControlBase-82;
const TInt KErrGsmCCSuspendedCallExistsButCallIdentityDoesNotWork=KErrGsmCallControlBase-83;
const TInt KErrGsmCCCallIdentityInUse=KErrGsmCallControlBase-84;
const TInt KErrGsmCCNoCallSuspended=KErrGsmCallControlBase-85;
const TInt KErrGsmCCRequestedCallIdentityAlreadyCleared=KErrGsmCallControlBase-86;
const TInt KErrGsmCCUserNotInCug=KErrGsmCallControlBase-87;
const TInt KErrGsmCCIncompatibleDestination=KErrGsmCallControlBase-88;
const TInt KErrGsmCCInvalidTransitNetworkSelection=KErrGsmCallControlBase-91;
const TInt KErrGsmCCIncompatibleSegmentedMessage=KErrGsmCallControlBase-94;
const TInt KErrGsmCCSemanticallyIncorrectMessage=KErrGsmCallControlBase-95;
const TInt KErrGsmCCInvalidMandatoryInformation=KErrGsmCallControlBase-96;
const TInt KErrGsmCCNonExistentMessageType=KErrGsmCallControlBase-97;
const TInt KErrGsmCCIncompatibleMessageInProtocolState=KErrGsmCallControlBase-98;
const TInt KErrGsmCCNonExistentInformationElement=KErrGsmCallControlBase-99;
const TInt KErrGsmCCConditionalIEError=KErrGsmCallControlBase-100;
const TInt KErrGsmCCIncompatibleMessageInCallState=KErrGsmCallControlBase-101;
const TInt KErrGsmCCRecoveryOnTimerExpiry=KErrGsmCallControlBase-102;
const TInt KErrGsmCCUnspecifiedProtocolError=KErrGsmCallControlBase-111;
const TInt KErrGsmCCUnspecifiedInterworkingError=KErrGsmCallControlBase-127;

/**
Supplementary Services Network Errors - defined by GSM 04.80
@internalComponent
*/
const TInt KErrGsmSuppServiceBase=KErrGsmCallControlBase-128;

const TInt KErrGsmSSUnknownSubscriber=KErrGsmSuppServiceBase-1;
const TInt KErrGsmSSIllegalSubscriber=KErrGsmSuppServiceBase-9;
const TInt KErrGsmSSBearerServiceNotProvisioned=KErrGsmSuppServiceBase-10;
const TInt KErrGsmSSTeleserviceNotProvisioned=KErrGsmSuppServiceBase-11;
const TInt KErrGsmSSIllegalEquipment=KErrGsmSuppServiceBase-12;
const TInt KErrGsmSSCallBarred=KErrGsmSuppServiceBase-13;
const TInt KErrGsmSSIllegalOperation=KErrGsmSuppServiceBase-16;
const TInt KErrGsmSSErrorStatus=KErrGsmSuppServiceBase-17;
const TInt KErrGsmSSNotAvailable=KErrGsmSuppServiceBase-18;
const TInt KErrGsmSSSubscriptionViolation=KErrGsmSuppServiceBase-19;
const TInt KErrGsmSSIncompatibility=KErrGsmSuppServiceBase-20;
const TInt KErrGsmSSFacilityNotSupported=KErrGsmSuppServiceBase-21;
const TInt KErrGsmSSAbsentSubscriber=KErrGsmSuppServiceBase-27;
const TInt KErrGsmSSSystemFailure=KErrGsmSuppServiceBase-34;
const TInt KErrGsmSSDataMissing=KErrGsmSuppServiceBase-35;
const TInt KErrGsmSSUnexpectedDataValue=KErrGsmSuppServiceBase-36;
const TInt KErrGsmSSPasswordRegistrationFailure=KErrGsmSuppServiceBase-37;
const TInt KErrGsmSSNegativePasswordCheck=KErrGsmSuppServiceBase-38;
const TInt KErrGsmSSPasswordAttemptsViolation=KErrGsmSuppServiceBase-43;
const TInt KErrGsmSSUnknownAlphabet=KErrGsmSuppServiceBase-71;
const TInt KErrGsmSSUssdBusy=KErrGsmSuppServiceBase-72;
const TInt KErrGsmSSMaxNumMptyParticipants=KErrGsmSuppServiceBase-126;
const TInt KErrGsmSSResourcesUnavailable=KErrGsmSuppServiceBase-127;

/**
SMS Errors - defined by GSM 04.11, 03.40 and 07.05
@internalComponent
*/
const TInt KErrGsmSmsBase=KErrGsmSuppServiceBase-128;

/**
04.11
@internalComponent
*/
const TInt KErrGsmSMSUnassignedNumber=KErrGsmSmsBase-1;
const TInt KErrGsmSMSOperatorDeterminedBarring=KErrGsmSmsBase-8;
const TInt KErrGsmSMSCallBarred=KErrGsmSmsBase-10;
const TInt KErrGsmSMSReserved=KErrGsmSmsBase-11;
const TInt KErrGsmSMSNetworkFailure=KErrGsmSmsBase-17;
const TInt KErrGsmSMSShortMessageTransferRejected=KErrGsmSmsBase-21;
const TInt KErrGsmSMSMemoryCapacityExceeded=KErrGsmSmsBase-22;
const TInt KErrGsmSMSDestinationOutOfOrder=KErrGsmSmsBase-27;
const TInt KErrGsmSMSUnidentifiedSubscriber=KErrGsmSmsBase-28;
const TInt KErrGsmSMSFacilityRejected=KErrGsmSmsBase-29;
const TInt KErrGsmSMSUnknownSubscriber=KErrGsmSmsBase-30;
const TInt KErrGsmSMSNetworkOutOfOrder=KErrGsmSmsBase-38;
const TInt KErrGsmSMSTemporaryFailure=KErrGsmSmsBase-41;
const TInt KErrGsmSMSCongestion=KErrGsmSmsBase-42;
const TInt KErrGsmSMSResourcesUnavailable=KErrGsmSmsBase-47;
const TInt KErrGsmSMSRequestedFacilityNotSubscribed=KErrGsmSmsBase-50;
const TInt KErrGsmSMSRequestedFacilityNotImplemented=KErrGsmSmsBase-69;
const TInt KErrGsmSMSInvalidShortMessageTransferReferenceValue=KErrGsmSmsBase-81;
const TInt KErrGsmSMSUnspecifiedInvalidMessage=KErrGsmSmsBase-95;
const TInt KErrGsmSMSInvalidMandatoryInformation=KErrGsmSmsBase-96;
const TInt KErrGsmSMSNonExistentMessageType=KErrGsmSmsBase-97;
const TInt KErrGsmSMSIncompatibleMessageWithSmsProtocolState=KErrGsmSmsBase-98;
const TInt KErrGsmSMSInformationElementNotImplemented=KErrGsmSmsBase-99;
const TInt KErrGsmSMSUnspecifiedProtocolError=KErrGsmSmsBase-111;
const TInt KErrGsmSMSUnspecifiedInterworkingError=KErrGsmSmsBase-127;

/**
03.40
@internalComponent
*/
const TInt KErrGsmSMSTelematicInterworkingNotSupported=KErrGsmSmsBase-128;
const TInt KErrGsmSMSShortMessageType0NotSupported=KErrGsmSmsBase-129;
const TInt KErrGsmSMSCannotReplaceShortMessage=KErrGsmSmsBase-130;
const TInt KErrGsmSMSUnspecifiedPIDError=KErrGsmSmsBase-143;
const TInt KErrGsmSMSDataCodingSchemeNotSupported=KErrGsmSmsBase-144;
const TInt KErrGsmSMSMessageClassNotSupported=KErrGsmSmsBase-145;
const TInt KErrGsmSMSUnspecifiedDCSError=KErrGsmSmsBase-159;
const TInt KErrGsmSMSCommandCannotBeActioned=KErrGsmSmsBase-160;
const TInt KErrGsmSMSCommandNotSupported=KErrGsmSmsBase-161;
const TInt KErrGsmSMSUnspecifiedCommandError=KErrGsmSmsBase-175;
const TInt KErrGsmSMSTpduNotSupported=KErrGsmSmsBase-176;
const TInt KErrGsmSMSServiceCentreBusy=KErrGsmSmsBase-192;
const TInt KErrGsmSMSNoSCSubscription=KErrGsmSmsBase-193;
const TInt KErrGsmSMSSCSystemFailure=KErrGsmSmsBase-194;
const TInt KErrGsmSMSInvalidSMEAddress=KErrGsmSmsBase-195;
const TInt KErrGsmSMSDestinationSMEBarred=KErrGsmSmsBase-196;
const TInt KErrGsmSMSDuplicateSM=KErrGsmSmsBase-197;
const TInt KErrGsmSMSTPVPFNotSupported=KErrGsmSmsBase-198;
const TInt KErrGsmSMSTPVPNotSupported=KErrGsmSmsBase-199;
const TInt KErrGsmSMSSimSMSStorageFull=KErrGsmSmsBase-208;
const TInt KErrGsmSMSNoSMSStorageCapabilityInSim=KErrGsmSmsBase-209;
const TInt KErrGsmSMSErrorInMS=KErrGsmSmsBase-210;
const TInt KErrGsmSMSMemCapacityExceeded=KErrGsmSmsBase-211;
const TInt KErrGsmSMSSimAppToolkitBusy=KErrGsmSmsBase-212;
const TInt KErrGsmSMSUnspecifiedErrorCause=KErrGsmSmsBase-255;

/**
07.05
@internalComponent
*/
const TInt KErrGsmSMSFailureInME=KErrGsmSmsBase-300;
const TInt KErrGsmSMSServiceOfMSReserved=KErrGsmSmsBase-301;
const TInt KErrGsmSMSOperationNotAllowed=KErrGsmSmsBase-302;
const TInt KErrGsmSMSOperationNotSupported=KErrGsmSmsBase-303;
const TInt KErrGsmSMSInvalidPDUModeParameter=KErrGsmSmsBase-304;
const TInt KErrGsmSMSInvalidTextModeParameter=KErrGsmSmsBase-305;
const TInt KErrGsmSMSSimNotInserted=KErrGsmSmsBase-310;
const TInt KErrGsmSMSSimPin1Required=KErrGsmSmsBase-311;
const TInt KErrGsmSMSPhoneToSimLockRequired=KErrGsmSmsBase-312;
const TInt KErrGsmSMSSimFailure=KErrGsmSmsBase-313;
const TInt KErrGsmSMSSimBusy=KErrGsmSmsBase-314;
const TInt KErrGsmSMSSimWrong=KErrGsmSmsBase-315;
const TInt KErrGsmSMSSimPuk1Required=KErrGsmSmsBase-316;
const TInt KErrGsmSMSPin2Required=KErrGsmSmsBase-317;
const TInt KErrGsmSMSPuk2Required=KErrGsmSmsBase-318;
const TInt KErrGsmSMSMemoryFailure=KErrGsmSmsBase-320;
const TInt KErrGsmSMSInvalidMemoryIndex=KErrGsmSmsBase-321;
const TInt KErrGsmSMSMemoryFull=KErrGsmSmsBase-322;
const TInt KErrGsmSMSUnknownSCAddress=KErrGsmSmsBase-330;
const TInt KErrGsmSMSNoNetworkService=KErrGsmSmsBase-331;
const TInt KErrGsmSMSNetworkTimeout=KErrGsmSmsBase-332;
const TInt KErrGsmSMSCnmaAckNotExpected=KErrGsmSmsBase-340;
const TInt KErrGsmSMSUnknownError=KErrGsmSmsBase-500;

/**
General TE-TA Errors - defined by GSM 07.07
@internalComponent
*/
const TInt KErrGsm0707Base=KErrGsmSmsBase-512;

const TInt KErrGsm0707PhoneFailure=KErrGsm0707Base;
const TInt KErrGsm0707NoConnectionToPhone=KErrGsm0707Base-1;
const TInt KErrGsm0707PhoneLinkReserved=KErrGsm0707Base-2;
const TInt KErrGsm0707OperationNotAllowed=KErrGsm0707Base-3;
const TInt KErrGsm0707OperationNotSupported=KErrGsm0707Base-4;
const TInt KErrGsm0707PhoneToSimLockRequired=KErrGsm0707Base-5;
const TInt KErrGsm0707SimNotInserted=KErrGsm0707Base-10;
const TInt KErrGsm0707SimPin1Required=KErrGsm0707Base-11;
const TInt KErrGsm0707SIMPuk1Required=KErrGsm0707Base-12;
const TInt KErrGsm0707SimFailure=KErrGsm0707Base-13;
const TInt KErrGsm0707SimBusy=KErrGsm0707Base-14;
const TInt KErrGsm0707SimWrong=KErrGsm0707Base-15;
const TInt KErrGsm0707IncorrectPassword=KErrGsm0707Base-16;
const TInt KErrGsm0707SimPin2Required=KErrGsm0707Base-17;
const TInt KErrGsm0707SIMPuk2Required=KErrGsm0707Base-18;
const TInt KErrGsm0707MemoryFull=KErrGsm0707Base-20;
const TInt KErrGsm0707InvalidIndex=KErrGsm0707Base-21;
const TInt KErrGsm0707NotFound=KErrGsm0707Base-22;
const TInt KErrGsm0707MemoryFailure=KErrGsm0707Base-23;
const TInt KErrGsm0707TextStringTooLong=KErrGsm0707Base-24;
const TInt KErrGsm0707InvalidCharsInTextString=KErrGsm0707Base-25;
const TInt KErrGsm0707DialStringTooLong=KErrGsm0707Base-26;
const TInt KErrGsm0707InvalidCharsInDialString=KErrGsm0707Base-27;
const TInt KErrGsm0707NoNetworkService=KErrGsm0707Base-30;
const TInt KErrGsm0707NetworkTimeout=KErrGsm0707Base-31;
const TInt KErrGsm0707UnknownError=KErrGsm0707Base-100;

#endif
