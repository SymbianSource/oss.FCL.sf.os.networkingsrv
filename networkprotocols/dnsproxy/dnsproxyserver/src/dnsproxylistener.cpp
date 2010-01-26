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
// This class waits on port53 to receive dns queries
// and creates the socket session for a query based on
// maximum possible sessions as configured in a.ini file.
// This class is an active object
//



/**
 @file
 @internalTechnology
*/
#include <commdbconnpref.h>
#include <nifman.h>
#include <comms-infras/es_config.h>
#include "inet6log.h"
#include <cflog.h>
#include "dns_qry_internal.h"
#include "dnsproxyengine.h"
#include "dnsproxylistener.h"
#include "dnsproxywriter.h"
#include "dnsproxymsgproc.h"
#include "dnsproxylog.h"
#include "naptinterface.h"

CDnsProxyListener* CDnsProxyListener::NewL(CDnsProxyEngine& aEngine)
/**
 * This is two phase construction
 * @param aEngine- reference of type CDnsProxyEngine
 * @leave - systemwide panics
 *
 * @internalTechnology
 */
	{
	CDnsProxyListener* aProxyReader = new(ELeave)CDnsProxyListener(aEngine);
	CleanupStack::PushL(aProxyReader);
	aProxyReader->ConstructL();
	CleanupStack::Pop();
	return aProxyReader;
	}


CDnsProxyListener::CDnsProxyListener(CDnsProxyEngine& aEngine):CActive(EPriorityStandard),iProxyEngine(aEngine)
/**
 * Constructor which initializes some of member variables
 * @param aEngine - reference of type CDnsProxyEngine
 *
 * @internalTechnology
 */
	{
	}

CDnsProxyListener::~CDnsProxyListener()
/**
 * This is destructor and deletes all the member variables
 * calls cancel method to cancel all out standing requests and close all open sockets
 *
 * @internalTechnology
**/
	{
	Cancel();

	delete iProxyWriter;
	delete iMsgProc;
	delete iTimer;
    iNaptSocket.Close();
	iListenerSocket.Close();
	iConnection.Close();
	iQueryContext.ResetAndDestroy();
	iQueryHandlerList.ResetAndDestroy();
	}

void CDnsProxyListener::ConstructL()
/**
 * This is second phase construction
 * This active object shall be added into scheduler and waits on port 53 to
 * receive any incoming dns packets
 * and also creates other objects which may leave due to some reason
 * @leave - systemwide errors
 *
 * @internalTechnology
**/
	{
	CActiveScheduler::Add(this);
	//Create an instance Message processor for response construction
	iMsgProc = CDnsProxyMessageProcessor::NewL(*this);
	// Create an instance of writer which sends back responses to querying
	// host on listener socket.
	iProxyWriter = CDnsProxyWriter::NewL(*this);
	// Setup the timeout manager
	iTimer = TimeoutFactory::NewL();
	//Initialize configured number of Query handlers
	InitQHandlerListL();
	//Initialize socket on Napt
	User::LeaveIfError(iNaptSocket.Open(iProxyEngine.GetSocketServer(), _L("napt")));
	}

void CDnsProxyListener::InitQHandlerListL()
/**
 * This method initalized Query handlers and append them to the list based
 * on configured session count.
 *
 * @param None
 *
 * @internalTechnology
**/
	{
	const TDnsProxyConfigParams& cf = iProxyEngine.GetConfig();

	for(TInt count = 0; count < cf.iSessionCount; count++)
 	 	{
 	 	//Create an instance of query handler to handle query
 	 	CDnsProxyQueryHandler*	qHandler = CDnsProxyQueryHandler::NewL(iProxyEngine, *this);
  	 	if(qHandler)
 	 		{
 	 		//Set the query handler to EFalse that is it not in use
 	 		qHandler->SetQHandlerState(EFalse);
 	 		CleanupStack::PushL(qHandler);
 	 		//Append the Query to the list
 	 		iQueryHandlerList.AppendL(qHandler);
 	 		CleanupStack::Pop(qHandler);
 	 		}
	 	}
	}

