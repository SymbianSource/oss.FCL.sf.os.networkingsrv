// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 @file 
 @internalComponent
*/


#ifndef __TUN_PAN__
#define __TUN_PAN__

//  Data Types
enum TTunPanic
    {
    ETunPanic_BadBind,
    ETunPanic_BadHeader,
    ETunPanic_BadCall,
    ETunPanic_CorruptPacketIn
    };

//  Function Prototypes
GLREF_C void Panic(TTunPanic aPanic);

#endif  // __TUN_PAN__

