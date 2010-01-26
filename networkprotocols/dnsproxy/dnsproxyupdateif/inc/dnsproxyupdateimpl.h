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
*/
#ifndef __CDNSPROXYUPDATEIMPL_H__
#define __CDNSPROXYUPDATEIMPL_H__

//user include files
#include "dnsproxyupdateif.h"
#include "dnsproxyclient.h"

/**
*Implementation class for ecom interface
*
*/
class CDNSProxyUpdateImpl : public CDNSProxyUpdateIf
{
public:

    /**
        *Factory method for CDNSProxyUpdateImpl
        */
    static CDNSProxyUpdateImpl* NewL();

    /**
        *C'tor of the class CDNSProxyUpdateImpl
        */
    CDNSProxyUpdateImpl();

    /**
        *2nd Phase constructor
        */
    void  ConstructL();

    /**
        *D'tor for the class CDNSProxyUpdateImpl
        */
    ~CDNSProxyUpdateImpl();

    /**
        * Interface function for adding entry to the DNS DB, this need to be implemented by the deriving class.
        */
    void AddDbEntry(const TDes8& aHostName, const TDes& aIpAddress, TRequestStatus& aStatus);

    /**
        *Interface function for removing entry from the DNS DB, this need to be implemented by the deriving class.
        */
    void RemoveDbEntry(const TDes& aIpAddress, TRequestStatus& aStatus);

private:
    //Declaration of private member variable
    RDNSClient iDnsClient;
};

#endif /*__CDNSPROXYUPDATEIMPL_H__*/
