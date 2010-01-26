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
// Implementation of the dummy version of class CNifAgentRef
// for testing purposes
// 
//

#include <e32uid.h>
#include <e32base.h>
#include <comms-infras/nifagt.h>
#include "TNifAgentRef.h"

const TUid KUidAGT = { 0x10003d39 };

// Function pointer used to access DLL through first ordinal function
typedef CNifAgentFactory* (*CreateNifAgentFactoryFunctionL)();

CNifAgentRef::CNifAgentRef(MDummyNifToAgtHarnessNotify* aNotify) 
 : iNotify(aNotify), iDisconnectRequired(EFalse)
	{ }


EXPORT_C CNifAgentRef::~CNifAgentRef()
//
// Free resources allocated during the test if it is 
// interrupted before completion
//
	{

	if(iDisconnectRequired)
		Disconnect();
	
	if(iAgent)
		delete iAgent;
	
	iLibrary.Close();
	}


EXPORT_C CNifAgentRef* CNifAgentRef::NewL(MDummyNifToAgtHarnessNotify* aNotify, const TBool aCSDAgent)
//
// When constructing a new CNifAgentRef, the client test object is passed
// as a MNifManTestNotify, allowing notifications to be passed up.
//
	{

	CNifAgentRef* ref = new (ELeave) CNifAgentRef(aNotify);
	CleanupStack::PushL(ref);
	ref->ConstructL(aCSDAgent);
	CleanupStack::Pop();
	return ref;
	}

void CNifAgentRef::ConstructL(const TBool aCSDAgent)
	{

    // Factory to create the CSD/PSD Agents
    CNifAgentFactory *factory;

    // Load the DUMMYCSD.AGT or DUMMYPSD.AGT library
    if (aCSDAgent == TRUE)
        {
	    _LIT(KAGTLibraryName, "DUMMYCSD.AGT");
	    TUidType uidType(KDynamicLibraryUid,KUidAGT);
	    User::LeaveIfError(iLibrary.Load(KAGTLibraryName, uidType));
        }
    else
        {
	    _LIT(KAGTLibraryName, "DUMMYPSD.AGT");
	    TUidType uidType(KDynamicLibraryUid,KUidAGT);
	    User::LeaveIfError(iLibrary.Load(KAGTLibraryName, uidType));
        }

    // Obtain a pointer to the 
	// extern "C" EXPORT_C CNifAgentFactory* NewAgentFactoryL()
	// function in the CSD/PSD Agent
	CreateNifAgentFactoryFunctionL funcL = 
        (CreateNifAgentFactoryFunctionL)iLibrary.Lookup(1);

	if (!funcL)
		User::Leave(KErrNotSupported);

	// Create a new agent factory
	factory = (*funcL)();
    CleanupStack::PushL(TCleanupItem(CNifAgentFactory::Cleanup, factory));

    // Use the factory to create a new instance of the CSD or PSD Agent
    if (aCSDAgent == TRUE)
        {
        TNifAgentInfo info;
		TInt index=0;
		_LIT(KCSDAgentName, "CSD");
        iAgent = factory->NewAgentL(KCSDAgentName);
		factory->InstallL();
		factory->Info(info,index);
        }
    else
        {
        TNifAgentInfo info;
		TInt index=0;
        _LIT(KPSDAgentName, "PSD");
        iAgent = factory->NewAgentL(KPSDAgentName);
		factory->InstallL();
 		factory->Info(info,index);
       }

    iAgent->iNotify = this;


    // Cleanup the agent factory
    CleanupStack::PopAndDestroy();

	}

