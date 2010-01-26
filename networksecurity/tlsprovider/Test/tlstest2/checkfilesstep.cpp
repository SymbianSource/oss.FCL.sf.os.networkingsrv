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
 @file checkfilestep.cpp
 @internalTechnology
*/
#include "checkfilesstep.h"
#include <tlsprovinterface.h>
#include "bautils.h" 

CCheckFilesStep::CCheckFilesStep()
	{
	SetTestStepName(KCheckFilesStep);
	}
	
CCheckFilesStep::~CCheckFilesStep()
	{
	}

TVerdict CCheckFilesStep::doTestStepL()
	{
	// check files presence
	RArray<TPtrC> fileNumExist;
	RArray<TPtrC> fileNumNonExist;
	GetFileNamesForCheck(fileNumExist, fileNumNonExist);

	CheckIfFilesExist(ETrue,fileNumExist);
	CheckIfFilesExist(EFalse,fileNumNonExist);
	return TestStepResult();
	}


void CCheckFilesStep::CheckIfFilesExist(TBool aCheckExist, const RArray<TPtrC>& aFileArray)
	{
	TInt nErr =0;
	RFs fs;
	User::LeaveIfError(fs.Connect());
	CleanupClosePushL(fs);
	
	if(aCheckExist)
		{
		for(TInt i =0; i< aFileArray.Count(); i++)
			{
			if(!BaflUtils::FileExists(fs, aFileArray[i]))
				{
				ERR_PRINTF2(_L("File missing: %S"), &aFileArray[i]);
				nErr++;
				}
			}
		}
	else
		{
		for(TInt i =0; i< aFileArray.Count(); i++)
			{
			if(BaflUtils::FileExists(fs, aFileArray[i]))
				{
				ERR_PRINTF2(_L("File exists (but shouldn't): %S"), &aFileArray[i]);
				nErr++;
				}
			}
		}
	if(nErr)
		{
		SetTestStepResult(EFail);
		}
	CleanupStack::PopAndDestroy(1);
	}


void CCheckFilesStep::GetFileNamesForCheck(RArray<TPtrC>& aFileNumExist,RArray<TPtrC>& aFileNumNonExist)
	{
	_LIT(KNumExist, "numexist"); 
	_LIT(KExistBase, "exist"); 
	_LIT(KNumNonExist, "numnonexist"); 
	_LIT(KNonExistBase, "nonexist"); 

	TInt entriesNumExist=0;
	TInt entriesNumNonExist=0;
	
	GetIntFromConfig(ConfigSection(), KNumExist, entriesNumExist);
	GetIntFromConfig(ConfigSection(), KNumNonExist, entriesNumNonExist);
	ExtractFileName(entriesNumExist, KExistBase, aFileNumExist);
	ExtractFileName(entriesNumNonExist, KNonExistBase, aFileNumNonExist);
	}
		
void CCheckFilesStep::ExtractFileName(TInt aEntries, const TDesC& aEntryBase, RArray<TPtrC>& aFileArray)
	{
	TPtrC fname;
	const TInt KKeyBufSize =64;
	
	for(TInt i=0; i<aEntries; i++)
		{
		//construct name of the key
		TBuf<KKeyBufSize> keyBuf(aEntryBase);
		keyBuf.AppendNum(i);
		
		if(GetStringFromConfig(ConfigSection(),keyBuf,fname))
			{
			aFileArray.Insert(fname, i);
			}
		}
	}
