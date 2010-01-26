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
 @file encryptstep.h
 @internalTechnology	
*/
#ifndef __ENCRYPTSTEP_H__
#define __ENCRYPTSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KEncryptStep, "EncryptStep");
_LIT(KRecordSize, "RecordSize");
_LIT(KRecordType, "RecordType");
_LIT(KSequenceNumber, "SequenceNumber");

class CEncryptStep : public CTlsStepBase
	{
public:
	CEncryptStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	void VerifyEncryptionL();
	
private:
	TInt doTestStepImplL();	
	};

#endif /* __ENCRYPTSTEP_H__ */
