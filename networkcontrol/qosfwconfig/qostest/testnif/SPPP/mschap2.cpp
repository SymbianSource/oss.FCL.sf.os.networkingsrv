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
// Extensions, Version 2 (MS-CHAP-V2) - RFC 2759, except the
// authenticator-controlled authentication retry mechanisms and the
// password changing mechanisms - this is in accordance with the
// requirements.
// 
//

/**
 @file
 @brief Source file for the implementation of Microsoft PPP CHAP
 @internalComponent 
*/

#include "mschap2.h"
#include "MSCHAP.H"

// using SHA-1
#include <hash.h>

// using DES
#include <symmetric.h>

// using random
#include <random.h>
// using MD4
#include "MD4.H"


CPppMsChap2::CPppMsChap2()
/**
   Constructor.
   @internalComponent
*/
	: iResponseValue(KPppMsChap2ResponseValueSize),
	iAuthenticatorResponse(KPppMsChap2AuthenticatorResponseSize)
	{
	}


CPppMsChap2::~CPppMsChap2()
/**
   Destructor.
   @internalComponent
*/
	{
#ifdef _UNICODE
	delete iUserName;
#endif
	}

void CPppMsChap2::InitL(CPppLcp* aLcp)
/**
   @copydoc CPppChap::InitL(CPppLcp*)
   @see CPppChap::InitL(CPppLcp*)
   @internalComponent
*/
	{
	CPppChap::InitL(aLcp);
	}


void CPppMsChap2::CheckChallengePacketL(RMBufPacket& aPacket)
/**
   @copydoc CPppChap::CheckChallengePacketL(RMBufPacket&)
   @copydoc CPppChap::CheckChallengePacketL(RMBufPacket&)
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
				KPppChapIdFieldSize + 
				KPppChapLengthFieldSize +
				KPppChapValueSizeFieldSize + 
				KPppMsChap2AuthenticatorChallengeSize,
			User::Leave(KErrUnderflow));

	__ASSERT_ALWAYS(*(aPacket.First()->Ptr() +
							KPppChapCodeFieldSize +
							KPppChapIdFieldSize +
							KPppChapLengthFieldSize) == 
						KPppMsChap2AuthenticatorChallengeSize,
				User::Leave(KErrOverflow));
	}


void CPppMsChap2::MakeResponseL(TUint8 /*aChallengeId*/, 
				const TDesC8& aChallengeValue, 
				TPtrC8& aResponseValue, 
				TPtrC8& aResponseName)