EXPORT_C void CNifAgentRef::Connect()
//
// Starts the agent connection attempt
//
	{

	iCompletionCode = 0x7FFFFFFF;
	if(iAgent)
		{

		// Connection Settings
        TConnectionSettings settings;
		TRAPD(ret,settings = iAgent->ConnectionSettingsL());
		if(ret!=KErrNone)
			{
			ConnectComplete(ret);
			return;
			}
		settings.iDirection  = ECommDbConnectionDirectionOutgoing;
		settings.iDialogPref = ECommDbDialogPrefDoNotPrompt;
		settings.iRank       = 1;
        TRAP(ret,iAgent->SetConnectionSettingsL(settings));
		if(ret!=KErrNone)
			{
			// Cannot stop CActiveScheduler object before it's started
			/*
			ConnectComplete(ret);
			return;
			*/
			iCompletionCode = ret;
			iDisconnectRequired = EFalse;
			return;
			}
        
        // Connection
		iDisconnectRequired=ETrue;
		iAgent->Connect(EAgentStartDialOut);

		// Block execution until connect complete is called
		CActiveScheduler::Start();
		}
	}

void CNifAgentRef::ConnectComplete(TInt aStatus) 
	{ 
	iCompletionCode = aStatus;
	iDisconnectRequired=EFalse;
	
	// Unblock execution 
	CActiveScheduler::Stop();
	}


EXPORT_C void CNifAgentRef::ReConnect()
//
// Reconnect must be called after calling connect and before calling Disconnect
//
	{
	iCompletionCode = 0x7FFFFFFF;
	if(iAgent)
		{
		iDisconnectRequired=ETrue;

        // Call the agent's Reconnect function
        iAgent->Connect(EAgentReconnect);
		
		// Block execution until ReconnectComplete is called
		CActiveScheduler::Start();
		}
	}


void CNifAgentRef::ReconnectComplete(TInt aStatus) 
	{ 

    // When the Agent calls back this function then call
    // the Connect function to perform the reconnection
	iCompletionCode = aStatus;
	iDisconnectRequired=EFalse;
	
	// Unblock execution
	CActiveScheduler::Stop();
	}


EXPORT_C void CNifAgentRef::Disconnect()
//
// Stops the agent
//
	{

	iCompletionCode = 0x7FFFFFFF;
	if(iAgent)
		{
		iAgent->Disconnect(0);

		// Block execution until DisconnectComplete is called
		CActiveScheduler::Start();
		}
	}

void CNifAgentRef::DisconnectComplete()
	{ 

	iCompletionCode=0;
	
	// Unblock execution
	CActiveScheduler::Stop();
	}

void CNifAgentRef::AgentProgress(TInt aStage, TInt aError) 
	{

	if(iCompletionCode == 0x7FFFFFFF)
		iProgressStage=aStage;

	// Let the test harness know, just in case it's interested
	iNotify->AgentProgress(aStage, aError);
	}

void CNifAgentRef::AgentProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
	{
	// @todo add some implementation here if necessary for the testing...
	}

TInt CNifAgentRef::Notification(TAgentToNifEventType aEvent, TAny* aInfo) 
	{

	// Let the Agt test module decide what to do with the notification
	return iNotify->Notification(aEvent, aInfo);
	}


EXPORT_C void CNifAgentRef::GetCompletionCode(TInt& aCompletionCode)
//
// Retrieves the last error returned by the CSD agent
//
	{
	aCompletionCode = iCompletionCode;
	}

EXPORT_C void CNifAgentRef::GetProgressStage(TInt& aProgressStage)
//
// Retrieves the last progress stage before a call to one of the 
// asyncronous completion functions
//
	{
	aProgressStage = iProgressStage;
	}


void CNifAgentRef::AuthenticateComplete(TInt aStatus)
	{ 
	iCompletionCode = aStatus;
	CActiveScheduler::Stop();
	}

void CNifAgentRef::ServiceStarted() 
	{
	}

void CNifAgentRef::ServiceClosed() 
	{
	}

TInt CNifAgentRef::IncomingConnectionReceived() 
	{ 
	return 0;
	}

void CNifAgentRef::AgentEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
	{
	}

TName CNifAgentRef::Name() const 
	{ 
	return iName;
	}

void CNifAgentRef::Close() 
	{
	}

EXPORT_C void CNifAgentRef::WaitForIncoming()
	{
	iCompletionCode = 0x7FFFFFFF;
	if(iAgent)
		{
		iAgent->Connect(EAgentStartDialIn);
		}
	}
