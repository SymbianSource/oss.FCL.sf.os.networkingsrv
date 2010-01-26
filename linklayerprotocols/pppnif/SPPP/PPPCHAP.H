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
// Authentication Protocol (CHAP) - RFC 1994, containing source code
// common to all PPP authentication protocols derived from CHAP.
// 
//

/**
 @file
 @brief Header file for the implementation of PPP Challenge Handshake
 @internalComponent 
*/

#ifndef __PPPCHAP_H__
#define __PPPCHAP_H__

// use RMBufChain, RMBufPacket
#include <nifmbuf.h>
#include "PPPAUTH.H"


// protocol specific constants for CHAP

/**
   The value of the CHAP Challenge Code.
   @internalComponent
*/
const TUint8 KPppChapChallengeCode = 1u;

/**
   The value of the CHAP Response Code.
   @internalComponent
*/
const TUint8 KPppChapResponseCode = 2u;

/**
   The value of the CHAP Success Code.
   @internalComponent
*/
const TUint8 KPppChapSuccessCode = 3u;

/**
   The value of the CHAP Failure Code.
   @internalComponent
*/
const TUint8 KPppChapFailureCode = 4u;


/**
   The size of the CHAP Code field.
   @internalComponent
*/
const TUint8 KPppChapCodeFieldSize = 1u;

/**
   The size of the CHAP Identifier field.
   @internalComponent
*/
const TUint8 KPppChapIdFieldSize = 1u;

/**
   The size of the CHAP Length field.
   @internalComponent
*/
const TUint8 KPppChapLengthFieldSize = 2u;

/**
   The size of the CHAP Value-Size field.
   @internalComponent
*/
const TUint8 KPppChapValueSizeFieldSize = 1u;

/**
   The minimum size of the CHAP Value field of Challenge & Response
   packets.
   @internalComponent
*/
const TUint8 KPppChapMinValueSize = 1u;

/**
   The minimum size of the CHAP Name field of Challenge & Response
   packets.
   @note While RFC 1994 states that: "The Name field is one or more
   octets [...]", in the case of MS-CHAP-V1 and MS-CHAP-V2 the Name
   field of the Challenge packet may be empty.  RFC2433 and RFC2759
   state that: "Microsoft authenticators do not currently provide
   information in the Name field.  This may change in the future."
   @internalComponent
*/
const TUint8 KPppChapMinNameSize = 1u;


/**
   Default period of the timer used for CHAP response retries.
   @internalComponent
*/
const TInt KPppChapResponseRetryTimerPeriod = 3000;

/**
   Default number of CHAP response retries.
   @internalComponent
*/
const TInt KPppChapMaxResponseRetryCount = 4;

NONSHARABLE_CLASS(CPppChap) : public CPppAuthentication, protected MTimer
/**
   Abstract base class for classes that implement the PPP Challenge
   Handshake Authentication Protocol (CHAP) or other PPP
   authentication protocols derived from CHAP - RFC 1994.
   @internalComponent
*/
	{
  public:
	virtual ~CPppChap();

	virtual void InitL(CPppLcp* aLcp);

	virtual void AuthenticateComplete(TInt aStatus);

	virtual TBool RecvFrame(RMBufChain& aPacket);

	virtual void LowerLayerUp();

	virtual void LowerLayerDown(TInt aStatus=KErrNone);


  protected:

	virtual void TimerComplete(TInt aStatus);

	/*final*/ virtual TUint PppId() const;

	virtual void ChallengeL(RMBufPacket& aPacket);

	virtual void SuccessL(RMBufPacket& aPacket);

	virtual void FailureL(RMBufPacket& aPacket);

	virtual void CheckChallengePacketL(RMBufPacket& aPacket);


/**
   Computes the CHAP Response Value and the CHAP Response Name.
   @param aChallengeId [in] The CHAP Challenge Identifier.
   @param aChallengeValue [in] The CHAP Challenge Value.
   @param aResponseValue [out] The CHAP Response Value.
   @param aResponseName [out] The CHAP Response Name.
*/
	virtual void MakeResponseL(TUint8 aChallengeId,
							   const TDesC8& aChallengeValue,
							   TPtrC8& aResponseValue,
							   TPtrC8& aResponseName)=0;

	void ProcessChallengePacketL(RMBufPacket& aPacket);

	void RespondL();

	void RetryResponseL();

	void SendResponseL(TUint8 aResponseId, 
					   const TDesC8& aResponseValue, 
					   const TDesC8& aResponseName);

	TBool CheckIdentifier(RMBufPacket& aPacket) const;

	static void MakeResponsePacketLC(TUint8 aIdentifier, 
									 const TDesC8& aValue, 
									 const TDesC8& aName, 
									 RMBufPacket& aPacket);

  protected:
	CPppChap();
	
/** Pointer to the latest CHAP Challenge received. */
	TPtr8 iChallengeRef;

/** The response retry count. */
	TInt iResponseRetryCount;

/** The latest CHAP Challenge Identifier received. */
	TUint8	iCurrentId;

  private:

/** Pointer to the CHAP Response Value. */
	TPtrC8 iResponseValueRef;

/** Pointer to the CHAP Response Name. */
	TPtrC8 iResponseNameRef;

/** The latest CHAP Challenge packet received. */
	RMBufChain iChallengePacket;
	};


inline TBool CPppChap::CheckIdentifier(RMBufPacket& aPacket) const
/**
   Verifies that the identifier contained in the packet matches the
   latest Challenge Identifier received.
   @param aPacket [in] A CHAP Success or Failure packet.
*/
	{
	return *(aPacket.First()->Ptr() + KPppChapCodeFieldSize) ==
				iCurrentId;
	}

inline TUint CPppChap::PppId() const
/**
   @copydoc CPppAuthentication::PppId()
   @see CPppAuthentication::PppId()
*/
	{
	return KPppIdChap;
	}

inline CPppChap::CPppChap()
	: iChallengeRef(NULL, 0)
	{
	}

#endif // ___PPPCHAP_H__
