/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file TE_PPPComp.h
*/
// __EDIT_ME__ Create your own class definition based on this
#if (!defined __TE_PPPCOMP_H__)
#define __TE_PPPCOMP_H__
#include <test/testexecuteserverbase.h>

class CTE_PPPComp : public CTestServer
	{
public:
	static CTE_PPPComp* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	RFs& Fs(){return iFs;};

private:
	RFs iFs;
	};
#endif
