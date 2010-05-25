// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalComponent
*/


#include <cdbcols.h>

#include "CAgentAdapter.h"
#include "agentscpr.h"
#include "IF_DEF.H"
#include <comms-infras/ss_platsec_apiext.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#include <comms-infras/nifagt_internal.h>
#endif

#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCCAgn, "NifManAgtPrCCAgn");
#endif

typedef CNifFactory* (*TAgentFactoryNewL)();


#ifdef __CFLOG_ACTIVE
#define KAgentSCprTag KESockSubConnectionTag
_LIT8(KAgentSCprSubTag, "agentscpr");
#endif



/**
Create a CAgentAdapter and Agent (loading the .agt library if necessary)
@param aAgentScpr The CAgentSubConnectionProvider that is to be called upon when
notifications are received from the agent
@param aAgentName The name of the agent to load/create for this CAgentAdaptor
*/
CAgentAdapter* CAgentAdapter::NewL(CAgentSubConnectionProvider& aAgentScpr, const TDesC& aAgentName)
    {
    CAgentAdapter* self = new(ELeave) CAgentAdapter (aAgentScpr);
    CleanupStack::PushL(self);
    self->CreateAgentL(aAgentName);
    CleanupStack::Pop();
    return self;
    }


/**
CAgentAdapter C'tor
@param aAgentScpr The CAgentSubConnectionProvider that is to be called upon when
notifications are received from the agent
*/
CAgentAdapter::CAgentAdapter(CAgentSubConnectionProvider& aAgentScpr)
    : iAgentScpr(aAgentScpr),
    iAgentState(EDisconnected),
    iAgentConnectType(EAgentNone),
    iLastProgress(KFinishedSelection,KErrNone)
    {
    }


/**
CAgentAdapter D'tor
*/
CAgentAdapter::~CAgentAdapter()
    {
    if ( iAgent )
    	{
    	// Delete the Agent
    	delete iAgent;

    	// Clean the Queue of AgentAdapterSessionNotifier still pending.
    	// At this stage the Agent has only removed them from the queue
    	// of pending service change notification.
    	// We need to explicitly call "CancelServiceChangeNotification" due
    	// the completelly different architecture post-399.
    	const TInt count = iAgentAdapterSessionNotifiers.Count();
    	for (TInt i = 0; i < count; ++i)
	    	{
	    	CAgentAdapterSessionNotifier* currentSessNotifier =	iAgentAdapterSessionNotifiers[0];
	    	iAgentAdapterSessionNotifiers.Remove(0);
	    	// The Destruction of an AgentAdapterSessionNotifier force it to
	    	// authomatically notify with KErrCancel node waiting for
	    	// Service Change Notification
	    	delete currentSessNotifier;
	    	}
	    iAgentAdapterSessionNotifiers.Reset();
    	}

    if (iFactory)
        {
        iFactory->Close();		// do this after deleting iAgent
        }
    }


void CAgentAdapter::PromptForReconnect()
    {
    iAgentState = EReconnecting;
    iAgent->Reconnect();
    }

/**
Begins connection of the Agent (called after SCPr receives BindTo and has
Joined the CAgentAdapter)
*/
void CAgentAdapter::ConnectAgent(TAgentConnectType aConnectType)
    {
    if (aConnectType == EAgentReconnect)
        {
        iAgentConnectType = aConnectType;
        iAgentState = EReconnecting;
        iAgent->Connect(aConnectType);
        }
    else
        {
        if (iAgentState != EConnecting || (
            aConnectType == EAgentStartCallBack && iAgentConnectType != EAgentStartCallBack))
            {
            iAgentConnectType = aConnectType;
            iAgentState = EConnecting;
            iAgent->Connect(aConnectType);
            }
        }
    }


/**
Begins the disconnection of the Agent
*/
void CAgentAdapter::DisconnectAgent(TInt aReason)
    {
    if (iAgentState == EConnecting)
        {
        iAgent->CancelConnect();
        }
    else if (iAgentState == EReconnecting)
        {
        iAgent->CancelReconnect();
        }
    // Issue disconnect only if agent is not already disconneting state
    else if (iAgentState != EDisconnecting)
        {	
        iAgentState = EDisconnecting;
        iAgent->Disconnect(aReason);
        }
    }


