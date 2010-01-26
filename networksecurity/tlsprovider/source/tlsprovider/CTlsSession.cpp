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
// CTlsSessionImpl.CPP
// 
//

#include "tlsprovider.h"
#include "CTlsEncrypt.h"
#include <ecom/ecom.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif

//
//  CTlsSessionImpl
//

CTlsSessionImpl::CTlsSessionImpl():
   CActive(0),
   iTempPtr((TUint8 *) 0, (TInt)0 )
	{
	if(CActiveScheduler::Current()) //Already installed
		{
		CActiveScheduler::Add( this );
		}
	}


CTlsSessionImpl* CTlsSessionImpl::NewL(
		MTLSSession* aSessionInterface,
		CCTCertInfo* aSelectedCertInfo,
		CCTKeyInfo* aSelectedKeyInfo,
		RPointerArray<CCertificate>* aStoredIntermediatesCACertificates)
	{ 
	CTlsSessionImpl* aPtrSession = new (ELeave)CTlsSessionImpl();	
	
	aPtrSession->iSessionInterface   = aSessionInterface;	
	aPtrSession->iSelectedCertInfo = aSelectedCertInfo;
	aPtrSession->iSelectedKeyInfo = aSelectedKeyInfo;
	aPtrSession->iStoredIntermediatesCACertificates = aStoredIntermediatesCACertificates;

	return(aPtrSession);
	}

void CTlsSessionImpl::ClientKeyExchange(		
		HBufC8*& aClientKeyExch,			
		TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::ClientKeyExchange()..."))

	iClientKeyExch = &aClientKeyExch;
	iOriginalState = iCurrentState=EGetClientKeyExchange;
	iNextState = EKeyGeneration;
	iStatus = KRequestPending;

	iSessionInterface->ClientKeyExchange(iMasterSecretInput,
										 iTlsCryptoAttributes->iProposedProtocol,
										 (iEncodedServerCerts) ? (iEncodedServerCerts->Des()) : (TPtr8(0,0)),
										 iTlsCryptoAttributes->iPublicKeyParams,
										 (*iClientKeyExch),iStatus);
	SetActive();
	return;	
	}




void CTlsSessionImpl::ClientCertificate(HBufC8*& aEncodedClientCert,TRequestStatus& aStatus)		
	{

	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;

	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::ClientCertificate()...Encoded form"))

	if(iOriginalState != EGetClientCerificateX509 && iOriginalState != EGetClientCertificateArray)
		iOriginalState = EGetClientCerificate;	
		
	iCurrentState=EGetClientCerificate;	
	iStatus = KRequestPending;
	
	iEncodedClientCertHldrPtr = &aEncodedClientCert;	
	
	if(iSelectedCertInfo || !iAbbrievatedHandshake)
		{		
		TRequestStatus* MyStatus = &iStatus;
		User::RequestComplete(MyStatus,KErrNone);
		}
	else
		{//info from cache => don't release	
		iSessionInterface->ClientCertificate(iSelectedCertInfo,iStatus);		
		}
	SetActive();
	return;	
	}



void CTlsSessionImpl::ClientCertificate(CX509Certificate*& aX509ClientCert,TRequestStatus& aStatus)		
	{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;

	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::ClientCertificate()...x509 format"))

	iOriginalState = EGetClientCerificateX509;
	aX509ClientCert= 0;
	iClientCertX509 = &aX509ClientCert;
   delete iEncodedClientCert;
   iEncodedClientCert = NULL;
	ClientCertificate(iEncodedClientCert,aStatus);	
	return;
	}

void CTlsSessionImpl::ClientCertificate(RPointerArray<HBufC8>* aClientCertArray, TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::ClientCertificate()...Array format"))

	iOriginalState = EGetClientCertificateArray;
	iClientCertArray = aClientCertArray;
	iClientCertArray->ResetAndDestroy();
	delete iEncodedClientCert;
	iEncodedClientCert = NULL;
	ClientCertificate(iEncodedClientCert,aStatus);
	return;	
	}


