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
#include "AgentPanic.h"
#include "Ni_Log.h"

#include "DbAccessImplv2.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManDbAccessc, "NifManDbAccess.c");
#endif

/**
@internalComponent
*/

//
// TConnectionSettings
//

EXPORT_C TConnectionSettings::TConnectionSettings():
	iRank(0),
	iDirection(ECommDbConnectionDirectionUnknown),
	iDialogPref(ECommDbDialogPrefUnknown),
	iBearerSet(0),
	iIAPId(0),
	iServiceId(0),
	iServiceType(),
	iBearerId(0),
	iBearerType(),
	iLocationId(0),
	iChargeCardId(0)
	{}


//
// CCommsDbAccess definitions
//

EXPORT_C CCommsDbAccess* CCommsDbAccess::NewL()
	{
	return CCommsDbAccess::NewL( EFalse );
 	}

EXPORT_C CCommsDbAccess* CCommsDbAccess::NewL(TBool aShowHidden)
	{
	CCommsDbAccess* self = new(ELeave) CCommsDbAccess;
	CleanupStack::PushL(self);
	self->ConstructL(aShowHidden);
	CleanupStack::Pop();
	return self;
	}

EXPORT_C CCommsDbAccess* CCommsDbAccess::NewL( MCommsDbAccess *aPimpl )
	{
	__ASSERT_DEBUG( aPimpl , User::Panic(KSpecAssert_NifManDbAccessc, 1));
	CCommsDbAccess *self = new (ELeave) CCommsDbAccess( aPimpl );
	return self;
	}

CCommsDbAccess::CCommsDbAccess()
	:iPimpl(0),
	 iOwnImpl(EFalse)
	{
	}

CCommsDbAccess::CCommsDbAccess( MCommsDbAccess *aPimpl )
 	:iPimpl( aPimpl ),
	 iOwnImpl(EFalse)
 	{
 	}

void CCommsDbAccess::ConstructL(TBool aShowHidden)
	{
	__ASSERT_DEBUG( !iPimpl , User::Panic(KSpecAssert_NifManDbAccessc, 2));
		iPimpl = CCommsDatAccessImpl::NewL( aShowHidden );
	iOwnImpl = ETrue;
	}

/**
   Retrieve the implementation pointer
*/
MCommsDbAccess* CCommsDbAccess::GetImpl() const
	{
	return iPimpl;
	}


EXPORT_C CCommsDbAccess::~CCommsDbAccess()
	{
	Close();
	if( iOwnImpl )
		{
		delete iPimpl;
		}
	iPimpl = 0;
	}

EXPORT_C void CCommsDbAccess::Close()
	{
	if(GetImpl())
		{
		GetImpl()->Close();
		}
	}

EXPORT_C void CCommsDbAccess::SetOverridesL(CCommDbOverrideSettings* aOverrides)
	{
	GetImpl()->SetOverridesL( aOverrides );
	}

EXPORT_C TBool CCommsDbAccess::IsShowingHiddenRecords()
	{
	return GetImpl()->IsShowingHiddenRecords();
	}

