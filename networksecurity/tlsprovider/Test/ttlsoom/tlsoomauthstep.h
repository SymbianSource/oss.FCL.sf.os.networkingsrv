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

#ifndef __TLSOOMAUTHSTEP_H__
#define __TLSOOMAUTHSTEP_H__

#include "tlsoomstepbase.h"

_LIT(KAuthStep, "Auth");

class CTlsOOMAuthStep : public CTlsOOMStepBase
	{
	
public:
	static CTlsOOMAuthStep* NewL(const TDesC& aConfigPath, CTestExecuteLogger& aLogger);	

private:
	CTlsOOMAuthStep(const TDesC& aConfigPath, CTestExecuteLogger& aLogger);
	void DoTestStepL();
	
	};


#endif /* __TLSOOMAUTHSTEP_H__ */
