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

#include <e32base.h>
#include <e32cons.h>
#include "TlsTestEngine.h"


_LIT(KTxtTlsTest,"TlsClientTest");
_LIT(KTxtConsoleTitle,"TLS Client Test");
_LIT(KTestFailed,"Test failed: leave code=%d");
_LIT(KFailedToConnect,"Failed to connect: leave code=%d");
_LIT(KTxtOK,"ok");
const TInt KMaxTriesToConnect = 1000;

// public
LOCAL_D CConsoleBase* console; 
LOCAL_C void StartTestL(); 

// private
LOCAL_C void InitializeL(); 

GLDEF_C TInt E32Main() // main function called by E32
    {
	__UHEAP_MARK;
	CTrapCleanup* cleanup=CTrapCleanup::New(); 
	TRAPD(error,InitializeL()); 
	__ASSERT_ALWAYS(!error,User::Panic(KTxtTlsTest,error));
	delete cleanup; 
	__UHEAP_MARKEND;
	return 0; 
    }

LOCAL_C void InitializeL() // initialize and call test code
    {
	console=Console::NewL(KTxtConsoleTitle,TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);
	TRAPD(error,StartTestL()); 
	if (error)
		console->Printf(KTestFailed, error);
	else
		console->Printf(KTxtOK);

	CleanupStack::PopAndDestroy(); // close console
    }


LOCAL_C void StartTestL()
    {
 	// create an active scheduler
    CActiveScheduler *pA=new CActiveScheduler;
    __ASSERT_ALWAYS(pA!=NULL,User::Panic(KTxtTlsTest, KErrNoMemory));
    
    //Install the active scheduler
    CActiveScheduler::Install(pA);
	
	while(ETrue)
		{
		CSecEngine* iSecEngine = CSecEngine::NewL();
	//	CTlsWatchdog* watchdog = new CTlsWatchdog(iSecEngine, console);
		iSecEngine->SetConsole(*console);

		TConnectSettings settings;
		settings.iAddress = _L(" 192.168.0.1");
		settings.iPortNum = 443;
							
		//watchdog->After(100000000);
		
		TRAPD(err, iSecEngine->ConnectL(settings));
		if(err)
			{
			console->Printf(KFailedToConnect, err);
			}
	
		iSecEngine->Cancel();
				
		delete iSecEngine;
		//delete watchdog;
		}

	//Install the active scheduler
    CActiveScheduler::Install(NULL);
	delete pA;
	    
    }



