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
// @file ipeventfactory.h
// @internalTechnology
// 
//


#ifndef __IPEVENTFACTORY_H__
#define __IPEVENTFACTORY_H__

#include <ss_protprov.h>

#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
class CIPEventNotifierFactory : public ESock::CProtocolFamilyFactoryBase
#else
class CIPEventNotifierFactory : public ESock::CProtocolFamilyFactoryBase
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
/** Specialisation of CProtocolFamilyFactoryBase for CIPEventNotifierProtocolFamily

@see CProtocolFamilyFactoryBase
@released Since 9.1
*/
	{
public:
	static CIPEventNotifierFactory* NewL(TAny* aConstructionParameters);

protected:
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CIPEventNotifierFactory(TUid aFactoryId, ESock::CProtocolFamilyFactoryContainer& aParentContainer);
#else
	CIPEventNotifierFactory(TUint aFactoryId, CProtocolFamilyFactoryContainer& aParentContainer);
#endif // SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY	
	virtual TInt DoReceiveMessage( NetMessages::CMessage& aNetMessage );
 	};
	
#endif	// __IPEVENTFACTORY_H__




