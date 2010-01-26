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
// Extensions (MS-CHAP) - RFC 2433, except the authenticator-controlled
// authentication retry mechanisms and the password changing mechanisms -
// this is in accordance with the requirements.
// 
//

/**
 @file
 @brief Source file for the implementation of Microsoft PPP CHAP
 @internalComponent 
*/

#include "MSCHAP.H"
#include "PPPConfig.h"

// using DES
#include <symmetric.h>

//using MD4
#include "MD4.H"


CPppMsChap::CPppMsChap()
/**
   Constructor.
   @internalComponent
*/
	: iResponseValue(KPppMsChapResponseValueSize)
	{
	}

void CPppMsChap::InitL(CPppLcp* aLcp)
/**
   @copydoc CPppChap::InitL(CPppLcp*)
   @see CPppChap::InitL(CPppLcp*)
   @internalComponent
*/
	{
	CPppChap::InitL(aLcp);

#ifdef __MS_CHAP_WITH_LAN_MANAGER__
	iUseNTResponse = ETrue;
#endif // __MS_CHAP_WITH_LAN_MANAGER__
	}

CPppMsChap::~CPppMsChap()
/**
   Destructor.
   @internalComponent
*/
	{
#ifdef _UNICODE
	delete iUserName;
#endif
	}


void CPppMsChap::CheckChallengePacketL(RMBufPacket& aPacket)
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
				KPppMsChapChallengeSize,
			User::Leave(KErrUnderflow));	

	__ASSERT_ALWAYS(*(aPacket.First()->Ptr() +
							KPppChapCodeFieldSize +
							KPppChapIdFieldSize +
							KPppChapLengthFieldSize) == 
						KPppMsChapChallengeSize,
				User::Leave(KErrOverflow));	
	}


void CPppMsChap::MakeResponseL(TUint8 /*aChallengeId*/, 
				const TDesC8& aChallengeValue, 
				TPtrC8& aResponseValue, 
				TPtrC8& aResponseName)
