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
// dnsproxymessageproc.cpp
// This class constructs the response packet for matched query packet that
// it gets from the queue. First it will get the name from the database
// checks against the query name. If it matched then generates the answer for
// query packet and appends into buffer then sends to querying host. Otherwise
// generates the authoritive answer and sends back to querying hosts.
//



/**
 @file
 @internalTechnology
*/


#include <e32base.h>
#include <e32math.h>
#include <e32des8.h> 
#include <e32debug.h> 
#include <utf.h>
#include "dns_qry_internal.h"
#include "inet6log.h"

#include "dnsproxymsgproc.h"
#include "dnsproxylistener.h"
#include "dnsproxyengine.h"
#include "message.h"
#include "dnsproxylog.h"


CDnsProxyMessageProcessor* CDnsProxyMessageProcessor::NewL(CDnsProxyListener& aListener)
/**
 * This is the two phase construction method and is leaving function
 * @param aListener - reference CDnsProxyListener class
 *
 * @internalTechnology
 **/

	{
	CDnsProxyMessageProcessor* response = new(ELeave) CDnsProxyMessageProcessor(aListener);
	return response;
	}
	
CDnsProxyMessageProcessor::CDnsProxyMessageProcessor(CDnsProxyListener& aListener):iListener(aListener),iAntype(0)
/**
 * This is the constructor for this class
 * @param aListener - reference CDnsProxyListener class
 * @return - None
 *
 * @internalTechnology
 **/

	{
	}

CDnsProxyMessageProcessor::~CDnsProxyMessageProcessor()
/**
 * This is the destructor  and deletes all the member variables and thus
 * releases all the memory assosicated with those objects
 * @param - None
 * @return - None
 *
 * @internalTechnology
 **/
	{
	}
	
void CDnsProxyMessageProcessor::GetDnsFailureMessage(TMsgBuf& aMsgBuf)
/**
 * This is create DNS Failure message
 
 * @param - TMsgBuf&
 * @return - None
 *
 * @internalTechnology
 **/
	{
	TDndHeader &hdr = aMsgBuf.Header();
	
	hdr.SetQR(1);
	//set dns server failure
	hdr.SetRCode(2);
	}
	
void CDnsProxyMessageProcessor::GetLocalDnsResponseL(TMsgBuf& aMsgbuf)
/**
 * This function constructs the response packet. First it gets the 
 * only query name by filtering the qtype and qclass then it tries to
 * match the names. If it matches then, generates the ANResource reord 
 * otherwise generates the NSResource Record.
 * @param -aMsg - Query packet
 * @internalTechnology
 **/

	{
	TDndQuestion qDns;
	qDns.Create(aMsgbuf, sizeof(TDndHeader));

	TBuf<KDnsMaxName> queryName;
	TInt ret_val = qDns.GetName(queryName);
	if(ret_val==KErrNone)
		{
		HBufC8* q_name = CnvUtfConverter::ConvertFromUnicodeToUtf8L(queryName);
		CleanupStack::PushL(q_name);
		
		TPtrC8 qname = q_name->Right(q_name->Length());
		EDnsQType iQType = qDns.QType();
				
		switch(qDns.QType())
			{
				case EDnsQType_A:
					{
					TDnsProxydbElement* element;
					TDndHeader& hdr = aMsgbuf.Header();
					CDnsProxyEngine& engine = iListener.GetDnsProxyEngine();
					
					element = engine.iDnsproxydb->GetRecordList(qname);
					if(element)
						{
						hdr.SetNsCount(0);
						GetRecordsFromDb(aMsgbuf,*element);
						}
					else	
						{
						SetQHeaderBits(hdr);
						hdr.SetNsCount(1);
						GetNsRecordSetL(aMsgbuf);
						}	
					}
					break;
				case EDnsQType_PTR:
					{
					TDnsProxydbElement* element = NULL;
					element = LookupPTRInDb(aMsgbuf);
	                if(element)
	                    {           
	                    GetPTRRecord(aMsgbuf,element->iHostName);
	                    }
					}
					break;
			}
			CleanupStack::PopAndDestroy(q_name);
		}
	}
	
void CDnsProxyMessageProcessor::SetQHeaderBits(TDndHeader& aHdr)
/**
 * This will set bits fileds of query header as required for 
 * response message
 * @param aHdr - reference to header section of query
 * @internalTechnology
 **/	

	{
	TInt aId = aHdr.ID();
	aHdr.SetId(aId);
	aHdr.SetQR(1);
	aHdr.SetOpcode(0);
	aHdr.SetAA(0);
	aHdr.SetRCode(0);

	TInt qdcount = aHdr.QDCOUNT();
	aHdr.SetQdCount(qdcount);
	aHdr.SetArCount(0);
	}
	
