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
// CTlsProviderImpl.CPP
// 
//

#include <ecom/ecom.h>
#include <securitydefsconst.h>
#include <x520ava.h>

#include "tlsprovider.h"
#include "cryptostrength.h"


//
//  CTlsProviderImpl
//

/**
This method creates a new CTlsProviderImpl object
@return Pointer to a new CTlsProviderImpl object is returned

*/
CTlsProviderImpl* CTlsProviderImpl::ConnectL()
	{
	
	CTlsProviderImpl* tPtr = new (ELeave)CTlsProviderImpl;
	CleanupStack::PushL(tPtr);
	tPtr->ConstructL();
	tPtr->iTlsCryptoAttributes = CTlsCryptoAttributes::NewL();
	CleanupStack::Pop();
	return tPtr;	
	}


CTlsProviderImpl::CTlsProviderImpl()
	:CActive(0), iValidationStatus(EValidatedOK, 0), iTlsProviderPolicy(NULL)
	{
	  if(CActiveScheduler::Current()) //Allready installed?
		{
		CActiveScheduler::Add( this );
		}
	}

TInt CipherSuiteOrder(
		const TTLSCipherSuite& aLeft,
		const TTLSCipherSuite& aRight)
	{	
	if((aLeft.iHiByte == aRight.iHiByte) && (aLeft.iLoByte == aRight.iLoByte))
		return 1;
	else
		return 0;
	}

static TInt CompareCipherIdentities(
	const TTLSCipherSuite& aLeft, const TTLSCipherSuite& aRight)
/**
	This ordering relation is used by CTlsProviderImpl::ReturnCipherListL
	to ensure that cipher appears in a list at most once.  (It will
	also order the ciphers by identity, but that is incidental.)
	
	It is used with a TLinearOrder template and an RArray.
	It orders the ciphers by identity, which means iHiByte is the most
	significant byte, and iLoByte is the least significant byte.
	
	@param	aLeft			Left cipher suite to compare.
	@param	aRight			Right cipher suite to compare.
	@return					<0 if the left cipher's identity is less
							than the right cipher's; 0 if they are
							the same; >0 otherwise.
	@see CTlsProviderImpl::ReturnCipherListL
 */
	{
	TInt leftHi = aLeft.iHiByte;
	TInt leftLo = aLeft.iLoByte;
	TInt leftIdentity = (leftHi << 8) | leftLo;

	TInt rightHi = aRight.iHiByte;
	TInt rightLo = aRight.iLoByte;
	TInt rightIdentity = (rightHi << 8) | rightLo;

	return leftIdentity - rightIdentity;
	}


static TInt CompareCipherPriorities(	
	const TTLSCipherSuite& aLeft, const TTLSCipherSuite& aRight)
/**
	This ordering relation is used by CTlsProviderImpl::ReturnCipherListL
	to put the ciphers in priority order, highest priority first.
	A higher priority means the numerically lower value of iPriority.
	E.g. priority 1 is before priority 2.
	
	It is wrapped in a TLinearOrder object and used to sort an RArray.
	
	@param	aLeft			Left cipher suite to compare.
	@param	aRight			Right cipher suite to compare.
	@return					<0 if the left cipher's priority is gt
							the right cipher; 0 if they are the same;
							>0 otherwise.
	
	@see CTlsProviderImpl::ReturnCipherListL
 */
	{
	const TTLSCipherSuiteMapping* cipherDetail = aLeft.CipherDetails();
	TInt leftPri = (cipherDetail == NULL)? pri_unsupp:  cipherDetail->iPriority;
	cipherDetail = aRight.CipherDetails();
	TInt rightPri = (cipherDetail == NULL)? pri_unsupp:  cipherDetail->iPriority;
	
	return leftPri - rightPri;
	}


/**
The first call to CTlsProviderImpl takes place before ClientHello is sent,then as the handshake progresses, 
the information relevant for Provider and token will be gradually filled in a structure. This 
structure CTlsCryptoAttributes will be returned in this API, If the structure already exists,
then the pointer to the same structure is returned.
@param None
@return Pointer to a CTlsCryptoAttributes structure 
@see CTlsCryptoAttributes
*/
CTlsCryptoAttributes* CTlsCryptoAttributes::NewL()
	{
	CTlsCryptoAttributes* tPtr = new (ELeave)CTlsCryptoAttributes;
	CleanupStack::PushL(tPtr);
	tPtr->iPublicKeyParams = new (ELeave)CTLSPublicKeyParams;
	CleanupStack::Pop();
	return tPtr;
	}

CTlsCryptoAttributes::CTlsCryptoAttributes()
	{
	}

CTlsCryptoAttributes::~CTlsCryptoAttributes()
	{
	delete iPskIdentityHint;
	iReqCertTypes.Close();
	if(iPublicKeyParams)
		delete iPublicKeyParams;
    for ( TInt n = 0; n < iDistinguishedCANames.Count(); n++ )
      	{
      	HBufC8* buf = (HBufC8*)(iDistinguishedCANames[n]);
      	delete buf;
      	}
	iDistinguishedCANames.Close();
	
	delete iServerNames;
	if(iServerDNFromCertSubject)
		delete iServerDNFromCertSubject;
	if(iServerDNFromCertIssuer)
		delete iServerDNFromCertIssuer;
	}

CTlsCryptoAttributes* CTlsProviderImpl::Attributes() 
	{
	return iTlsCryptoAttributes;
	}


CTlsSessionImpl* CTlsProviderImpl::TlsSessionPtr() 
	{
	return iTlsSessionImpl;
	}


