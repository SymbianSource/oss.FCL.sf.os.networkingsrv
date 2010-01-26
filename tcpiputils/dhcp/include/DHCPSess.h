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
// @file DHCPSess.h
// Implements a Session of a Symbian OS server for the RConfigDaemon API
// 
//

/**
 @file DHCPSess.h
*/

#ifndef __DHCPSESS_H__
#define __DHCPSESS_H__

#include <e32base.h>
#include <e32svr.h>
#include <es_enum.h>

class CDHCPControl;
class CDHCPServer;
class CDHCPSession : public CSession2
/**
 * The DHCPSession class
 *
 * Implements a Session of a Symbian OS server for the RConfigDaemon API
 */
	{
public:
	static CDHCPSession* NewL();
	virtual ~CDHCPSession();

	// CSession
	virtual void ServiceL(const RMessage2& aMessage);
	void CompleteMessage(TInt aErr);
   	CDHCPServer* DHCPServer() const;

protected:
	void ConstructL();
	CDHCPSession();

private:
	void DoServiceL(const RMessage2& aMessage);
	void IoctlL(const RMessage2& aMessage);
	void ControlL(const RMessage2& aMessage);
	void ConfigureL(const RMessage2& aMessage);

    void CreateControlL(const TConnectionInfoBuf& aConfigInfo);
	void HandleHeapDebug(const RMessage2& aMessage);
private:
	RPointerArray<CDHCPControl> iDHCPIfs;	// owns

	RMessage2 iMessage;
   TInt iConfigType;
	};


inline CDHCPSession* CDHCPSession::NewL()
/**
 * The CDHCPSession::NewL method
 *
 * Construct a Symbian OS session object
 *
 * @internalComponent
 *
 * @return	A new CDHCPSession object
 */
	{
	return new (ELeave) CDHCPSession;
	}

inline CDHCPServer* CDHCPSession::DHCPServer() const
/**
 * The CDHCPSession::DHCPServer method
 *
 * type wrapper
 *
 * @internalComponent
 *
 * @return	A CDHCPServer object
 */
	{
   	return (CDHCPServer*)(Server());
	}

#endif