void CDnsProxyMessageProcessor::GetRecordsFromDb(TMsgBuf& aMsg,TDnsProxydbElement& aElement)
/**
 * This will give constructed response message for given query
 * @param aMsg - query 
 * @param - aElement - reference to TDnsProxydbElement class
 * @internalTechnology
 *
 **/	
	{
	iAntype =0;
	TDndHeader &hdr = aMsg.Header();
	SetQHeaderBits(hdr);
	TInt hdr_id = hdr.ID();

	if(aElement.iRecord.Length()==0)
		{
		GetAnRecordSet(aMsg,aElement);
		}
	else
		{
		TBuf8<KDnsMaxMessage>record;
		record = aElement.iRecord;
		aMsg = TMsgBuf::Cast(record);
		TDndHeader& header = aMsg.Header();
		TInt hdid = header.ID();
		LOG(Log::Printf(_L("\t \n --- Transaction Id:[%d]"),hdid));
		header.SetId(hdr_id);
		TUint16 qtype = aElement.iDnsQType;
		if(qtype==EDnsType_A)
			iAntype++;
		}
	hdr.SetAnCount(iAntype);
	
	hdr.SetNsCount(0);
	}
	
void CDnsProxyMessageProcessor::GetAnRecordSet(TMsgBuf& aQMsg,TDnsProxydbElement& aElement)
/**
 * Constructs answer resource record/s and appends it to buffer
 * @param -  aQMsg, dns query message
 * @param -  aElement, reference to TDnsProxydbElement class
 * @leave - system wide errors, buffer overflow
 * @internalTechnology
 **/	
	{
	CDnsProxyEngine& engine = iListener.GetDnsProxyEngine();
			
	aQMsg.Header();
	
	TInetAddr address = aElement.iHostAddr;

	THostName name;
	aQMsg.GetName(sizeof(aQMsg.Header()),name);	
	
	
	TDndQuestion question;
	TInt index = question.Create(aQMsg,sizeof(aQMsg.Header()));

	TInt len = aQMsg.Length();

	TDndRROut rrout(aQMsg);
	
	TDnsProxyConfigParams iEngineConfig = iListener.GetDnsProxyEngine().GetConfig();
	rrout.iType = (TUint16)question.QType();
	rrout.iClass = (TUint16)question.QClass();
	rrout.iTTL = iEngineConfig.iTTL;
	TInt ret = rrout.Append(KNullDesC8, sizeof(TDndHeader));
	iAntype++;
	rrout.AppendRData(address);
	
	engine.iDnsproxydb->AddRRecord(aElement,aQMsg,question.QType());			

	}
	
void CDnsProxyMessageProcessor::GetNsRecordSetL(TMsgBuf& aMsg)
/**
 * constructs and appends the authoritive resource records
 * @param -  aQMsg, dns query message
 * @leave - system wide errors, buffer overflow
 *
 * @internalTechnology
 **/
	{
	TDnsProxyNsRecord nsrecord;
	TBuf8<KDnsMaxMessage> record;

	//read from TTL from config
	CDnsProxyEngine& engine = iListener.GetDnsProxyEngine();
	TUint ttl =  engine.GetConfig().iTTL;
	//Create RBuf
	RBuf8 qname;
	
	TInt offset = sizeof(TDndHeader);
	const TUint8* ptr = (aMsg.Ptr() + offset);
	TPtrC8 ptrc8(aMsg.Ptr() + offset, aMsg.Size() - offset);
	
	qname.CreateL(ptrc8.Size());
	CleanupClosePushL(qname);
	
	qname.Copy(TPtrC8(&ptr[0],ptrc8.Size()));
	
	nsrecord.ResetBuffer();
	nsrecord.iQType = EDnsType_SOA;
	nsrecord.iCType = EDnsClass_IN;
	nsrecord.iTTL = ttl;
	nsrecord.iSerialNum 	= engine.iConfigParams.iSerialNum;
	nsrecord.iRefreshTime	= engine.iConfigParams.iRefreshTime;
	nsrecord.iRetryTime		= engine.iConfigParams.iRetryTime;
	nsrecord.iExpiretime	= engine.iConfigParams.iExpiretime;
	
	record = nsrecord.CreateNSResourceRecord(qname);
	aMsg.Append(record);
	CleanupStack::PopAndDestroy(&qname);
	}
	
