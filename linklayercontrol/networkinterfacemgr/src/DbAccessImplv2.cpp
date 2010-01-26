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
// All comms database access is performed through a CCommsDatabase object
// 
//

/**
 @file
*/

#include <nifvar.h>
#include <comms-infras/nifprvar.h>
#include <commdb.h>
#include "DbAccess.h"
#include "NetConError.h"
#include "AgentPanic.h"
#include "Ni_Log.h"
#include "DbAccessImplv2.h"
#include <commsdat.h>
#include <commsdatutils.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <commsdattypesv1_1_partner.h>
#endif

/**
Label for retrieving active isp service type
@internalComponent
*/
#define SERVICE_TYPE									_S("ServiceType")

/**
@internalComponent
*/
namespace
	{
	_LIT(KPPPModemCsyName,"ModemBearer\\CSYName");
	_LIT(KPPPModemPortName,"ModemBearer\\PortName");

	const TUint32 KCommsDbMaxConnections = 2;
	}

//
// CCommsDatAccessImpl definitions
//

CCommsDatAccessImpl* CCommsDatAccessImpl::NewL()
  	{
	return CCommsDatAccessImpl::NewL( EFalse );
 	}

CCommsDatAccessImpl* CCommsDatAccessImpl::NewL(TBool aShowHidden)
  	{
	CCommsDatAccessImpl* p = new(ELeave) CCommsDatAccessImpl();
  	CleanupStack::PushL(p);
  	p->ConstructL(aShowHidden);
  	CleanupStack::Pop();
  	return p;
 	}

CCommsDatAccessImpl::CCommsDatAccessImpl():
	iNetworkMode(RMobilePhone::ENetworkModeUnknown)	
	{		
	}

void CCommsDatAccessImpl::ConstructL(TBool& aShowHidden)
	{
	iDb = CMDBSession::NewL(KCDVersion1_2);
	if(aShowHidden)
		{
		// Reveal hidden or private IAP records if a licensee has chosen to protect a record
		// using one of these flags - the API to do this is public so internal components
		// have to support the use of such records.
		iDb->SetAttributeMask(ECDHidden | ECDPrivate); 
		}
		
	//create a container for the iap record	
	iIAPSetting = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	}

CCommsDatAccessImpl::~CCommsDatAccessImpl()
	{
	Close();
	delete iDb;

	// NOTE! Do not delete iOverrides and iServiceChangeObserver as
	// they are owned by other objects and they are just used
	// throught the pointers in the DbAccess.cpp!
	}

void CCommsDatAccessImpl::Close()
	{
	if (iIAPSetting != NULL)
		{
		delete iIAPSetting->iService.iLinkedRecord;
		iIAPSetting->iService.iLinkedRecord = NULL;
		delete iIAPSetting->iBearer.iLinkedRecord;
		iIAPSetting->iBearer.iLinkedRecord = NULL;
		delete iIAPSetting->iLocation.iLinkedRecord;
		iIAPSetting->iLocation.iLinkedRecord = NULL;
		delete iIAPSetting->iNetwork.iLinkedRecord;
		iIAPSetting->iNetwork.iLinkedRecord = NULL;
		}
	delete iIAPSetting;
	iIAPSetting = NULL;
	delete iChargecardSetting;
	iChargecardSetting = NULL;
	delete iLanServiceExtensionTable;
	iLanServiceExtensionTable = NULL;
	iGotSettings=EFalse;
	}

void CCommsDatAccessImpl::SetOverridesL(CCommDbOverrideSettings* aOverrides)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetOverridesL()"));)
	iOverrides = aOverrides;
	}

TBool CCommsDatAccessImpl::IsShowingHiddenRecords()
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::IsShowingHiddenRecords()"));)

	TMDBAttributeFlags attributeMask = iIAPSetting->Attributes();
	
	if(attributeMask & ECDHidden)
		{
		return ETrue;
		}
	else
		{
		return EFalse;
		}	
	}

void CCommsDatAccessImpl::CreateCacheL()
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CreateCacheL()"));)

	// CreateCacheL loads the linked records only on condition that they are equal to NULL
	// This causes a wrong link when the iIAPSetting object's fields are assigned new values
	// The following section of deletes ensures that the corrwct links will be created in a case of reload.
		
	const TDesC& servType = iIAPSetting->iServiceType;
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CreateCacheL() service type = \"%S\""), &servType);)

	delete iIAPSetting->iService.iLinkedRecord;
	iIAPSetting->iService.iLinkedRecord = NULL;	
	delete iIAPSetting->iBearer.iLinkedRecord;
	iIAPSetting->iBearer.iLinkedRecord = NULL;
	delete iIAPSetting->iLocation.iLinkedRecord;
	iIAPSetting->iLocation.iLinkedRecord = NULL;
	delete iIAPSetting->iNetwork.iLinkedRecord;
	iIAPSetting->iNetwork.iLinkedRecord = NULL;
		
	// service
	if (iIAPSetting->iService.iLinkedRecord == 0)
		{

		if (servType.CompareF(TPtrC(KCDTypeNameDialOutISP))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDDialOutISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialOutISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDialInISP))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDDialInISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialInISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameOutgoingWCDMA))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDOutgoingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdOutgoingGprsRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameIncomingWCDMA))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDIncomingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIncomingGprsRecord));
			}
		/*else if (servType.CompareF(TPtrC(WHAT IS THIS NAME?))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDWCDMAPacketServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWCDMAPacketServiceRecord));
			}*/
		else if (servType.CompareF(TPtrC(KCDTypeNameDefaultWCDMA))==0)
			{
			iIAPSetting->iService.iLinkedRecord = static_cast<CCDDefaultWCDMARecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDefaultWCDMARecord));
			}
		else
			{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CreateCacheL() service type does not match any supported type")));
			User::Leave(KErrCorrupt);
			}
		
		iIAPSetting->iService.iLinkedRecord->SetRecordId(iIAPSetting->iService);
		iIAPSetting->iService.iLinkedRecord->LoadL(*iDb);
	
		if (servType.CompareF(TPtrC(KCDTypeNameOutgoingWCDMA)) == 0)
			{
			// umts R99 and onwards table
			CCDOutgoingGprsRecord* outgoingGprs = static_cast<CCDOutgoingGprsRecord*>(iIAPSetting->iService.iLinkedRecord);
			if (outgoingGprs->iUmtsR99QoSAndOnTable.iLinkedRecord == 0 && outgoingGprs->iUmtsR99QoSAndOnTable != 0 )
				{
				outgoingGprs->iUmtsR99QoSAndOnTable.iLinkedRecord = static_cast<CCDUmtsR99QoSAndOnTableRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdUmtsR99QoSAndOnTableRecord));
				outgoingGprs->iUmtsR99QoSAndOnTable.iLinkedRecord->SetRecordId(outgoingGprs->iUmtsR99QoSAndOnTable);
				outgoingGprs->iUmtsR99QoSAndOnTable.iLinkedRecord->LoadL (*iDb);
				}
			}
		}

	// bearer
	if (iIAPSetting->iBearer.iLinkedRecord == 0)
		{
		const TDesC& bearType = iIAPSetting->iBearerType;
		LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CreateCacheL() bearer type = \"%S\""), &bearType);)

		if (bearType.CompareF(TPtrC(KCDTypeNameModemBearer))==0)
			{
			iIAPSetting->iBearer.iLinkedRecord = static_cast<CCDModemBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdModemBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameLANBearer))==0)
			{
			iIAPSetting->iBearer.iLinkedRecord = static_cast<CCDLANBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameVirtualBearer))==0)
			{
			iIAPSetting->iBearer.iLinkedRecord = static_cast<CCDVirtualBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVirtualBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameWAPSMSBearer))==0)
			{
			iIAPSetting->iBearer.iLinkedRecord = static_cast<CCDWAPSMSBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWAPSMSBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameWAPIPBearer))==0)
			{
			iIAPSetting->iBearer.iLinkedRecord = static_cast<CCDWAPIPBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWAPIPBearerRecord));
			}
		else
			{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CreateCacheL() bearer type does not match any supported type")));
			User::Leave(KErrCorrupt);
			}
		iIAPSetting->iBearer.iLinkedRecord->SetRecordId(iIAPSetting->iBearer);
		}

	// location
	if (iIAPSetting->iLocation.iLinkedRecord == 0)
		{
		iIAPSetting->iLocation.iLinkedRecord = static_cast<CCDLocationRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLocationRecord));
		iIAPSetting->iLocation.iLinkedRecord->SetRecordId(iIAPSetting->iLocation);
		}

	// network
	if (iIAPSetting->iNetwork.iLinkedRecord == 0)
		{
		iIAPSetting->iNetwork.iLinkedRecord = static_cast<CCDNetworkRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdNetworkRecord));
		iIAPSetting->iNetwork.iLinkedRecord->SetRecordId(iIAPSetting->iNetwork);
		}

	// load everything from the database
	iIAPSetting->iBearer.iLinkedRecord->LoadL(*iDb);
	iIAPSetting->iLocation.iLinkedRecord->LoadL(*iDb);
	iIAPSetting->iNetwork.iLinkedRecord->LoadL(*iDb);
	}
	
	
