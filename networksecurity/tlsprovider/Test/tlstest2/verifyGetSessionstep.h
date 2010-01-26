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
 @file verifyGetSessionstep.h
 @internalTechnology	
*/
#ifndef __VERIFYGETSESSIONSTEP_H__
#define __VERIFYGETSESSIONSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"


_LIT(KGetSessionStep, "VerifyGetSessionStep");
_LIT(KExpectedValue, "ExpectedResult");
_LIT(KTamperHandshakeMessage, "TamperHandshake");

class CVerifyGetSessionStep : public CTlsStepBase
	{
public:
	CVerifyGetSessionStep();
	
	// to create a block of random data and create hash objects from it.
	void ValidateServerFinishL(const TDesC8& aMasterSecret);
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	
	};


#endif /* __VERIFYGETSESSIONSTEP_H__*/
