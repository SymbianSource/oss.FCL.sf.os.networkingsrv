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

#ifndef STARTUPCOMMSSTEP_H
#define STARTUPCOMMSSTEP_H

#include <testexecutestepbase.h>

_LIT(KStartupCommsStep, "StartupCommsStep");

class CStartupCommsStep : public CTestStep
	{
	public:
	CStartupCommsStep ();
	TVerdict doTestStepL();
	};

#endif //STARTUPCOMMSSTEP_H
