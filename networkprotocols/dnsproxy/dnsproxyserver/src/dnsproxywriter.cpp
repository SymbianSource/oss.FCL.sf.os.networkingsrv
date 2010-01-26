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
// This class constructs the DNS response packet and sends it back to
// querying hosts. After sending the DNS query response to querying hosts,
// DNS writer checks if there is anything pending in the queue which needs
// to be send back to the querying host.Once done it deletes the completed
// query context from the queue. This is dervied from CActive.
//



/**
 @file
 @internalTechnology
*/
#include "inet6log.h"
#include <cflog.h>
#include "dnsproxylistener.h"
#include "dnsproxywriter.h"

CDnsProxyWriter* CDnsProxyWriter::NewL(CDnsProxyListener& aListener)
/**
 * This method creates an instance of CDnsProxyWriter.
 * @param aListener - Reference of the listener object
 * @return - Pointer to class instance
 *
 * @internalTechnology
 **/
	{
    CDnsProxyWriter*writer = new(ELeave)CDnsProxyWriter(aListener);
	return writer;
	}

CDnsProxyWriter::CDnsProxyWriter(CDnsProxyListener& aListener):CActive(EPriorityStandard),iListener(aListener)
/**
 * This is constructor for CDnsProxyWriter.It adds itself to CActive scheduler.
 * @param aListener - Reference of the listener object
 * @return - None
 *
 * @internalTechnology
 **/

	{
	CActiveScheduler::Add(this);
	}

void CDnsProxyWriter::ConstructL()
/**
 * Second phase construction
 * @param -None
 * @return -None
 *
 * @internalTechnology
 **/

	{
	}

CDnsProxyWriter::~CDnsProxyWriter()
/**
 * This is destructor for CDnsProxyWriter.It cancels any pending requests.
 * @param -None
 * @return -None
 *
 * @internalTechnology
 **/

	{
    Cancel();
	}

void CDnsProxyWriter::RunL()
/**
 * This method implements RunL method of this active object.It deletes all the completed queries
 * and processes the next resolved query. That is, query whose state is EResolved.
 *
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/

	{
		//delete the completed messages
		TInt index = 0;
		TQueryContext *qCtx = NULL;

		while(iListener.iQueryContext.Count() > index)
			{
			TQueryState queryState = iListener.iQueryContext[index]->GetQueryState();
			TQueryContext *qCtx = iListener.GetQCtxFromList(EQueryDone);
			if(qCtx)
				{
				iListener.DeleteQCtxFromListL(qCtx);
				qCtx = NULL;
				}
            index++;
			}
			
		qCtx = iListener.GetQCtxFromList(EResolved);
		if(qCtx)
			{
			WriteTo(qCtx);
			}
	}

void CDnsProxyWriter::DoCancel()
/**
 * This method cancels outstanding write request.
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/

	{
	iListener.iListenerSocket.CancelSend();
	}

void CDnsProxyWriter::WriteTo(TQueryContext* aQueryContext)
/**
 * This method gets DNS response from query context and sends it to the querying host.
 * @param queryContext - Pointer to the query context
 * @return - None
 *
 * @internalTechnology
 **/
	{
	if(IsActive())
		return;
	
	TPtrC8 msg = aQueryContext->GetDnsReplyMessage();
	addr = aQueryContext->GetSourceAddr();
	aQueryContext->SetQueryState(EQueryDone);

	iListener.iListenerSocket.SendTo(msg,aQueryContext->GetSourceAddr(),0,iStatus);
	SetActive();
	}

TInt CDnsProxyWriter::RunError(TInt /*aErr*/)
/**
 * This method implements RunError method of an active object. This helps in debugging.
 * @param aErr - Error code
 * @return - Error code
 *
 * @internalTechnology
 **/
	{
	iListener.DeleteAllQCtxFromList();
	return KErrNone;
	}
