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


#ifndef __TUNFAMILY_H__
#define __TUNFAMILY_H__

#include <in_bind.h>

class CProtocolFamilyTun : public CProtocolFamilyBase
/**  
* Class is derived from CProtocolFamilyBase.
* This will Install Protocol Family and load new protocol module tun.
* Implements virtual functions of base class CProtocolFamilyBase( Install, remove)
* 
**/ 
    {
public:
    CProtocolFamilyTun();
    ~CProtocolFamilyTun();
    TInt Install();
    TInt Remove();
    TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
    CProtocolBase* NewProtocolL(TUint /*aSockType*/, TUint aProtocol);
    };
    
#endif  //__TUNFAMILY_H__
