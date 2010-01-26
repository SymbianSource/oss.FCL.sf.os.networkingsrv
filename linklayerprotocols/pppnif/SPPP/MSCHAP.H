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
// Extensions (MS-CHAP) - RFC 2433, except the authenticator-controlled
// authentication retry mechanisms and the password changing mechanisms -
// this is in accordance with the requirements.
// 
//

/**
 @file
 @brief Header file for the implementation of Microsoft PPP CHAP
 @internalComponent 
*/

#ifndef __MSCHAP_H__
#define __MSCHAP_H__

#include "PPPCHAP.H"

// protocol specific constants for MS-CHAP

/**
   The size of the MS-CHAP Microsoft LAN Manager compatible challenge
   response subfield.
   @internalComponent
*/
const TUint8 KPppMsChapLanManResponseSize = 24u;

/**
   The size of the MS-CHAP Microsoft Windows NT compatible challenge
   response subfield.
   @internalComponent
*/
const TUint8 KPppMsChapNTResponseSize = 24u;

/**
   The size of the MS-CHAP Response Flags subfield.
   @internalComponent
*/
const TUint8 KPppMsChapResponseFlagsSize = 1u;

/**
   The size of the CHAP Response value for MS-CHAP.
   @internalComponent
*/
const TUint8 KPppMsChapResponseValueSize =
		KPppMsChapLanManResponseSize +
		KPppMsChapNTResponseSize +
		KPppMsChapResponseFlagsSize;


// extra protocol specific constants for MS-CHAP

/**
   The size of the MS-CHAP Challenge.
   @internalComponent
*/
const TUint8 KPppMsChapChallengeSize = 8u;

/**
   The size of the MS-CHAP password hash.
   @internalComponent
*/
const TUint8 KPppMsChapHashSize = 16u;

/**
   The size of the MS-CHAP padded password hash.
   @internalComponent
*/
const TUint8 KPppMsChapPaddedHashSize = 21u;

/**
   The size of the key used by MS-CHAP for DES encryption.
   @internalComponent
*/
const TUint8 KPppMsChapDESKeySize = 7u;

/**
   The size of the key used by MS-CHAP for DES encryption.
   @internalComponent
*/
const TUint8 KPppDESKeySize = 8u;

/**
   The size of the clear text used by MS-CHAP for DES encryption.
   @internalComponent
*/
const TUint8 KPppMsChapDESClearTextSize = 8u;

/**
   The size of the cipher text used by MS-CHAP for DES encryption.
   @internalComponent
*/
const TUint8 KPppMsChapDESCipherTextSize = 8u;

/**
   The maximum length of the Microsoft Windows NT Password (as
   specified in RFC 2433 and RFC 2759)
   @internalComponent
*/
const TInt KPppMsChapMaxNTPasswordLength = 256;

/**
   The maximum length of the Microsoft Windows NT User Name (as
   specified in RFC 2433 and RFC 2759)
   @internalComponent
*/
const TInt KPppMsChapMaxNTUserNameLength = 256;

#ifdef __MS_CHAP_WITH_LAN_MANAGER__

/**
   The maximum length of the LAN Manager Password (as specified in RFC
   2433)
   @internalComponent
*/
const TInt KPppMsChapMaxLANManagerPasswordLength = 14;

#endif //__MS_CHAP_WITH_LAN_MANAGER__


// MS-CHAP specific error codes

/**
   MS-CHAP error code for failures due to restricted logon.
   @internalComponent
*/
const TUint KPppMsChapErrorRestrictedLogon = 646u;

/**
   MS-CHAP error code for failures due to the user account being
   disabled.
   @internalComponent
*/
const TUint KPppMsChapErrorAccountDisabled = 647u;

/**
   MS-CHAP error code for failures due to the user password having
   been expired.
   @internalComponent
*/
const TUint KPppMsChapErrorPasswordExpired = 648u;

/**
   MS-CHAP error code for failures due lack of dialing permission.
   @internalComponent
*/
const TUint KPppMsChapErrorNoDialinPermission = 649u;

/**
   MS-CHAP error code for authentication failures.
   @internalComponent
*/
const TUint KPppMsChapAuthenticationFailure = 691u;

/**
   MS-CHAP error code for failures in changing the user password.
   @internalComponent
*/
const TUint KPppMsChapErrorChangingPassword = 709u;

/*final*/ NONSHARABLE_CLASS(CPppMsChap) : public CPppChap
/**
   Class that implements the Microsoft PPP CHAP Extensions (MS-CHAP) -
   RFC 2433, except the authenticator-controlled authentication retry
   mechanisms and the password changing mechanisms - this is in
   accordance with the requirements.
   @internalComponent
*/
	{
	friend class CPppMsChap2;
	
  public:
	static CPppAuthentication* NewL();

	virtual ~CPppMsChap();

	virtual void InitL(CPppLcp* aLcp);

  protected:
	virtual void CheckChallengePacketL(RMBufPacket& aPacket);

	virtual void FailureL(RMBufPacket& aPacket);

	virtual void MakeResponseL(TUint8 aChallengeId, 
		     		const TDesC8& aChallengeValue, 
				TPtrC8& aResponseValue, 
				TPtrC8&	aResponseName);


  private:

	CPppMsChap();

	static void NTChallengeResponseL(const TDesC8& aChallenge,
					const TDesC16& aPassword,
					TDes8& aResponse);

	static void NtPasswordHashL(const TDesC16& aPassword, 
				TDes8& aPasswordHash);


#ifdef __MS_CHAP_WITH_LAN_MANAGER__

	static void LmChallengeResponseL(const TDesC8& aChallenge,
					const TDesC8& aPassword, 
					TDes8& aResponse);

	static void LmPasswordHashL(const TDesC8& aPassword, 
				TDes8& aPasswordHash);

	static void DesHashL(const TDesC8& aClear, TDes8& aCypher);

#endif

	static void ChallengeResponseL(const TDesC8& aChallenge,
				TDes8& aPaddablePasswordHash, 
				TDes8& aResponse);

	static void DesEncryptL(const TDesC8& aClear, 
				const TDesC8& aKey, 
				TDes8& aCypher);

	static void MakeDesKey(const TDesC8& aMsChapKey, 
				TDes8& aDesKey);

 	static void ProcessFailureMessageL(
			const TDesC8& aFailureMessage, 
			TUint& aErrorCode, 
			TUint8& aRetryFlag,
			TBool& aHasNewChallenge, 
			TDes8& aChallenge,
			TUint8& aPasswordProtoVersion);

	static TInt TranslateMsChapError(TUint aMsChapError);

	void RetryPasswordL();

  private:
/** Buffer containing the MS-CHAP Response Value. */
	TBuf8<KPppMsChapResponseValueSize> iResponseValue;

#ifdef _UNICODE
/* Buffer containing the username. */
	HBufC8* iUserName;
#endif // _UNICODE

#ifdef __MS_CHAP_WITH_LAN_MANAGER__

/** Boolean flag that indicates if the Microsoft Windows NT password
will be used for authentication. */
	TBool iUseNTResponse;

#endif // __MS_CHAP_WITH_LAN_MANAGER__
	};


inline CPppAuthentication* CPppMsChap::NewL()
/**
   Object factory for CPppMsChap.
   @leave Standard Symbian OS error codes. e.g. KErrNoMemory.
   @note This function will not initialize the created object.  The
   InitL member function shall be called first in order to initialize
   the created object before using it.  This behavior is in agreement
   with the requirements for a migration to a plugin architecture.
*/
	{
	return new(ELeave) CPppMsChap;
	}

#endif //__MSCHAP_H__
