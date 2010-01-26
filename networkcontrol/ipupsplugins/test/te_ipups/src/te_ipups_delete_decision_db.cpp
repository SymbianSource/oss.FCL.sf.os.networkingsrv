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
#include "te_ipups_delete_decision_db.h"
 
CIpUpsDeleteDecisionDB::CIpUpsDeleteDecisionDB()
/**
 * Constructor
 */
	{
	SetTestStepName(KIpUpsDeleteDecisionDB);
	}

CIpUpsDeleteDecisionDB::~CIpUpsDeleteDecisionDB()
/**
 * Destructor
 */
	{ 	
	}

TVerdict CIpUpsDeleteDecisionDB::doTestStepPreambleL()
/**
 * @return - TVerdict code
 */
	{	
 	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CIpUpsDeleteDecisionDB::doTestStepL()
/**
 * @return - TVerdict code
 */
	{
	UserPromptService::RUpsManagement upsManagement;
	
	//Connect to the UPS Manager
	User::LeaveIfError(upsManagement.Connect());
	
	//Delete the contents of the Decision Database
 	TRAPD(err, upsManagement.DeleteDatabaseL());
 	
 	if (err != KErrNone)
 		{
 		ERR_PRINTF2(_L("UPS Database Deletion attempt failed ( %d )"), err);
 		}
 	else
 		{
 		INFO_PRINTF1(_L("UPS Database Deleted."));
 		} 	
 	
 	SetTestStepError(err);
 	
	return TestStepResult();
	}
	
TVerdict CIpUpsDeleteDecisionDB::doTestStepPostambleL()
/**
 * @return - TVerdict code
 */
	{
	return TestStepResult();
	}
