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
// ECOM implementation for the IP connection and sub-connection provider factories
// 
//

/**
 @file
 @internalComponent
*/


#include <ecom/implementationproxy.h>
#include <ecom/ecom.h>

#include "IPCprFactory.h"
#include "ipdeftscprfactory.h"
#include "IPSCPRFactory.h"
#include <ip_subconparams.h>

//
// ECOM Implementation
//

const TImplementationProxy ImplementationTable[] =
	{   
	IMPLEMENTATION_PROXY_ENTRY(CIPConnectionProviderFactory::iUid, CIPConnectionProviderFactory::NewL),
	IMPLEMENTATION_PROXY_ENTRY(CIpDefaultSubConnectionProviderFactory::iUid, CIpDefaultSubConnectionProviderFactory::NewL),   
	IMPLEMENTATION_PROXY_ENTRY(CIpSubConnectionProviderFactory::iUid, CIpSubConnectionProviderFactory::NewL),
	};


EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

