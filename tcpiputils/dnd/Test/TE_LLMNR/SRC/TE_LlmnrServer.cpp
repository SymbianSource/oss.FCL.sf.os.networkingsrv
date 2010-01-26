// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Test server for the LLMNR test
// 
//

/**
 @file TE_LlmnrServer.cpp
*/
#include "TE_LlmnrServer.h"
#include "LlmnrTestStartStop.h"
#include "LlmnrInit.h"
#include "LlmnrTestQueries.h"
#include "LlmnrTestNameConflict.h"

#include <c32comm.h>
#include <e32math.h>
#include <hash.h>

_LIT(KServerName,"TE_LlmnrServer");

/**
* @return - Instance of the test server
* Called inside the MainL() function to create and start the
* CTestServer derived server.
*/
CLlmnrTestServer* CLlmnrTestServer::NewL()
    {
    // __EDIT_ME__ new your server class here
    CLlmnrTestServer * server = new (ELeave) CLlmnrTestServer();
    CleanupStack::PushL(server);
    // CServer base class call
    server->StartL(KServerName);
    CleanupStack::Pop(server);
    return server;
    }

#if (!defined EKA2)
LOCAL_C void MainL()
/**
* REQUIRES semaphore to sync with client as the Rendezvous()
* calls are not available
*/
    {
    CActiveScheduler* sched=NULL;
    sched=new(ELeave) CActiveScheduler;
    CleanupStack::PushL(sched);
    CActiveScheduler::Install(sched);
    
    CLlmnrTestServer* server = NULL;
    TRAPD(err,server = CLlmnrTestServer::NewL());
    if(!err)
        {
        CleanupStack::PushL(server);
        RSemaphore sem;
        // The client API will already have created the semaphore
        User::LeaveIfError(sem.OpenGlobal(KServerName));
        CleanupStack::Pop(server);
        // Sync with the client then enter the active scheduler
        sem.Signal();
        sem.Close();
        sched->Start();
        }
    CleanupStack::Pop(sched);
    delete server;
    delete sched;
    }
#else
// EKA2 much simpler
// Just an E32Main and a MainL()
LOCAL_C void MainL()
/**
* Much simpler, uses the new Rendezvous() call to sync with the client
*/
    {
    // Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
    RProcess().DataCaging(RProcess::EDataCagingOn);
    RProcess().SecureApi(RProcess::ESecureApiOn);
#endif
    CActiveScheduler* sched=NULL;
    sched=new(ELeave) CActiveScheduler;
    CActiveScheduler::Install(sched);
    CLlmnrTestServer* server = NULL;
    // Create the CTestServer derived server
    TRAPD(err,server = CLlmnrTestServer::NewL());
    if(!err)
        {
        // Sync with the client and enter the active scheduler
        RProcess::Rendezvous(KErrNone);
        sched->Start();
        }
    delete server;
    delete sched;
    }
#endif

// Only a DLL on emulator for typhoon and earlier
#if (defined __WINS__ && !defined EKA2)
GLDEF_C TInt E32Dll(enum TDllReason)
    {
    return 0;
    }
#else

GLDEF_C TInt E32Main()
/**
* @return - Standard Epoc error code on exit
*/
    {
    CTrapCleanup* cleanup = CTrapCleanup::New();
    if(cleanup == NULL)
        {
        return KErrNoMemory;
        }
    TRAP_IGNORE(MainL());
    delete cleanup;
    return KErrNone;
    }
#endif

// Create a thread in the calling process
// Emulator typhoon and earlier
#if (defined __WINS__ && !defined EKA2)
TInt ThreadFunc (TAny* /*aParam*/)
/**
* @return - Server exit code
* @param - unused
* Server Thread function. Guts of the code in the MainL() function
*/
    {
    CTrapCleanup* cleanup = CTrapCleanup::New();
    if(cleanup == NULL)
        {
        return KErrNoMemory;
        }
    TRAPD(err,MainL());
    delete cleanup;
    return KErrNone;
    }

EXPORT_C TInt NewServer() 
/**
* @return - Standard Epoc error codes
* 1st and only ordinal, called by the client API to initialise the server
*/
    {
    _LIT(KThread,"Thread");
    RThread thread;
    // __EDIT_ME__ - Make sure the TBuf is large enough
    TBuf<KMaxTestExecuteNameLength> threadName(KServerName);
    // Create a hopefully unique thread name and use the ThreadFunc
    threadName.Append(KThread);
    const TInt KMaxHeapSize = 0x1000000;			//< Allow a 1Mb max heap
    TInt err = thread.Create(threadName, ThreadFunc, KDefaultStackSize,
        KMinHeapSize, KMaxHeapSize,
        NULL, EOwnerProcess);
    if(err)
        return err;
    thread.Resume();
    thread.Close();
    return KErrNone;
    }

