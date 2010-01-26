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
 @file ipeventnotifierfamily.h
 @internalTechnology
*/


#ifndef __IPEVENTNOTIFIERFAMILY_H__
#define __IPEVENTNOTIFIERFAMILY_H__

#include <ip6_hook.h>

#include "ipeventnotifier.h"



class CIPEventNotifierProtocolFamily : public CProtocolFamilyBase
	{
public:
	// explicit so we don't get default copy constructor
	explicit CIPEventNotifierProtocolFamily();
	virtual ~CIPEventNotifierProtocolFamily();
	
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc*& aProtocolList);
	CProtocolBase* NewProtocolL(TUint aSockType, TUint aProtocol);
	};


#endif // __IPEVENTNOTIFIERFAMILY_H__



