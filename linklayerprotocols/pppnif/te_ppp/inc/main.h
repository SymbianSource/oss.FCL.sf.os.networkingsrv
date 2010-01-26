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
 @file main.h
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include <testexecuteserverbase.h>

_LIT(KPPPANVL, "PPPANVL");

class CPPPAnvlServer : public CTestServer
{
public:
	static CPPPAnvlServer* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	virtual ~CPPPAnvlServer();
private:
	CPPPAnvlServer();
};

#endif
