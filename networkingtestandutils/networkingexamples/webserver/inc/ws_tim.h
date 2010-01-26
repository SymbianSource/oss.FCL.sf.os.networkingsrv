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
// ws_tim.h - http server timer module
// This class is used to Handle the Time Outs
//



/**
 @internalComponent
*/
#ifndef __WS_TIM_H
#define __WS_TIM_H

#include <e32base.h>
#include "ws_con.h"

class CWebServerCon;

// CTimeOutTimer: timer for comms time-outs
class CTimeOutTimer: public CTimer
	{
public:
	static CTimeOutTimer* NewL(const TInt aPriority,CWebServerCon* aWebServerConnection);
	~CTimeOutTimer();

protected:
    CTimeOutTimer(const TInt aPriority);
	void ConstructL(CWebServerCon* aWebServerConnection);
    virtual void RunL();

private:
	CWebServerCon* iWebServerConnection;
	};

#endif