/**
   @copydoc CPppChap::MakeResponseL(TUint8,const TDesC8&,TPtrC8&,TPtrC8&)
   @see CPppChap::MakeResponseL(TUint8,const TDesC8&,TPtrC8&,TPtrC8&)
   @internalComponent
*/
	{
	ASSERT(aChallengeValue.Length() ==
			KPppMsChap2AuthenticatorChallengeSize);

	TPtr8 peerChallenge(const_cast<TUint8*>(iResponseValue.Ptr()),
				KPppMsChap2PeerChallengeSize,
				KPppMsChap2PeerChallengeSize);
	GeneratePeerChallengeL(peerChallenge);

// The NT username shall be char and not be longer than
// KPppMsChapMaxNTUserNameLength
	const TDesC& username = iPppLcp->UserName();
	__ASSERT_ALWAYS(username.Length() <=
				KPppMsChapMaxNTUserNameLength,
			User::Leave(KErrTooBig));

// The NT password shall be Unicode and not be longer than
// KPppMsChapMaxNTPasswordLength
	const TDesC& password = iPppLcp->PassWord();
	__ASSERT_ALWAYS(password.Length() <=
				KPppMsChapMaxNTPasswordLength,
			User::Leave(KErrTooBig));

#ifdef _UNICODE
// The MS-CHAP-V2 routines require the username to be represented as
// 0-to-256-char (RFC 2759), so convert the username.
	delete iUserName;
	iUserName = 0;
	iUserName = HBufC8::NewL(username.Length());
	iUserName->Des().Copy(username);
	aResponseName.Set(*iUserName);

	TPtrC16 uniPassword(password);
#else //! _UNICODE
	aResponseName.Set(username);

// The MS-CHAP-V2 routines require the password to be represented as
// 0-to-256-unicode-char (RFC 2759), so convert the password to
// Unicode.
	HBufC16& uniPassword = *HBufC16::NewLC(password.Length());
	uniPassword.Des().Copy(password);
#endif //! _UNICODE

	TPtr8 ntResponse(const_cast<TUint8*>(iResponseValue.Ptr()) +
				KPppMsChap2PeerChallengeSize +
				KPppMsChap2ResponseReservedSize,
			KPppMsChap2NTResponseSize,
			KPppMsChap2NTResponseSize);

	GenerateNTResponseL(aChallengeValue, 
			peerChallenge, 
			aResponseName,
			uniPassword, 
			ntResponse);

	aResponseValue.Set(iResponseValue);

	GenerateAuthenticatorResponseL(uniPassword, 
					ntResponse,
					peerChallenge, 
					aChallengeValue,
					aResponseName,
					iAuthenticatorResponse);

#ifndef _UNICODE
	CleanupStack::PopAndDestroy(&uniPassword);
#endif //! _UNICODE

	ASSERT(aResponseValue.Length() == KPppMsChap2ResponseValueSize);
	ASSERT(aResponseName.Length() >= KPppChapMinNameSize &&
		   aResponseName.Length() <= KPppMsChapMaxNTUserNameLength);
	}


