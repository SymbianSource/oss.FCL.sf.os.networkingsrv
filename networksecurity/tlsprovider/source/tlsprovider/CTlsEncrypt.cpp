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

#include "CTlsEncrypt.h"
#include <nullcipher.h>

//
//  CTlsEncrypt
//

CTlsEncrypt* CTlsEncrypt::NewL(	TTLSMasterSecretInput& aMasterSecretInput,
								TTLSCipherSuite&  aCipherSuiteId,
								HBufC8* aKeyMaterial,
								CTlsCryptoAttributes* aTlsCryptoAttributes)
	{ 
	CTlsEncrypt* aPtrEncrypt = new (ELeave)CTlsEncrypt();
	aPtrEncrypt->iMasterSecretInput = aMasterSecretInput;
	aPtrEncrypt->iCipherSuiteId = aCipherSuiteId;
	aPtrEncrypt->iKeyMaterial = aKeyMaterial;
	aPtrEncrypt->iTlsCryptoAttributes = aTlsCryptoAttributes;
	
	return(aPtrEncrypt);
	}



TInt CTlsEncrypt::EncryptL(const TDesC8& aInput,HBufC8*& aOutput,
								   TInt64& aSeqNumber,TRecordProtocol& aType)
		{
	if(!aInput.Length())
		return KErrBadDescriptor;
	
	TBuf8<128> Macbuf;

	TLSPROV_LOG2(_L("Before Encryption...RecordType: %d"),(TInt)aType)
	
	TLSPROV_LOG_HEX(aInput.Ptr(),aInput.Size() )

	ComputeMacL(Macbuf,aInput,EFalse,aSeqNumber,aType);


	HBufC8* TempInput = HBufC8::NewLC(Macbuf.Length()  + aInput.Length());
	TPtr8 InputPtr = TempInput->Des();

	InputPtr.Append(aInput);
	InputPtr.Append(Macbuf);
	
   TInt nAlloc = iCryptos.iEncryptor->MaxFinalOutputLength(InputPtr.Length());
   if ( !aOutput || aOutput->Des().MaxLength() < nAlloc )
   {
      delete aOutput;
      aOutput = NULL;
	   aOutput = HBufC8::NewL( nAlloc );
   }
	TPtr8 Output = aOutput->Des();
   Output.Zero();

	iCryptos.iEncryptor->ProcessFinalL(InputPtr,Output);

	TLSPROV_LOG(_L("After Encryption with mac"))
	TLSPROV_LOG_HEX(aOutput->Ptr(),aOutput->Size() )

	CleanupStack::PopAndDestroy(TempInput);
	return KErrNone;

	}



TInt CTlsEncrypt::DecryptAndVerifyL(const TDesC8& aInput,HBufC8*& aOutput,
									TInt64& aSeqNumber, TRecordProtocol& aType)
	{

	if(!aInput.Length())
		return KErrBadDescriptor;

	TLSPROV_LOG2(_L("Before Decryption...RecordType: %d"),(TInt)aType)
	
	TLSPROV_LOG_HEX(aInput.Ptr(),aInput.Size() )


   TInt nAlloc = iCryptos.iDecryptor->MaxFinalOutputLength(aInput.Size()) + 24;
   if ( !aOutput || aOutput->Des().MaxLength() < nAlloc )
   {
      delete aOutput;
      aOutput = NULL;
	   aOutput = HBufC8::NewL( nAlloc );
   }
	TPtr8 DecOutput = aOutput->Des();
   DecOutput.Zero();

	TRAP_IGNORE(iCryptos.iDecryptor->ProcessFinalL(aInput,DecOutput));

	TUint HashSize = KSetOfTLSCipherSuites[iCipherIndex].iHashSize;
	
	if(DecOutput.Length() < HashSize)
		{
		return KErrSSLAlertDecryptError;
		}
		
   //set ptr to MAC
	TPtrC8 ReceivedMac = DecOutput.Mid(DecOutput.Length()-HashSize,HashSize);
   //& set length to trim MAC
   DecOutput.SetLength( DecOutput.Length()-HashSize );

	TBuf8<64> CalculatedMac;
	
	ComputeMacL(CalculatedMac,DecOutput,ETrue,aSeqNumber,aType);

	TInt err = KErrBadMAC;	
	if(ReceivedMac.Compare(CalculatedMac) == 0)
		{
		err = KErrNone;
		}
	else
		{
		TLSPROV_LOG(_L("Decryption: Received MAC error"))
		err =  KErrSSLAlertBadRecordMac;
		}

	TLSPROV_LOG(_L("After Decryption , no mac"))
	TLSPROV_LOG_HEX(aOutput->Ptr(),aOutput->Size() )

	return err;
	
	}

