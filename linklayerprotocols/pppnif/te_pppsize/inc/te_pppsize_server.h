/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file te_pppsize_server.h
*/

#ifndef _TE_PPPSIZE_SERVER_H
#define _TE_PPPSIZE_SERVER_H

#include <test/testexecuteserverbase.h>

_LIT(KPPPMinMaxMMU, "PPPMinMaxMMU");

class CPPPSizeServer : public CTestServer
{
public:
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	static CPPPSizeServer* NewL();
	virtual ~CPPPSizeServer();
private:
	CPPPSizeServer();

};

#endif