/**
   @copydoc CPppChap::MakeResponseL(TUint8, const TDesC8&, TPtrC8&, TPtrC8&)
   @see CPppChap::MakeResponseL(TUint8, const TDesC8&, TPtrC8&, TPtrC8&)
   @internalComponent
*/
	{
	ASSERT(aChallengeValue.Length() == KPppMsChapChallengeSize);

    const CCredentialsConfig* credentials = iPppLcp->GetCredentials();
	const TDesC& password = credentials->GetPassword();

#ifndef __MS_CHAP_WITH_LAN_MANAGER__

// The NT password shall not be longer than
// KPppMsChapMaxNTPasswordLength
	__ASSERT_ALWAYS(password.Length() <=
				KPppMsChapMaxNTPasswordLength,
			User::Leave(KErrTooBig));

#else // __MS_CHAP_WITH_LAN_MANAGER__

	if (iUseNTResponse)
// The NT password shall not be longer than
// KPppMsChapMaxNTPasswordLength
		__ASSERT_ALWAYS(password.Length() <=
					KPppMsChapMaxNTPasswordLength,
				User::Leave(KErrTooBig));
	else
// The LAN Manager password shall not be longer than
// KPppMsChapMaxNTPasswordLength OEM characters
		__ASSERT_ALWAYS(password.Length() <=
				KPppMsChapMaxLANManagerPasswordLength,
				User::Leave(KErrTooBig));

#endif // __MS_CHAP_WITH_LAN_MANAGER__

#ifdef _UNICODE
	TPtrC16 uniPassword(password);
#else // ! _UNICODE
// The following MS-CHAP routines require the password to be
// represented as 0-to-256-unicode-char (RFC 2433), so convert the
// password to Unicode.
	HBufC16& uniPassword = *HBufC16::NewLC(password.Length());
 	uniPassword.Des().Copy(password);
#endif // ! _UNICODE

	TPtr8 ntResponse(const_cast<TUint8*>(iResponseValue.Ptr()) +
		KPppMsChapLanManResponseSize,
		KPppMsChapNTResponseSize,
		KPppMsChapNTResponseSize);
	NTChallengeResponseL(aChallengeValue, 
				uniPassword,
				ntResponse);

	TPtr8 lmResponse(const_cast<TUint8*>(iResponseValue.Ptr()),
			KPppMsChapLanManResponseSize,
			KPppMsChapLanManResponseSize);

#ifdef __MS_CHAP_WITH_LAN_MANAGER__

#ifdef _UNICODE
// The following MS-CHAP routines require the password to be
// represented as 0-to-14-oem-char (RFC 2433), so convert the
// password.
	HBufC8& oemPassword = *HBufC8::NewLC(password.Length());
 	oemPassword.Des().Copy(password);
#else // ! _UNICODE
	TPtrC8 oemPassword(password);
#endif // ! _UNICODE

	LmChallengeResponseL(aChallengeValue, 
				oemPassword,
				lmResponse);
	*(const_cast<TUint8*>(iResponseValue.Ptr()) +
			KPppMsChapLanManResponseSize +
			KPppMsChapNTResponseSize) 
		= static_cast<TUint8>(iUseNTResponse);

#else //! __MS_CHAP_WITH_LAN_MANAGER__

	lmResponse.FillZ();
	*(const_cast<TUint8*>(iResponseValue.Ptr()) +
			KPppMsChapLanManResponseSize +
         		KPppMsChapNTResponseSize) = 1;
#endif //! __MS_CHAP_WITH_LAN_MANAGER__

	aResponseValue.Set(iResponseValue);

// The NT username shall not be longer than
// KPppMsChapMaxNTUserNameLength
	const TDesC& username = credentials->GetUserName();
	__ASSERT_ALWAYS(username.Length() <=
		KPppMsChapMaxNTUserNameLength,
		User::Leave(KErrTooBig));

#ifdef _UNICODE
// The MS-CHAP routines require the username to be represented as
// 0-to-256-char (RFC 2433), so convert the username.
	delete iUserName;
	iUserName = 0;
	iUserName = HBufC8::NewL(username.Length());
	iUserName->Des().Copy(username);
	aResponseName.Set(*iUserName);
#else //! _UNICODE
	aResponseName.Set(username);
#endif //! _UNICODE

#ifdef _UNICODE

#ifdef __MS_CHAP_WITH_LAN_MANAGER__ 
	CleanupStack::PopAndDestroy(&oemPassword);
#endif // __MS_CHAP_WITH_LAN_MANAGER__ 

#else //! _UNICODE
	CleanupStack::PopAndDestroy(&uniPassword);
#endif //! _UNICODE

	ASSERT(aResponseValue.Length() == KPppMsChapResponseValueSize);
	ASSERT(aResponseName.Length() >= KPppChapMinNameSize &&
		   aResponseName.Length() <= KPppMsChapMaxNTUserNameLength);
	}


void CPppMsChap::FailureL(RMBufPacket& aPacket)
/**
   @copydoc CPppChap::FailureL(RMBufPacket&)
   @see CPppChap::FailureL(RMBufPacket&)
   @internalComponent
*/
	{
	__ASSERT_ALWAYS(aPacket.Length() >= KPppChapCodeFieldSize +
					KPppChapIdFieldSize + 
					KPppChapLengthFieldSize ,
			User::Leave(KErrUnderflow));
	if (!CheckIdentifier(aPacket))
		User::Leave(KErrGeneral);

	TimerCancel();

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
	TBool hasNewChallenge;
	TInt sysError = KErrIfAuthenticationFailure;

	ProcessFailureMessageL(failureMessage, 
				msChapError,
				isRetryAllowed, 
				hasNewChallenge, 
				iChallengeRef,
				passwordProtoVersion);

	sysError = TranslateMsChapError(msChapError);

#ifndef __MS_CHAP_WITH_LAN_MANAGER__

// The authenticator-controlled authentication retry mechanisms and
// the password changing mechanisms have not been implemented in this
// release - this is in accordance with the project requirements.
// Consequently simply fail.

	DoFail(sysError);

#else // __MS_CHAP_WITH_LAN_MANAGER__

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

	if (!hasNewChallenge)
		iChallengeRef[0] += 23; // magic constant from RFC 2433


	if(!iUseNTResponse)
		{
// Has already retried authentication using a LAN Manager compatible
// Challenge Response.  Have tried Microsoft Windows NT compatible
// Challenge Response and LAN Manager compatible Challenge Response,
// so fail now
		iUseNTResponse = ETrue;
		DoFail(sysError);
		return;
		}

// Retry authentication using a LAN Manager compatible Challenge
// Response.
	iUseNTResponse = EFalse;

	RetryPasswordL();

#endif //  __MS_CHAP_WITH_LAN_MANAGER__
	}


