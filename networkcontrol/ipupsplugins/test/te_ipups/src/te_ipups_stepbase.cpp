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

#include "te_ipups_stepbase.h"

TVerdict CTeIpUpsStepBase::doTestStepPreambleL()
/**
 * @return - TVerdict
 */
	{
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CTeIpUpsStepBase::doTestStepPostambleL()
/**
 * @return - TVerdict
 */
	{
	return TestStepResult();
	}

CTeIpUpsStepBase::~CTeIpUpsStepBase()
	{
	}

CTeIpUpsStepBase::CTeIpUpsStepBase()
	{	
	}
