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

#include "rasdef.h"

void Panic(TNtRasPanic aPanic)
	{
   _LIT(KNtRas, "NtRas");
	User::Panic(KNtRas,aPanic);
	}


// Force export of non-mangled name
extern "C" { IMPORT_C CNifFactory *NewAgentFactoryL(); }
EXPORT_C CNifFactory *NewAgentFactoryL()
	{
	return new (ELeave) CNtRasFactory;
	}
