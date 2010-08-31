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


#if (!defined NOTIFY_COUNT_STEP_H)
#define NOTIFY_COUNT_STEP_H
#include <test/testexecutestepbase.h>

class CIpUpsNotifyCount : public CTeIpUpsStepBase
	{
public:
	CIpUpsNotifyCount();
	virtual ~CIpUpsNotifyCount();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepL();
	virtual TVerdict doTestStepPostambleL();	
	};

_LIT(KIpUpsNotifyCount,"IpUpsNotifyCount");

_LIT(KIpUpsStorePromptTriggerCount, "StorePromptTriggerCount");

#endif