void CTlsEncrypt::CreateEncryptorL()
	{

	TLSPROV_LOG(_L("Creating Encryption Decryption Objects"))

	TLSPROV_LOG(_L("Key material:"))
	TLSPROV_LOG_HEX(iKeyMaterial->Ptr(),iKeyMaterial->Size() )


	TTLSBulkCipherAlgorithm ciphAlg = ENullSymCiph;
	TTLSCipherType ciphType = EStream;
	iCipherIndex = 0;
	while(iCipherIndex< TLS_CIPHER_SUITES_NUMBER )  //For every cipher defined
		{
		if( (KSetOfTLSCipherSuites[iCipherIndex].iCipherSuite) == iCipherSuiteId) 
			{
			ciphAlg = KSetOfTLSCipherSuites[iCipherIndex].iBulkCiphAlg;			
			ciphType = KSetOfTLSCipherSuites[iCipherIndex].iCipherType;			
			break;
			}
		iCipherIndex++;
		}

	if (iCipherIndex == TLS_CIPHER_SUITES_NUMBER)
		{
		TLSPROV_LOG3(_L("Not supported CipherSuiteId: LoByte=%d, HiByte=%d"),iCipherSuiteId.iLoByte,iCipherSuiteId.iHiByte)
		User::Leave(KErrCipherNotSupported);
		}

	TInt StartPoint = (KSetOfTLSCipherSuites[iCipherIndex].iHashSize * 2);
	TInt KeyLength	=  KSetOfTLSCipherSuites[iCipherIndex].iKeyMaterial;
	TInt IvLength	=  KSetOfTLSCipherSuites[iCipherIndex].iIVSize;

	TLSPROV_LOG2(_L("KeyMaterial length %d"),iKeyMaterial->Length())
	TLSPROV_LOG2(_L("keyLength  %d:"),KeyLength)
	TLSPROV_LOG2(_L("Hash Size %d:"),(StartPoint/2))
	TLSPROV_LOG2(_L("IvLength  %d:"),IvLength)


	if(iKeyMaterial->Length() < (StartPoint + (KeyLength*2) + (IvLength*2) ))
		User::Leave(KErrArgument);  


	iClientWriteKey = iKeyMaterial->Mid(StartPoint,KeyLength);
	iServerWriteKey = iKeyMaterial->Mid((StartPoint+KeyLength),KeyLength);
	iClientWriteIV 	= iKeyMaterial->Mid((StartPoint + (KeyLength*2)),IvLength);
	iServerWriteIV  = iKeyMaterial->Mid((StartPoint + (KeyLength*2)+IvLength),IvLength);

	TLSPROV_LOG(_L("iClientWriteKey:"))
	TLSPROV_LOG_HEX(iClientWriteKey.Ptr(),iClientWriteKey.Size() )
	TLSPROV_LOG(_L("iServerWriteKey:"))
	TLSPROV_LOG_HEX(iServerWriteKey.Ptr(),iServerWriteKey.Size() )
	TLSPROV_LOG(_L("iClientWriteIV:"))
	TLSPROV_LOG_HEX(iClientWriteIV.Ptr(),iClientWriteIV.Size() )
	TLSPROV_LOG(_L("iServerWriteIV:"))
	TLSPROV_LOG_HEX(iServerWriteIV.Ptr(),iServerWriteIV.Size() )


	if(KSetOfTLSCipherSuites[iCipherIndex].iIsExportable)
		{
		GenerateExportKeysL();
		}

	if(ciphType == EStream)
		{
		switch(ciphAlg)
			{
			case ERc4:
				iCryptos.iEncryptor  = CARC4::NewL(iClientWriteKey,0);		
				iCryptos.iDecryptor  = CARC4::NewL(iServerWriteKey,0);	
				break;
			case ENullSymCiph:
				iCryptos.iEncryptor  = CNullCipher::NewL();		
				iCryptos.iDecryptor  = CNullCipher::NewL();	
				break;
			BULLSEYE_OFF
			default:
				User::Leave(KErrNotSupported);
			BULLSEYE_RESTORE
			}
		}
    else
	   {
       CBlockTransformation* BlockTEncryptor = NULL;
	   CBlockTransformation* BlockTDecryptor = NULL;
       switch (ciphAlg)
       {
	   case EAes:
		   {
		   BlockTEncryptor = CAESEncryptor::NewLC(iClientWriteKey);
		   BlockTEncryptor = CModeCBCEncryptor::NewL(BlockTEncryptor, iClientWriteIV);  
		   CleanupStack::Pop(); // raw BlockTEncryptor
		   CleanupStack::PushL(BlockTEncryptor);
		   
		   
		   BlockTDecryptor = CAESDecryptor::NewLC( iServerWriteKey);
		   BlockTDecryptor = CModeCBCDecryptor::NewL(BlockTDecryptor, iServerWriteIV);
		   CleanupStack::Pop(); // raw BlockTDecryptor
		   CleanupStack::PushL(BlockTDecryptor);
		   }
           break;

	   case EDes:	
	   case EDes40:	   
		   {
		   BlockTEncryptor = CDESEncryptor::NewLC(iClientWriteKey, EFalse);
		   BlockTEncryptor = CModeCBCEncryptor::NewL(BlockTEncryptor, iClientWriteIV);  
		   CleanupStack::Pop(); // raw BlockTEncryptor
		   CleanupStack::PushL(BlockTEncryptor);


		   BlockTDecryptor = CDESDecryptor::NewLC(iServerWriteKey, EFalse);
		   BlockTDecryptor = CModeCBCDecryptor::NewL(BlockTDecryptor, iServerWriteIV);
		   CleanupStack::Pop(); // raw BlockTDecryptor
		   CleanupStack::PushL(BlockTDecryptor);
		   }
           break;

	   case E3Des:
		   {			
		   BlockTEncryptor =   C3DESEncryptor::NewLC(iClientWriteKey );
		   BlockTEncryptor = CModeCBCEncryptor::NewL(BlockTEncryptor, iClientWriteIV);  
		   CleanupStack::Pop(); // raw BlockTEncryptor
		   CleanupStack::PushL(BlockTEncryptor);

		   BlockTDecryptor =   C3DESDecryptor::NewLC(iServerWriteKey );
		   BlockTDecryptor = CModeCBCDecryptor::NewL(BlockTDecryptor, iServerWriteIV);
		   CleanupStack::Pop(); // raw BlockTDecryptor
		   CleanupStack::PushL(BlockTDecryptor);
		   }
		   break;
	   BULLSEYE_OFF
	   default:
		   User::Leave(KErrNotSupported);      
		   break;
	   BULLSEYE_RESTORE
       };

	   CPaddingSSLv3* paddingDe = CPaddingSSLv3::NewLC(BlockTDecryptor->BlockSize());

	   iCryptos.iDecryptor = CBufferedDecryptor::NewL(BlockTDecryptor, paddingDe);
	   CleanupStack::Pop(paddingDe);
	   CleanupStack::Pop(BlockTDecryptor);

	   CPaddingSSLv3* paddingEn = CPaddingSSLv3::NewLC(BlockTEncryptor->BlockSize());
	   
	   iCryptos.iEncryptor = CBufferedEncryptor::NewL(BlockTEncryptor, paddingEn);
	   CleanupStack::Pop(paddingEn);
	   CleanupStack::Pop(BlockTEncryptor);

	   }	

	return;     
}


