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
// \file mmretrieve.h
// Multimode ETel API v1.0
// Header file for asynchronous list retrieval classes
// 
//

#ifndef _MMRETRIEVE_H_
#define _MMRETRIEVE_H_

#include "ETELMM.H"
#include "mmlist.h"

#include <e32base.h>

class CAsyncRetrieveVariableLengthBuffer : public CActive
/**
class CAsyncRetrieveVariableLengthBuffer mmretrieve.h "INC/mmretrieve.h"
brief Base class for generic actions in retrieving a variable length buffer in two phases
CAsyncRetrieveVariableLengthBuffer inherits from CActive
@internalComponent
*/
	{
protected:

	//
	// Start the retrieval
	//

	void Start(TRequestStatus& aReqStatus, TDes8* aPhase1Request, TDes8* aPhase2Request);
	CAsyncRetrieveVariableLengthBuffer();
	virtual ~CAsyncRetrieveVariableLengthBuffer();

	TBool CompleteIfInUse(TRequestStatus& aReqStatus);
	void FreeBuffer();

private:
	virtual void RestoreListL();
	virtual void Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2) = 0;
	virtual void CancelReq(TInt aIpc1,TInt aIpc2) = 0;

	virtual void DoCancel();
	virtual void RunL();

	void StartPhase2L();

protected:
	enum {
		EIdle,
		ERetrievePhase1,
		ERetrievePhase2
		} iState;

	CBufBase* iResultsBuf;
	TPtr8 iResultsPtr;
	TInt iIpcPhase1;
	TInt iIpcPhase2;
	TInt iIpcCancel;
private:
	TRequestStatus* iUserStatus;
	TDes8* iPhase1RequestData;
	TDes8* iPhase2RequestData;
	TPckgBuf<TInt> iBufferSize;
	};

class CAsyncRetrieveWithClientIds : public CAsyncRetrieveVariableLengthBuffer
/**
class CAsyncRetrieveWithClientIds mmretrieve.h "INC/mmretrieve.h"
brief Base class that adds passing of client ID within each phase of the retrieval
CAsyncRetrieveWithClientIds inherits from CAsyncRetrieveVariableLengthBuffer
@internalComponent
*/
	{
protected:
	CAsyncRetrieveWithClientIds(TInt aSessionHandle, TInt aSubSessionHandle);
protected:
	TPckgBuf<RMobilePhone::TClientId> iId;
	};

class CAsyncRetrieveStoreList : public CAsyncRetrieveWithClientIds
/**
class CAsyncRetrieveStoreList mmretrieve.h "INC/mmretrieve.h"
brief Class that specialises in retrieving lists from phone store classes
CAsyncRetrieveStoreList inherits from CAsyncRetrieveWithClientIds
@internalComponent
*/
	{
public:
	CAsyncRetrieveStoreList(RMobilePhoneStore& aStore);
	virtual void Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2);
	virtual void CancelReq(TInt aIpc1, TInt aIpc2);

	IMPORT_C void Start(TRequestStatus& aReqStatus);

protected:
	RMobilePhoneStore& iStore;
	};

class CAsyncRetrievePhoneList : public CAsyncRetrieveWithClientIds
/**
class CAsyncRetrievePhoneList mmretrieve.h "INC/mmretrieve.h"
brief Class that specialises in retrieving lists from phone or network
CAsyncRetrievePhoneList inherits from CAsyncRetrieveWithClientIds
@internalComponent
*/
	{
public:
	CAsyncRetrievePhoneList(RMobilePhone& aPhone);
	virtual void Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2);
	virtual void CancelReq(TInt aIpc1, TInt aIpc2);

protected:
	RMobilePhone& iPhone;
	};

