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
// \file mm_hold.h
// Multimode ETel API v1.0
// Header file for ptr holder classes, used to store client data
// 
//

#ifndef _MM_HOLD_H_
#define _MM_HOLD_H_

#include "ETELMM.H"

class CMmPtrHolder : public CBase
/**
class CMmPtrHolder mm_hold.h "INC/mm_hold.h"
brief Is a base class for all ptr holder classes
The ptr holder classes contain the TPtr8's used by asynchronous client-side functions.
The API code also uses them to ensure BC by keeping size of R-classes constant
CMmPtrHolder inherits from CBase
@internalComponent
*/
	{
public:
//	static CMmPtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
	virtual ~CMmPtrHolder();
public:
	template <typename T> inline TPtr8& Set(TInt aSlot,T& aObject)
		{
		TPtr8& ptr=Ptr(aSlot);
		ptr.Set(REINTERPRET_CAST(TText8*,(&aObject)),sizeof(T),sizeof(T));
		return ptr;
		}

	template <typename T> inline TPtrC8& SetC(TInt aSlot, const T& aObject)
		{
		TPtrC8& ptr=PtrC(aSlot);
		ptr.Set(REINTERPRET_CAST(const TText8*,(&aObject)),sizeof(T));
		return ptr;
		}

protected:
	virtual void ConstructL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);	
	CMmPtrHolder();
private:
	TPtr8& Ptr(TInt aIndex);
	TPtrC8& PtrC(TInt aIndex);
protected:
	RArray<TPtr8> iPtrArray;
	RArray<TPtrC8> iPtrCArray;
	};

