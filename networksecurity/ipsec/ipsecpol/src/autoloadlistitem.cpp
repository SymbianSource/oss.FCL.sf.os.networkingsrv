// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IpsecPol - autoloadlistitem.cpp
//


#include "autoloadlistitem.h"
#include "ipsecpol.h"


//=====================================================================
// 
//  construct/destruct
// 
// ==================================================================== 

CAutoloadListItem::CAutoloadListItem()
    {
    
    }

CAutoloadListItem::~CAutoloadListItem()
    { 
    }

//=====================================================================
// 
//  CAutoloadListItem::NewL
//  create apnlist object
// 
// ==================================================================== 

CAutoloadListItem* CAutoloadListItem::NewL(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle)
    {
    CAutoloadListItem* self = CAutoloadListItem::NewLC(aPreloadPolicyHandle, aPostloadPolicyHandle, aParentPolicyHandle);
    CleanupStack::Pop(); // self
    return self;
    }

CAutoloadListItem* CAutoloadListItem::NewLC(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle)
    {
    CAutoloadListItem* self;
    self = new(ELeave) CAutoloadListItem;
    CleanupStack::PushL(self);
    self->ConstructL( aPreloadPolicyHandle, aPostloadPolicyHandle, aParentPolicyHandle);
    return self;
    }

//=====================================================================
// 
//  CAutoloadListItem::ConstructL
//  initialize apnlist object
// 
// ==================================================================== 

void CAutoloadListItem::ConstructL(const TUint32 aPreloadPolicyHandle, const TUint32 aPostloadPolicyHandle, const TUint32 aParentPolicyHandle)
    {
    iParentPolicyHandle = aParentPolicyHandle;
    iPreloadHandle = aPreloadPolicyHandle; 
    iPostloadHandle = aPostloadPolicyHandle; 
    }

//=====================================================================
// 
//  CAutoloadListItem::GetAutoloadPolicyHandle()
//  return APN owned by the object
// 
// ==================================================================== 


TUint32 CAutoloadListItem::GetParentPolicyHandle()
    {
    return 	iParentPolicyHandle;
    }


TUint32 CAutoloadListItem::GetPreloadPolicyHandle()
    {
    return 	iPreloadHandle;
    }

TUint32 CAutoloadListItem::GetPostloadPolicyHandle()
    {
    return 	iPostloadHandle;
    }    



//End of file
