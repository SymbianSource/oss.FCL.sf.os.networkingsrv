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
 

#include "DNSSuffixHostResolver.h"


#include <in_sock.h>


CDNSSuffixHostResolver::CDNSSuffixHostResolver(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn) :
    CActive(EPriorityStandard), iCallBack(aCallback),iSockServ(aSockServ), iConn(aConn)
    {
    }

CDNSSuffixHostResolver* CDNSSuffixHostResolver::NewLC(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn)
    {
    CDNSSuffixHostResolver* self = new (ELeave) CDNSSuffixHostResolver(aCallback,aSockServ,aConn);
    CleanupStack::PushL(self);
    self->ConstructL();
    return self;
    }

CDNSSuffixHostResolver* CDNSSuffixHostResolver::NewL(MCallBackHandler& aCallback,RSocketServ& aSockServ,RConnection& aConn)
    {
    CDNSSuffixHostResolver* self = CDNSSuffixHostResolver::NewLC(aCallback,aSockServ,aConn);
    CleanupStack::Pop(); // self;
    return self;
    }

void CDNSSuffixHostResolver::ConstructL()
    {
    CActiveScheduler::Add(this); // Add to scheduler
    
    if (iConn.SubSessionHandle() != 0)
        {
        // Explicit resolver
        iResolver.Open(iSockServ,KAfInet, KProtocolInetUdp, iConn);    
        }
    else
        {
        // Implicit resolver
        iResolver.Open(iSockServ,KAfInet, KProtocolInetUdp);
        }    
    }


CDNSSuffixHostResolver::~CDNSSuffixHostResolver()
    {
    Cancel(); // Cancel any request, if outstanding
    
    if (iResolver.SubSessionHandle())
        iResolver.Close();
    }

void CDNSSuffixHostResolver::DoCancel()
    {
    iResolver.Cancel();
    }

TInt CDNSSuffixHostResolver::ResolveL(const TDesC& aHostName)
    { 
    iResolver.GetByName(aHostName,iEntry,iStatus);
    SetActive();
    return KErrNone;
    }


void CDNSSuffixHostResolver::RunL()
    {
    if (&iCallBack)
        iCallBack.HandleCallBackL(iStatus.Int());
    }


TInt CDNSSuffixHostResolver::RunError(TInt aError)
    {
    return aError;
    }

// End of file