class CMobilePhonePtrHolder : public CMmPtrHolder
/**
class CMobilePhonePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobilePhone requests
CMobilePhonePtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobilePhone asynchronous requests

	enum TPhonePtrHolderSlots
		{
		ESlot1NotifyIccAccessCapsChange =0,
		ESlot1GetServiceTable,
		ESlot1GetBatteryInfo,
		ESlot1NotifyBatteryInfoChange,
		ESlot1GetSignalStrength,
		ESlot2GetSignalStrength,
		ESlot1NotifySignalStrengthChange,
		ESlot2NotifySignalStrengthChange,
		ESlot1GetIndicator,
		ESlot1NotifyIndicatorChange,
		ESlot1GetPhoneId,
		ESlot1GetSubscriberId,
		ESlot1NotifyDTMFCapsChange,
		ESlot1NotifyModeChange,
		ESlot1GetCurrentNetwork,
		ESlot1NotifyCurrentNetworkChange,
		ESlot1GetNetworkRegistrationStatus,
		ESlot1NotifyNetworkRegistrationStatusChange,
		ESlot1NotifyNetworkSelectionSettingChange,
		ESlot1NotifyNITZInfoChange,
		ESlot1NotifyDefaultPrivacyChange,
		ESlot1NotifyCallServiceCapsChange,
		ESlot1GetFeatureCode,
		ESlot1NotifyCallForwardingStatusChange,
		ESlot1PhoneNotifyCallForwardingActive,
		ESlot2PhoneNotifyCallForwardingActive,
		ESlot2GetIdentityServiceStatus,
		ESlot1NotifyCallBarringStatusChange,
		ESlot1GetCCBSStatus,
		ESlot2GetCCBSStatus,
		ESlot1NotifyCCBSStatusChange,
		ESlot1NotifyAlternatingCallCapsChange,
		ESlot1NotifyAlternatingCallModeChange,
		ESlot2NotifyAlternatingCallModeChange,
		ESlot1NotifyALSLineChange,
		ESlot1NotifyCostCapsChange,
		ESlot1NotifySecurityCapsChange,
		ESlot1NotifyLockInfoChange,
		ESlot1NotifySecurityEvent,
		ESlot1NotifyMessageWaiting,
		ESlot2NotifyMessageWaiting,
		ESlot1NotifyFdnStatusChange,
		ESlot1GetIncomingCallType,
		ESlot1NotifyIncomingCallTypeChange,
		ESlot1GetUUSSetting,
		ESlot1NotifyUUSSettingChange,
		EMaxNumberPhonePtrSlots
		};

	enum TPhonePtrCHolderSlots
		{
		ESlot1SetNetworkSelectionSetting = 0,
		ESlot1SetDefaultPrivacy,
		ESlot1ProgramFeatureCode,
		ESlot1SetCallForwardingStatus,
		ESlot2SetCallForwardingStatus,
		ESlot1GetIdentityServiceStatus,
		ESlot1SetCallBarringStatus,
		ESlot2SetCallBarringStatus,
		ESlot1SetCallBarringPassword,
		ESlot1SetCallWaitingStatus,
		ESlot2SetCallWaitingStatus,
		ESlot1SetAlternatingCallMode,
		ESlot2SetAlternatingCallMode,
		ESlot1SetALSLine,
		ESlot1ClearCostMeter,
		ESlot1SetMaxCostMeter,
		ESlot1GetLockInfo,
		ESlot1SetLockSetting,
		ESlot2SetLockSetting,
		ESlot1ChangeSecurityCode,
		ESlot2ChangeSecurityCode,
		ESlot1VerifySecurityCode,
		ESlot2VerifySecurityCode,
		ESlot1SetFdnSetting,
		ESlot1SelectNetwork,
		ESlot2SelectNetwork,
		ESlot1SetMulticallParams,
		ESlot1SetIncomingCallType,
		ESlot1SetUUSSetting,
		EMaxNumberPhonePtrCSlots
		};

public:
	static CMobilePhonePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
public:
	RMobilePhone::TMobilePhoneServiceTable iServiceTable;
	RMobilePhone::TMobilePhonePrivacy iPrivacySetting;
	RMobilePhone::TMobilePhoneCFCondition iSetCFCondition;
	RMobilePhone::TNetworkServiceAndAction iProgramFeatureCode;
	RMobilePhone::TNetworkServiceAndAction iGetFeatureCode;
	RMobilePhone::TIdServiceAndLocation iGetIdentityServiceStatus;
	RMobilePhone::TMobilePhoneCBCondition iSetCBStatusCondition;
	RMobilePhone::TMobileInfoLocation iGetCCBSStatusLocation;
	RMobilePhone::TMobileService iSetCallWaitingStatusGroup;
	RMobilePhone::TMobilePhoneServiceAction iSetCallWaitingStatusAction;
	RMobilePhone::TMobilePhoneAlternatingCallMode iSetAlternatingCallModeMode;
	RMobilePhone::TMobileService iSetAlternatingCallModeService;
	RMobilePhone::TMobilePhoneALSLine iSetALSLine;
	TUint iSetMaxCostMeterUnits;
	RMobilePhone::TMobilePhoneCostMeters iClearCostMeter;
	RMobilePhone::TMobilePhoneLock iLock;
	RMobilePhone::TMobilePhoneLock iSetLockSettingLock;
	RMobilePhone::TMobilePhoneLockSetting iSetLockSettingSetting;
	RMobilePhone::TMobilePhoneSecurityCode iVerifySecurityCodeType;
	RMobilePhone::TCodeAndUnblockCode iVerifySecurityCodeData;
	RMobilePhone::TMobilePhoneSecurityCode iChangeSecurityCodeType;
	RMobilePhone::TMobilePhoneFdnSetting iSetFdnSetting;
	TBool iIsManual;
	TInt  iUserMaxBearers;
	RMobilePhone::TMobilePhoneIncomingCallType iCallType;
	RMobilePhone::TMobilePhoneUUSSetting iSetUUSSetting;
	};	

class CMobileLinePtrHolder : public CMmPtrHolder
/**
class CMobileLinePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileLine requests
CMobileLinePtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:
	// The ptr holder slot numbers used by RMobileLine asynchronous requests

	enum TLinePtrHolderSlots
		{
		ESlot1LineNotifyMobileLineStatusChange=0,
		EMaxNumberLinePtrSlots
		};

	enum TLinePtrCHolderSlots
		{
		EMaxNumberLinePtrCSlots = 0
		};

public:
	static CMobileLinePtrHolder* NewL(TInt aSizeOfPtrArray, TInt aSizeOfPtrCArray);
public:
	};	

class CMobileCallPtrHolder : public CMmPtrHolder
/**
class CMobileCallPtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileCall requests
CMobileCallPtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:
		// The ptr holder slot numbers used by RMobileCall asynchronous requests

	enum TCallPtrHolderSlots
		{
		ESlot1NotifyMobileCallStatusChange = 0,
		ESlot1NotifyCallEvent,
		ESlot1NotifyPrivacyConfirmation,
		ESlot1NotifyTrafficChannelConfirmation,
		ESlot1NotifyUUSCapsChange,
		ESlot1ReceiveUUI,
		EMaxNumberCallPtrSlots
		};

	enum TCallPtrCHolderSlots
		{
		ESlot1Deflect = 0,
		ESlot2Deflect,
		ESlot1SetDynamicHscsdParams,
		ESlot2SetDynamicHscsdParams,
		ESlot1GetMobileDataCallRLPRange,
		ESlot1SendUUI,
		ESlot2SendUUI,
		ESlot1HangupWithUUI,
		ESlot1AnswerWithUUI,
		EMaxNumberCallPtrCSlots
		};

public:
	static CMobileCallPtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
public:
	RMobileCall::TMobileCallAiur iAiur;
	TInt iRxTimeslots;
	TInt iRLPVersion;
	TBool iMoreUUI;
	RMobileCall::TMobileCallDeflect iDeflectType;
	};	

class CMobileConferenceCallPtrHolder : public CMmPtrHolder
/**
class CMobileConferenceCallPtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileConferenceCall requests
CMobileConferenceCallPtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileConferenceCall asynchronous requests

	enum TConferenceCallPtrHolderSlots
		{
		ESlot1NotifyConferenceCallCapsChange = 0,
		ESlot1NotifyConferenceStatusChange,
		ESlot1NotifyConferenceEvent,
		EMaxNumberConferenceCallPtrSlots	
		};

	enum TConferenceCallPtrCHolderSlots
		{
		EMaxNumberConferenceCallPtrCSlots
		};

public:
	static CMobileConferenceCallPtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);	
	};	

class CSmsMessagingPtrHolder : public CMmPtrHolder
/**
class CSmsMessagingPtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileSmsMessaging requests
CSmsMessagingPtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:
	// The ptr holder slot numbers used by RMobileSmsMessaging asynchronous requests

	enum TSmsPtrHolderSlots
		{
		ESlot1NotifyReceiveModeChange = 0,
		ESlot1NotifyMoSmsBearerChange,
		EMaxNumberSmsPtrSlots
		};

	enum TSmsPtrCHolderSlots
		{
		ESlot1AckSmsStored = 0,
		ESlot1NackSmsStored,
		ESlot1GetMessageStoreInfo,
		ESlot1SetReceiveMode,
		ESlot1SetMoSmsBearer,
		EMaxNumberSmsPtrCSlots
		};

public:
	static CSmsMessagingPtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
	~CSmsMessagingPtrHolder();
protected:
	CSmsMessagingPtrHolder();
public:
	TBool iAckSmsStoredFull;
	TInt iNackSmsStoredCause;
	TInt iGetMessageStoreInfoIndex;
	RMobileSmsMessaging::TMobileSmsReceiveMode iReceiveMode;
	RMobileSmsMessaging::TMobileSmsBearer iSmsBearer;
	CBufBase* iSmspBuf;
	TPtr8 iSmspPtr;
	};	

class CCbsMessagingPtrHolder : public CMmPtrHolder
/**
class CCbsMessagingPtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileBroadcastMessaging requests
CCbsMessagingPtrHolder inherits from CMmPtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileBroadcastMessaging asynchronous requests

	enum TBroadcastPtrHolderSlots
		{
		ESlot1NotifyFilterSettingChange=0,
		ESlot1StoreCbmiList,
		EMaxNumberBroadcastPtrSlots
		};

	enum TBroadcastPtrCHolderSlots
		{
		ESlot1SetFilterSetting=0,
		ESlot1StoreBroadcastIdListL,
		EMaxNumberBroadcastPtrCSlots	
		};

public:
	static CCbsMessagingPtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
	~CCbsMessagingPtrHolder();
protected:
	CCbsMessagingPtrHolder();
public:
	RMobileBroadcastMessaging::TMobilePhoneBroadcastFilter iSetFilterSetting;
	RMobileBroadcastMessaging::TMobileBroadcastIdType iIdType;
	CBufBase* iBroadcastIdBuf;
	TPtr8 iBroadcastIdPtr;
	};	

class CUssdMessagingPtrHolder : public CMmPtrHolder
/**
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileUssdMessaging asynchronous requests

	enum TUssdPtrHolderSlots
		{
		EMaxNumberUssdPtrSlots=0,
		};

	enum TUssdPtrCHolderSlots
		{
		EMaxNumberUssdPtrCSlots=0
		};

public:
	~CUssdMessagingPtrHolder();
protected:
	CUssdMessagingPtrHolder();
public:
	};	

class CMobilePhoneStorePtrHolder : public CMmPtrHolder
/**
class CMobilePhoneStorePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobilePhoneStore requests
CMobilePhoneStorePtrHolder inherits from CMmPtrHolder
CMobilePhoneStorePtrHolder is a base class PtrHolder used by all classes which inherit
from RMobilePhoneStore.  Each class derived from RMobilePhoneStore can create its own
PtrHolder class derived from CMobilePhoneStorePtrHolder.  RMobilePhoneStore derived
classes construct the appropriate PtrHolder class which is passed down to 
RMobilePhoneStore via the RMobilePhoneStore::BaseConstruct method.  Both methods of 
RMobilePhoneStore and a derived class must share the same PtrHolder.  It follows that
they must cooperate to ensure that methods in the base and derived classes do not
use the same slots.  The derived classes should use the slots that are equal to or
greater than EMaxNumberPhoneStorePtrSlots & EMaxNumberPhoneStorePtrCSlots.
@internalComponent
*/
	{
public:
	
	// The ptr holder slot numbers used by RMobilePhoneStore asynchronous requests

	enum TPhoneStorePtrHolderSlots
		{
		ESlot1NotifyStoreEvent = 0,
		ESlot2NotifyStoreEvent,
		EMaxNumberPhoneStorePtrSlots
		};

	enum TPhoneStorePtrCHolderSlots
		{
		ESlot1Delete = 0,
		EMaxNumberPhoneStorePtrCSlots
		};

public:
	static CMobilePhoneStorePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
public:
	TInt iDeleteIndex;
	};	


