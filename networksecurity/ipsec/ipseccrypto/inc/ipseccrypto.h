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
//



/**
 @internalComponent
*/
#ifndef __IPSECCRYPTO_H__
#define __IPSECCRYPTO_H__

#include <networking/crypto.h>
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
#include "cryptosymmetriccipherapi.h"
#include <cryptospi/cryptomacapi.h>
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT

typedef enum
	{
	EAlgorithm_Descbc,
	EAlgorithm_3Descbc,
	EAlgorithm_Sha1,
	EAlgorithm_Md5,
	EAlgorithm_Aescbc,
	EAlgorithm_Aesctr,
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	EAlgorithm_AesXcbcMac96,
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
	EAlgorithm_Max,	 // Must be the last entry!!
	} TEayAlgorithm;

class CProtocolEay : public CProtocolCrypto
	{
public:
	CProtocolEay();
	~CProtocolEay() {}      // Nothing to cleanup
	virtual void Identify(TServerProtocolDesc *) const;
	virtual TUint AlgorithmList(TAlgorithmDesc *&aList);
	virtual CryptoSpi::CSymmetricCipher* SymmetricCipherL(TUint anAlg, const TDesC8 &aKey);
	virtual CMessageDigestCrypto* MessageDigest(TUint anAlg);
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT
	virtual CryptoSpi::CMac*  GetMacImplementationL(const TDesC8& aKey);
	HBufC8* CProtocolEay::RetrieveMacValueL(CryptoSpi::CMac *aMac,const TDesC8& aSourceData );
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT
	
	};

#endif
