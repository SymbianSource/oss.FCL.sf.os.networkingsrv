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
 @internalTechnology
*/

#include "dnsproxyupdateimpl.h"
#include <e32debug.h>


/**
*Factory method for CDNSProxyUpdateImpl
*/
CDNSProxyUpdateImpl* CDNSProxyUpdateImpl::NewL()
    {
    CDNSProxyUpdateImpl* self = new(ELeave) CDNSProxyUpdateImpl;
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

/**
*C'tor of the class CDNSProxyUpdateImpl
*/
CDNSProxyUpdateImpl::CDNSProxyUpdateImpl()
    {
    }

/**
*2nd Phase constructor
*/
void  CDNSProxyUpdateImpl::ConstructL()
    {
    User::LeaveIfError(iDnsClient.Connect());    
    }

/**
*D'tor for the class CDNSProxyUpdateImpl
*/
CDNSProxyUpdateImpl::~CDNSProxyUpdateImpl()
    {
    iDnsClient.Close();
    }

/**
* Interface function for adding entry to the DNS DB, this need to be implemented by the deriving class.
*/
void CDNSProxyUpdateImpl::AddDbEntry(const TDes8& aHostName, const TDes& aIpAddress, TRequestStatus& aStatus)
    {
    iDnsClient.AddDbEntry(aHostName, aIpAddress, aStatus);
    }

/**
*Interface function for removing entry from the DNS DB, this need to be implemented by the deriving class.
*/
void CDNSProxyUpdateImpl::RemoveDbEntry(const TDes& aIpAddress, TRequestStatus& aStatus)
    {
    iDnsClient.RemoveDbEntry(aIpAddress, aStatus);
    }

