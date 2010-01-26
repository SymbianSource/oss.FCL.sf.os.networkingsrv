// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements a Dummy Dynamic DNS update plug-in
// 
//

/**
 @file DummyDNSUpdate.cpp
*/

#include "DummyDNSUpdate.h"
#include "implementationproxy.h"

#if !defined(EKA2)
GLDEF_C TInt E32Dll(TDllReason /*aReason*/)
/**
 * The E32Dll method DLL entry point
 *
 * @return	KErrNone
 */
	{
	return(KErrNone);
  	}
#endif

// Define the interface UIDs
const TImplementationProxy ImplementationTable[] = 
    {
    IMPLEMENTATION_PROXY_ENTRY(0x101FEBE0, CDummyDnsUpdate::NewL)
    };

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }

CDummyDnsUpdate* CDummyDnsUpdate::NewL()
   {
   CDummyDnsUpdate* pDummyDnsUpdate = new(ELeave)CDummyDnsUpdate;
   CleanupStack::PushL( pDummyDnsUpdate );
   pDummyDnsUpdate->ConstructL();
   CleanupStack::Pop();
   return pDummyDnsUpdate;
   }

CDummyDnsUpdate::~CDummyDnsUpdate()
	{
	}


void CDummyDnsUpdate::Update( TDesC& /*aInterfaceName*/, TDesC8* /*aHostName*/, TDesC8* /*aDomainName*/ )
{
}

void CDummyDnsUpdate::ConstructL()
{
}

