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
//



/**
 @file
 @publishedPartner
 @released
*/

#ifndef __CDNSPROXYUPDATEIF_H__
#define __CDNSPROXYUPDATEIF_H__

#include <e32base.h>
#include <ecom/ecom.h>


/**
* Ecomp plugin implementation class
*/
class CDNSProxyUpdateIf : public CBase
{
public:

	/**
	* Factory method for CDNSProxyUpdateIf
	*/
   static CDNSProxyUpdateIf* NewL(TUid aUid );

	/**
	* virtual D'tor for the class.
	*/
    virtual ~CDNSProxyUpdateIf();

	/**
	* Interface function for adding entry to the DNS DB, this need to be implemented by the deriving class.
	*/
    virtual void AddDbEntry(const TDes8& aHostName, const TDes& aIpAddress, TRequestStatus& aStatus) = 0;

	/**
	*Interface function for removing entry from the DNS DB, this need to be implemented by the deriving class.
	*/
    virtual void RemoveDbEntry(const TDes& aIpAddress, TRequestStatus& aStatus) = 0;


private:

    //private member varible for identifying the implementations
    TUid                        iDtorIdKey;
};

/**
* Factory method for CDNSProxyUpdateIf
*/
inline CDNSProxyUpdateIf* CDNSProxyUpdateIf::NewL(TUid aUid )
    {
    CDNSProxyUpdateIf* self = reinterpret_cast<CDNSProxyUpdateIf*>(
    REComSession::CreateImplementationL(
        aUid,
        _FOFF(CDNSProxyUpdateIf, iDtorIdKey)));
    return self;
    }

/**
* D'Tor for ~CDNSProxyUpdateIf
*/
inline CDNSProxyUpdateIf::~CDNSProxyUpdateIf()
    {
    REComSession::DestroyedImplementation(iDtorIdKey);
    }


#endif //__CDNSPROXYUPDATEIF_H__






