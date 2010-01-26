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
// Implementation file for the IP Address Info Parameter Set
// 
//

/**
 @file
 @internalComponent
 @released since 9.5
*/


#include <e32std.h>
#include <e32test.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <networking/ipaddrinfoparams.h>
#include "ipaddrinfoparams_factory.h"
#include <comms-infras/metatypearray.h>

START_ATTRIBUTE_TABLE( CSubConIPAddressInfoParamSet, CSubConIPAddressInfoParamSet::EUid, CSubConIPAddressInfoParamSet::ETypeId )
	REGISTER_ATTRIBUTE( CSubConIPAddressInfoParamSet, iParams, TMetaArray<TSubConIPAddressInfo> )
	REGISTER_ATTRIBUTE( CSubConIPAddressInfoParamSet, iOpCode, TMetaNumber )
END_ATTRIBUTE_TABLE()

CSubConGenericParameterSet* CSubConIPAddressInfoParameterFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);

	if (type == CSubConIPAddressInfoParamSet::ETypeId)
		{
		return new (ELeave) CSubConIPAddressInfoParamSet;
		}
	else
		{
		User::Leave(KErrNotFound);
		}

	return NULL;
	}

