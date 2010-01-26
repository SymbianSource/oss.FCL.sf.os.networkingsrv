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
// @internalTechnology
//



#include "dnsproxyengine.h"
#include "dnsproxyqryhandler.h"
#include "dnsproxyservconf.h"
#include "dnsproxylistener.h"
#include "dnsproxymsgproc.h"
#include "dnsproxylog.h"
#include <e32math.h>

// Creates an instance of query handlers
CDnsProxyQueryHandler* CDnsProxyQueryHandler::NewL(CDnsProxyEngine& aEngine, CDnsProxyListener& aListener)
/**
 * This is the method to create an instance of CDnsProxyQueryHandler
 * @param	aEngine
 *
 * @internalTechnology
*/
	{
	CDnsProxyQueryHandler* gResolver = new(ELeave)CDnsProxyQueryHandler(aEngine, aListener);
	gResolver->ResetServerCount();
	gResolver->iCurrentUplink = 0;
	return gResolver;
	}

CDnsProxyQueryHandler::CDnsProxyQueryHandler(CDnsProxyEngine& aEngine, CDnsProxyListener& aListener):CActive(EPriorityStandard),iTimeout(TQueryTimeoutLinkage::TimeoutL),iProxyEngine(aEngine),iListener(aListener)
/**
 * Constructor and adds active object into active scheduler
 * @param aEngine - reference to CDnsProxyEngine
 *
 * @internalTechnology
 **/
	{
	CActiveScheduler::Add(this);
	}

/*
 *  destructor and releases all the resources
 * */
CDnsProxyQueryHandler::~CDnsProxyQueryHandler()
/**
 * Destructor method cancel any pending operations and closes socket.
 *
 * @internalTechnology
 **/
	{
	Cancel();
	DeActivateSocket();
	}


//Invoked when request completes
/*
 * This function RunL will be invoked whenever request completes
 *
 * */
void CDnsProxyQueryHandler::RunL()
/**
 * RunL of this active object is invoked when any pending read/write finishes.
 *
 * @internalTechnology
 **/
	{
	__LOG3("CDnsProxyQueryHandler(%x)::RunL() Status(%d) and State(%d)", this, iStatus.Int(), GetActiveObjectState())		
	if(iStatus.Int() == KErrNone)
		{
		if(GetActiveObjectState() == EInit)
		   {
		   SetTimer(iProxyEngine.GetConfig().iTimerVal);
		   RunReader();
		   }
		else if(GetActiveObjectState() == EComplete)
			{
		    SetActiveObjectState(EInit);
		    CancelTimer();
		    SetQHandlerState(EFalse);
		    iQueryContext->SetQueryState(EResolved);
		    iQueryContext->SetDnsReplyMessage(iReply);
			iProxyEngine.GetProxyListener()->SendTo(iQueryContext);
		    //Query Finished from Handler Perspective
			QueryDone();
			//Process Next Query
			ProcessNextQueryL();
		   	}
		 }
	else
		{
		//Delete the Query context under processing
    	iListener.DeleteQCtxFromListL(iQueryContext);
    	//Complete the query
		QueryDone();
		//Process Next Query
		ProcessNextQueryL();
    	}	 
	}

