// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PRECOPY.CPP
// @file
// @internalComponent
// Pre Copy file preparation
// 
//

// EPOC includes
#include <e32base.h>
#include <f32file.h>

#include "TS_DummyPPPStep.h"

//
//
// Test Pre Copy
/* This New Class is added to move copyfile command from script file
   This test does a copy operation 	
*/
CDummyPPPPreCopy::CDummyPPPPreCopy()
	{
	// store the name of this test case
	iTestStepName = _L("PreCopyTest");
	}

CDummyPPPPreCopy::~CDummyPPPPreCopy()
	{
	}

// Copy Test Main Code
enum TVerdict CDummyPPPPreCopy::doTestStepL( void )
	{
	copyFileL(KAgentSrcPath, KAgentDestPath);				// agentdialog.ini
	TRAPD (error, copyFileL(KPppDestPath, KPppTempPath));	// back up ppp.ini (if it exists)
	copyFileL (KPppSrcPath, KPppDestPath);					// get ppp.ini for test
	return EPass;
	}


