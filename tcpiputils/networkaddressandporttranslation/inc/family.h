// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @prototype
*/


#ifndef __NAPTFAMILY_H__
#define __NAPTFAMILY_H__

#include <in_bind.h>
#include <in6_event.h>
#include <in_sock.h>
#include "panic.h"


class CProtocolFamilyNapt : public CProtocolFamilyBase
/**  
* Class is derived from CProtocolFamilyNapt.
* This will Install Protocol Family and load new protocol napt.
* Implements virtual functions of base class CProtocolFamilyBase( Install, remove)
* 
**/	
	{
public:
	CProtocolFamilyNapt();
	~CProtocolFamilyNapt();
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	CProtocolBase* NewProtocolL(TUint /*aSockType*/, TUint aProtocol);
	TBool iStartNapt;
	};
	
#endif  //__NAPTFAMILY_H__
