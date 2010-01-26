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

#ifndef __TLSOOMSTEPWRAPPER_H__
#define __TLSOOMSTEPWRAPPER_H__

#include "tlsoomstepbase.h"

class CTlsOOMStepWrapper : public CTestStep
	{
	
public:
	CTlsOOMStepWrapper(const TDesC& aStepName);
	TVerdict doTestStepL();
	
private:
	TPtrC GetConfigPath();
	TPtrC GetTlsServerName();
	TPtrC GetTlsSessionID();
	
private:
	const TDesC& iStepName;
	
	};


#endif /* __TLSOOMSTEPWRAPPER_H__ */
