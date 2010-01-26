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
// TNifAgentRef.CPP
// Implementation of the dummy version of class CNifAgentRefN1
// for testing purposes
// 
//

#define Class_NifAgentRef
#include <e32uid.h>
#include <e32base.h>
#include <nifagt.h>
#include "tdummynifagentref.h"
#include <ni_log.h>
#include "IF_DEF.H"
#include "NI_STD.H"

const TUid KUidAGT = { 0x10003d39 };

// Function pointer used to access DLL through first ordinal function
typedef CNifAgentFactory* (*CreateNifAgentFactoryFunctionL)();

CNifAgentRefN1::CNifAgentRefN1(MDummyNifToAgtHarnessNotify* aNotify) 
: CNifAgentRef( (TInt)0 ), iNotify(aNotify), iDisconnectRequired(EFalse)
	{
	iIsSubClassed = ETrue;
	}
/*
GLDEF_C TInt E32Dll(TDllReason)
//
// DLL entry point
//
	{
	return KErrNone;
	}
*/

EXPORT_C CNifAgentRefN1::~CNifAgentRefN1()
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


EXPORT_C CNifAgentRefN1* CNifAgentRefN1::NewL(MDummyNifToAgtHarnessNotify* aNotify, const TBool aCSDAgent)
//
// When constructing a new CNifAgentRefN1, the client test object is passed
// as a MNifManTestNotify, allowing notifications to be passed up.
//
	{
	CNifAgentRefN1* ref = new (ELeave) CNifAgentRefN1(aNotify);
	CleanupStack::PushL(ref);
	ref->ConstructL(aCSDAgent);
	CleanupStack::Pop();
	return ref;
	}

void CNifAgentRefN1::ConstructL(const TBool aCSDAgent)
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

EXPORT_C void CNifAgentRefN1::Connect()
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

void CNifAgentRefN1::ConnectComplete(TInt aStatus) 
	{ 
	iCompletionCode = aStatus;
	iDisconnectRequired=EFalse;
	
	// Unblock execution 
	CActiveScheduler::Stop();
	}


EXPORT_C void CNifAgentRefN1::ReConnect()
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


void CNifAgentRefN1::ReconnectComplete(TInt aStatus) 
	{ 

    // When the Agent calls back this function then call
    // the Connect function to perform the reconnection
	iCompletionCode = aStatus;
	iDisconnectRequired=EFalse;
	
	// Unblock execution
	CActiveScheduler::Stop();
	}


EXPORT_C void CNifAgentRefN1::Disconnect()
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

void CNifAgentRefN1::DisconnectComplete()
	{ 

	iCompletionCode=0;
	
	// Unblock execution
	CActiveScheduler::Stop();
	}

void CNifAgentRefN1::AgentProgress(TInt aStage, TInt aError) 
	{

	if(iCompletionCode == 0x7FFFFFFF)
		iProgressStage=aStage;

	// Let the test harness know, just in case it's interested
	iNotify->AgentProgress(aStage, aError);
	}

void CNifAgentRefN1::AgentProgress(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
	{
	// @todo add some implementation here if necessary for the testing...
	}

TInt CNifAgentRefN1::Notification(TAgentToNifEventType aEvent, TAny* aInfo) 
	{

	// Let the Agt test module decide what to do with the notification
	return iNotify->Notification(aEvent, aInfo);
	}


EXPORT_C void CNifAgentRefN1::GetCompletionCode(TInt& aCompletionCode)
//
// Retrieves the last error returned by the CSD agent
//
	{
	aCompletionCode = iCompletionCode;
	}

EXPORT_C void CNifAgentRefN1::GetProgressStage(TInt& aProgressStage)
//
// Retrieves the last progress stage before a call to one of the 
// asyncronous completion functions
//
	{
	aProgressStage = iProgressStage;
	}


void CNifAgentRefN1::AuthenticateComplete(TInt aStatus)
	{ 
	iCompletionCode = aStatus;
	CActiveScheduler::Stop();
	}

void CNifAgentRefN1::ServiceStarted() 
	{
	}

void CNifAgentRefN1::ServiceClosed() 
	{
	}

TInt CNifAgentRefN1::IncomingConnectionReceived() 
	{ 
	return 0;
	}

void CNifAgentRefN1::AgentEvent(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
	{
	}

TName CNifAgentRefN1::Name() const 
	{ 
	return iName;
	}

void CNifAgentRefN1::Close() 
	{
	}

EXPORT_C void CNifAgentRefN1::WaitForIncoming()
	{
	iCompletionCode = 0x7FFFFFFF;
	if(iAgent)
		{
		iAgent->Connect(EAgentStartDialIn);
		}
	}



