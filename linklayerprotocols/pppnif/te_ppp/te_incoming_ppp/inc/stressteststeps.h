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
// Interface for Incoming PPP stress testing steps 
// 
//

/**
 @file 
 @internalComponent
*/
#ifndef __STRESSTESTSTEPS_H__
#define __STRESSTESTSTEPS_H__

#include "loopbackteststepbase.h"

#include <e32base.h>
#include <es_sock.h> 
#include <commdbconnpref.h>

namespace te_ppploopback
{

/** Defines the stress test step for incoming ppp.

@internalComponent
@test
*/
class CPppStressTestStep: public CLoopbackTestStepBase
	{
public:
	CPppStressTestStep();	
	TVerdict doTestStepL();
		
private:
	void LogAndLeaveIfErrorL(const TInt aErrorCode, const TDesC& aMessage);
	
	/** ESock, for access to RConnection */
	RSocketServ iEsock;
	
	/** PPP server instance */
	RConnection iServerConn;
	
	/** Connection prefereces to setup server PPP connection */
	TCommDbConnPref iServerConnPrefs;
	
	/** PPP client instance */
	RConnection iClientConn;
	
	/** Connection prefereces to setup client PPP connection */
	TCommDbConnPref iClientConnPrefs;	
	};
_LIT(KPppStressTestStepName, "CPppStressTestStep");

}
#endif // __STRESSTESTSTEPS_H__
