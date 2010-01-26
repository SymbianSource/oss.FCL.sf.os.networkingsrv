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
// Implements the Message sender DHCPIP6 specific access functthat fustions 
// resends messages until max retry or max count has been reached..
// 
//

/**
 @file DHCPIP6MsgSender.cpp
 @internalTechnology
*/

#include "DHCPServer.h"
#include "DHCPIP6MsgSender.h"
#include "DhcpIP6Msg.h"
#include <in_sock.h>
#include <es_sock.h>

/*
the retransmition alg recomended by RFC3315 (DHCPv6). Could be used for DHCPv4 as well
for DHPCv4 doesn't specify anything
   RT for the first message transmission is based on IRT:

      RT = IRT + RAND*IRT

   RT for each subsequent message transmission is based on the previous
   value of RT:

      RT = 2*RTprev + RAND*RTprev

   MRT specifies an upper bound on the value of RT (disregarding the
   randomisation added by the use of RAND).  If MRT has a value of 0,
   there is no upper limit on the value of RT.  Otherwise:

      if (RT > MRT)
     RT = MRT + RAND*MRT

*/
TBool CDHCPIP6MessageSender::SendingContinues() const
/**
  * determines whether to continue the sending
  *
  * 
  * @internalTechnology
  *
  */
	{
	if ( iRetryCount )
		{//EElapsedTime option MUST be the first option in the option part of the message
		//see CDHCPIP6StateMachine::SetMessageHeader fn
		TUint32 elapsedHundrOfSecs = iMicroSecs.Int() / KMicrosecondsInSecs * 100;
		TUint8* elapsedOptionBody = const_cast<TUint8*>(iMsg->Des().Ptr()) + DHCPv6::KDHCPHeaderLength + DHCPv6::KOptionHeaderLength;
		TUint32 current = TBigEndian::GetValue( elapsedOptionBody, DHCPv6::KElapsedTimeOptionLen );
		TBigEndian::SetValue( elapsedOptionBody, DHCPv6::KElapsedTimeOptionLen, current + elapsedHundrOfSecs );
		}
	return iRetryCount < iMaxRetryCount && iRetryDuration < iMaxRetryDuration;
	}

TBool CDHCPIP6MessageSender::CalculateDelay()
/**
  * Calculates the first and the next retransmition time. Returns ETrue is the time calculated
  * is greater than zero.
  *
  * @internalTechnology
  *
  */
    {
    if ( iRetryCount > -1 )
        {
        TInt ms = iMicroSecs.Int();
        ms = iInitialRetryTimeout ? iInitialRetryTimeout : ms + 2*ms + iXid.Rnd( -ms/10, ms/10 );
      
        if ( ms > iMaxRetryTimeout || ms <= 0 )
            {
            ms = iMaxRetryTimeout + iXid.Rnd( -iMaxRetryTimeout/10, iMaxRetryTimeout/10 );
            }
        iInitialRetryTimeout = 0;
        iMicroSecs = ms;
        }
    else
        {//msg to be send for the first time
        iMicroSecs = iXid.Rnd( 0, iFirstSendDelay );
        }
   
    return iMicroSecs.Int() > 0;    
    }

void CDHCPIP6MessageSender::SetListener( MMSListener* aMMSListener )
   {
   iMMSListener = aMMSListener;
   }

MMSListener* CDHCPIP6MessageSender::EventListener() const
   {
   return iMMSListener;
   }

void CDHCPIP6MessageSender::SetInitialRetryTimeout( TInt aSeconds )
   {
   iInitialRetryTimeout = KMicrosecondsInSecs * aSeconds;
   }

void CDHCPIP6MessageSender::SetMaxRetryTimeout( TInt aSeconds )
   {
   iMaxRetryTimeout = KMicrosecondsInSecs * aSeconds;
   }

void CDHCPIP6MessageSender::SetMaxRetryCount( TInt aCount )
   {
   iMaxRetryCount = aCount;
   }

void CDHCPIP6MessageSender::SetMaxRetryDuration( TInt aSeconds )
   {
   iMaxRetryDuration = KMicrosecondsInSecs * aSeconds;
   }

void CDHCPIP6MessageSender::SetFirstSendDelay( TInt aSeconds )
   {
   iFirstSendDelay = KMicrosecondsInSecs * aSeconds;
   }
