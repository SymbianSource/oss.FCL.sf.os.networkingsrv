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
// This file contains all the code for the retrieval classes defined in the api which
// retrieve variable length buffers from the TSY, such as Sms messages or list of detected
// networks
// 
//

// Multimode header files
#include "ETELMM.H"
#include "mmlist.h"
#include "mmretrieve.h"
#include "ETELEXT.H"

#include "mm_hold.h"

/***********************************************************************************/
//
// CAsyncRetrieveVariableLengthBuffer is the base class used by any class in Etelmm 
// which retrieves variable length buffers from the Tsy.  Buffer retrieval must be
// performed in two stages.  Each stage has its own separate IPC number, which are
// stored in iIpcPhase1 and iIpcPhase2 respectively.  There is only one cancel IPC
// for both stages.  Classes deriving from CAsyncRetrieveVariableLengthBuffer should 
// initialise these values appropriately.  Each request is accompanied by two parameters.
// In phase1 requests the first parameter can be used by derived classes to provide
// the Tsy with information about the sort of buffer being retrieved.  The second
// parameter represents the size of the buffer required to hold the requested data.
// The Tsy should initialise this parameter to contain the appropriate value.  
// The first parameter of the phase 2 request also contains information which the TSY
// can inspect to determine which data to return.  The second parameter references a
// client side buffer into which the TSY should write the requested data.  Once the
// data has been retrieved from the TSY the RestoreList method is called to allow derived
// classes to convert the data in the iResultsBuf into the correct format.  
// Note it is acceptable for the first parameters of either phase to be blank
//
/***********************************************************************************/

CAsyncRetrieveVariableLengthBuffer::CAsyncRetrieveVariableLengthBuffer() 
: 	CActive(EPriorityStandard), 
	iState(EIdle), 
	iResultsPtr(NULL,0)
	{
	CActiveScheduler::Add(this);
	}

CAsyncRetrieveVariableLengthBuffer::~CAsyncRetrieveVariableLengthBuffer()
	{
	Cancel();
	delete iResultsBuf;
	}

void CAsyncRetrieveVariableLengthBuffer::Start(TRequestStatus& aReqStatus, TDes8* aPhase1Request, TDes8* aPhase2Request)
/**
 * This method performs the first phase of the retrieval of a variable length buffer
 * The first phase consists of retrieving the size of buffer to be read
 *
 * \param aPhase1Request Pointer to data associated with phase 1
 * \param aPhase2Request Pointer to data associated with phase 2
 */
	{
	__ASSERT_DEBUG(aPhase1Request != NULL,PanicClient(EEtelPanicNullHandle));
	__ASSERT_DEBUG(aPhase2Request != NULL,PanicClient(EEtelPanicNullHandle));

	delete iResultsBuf;
	iResultsBuf = NULL;

	iUserStatus=&aReqStatus;
	*iUserStatus=KRequestPending;

	iPhase1RequestData = aPhase1Request;
	iPhase2RequestData = aPhase2Request;

	Get(iIpcPhase1,iStatus,*iPhase1RequestData,iBufferSize);

	SetActive();
	iState=ERetrievePhase1;
	}


void CAsyncRetrieveVariableLengthBuffer::StartPhase2L()
/**
 * This method performs the second phase of the retrieval of a variable length buffer
 * The second phase consists of allocating a buffer of the appropriate size and then filling
 * it by reading the data across from the TSY.
 */
	{
	*iUserStatus=KRequestPending;
	
	iResultsBuf = CBufFlat::NewL(1);
	iResultsBuf->ResizeL(iBufferSize());	

	TPtr8 tempPtr(iResultsBuf->Ptr(0));
	tempPtr.FillZ();

	iResultsPtr.Set(tempPtr);

	Get(iIpcPhase2,iStatus,*iPhase2RequestData,iResultsPtr);
	SetActive();
	iState=ERetrievePhase2;
	}

void CAsyncRetrieveVariableLengthBuffer::DoCancel()
/**
 * This method cancels the active phase of the retrieval
 * It is called from the CActive::Cancel() method
 */
	{
	__ASSERT_DEBUG(iState != EIdle,PanicClient(EEtelPanicInvalidRequestType));

	if (iState==ERetrievePhase1)
		CancelReq(iIpcCancel,iIpcPhase1);
	else
		CancelReq(iIpcCancel,iIpcPhase2);

	// have to complete the userstatus too
	*iUserStatus=KErrCancel;
	User::RequestComplete(iUserStatus,iUserStatus->Int()); 
	iState=EIdle;
	}