TUint CTlsEncrypt::ComputeMacL(TDes8& aMacbuf,const TDesC8& aData,TBool aIsServerMac,
							  TInt64& aSeqNumber,TRecordProtocol aType)
	{
	TPtrC8 MacSecret;
	
	TInt HashLength = (KSetOfTLSCipherSuites[iCipherIndex].iHashSize);

	TLSPROV_LOG(_L("Computing MAC:"))

	if(aIsServerMac)
		MacSecret.Set(iKeyMaterial->Mid(HashLength,HashLength));
	else
		MacSecret.Set(iKeyMaterial->Mid(0,HashLength));

	CMessageDigest *dig=NULL;
	CMessageDigest *dig2=NULL;
	switch(KSetOfTLSCipherSuites[iCipherIndex].iMacAlg)
		{
		case EMd5:
			if((iTlsCryptoAttributes->iNegotiatedProtocol) == KSSL3_0)
				{
				dig2 =CMD5::NewL();
				CleanupStack::PushL(dig2);
				}
			dig =CMD5::NewL();
			CleanupStack::PushL(dig);
			break;
		case ESha:
			if(iTlsCryptoAttributes->iNegotiatedProtocol == KSSL3_0)
				{
				dig2 =CSHA1::NewL();
				CleanupStack::PushL(dig2);
				}
			dig =CSHA1::NewL();
			CleanupStack::PushL(dig);
			break;
		BULLSEYE_OFF
		default:
			User::Leave(KTLSErrBadArgument);
			break;
		BULLSEYE_RESTORE
		
		};

	TBuf8<20> hash;
	if(iTlsCryptoAttributes->iNegotiatedProtocol == KTLS1_0)
		{

		CHMAC* hmacdig = CHMAC::NewL(MacSecret,dig);
		CleanupStack::Pop(dig);
		CleanupStack::PushL(hmacdig);
		// sequence number
		TUint8 seqNum[8];
		TInt i;
		TInt64 tmp;
		tmp=aSeqNumber;
		for (i=7;i>=0;i--)
			{
			seqNum[i]=(TUint8)(tmp%0x100);
			tmp>>=8;
			}
		
		TPtrC8 seqptr(seqNum,8);

		// compressed type
		TUint8 type=TUint8(aType); 
		TPtrC8 typeptr(&type,1);
		//compressed version
		TBuf8<2> vers;
		vers.SetMax();
		vers[0]=(TUint8)(iTlsCryptoAttributes->iNegotiatedProtocol).iMajor;
		vers[1]=(TUint8)(iTlsCryptoAttributes->iNegotiatedProtocol).iMinor;
		// compressed length
		TBuf8<2> lenptr;
		lenptr.SetMax();
		lenptr[0]=(TUint8)(aData.Length()>>8);
		lenptr[1]=(TUint8)(aData.Length());

		hmacdig->Hash(seqptr);
		hmacdig->Hash(typeptr);
		hmacdig->Hash(vers);
		hmacdig->Hash(lenptr);
		// compressed fragment
		aMacbuf.Copy(hmacdig->Hash(aData));

		//Note, ownership of dig is passed to hmacdig, no need to delete it
		CleanupStack::PopAndDestroy(hmacdig);
		}
	else if((iTlsCryptoAttributes->iNegotiatedProtocol) == KSSL3_0)
		{

		dig->Hash(MacSecret);
		// sequence number
		TUint8 seqNum[8];
		TInt i;
		TInt64 tmp;
		tmp=aSeqNumber;
		for (i=7;i>=0;i--)
			{
			seqNum[i]=(TUint8)(tmp%0x100);
			tmp>>=8;
			}
		
		TPtrC8 seqptr(seqNum,8);
		// pad1
		TUint8 pad[64];
		TInt padlen;
		if(KSetOfTLSCipherSuites[iCipherIndex].iMacAlg == EMd5)
			padlen=48;
		else
			padlen=40;
		for(i=0;i<padlen;i++) pad[i]=KIpad;

		TPtr8 pad1(pad,padlen);
		pad1.SetMax();
		// compressed type
		TUint8 type=TUint8(aType); 
		TPtrC8 typeptr(&type,1);
		// compressed length
		TBuf8<2> lenptr;
		lenptr.SetMax();
		lenptr[0]=(TUint8)(aData.Length()>>8);
		lenptr[1]=(TUint8)(aData.Length());

		dig->Hash(pad1);
		dig->Hash(seqptr);
		dig->Hash(typeptr);
		dig->Hash(lenptr);
		// compressed fragment
		hash.Copy(dig->Hash(aData));
		// pad2
		for( i=0;i<padlen;i++) pad[i]=KOpad;
		TPtrC8 pad2(pad,padlen);
		dig2->Hash(MacSecret);
		dig2->Hash(pad2);
		aMacbuf.Copy(dig2->Hash(hash));
		CleanupStack::PopAndDestroy(2,dig2);

	}
	
	return hash.Length();
	}



