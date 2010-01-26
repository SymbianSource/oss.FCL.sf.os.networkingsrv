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
// Authentication Protocol with MD5 (CHAP with MD5) - RFC 1994.
// 
//

/**
 @file
 @brief Source file for the implementation of PPP Challenge Handshake
 @internalComponent
*/

#include "chapmd5.h"
// use CMD5
#include <hash.h>
#include "PPPConfig.h"

CPppChapMd5::CPppChapMd5()
/**
   Constructor. 
   @internalComponent
*/
	{
	}

CPppChapMd5::~CPppChapMd5()
/**
   Destructor.
   @internalComponent
*/
	{
#ifdef _UNICODE
	delete iUserName;
#endif

	delete iMd5;
	}


void CPppChapMd5::InitL(CPppLcp* aLcp)
/**
   @copydoc CPppChap::InitL(CPppLcp*)
   @see CPppChap::InitL(CPppLcp*)
   @internalComponent
*/
	{
	CPppChap::InitL(aLcp);
	iMd5 = CMD5::NewL();
	}


void CPppChapMd5::MakeResponseL(TUint8 /*aChallengeId*/, 
				const TDesC8& aChallengeValue, 
				TPtrC8& aResponseValue, 
				TPtrC8& aResponseName)
/**
   @copydoc CPppChap::MakeResponseL(TUint8, const TDesC8&, TPtrC8&,
   TPtrC8&)
   @see CPppChap::MakeResponseL(TUint8, const TDesC8&, TPtrC8&,
   TPtrC8&)
   @internalComponent
*/
	{
	ASSERT(aChallengeValue.Length() >= KPppChapMinValueSize);

    const CCredentialsConfig* credentials = iPppLcp->GetCredentials();
	const TDesC& username = credentials->GetUserName();

	__ASSERT_ALWAYS(username.Length() <= 
				KMaxTInt16 - 
				KPppChapCodeFieldSize -
				KPppChapIdFieldSize - 
				KPppChapLengthFieldSize -
				KPppChapValueSizeFieldSize -
				aResponseValue.Length(), 
			User::Leave(KErrTooBig));

#ifdef _UNICODE
	delete iUserName;
	iUserName=0;
	iUserName = HBufC8::NewL(username.Length());
	iUserName->Des().Copy(username);
 	aResponseName.Set(*iUserName);

	const TDesC& pwd=credentials->GetPassword();
	HBufC8& password = *HBufC8::NewLC(pwd.Length());
 	password.Des().Copy(pwd);
#else //! _UNICODE
	aResponseName.Set(username);
	const TDesC& password = credentials->GetPassword();
#endif //! _UNICODE

	iMd5->Update(TPtrC8(&iCurrentId, KPppChapIdFieldSize));
	iMd5->Update(password);
	aResponseValue.Set(iMd5->Final(aChallengeValue));

#ifdef _UNICODE
	CleanupStack::PopAndDestroy(&password);
#endif // _UNICODE

	ASSERT(aResponseValue.Length() == KPppChapMd5ValueSize);
	ASSERT(aResponseName.Length() >= KPppChapMinNameSize &&
		   aResponseName.Length() <= 
		   KMaxTInt16 - 
		   KPppChapCodeFieldSize -
		   KPppChapIdFieldSize - 
		   KPppChapLengthFieldSize -
		   KPppChapValueSizeFieldSize -
		   aResponseValue.Length());
	}