class CRetrieveMobilePhoneDetectedNetworks : public CAsyncRetrievePhoneList
/**
class CRetrieveMobilePhoneDetectedNetworks mmretrieve.h "INC/mmretrieve.h"
brief Retrieves the detected network list from the phone
CRetrieveMobilePhoneDetectedNetworks inherits from CAsyncRetrievePhoneList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneDetectedNetworks* NewL(RMobilePhone& aPhone);
	IMPORT_C ~CRetrieveMobilePhoneDetectedNetworks();

	IMPORT_C void Start(TRequestStatus& aReqStatus);

	IMPORT_C CMobilePhoneNetworkList* RetrieveListL();

protected:
	CRetrieveMobilePhoneDetectedNetworks(RMobilePhone& aPhone);
	void ConstructL();

	virtual void RestoreListL();

private:
	CMobilePhoneNetworkList* iResults;
	};

class CRetrieveMobilePhoneCFList : public CAsyncRetrievePhoneList
/**
class CRetrieveMobilePhoneCFList mmretrieve.h "INC/mmretrieve.h"
brief Retrieves the Call Forwarding status list from the phone
CRetrieveMobilePhoneCFList inherits from CAsyncRetrievePhoneList
@internalComponent
*/
	{
public:

	// for use by client-side API code and TSY only

	struct TGetCallForwardingRequest
		{
		RMobilePhone::TClientId iClient;
		RMobilePhone::TMobilePhoneCFCondition iCondition;
		RMobilePhone::TMobileInfoLocation iLocation;
		};

public:
	IMPORT_C static CRetrieveMobilePhoneCFList* NewL(RMobilePhone& aPhone);
	IMPORT_C ~CRetrieveMobilePhoneCFList();

	IMPORT_C void Start(TRequestStatus& aReqStatus, 
						RMobilePhone::TMobilePhoneCFCondition aCondition, 
						RMobilePhone::TMobileInfoLocation aLocation = RMobilePhone::EInfoLocationCachePreferred);

	IMPORT_C CMobilePhoneCFList* RetrieveListL();

protected:
	CRetrieveMobilePhoneCFList(RMobilePhone& aPhone);
	void ConstructL();

	virtual void RestoreListL();

private:
	CMobilePhoneCFList* iResults;
	TPckgBuf<TGetCallForwardingRequest> iCFRequestData;
	};

class CRetrieveMobilePhoneCBList : public CAsyncRetrievePhoneList
/**
class CRetrieveMobilePhoneCBList mmretrieve.h "INC/mmretrieve.h"
brief Retrieves the Call Barring status list from the phone
CRetrieveMobilePhoneCBList inherits from CAsyncRetrievePhoneList
@internalComponent
*/
	{
public:

	// for use by client-side API code and TSY only

	struct TGetCallBarringRequest
		{
		RMobilePhone::TClientId iClient;
		RMobilePhone::TMobilePhoneCBCondition iCondition;
		RMobilePhone::TMobileInfoLocation iLocation;
		};


public:
	IMPORT_C static CRetrieveMobilePhoneCBList* NewL(RMobilePhone& aPhone);
	IMPORT_C ~CRetrieveMobilePhoneCBList();

	IMPORT_C void Start(TRequestStatus& aReqStatus, 
						RMobilePhone::TMobilePhoneCBCondition aCondition, 
						RMobilePhone::TMobileInfoLocation aLocation = RMobilePhone::EInfoLocationCachePreferred);

	IMPORT_C CMobilePhoneCBList* RetrieveListL();

protected:
	CRetrieveMobilePhoneCBList(RMobilePhone& aPhone);
	void ConstructL();

	virtual void RestoreListL();

private:
	CMobilePhoneCBList* iResults;
	TPckgBuf<TGetCallBarringRequest> iCBRequestData;
	};

class CRetrieveMobilePhoneCWList : public CAsyncRetrievePhoneList
/**
class CRetrieveMobilePhoneCWList mmretrieve.h "INC/mmretrieve.h"
brief Retrieves the Call Waiting status list from the phone
CRetrieveMobilePhoneCWList inherits from CAsyncRetrievePhoneList
@internalComponent
*/
	{
public:

	// for use by client-side API code and TSY only

	struct TGetCallWaitingRequest
		{
		RMobilePhone::TClientId iClient;
		RMobilePhone::TMobileInfoLocation iLocation;
		};

public:
	IMPORT_C static CRetrieveMobilePhoneCWList* NewL(RMobilePhone& aPhone);
	IMPORT_C ~CRetrieveMobilePhoneCWList();

	IMPORT_C void Start(TRequestStatus& aReqStatus,RMobilePhone::TMobileInfoLocation aLocation = RMobilePhone::EInfoLocationCachePreferred);

	IMPORT_C CMobilePhoneCWList* RetrieveListL();

protected:
	CRetrieveMobilePhoneCWList(RMobilePhone& aPhone);
	void ConstructL();

	virtual void RestoreListL();

private:
	CMobilePhoneCWList* iResults;
	TPckgBuf<TGetCallWaitingRequest> iCWRequestData;
	};

