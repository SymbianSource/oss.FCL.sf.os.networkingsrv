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
//

#include "qos_extension.h"


/**
Default constructor.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
*/
EXPORT_C CExtensionBase::CExtensionBase()
    {
    iData = NULL;
    }

/**
Destructor. This deletes any extension policies.

@publishedPartner
@released
@capability NetworkServices Restrict QoS operations in similar way as 
normal socket operations.
*/
EXPORT_C CExtensionBase::~CExtensionBase()
    {
    delete iData;
    }