void CCommsDatAccessImpl::GetCurrentSettingsL(TConnectionSettings& aSettings, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Get default settings from overrides or database.
No checking that the default exists yet -
that's done in the SetCurrentSettingsL() function.
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetCurrentSettingsL()"));)

	aSettings.iRank = aRank;
	aSettings.iDirection = aDirection;

	// Identify IAP associated with connection preference with ranking 1
	// and open IAP record in a table view

	//this is a workaround for the missing preference table entries (SNAP selection)
	TRAPD(err,GetIapConnectionPreferenceL(aSettings));
	if ((err==KErrNetConDatabaseDefaultUndefined || err==KErrNetConDatabaseNotFound) && aRank>1)
	    err=KErrNone;
    User::LeaveIfError(err);		
		
	iIAPSetting->SetRecordId(aSettings.iIAPId);
  	iIAPSetting->LoadL(*iDb);	
	CreateCacheL();

	aSettings.iBearerType = iIAPSetting->iBearerType;
	aSettings.iBearerId = iIAPSetting->iBearer;
	aSettings.iLocationId = iIAPSetting->iLocation;

	iGotSettings=ETrue;	
	}

void CCommsDatAccessImpl::GetServiceSettingsL(TConnectionSettings& aSettings)
/**
Get service settings from database.
No checking that the default exists yet -
that's done in the SetCurrentSettingsL() function.
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetServiceSettingsL()"));)
	
	aSettings.iServiceId = iIAPSetting->iService;	
	aSettings.iServiceType = iIAPSetting->iServiceType;
	iIAPSetting->iService.iLinkedRecord->RefreshL(*iDb);
	
	
	// If this is a CSD connection get the chargecard ID
	if((aSettings.iServiceType).CompareF(TPtrC(DIAL_OUT_ISP))==0)//defect || ((aSettings.iServiceType).CompareF(TPtrC(DIAL_IN_ISP))==0))
		{  
		aSettings.iChargeCardId = ((CCDDialOutISPRecord*)(iIAPSetting->iService.iLinkedRecord))->iChargecard;
		if(aSettings.iChargeCardId)
			{
			// iChargecardSetting wasn't created on startup so create/recreate here
 			delete iChargecardSetting;
  			iChargecardSetting = 0;					
			iChargecardSetting = static_cast<CCDChargecardRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdChargecardRecord));
			
			iChargecardSetting->SetRecordId(aSettings.iChargeCardId);
  			iChargecardSetting->LoadL(*iDb);	
			}		
		}
	else
		{
  		// If this is a LAN-type connection, check to see whether there's an extension table (ie. it's a WLAN or BT PAN connection)
  		if((aSettings.iServiceType).CompareF(TPtrC(LAN_SERVICE))==0)
  			{
  			delete iLanServiceExtensionTable;
			CCDLANServiceRecord* lanRecord = static_cast<CCDLANServiceRecord*>(iIAPSetting->iService.iLinkedRecord);
  			const TDesC& extName = lanRecord->iServiceExtensionTableName;

			if(extName.CompareF(TPtrC(KCDTypeNamePANServiceExt)) == 0)
				{
				iLanServiceExtensionTable = static_cast<CCDPANServiceExtRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdPANServiceExtRecord));
				iLanServiceExtensionTable->SetRecordId(lanRecord->iServiceExtensionTableRecordId);
				iLanServiceExtensionTable->LoadL(*iDb);
				}
			else if(extName.CompareF(TPtrC(KCDTypeNameWLANServiceExt))==0)
				{
				iLanServiceExtensionTable = static_cast<CCDWLANServiceExtRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWLANServiceExtRecord));
				iLanServiceExtensionTable->SetRecordId(lanRecord->iServiceExtensionTableRecordId);
				iLanServiceExtensionTable->LoadL(*iDb);
				}
			// else no extension table to initialize
			}
		}
		
		if(LocationRequiredL())
			{
			if (iIAPSetting->iLocation.iLinkedRecord == NULL)
				{
				iIAPSetting->iLocation.iLinkedRecord = static_cast<CCDLocationRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLocationRecord));
				}

			iIAPSetting->iLocation.iLinkedRecord->SetRecordId(aSettings.iLocationId);
			
			if (!iIAPSetting->iLocation.iLinkedRecord->FindL(*iDb))
				{
				User::Leave(KErrNotFound);
				}
			}
	}

void CCommsDatAccessImpl::GetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Get IapId for the connection preference of ranking aRank.
Leaves if not found
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetPreferedIapL()"));)

	CCDConnectionPrefsRecord* ptrConnectionPref = static_cast<CCDConnectionPrefsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdConnectionPrefsRecord));
	CleanupStack::PushL(ptrConnectionPref);

	ptrConnectionPref->iRanking = aRank;
	ptrConnectionPref->iDirection = aDirection;
	if (!ptrConnectionPref->FindL(*iDb))
		{
		User::Leave(KErrNotFound);
		}
	
	aIapId = ptrConnectionPref->iDefaultIAP;

	CleanupStack::PopAndDestroy(ptrConnectionPref);
	}

void CCommsDatAccessImpl::GetIapConnectionPreferenceL(TConnectionSettings& aSettings)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIapConnectionPreferenceL(TConnectionSettings& aSettings)"));)

	// set to defaults before we start
	aSettings.iDialogPref = ECommDbDialogPrefUnknown;
	aSettings.iBearerSet = 0;
	aSettings.iIAPId = 0;


	iDirection = aSettings.iDirection;

	// if we already have overrides stored here then use them
	if (iOverrides)
		{
		CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref overPref;
		overPref.iRanking = aSettings.iRank;
		overPref.iDirection = aSettings.iDirection;

		iOverrides->GetConnectionPreferenceOverride(overPref);
		// write them back into the parameter we are to fill with data
		aSettings.iDialogPref = overPref.iDialogPref;
		aSettings.iBearerSet = overPref.iBearer.iBearerSet;
		aSettings.iIAPId = overPref.iBearer.iIapId;
		
		if (aSettings.iDialogPref != ECommDbDialogPrefPrompt)
			{
			iIAPOverridden = ETrue;
			}
		else
			{
			iIAPOverridden = EFalse;
			}
		}

	// if the overrides are not complete then we need to read the remaining settings from the database table
	TBool overridesComplete = (aSettings.iDialogPref != ECommDbDialogPrefUnknown) && (aSettings.iBearerSet != 0) && (aSettings.iIAPId != 0);
	if (!overridesComplete)
		{
		// create the connection preference, prefill it with stuff and then read from the database
		CCDConnectionPrefsRecord* connpref = static_cast<CCDConnectionPrefsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdConnectionPrefsRecord));
		CleanupStack::PushL(connpref);

		connpref->iRanking = aSettings.iRank;
		connpref->iDirection = aSettings.iDirection;
		if(!connpref->FindL(*iDb))
			{
			User::Leave(KErrNetConDatabaseDefaultUndefined);
			}

		// now for each field which is currently unset overwrite it with whatever was in the appropriate table in the database

		if (aSettings.iDialogPref == ECommDbDialogPrefUnknown)
			{
			aSettings.iDialogPref = static_cast<TCommDbDialogPref>(static_cast<TUint32>(connpref->iDialogPref));
			}

		if (aSettings.iBearerSet == 0)
			{
			aSettings.iBearerSet = connpref->iBearerSet;
			}

		if (aSettings.iIAPId != 0)
			{
			if (aSettings.iDialogPref!=ECommDbDialogPrefPrompt)
				{
				iIAPOverridden = ETrue;
				}
			else
				{
				iIAPOverridden = EFalse;
				}
			}
		else
			{
			iIAPOverridden = EFalse;
			aSettings.iIAPId = connpref->iDefaultIAP;
			}

		CleanupStack::PopAndDestroy(connpref);
		}
	}

void CCommsDatAccessImpl::SetCurrentSettingsL(const TConnectionSettings& aSettings)
/**
Set current settings to aSettings and check that relevant
records exist in the databases. If the settings have been changed
(not overrides) write the new values back to the database.
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetCurrentSettingsL()"));)

 		
	TInt serviceId = iIAPSetting->iService;
		
	//if the IAP is already overriden, just update the existing record
	if(iIAPOverridden)
		{
		iIAPSetting->RefreshL(*iDb);						
		}
	else
		{
		iIAPSetting->SetRecordId(aSettings.iIAPId);
  		iIAPSetting->LoadL(*iDb);
		}		

	CreateCacheL();		
	
	if(serviceId != iIAPSetting->iService)
		{
		CheckForServiceChange(ETrue);
		}
	
	// If this is a CSD connection get the chargecard ID
	if(((aSettings.iServiceType).CompareF(TPtrC(DIAL_OUT_ISP)) == 0) || ((aSettings.iServiceType).CompareF(TPtrC(DIAL_IN_ISP)) == 0))
		{
		// iChargecardSetting wasn't created on startup so create/recreate here
		if(aSettings.iChargeCardId)
			{
 			delete iChargecardSetting;
  			iChargecardSetting = 0;					
			iChargecardSetting = static_cast<CCDChargecardRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdChargecardRecord));
		
			iChargecardSetting->SetRecordId(aSettings.iChargeCardId);
  			iChargecardSetting->LoadL(*iDb);
			}
		}

		if(LocationRequiredL())
			{
			if (iIAPSetting->iLocation.iLinkedRecord == NULL)
				{
				iIAPSetting->iLocation.iLinkedRecord = static_cast<CCDLocationRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLocationRecord));
				}

			iIAPSetting->iLocation.iLinkedRecord->SetRecordId(aSettings.iLocationId);
			
			if (!iIAPSetting->iLocation.iLinkedRecord->FindL(*iDb))
				{
				User::Leave(KErrNotFound);
				}
			}
	}

void CCommsDatAccessImpl::SetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Set current settings to aSettings and check that relevant
records exist in the databases. If the settings have been changed
(not overrides) write the new values back to the database.

*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetPreferedIapL()"));)

	CCDConnectionPrefsRecord* ptrConnectionPref = static_cast<CCDConnectionPrefsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdConnectionPrefsRecord));
	CleanupStack::PushL(ptrConnectionPref);

	ptrConnectionPref->iDirection = aDirection;
	ptrConnectionPref->iRanking = aRank;
	if(!ptrConnectionPref->FindL(*iDb))
		{
		User::Leave(KErrNotFound);
		}

	ptrConnectionPref->iDefaultIAP = aIapId;

	ptrConnectionPref->ModifyL(*iDb);

	CleanupStack::PopAndDestroy(ptrConnectionPref);
	}

TBool CCommsDatAccessImpl::DoesIapExistL(TUint32 aIapId)
/**
Check for the presence of an IAP of record number aIapId
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::DoesIapExistL()"));)

	CCDIAPRecord* ptrIapRecord = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(ptrIapRecord);

	ptrIapRecord->SetRecordId(aIapId);
	TRAPD(ret, ptrIapRecord->LoadL(*iDb));

	CleanupStack::PopAndDestroy(ptrIapRecord);

	if (ret != KErrNone)
		{
		return EFalse;
		}
	else
		{
		return ETrue;
		}
	}

void CCommsDatAccessImpl::GetFirstValidIapL(TUint32& aIapId)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetFirstValidIapL()"));)

	// this will load IAP table.
	CMDBRecordSet<CCDIAPRecord>* iapRS = new(ELeave) CMDBRecordSet<CCDIAPRecord>(KCDTIdIAPRecord);
	CleanupStack::PushL(iapRS);
 
	iapRS->LoadL(*iDb); // will leave if problematic
	// if the first record is not there it would leave line above
	aIapId = iapRS[0].RecordId();
	
	CleanupStack::PopAndDestroy(iapRS);
	}

TBool CCommsDatAccessImpl::LocationRequiredL()
/**
Check whether this config needs location settings (direct connections and
dial in connection do not, nor do GPRS)
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::LocationRequiredL()"));)

	TPtrC boo;
	CCDServiceRecordBase* service = static_cast<CCDServiceRecordBase*>(iIAPSetting->iService.iLinkedRecord);
	boo.Set(service->iRecordName);
	if (boo.CompareF(TPtrC(DIAL_OUT_ISP))==0)
		{
		TDialString telNum;
		telNum = static_cast<CCDDialOutISPRecord*>(iIAPSetting->iService.iLinkedRecord)->iDefaultTelNum;
		if (telNum.Length()==0)			// direct connection
			{
			return EFalse;
			}
		else
			{
			return ETrue;	
			}
		}
	return EFalse;
	}

TBool CCommsDatAccessImpl::IsTelNumLengthZeroForRasConnectionL(TConnectionSettings& aSettings)
/**
Check whether this connection is RAS connection. Default telephone
number length will be zero if it is, as RAS does not need to dial up.
Direct connection is part of CsdAgx and that's why we need to check
if the service setting is DIAL_OUT_ISP
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::IsTelNumLengthZeroForRasConnectionL()"));)

	// First get the IAP service and IAP service type with the aSettings
	GetServiceSettingsL(aSettings);
	//Default return value, tel number is not zero
	TBool flag=EFalse;
	
	if(aSettings.iServiceType.CompareF(TPtrC(DIAL_OUT_ISP))==0)
		{
		TDialString telNum;
		CCDDialOutISPRecord* ispRecord = static_cast<CCDDialOutISPRecord*>(iIAPSetting->iService.iLinkedRecord);
		
		if(ispRecord)
			{
			telNum = ispRecord->iDefaultTelNum;
			}
		// Zero for direct connections (RAS)
		if (telNum.Length()==0)
			{
			flag=ETrue;
			}
		}
	// dial in or GPRS so we don't need to check this
	else
		{
		flag=EFalse;
		}
	return flag;
	}

void CCommsDatAccessImpl::SetIapConnectionPreferenceL(const TConnectionSettings& aSettings)
/**
If these are not override settings, set them in the database
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetIapConnectionPreferenceL()"));)
	__ASSERT_ALWAYS(iGotSettings, AgentPanic(Agent::EDbSettingsNotRead));

	if (!iOverrides)	// if not using any overrides then we need to update the bearer
		{
		CCDConnectionPrefsRecord* ptrConnectionPref = static_cast<CCDConnectionPrefsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdConnectionPrefsRecord));
		CleanupStack::PushL(ptrConnectionPref);

		ptrConnectionPref->iRanking = aSettings.iRank;
		ptrConnectionPref->iDirection = static_cast<TCommDbConnectionDirection>(aSettings.iDirection);
		if(!ptrConnectionPref->FindL(*iDb))
			{
			User::Leave(KErrNotFound);
			}
		// Only update the IAP in the database if the user selected it with Prompt mode
		if(ptrConnectionPref->iDialogPref == static_cast<TCommDbDialogPref>(ECommDbDialogPrefPrompt))
			{
			ptrConnectionPref->iDefaultIAP = aSettings.iIAPId;
			ptrConnectionPref->ModifyL(*iDb);
			}
	
		CleanupStack::PopAndDestroy(ptrConnectionPref);		
		}
	}

void CCommsDatAccessImpl::GetBearerAvailabilityTsyNameL(TDes& aTsyName)
/**
Get the name of the TSY that should be used for bearer availability checking.
If this global setting is not found then just use the TSY of the current modem
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBearerAvailabilityTsyNameL()"));)

	CCDGlobalSettingsRecord* ptrGlobal = static_cast<CCDGlobalSettingsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdGlobalSettingsRecord));
	CleanupStack::PushL(ptrGlobal);
	ptrGlobal->SetRecordId(1);
	TRAPD(err, ptrGlobal->LoadL(*iDb));

	if(err == KErrNone)
		{
		aTsyName = ptrGlobal->iBearerAvailabilityCheckTSY;
		}
	else if (err == KErrNotFound)
		{
		GetTsyNameL(aTsyName);
		}	
	else
		{
		User::Leave(err);
		}

	CleanupStack::PopAndDestroy(ptrGlobal);
	}

void CCommsDatAccessImpl::GetTsyNameL(TDes& aTsyName)
/**
Get the TSY name from the override settings or from the current modem settings
*/
	{	
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetTsyNameL()"));)

	//??? if we reach here then how do we know which modem to use? iIapSetting is uninitialised! we could initialise but for which iap id?
	CCDModemBearerRecord* modem = static_cast<CCDModemBearerRecord*>(iIAPSetting->iBearer.iLinkedRecord);
	aTsyName = modem->iTsyName;
	}

