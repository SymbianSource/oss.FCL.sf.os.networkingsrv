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
// ECOM implementation for the Ethe802 provider factories
// 
//

/**
 @file
 @internalComponent
*/


#include <ecom/implementationproxy.h>
#include <ecom/ecom.h>

#include "EthCfproto.h"
#include "ethmcprfactory.h"


// ---------------- ECOM Implementation ----------------

const TImplementationProxy ImplementationTable[] =
   {   
   IMPLEMENTATION_PROXY_ENTRY(KEthFlowImplementationUid, CEthSubConnectionFlowFactory::NewL),
   IMPLEMENTATION_PROXY_ENTRY(CEthMetaConnectionProviderFactory::iUid, CEthMetaConnectionProviderFactory::NewL)
   };


EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
   {
   aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
   return ImplementationTable;
   }

