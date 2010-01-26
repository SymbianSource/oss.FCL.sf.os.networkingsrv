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
 @file
 @internalComponent 
*/

#if !defined(__IRLANUTL_H__)
#define __IRLANUTL_H__

enum TIrlanPanic
{ 
	EIrlanParameterNameTooBig,
	EIrlanParameterValueTooBig,
	EIrlanBadState,
	EIrlanErrorState,
	EIrlanDataWrite,
	EIrlanDataRead,
	EIrlanCannotAllocMBuf,
};

enum TIrlanFault
{
	EIrlanOK, //< Specifies ok
	EIrlanInvalidParameterCount, //<  Specifies Invalid parameter count
	EIrlanInvalidParameter, //< Specifies Invalid parameter.
	EIrlanInvalidParameterValue, //< Specifies Invalid parameter value
	EIrlanInvalidIrlanMode, //< Specifies Invalid Irlan mode
	EIrlanNonEtherIIPacket, //< Specifies Non Ether packet.
};

/**
@internalComponent
*/
class IrlanUtil
{
public:
	static void Fault(TIrlanFault aFault);
	static void Panic(TIrlanPanic aPanic);
};

#endif

