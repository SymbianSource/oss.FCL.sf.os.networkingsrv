// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// dnsproxyserver.h
// Header file for the DNS Proxy server side implementation.
//



/**
 @file
 @internalComponent
*/

#ifndef __DNSPROXYSERVSESS_H__
#define __DNSPROXYSERVSESS_H__

#include <e32base.h>
#include "dnsproxyengine.h"

//forward declaration
class CDnsProxyEngine;

/**
This class is the session handler class derived from Symbian CSession2. The class provides
command handlers for different client commands.  
**/
class CDnsProxyServerSession : public CSession2
    {

public:
    ~CDnsProxyServerSession();
    static CDnsProxyServerSession* NewL();
    virtual void ServiceL(const RMessage2& aMessage);
    // Stops DNS Proxy engine and does a complete cleanup of resources.
    void StopDNSEngine();
    
protected:
	 CDnsProxyServerSession();    
   
private:
	void DoServiceL(const RMessage2& aMessage); 
    // Starts DNS Proxy engine at the DNS Proxy startup
    TInt StartDnsEngine(const RMessage2& aMessage);
    // This method updates DNS Proxy database with hostname and IP address information.
    TInt UpdateDatabaseL(const RMessage2& aMessage);
    //Gets the domain name from the DHCP server
    TInt UpdateDomainName(const RMessage2& aMessage);
    // Sets Uplink information
	TInt UpdateUplinkConfig(const RMessage2& aMessage);
	// Remove entry from Db
	TInt RemoveDbEntry(const RMessage2& aMessage);
	void ConstructL();
	
private:
	// An instance of CDnsProxy Engine which is a DNS Proxy control class.
    CDnsProxyEngine* iProxyEngine;
    // client side message.
    RMessage2 iMessage; 
    //This flag is to check wether engine is started or not.
    TBool iIsEngineStarted;
    };
    
#endif //__DNSPROXYSERVSESS_H__