void CCommsDatAccessImpl::SetCommPortL(const RCall::TCommPort& aCommPort)
/**
Set the comm port from ETEL, so that we all use the same one for
dial up and dial in
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetCommPortL()"));)
	iCommPort = aCommPort;
	}

void CCommsDatAccessImpl::GetServiceTypeL(TDes& aServiceType)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetServiceTypeL()"));)
	aServiceType = iIAPSetting->iServiceType;
	}

void CCommsDatAccessImpl::GetAuthParamsL(TBool& aPromptForAuth,TDes& aUsername,TDes& aPassword)
/**
Get boolean PromptForAuth and authentication name and password
Not valid for dial in ISP.
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetAuthParamsL()"));)
	
	TBuf<KCommsDbSvrMaxFieldLength> serviceType;
	GetServiceTypeL(serviceType);
	if (serviceType == TPtrC(KCDTypeNameDialInISP))
		{
		AgentPanic(Agent::EIllegalDbRequestForDialIn);
		}
	else if(serviceType == TPtrC(KCDTypeNameDialOutISP))
		{
		// extract service table pointer from IAP table
		CCDServiceRecordBase* serviceSetting = static_cast<CCDServiceRecordBase*>((iIAPSetting->iService).iLinkedRecord);
		if(serviceSetting)
			{
			CCDDialOutISPRecord* serviceRecord = static_cast<CCDDialOutISPRecord*>(serviceSetting);

			aPromptForAuth = serviceRecord->iIfPromptForAuth;
			aUsername = serviceRecord->iIfAuthName;
			aPassword = serviceRecord->iIfAuthPass;	
			}
		}
	else
		{
		User::Leave(KErrNotFound);
		}	
	}

void CCommsDatAccessImpl::GetAgentExtL(const TDesC& /*aServiceType*/, TDes& /*aAgentExt*/)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetAgentExtL()"));)
	User::Leave(KErrNotSupported);
	}

