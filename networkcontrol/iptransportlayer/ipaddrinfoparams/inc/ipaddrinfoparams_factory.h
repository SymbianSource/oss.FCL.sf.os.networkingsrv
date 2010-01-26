// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP IP Address Parameters Info factory class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef IPADDRINFOPARAMS_FACTORY_H
#define IPADDRINFOPARAMS_FACTORY_H

#include <comms-infras/ss_subconnprov.h>

/** Factory used to create instance of IP Address Parameters Info.

@internalComponent
@released Since 9.5
*/
class CSubConIPAddressInfoParameterFactory : public CBase
	{
public:
    enum
        {
        EUid = 0x102822D5,
        };	
	static CSubConGenericParameterSet* NewL(TAny* aConstructionParameters);
	};


#endif
//IPADDRINFOPARAMS_FACTORY_H
