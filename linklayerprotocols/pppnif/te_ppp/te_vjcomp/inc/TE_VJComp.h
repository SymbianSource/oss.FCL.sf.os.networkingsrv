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
 @file TE_VJComp.h
*/
#if (!defined __TE_VJCOMP_H__)
#define __TE_VJCOMP_H__
#include <test/testexecuteserverbase.h>
#include "TE_VJCompStepBase.h"

// __TEST_LIT macro define a literal string from a name
// Used by __TEST_CLASS macro
#define __TEST_LIT(name) _LIT(K##name, #name)

// __TEST_CLASS macro define a literal from a classname and can be as the keyword class
// Example: __TEST_CLASS(CTpBearerReplyOptionEncodeStep) defines:
//			_LIT(KCTpBearerReplyOptionEncodeStep, "CTpBearerReplyOptionEncodeStep");
//          class CTpBearerReplyOptionEncodeStep
#define __TEST_CLASS(className) __TEST_LIT(className); class className

// __DEFINE_TEST_CLASS macro define a basic declaration for testClass as a 
// subclass of CTE_VJCompStepBase and define a literal from testClass 
#define __DEFINE_TEST_CLASS(testClass) __TEST_CLASS(testClass) : public CTE_VJCompStepBase\
			{ \
		public: \
			inline testClass() {SetTestStepName(K##testClass);}; \
			virtual void ProcessPacketL(); \
			};
			

class CTE_VJComp : public CTestServer
	{
public:
	static CTE_VJComp* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	RFs& Fs(){return iFs;};

private:
	RFs iFs;
	};
#endif