inline void CPppMsChap::ProcessFailureMessageL(
					const TDesC8& aFailureMessage,
					TUint& aErrorCode, 
					TUint8& aRetryFlag, 
					TBool& aHasNewChallenge, 
					TDes8& aChallenge, 
					TUint8&	aPasswordProtoVersion)
/**
   Processes a MS-CHAP Failure Message.
   @param aFailureMessage [in] A MS-CHAP Failure Message.  The Failure
   Message needs to be in the format specified in RFC 2433:
   "E=eeeeeeeeee R=r C=cccccccccccccccc V=vvvvvvvvvv".
   @param aErrorCode [out] The MS-CHAP Failure error code.
   @param aRetryFlag [out] The retry flag.  The flag will be set to
   "1" if a retry is allowed, and "0" if not.  When the authenticator
   sets this flag to "1" it disables short timeouts, expecting the
   peer to prompt the user for new credentials and resubmit the
   response.
   @param aHasNewChallenge [out] The flag that indicates if the
   Failure Message contains a new Challenge Value.
   @param aChallenge [out] The new Challenge Value, if the Failure
   Message contains a one.
   @param aPasswordProtoVersion [out] The password changing protocol
   supported by the peer.
   @internalComponent
*/
	{
	ASSERT(aChallenge.Length() == KPppMsChapChallengeSize);
	
	TLex8 input(aFailureMessage);

	if (input.Get() != 'E')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);


// RFC 2433: ""eeeeeeeeee" is the ASCII representation of a decimal
// error code (need not be 10 digits) corresponding to one of those
// listed below, though implementations should deal with codes not on
// this list gracefully."

	
	TInt ret;
	if ((ret = input.Val(aErrorCode)) != KErrNone)
		if (ret != KErrOverflow)
			User::Leave(KErrGeneral);
		else
// Gracefully handle unusually large, yet valid, MS-CHAP specific
// error code values.  This code only handles the MS-CHAP specific
// error code values specified in RFC 2433.
			aErrorCode=0;

	input.SkipSpace();

	if (input.Get() != 'R')
		User::Leave(KErrGeneral);

	if (input.Get() != '=')
		User::Leave(KErrGeneral);

	if (input.Val(aRetryFlag, EDecimal)!=KErrNone)
		User::Leave(KErrGeneral);

	input.SkipSpace();

	switch (input.Get())
		{
	case 'C':
		{		
		if (input.Get() != '=')
			User::Leave(KErrGeneral);

		TPtrC8 token(input.NextToken());
// This field is 16 hexadecimal digits representing an ASCII
// representation of a new challenge value.  Each octet is represented
// in 2 hexadecimal digits.
		if (token.Length() != KPppMsChapChallengeSize*2)
			User::Leave(KErrGeneral);

		TLex8 lex;
		TUint8 octet;
		TUint8* pChallengeOctet = 
			const_cast<TUint8*>(aChallenge.Ptr());
		TUint8 i = 0;
		do
			{
			lex.Assign(token.Mid(i*2, 2));
			if (lex.Val(octet, EHex) != KErrNone)
				User::Leave(KErrGeneral);

			*(pChallengeOctet + i) = octet;
			}
		while (++i < KPppMsChapChallengeSize);

		aHasNewChallenge = ETrue;

		input.SkipSpace();

		if (input.Get()!='V')
			{
			aPasswordProtoVersion = 1;
			User::Leave(KErrGeneral);
			}
		}

// As specified in RFC 2433, the field containing the ASCII
// representation of a new challenge value is optional, so fall
// through.
		

	case 'V':

// RFC 2433: "The "vvvvvvvvvv" is the decimal version code (need not
// be 10 digits) indicating the MS-CHAP protocol version supported on
// the server.  Currently, this is interesting only in selecting a
// Change Password packet type.  If the field is not present the
// version should be assumed to be 1; since use of the version 1
// Change Password packet has been deprecated, this field SHOULD
// always contain a value greater than or equal to 2."


		aHasNewChallenge = EFalse;

		if (input.Get() != '=')
			User::Leave(KErrGeneral);

		if ((ret=input.Val(aPasswordProtoVersion,
				EDecimal)) != KErrNone)
			if (ret!= KErrOverflow)
				User::Leave(KErrGeneral);
			else
// Gracefully handle unusually large, yet valid, password changing
// protocol version values.  This code only handles the password
// changing protocol version codes values specified in RFC 2433.
				aPasswordProtoVersion = 0;

		break;
	 
	default:
		aHasNewChallenge = EFalse;
		aPasswordProtoVersion = 1;
		}
	}