/*The following function gets the encoded certificate and converts it into
an x509 object..Only called from a direct secure socket API call	
*/
void CTlsSessionImpl::GetX509CertL(HBufC8*& aEncodedCert,CX509Certificate*& aOutputX509)
	{

	if(aEncodedCert)
		{
		CPKIXCertChain* CertsChain = CPKIXCertChain::NewLC(iFs,aEncodedCert->Des(),
												  TUid::Uid(KUidUnicodeSSLProtocolModule));
		if(CertsChain->Count())
			{
			aOutputX509 = CX509Certificate::NewL(CertsChain->Cert(0));		
			}
		CleanupStack::PopAndDestroy(CertsChain);	
		}
	}




void CTlsSessionImpl::ServerCertificate(CX509Certificate*& aX509ServerCert, TRequestStatus& aStatus)		
	{

	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}

	TLSPROV_LOG(_L("CTlsSessionImpl::ServerCertificate()...x509 format"))
	iCurrentState = EGetServerCertificate;
	iOriginalState = EGetServerCertificate;
	iX509ServerCert = &aX509ServerCert;		

	if(iEncodedServerCerts)
		{
		TRAPD(err,GetX509CertL(iEncodedServerCerts,(*iX509ServerCert)) );
		if(*iX509ServerCert)
			User::RequestComplete(iOriginalRequestStatus,KErrNone);	
		else
			User::RequestComplete(iOriginalRequestStatus,(!err?KErrNoCertsAvailable:err));
		iX509ServerCert = 0;
		}
	else 
		{
		iStatus = KRequestPending;
		iSessionInterface->ServerCertificate(iServerCert_rv,iStatus);
		SetActive();
		}	
	}


	

void CTlsSessionImpl::CertificateVerifySignatureL(
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::CertificateVerifySignature()..."))
	
	iComputeDigitalSig = &aOutput;
	TInt err = KErrNotSupported;
   delete iTempHolder;
   iTempHolder = NULL;
	if(iProtocolVersion == KSSL3_0) //SSL
		{
   	TLSPROV_LOG(_L("KSSL3_0..."))
   	iCurrentState = ECertificateVerifyMsg;
		iStatus = KRequestPending;		
		iSessionInterface->PHash(aMd5DigestInput,aShaDigestInput,iTempHolder,iSessionInterface->ECertificateVerifyOp,iStatus);
		SetActive();
		return;
		}
	else if(iProtocolVersion == KTLS1_0) //TLS
		{
   	TLSPROV_LOG(_L("KTLS1_0..."))
		TInt length = aMd5DigestInput->HashSize()+aShaDigestInput->HashSize();
      iTempHolder = HBufC8::NewL( length );
      iTempPtr.Set( iTempHolder->Des() );
      if ( Attributes()->isignatureAlgorithm == ERsaSigAlg )
         {
		   iTempPtr.Copy(aMd5DigestInput->Hash(KNullDesC8));
         }
   	iTempPtr.Append(aShaDigestInput->Hash(KNullDesC8));		

		if( (NULL != iSelectedCertInfo) && (NULL != iSelectedKeyInfo) )
         {
			iSessionInterface->ComputeDigitalSignature(iTempPtr,(*iComputeDigitalSig),*iSelectedCertInfo,*iSelectedKeyInfo,*iOriginalRequestStatus);		
   		return;
         }
			
		}
	User::RequestComplete(iOriginalRequestStatus, err);
	}



/**
This asynchronous method generates a SSL/TLS protocol's Client 'Finished' message. 
This input for this message  is a hash of the concatenation of all the handshake messages 
exchanged thus far (as specified by RFC2246 and SSL3.0 specification). 
In order to create the required output, 
	TLS Protocol:
	"client finished" + iMd5DigestInput + iShaDigestInput

	SSL Protocol:
	(iMd5DigestInput +"CLNT") + (iShaDigestInput +"CLNT")	

@param aMd5DigestInput Md5 hash of Handshake message
@param aShaDigestInput Md5 hash of Handshake message
@param aOutput Client's 'Finished' message  
@param aIsServer asynchronous request status set on the completion 
@return void Asynchronous function
@publishedPartner
*/