TInt CDnsProxyMessageProcessor::GetQNameSuffixL(TMsgBuf& aMsg)
/**
 * This function gets the suffix of recieved query
 * @param aMsg - reference to dns query message
 * @return ret_val - represents whether query is local or global query
 *
 * @internalTechnology
 **/
	{
    CDnsProxyEngine& engine = iListener.GetDnsProxyEngine();
   	
	TInt retVal = KGlobalQuery;
	
	/*In case suffix information is not available
	  treat it as global query no need of comparison*/
	if(!engine.IsSuffixAvailable())
	    return retVal;
	TDndQuestion qDns;
	qDns.Create(aMsg, sizeof(TDndHeader));
	EDnsQType iQType = qDns.QType();
    
	switch (qDns.QType()) 
		{
		case EDnsQType_A:
			{
			TBuf<KMaxName> buf;
			buf.Copy(engine.iSuffixInfo);
			HBufC8* nsuffix = CnvUtfConverter::ConvertFromUnicodeToUtf8L(buf);
			CleanupStack::PushL(nsuffix);	
			TPtrC8 qsuffix = nsuffix->Right(nsuffix->Length());
			TBuf<KDnsMaxName> queryName;
			TInt err = qDns.GetName(queryName);
			if(err == KErrNone)
				{
				HBufC8* new_name = CnvUtfConverter::ConvertFromUnicodeToUtf8L(queryName);
				TPtrC8 qname = new_name->Right(new_name->Length());
				if(qname.Length() > qsuffix.Length())
					{
					TPtrC8 suffix = qname.Mid((qname.Length())-(qsuffix.Length()),qsuffix.Length());
					TInt errVal = suffix.Compare(qsuffix);
					if(errVal==KErrNone)
						{
						retVal = KLocalQuery;
						__LOG("Processing Local Query-Type-A");
						}
					else
						__LOG("Processing Global Query-Type-A");	
					}
		        		
				delete new_name;
				}
			CleanupStack::PopAndDestroy(nsuffix);	
			}
			break;
		case EDnsQType_PTR:
			{
			TDnsProxydbElement* element = NULL;			
			element = LookupPTRInDb(aMsg);
			if(element)
				{
				retVal = KLocalQuery;
				}
			else 
				{
				retVal = KGlobalQuery;
				}
			}
			break;
		default:
			{
			retVal = KGlobalQuery;
			}
				
		} // end switch
	return retVal;
	}
TDnsProxydbElement* CDnsProxyMessageProcessor::LookupPTRInDb(TMsgBuf& aMsg)
/**
* Looksup in db for existance of the recod. If present then process query as local
* otherwise treats PTR query as a global one
* @param aMsg - Query Message
* @return TDnsProxydbElement - pointer to TDnsProxydbElement object
*
**/
	{
	CDnsProxyEngine& engine = iListener.GetDnsProxyEngine();
	
	TBuf<KPTRSufix>ptrbuf;
	ptrbuf.Copy(KPTRSUFFIX);
	
	TDndQuestion qDns;
	qDns.Create(aMsg, sizeof(TDndHeader));
	
	TBuf<KDnsMaxName> queryName;
	TInt flag = qDns.GetName(queryName);
	
	TDnsProxydbElement* element = NULL;
	if(queryName.Length()>ptrbuf.Length())
		{
		TPtrC16 ipaddr = queryName.Mid(0,(queryName.Length()-(ptrbuf.Length()+1)));
		TInetAddr address;
		address.Input(ipaddr);
		TUint32 ad = address.Address();
		TUint32 swapaddr =(((ad&0x000000FF)<<24)+((ad&0x0000FF00)<<8)+
	   					  ((ad&0x00FF0000)>>8)+((ad&0xFF000000)>>24));
		TInetAddr inetaddr;
		inetaddr.SetAddress(swapaddr);
	
		element = engine.iDnsproxydb->FindIpAddress(inetaddr);
		}
					
	return element;
	}
