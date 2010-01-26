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
//

#include "clistatic.h"


TInt Launcher::LaunchServer(const TDesC& aServerName,
                            const TDesC& aServerFileName,
                            const TUid& aServerUid3,
                            const TUint aWinsMinHeapSize,
                            const TUint aWinsMaxHeapSize,
                            const TUint aWinsStackSize)
    {
    const TUidType serverUid(KNullUid,KNullUid,aServerUid3);
    
    //
    // EPOC and EKA2 is easy, we just create a new server process. Simultaneous
    // launching of two such processes should be detected when the second one
    // attempts to create the server object, failing with KErrAlreadyExists.
    //
    RProcess server;
    TInt r=server.Create(aServerFileName,KNullDesC,serverUid);
    (void)aServerName;
    (void)aWinsMinHeapSize;
    (void)aWinsMaxHeapSize;
    (void)aWinsStackSize;
    
    
    if (r!=KErrNone)
        return r;
    TRequestStatus stat;
    server.Rendezvous(stat);
    if (stat!=KRequestPending)
        server.Kill(0);     // abort startup
    else
        server.Resume();    // logon OK - start the server
    User::WaitForRequest(stat);     // wait for start or death
    // we can't use the 'exit reason' if the server panicked as this
    // is the panic 'reason' and may be '0' which cannot be distinguished
    // from KErrNone
    r=(server.ExitType()==EExitPanic) ? KErrGeneral : stat.Int();
    server.Close();
    return r;
    }

