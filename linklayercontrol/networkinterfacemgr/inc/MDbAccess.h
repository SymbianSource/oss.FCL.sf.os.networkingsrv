#ifndef __MDBACCESS_H__
#define __MDBACCESS_H__
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
// Agent Database Access Class Header
// 
//

/**
 @file MDBACCESS.H
*/

#include <etel.h>
#include <etelmm.h>
#include <comms-infras/connectionsettings.h>
#include <commsdattypesv1_1.h>
#include <comms-infras/dbaccess.h>

class CCommDbOverrideSettings;
class TConnectionSettings;
class MServiceChangeObserver;

class MCommsDbAccess
/**
MCommsDbAccess
Abstract interface to CCommsDbAccess implementation

Note that the interface class provides certain sensible 'default'
implementations of some of the interface functions, to reduce the
burden on implementations

@internalTechnology
*/
	{
public:
	IMPORT_C virtual ~MCommsDbAccess();

	/** Close database access */
	virtual void Close() = 0;

	/**
	Sets overrides on the database

	@param aOverrides pointer to the override settings to store.  Note that ownership of these overrides is retained by the caller.
	@exception leaves if database access fails
	*/
	virtual void SetOverridesL(CCommDbOverrideSettings* aOverrides) = 0;

	// Get functions
	virtual TBool IsShowingHiddenRecords() = 0;

	/** Default setting */
	/**
	Retrieves the current connection preferences from CommDb

	@param aSettings on return contatins the connection preferences for the specified direction and rank
	@param aDirection the connection direction (either incoming or outgoing)
	@param aRank the current connection attempt
	@exception leaves if database access fails
	*/
	virtual void GetCurrentSettingsL(TConnectionSettings& aSettings, TCommDbConnectionDirection aDirection, TUint32 aRank) = 0;
	/**
	Stores the current connection preferences in CommDb

	@param aSettings contatins the connection preferences for the specified direction and rank (stored inside aSettings)
	@exception leaves if database access fails
	*/
	virtual void SetCurrentSettingsL(const TConnectionSettings& aSettings) = 0;
	/**
	Retrieves the IapId for the connection type of ranking aRank

	@param IapId the id of the IAP to look for in the IAP table.
	@param aDirection the connection direction (either incoming or outgoing)
	@param aRank the current connection attempt
	@exception leaves if database access fails
	*/
	virtual void GetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank) = 0;
	/**
	Stores the prefered IapId for the connection type of ranking aRank

	@param IapId the id of the IAP to look for in the IAP table
	@param aDirection the connection direction (either incoming or outgoing).
	@param aRank the current connection attempt
	@exception leaves if database access fails
	*/
	virtual void SetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank) = 0;
	/**
	Checks in the database for the precence of an Iap

	@param aIapId, the id of the IAP to look for in the IAP table
	@returns ETrue if the Iap exists
	*/
	virtual TBool DoesIapExistL(TUint32 aIapId) = 0;
	virtual void GetFirstValidIapL(TUint32& aIapId) = 0;

	/**
	For a specified IAP, retrieves the CommDb ID of the service table and the type of the service table (e.g. DIAL_OUT_ISP, INCOMING_GPRS etc)

	@param aSettings (both input and output) specifies the IAP ID and returns the service ID and type
	@exception leaves if database access fails
	*/
	virtual void GetServiceSettingsL(TConnectionSettings& aSettings) = 0;
	/**
	Determines whether this is an NTRAS connection.
	This is used to decide whether or not to check for bearer availability before selecting an IAP.
	The function looks at phone number stored in the service record.

	@param aSettings specifies the ID of the service record to look at.
	@returns ETrue if the phone number in zero, otherwise EFalse
	@exception leaves if database access fails
	*/
	virtual TBool IsTelNumLengthZeroForRasConnectionL(TConnectionSettings& aSettings) = 0;
	/**
	  Fetch the maximum number of connection attempts from the database
	*/
	virtual TInt GetConnectionAttempts() = 0;

	/** Modem related */
	virtual void GetBearerAvailabilityTsyNameL(TDes& aTsyName) = 0;
	/**
	Retrieve the TSY name from the database

	@param aName the name of the TSY
	@exception leaves if database could not be accessed
	*/
	virtual void GetTsyNameL(TDes& aTsyName) = 0;
	virtual void SetCommPortL(const RCall::TCommPort& aCommPort) = 0;

	/** Service Related */
	virtual void GetServiceTypeL(TDes& aServiceType) = 0;
	virtual void GetAuthParamsL(TBool& aPromptForAuth,TDes& aUsername,TDes& aPassword) = 0;

	/** Agent Related */
	virtual void GetAgentExtL(const TDesC& aServiceType, TDes& aAgentExt) = 0;

	/** MobileIP Related */
	/**
	Set the phone network mode in the database

	@param aMode the current network mode
	*/
  	virtual void SetNetworkMode(RMobilePhone::TMobilePhoneNetworkMode aNetworkMode) = 0;
	/**
	Fetch the phone network mode from the database

	@returns the current network mode
	*/
	virtual RMobilePhone::TMobilePhoneNetworkMode NetworkMode() const = 0;

	/** Call to database server for use by NifMan */
	virtual TInt ReadInt(const TDesC& aField, TUint32& aValue) = 0;
	virtual TInt ReadBool(const TDesC& aField, TBool& aValue) = 0;
	virtual TInt ReadDes(const TDesC& aField, TDes8& aValue) = 0;
	virtual TInt ReadDes(const TDesC& aField, TDes16& aValue) = 0;
	virtual HBufC* ReadLongDesLC(const TDesC& aField) = 0;
	virtual TInt WriteInt(const TDesC& aField, TUint32 aValue) = 0;
	virtual TInt WriteBool(const TDesC& aField, TBool aValuge) = 0;
	virtual TInt WriteDes(const TDesC& aField, const TDesC8& aValue) = 0;
	virtual TInt WriteDes(const TDesC& aField, const TDesC16& aValue) = 0;

	/** Service change noification */
	/**
	Request a notification if the database is changed

	@param aObserver Pointer to call back on with notification
	@exception leaves if unable to monitor
	*/
	virtual void RequestNotificationOfServiceChangeL(MServiceChangeObserver* aObserver) = 0;
	/**
	Cancel the request for notification if the database is changed

	@param aObserver Pointer to call back on with notification
	@exception leaves if unable to remove the subscription
	*/
	virtual void CancelRequestNotificationOfServiceChange(MServiceChangeObserver* aObserver) = 0;

	/** Reads from the agent extensions: read from the overrides, or
		if they don't exist straight from the database */
	/**
	Retrieve a TUint32 value from the database

	@param aTable the table to access
	@param aField the name of the field to retrieve
	@param aValue on return contains the value of the specified field
	@exception leaves if database access fails
	*/
	virtual void GetIntL(const TDesC& aTable, const TDesC& aField, TUint32& aValue) = 0;
	virtual void GetBoolL(const TDesC& aTable, const TDesC& aField, TBool& aValue) = 0;
	/**
	Retrieve a TDesC value from the database

	@param aTable the table to access
	@param aField the name of the field to retrieve
	@param aValue on return contains the value of the specified field
	@exception leaves if database access fails
	*/
	virtual void GetDesL(const TDesC& aTable, const TDesC& aField, TDes8& aValue) = 0;
	/**
	Retrieve a TDesC value from the database

	@param aTable the table to access
	@param aField the name of the field to retrieve
	@param aValue on return contains the value of the specified field
	@exception leaves if database access fails
	*/
	virtual void GetDesL(const TDesC& aTable, const TDesC& aField, TDes16& aValue) = 0;
	virtual HBufC* GetLongDesLC(const TDesC& aTable, const TDesC& aField) = 0;
	virtual TInt GetLengthOfLongDesL(const TDesC& aTable, const TDesC& aField) = 0;
	virtual void GetGlobalL(const TDesC& aName,TUint32& aVal) = 0;

	/** Some specific function for agent exts */
	virtual CCommsDbAccess::CCommsDbAccessModemTable* ModemTable() = 0;
	
	virtual TUint32 LocationId() const = 0;
	virtual TCommDbConnectionDirection GetConnectionDirection() const = 0;

	/** Data capability checking */
	IMPORT_C TInt CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage );
	IMPORT_C TInt CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage );
protected:
	/** Data capability checking */
	virtual TInt DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage ) = 0;
	virtual TInt DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage ) = 0;
	};


#endif /* #ifndef __MDBACCESS_H__ */