void CAsyncRetrieveVariableLengthBuffer::RunL()
/**
 * This method processes the completion of each asynchronous phase of the retrieval
 * If phase 1 is now complete, it starts phase 2
 * If phase 2 is now complete, it complete's the client's request
 */
	{
	if (iStatus.Int()!=KErrNone)
		{
		*iUserStatus=iStatus.Int();
		User::RequestComplete(iUserStatus,iUserStatus->Int());
		iState=EIdle;
		}
	else
		{
		switch(iState)
			{
		case ERetrievePhase1:
				{
				if (iBufferSize()<1)
					{
					*iUserStatus=KErrNotFound;
					User::RequestComplete(iUserStatus,iUserStatus->Int());
					iState=EIdle;
					}
				else
					{
					TRAPD(ret,StartPhase2L());
					if (ret!=KErrNone)
						{
						*iUserStatus=ret;
						User::RequestComplete(iUserStatus,iUserStatus->Int());
						iState=EIdle;		
						}
					}
				}
		break;

		case ERetrievePhase2:
			TRAPD(ret,RestoreListL());
			*iUserStatus=ret;
			User::RequestComplete(iUserStatus,iUserStatus->Int());
			iState=EIdle;
			break;
			
		case EIdle:
		default:
			*iUserStatus=KErrGeneral;
			User::RequestComplete(iUserStatus,iUserStatus->Int());
			break;
			}
		}
	}

TBool CAsyncRetrieveVariableLengthBuffer::CompleteIfInUse(TRequestStatus& aReqStatus)
/**
 * This method checks whether the AO is currently in use or not
 * If it is in use, then the client's request is completed with KErrInUse
 *
 * \return TBool A boolean indicating whether the AO is in use or not
 */
	{
	TBool retVal = EFalse;
	if (iState != EIdle)
		{
		TRequestStatus* rs = &aReqStatus;
		retVal = ETrue;
		User::RequestComplete(rs,KErrInUse);
		}
	return retVal;
	}

void CAsyncRetrieveVariableLengthBuffer::FreeBuffer()
/**
 * This method deallocates the buffer used to hold the retrieved data
 */
	{
	delete iResultsBuf;
	iResultsBuf = NULL;
	iResultsPtr.Set(NULL,0,0);
	}

void CAsyncRetrieveVariableLengthBuffer::RestoreListL()
	{}

/***********************************************************************************/
//
// base class for buffer retrieval which require client ids to be sent to tsy
//
/***********************************************************************************/


CAsyncRetrieveWithClientIds::CAsyncRetrieveWithClientIds(TInt aSessionHandle, TInt aSubSessionHandle)
	{
	iId().iSessionHandle = aSessionHandle;
	iId().iSubSessionHandle = aSubSessionHandle;
	}

/***********************************************************************************/
//
// base class for list retrieval from RMobilePhoneStore
//
/***********************************************************************************/

CAsyncRetrieveStoreList::CAsyncRetrieveStoreList(RMobilePhoneStore& aStore) : 
	CAsyncRetrieveWithClientIds(aStore.SessionHandle().Handle(), aStore.SubSessionHandle()), 
	iStore(aStore)
	{
	iIpcPhase1=EMobilePhoneStoreReadAllPhase1;
	iIpcPhase2=EMobilePhoneStoreReadAllPhase2;
	iIpcCancel=EMobilePhoneStoreReadAllCancel;
	}

void CAsyncRetrieveStoreList::Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2)
/**
 * This method uses the Get IPC request within RTelSubsessionBase
 * to send one of the asynchronous phase requests to ETel server
 *
 * \param aIpc The IPC value of the asynchronous phase request
 * \param aReqStatus The request status for the asynchronous phase request
 * \param aDes1 The first parameter for the asynchronous phase request
 * \param aDes2 The second parameter for the asynchronous phase request
 */
	{
	iStore.Get(aIpc,aReqStatus,aDes1,aDes2);
	}

void CAsyncRetrieveStoreList::CancelReq(TInt aIpc1, TInt aIpc2)
/**
 * This method uses the Cancel IPC request within RTelSubsessionBase
 * to send a cancel for one of the asynchronous phases to ETel server
 *
 * \param aIpc1 The IPC value of the cancel request
 * \param aIpc2 The IPC value of the original asynchronous phase request
 */
	{
	iStore.CancelReq(aIpc1,aIpc2);
	}