void CDnsProxyMessageProcessor::GetPTRRecord(TMsgBuf& aMsg,const TDesC8& aName)
/**
* Constructs PTR record and appends to TMsgBuf
* @param aMsg - Query Message
* @param aElement - reference to TDnsProxydbElement class which matched to query name
*
* @internalTechnology
**/
	{
	TDndHeader &hdr = aMsg.Header();
	
	SetQHeaderBits(hdr);
	hdr.SetAA(1);
	hdr.SetAnCount(1);
	
	TDndQuestion question;
	TInt index = question.Create(aMsg,sizeof(TDndHeader));

	TInt len = aMsg.Length();

	TDndRROut rrout(aMsg);
	
	rrout.iType = (TUint16)question.QType();
	rrout.iClass = (TUint16)question.QClass();
	
	rrout.iTTL = iListener.GetDnsProxyEngine().GetConfig().iTTL;
	TInt ret = rrout.Append(KNullDesC8, sizeof(TDndHeader));
	
	TInt err = rrout.AppendRData(aName,0);
	}
TDesC8& TDnsProxyNsRecord::CreateNSResourceRecord(const TDesC8& aMsg)
/**
 * Creates new authorative answer record for given query. This will be 
 * generated when ther is no matching name found in db
 * @param aMsg - Dns query 
 * @return iNSBuf - buffer having constructed authorative answer record
 *
 * @internalTechnology
 **/	
	{
	
	TInt offset_pos = aMsg.Locate(0);
	const TUint8* ptr8 = (aMsg.Ptr());
	TBufC8<KDnsMaxMessage> abuf_ns = TPtrC8(&ptr8[0],offset_pos);
	
	iNSBuf.Append(abuf_ns);
	//this will identify the termination of the QName
	iNSBuf.Append((TChar)0);

	iNSBuf.Append(iQType/0x100);
	iNSBuf.Append(iQType%0x100);
	
	iNSBuf.Append(iCType/0x100);
	iNSBuf.Append(iCType%0x100);
	AppendTTLBuf();
	AppendRRLength(aMsg);

	return iNSBuf;
	}
	
void TDnsProxyNsRecord::AppendTTLBuf()
/**
 * Appends the TTL to ns record set
 *
 * @internalTechnology
 **/
	{
	iNSBuf.Append((TChar)(iTTL / 0x1000000));
	iNSBuf.Append((TChar)((iTTL / 0x10000) % 0x100));
	iNSBuf.Append((TChar)((iTTL / 0x100) % 0x100));
	iNSBuf.Append((TChar)(iTTL % 0x100));
	}
	
void TDnsProxyNsRecord::AppendRRLength(const TDesC8& aMsg)
/**
 * This function creates the NS Resource Record as specified in
 * the RFC1035 section 3.3.13.-SOA RDATA Format
 * @param - aMsg ,reference to buf which will have ns record set
 *
 * @internalTechnology
 **/
	{
	TBuf8<KDnsMaxMessage> buf;
	buf.Append(aMsg);
	buf.Append((TChar)0);
	buf.Append(aMsg);
	buf.Append((TChar)0);
	
	buf.Append((TChar)(iSerialNum / 0x1000000));
	buf.Append((TChar)((iSerialNum / 0x10000) % 0x100));
	buf.Append((TChar)((iSerialNum / 0x100) % 0x100));
	buf.Append((TChar)(iSerialNum % 0x100));

	buf.Append((TChar)(iRefreshTime / 0x1000000));
	buf.Append((TChar)((iRefreshTime / 0x10000) % 0x100));
	buf.Append((TChar)((iRefreshTime / 0x100) % 0x100));
	buf.Append((TChar)(iRefreshTime % 0x100));

	buf.Append((TChar)(iRetryTime / 0x1000000));
	buf.Append((TChar)((iRetryTime / 0x10000) % 0x100));
	buf.Append((TChar)((iRetryTime / 0x100) % 0x100));
	buf.Append((TChar)(iRetryTime % 0x100));

	buf.Append((TChar)(iExpiretime / 0x1000000));
	buf.Append((TChar)((iExpiretime / 0x10000) % 0x100));
	buf.Append((TChar)((iExpiretime / 0x100) % 0x100));
	buf.Append((TChar)(iExpiretime % 0x100));

	buf.Append((TChar)(iTTL / 0x1000000));
	buf.Append((TChar)((iTTL / 0x10000) % 0x100));
	buf.Append((TChar)((iTTL / 0x100) % 0x100));
	buf.Append((TChar)(iTTL % 0x100));

	TInt len = buf.Size();
	iNSBuf.Append((TChar)(len/0x0100));
	iNSBuf.Append((TChar)(len%0x0100));
	
	iNSBuf.Append(buf);

	}

void TDnsProxyNsRecord::ResetBuffer()
/**
 * before appending anything newly, deletes its contents first then append
 *
 * @internalTechnology
 **/
	{
	TInt len = iNSBuf.Length();
	if(len>0)
		iNSBuf.Delete(0,len);
	}
