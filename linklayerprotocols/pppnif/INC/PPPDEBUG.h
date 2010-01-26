/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Defines some debug-only contants that can be used to invoke
* test-only code in PPP.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __PPPDEBUG_H__
#define __PPPDEBUG_H__

#include <in_iface.h>

#ifdef _DEBUG
/**
 * Defines a debug-only PPP control option name that 
 * forces the renegotiation of the link. This option name
 * is for testing only and should not be used for any
 * other purpose.
 *
 * @internalComponent
*/
const TUint KSoIfForceRenegotiatePPP = KSoIfGetConnectionInfo + 25; // 25 is used to reduce the chance of a conflict.
#endif // #ifdef _DEBUG

#endif // #ifndef __PPPDEBUG_H__