void CTlsSessionImpl::GenerateFinishedMessageL(CMessageDigest* aMd5DigestInput,
										  CMessageDigest* aShaDigestInput,
										  HBufC8*& aOutput,
										  TBool aIsServer)
	{
	HBufC8* Input = 0;
	
	unsigned char* label;
	if(aIsServer)
		label=(unsigned char *)"SRVR";
	else
		label=(unsigned char *)"CLNT";

	TPtrC8 client(label,4);

	if(iProtocolVersion == KSSL3_0) //SSL
		{		
		aMd5DigestInput->Hash(client);
		aShaDigestInput->Hash(client);	
		iStatus = KRequestPending;
		iSessionInterface->PHash(aMd5DigestInput,aShaDigestInput,aOutput,iSessionInterface->EServerFinishedOp,iStatus);
		SetActive();
		return;				
		}
	else if(iProtocolVersion == KTLS1_0) //TLS
		{
		
		TInt length;
		TBuf8<64> Seed;
		Seed.Copy(aMd5DigestInput->Hash(KNullDesC8));
		TPtrC8 PtSeed(aShaDigestInput->Hash(KNullDesC8));
		Seed.Append(PtSeed);		

		length =	(aIsServer?KTLSServerFinishedLabel.Length():KTLSClientFinishedLabel.Length())
					+aMd5DigestInput->HashSize()
					+aShaDigestInput->HashSize();

		TRAPD(err, ( Input = HBufC8::NewL(length) ) );
				
		if(!err)
			{
			CleanupStack::PushL(Input);
			TPtr8 tempPtr = Input->Des();
			(aIsServer?tempPtr.Append(KTLSServerFinishedLabel):tempPtr.Append(KTLSClientFinishedLabel));
			
			tempPtr.Append(Seed);
			iStatus = KRequestPending;

			iSessionInterface->PHash(Input->Des(),
									 aOutput,
									 (aIsServer?iSessionInterface->EServerFinishedOp:iSessionInterface->EClientFinishedOp),
									 iStatus);
			SetActive();	
			CleanupStack::PopAndDestroy(Input);			
			return;	
			}		
				
		User::RequestComplete(iOriginalRequestStatus, err);
		}
	}

void CTlsSessionImpl::ClientFinishedMsgL(		
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,
		HBufC8*& aOutput, 
		TRequestStatus& aStatus) 
	{

	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::ClientFinishedMsg()..."))
	
	iCurrentState = EClientFinishedMsg;
	GenerateFinishedMessageL(aMd5DigestInput,aShaDigestInput,aOutput,EFalse);	
	}



void CTlsSessionImpl::VerifyServerFinishedMsgL(			
		CMessageDigest* aMd5DigestInput,
		CMessageDigest* aShaDigestInput,	
		const TDesC8& aActualFinishedMsg,  
		TRequestStatus& aStatus)
	{	
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
	TLSPROV_LOG(_L("CTlsSessionImpl::VerifyServerFinishedMsg()..."))

	
	iCurrentState = EVerifyServerFinishedMsg;	
	iActualFinishedMsg = aActualFinishedMsg.AllocL();
	GenerateFinishedMessageL(aMd5DigestInput,aShaDigestInput,iServerFinished,ETrue);	
	}


CTlsCryptoAttributes* CTlsSessionImpl::Attributes() 
		{
		return iTlsCryptoAttributes;		
		}