/**
Starts the process of obtaining authentication information via the Agent
(called after the SCPr receives an Authentication request from the SCF)
*/
void CAgentAdapter::Authenticate(TDes& aUsername, TDes& aPassword)
    {
    iAgent->Authenticate(aUsername, aPassword);
    }


/**
Stops the process of obtaining authentication information via the Agent
*/
void CAgentAdapter::CancelAuthenticate()
    {
    iAgent->CancelAuthenticate();
    }




// ---------------- Minimal database access methods ----------------

/**
Get the port name for NIFs such as the PPP NIF.
@param aPortName - After a successful call contains the port name from the agent
*/
TInt CAgentAdapter::ReadPortName (TDes8& aPortName)
    {
    _LIT(KModemPortName, "ModemBearer\\PortName");
    return iAgent->ReadDes(KModemPortName, aPortName);
    }


/**
Get the port name for NIFs such as the PPP NIF.
@param aPortName - After a successful call contains the port name from the agent
*/
TInt CAgentAdapter::ReadPortName (TDes16& aPortName)
    {
    _LIT(KModemPortName, "ModemBearer\\PortName");
    return iAgent->ReadDes(KModemPortName, aPortName);
    }


/**
Gets excess data for NIFs such as the PPP NIF.
@param aBuffer - After a successful call contains excess data from the agent
*/
TInt CAgentAdapter::ReadExcessData(TDes8& aBuffer)
    {
    return iAgent->GetExcessData(aBuffer);
    }


/**
Gets the Interface Parameters for NIFs.
@param aIfParams - After a successful call contains interface parameters from the agent
*/
TInt CAgentAdapter::ReadIfParams(TDes8& aIfParams)
    {
    return iAgent->ReadDes(TPtrC(SERVICE_IF_PARAMS), aIfParams);
    }


/**
Gets the Interface Parameters for NIFs.
@param aIfParams - After a successful call contains interface parameters from the agent
*/
TInt CAgentAdapter::ReadIfParams(TDes16& aIfParams)
    {
    return iAgent->ReadDes(TPtrC(SERVICE_IF_PARAMS), aIfParams);
    }


/**
Gets the Interface Networks for NIFs.
@param aNetworks - After a successful call contains the network layer protocols
*/
TInt CAgentAdapter::ReadIfNetworks(TDes8& aIfNetworks)
    {
    return iAgent->ReadDes(TPtrC(SERVICE_IF_NETWORKS), aIfNetworks);
    }


/**
Gets the Interface Networks for NIFs.
@param aNetworks - After a successful call contains the network layer protocols
*/
TInt CAgentAdapter::ReadIfNetworks(TDes16& aIfNetworks)
    {
    return iAgent->ReadDes(TPtrC(SERVICE_IF_NETWORKS), aIfNetworks);
    }


/**
Gets the name of the NIF (Flow)
@param aNifName - After a successful call contains the NIF (Flow) Name
*/
TInt CAgentAdapter::ReadNifName(TDes8& aNifName)
    {
    return iAgent->ReadDes(TPtrC(IF_NAME), aNifName);
    }


/**
Gets the name of the NIF (Flow)
@param aNifName - After a successful call contains the NIF (Flow) Name
*/
TInt CAgentAdapter::ReadNifName(TDes16& aNifName)
    {
    return iAgent->ReadDes(TPtrC(IF_NAME), aNifName);
    }


TInt CAgentAdapter::NotificationToAgent(TFlowToAgentEventType aEvent, TAny* aInfo)
    {
    return iAgent->Notification(static_cast<TNifToAgentEventType>(aEvent), aInfo);
    }


// ---------------- MNifAgentNotify Methods ----------------

/**
Agent has started - The SCPR should send BindTo information to the SCF
*/
void CAgentAdapter::ConnectComplete(TInt aStatus)
    {
    if (iAgentState == EConnecting)
        {
        if (aStatus == KErrNone)
            {
            iAgentState = EConnected;
#ifdef __CFLOG_ACTIVE
            TRAPD(err, iAgentScpr.ConnectionUpL());
            if (err != KErrNone)
                {
                __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter::ConnectComplete() - Trapped [err=%d]"), err));
                __ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 1));
                }
#else
			TRAP_IGNORE(iAgentScpr.ConnectionUpL());
#endif
            }
        else
            {
            iLastProgress.iError = aStatus;
            // Set the agent state to EDisconnecting
            iAgentState = EDisconnecting;
            iAgent->Disconnect(aStatus);
            }
        }
   }


