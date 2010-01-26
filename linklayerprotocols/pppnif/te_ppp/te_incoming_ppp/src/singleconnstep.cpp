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
// PPP Loopback testing implementation
// 
//

/**
 @file 
 @internalComponent
*/


#include "singleconnstep.h"
using namespace te_ppploopback;

namespace
	{
	// For reading test configuration ini file.
	_LIT(KMicrosecsToServerConn,"MicrosecsToServerConn"); 
	_LIT(KMicrosecsToClientConn,"MicrosecsToClientConn");
	_LIT(KDoStartSvrFirst, "DoStartServerFirst");
	}	


/**
 C++ Constructor
 
 
 @post Test name is setup. 
 */	
CSingleConnStep::CSingleConnStep():
	CLoopbackTestStepBase()	
	{	
	SetTestStepName(KSingleConnStep);
	}

/**
 C++ Destructor
 
 */	
CSingleConnStep::~CSingleConnStep()
	{
	}


/**
 Carries out the test sequence
 Test cases: SERVER_INIT, CLIENT_INIT, SIM_INIT
 Tests normal PPP operation.
 
 @return result of the test
 */
TVerdict CSingleConnStep::doTestStepL()
	{
	SetTestStepResult(EFail);
	
	 // Load testing configuration and write ppp ini files.
	ConfigurePppServerL();
	ConfigurePppClientL();	
	
	// Note: microsectsToXConn is not implemented as one variable purely for 
	// clarity and for consistency with TestExecute script file.
	TInt microsecsToServerConn = 0;
	GetIntFromConfig(ConfigSection(),KMicrosecsToServerConn, microsecsToServerConn);
	
	TInt microsecsToClientConn = 0;
	GetIntFromConfig(ConfigSection(), KMicrosecsToClientConn, microsecsToClientConn);	
	
	TBool doStartSvrFirst = EFalse;
	GetBoolFromConfig(ConfigSection(),KDoStartSvrFirst, doStartSvrFirst);
	
	INFO_PRINTF4(_L("Config: doStartSvrFirst=%d, toSvrConn=%d, toClientConn=%d"), doStartSvrFirst, microsecsToServerConn, microsecsToClientConn);
	
	
	InstallActiveSchedLC();
	
	InitPppServerL();
	InitPppClientL();
	
	
	// PPP Connection establishment phase:
	// Based on the test case we can start first either the client or the server, and
	// vary the time between.
	// The wait is implemented in a function call. It is assumed that the call overhead is
	// negligible compared to the timeout specified for testing.
	//-------------------------------------------------------------------------------------------
	if(doStartSvrFirst)
		{		
		iServer->ConnectToPeerL();
		INFO_PRINTF1(_L("Server started, waiting for client"));
		User::After(microsecsToClientConn);
		iClient->ConnectToPeerL();		
		INFO_PRINTF1(_L("Client started, waiting for server"));
		}
	else
		{		
		iClient->ConnectToPeerL();
		INFO_PRINTF1(_L("Client started, waiting for server"));
		User::After(microsecsToServerConn);
		iServer->ConnectToPeerL();		
		INFO_PRINTF1(_L("Server started, waiting for client"));
		}	
	
	iStepSched->Start();
	// The rest of the testing is handled in the callbacks in the TestStepBase.
	// We only return to here when the testing is complete.
	
	// The rest has no significance for PPP loopback testing.
	//-------------------------------------------------------------------
	
	ShutdownAndDestroyPppClientL();
	ShutdownAndDestroyPppServerL();	
		
	//User::After(5 * 1000000); // DEBUG: Wait for system logs
		
	RemoveActiveSchedL();
	return TestStepResult();	
	}



	
	







	

