// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @test
 @internalComponent - Internal Symbian test code 
*/

#if (!defined TE_IPUPS_STEP_BASE)
#define TE_IPUPS_STEP_BASE
#include <test/testexecutestepbase.h>
#include <ups/upsclient.h>

class CTeIpUpsStepBase : public CTestStep
	{
public:
	virtual ~CTeIpUpsStepBase();
	CTeIpUpsStepBase();
	virtual TVerdict doTestStepPreambleL(); 
	virtual TVerdict doTestStepPostambleL();	
	};

#endif