/**
Authentication completed by Agent
*/
void CAgentAdapter::AuthenticateComplete(TInt aStatus)
    {
#ifdef __CFLOG_ACTIVE
	TRAPD(err, iAgentScpr.AuthenticateCompleteL(aStatus));
    if (err != KErrNone)
        {
        __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter::AuthenticateComplete() - Trapped [err=%d]"), err));
        __ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 2));
        }
#else
	TRAP_IGNORE(iAgentScpr.AuthenticateCompleteL(aStatus));
#endif
    }


/**
Agent has stopped
*/
void CAgentAdapter::DisconnectComplete()
    {
    if (iAgentState == EConnecting)
        {
        if (iLastProgress.iError != KErrNone)
	        {
	        iAgentScpr.Error(iLastProgress);
	        }
        iAgentState = EDisconnected;
        }
    else
        {
#ifdef __CFLOG_ACTIVE
    	TRAPD(err,
    		iAgentState = EDisconnected;
    		iAgentScpr.ConnectionDownL();
    	//	iAgentScpr.ProgressL(KConnectionUninitialised);
    		);
            if (err != KErrNone)
                {
                __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter::DisconnectComplete() - Trapped [err=%d]"), err));
                __ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 3));
                }
#else
    	TRAP_IGNORE(
    		iAgentState = EDisconnected;
    		iAgentScpr.ConnectionDownL();
            //iAgentScpr.ProgressL(KConnectionUninitialised);
    		);
#endif
        }
   }


/**
Progress notifications from the Agent Connect()
*/
void CAgentAdapter::AgentProgress(TInt aStage, TInt aError)
    {
    iLastProgress.iStage = aStage;
    iLastProgress.iError = aError;
    if (aError == KErrNone)
        {
#ifdef __CFLOG_ACTIVE
		TRAPD(err, iAgentScpr.ProgressL(aStage));
        if (err != KErrNone)
            {
            __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter:\tAgentProgress() - Trapped [err=%d]"), err));
            __ASSERT_DEBUG(EFalse, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 4));
            }
#else
		TRAP_IGNORE(iAgentScpr.ProgressL(aStage));
#endif
        }
    else
        {
        iAgentScpr.Error(iLastProgress);
        }
    }


/**
Notification from the Agent to the Nif (Flow)
*/
TInt CAgentAdapter::Notification(TAgentToNifEventType aEvent, TAny* aInfo /*=NULL*/)
   {
   // Let the SCPR decide what to do with the notification
   return iAgentScpr.NotificationFromAgent(static_cast<TAgentToFlowEventType>(aEvent), aInfo);
   }


/**
Event notification from the Agent
(Only known use of this is in bluetooth panagt.cpp)
*/
void CAgentAdapter::AgentEvent(TNetworkAdaptorEventType aEventType, TUint aEvent, const TDesC8& aEventData, TAny* aSource /*=NULL*/)
   {
   // Let the SCPR decide what to do with the event
   iAgentScpr.NetworkAdaptorEvent(aEventType, aEvent, aEventData, aSource);
   }


/**
Traditionally reconnection would be initiated from the CNifAgentRef and handled
by NetCon. NetCon would call this method via the ReconnectComplete() in the
CAgentBase. This represents the completion of the prompting of the user for a
decision to be made on whether or not to reconnect, not the actual reconnection
*/
void CAgentAdapter::ReconnectComplete(TInt aStatus)
    {
    iAgentScpr.PromptForReconnectComplete(aStatus);
    }


/**
Agent service started (message from agent)
Traditionally the CNifAgentRef implementation would load the NIF here.
*/
void CAgentAdapter::ServiceStarted()
   {
   iAgentScpr.ServiceStarted();
   }




// ---------------- Empty methods to satisfy interfaces ----------------

/**
Traditionally called by the CNifAgentRef as a result of a DisconnectComplete,
deletion of the the object, or failure to bind. The method would have deleted
the NIF. We do nothing here.
*/
void CAgentAdapter::ServiceClosed()
   {
   // Should never be called
   __CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter:\tServiceClosed(): Nothing to do here. Should never be called.")));
   }


