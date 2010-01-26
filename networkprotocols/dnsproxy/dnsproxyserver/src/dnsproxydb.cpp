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
// This class represents the database in the form of array
//



/**
 @file
 @internalTechnology
*/
#include <e32debug.h>
#include <cflog.h>
#include "dnsproxydb.h"
#include "dnsproxyengine.h"
#include "inet6log.h"
#include "dnsproxylog.h"

//
CDnsProxyDb* CDnsProxyDb::iDbObject= NULL;

CDnsProxyDb* CDnsProxyDb::CreateInstanceL(CDnsProxyEngine& aEngine)
/**
 * Creates the single object everytime
 * @return iDbObject- reference to CDnsProxyDb;
 *
 * @internalTechnology
 **/
	{
	if(!iDbObject)
		{
		iDbObject = new(ELeave)CDnsProxyDb(aEngine);
		}
	return iDbObject;
	}

CDnsProxyDb::CDnsProxyDb(CDnsProxyEngine& aEngine):iEngine(aEngine)
/**
 * Constructor
 *
 * @internalTechnology
 **/

	{
	}

CDnsProxyDb::~CDnsProxyDb()
/**
 * Destructor
 *
 * @internalTechnology
 **/
	{
	Cleanup();
	}

void CDnsProxyDb::UpdateDbL(const TDesC8& aHostName,TInetAddr& aHostAddr)
/**
 * Updates the DB with HostName and Host Address
 * @param aHostName - Host Name
 * @param aHostAddr - Host Address
 *
 * @internalTechnology
 **/
	{
	
	TDnsProxydbElement* aDbElement = new TDnsProxydbElement;
	aDbElement->iHostName.Copy(aHostName);
	aDbElement->iHostAddr = aHostAddr ;
	iElementList.AppendL(aDbElement);
	}

TDnsProxydbElement* CDnsProxyDb::GetRecordList(const TDesC8& aQName)
/**
 * Returns the array of TDnsProxydbElement object having same name as that of QueryName
 * @param  aName- Query Name to be searched in db
 * @return element - returns the reference to TDnsProxydbElement
 *
 * @internalTechnology
 **/
	{
	TBool flag = EFalse;
	TDnsProxydbElement* element = NULL;
	for(TInt i =0;i<iElementList.Count();i++)
		{
		 if(iElementList[i])
			{
			flag = CompareNames(iElementList[i]->iHostName,aQName);
			if(flag)
				{
				element = iElementList[i];
				break;
				}
			}
		}
	return element;	
	}

void CDnsProxyDb::AddRRecord(TDnsProxydbElement& aElementList,const TDesC8& aRRecord,TUint16 aDnsQType)
/**
 * Adds record into db
 * @param aElementList - associates with record to be inserted into db
 * @param aRRecord - record to be added into db which may be of type AN or NS
 * @param aFlag - which will indicate type of record like NS or AN
 * @internalTechnology
 **/
	{
	aElementList.iRecord.Copy(aRRecord);
	aElementList.iDnsQType = aDnsQType;
	}

void CDnsProxyDb::Cleanup()
/**
 * Destroys all the obejcts  present in the array list and empties it
 * @internalTechnology
 **/
	{
	iElementList.ResetAndDestroy();
	}
TDnsProxydbElement* CDnsProxyDb::FindIpAddress(TInetAddr& aAddr)
/**
 * Finds specified entry from db based on IP address
 * @param aAddr - TInetAddress
 * @return dbelement - pointer to TDnsProxydbElement class
 *
 * @internalTechnology
 **/
	{
					
	TDnsProxydbElement* dbelement = NULL;
	for(TInt i = 0;i < iElementList.Count();i++)
		{

		if(iElementList[i])
			{
			if(aAddr.Match(iElementList[i]->iHostAddr))
				{
				dbelement = iElementList[i];
				break;
				}
			}
		}
	
	return dbelement;	
	}

void CDnsProxyDb::DeleteDbEntry(TInetAddr& aAddr)
/**
 * Deletes specified entry from db based on IP address
 * @return TBool - returns ETrue if it is successful otherwise EFalse
 *
 * @internalTechnology
 **/
	{
	for(TInt i =0;i<iElementList.Count();i++)
		{
		if(iElementList[i])
			{

			TInetAddr ipaddr = iElementList[i]->iHostAddr;
			if(ipaddr.Match(aAddr))
				{
				delete iElementList[i];
				iElementList[i] = NULL;
				iElementList.Remove(i);
    			iElementList.Compress();
				}
			}
		}
	}

TBool CDnsProxyDb::CompareNames(const TDesC8& aHostName,const TDesC8& aQueryName)
/**
 * Compare Query Name with Host Name present in db
 * @param aHostName - Host name retrived from db
 * @param aQueryName - Query name to be compared with Host Name
 * @return TBool - ETrue if it sucessful otherwise EFalse
 *
 * @internalTechnology
 **/
	{
	TInt ret_val = aHostName.Compare(aQueryName);
	if(ret_val==0)
		return ETrue;
	return EFalse;
	}

TInt CDnsProxyDb::GetDbSize()
/**
 * Retunrs the current size of the db
 * @return size - current size of db
 *
 * @internalTechnology
 **/
	{
	return iElementList.Count();
	}
