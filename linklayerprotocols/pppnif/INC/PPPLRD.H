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

/**
 @file
 @internalComponent 
*/

#if !defined(__PPPLRD_H__)
#define __PPPLRD_H__

#include <nifutl.h>

//SPECS//
// The specs I got for LRD are:
// · LCP echo/reply packet are used to ensure the link quality.
// The link will be closed if 4 out of 5 echo reply pairs fail 
// i.e. server does not answer. LCP echo packet is sent after 
// every 10 seconds.
// So we just create a timer with a 10 secs period.
// On every tick 
// - Check it got an answer to the previous echo
// - If the link is open send another echo. 


//
// lrd functionnality can be tweaked through ppp.ini
// entries are:
// [lrd]
// PPPEnableLRD = 0:disabled/1:enabled
// PPPLRDPeriod = (a value between min period and max period)
_LIT(LRDSECTIONNAME,"lrd");
_LIT(LRDENTRYNAME_ENABLE,"PPPEnableLRD");
const TInt KPppLrdDefaultEnabledMode = 0;

_LIT(LRDENTRYNAME_PERIOD,"PPPLRDPeriod");
const TInt KPppLrdTimerPriority = 10;
const TInt KPppLrdDefaultTimerPeriod = 10; // 10 secs by default
const TInt KPppLrdTimerPeriodMax = 60; // 60 secs 
// Why would anybody want more than 60?
const TInt KPppLrdTimerPeriodMin = 03; // really small period 
// stops the ppp stack from working

// Nile spec says:
// Fails if 4 out of 5 (most recent) tries fail
// the way we detect that is by keeping track of our tries
// in a shift register associated with a counter 
// which keeps track of the number of bits in it.
// On each period, shift the register left (update bit counter)
// set the rightmost bit if we got an echo reply (update bit counter)
// Check the bit counter and see it has not dropped below KMaxNFailure
const TUint KSampleWidth = 5;
const TUint	KMaxNFailure = 4;
const TUint KShiftRegisterWidth = (1<<KSampleWidth)-1;
const TUint KBitOverflow = 1<<KSampleWidth;
class CPppLcp;
NONSHARABLE_CLASS(CPppLrd) : public MTimer 
	{
public:
	CPppLrd(CPppLcp*);
	virtual ~CPppLrd();
	static CPppLrd* NewL(CPppLcp*);
	void ConstructL();
	void StartTimer();
	void StopTimer();
	void ResetTimer();
	void RecvEchoReply(TUint8);
	void EnableOrDisableTimer(TBool aEnable);
private:
// MTimer upcall
	void TimerComplete(TInt aStatus);
	void ReadIniFileL();
	TBool	iPppLrdEnabled;
	TBool	iLastEchoAck;
	TUint8	iEchoReqFrameIdentifier;
	TInt	iLrdPeriod;
	TUint	iBitCount;
	TUint	iShiftRegister;
	CPppLcp*	iPppLcp;
	};

#endif
