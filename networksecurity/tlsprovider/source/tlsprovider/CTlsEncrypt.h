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
// This file does the implementation for encryption , decryption, mac computation and PRFL
// Security component: TLS Provider. 
// 
//

/**
 @file 
 @internalTechnology
*/

#ifndef __TLSENCRYPT_H__
#define __TLSENCRYPT_H__


#include <e32std.h>
#include <e32base.h>

#include "tlstypedef.h"
#include "tlsprovider_log.h"


#include "3des.h"
#include "rijndael.h"
#include "cbcmode.h"
#include "padding.h"
#include "blocktransformation.h"
#include "bufferedtransformation.h"
#include "arc4.h"
#include "ct.h"
#include "pkixcertchain.h"
#include "x509keys.h"
#include <random.h>
#include <hash.h>
#include <unifiedkeystore.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif


const TInt KTLSPRFMaxOutputLen = 1024; //not proper value
class TCryptoHolder
{
public:
	CSymmetricCipher* iEncryptor;
	CSymmetricCipher* iDecryptor;
};

class CTlsEncrypt : public CBase
	{
public:
	IMPORT_C static void TLSPRFL( const TDesC8& aSecret,
				 const TDesC8& aLabelAndSeed,
				 const TInt aLen, 
				 TDes8& aOut);

	IMPORT_C static void TLSPRFComputationsL(
		   const TDesC8& aSecret,
		   const TDesC8& aSeed,
		   const TTLSMACAlgorithm& aMacAlg, //sha-1 is default
		   const TInt aLen, //no of bytes to produce
		   TDes8& aOut);

	static CTlsEncrypt* NewL(	TTLSMasterSecretInput& aMasterSecretInput,
								TTLSCipherSuite&  aCipherSuiteId,
								HBufC8* aKeyMaterial,
								CTlsCryptoAttributes* aTlsCryptoAttributes);

	TInt EncryptL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
        TInt64& aSeqNumber,
		TRecordProtocol& aType);
	

	TInt DecryptAndVerifyL(
		const TDesC8& aInput,
		HBufC8*& aOutput,
		TInt64& aSeqNumber,
		TRecordProtocol& aType);

	void CreateEncryptorL();

	void GenerateExportKeysL();

	TUint ComputeMacL(TDes8& aMacbuf,const TDesC8& aData,TBool aIsServerMac,
							  TInt64& aSeqNumber,TRecordProtocol aType);
	~CTlsEncrypt();
private:
		
	TInt iCipherIndex;
	TTLSCipherSuite  iCipherSuiteId;


	CTlsCryptoAttributes* iTlsCryptoAttributes; //Pointer owned by CTlsProviderImpl
	HBufC8* iKeyMaterial; //Pointer owned by CTlsSessionImpl


	TTLSMasterSecretInput iMasterSecretInput;
	TCryptoHolder iCryptos;
		
	TBuf8<32> iClientWriteKey;
	TBuf8<32> iServerWriteKey;
	TBuf8<32> iClientWriteIV;
	TBuf8<32> iServerWriteIV;
	};

#endif