TInt CDnsProxyListener::Activate()
/**
 * This function opens the socket and attaches it to the passed connection.
 * @leave - system wide errors
 *
 * @internalTechnology
**/
	{
	TInt err;
	err = iConnection.Open(iProxyEngine.GetSocketServer());
	if(err != KErrNone)
		return err;
	
	err = iConnection.Attach(GetDownlinkConnInfo(), RConnection::EAttachTypeNormal);
	if(err != KErrNone)
		return err;
	
 	err = iListenerSocket.Open(iProxyEngine.GetSocketServer(), KAfInet, KSockDatagram, KProtocolInetUdp,iConnection);
	if(err != KErrNone)
		return err;
	
	 //on this port, will get UDP packet
	iLocalAddr.SetPort(KDnsProxyPort);
	err = iListenerSocket.Bind(iLocalAddr);
	if(err != KErrNone)
		return err;
	
	err = KeepListening();
	if(err != KErrNone)
		return err;
	
	return err;
	}

TInetAddr CDnsProxyListener::GetInterfaceIpAddress()
/**
 * This method gets the IP address of the interface where DNS proxy is listening.
 * @param - None
 * @return - TInetAddr
 *
 * @internalTechnology
 **/
	{

	TRequestStatus status;
	TConnectionAddrBuf address;
	TInetAddr ip;
	//Internet family type
	address().iAddressFamily = KAfInet;

	//Get the IP address of the interface
	iConnection.Ioctl(KCOLConfiguration, KConnGetCurrentAddr, status, &address);
	User::WaitForRequest(status); //
	if(KErrNone == status.Int())
		{
		//Set ip address.
		ip = TInetAddr::Cast(address().iAddr);
		}

	return ip;

	}

TInt CDnsProxyListener::KeepListening()
/**
 * This method listens on standard DNS port 53 for incoming DNS requests.
 * @leave - system wide errors
 *
 * @internalTechnology
**/

	{
	LOG(Log::Printf(_L("\t\n --- Listening on Port-53")));

	if(!IsActive())
		{
		iListenerSocket.RecvFrom(iBuf,iFrom,0,iStatus);
		SetActive();
		return KErrNone;
		}
	else
		return KErrInUse;	
	}
/*
* This is called by scheduler once it completes the event
* Once packets is received then socket session object will be created
* After that again it will wait on port 53
*/
void CDnsProxyListener::RunL()
/**
 * This is called by scheduler once it completes the event
 * Once packets is received then query handler object is created
 *
 * @internalTechnology
**/
	{
	if(iStatus.Int() == KErrNone)
		{
		AddtoListL();
		}
	KeepListening();
	}

void CDnsProxyListener::AddtoListL()
/**
 * This function creates the query context object and adds the same to the queue.
 * for further processing. In case queue size has reached its limit query is dropped.
 * The initial state of the query is UNASSIGNED state which means query has not yet
 * been assigned for further processing
 *
 * @internalTechnology
**/
	{
	const TDnsProxyConfigParams &cf = GetDnsProxyEngine().GetConfig();
	//In case queue is already full drop the query
	if(iQueryContext.Count() >= cf.iQSize)
		{
		LOG(Log::Printf(_L("\t \n --- CDnsProxyListener:List is full")));
		return;
		}


	//first put the query into the list for further processing.
	TQueryContext* aQueryContext = new (ELeave) TQueryContext();
	CleanupStack::PushL(aQueryContext);
	aQueryContext->SetQueryState(EUnAssigned);
	aQueryContext->SetDnsQueryMessage(iBuf);
	aQueryContext->SetSourceAddr(iFrom);

	TMsgBuf& msgbuf = TMsgBuf::Cast(iBuf);

	TInt suffix = iMsgProc->GetQNameSuffixL(msgbuf);
	if(suffix == KLocalQuery)
		aQueryContext->SetGlobalScope(EFalse);
	else
	    aQueryContext->SetGlobalScope(ETrue);

	iQueryContext.AppendL(aQueryContext);
	CleanupStack::Pop(aQueryContext);

	//process the packet
	ProcessQPacketL(aQueryContext);
	}