void CTlsEncrypt::GenerateExportKeysL()
	{
	TLSPROV_LOG(_L("Generating EXPORT keys:"))

	
	const TTLSCipherSuiteMapping* cipherDetails = iTlsCryptoAttributes->iCurrentCipherSuite.CipherDetails();
	
	if(NULL == cipherDetails)
		{
		// Unable to retrieve cipher details
		User::Leave(KErrGeneral);
		}

	if(iTlsCryptoAttributes->iNegotiatedProtocol == KTLS1_0)
		{
		TBuf8<64> tmpsecret;
		TBuf8<128> seed;

		TInt expandedKeySize = cipherDetails->iExpKeySize;
		
		tmpsecret.Copy(iClientWriteKey);
		seed.Copy(_L8("client write key"));
		seed.Append(iMasterSecretInput.iClientRandom);
		seed.Append(iMasterSecretInput.iServerRandom);		
		TLSPRFL( tmpsecret,seed,expandedKeySize,iClientWriteKey);
		

		tmpsecret.Copy(iServerWriteKey);
		seed.Copy(_L8("server write key"));
		seed.Append(iMasterSecretInput.iClientRandom);
		seed.Append(iMasterSecretInput.iServerRandom);
		TLSPRFL( tmpsecret,seed,expandedKeySize,iServerWriteKey);

		tmpsecret.Copy(_L8(""));
		seed.Copy(_L8("IV block"));
		seed.Append(iMasterSecretInput.iClientRandom);
		seed.Append(iMasterSecretInput.iServerRandom);

		TInt ivSize = cipherDetails->iIVSize;

		TBuf8<64> tmpIV;
		TLSPRFL(tmpsecret,seed,ivSize*2,tmpIV);
		iClientWriteIV=tmpIV.Left(ivSize);	
		iServerWriteIV=tmpIV.Right(ivSize);	
	
		return;
		}
	else
		{
		CMD5* digMD5=CMD5::NewL();
		digMD5->Hash(iClientWriteKey);	
		digMD5->Hash(iMasterSecretInput.iClientRandom);	
		iClientWriteKey.Copy(digMD5->Hash(iMasterSecretInput.iServerRandom));
		iClientWriteKey.SetLength(cipherDetails->iExpKeySize);
		delete digMD5;

		digMD5=CMD5::NewL();
		digMD5->Hash(iServerWriteKey);	
		digMD5->Hash(iMasterSecretInput.iServerRandom);	
		iServerWriteKey.Copy(digMD5->Hash(iMasterSecretInput.iClientRandom));
		iServerWriteKey.SetLength(cipherDetails->iExpKeySize);
		delete digMD5;

		digMD5=CMD5::NewL();
		digMD5->Hash(iMasterSecretInput.iClientRandom);	
		iClientWriteIV.Copy(digMD5->Hash(iMasterSecretInput.iServerRandom));
		iClientWriteIV.SetLength(8);		
		delete digMD5;

		digMD5=CMD5::NewL();
		digMD5->Hash(iMasterSecretInput.iServerRandom);	
		iServerWriteIV.Copy(digMD5->Hash(iMasterSecretInput.iClientRandom));
		iServerWriteIV.SetLength(8);		
		delete digMD5;
		}
	}


