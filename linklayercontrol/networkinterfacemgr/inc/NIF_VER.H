/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file NIF_VER.H
 @internalComponent
*/

#if !defined(__NIF_VER_H__)
#define __NIF_VER_H__

#include <e32def.h>

/**
@internalComponent
*/
const TUint KNif32MajorVersionNumber=1;
const TUint KNif32MinorVersionNumber=0;

/** this must not be changed - expected by TCPIP to be >=68 */
const TUint KNif32BuildVersionNumber=68;	

#endif
