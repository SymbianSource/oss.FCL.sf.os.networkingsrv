/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
*/

#ifndef __TLSOOMCONNECTSTEP_H__
#define __TLSOOMCONNECTSTEP_H__

#include "tlsoomstepbase.h"

_LIT(KConnectStep, "Connect");

class CTlsOOMConnectStep : public CTlsOOMStepBase
	{
	
public:
	static CTlsOOMConnectStep* NewL(CTestExecuteLogger& aLogger);	

private:
	CTlsOOMConnectStep(CTestExecuteLogger& aLogger);
	void DoTestStepL();
	
	
	};

#endif /* __TLSOOMCONNECTSTEP_H__ */
