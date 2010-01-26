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
// ECOM implementation for the IP Protocol provider factories
// 
//

/**
 @file
 @internalComponent
*/


#include <ecom/implementationproxy.h>
#include <ecom/ecom.h>

#include "IPProtoTierManagerFactory.h"
#include "IPProtoMCprFactory.h"
#include "IPProtoCPRFactory.h"
#include "IPProtoSCPRFactory.h"
#include "IPProtoMessages.h"
#include "flow.h"

//
// ECOM Implementation
//

const TImplementationProxy ImplementationTable[] =
	{   
	IMPLEMENTATION_PROXY_ENTRY(CIPProtoTierManagerFactory::iUid, CIPProtoTierManagerFactory::NewL),	   
	IMPLEMENTATION_PROXY_ENTRY(CIPProtoMetaConnectionProviderFactory::iUid, CIPProtoMetaConnectionProviderFactory::NewL),
   	IMPLEMENTATION_PROXY_ENTRY(CIPProtoConnectionProviderFactory::iUid, CIPProtoConnectionProviderFactory::NewL),
   	IMPLEMENTATION_PROXY_ENTRY(CIPProtoSubConnectionProviderFactory::iUid, CIPProtoSubConnectionProviderFactory::NewL),
   	IMPLEMENTATION_PROXY_ENTRY(KIPShimFlowImplUid, CIPShimFlowFactory::NewL),
	IMPLEMENTATION_PROXY_ENTRY(CIPProtoSubConnParameterFactory::EUid, CIPProtoSubConnParameterFactory::NewL)
   	};


EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
   {
   aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
   return ImplementationTable;
   }

