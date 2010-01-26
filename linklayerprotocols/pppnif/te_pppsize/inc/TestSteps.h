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
 @file TestSteps.h
*/

#if !defined(_TESTSTEPS_H_)
#define _TESTSTEPS_H_

#include <test/testexecutestepbase.h>

class CPPPMinMaxMMU : public CTestStep  
{
public:
	virtual TVerdict doTestStepL();
	CPPPMinMaxMMU();
	virtual ~CPPPMinMaxMMU();
private:
	void CommInitL(TBool aEnhanced);
};

#endif
