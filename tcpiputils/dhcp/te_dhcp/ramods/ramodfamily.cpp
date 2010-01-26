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
//

/**
 @file ramodfamily.cpp
 @internalComponent
*/

 
#include "ramodfamily.h"
#include "ramod.h"
//TODO why is this header included? #include <ss_protprov.h>
#include <ss_glob.h>

/*
 * @internalTechnology
 */
IMPORT_C CProtocolFamilyBase* Install(void);

/*
 * @internalTechnology
 */
EXPORT_C CProtocolFamilyBase* Install(void)
	{
	return new (ELeave) CramodProtocolFamily;
	}

/**
 *	CramodProtocolFamily
 */
CramodProtocolFamily::CramodProtocolFamily()
	{
	__DECLARE_NAME(_S("CramodProtocolFamily"));
	}

CramodProtocolFamily::~CramodProtocolFamily()
	{
	}

TInt CramodProtocolFamily::Install()
	{
	return KErrNone;
	}

TInt CramodProtocolFamily::Remove()
	{
	return KErrNone;
	}

TUint CramodProtocolFamily::ProtocolList(TServerProtocolDesc*& aProtocolList)
	{
	TServerProtocolDesc* p = new (ELeave) TServerProtocolDesc[1]; // Esock catches this leave

	Cramod::FillIdentification(p[0]);
	aProtocolList = p;
	return 1;
	}

CProtocolBase* CramodProtocolFamily::NewProtocolL(TUint /*aSockType*/,
												   TUint aProtocol)
	{
	__ASSERT_ALWAYS(aProtocol == KMyProtocolId, User::Leave(KErrNotSupported) );

	CProtocolBase* pProto=Cramod::NewL();
	
	return pProto;
	}




