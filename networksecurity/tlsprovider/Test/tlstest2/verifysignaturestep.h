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
 @file verifysignaturestep.h
 @internalTechnology	
*/
#ifndef __VERIFYSIGNATURESTEP_H__
#define __VERIFYSIGNATURESTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KVerifySignatureStep, "VerifySignature");
_LIT(KTamperedDigest, "TamperedDigest");

class CVerifySignatureStep : public CTlsStepBase
	{
public:
	CVerifySignatureStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	};

#endif /* __VERIFYSIGNATURESTEP_H__ */
