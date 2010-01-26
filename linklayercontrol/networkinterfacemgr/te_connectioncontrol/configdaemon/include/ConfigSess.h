/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Implements a session for the Symbian OS server for the test 
* configuration daemon.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __CONFIGSESS_H__
#define __CONFIGSESS_H__

#include <e32base.h>
#include <e32svr.h>
#include <es_enum.h>

class CConfigControl;
class CConfigServer;
class CConfigSession : public CSession2
/**
 * The CConfigSession class
 *
 * Implements a Session of a Symbian OS server for the RConfigDaemon API
 *
 * @internalComponent
 */
	{
public:
	// construction/destruction
	static CConfigSession* NewL();
	virtual ~CConfigSession();

	// CSession
	virtual void ServiceL(const RMessage2& aMessage);
	void CompleteMessage(TInt aErr);
   	CConfigServer* ConfigServer() const;

protected:
	// constuction.
	void ConstructL();
	
	// implementation of the basic api
	void ConfigureL(const RMessage2& aMessage);
	void LinkLayerDownL(const RMessage2& aMessage);
	void LinkLayerUpL(const RMessage2& aMessage);
	void DeregisterL(const RMessage2& aMessage);
	void ProgressL(const RMessage2& aMessage);
	void IoctlL(const RMessage2& aMessage);
	void Cancel(const RMessage2& aMessage);
	void CancelMask(const RMessage2& aMessage);
	
	// utlities
   	CConfigControl* CreateControlL();

private:
	/** Object that actually services the requests. */
	CConfigControl* iConfigIf;
	};


inline CConfigSession* CConfigSession::NewL()
/**
 * The CConfigSession::NewL method
 *
 * Construct a Symbian OS session object
 *
 * @internalComponent
 *
 * @return A new CConfigSession object.
 */
	{
	return new (ELeave) CConfigSession;
	}

inline CConfigServer* CConfigSession::ConfigServer() const
/**
 * The CConfigSession::ConfigServer method
 *
 * type wrapper
 *
 * @internalComponent
 *
 * @return A new CConfigServer object.
 */
	{
   	return (CConfigServer*)(Server());
	}

#endif


