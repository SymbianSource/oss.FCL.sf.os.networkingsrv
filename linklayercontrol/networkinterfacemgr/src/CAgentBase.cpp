// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// CAgentBase implementation.
// CAgentBase is the base class for an agent that accesses CommDb and uses overrides
// 
//

/**
 @file CAgentBase.cpp
 @publishedPartner
 @deprecated since v9.5. Use MCPR/CPR/SCPRs instead of agents. 
*/

#include <cdbcols.h>
#include "CAgentBase.h"
#include "AgentPanic.h"
#include "Ni_Log.h"
#include "IF_DEF.H"
#include "cagentdlgproc.h"

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <networking/cfbearers.h>
#endif //SYMBIAN_ADAPTIVE_TCP_WINDOW_RECEIVE


/**
Agent Panic
@internalComponent
*/
_LIT(KAgentPanic, "CAgentBase");

/**
Panic - programming error!
@internalComponent 
*/
GLDEF_C void AgentPanic(Agent::TAgentPanic aPanic)
/**
 * Global panic function for agent base classes.
 * A global helper function for agent base classes to log the panic number before triggering the panic itself
 * @internalComponent
 */
	{
	LOG( NifmanLog::Printf(_L("CAgentBase Panic %d"), aPanic); )
	User::Panic(KAgentPanic, aPanic);
	}


//
//  Construction and Destruction
//

EXPORT_C CAgentBase::~CAgentBase()
/**
Destructor
Releases resources used by CAgentBase

*/
	{
	if (iOverrides)
		{
		LOG_DETAILED( NifmanLog::Printf(_L("Agent %x:\t\tDeleting overrides at %x"), this, iOverrides); )

		delete iOverrides;
		}

	if (iDlgPrc)
		{
		iDlgPrc->CancelEverything();
		delete iDlgPrc;
		}

	if (iDatabase)
		{
		iDatabase->CancelRequestNotificationOfServiceChange(this);
		iServiceChangeNotification.Reset();
		delete iDatabase;
		}

	if(iAuthenticateCallback)
		delete iAuthenticateCallback;

	}

EXPORT_C CAgentBase::CAgentBase()
: iAuthenticateError(KErrNone)
/**
Default Constructor
*/
	{ }

EXPORT_C void CAgentBase::ConstructL()
/**
2nd Phase of construction.
Instantiate Member variables.
The derived class' ConstructL() should also call this one.

@exception Leaves if construction of either the database or 
the dialog processor leaves
*/
	{
	// construct the database
	iDatabase = CCommsDbAccess::NewL(ETrue);
	
	iDatabase->RequestNotificationOfServiceChangeL(this);

	// construct the dialog processor
	iDlgPrc = CDialogProcessor::NewL();

	// create the callback for authentication
	TCallBack authCallback(AuthenticateCb, this);
	iAuthenticateCallback = new (ELeave) CAsyncCallBack(authCallback, CActive::EPriorityStandard);
	}


//
//  Partial implementation of the CNifAgentBase interface
//

EXPORT_C void CAgentBase::SetConnectionSettingsL(const TConnectionSettings& aSettings)
/**
SetConnectionSettings

Used by Network Controller to set the database to
reflect the settings selected by the user

@param aSettings contains the connection settings selected by the user
@exception leaves if there is a problem with access to the database
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("Agent %x:\t\tSetConnectionSettingsL(aSettings = 0x%08x)"), this, &aSettings); )

	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	iSettings = aSettings;
	iDatabase->GetCurrentSettingsL(iSettings, iSettings.iDirection, iSettings.iRank);
	iDatabase->GetServiceSettingsL(iSettings);
	iDatabase->SetCurrentSettingsL(iSettings);

	LOG_DETAILED(
				NifmanLog::Printf(_L("Agent %x:\t\tNew Connection Settings:"), this);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiRank = %d"), this, aSettings.iRank);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiDirection = %d"), this, aSettings.iDirection);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiDialogPref = %d"), this, aSettings.iDialogPref);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiBearerSet = %d"), this, aSettings.iBearerSet);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiIAPId = %d"), this, aSettings.iIAPId);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiServiceId = %d"), this, aSettings.iServiceId);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiServiceType = '%S'"), this, &(aSettings.iServiceType));
				NifmanLog::Printf(_L("Agent %x:\t\t\tiBearerId = %d"), this, aSettings.iBearerId);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiLocationId = %d"), this, aSettings.iLocationId);
				NifmanLog::Printf(_L("Agent %x:\t\t\tiChargeCardId = %d"), this, aSettings.iChargeCardId);
				)
	}

EXPORT_C TConnectionSettings& CAgentBase::ConnectionSettingsL()
/**
@return iSettings containing the connection settings selected by the user
*/
	{
	return iSettings;
	}