/**
The functionality provided by this method has never been used.
*/
void CAgentAdapter::AgentProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
   {
   // Should never be called
   __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 5));
   }


/**
Incoming connections not supported?
*/
TInt CAgentAdapter::IncomingConnectionReceived()
   {
   // Should never be called
   __ASSERT_DEBUG(0, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 6));
   return KErrNotSupported;
   }




// ---------------- Agent Creation / Lookup Methods ----------------

/**
Load a factory and check the Uid etc
function always opens factory CObject if successful
*/
CNifFactory* CAgentAdapter::FindOrCreateAgentFactoryL(const TDesC& aFilename)
	{
    #ifdef _UNICODE
    TUid agentUid(TUid::Uid(KUidUnicodeNifmanAgent));
    #else
    TUid agentUid(TUid::Uid(KUidNifmanAgent));
    #endif


    // Make sure NifMan is installed
    Nif::CheckInstalledL();

    CObjectCon& factoryContainer = *(CNifMan::Global()->iAgentFactories);

	TParse parse;
	User::LeaveIfError(parse.Set(aFilename, 0, 0));

	TName dummy1;
	TInt find=0;
	CNifFactory* factory(NULL);

	if (factoryContainer.FindByName(find, parse.Name(), dummy1) != KErrNone)
        {
        // Load the module
        TAutoClose<RLibrary> lib;
        User::LeaveIfError(lib.iObj.Load(aFilename));
        lib.PushL();

		// The Uid check
		if (lib.iObj.Type()[1] != agentUid)
            {
            User::Leave(KErrBadLibraryEntryPoint);
            }

		TAgentFactoryNewL libEntry = (TAgentFactoryNewL)lib.iObj.Lookup(1);
		if (libEntry==NULL)
            {
            User::Leave(KErrNoMemory);
            }

		factory =(*libEntry)(); // Opens CObject
		if (!factory)
            {
            User::Leave(KErrBadDriver);
            }

		CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, factory));
		factory->InitL(lib.iObj, factoryContainer); // Transfers the library object if successful

		// Can pop the library now - auto close will have no effect because handle is null
		CleanupStack::Pop();
		lib.Pop();
		}
	else
		{
		factory = (CNifFactory*)factoryContainer.At(find);
		factory->Open(); // Add to ref count
		}

	return factory;
	}


/**
Create an agent
*/
void CAgentAdapter::CreateAgentL(const TDesC& aName)
	{
	__ASSERT_DEBUG(iAgent == NULL, User::Panic(KSpecAssert_NifManAgtPrCCAgn, 7));
	if (aName.Length() == 0)
		{
#ifdef __CFLOG_ACTIVE
		__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter:\tCreateAgentL(): Error - null agent name")));
#endif
		User::Leave(KErrCorrupt);
		}

	_LIT(KAgentExtension, ".agt");
	TFileName module(aName);
	if (module.Right(4).CompareF(KAgentExtension) != 0)
	   {
		module.Append(KAgentExtension);
		}

	// Try to find module in existing factories

	iFactory = static_cast<CNifAgentFactory*>(FindOrCreateAgentFactoryL(module));

	// Got the factory - now get the agent
	module = module.Left(module.Length() - 4);
    iAgent = iFactory->NewAgentL(module);
	iAgent->iNotify = this;

    TConnectionSettings settings;
    settings.iRank = 1;
	settings.iDirection = ECommDbConnectionDirectionOutgoing;

	const CAgentProvisionInfo& api = static_cast<const CAgentProvisionInfo&>(iAgentScpr.AccessPointConfig().FindExtensionL(
	        CAgentProvisionInfo::TypeId()));
	settings.iIAPId = api.IapId();
	settings.iBearerSet = api.BearerSet();
	settings.iDialogPref = ECommDbDialogPrefDoNotPrompt;

	CCommsDbConnectionPrefTableView::TCommDbIapBearer bearer;
	bearer.iIapId = settings.iIAPId;
	bearer.iBearerSet = settings.iBearerSet;

	CCommsDbConnectionPrefTableView::TCommDbIapConnectionPref pref;
	pref.iRanking = settings.iRank;
	pref.iDirection = settings.iDirection;
	pref.iDialogPref = settings.iDialogPref;
	pref.iBearer = bearer;

	CStoreableOverrideSettings* overrides = CStoreableOverrideSettings::NewL(CStoreableOverrideSettings::EParamListPartial);
	CleanupStack::PushL(overrides);
	User::LeaveIfError(overrides->SetConnectionPreferenceOverride(pref));
	iAgent->SetOverridesL(overrides);	//Ownership is passed to Agent
	CleanupStack::Pop(overrides);

	iAgent->SetConnectionSettingsL(settings);
	}