TInt CTlsSessionImpl::EncryptL(const TDesC8& aInput,HBufC8*& aOutput,
								   TInt64& aSeqNumber,TRecordProtocol& aType)
	{
	if(!iEncrypt)
		User::Leave(KErrNotReady);
	return (iEncrypt->EncryptL(aInput,aOutput,aSeqNumber,aType));
	}


TInt CTlsSessionImpl::DecryptAndVerifyL(const TDesC8& aInput,HBufC8*& aOutput,
											 TInt64& aSeqNumber, TRecordProtocol& aType)
	{
	if(!iEncrypt)
		User::Leave(KErrNotReady);
	return (iEncrypt->DecryptAndVerifyL(aInput,aOutput,aSeqNumber,aType));
	}



/*
	Full Handshake
	==============
	This function implements the full handshake protocol which exchanges (via PKI) the
	key used to encrypt/decrypt the symmetrical encoded data stream that follows the
	handshake. Full PKI protocols are not used for the remaining data stream as it is
	too computationally expensive!
*/		
void CTlsSessionImpl::ConstructL(		
		CTlsCryptoAttributes* aTlsCryptoAttributes, 
		HBufC8*  aEncodedServerCerts,					
		TRequestStatus& aStatus)
	{
	if(IsActive())
		{
		TRequestStatus *clientRequest = &aStatus;
		User::RequestComplete(clientRequest, KErrInUse);
		return;
		}

	iOriginalState =  EConstruct;
	User::LeaveIfError(iFs.Connect());
    iAbbrievatedHandshake = EFalse;
	iProtocolVersion = aTlsCryptoAttributes->iNegotiatedProtocol;
	iCipherSuiteId = aTlsCryptoAttributes->iCurrentCipherSuite;
	iMasterSecretInput = aTlsCryptoAttributes->iMasterSecretInput;
	iEncodedServerCerts = aEncodedServerCerts;
	iSessionInterface->InitL(aTlsCryptoAttributes->iSessionNameAndID,
							 aTlsCryptoAttributes->iCurrentCipherSuite,
							 aTlsCryptoAttributes->iCompressionMethod,
							 iProtocolVersion,
							 EFalse);

	iTlsCryptoAttributes = aTlsCryptoAttributes;
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	User::RequestComplete(iOriginalRequestStatus,KErrNone);
	// We are returning a ptr so can now take ownership of stuff passed to NewL and this function...
	iConstructionComplete = ETrue;
	return;	
	}

void CTlsSessionImpl::BuildClientIntermediateCertChainL(RPointerArray<CCertificate> &aCertChain,
														const CX509Certificate* aClientCert) const
{
	aCertChain.Reset();
	TInt intermediateCertsCount = iStoredIntermediatesCACertificates->Count();	
	if (intermediateCertsCount == 0)
		{
		return;
		}
		
	TLSPROV_LOG(  _L("CTlsSessionImpl::BuildClientIntermediateCertChainL	(iEncodedClientCertHldr) (no length prepended):") )
	TLSPROV_LOG_HEX( (*iEncodedClientCertHldrPtr)->Des().Ptr(), (*iEncodedClientCertHldrPtr)->Size() )
			
	const TPtrC8* currIssuerName = aClientCert->DataElementEncoding(CX509Certificate::EIssuerName);
	const CX509Certificate *currentCert = aClientCert;
				
	// starting from selected Client Certificate, repeatedly interate through 
	// an array in order to detect which certificate was used for current cert's 
	// signature until there are no matches between Subject and Issuer names 
	TBool issuerFound = ETrue;

	while (issuerFound)
		{
		issuerFound = EFalse;
		for (TInt i = 0; i < intermediateCertsCount; ++i)
			{
			CX509Certificate *x509Cert = (CX509Certificate*)((*iStoredIntermediatesCACertificates)[i]);
			const TPtrC8* subjectName = x509Cert->DataElementEncoding(CX509Certificate::ESubjectName);
			
			TLSPROV_LOG_HEX(subjectName->Ptr(),subjectName->Size())
					
			if ( subjectName->Compare(*currIssuerName) != 0 )
				{
				continue;
				}

			TLSPROV_LOG(_L("Issuer is found, verifying signature..."))
			const CSubjectPublicKeyInfo& publicKey = x509Cert->PublicKey();
			if (currentCert->VerifySignatureL(publicKey.KeyData()) == EFalse)
				{
				TLSPROV_LOG(_L("Match failed"));
				continue;
				}

			TLSPROV_LOG(_L("Signature is verified successfully"))
				
			// Issuer is found
			// Assign new currIssuerName and store current certificate
			aCertChain.Append(x509Cert);
			currIssuerName = x509Cert->DataElementEncoding(CX509Certificate::EIssuerName);
			currentCert = x509Cert;
						
			const TPtrC8* issuerName  = x509Cert->DataElementEncoding(CX509Certificate::EIssuerName);
			if ( subjectName->Compare(*issuerName) == 0 )
				{
				TLSPROV_LOG( _L("Match is SelfSigned Certificate") )
				return;
				}
			issuerFound = ETrue;
			break;				
			}
		}	
}

