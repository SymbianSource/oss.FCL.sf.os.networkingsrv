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
//

/**
 @file SLIPDLL.CPP
*/

#include "SLIP.H"


//
// Panic
//

/**
Panic the Protocol

@internalComponent
@param aPanic panic number
*/
void SlipPanic(TSlipPanic aPanic)
	{
   _LIT(KSlip, "SLIP");
	User::Panic(KSlip,aPanic);
	}

/**
Panic the Protocol

@internalComponent
@param aFault panic number
*/
void SlipFault(TSlipFault aFault)
	{
   _LIT(KSlipFault, "SLIP Fault");
	User::Panic(KSlipFault,aFault);
	}


TInt CSlipIfFactory::Info(TNifIfInfo& aInfo, TInt) const
	{
	CSlip::FillInInfo(aInfo);
	return 1;
	}

void CSlipIfFactory::InstallL()
	{

	}

CNifIfBase* CSlipIfFactory::NewInterfaceL(const TDesC& aName)
	{

   _LIT(KSlip, "Slip");
	if(aName.CompareF(KSlip))
		User::Leave(KErrNotSupported);

	return new (ELeave) CSlip(*this);
	}
	
// Force export of non-mangled name
extern "C" { IMPORT_C CNifFactory *Install(); }

EXPORT_C CNifFactory *Install()
	{
	return new (ELeave) CSlipIfFactory;
	}

