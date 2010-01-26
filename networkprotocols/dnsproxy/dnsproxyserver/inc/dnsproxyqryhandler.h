//dnsproxyqryhandler.h

/*
* Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* @file
* @internalTechnology
*
*/



#ifndef DNS_PROXYQRYHANDLER_H
#define DNS_PROXYQRYHANDLER_H

#include <in_sock.h>
#include <timeout.h>
#include "dns_hdr.h"

//forward declaration
class CDnsProxyEngine;
class CDnsProxyListener;
class TQueryContext;
class CDnsServerConfig;

/*
 * This enum represents the state of query handler active object.
 * @internalTechnology
 * */ 
 enum TStatus
	{
	EInit,       // This is the initial state of an active object once sendto is called
	EComplete    // This is the response waiting state which means receive is pending
	};
	
class CDnsProxyQueryHandler:public CActive
	{
/*
 * This class sends global query to available uplinks at given time
 * receives response and sends the same back to the querying host using
 * local writer.
 * This class is derived from CActive
 * @internalTechnology
 * */
	public:
		//two phase construction
		static CDnsProxyQueryHandler* NewL(CDnsProxyEngine& aEngine, CDnsProxyListener& aListener);
		//destructor
		~CDnsProxyQueryHandler();
	public:
		//Invoked when request compltes
		void RunL();
		//Method to cancel outstanding request of the active object
		void DoCancel();
		//this will be invoked whenever RunL function leaves
		TInt RunError(TInt aErr);
		void BindRandomPort();
		void SendQuery(TInetAddr& aServerAddress);
		void ProcessQueryL();
		void SetQueryContext(TQueryContext* aQueryContext);
		void RunReader();
		void CancelTimer(); 
		TBool GetQHandlerState();
		void SetQHandlerState(TBool aQueryHandlerStatus);
		
		// A callback method when the timeout expires
		void TimeoutL(const TTime &aNow);
		// Request a call to Timeout after specified time (seconds)
		void SetTimer(TUint aTimeout);
	
	protected:
		//constructor
		CDnsProxyQueryHandler(CDnsProxyEngine& aEngine, CDnsProxyListener& aListener);
		
	private:
		// Process Any pending Query
		void ProcessNextQueryL();
		//Set the state of AO
		void SetActiveObjectState(TStatus aState);
		//Get the state of AO
		TStatus GetActiveObjectState();
		//Reset the Query Handler
		void QueryDone();
		//Reset server count
		void ResetServerCount();
		//Reset timeout count
		void ResetTimerCount();
		//Reset timeout count
		void ResetSockActiveStatus();
		//send DNS server failure to the host if the uplink is not available.
		void SendDnsFailureL();
		//Activate the socket
		void ActivateSocketL();
		//Deactivate the socket
		void DeActivateSocket();
	
	public:
		RTimeout iTimeout;	           // Timer
				
	private:	
		CDnsProxyEngine& iProxyEngine; // Instance of control class
		CDnsProxyListener& iListener;  // Instance of listener class
		RSocket iSocket;               // Outbound socket
		TBool iActiveStatus;           // Status of the socket 
		TBool iQueryHandlerStatus;     // Status of QueryHandler
		TInt iServerCount;             // Number of servers available
		TInt iCurServer;               // Current server in use
		TInt iTimerCount;              // This is current retry count
		TInt iRetryCount;              // This is maximum possible retries
		TQueryContext* iQueryContext;  // Current Query context under processing
		CDnsServerConfig* iProxyServer;// External DNS server configuration  
		TBuf8<KDnsMaxMessage> iReply;  // DNS reply 
		TSockAddr iFrom;               // Query Source
		TInetAddr iServerAddress;      // Current DNS server address
		TUint32 iCurrentUplink;        // Current Uplink in Use
		
		TStatus iSocketStatus;         // Active object state
		
	};
	
inline void CDnsProxyQueryHandler::ResetServerCount()
/**
 *  Reset server count
 *  @internalTechnology
 * */
	{ 
	iCurServer = 0; 
	}
	
inline void CDnsProxyQueryHandler::ResetTimerCount()
/**
 *  Reset timeout count
 *  @internalTechnology
 * */
	{
	iTimerCount = 0; 
	}
		
inline void CDnsProxyQueryHandler::ResetSockActiveStatus()
/**
 *  Reset socket active status
 *  @internalTechnology
 * */
	{ 
	iActiveStatus = EFalse; 
	}
	
#endif /*DNS_PROXYQRYHANDLER_H*/