void CPppMsChap2::SuccessL(RMBufPacket& aPacket)
/**
   @copydoc CPppChap::SuccessL(RMBufPacket&)
   @see CPppChap::SuccessL(RMBufPacket&)
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
				KPppChapIdFieldSize + 
				KPppChapLengthFieldSize +
				KPppMsChap2AuthenticatorResponseSize, 
			User::Leave(KErrUnderflow));

// check the id
	if (!CheckIdentifier(aPacket))
		User::Leave(KErrGeneral);

// no more retries
	TimerCancel();

	// Read the length of the MS-CHAP-V2 Failure packet and compute
	// the length of the CHAP Message field and go past the CHAP Code
	// field, the CHAP Identifier field and the CHAP Length field, in
	// order to read the CHAP Message field.

	TPtrC8 authResponse(aPacket.First()->Ptr() + 
					KPppChapCodeFieldSize + 
					KPppChapIdFieldSize + 
					KPppChapLengthFieldSize,
			KPppMsChap2AuthenticatorResponseSize);

// 	TPtrC8 message(ptr + KPppChapCodeFieldSize + KPppChapIdFieldSize + KPppChapLengthFieldSize + KPppMsChap2AuthenticatorResponseSize + 3, BigEndian::Get16(ptr + KPppChapCodeFieldSize + KPppChapIdFieldSize) - KPppChapCodeFieldSize - KPppChapIdFieldSize - KPppChapLengthFieldSize - KPppMsChap2AuthenticatorResponseSize - 3);

	if (authResponse!=iAuthenticatorResponse)
		DoFail(KErrIfAuthenticationFailure);
	else
		DoSucceed();
	}


void CPppMsChap2::FailureL(RMBufPacket& aPacket)
/**
   @copydoc CPppChap::FailureL(RMBufPacket&)
   @see CPppChap::FailureL(RMBufPacket&)
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
				KPppChapIdFieldSize + 
				KPppChapLengthFieldSize +
				KPppMsChap2AuthenticatorChallengeSize*2
				+ 2,
			User::Leave(KErrUnderflow));

// check the id
	if (!CheckIdentifier(aPacket))
		User::Leave(KErrGeneral);

	TimerCancel();

#ifndef _DEBUG

// The authenticator-controlled authentication retry mechanisms and
// the password changing mechanisms have not been implemented in this
// release - this is in accordance with the project requirements.
// Consequently simply fail.

	DoFail(KErrIfAuthenticationFailure);

#else // _DEBUG

// Read the length of the MS-CHAP-V2 Failure packet and compute the
// length of the CHAP Message field and go past the CHAP Code field,
// the CHAP Identifier field and the CHAP Length field, in order to
// read the CHAP Message field
	TPtrC8 failureMessage(aPacket.First()->Ptr() +
					KPppChapCodeFieldSize +
					KPppChapIdFieldSize +
					KPppChapLengthFieldSize, 
				aPacket.Length() - 
					KPppChapCodeFieldSize -
					KPppChapIdFieldSize -
					KPppChapLengthFieldSize);

	if (failureMessage.Length()==0)
		{
		DoFail(KErrIfAuthenticationFailure);
		return;
		}

	TUint msChapError;
	TUint8 isRetryAllowed;
	TUint8 passwordProtoVersion;
	TPtrC8 message;
	TInt sysError=KErrIfAuthenticationFailure;

	ProcessFailureMessageL(failureMessage, 
				msChapError,
				isRetryAllowed, 
				iChallengeRef,
				passwordProtoVersion, 
				message);

	sysError=TranslateMsChapError(msChapError);

// The code only handles KPppMsChapAuthenticationFailure, and no other
// MS-CHAP specific errors.  In particular, this code does not handle
// KPppMsChapErrorPasswordExpired, since the password changing
// mechanisms have not been implemented in this release - this is in
// accordance with the project requirements.
	if (msChapError != KPppMsChapAuthenticationFailure)
		{
		DoFail(sysError);
		return;
		}

	if (!isRetryAllowed)
		{
		DoFail(sysError);
		return;
		}

// The authenticator-controlled authentication retry mechanisms and
// the password changing mechanisms have not been implemented in this
// release - this is in accordance with the project requirements.
// Consequently simply fail.
	DoFail(sysError);

#endif // _DEBUG
	}


inline void CPppMsChap2::ProcessFailureMessageL(
				const TDesC8& aFailureMessage, 
				TUint& aErrorCode, 
				TUint8& aRetryFlag, 
				TDes8& aAuthChallenge, 
				TUint8& aPasswordProtoVersion, 
				TPtrC8& aMessage)
/**
   Processes a MS-CHAP-V2 Failure Message.
   @param aFailureMessage [in] A MS-CHAP-V2 Failure Message.  The
   Failure Message needs to be in the format specified in RFC 2759:
   "E=eeeeeeeeee R=r C=cccccccccccccccccccccccccccccccc V=vvvvvvvvvv
   M=<msg>".
   @param aErrorCode [out] The MS-CHAP-V2 Failure error code.
   @param aRetryFlag [out] The retry flag.  The flag will be set to
   "1" if a retry is allowed, and "0" if not.  When the authenticator
   sets this flag to "1" it disables short timeouts, expecting the
   peer to prompt the user for new credentials and resubmit the
   response.
   @param aAuthChallenge [out] The new Authenticator Challenge Value.
   @param aPasswordProtoVersion [out] The password changing protocol
   supported by the peer.
   @param aMessage [out] A failure text message.
   @internalComponent
*/
	{
	ASSERT(aAuthChallenge.Length() ==
			KPppMsChap2AuthenticatorChallengeSize);
	
	TLex8 input(aFailureMessage);

	if (input.Get() != 'E')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);


// RFC 2759: ""eeeeeeeeee" is the ASCII representation of a decimal
// error code (need not be 10 digits) corresponding to one of those
// listed below, though implementations should deal with codes not on
// this list gracefully."

	
	TInt ret;
	if ((ret = input.Val(aErrorCode))!=KErrNone)
		if (ret!= KErrOverflow)
			User::Leave(KErrGeneral);
		else
// Gracefully handle unusually large, yet valid, MS-CHAP-V2 specific
// error code values.  This code only handles the MS-CHAP-V2 specific
// error code values specified in RFC 2759.
			aErrorCode=0;

	input.SkipSpace();

	if (input.Get() != 'R')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);

	if (input.Val(aRetryFlag, EDecimal)!=KErrNone)
		User::Leave(KErrGeneral);

	input.SkipSpace();

	if (input.Get() != 'C')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);

	TPtrC8 token(input.NextToken());
