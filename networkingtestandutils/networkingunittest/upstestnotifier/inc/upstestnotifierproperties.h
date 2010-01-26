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

#ifndef __UPSTESTNOTIFIERPROPERTIES_H__
#define __UPSTESTNOTIFIERPROPERTIES_H__

#include <e32property.h>

/**
P&S category for all UPS Test Notifier properties.
*/
static const TUid KUidPSUPSTestNotifCategory = {0x10272F43}; //UID from RobL - Pre-Platsec P&S

//Properties published by the UPS Notifier
static const TInt KUnStart = 0;

static const TInt KUnNotifyCount = KUnStart + 0;
static const RProperty::TType KUnCountKeyType = RProperty::EInt;

static const TInt KUnNotifyValues = KUnStart + 1;
static const RProperty::TType KUnNotifyValuesKeyType = RProperty::EByteArray;

static const TInt KUnStoredNotifyCount = KUnStart + 2;
static const RProperty::TType KUnStoredCountKeyType = RProperty::EInt;

//Properties published by the UPS Notifier Tests
static const TInt KUtStart = 100;

static const TInt KUtButtonPress = KUtStart + 0;
static const RProperty::TType KUtButtonPressKeyType = RProperty::EByteArray;

static const TInt KUtButtonPressDelay = KUtStart + 1;
static const RProperty::TType KUtButtonPressDelayKeyType = RProperty::EInt;

static const TInt KUtFileOverride = KUtStart + 2;
static const RProperty::TType KUtFileOverrideKeyType = RProperty::EInt;

static const TInt KNoFileOverride = 0;
static const TInt KFileOverride = 1;


#endif /*__UPSTESTNOTIFIERPROPERTIES_H___*/
