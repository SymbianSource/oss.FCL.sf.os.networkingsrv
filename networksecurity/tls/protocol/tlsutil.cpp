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
//

/**
 @file
 @internalComponent 
*/

#include <e32def.h>
#include <e32cmn.h>
#include <e32std.h>
#include "tlsconnection.h"

GLDEF_C void TlsPanic(TTlsPanic aPanic)
/**
Global panic function for Tls protocol
*/
{
	_LIT(KTlsPanic, "TLS protocol");
	User::Panic(KTlsPanic, aPanic);
}