EXPORT_C void CCommsDbAccess::GetCurrentSettingsL(TConnectionSettings& aSettings, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Get default settings from overrides or database.
No checking that the default exists yet -
that's done in the SetCurrentSettingsL() function.
*/
	{
	GetImpl()->GetCurrentSettingsL( aSettings, aDirection, aRank );
	}

EXPORT_C void CCommsDbAccess::GetServiceSettingsL(TConnectionSettings& aSettings)
/**
Get service settings from database.
No checking that the default exists yet -
that's done in the SetCurrentSettingsL() function.
*/
	{
	GetImpl()->GetServiceSettingsL( aSettings );
	}


EXPORT_C void CCommsDbAccess::GetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Get IapId for the connection preference of ranking aRank.
Leaves if not found
*/
	{
	GetImpl()->GetPreferedIapL( aIapId, aDirection, aRank );
	}

EXPORT_C void CCommsDbAccess::SetModemAndLocationL(const TConnectionSettings& /*aSettings*/)
/**
Set the modem and location and check that relevant records exits in the database
rite the new values back into the database

MM.TSY only supports a single modem, it cannot drive separate
fax/data and phone/sms modems
Added a fix so if the fax/data modem uses MM.TSY and the phone/SMS
is some other modem also using MM.TSY, the phone/SMS modem will be
changed to the same value as the fax/data modem

Check if the fax/data modem uses MM.TSY
If not then thats ok so return and continue connection setup
Find out which modem is used for Phone/SMS services
If it is the same modem, again that's ok so return and continue setup
Check if the phone/sms modem uses MM.TSY
If not then thats ok so return and continue connection setup
So now we know fax/data and phone/sms use different modems but
both modems are driven by MM.TSY
To avoid a panic in MM.TSY, set the phone/sms modem to be the same as
the fax/data modem

*/

	{
	// DEPRECATED
	User::Leave(KErrNotSupported);
	}

EXPORT_C void CCommsDbAccess::SetCurrentSettingsL(const TConnectionSettings& aSettings)
/**
Set current settings to aSettings and check that relevant
records exist in the databases. If the settings have been changed
(not overrides) write the new values back to the database.
*/
	{
	GetImpl()->SetCurrentSettingsL( aSettings );
	}

EXPORT_C void CCommsDbAccess::SetPreferedIapL(TUint32& aIapId, TCommDbConnectionDirection aDirection, TUint32 aRank)
/**
Set current settings to aSettings and check that relevant
records exist in the databases. If the settings have been changed
(not overrides) write the new values back to the database.

*/
	{
	GetImpl()->SetPreferedIapL( aIapId, aDirection, aRank );
	}

EXPORT_C TBool CCommsDbAccess::DoesIapExistL(TUint32 aIapId)
/**
Check for the presence of an IAP of record number aIapId
*/
	{
	return GetImpl()->DoesIapExistL( aIapId );
	}

EXPORT_C void CCommsDbAccess::GetFirstValidIapL(TUint32& aIapId)
	{
	GetImpl()->GetFirstValidIapL( aIapId );
	}

EXPORT_C TBool CCommsDbAccess::IsTelNumLengthZeroForRasConnectionL(TConnectionSettings& aSettings)
/**
Check whether this connection is RAS connection. Default telephone
number lenght will be zero if it is, as RAS does not need to dial up.
Direct connection is part of CsdAgx and that's why we need to check
if the service setting is DIAL_OUT_ISP
*/
	{
	return GetImpl()->IsTelNumLengthZeroForRasConnectionL( aSettings );
	}

EXPORT_C TInt CCommsDbAccess::GetConnectionAttempts()
	{
	return GetImpl()->GetConnectionAttempts();
	}

EXPORT_C void CCommsDbAccess::GetBearerAvailabilityTsyNameL(TDes& aTsyName)
/**
Get the name of the TSY that should be used for bearer availability checking.
If this global setting is not found then just use the TSY of the current modem
*/
	{
	GetImpl()->GetBearerAvailabilityTsyNameL( aTsyName );
	}

EXPORT_C void CCommsDbAccess::GetTsyNameL(TDes& aTsyName)
/**
Get the TSY name from the override settings or from the current modem settings
*/
	{
	GetImpl()->GetTsyNameL( aTsyName );
	}

EXPORT_C void CCommsDbAccess::SetCommPortL(const RCall::TCommPort& aCommPort)
/**
Set the comm port from ETEL, so that we all use the same one for
dial up and dial in
*/
	{
	GetImpl()->SetCommPortL( aCommPort );
	}

EXPORT_C void CCommsDbAccess::GetServiceTypeL(TDes& aServiceType)
	{
	GetImpl()->GetServiceTypeL( aServiceType );
	}

void CCommsDbAccess::GetAuthParamsL(TBool& aPromptForAuth,TDes& aUsername,TDes& aPassword)
/**
Get boolean PromptForAuth and authentication name and password
Not valid for dial in ISP.
*/
	{
	GetImpl()->GetAuthParamsL( aPromptForAuth, aUsername, aPassword );
	}

void CCommsDbAccess::GetAgentExtL(const TDesC& aServiceType, TDes& aAgentExt)
	{
	GetImpl()->GetAgentExtL( aServiceType, aAgentExt );
	}

EXPORT_C void CCommsDbAccess::SetNetworkMode(const RMobilePhone::TMobilePhoneNetworkMode aNetworkMode)
/**
MobileIP: used for Mobile IP to know on what network we are on.
*/
	{
	GetImpl()->SetNetworkMode( aNetworkMode );
	}

EXPORT_C RMobilePhone::TMobilePhoneNetworkMode CCommsDbAccess::NetworkMode() const
/**
What type of network we are on?
*/
	{
	return GetImpl()->NetworkMode();
	}

EXPORT_C TInt CCommsDbAccess::ReadInt(const TDesC& aField, TUint32& aValue)
/**
Read the integer in the field aField of the database into aValue
*/
	{
	return GetImpl()->ReadInt( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::ReadBool(const TDesC& aField, TBool& aValue)
/**
Read the boolean in the field aField of the database into aValue
*/
	{
	return GetImpl()->ReadBool( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::ReadDes(const TDesC& aField, TDes8& aValue)
/**
Read the text in the field aField of the database into aValue
*/
	{
	return GetImpl()->ReadDes( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::ReadDes(const TDesC& aField, TDes16& aValue)
/**
Read the text in the field aField of the database into aValue
*/
	{
	return GetImpl()->ReadDes( aField, aValue );
	}

EXPORT_C HBufC* CCommsDbAccess::ReadLongDesLC(const TDesC& aField)
/**
Read the long text in the field aField of the database and return it
*/
	{
	return GetImpl()->ReadLongDesLC( aField );
	}

EXPORT_C TInt CCommsDbAccess::WriteInt(const TDesC& aField, TUint32 aValue )
	{
	return GetImpl()->WriteInt( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::WriteBool(const TDesC& aField, TBool aValue )
	{
	return GetImpl()->WriteBool( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::WriteDes(const TDesC& aField, const TDesC16& aValue )
	{
	return GetImpl()->WriteDes( aField, aValue );
	}

EXPORT_C TInt CCommsDbAccess::WriteDes(const TDesC& aField, const TDesC8& aValue )
	{
	return GetImpl()->WriteDes( aField, aValue );
	}

EXPORT_C void CCommsDbAccess::RequestNotificationOfServiceChangeL(MServiceChangeObserver* aObserver)
	{
	GetImpl()->RequestNotificationOfServiceChangeL( aObserver );
	}

EXPORT_C void CCommsDbAccess::CancelRequestNotificationOfServiceChange(MServiceChangeObserver* aObserver)
	{
	GetImpl()->CancelRequestNotificationOfServiceChange( aObserver );
	}

EXPORT_C void CCommsDbAccess::GetIntL(const TDesC& aTable, const TDesC& aField, TUint32& aValue)
	{
	GetImpl()->GetIntL( aTable, aField, aValue );
	}

EXPORT_C void CCommsDbAccess::GetBoolL(const TDesC& aTable, const TDesC& aField, TBool& aValue)
	{
	GetImpl()->GetBoolL( aTable, aField, aValue );
	}

EXPORT_C void CCommsDbAccess::GetDesL(const TDesC& aTable, const TDesC& aField, TDes8& aValue)
	{
	GetImpl()->GetDesL( aTable, aField, aValue );
	}

EXPORT_C void CCommsDbAccess::GetDesL(const TDesC& aTable, const TDesC& aField, TDes16& aValue)
	{
	GetImpl()->GetDesL( aTable, aField, aValue );
	}

EXPORT_C HBufC* CCommsDbAccess::GetLongDesLC(const TDesC& aTable, const TDesC& aField)
	{
	return GetImpl()->GetLongDesLC( aTable, aField );
	}

EXPORT_C TInt CCommsDbAccess::GetLengthOfLongDesL(const TDesC& aTable, const TDesC& aField)
	{
	return GetImpl()->GetLengthOfLongDesL( aTable, aField );
	}

EXPORT_C void CCommsDbAccess::GetGlobalL(const TDesC& aName,TUint32& aVal)
	{
	GetImpl()->GetGlobalL( aName, aVal );
	}

EXPORT_C CCommsDbAccess::CCommsDbAccessModemTable* CCommsDbAccess::ModemTable()
	{
	return GetImpl()->ModemTable();
	}

EXPORT_C TUint32 CCommsDbAccess::LocationId() const
	{
	return GetImpl()->LocationId();
	}

EXPORT_C TCommDbConnectionDirection CCommsDbAccess::GetConnectionDirection() const
	{
	return GetImpl()->GetConnectionDirection();
	}


 	/** Data capability checking */
EXPORT_C TInt CCommsDbAccess::CheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	return GetImpl()->CheckReadCapability( aField, aMessage );
	}

EXPORT_C TInt CCommsDbAccess::CheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	return GetImpl()->CheckWriteCapability( aField, aMessage );
	}