void CTlsProviderImpl::CreateL(
		CTLSSession*& aTlsSession,		
		TRequestStatus& aStatus) 
		{
		TInt Err = 0;
		aStatus = KRequestPending;
		iOriginalRequestStatus = &aStatus;
		aTlsSession = 0;
		iTlsSessionHldr = 0;
		if(IsActive())
			{
			Err = KErrInUse;			
			}

	
		if(iTlsCryptoAttributes)
 			{
			if((iTlsCryptoAttributes->iSessionNameAndID.iSessionId.Length()) && 
				(iTlsCryptoAttributes->iSessionNameAndID.iSessionId == iSessionData.iSessionId)
			  )
				{
				iSelectedTypeIndex = 0;
				iAbbreviatedHandshake = ETrue;
				}
			else
				{
				iAbbreviatedHandshake = EFalse;

 				TIdentityRelation<TTLSCipherSuite> CipherOrder(CipherSuiteOrder);
				if((iTlsCryptoAttributes->iNegotiatedProtocol != KTLS1_0) || (iTlsCryptoAttributes->iNegotiatedProtocol != KSSL3_0) )
 					Err = KErrSSLAlertIllegalParameter;
 				if(iSupportedCipherSuiteList.Count() &&
 					(iSupportedCipherSuiteList.Find((iTlsCryptoAttributes->iCurrentCipherSuite),CipherOrder) ==KErrNotFound))
 						Err = KErrSSLAlertIllegalParameter;
 				if(iTlsCryptoAttributes->iMasterSecretInput.iServerRandom.Size() != 32)
 					Err = KErrSSLAlertIllegalParameter;	
				if(!(
					(iTlsCryptoAttributes->iMasterSecretInput.iServerRandom).Compare(iTlsCryptoAttributes->iMasterSecretInput.iClientRandom)
					)
				  )
					Err = KErrSSLAlertIllegalParameter;	
				if(!iTotalTokenTypeCount)
					Err = KErrSSLAlertIllegalParameter;	
				}

			}

		if(Err != KErrNone || !iTlsCryptoAttributes)
			{	
   			User::RequestComplete(iOriginalRequestStatus, Err);
  			return; 
   			}

		iOriginalState =  ECreate;			
		iTlsSessionHldr = &aTlsSession;
		iSelectedTypeIndex = -1;	
		
		TBool TokenFound = EFalse;
		if(!iAbbreviatedHandshake) //Full handshake
			{
			TokenFound = SelectToken();

			if(!TokenFound) //No tokens supporting the selected cipher
				{
				User::RequestComplete(iOriginalRequestStatus, KErrCipherNotSupported);
				}
			else
				{
				if(iTlsCryptoAttributes->iClientAuthenticate)
					{
					//Before we initilize client authenticate, let's check if server
					//provides us it's DN name
					if(!iTlsCryptoAttributes->iDistinguishedCANames.Count())
						{
							//seems we need to use the server DN obtained from server cert as a backup
							iTlsCryptoAttributes->iDistinguishedCANames.AppendL(iTlsCryptoAttributes->iServerDNFromCertSubject);
							iTlsCryptoAttributes->iDistinguishedCANames.AppendL(iTlsCryptoAttributes->iServerDNFromCertIssuer);
							iTlsCryptoAttributes->iServerDNFromCertSubject = NULL; //ownership handover 
							iTlsCryptoAttributes->iServerDNFromCertIssuer = NULL; //ownership handover 
						}
					
					iClntAuthenticate = CTlsClntAuthenticate::NewL(*iTlsCryptoAttributes,
																	 iEncodedServerCerts);
					iCurrentState = EClientAuthenticate;
					
					iStoredIntermediatesCACertificates.Reset();
					iClntAuthenticate->DoClientAuthenticate(iSelectedCertInfo,
															iSelectedKeyInfo,
															&iStoredIntermediatesCACertificates,
															iStatus);
					SetActive();
					}
				else
					{
					// Get the session interface and create CtlsSession object
					TLSPROV_LOG2(_L("Using (newly selected) token %d's provider interface"), iSelectedTypeIndex)
					iPtrTokenSearch->GetSessionInterface(iListAllTokensAndTypes[iSelectedTypeIndex].iProviderInterface,
								iSessionInterface,iStatus);
					iCurrentState = EGetSessionInterface;
					iStatus = KRequestPending;
					SetActive();
					}
				}
			}
		else //Abbrevated handshake
			{
			iCurrentState = EGetSessionInterface;
			iNextState =  EConstructResumed;
			iStatus = KRequestPending;
			iPtrTokenSearch->GetSessionInterface(iSessionData.iProviderInterface,
			iSessionInterface,iStatus);
			SetActive();
			}
		return;
		}

void CTlsProviderImpl::GetSessionL(	
		TTLSServerAddr& aServerName,
		TTLSSessionId& aSessionId,
		TRequestStatus& aStatus) 
	{ 
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}

	iPServerName =  &aServerName;
	iOutputSessionData.iSessionId.Zero();
	iPSessionId= &aSessionId;

	if(iPtrTokenSearch)
		iTotalTokenTypeCount = iPtrTokenSearch->TotalTypeCount();

	iCurrentTokentype = 0;
	iCurrentToken = 0;
	iOriginalState = EGetSession;


	if(!iTotalTokenTypeCount)
		{	
		iCurrentState =  EBrowseTokens;
		iPtrTokenSearch = CTlsBrowseToken::NewL();		
		iStatus = KRequestPending;
		iPtrTokenSearch->StartBrowsingL(iListAllTokensAndTypes,iStatus);
		SetActive();
		return;
		}
	else
		RetrieveSession();
	
	return;	
	}

void CTlsProviderImpl::RetrieveSession()
	{

	while((iCurrentTokentype + 1) <= iListAllTokensAndTypes.Count())
		{
		iCurrentState = EGetSession;		
		iReqProtList.Reset();			
		if(iTlsCryptoAttributes)
			{		
			if((iTlsCryptoAttributes->iProposedProtocol.iMajor !=0)
				&& (iTlsCryptoAttributes->iProposedProtocol.iMajor != 0))
				{				
				iReqProtList.Append(iTlsCryptoAttributes->iProposedProtocol);
				}		
			}
		iStatus = KRequestPending;
		iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface->GetSession(
				( (iOriginalState !=  EClearSessionCache)?(*iPServerName):(iServerNameAndId.iServerName)),
				iReqProtList,
				iOutputSessionData,iStatus);
		SetActive();	
		return;	
		}
	ReturnResult();
	}



void CTlsProviderImpl::CipherSuitesL(RArray<TTLSCipherSuite>& aUserCipherSuiteList,TRequestStatus& aStatus)
	{

	aStatus = KRequestPending;
	iOriginalState =  EGetCiphers;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}
		
	TLSPROV_LOG2(_L("Obtaining the list of ciphersuites %d"), iTotalTokenTypeCount)

	iUserCipherSuiteList = &aUserCipherSuiteList;
	aUserCipherSuiteList.Reset();

	if(iPtrTokenSearch)
		iTotalTokenTypeCount = iListAllTokensAndTypes.Count();

	if(!iTotalTokenTypeCount)
		{	

		iCurrentState =  EBrowseTokens;
		iStatus = KRequestPending;
		iPtrTokenSearch = CTlsBrowseToken::NewL();		
		iPtrTokenSearch->StartBrowsingL(iListAllTokensAndTypes,iStatus);
		SetActive();
		return;		
		}
	else 
		{
		//we have the Token details already...so lets return the cipher list
		TRAPD(err,ReturnCipherListL());
		User::RequestComplete(iOriginalRequestStatus, err);
		}
	return;
	}

