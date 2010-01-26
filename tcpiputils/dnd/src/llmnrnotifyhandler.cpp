// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// llmnrnotifyhandler.cpp - Link-local multicast name resolution 
// notify handler
// CDndLlmnrNotifyHandler
//

#ifdef LLMNR_ENABLED

#if defined(TCPIP6_USE_COMMDB)
#	include <commdb.h>
#else
#	include <commsdattypesv1_1.h>
#       include <commsdattypesv1_1_partner.h>
	using namespace CommsDat;
#endif

#include "engine.h"
#include "llmnrresponder.h"
#include "inet6log.h"

CDndLlmnrNotifyHandler::CDndLlmnrNotifyHandler(CDndLlmnrResponder &aMaster)
 : iTimeout(CLlmnrNotifyHandlerTimeoutLinkage::Timeout), iMaster(aMaster)
 	{
 	}

CDndLlmnrNotifyHandler::~CDndLlmnrNotifyHandler()
	{
	iTimeout.Cancel();
	}

void CDndLlmnrNotifyHandler::ConstructL()
	{
	LOG(Log::Printf(_L("CDndLlmnrNotifyHandler::ConstructL() size=%d\r\n"), (TInt)sizeof(*this)));
	}

void CDndLlmnrNotifyHandler::ConfigurationChanged()
	{
	iMaster.iControl.Timer().Set(iTimeout, iMaster.iLlmnrConf->iNotifyTime);
	}

void CDndLlmnrNotifyHandler::ScanInterfaces()
	{
	(void)iMaster.iControl.CheckResult
		(_L("Setting Socket options"),
		 iMaster.iControl.iSocket.SetOpt(KSoInetEnumInterfaces,
		 KSolInetIfCtrl));

	TSoInetInterfaceInfo *info = new TSoInetInterfaceInfo; // allocate large struct from heap!
	if (info == NULL)
		return; // No memory!
	TPckg<TSoInetInterfaceInfo> opt(*info);

	TPckgBuf<TSoInetIfQuery> opt2;
	opt2().iName.SetLength(0);

	TUint32 iap_id = 0;			// Current IAP ID (== iZone[1])
	TInt llmnr_disabled = 0;	// (init just to silence warning! this value is never used).

	iMaster.UpdateStart();
	while(iMaster.iControl.iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		// Check address configured
		if(opt().iAddress.IsUnspecified())
			continue; // No address, ignore
		// KSoInetNextInterface returns an entry for each configured
		// address. Skip entry, if previous entry was the same real
		// interface. [This is only optimization that lessens the
		// generated log messages--not for speed].
		if (opt().iName.Compare(opt2().iName) == 0)
			continue;
		opt2().iName = opt().iName;
		TInt err = iMaster.iControl.iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, opt2);
		if(err != KErrNone)
			{
			LOG(Log::Printf(_L("CDndLlmnrNotifyHandler::ScanInterfaces GetOpt KSoInetIfQueryByName error: %d"),err));
			}
		else
			{
			LOG(Log::Printf(_L("CDndLlmnrNotifyHandler::ScanInterfaces - Interface up, name: %S\r\n"), &opt().iName));

			if (iap_id != opt2().iZone[1])
				{
				// Different IAP, need to find whether LLMNR is to be enabled or disabled
				iap_id = opt2().iZone[1];
				TRAPD(err, llmnr_disabled = IsLlmnrDisabledL(iap_id));
				if (err != KErrNone)
					llmnr_disabled = 1;	// By default (if no configuration present), disable it.
				}
			TIpVer ipver;
			if(opt().iAddress.Family() == KAfInet || opt().iAddress.IsV4Mapped())
				ipver = EIPv4;
			else
				ipver = EIPv6;
			iMaster.UpdateInterface(opt2().iName, ipver, opt2().iZone, opt().iHwAddr, llmnr_disabled);
			}
		}
	delete info;
	iMaster.UpdateFinish();
	}

void CDndLlmnrNotifyHandler::Timeout(const TTime &)
	{
	LOG(Log::Printf(_L("--> CDndLlmnrNotifyHandler::Timeout() -start-")));
	ScanInterfaces();
	LOG(Log::Printf(_L("<-- CDndLlmnrNotifyHandler::Timeout() -exit-")));
	}



