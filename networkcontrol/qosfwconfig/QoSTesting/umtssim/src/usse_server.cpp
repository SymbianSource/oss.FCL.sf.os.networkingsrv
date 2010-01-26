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
// usse_server.cpp - implementation of umts simulator server object CUmtsSimServServer and it's startup routine
//

#include "us_cliserv.h"
#include "usse_server.h"
#include "usse_simulator.h"

#include <e32svr.h>
#include <f32file.h>


#ifdef __WINS__
// create thread instead of process on emulator
#define UMTSSIM_SINGLE_PROCESS
#endif

_LIT(KUMTSSimStartSemaphoreName, "UmtsSimServerSTART");

// DLL entry point

/* **************************
    * CUmtsSimServServer class
    *
    * Represents the server
    * Started with the first client connect call.
    * Start includes setting up active scheduler and the server active object
    * *************************** */


// utility - panic the server
void CUmtsSimServServer::PanicServer(TUmtsSimServPanic aPanic)
    {
    _LIT(KTxtUmtsSimServer,"UmtsSimServer");
    User::Panic(KTxtUmtsSimServer,aPanic);
    }


// start the server thread
// This is called from the client.
EXPORT_C TInt StartThread()
    {
    // check server not already started
    TFindServer findUmtsSimServer(KUmtsSimServerName);
    TFullName name;
    if(findUmtsSimServer.Next(name)==KErrNone)
        { // found server already - bad news
		return KErrGeneral;
        }

    RSemaphore serverStarted;
    TInt err = serverStarted.CreateGlobal(KUMTSSimStartSemaphoreName, 0);
	if(err != KErrNone)
		return err;

    // create server thread (emulator) or process (target)

    #ifdef UMTSSIM_SINGLE_PROCESS
    RThread server;
	err = server.Create(
		KUmtsSimServerName,					// name of thread
		CUmtsSimServServer::ThreadFunction,	// thread function
		KDefaultStackSize, KDefaultHeapSize, KDefaultHeapSize,
		NULL
        );
    #else
	RProcess server;
        {
		RFs fs;
		err = fs.Connect();
		if(err != KErrNone)
            {
			fs.Close();
			serverStarted.Close();
			return err;
            }

		TFindFile finder(fs);
		err = finder.FindByDir(KUmtsSimServerExeFile, KUmtsSimServerExePath);
		if(err != KErrNone)
            {
			fs.Close();
			serverStarted.Close();
			return err;
            }

		_LIT(KEmptyCommand, "");
		err = server.Create(
			finder.File(),			// exe path
			KEmptyCommand			// commandline
            );

		fs.Close();
        }
        #endif

        if(err != KErrNone)
            {
            serverStarted.Close();
            server.Close();
            return err;
            }

        server.Resume(); // kick it into life

        serverStarted.Wait(); // wait until it's got going

        // tidy up
        server.Close(); // no longer interested in that thread/process
        serverStarted.Close(); // or semaphore


        // all well
        return KErrNone;
    }


// thread function; the active scheduler is installed and started here
EXPORT_C TInt CUmtsSimServServer::ThreadFunction(TAny*)
    {
	__UHEAP_MARK; // memory check start

    #ifdef UMTSSIM_SINGLE_PROCESS
    // prevent unload if starter client dies
    RLibrary lib;
    _LIT(KServerLibraryName, "umtssim_server.dll");
    if(lib.Load(KServerLibraryName) != KErrNone)
		PanicServer(ELoadServerLibrary);
    #endif

    // create cleanup stack
	CTrapCleanup* cleanup = CTrapCleanup::New();
    __ASSERT_ALWAYS(cleanup, PanicServer(ECreateTrapCleanup));

    // construct and install active scheduler
    CActiveScheduler* scheduler=new CActiveScheduler;
    __ASSERT_ALWAYS(scheduler,PanicServer(EMainSchedulerError));
    CActiveScheduler::Install(scheduler);

	// construct server, an active object
	CUmtsSimServServer* server = NULL;
    TRAPD(err,server = CUmtsSimServServer::NewL());
    __ASSERT_ALWAYS(!err,PanicServer(ESvrCreateServer));

    // signal everything has started
    RSemaphore started;

	TRAPD(err1,
          User::LeaveIfError(started.OpenGlobal(KUMTSSimStartSemaphoreName)));
	__ASSERT_ALWAYS(!err1,PanicServer(ESvrCreateServer));


    started.Signal(); // now started ok
	started.Close();

    // start handling requests
    CActiveScheduler::Start();

	// closing server
	delete server;

	CActiveScheduler::Install(NULL); // remove current
	delete scheduler;
	delete cleanup;

    #ifdef UMTSSIM_SINGLE_PROCESS
	// lib.Close(); -- not safe to do here, done automatically by system
    #endif

	__UHEAP_MARKEND; // memory check end

    return KErrNone;
    }


// allocation and construction
CUmtsSimServServer* CUmtsSimServServer::NewL()
    {
    CUmtsSimServServer* self=new (ELeave) CUmtsSimServServer;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    self->StartL(KUmtsSimServerName);
    return self;
    }


// return a new object container
CObjectCon* CUmtsSimServServer::NewContainerL()
    {
    return iContainerIndex->CreateL();
    }

void CUmtsSimServServer::DeleteContainer(CObjectCon* aContainer)
    {
    iContainerIndex->Remove(aContainer);
	// delete aContainer; -- Remove is specsed to delete container
    }

// C++ constructor - just pass priority to base CServer class
CUmtsSimServServer::CUmtsSimServServer()
    : CServer2(EUmtsSimServerPriority)
    {
    __DECLARE_NAME(_S("CUmtsSimServServer"));
    }


// second-phase constructor, create the container index
void CUmtsSimServServer::ConstructL()
    {
    iContainerIndex=CObjectConIx::NewL();
	iSimulator = CUmtsSimulator::NewL();
    }


// destruction
CUmtsSimServServer::~CUmtsSimServServer()
    {
    // clean up
    delete iContainerIndex;
	delete iSimulator;
    }


// Create a new client session for this server.
// CSharableSession* CUmtsSimServServer::NewSessionL(const TVersion &aVersion) const
// Changed because of migration to Client/Server V2 API
CSession2* CUmtsSimServServer::NewSessionL(const TVersion &aVersion, const RMessage2 & /*aMessage*/) const
    {
    // check version is ok
    TVersion v(KUmtsSimServMajorVersionNumber,
			   KUmtsSimServMinorVersionNumber,
			   KUmtsSimServBuildVersionNumber);
    if (!User::QueryVersionSupported(v,aVersion))
        User::Leave(KErrNotSupported);

    // make new session
    // RThread aClient= Message().Client();
    // Changed because of migration to Client/Server V2 API
     
    // aClient was not actually used for anything in NewL.
    // CUmtsSimServSession* session = CUmtsSimServSession::NewL(aClient,(CUmtsSimServServer*)this, iSimulator);
    CUmtsSimServSession* session = CUmtsSimServSession::NewL((CUmtsSimServServer*)this, iSimulator);

	// add session to own booking
	iSimulator->AddSession(*session);
	return session;
    }
