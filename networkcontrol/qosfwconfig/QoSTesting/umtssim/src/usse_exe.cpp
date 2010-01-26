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
// usse_exe.cpp -- wrapper for starting server process
//

#include <e32base.h>
#include "usse_server.h"

GLDEF_C TInt E32Main()
    {
    return CUmtsSimServServer::ThreadFunction(NULL);
    }