/**
* Acquire LLMNR configuration flag for the IAP from the CommDB.
*
* @return
*	@li 0, if configuration not found or LLMNR is enabled by the configuration
*	@li 1, if LLMNR is disabled by the configuration.
*
* Leave is implicit disable.
*/
TInt CDndLlmnrNotifyHandler::IsLlmnrDisabledL(TUint32 aIap)
	{
	TBool enable = 1;	// If CommsDB does not have SERVICE_ENABLE_LLMNR, the default is enabled!
#if defined(TCPIP6_USE_COMMDB)
#	ifdef SERVICE_ENABLE_LLMNR
	CCommsDatabase* db = CCommsDatabase::NewL();
	CleanupStack::PushL(db);	// cleanup 1

	CCommsDbTableView* iap = db->OpenViewMatchingUintLC(TPtrC(IAP), TPtrC(COMMDB_ID), aIap);	// cleanup 2
	User::LeaveIfError(iap->GotoFirstRecord());
	TBuf<KCommsDbSvrMaxFieldLength> service_type;
	TUint32 service;
	iap->ReadTextL(TPtrC(IAP_SERVICE_TYPE), service_type);
	iap->ReadUintL(TPtrC(IAP_SERVICE), service);
	CCommsDbTableView *srv = db->OpenViewMatchingUintLC(service_type, TPtrC(COMMDB_ID), service);	// cleanup 3
	User::LeaveIfError(srv->GotoFirstRecord());
	srv->ReadBoolL(TPtrC(SERVICE_ENABLE_LLMNR), enable);
	CleanupStack::PopAndDestroy(3);
#	else
	(void)aIap;
#	endif
#else
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession *dB = CMDBSession::NewLC(KCDVersion1_2);
#else
	CMDBSession *dB = CMDBSession::NewLC(KCDVersion1_1);
#endif

	
	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	dB->SetAttributeMask( ECDHidden | ECDPrivate );

	CCDIAPRecord* iap = static_cast<CCDIAPRecord *>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(iap);
	iap->SetRecordId(aIap);
	LOG(Log::Printf(_L("IsLlmnrDisabledL: LoadL for IAP=%d"), aIap));
	iap->LoadL(*dB);
	LOG(Log::Printf(_L("IsLlmnrDisabledL: LoadL OK, doing LoadL for service")));
#if 0
	iap->iService.LoadL(*dB);
#else
	const TDesC& servType = iap->iServiceType;

	if (servType.CompareF(TPtrC(KCDTypeNameDialOutISP))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDDialOutISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialOutISPRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameDialInISP))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDDialInISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialInISPRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameWLANServiceExt))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDWLANServiceExtRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWLANServiceExtRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameOutgoingWCDMA))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDOutgoingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdOutgoingGprsRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameIncomingWCDMA))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDIncomingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIncomingGprsRecord));
		}
	else if (servType.CompareF(TPtrC(KCDTypeNameDefaultWCDMA))==0)
		{
		iap->iService.iLinkedRecord = static_cast<CCDDefaultWCDMARecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDefaultWCDMARecord));
		}
	else
		{
		LOG(Log::Printf(_L("CDndLlmnrNotifyHandler::IsLlmnrDisabledL() service type not supported (%S)\r\n"), &servType));
		goto fail;
		}
	iap->iService.iLinkedRecord->SetRecordId(iap->iService);
	iap->iService.iLinkedRecord->LoadL(*dB);
#endif
fail:
	CCDServiceRecordBase *const ptrService = static_cast<CCDServiceRecordBase *>(iap->iService.iLinkedRecord);
	LOG(Log::Printf(_L("IsLlmnrDisabledL: LoadL for service returns=[%u]"), (TInt)ptrService));
	if (ptrService)
		enable = ptrService->iServiceEnableLlmnr;
	else
		enable = 0;
	CleanupStack::PopAndDestroy(2);		
#endif
	LOG(Log::Printf(_L("IsLlmnrDisabledL: enable=%d"), enable));
	return enable == 0;
	}


#endif