/***********************************************************************************/
//
// base class for list retrieval from RMobilePhone
//
/***********************************************************************************/

CAsyncRetrievePhoneList::CAsyncRetrievePhoneList(RMobilePhone& aPhone) : 
	CAsyncRetrieveWithClientIds(aPhone.SessionHandle().Handle(), aPhone.SubSessionHandle()),
	iPhone(aPhone) 
	{
	}

void CAsyncRetrievePhoneList::Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2)
/**
 * This method uses the Get IPC request within RTelSubsessionBase
 * to send one of the asynchronous phase requests to ETel server
 *
 * \param aIpc The IPC value of the asynchronous phase request
 * \param aReqStatus The request status for the asynchronous phase request
 * \param aDes1 The first parameter for the asynchronous phase request
 * \param aDes2 The second parameter for the asynchronous phase request
 */
	{
	iPhone.Get(aIpc,aReqStatus,aDes1,aDes2);
	}

void CAsyncRetrievePhoneList::CancelReq(TInt aIpc1, TInt aIpc2)
/**
 * This method uses the Cancel IPC request within RTelSubsessionBase
 * to send a cancel for one of the asynchronous phases to ETel server
 *
 * \param aIpc1 The IPC value of the cancel request
 * \param aIpc2 The IPC value of the original asynchronous phase request
 */
	{
	iPhone.CancelReq(aIpc1,aIpc2);
	}


/***********************************************************************************/
//
// Retrieve a list of detected networks
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneDetectedNetworks* CRetrieveMobilePhoneDetectedNetworks::NewL(RMobilePhone& aPhone)
	{
	CRetrieveMobilePhoneDetectedNetworks* r=new(ELeave) CRetrieveMobilePhoneDetectedNetworks(aPhone);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneDetectedNetworks::CRetrieveMobilePhoneDetectedNetworks(RMobilePhone& aPhone) 
:	CAsyncRetrievePhoneList(aPhone)
	{
	}

void CRetrieveMobilePhoneDetectedNetworks::ConstructL()
	{
	iIpcPhase1=EMobilePhoneGetDetectedNetworksPhase1;
	iIpcPhase2=EMobilePhoneGetDetectedNetworksPhase2;
	iIpcCancel=EMobilePhoneGetDetectedNetworksCancel;
	}

EXPORT_C CRetrieveMobilePhoneDetectedNetworks::~CRetrieveMobilePhoneDetectedNetworks()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneNetworkList* CRetrieveMobilePhoneDetectedNetworks::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneNetworkList* Pointer to the detected networks list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneNetworkList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneDetectedNetworks::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of the detected networks list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;
		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iId,&iId);
		}
	}
		

void CRetrieveMobilePhoneDetectedNetworks::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneNetworkList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}