void CDnsProxyListener::ProcessQPacketL(TQueryContext* aQueryContext)
/**
 * This function creates the session object and passes the query packet
 * to the socket session. The socket session is responsible for resolving the query
 * and sending the response back to the querying host.In case of local domain name
 * resolution query the response is send back to the querying host.
 *
 * @param aQueryContext
 *
 * @internalTechnology
 **/

	{
    //Get configuration parameters
    GetDnsProxyEngine().GetConfig();
	if(aQueryContext->GetGlobalScope() == EFalse)
		{
		TMsgBuf& msgbuf = TMsgBuf::Cast(iBuf);
		iMsgProc->GetLocalDnsResponseL(msgbuf);

		aQueryContext->SetQueryState(EResolved);                  
		aQueryContext->SetDnsReplyMessage(msgbuf);

		SendTo(aQueryContext);
 		}
 	else
 	 	{
 	 	//Get the available Query handler from List
 	 	CDnsProxyQueryHandler* qHandler = GetQHandlerFromList();

 	 	if(qHandler)
 	 		{
 	 		//Set the query state to assigned from unassigned.
			aQueryContext->SetQueryState(EAssigned);
			//Set query context for the Qhandler
 	 		qHandler->SetQueryContext(aQueryContext);
 	 		//Process Query Context
 	 		qHandler->ProcessQueryL();

 	 		}
	     }
	}

// This function returns an instance of Engine object
CDnsProxyEngine& CDnsProxyListener::GetDnsProxyEngine()
/**
 * This method returns an instance of Dns Engine
 *
 * @internalTechnology
 **/
	{
	return iProxyEngine;
	}

CDnsProxyQueryHandler* CDnsProxyListener::GetQHandlerFromList()
/**
 * This method returns the available query handler from the Query handler list
 *
 * @internalTechnology
 **/
	{
	TInt index = 0;
	CDnsProxyQueryHandler* qh = NULL;
	//Check the list for available query handler
	while(iQueryHandlerList.Count() > index)
		{
   		if(iQueryHandlerList[index]->GetQHandlerState() == EFalse)
   			{
   			qh = iQueryHandlerList[index];
   			break;
   			}
   		index++;
		}
		return qh;
	}

void CDnsProxyListener::DeleteQCtxFromListL(TQueryContext* aQueryCtx)
/**
 * This method deletes or removes specific query context from list
 * @param aQueryCtx - Pointer to TQueryContext to be deleted
 * @internalTechnology
 **/
	{
	TInt index = 0;
	__LOG1("CDnsProxyListener::::DeleteQCtxFromListL, Queue Count before deletion::%d", iQueryContext.Count());
	index = iQueryContext.FindL(aQueryCtx);
	iQueryContext.Remove(index);
    iQueryContext.Compress();
	__LOG1("CDnsProxyListener::DeleteQCtxFromList Index is::%d", index);
	__LOG1("CDnsProxyListener::::DeleteQCtxFromListL, Queue Count after deletion::%d", iQueryContext.Count());
	delete aQueryCtx;
	}

void CDnsProxyListener::DeleteAllQCtxFromList()
/**
 * This method deletes all the queries in the list.
 * @param None.
 * @return None
 * @internalTechnology
**/
	{
	TInt index = 0;

	__LOG1("CDnsProxyListener::DeleteAllQCtxFromList, Queue count in DeleteAllQCtxFromList::%d", iQueryContext.Count());
	for (index = 0; index < iQueryContext.Count(); index++)
		{
		if (iQueryContext[index]->GetQueryState() != EAssigned)
			{
			delete iQueryContext[index];
		    iQueryContext[index] = NULL;
		    iQueryContext.Remove(index);
			}
		}
    iQueryContext.Compress();
	}
	
CDnsProxyMessageProcessor& CDnsProxyListener::GetMessageProc()
/**
 * This method returns reference to the message proc
 * @param None.
 * @return None
 * @internalTechnology
**/
	{
	return *iMsgProc;
	}
	
