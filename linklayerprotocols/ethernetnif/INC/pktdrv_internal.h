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

/**
 @file pktdrv_internal.h
 @internalAll
 @released 
*/


#if !defined(__PKTDRV__INTERNAL_H__)
#define __PKTDRV__INTERNAL_H__
#include <networking/pktdrv.h>
/**
Uid3 for Irlan pkt drv
@internalAll
*/
const TInt KUidIrlanNifmanPktDrv	= 0x10000540; 

/**
@internalComponent
@note No-one uses this - seems to be a hangover from ER5u version
*/
const TInt KLinkLayerUp		=	1;  //< Notify reasons


/**
@internalComponent
*/
enum TIeee802AddrPanics
{
	EIeee802AddrBadDescriptor,
	EIeee802AddrBadTInt64,
	EIeee802AddrBadMBuf
};


/**
@internalTechnology
*/
typedef ESock::CRefCountOwner<CPktDrvFactory> CPacketDriverOwner;

GLREF_C void Panic(TIeee802AddrPanics aCode);

#endif