TInt CAgentAdapter::Control(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, ESock::MPlatsecApiExt* aPlatsecItf)
	{
	if(!aPlatsecItf->HasCapability(ECapabilityNetworkControl))
		{
		return KErrPermissionDenied;
		}

	return iAgent->Control(aOptionLevel, aOptionName, aOption);
	}


void CAgentAdapter::ClientAttachControl()
	{
	TBuf8<1> unusedDummy; // Not used by the agent.
	iAgent->Control(KCOLAgent, KCOAgentNotifyClientAttach, unusedDummy); // Error is ignored.
	}

void CAgentAdapter::RequestServiceChangeNotificationL(
		const Messages::TNodeId& aSender,
		ESock::RLegacyResponseMsg& aResponse)
/**
 * Routing the RequestServiceChangeNotification to the Agent
 */
	{
	__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter [this=%08x]: MLinkCprServiceChangeNotificationApiExt::RequestServiceChangeNotificationL()"), this));

	// An AgentAdapterSessionNotifier to Handle the Notifications
	CAgentAdapterSessionNotifier* agentSessNotifier =
		CAgentAdapterSessionNotifier::NewL(this, aSender, aResponse);
	// Store the CAgentAdapterSessionNotifier here
	iAgentAdapterSessionNotifiers.Append(agentSessNotifier);
	// Pass the CAgentAdapterSessionNotifier to the Agent to generate notifications
	iAgent->RequestNotificationOfServiceChangeL(agentSessNotifier);
	}

TBool CompareSessionNotifiers(const CAgentAdapterSessionNotifier& aLeft, const CAgentAdapterSessionNotifier& aRight)
/**
 Compares two objects of class T using the equality operator defined for class T.
 */
     {return aLeft == aRight;}


void CAgentAdapter::CancelServiceChangeNotification(const Messages::TNodeId& aSender)
/**
 * Routing the CancelServiceChangeNotification to the Agent
 */
	{
	__CFLOG_VAR((KAgentSCprTag, KAgentSCprSubTag, _L8("CAgentAdapter [this=%08x]: MLinkCprServiceChangeNotificationApiExt::CancelServiceChangeNotification()"), this));

	// Create an "empty" AgentAdapterSessionNotifier
	// (it does not have any RResponseMsg)
	CAgentAdapterSessionNotifier* emptyAgentSessNotifier =
		CAgentAdapterSessionNotifier::NewL(this, aSender);
	// Create a Relation to use to find the "good one"

    TIdentityRelation<CAgentAdapterSessionNotifier> identityRelation(CompareSessionNotifiers);
	TInt result = iAgentAdapterSessionNotifiers.Find(emptyAgentSessNotifier,
                                                     identityRelation);
	if ( KErrNotFound != result )
		{
		// Retrieve it
		CAgentAdapterSessionNotifier* agentSessNotifier =
			iAgentAdapterSessionNotifiers[result];
		// Send it to the Agent to Cancel/Remove it
		iAgent->CancelRequestNotificationOfServiceChange(agentSessNotifier);
		// Remove it from inside CAgentAdapter
		iAgentAdapterSessionNotifiers.Remove(result);
		// Delete the Agent Session Notifier (causing to send a KErrCancel
		delete agentSessNotifier;
		}
	// We don't need this anymore
	delete emptyAgentSessNotifier;
	}

// ---------- CAgentAdapter::CAgentAdapterSessionNotifier ----------

/**
 * Private Constructor to realize Phase #1 Construction.
 */
CAgentAdapterSessionNotifier::CAgentAdapterSessionNotifier (
		CAgentAdapter* aCreator,
		const Messages::TNodeId& aSender)
	: iSender(aSender), iResponseMsg(NULL), iCreator(aCreator)
    {
    }

void CAgentAdapterSessionNotifier::ConstructL (ESock::RLegacyResponseMsg& aResponseMsg)
/**
 * Phase #2 Constructor.
 * It stores a copy the original RResponseMsg to allow to answer "later"
 * to the request of Service Change Notification.
 */
		{
		iResponseMsg = new (ELeave) ESock::RLegacyResponseMsg(aResponseMsg);
		}

