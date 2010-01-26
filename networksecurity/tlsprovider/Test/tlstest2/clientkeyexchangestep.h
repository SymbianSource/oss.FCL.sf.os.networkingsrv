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
 @file clientkeyexchangestep.h
 @internalTechnology	
*/

#ifndef __CLIENTKEYEXCHANGESTEP_H__
#define __CLIENTKEYEXCHANGESTEP_H__

#include <e32base.h>

#include <tlstypedef.h>
#include "tlsstepbase.h"

class CTLSProvider;

_LIT(KClientKeyExchangeStep, "ClientKeyExchangeStep");

class CClientKeyExchangeStep : public CTlsStepBase
	{
public:
	CClientKeyExchangeStep();
	
	void ExamineKeyExchangeMessageL(const TDesC8& aMessage,
		const RArray<TTLSCipherSuite>& aCiphers);

	TVerdict doTestStepL();
	TVerdict doTestStepPreambleL();
	};

#endif /* __CLIENTKEYEXCHANGESTEP_H__ */
