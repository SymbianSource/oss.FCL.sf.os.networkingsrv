// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file
 @internalComponent
*/

#include <e32def.h>
#include <comms-infras/nifagt.h>

GLDEF_C void ImportPanic()
	{
	_LIT(KLDRIMPORT, "LDR-IMPORT");
	const TInt KLdrImportedOrdinalDoesNotExist = 2;
	
	User::Panic(KLDRIMPORT, KLdrImportedOrdinalDoesNotExist);
	}

/**
.def file placeholder for CNifCompanionSession, obsolete in this version.
*/
extern "C"
	{
	EXPORT_C void NonExistentImport1(){ImportPanic();}
	}


#if defined(__EABI__) || defined(__X86GCC__)
/*
.def file placeholders for RVCT EABI compliant compiler - when CSD agent includes nifutl.h it gets an
extra export of the run time info for MComm, thus removed for EABI builds and replaced with these
*/

extern "C"
	{
	EXPORT_C void MCommReplacementImport1() { ImportPanic(); }
	EXPORT_C void MCommReplacementImport2() { ImportPanic(); }
	EXPORT_C void MCommReplacementImport3() { ImportPanic(); }
	EXPORT_C void MTimerReplacementImport1() { ImportPanic(); }
	EXPORT_C void MTimerReplacementImport2() { ImportPanic(); }	
	EXPORT_C void MTimerReplacementImport3() { ImportPanic(); }	
	}

#endif

