// Copyright (c) 2009-2010 Nokia Corporation and/or its subsidiary(-ies).
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
// family.cpp - Packet Probe Hook
//

#include <e32std.h>
#include "family.h"
#include "prt.h"
//
// Nothing interesting here...
//
GLDEF_C TInt E32Dll()
	{
	return KErrNone;
	}

class CProtocolFamilyProbe : public CProtocolFamilyBase
	{
public:
	CProtocolFamilyProbe();
	~CProtocolFamilyProbe();
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	CProtocolBase* NewProtocolL(TUint /*aSockType*/, TUint aProtocol);
	};

extern "C" { IMPORT_C CProtocolFamilyBase* Install(); }
EXPORT_C CProtocolFamilyBase* Install()
	{
	// Due to naming convention this cannot leave
	// However null return value handled correctly in the caller...
	return new CProtocolFamilyProbe;
	}

//----------------------------------------------------------------------
// Dummy protocol family declaration. Required for DLL's
//

CProtocolFamilyProbe::CProtocolFamilyProbe()
	{
	__DECLARE_NAME(_S("CProtocolFamilyProbe"));
	}

CProtocolFamilyProbe::~CProtocolFamilyProbe()
	{
	}

TInt CProtocolFamilyProbe::Install()
	{
	return KErrNone;
	}

TInt CProtocolFamilyProbe::Remove()
	{
	return KErrNone;
	}

TUint CProtocolFamilyProbe::ProtocolList(TServerProtocolDesc *& aList)
	{
	// This function might leave, but it has been taken care of in the calling function
	const TInt index =2;
	aList = new TServerProtocolDesc[index];
	if (aList == NULL)
		return 0;
	
	CProtocolProbe::FillIdentification(aList[0], 1);
	CProtocolProbe::FillIdentification(aList[1], 2);
	return 2;
	}


CProtocolBase* CProtocolFamilyProbe::NewProtocolL(TUint /*aSockType*/,
												 TUint aProtocol)
	{
	return CProtocolProbe::NewL(aProtocol);
	}

void Panic(TProbePanic aPanic)
	{
    User::Panic(_L("PROBE"), aPanic);
	}
