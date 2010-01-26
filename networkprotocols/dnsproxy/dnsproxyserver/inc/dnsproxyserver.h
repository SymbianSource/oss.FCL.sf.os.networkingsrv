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
// Header file for the DNS Proxy server side implementation.
//



/**
 @file
 @internalComponent
*/

#ifndef __DNSPROXYSERVER_H__
#define __DNSPROXYSERVER_H__

#include <e32base.h>
#include "dnsproxyengine.h"

//forward declaration of the classes
class CDnsProxyServerSession;

/**
This is the DNS Proxy Server class and is derived from symbian CServer2 class.
**/
#ifdef SYMBIAN_NETWORKING_PLATSEC
class CDnsProxyServer : public CPolicyServer
#else
class CDnsProxyServer : public CServer2
#endif
	{
public:
    static CDnsProxyServer* NewL();
    ~CDnsProxyServer();
    void DecreaseSessionCount(CDnsProxyServerSession& aSession) const;
    
protected:
	CDnsProxyServer();    

private:
    virtual CSession2* NewSessionL(const TVersion& aVersion,const RMessage2& aMessage) const;
	
    //To check the capabilities.
#ifdef SYMBIAN_NETWORKING_PLATSEC
	static const TUint PolicyRangeCount;
	static const TInt PolicyRanges[];
	static const CPolicyServer::TPolicyElement PolicyElements[];
	static const TUint8 PolicyElementsIndex[];
	static const CPolicyServer::TPolicy Policy;
#endif	//	SYMBIAN_NETWORKING_PLATSEC

private :   
    TInt iCount;
	};

#endif //__DNSPROXYSERVER_H__