inline void CPppMsChap::RetryPasswordL()
/**
   Retries the authentication.
   @internalComponent
*/
	{
	++iCurrentId;

	iResponseRetryCount = 0;
	RespondL();
	}


inline void CPppMsChap::NTChallengeResponseL(const TDesC8& aChallenge,
					     const TDesC16& aPassword,
					     TDes8& aResponse)
/**
   Computes a MS-CHAP Windows NT compatible Challenge Response.
   @param aChallenge [in] A MS-CHAP Challenge (8 octets).
   @param aPassword [in] The Microsoft Windows NT password (0 to 256
   Unicode char).
   @param aResponse [out] The MS-CHAP Windows NT compatible Challenge
   Response (24 octets).
   @note This function implements the NTChallengeResponse routine
   specified in RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aChallenge.Length() == KPppMsChapChallengeSize);
	ASSERT(aPassword.Length() <= KPppMsChapMaxNTPasswordLength);
	ASSERT(aResponse.Length() == KPppMsChapNTResponseSize);

	HBufC8* paddedPasswordHashBuf =
			HBufC8::NewLC(KPppMsChapPaddedHashSize);
	TPtr8 paddablePasswordHash(paddedPasswordHashBuf->Des());

	paddablePasswordHash.SetLength(KPppMsChapHashSize);
	NtPasswordHashL(aPassword, paddablePasswordHash);

	ChallengeResponseL(aChallenge, 
			paddablePasswordHash,
			aResponse);

	CleanupStack::PopAndDestroy(paddedPasswordHashBuf);
	ASSERT(aResponse.Length() == KPppMsChapNTResponseSize);
	}


inline void CPppMsChap::NtPasswordHashL(const TDesC16& aPassword,
					TDes8& aPasswordHash)
/**
   Computes the hash of the Microsoft Windows NT password using MD4.
   @param aPassword [in] The Microsoft Windows NT password (0 to 256
   Unicode char).
   @param aPasswordHash [out] The MD4 hash of the Microsoft Windows NT
   password (16 octets).
   @note This function implements the NtPasswordHash routine specified
   in RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aPassword.Length() <= KPppMsChapMaxNTPasswordLength);
	ASSERT(aPasswordHash.Length() == KPppMsChapHashSize);

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
		aPassword.Size()));
	md4->Output(aPasswordHash);

	CleanupStack::PopAndDestroy(md4);

	ASSERT(aPasswordHash.Length() == KPppMsChapHashSize);
	}


inline void CPppMsChap::ChallengeResponseL(const TDesC8& aChallenge,
					TDes8& aPaddablePasswordHash,
					TDes8& aResponse)
/**
   Computes the Challenge Response.
   @param aChallenge [in] A MS-CHAP Challenge (8 octets).
   @param aPaddablePasswordHash [in/out] The hash of the password in a
   paddable buffer (16 octets in a buffer with at least 21 octets
   maximum length).
   @param aResponse [out] The Challenge Response (24 octets).
   @note This function implements the ChallengeResponse routine
   specified in RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aChallenge.Length()==KPppMsChapChallengeSize);
	ASSERT(aPaddablePasswordHash.Length()==KPppMsChapHashSize &&
		aPaddablePasswordHash.MaxLength() >=
			KPppMsChapPaddedHashSize);
	ASSERT(aResponse.Length() == KPppMsChapNTResponseSize);

// aPaddablePasswordHash contains the hash of the password (16 octets)
// zero-padded to 21 octets

// RFC 2433 - ChallengeResponse(): "Set ZPasswordHash to
// PasswordHash zero-padded to 21 octets", i.e. 5 octets
	aPaddablePasswordHash.AppendFill(0, 
					KPppMsChapPaddedHashSize -
						KPppMsChapHashSize);

// The first 8 octets of aResponse
	TPtr8 responseChunk(const_cast<TUint8*>(aResponse.Ptr()),
				KPppDESKeySize, 
				KPppDESKeySize);
	DesEncryptL(aChallenge,
		aPaddablePasswordHash.Left(KPppMsChapDESKeySize),
		responseChunk);

// The second 8 octets of aResponse
	responseChunk.Set(const_cast<TUint8*>(aResponse.Ptr()) +
					KPppDESKeySize, 
			KPppDESKeySize,
			KPppDESKeySize);
  	DesEncryptL(aChallenge,
		aPaddablePasswordHash.Mid(KPppMsChapDESKeySize,
			KPppMsChapDESKeySize), 
		responseChunk);

// The third 8 octets of aResponse
	responseChunk.Set(const_cast<TUint8*>(aResponse.Ptr()) +
		2*KPppDESKeySize, KPppDESKeySize, KPppDESKeySize);
 	DesEncryptL(aChallenge,
		aPaddablePasswordHash.Mid(2*KPppMsChapDESKeySize,
			KPppMsChapDESKeySize),
		responseChunk);


// Restore the original length of the password hash
	aPaddablePasswordHash.SetLength(KPppMsChapHashSize);

	ASSERT(aResponse.Length() == KPppMsChapNTResponseSize);
	}

void CPppMsChap::DesEncryptL(const TDesC8& aClear, 
			const TDesC8& aKey,
			TDes8& aCypher)
/**
   Encrypts a plaintext into a ciphertext using the DES encryption
   algorithm in ECB mode.
   @param aClear [in] A plaintext (8 octets).
   @param aKey [in] A key (7 octets).
   @param aCypher [out] The ciphertext (8 octets).
   @note This function implements the DesEncrypt routine specified in
   RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aClear.Length() == KPppMsChapDESClearTextSize);
	ASSERT(aKey.Length() == KPppMsChapDESKeySize);
	ASSERT(aCypher.Length() == KPppMsChapDESCipherTextSize);

	HBufC8* desKeyBuf=HBufC8::NewMaxLC(KPppDESKeySize);
	TPtr8 desKey(desKeyBuf->Des());

// RFC 2433: "Use the DES encryption algorithm [4] in ECB mode [10] to
// encrypt Clear into Cypher such that Cypher can only be decrypted
// back to Clear by providing Key.  Note that the DES algorithm takes
// as input a 64-bit stream where the 8th, 16th, 24th, etc.  bits are
// parity bits ignored by the encrypting algorithm.  Unless you write
// your own DES to accept 56-bit input without parity, you will need
// to insert the parity bits yourself."

	MakeDesKey(aKey, desKey);


 	CBlockTransformation* encryptor = 
		CDESEncryptor::NewLC(desKey, EFalse);
	CPaddingNone* padding = CPaddingNone::NewLC();
 	CBufferedEncryptor* bufEncryptor =
 		CBufferedEncryptor::NewL(encryptor, padding);
	CleanupStack::Pop(padding);
	CleanupStack::Pop(encryptor);
	CleanupStack::PushL(bufEncryptor);

	aCypher.Zero();
	bufEncryptor->ProcessFinalL(aClear, aCypher);

 	CleanupStack::PopAndDestroy(bufEncryptor);


	CleanupStack::PopAndDestroy(desKeyBuf);
	ASSERT(aCypher.Length() == KPppMsChapDESCipherTextSize);
	}


inline void CPppMsChap::MakeDesKey(const TDesC8& aMsChapKey, 
				TDes8& aDesKey)
/**
   Creates a DES key by inserting the parity bits.  The DES algorithm
   takes as input a 64-bit stream where the 8th, 16th, 24th, etc. bits
   are parity bits ignored by the encrypting algorithm.
   @param aMsChapKey [in] A key used by MS-CHAP for DES encryption. (7
   octets).
   @param aDesKey [out] A DES key (8 octets).
   @internalComponent
*/
	{
	ASSERT(aMsChapKey.Length() == KPppMsChapDESKeySize);
	ASSERT(aDesKey.Length() == KPppDESKeySize);

// RFC 2433, RFC 2759: "Use the DES encryption algorithm [4] in ECB
// mode [10] to encrypt Clear into Cypher such that Cypher can only be
// decrypted back to Clear by providing Key.  Note that the DES
// algorithm takes as input a 64-bit stream where the 8th, 16th, 24th,
// etc.  bits are parity bits ignored by the encrypting algorithm.
// Unless you write your own DES to accept 56-bit input without
// parity, you will need to insert the parity bits yourself."

	TUint8* pdk = const_cast<TUint8*>(aDesKey.Ptr());
	const TUint8* pmk = aMsChapKey.Ptr();
	TUint16 high, low;
	TUint8 i = 0;
    do
		{
		high = *(pmk + i/8);
		low = *(pmk + i/8 + 1);
		*(pdk + i/7) = static_cast<TUint8>(
			((high << 8 | low) >> (8 - i%8)) & 0xfe);
		i += 7;
		}
	while (i < 49);

	*(pdk + 7) = static_cast<TUint8>(*(pmk + 6) << 1 & 0xfe);

	ASSERT(aDesKey.Length() == KPppDESKeySize);
	}