void CCommsDatAccessImpl::SetNetworkMode(RMobilePhone::TMobilePhoneNetworkMode aNetworkMode)
/**
MobileIP: used for Mobile IP to know on what network we are on.
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::SetNetworkMode()"));)
	iNetworkMode = aNetworkMode;
	}

RMobilePhone::TMobilePhoneNetworkMode CCommsDatAccessImpl::NetworkMode() const
/**
What type of network we are on?
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::NetworkMode()"));)
	return iNetworkMode;
	}

TInt CCommsDatAccessImpl::ReadInt(const TDesC& aField, TUint32& aValue)
/**
Read the integer in the field aField of the database into aValue
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ReadInt()"));)

	TInt ret = KErrNone;

	if (aField == TPtrC(LAST_SOCKET_ACTIVITY_TIMEOUT) ||
		aField == TPtrC(LAST_SESSION_CLOSED_TIMEOUT) ||
		aField == TPtrC(LAST_SOCKET_CLOSED_TIMEOUT))
		{
	
		TRAP(ret, GetIntL(static_cast<CCDBearerRecordBase*>((iIAPSetting->iBearer).iLinkedRecord), aField, aValue));
		}
	else
		{
		TBuf<KCommsDbSvrMaxColumnNameLength> field = aField;
		CCDRecordBase* table = NULL;
		TRAP(ret,(table = ConvertFieldNameL(field)));

		if (ret == KErrNone)
			{
			TRAP(ret,GetIntL(table,field,aValue));
			}
		}

	return ret;
	}

TInt CCommsDatAccessImpl::ReadBool(const TDesC& aField, TBool& aValue)
/**
Read the boolean in the field aField of the database into aValue
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ReadBool()"));)

	TBuf<KCommsDbSvrMaxColumnNameLength> field=aField;
	CCDRecordBase* table=NULL;
	TRAPD(ret,(table=ConvertFieldNameL(field)));
	if (ret==KErrNone)
		{
		TRAP(ret,GetBoolL(table,field,aValue));
		}
	return ret;
	}

TInt CCommsDatAccessImpl::ReadDes(const TDesC& aField, TDes8& aValue)
/**
Read the text in the field aField of the database into aValue
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ReadDes()"));)

	TInt ret(0);
	if (aField.CompareF(TPtrC(IF_NAME))==0)
		{
		CCDRecordBase* ptrRecord= (static_cast<CCDBearerRecordBase*>((iIAPSetting->iBearer).iLinkedRecord));
		TRAP(ret, GetDesL(ptrRecord, aField, aValue));	
		}
	else
		{
		TBuf<KCommsDbSvrMaxColumnNameLength> field=aField;
		CCDRecordBase* table=NULL;
		TRAP(ret,(table=ConvertFieldNameL(field)));
		if (ret==KErrNone)
			{
			TRAP(ret,GetDesL(table,field,aValue));
			}
		}
	return ret;
	}

TInt CCommsDatAccessImpl::ReadDes(const TDesC& aField, TDes16& aValue)
/**
Read the text in the field aField of the database into aValue
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ReadDes()"));)

	TInt ret=KErrNone;
	if ((aField.CompareF(KPPPModemCsyName())==0) && (iCommPort.iCsy.Length()>0))
		{
		if (iCommPort.iCsy.Length()>KCommsDbSvrMaxFieldLength)
			{
			return KErrOverflow;
			}
		aValue.Copy(iCommPort.iCsy);
		}
	else if ((aField.CompareF(KPPPModemPortName())==0) && (iCommPort.iPort.Length()>0))
		{
		if (iCommPort.iPort.Length()>KCommsDbSvrMaxFieldLength)
			{
			return KErrOverflow;
			}
		aValue.Copy(iCommPort.iPort);
		}
	else if (aField == TPtrC(IF_NAME))
		{
		
		CCDRecordBase* ptrRecord= (static_cast<CCDRecordBase*>((iIAPSetting->iBearer).iLinkedRecord));
		TRAP(ret, GetDesL(ptrRecord, aField, aValue));
		return ret;
		}
	else if (aField.Length() > 1 && aField.Left(aField.Length() - 1) == TPtrC(SERVICE_TYPE))
		{
		TRAP(ret,GetServiceTypeL(aValue));
		}
	else
		{
		TBuf<KCommsDbSvrMaxColumnNameLength> field=aField;
		CCDRecordBase* table=NULL;
		TRAP(ret,(table=ConvertFieldNameL(field)));
		if (ret==KErrNone)
			{
			TRAP(ret,GetDesL(table,field,aValue));
			}
		}
	return ret;
	}

HBufC* CCommsDatAccessImpl::ReadLongDesLC(const TDesC& aField)
/**
Read the long text in the field aField of the database and return it
*/
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ReadLongDesLC()"));)

	TBuf<KCommsDbSvrMaxColumnNameLength> field = aField;
	CCDRecordBase* table = ConvertFieldNameL(field);
	return GetLongDesLC(table,field);
	}


