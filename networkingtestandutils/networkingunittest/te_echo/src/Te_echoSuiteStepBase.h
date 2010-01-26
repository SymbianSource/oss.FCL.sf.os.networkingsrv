/**
* Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file Te_echoSuiteStepBase.h
 @internalTechnology
*/

#if (!defined __TE_ECHO_STEP_BASE__)
#define __TE_ECHO_STEP_BASE__
#include <test/testexecutestepbase.h>
#include <networking/echodaemon.h>

class CTe_echoSuiteStepBase : public CTestStep
	{
public:
	virtual ~CTe_echoSuiteStepBase();
	CTe_echoSuiteStepBase();
	void ReadConfigFromIniL();
	virtual TVerdict doTestStepPreambleL(); 
	virtual TVerdict doTestStepPostambleL();

protected:
	TInt iIap;
	TInt iProtocol;
	TInt iPort;
	};

#endif
