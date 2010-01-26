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
 @file decryptstep.h
 @internalTechnology	
*/
#ifndef __DECRYPTSTEP_H__
#define __DECRYPTSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"

_LIT(KDecryptStep, "DecryptStep");
_LIT(KDRecordSize, "RecordSize");
_LIT(KDRecordType, "RecordType");
_LIT(KDSequenceNumber, "SequenceNumber");
_LIT(KTamperedRecordType, "TamperedRecordType");
_LIT(KTamperedSequenceNumber, "TamperedSequenceNumber");

class CDecryptStep : public CTlsStepBase
	{
public:
	CDecryptStep();
	
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	
	void VerifyDecryptionL();
	
	};

#endif /* __DECRYPTSTEP_H__ */
