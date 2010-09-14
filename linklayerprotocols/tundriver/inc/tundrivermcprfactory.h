/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Header file for the tunnel driver mcpr factory configuration.
* 
*
*/

/**
 @file tundrivermcprfactory.h
 @internalTechnology
*/

#ifndef _TUNDRIVERMCPR_FACTORY_H
#define _TUNDRIVERMCPR_FACTORY_H

#include <comms-infras/ss_metaconnprov.h>

class CTunDriverMetaConnectionProviderFactory : public ESock::CMetaConnectionProviderFactoryBase
	{
public:
    enum { iUid = 0x10281E05 };
	static CTunDriverMetaConnectionProviderFactory* NewL(TAny* aParentContainer);

protected:
    CTunDriverMetaConnectionProviderFactory(TUid aFactoryId, ESock::CMetaConnectionFactoryContainer& aParentContainer);
	ESock::ACommsFactoryNodeId* DoCreateObjectL(ESock::TFactoryQueryBase& aQuery);
	};

#endif
//_TUNDRIVERMCPR_FACTORY_H