TBool CTlsSessionImpl::MatchRequestedIssuerDN(const CCertificate* aCert) const
{
	const TPtrC8* certIssuerName = aCert->DataElementEncoding(CX509Certificate::EIssuerName);
	for (int i = 0; i < iTlsCryptoAttributes->iDistinguishedCANames.Count(); ++i)
		{
		if ( *(iTlsCryptoAttributes->iDistinguishedCANames[i]) == *certIssuerName )
			{
			return ETrue;
			}
		}
	return EFalse;	
}


/*
	Abbreviated Handshake
	=====================
	This function implements the 'abbreviated' handshake protocol. This is slightly
	different to the full handshake in that it uses any existing connection session
	and also, the client certificate is cached within the session context. This allows
	a much quicker response as the session is not re-established, the cipher suite used
	is not re-negotiated and the client authentication information is already established.
*/		
void CTlsSessionImpl::ConstructResumedL(
		CTlsCryptoAttributes* aTlsCryptoAttributes,		
		TRequestStatus& aStatus) 
	{
	if(IsActive())
		{
		TRequestStatus *clientRequest = &aStatus;
		User::RequestComplete(clientRequest, KErrInUse);
		return;
		}

   iAbbrievatedHandshake = ETrue;
	User::LeaveIfError(iFs.Connect());
	iProtocolVersion = aTlsCryptoAttributes->iNegotiatedProtocol;
	iCipherSuiteId = aTlsCryptoAttributes->iCurrentCipherSuite;
	iMasterSecretInput = aTlsCryptoAttributes->iMasterSecretInput;
	
	iSessionInterface->InitL(aTlsCryptoAttributes->iSessionNameAndID,
							 aTlsCryptoAttributes->iCurrentCipherSuite,
							 aTlsCryptoAttributes->iCompressionMethod,
							 iProtocolVersion,
							 ETrue);
	iCurrentState = EGetClientKeyExchange;
	iNextState=EKeyGeneration;
	
	iTlsCryptoAttributes = aTlsCryptoAttributes;

	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	TRequestStatus* MyStatus = &iStatus;
	User::RequestComplete(MyStatus,KErrNone);			
	SetActive();
	// We are returning a ptr so can now take ownership of stuff passed to NewL and this function...
	iConstructionComplete = ETrue;
	return;
	}



void CTlsSessionImpl::GenerateKeysL()
	{
	//Call PHash for Key block generation...

   delete iTempHolder;
   iTempHolder = NULL;
	iTempHolder = HBufC8::NewL(
			((iProtocolVersion == KTLS1_0)?KTLSKeyExpansionLabel.Length():0)
			+ (KTLSServerClientRandomLen*2)	);


	iTempPtr.Set( iTempHolder->Des() );
	if(iProtocolVersion == KTLS1_0) 
		iTempPtr.Append(KTLSKeyExpansionLabel);

	iTempPtr.Append(iMasterSecretInput.iServerRandom);
	iTempPtr.Append(iMasterSecretInput.iClientRandom);


	iCurrentState = EKeyGeneration;		
	iStatus = KRequestPending;
	iSessionInterface->PHash(iTempPtr,iKeyMaterial,iSessionInterface->EKeyBlockOp,iStatus);
	SetActive();
	return;	
	}



