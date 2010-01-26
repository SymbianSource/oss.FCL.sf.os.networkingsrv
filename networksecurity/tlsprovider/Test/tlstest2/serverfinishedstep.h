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
 @file serverfinishedstep.h
 @internalTechnology	
*/
#ifndef __SERVERFINISHEDSTEP_H__
#define __SERVERFINISHEDSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KServerFinishedStep, "ServerFinishedStep");
_LIT(KTampterHandshakeMessages, "TamperHandshake");

class CServerFinishedStep : public CTlsStepBase
	{
public:
	CServerFinishedStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	void ValidateServerFinishedL(const TDesC8& aMasterSecret);
	};

#endif /* __SERVERFINISHEDSTEP_H__ */
