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
 @file ramodfamily.h
 @internalTechnology
*/


#ifndef __ramodFAMILY_H__
#define __ramodFAMILY_H__

#include <ip6_hook.h>

#include "ramod.h"



class CramodProtocolFamily : public CProtocolFamilyBase
	{
public:
	// explicit so we don't get default copy constructor
	explicit CramodProtocolFamily();
	virtual ~CramodProtocolFamily();
	
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc*& aProtocolList);
	CProtocolBase* NewProtocolL(TUint aSockType, TUint aProtocol);
	};


#endif // __ramodFAMILY_H__



