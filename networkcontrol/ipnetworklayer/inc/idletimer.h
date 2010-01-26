/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



#ifndef IDLETIMERS_H_
#define IDLETIMERS_H_

#include <e32base.h>
#include <comms-infras/metadata.h>

static const TInt KTimerTick = 1000000;
static const TInt KTimerCorrectionPeriod = 4;
/**
 *Don't ever want to set a timer for much less than 1/5 of a second
 */
static const TInt KMinTimerTick = KTimerTick / 5;

enum 
    {
    EPacketActivity,
    EIdleTimerValues
    };

class TPacketActivity : public Meta::SMetaData
    {
public:    
    enum 
        { 
        iId = EPacketActivity,
        iUid = 0x10281DFD //same as the IdleTimers.dll
        };

    TPacketActivity(TBool* aPacketActivity)
    	:iPacketActivity(aPacketActivity) {}

	volatile TBool* iPacketActivity;
    DATA_VTABLE
    };
    

class TIdleTimerValues : public Meta::SMetaData
    {
public:    
    enum 
        { 
        iId = EIdleTimerValues,
        iUid = 0x10281DFD //same as the IdleTimers.dll
        };

    TUint32 iShortTimer;
    TUint32 iMediumTimer;
    TUint32 iLongTimer;
    
    DATA_VTABLE
    };
	

#endif //IDLETIMERS_H_
