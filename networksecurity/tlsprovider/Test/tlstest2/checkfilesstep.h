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

/**
 @file checkfilesstep.h
 @internalTechnology	
*/
#ifndef __CHECKFILESSTEP_H__
#define __CHECKFILESSTEP_H__

#include <e32base.h>
#include "tlsstepbase.h"


_LIT(KCheckFilesStep, "CheckFilesStep");

class CCheckFilesStep : public CTlsStepBase
	{

public:
	CCheckFilesStep();
	~CCheckFilesStep();
	
	TVerdict doTestStepL();
	void CheckIfFilesExist(TBool aCheckExist, const RArray<TPtrC>& aFileArray);
	void ExtractFileName(TInt aEntries, const TDesC& aEntryBase, RArray<TPtrC>& aFileArray);
	void GetFileNamesForCheck(RArray<TPtrC>& aFileNumExist,RArray<TPtrC>& aFileNumNonExist);
	};

#endif /* __CHECKFILESSTEP_H__ */
