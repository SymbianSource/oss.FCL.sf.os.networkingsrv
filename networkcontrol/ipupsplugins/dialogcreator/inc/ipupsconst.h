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
// This file specifies the constants which are used to configure
// the NetUps during testing. The associated enumerations are to
// to be found in netupstls.h.
// @internalComponent
// @prototype
// 
//

#ifndef IPUPSCONST_H
#define IPUPSCONST_H

#include <e32def.h>

/** @SYMPatchable Disable UPS IP functionality 
*/
IMPORT_C extern const TUint32 KNotifierImplementationId;
IMPORT_C extern const TUint32 KTestNotifierImplementationId;


#endif // IPUPSCONST_H