// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements client hardware address fetching.
// 
//

/**
 @file
 @internalTechnology
*/
#include "naptutil.h"

/** 
  * Fucntion to fetch the IP from CommsDb, and Traps any leave.
  * @internalTechnology
  * @param aIapId - IAP identifier
  * @param aClientIp - Returns the fetched IP address 
  */
  
TInt TNaptUtil::GetClientIp(TInt aIapId, TUint32& aClientIp)
{
	TRAPD(err, GetClientIpFromCommsL(aIapId, aClientIp));
	return err;
}
  

/** 
  * Open the IAP record
  * @internalTechnology
  * @param aDbSession - Commsdb session
  * @param aIapRecord - IAP record
  * @param aIapId - IAP identifier
  */
void TNaptUtil::OpenIAPViewLC(CMDBSession*& aSession, CCDIAPRecord*& aIapRecord, TInt aIapId)
	{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	aSession = CMDBSession::NewLC(KCDVersion1_2);
#else
	aSession = CMDBSession::NewLC(KCDVersion1_1);
#endif

	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	aSession->SetAttributeMask(ECDHidden | ECDPrivate);
	
	aIapRecord = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(aIapRecord);

    aIapRecord->SetRecordId(aIapId);
        	
	aIapRecord->LoadL(*aSession);
	}
/** 
  * Initialize the service link
  * @internalTechnology
  * @param aDbSession - Commsdb session
  * @param aIapRecord - IAP record
  */
void TNaptUtil::InitialServiceLinkL(CMDBSession* aDbSession, CCDIAPRecord* aIapRecord)
	{
	if (aIapRecord->iService.iLinkedRecord == 0)
		{
		const TDesC& servType = aIapRecord->iServiceType;

		if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
			{
			aIapRecord->iService.iLinkedRecord = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
			}
		else
			{
			User::Leave(KErrBadName);	
			}
		aIapRecord->iService.iLinkedRecord->SetRecordId(aIapRecord->iService);
		}
	aIapRecord->iService.iLinkedRecord->LoadL(*aDbSession);
	}
/** 
  * Fetch the server IP address from comms and generate client ID
  * @internalTechnology
  * @param aIapId - IAP identifier
  * @param aClientIp - Returns the fetched IP address 
  */
void TNaptUtil::GetClientIpFromCommsL(TInt aIapId, TUint32& aClientIp)
{
	CMDBSession* session;
	CCDIAPRecord* iap;

	// next call leaves session and iap object on cleanupstack
	OpenIAPViewLC(session, iap, aIapId);
	CCDServiceRecordBase* service = NULL;
	InitialServiceLinkL(session, iap);
	service = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);
	TInt ignoreThis;

	CMDBField<TDesC>* cdbLoadBool = static_cast<CMDBField<TDesC>*>(service->GetFieldByNameL(KCDTypeNameIpAddr, ignoreThis));
	TInetAddr inetAddr;
	inetAddr.Input(*cdbLoadBool);
	TUint32 serverAddr = inetAddr.Address();
	TUint32 hostId = (serverAddr & ~KInetAddrNetMaskC) + 1;
	if (hostId >= 255)
	 {
	 hostId = 1;
	 }
	aClientIp = (serverAddr & KInetAddrNetMaskC) | hostId;
	service = NULL;
	CleanupStack::PopAndDestroy(iap);
	CleanupStack::PopAndDestroy(session);
}
