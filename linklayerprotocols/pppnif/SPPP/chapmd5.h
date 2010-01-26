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
//

#ifndef __CHAPMD5_H__
#define __CHAPMD5_H__

/**
@file
@brief Header file for the implementation PPP Challenge Handshake
Authentication Protocol with MD5 (CHAP with MD5) - RFC 1994.
@internalComponent
*/

#include "PPPCHAP.H"

class CMD5;

// protocol specific definitions for CHAP with MD5

/**
   The size of the CHAP with MD5 Value field.
   @internalComponent
*/
const TUint8 KPppChapMd5ValueSize	= 16u;


/*final*/ NONSHARABLE_CLASS(CPppChapMd5) : public CPppChap
/**
   Class that implements the PPP Challenge Handshake Authentication
   Protocol (CHAP) with MD5 - RFC 1994.
   @internalComponent
*/
	{
  public:
	static CPppAuthentication* NewL();

	virtual ~CPppChapMd5();

	virtual void InitL(CPppLcp* aLcp);

  protected:
	virtual void MakeResponseL(TUint8 aChallengeId, 
							   const TDesC8& aChallengeValue, 
							   TPtrC8& aResponseValue, 
							   TPtrC8& aResponseName);

  private:
	CPppChapMd5();

/* Pointer to the CMD5 object used for computing the CHAP Response
value. */
	CMD5* iMd5;

#ifdef _UNICODE
/* A buffer containing the user name. */
	HBufC8* iUserName;
#endif
	};

inline CPppAuthentication* CPppChapMd5::NewL()
/**
   Object factory for CPppChapMd5.
   @leave Standard Symbian OS error codes. e.g. KErrNoMemory.
   @note This function will not initialize the created object.  The
   InitL member function shall be called first in order to initialize
   the created object before using it.  This behavior is in agreement
   with the requirements for a migration to a plugin architecture.
*/
	{
	return new(ELeave) CPppChapMd5;
	}

#endif