void CTlsProviderImpl::OnEBrowseTokens()
	{
   if ( iStatus == KErrNone )
      {
	   if(iPtrTokenSearch)
		   iTotalTokenTypeCount = iListAllTokensAndTypes.Count();

	   switch(iOriginalState)
	      {
	      case EGetCiphers:
		      ReturnResult();
		      break;
	      case EGetSession:
	      case EClearSessionCache:
		      {
		      RetrieveSession();		
		      }
		      break;
	      default:
		      break;
	      }
      }
   else
      {
		User::RequestComplete(iOriginalRequestStatus, iStatus.Int());
      }
	return;
	}	

void CTlsProviderImpl::VerifyServerCertificate(
		const TDesC8& aEncodedServerCerts, 
		CX509Certificate*& aServerCert,		  			
		TRequestStatus& aStatus)
	{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}

	TTime ValidationTime;	
	ValidationTime.UniversalTime();
	iStatus = KRequestPending;
	iX509ServerCert= &aServerCert;

	iOriginalState =  EValidateCertificate;

	TRAPD(leaveValue, 		
		iEncodedServerCerts=aEncodedServerCerts.AllocL();		
		(iCertVerificationResult= CPKIXValidationResult::NewL()); 
		iServerCertsChain = CPKIXCertChain::NewL(iFs,  aEncodedServerCerts,
												  TUid::Uid(KUidUnicodeSSLProtocolModule));
      
		if ( iTlsProviderPolicy->PKICriticalExtensions() != 0 )
		    {
            TLSPROV_LOG2(_L("Adding %d additionally Supported Critical Extensions"), iTlsProviderPolicy->PKICriticalExtensions())
            iServerCertsChain->AddSupportedCriticalExtensionsL(iTlsProviderPolicy->GetPKICriticalExtensions());
		    }

		iServerCertsChain->ValidateL(*iCertVerificationResult,ValidationTime,iStatus) );
	
	if (leaveValue!=KErrNone)
		{
		delete iServerCertsChain;
		iServerCertsChain = 0;
		delete iEncodedServerCerts;
		iEncodedServerCerts = 0;
		iStatus = KErrNone;
		User::RequestComplete(iOriginalRequestStatus, leaveValue);
		}
	else
		{
		iCurrentState = EValidateCertificate;
		SetActive();
		}
	
	return;
	}



TBool CTlsProviderImpl::VerifySignatureL(
		const CSubjectPublicKeyInfo& aServerPublicKey, 
		const TDesC8& aDigest,const TDesC8& aSignature)
	{
	TBool tempResult;
	tempResult = EFalse;
	switch(iTlsCryptoAttributes->isignatureAlgorithm)
	{
	case ERsaSigAlg:
		{
	
		//create an RSA public key object from the server certificate
		CRSAPublicKey* rsaPublicKey = CX509RSAPublicKey::NewLC(aServerPublicKey.KeyData());

		//create an RSA signature object from the signature buffer
		RInteger sigInt = RInteger::NewL(aSignature);
		CleanupStack::PushL(sigInt);
		CRSASignature* rsaSignature = CRSASignature::NewL(sigInt);
		CleanupStack::Pop(&sigInt);	//	Owned by rsaSignature
		CleanupStack::PushL(rsaSignature);

		//create a verifier with the server public key
		CRSAPKCS1v15Verifier*  Verifier = CRSAPKCS1v15Verifier::NewLC(*rsaPublicKey);
		
		//verify the signature
		tempResult = Verifier->VerifyL(aDigest, *rsaSignature);	
		CleanupStack::PopAndDestroy(3, rsaPublicKey);		
		}
		break;
	case EDsa:
		{
		//digest and signature are 8-bit descriptors
		//create a DSA public key object from the server certificate
		CDSAPublicKey* dsaPublicKey = CX509DSAPublicKey::NewL(aServerPublicKey.EncodedParams(),
															  aServerPublicKey.KeyData());

		CleanupStack::PushL(dsaPublicKey);

		//create verifier, verify sig
		CDSAVerifier* verifier = CDSAVerifier::NewL(*dsaPublicKey);
		CleanupStack::PushL(verifier);
		
		//create DSA signature object from signature buffer
		CDSASignature* dsaSignature = CX509DSASignature::NewLC(aSignature);
		
		tempResult = verifier->VerifyL(aDigest, *dsaSignature);	
		CleanupStack::PopAndDestroy(3, dsaPublicKey);  
		}
		break;
	default:
		break;
	}
		
	return tempResult;
	}



void CTlsProviderImpl::GenerateRandom(TDes8& aBuffer)
	{
	// Fill the buffer with zero bytes
	aBuffer.SetLength(KTLSServerClientRandomLen);
	aBuffer.Fill(0);
	
	// The last 28 bytes should be filled with random data
	TPtr8 random = aBuffer.MidTPtr(4);
	TRAPD(errRandom,TRandom::RandomL(random));
	if (errRandom)
		{
		aBuffer.SetLength(0);
		return;
		}
	
	// The first 4 bytes should a unix timestamp.
	TTime now;
	now.UniversalTime();
	
	_LIT(KEpoch,"19700000:");
	TTime epoch(KEpoch);
	
	TTimeIntervalSeconds timestamp;
	TInt err = now.SecondsFrom(epoch, timestamp);
	
	if (err == KErrNone)
		{
		TInt timestampInt = timestamp.Int();
		aBuffer[0] = timestampInt >> 24;
		aBuffer[1] = timestampInt >> 16;
		aBuffer[2] = timestampInt >> 8;
		aBuffer[3] = timestampInt;
		}
		
	}


void CTlsProviderImpl::ClearSessionCacheL(
		TTLSSessionNameAndID& aServerNameAndId, 		
		TRequestStatus& aStatus) 
{
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	if(IsActive())
		{
		User::RequestComplete(iOriginalRequestStatus, KErrInUse);
		return;
		}

	iServerNameAndId = aServerNameAndId;	
	iOriginalState =  EClearSessionCache;
	iOutputSessionData.iSessionId.Zero();


	if(iPtrTokenSearch)
		iTotalTokenTypeCount = iPtrTokenSearch->TotalTypeCount();

	if(!iTotalTokenTypeCount)
		{				
		iPtrTokenSearch = CTlsBrowseToken::NewL();	
		iCurrentState =  EBrowseTokens;	
		iStatus = KRequestPending;
		iPtrTokenSearch->StartBrowsingL(iListAllTokensAndTypes,iStatus);
		SetActive();
		return;
		}
	else
		RetrieveSession();	

	return;
}

#ifdef _DEBUG

