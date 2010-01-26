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
// IPProto-proprietary messages
// 
//

/**
 @file
 @internalTechnology
*/

#include <ss_glob.h>
#include <comms-infras/ss_thread.h>
#include "IPProtoMessages.h"



START_ATTRIBUTE_TABLE( CSubConTFTParameterSet, CSubConTFTParameterSet::EUid, CSubConTFTParameterSet::ETypeId )
END_ATTRIBUTE_TABLE()


CSubConExtensionParameterSet* CIPProtoSubConnParameterFactory::NewL(TAny* aConstructionParameters)
	{
	TInt32 type = reinterpret_cast<TInt32>(aConstructionParameters);
	switch (type)
		{
	case (CSubConTFTParameterSet::ETypeId):
		return new (ELeave) CSubConTFTParameterSet;
 
	default:
		User::Leave(KErrNotFound);
		}
	return NULL;
	}
