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
 @file ciphersuitestep.h
 @internalTechnology	
*/
#ifndef __TLSCIPHERSTEP_H__
#define __TLSCIPHERSTEP_H__

#include <e32base.h>
#include <tlstypedef.h>
#include "tlsstepbase.h"

_LIT(KCipherSuitesStep, "CipherSuitesStep");

_LIT(KNumCipherSuites, "NumCipherSuites");
_LIT(KCipherSuiteBase, "CipherSuite");

class CCipherSuitesStep : public CTlsStepBase
	{
public:
	CCipherSuitesStep();

	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	~CCipherSuitesStep();
	RArray<TTLSCipherSuite> iSuites;	
	};

#endif /* __TLSCIPHERSTEP_H__ */
