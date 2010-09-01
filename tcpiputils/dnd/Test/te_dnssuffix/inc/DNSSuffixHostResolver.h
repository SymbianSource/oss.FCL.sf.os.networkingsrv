// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
 

#ifndef DNSSUFFIXHOSTRESOLVER_H
#define DNSSUFFIXHOSTRESOLVER_H

#include <e32base.h>	
#include <e32std.h>		
#include <es_sock.h>

#include "CallBackHandler.h"

// Forwared declarations


class CDNSSuffixHostResolver : public CActive
    {
public:
    // Cancel and destroy
    ~CDNSSuffixHostResolver();

    // Two-phased constructor.
    static CDNSSuffixHostResolver* NewL(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn);

    // Two-phased constructor.
    static CDNSSuffixHostResolver* NewLC(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn);

public:
    // New functions
    // Function for making the initial request
    TInt ResolveL(const TDesC& aHostName);

private:
    // C++ constructor
    CDNSSuffixHostResolver(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn);

    // Second-phase constructor
    void ConstructL();

private:
    // From CActive
    // Handle completion
    void RunL();

    // How to cancel me
    void DoCancel();

    // Override to handle leaves from RunL(). Default implementation causes
    // the active scheduler to panic.
    TInt RunError(TInt aError);

private:
    MCallBackHandler&   iCallBack;
    RSocketServ&    iSockServ;
    RConnection&    iConn;
    RHostResolver   iResolver;
    TNameEntry      iEntry;
    };

#endif // DNSSUFFIXHOSTRESOLVER_H
