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
//

#include <e32std.h>
#include "anvlmain.h"
#include "anvllog.h"
#include "listener.h"
#include "anvlglob.h"
#include "anvlsock.h"


CAnvltestListener::CAnvltestListener(CAnvltestEngine* control, int selectedIpVersion)
    :CActive(0)
	,iFirstRunL(TRUE)
	,iControl(control)
	,ipVersion(selectedIpVersion)
    {
    CActiveScheduler::Add(this);
    }

void CAnvltestListener::ConstructL()
    {
    TRequestStatus *status = &iStatus;

    iServer = StartServerL(iControl);
    
    SetActive();
    iStatus = KRequestPending;
    User::RequestComplete(status, 0);
	
	
    // ipVersion = iControl->GetIpVersion();

    }

CAnvltestListener::~CAnvltestListener()
    {
    Cancel();
    iAnvlThread.Kill(KErrNone);

    delete iServer;
    }

void CAnvltestListener::RunL()
    {
    if (iFirstRunL)
        {
        TRequestStatus *status = &iStatus;
        iFirstRunL = FALSE;
		
		iControl->ShowText(_L("\n\n\n ANVL Test Server Started\n\n"));

				
		SetActive();
        iStatus = KRequestPending;
        User::RequestComplete(status, 0);
        }
    else
        {
        StartThread();
        }
    }

void CAnvltestListener::DoCancel()
    {
    ;
    }


TInt CAnvltestListener::StartThread(void)
    {

    TInt res = KErrNone;
    //RThread anvlThread;
    RThread *anvlThread;

    anvlThread = &iAnvlThread;
    iOriThreadId = RThread().Id();

    res = anvlThread->Create(ANVL_THREAD_NAME,
                            CAnvltestListener::ThreadFunction,
                            KDefaultStackSize,
                            KAnvlDefaultHeapSize,
                            KAnvlDefaultHeapSize,
                            this                // passed as TAny* argument to thread function
                           );

    if ( res == KErrNone )                      // thread created ok - now start it going
        {
        anvlThread->SetPriority(KAnvlThreadPriority);
        anvlThread->Resume();
        // anvlThread->Close();
        }
    else                                        // thread not created ok
        {
        anvlThread->Close();
        }

    return res;

    };

TInt CAnvltestListener::ThreadFunction(TAny* anArg)
    {
    
    RTimer timer; 
    
	int loop = FALSE;

 	while (loop != TRUE)
 	{
 	
    AnvlGlob *anvlglob;
    CTrapCleanup* cleanup = CTrapCleanup::New();    // get clean-up stack
    CAnvltestListener   *listener;

#if 0   
    //    create active scehuler for the thread
    CActiveScheduler *pA = new CActiveScheduler;
    __ASSERT_ALWAYS( pA != NULL, PanicServer( EMainSchedulerError ) );
    CActiveScheduler::Install( pA );

    
    CShutDown* shut_down = NULL;
    TRAP( r,shut_down = new (ELeave) CShutDown );
    if ( r != KErrNone )    return Fault( EStServerCreate );

    CActiveScheduler::Add( shut_down );

    //    activate shutdown object
    shut_down->Start();

    CActiveScheduler::Start();
#endif

    listener = (CAnvltestListener*)anArg;
    anvlglob = AnvlCreateGlobalsL();
    //anvlglob->printObj = listener->iControl;
    anvlglob->printObj = listener;


    CAnvlSockMain   *sockMain;
    sockMain = new (ELeave) CAnvlSockMain;
    anvlglob->anvl_main_blk = sockMain;
    
    sockMain->InitL();

    
	loop = mntcpapp_main(0,listener->ipVersion); 

	
    sockMain->Close();

    	
    AnvlDeleteGlobals();
    
    delete cleanup;

    TRequestStatus delayStatus;
    timer.CreateLocal();
    timer.After(delayStatus,60000000);
    User::WaitForRequest(delayStatus);
    timer.Close();
  
 
	} 
	
    return( KErrNone );

    };