// This field is 32 hexadecimal digits representing an ASCII
// representation of a new challenge value.  Each octet is represented
// in 2 hexadecimal digits.
	if (token.Length() != KPppMsChap2AuthenticatorChallengeSize*2)
		User::Leave(KErrGeneral);

	TLex8 lex;
	TUint8 octet;
	TUint8* pChallengeOctet = 
			const_cast<TUint8*>(aAuthChallenge.Ptr());
	TUint8 i = 0;
	do
		{
		lex.Assign(token.Mid(i*2, 2));
		if (lex.Val(octet, EHex) != KErrNone)
			User::Leave(KErrGeneral);

		*(pChallengeOctet + i) = octet;
		} 
	while (++i < KPppMsChap2AuthenticatorChallengeSize);

	input.SkipSpace();

	if (input.Get() != 'V')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);


// RFC 2759: "The "vvvvvvvvvv" is the ASCII representation of a
// decimal version code (need not be 10 digits) indicating the
// password changing protocol version supported on the server.  For
// MS-CHAP-V2, this value SHOULD always be 3."


	if ((ret = input.Val(aPasswordProtoVersion, EDecimal)) != 
			KErrNone)
		if (ret != KErrOverflow)
			User::Leave(KErrGeneral);
		else
// Gracefully handle unusually large, yet valid, password changing
// protocol version values.  This code only handles the password
// changing protocol version values specified in RFC 2759.
			aPasswordProtoVersion=0;

	input.SkipSpace();

	switch (input.Get())
		{
	case 'M':
		if (input.Get() != '=')
			User::Leave(KErrGeneral);

		aMessage.Set(input.NextToken());
		break;

	case 0:
		break;

	default:
		User::Leave(KErrGeneral);
		}

	ASSERT(aAuthChallenge.Length() ==
		   KPppMsChap2AuthenticatorChallengeSize);
	}


TInt CPppMsChap2::TranslateMsChapError(TUint aMsChapError)
/**
   Translates a MS-CHAP-V2 error code into a Symbian OS PPP NIF
   specific error code.
   @param aMsChapError A MS-CHAP-V2 error code.
   @return The Symbian OS PPP NIF specific error code corresponding to
   the MS-CHAP error code.
   @internalComponent
*/
	{
	return CPppMsChap::TranslateMsChapError(aMsChapError);
	}


inline void CPppMsChap2::RetryPasswordL()
/**
   Retries the authentication.
   @internalComponent
*/
	{
	++iCurrentId;

	iResponseRetryCount = 0;
	RespondL();
	}


inline void CPppMsChap2::GeneratePeerChallengeL(TDes8& aChallenge)
/**
   Generates a MS-CHAP-V2 Peer Challenge.
   @param aChallenge [out] A MS-CHAP-V2 Peer Challenge (16 octets).
   @internalComponent
*/
	{
	TRandom::Random(aChallenge);
	}

inline void CPppMsChap2::GenerateNTResponseL(
				const TDesC8& aAuthenticatorChallenge,
				const TDesC8& aPeerChallenge, 
				const TDesC8& aUserName, 
				const TDesC16& aPassword, 
				TDes8& aResponse)