EXPORT_C void CAgentBase::SetOverridesL(CStoreableOverrideSettings* aOverrideSettings)
/**
Transfer ownership of storeable override settings to this agent
@param aOverrideSettings 
@note that since ownership of the override settings is transfered
to this agent and they must be deleted in the d'tor
*/
	{
	LOG_DETAILED( NifmanLog::Printf(_L("Agent %x:\t\tSetOverridesL(aOverrideSettings = 0x%08x)"), this, aOverrideSettings); )

	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	if(iOverrides)
		{
		// we already have some overrides so free them before setting the new ones
		delete iOverrides;
		iOverrides = NULL;
		}

	iOverrides = aOverrideSettings;
	iDatabase->SetOverridesL(aOverrideSettings);

	// refresh settings
//	iDatabase->GetCurrentSettingsL(iSettings, iSettings.iDirection, iSettings.iRank);
//	iDatabase->SetCurrentSettingsL(iSettings);
	}

EXPORT_C CStoreableOverrideSettings* CAgentBase::OverridesL()
/**
Retrieve a pointer to the override settings owned by this agent

@return aOverrideSettings
@note that ownership of the override settings remains
with this agent and they must not be deleted elsewhere
*/
	{
	return iOverrides;
	}

EXPORT_C void CAgentBase::Reconnect()
/**
Request that the State Machine re-establishes a connection.
*/
	{
	CNifMan::Global()->AgentDialogProcessor()->PromptForReconnect(*this);
	}

EXPORT_C void CAgentBase::CancelReconnect()
/**
Cancel a previous request to Reconnect.
*/
	{
	CNifMan::Global()->AgentDialogProcessor()->CancelPromptForReconnect(*this);
	}

EXPORT_C void CAgentBase::Authenticate(TDes& aUsername, TDes& aPassword)
/**
A request from the NIF to Authenticate the User

@param aUserName on return will contain the User Name
@param aPassword on return will contain the Password
@note Although the username and password are returned by this
function the NIF should not use either parameter until it receives
the AuthenticateComplete() signal.
*/
	{
	__ASSERT_DEBUG(iDlgPrc, AgentPanic(Agent::ENullDialogProcessor));
	
	iAuthenticateError = ReadDes(TPtrC(SERVICE_IF_AUTH_NAME), aUsername);
	if (iAuthenticateError!=KErrNone)
		{
		iAuthenticateCallback->CallBack();
		return;
		}

	iAuthenticateError = ReadDes(TPtrC(SERVICE_IF_AUTH_PASS), aPassword);
	if (iAuthenticateError!=KErrNone)
		{
		iAuthenticateCallback->CallBack();
		return;
		}

	TBool promptForAuth(EFalse);
	iAuthenticateError = ReadBool(TPtrC(SERVICE_IF_PROMPT_FOR_AUTH), promptForAuth);
	if (iAuthenticateError!=KErrNone)
		{
		iAuthenticateCallback->CallBack();
		return;
		}

	// at this point all of the authentication parameters have been successfully read from the database
	if (promptForAuth)
	    {
		iDlgPrc->Authenticate(*this, aUsername, aPassword, IsReconnect());
	    }
    else
        {
    	iAuthenticateCallback->CallBack();
        }
	}

EXPORT_C void CAgentBase::CancelAuthenticate()
/**
A request from the NIF to cancel authentication
*/
	{
	__ASSERT_DEBUG(iDlgPrc, AgentPanic(Agent::ENullDialogProcessor));

	iDlgPrc->Cancel();
	iAuthenticateCallback->Cancel();
	}

EXPORT_C TBool CAgentBase::IsActive() const
/**
Is this Agent in use?

@return ETrue always
@todo RC to investigate use of IsActive()
*/
	{
	return ETrue;
	}

//
//  Access to database settings
//