TInt CPppMsChap::TranslateMsChapError(TUint aMsChapError)
/**
   Translates a MS-CHAP-V1 error code into a Symbian OS PPP NIF
   specific error code.
   @param aMsChapError A MS-CHAP-V1 error code.
   @return The Symbian OS PPP NIF specific error code corresponding to
   the MS-CHAP error code.
   @internalComponent
*/
	{
	// Always return from every conditional branch of the following
	// switch statement
	switch(aMsChapError)
		{
	case KPppMsChapErrorRestrictedLogon:
		return KErrIfRestrictedLogonHours;
		
	case KPppMsChapErrorAccountDisabled:
		return KErrIfAccountDisabled;
		
	case KPppMsChapErrorPasswordExpired:
		return KErrIfPasswdExpired;
		
	case KPppMsChapErrorNoDialinPermission:
		return KErrIfNoDialInPermission;
		
	case KPppMsChapErrorChangingPassword:
		return KErrIfChangingPassword;
		
	case KPppMsChapAuthenticationFailure:
		return KErrIfAuthenticationFailure;
		}

	// If unknown MS-CHAP failure code 
	// use KErrIfAuthenticationFailure
	// default:
	return KErrIfAuthenticationFailure;
	}


#ifdef __MS_CHAP_WITH_LAN_MANAGER__