void CDnsProxyQueryHandler::ProcessQueryL()
/**
 * This function gets called from the listener to start processing the query once
 * it is assigned to the query handler.
 * The function builds the dns server list, if there is not existing one.In case there
 * are no DNS servers available/configured on the interfaces it sends DNS server failure
 * as there is no uplink available.
 * In case server count is zero or we are finished with all the configured servers
 * reset the server count and send dns server failure message.
 * Also check if there is anything else pending in the queue for processing.
 *
 * @internalTechnology
 **/
	{
	/**
	 * Build the server list here. This gives available DNS server addresses on the
	 * already up interfaces.
	 */
	__LOG("\n CDnsProxyQueryHandler::ProcessQueryL Entry")
	
	if(iListener.GetConfiguredUplinkIap() != KErrNone)
		{
		//No Uplink available, hence send Dns server failure message
    	__LOG("\n CDnsProxyQueryHandler::ProcessQueryL :No Uplink Available")
    	SendDnsFailureL();
    	return;
		}
    
	TInt count = iProxyEngine.iProxyServer->GetServerCount();
	if((count == 0) || (iListener.GetUplinkIap() != iCurrentUplink))
		{
		__LOG("\n CDnsProxyQueryHandler::ProcessQueryL Server Count is Zero")
		iCurrentUplink = iListener.GetUplinkIap();
		iProxyEngine.iProxyServer->BuildGlobalDnsServerListL(iCurrentUplink);
		count = iProxyEngine.iProxyServer->GetServerCount();
		__LOG1("\n CDnsProxyQueryHandler::ProcessQueryL Server Count is :%d", count)
		}
	//In case count is greater than zero do further processing else send dns failure message
	if (count > 0)
		{
		TDnsServerInfo* info = NULL;
		do 
			{
			//Get DNS server information from the list
			info = iProxyEngine.iProxyServer->GetDnsServerInfo(iCurServer);

			//coverity[new_values]
			if(info)
				{
				//Set Uplink connection information in context
				iListener.SetUplinkConnectionInfo(info->iConnInfo);
				//Attach to given Uplink information 
				TInt err = iListener.ActivateUplink();
				if(err == KErrNone)
					{
					//Activate the socket function
					ActivateSocketL();
					//Send Query to Dns server address
					SendQuery(info->iAddr);
					}
				else
					{
					//In case uplink is down, send Dns failure
					__LOG("\n CDnsProxyQueryHandler::ProcessQueryL Unable to activate uplink")
					SendDnsFailureL();
					}
				break;
				}
			else
				{
				//Loop through the available list
				iCurServer++;
				if(iCurServer == count)
					{
					__LOG("\n CDnsProxyQueryHandler::ProcessQueryL Invalid Uplinks")
					ResetServerCount();	
					SendDnsFailureL();
					}
					
				}
			}
		//coverity[dead_error_line]
		//coverity[dead_error_condition]
		while(info != NULL && iCurServer < count);
		}
    else
    	{
    	//No Uplink available, hence send Dns server failure message
    	__LOG("\n CDnsProxyQueryHandler::ProcessQueryL :No Uplink Available")
    	SendDnsFailureL();
    	}		
	__LOG("\n CDnsProxyQueryHandler::ProcessQueryL Exit")
	}
			
//Method to cancel outstanding request of the active object
void CDnsProxyQueryHandler::DoCancel()
/**
 * This method is an implementation of DoCancel method of an active object
 * Cancels pending read/ write operations and cancel the timer if active.
 *
 * @internalTechnology
 **/
	{
	if (GetQHandlerState() != EFalse)
		{
		if(GetActiveObjectState() == EInit)
			{
		    iSocket.CancelSend();
			}
		else
			{
			CancelTimer();
			iSocket.CancelRecv();
			}
		}
	}


void CDnsProxyQueryHandler::CancelTimer()
/**
 * This method cancels the timer if already active and gets called from DoCancel method.
 * Also it resets the timer count.
 *
 * @internalTechnology
 **/
	{
	if(iTimeout.IsActive())
		{
		iTimeout.Cancel();
		}
	iTimerCount = 0; //ReInitialize the Timer Count

	}

void CDnsProxyQueryHandler::SetQueryContext(TQueryContext* aQueryContext)
/**
 * This method sets the query context to be processes for this query handler.
 *
 * @internalTechnology
 **/
	{
	iQueryContext = aQueryContext;
	}

void CDnsProxyQueryHandler::SetTimer(TUint aTime)
/**
 * This method sets the timer for the query whose response is pending.
 * @param aTime - the value of timer
 *
 * @internalTechnology
 **/
	{

	iTimeout.Set(iProxyEngine.GetProxyListener()->iTimer,aTime);

	}

TBool CDnsProxyQueryHandler::GetQHandlerState()
/**
 * This method gets the current state of query handler.
 *
 * @internalTechnology
 **/
	{
	return iQueryHandlerStatus;
	}


void CDnsProxyQueryHandler::SetQHandlerState(TBool aQueryHandlerStatus)
/**
 * This method sets the current state of query handler.
 * @param aQueryHandlerStatus - New state
 *
 * @internalTechnology
 **/
	{
	iQueryHandlerStatus = aQueryHandlerStatus;
	}

/*
 * Sets the time out value to specified object
 * @param aNow - time out value
 * */