void CTlsSessionImpl::RunL()
{
	if(iStatus.Int() != KErrNone)
		User::Leave(iStatus.Int());

	switch(iCurrentState)
	{

	case EGetClientKeyExchange:
		{
		GenerateKeysL();
		}
		break;
		

	case EKeyGeneration:
		{
		//Create Encryption, Decryption and hash objects	
		iEncrypt = CTlsEncrypt::NewL(iMasterSecretInput,iTlsCryptoAttributes->iCurrentCipherSuite,iKeyMaterial,iTlsCryptoAttributes);
		iEncrypt->CreateEncryptorL();
		iClientKeyExch = 0; // Finished create the key exchange msg, ownership passes to caller
		User::RequestComplete(iOriginalRequestStatus,KErrNone);
		}
		break;


	case EGetServerCertificate:
		{
		//iServerCert_rv is the actual pointer to the cert stored in token..so dont delete it
		TRAPD(err,GetX509CertL(iServerCert_rv,(*iX509ServerCert)) );
		if(*iX509ServerCert)
			User::RequestComplete(iOriginalRequestStatus,KErrNone);
		else
			User::RequestComplete(iOriginalRequestStatus,(!err?KErrNoCertsAvailable:err));				
	
		}
		break;
		

	case EComputeDigitalSignature:
		{
		User::RequestComplete(iOriginalRequestStatus,iStatus.Int());
		}
		break;

	case EReturnCert:
		{			
		TInt err = KErrNone;
		if(iOriginalState == EGetClientCerificateX509)
			{					
			TRAP(err,GetX509CertL(*iEncodedClientCertHldrPtr,(*iClientCertX509)));
			iOriginalState = ENullState;
			if(!(*iClientCertX509) || err !=KErrNone)
				{
				(!err)?(err =KErrNoCertsAvailable):err;					
				}
			iEncodedClientCertHldrPtr = 0;
			}
		else
			{
			// First, check whether we need to build a chain at all. If the client certificate's Issuer DN
			// is equal to any of the ones requested by the server, we do not need to build a chain
			CX509Certificate* clientCert = CX509Certificate::NewLC((*iEncodedClientCertHldrPtr)->Des());			
			if(iOriginalState == EGetClientCertificateArray)
				{
				HBufC8* cert = (*iEncodedClientCertHldrPtr)->AllocL();
				iClientCertArray->Append(cert);
				}
			if (MatchRequestedIssuerDN(clientCert) == EFalse)
				{
				// Client certificate's issuer DN does not match any of the DNs requested by the server
				// We need to build a certificates chain, and see if we can get a matching issuer DN there

				RPointerArray<CCertificate> intermediateCerts;
				CleanupClosePushL(intermediateCerts);	
				BuildClientIntermediateCertChainL(intermediateCerts, clientCert);				
				TInt intermediateCertsCount = intermediateCerts.Count();
				TBool isDNFound = intermediateCertsCount ? MatchRequestedIssuerDN(intermediateCerts[intermediateCertsCount - 1]) : EFalse;
				if ( isDNFound )
					{
					// We found a matching issuer DN in the intermediate chain, so we'll rebuild
					// the certificate we send to the server to include the chain
					if(iOriginalState == EGetClientCertificateArray)
						{
						for (TInt i = 0; i < intermediateCertsCount; ++i)
							{
							const TPtrC8 enc = ((CX509Certificate*)intermediateCerts[i])->Encoding();
							HBufC8* cert = enc.AllocL();
							iClientCertArray->Append(cert);
							}
						}
					else
						{
						TInt newLen = (*iEncodedClientCertHldrPtr)->Length();
						for (TInt i = 0; i < intermediateCertsCount; ++i)
							{
							const TPtrC8 enc = ((CX509Certificate*)intermediateCerts[i])->Encoding();
							newLen += enc.Length();
							}
						*iEncodedClientCertHldrPtr =
							(*iEncodedClientCertHldrPtr)->ReAllocL(newLen);
						for (TInt i = 0; i < intermediateCertsCount; ++i)
							{
							const TPtrC8 enc = ((CX509Certificate*)intermediateCerts[i])->Encoding();
							(*iEncodedClientCertHldrPtr)->Des().Append(enc);
							}
						}					
					}
				CleanupStack::PopAndDestroy();	// intermediateCerts
				}
			CleanupStack::PopAndDestroy(); // clientCert
			iEncodedClientCertHldrPtr = NULL;
			}
					
		User::RequestComplete(iOriginalRequestStatus,err);	
		iOriginalState = iCurrentState = ENullState;
		}
		break;

	case EGetClientCerificate:
		{
		if(!iSelectedCertInfo)
			{
			User::RequestComplete(iOriginalRequestStatus, KErrNone);  //No certs found, an empty certificate will be returned..Still a successfull call
			iOriginalState = iCurrentState = ENullState;
			return;
			}

		if(!iPtrUnifiedCertStore)
			{
			iPtrUnifiedCertStore = CUnifiedCertStore::NewL(iFs,EFalse);
			iStatus = KRequestPending;			
			iPtrUnifiedCertStore->Initialize(iStatus);
			SetActive();
			return;	
			}


		*iEncodedClientCertHldrPtr = HBufC8::NewMaxL(iSelectedCertInfo->Size());
		iTempPtr.Set( (*iEncodedClientCertHldrPtr)->Des() );
			

		iStatus = KRequestPending;
		iCurrentState = EReturnCert;			
		iPtrUnifiedCertStore->Retrieve((*iSelectedCertInfo),iTempPtr,iStatus);	
		SetActive();
		}		
		break;

	case EClientFinishedMsg:
		{		
		User::RequestComplete(iOriginalRequestStatus,iStatus.Int());
		}
		break;
	
	case EVerifyServerFinishedMsg:
		{

		iServerMsgVerified = iStatus.Int();	
		if(!iServerMsgVerified && (iActualFinishedMsg->Compare(iServerFinished->Des()) != 0))
			{
			iServerMsgVerified = KErrBadServerFinishedMsg;
			TLSPROV_LOG(_L("CTlsSessionImpl... Verify ServerFinishedMessage Failed..."))
			}
		else
			{
			iServerMsgVerified =  KErrNone;
			TLSPROV_LOG(_L("CTlsSessionImpl... Successfully verified ServerFinishedMessage ..."))
			}

		delete iServerFinished;
		iServerFinished = 0;
		delete iActualFinishedMsg;
		iActualFinishedMsg = 0;
		if(!iAbbrievatedHandshake && (iTlsCryptoAttributes->iSessionNameAndID).iSessionId.Size())
			{
			iStatus = KRequestPending;
			iCurrentState = EConnectionEstablished;
			iSessionInterface->ConnectionEstablished((!iServerMsgVerified?ETrue:EFalse),ETrue,iTlsCryptoAttributes->iClientAuthenticate,iStatus );		
			SetActive();
			return;		
			}
		else
			User::RequestComplete(iOriginalRequestStatus,iServerMsgVerified);
		
		}
		break;

	case EConnectionEstablished:
		{
		User::RequestComplete(iOriginalRequestStatus,iServerMsgVerified);
		}
		break;

	case ECertificateVerifyMsg:
		{	
		iCurrentState = ENullState;
		iTempPtr.Set( iTempHolder->Des() );
		iSessionInterface->ComputeDigitalSignature(iTempPtr,(*iComputeDigitalSig),*iSelectedCertInfo,*iSelectedKeyInfo,*iOriginalRequestStatus);		
		}	
		break;
	BULLSEYE_OFF
	default:
		break;
	BULLSEYE_RESTORE
	}

}

