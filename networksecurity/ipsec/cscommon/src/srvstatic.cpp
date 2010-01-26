// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32svr.h>

#include "srvstarter.h"

static void RunServerL()
//
// Perform all server initialisation, in particular creation of the
// scheduler and server and then run the scheduler
//
    {
    // create and install the active scheduler we need
    CActiveScheduler* s=new(ELeave) CActiveScheduler;
    CleanupStack::PushL(s);
    CActiveScheduler::Install(s);
    //
    // create the server
    CServer2* server = Starter::CreateAndStartServerL();
    CleanupStack::PushL(server);
    //
    
    // naming the server thread after server startup helps to debug panics
    User::LeaveIfError(RThread::RenameMe(Starter::ServerName()));
    
    // Initialisation complete, now signal the client
    RProcess::Rendezvous(KErrNone);
    
    //
    // Ready to run
    CActiveScheduler::Start();
    //
    // Cleanup the server and scheduler
    CleanupStack::PopAndDestroy(2); // server, s
    }

TInt E32Main()
//
// Server process entry-point
//
    {
    __UHEAP_MARK;
    
    CTrapCleanup* cleanup=CTrapCleanup::New();
    TInt r=KErrNoMemory;
    if (cleanup)
        {
        TRAP(r,RunServerL());
        delete cleanup;
        }
    
    __UHEAP_MARKEND;
    return r;
    }