void CTlsProviderImpl::Panic(CTlsProviderImpl::TPanic aPanic)
/**
	Halt the current thread with the panic category "TlsProvImpl"
	and the supplied panic reason.
	
	@param	aPanic			Panic reason.
 */
	{
	_LIT(KPanicCat, "TlsProvImpl");
	User::Panic(KPanicCat, aPanic);
	}
	
static TBool AreSuitesInPriorityOrder(const RArray<TTLSCipherSuite>& aSuites)
/**
	Predicate function determines whether the supplied array contains
	cipher suites in priority order, highest-priority first.
	(Higher priority suites have numerically lower iPriority fields.)
	
	@param	aSuites			Array of cipher suites.
	@return					ETrue if the suites are in priority
							order, EFalse otherwise.
 */
	{
	TInt curSuite = aSuites.Count();
	if (curSuite == 0)
		return ETrue;

	const TTLSCipherSuiteMapping* cipherDetail = aSuites[--curSuite].CipherDetails();
	TInt nextPri = (cipherDetail == NULL)? pri_unsupp: cipherDetail->iPriority;
	while (--curSuite >= 0)
		{
		cipherDetail = aSuites[curSuite].CipherDetails();
		TInt curPri = (cipherDetail == NULL)? pri_unsupp: cipherDetail->iPriority;
		if (nextPri < curPri)
			return EFalse;
		nextPri = curPri;
		}
	
	return ETrue;
	}

#endif	// #ifdef _DEBUG

TBool CTlsProviderImpl::IsCipherAvailable( const TTLSCipherSuiteMapping& aCipherSuiteMapping ) const
{
//this is a simplification that is allowable as DES_CBS & 3DES_CBS cipher ignore one & three bytes
//respectively from the key material so the key length is in fact shorter than the computed one
//but this still allows us to distinguish between strong & weak cipher suites...
   TInt nKeyBits = aCipherSuiteMapping.iKeyMaterial * 8;
   return aCipherSuiteMapping.iSupported && (TCrypto::Strength() == TCrypto::EStrong || nKeyBits <= 64);
}


void CTlsProviderImpl::ReturnCipherListL()
	{
	
	TInt aTotalTokenTypes = iListAllTokensAndTypes.Count();

	iUserCipherSuiteList->Reset();
	TLinearOrder<TTLSCipherSuite> CipherOrder(CompareCipherIdentities);

	TLSPROV_LOG2(_L("CTlsProviderImpl::ReturnCipherListL %d"), aTotalTokenTypes)
	TBool thisCipherSupportedByAToken = EFalse;
	
	for (TInt CipherIndex = 0; CipherIndex < TLS_CIPHER_SUITES_NUMBER; ++CipherIndex)
		{
		const TTLSCipherSuiteMapping& mapping = KSetOfTLSCipherSuites[CipherIndex];
		thisCipherSupportedByAToken = EFalse;   // reset
		
		if(! IsCipherAvailable(mapping))
			continue;

		// NULL encryption based ciphersuites are disallowed unless explicitly enabled
		if( (! iTlsCryptoAttributes->iAllowNullCipherSuites) && (mapping.iBulkCiphAlg == ENullSymCiph))
			{
			continue;
			}
		
		// PSK key exchange ciphersuites are not usable unless PSK has been configured.
		if((! iTlsCryptoAttributes->iPskConfigured) &&
			((mapping.iKeyExAlg==EPsk) || (mapping.iKeyExAlg==EDhePsk) || (mapping.iKeyExAlg==ERsaPsk)))
			{
			continue;
			}
		
		TLSPROV_LOG3(_L("cipher 0x%02x.0x%02x available"), mapping.iCipherSuite.iHiByte, mapping.iCipherSuite.iLoByte)
		
		//offer just these matching the current crypto strength.
		for (TInt i = 0; i < aTotalTokenTypes; i++) //For every Token type
			{
			CTokenInfo* tokenInfo = iListAllTokensAndTypes[i].iTokenInfo;
			
			// check if this token contains the required key exchange and
			// signature algorithms
			
			TBool reqAlgs = 
					tokenInfo->aKeyExchAlgs.Find(mapping.iKeyExAlg) != KErrNotFound
				&&	tokenInfo->aSignatureExchAlgs.Find(mapping.iSigAlg) != KErrNotFound;
			
			if (! reqAlgs)
				continue;
			
			TLSPROV_LOG2(_L("token %d supports this cipher"), i)
			thisCipherSupportedByAToken = ETrue;
			
			// Add this cipher to the token's list of suites supported
			if(tokenInfo->iCipherSuitesSupported.InsertInOrder(mapping.iCipherSuite,CipherOrder) == KErrNoMemory)
				User::Leave(KErrNoMemory); 
			}
		
		if (thisCipherSupportedByAToken)
			{
			// don't need to order the ciphers as they are sorted below
			TLSPROV_LOG(_L("adding this cipher to iUserCipherSuiteList"))
			iUserCipherSuiteList->AppendL(mapping.iCipherSuite);
			}
		}

	// order the cipher suites by priority, highest-priority first.
	const TLinearOrder<TTLSCipherSuite> priOrder(CompareCipherPriorities);
	iUserCipherSuiteList->Sort(priOrder);

	// also order the cipher suites stored with each token type
	for (TInt j = aTotalTokenTypes - 1; j >= 0 ; --j)
		{
		CTokenInfo* tokenInfo = iListAllTokensAndTypes[j].iTokenInfo;
		tokenInfo->iCipherSuitesSupported.Sort(priOrder);
		}
	
#ifdef _DEBUG
	// Ensure suites are in priority order, as required for
	// DEF063433 - TLS is sending CipherSuites in the wrong order 
	
	if (! AreSuitesInPriorityOrder(*iUserCipherSuiteList))
		Panic(ERCLBadUserOrder);
	
	for (TInt k = aTotalTokenTypes - 1; k >= 0 ; --k)
		{
		const CTokenInfo* tokenInfo = iListAllTokensAndTypes[k].iTokenInfo;
		const RArray<TTLSCipherSuite>& suites = tokenInfo->iCipherSuitesSupported;
		if (! AreSuitesInPriorityOrder(suites))
			Panic(ERCLBadTokenOrder);
		}
#endif

	TLSPROV_LOG(_L("Returning the list of ciphersuites"))
	}
	
void CTlsProviderImpl::ReturnSession()
	{
	if((iOutputSessionData.iSessionId).Length())
		{
		*iPSessionId = iOutputSessionData.iSessionId;
		iPSessionId = 0;
		}
		User::RequestComplete(iOriginalRequestStatus, KErrNone);				
	}

