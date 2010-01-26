// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <ecom/implementationproxy.h>
#include "dnsproxyupdateimpl.h"

/**
*Implementation proxy array
*/
static const TImplementationProxy ImplementationTable[] =
    {
        {
           {0x200215F5}, (TProxyNewLPtr)(CDNSProxyUpdateImpl::NewL)

        }
    };

/**
*ImplementationGroupProxy method
* 
*@param  aTableCount : no of implementation present.
*
*@return : ImplementationTable
*/
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
    }

