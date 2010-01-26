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
 @file verifyCancellationstep.h
 @internalTechnology	
*/
#ifndef __VERIFYCANCELLATIONSTEP_H__
#define __VERIFYCANCELLATIONSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KVerifyCancellationStep, "VerifyCancellationStep");
_LIT(KTamperHandshakeMessages, "TamperHandshake");

class CVerifyCancellationStep : public CTlsStepBase
	{
public:
	CVerifyCancellationStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	void ValidateServerFinishedL(const TDesC8& aMasterSecret);
	};

#endif /*__VERIFYCANCELLATIONSTEP_H__*/
