// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definitions header file for CDHCPServer
// 
//

/**
 @file 
*/

#ifndef __DHCPSERVER_H__
#define __DHCPSERVER_H__

#include <e32base.h>
#include <e32svr.h>
#include <es_sock.h>
#include "ExpireTimer.h"
#include <cflog.h>

class CDHCPSession;
#ifdef SYMBIAN_NETWORKING_PLATSEC
class CDHCPServer : public CPolicyServer, public MExpireTimer
#else
class CDHCPServer : public CServer2, public MExpireTimer
#endif
/**
 * The CDHCPServer class
 *
 * Implements a Symbian OS server side DHCP configuration daemon
 */
	{
public:
	static CDHCPServer* NewL();
	~CDHCPServer();
	
	RSocketServ& ESock();
	void SessionConnected() const;
	void Close(CDHCPSession* aDHCPSession);
	void TimerExpired();
	static TInt& DebugFlags();
protected:
	CDHCPServer();
	void ConstructL();
	
private:
	CSession2* NewSessionL(const TVersion& aVersion, const RMessage2& aMessage) const;
	RSocketServ iEsock;
	CExpireTimer* iTimer;
	static TInt iDebugFlags;
	__CFLOG_DECLARATION_MEMBER;

#ifdef SYMBIAN_NETWORKING_PLATSEC
	static const TUint PolicyRangeCount;
	static const TInt PolicyRanges[];
	static const CPolicyServer::TPolicyElement PolicyElements[];
	static const TUint8 PolicyElementsIndex[];
	static const CPolicyServer::TPolicy Policy;
#endif	//	SYMBIAN_NETWORKING_PLATSEC
	};

inline RSocketServ& CDHCPServer::ESock()
/**
  * Returns handle to ESock connection
  *
  *	@internalTechnology
  *
  */
	{
	return iEsock;
	}

#endif

