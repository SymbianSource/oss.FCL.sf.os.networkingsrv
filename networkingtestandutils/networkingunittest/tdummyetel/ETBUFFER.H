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

#if !defined(__ET_BUFFER_H__)
#define __ET_BUFFER_H__

/**
Number of server-side slots for each notification (greater than 1)
@internalComponent
*/
const TUint KSlotNumbersDefault				= 1;

/**
RCall
@internalComponent
*/
const TUint KSlotNumbersCallCapsChange		= 2;
const TUint KSlotNumbersCallStatusChange	= 4;

/**
RLine
@internalComponent
*/
const TUint KSlotNumbersLineStatusChange	= 4;
const TUint KSlotNumbersLineCallAddedChange = 4;



#endif
