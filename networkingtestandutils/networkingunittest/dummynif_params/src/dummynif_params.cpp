// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "dummynif_params.h"
#include <e32std.h>
#include <e32test.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <comms-infras/metadata.h>
#include <comms-infras/metatype.h>

//SMetaDataECom macros

START_ATTRIBUTE_TABLE( TDummyPref, TDummyPref::EUid, TDummyPref::ETypeId )
	REGISTER_ATTRIBUTE( TDummyPref, iAP, TMetaNumber )	
END_ATTRIBUTE_TABLE()


SMetaDataECom* CDummyParamaterFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
	switch (type)
		{
	case (TDummyPref::ETypeId):
		return new (ELeave) TDummyPref;
	default:
		User::Leave(KErrNotFound);
		return NULL;
		}
	}
