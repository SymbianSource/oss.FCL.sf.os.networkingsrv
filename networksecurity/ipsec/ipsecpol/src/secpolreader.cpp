// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// secpolreader.cpp - IPSec event message reader
//

#include <es_sock.h>
#include <in_sock.h>

#include "secpolreader.h"
#include "ipsecpolmanhandler.h"

//
// CSecpolReader::CSecpolReader
//
CSecpolReader::CSecpolReader(CIPSecPolicyManagerHandler *control)
: CActive(1), iControl(control)
    {
    CActiveScheduler::Add(this);
    }

//
// CSecpolReader::~CSecpolReader
//
CSecpolReader::~CSecpolReader()
    {
    if (IsActive())
        Cancel();
    iSocket.Close();
    }

//
// CSecpolReader::Construct()
//  Open and activate the socket input
//
TInt CSecpolReader::Construct(const TDesC &aSocket)
    {
    TInt err;
    TInetAddr addr;
    
    err = iSocket.Open(iControl->iSS,aSocket);
    if( err != KErrNone )
	return err;
    addr.SetAddress(KInetAddrNone);
    addr.SetPort(0);
    err = iSocket.Bind(addr);
    if( err != KErrNone )
	return err;
    iSocket.LocalName(addr);
    iSocket.RecvFrom(iMsg, iAddr, 0, iStatus);
    SetActive();
    return err;
    }

//
// CSecpolReader::RunL
//  Called when request completed
//
void CSecpolReader::RunL()
    {
    //    DEB(TBuf<1000> str;)
    //    DEB(TBuf<40> buf;)
    //  TPayload packet(iMsg.Ptr());
    iSocket.RecvFrom(iMsg, iAddr, 0, iStatus);  // start a new read
    SetActive();
    }

//
// CSecpolReader::DoCancel
//  Called when a pending request should be cancelled
//
void CSecpolReader::DoCancel()
    {
    iSocket.CancelRecv();
    }

//
// CSecpolReader::RunError
// Called when RunL() leaves 
//
TInt CSecpolReader::RunError(TInt /*aError*/)
    {
    iSocket.RecvFrom(iMsg, iAddr, 0, iStatus);  // start a new read 
    SetActive();
    
    return KErrNone; // Active scheduler Error() method NOT called              
    }
