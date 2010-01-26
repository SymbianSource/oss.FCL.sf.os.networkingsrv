//dnsproxylistener.h
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
* dnsproxylistener.h - Dns Proxy Reader
* @file
* @internalTechnology
*
*/




#ifndef  DNSPROXY_LISTENER_H
#define  DNSPROXY_LISTENER_H

#include <f32file.h> 
#include "es_sock.h"
#include <timeout.h>
#include "dns_hdr.h"
#include <es_enum.h>
#include "dnsproxyqryhandler.h"

/*
 CDnsProxyListener class is an active object and implements the async function 
 namely ReceFrom ,to get query packet.Once after receiving ,creats resolver object.
 It waits on port 53 to get any incoming query packets.
 @internalTechnology
*/
//forward declaration
class CDnsProxyEngine;
class CDnsProxyWriter;
class CDnsProxyMessageProcessor;

const TInt KLocalQuery = 1;
const TInt KGlobalQuery = 2;
const TUint32 KDnsProxyPort = 53;


enum TQueryState
/*
 * This enum represents the state of query under processing.
 * @internalTechnology
 **/ 

	{
	EUnAssigned,    // This is the initial state wherein query has not yet been assigned for processing
	EAssigned,      // This state represents the query is under processing
	EResolved,      // This state represents the query has been resolved and the response needs to be sent back
	EQueryDone      // Query is Complete and can be deleted.
	};

	
class TQueryContext
/*
 * This class holds the query context for each query. This holds the state of the query, the query itself
 * and originating point of the query.
 * @internalTechnology
 * */ 

	{
	public:
		TQueryState GetQueryState() const;
		void SetQueryState(TQueryState aResState);
		
		TDesC8& GetDnsQueryMessage();
		void SetDnsQueryMessage(const TDesC8& aMsg);
		
		TDesC8& GetDnsReplyMessage();
		void SetDnsReplyMessage(const TDesC8& aMsg);
		
		TSockAddr& GetSourceAddr();
		void SetSourceAddr(const TSockAddr& aSourceAddr);
		
		TBool GetGlobalScope();
		void SetGlobalScope(TBool aGlobalScope);
		
	private:
		TSockAddr               iSourceAddr;
		TQueryState             iQueryState;	
		TBuf8<KDnsMaxMessage>   iDnsReplyMsg;
		TBuf8<KDnsMaxMessage>   iDnsQueryMsg;
		TBool                   iGlobalScope;
	};

class CDnsProxyListener: public CActive
/*
 * This class is the DNS proxy listener class. This waits on standard DNS port 53 for incoming queries, 
 * builds query context for each query and assigns the same to the available query handlers in the list.
 * The class also owns and maintains the list of query contexts and query handlers.
 * and originating point of the query.
 * @internalTechnology
 * */ 

	{
	friend class CDnsProxyWriter;
	public:
		static CDnsProxyListener* NewL(CDnsProxyEngine& aEngine);

	public:
		TInt Activate();
		//Invoked when request compltes
		void RunL();
		//Method to cancel otstanding request of the active object
		void DoCancel();
		CDnsProxyEngine& GetDnsProxyEngine();
		
		void SendTo(TQueryContext* aQueryContext);
		TInt RunError(TInt aErr);
		//Deletes specific context from the list
		void DeleteQCtxFromListL(TQueryContext* aQueryCtx);
		//Deletes all query contexts
		void DeleteAllQCtxFromList();
		TQueryContext* GetQCtxFromList(TQueryState aState);
		void StopListener();
		//Gets interface IP address using iConnection
		TInetAddr GetInterfaceIpAddress();
		//Activate Uplink for DNS Proxy
		TInt ActivateUplink();
		//DeActivate Uplink for DNS Proxy
		void DeActivateUplink();
		//Set Uplink Connection Info in the context
		TInt SetUplinkConnectionInfo(TConnectionInfoBuf& aUplinkConnInfo);
		//Set Uplink Connection Info in the context
		TInt SetUplinkConnectionInfo(const RMessage2& aMessage);
		//Set Downlink Connection Info in the context
		TInt SetDlinkConnectionInfo(const RMessage2& aMessage);
		//Get Uplink IAP Identifier
		TUint32 GetUplinkIap();
		//Get Downlink IAP Identifier
		TUint32 GetDlinkIap();
		RConnection& GetUplinkConnHandle();
		TInt GetConfiguredUplinkIap();
		
		CDnsProxyMessageProcessor& GetMessageProc();
		~CDnsProxyListener();	
	
	private:
		CDnsProxyListener(CDnsProxyEngine& aEngine);
		void ConstructL();
		void InitQHandlerListL();
		CDnsProxyQueryHandler* GetQHandlerFromList();
		TInt KeepListening();	
		void ProcessQPacketL(TQueryContext* aQueryContext);
		void AddtoListL();
		TBool IsUplinkAvailable();
		//Set uplink IAP identifier
		TInt SetUplinkIapId();
		void SetUplinkIapId(TUint32 aIapId);
		TInt SetDlinkIapId();
		TConnectionInfoBuf& GetUplinkConnInfo();
		TConnectionInfoBuf& GetDownlinkConnInfo();
			
	public:
		RPointerArray<CDnsProxyQueryHandler> iQueryHandlerList;
		RPointerArray<TQueryContext> iQueryContext;
		//generic timer handler.
		MTimeoutManager* iTimer;
				
			
	private:
		RSocket 				        iListenerSocket;
		RSocket                         iNaptSocket;
		TInetAddr 				        iLocalAddr;	
		TSockAddr  				        iFrom;
		CDnsProxyMessageProcessor* 		iMsgProc;
		CDnsProxyWriter* 		        iProxyWriter;
		RConnection 			        iConnection;
		//this buffer will store query packet
		TBuf8<KDnsMaxMessage> 			iBuf;	 			
		CDnsProxyEngine& 		        iProxyEngine;
		TBool                           iUplinkAvailable;      //< Uplink Availability
		RConnection                     iUplinkConnection;    //< Uplink connection handle
		TConnectionInfoBuf          	iUplinkConnInfo;    //< Uplink connection info
	    TConnectionInfoBuf          	iDlinkConnInfo;     //< Connection Info passed from DHCP
		TUint32 						iUplinkIapId;
		TUint32 						iDlinkIapId;
		TInt                            iActiveSocket;
	};
	
class TQueryTimeoutLinkage : public TimeoutLinkage<CDnsProxyQueryHandler, _FOFF(CDnsProxyQueryHandler, iTimeout)>
	{
public:
	static void TimeoutL(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
		{
		Object(aLink)->TimeoutL(aNow);
		}
	};

#endif /*DNSPROXY_LISTENER_H*/
