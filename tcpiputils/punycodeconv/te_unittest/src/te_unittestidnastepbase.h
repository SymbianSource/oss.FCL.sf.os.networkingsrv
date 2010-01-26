// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @internalTechnology
*/


#ifndef __TE_UNITTEST_IDNATESTSTEP_BASE__
#define __TE_UNITTEST_IDNATESTSTEP_BASE__

#include "testexecutestepbase.h"

#include "f32file.h"
#include "c32comm.h"

#include <e32base.h>

class CIDNATestStep : public CTestStep
	{
public:
 	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepPostambleL();
protected:
	
private:
	CActiveScheduler* iTestScheduler;

	};


#endif // __TE_UNITTEST_IDNATESTSTEP_BASE__


