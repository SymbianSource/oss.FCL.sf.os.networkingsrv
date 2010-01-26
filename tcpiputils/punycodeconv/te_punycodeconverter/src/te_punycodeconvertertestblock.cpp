// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains the implementation of TEF3.0 respective test block
//



/**
 @file
 @internalTechnology
*/

#include "te_punycodeconvertertestblock.h" 

/**
Function to manage the wrapper object instantiation. As part of this test suite, 
only one wrapper is used.

This function is called by the TestEngine while parsing the CREATE_OBJECT command
in the script file.

@return Returns the wrapper instance pointer
@param aData Name of the wrapper has to be instantiated.
*/
CDataWrapper* CPunycodeConverterTestBlock::CreateDataL(const TDesC& aData)
	{
	CDataWrapper* wrapper = NULL;
	if (KPunycodeConverterTestWrapper() == aData)
		{
		wrapper = CPunycodeConverterTestWrapper::NewL();
		}
	return wrapper;
	}