class CRetrieveMobilePhoneCcbsList : public CAsyncRetrievePhoneList
/**
class CRetrieveMobilePhoneCcbsList mmretrieve.h "INC/mmretrieve.h"
brief Retrieves the CCBS request list from the phone
CRetrieveMobilePhoneCcbsList inherits from CAsyncRetrievePhoneList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneCcbsList* NewL(RMobilePhone& aPhone);
	IMPORT_C ~CRetrieveMobilePhoneCcbsList();

	IMPORT_C void Start(TRequestStatus& aReqStatus);

	IMPORT_C CMobilePhoneCcbsList* RetrieveListL();

protected:
	CRetrieveMobilePhoneCcbsList(RMobilePhone& aPhone);
	void ConstructL();

	virtual void RestoreListL();

private:
	CMobilePhoneCcbsList* iResults;
	};

class CRetrieveMobilePhoneSmsList : public CAsyncRetrieveStoreList
/**
class CRetrieveMobilePhoneSmsList mmretrieve.h "INC/mmretrieve.h"
brief Reads stored, fixed length SMS message list from the phone or SIM message store
CRetrieveMobilePhoneSmsList inherits from CAsyncRetrieveStoreList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneSmsList* NewL(RMobilePhoneStore& aStore, RMobileSmsStore::TMobileSmsEntryExtensionId aType);
	IMPORT_C ~CRetrieveMobilePhoneSmsList();

	enum
		{
		KValueNotUsed = -1
		};

	struct TBatchRequestData
		{
		RMobilePhone::TClientId iClient;
		RMobileSmsStore::TMobileSmsEntryExtensionId iEntryType;
		TBool iBatchRequest;
		TInt iStartIndex;
		TInt iBatchSize;
		};
		
	IMPORT_C void Start(TRequestStatus& aReqStatus);
	IMPORT_C void StartBatch(TRequestStatus& aReqStatus, TInt aStartIndex, TInt aBatchSize);

	IMPORT_C CMobilePhoneGsmSmsList* RetrieveGsmListL();
	
protected:
	CRetrieveMobilePhoneSmsList(RMobilePhoneStore& aStore, RMobileSmsStore::TMobileSmsEntryExtensionId aType);
	void ConstructL();
	void InitiateRequest(TRequestStatus& aReqStatus, TBool aBatch, TInt aStartIndex, TInt aBatchSize);

	virtual void RestoreListL();

private:
	CMobilePhoneGsmSmsList* iGsmResults;
	RMobileSmsStore::TMobileSmsEntryExtensionId iEntryType;
	TPckgBuf<TBatchRequestData> iRequestData;
	};

class CRetrieveMobilePhoneSmspList : public CAsyncRetrieveWithClientIds
/**
class CRetrieveMobilePhoneSmspList mmretrieve.h "INC/mmretrieve.h"
brief Reads SMS parameter list from the SIM's SMSP store
CRetrieveMobilePhoneSmspList inherits from CAsyncRetrieveWithClientIds
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneSmspList* NewL(RMobileSmsMessaging& aMessaging);
	IMPORT_C ~CRetrieveMobilePhoneSmspList();

	IMPORT_C CMobilePhoneSmspList* RetrieveListL();
	IMPORT_C void Start(TRequestStatus& aReqStatus);

protected:
	CRetrieveMobilePhoneSmspList(RMobileSmsMessaging& aMessaging);
	void ConstructL();
	virtual void RestoreListL();

private:
	virtual void Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2);
	virtual void CancelReq(TInt aIpc1, TInt aIpc2);

private:
	RMobileSmsMessaging& iMessaging;
	CMobilePhoneSmspList* iResults;
	};

class CRetrieveMobilePhoneBroadcastIdList : public CAsyncRetrieveWithClientIds
/**
class CRetrieveMobilePhoneBroadcastIdList mmretrieve.h "INC/mmretrieve.h"
brief Reads CBMI list from the phone. This list may be stored on SIM
CRetrieveMobilePhoneBroadcastIdList inherits from CAsyncRetrieveWithClientIds
@internalComponent
*/
	{
public:
		// for use by client-side API code and TSY only

	struct TGetBroadcastIdRequest
		{
		RMobileBroadcastMessaging::TMobileBroadcastIdType iIdType;
		RMobilePhone::TClientId iClient;
		};

public:
	IMPORT_C static CRetrieveMobilePhoneBroadcastIdList* NewL(RMobileBroadcastMessaging& aMessaging);
	IMPORT_C ~CRetrieveMobilePhoneBroadcastIdList();

	IMPORT_C CMobilePhoneBroadcastIdList* RetrieveListL();
	IMPORT_C void Start(TRequestStatus& aReqStatus, RMobileBroadcastMessaging::TMobileBroadcastIdType aIdType);

protected:
	CRetrieveMobilePhoneBroadcastIdList(RMobileBroadcastMessaging& aMessaging);
	void ConstructL();
	virtual void RestoreListL();

private:
	virtual void Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2);
	virtual void CancelReq(TInt aIpc1, TInt aIpc2);

