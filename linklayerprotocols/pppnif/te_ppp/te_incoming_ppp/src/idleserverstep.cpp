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
// Implementation for CIdleServerStep class 
// 
//

/**
 @file 
 @internalComponent
*/


#include "idleserverstep.h"
using namespace te_ppploopback;

// Local variables.
namespace
	{
	// Configuration file entries.
	_LIT(KMicrosecsIdleTimeout, "MicrosecsIdleTimeout");
	_LIT(KMicrosecsToFirstClientConn,"MicrosecsToFirstClientConn");	
	_LIT(KMicrosecsToSecondClientConn,"MicrosecsToSecondClientConn");	
	}	


/**
 C++ Constructor
 
 
 @post Test name is setup. 
 */	
CIdleServerStep::CIdleServerStep():
	CLoopbackTestStepBase()
	{
	SetTestStepName(KIdleServerStep);
	}

/**
 C++ Destructor
 
 */	
CIdleServerStep::~CIdleServerStep()
	{
	}


/**
 Carries out the test sequence for the IDLE_SERVER test case
 Test whether the server can handle more than one client connection in idle mode.

 @leave if any of the called methods leave.
 */
TVerdict CIdleServerStep::doTestStepL()
	{
	SetTestStepResult(EFail);
	
	 // Load testing configuration and write ppp ini files.
	ConfigurePppServerL();
	ConfigurePppClientL();	
	
	TInt microsecsIdleTimeout = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsIdleTimeout, microsecsIdleTimeout);
	
	TInt microsecsToFirstClientConn = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsToFirstClientConn, microsecsToFirstClientConn);	
	
	TInt microsecsToSecondClientConn = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsToSecondClientConn, microsecsToSecondClientConn);	
		
	INFO_PRINTF4(_L("Idle timeout %d, To First Client Conn: %d, To Second Client Conn: %d"),microsecsIdleTimeout, microsecsToFirstClientConn, microsecsToSecondClientConn );
	
	
	
	InstallActiveSchedLC();
	
	InitPppServerL();
	InitPppClientL();
		
	// Open PPP link.
	//-------------------------------------------------------------------------------------------
	iServer->ConnectToPeerL();
	INFO_PRINTF1(_L("Server started, waiting for client"));
	PutPppServerInIdleMode(); // waits for the server to go into idle mode
	User::After(microsecsToFirstClientConn); 	
	iClient->ConnectToPeerL();		
	INFO_PRINTF1(_L("Client started, waiting for server"));

	iStepSched->Start();
	//-------------------------------------------------------------------------------------------	
	ShutdownAndDestroyPppClientL();
	
	iServer->ConnectToPeerL(); // restart the server
	PutPppServerInIdleMode(); // waits for the server to go into idle mode
	User::After(microsecsToSecondClientConn);	
	
	InitPppClientL(); // Create the second client
	iClient->ConnectToPeerL();
	
    //-------------------------------------------------------------------------------------------
	iStepSched->Start();  

	// The rest has no significance for PPP loopback testing.
	//-------------------------------------------------------------------
	ShutdownAndDestroyPppClientL();
	ShutdownAndDestroyPppServerL();
		
	//User::After(5 * 1000000); //DEBUG: Wait for system logs to flush. 
		
	RemoveActiveSchedL();
	return TestStepResult();	
	}			
		
	







	