void CDnsProxyQueryHandler::TimeoutL(const TTime& /*aNow*/)
/**
 * Sets the time out value to specified object
 * @param aNow - time out value
 *
 * @internalTechnology
 **/
	{
	__LOG1("CDnsProxyQueryHandler(%x)::TimeoutL:Query Timed Out", this)
	
	// If already active cancel any pending request.
	Cancel();

	//Compare the timer count with the configured retry count
	if(iTimerCount < iProxyEngine.GetConfig().iRetryCount)
		{
		//Set active object state to Init
		SetActiveObjectState(EInit);
		// increment timer(retry count) count
		iTimerCount++;
		// Move to next server in the list
		iCurServer++;
		//Process the query again.
		ProcessQueryL();
		}
	else
	    {
	    //See if next query available in the queue to handle and deletes
	    // the query handler
	    QueryDone();
	    //Delete the Query context under processing
		iListener.DeleteQCtxFromListL(iQueryContext);
	    //Process Next Unassigned query in the list if any
	    ProcessNextQueryL();
	    }
	}

void CDnsProxyQueryHandler::ProcessNextQueryL()
/**
 * The method fetches next available query context in the list for processing.
 * and starts processing the same.
 * @param None
 *
 * @internalTechnology
 **/
	{
    __LOG1("CDnsProxyQueryHandler(%x)::ProcessNextQueryL() Entry", this)
	TQueryContext *qCtx = iListener.GetQCtxFromList(EUnAssigned);

	if((qCtx != NULL) && (qCtx->GetGlobalScope() == 1))
	    {
	    //Set the query state to assigned from unassigned.
		qCtx->SetQueryState(EAssigned);
		//Set query context for the Qhandler
 		SetQueryContext(qCtx);
 		//Process Query Context
 		ProcessQueryL();
	    }
	 __LOG1("CDnsProxyQueryHandler(%x)::ProcessNextQueryL() Exit", this)   
	    
	}

void CDnsProxyQueryHandler::ActivateSocketL()
/**
 * The method closes the socket if already open. It then opens the socket and binds
 * the same to new random port. The query handler status is ETRUE which means it's
 * processing the query.
 *
 *
 * @internalTechnology
 **/
	{
	__LOG1("CDnsProxyQueryHandler(%x)::ActivateSocketL() Entry", this)
	if(iActiveStatus == EFalse)
		{
		iActiveStatus = ETrue;
		SetQHandlerState(ETrue);
		User::LeaveIfError(iSocket.Open(iProxyEngine.GetSocketServer(), KAfInet, KSockDatagram, KProtocolInetUdp,iListener.GetUplinkConnHandle()));	
 		BindRandomPort();
		}
    __LOG1("CDnsProxyQueryHandler(%x)::ActivateSocketL() Exit", this)		
	}

void CDnsProxyQueryHandler::DeActivateSocket()
/**
 * The method closes the socket if already open.
 *
 * @internalTechnology
 **/
	{
	__LOG2("CDnsProxyQueryHandler(%x)::DeActivateSocketL() Entry, Active Status(%d)", this, iActiveStatus)
	if(iActiveStatus)
		{
		iSocket.CancelSend();
		iSocket.CancelRecv();
		iSocket.Close();
		iActiveStatus = EFalse; 
		}
    __LOG2("CDnsProxyQueryHandler(%x)::DeActivateSocketL() Exit, Active Status(%d)", this, iActiveStatus)		
    }

void CDnsProxyQueryHandler::SendQuery(TInetAddr& aServerAddress)
/**
 * The method sends the query under processing to the external DNS server.
 * @param aServerAddress -Address of the DNS server to which query needs to be send
 *
 * @internalTechnology
 **/
	{
	__LOG1("CDnsProxyQueryHandler(%x)::SendQuery() Entry", this)		
	if(IsActive())
		{
		__LOG1("CDnsProxyQueryHandler(%x)::SendQuery() is already active", this)		
		return;
		}

    SetActiveObjectState(EInit);
	TPtrC8 msg = iQueryContext->GetDnsQueryMessage();
	iSocket.SendTo(msg,aServerAddress,0,iStatus);
	SetActive();
	__LOG1("CDnsProxyQueryHandler(%x)::SendQuery() Exit", this)		
	}