/**
   Generates a MS-CHAP-V2 NT-Response.
   @param aAuthenticatorChallenge [in] The MS-CHAP-V2 authenticator
   challenge (16 octets).
   @param aPeerChallenge [in] The MS-CHAP-V2 peer challenge (16
   octets).
   @param aUserName [in] The Microsoft Windows NT username (0 to 256
   char).
   @param aPassword [in] The Microsoft Windows NT password (0 to 256
   unicode char).
   @param aResponse [out] The MS-CHAP-V2 Challenge Response,
   NT-Response (24 octets).
   @note This function implements the GenerateNTResponse routine
   specified in RFC 2759.
   @internalComponent
*/
	{
	ASSERT(aAuthenticatorChallenge.Length() ==
		KPppMsChap2AuthenticatorChallengeSize);
	ASSERT(aPeerChallenge.Length() ==
		KPppMsChap2PeerChallengeSize);
	ASSERT(aUserName.Length() <= KPppMsChapMaxNTUserNameLength);
	ASSERT(aPassword.Length() <= KPppMsChapMaxNTPasswordLength);
	ASSERT(aResponse.Length() == KPppMsChap2NTResponseSize);

	HBufC8* challengeHashBuf =
			HBufC8::NewMaxLC(KPppMsChap2ChallengeHashSize);
	TPtr8 challengeHash(challengeHashBuf->Des());
	ChallengeHashL(aPeerChallenge,
			aAuthenticatorChallenge, 
			aUserName,
			challengeHash);

	HBufC8* paddedPasswordHashBuf =
			HBufC8::NewLC(KPppMsChap2PaddedHashSize);
	TPtr8 paddablePasswordHash(paddedPasswordHashBuf->Des());

	paddablePasswordHash.SetLength(KPppMsChap2HashSize);
	NtPasswordHashL(aPassword, paddablePasswordHash);

	ChallengeResponseL(challengeHash, 
			paddablePasswordHash,
			aResponse);

	CleanupStack::PopAndDestroy(paddedPasswordHashBuf);
	CleanupStack::PopAndDestroy(challengeHashBuf);

	ASSERT(aResponse.Length()==KPppMsChap2NTResponseSize);
	}


void CPppMsChap2::ChallengeHashL(const TDesC8& aPeerChallenge, 
				const TDesC8& aAuthenticatorChallenge,
				const TDesC8& aUserName, 
				TDes8& aChallengeHash)
/**
   Computes the hash of the Peer Challenge, Authenticator Challenge
   and username using SHA-1.
   @param aPeerChallenge [in] The Peer Challenge (16 octets).
   @param aAuthenticatorChallenge [in] The Authenticator Challenge (16
   octets).
   @param aUserName [in] The Microsoft Windows NT username (0 to 256
   char).
   @param aChallengeHash [out] The hash of the peer challenge,
   authenticator challenge and username, computed using SHA-1 (8
   octets).
   @note This function implements the ChallengeHash routine specified
   in RFC 2759.
   @internalComponent
*/
	{
	ASSERT(aPeerChallenge.Length() ==
		KPppMsChap2PeerChallengeSize);
	ASSERT(aAuthenticatorChallenge.Length() ==
		   KPppMsChap2AuthenticatorChallengeSize);
	ASSERT(aUserName.Length() <= KPppMsChapMaxNTUserNameLength);
	ASSERT(aChallengeHash.Length()==KPppMsChap2ChallengeHashSize);

	CSHA1* sha1 = CSHA1::NewL();
	CleanupStack::PushL(sha1);

// RFC 2759: "Only the user name (as presented by the peer and
// excluding any prepended domain name)"
	TPtrC8 userName(aUserName);
	TInt i = aUserName.Locate('\\');
	if (i >= 0 && i < userName.Length() - 1)
		userName.Set(aUserName.Mid(i + 1));
	else if (i >= userName.Length() - 1)
		User::Leave(KErrGeneral);


	sha1->Update(aPeerChallenge);
	sha1->Update(aAuthenticatorChallenge);

	aChallengeHash.Copy(sha1->Final(userName).Ptr(),
				KPppMsChap2ChallengeHashSize);


	CleanupStack::PopAndDestroy(sha1);

	ASSERT(aChallengeHash.Length()==KPppMsChap2ChallengeHashSize);
	}


void CPppMsChap2::NtPasswordHashL(const TDesC16& aPassword,
				TDes8& aPasswordHash)