// This function deletes Query from the queue
TQueryContext* CDnsProxyListener::GetQCtxFromList(TQueryState aState)
/**
 * This method gets query with specified state from the query context list
 * @param aState - Current state of the query
 *
 * @internalTechnology
 **/
	{
	TInt index = 0;
	TQueryContext* qCtx = NULL;
	while(iQueryContext.Count() > index)
		{
		TQueryState queryState = iQueryContext[index]->GetQueryState();

		if(queryState == aState)
			{
			qCtx = iQueryContext[index];
		   	break;
			}
		index++;
		}
	return qCtx;
	}

void CDnsProxyListener::DoCancel()
/**
 * This will cancel pending receive operation on listener socket
 *
 * @internalTechnology
 **/
	{
	iListenerSocket.CancelRecv();
	}

void CDnsProxyListener::StopListener()
/**
 * This will cancel all pending read and write in DNS Proxy implementation.
 * The method is called at the time of DNS Proxy shutdown.
 *
 * @internalTechnology
 **/
	{
	Cancel();
	iProxyWriter->Cancel();
	for (TInt index = 0; index < iQueryHandlerList.Count(); index++)
		{
		iQueryHandlerList[index]->Cancel();
		}
	}

void CDnsProxyListener::SendTo(TQueryContext* aQueryContext)
/**
 * This method sends the DNS response packet to the querying host.
 * The method is called at the time of DNS Proxy shutdown.
 * @param queryContext - Query context of the query to be send.
 *
 * @internalTechnology
 **/
	{
	iProxyWriter->WriteTo(aQueryContext);
	}

TInt CDnsProxyListener::RunError(TInt /*aErr*/)
/**
 * This is called whenver RunL leaves
 * @param aErr - contains leave reason number
 * @return - KErrNone
 *
 * @internalTechnology
 **/
	{
	DeleteAllQCtxFromList();
	return KErrNone;
	}

// This method returns the query state.
TQueryState TQueryContext::GetQueryState() const
/**
 * This methods gets the state of the query under processing.
 * @param none
 * @return - iQueryState - current state of the query
 *
 * @internalTechnology
 **/

	{
	return iQueryState;
	}

//This method sets the query state to the given value
void TQueryContext::SetQueryState(TQueryState aQueryState)
/**
 * This methods sets the state of the query under processing.
 * @param aQueryState - query state
 * @return - none
 *
 * @internalTechnology
 **/
	{
	iQueryState = aQueryState;
	}

//This methods returns the pointer to received query
TDesC8& TQueryContext::GetDnsQueryMessage()
/**
 * This methods gets the query under processing.
 * @param - none
 * @return - reference to the query buffer
 *
 * @internalTechnology
 **/

	{
	return iDnsQueryMsg;
	}

//This methods sets the dns query message in context of query
void TQueryContext::SetDnsQueryMessage(const TDesC8& aMsg)
/**
 * This methods sets the query reference in the query context.
 * @param aMsg - Query buffer
 * @return - None
 *
 * @internalTechnology
 **/
	{
	iDnsQueryMsg.Copy(aMsg);
	}

// This method returns the query reply message
TDesC8& TQueryContext::GetDnsReplyMessage()
/**
 * This methods gets the DNS response message from the query context.
 * @param - None
 * @return - reference of the response buffer
 *
 * @internalTechnology
 **/
	{
	return iDnsReplyMsg;
	}

// This method sets the query reply message in the query context
void TQueryContext::SetDnsReplyMessage(const TDesC8& aMsg)
/**
 * This methods sets the DNS response message in the query context.
 * This copies query from the receive buffer to the query context buffer.
 * @param aMsg - query buffer
 * @return - none
 *
 * @internalTechnology
 **/
	{
	iDnsReplyMsg.Copy(aMsg);
	}

// The method returns the address of querying host
TSockAddr& TQueryContext::GetSourceAddr()
/**
 * This methods gets the address of the queying host from which query has been
 * received.
 *
 * @param - none
 * @return - none
 *
 * @internalTechnology
 **/

	{
	return iSourceAddr;
	}

