// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Source file for the DNS Proxy server side implementation.
// DNS Proxy client session handler code.
//



/**
 @file
 @internalComponent
*/

#include "dnsproxyserver.h"
#include "dnsproxysession.h"
#include "dnsproxyclientconfigparams.h"
//debug print
#include <e32debug.h>
#include "dnsproxylog.h"

#ifdef SYMBIAN_NETWORKING_PLATSEC
#include "dnsproxypolicy.h"
#endif


#ifdef SYMBIAN_NETWORKING_PLATSEC
CDnsProxyServer::CDnsProxyServer(): CPolicyServer(EPriorityStandard,Policy,ESharableSessions), iCount(0)
#elif defined(EKA2)
CDnsProxyServer::CDnsProxyServer(): CServer2(EPriorityStandard), iCount(0)
#else
//On  EKA1, session is open in one thread and used in another
CDnsProxyServer::CDnsProxyServer(): CServer2(EPriorityStandard, ESharableSessions), iCount(0)
#endif
/**
 * The CDnsProxyServer::CDnsProxyServer method
 *
 * Constructor
 *
 * @internalComponent
 */
    {
    __LOG("CDnsProxyServer::CDnsProxyServer Entry/Exit")
    }

CDnsProxyServer::~CDnsProxyServer()
    {
    __LOG("CDnsProxyServer::~CDnsProxyServer Entry/Exit")

	}

CDnsProxyServer* CDnsProxyServer::NewL()
/**
 * Creates an instance of DNS Proxy server class
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServer::NewL Entry\n");
    CDnsProxyServer* self = new(ELeave) CDnsProxyServer;
    self->Start(KServerName);
    __LOG("\n CDnsProxyServer::NewL Exit\n");
    return self;
    }

CSession2* CDnsProxyServer::NewSessionL(const TVersion& /*aVersion*/,const RMessage2& /*aMessage*/) const
/**
 * Starts server session object.
 * @param aVersion
 * @param aMessage
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServer::NewSessionL Entry \n");
    CDnsProxyServerSession* dnssession = CDnsProxyServerSession::NewL();
    if(dnssession)
    	{
    	const_cast<CDnsProxyServer*>(this)->iCount++;
    	}
    return dnssession;
    }

/**
 * Starts server session object.
 * @param aVersion
 * @param aMessage
 * @return - None
 *
 * @internalTechnology
 **/
void CDnsProxyServer::DecreaseSessionCount(CDnsProxyServerSession& aSession)const
    {
    __LOG("\n CDnsProxyServer::DecreseSession Entry\n");
    const_cast<CDnsProxyServer*>(this)->iCount--;
    __LOG1("\n CDnsProxyServer::DecreseSession session count = %d\n",iCount);
    if(0 == iCount)
        {
        aSession.StopDNSEngine();
        __LOG("\n *** cout zero stoping scheduler ***\n");
        CActiveScheduler::Stop();
        }
    __LOG("\n CDnsProxyServer::DecreseSession Exit\n");
    }

static void RunTheServerL()
/**
 * This method implements the CServer2 method and runs the server.
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n RunTheServerL \n");
    //create and install the active scheduler
    CActiveScheduler* scheduler = new (ELeave) CActiveScheduler;
    CleanupStack::PushL(scheduler);
    CActiveScheduler::Install(scheduler);

    //create the dns proxy server
    CDnsProxyServer *server = CDnsProxyServer::NewL();
    CleanupStack::PushL(server);
    //naming the server thread after the server start. This helps to debug panics.
    User::LeaveIfError(User::RenameThread(KServerExeName));
    RProcess::Rendezvous(KErrNone);
    //enter the wait loop
    CActiveScheduler::Start();
    __LOG("\n after start CActiveScheduler::Start \n");
    CleanupStack::PopAndDestroy(server);
    CleanupStack::PopAndDestroy(scheduler);
    __LOG("RunTheServerL Exit")
    }


// Server process entry-point
TInt E32Main()
/**
 * The main thread function, the Ordinal 1!
 * @return - Standard Epoc error code on exit
 */
    {
    __UHEAP_MARK; // Heap checking

    CTrapCleanup* cleanup=CTrapCleanup::New();
    TInt r=KErrNoMemory;
    if (cleanup)
        {
        TRAP(r,RunTheServerL());
        delete cleanup;
        }
    __UHEAP_MARKEND;
    return r;
    }

