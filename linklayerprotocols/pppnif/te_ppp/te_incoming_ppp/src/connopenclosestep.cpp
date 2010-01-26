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
// implementation for CConnOpenCloseStep
// 
//

/**
 @file 
 @internalComponent
*/



#include "connopenclosestep.h"
using namespace te_ppploopback;

// Local variables.
namespace
	{
	// For reading test configuration ini file
	_LIT(KMicrosecsToClientConn,   "MicrosecsToClientConn");
	_LIT(KMicrosecsToClientDisconn,"MicrosecsToClientDisconn");
	}	


/**
 C++ Constructor
 
 @post Test name is setup. 
 */	
CConnOpenCloseStep::CConnOpenCloseStep():
	CLoopbackTestStepBase()
	{
	SetTestStepName(KConnOpenCloseStep);
	}

/**
 C++ Destructor
 
 
 @post resources associated with Client and Server are released. 
 */	
CConnOpenCloseStep::~CConnOpenCloseStep()
	{
	}

/**
 Carries out the test sequence
 Test case: CONN_OPEN_CLOSE
 Tests whether PPP Server can handle client disconnects.
 
 @return result of test
 */	
TVerdict CConnOpenCloseStep::doTestStepL()
	{
	SetTestStepResult(EFail);
	
	 // Load testing configuration and write ppp ini files.
	ConfigurePppServerL();
	ConfigurePppClientL();	
	

	TInt microsecsToClientConn = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsToClientConn, microsecsToClientConn);		
	
	TInt microsecsToClientDisconn = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsToClientDisconn, microsecsToClientDisconn);
	if(microsecsToClientDisconn == 0)
		{
		microsecsToClientDisconn = 10 * 1000000; // Default value: 10 seconds.
		}
	
	INFO_PRINTF3(_L("Config: toClientConn= %d, toClientDisconn= %d"), microsecsToClientConn, microsecsToClientDisconn);
	
	
	InstallActiveSchedLC();
	
	InitPppServerL();
	InitPppClientL();
	
		
	SetupForNoMessageExchange();
	
	// Open PPP link.
	//-------------------------------------------------------------------------------------------
	iServer->ConnectToPeerL();
	INFO_PRINTF1(_L("Server started, waiting for client"));
	User::After(microsecsToClientConn); 	// NB: This time must be correspond to LCP retry timeout
	iClient->ConnectToPeerL();		
	INFO_PRINTF1(_L("Client started, waiting for server"));
	
	CTimeoutTimer* timer = CTimeoutTimer::NewLC();
	timer->SetListener(this, MPppEndpointListener::ETimeoutTimer);
	
	// Close client side PPP link after 10 seconds 
	// Note: on some systems, it takes more than 10 seconds for the PPP link to be
	// established, resulting in test failure. If this is the case, increase this 
	// time as necessary.
	timer->RequestTimeoutL(microsecsToClientDisconn);
	iStepSched->Start();
	CleanupStack::PopAndDestroy(timer);
	
	
	ShutdownAndDestroyPppClientL();
	
	//Simulate some waiting timer before the second client connection.
	// Not critical for the test.
	User::After(3*1000000); 
	// Create the second client:
	// This time we exchange messages.
	SetupForMessageExchange();
	
	InitPppClientL();
	iServer->ConnectToPeerL(); // restart the server
	iClient->ConnectToPeerL();
	
    iStepSched->Start();  

	// The rest has no significance for PPP loopback testing.
	//-------------------------------------------------------------------
	ShutdownAndDestroyPppClientL();
	ShutdownAndDestroyPppServerL();
		
	//User::After(5 * 1000000); // Debug: Wait for system logs to flush. 
		
	RemoveActiveSchedL();
	return TestStepResult();	
	}

/**
 Disconnects the client upon timeout
 
 @pre client is connected
 @post client is disconnected
 */	
void CConnOpenCloseStep::OnTimerEvent(TInt /*aErrorCode*/ )
	{
	TRAPD(err, iClient->DisconnectFromPeerL());	
	INFO_PRINTF2(_L("Client disconnected with error=%d"), err);
	}



	