void CTlsProviderImpl::OnEGetSessionInterfaceL()
	{
	
	if(!iStatus.Int()) //Successfull, Session interface obtained
		{
		TLSPROV_LOG(_L("Session interface from token obtained successfully"))

		iCurrentState = EStartSession;
		
		//we can delete the token search object here
		if(iPtrTokenSearch)
			{
			delete iPtrTokenSearch;
			iPtrTokenSearch = 0;
			}

		iTlsSessionImpl = 0;
		CTlsSessionImpl *newImpl = 0;
		TRAPD(Kerr, ( newImpl = CTlsSessionImpl::NewL(
			iSessionInterface,		
			iSelectedCertInfo,
			iSelectedKeyInfo,
			&iStoredIntermediatesCACertificates)
			) );
		
		if(Kerr != KErrNone)
			{			
			User::RequestComplete(iOriginalRequestStatus,Kerr);
			return;
			}		

		CleanupStack::PushL(newImpl);
		
		if(iNextState == EConstructResumed)
			{			
			iStatus = KRequestPending;
			iCurrentState = iNextState;
			newImpl->ConstructResumedL(iTlsCryptoAttributes,iStatus);			
			SetActive();
			CleanupStack::Pop(newImpl);
			iTlsSessionImpl = newImpl;
			return;

			}
		else
			{	
			iCurrentState =  EConstruct;
			iStatus = KRequestPending;
			newImpl->ConstructL(iTlsCryptoAttributes,iEncodedServerCerts,iStatus);	
			SetActive();
			CleanupStack::Pop(newImpl);
			iTlsSessionImpl = newImpl;
			return;
			}
		}
	else	//session interface not obtained..Dodgy token..release it
		{
		User::RequestComplete(iOriginalRequestStatus, iStatus.Int());	
		}
	}


void CTlsProviderImpl::OnEGetSession()
	{
	if( (!iStatus.Int())  && 
		( (iOutputSessionData.iSessionId).Size()) 
	  )
		{

		TLSPROV_LOG(_L(" GetSession  obtained successfully"))

		if(iOriginalState ==  EClearSessionCache)
			{
			TBool result;
			iStatus = KRequestPending;
			iCurrentState = EClearSessionCache;
			iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface->ClearSessionCache(iServerNameAndId.iServerName,
				iServerNameAndId.iSessionId,result,iStatus);
			SetActive();
			return;
			}
				
		iSessionData.iSessionId = iOutputSessionData.iSessionId;
		iSessionData.iProviderInterface = iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface;
			
		if(iOriginalState == EGetSession ) 
			{
			//return the result here
			ReturnSession();
			return;
			}
		}
	iCurrentTokentype++;
	RetrieveSession();	
	return;	
	}


void CTlsProviderImpl::RunL()
{
	
	switch(iCurrentState)
	{
	
	//Obtain Session Interface
	case EGetSessionInterface:
		{
		OnEGetSessionInterfaceL();
		}
		break;
	
	//Retrieving and clearing of Sesion cache
	case EGetSession:
		{
		OnEGetSession();
		}
		break;

	case EClearSessionCache:
		{	
		if(!iStatus.Int()) 
			{	
			if((iSessionData.iSessionId).Compare(iOutputSessionData.iSessionId) ==0)
				{
				(iSessionData.iSessionId).Zero();
				}
			}
		User::RequestComplete(iOriginalRequestStatus,iStatus.Int());						
		}
		break;	
	
	//Abbreviated handshake	
	case EConstructResumed:
		{
		if(iStatus.Int() != KErrNone)
			{
			delete iTlsSessionImpl;
			iTlsSessionImpl = NULL;
			}
		*iTlsSessionHldr = CTLSSession::NewL(iTlsSessionImpl);		
		iTlsSessionOwnershipPassedToCaller = ETrue;
		User::RequestComplete(iOriginalRequestStatus,iStatus.Int());
		return;
		}

	//Full handshake
	case EConstruct:
		{
		iEncodedServerCerts = 0; //Owned by CTLSSession 
		if(iStatus.Int() != KErrNone)
			{
			delete iTlsSessionImpl;
			iTlsSessionImpl = NULL;
			}
		*iTlsSessionHldr = CTLSSession::NewL(iTlsSessionImpl);
		iTlsSessionOwnershipPassedToCaller = ETrue;
		User::RequestComplete(iOriginalRequestStatus,iStatus.Int());
		return;
		}

	//To obtain client certificate
	case EClientAuthenticate:
		{
		delete iClntAuthenticate;
		iClntAuthenticate = 0;
		
		iCurrentState = EGetSessionInterface;
		iNextState = EStartSession;	
		iStatus = KRequestPending;
				
		// Iterate through tokens, GetSessionInterface() for this token if pointers match
		for(TInt i=0; i< iListAllTokensAndTypes.Count(); i++)
			{
			MCTToken* tempPtr = &(iListAllTokensAndTypes[i].iProviderInterface->Token());
			if(iSelectedKeyInfo && (tempPtr == &(iSelectedKeyInfo->Token())))
				{
				TLSPROV_LOG(_L("EClientAuthenticate: iSelectedKeyInfo->Token() valid; reusing this token interface"))
				// Keep existing selected token
				iPtrTokenSearch->GetSessionInterface(iListAllTokensAndTypes[i].iProviderInterface, iSessionInterface,iStatus);
				SetActive();
				return;
				}
			}
			
		// Otherwise, have already selected (potentially new) token in call to SelectToken()
		// in previous state. Use iSelectedTypeIndex in call to GetSessionInterface()
		TLSPROV_LOG2(_L("EClientAuthenticate: Using (newly selected) token %d's provider interface"), iSelectedTypeIndex)
		iPtrTokenSearch->GetSessionInterface(iListAllTokensAndTypes[iSelectedTypeIndex].iProviderInterface,
				iSessionInterface,iStatus);
		SetActive();
		}
		break;
	
	case EQueryCache:
		{
		OnQueryCacheL();
		}
		break;
	
	case EUserDialog:
		{
		OnEUserDialogL();
		}
		break;

	case EValidateCertificate:
		{
		User::LeaveIfError(iStatus.Int());
		
		//We set iServerDNFromCert in case server doesn't
		//give us DistingshedName in CertificateRequest Message. 
		if(iValidationStatus.iReason == EValidatedOK)
		{
			TInt serverCertChainLength = iServerCertsChain->Count();
			TLSPROV_LOG2(_L("EValidateCertificate iServerCertsChain length = %d"), serverCertChainLength);
			CX509Certificate* cert = CX509Certificate::NewL(iServerCertsChain->Cert(serverCertChainLength-1));
			CleanupStack::PushL(cert);
			iTlsCryptoAttributes->iServerDNFromCertIssuer = cert->DataElementEncoding(CX509Certificate::EIssuerName)->AllocL();
			iTlsCryptoAttributes->iServerDNFromCertSubject = cert->DataElementEncoding(CX509Certificate::ESubjectName)->AllocL();
			CleanupStack::PopAndDestroy(cert);					

		}
		delete iServerCertsChain;
		iServerCertsChain = 0;
		iValidationStatus = iCertVerificationResult->Error();
		delete iCertVerificationResult;
		iCertVerificationResult = 0;
		TBool acceptCert = EFalse;
		GetX509CertL(iEncodedServerCerts,*iX509ServerCert);
		
		if (iValidationStatus.iReason == EValidatedOK)
			{
 			
			// If the certificate is fine, check whether the server name matches
			// the site the certificate was issued to.
		
			if (iTlsCryptoAttributes->idomainName.Length() != 0) // skip the check if domain name is unset.
				{
				acceptCert = ValidateDNSNameL(**iX509ServerCert);
				}
			else
				{
				acceptCert = ETrue;
				}
			
			if(acceptCert)
				{				
				acceptCert = CheckExtendedKeyUsageL(**iX509ServerCert);
				if(!acceptCert)
					{
					iValidationStatus.iReason = ECriticalExtendedKeyUsage;
					}
				}
			}
		
		if(acceptCert)
			{
			TLSPROV_LOG(_L("Server Certificate verified successfully"))			
			iCurrentState = iOriginalState = ENullState;
			User::RequestComplete(iOriginalRequestStatus,KErrNone); 
			}
		else
			{
			if(iTlsCryptoAttributes && iTlsCryptoAttributes->iDialogNonAttendedMode)
				{	
				TLSPROV_LOG(_L("Server Certificate validation failed but in DialogNonAttended mode"))	
				TInt err(0);
				
				switch(iValidationStatus.iReason)
					{
				case EChainHasNoRoot:
					err = KErrSSLAlertUnknownCA;
					break;
				case EDateOutOfRange:
					err = KErrSSLAlertCertificateExpired;
					break;
				case ECertificateRevoked:
					err = KErrSSLAlertCertificateRevoked;
					break;
				case ECriticalExtendedKeyUsage:
					err = KErrSSLAlertUnsupportedCertificate;
					break;
				default:
					err = KErrSSLAlertIllegalParameter;
					break;
					}
					
				iCurrentState = iOriginalState = ENullState;
				User::RequestComplete(iOriginalRequestStatus,err); 				
				}
			else
				{			
				
				User::LeaveIfError(iCacheClient.Open(**iX509ServerCert));
				HandleBadCertificateL(iValidationStatus);
				
				}		
			}

		}
		break;
	//Obtain interface form all tokens supproted
	case EBrowseTokens:
		OnEBrowseTokens();
		break;
	BULLSEYE_OFF
	default:
		{
		TLSPROV_LOG(_L("PANIC: in Default state of the state machine..cant happen"))	
		}
		break;
	BULLSEYE_RESTORE
	}

}	
		
