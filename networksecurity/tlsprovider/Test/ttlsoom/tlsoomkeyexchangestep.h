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

#ifndef __TLSOOMKEYEXCHANGESTEP_H__
#define __TLSOOMKEYEXCHNAGESTEP_H__

#include "tlsoomstepbase.h"

_LIT(KKeyExchangeStep, "KeyExchange");

class CTlsOOMKeyExchangeStep : public CTlsOOMStepBase
	{
	
public:
	static CTlsOOMKeyExchangeStep* NewL(const TDesC& aConfigPath, const TDesC& aServerName,
		const TDesC& aSessionID, CTestExecuteLogger& aLogger);	

private:
	CTlsOOMKeyExchangeStep(const TDesC& aConfigPath, const TDesC& aServerName,
		const TDesC& aSessionID, CTestExecuteLogger& aLogger);	

	void DoTestStepL();
	
private:
	const TDesC& iServerName;
	const TDesC& iSessionID;
	
	
	};

#endif /* __TLSOOMKEYEXCHANGESTEP_H__ */
