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
// Written by Mal September 1997 
// 
//

/**
 @file
*/

#include <e32base.h>
#include "IRLANUTL.H"

/**
Panic the driver due to user error.
@param aFault The Fault number.
*/
void IrlanUtil::Fault(TIrlanFault aFault)
{
	User::Panic(_L("Irlan Fault"),aFault);
}

/**
Panic the driver due to protocol bug.
@param aPanic The panic number.
*/
void IrlanUtil::Panic(TIrlanPanic aPanic)
{
	User::Panic(_L("Irlan Panic"),aPanic);
}