/***********************************************************************************/
//
// Retrieve a list of call forwarding status/info
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneCFList* CRetrieveMobilePhoneCFList::NewL(RMobilePhone& aPhone)
	{
	CRetrieveMobilePhoneCFList* r=new(ELeave) CRetrieveMobilePhoneCFList(aPhone);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneCFList::CRetrieveMobilePhoneCFList(RMobilePhone& aPhone) : 
	CAsyncRetrievePhoneList(aPhone)
	{
	}

void CRetrieveMobilePhoneCFList::ConstructL()
	{
	iIpcPhase1=EMobilePhoneGetCallForwardingStatusPhase1;
	iIpcPhase2=EMobilePhoneGetCallForwardingStatusPhase2;
	iIpcCancel=EMobilePhoneGetCallForwardingStatusCancel;	
	}

EXPORT_C CRetrieveMobilePhoneCFList::~CRetrieveMobilePhoneCFList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneCFList* CRetrieveMobilePhoneCFList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneCFList* Pointer to the call forwarding status list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneCFList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneCFList::Start(TRequestStatus& aReqStatus, 
												RMobilePhone::TMobilePhoneCFCondition aCondition, 
												RMobilePhone::TMobileInfoLocation aLocation)
/**
 * This method starts the retrieval of the call forwarding status list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		iCFRequestData().iCondition=aCondition;
		iCFRequestData().iLocation=aLocation;
		iCFRequestData().iClient=iId();

		CAsyncRetrievePhoneList::Start(aReqStatus,&iCFRequestData,&iId);
		}
	}
		
void CRetrieveMobilePhoneCFList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneCFList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

/***********************************************************************************/
//
// Retrieve a list of call barring status/info
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneCBList* CRetrieveMobilePhoneCBList::NewL(RMobilePhone& aPhone)
	{
	CRetrieveMobilePhoneCBList* r=new(ELeave) CRetrieveMobilePhoneCBList(aPhone);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneCBList::CRetrieveMobilePhoneCBList(RMobilePhone& aPhone) : 
	CAsyncRetrievePhoneList(aPhone)
	{
	}

void CRetrieveMobilePhoneCBList::ConstructL()
	{
	iIpcPhase1=EMobilePhoneGetBarringStatusPhase1;
	iIpcPhase2=EMobilePhoneGetBarringStatusPhase2;
	iIpcCancel=EMobilePhoneGetBarringStatusCancel;
	}

EXPORT_C CRetrieveMobilePhoneCBList::~CRetrieveMobilePhoneCBList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneCBList* CRetrieveMobilePhoneCBList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneCBList* Pointer to the call barring status list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneCBList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneCBList::Start(TRequestStatus& aReqStatus, 
												RMobilePhone::TMobilePhoneCBCondition aCondition, 
												RMobilePhone::TMobileInfoLocation aLocation)
/**
 * This method starts the retrieval of the call barring status list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		iCBRequestData().iCondition=aCondition;
		iCBRequestData().iLocation=aLocation;
		iCBRequestData().iClient=iId();

		CAsyncRetrievePhoneList::Start(aReqStatus,&iCBRequestData,&iId);
		}
	}
		
void CRetrieveMobilePhoneCBList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneCBList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

/***********************************************************************************/
//
// Retrieve a list of call waiting status/info
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneCWList* CRetrieveMobilePhoneCWList::NewL(RMobilePhone& aPhone)
	{
	CRetrieveMobilePhoneCWList* r=new(ELeave) CRetrieveMobilePhoneCWList(aPhone);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneCWList::CRetrieveMobilePhoneCWList(RMobilePhone& aPhone) : 
	CAsyncRetrievePhoneList(aPhone)
	{
	}

void CRetrieveMobilePhoneCWList::ConstructL()
	{
	iIpcPhase1=EMobilePhoneGetWaitingStatusPhase1;
	iIpcPhase2=EMobilePhoneGetWaitingStatusPhase2;
	iIpcCancel=EMobilePhoneGetWaitingStatusCancel;
	}

EXPORT_C CRetrieveMobilePhoneCWList::~CRetrieveMobilePhoneCWList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneCWList* CRetrieveMobilePhoneCWList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneCWList* Pointer to the call waiting status list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneCWList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneCWList::Start(TRequestStatus& aReqStatus, 
												RMobilePhone::TMobileInfoLocation aLocation)
/**
 * This method starts the retrieval of the call waiting status list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		iCWRequestData().iLocation=aLocation;
		iCWRequestData().iClient=iId();
	
		CAsyncRetrievePhoneList::Start(aReqStatus,&iCWRequestData,&iId);
		}
	}
		
void CRetrieveMobilePhoneCWList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneCWList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}


/***********************************************************************************/
//
// Retrieve a list of call completion Requests
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneCcbsList* CRetrieveMobilePhoneCcbsList::NewL(RMobilePhone& aPhone)
	{
	CRetrieveMobilePhoneCcbsList* r=new(ELeave) CRetrieveMobilePhoneCcbsList(aPhone);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneCcbsList::CRetrieveMobilePhoneCcbsList(RMobilePhone& aPhone) : 
	CAsyncRetrievePhoneList(aPhone)
	{
	}

void CRetrieveMobilePhoneCcbsList::ConstructL()
	{
	iIpcPhase1=EMobilePhoneGetCompletionRequestsPhase1;
	iIpcPhase2=EMobilePhoneGetCompletionRequestsPhase2;
	iIpcCancel=EMobilePhoneGetCompletionRequestsCancel;	
	}

EXPORT_C CRetrieveMobilePhoneCcbsList::~CRetrieveMobilePhoneCcbsList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneCcbsList* CRetrieveMobilePhoneCcbsList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneCcbsList* Pointer to the CCBS request list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneCcbsList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneCcbsList::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of the CCBS request list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		CAsyncRetrievePhoneList::Start(aReqStatus,&iId,&iId);
		}
	}
		
void CRetrieveMobilePhoneCcbsList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneCcbsList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

/***********************************************************************************/
//
// Retrieve a list of SMS messages from a phone-side SMS store
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneSmsList* CRetrieveMobilePhoneSmsList::NewL(RMobilePhoneStore& aStore, RMobileSmsStore::TMobileSmsEntryExtensionId aType)
	{
	CRetrieveMobilePhoneSmsList* r=new(ELeave) CRetrieveMobilePhoneSmsList(aStore, aType);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneSmsList::CRetrieveMobilePhoneSmsList(RMobilePhoneStore& aStore, RMobileSmsStore::TMobileSmsEntryExtensionId aType) 
:	CAsyncRetrieveStoreList(aStore), iEntryType(aType)
	{
	}

void CRetrieveMobilePhoneSmsList::ConstructL()
	{
	}

EXPORT_C CRetrieveMobilePhoneSmsList::~CRetrieveMobilePhoneSmsList()
	{
	delete iGsmResults;
	}

EXPORT_C CMobilePhoneGsmSmsList* CRetrieveMobilePhoneSmsList::RetrieveGsmListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneGsmSmsList* Pointer to the GSM SMS message list
 */
	{
	if ((iEntryType==RMobileSmsStore::KETelMobileGsmSmsEntryV1) && iGsmResults)
		{
		// Give ownership of GSM SMS list to caller of this method
		CMobilePhoneGsmSmsList* ptr=iGsmResults;
		iGsmResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneSmsList::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of all SMS messages into a list
 */
	{
	InitiateRequest(aReqStatus,EFalse,KValueNotUsed,KValueNotUsed);
	}
	
EXPORT_C void CRetrieveMobilePhoneSmsList::StartBatch(TRequestStatus& aReqStatus, TInt aStartIndex, TInt aBatchSize)
/**
 * This method starts the retrieval of a batch of SMS messages into a list
 *
 * \param aStartIndex Specifies the first index of the batch
 * \param aBatchSize Specifies how many entries to read after the start
 */
	{
	InitiateRequest(aReqStatus,ETrue,aStartIndex,aBatchSize);
	}

void CRetrieveMobilePhoneSmsList::InitiateRequest(TRequestStatus& aReqStatus, TBool aBatch, TInt aStartIndex, TInt aBatchSize)
/**
 * This method uses the base class's generic Start() method to start either reading 
 * the whole list or just a batch of entries
 *
 * \param aBatch Specifies whether a batch or all entries is being read
 * \param aStartIndex Specifies the first index of the batch
 * \param aBatchSize Specifies how many entries to read after the start
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iGsmResults;
		iGsmResults = NULL;
		iRequestData().iClient.iSessionHandle = iId().iSessionHandle;
		iRequestData().iClient.iSubSessionHandle = iId().iSubSessionHandle;
		iRequestData().iEntryType = iEntryType;
		iRequestData().iBatchRequest = aBatch;
		iRequestData().iStartIndex = aStartIndex;
		iRequestData().iBatchSize = aBatchSize;

		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iRequestData,&iId);
		}
	}

void CRetrieveMobilePhoneSmsList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	if (iEntryType==RMobileSmsStore::KETelMobileGsmSmsEntryV1)
		{
		iGsmResults=CMobilePhoneGsmSmsList::NewL();
		iGsmResults->RestoreL(iResultsPtr);
		}
	FreeBuffer();
	}

/***********************************************************************************/
//
// Retrieve a list of SMS parameters from a phone-side SMSP store
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneSmspList* CRetrieveMobilePhoneSmspList::NewL(RMobileSmsMessaging& aMessaging)
	{
	CRetrieveMobilePhoneSmspList* r=new(ELeave) CRetrieveMobilePhoneSmspList(aMessaging);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneSmspList::CRetrieveMobilePhoneSmspList(RMobileSmsMessaging& aMessaging) 
:	CAsyncRetrieveWithClientIds(aMessaging.SessionHandle().Handle(), aMessaging.SubSessionHandle()),
	iMessaging(aMessaging)
	{
	}

void CRetrieveMobilePhoneSmspList::ConstructL()
	{
	iIpcPhase1=EMobileSmsMessagingGetSmspListPhase1;
	iIpcPhase2=EMobileSmsMessagingGetSmspListPhase2;
	iIpcCancel=EMobileSmsMessagingGetSmspListCancel;
	}

EXPORT_C CRetrieveMobilePhoneSmspList::~CRetrieveMobilePhoneSmspList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneSmspList* CRetrieveMobilePhoneSmspList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneSmspList* Pointer to the SMS parameter list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneSmspList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}
		
void CRetrieveMobilePhoneSmspList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneSmspList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

EXPORT_C void CRetrieveMobilePhoneSmspList::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of the SMS parameters list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iId,&iId);
		}
	}

void CRetrieveMobilePhoneSmspList::Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2)
/**
 * This method uses the Get IPC request within RTelSubsessionBase
 * to send one of the asynchronous phase requests to ETel server
 *
 * \param aIpc The IPC value of the asynchronous phase request
 * \param aReqStatus The request status for the asynchronous phase request
 * \param aDes1 The first parameter for the asynchronous phase request
 * \param aDes2 The second parameter for the asynchronous phase request
 */
	{
	iMessaging.Get(aIpc,aReqStatus,aDes1,aDes2);
	}

void CRetrieveMobilePhoneSmspList::CancelReq(TInt aIpc1, TInt aIpc2)
/**
 * This method uses the Cancel IPC request within RTelSubsessionBase
 * to send a cancel for one of the asynchronous phases to ETel server
 *
 * \param aIpc1 The IPC value of the cancel request
 * \param aIpc2 The IPC value of the original asynchronous phase request
 */
	{
	iMessaging.CancelReq(aIpc1,aIpc2);
	}

/***********************************************************************************/
//
// Retrieve a list of Broadcast ID entries from the SIM or phone
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneBroadcastIdList* CRetrieveMobilePhoneBroadcastIdList::NewL(RMobileBroadcastMessaging& aMessaging)
	{
	CRetrieveMobilePhoneBroadcastIdList* r=new(ELeave) CRetrieveMobilePhoneBroadcastIdList(aMessaging);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneBroadcastIdList::CRetrieveMobilePhoneBroadcastIdList(RMobileBroadcastMessaging& aMessaging) : 
	CAsyncRetrieveWithClientIds(aMessaging.SessionHandle().Handle(), aMessaging.SubSessionHandle()),
	iMessaging(aMessaging)
	{
	}

void CRetrieveMobilePhoneBroadcastIdList::ConstructL()
	{
	iIpcPhase1=EMobileBroadcastMessagingGetIdListPhase1;
	iIpcPhase2=EMobileBroadcastMessagingGetIdListPhase2;
	iIpcCancel=EMobileBroadcastMessagingGetIdListCancel;
	}

EXPORT_C CRetrieveMobilePhoneBroadcastIdList::~CRetrieveMobilePhoneBroadcastIdList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneBroadcastIdList* CRetrieveMobilePhoneBroadcastIdList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneBroadcastIdList* Pointer to the BroadcastId filter list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneBroadcastIdList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}

EXPORT_C void CRetrieveMobilePhoneBroadcastIdList::Start(TRequestStatus& aReqStatus, RMobileBroadcastMessaging::TMobileBroadcastIdType aIdType)
/**
 * This method starts the retrieval of the BroadcastId filter list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		iBroadcastIdRequestData().iIdType=aIdType;
		iBroadcastIdRequestData().iClient=iId();
	
		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iBroadcastIdRequestData,&iId);
		}
	}
		
void CRetrieveMobilePhoneBroadcastIdList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneBroadcastIdList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

void CRetrieveMobilePhoneBroadcastIdList::Get(TInt aIpc, TRequestStatus& aReqStatus, TDes8& aDes1, TDes8& aDes2)
/**
 * This method uses the Get IPC request within RTelSubsessionBase
 * to send one of the asynchronous phase requests to ETel server
 *
 * \param aIpc The IPC value of the asynchronous phase request
 * \param aReqStatus The request status for the asynchronous phase request
 * \param aDes1 The first parameter for the asynchronous phase request
 * \param aDes2 The second parameter for the asynchronous phase request
 */
	{
	iMessaging.Get(aIpc,aReqStatus,aDes1,aDes2);
	}

void CRetrieveMobilePhoneBroadcastIdList::CancelReq(TInt aIpc1, TInt aIpc2)
/**
 * This method uses the Cancel IPC request within RTelSubsessionBase
 * to send a cancel for one of the asynchronous phases to ETel server
 *
 * \param aIpc1 The IPC value of the cancel request
 * \param aIpc2 The IPC value of the original asynchronous phase request
 */
	{
	iMessaging.CancelReq(aIpc1,aIpc2);
	}

/***********************************************************************************/
//
// Retrieve a list of entries from a phone-side NAM store
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneNamList* CRetrieveMobilePhoneNamList::NewL(RMobilePhoneStore& aStore)
	{
	CRetrieveMobilePhoneNamList* r=new(ELeave) CRetrieveMobilePhoneNamList(aStore);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneNamList::CRetrieveMobilePhoneNamList(RMobilePhoneStore& aStore) : 
	CAsyncRetrieveStoreList(aStore)
	{
	}

void CRetrieveMobilePhoneNamList::ConstructL()
	{
	iRequestData().iClient.iSessionHandle = iId().iSessionHandle;
	iRequestData().iClient.iSubSessionHandle = iId().iSubSessionHandle;
	}

EXPORT_C CRetrieveMobilePhoneNamList::~CRetrieveMobilePhoneNamList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneNamList* CRetrieveMobilePhoneNamList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneNamList* Pointer to the NAM contents list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneNamList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}
		
void CRetrieveMobilePhoneNamList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneNamList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

EXPORT_C void CRetrieveMobilePhoneNamList::Start(TRequestStatus& aReqStatus, TInt aNamId)
/**
 * This method starts the retrieval of the NAM contents list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		iRequestData().iNamId = aNamId;
		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iRequestData,&iId);
		}
	}


/***********************************************************************************/
//
// Retrieve a list of entries from a phone-side Own Number store
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneONList* CRetrieveMobilePhoneONList::NewL(RMobilePhoneStore& aStore)
	{
	CRetrieveMobilePhoneONList* r=new(ELeave) CRetrieveMobilePhoneONList(aStore);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneONList::CRetrieveMobilePhoneONList(RMobilePhoneStore& aStore) : 
	CAsyncRetrieveStoreList(aStore)
	{
	}

void CRetrieveMobilePhoneONList::ConstructL()
	{
	}

EXPORT_C CRetrieveMobilePhoneONList::~CRetrieveMobilePhoneONList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneONList* CRetrieveMobilePhoneONList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneONList* Pointer to the Own Numbers list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneONList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}
		
void CRetrieveMobilePhoneONList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneONList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

EXPORT_C void CRetrieveMobilePhoneONList::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of the Own Numbers list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iId,&iId);
		}
	}

