/*
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
* Name        : 6to4_family.h
* Part of     : 6to4 plugin / 6to4.prt
* Implements 6to4 automatic and configured tunnels, see
* RFC 3056 & RFC 2893
* Version     : 0.2
*
*/




/**
 @internalComponent
*/


#ifndef __6TO4_FAMILY_H
#define __6TO4_FAMILY_H

//  INCLUDES
// CONSTANTS
const TUint16 KAf6to4 = 0x624;
const TUint16 KProtocol6to4 = 0xfb5;    // likewise

// MACROS
// DATA TYPES
enum T6to4Panic
	{
	E6to4Panic_BadBind,
	E6to4Panic_NotSupported
	};

// FUNCTION PROTOTYPES
void Panic (T6to4Panic aPanic);

// FORWARD DECLARATIONS
// CLASS DECLARATION

#endif      // __6TO4_FAMILY_H   
			
// End of File