void CDnsProxyQueryHandler::SendDnsFailureL()
/**
 * The method sends the DNS server failure to the querying host.
 * @param none
 *
 * @internalTechnology
 **/
	{
	__LOG1("CDnsProxyQueryHandler(%x)::SendDnsFailureL() Entry", this)
	TBuf8<KDnsMaxMessage> buf = iQueryContext->GetDnsQueryMessage();
	TMsgBuf &msgbuf = TMsgBuf::Cast(buf);	
	
	iListener.GetMessageProc().GetDnsFailureMessage(msgbuf);
		
	SetQHandlerState(EFalse);
	iQueryContext->SetQueryState(EResolved); 
	iQueryContext->SetDnsReplyMessage(msgbuf);
		
	iProxyEngine.GetProxyListener()->SendTo(iQueryContext);
	//Query Finished from Handler Perspective
	QueryDone();
	//Process Next Query
	ProcessNextQueryL();
	__LOG1("CDnsProxyQueryHandler(%x)::SendDnsFailureL() Exit", this)		
	}

void CDnsProxyQueryHandler::QueryDone()
/**
 * The method resets the Queryhandler.
 *
 * @internalTechnology
 **/

	{
    __LOG1("CDnsProxyQueryHandler(%x)::QueryDone() Entry", this)		
    Cancel();
    
    //Cancels timer if active
    CancelTimer();
    //Deactivates socket
	DeActivateSocket();
	iListener.DeActivateUplink();
	ResetSockActiveStatus();
	ResetServerCount();
	ResetTimerCount();
	SetQueryContext(NULL);
	SetActiveObjectState(EInit);
	//Set QHandler state to False which means it is available for processing next
	SetQHandlerState(EFalse);
	__LOG1("CDnsProxyQueryHandler(%x)::QueryDone() Exit", this)		

	}

void CDnsProxyQueryHandler::RunReader()
/**
 * The method initiates receive request for reading DNS response.
 *
 * @internalTechnology
 **/
	{
	__LOG1("CDnsProxyQueryHandler(%x)::RunReader() Entry", this)		
	// If already active then return
	TBool is_active = IsActive();
	if(is_active)
		{
	    return;
		}

	// Set socket status to Ecomplete.
	SetActiveObjectState(EComplete);

	iSocket.RecvFrom(iReply,iFrom,0,iStatus);
	SetActive(); // Set the active object to active.
	__LOG1("CDnsProxyQueryHandler(%x)::RunReader() Exit", this)		
	}

void CDnsProxyQueryHandler::BindRandomPort()
/**
 * This method generates the random port number and binds the socket to it.
 * This function will be removed as it can be left to TCP/IP stack itself.
 *
 * @internalTechnology
 * */
	{
	TInt64 seed =0;
	TInt err = 0;
	TInt min_num = 1100; //because standard port number range is:1 to 1024
	TInt max_min = 65500;
	TTime time;
	TInt64 	iRandom_port;

	do
		{
		time.UniversalTime();
		seed = time.Int64();

		TInt port_num = Math::Rand(seed);
		TInt res = max_min - min_num;
		TInt val = port_num % res; //if it fails then use min_num instead res
		iRandom_port = val+min_num;

		TInetAddr inetaddr2;
		inetaddr2.SetPort(iRandom_port);

		err = iSocket.Bind(inetaddr2);
		}while(err!=KErrNone);
	}

TInt CDnsProxyQueryHandler::RunError(TInt aErr)
/**
 * This method implements RunError method of an active object. This helps in debugging.
 *
 * @internalTechnology
 * */
	{
    __LOG2("CDnsProxyQueryHandler(%x)::RunError %d", this, aErr)		
    if(iQueryContext)
		{
		//Delete the Query context under processing
    	TRAPD(err,iListener.DeleteQCtxFromListL(iQueryContext));
    	__LOG1("DeleteQCtxFromListL::RunError %d",err)
    	}
    //Complete the query
	QueryDone();
    //Flush all the queries in queue
	iListener.DeleteAllQCtxFromList();
	    	
	return KErrNone;
	}

TStatus CDnsProxyQueryHandler::GetActiveObjectState()
/**
 * This method returns the state of the object
 * @param None
 * @return None
 *
 * @internalTechnology
 * */
	{
	return iSocketStatus;

	}

void CDnsProxyQueryHandler::SetActiveObjectState(TStatus aState)
/**
 * This method returns the state of the object
 * @param None
 * @return None
 *
 * @internalTechnology
 * */
	{
	iSocketStatus = aState;

	}