/***********************************************************************************/
//
// Retrieve a list of entries from a phone-side Emergency Number store
//
/***********************************************************************************/

EXPORT_C CRetrieveMobilePhoneENList* CRetrieveMobilePhoneENList::NewL(RMobilePhoneStore& aStore)
	{
	CRetrieveMobilePhoneENList* r=new(ELeave) CRetrieveMobilePhoneENList(aStore);
	CleanupStack::PushL(r);
	r->ConstructL();
	CleanupStack::Pop();
	return r;
	}	

CRetrieveMobilePhoneENList::CRetrieveMobilePhoneENList(RMobilePhoneStore& aStore) : 
	CAsyncRetrieveStoreList(aStore)
	{
	}

void CRetrieveMobilePhoneENList::ConstructL()
	{
	}

EXPORT_C CRetrieveMobilePhoneENList::~CRetrieveMobilePhoneENList()
	{
	delete iResults;
	}

EXPORT_C CMobilePhoneENList* CRetrieveMobilePhoneENList::RetrieveListL()
/**
 * This method returns a pointer to the retrieved list
 *
 * \return CMobilePhoneENList* Pointer to the Emergency Numbers list
 */
	{
	if (iResults)
		{
		// Give ownership of list to caller of this method
		CMobilePhoneENList* ptr=iResults;
		iResults=NULL;
		return ptr;
		}
	else
		{
		User::Leave(KErrNotFound);
		return NULL;
		}
	}
		
void CRetrieveMobilePhoneENList::RestoreListL()
/**
 * This method restores a list from a buffer that contains the streamed version of the list
 */
	{
	iResults=CMobilePhoneENList::NewL();
	iResults->RestoreL(iResultsPtr);
	FreeBuffer();
	}

EXPORT_C void CRetrieveMobilePhoneENList::Start(TRequestStatus& aReqStatus)
/**
 * This method starts the retrieval of the Emergency Numbers list
 */
	{
	if (!CompleteIfInUse(aReqStatus))
		{
		delete iResults;
		iResults = NULL;

		CAsyncRetrieveVariableLengthBuffer::Start(aReqStatus,&iId,&iId);
		}
	}


