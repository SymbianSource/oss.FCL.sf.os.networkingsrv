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
// Extensions, Version 2 (MS-CHAP-V2) - RFC 2759, except the
// authenticator-controlled authentication retry mechanisms and the
// password changing mechanisms - this is in accordance with the
// requirements.
// 
//

/**
 @file
 @brief Header file for the implementation of Microsoft PPP CHAP
 @internalComponent
*/

#ifndef __MSCHAP2_H__
#define __MSCHAP2_H__

#include "PPPCHAP.H"

// protocol specific constants for MS-CHAP-V2

/**
   The size of the MS-CHAP-V2 Authenticator Challenge.
   @internalComponent
*/
const TUint8 KPppMsChap2AuthenticatorChallengeSize 	= 16u;

/**
   The size of the MS-CHAP-V2 Response Peer-Challenge subfield.
   @internalComponent
*/
const TUint8 KPppMsChap2PeerChallengeSize = 16u;

/**
   The size of the MS-CHAP-V2 Response Reserved subfield.
   @internalComponent
*/
const TUint8 KPppMsChap2ResponseReservedSize = 8u;

/**
   The size of the MS-CHAP-V2 NT-Response subfield.
   @internalComponent
*/
const TUint8 KPppMsChap2NTResponseSize = 24u;

/**
   The size of the MS-CHAP-V2 Response Flags subfield.
   @internalComponent
*/
const TUint8 KPppMsChap2ResponseFlagsSize = 1u;

/**
   The size of the CHAP Response Value for MS-CHAP-V2.
   @internalComponent
*/
const TUint8 KPppMsChap2ResponseValueSize =
				KPppMsChap2PeerChallengeSize + 
				KPppMsChap2ResponseReservedSize +
				KPppMsChap2NTResponseSize + 
				KPppMsChap2ResponseFlagsSize;


/**
   The size of the MS-CHAP-V2 password hash.
   @internalComponent
*/
const TUint8 KPppMsChap2HashSize = 16u;

/**
   The size of the MS-CHAP padded password hash.
   @internalComponent
*/
const TUint8 KPppMsChap2PaddedHashSize = 21u;

/**
   The size of the MS-CHAP-V2 challenge hash (the hash of the peer
   challenge, authenticator challenge and username).
   @internalComponent
*/
const TUint8 KPppMsChap2ChallengeHashSize = 8u;

/**
   The size of the MS-CHAP-V2 Authenticator Response Value encoded in
   the format "S=<auth_string>" as specified in RFC 2759.
   @internalComponent
*/
const TUint8 KPppMsChap2AuthenticatorResponseSize = 42u;


/*final*/ NONSHARABLE_CLASS(CPppMsChap2) : public CPppChap
/**
   Class that implements the Microsoft PPP CHAP Extensions, Version 2
   (MS-CHAP-V2) - RFC 2759, except the authenticator-controlled
   authentication retry mechanisms and the password changing
   mechanisms - this is in accordance with the requirements.
   @internalComponent
*/
	{
  public:
	static CPppAuthentication* NewL();

	virtual ~CPppMsChap2();

	virtual void InitL(CPppLcp* aLcp);

  protected:
	virtual void CheckChallengePacketL(RMBufPacket& aPacket);

	virtual void SuccessL(RMBufPacket& aPacket);

	virtual void FailureL(RMBufPacket& aPacket);

	virtual void MakeResponseL(TUint8 aChallengeId, 
							   const TDesC8& aChallengeValue, 
							   TPtrC8& aResponseValue, 
							   TPtrC8& aResponseName);

  private:

	CPppMsChap2();

	static void GeneratePeerChallengeL(TDes8& aChallenge);

	static void GenerateNTResponseL(
					const TDesC8& aAuthenticatorChallenge, 
					const TDesC8& aPeerChallenge, 
					const TDesC8& aUserName, 
					const TDesC16& aPassword, 
					TDes8& aResponse);

	static void ChallengeHashL(const TDesC8& aPeerChallenge, 
							   const TDesC8& aAuthenticatorChallenge, 
							   const TDesC8& aUserName, 
							   TDes8& aChallengeHash);

	static void NtPasswordHashL(const TDesC16& aPassword, 
								TDes8& aPasswordHash);

	static void ChallengeResponseL(const TDesC8& aChallengeHash, 
								   TDes8& aPaddablePasswordHash, 
								   TDes8& aResponse);

	static void HashNtPasswordHashL(const TDesC8& aPasswordHash, 
									TDes8& aPasswordHashHash);

	static void GenerateAuthenticatorResponseL(
					const TDesC16& aPassword, 
					const TDesC8& aNTResponse, 
					const TDesC8& aPeerChallenge, 
					const TDesC8& aAuthenticatorChallenge, 
					const TDesC8& aUserName, 
					TDes8& aAuthenticatorResponse);

	static void DesEncryptL(const TDesC8& aClear, 
							const TDesC8& aKey,
							TDes8& aCypher);

	static void ProcessFailureMessageL(const TDesC8& aFailureMessage,
									   TUint& aErrorCode, 
									   TUint8& aRetryFlag, 
									   TDes8& aAuthChallenge,
									   TUint8& passwordProtoVersion, 
									   TPtrC8& aMessage);

	static TInt TranslateMsChapError(TUint aMsChapError);

	void RetryPasswordL();

  private:
/** Buffer containing the MS-CHAP-V2 Response Value */
	TBuf8<KPppMsChap2ResponseValueSize> iResponseValue;

/** Buffer containing the expected MS-CHAP-V2 Authenticator Response
Value encoded in the format "S=<auth_string>" as specified in RFC 2759
(42 octets). */
	TBuf8<KPppMsChap2AuthenticatorResponseSize> 
		iAuthenticatorResponse;

#ifdef _UNICODE
/* Buffer containing the username. */
	HBufC8* iUserName;
#endif
	};


inline CPppAuthentication* CPppMsChap2::NewL()
/**
   Object factory for CPppMsChap2.
   @leave Standard Symbian OS error codes. e.g. KErrNoMemory.
   @note This function will not initialize the created object.  The
   InitL member function shall be called first in order to initialize
   the created object before using it.  This behavior is in agreement
   with the requirements for a migration to a plugin architecture.
*/
	{
	return new(ELeave) CPppMsChap2;
	}

#endif //__MSCHAP2_H__
