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
// ws_tim.cpp - http server timer module
// This class is used to Handle the Time Outs
//

#include "ws_tim.h"

CTimeOutTimer::CTimeOutTimer(const TInt aPriority)
    : CTimer(aPriority)
    {
    }

CTimeOutTimer::~CTimeOutTimer()
    {
	Cancel();
    }

CTimeOutTimer* CTimeOutTimer::NewL(const TInt aPriority, CWebServerCon* aWebServerConnection)
    {
    CTimeOutTimer *p = new (ELeave) CTimeOutTimer(aPriority);
    CleanupStack::PushL(p);
	p->ConstructL(aWebServerConnection);
	CleanupStack::Pop();
    return p;
    }

void CTimeOutTimer::ConstructL(CWebServerCon* aWebServerConnection)
    {
	iWebServerConnection=aWebServerConnection;
	CTimer::ConstructL();
	CActiveScheduler::Add(this);
    }

void CTimeOutTimer::RunL()
// Timer request has completed, so notify the timer's owner
    {
	iWebServerConnection->ConnectionTimeOutL();
	}