TBool CTlsProviderImpl::CheckExtendedKeyUsageL(const CX509Certificate& aCert)
	{
	TBool result(ETrue);
		
	const CX509CertExtension* ext = aCert.Extension(KExtendedKeyUsage);
	if (ext)
			{
			result = EFalse;
			CX509ExtendedKeyUsageExt* extendedKeyUsage = CX509ExtendedKeyUsageExt::NewLC(ext->Data());
			const CArrayPtrFlat<HBufC>& usages = extendedKeyUsage->KeyUsages();
			
			for (TInt k = 0; k < usages.Count(); k++)
            {
            if (usages[k]->Compare(KServerAuthOID) == 0)
                    {
                    result = ETrue;
                    break;
                    }
            }
        	CleanupStack::PopAndDestroy(); // Cannot leave before we get here
			}
				
	return result;
	}

void CTlsProviderImpl::HandleBadCertificateL(const TValidationStatus aResult)
	{
	TCacheEntryState entryState = iCacheClient.GetStateL();
				
	switch (entryState)
		{
	case ENewEntry:
		// Show dialog
		ShowUntrustedDialogL(aResult);
		break;
					
	case EEntryAwaitingApproval:
		// Request notification on status change
		iCurrentState = EQueryCache;
		iCacheClient.RequestNotify(iStatus);
		SetActive();
		break;
					
	case EEntryDenied:
		iCurrentState = iOriginalState = ENullState;
		User::RequestComplete(iOriginalRequestStatus,KErrAbort); 
		break;
					
	case EEntryApproved:
		iCurrentState = iOriginalState = ENullState;
		User::RequestComplete(iOriginalRequestStatus,KErrNone); 
		break;
					
		}
	}

void CTlsProviderImpl::ShowUntrustedDialogL(const TValidationStatus aResult)
	{
	iCurrentState = EUserDialog;
#ifdef _USESECDLGSV_
	TLSPROV_LOG(_L("Server Certificate validation failed old dialog"))	
	User::LeaveIfError( iDialogServ.Connect() );
    iDialogServ.DisplayCertNotTrustedDlg(iProceed, **iX509ServerCert, iStatus);
#else
	TLSPROV_LOG(_L("Server Authentication Failure New dialog"))	
	iSecurityDialog = SecurityDialogFactory::CreateL();
	iStatus = KRequestPending;			
	TPtrC8 encodedCert(iEncodedServerCerts->Des());
	// Calls Server authentication failure dialog box
	iSecurityDialog->ServerAuthenticationFailure(iTlsCryptoAttributes->idomainName,aResult.iReason,encodedCert,iStatus );
#endif
	SetActive();
	}

