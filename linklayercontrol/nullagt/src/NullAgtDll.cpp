// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file NullAgtDll.CPP 
 @internalComponent
*/

#include <e32base.h>
#include "NullAgent.h"


/**
@param aPanic,a variable of enum TNullAgentPanic of class NullAgent which contains panic codes.
*/
GLDEF_C void NullAgentPanic(NullAgent::TNullAgentPanic aPanic)
	{

	User::Panic(KNullAgentName, aPanic);
	}
