// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __SESSION_H
#define __SESSION_H


#include <d32dbms.h>
#include "server.h"
#include "echodaemon.h"

class CEchoDaemonSession : public CSession2
	{
public:
	static CEchoDaemonSession* NewL(CEchoDaemonServer& aServer);
	~CEchoDaemonSession();
	virtual void ServiceL (const RMessage2& aMessage);
	CEchoDaemonSession(CEchoDaemonServer& aServer);
	void ConstructL();
	
private:
	void PanicClient(const RMessage2& aMessage, TInt aPanic);
	TInt DispatchMessageL(const RMessage2& aMessage);

	TInt Start(const RMessage2& aMessage);
	TInt Stop(const RMessage2& aMessage);
	TInt StopAll(const RMessage2& aMessage);
	
private:	// Data members
	CEchoDaemonServer&			iServer;
	};


#endif	// __SESSION_H