protected:
	RMobileBroadcastMessaging& iMessaging;
	CMobilePhoneBroadcastIdList* iResults;
	TPckgBuf<TGetBroadcastIdRequest> iBroadcastIdRequestData;
	};

class CRetrieveMobilePhoneNamList : public CAsyncRetrieveStoreList
/**
class CRetrieveMobilePhoneNamList mmretrieve.h "INC/mmretrieve.h"
brief Reads NAM list from the phone's NAM store
CRetrieveMobilePhoneNamList inherits from CAsyncRetrieveStoreList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneNamList* NewL(RMobilePhoneStore& aStore);
	IMPORT_C ~CRetrieveMobilePhoneNamList();

	struct TNamRequestData
		{
		RMobilePhone::TClientId iClient;
		TInt iNamId;
		};

	IMPORT_C CMobilePhoneNamList* RetrieveListL();
	IMPORT_C void Start(TRequestStatus& aReqStatus, TInt aNamId);

protected:
	CRetrieveMobilePhoneNamList(RMobilePhoneStore& aStore);
	void ConstructL();
	
	virtual void RestoreListL();

private:
	CMobilePhoneNamList* iResults;
	TPckgBuf<TNamRequestData> iRequestData;
	};

class CRetrieveMobilePhoneONList : public CAsyncRetrieveStoreList
/**
class CRetrieveMobilePhoneONList mmretrieve.h "INC/mmretrieve.h"
brief Reads Own Number list from the phone or SIM's Own Number store
CRetrieveMobilePhoneONList inherits from CAsyncRetrieveStoreList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneONList* NewL(RMobilePhoneStore& aStore);
	IMPORT_C ~CRetrieveMobilePhoneONList();

	IMPORT_C CMobilePhoneONList* RetrieveListL();
	IMPORT_C void Start(TRequestStatus& aReqStatus);

protected:
	CRetrieveMobilePhoneONList(RMobilePhoneStore& aStore);
	void ConstructL();
	
	virtual void RestoreListL();

private:
	CMobilePhoneONList* iResults;
	};

class CRetrieveMobilePhoneENList : public CAsyncRetrieveStoreList
/**
class CRetrieveMobilePhoneENList mmretrieve.h "INC/mmretrieve.h"
brief Reads Emergency Number list from the phone or SIM store
CRetrieveMobilePhoneENList inherits from CAsyncRetrieveStoreList
@internalComponent
*/
	{
public:
	IMPORT_C static CRetrieveMobilePhoneENList* NewL(RMobilePhoneStore& aStore);
	IMPORT_C ~CRetrieveMobilePhoneENList();

	IMPORT_C CMobilePhoneENList* RetrieveListL();
	IMPORT_C void Start(TRequestStatus& aReqStatus);

protected:
	CRetrieveMobilePhoneENList(RMobilePhoneStore& aStore);
	void ConstructL();
	
	virtual void RestoreListL();

private:
	CMobilePhoneENList* iResults;
	};


#endif