TInt CTlsSessionImpl::RunError(TInt aError)
	{
	TLSPROV_LOG(_L("CTlsSessionImpl...RunError called"))
	if(iClientKeyExch)
		{
		// Must have been in the middle of creating a client key exchange
		delete *iClientKeyExch;
		*iClientKeyExch = 0;
		iClientKeyExch = 0;
		}
	User::RequestComplete(iOriginalRequestStatus, aError);
	return KErrNone;
	}

void CTlsSessionImpl::CancelRequest()
	{
	TLSPROV_LOG(_L("CTlsSessionImpl::Cancelling any outstanding request..."))
	Cancel();
	}

void CTlsSessionImpl::DoCancel()
	{
	TLSPROV_LOG2(_L("CTlsSessionImpl::DoCancel()...state %d"), iCurrentState)
	switch(iCurrentState)
	{			
	case EGetClientKeyExchange:
		{
		if(iSessionInterface)
			iSessionInterface->CancelClientKeyExchange();
		}
		break;
	case EKeyGeneration:
	case EClientFinishedMsg:
	case EVerifyServerFinishedMsg:
		{
		if(iSessionInterface)
			iSessionInterface->CancelPHash();
		}
		break;
	case EGetServerCertificate:
		{
		if(iSessionInterface)
			iSessionInterface->CancelServerCertificate();
		}
		break;
	case EGetClientCerificate:
		{
		if(iPtrUnifiedCertStore)
			iPtrUnifiedCertStore->CancelRetrieve();
		}
		break; 
	case EComputeDigitalSignature:
		{
		if(iSessionInterface)
			iSessionInterface->CancelComputeDigitalSignature();
		}
		break;
	case EConnectionEstablished:
		{
		if(iSessionInterface)
			iSessionInterface->CancelConnectionEstablished();
		}
		break;  
	case ECertificateVerifyMsg:
		{
		if(iSessionInterface)
			iSessionInterface->CancelComputeDigitalSignature();
		}
		break;

	default:
		break;
	}
	iCurrentState = ENullState;
	User::RequestComplete(iOriginalRequestStatus, KErrCancel);
	return;
	}	

