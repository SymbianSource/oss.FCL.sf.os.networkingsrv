// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __NETUPSPROPERTIES_H__
#define __NETUPSPROPERTIES_H__

#include <e32property.h>

/**
P&S category for all UPS Test Notifier properties.
*/
//static const TUid KUidNetUpsTestNotifCategory = {0x10285ACE}; 
static const TUid KUidNetUpsTestNotifCategory = {0x10204BB6}; 

//Properties published by the UPS Notifier
static const TInt KNetUpsTestKeyStart = 0;

static const TInt KNetUpsSubSessionTestErrorPath = KNetUpsTestKeyStart + 0;
static const RProperty::TType KNetUpsSubSessionTestErrorPathType = RProperty::EInt;

static const TInt KNetUpsSubSessionError = KNetUpsTestKeyStart + 1;
static const RProperty::TType KNetUpsSubSessionErrorType = RProperty::EInt;

#endif /*__NETUPSTESTNOTIFIERPROPERTIES_H___*/
