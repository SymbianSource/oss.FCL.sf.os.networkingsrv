// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __NIFMANBCTESTSUITE_H_
#define __NIFMANBCTESTSUITE_H_

#include <networking/testsuite.h>

class CNifmanBCTestStep;
class CNifmanBCTestSuite : public CTestSuite
	{
public:
	void InitialiseL();
	virtual ~CNifmanBCTestSuite();
	void AddTestStepL(CNifmanBCTestStep* aTestStep );
	TPtrC GetVersion();
private:
	TInt CommsInit();
	};

#endif

