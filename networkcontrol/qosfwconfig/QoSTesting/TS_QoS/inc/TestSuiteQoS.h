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

#if (!defined __TEST_SUITE_QOS_H_)
#define __TEST_SUITE_QOS_H_

#include "TestSuite.h"
#include <Es_sock.h>

class CMyScheduler;

class  CTestSuiteQoS : public CTestSuite
{
public:	
	void InitialiseL(void);
	virtual ~CTestSuiteQoS();
	void AddTestStepL(CTestStepQoS * ptrTestStep);
	TPtrC GetVersion(void);

private:
	CMyScheduler *iScheduler;
};


//
// Class for implementing the active scheduler
//
class CMyScheduler : public CActiveScheduler
{
public:
	void Error (TInt aError) const;
private:
};


#endif /* __TEST_SUITE_QOS_H_ */