void CCommsDatAccessImpl::RequestNotificationOfServiceChangeL(MServiceChangeObserver* aObserver)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::RequestNotificationOfServiceChangeL()"));)

	// allow repeated requests by the same observer
	if (iServiceChangeObserver && (aObserver != iServiceChangeObserver) )
		{
		User::Leave(KErrInUse);
		}

	iServiceChangeObserver = aObserver;
	}

void CCommsDatAccessImpl::CheckForServiceChange(TBool aChanged)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CheckForServiceChange()"));)

	if (!aChanged)
		{
		return;
		}

	if (iServiceChangeObserver)
		{
		iServiceChangeObserver->ServiceChangeNotification(iIAPSetting->iService, static_cast<CCDServiceRecordBase*>((iIAPSetting->iService).iLinkedRecord)->iRecordName);
		iServiceChangeObserver = NULL;
		}
	}

void CCommsDatAccessImpl::CancelRequestNotificationOfServiceChange(MServiceChangeObserver* aObserver)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::CancelRequestNotificationOfServiceChange()"));)

	if (iServiceChangeObserver == aObserver)
		{
		iServiceChangeObserver = NULL;
		}
	}

void CCommsDatAccessImpl::GetIntL(const TDesC& aTable, const TDesC& aField, TUint32& aValue)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	GetIntL(ptrRecord,aField,aValue);
	}

