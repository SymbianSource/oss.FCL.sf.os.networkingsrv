//dnsproxymessageproc.h
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
* dnsproxymessageproc.h - Dns Proxy message processor
* @file
* @internalTechnology
*
*/




#ifndef _DNSPROXY_MESSAGE_PROCESSOR_H__
#define _DNSPROXY_MESSAGE_PROCESSOR_H__

#include <e32base.h>
#include <es_sock.h>
#include "in_sock.h" 
#include "dnsproxydb.h"
#include "message.h"

_LIT(KPTRSUFFIX,"in-addr.arpa");
const TInt KPTRSufix = 50;
const TInt KPTR		 = 50;

class CDnsProxyListener;
class CDnsProxyMessageProcessor: public CBase
/*
* This class constructs the response packet.
* For constructing the response packet, it will extract the 
* query packets from the queue,  then constructs the packt.
* After constructing the response packet, it will send the constructed
* packet back to write class for sending it to querying hosts.
*
* @internalTechnology
*/
	{
		friend class TDnsProxyNsRecord;
	
	public:
		static CDnsProxyMessageProcessor* NewL(CDnsProxyListener& aListener);
		~CDnsProxyMessageProcessor();
		//constructors the DNS response
		void GetLocalDnsResponseL(TMsgBuf &msgbuf);
		//finds the suffix of the query name
		TInt GetQNameSuffixL(TMsgBuf& aMsg);
		TDnsProxydbElement* LookupPTRInDb(TMsgBuf& aMsg);
		void GetDnsFailureMessage(TMsgBuf& aMsgBuf);		
	
	private:
	    //constructor
		CDnsProxyMessageProcessor(CDnsProxyListener& aListener);
		//destructor
		void GetRecordsFromDb(TMsgBuf& aMsg,TDnsProxydbElement& aElement);
		void SetQHeaderBits(TDndHeader &aHdr);
		//constructs the ANRData as specified in RFC1035 sect4.1.3
		void GetAnRecordSet(TMsgBuf& aQMsg,TDnsProxydbElement& aElement);
		//constructs the NSRdata as specified in RFC1035 section 3.3.13
		void GetNsRecordSetL(TMsgBuf& aQMsg);
		void GetPTRRecord(TMsgBuf& aMsg,const TDesC8& aName);	
	
	private:
		CDnsProxyListener&  iListener;
		TInt 				iAntype;
	};

class TDnsProxyNsRecord 
/*
* This class is used for constructing authoritative answer record set
*
* @internalTechnology
*/
	{
	public:
		TDesC8& CreateNSResourceRecord(const TDesC8& aMsg);
		void ResetBuffer();
	
	private:
		void AppendRRLength(const TDesC8& aMsg);
		void AppendTTLBuf();
	
	public:
		TUint16 	iQType;
		TUint16		iCType;
		TUint32 	iTTL; 
		TUint32 	iSerialNum;
		TUint32 	iRefreshTime;
		TUint32 	iRetryTime;
		TUint32 	iExpiretime;
		TBuf8<KDnsMaxMessage>  iNSBuf;
	};
	
#endif/*_DNSPROXY_MESSAGE_PROCESSOR_H__*/