TInt CTlsSessionImpl::KeyDerivation(
		const TDesC8& aLabel, 
		const TTLSMasterSecretInput& aMasterSecretInput, 
		TDes8& aKeyingMaterial)
	{
	if(iSessionInterface == NULL)
		{
		return KErrNotReady;
		}
	else
		{
		return iSessionInterface->KeyDerivation(aLabel,aMasterSecretInput,aKeyingMaterial);
		}
	}

CTlsSessionImpl::~CTlsSessionImpl()
	{
	if(iConstructionComplete)
		{
		// Delete members who are passed into us via NewL
		// (we only take ownership of these when ConstructL completes successfully)
		if(iSessionInterface)		
			iSessionInterface->Release();	
		
		if(iSelectedCertInfo && !iAbbrievatedHandshake)
			iSelectedCertInfo->Release();	

		if(iSelectedKeyInfo)
			iSelectedKeyInfo->Release();

		// Note iStoredIntermediatesCACertificates is a pointer to an RPointerArray which is a member variable of another
		// object so must not be deleted here.

		// Delete members who are passed into us via ConstructL
		// (we only take ownership of these when ConstructL completes successfully)
		delete iTlsCryptoAttributes;
		delete iEncodedServerCerts;
		}

	delete iKeyMaterial;
   	delete iEncodedClientCert;
	delete iPtrUnifiedCertStore;
   	delete iTempHolder;
	delete iEncrypt;

   	iFs.Close();
	REComSession::FinalClose();
	}