EXPORT_C void CTlsEncrypt::TLSPRFL( const TDesC8& aSecret,
				 const TDesC8& aLabelAndSeed,
				 const TInt aLen, 
				 TDes8& aOut)
	{	
	
	if ( aLen > aOut.MaxLength() )
		{
		User::Leave( KTLSErrBadArgument );
		}

	aOut.SetLength( aLen );

	TInt half = (aSecret.Length() / 2) + (aSecret.Length() % 2);

	TBuf8<KTLSPRFMaxOutputLen> output2;
	
	CTlsEncrypt::TLSPRFComputationsL( aSecret.Left( half ), aLabelAndSeed, EMd5, aLen, aOut );
	CTlsEncrypt::TLSPRFComputationsL( aSecret.Right( half ) , aLabelAndSeed, ESha, aLen, output2 );

	// Xor:
	TInt i = 0;
	for( i=0; i < aLen; i++)
		aOut[i]^=output2[i];

	} 


EXPORT_C void CTlsEncrypt::TLSPRFComputationsL(
		   const TDesC8& aSecret,
		   const TDesC8& aSeed,
		   const TTLSMACAlgorithm& aMacAlg, //sha-1 is default
		   const TInt aLen, //no of bytes to produce
		   TDes8& aOut) // output buffer
	{
	
	if ( aLen > aOut.MaxLength() )
		{		
		User::Leave( KTLSErrBadArgument );
		}
	
	CMessageDigest::THashId hashId = CMessageDigest::ESHA1;
	
	switch ( aMacAlg )
	{
	case EMd5:
		hashId = CMessageDigest::EMD5;
		break;
	BULLSEYE_OFF
	default:
		break;
	BULLSEYE_RESTORE
	};
	
	CMessageDigest* digest1 = NULL;     	
	digest1 = CMessageDigestFactory::NewDigestL(hashId);
	CleanupStack::PushL( digest1 );

	CHMAC* hmac1 = CHMAC::NewL( aSecret, digest1 );
	CleanupStack::Pop( digest1 );
	CleanupStack::PushL( hmac1 );
	
	CMessageDigest* digest2 = NULL;
	digest2 = CMessageDigestFactory::NewDigestL(hashId);
	CleanupStack::PushL( digest2 );
	
	CHMAC* hmac2 = CHMAC::NewL( aSecret, digest2 );
	CleanupStack::Pop( digest2 );
	CleanupStack::PushL( hmac2 );
	
	aOut.Zero();
				
	hmac2->Update( aSeed );
	TBuf8<KTLSPRFMaxOutputLen> A_i (hmac2->Final()); 	
	
	do
	{
		hmac1->Reset();

		hmac1->Update( A_i );
		hmac1->Update( aSeed );
		TPtrC8 tmpHash = hmac1->Final();  
	
		hmac2->Reset();
		hmac2->Update( A_i );
		A_i.Copy( hmac2->Final() );
		
		if(  aLen - aOut.Length() > tmpHash.Length() )
			aOut.Append( tmpHash );
		else 
			aOut.Append( tmpHash.Ptr() , aLen - aOut.Length() );
		
	}
	while( aOut.Length() < aLen );     
				
	aOut.SetLength( aLen );       
	
	CleanupStack::PopAndDestroy(2,hmac1);
	
	} 


CTlsEncrypt::~CTlsEncrypt()
	{
	delete iCryptos.iEncryptor;
	delete iCryptos.iDecryptor;
	}