//This method sets the address of querying host
void TQueryContext::SetSourceAddr(const TSockAddr& aSourceAddr)
/**
 * This methods sets the address of the queying host from which query has been
 * received in the query context.
 *
 * @param aSourceAddr - port and ip address from which query is received.
 * @return - none
 *
 * @internalTechnology
 **/
	{
	iSourceAddr = aSourceAddr;
	}

// Query scope identifier: global name query or local name query
TBool TQueryContext::GetGlobalScope()
/**
 * This methods gets scope of the query from the query context.
 * The query can be a global name resolution query
 * or it can be a local name resolution query.
 * @param - none
 * @return - none
 *
 * @internalTechnology
 **/
	{
	return iGlobalScope;
	}

// Query scope identifier: global name query or local name query
void TQueryContext::SetGlobalScope(TBool aGlobalScope)
/**
 * This methods sets the scope of the query in the query context.
 * The query can be a global name resolution query
 * or it can be a local name resolution query.
 * @param aGlobalScope - Scope identifier
 * @return - none
 *
 * @internalTechnology
 **/
	{
	iGlobalScope = aGlobalScope;
	}
	
	

TInt CDnsProxyListener::ActivateUplink()
/*
 * This method opens a connection and attaches itself to existing connection. 
 * @param  aUplinkConnInfo - Uplink connection information to attach the connection
 * @return None
 *
 * @internalTechnology
 **/
	{
	TRequestStatus astatus;
	__LOG("\n CDnsProxyListener::Activate Uplink Entry")
	TInt err = KErrNone;
	if(!IsUplinkAvailable())
		{
		err = iUplinkConnection.Open(iProxyEngine.GetSocketServer());
		if(err == KErrNone)
			{
			err = iUplinkConnection.Attach(GetUplinkConnInfo(), RConnection::EAttachTypeNormal);
			if (err == KErrNone)
			  	iUplinkAvailable = ETrue;
			}
		}
	__LOG("\n CDnsProxyListener::Activate Uplink Exit")
    return err;
	}

void CDnsProxyListener::DeActivateUplink()
/*
 * This method opens a connection and attaches itself to the existing connection. 
 * @param  None
 * @return None
 *
 * @internalTechnology
 **/
	{
	iUplinkConnection.Close();
	iUplinkAvailable = EFalse;
	}	
	
TBool CDnsProxyListener::IsUplinkAvailable()
/**
 * This method checks the availability of uplink. 
 * @param  None
 * @return iUplinkAvailable
 *
 * @internalTechnology
 **/
	{	
	__LOG("\n CDnsProxyListener::IsUplinkAvailable Entry/ Exit")
	return iUplinkAvailable;
  	
	}

RConnection& CDnsProxyListener::GetUplinkConnHandle()
/**
 * This method returns uplink connection handle. 
 * @param  None
 * @return iUplinkConnection
 *
 * @internalTechnology
 **/

	{
	__LOG("\n CDnsProxyListener::GetUplinkConnHandle Entry/ Exit")
	return iUplinkConnection;
	}