EXPORT_C TInt CAgentBase::DoReadInt(const TDesC& aField, TUint32& aValue,const RMessagePtr2* aMessage)
/**
Read an integer value from the database

@param   aField the name of the field to read from
@param   aValue on return, contains the value of the field
@return KErrNotFound if the field is not present
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckReadCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->ReadInt( aField, aValue );
		}

#if defined (__FLOG_ACTIVE)

	if (ret==KErrNone)
		NifmanLog::Printf(_L("Agent %x:\t\tReadInt(aField = '%S', aValue = %d)"), this, &aField, aValue);
	else
		NifmanLog::Printf(_L("Agent %x:\t\tReadInt(aField = '%S', aValue) returning error %d"), this, &aField, ret);

#endif

	return ret;
	}

EXPORT_C TInt CAgentBase::DoWriteInt(const TDesC& aField, TUint32 aValue,const RMessagePtr2* aMessage)
/**
Write an integer value to the database

@param   aField the name of the field to write to
@param   aValue contains the value of the field
@return KErrNotFound if the field is not present
        KErrNotSupported if the operation is not available
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckWriteCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->WriteInt( aField, aValue );
		}
	return ret;
	}

EXPORT_C TInt CAgentBase::DoReadDes(const TDesC& aField, TDes8& aValue,const RMessagePtr2* aMessage)
/**
Read a descriptor value from the database

@param   aField the name of the field to read from
@param   aValue on return, contains the value of the field
@return KErrNotFound if the field is not present
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckReadCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->ReadDes(aField, aValue);
		}


#if defined (__FLOG_ACTIVE)

	if (ret==KErrNone)
		NifmanLog::Printf(_L("Agent %x:\t\tReadDes(aField = '%S', aValue = %S)"), this, &aField, &aValue);
	else
		NifmanLog::Printf(_L("Agent %x:\t\tReadDes(aField = '%S', aValue) returning error %d"), this, &aField, ret);

#endif

	return ret;
	}

EXPORT_C TInt CAgentBase::DoWriteDes(const TDesC& aField, const TDesC8& aValue,const RMessagePtr2* aMessage)
/**
Write a descriptor value to the database

@param   aField the name of the field to write to
@param   aValue contains the value of the field
@return KErrNotFound if the field is not present
         KErrNotSupported if the operation is not available
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckWriteCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->WriteDes(aField, aValue);
		}
	return ret;
	}

EXPORT_C TInt CAgentBase::DoReadDes(const TDesC& aField, TDes16& aValue,const RMessagePtr2* aMessage)

/**
Read a descriptor value from the database

@param   aField the name of the field to read from
@param   aValue on return, contains the value of the field
@return KErrNotFound if the field is not present
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckReadCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->ReadDes(aField, aValue);
		}


#if defined (__FLOG_ACTIVE)

	if (ret==KErrNone)
		NifmanLog::Printf(_L("Agent %x:\t\tReadDes(aField = '%S', aValue = '%S')"), this, &aField, &aValue);
	else
		NifmanLog::Printf(_L("Agent %x:\t\tReadDes(aField = '%S', aValue) returning error %d"), this, &aField, ret);

#endif

	return ret;
	}

EXPORT_C TInt CAgentBase::DoWriteDes(const TDesC& aField, const TDesC16& aValue,const RMessagePtr2* aMessage)
/**
Write a descriptor value to the database

@param   aField the name of the field to write to
@param   aValue contains the value of the field
@return KErrNotFound if the field is not present
         KErrNotSupported if the operation is not available
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckWriteCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->WriteDes(aField, aValue);
		}
	return ret;
	}

EXPORT_C TInt CAgentBase::DoReadBool(const TDesC& aField, TBool& aValue,const RMessagePtr2* aMessage)
/**
Read a Boolean value from the database

@param   aField the name of the field to read from
@param   aValue on return, contains the value of the field
@return KErrNotFound if the field is not present
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckReadCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->ReadBool(aField, aValue);
		}


#if defined (__FLOG_ACTIVE)

	if (ret==KErrNone)
		NifmanLog::Printf(_L("Agent %x:\t\tReadBool(aField = '%S', aValue = %d)"), this, &aField, (TInt)aValue);
	else
		NifmanLog::Printf(_L("Agent %x:\t\tReadBool(aField = '%S', aValue) returning error %d"), this, &aField, ret);

#endif

	return ret;
	}

EXPORT_C TInt CAgentBase::DoWriteBool(const TDesC& aField, TBool aValue,const RMessagePtr2* aMessage)
/**
Write a Boolean value to the database

@param   aField the name of the field to write to
@param   aValue contains the value of the field
@return KErrNotFound if the field is not present
         KErrNotSupported if the operation is not available
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	TInt ret = CheckWriteCapability( aField, aMessage );
	if( KErrNone == ret )
		{
		ret = iDatabase->WriteBool(aField, aValue);
		}
	return ret;

	}

EXPORT_C HBufC* CAgentBase::DoReadLongDesLC(const TDesC& aField,const RMessagePtr2* aMessage)
/**
Read a long descriptor value from the database

Leaves if the allocation of the HBufC fails

@param   aField the name of the field to read from
@return a pointer to the HBufC containing the data
*/
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));

	LOG_DETAILED( NifmanLog::Printf(_L("Agent %x:\t\tReadLongDesLC(aField = '%S')"), this, &aField); )

	User::LeaveIfError( CheckReadCapability( aField, aMessage ) );
	return iDatabase->ReadLongDesLC(aField);

	}