CAgentAdapterSessionNotifier* CAgentAdapterSessionNotifier::NewL(
		CAgentAdapter* aCreator,
		const Messages::TNodeId& aSender,
		ESock::RLegacyResponseMsg& aResponseMsg)
/**
 * 2-Phase Static Constructor.
 *
 * @param aCreator Creator of this AgentAdapterSessionNotifier
 * @param aSender The Node which asked to be notified for Serv.Change Notif.
 * @param aResponseMsg Allows to send back the Answer
 */
	{
	CAgentAdapterSessionNotifier* self =
		new (ELeave) CAgentAdapterSessionNotifier(aCreator, aSender);
	CleanupStack::PushL(self);
	self->ConstructL(aResponseMsg);
	CleanupStack::Pop(self);
	return self;
	}

CAgentAdapterSessionNotifier* CAgentAdapterSessionNotifier::NewL(
		CAgentAdapter* aCreator,
		const Messages::TNodeId& aSender)
/**
 * 2-Phase Static Constructor.
 * This constructor builds an "empty" AgentAdapterSessionNotifier. It does
 * not contain a RResponseMsg. In general, not supposed to be used.
 *
 * @param aCreator Creator of this AgentAdapterSessionNotifier
 * @param aSender The Node which asked to be notified for Serv.Change Notif.
 */
	{
	// Don't need to call the Second-Phase ConstructL: iResponse is
	// already initialized to NULL
	return new (ELeave) CAgentAdapterSessionNotifier(aCreator, aSender);
	}

CAgentAdapterSessionNotifier::~CAgentAdapterSessionNotifier ()
/**
 * Destructor.
 * It also complete the internal RResponseMsg returning "KErrCancel"
 * if it was not done before.
 * */
	{
	// In case no one called "CancelServiceChangeNotification",
	// we avoid that who asked for ServiceChangeNotification
	// hold on undefinitely.
	CancelServiceChangeNotification();
	}

TBool CAgentAdapterSessionNotifier::operator== (const CAgentAdapterSessionNotifier& aOtherInstanceOfThisClass) const
/**
 * Operator to allow the usage of Find(...) inside a R(Pointer)Array container
 * @return "ETrue" if the comparison is Positive; otherwise "EFalse"
 */
	{
	return NodeId() == aOtherInstanceOfThisClass.NodeId();
	}

const Messages::TNodeId& CAgentAdapterSessionNotifier::NodeId() const
/** Return an Id that identify this Agent Session
 * @return A TInt as Id */
	{
	return iSender;
	}

void CAgentAdapterSessionNotifier::ServiceChangeNotification(
		TUint32 aId, const TDesC& aType)
/**
 * Implementation of MAgentSessionNotify interface.
 * This send back the ServiceChangeNotification to the Client side, updating
 * the Id and the Type.
 *
 * @param aId New ISP
 * @param aType New Service Type
 * */
	{
	// Notify Nodes who asked to be so:
	// 1) Write the informations in the Message
	TPckgC<TUint32> id(aId);
	iResponseMsg->WriteL(0, id);
	iResponseMsg->WriteL(1, aType);
	// 2) Notify it
	iResponseMsg->Complete(KErrNone);
	delete iResponseMsg;
	iResponseMsg = NULL;

	// Find itself and remove from the Array in the AgentAdapter
	TInt result = iCreator->iAgentAdapterSessionNotifiers.Find(this);
	if ( KErrNotFound != result )
		{
		// Remove it from inside CAgentAdapter
		iCreator->iAgentAdapterSessionNotifiers.Remove(result);
		// Delete itself
		delete this;
		}
	}

void CAgentAdapterSessionNotifier::CancelServiceChangeNotification(TInt aReason)
/**
 * Implementation of MAgentSessionNotify interface.
 * This send back the ServiceChangeNotification to the Client side, but with
 * the Status set to "aReason".
 *
 * @param aReason The Reason for this Cancel. Default value is "KErrCancel".
 */
	{
	if ( NULL != iResponseMsg )
		{
		iResponseMsg->Complete(aReason);
		delete iResponseMsg;
		iResponseMsg = NULL;
		}
	}

