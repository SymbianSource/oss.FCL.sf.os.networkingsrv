// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file createnegativestep.cpp
 @internalTechnology
*/
#include "createnegativestep.h"

#include <tlsprovinterface.h>
#include <tlstypedef.h>

const TTLSProtocolVersion KBadSetting = {4,4}; 

CNegativeCreateStep::CNegativeCreateStep()
	{
	SetTestStepName(KNegativeCreatedStep);
	}
	
TVerdict CNegativeCreateStep::doTestStepPreambleL()
	{
	ConstructL();
	return EPass;
	}


TVerdict CNegativeCreateStep::doTestStepL()
	{
	CTlsCryptoAttributes* atts = Provider()->Attributes();	
	atts->iNegotiatedProtocol = KBadSetting;
	
	TInt err = CreateSessionL(); 
	
	// Part A
	if(err != KErrSSLAlertIllegalParameter)
		{
		INFO_PRINTF3(_L("A Failed: CreateSessionL expected error: %d, returned error: %d)"), 
				KErrSSLAlertIllegalParameter, err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	// Part B
				
	atts->iNegotiatedProtocol = KTLS1_0;
	atts->iCurrentCipherSuite.iHiByte = 20;
	atts->iCurrentCipherSuite.iLoByte = 20;
	err = 0;
	err = CreateSessionL(); 
	
	if(err != KErrSSLAlertIllegalParameter)
		{
		INFO_PRINTF3(_L("B Failed: CreateSessionL expected error: %d, returned error: %d)"), 
				KErrSSLAlertIllegalParameter, err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	
	// Part C
	atts->iCurrentCipherSuite.iHiByte = 0;
	atts->iCurrentCipherSuite.iLoByte = 3;
	err = 0;
	err = CreateSessionL(); 
	
	if(err != KErrSSLAlertIllegalParameter)
		{
		INFO_PRINTF3(_L("B Failed: CreateSessionL expected error: %d, returned error: %d)"), 
				KErrSSLAlertIllegalParameter, err);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
					
	return TestStepResult();
	}
	