//
//  Service Change Notification
//

EXPORT_C void CAgentBase::RequestNotificationOfServiceChangeL(MAgentSessionNotify* aSession)
/**
A request for service change notification

@param aSession the session interested in the notification
@exception Leaves if aSession cannot be added to the container
*/
	{
	// Storing who asked to be Notified for ServiceChangeNotification
	User::LeaveIfError(iServiceChangeNotification.Append(aSession));
	}

EXPORT_C void CAgentBase::CancelRequestNotificationOfServiceChange(MAgentSessionNotify* aSession)
/**
Cancel a previous request for service change notification

@param aSession the session wishing to cancel the notification
*/
	{
	const TInt result = iServiceChangeNotification.Find(aSession);

	if (result != KErrNotFound)
		{
		iServiceChangeNotification.Remove(result);
		}
	}

EXPORT_C void CAgentBase::ServiceChangeNotification(TUint32 aId, const TDesC& aType)
/**
Notification of service change.
This is the method through which the Agent receive notification about
Service Change events and Notify about it who asked for that.

@param aId New ISP
@param aType New Service Type
*/
	{
	const TInt count = iServiceChangeNotification.Count();

	for (TInt i = 0; i < count; ++i)
		{
		iServiceChangeNotification[0]->ServiceChangeNotification(aId, aType);
		iServiceChangeNotification.Remove(0);
		}
	}


//
//  Callbacks used for authentication
//

TInt CAgentBase::AuthenticateCb(TAny* aThisPtr)
/**
AuthenticateCb

@param aThisPtr pointer to the instance object that triggered the callback
@return KErrNone
*/
	{
	CAgentBase* self = (CAgentBase*)aThisPtr;

	__ASSERT_DEBUG(self->iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));
	
	LOG( NifmanLog::Printf(_L("Agent %x:\t\tAuthenticate callback complete"), self); )

	self->iNotify->AuthenticateComplete(self->iAuthenticateError);
	return KErrNone;
	}

EXPORT_C void CAgentBase::MDPOAuthenticateComplete(TInt aError)
/**
Authenticate dialog box has completed

@param   aError 
*/
	{
	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	iNotify->AuthenticateComplete(aError);
	}


//
//  Implementation of MNetworkControllerObserver interface
//

EXPORT_C void CAgentBase::SelectComplete(const TDesC&)
/**
Panics the current thread with a USER 0 panic. Typically, this is called 
when a test for a class invariant fails, i.e. when a test which checks 
that the internal data of an object is self-consistent, fails.

*/
	{
	User::Invariant();
	}

EXPORT_C void CAgentBase::SelectComplete(TInt)
/**
@see EXPORT_C void CAgentBase::SelectComplete(const TDesC&)
*/
	{
	User::Invariant();
	}

EXPORT_C void CAgentBase::ReconnectComplete(const TInt aError)
/**
Reconnect dialog box has completed

*/
	{
	__ASSERT_DEBUG(iNotify, AgentPanic(Agent::ENullNifmanNotifyPointer));

	iNotify->ReconnectComplete(aError);
	}

EXPORT_C TInt CAgentBase::DoCheckReadCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));
	return iDatabase->CheckReadCapability( aField, aMessage );
	}

EXPORT_C TInt CAgentBase::DoCheckWriteCapability( const TDesC& aField, const RMessagePtr2* aMessage )
	{
	__ASSERT_DEBUG(iDatabase, AgentPanic(Agent::ENullDatabase));
	return iDatabase->CheckWriteCapability( aField, aMessage );
	}
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Retrieve the default bearer information to the Agent CPR
@return default bearer information to the Agent CPR.
*/
EXPORT_C TUint32 CAgentBase::GetBearerInfo() const
	{
	//Return the default value, if the agent
	//has not overridden this function
	return  KDefaultBearer;
	}
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

