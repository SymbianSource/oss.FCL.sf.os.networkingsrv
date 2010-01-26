//dnsproxydb.h
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
* dnsproxydb.h - Dns Proxy db
* @file
* @internalTechnology
*
*/



#ifndef 	_DNSPROXY_DB_H__
#define 	_DNSPROXY_DB_H__

#include "in_sock.h"
#include "dns_hdr.h"

/**
 * This is singleton class and every time returns same instance. 
 * This class maintains the data's in rpointer array
 * @internalTechnology
 **/
class TDnsProxydbElement; 
class CDnsProxyEngine;

class CDnsProxyDb: public CBase
	{
	public:
	    //creats object and returns reference 
	    static CDnsProxyDb* CreateInstanceL(CDnsProxyEngine& aEngine);
	    //Destructor
	    ~CDnsProxyDb();
		//updates database
		void UpdateDbL(const TDesC8& aName,TInetAddr& aAddr);
		//adds record into database
		void AddRRecord(TDnsProxydbElement& aElementList,const TDesC8& aRRecord,TUint16 aDnsQType);
		//returns array of TDnsProxydbElement
		TDnsProxydbElement* GetRecordList(const TDesC8& aName);
		//cleans the rray
		void Cleanup();
		//returns size of db
		TInt GetDbSize();
		TDnsProxydbElement* FindIpAddress(TInetAddr& aAddr);
		void DeleteDbEntry(TInetAddr& aAddr);
	
	private:
		//compares qname with host name
		TBool CompareNames(const TDesC8& aHostName,const TDesC8& aQueryName);	
			//constructor
		CDnsProxyDb(CDnsProxyEngine& aEngine);
			
	private:
		static CDnsProxyDb* iDbObject;
		RPointerArray<TDnsProxydbElement> iElementList;
		RPointerArray<TDnsProxydbElement> iRecordElement;
		CDnsProxyEngine&				  iEngine;
	};
/*
This class maintains db list
*/	
class TDnsProxydbElement
	{
	public:	
		TBuf8<KDnsMaxName>		iHostName;
		TBuf8<KDnsMaxMessage>	iRecord;
		TInetAddr 				iHostAddr;
		TUint16 				iDnsQType;
	};
#endif/*_DNSPROXY_DB_H__*/