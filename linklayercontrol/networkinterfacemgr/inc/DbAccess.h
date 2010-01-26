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
// Agent Database Access Class Header
// 
//

/**
 @file DBACCESS.H
*/


#ifndef __DBACCESS_H__
#define __DBACCESS_H__

#include <etel.h>
#include <etelmm.h>
#include <nifvar.h>
#include <cdbover.h>
#include <comms-infras/connectionsettings.h>
#include <agentdialog.h>
#include <commsdattypesv1_1.h>

/**
@internalComponent
*/
_LIT(KGeneralServiceTable, "ISP");

class CCommDbOverrideSettings;

NONSHARABLE_CLASS(CDefaultRecordAccess) : public CBase
/**
@internalTechnology
*/
	{
public:
	CDefaultRecordAccess();
	CDefaultRecordAccess(const TDesC& aName);
	~CDefaultRecordAccess();
	void Close();

	TBool OpenRecordL(CCommsDatabase* aDb, TUint32 aId);
	TBool OpenRecordL(CCommsDatabase* aDb, TUint32 aId, const TDesC& aTableName);

	TBool SetOverridden(TBool aOverridden, TUint32 aId);

	inline CCommsDbTableView* Table() const 
	    { return iTable; }

	inline TPtrC Name() const 
    	{ return iName; }

	inline TUint32 Id() const 
    	{ return iId; }

private:
	void CloseTable();
private:
	CCommsDbTableView* iTable;
	TBuf<KCommsDbSvrMaxColumnNameLength> iName;
	TUint32 iId;
	TBool iOverridden;
	};

class MServiceChangeObserver
/**
@internalTechnology
*/
	{
public:
	virtual void ServiceChangeNotification(TUint32 aId, const TDesC& aType) = 0;
	};

class MCommsDbAccess;
class CCommsDbAccess : public CBase
/**
CCommsDbAccess
One of these per CAgentController object.  Has a CCommsDatabase object and CCommsDbTableView's for 
accessing data in CommDb.  Also uses a CCommDbOverrideSettings for accessing overridden 
fields.  Also has a CDbChangeNotification for checking when the database is changed. 
Exported methods in CCommsDbAccess object are also used by the CScriptBase class.
@internalTechnology
*/
	{
public:
	IMPORT_C static CCommsDbAccess* NewL();
	IMPORT_C static CCommsDbAccess* NewL(TBool aShowHidden);
 	IMPORT_C static CCommsDbAccess* NewL(MCommsDbAccess *aPimpl);

	IMPORT_C ~CCommsDbAccess();

	/** Close database access */
	IMPORT_C void Close();

	/** Set function */
	IMPORT_C void SetOverridesL(CCommDbOverrideSettings* aOverrides);

	// Get functions
	IMPORT_C TBool IsShowingHiddenRecords();

	/** Default setting */ 
	IMPORT_C void GetCurrentSettingsL(TConnectionSettings& aSettings, TCommDbConnectionDirection aDirection, TUint32 aRank);
	IMPORT_C void SetCurrentSettingsL(const TConnectionSettings& aSettings);
	IMPORT_C void GetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank);
	IMPORT_C void SetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank);
	IMPORT_C TBool DoesIapExistL(TUint32 aIapId);
	IMPORT_C void GetFirstValidIapL(TUint32& aIapId);
	IMPORT_C void SetModemAndLocationL(const TConnectionSettings& aSettings); // DEPRECATED
	IMPORT_C void GetServiceSettingsL(TConnectionSettings& aSettings);
	IMPORT_C TBool IsTelNumLengthZeroForRasConnectionL(TConnectionSettings& aSettings);
	IMPORT_C TInt GetConnectionAttempts();

	/** Modem related */
	IMPORT_C void GetBearerAvailabilityTsyNameL(TDes& aTsyName);
	IMPORT_C void GetTsyNameL(TDes& aTsyName);
	IMPORT_C void SetCommPortL(const RCall::TCommPort& aCommPort);

	/** Service Related */
	IMPORT_C void GetServiceTypeL(TDes& aServiceType);
	void GetAuthParamsL(TBool& aPromptForAuth,TDes& aUsername,TDes& aPassword);

	/** Agent Related */
	void GetAgentExtL(const TDesC& aServiceType, TDes& aAgentExt);

	/** MobileIP Related */
  	IMPORT_C void SetNetworkMode(const RMobilePhone::TMobilePhoneNetworkMode aNetworkMode);
	IMPORT_C RMobilePhone::TMobilePhoneNetworkMode NetworkMode() const;

	/** Call to database server for use by NifMan */
	IMPORT_C TInt ReadInt(const TDesC& aField, TUint32& aValue);
	IMPORT_C TInt ReadBool(const TDesC& aField, TBool& aValue);
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes8& aValue);
	IMPORT_C TInt ReadDes(const TDesC& aField, TDes16& aValue);
	IMPORT_C HBufC* ReadLongDesLC(const TDesC& aField);
	IMPORT_C TInt WriteInt(const TDesC& aField, TUint32 aValue);
	IMPORT_C TInt WriteBool(const TDesC& aField, TBool aValuge);
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC8& aValue);
	IMPORT_C TInt WriteDes(const TDesC& aField, const TDesC16& aValue);

	/** Service change noification */
	IMPORT_C void RequestNotificationOfServiceChangeL(MServiceChangeObserver* aObserver);
	IMPORT_C void CancelRequestNotificationOfServiceChange(MServiceChangeObserver* aObserver);

	/** Reads from the agent extensions: read from the overrides, or 
	if they don't exist straight from the database */
	IMPORT_C void GetIntL(const TDesC& aTable, const TDesC& aField, TUint32& aValue);
	IMPORT_C void GetBoolL(const TDesC& aTable, const TDesC& aField, TBool& aValue);
	IMPORT_C void GetDesL(const TDesC& aTable, const TDesC& aField, TDes8& aValue);
	IMPORT_C void GetDesL(const TDesC& aTable, const TDesC& aField, TDes16& aValue);
	IMPORT_C HBufC* GetLongDesLC(const TDesC& aTable, const TDesC& aField);
	IMPORT_C TInt GetLengthOfLongDesL(const TDesC& aTable, const TDesC& aField);
	IMPORT_C void GetGlobalL(const TDesC& aName,TUint32& aVal);

	/** Some specific function for agent exts */
	/**
	 * @note The class CCommsDbAccess::CCommsDbAccessModemTable* is a container of a database 
	 * record and the session. The session is required to allow the caller to write back to the database.
	 */
	class CCommsDbAccessModemTable : public CBase
		{
		public:
			CCDRecordBase* iRecord;
			CMDBSession* iSession;
		};
	/**
	 * The caller of this function takes ownership of the object returned and is responsible for deleting 
	 * it. However the caller is not responsible for deleting the referenced record and session.
	 */
	IMPORT_C CCommsDbAccessModemTable* ModemTable();
	
	IMPORT_C TUint32 LocationId() const;
	IMPORT_C TCommDbConnectionDirection GetConnectionDirection() const;

	/** Data capability checking */
	IMPORT_C TInt CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	IMPORT_C TInt CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );
private:
	CCommsDbAccess();
 	explicit CCommsDbAccess( MCommsDbAccess *aPimpl );
	void ConstructL(TBool aShowHidden);

	MCommsDbAccess* GetImpl() const;
private:
	MCommsDbAccess* iPimpl;		//< Implementation 
	TBool iOwnImpl;				//< Flag to indicate this object owns the implementation pointer
	};

#endif