CTlsProviderImpl::~CTlsProviderImpl()
	{
	Cancel();
	
	if ( iTlsProviderPolicy )
	    {
	    delete iTlsProviderPolicy;
	    iTlsProviderPolicy = NULL;
	    }
	
#if !defined(_USESECDLGSV_)
	if(iSecurityDialog)
		{
		iSecurityDialog->Release();
		}
#endif
	iReqProtList.Close();

	if((!iTlsSessionOwnershipPassedToCaller) && (iTlsSessionImpl))
		{
		// We have created a CTlsSessionImpl (which now owns iSelectedCertInfo etc)
		// BUT the CTLSSession create failed so we still own the CTlsSessionImpl and 
		// are still responsible for deleting it.
		delete iTlsSessionImpl;
		iTlsSessionImpl = 0;
		}
	else
		{
		if(iTlsSessionImpl == NULL) //Session object not created
			{
			if(iSelectedCertInfo)
				iSelectedCertInfo->Release();	

			if(iSelectedKeyInfo)
				iSelectedKeyInfo->Release();
			
			if(iSessionInterface)		
				iSessionInterface->Release();	

			if(iTlsCryptoAttributes)
				delete iTlsCryptoAttributes;
			
			if(iEncodedServerCerts)
				delete iEncodedServerCerts;
			}
		}
	
	if(iServerCertsChain)
		{
		delete iServerCertsChain;	
		iServerCertsChain = NULL;
		}
	
	delete iCertVerificationResult;
	iCertVerificationResult = NULL;

	if(iPtrTokenSearch)
		{
		delete iPtrTokenSearch;	
		iPtrTokenSearch = NULL;
		}
	
	TInt count = iListAllTokensAndTypes.Count();
	TInt i(0);
	
	while(i<count)
		{
		iListAllTokensAndTypes[i].Release();
		i++;
		}
	
	// clear the intermediate certificates
	TInt certCount = iStoredIntermediatesCACertificates.Count();
	for (TInt i = 0; i < certCount; i++)
		{
		CX509Certificate *x509Cert = (CX509Certificate*)(iStoredIntermediatesCACertificates[i]);
		delete x509Cert;
		}				
	iStoredIntermediatesCACertificates.Reset();   				

	iListAllTokensAndTypes.Close();
	if(iClntAuthenticate)
		delete iClntAuthenticate;
   
   	iCacheClient.Close();

	iFs.Close();
	REComSession::FinalClose();
	}



void CTlsProviderImpl::CancelRequest()
	{
	if(IsActive())
		Cancel();
	}

void CTlsProviderImpl::DoCancel()
	{
	switch(iCurrentState)
	{			
	
	case EGetSessionInterface:
		{
		iPtrTokenSearch->CancelRequest();
		break;
		}
	case EGetCiphers:
		{
		if(iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface)
			iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface->CancelCryptoCapabilities();
		}
		break;
	case EGetSession:
		{
		if(iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface)
			iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface->CancelGetSession();
		}
		break;
	case EClearSessionCache:
		{
		if(iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface)
			iListAllTokensAndTypes[iCurrentTokentype].iProviderInterface->CancelClearSessionCache();
		}
		break;
	case EConstruct:
	case EConstructResumed:
		{
		if(iTlsSessionImpl)
			iTlsSessionImpl->CancelRequest();
		}
		break;			
	
	case EValidateCertificate:
		{
		if(iServerCertsChain)
			iServerCertsChain->CancelValidate();

		}
		break;
	case EBrowseTokens:
		{
		if(iPtrTokenSearch)
			iPtrTokenSearch->CancelRequest();
		}
		break;
		
	case EQueryCache:
		{
		iCacheClient.Cancel();
		}
		break;
	
	case EUserDialog:
		{
#ifdef _USESECDLGSV_
		iDialogServ.Cancel();
#else
		iSecurityDialog->Cancel();
#endif
		}
		break;
			
	default:
		break;
	}
	iCurrentState = ENullState;
	
	User::RequestComplete(iOriginalRequestStatus, KErrCancel);
	
	return;
	}	


void CTlsProviderImpl::ReturnResult()
	{
	switch(iOriginalState)
		{
		case EGetCiphers:
			{
			iCurrentTokentype= 0;
			TRAPD(err,ReturnCipherListL());
			if(err != KErrNone)
				{
				iUserCipherSuiteList->Reset();
				}
			User::RequestComplete(iOriginalRequestStatus, err);
			}
			break;
		case EGetSession:
			{
			iCurrentTokentype= 0;
			ReturnSession();
			}
			break;
		case EClearSessionCache:
			{
			User::RequestComplete(iOriginalRequestStatus, iStatus.Int());
			}
			break;
		BULLSEYE_OFF
		default:
			{
			//Not possible......Log him anyway
			}
			break;
		BULLSEYE_RESTORE
		}
	iCurrentState = ENullState;
	}


void CTlsProviderImpl::OnQueryCacheL()
	{
	HandleBadCertificateL(iValidationStatus);
	}

void CTlsProviderImpl::OnEUserDialogL()
	{
	TCacheEntryState entryStatus;
#ifdef _USESECDLGSV_	
  	entryStatus = iProceed ? EEntryApproved : EEntryDenied;
#else
	entryStatus = iStatus.Int() == KErrNone ? EEntryApproved : EEntryDenied;
#endif
	
	iCacheClient.SetStateL(entryStatus);
	User::RequestComplete(iOriginalRequestStatus, entryStatus == EEntryApproved ? KErrNone : KErrAbort);
	iCurrentState = iOriginalState = ENullState;
	}

TBool CTlsProviderImpl::SelectToken()
	{
	TLSPROV_LOG(_L("In CTlsProviderImpl::SelectToken()"))
	if(iSelectedTypeIndex == -1)
		{
		TIdentityRelation<TTLSCipherSuite> CipherOrder(CipherSuiteOrder);
		TInt selectedSoftwareToken = -1;
		TInt selectedHardwareToken = -1;
		TBool foundToken = EFalse;
		
		// Iterate through all tokens; pick a supported token
		TInt total = iListAllTokensAndTypes.Count();
		for(TInt i = 0; i < total; i++)
			{
			TLSPROV_LOG3(_L("token %d supports %d total ciphers"),
				  i, iListAllTokensAndTypes[i].iTokenInfo->iCipherSuitesSupported.Count())
			if ((iListAllTokensAndTypes[i].iTokenInfo)->iCipherSuitesSupported.Find(
					iTlsCryptoAttributes->iCurrentCipherSuite, CipherOrder) != KErrNotFound)
				{
				// Select the first valid token found (from zero upwards)
				foundToken = ETrue;
				if (iListAllTokensAndTypes[i].iSoftwareToken && selectedSoftwareToken == -1)
					selectedSoftwareToken = i;
				
				if (! iListAllTokensAndTypes[i].iSoftwareToken && selectedHardwareToken == -1)
					selectedHardwareToken = i;
				}
			
			// No need to continue if we've found a hardware token, otherwise keep
			// searching, since there might be a hardware token later in the search
			// and this should always be used in preference to a software token
			if (selectedHardwareToken != -1)
				break;
			}
			
		if (foundToken == EFalse)
			{
			TLSPROV_LOG(_L("Didn't find a suitable token for this cipher"))
			return EFalse;
			}
		
		// Prefer a hardware token
		if (selectedHardwareToken != -1)
			{
			iSelectedTypeIndex = selectedHardwareToken;
			TLSPROV_LOG2(_L("Hardware Token %d selected"), iSelectedTypeIndex)
			}
		else
			{
			iSelectedTypeIndex = selectedSoftwareToken;
			TLSPROV_LOG2(_L("Software Token %d selected"), iSelectedTypeIndex)
			}
		}

	return ETrue;
	}


