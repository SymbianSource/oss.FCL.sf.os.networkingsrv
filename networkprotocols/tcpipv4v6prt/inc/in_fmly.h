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
// in_fmly.h - family values
//



/**
 @internalComponent
*/
#ifndef __IN_FMLY_H__
#define __IN_FMLY_H__

#include <e32def.h>

const TUint KInet6MajorVersionNumber=0;
const TUint KInet6MinorVersionNumber=1;
const TUint KInet6BuildVersionNumber=1;

const TInt KInet6DefaultPriority = 10;

// Panic codes
enum TInet6Panic
	{
	EInet6Panic_NotSupported,
	EInet6Panic_NoData,
	EInet6Panic_BadBind
	};

void Panic(TInet6Panic aPanic);

#endif
