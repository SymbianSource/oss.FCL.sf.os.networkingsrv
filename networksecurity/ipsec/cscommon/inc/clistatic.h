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
 @internalComponent
*/

#ifndef __CLISTATIC_H__
#define __CLISTATIC_H__

#include <e32std.h>

const TUint KDefaultMinHeapSize =  0x1000;  //  4K
const TUint KDefaultMaxHeapSize = 0x10000;  // 64K

const TInt KDefaultMessageSlots = 2;

class Launcher
    {
public:    
    static TInt LaunchServer(const TDesC& aServerName,
                             const TDesC& aServerFileName,
                             const TUid& aServerUid3,
                             const TUint aWinsMinHeapSize = KDefaultMinHeapSize,
                             const TUint aWinsMaxHeapSize = KDefaultMaxHeapSize,
                             const TUint aWinsStackSize = KDefaultStackSize);

    };

#endif // __CLISTATIC_H__