void CCommsDatAccessImpl::GetBoolL(const TDesC& aTable, const TDesC& aField, TBool& aValue)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	GetBoolL(ptrRecord, aField, aValue);
	}

void CCommsDatAccessImpl::GetDesL(const TDesC& aTable, const TDesC& aField, TDes8& aValue)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	GetDesL(ptrRecord, aField, aValue);
	}

void CCommsDatAccessImpl::GetDesL(const TDesC& aTable, const TDesC& aField, TDes16& aValue)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	GetDesL(ptrRecord, aField, aValue);
	}

HBufC* CCommsDatAccessImpl::GetLongDesLC(const TDesC& aTable, const TDesC& aField)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetLongDesLC()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	return GetLongDesLC(ptrRecord, aField);
	}

TInt CCommsDatAccessImpl::GetLengthOfLongDesL(const TDesC& aTable, const TDesC& aField)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetLengthOfLongDesL()"));)

	CCDRecordBase* ptrRecord = ConvertTableNameToRecordAccessL(aTable);
	return GetLengthOfLongDesL(ptrRecord, aField);
	}

void CCommsDatAccessImpl::GetGlobalL(const TDesC& aName,TUint32& aVal)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetGlobalL()"));)
	TInt ret = KErrNone;

	if (iOverrides==NULL)
		{
		CCDGlobalSettingsRecord* ptrGlobal = static_cast<CCDGlobalSettingsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdGlobalSettingsRecord));
		CleanupStack::PushL(ptrGlobal);
		ptrGlobal->SetRecordId(1);
		ptrGlobal->LoadL(*iDb);
		TRAPD(ret,GetIntL(ptrGlobal,aName,aVal););
		User::LeaveIfError((ret==KErrNotFound)?KErrNetConDatabaseDefaultUndefined:ret);
		CleanupStack::PopAndDestroy(ptrGlobal);	
		}
	else
		{
		ret=iOverrides->GetIntOverride(aName,KNullDesC,aVal);
		if (ret==KErrNotFound)	// has not be overridden or cannot be
			{
			CCDGlobalSettingsRecord* ptrGlobal = static_cast<CCDGlobalSettingsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdGlobalSettingsRecord));
			CleanupStack::PushL(ptrGlobal);
			ptrGlobal->SetRecordId(1);
			ptrGlobal->LoadL(*iDb);
			TRAPD(ret,GetIntL(ptrGlobal,aName,aVal););
			User::LeaveIfError((ret==KErrNotFound)?KErrNetConDatabaseDefaultUndefined:ret);
			CleanupStack::PopAndDestroy(ptrGlobal);	
			}
		}
	User::LeaveIfError(ret);
	}

CCommsDbAccess::CCommsDbAccessModemTable* CCommsDatAccessImpl::ModemTable()
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ModemTable()"));)

	CCommsDbAccess::CCommsDbAccessModemTable* modemTable(NULL);
	TRAPD(err, modemTable = new (ELeave) CCommsDbAccess::CCommsDbAccessModemTable());
	if (err != KErrNone)
		{
		return NULL;
		}

	modemTable->iRecord = (static_cast<CCDRecordBase*>((iIAPSetting->iBearer).iLinkedRecord));
	modemTable->iSession = iDb;
	return modemTable;
	}

TUint32 CCommsDatAccessImpl::LocationId() const
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::LocationId()"));)
	return iIAPSetting->iLocation;
	}

TCommDbConnectionDirection CCommsDatAccessImpl::GetConnectionDirection() const
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetConnectionDirection()"));)
	return iDirection;
	}
	
	
	
	
	
	
	
	

//
// Private functions
//


CCDRecordBase* CCommsDatAccessImpl::ConvertFieldNameL(TDes& aName)
	{
	TInt pos = aName.Locate(TChar(KSlashChar));

	if (pos == KErrNotFound) // Default is service table
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iService).iLinkedRecord);
		}

	TBuf<KCommsDbSvrMaxColumnNameLength> tableName = aName.Left(pos);
	aName.Delete(0,pos + 1);

	return ConvertTableNameToRecordAccessL(tableName);
	}

CCDRecordBase* CCommsDatAccessImpl::ConvertTableNameToRecordAccessL(const TDesC& aTable)
	{
	LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::ConvertTableNameToRecordAccessL() aTable=%S"), &aTable);)

	// request for iap
	if (aTable.Length() >= 3 &&
		aTable.Right(3) == TPtrC(IAP) )
		{
		return static_cast<CCDRecordBase*>(iIAPSetting);	
		}

	if (aTable == TPtrC(LOCATION))
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iLocation).iLinkedRecord);
		}
	
	if (aTable == iIAPSetting->iServiceType)
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iService).iLinkedRecord);
		}

	CCDServiceRecordBase* serviceSetting = static_cast<CCDServiceRecordBase*>((iIAPSetting->iService).iLinkedRecord);

	if ((serviceSetting && (aTable.CompareF(serviceSetting->iRecordName) == 0)) || aTable == KGeneralServiceTable)
	//if (aTable == iServiceSetting.Name() || aTable == KGeneralServiceTable)
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iService).iLinkedRecord);
		}

	if (iLanServiceExtensionTable && serviceSetting)
		{
		if (aTable == static_cast<CCDLANServiceRecord*>(serviceSetting)->iServiceExtensionTableName)
			{
			return static_cast<CCDRecordBase*>(iLanServiceExtensionTable);
			}
		}

	if (aTable == TPtrC(MODEM_BEARER) || aTable == TPtrC(OLD_MODEM_TABLE))
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iBearer).iLinkedRecord);
		}

	if (aTable == iIAPSetting->iBearerType)
		{
		return static_cast<CCDRecordBase*>((iIAPSetting->iBearer).iLinkedRecord);
		}

	if (aTable == TPtrC(CHARGECARD))
		{
		return static_cast<CCDRecordBase*>(iChargecardSetting);	
		}
		
	if (serviceSetting && aTable == TPtrC(QOS_UMTS_R99_AND_ON_TABLE))
		{
		CCDWCDMAPacketServiceRecord* cdmaServiceRecord;
		cdmaServiceRecord = static_cast<CCDWCDMAPacketServiceRecord*>(serviceSetting);
		return static_cast<CCDRecordBase*>(cdmaServiceRecord->iUmtsR99QoSAndOnTable.iLinkedRecord);
		}

	User::Leave(KErrNotSupported);
	return NULL;
	}