/*The following function gets the encoded certificate and converts it into
an x509 object..Only called from a direct secure socket API call	
*/
void CTlsProviderImpl::GetX509CertL(HBufC8*& aEncodedCert,CX509Certificate*& aOutputX509)
	{
	if(aEncodedCert)
		{
		TLSPROV_LOG(_L("CTlsProviderImpl::GetX509CertL"))	
		CPKIXCertChain* CertsChain = CPKIXCertChain::NewLC(iFs,aEncodedCert->Des(),
												  TUid::Uid(KUidUnicodeSSLProtocolModule));
		if(CertsChain->Count())
			{
   		TLSPROV_LOG(_L("CX509Certificate::NewL"))	
			aOutputX509 = CX509Certificate::NewL(CertsChain->Cert(0));		
			}
		CleanupStack::PopAndDestroy(CertsChain);	
		}
	}

TBool CTlsProviderImpl::ValidateDNSNameL(const CX509Certificate& aSource)
	{
	TBool ret = EFalse;
	TBool hasAltNameExt = EFalse;
	
	// Get a unicode descriptor with the server DNS name
	HBufC* server = HBufC::NewLC(iTlsCryptoAttributes->idomainName.Length());
	TPtr des = server->Des();
	des.Copy(iTlsCryptoAttributes->idomainName);
	
	// and contruct a domain name from it
	CX509DNSName* serverDns = CX509DNSName::NewLC(*server);
	CX509DNSName* dNS = NULL;
	
	// try fetch Alt Name first (see RFC 2595)
	const CX509CertExtension* ext = aSource.Extension(KSubjectAltName); 
	if (ext)
		{
		CX509AltNameExt* subjectAltName = CX509AltNameExt::NewLC(ext->Data()); 
		const CArrayPtrFlat<CX509GeneralName>& gNs = subjectAltName->AltName();
		TInt count = gNs.Count(); 

		for (TInt i = 0; i < count; i++)
			{ 
			const CX509GeneralName* gN = gNs.At(i); 
			if (gN->Tag() == EX509DNSName) 
				{
				hasAltNameExt = ETrue;
				
				// wildcards are allowed at every part of DNS name
				// e.g. "foo.example.com" would be validated
				// against cert "foo.*.com", even though RFC 2595 states:
				// "A '*' wildcard character MAY be used as the left-most name 
				// component in the certificate"
				TBool wildcard = (gN->Data().Locate('*') > KErrNotFound);
				dNS = CX509DNSName::NewL(gN->Data()); 
	
				// Check if the server matches this cert name
				if (NameIsInSubtree(*serverDns, *dNS, wildcard))
					{
					ret = ETrue;
					}
					
				delete dNS;
				if ( ret )
					break;
				} 
			} 
			CleanupStack::PopAndDestroy(subjectAltName); 
		}
		
	// if the certificate has an alternative name extension, with DNS entries, ignore the common name	
	if(!hasAltNameExt)
		{
		// no name yet lets try common name from Subject
		HBufC* commonName = aSource.SubjectName().ExtractFieldL(KX520CommonName);
		if(commonName)
			{
			CleanupStack::PushL(commonName);
			TPtr name = commonName->Des();
			// check for wildcard, we consider it only if left-most name component (see RFC 2595)
			TBool wildcard = EFalse;
			if(0 == name.Locate('*'))
				{
				// wildcard as the left-most part,
				// lets return the name without it...
				// Do NOT remove the '.' otherwise DNS name "example.com" would validate
				// against cert "*.example.com" and that's not correct
				name = name.Mid(1);
				wildcard = ETrue;
				}
			dNS = CX509DNSName::NewL(name);
			ret = NameIsInSubtree(*serverDns, *dNS, wildcard);
			delete dNS;
			CleanupStack::PopAndDestroy(commonName);
			}
		}
		
	CleanupStack::PopAndDestroy(2, server);
	return ret;
	}

TBool CTlsProviderImpl::NameIsInSubtree(CX509DNSName& aServerName, CX509DNSName& aCertName, TBool aIsWildcard)
	{
	return (aServerName.IsWithinSubtree(aCertName) && 
  		(aIsWildcard || (aCertName.Name().Length() == aServerName.Name().Length())));
	}

TInt CTlsProviderImpl::RunError(TInt aError)
	{
	
	if (iCurrentState == EUserDialog)
		{
		// Treat this as denied
		// ignore any problems writing this entry to disk
		TRAP_IGNORE(iCacheClient.SetStateL(EEntryDenied));
		}
	
	User::RequestComplete(iOriginalRequestStatus, aError);
	return KErrNone;
	}


MCTToken* CTlsProviderImpl::GetTokenHandle()
	{
	
	if(iListAllTokensAndTypes.Count() > iSelectedTypeIndex && iSelectedTypeIndex >=0 )
		{
      CTokenTypesAndTokens& Tokens = iListAllTokensAndTypes[iSelectedTypeIndex];
      if ( Tokens.iProviderInterface )
         {
   		MCTToken& tempPtr = Tokens.iProviderInterface->Token();
		   iListAllTokensAndTypes[iSelectedTypeIndex].iProviderInterface->Release();
		   iListAllTokensAndTypes[iSelectedTypeIndex].iProviderInterface = 0;
   		return  &tempPtr;	
         }
		}
	return NULL;
	}

void CTokenTypesAndTokens::Release()
	{
	if(iProviderInterface)
		{
		MCTToken& tempPtr = iProviderInterface->Token();
		iProviderInterface->Release();	//Release provider interface
		tempPtr.Release();	//Release token
		iProviderInterface = 0;
		}
	if(iTokenInfo)
		{
		iTokenInfo->Close();
		delete iTokenInfo;
		}
		
	}

CTokenTypesAndTokens::~CTokenTypesAndTokens()
	{
	Release();	
	}

void CTokenInfo::Close()
	{
	aKeyExchAlgs.Close();
	aSignatureExchAlgs.Close();
	iCipherSuitesSupported.Close();
	iSupportedProtocols.Close();
	}

CTokenInfo::~CTokenInfo()
	{
	Close();
	}

void CTlsProviderImpl::ConstructL()
	{
	User::LeaveIfError(iFs.Connect());
	
    TLSPROV_LOG(_L("Creating a newer instance of TlsProviderPolicy"))
    iTlsProviderPolicy = CTlsProviderPolicy::NewL();
	}

CTLSPublicKeyParams::~CTLSPublicKeyParams()
	{
	delete iValue1;
	delete iValue2;
	delete iValue3;	
	delete iValue4;
	delete iValue5;
	}