/**
   Computes the hash of the Microsoft Windows NT password using MD4.
   @param aPassword [in] The Microsoft Windows NT password (0 to 256
   Unicode char).
   @param aPasswordHash [out] The MD4 hash of the Microsoft Windows NT
   password (16 octets).
   @note This function implements the NtPasswordHash routine specified
   in RFC 2759.
   @internalComponent
*/
	{
	ASSERT(aPassword.Length() <= KPppMsChapMaxNTPasswordLength);
	ASSERT(aPasswordHash.Length()==KPppMsChap2HashSize);

// The following code does not use the Symbian Security subsystem
// components, because they do not provide a MD4 implementation yet.
// This is a provisional solution until the Symbian Security subsystem
// components will provide a MD4 implementation.

	CMd4* md4 = CMd4::NewL();
	CleanupStack::PushL(md4);


// The following code assumes that the data in TDesC16 descriptors is
// stored in little endian byte order, which is currently a
// characteristic of Symbian OS, so the reinterpret_cast is assumed to
// be safe here.
	md4->Input(TPtrC8(reinterpret_cast<const TUint8*>(
					aPassword.Ptr()), 
			aPassword.Length()*2));
	md4->Output(aPasswordHash);
	
	CleanupStack::PopAndDestroy(md4);

	ASSERT(aPasswordHash.Length()==KPppMsChap2HashSize);
	}


inline void CPppMsChap2::ChallengeResponseL(
					const TDesC8& aChallengeHash,
					TDes8& aPaddablePasswordHash, 
					TDes8& aResponse)
/**
   Computes the challenge response using DES.
   @param aChallengeHash [in] The hash of the peer challenge,
   authenticator challenge and username, computed using SHA-1 (8
   octets).
   @param aPaddablePasswordHash [in/out] The hash of the password in a
   paddable buffer (16 octets in buffer with at least 21 octets
   maximum length).
   @param aResponse [out] The challenge response (24 octets).
   @note This function implements the ChallengeResponse routine
   specified in RFC 2759.
   @internalComponent
*/
	{
	CPppMsChap::ChallengeResponseL(aChallengeHash,
					aPaddablePasswordHash, 
					aResponse);
	}


void CPppMsChap2::DesEncryptL(const TDesC8& aClear, 
				const TDesC8& aKey, 
				TDes8& aCypher)
/**
   Encrypts a plaintext into a ciphertext using the DES encryption
   algorithm in ECB mode.
   @param aClear [in] A plaintext (8 octets).
   @param aKey [in] A key (7 octets).
   @param aCypher [out] The ciphertext (8 octets).
   @note This function implements the DesEncrypt routine specified in
   RFC 2759.
   @internalComponent
*/
	{
	CPppMsChap::DesEncryptL(aClear, aKey, aCypher);
	}


inline void CPppMsChap2::HashNtPasswordHashL(
					const TDesC8& aPasswordHash, 
					TDes8& aPasswordHashHash)
/**
   Computes the hash of the hash of the Microsoft Windows NT password
   using MD4.
   @param aPasswordHash [in] The hash of the Microsoft Windows NT
   password (16 octets).
   @param aPasswordHashHash [out] The hash of the hash of the
   Microsoft Windows NT password, computed using MD4 (16 octets).
   @note This function implements the HashNtPasswordHash routine
   specified in RFC 2759.
   @internalComponent
*/
	{
	ASSERT(aPasswordHash.Length()==KPppMsChap2HashSize);
	ASSERT(aPasswordHashHash.Length()==KPppMsChap2HashSize);

	CMd4* md4 = CMd4::NewL();
	CleanupStack::PushL(md4);
	
// 	aPasswordHashHash.Copy(md4.Final(aPasswordHash));

	md4->Input(aPasswordHash);
	md4->Output(aPasswordHashHash);
	
	CleanupStack::PopAndDestroy(md4);

	ASSERT(aPasswordHashHash.Length()==KPppMsChap2HashSize);
	}


inline void CPppMsChap2::GenerateAuthenticatorResponseL(
				const TDesC16& aPassword, 
				const TDesC8& aNTResponse, 
				const TDesC8& aPeerChallenge,
				const TDesC8& aAuthenticatorChallenge,
				const TDesC8& aUserName, 
				TDes8& aAuthenticatorResponse)
