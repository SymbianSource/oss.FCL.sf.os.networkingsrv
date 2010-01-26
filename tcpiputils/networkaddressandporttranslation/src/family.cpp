// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "family.h"
#include "hookdefs.h"
#include <in_chk.h>


class CProtocolNapt; 
 
CProtocolFamilyNapt::CProtocolFamilyNapt()
	{
	__DECLARE_NAME(_S("CProtocolFamilyNapt"));
	}

CProtocolFamilyNapt::~CProtocolFamilyNapt()
//Destructor
	{
	}

TInt CProtocolFamilyNapt::Install()
	{
	return KErrNone;
	}

TInt CProtocolFamilyNapt::Remove()
	{
	return KErrNone;
	}

TUint CProtocolFamilyNapt::ProtocolList(TServerProtocolDesc* &aProtocolList)
	{
	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[1]; // Esock catches this leave
	CProtocolNapt::Describe(p[0]);
	aProtocolList = p;
	return 1;
	}

CProtocolBase* CProtocolFamilyNapt::NewProtocolL(TUint /*aSockType*/,
												   TUint aProtocol)
	{
	if (aProtocol != KProtocolNAPT)
		{
		User::Leave(KErrNotSupported);
		}
	CProtocolNapt* napt = CProtocolNapt::NewL();

#ifdef __DEBUG
//this is to check memory leaks.This macro will be enables in debug build only.
//This will macro when protocol is loaded and will be unmarked when it is unloaded.
__UHEAP_MARK;
#endif

	return napt; 
	}



//
// Entrypoint
//
#ifndef EKA2
GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}
#endif //#ifndef EKA2

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase* Install(void); }
EXPORT_C CProtocolFamilyBase* Install(void)
	{
	CProtocolFamilyNapt* protocol = new CProtocolFamilyNapt();
	if (protocol)
		{
		return protocol;
		}
	else 
		{
		return NULL;
		}
	}