#endif

    /**
    * @return - A CTestStep derived instance
    * Implementation of CTestServer pure virtual
*/
CTestStep* CLlmnrTestServer::CreateTestStep(const TDesC& aStepName)
    {
    CTestStep* testStep = NULL;
    
    if(aStepName == KTestStepLLMNR_StartUp)
        testStep = new CTestStepLLMNR_StartUp(this);
    
    if(aStepName == KTestStepLLMNR_ShutDown)
        testStep = new CTestStepLLMNR_ShutDown(this);
    
    if(aStepName == KTestStepLLMNR_Init)
        testStep = new CTestStepLLMNR_Init(this);
    
    if(aStepName == KTestStepLLMNR_Queries)
        testStep = new CTestStepLLMNR_Queries(this);
    
    if(aStepName == KTestStepLLMNR_NameConflict)
        testStep = new CTestStepLLMNR_NameConflict(this);
    
    
    return testStep;
    }


/**
*   Make a number a little bit random. Takes maximal value and dispersion, generates positive value 
*   greater than 0 and less than aMaxNum.
*
*   @param  aMaxNum  maximal value to be generated
*   @param  aDisp    dispersion, if <=0 assumed to be aMaxNum/2
*   @retval          positive random value less than aMaxNum
*/
TInt CLlmnrTestServer::RandomizeNum(TInt aMaxNum, TInt aDisp/*=0*/) const
    {
    
    if(aMaxNum <= 0) 
        return 0;
    
    if(aDisp <=0 )
        aDisp = aMaxNum/2;
    
    if(aDisp <=0 )
        return aMaxNum;
    
    TInt nRes = aMaxNum - Math::Rand(iRndSeed) % aDisp;
    
    return  (nRes > 0) ? nRes : 0;
    }



//---------------------------------------------------------------------------------------------------------
TNodeInfo::TNodeInfo()
    {
    iCntTrials =0;
    iAlive = EFalse;
    iLocal = ETrue;
    iIpUnique = EFalse;
    iClientConnected = EFalse;
    }


//---------------------------------------------------------------------------------------------------------

TNetworkInfo::TNetworkInfo()
    {
    iInitialized = EFalse;
    }

TNetworkInfo::~TNetworkInfo()
    {
    Reset();
    }

/**  Reset information, clear arrays etc. */
void TNetworkInfo::Reset()
    {
    iNodes.Reset(); //-- reset network nodes array
    iInitialized=EFalse;
    }

/**
*   Find first occurence of the given IP address in TNodeInfo array.
*   Uses linear search. Not efficient, but acceptable for a small amount of nodes 
*
*   @param  aIPaddr - IP address to find
*   @return KErrNotFound if the IP address isn't found or positive index in TNodeInfo array otherwise
*/
TInt TNetworkInfo::FindIP(const TInetAddr& aIPaddr) const
    {
    TInt index(KErrNotFound);
    
    //-- perform linear search. Not efficient, but simple
    for(TInt i=0; i< iNodes.Count(); ++i)
        if(iNodes[i].iAddr.Match(aIPaddr))
            {
            index = i;
            break;
            }
        
        return index;
    }

/**
*   Find first occurence of the given HostName in TNodeInfo array.
*   Uses linear search. Not efficient, but for small amount of nodes acceptable
*
*   @param  aHostName - host name to find
*   @return KErrNotFound if not found or positive index in TNodeInfo array otherwise
*/
TInt TNetworkInfo::FindHostName(const TDesC16& aHostName) const
    {
    TInt index(KErrNotFound);
    
    //-- perform linear search. Not efficient, but simple
    for(TInt i=0; i< iNodes.Count(); ++i)
        if(iNodes[i].iHostName.CompareF(aHostName) == 0)
            {
            index = i;
            break;
            }
        
        return index;
    }

TInt TNetworkInfo::FindHostName(const TDesC8& aHostName) const
    {
    TName tmpName;
    tmpName.Copy(aHostName);
    
    return FindHostName(tmpName);
    }

/**
*   Calculates MD5 hash of the nodes information in iNodes. Used for name conflict test.
*   @param  aHashBuf buffer descriptor, where the 16-bytes hash will be placed.
*   @return standard error codes.
*/
TInt TNetworkInfo::NodesDataHash(TDes8& aHashBuf)   const
    {
    if(NodesCount() <=0)
        return KErrNotFound; //-- no data to make hash
    
    //-- construnt hash calculator
    CMD5* pHash = NULL;
    TRAPD(nRes, pHash=CMD5::NewL());
    
    if(nRes != KErrNone)
        return nRes;
    
    pHash->Reset();
    
    TName       tmpBuf;
    TBuf8<256>  tmpBuf8;
    TPtrC8      hashPtr;    
    
    
    for(TInt i=0; i< iNodes.Count(); ++i)
        {
        if(iNodes[i].iAlive)
            {
            
            iNodes[i].iAddr.Output(tmpBuf);     //-- print out node's IP address
            tmpBuf.Append(iNodes[i].iHostName); //-- append node's name
            tmpBuf8.Copy(tmpBuf);               //-- convert to 8-bit encoding
            
            hashPtr.Set(pHash->Hash(tmpBuf8)); //-- update hash
            }
        }
    
    aHashBuf.Copy(hashPtr);
    
    delete pHash;
    
    return KErrNone;
    }

//---------------------------------------------------------------------------------------------------------




