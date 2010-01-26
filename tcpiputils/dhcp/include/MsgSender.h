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
// Message sender header file
// 
//

/**
 @file MsgSender.h
*/

#ifndef MSGSENDER_H
#define MSGSENDER_H

#include <e32base.h>
#include <./libc/limits.h>
#include "ExpireTimer.h"
#include "DHCP_Std.h"

class TInetAddr;
class RSocket;

const TInt KMicrosecondsInSecs = 1000000;

class MMSListener
/**
  * Just a notification interface
  *
  * @internalTechnology
  */
	{
public:
	virtual TInt MSReportError(TInt aError) = 0;
	};

class CMessageSender : public CActive, public MExpireTimer
	{
public:
	CMessageSender(MMSListener* aMMSListener, RSocket& aSocket, TTime *aTaskStartedAtCopy, const TUint aFamily);
	virtual ~CMessageSender();

	//if aRetransmitAfterMcSecs == 0 than just one transmition happens
	void SendL(TInetAddr& aAddr, const TDesC8& aMsg, TTimeIntervalMicroSeconds aRetransmitAfterSecs, TInt aRetryCount);
	void Cancel();
   
	//MExpireTimer virtuals
	virtual void TimerExpired();
	void SetTaskStartedTime(TTime aTaskStartedAtCopy);
	const TUint GetFamilyType();

protected:
	virtual void DoCancel();
	virtual void RunL();
	virtual TInt RunError(TInt aError);

protected:
   virtual TBool SendingContinues() const;
   virtual TBool CalculateDelay();

   void StartTimerL();

protected:
	HBufC8* iMsg;
	CExpireTimer* iExpireTimer;
	TInetAddr iAddr;
	RSocket& iSocket;
	TTimeIntervalMicroSeconds32 iMicroSecs;
	MMSListener* iMMSListener;
   TInt iRetryCount; //counts how many times the message's been successfully sent
   TInt iRetryDuration;
   TInt iMaxRetryCount; //counts the max times the message has to be sent
   TTime iTaskStartedAtCopy;
	const TUint iFamily;
	};

inline CMessageSender::CMessageSender(MMSListener* aMMSListener,RSocket& aSocket, TTime *aTaskStartedAtCopy, const TUint aFamily) :
      CActive(EPriorityStandard), iSocket(aSocket),iMMSListener(aMMSListener),iTaskStartedAtCopy(*aTaskStartedAtCopy),iFamily(aFamily)
      {
      }
	  
inline void CMessageSender::SetTaskStartedTime(TTime aTaskStartedAtCopy)
     {
     iTaskStartedAtCopy = aTaskStartedAtCopy;
     }

inline const TUint CMessageSender::GetFamilyType()
     {
     return iFamily;
     }

#endif
