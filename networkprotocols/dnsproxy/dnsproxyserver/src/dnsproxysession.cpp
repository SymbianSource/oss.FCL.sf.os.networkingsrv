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
// dnsproxyserver.cpp
// Source file for the DNS Proxy server side implementation.
// DNS Proxy client session handler code.
//



/**
 @file
 @internalComponent
*/

#include "dnsproxysession.h"
#include "dnsproxyclientconfigparams.h"
#include "dnsproxylog.h"
#include <e32debug.h>

CDnsProxyServerSession* CDnsProxyServerSession::NewL()
/**
 * Factory method for CDnsProxyServerSession.
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::NewL Entry\n");
    CDnsProxyServerSession* self = new (ELeave)CDnsProxyServerSession;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    __LOG("\n CDnsProxyServerSession::NewL Exit\n");
    return self;
    }

 void CDnsProxyServerSession::ConstructL()
 /**
  * 2nd phase c'tor for CDnsProxyServerSession.
  *
  * @internalTechnology
  **/
    {
    __LOG("\n CDnsProxyServerSession::ConstructL Entry\n");
    iProxyEngine = CDnsProxyEngine::GetInstanceL();
    __LOG("\n CDnsProxyServerSession::ConstructL Exit\n");

    }

CDnsProxyServerSession::CDnsProxyServerSession(): iProxyEngine(NULL), iIsEngineStarted(EFalse)
/**
 * Standard constructor for CDnsProxyServerSession.
 *
 * @internalTechnology
 **/
    {
    }

CDnsProxyServerSession::~CDnsProxyServerSession()
/**
 * Standard destructor for CDnsProxyServerSession.
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::~CDnsProxyServerSession Entry\n");
    const CDnsProxyServer* server = static_cast<const CDnsProxyServer*>(Server());

	//this condition is added to avoid panic when there is no memory
    if(NULL != server)
		{
		server->DecreaseSessionCount(*this);
		}
    __LOG("\n CDnsProxyServerSession::~CDnsProxyServerSession Exit\n");
    }

void CDnsProxyServerSession::ServiceL(const RMessage2& aMessage)
/**
 * ServiceL method implemented by derived class
 *
 * @param - aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
	{
	TRAPD(err,DoServiceL(aMessage));
	if (err != KErrNone)
		{
		if (!iMessage.IsNull())
			iMessage.Complete(err);
		}
	}

void CDnsProxyServerSession::DoServiceL(const RMessage2& aMessage)
/**
 * This is command handler function which handles and services client requests.
 *
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
    TInt err;
    switch (aMessage.Function())
        {
        // Handle start command
        case EProxyConfigure:
        	iMessage = aMessage;
        	__LOG("***CDnsProxyServerSession: StartDnsEngine - Start****")
           err = StartDnsEngine(iMessage);
        	__LOG("***CDnsProxyServerSession: StartDnsEngine - End****")
            if(err!=KErrNone)
            	iMessage.Complete(err);
            else
            	iMessage.Complete(KErrNone);
            break;

       // Add DB command
        case EProxyAddDb:
        	iMessage = aMessage;
        	__LOG("***CDnsProxyServerSession: UpdateDatabaseL - Start****")
            err = UpdateDatabaseL(iMessage);
	        __LOG("***CDnsProxyServerSession: UpdateDatabaseL - End****")
            if(err!=KErrNone)
            	iMessage.Complete(err);
            else
            	iMessage.Complete(KErrNone);
            break;

        // Remove DB command
        case EProxyRemoveDb:
        	iMessage = aMessage;
        	__LOG("***CDnsProxyServerSession: RemoveDbEntry - Start****")
            err = RemoveDbEntry(iMessage);
        	__LOG("***CDnsProxyServerSession: RemoveDbEntry - End****")

            if(err!=KErrNone)
            	iMessage.Complete(err);
            else
            	iMessage.Complete(KErrNone);
            break;

        // Handle Update domain name
        case EProxyUpdateDomainName:
            iMessage = aMessage;
        	__LOG("***CDnsProxyServerSession: UpdateDomainName - Start****")
            err = UpdateDomainName(iMessage);
        	__LOG("***CDnsProxyServerSession: UpdateDomainName - End****")
            if(err!=KErrNone)
            	iMessage.Complete(err);
            else
            	iMessage.Complete(KErrNone);
            break;

        case EProxyConfigureUplink:
        	iMessage = aMessage;
        	__LOG("***CDnsProxyServerSession: UpdateUplinkConfig - Start****")
            err = UpdateUplinkConfig(iMessage);
        	__LOG("***CDnsProxyServerSession: UpdateUplinkConfig - End****")
            if(err!=KErrNone)
            	iMessage.Complete(err);
            else
            	iMessage.Complete(KErrNone);
            break;
	        // This should never happen.
        default:
        	iMessage = aMessage;
            iMessage.Complete(KErrNotSupported);
            break;
        }
    }

TInt CDnsProxyServerSession::StartDnsEngine(const RMessage2& aMessage)
/**
 * This method starts the DNS proxy engine, creates an engine instance
 * and start DNS proxy server.
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
     __LOG("\n CDnsProxyServerSession::StartDNSEngine \n");
     iIsEngineStarted = ETrue;
     __LOG1("\niProxyEngine adress = %d \n",iProxyEngine);
     return iProxyEngine->Start(aMessage);
    }

void CDnsProxyServerSession::StopDNSEngine()
/**
 * This method stops and cleans up DNS proxy.
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
	if((iProxyEngine)&& (iIsEngineStarted))
        {
        iProxyEngine->Stop();
        }
     iIsEngineStarted = EFalse;
     delete iProxyEngine;
    }

TInt CDnsProxyServerSession::UpdateDatabaseL(const RMessage2& aMessage)
/**
 * This method is used for DNS Proxy database implementation from ECOM plugin
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::UpdateDatabaseL \n");
    __LOG1("\n iProxyEngine %d \n", iProxyEngine);
    return iProxyEngine->AddDbEntryL(aMessage);
    }
TInt CDnsProxyServerSession::RemoveDbEntry(const RMessage2& aMessage)
/**
 * This method is used for DNS Proxy database implementation from ECOM plugin
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::RemoveDbEntry \n");
    return iProxyEngine->RemoveDbEntry(aMessage);
    }

TInt CDnsProxyServerSession::UpdateDomainName(const RMessage2& aMessage)
/**
 * This method is used for DNS Proxy database implementation from ECOM plugin
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::UpdateDomainName \n");
    return iProxyEngine->UpdateDomainInfo(aMessage);
    }
TInt CDnsProxyServerSession::UpdateUplinkConfig(const RMessage2& aMessage)
/**
 * This method is used to set uplink information
 * @param aMessage - command send by the client
 * @return - None
 *
 * @internalTechnology
 **/
    {
    __LOG("\n CDnsProxyServerSession::UpdateUplinkConfig \n");
    //if engine is not created,return KErrNotReady.
    TInt err;
    if(!iProxyEngine)
    	err = KErrNotReady;
    else
        err = iProxyEngine->SetUplinkConnectionInfo(aMessage);
    return err;
    }

