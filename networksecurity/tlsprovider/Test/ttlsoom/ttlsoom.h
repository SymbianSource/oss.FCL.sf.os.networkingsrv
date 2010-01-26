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

#ifndef __TTLSOOM_H__
#define __TTLSOOM_H__

#include <TestExecuteServerBase.h>

class CTlsOOMServer : public CTestServer
	{
public:
	static CTlsOOMServer* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);

	};

#endif	/*__TTLSOOM_H__ */