TInt CDnsProxyListener::SetDlinkConnectionInfo(const RMessage2 &aMessage)
/**
 * This method sets Dlink Connection Info. 
 * @param  None
 * @return TInt
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyListener::SetDlinkConnectionInfo Entry")
	aMessage.Read(0, iDlinkConnInfo);
	TInt err = SetDlinkIapId();
	__LOG("\n CDnsProxyListener::SetDlinkConnectionInfo End")
	return err;
	}

TInt CDnsProxyListener::SetDlinkIapId()
/**
 * This method sets Dlink IAP id. 
 * @param  None
 * @return TInt
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyListener::SetDlinkIapId Entry")
	const TConnectionInfo* info = reinterpret_cast<const TConnectionInfo*>(iDlinkConnInfo.Ptr());
	
	if(iDlinkConnInfo.Length() != sizeof(TConnectionInfo))
		return KErrArgument;
	else
		iDlinkIapId = info->iIapId;	
	__LOG("\n CDnsProxyListener::SetDlinkIapId Exit")
	return KErrNone;
	}	
	


TInt CDnsProxyListener::SetUplinkConnectionInfo(const RMessage2 &aMessage)
/**
 * This method reads uplink connection info from the message. 
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/

	{
	TInt err;
	__LOG("\n CDnsProxyListener::SetUplinkConnectionInfo Entry")
	
	err = aMessage.Read(0, iUplinkConnInfo);
	
	if(err == KErrNone) 
		err = SetUplinkIapId();
	
	__LOG("\n CDnsProxyListener::SetUplinkConnectionInfo Exit")
	return err;
    }   
    
TInt CDnsProxyListener::SetUplinkConnectionInfo(TConnectionInfoBuf& aUplinkConnInfo)
/**`
 * This method reads uplink connection info from the message. 
 * @param aMessage - Message send by the client
 * @return - None
 *
 * @internalTechnology
 **/

	{
	TInt err = KErrNone;
	__LOG("\n CDnsProxyListener::SetUplinkConnectionInfo Entry")
	if (iUplinkConnInfo == aUplinkConnInfo)
		{
		__LOG("\n CDnsProxyEngine::SetUplinkConnectionInfo Connection Info is same as existing one")	
		}
	else
		{
		DeActivateUplink();
		iUplinkConnInfo = aUplinkConnInfo;
		err = SetUplinkIapId();
		}
	__LOG("\n CDnsProxyListener::SetUplinkConnectionInfo Exit")
	return err;
    }        

	
TInt CDnsProxyListener::SetUplinkIapId()
/**
 * This method sets Uplink IAP id. 
 * @param  None
 * @return TInt
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyListener::SetUplinkIapId Entry")

	const TConnectionInfo* info = reinterpret_cast<const TConnectionInfo*>(iUplinkConnInfo.Ptr());
	if(iUplinkConnInfo.Length() != sizeof(TConnectionInfo))
		return KErrArgument;
	else
		iUplinkIapId = info->iIapId;
	__LOG("\n CDnsProxyListener::SetUplinkIapId Exit")
	return KErrNone;
	}
	
void CDnsProxyListener::SetUplinkIapId(TUint32 aIapId)
/**
 * This method sets Uplink IAP id. 
 * @param  None
 * @return TInt
 *
 * @internalTechnology
 **/
	{
	__LOG("\n CDnsProxyListener::SetUplinkIapId Entry")

	iUplinkIapId = aIapId;
	__LOG("\n CDnsProxyListener::SetUplinkIapId Exit")
	
	}		
	
TConnectionInfoBuf& CDnsProxyListener::GetUplinkConnInfo()
/**
 * This method gets Uplink Conn Info. 
 * @param  None
 * @return TConnectionInfoBuf&
 *
 * @internalTechnology
 **/
	{
	return iUplinkConnInfo;
	}

TConnectionInfoBuf& CDnsProxyListener::GetDownlinkConnInfo()
/**
 * This method gets Downlink conn info. 
 * @param  None
 * @return TConnectionInfoBuf&
 *
 * @internalTechnology
 **/
	{
	return iDlinkConnInfo;
	}

TUint32 CDnsProxyListener::GetDlinkIap()
/**
 * This method gets Downlink Iap Id. 
 * @param  None
 * @return TUint32
 *
 * @internalTechnology
 **/
	{
	return iDlinkIapId;
	}
	
TUint32 CDnsProxyListener::GetUplinkIap()
/**
 * This method gets Downlink Iap Id. 
 * @param  None
 * @return TUint32
 *
 * @internalTechnology
 **/
	{
	return iUplinkIapId;
	}	
   		
TInt CDnsProxyListener::GetConfiguredUplinkIap()
/**
 * This method gets uplink IAP information from Napt based on downlink IAP Id. 
 * @param aOption - TUplinkInfo structure filled with private IAP Id
 *
 *
 * @internalTechnology
*/
	{
	TPckgBuf<TUplinkInfo> info;
	info().iPrivateIap = GetDlinkIap();
	info().iPublicIap = 0; 
	
	TInt err = iNaptSocket.GetOpt(KSoNaptUplink, KSolNapt, info);
	if((err == KErrNone) && (GetUplinkIap() != info().iPublicIap))
		SetUplinkIapId(info().iPublicIap);
	return err;		
	}
