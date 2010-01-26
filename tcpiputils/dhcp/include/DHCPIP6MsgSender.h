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
// DHCP IP6 Message sender header file
// 
//

/**
 @file DHCPIP6MsgSender.h
*/

#ifndef DHCPIP6MSGSENDER_H
#define DHCPIP6MSGSENDER_H

#include "MsgSender.h"

const TInt KIP6RndMin = -KMicrosecondsInSecs / 1000;
const TInt KIP6RndMax = KMicrosecondsInSecs / 1000;
const TInt KIP6MaxSecs = 0xFF;

class CDHCPIP6MessageSender : public CMessageSender
	{
public:
	CDHCPIP6MessageSender( MMSListener* aMMSListener, RSocket& aSocket, TTime *aTaskStart, const TUint aFamily);

   void SetListener( MMSListener* aMMSListener );
   MMSListener* EventListener() const;
   void SetMaxRetryTimeout( TInt aSeconds );
   void SetMaxRetryCount( TInt aCount );
   void SetMaxRetryDuration( TInt aSeconds );
   void SetInitialRetryTimeout( TInt aSeconds );
   void SetFirstSendDelay( TInt aSeconds );

protected:
   virtual TBool SendingContinues() const;
   virtual TBool CalculateDelay();

protected:
   TDhcpRnd iXid;
   //the following values are stored in microsecods
   TInt iInitialRetryTimeout;  //initial timeout between two subsequent retransmission
   TInt iMaxRetryTimeout;  //max timeout between two subsequent retransmission
   TInt iMaxRetryCount;    //upper limit of the number of times to retransmit the message
   TInt iMaxRetryDuration; //upper limit of the time to retransmit the message
                           //if iMaxRetryDuration == MAX_INT than the retransmission ends
                           //when RT > MRT || RC > MRC
   TInt iFirstSendDelay;
	};

inline CDHCPIP6MessageSender::CDHCPIP6MessageSender( MMSListener* aMMSListener, RSocket& aSocket, TTime *aTaskStartedAtCopy, const TUint aFamily) :
      CMessageSender( aMMSListener, aSocket, aTaskStartedAtCopy, aFamily ),
      iMaxRetryTimeout( static_cast<TInt>(KWaitForResponseTime) ),
      iMaxRetryCount( INT_MAX ),
      iMaxRetryDuration( INT_MAX )
      {
      }

#endif
