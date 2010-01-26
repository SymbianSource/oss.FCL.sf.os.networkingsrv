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
// Implements the Message sender that just
// resends messages until  max retry or max count has been reached
// max retry and max count are constants
// 
//

/**
 @file MsgSender.cpp
 @internalTechnology
*/

#include "DHCP_Std.h"
#include "DHCPServer.h"
#include "MsgSender.h"
#include <in_sock.h>
#include <es_sock.h>
#include "DHCPAuthentication.h"
#include "DHCPIP4StateMachine.h"
CMessageSender::~CMessageSender()
/**
  * Destructor for message sender
  *
  * @internalTechnology
  */
	{
	Cancel();
	delete iMsg;
	delete iExpireTimer;
	}

void CMessageSender::DoCancel()
/**
  * Specific cancel implementation for message sender
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CMessageSender::DoCancel")));
	iSocket.CancelSend();
	}

void CMessageSender::Cancel()
/**
  * Specific cancel implementation overriding CActive's implemention
  *
  * @internalTechnology
  */
	{
	if (iExpireTimer)
		{
		iExpireTimer->Cancel();
		}
	CActive::Cancel();
	if (IsAdded())
		{
		Deque();
		}
	}

void CMessageSender::TimerExpired()
/**
  * Ticking timer expired so resend the message
  *
  * @internalTechnology
  */
	{
    iRetryCount++;
    iRetryDuration += iMicroSecs.Int();
	
	TTime now;
	TTimeIntervalSeconds tempSecs;
	now.HomeTime(); 
	(void)now.SecondsFrom(iTaskStartedAtCopy, tempSecs);
	TUint16 time =static_cast<TUint16>(tempSecs.Int());

	if(CMessageSender::GetFamilyType() == KAfInet)
		{
		TUint8* elapsedOptionBody = const_cast<TUint8*>(iMsg->Des().Ptr()) + DHCPv4::KLengthUptoSecs;
		TBigEndian::SetValue( elapsedOptionBody, DHCPv4::KSecsElapsedLength, time );
		}
		
    if ( SendingContinues() )
        {
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CMessageSender::TimerExpired - SendTo")));
    	iSocket.SendTo(*iMsg, iAddr, 0, iStatus);
	    SetActive();
        }
    else
        {
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CMessageSender::TimerExpired - RunError")));
        RunError(KErrTimedOut);
        }
	}

void CMessageSender::SendL(TInetAddr& aAddr, const TDesC8& aMsg, TTimeIntervalMicroSeconds aSecs, TInt aRetryCount)
/**
  * Copy message and send the first time
  *
  * @internalTechnology
  */
	{
	iMaxRetryCount = aRetryCount;
   iRetryCount = -1; //the very first send doesn't count as a retry
   iRetryDuration = 0;
	iAddr = aAddr;
	if (!iMsg || iMsg->Des().MaxLength() < aMsg.Length())
		{
		delete iMsg;
		iMsg = NULL;
		iMsg = HBufC8::NewL(aMsg.Length());
		}
	iMsg->Des().Copy(aMsg);
	ASSERT(!IsAdded());
	CActiveScheduler::Add(this);

   	iMicroSecs = aSecs.Int64();
   if ( CalculateDelay() )
      {
      StartTimerL();
      }
   else
      {
      TimerExpired();
      }
	}

void CMessageSender::StartTimerL()
   {
	if (!iExpireTimer)
		{
		iExpireTimer = CExpireTimer::NewL();
		}
	iExpireTimer->After(iMicroSecs, *this);
   }

TBool CMessageSender::SendingContinues() const
   {
   
   if(iRetryCount == iMaxRetryCount /*DHCPv4::KReqMaxRetry*/)
   	{
   	return EFalse;
   	}
#ifdef _DEBUG
   TTimeIntervalMicroSeconds32 msec = KWaitForResponseTime * KMicrosecondsInSecs;
   if (CDHCPServer::DebugFlags() & KDHCP_SetShortRetryTimeOut)
      {
      msec = KDHCP_ShortRetryTimeOut * KMicrosecondsInSecs;
      }

   return iMicroSecs.Int()<=msec.Int();
#else
   return iMicroSecs.Int()<=static_cast<TInt>(KWaitForResponseTime * KMicrosecondsInSecs);
#endif
   }

TBool CMessageSender::CalculateDelay()
   {
   if ( iRetryCount > -1 )
      {// exponential backoff algorithm
	   //iMicroSecs = iMicroSecs.Int()*2;	
	   	iMicroSecs = iMicroSecs.Int() * 2;
      }
    return iRetryCount > -1;
   }

void CMessageSender::RunL()
/**
  * RunL called when the message has been sent. It checks the retransmition conditions
  * and either ends with timeout or re-sets the timer accordingly.
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CMessageSender::RunL - %d"), iStatus.Int()));
	if ( !iMMSListener || iStatus.Int() == KErrNone 
			|| iMMSListener->MSReportError(iStatus.Int()) == KErrNone )
		{
        CalculateDelay();
        StartTimerL();
		}
	else
	    {
	    User::Leave(iStatus.Int());    
	    }
	}

TInt CMessageSender::RunError(TInt aError)
/**
  * RunError performs some cleanup after RunL leaves
  *
  * @internalTechnology
  */
	{
	if (iMMSListener)
		{
		if ( (aError = iMMSListener->MSReportError(aError)) == KErrNone )
         {//continue, no check for max values we assume that the listener knows what
         //it's doing
         ASSERT( iExpireTimer );
         ++iRetryCount;
         CalculateDelay();
			iExpireTimer->After(iMicroSecs, *this);
         }
		}
	return KErrNone;
	}