// NB The use of the LAN Manager compatible challenge response has
// been deprecated according to RFC 2433.
inline void CPppMsChap::LmChallengeResponseL(const TDesC8& aChallenge,
					const TDesC8& aPassword, 
					TDes8& aResponse)
/**
   Computes a MS-CHAP LAN Manager compatible Challenge Response.
   @param aChallenge [in] A MS-CHAP Challenge (8 octets).
   @param aPassword [in] The LAN Manager password (0 to 14 OEM char).
   @param aResponse [out] The MS-CHAP LAN Manager compatible Challenge
   Response (24 octets).
   @note This function implements the LmChallengeResponse routine
   specified in RFC 2433.
   @note The use of the LAN Manager compatible Challenge Response has
   been deprecated according to RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aChallenge.Length() == KPppMsChapChallengeSize);
	ASSERT(aPassword.Length() <=
		KPppMsChapMaxLANManagerPasswordLength);
	ASSERT(aResponse.Length() == KPppMsChapLanManResponseSize);

	HBufC8* paddedPasswordHashBuf =
			HBufC8::NewLC(KPppMsChapPaddedHashSize);
	TPtr8 paddablePasswordHash(paddedPasswordHashBuf->Des());

	paddablePasswordHash.SetLength(KPppMsChapHashSize);
	LmPasswordHashL(aPassword, paddablePasswordHash);

	ChallengeResponseL(aChallenge, 
			paddablePasswordHash,
			aResponse);

	CleanupStack::PopAndDestroy(paddedPasswordHashBuf);
	ASSERT(aResponse.Length() == KPppMsChapLanManResponseSize);
	}


// NB The use of the LAN Manager compatible challenge response has
// been deprecated according to RFC 2433.
inline void CPppMsChap::LmPasswordHashL(const TDesC8& aPassword,
					TDes8& aPasswordHash)
/**
   Computes the hash of the LAN Manager password using DES.
   @param aPassword [in] The LAN Manager password (0 to 14 OEM char).
   @param aPasswordHash [out] The DES hash of the LAN Manager password
   (16 octets).
   @note This function implements the LmPasswordHash routine specified
   in RFC 2433.
   @note The use of the LAN Manager compatible Challenge Response has
   been deprecated according to RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aPassword.Length() <=
		KPppMsChapMaxLANManagerPasswordLength);
	ASSERT(aPasswordHash.Length() == KPppMsChapHashSize);

	HBufC8* ucasePasswordBuf =
	       HBufC8::NewLC(KPppMsChapMaxLANManagerPasswordLength);
	TPtr8 ucasePassword(ucasePasswordBuf->Des());
	ucasePassword.Copy(aPassword);
	ucasePassword.UpperCase();
	ucasePassword.AppendFill(0,
		KPppMsChapMaxLANManagerPasswordLength -
			ucasePassword.Length());

// The first 8 octets of aResponse
	TPtr8 responseChunk(const_cast<TUint8*>(aPasswordHash.Ptr()),
				KPppDESKeySize, KPppDESKeySize);
	DesHashL(ucasePassword.Left(KPppMsChapDESKeySize), 
		responseChunk);

	responseChunk.Set(const_cast<TUint8*>(aPasswordHash.Ptr()) +
				KPppDESKeySize, 
			KPppDESKeySize,
			KPppDESKeySize);
	DesHashL(ucasePassword.Mid(KPppMsChapDESKeySize,
			KPppMsChapDESKeySize), 
		responseChunk);

	CleanupStack::PopAndDestroy(ucasePasswordBuf);
	ASSERT(aPasswordHash.Length() == KPppMsChapHashSize);
	}

// NB The use of the LAN Manager compatible challenge response has
// been deprecated according to RFC 2433.
inline void CPppMsChap::DesHashL(const TDesC8& aClear, TDes8& aCypher)
/**
   Makes aCypher an irreversibly encrypted form of aClear by
   encrypting known text using aClear as the secret key.  The known
   text consists of the string "KGS!@#$%".
   @param aClear [in] A plaintext used as the secret key for
   encryption (7 octets).
   @param aCypher [out] The ciphertext (8 octets).
   @note This function implements the DesHash routine specified in RFC
   2433.
   @note The use of the LAN Manager compatible challenge response has
   been deprecated according to RFC 2433.
   @internalComponent
*/
	{
	ASSERT(aClear.Length() == KPppMsChapDESKeySize);
	ASSERT(aCypher.Length() == KPppMsChapDESCipherTextSize);

	HBufC8* desKeyBuf = HBufC8::NewMaxLC(KPppDESKeySize);
	TPtr8 desKey(desKeyBuf->Des());

	MakeDesKey(aClear, desKey);

// A magic string literal specified in RFC 2433 used as clear text for
// making aCypher an irreversibly encrypted form of aClear by
// encrypting this clear text using aClear as the secret key.
	_LIT8(KStdText, "KGS!@#$%");


 	CBlockTransformation* encryptor = 
		CDESEncryptor::NewLC(desKey, EFalse);
	CPaddingNone* padding = CPaddingNone::NewLC();
 	CBufferedEncryptor* bufEncryptor =
		CBufferedEncryptor::NewL(encryptor, padding);
	CleanupStack::Pop(padding);
	CleanupStack::Pop(encryptor);
	CleanupStack::PushL(bufEncryptor);

	aCypher.Zero();
	bufEncryptor->ProcessFinalL(KStdText, aCypher);

 	CleanupStack::PopAndDestroy(bufEncryptor);


	CleanupStack::PopAndDestroy(desKeyBuf);
	ASSERT(aCypher.Length() == KPppMsChapDESCipherTextSize);
	}

#endif // __MS_CHAP_WITH_LAN_MANAGER__