class CSmsStorePtrHolder : public CMobilePhoneStorePtrHolder
/**
class CSmsStorePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileSmsStore requests
CSmsStorePtrHolder inherits from CMobilePhoneStorePtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileSmsStore asynchronous requests

	enum TSmsStorePtrHolderSlots
		{
		EMaxNumberSmsStorePtrSlots = EMaxNumberPhoneStorePtrSlots
		};

	enum TSmsStorePtrCHolderSlots
		{
		ESlot1GetStatusReport = EMaxNumberPhoneStorePtrCSlots,
		EMaxNumberSmsStorePtrCSlots
		};

public:
	static CSmsStorePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
public:
	TInt iGetStatusReportIndex;
	};	

class CNamStorePtrHolder : public CMobilePhoneStorePtrHolder
/**
class CNamStorePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileNamStore requests
CNamStorePtrHolder inherits from CMobilePhoneStorePtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileNamStore asynchronous requests

	enum TNamStorePtrHolderSlots
		{
		EMaxNumberNamStorePtrSlots = EMaxNumberPhoneStorePtrSlots
		};

	enum TNamStorePtrCHolderSlots
		{
		ESlot1SetActiveNam = EMaxNumberPhoneStorePtrCSlots,
		ESlot1NamListStoreAll,
		EMaxNumberNamStorePtrCSlots
		};

public:
	static CNamStorePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
	~CNamStorePtrHolder();
protected:
	CNamStorePtrHolder();
public:
	CBufBase* iNamBuf;
	TPtr8 iNamPtr;
	TInt iSetActiveNamNamId;
	TInt iStoreAllNamId;
	};	

class CONStorePtrHolder : public CMobilePhoneStorePtrHolder
/**
class CONStorePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobileONStore requests
CONStorePtrHolder inherits from CMobilePhoneStorePtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobileOnStore asynchronous requests

	enum TOnStorePtrHolderSlots
		{
		EMaxNumberONStorePtrSlots = EMaxNumberPhoneStorePtrSlots
		};

	enum TOnStorePtrCHolderSlots
		{
		EMaxNumberONStorePtrCSlots = EMaxNumberPhoneStorePtrCSlots
		};

public:
	static CONStorePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
	~CONStorePtrHolder();
protected:
	CONStorePtrHolder();
public:
	CBufBase* iONBuf;
	TPtr8 iONPtr;
	};	

class CPhoneBookStorePtrHolder : public CMobilePhoneStorePtrHolder
/**
class CPhoneBookStorePtrHolder mm_hold.h "INC/mm_hold.h"
brief Holds the TPtr8 arrays for all RMobilePhonebookStore requests
CPhoneBookStorePtrHolder inherits from CMobilePhoneStorePtrHolder
@internalComponent
*/
	{
public:

	// The ptr holder slot numbers used by RMobilePhonebookStore asynchronous requests

	enum TPhoneBookStorePtrHolderSlots
		{
		ESlot1PhoneBookStoreWrite = EMaxNumberPhoneStorePtrSlots,
		EMaxNumberPhoneBookStorePtrSlots
		};

	enum TPhoneBookStorePtrCHolderSlots
		{
		ESlot1PhoneBookStoreRead = EMaxNumberPhoneStorePtrCSlots,
		EMaxNumberPhoneBookStorePtrCSlots 
		};

public:
	static CPhoneBookStorePtrHolder* NewL(TInt aSizeOfPtrArray,TInt aSizeOfPtrCArray);
public:
	RMobilePhoneBookStore::TPBIndexAndNumEntries iReadPhoneBookEntry;
	};	

#endif
