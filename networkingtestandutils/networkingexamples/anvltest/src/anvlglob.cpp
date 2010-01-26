// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// anvlglob.h
//

#include <e32std.h>
#include "anvlglob.h"

#ifdef __EXE__
//The Dll:: methods are not available when building an EXE.
//In order to use the same code base for EXEs, we provide 
//the implementation ourselves. Note that this is not a per
//thread pointer, so don't run the test steps concurrently.

static TAny* gTlsData;

TInt Dll::SetTls(TAny* aPtr)
    {
    gTlsData = aPtr;
    return KErrNone;
    }

TAny* Dll::Tls()
    {
    return gTlsData;
    }   

#endif


AnvlGlob *AnvlCreateGlobalsL(void)
    {
    AnvlGlob *anvlGlob = STATIC_CAST(AnvlGlob*, Dll::Tls());
    if (anvlGlob == NULL)
        {
        anvlGlob = new (ELeave) AnvlGlob;
        Mem::FillZ(anvlGlob, sizeof(AnvlGlob));
        Dll::SetTls(anvlGlob);
        }
    return anvlGlob;
    }

void AnvlDeleteGlobals(void)
    {
    AnvlGlob *anvlGlob = STATIC_CAST(AnvlGlob*, Dll::Tls());
    if (anvlGlob == NULL)
        return;
    
    delete anvlGlob;
    Dll::SetTls(NULL);
    }
    
AnvlGlob *AnvlGetGlobals(void)
    {
    return STATIC_CAST(AnvlGlob*, Dll::Tls());
    }
    
AnvlGlob *AnvlGetGlobalsPlusPlus(void)
    {
    return STATIC_CAST(AnvlGlob*, Dll::Tls());
    }
