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

#include "family.h"
#include "qos_prot.h"

// DLL Install
GLDEF_C TInt E32Dll()
	{
	return(KErrNone);
	}

// Force export of non-mangled name
extern "C" { IMPORT_C CProtocolFamilyBase *Install(); }
EXPORT_C CProtocolFamilyBase *Install()
	{
	return new CProtocolFamilyQoS;
	}


// Protocol Family
CProtocolFamilyQoS::CProtocolFamilyQoS()
	{
	}

CProtocolFamilyQoS::~CProtocolFamilyQoS()
	{
	}


TInt CProtocolFamilyQoS::Install()
	{
	// Nothing to do here
	return KErrNone;
	}

TInt CProtocolFamilyQoS::Remove()
	{
	return KErrNone;
	}

TUint CProtocolFamilyQoS::ProtocolList(TServerProtocolDesc *& aProtocolList)
	{
	aProtocolList = new TServerProtocolDesc[1];
	if (aProtocolList == NULL)
		return 0;
	CProtocolQoS::Identify(aProtocolList[0]);
	return 1;
	}

CProtocolBase* CProtocolFamilyQoS::NewProtocolL(TUint aSockType,TUint aProtocol)
	{
	if (aProtocol != KProtocolQoS)
		User::Leave(KErrNotSupported);

	// KSockDatagram has no meaning here??
	if (aSockType != KSockDatagram)
		User::Leave(KErrNotSupported);

	return new (ELeave) CProtocolQoS;
	}