/**
   Generates the expected MS-CHAP-V2 Authenticator Response Value.
   @param aPassword [in] The Microsoft Windows NT password (0 to 256
   Unicode char).
   @param aNTResponse [in] The MS-CHAP-V2 NT-Response (24 octets).
   @param aPeerChallenge [in] The Peer Challenge (16 octets).
   @param aAuthenticatorChallenge [in] The Authenticator Challenge (16
   octets).
   @param aUserName [in] The Microsoft Windows NT username (0 to 256
   char).
   @param aAuthenticatorResponse [out] The expected MS-CHAP-V2
   Authenticator Response Value encoded in the format
   "S=<auth_string>" as specified in RFC 2759 (42 octets).
   @note This function implements the GenerateAuthenticatorResponse
   routine specified in RFC 2759.
   @internalComponent
*/
	{
	ASSERT(aPassword.Length()<=KPppMsChapMaxNTPasswordLength);
	ASSERT(aNTResponse.Length() == KPppMsChap2NTResponseSize);
	ASSERT(aPeerChallenge.Length() ==
		KPppMsChap2PeerChallengeSize);
	ASSERT(aAuthenticatorChallenge.Length() ==
		   KPppMsChap2AuthenticatorChallengeSize);
	ASSERT(aUserName.Length()<=KPppMsChapMaxNTUserNameLength);
	ASSERT(aAuthenticatorResponse.Length() ==
		   KPppMsChap2AuthenticatorResponseSize);

	HBufC8* passwordHashBuf=HBufC8::NewMaxLC(KPppMsChap2HashSize);
	TPtr8 passwordHash(passwordHashBuf->Des());

	NtPasswordHashL(aPassword, passwordHash);

	HashNtPasswordHashL(passwordHash, passwordHash);

	CSHA1* sha1 = CSHA1::NewL();
	CleanupStack::PushL(sha1);

// A magic string literal specified in RFC 2759 used in reponse
// generation by the GenerateAuthenticatorResponse routine for SHA-1
// encryption.
	_LIT8(KMagic1, "Magic server to client signing constant");


	sha1->Update(passwordHash);
	sha1->Update(aNTResponse);
	TPtrC8 hash(sha1->Final(KMagic1));


	HBufC8* challengeHashBuf =
			HBufC8::NewMaxLC(KPppMsChap2ChallengeHashSize);
	TPtr8 challengeHash(challengeHashBuf->Des());
	ChallengeHashL(aPeerChallenge, 
			aAuthenticatorChallenge, 
			aUserName,
			challengeHash);

// Another magic string literal specified in RFC 2759 used in reponse
// generation by the GenerateAuthenticatorResponse routine for SHA-1
// encryption.
	_LIT8(KMagic2, "Pad to make it do more than one iteration");


	sha1->Update(hash);
	sha1->Update(challengeHash);
	const TUint8* pHash = sha1->Final(KMagic2).Ptr();

	
	_LIT8(KFormat,
		  "S=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
			"%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X");
	aAuthenticatorResponse.Format(KFormat, 
					*pHash, 
					*(pHash + 1),
					*(pHash + 2), 
					*(pHash + 3), 
					*(pHash + 4), 
					*(pHash + 5), 
					*(pHash + 6), 
					*(pHash + 7), 
					*(pHash + 8), 
					*(pHash + 9), 
					*(pHash + 10),
					*(pHash + 11), 
					*(pHash + 12), 
					*(pHash + 13), 
					*(pHash + 14),
					*(pHash + 15), 
					*(pHash + 16), 
					*(pHash + 17), 
					*(pHash + 18),
					*(pHash + 19));

	CleanupStack::PopAndDestroy(challengeHashBuf);

	CleanupStack::PopAndDestroy(sha1);
	CleanupStack::PopAndDestroy(passwordHashBuf);

	ASSERT(aAuthenticatorResponse.Length() ==
		   KPppMsChap2AuthenticatorResponseSize);
	}
