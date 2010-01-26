#ifndef __DBACCESSIMPLv2_H__
#define __DBACCESSIMPLv2_H__
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
// Agent Database Access Implementation Class Header
// 
//

/**
 @file DBACCESSIMPL.H
*/

#include "MDbAccess.h"
#include <commsdattypesv1_1.h>	
using namespace CommsDat;

NONSHARABLE_CLASS(CCommsDatAccessImpl) : public CBase, public MCommsDbAccess
/**
CCommsDatAccessImpl
One of these per CAgentController object.  Has a CCommsDatabase object and CCommsDbTableView's for 
accessing data in CommDb.  Also uses a CCommDbOverrideSettings for accessing overridden 
fields.  Also has a CDbChangeNotification for checking when the database is changed. 
Exported methods in CCommsDatAccessImpl object are also used by the CScriptBase class.
@internalTechnology
*/
	{
public:
	static CCommsDatAccessImpl* NewL();
	static CCommsDatAccessImpl* NewL(TBool aShowHidden);
	virtual ~CCommsDatAccessImpl();

	/** Close database access */
	virtual void Close();

	/** Set function */
	virtual void SetOverridesL(CCommDbOverrideSettings* aOverrides);

	// Get functions
	virtual TBool IsShowingHiddenRecords();

	/** Default setting */ 
	virtual void GetCurrentSettingsL(TConnectionSettings& aSettings, TCommDbConnectionDirection aDirection, TUint32 aRank);
	virtual void SetCurrentSettingsL(const TConnectionSettings& aSettings);
	virtual void GetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank);
	virtual void SetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank);
	virtual TBool DoesIapExistL(TUint32 aIapId);
	virtual void GetFirstValidIapL(TUint32& aIapId);
	virtual void GetServiceSettingsL(TConnectionSettings& aSettings);
	virtual TBool IsTelNumLengthZeroForRasConnectionL(TConnectionSettings& aSettings);

	/**
	  Fetch the maximum number of connection attempts from the database
	*/
	virtual TInt GetConnectionAttempts();

	/** Modem related */
	virtual void GetBearerAvailabilityTsyNameL(TDes& aTsyName);
	virtual void GetTsyNameL(TDes& aTsyName);
	virtual void SetCommPortL(const RCall::TCommPort& aCommPort);

	/** Service Related */
	virtual void GetServiceTypeL(TDes& aServiceType);
	virtual void GetAuthParamsL(TBool& aPromptForAuth,TDes& aUsername,TDes& aPassword);

	/** Agent Related */
	virtual void GetAgentExtL(const TDesC& aServiceType, TDes& aAgentExt);

	/** MobileIP Related */
  	virtual void SetNetworkMode(RMobilePhone::TMobilePhoneNetworkMode aNetworkMode);
	virtual RMobilePhone::TMobilePhoneNetworkMode NetworkMode() const;

	/** Call to database server for use by NifMan */
	virtual TInt ReadInt(const TDesC& aField, TUint32& aValue);
	virtual TInt ReadBool(const TDesC& aField, TBool& aValue);
	virtual TInt ReadDes(const TDesC& aField, TDes8& aValue);
	virtual TInt ReadDes(const TDesC& aField, TDes16& aValue);
	virtual HBufC* ReadLongDesLC(const TDesC& aField);
	virtual TInt WriteInt(const TDesC& aField, TUint32 aValue);
	virtual TInt WriteBool(const TDesC& aField, TBool aValuge);
	virtual TInt WriteDes(const TDesC& aField, const TDesC8& aValue);
	virtual TInt WriteDes(const TDesC& aField, const TDesC16& aValue);

	/** Service change noification */
	virtual void RequestNotificationOfServiceChangeL(MServiceChangeObserver* aObserver);
	virtual void CancelRequestNotificationOfServiceChange(MServiceChangeObserver* aObserver);

	/** Reads from the agent extensions: read from the overrides, or 
	if they don't exist straight from the database */
	virtual void GetIntL(const TDesC& aTable, const TDesC& aField, TUint32& aValue);
	virtual void GetBoolL(const TDesC& aTable, const TDesC& aField, TBool& aValue);
	virtual void GetDesL(const TDesC& aTable, const TDesC& aField, TDes8& aValue);
	virtual void GetDesL(const TDesC& aTable, const TDesC& aField, TDes16& aValue);
	virtual HBufC* GetLongDesLC(const TDesC& aTable, const TDesC& aField);
	virtual TInt GetLengthOfLongDesL(const TDesC& aTable, const TDesC& aField);
	virtual void GetGlobalL(const TDesC& aName,TUint32& aVal);

	/** Some specific function for agent exts */
	virtual CCommsDbAccess::CCommsDbAccessModemTable* ModemTable();
	virtual TUint32 LocationId() const;
	virtual TCommDbConnectionDirection GetConnectionDirection() const;

protected:
	/** Data capability checking */
	virtual TInt DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	virtual TInt DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );

private:
	CCommsDatAccessImpl();
	void ConstructL(TBool& aShowHidden);

	/** Getting and setting connection preferences */
	void GetIapConnectionPreferenceL(TConnectionSettings& aSettings);
	void SetIapConnectionPreferenceL(const TConnectionSettings& aSettings);
	TBool LocationRequiredL();

	/** Convert name from NifMan */
	CCDRecordBase* ConvertFieldNameL(TDes& aName);
	CCDRecordBase* ConvertTableNameToRecordAccessL(const TDesC& aTable);
	
	/** Service change notification */
	void CheckForServiceChange(TBool aChanged);

	/** Reads from overrides and then if not there read from database */
	void GetIntL(CCDRecordBase* aRecord, const TDesC& aField, TUint32& aValue);
	void GetBoolL(CCDRecordBase* aRecord, const TDesC& aField, TBool& aValue);
	void GetDesL(CCDRecordBase* aRecord, const TDesC& aField, TDes8& aValue);
	void GetDesL(CCDRecordBase* aRecord, const TDesC& aField, TDes16& aValue);
	HBufC* GetLongDesLC(CCDRecordBase* aRecord, const TDesC& aField);
	TInt GetLengthOfLongDesL(CCDRecordBase* aRecord, const TDesC& aField);
	
	void CreateCacheL();

private:
	CMDBSession* iDb;
	TCommDbConnectionDirection iDirection;
	CCDIAPRecord* iIAPSetting;
	//CCDServiceRecordBase* iServiceSetting;
	CCDChargecardRecord* iChargecardSetting;
	CCDBearerRecordBase* iBearerSetting;
	CCDLocationRecord* iLocationSetting;
	CCDRecordBase* iLanServiceExtensionTable;	// used for the LAN service extension tables, such as BT PAN and WLAN
													// needs to appear as part of the LAN service table to clients, as they cannot cope with the extra level of indirection in the database
													// (they have no way of specifying a record id in a table, and therefore cannot access the correct records in the PAN and WLAN extension tables)
	TBool iGotSettings;
	CCommDbOverrideSettings* iOverrides;
	RCall::TCommPort iCommPort;
	MServiceChangeObserver* iServiceChangeObserver;

	/** MobileIP network mode */
	RMobilePhone::TMobilePhoneNetworkMode iNetworkMode;
	TBool iShowHidden;
	
	//NEw new
	TBool iIAPOverridden;
	
	};


#endif /* #ifndef __DBACCESSIMPL_H__ */