void CCommsDatAccessImpl::GetIntL(CCDRecordBase* aRecord, const TDesC& aField, TUint32& aValue)
	{
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}

	if (iOverrides)
		{
		TInt ret = iOverrides->GetIntOverride(aRecord->iRecordName,aField,aValue);
		if (ret == KErrNone)
			{
			return;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	if (aField.CompareF(TPtrC(KCDTypeNameRecordTag)) == 0)
		{
		aValue = aRecord->RecordId();
		LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(aField=%S, aValue=%d) - reading record id"), &aField, aValue);)
		}
	else
		{
		TInt type(0);
		CMDBField<TUint32>* field = static_cast<CMDBField<TUint32> *>(aRecord->GetFieldByNameL(aField, type));
		if (type != EInt && type != EUint32)
			{
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(): Leave - Bad type"));)
				User::Leave(KErrBadName);
			}
		// mimic same behaviour as provided by commdb - todo remove once viki gives us this value anyway
		if (field->IsNull())
			{
			// if field is null read from the template record
			TUint32 id = field->RecordId();
			
			CMDBField<TUint32>* templateField = new (ELeave) CMDBField<TUint32>();
			templateField->SetElementId(field->ElementId());
			templateField->SetRecordId(KCDDefaultRecord);
			CleanupStack::PushL(templateField);

			TRAPD(err,templateField->LoadL(*iDb));
			if (err == KErrNotFound)
				{
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(): Leave - KErrUnknown"));)
				User::Leave(KErrUnknown);
				}	
			else if (err != KErrNone)
				{
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(): Leave - %d"),err);)
				User::Leave(err);
				}
			aValue = static_cast<TUint32>(*templateField);
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(aField=%S, aValue=%d) read from template record"), &aField, aValue);)
			CleanupStack::PopAndDestroy(templateField);
			}
		else
			{
			aValue = static_cast<TUint32>(*field);
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetIntL(aField=%S, aValue=%d)"), &aField, aValue);)
			}
		}
	}

void CCommsDatAccessImpl::GetBoolL(CCDRecordBase* aRecord, const TDesC& aField, TBool& aValue)
	{
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}
		
	if (iOverrides)
		{
		TInt ret = iOverrides->GetBoolOverride(aRecord->iRecordName,aField,aValue);
		if (ret == KErrNone)
			{
			return;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	TInt type(0);
	CMDBField<TBool>* field = static_cast<CMDBField<TBool> *>(aRecord->GetFieldByNameL(aField, type));
	if (type != EBool)
		{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL(): Leave - Bad type"));)
			User::Leave(KErrBadName);
		}
	
	// mimic same behaviour as provided by commdb - todo remove once viki gives us this value anyway
	if (field->IsNull())
		{
		// if field is null read from the template record
		TUint32 id = field->RecordId();
		
		CMDBField<TBool>* templateField = new (ELeave) CMDBField<TBool>();
		templateField->SetElementId(field->ElementId());
		templateField->SetRecordId(KCDDefaultRecord);
		CleanupStack::PushL(templateField);

		TRAPD(err,templateField->LoadL(*iDb));
		if (err == KErrNotFound)
			{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL(): Leave - KErrUnknown"));)
			User::Leave(KErrUnknown);
			}	
		else if (err != KErrNone)
			{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL(): Leave - %d"),err);)
			User::Leave(err);
			}
		aValue = static_cast<TBool>(*templateField);
		LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL(aField=%S, aValue=%d) read from template record"), &aField, aValue);)
		CleanupStack::PopAndDestroy(templateField);
		}
	else
		{
		aValue = static_cast<TBool>(*field);
		LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetBoolL(aField=%S, aValue=%d)"), &aField, aValue);)
		}
	}

void CCommsDatAccessImpl::GetDesL(CCDRecordBase* aRecord, const TDesC& aField, TDes8& aValue)
	{
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}

	if (iOverrides)
		{
		TInt ret = iOverrides->GetDesOverride(aRecord->iRecordName,aField,aValue);
		if (ret == KErrNone)
			{
			return;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	aValue.Zero();

	TInt type(0);
	
	CMDBElement* baseField = aRecord->GetFieldByNameL(aField, type);
	// mimic same behaviour as provided by commdb - todo remove once viki gives us this value anyway
	if (baseField->IsNull())
		{
		// if field is null read from the template record
		TUint32 id = baseField->RecordId();
		
		switch (type)
			{
			case EDesC8:
				{
				CMDBField<TDesC8>* templateField = new (ELeave) CMDBField<TDesC8>();
				templateField->SetElementId(baseField->ElementId());
				templateField->SetRecordId(KCDDefaultRecord);
				CleanupStack::PushL(templateField);

				TRAPD(err,templateField->LoadL(*iDb));
				if (err == KErrNone)
					{
					//aValue = static_cast<TDesC8>(*templateField);
					aValue = *templateField;
					LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8(aField=%S ..."), &aField);)
					//LOG(NifmanLog::Printf(_L8("... aValue=%S)  read from template record"), &aValue);)
					}
				CleanupStack::PopAndDestroy(templateField);
				break;
				}
			case EMedText:
			case EText:
				{
				CMDBField<TDesC>* templateField = new (ELeave) CMDBField<TDesC>();
				templateField->SetElementId(baseField->ElementId());
				templateField->SetRecordId(KCDDefaultRecord);
				CleanupStack::PushL(templateField);

				TRAPD(err,templateField->LoadL(*iDb));
				if (err == KErrNone)
					{
					CMDBField<TDesC>* field16 = static_cast<CMDBField<TDesC>*>(templateField);
					const TDesC& refField = *field16;
					aValue.Copy(refField);
					
					LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8(aField=%S, aValue=%S)  read from template record"), &aField, &refField);)
					}
				CleanupStack::PopAndDestroy(templateField);
				break;
				}
			default:
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8() Bad type"), &aField);)
				User::Leave(KErrBadName);
			}
		}
	else
		{
		switch (type)
			{
			case EDesC8:
				{
				CMDBField<TDesC8>* myField = static_cast<CMDBField<TDesC8>*>(baseField);
				//aValue = static_cast<TDesC8&>(*myField);
				aValue = *myField;
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8(aField=%S ..."), &aField);)
				//LOG(NifmanLog::Printf(_L8("... aValue=%S)  read from template record"), &aValue);)
				break;
				}
			case EMedText:
			case EText:
				{
				CMDBField<TDesC>* field16 = static_cast<CMDBField<TDesC>*>(baseField);
				const TDesC& refField = *field16;
				aValue.Copy(refField);
				
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8(aField=%S, aValue=%S)  read from template record"), &aField, &refField);)
				//LOG(NifmanLog::Printf(_L8("... aValue=%S)  read from template record"), &aValue);)
				break;
				}
			default:
				LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL8() Bad type"), &aField);)
				User::Leave(KErrBadName);
			}
		}
	}

void CCommsDatAccessImpl::GetDesL(CCDRecordBase* aRecord, const TDesC& aField, TDes16& aValue)
	{
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}

	if (iOverrides)
		{
		TInt ret = iOverrides->GetDesOverride(aRecord->iRecordName,aField,aValue);
		if (ret == KErrNone)
			{
			return;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	aValue.Zero();

	TInt type(0);
	CMDBField<TDesC>* field = static_cast<CMDBField<TDesC> *>(aRecord->GetFieldByNameL(aField, type));
	if (type != EText && type != EMedText && type != ELongText)
		{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL(): Leave - Bad type"));)
			User::Leave(KErrBadName);
		}
	// mimic same behaviour as provided by commdb - todo remove once viki gives us this value anyway
	if (field->IsNull())
		{
		// if field is null read from the template record
		TUint32 id = field->RecordId();
		
		CMDBField<TDesC>* templateField = new (ELeave) CMDBField<TDesC>();
		templateField->SetElementId(field->ElementId());
		templateField->SetRecordId(KCDDefaultRecord);
		CleanupStack::PushL(templateField);

		TRAPD(err,templateField->LoadL(*iDb));
		if (err == KErrNone)
			{
			aValue = *templateField;
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL(aField=%S, aValue=%S) read from template record"), &aField, &aValue);)
			}
		CleanupStack::PopAndDestroy(templateField);
		}
	else
		{
		aValue = *field;
		LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL(aField=%S, aValue=%S)"), &aField, &aValue);)
		}
	}

HBufC* CCommsDatAccessImpl::GetLongDesLC(CCDRecordBase* aRecord, const TDesC& aField)
	{	
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}

	if (iOverrides)
		{
		TInt len(0);
		TInt ret = iOverrides->GetLongDesOverrideLength(aRecord->iRecordName,aField,len);
		if (ret == KErrNone)
			{
			HBufC* buf = HBufC::NewLC(len);
			TPtr temp(buf->Des());
			User::LeaveIfError(iOverrides->GetLongDesOverride(aRecord->iRecordName,aField,temp));
			return buf;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	HBufC* buf(NULL);
	TInt type(0);

	CMDBField<TDesC>* field = static_cast<CMDBField<TDesC> *>(aRecord->GetFieldByNameL(aField, type));
	if (type != ELongText)
		{
			LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetLongDesLC(): Leave - Bad type"));)
			User::Leave(KErrBadName);
		}

	// mimic same behaviour as provided by commdb - todo remove once viki gives us this value anyway
	if (field->IsNull())
		{
		// if field is null read from the template record
		TUint32 id = field->RecordId();
		
		CMDBField<TDesC>* templateField = new (ELeave) CMDBField<TDesC>();
		templateField->SetElementId(field->ElementId());
		templateField->SetRecordId(KCDDefaultRecord);
		CleanupStack::PushL(templateField);

		TRAPD(err,templateField->LoadL(*iDb));
		if (err == KErrNone)
			{
			const TDesC& ptrTemplateField = *templateField;
			buf = HBufC::NewMaxLC(ptrTemplateField.Length());
			buf->Des().Copy(ptrTemplateField);
						
			//LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL(aField=%S, aValue=%S) read from template record"), &aField, &buf);)
			}
		else
			{
			// return empty buffer
			buf = HBufC::NewLC(0);
			}
		CleanupStack::Pop(buf);
		CleanupStack::PopAndDestroy(templateField);
		CleanupStack::PushL(buf);
		}
	else
		{
		const TDesC& ptrField = *field;
		buf = HBufC::NewMaxLC(ptrField.Length());
		buf->Des().Copy(ptrField);
		
		//LOG(NifmanLog::Printf(_L("CCommsDatAccessImpl::GetDesL(aField=%S, aValue=%S)"), &aField, &buf);)
		}

	return buf;
	}

TInt CCommsDatAccessImpl::GetLengthOfLongDesL(CCDRecordBase* aRecord, const TDesC& aField)
	{
	if (!aRecord)
		{
		User::Leave(KErrNotReady);
		}

	TInt len = 0;

	if (iOverrides)
		{
		TInt ret = iOverrides->GetLongDesOverrideLength(aRecord->iRecordName,aField,len);
		if (ret == KErrNone)
			{
			return len;
			}
		if (ret != KErrNotFound)
			{
			User::LeaveIfError(ret);
			}
		}

	TInt type(0);
	CMDBField<TDesC>* field = static_cast<CMDBField<TDesC>*>(aRecord->GetFieldByNameL(aField, type));
	//TInt length = static_cast<TDesC>(*field).Length();
	const TDesC& lenfield = *field;
	TInt length = lenfield.Length();
	return length;
	}

TInt CCommsDatAccessImpl::WriteDes(const TDesC& /*aField*/, const TDesC16& /*aValue*/)
/**
Writes from the NIF are currently not supported...
*/
	{
	return KErrNotSupported;
	}

TInt CCommsDatAccessImpl::WriteDes(const TDesC& /*aField*/, const TDesC8& /*aValue*/)
/**
Writes from the NIF are currently not supported...
*/
	{
	return KErrNotSupported;
	}


TInt CCommsDatAccessImpl::WriteBool(const TDesC& /*aField*/, TBool /*aValue*/)
/**
Writes from the NIF are currently not supported...
*/
	{
	return KErrNotSupported;
	}

TInt CCommsDatAccessImpl::WriteInt(const TDesC& /*aField*/, TUint32 /*aValue*/)
/**
Writes from the NIF are currently not supported...
*/
	{
	return KErrNotSupported;
	}


TInt CCommsDatAccessImpl::GetConnectionAttempts()
	{
	TUint32 attempts = KCommsDbMaxConnections; // Use two if not set in commdb
	TRAPD(err, GetGlobalL(TPtrC(CONNECTION_ATTEMPTS),attempts));
	if (err != KErrNone)
		{
		return err;
		}
	return attempts;
	}


	/** Data capability checking */
TInt CCommsDatAccessImpl::DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	CommsDatUtils::CCommsDatUtils* commsUtils = CommsDatUtils::CCommsDatUtils::NewL();
    TInt ret = commsUtils->CheckReadCapability(aField, aMessage);
    delete commsUtils;

	return ret;
	}

TInt CCommsDatAccessImpl::DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	CommsDatUtils::CCommsDatUtils* commsUtils = CommsDatUtils::CCommsDatUtils::NewL();
    TInt ret = commsUtils->CheckWriteCapability(aField, aMessage);
    delete commsUtils;

	return 0;
	}

