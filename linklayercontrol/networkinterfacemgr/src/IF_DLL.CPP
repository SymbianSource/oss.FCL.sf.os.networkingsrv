// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Interface Manager
// 
//

/**
 @file
*/


#include "IF_DEF.H"
#include "NI_STD.H"
#include "Ni_Log.h"

void Panic(TNifManPanic aPanic)
	{
	_LIT(nifmanString,"NifMan");
	LOG( NifmanLog::Printf(_L("%S Panic %d"), &nifmanString(), aPanic); )
	
	User::Panic(nifmanString,aPanic);
	}

void MBufExtPanic(TMBufExtPanic aPanic)
	{
	_LIT(mBufExtString,"MBufExt");
	LOG( NifmanLog::Printf(_L("%S Panic %d"), &mBufExtString(), aPanic); )

	User::Panic(mBufExtString, aPanic);
	}

void NetUtilPanic(TNetUtilPanic aPanic)
	{
	_LIT(netUtilString,"NetUtil");
	LOG( NifmanLog::Printf(_L("%S Panic %d"), &netUtilString(), aPanic); )

	User::Panic(netUtilString, aPanic);
	}


// Force export of non-mangled name
extern "C" { IMPORT_C CSocketServExtBase *Install(); }
EXPORT_C CSocketServExtBase *Install()
	{
	return new CNifMan;
	}

