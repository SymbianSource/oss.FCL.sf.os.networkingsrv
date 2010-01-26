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

#include "Ctlsclntauthenticate.h"
#include <cctcertinfo.h>
#include <ccertattributefilter.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif

//
//  CTlsClntAuthenticate
//


CTlsClntAuthenticate* CTlsClntAuthenticate::NewL(const CTlsCryptoAttributes& aTlsCryptoAttributes,
												 HBufC8* aEncodedServerCerts)
	{ 
	CTlsClntAuthenticate* aPtrClnt = new (ELeave)CTlsClntAuthenticate(
		aTlsCryptoAttributes, aEncodedServerCerts);
	return(aPtrClnt);
	}


CTlsClntAuthenticate::CTlsClntAuthenticate(const CTlsCryptoAttributes& aTlsCryptoAttributes,
												 HBufC8* aEncodedServerCerts)
	: CActive(0),
	  iTlsCryptoAttributes(aTlsCryptoAttributes),
	  iEncodedServerCerts(aEncodedServerCerts)
	{
	if(CActiveScheduler::Current()) //Allready installed?
		{
		CActiveScheduler::Add( this );
		}
	}


void CTlsClntAuthenticate::DoClientAuthenticate(
				CCTCertInfo*& aSelectedCertInfo,
				CCTKeyInfo*& aSelectedKeyInfo,
				RPointerArray<CCertificate>* aStoredIntermediatesCACertificates,
				TRequestStatus& aStatus)
	{
  	TLSPROV_LOG(_L("CTlsClntAuthenticate::DoClientAuthenticate()..."))
	aStatus = KRequestPending;
	iOriginalRequestStatus = &aStatus;
	iCurrentState = ENullState;
	TInt err = KErrNone;

	if(IsActive())
		{
		err = KErrInUse;			
		}
	else
		{

		iSelectedCertInfo = &aSelectedCertInfo;
		iSelectedKeyInfo = &aSelectedKeyInfo;
		iStoredIntermediatesCACertificates = aStoredIntermediatesCACertificates;

		err=iFs.Connect();
		if (err==KErrNone)
			{
			TRAP(err, GetAvailableKeyListL());
			}
		}	
	
	if(err != KErrNone)
		User::RequestComplete(iOriginalRequestStatus, err);

	
	}


void CTlsClntAuthenticate::GetAvailableKeyListL()	
	{	
  	TLSPROV_LOG(_L("CTlsClntAuthenticate::GetAvailableKeyListL()..."))
	iFs.Connect();
	iPtrUnifiedKeyStore = CUnifiedKeyStore::NewL(iFs);
	
	iStatus = KRequestPending;
	iCurrentState =  EGetAvailableKeyList;
	iPtrUnifiedKeyStore->Initialize(iStatus);
	SetActive();
	return;	
	}



void CTlsClntAuthenticate::OnGetAvailableKeyListL()
	{

	TInt KeyStoreCount = iPtrUnifiedKeyStore->KeyStoreCount();
  	TLSPROV_LOG2(_L("CTlsClntAuthenticate::GetAvailableKeyListL() KeyStoreCount = %d..."), KeyStoreCount)
	
	MCTKeyStore* IndividualKeyStore;
	for(TInt i = 0; i<KeyStoreCount;i++ )
		{
		IndividualKeyStore= &(iPtrUnifiedKeyStore->KeyStore(i));	
		User::LeaveIfError(iSupportedKeyStores.Append(IndividualKeyStore));		
		}

	iKeyCount = iSupportedKeyStores.Count();
	if(iKeyCount)
		{
		iCurrentState = EGetKeyList;
		TRequestStatus* MyStatus = &iStatus;
		User::RequestComplete(MyStatus,KErrNone);
		SetActive();
		return;
		}
	else
		User::Leave(KErrNotFound);
		
	}



void CTlsClntAuthenticate::OnEGetKeyListL()
	{
	
  	TLSPROV_LOG2(_L("CTlsClntAuthenticate::GetAvailableKeyListL() iKeyCount = %d..."), iKeyCount)
	if((iKeyCount > 0))
		{
		TCTKeyAttributeFilter keysFilter;
		keysFilter.iUsage = EPKCS15UsageSign; 

		keysFilter.iKeyAlgorithm =  iTlsCryptoAttributes.isignatureAlgorithm == ERsaSigAlg ? CCTKeyInfo::ERSA : CCTKeyInfo::EDSA;

		iCurrentState = EGetKeyList;
		iStatus = KRequestPending;
		iSupportedKeyStores[--iKeyCount]->List(iKeyInfos , keysFilter, iStatus );
		SetActive();
		return;
		}
	iSupportedKeyStores.Reset();
	
	if(iKeyInfos.Count())
		{
		
		iKeyCount = iKeyInfos.Count();
     	TLSPROV_LOG2(_L("CTlsClntAuthenticate::GetAvailableKeyListL() iKeyCount in key store= %d..."), iKeyCount)
		iPtrFilter =  CCertAttributeFilter::NewL();
 		//Only user certificates are considered for client authentication
 		iPtrFilter->SetOwnerType(EUserCertificate);
		iPtrUnifiedCertStore = CUnifiedCertStore::NewL(iFs,EFalse);
		
		iStatus = KRequestPending;
		
		CTlsProviderPolicy*	tlsProviderPolicy(NULL);
 		TRAP_IGNORE(tlsProviderPolicy=CTlsProviderPolicy::NewL());
 		
 		if (tlsProviderPolicy)
 			{
 			iClientAuthenticationDlgEnabled=tlsProviderPolicy->ClientAuthenticationDialogEnabled();
 			delete tlsProviderPolicy;
 			} 
 					
 		//if the issuer name is empty, then we will not filter the cert by issuer's DN name.
 		//otherwise the Intermediates CAcerts need to be retrieved in order to add more issuer name.
 		TInt count=iTlsCryptoAttributes.iDistinguishedCANames.Count();
 		if (!count)
 			{
 			iCurrentState = EGetCertList;	
 			}
 		else
 			{
 			//start building the full list
 			for (TInt i=0;i<count;i++)
 				{
 				HBufC8* tmp=iTlsCryptoAttributes.iDistinguishedCANames[i]->AllocLC();
 				TLSPROV_LOG2(_L("iDistinguishedCANames[%d] :"),i)
 				TLSPROV_LOG_HEX(iTlsCryptoAttributes.iDistinguishedCANames[i]->Ptr(),iTlsCryptoAttributes.iDistinguishedCANames[i]->Length())
 				iFullIssuerDistinguishedCANames.AppendL(tmp);
 				CleanupStack::Pop(tmp);
 				
				//start building the current work list of issuer's name	
				HBufC8* tmp1=iTlsCryptoAttributes.iDistinguishedCANames[i]->AllocLC();
				iCurrentIssuerDistinguishedCANames.AppendL(tmp1);
				CleanupStack::Pop(tmp1);
 				}
 				
 				//Only retrieve the CA cert till there are no child CA certs left
 				iPtrIntermediatesCACertFilter=CCertAttributeFilter::NewL();
 				iPtrIntermediatesCACertFilter->SetOwnerType(ECACertificate);
 				iToListIntermediatesCACert=ETrue;
 				iCurrentState = EListIntermediatesCACerts;					
 			}			

		iPtrUnifiedCertStore->Initialize(iStatus);
		SetActive();
		}
	else
		User::Leave(KErrNotFound);
	}

void CTlsClntAuthenticate::OnEListIntermediatesCACertsL()
	{
	//Only when we need to list all the CA cert and there are new issuer name avalible
	if (iToListIntermediatesCACert && iCurrentIssuerDistinguishedCANames.Count()>0)
		{
		//List all the intermediates CA certificate.
		iPtrUnifiedCertStore->List(iIntermediatesCACertInfos, 
									*iPtrIntermediatesCACertFilter, 
									iCurrentIssuerDistinguishedCANames, 
									iStatus);
		iCurrentState = EListIntermediatesCACerts;
		//Clear the flag for next time
		iToListIntermediatesCACert=EFalse;
		SetActive();				
		}
	else
		{
		//Reset the iCurrentIssuerDistinguishedCANames for the next level CA Cert
		iCurrentIssuerDistinguishedCANames.ResetAndDestroy();
		iIntermediatesCACertCount=iIntermediatesCACertInfos.Count();
		iToListIntermediatesCACert=EFalse;
		
		if (iIntermediatesCACertCount>0)
			{
			//add more intermediate CA certs in next state if possible
			iCurrentState = EGetIntermediatesIssuer;
			}
		else
			{
			//Got all the issuer's DN name and Intermediate CA certs
			iCurrentState = EGetCertList;				
			}

		TRequestStatus* MyStatus = &iStatus;
		User::RequestComplete(MyStatus,KErrNone);
		SetActive();
		}
	}

#ifdef _DEBUG	
static void LogHBufC(TRefByValue<const TDesC> aFmt, HBufC* aBuffer)
	{
    HBufC *buf = HBufC::NewL(aBuffer->Length() + 1);
    buf->Des().FillZ();
    buf->Des().Copy(aBuffer->Des());
   	TLSPROV_LOG2(aFmt,buf->Des().PtrZ())
	delete buf;
	}

static void LogCert(CX509Certificate* aX509Cert, TRefByValue<const TDesC> aMsg, TBool aFound)
	{
	HBufC *pIssuer = aX509Cert->IssuerL();
	HBufC *pSubject = aX509Cert->SubjectL();
	if ( aFound )
		{
		TLSPROV_LOG(aMsg) 
		TLSPROV_LOG(_L(" (found)")) 
		}
	else
		{
		TLSPROV_LOG(aMsg)
		TLSPROV_LOG(_L(" (not found)"))
		}
   	LogHBufC(_L("Issuer: %s"), pIssuer);
   	LogHBufC(_L("Subject: %s"), pSubject);
	delete pSubject;
	delete pIssuer;
	}
#endif

TBool CTlsClntAuthenticate::LookupCert(CX509Certificate* aCert)
	{
	TBool found = EFalse;
	TInt certCount = iStoredIntermediatesCACertificates->Count();
	for (int i = 0; i < certCount; i++)
		{
		CX509Certificate *x509Cert = (CX509Certificate*)((*iStoredIntermediatesCACertificates)[i]);
#ifdef _DEBUG
		LogCert(x509Cert, _L("Verified cert"), ETrue);
#endif
		if ( x509Cert->IsEqualL(*aCert) )
			{
			found = ETrue;
			break;
			}
		}
	return found;
	}
	
void CTlsClntAuthenticate::OnEGetIntermediatesIssuerL()
	{
	//update the issuer list
	if (iIntermediatesCertificate)
		{
		//get the x509 certificate and the certs' subject name
		CX509Certificate* x509Cert=(CX509Certificate*)iIntermediatesCertificate;
		const TPtrC8* subName=x509Cert->DataElementEncoding(CX509Certificate::ESubjectName);

		//Check if the intermediate CA certificate's subject name is already in the final list
		TBool found(EFalse);
		TInt count=iFullIssuerDistinguishedCANames.Count();
		for (TInt i=0;i<count;i++)
			{
			if (*subName==*iFullIssuerDistinguishedCANames[i])
				{
				// if found then break immediately
				found=ETrue;
				break;
				}
			}
   
		//update the issuer DN name arrays and current working issuer array
		//only if the name is not found before
		if (!found)
			{
#ifdef _DEBUG
			LogCert(x509Cert, _L("New Certificate"), EFalse);
#endif			
			HBufC8* subject = NULL;
			
			subject = subName->AllocLC();
			iFullIssuerDistinguishedCANames.AppendL(subject);
			CleanupStack::Pop(subject);
			
			subject = subName->AllocLC();
			iCurrentIssuerDistinguishedCANames.AppendL(subject);
			CleanupStack::Pop(subject);

			// Store a copy of new imtermediate certificate
			CX509Certificate *cert = CX509Certificate::NewL(*x509Cert);
			iStoredIntermediatesCACertificates->Append(cert);
			}
		else
			{
				if ( !LookupCert(x509Cert) )
					{
					// Store a copy of new certificate (probably requested CA)
					CX509Certificate *cert = CX509Certificate::NewL(*x509Cert);
					iStoredIntermediatesCACertificates->Append(cert);
					}
#ifdef _DEBUG
			LogCert(x509Cert, _L("Existing Certificate"), ETrue);
#endif
			}
		//delete the certificate for next retrieve
		delete iIntermediatesCertificate;
		iIntermediatesCertificate=NULL;
		}
		
	//Go through all the intermediate CA certs in the level.	
	if (iIntermediatesCACertCount-- > 0)
		{
		iCurrentState = EGetIntermediatesIssuer;
		//Retrieve the certificate in order to get the subject DN name.
		iPtrUnifiedCertStore->Retrieve(*iIntermediatesCACertInfos[iIntermediatesCACertCount], iIntermediatesCertificate, iStatus);
		}
	else
		{
		//If this level's CA certs are done, go back to EListIntermediatesCACerts
		iCurrentState = EListIntermediatesCACerts;
		//Reset the certinfo and the count for next level intermediate CA certs
		iIntermediatesCACertCount=iIntermediatesCACertInfos.Count();

		for (TInt i=iIntermediatesCACertCount-1;i>=0;i--)
			{
			CCTCertInfo* tmp=iIntermediatesCACertInfos[i];
			TLSPROV_LOG2(_L("Found Intermediate Cert: <%s>"),tmp->Label().Ptr())
			iIntermediatesCACertInfos.Remove(i);
			tmp->Release();	
			}
		iIntermediatesCACertInfos.Reset();
		iIntermediatesCACertCount=0;
		iToListIntermediatesCACert=ETrue;
		
		//All the intermediate CA in this level has been processed.
		TRequestStatus* MyStatus = &iStatus;
		User::RequestComplete(MyStatus,KErrNone);			
		}
	SetActive();
	}


void CTlsClntAuthenticate::OnEGetCertListL()
	{
	TInt CertList;
		
	if(iKeyCount-- > 0)
		{		
		CertList = iCertInfos.Count();

		iPtrFilter->SetSubjectKeyId(iKeyInfos[iKeyCount]->ID() );

		iCurrentState = EGetCertList;
		iStatus = KRequestPending;

 		if(iFullIssuerDistinguishedCANames.Count())
         {
        	TLSPROV_LOG2(_L("CTlsClntAuthenticate::OnEGetCertListL() distinquished names...CertListCount = %d"), CertList)
        	TLSPROV_LOG(_L("iPtrFilter->iSubjectKeyId"))
          	TLSPROV_LOG_HEX( iPtrFilter->iSubjectKeyId.Ptr(), iPtrFilter->iSubjectKeyId.Length() )
        	TLSPROV_LOG2(_L("iTlsCryptoAttributes.iDistinguishedCANames.Count() = %d"), iTlsCryptoAttributes.iDistinguishedCANames.Count())
        	for (int i = 0; i < iTlsCryptoAttributes.iDistinguishedCANames.Count(); i++)
        		{
	         	TLSPROV_LOG_HEX( iTlsCryptoAttributes.iDistinguishedCANames[i]->Ptr(), iTlsCryptoAttributes.iDistinguishedCANames[i]->Length() )
        		}
			iPtrUnifiedCertStore->List(iCertInfos,
						   *iPtrFilter, 
						   iFullIssuerDistinguishedCANames, 
						   iStatus);
         }
		else
         {
         	// We can not filter by Issuers DN, so retrieve a list of all client certs
        	TLSPROV_LOG(_L("CTlsClntAuthenticate::OnEGetCertListL()..."))
        	TLSPROV_LOG(_L("iPtrFilter->iSubjectKeyId"))
         	TLSPROV_LOG_HEX( iPtrFilter->iSubjectKeyId.Ptr(), iPtrFilter->iSubjectKeyId.Length() )
			iPtrUnifiedCertStore->List(iCertInfos,
						   *iPtrFilter, 
      					   iStatus);
         }
		
		SetActive();
		return;
		}	

	// we have all the certificate candidates, so let's check to see if any have expired and remove them if they are...
	RemoveExpiredCertificatesL();
		
	CertList = iCertInfos.Count();

	if(CertList)
		{
		//Display dialog

     	TLSPROV_LOG(_L("CTlsClntAuthenticate::OnEGetCertListL() create & display certificate list..."))
		while(CertList--)
			{
 			User::LeaveIfError(iClientCertHandleList.Append(iCertInfos[CertList]->Handle()));
			}
		iCurrentState = EUserDialog;

		if (iClientAuthenticationDlgEnabled)
			{
			TLSPROV_LOG(_L("Client Certificate Selection Dialog"))	
			iSecurityDialog = SecurityDialogFactory::CreateL();
			iStatus = KRequestPending;			
			TBool doClientAuthentication(ETrue);
			TPtrC8 encodedCert(iEncodedServerCerts->Des());
			iSecurityDialog->EstablishSecureConnection(encodedCert, iClientCertHandleList, MSecurityDialog::ETLS, doClientAuthentication, iUserSelectedCert, iStatus);
			SetActive();	
			return;
			}

		iUserSelectedCert = iClientCertHandleList[0];
		TRequestStatus* MyStatus = &iStatus;
		SetActive();
		User::RequestComplete(MyStatus,KErrNone);
		}	
	else
		User::Leave(KErrNotFound);
	}


void CTlsClntAuthenticate::OnEUserDialogL()
	{
 	iClientCertHandleList.Reset();
	TInt err = iStatus.Int();

	if(!err)
		{
		
     	TLSPROV_LOG(_L("CTlsClntAuthenticate::OnEUserDialogL() certificate selected"))
		TInt CertList = iCertInfos.Count();	
		TInt KeyList = iKeyInfos.Count();
		
		TLSPROV_LOG3(_L("CertList = %d, KeyList = %d"),CertList,KeyList);

		for(TInt i = 0; i < CertList; i++)
			{
			if( iCertInfos[i]->Handle() == iUserSelectedCert)
				{
				//Matching certificate found
				
				for(TInt j = 0; j < KeyList; j++ )
					{
					if(iKeyInfos[j]->ID() == iCertInfos[i]->SubjectKeyId())
						{
						// We can pass ownership of the selected cert info back
						// to the client, but need a key info without the
						// protector, so a new CCTKeyInfo object is created
						// here that is almost a copy.
				     	TLSPROV_LOG2(_L("\tSelected cert info %08x"),
				     				iCertInfos[i]);
						TLSPROV_LOG2(_L("\tSelected key info %08x"),
				     				iKeyInfos[j]);

						*iSelectedCertInfo =  iCertInfos[i];
						CCTKeyInfo& keyInfo = *(iKeyInfos[j]);
						HBufC* label = keyInfo.Label().AllocLC();

						*iSelectedKeyInfo = CCTKeyInfo::NewL(keyInfo.ID(),
															keyInfo.Usage(),
															keyInfo.Size(),
															NULL,
															label,
															keyInfo.Token(),
															keyInfo.HandleID(),
															keyInfo.UsePolicy(),
															keyInfo.ManagementPolicy(),
															keyInfo.Algorithm(),
															keyInfo.AccessType(),
															keyInfo.Native(),
															keyInfo.StartDate(),
															keyInfo.EndDate()
															);
						CleanupStack::Pop(label);

						iCertInfos.Remove(i);
						
						User::RequestComplete(iOriginalRequestStatus, KErrNone);
						return;
						}					
					}
				}
				
			}			
		}
	
	User::Leave(KErrNotFound);
	
	}



void CTlsClntAuthenticate::RunL()
	{
	User::LeaveIfError(iStatus.Int());
	switch(iCurrentState)
	{
	case EGetAvailableKeyList:
		{
		OnGetAvailableKeyListL();		
		}
		break;
	
	case EGetKeyList:
		{
		OnEGetKeyListL();	
		}
		break;

 	case EListIntermediatesCACerts:
 		{
 		OnEListIntermediatesCACertsL();	
 		}
 		break;
 		
 	case EGetIntermediatesIssuer:			
 		{
 		OnEGetIntermediatesIssuerL();
 		}
 		break;	

	case EGetCertList:
		{
		OnEGetCertListL();
		}
		break;
	
	case EUserDialog:
		{
		OnEUserDialogL();
		}
		break;

	default:
		User::Leave(KErrNotFound);
		break;
	}
	}

void CTlsClntAuthenticate::CancelRequest()
	{
	Cancel();
	}

TInt CTlsClntAuthenticate::RunError(TInt aError)
	{
	User::RequestComplete(iOriginalRequestStatus, aError);
	return KErrNone;
	}

void CTlsClntAuthenticate::DoCancel()
	{
	switch(iCurrentState)
	{
	case EGetAvailableKeyList:
		{
		if(iPtrUnifiedKeyStore)
			iPtrUnifiedKeyStore->CancelInitialize();
		}
		break;
	
	case EGetKeyList:
		{
		if(iSupportedKeyStores[iKeyCount])
			iSupportedKeyStores[iKeyCount]->CancelList();		
		}
		break;
	
	case EListIntermediatesCACerts:
		{
		if(iPtrUnifiedCertStore)
			{
			iPtrUnifiedCertStore->CancelInitialize();
			iPtrUnifiedCertStore->CancelList();
			}			
		}
		break;
 		
	case EGetIntermediatesIssuer:			
		{
		if(iPtrUnifiedCertStore)
			{
			iPtrUnifiedCertStore->CancelRetrieve();
			}			
		}
		break;

	case EGetCertList:
		{
		if(iPtrUnifiedCertStore)
			{
			iPtrUnifiedCertStore->CancelList();
			}		
		}
		break;
	default:
		break;
	}
	iCurrentState = ENullState;
	User::RequestComplete(iOriginalRequestStatus, KErrCancel);
	}


CTlsClntAuthenticate::~CTlsClntAuthenticate()
	{
	iKeyInfos.Close();
	iCertInfos.Close();
	delete iPtrUnifiedKeyStore;
	delete iPtrFilter;
	delete iPtrUnifiedCertStore;

	if(iSecurityDialog)
		iSecurityDialog->Release();
	
	iSupportedKeyStores.Close();
	iFs.Close();
 	iClientCertHandleList.Close();
 	iFullIssuerDistinguishedCANames.ResetAndDestroy();
 			
 	iIntermediatesCACertInfos.Close();
 	delete iPtrIntermediatesCACertFilter;
 	iCurrentIssuerDistinguishedCANames.ResetAndDestroy();

	}

	
// return information about the specified key
CCTKeyInfo* CTlsClntAuthenticate::GetKeyInfoL(const TKeyIdentifier& aKeyID)
	{	
	TInt key;
	// check the key info list for a match and return its details
	for (key=0; key<iKeyInfos.Count(); key++)
		{
		CCTKeyInfo* info = iKeyInfos[key];
		if (info->ID()==aKeyID)
			return info;
		}
	return 0;
	}

// removes expired certificates from the certificate list. This is based on the start/end time of the key assoiated with the issuer
void CTlsClntAuthenticate::RemoveExpiredCertificatesL()
	{
	TInt index = 0;
	TTime now;
	
	// get time now
	now.UniversalTime();
	while (index < iCertInfos.Count())
		{
		TBool bExpired=EFalse;
		CCTCertInfo* cert = iCertInfos[index];
		
		// check that issuer key not expired
		CCTKeyInfo* issuer = GetKeyInfoL(cert->IssuerKeyId());
		if (issuer && (issuer->StartDate()>now || issuer->EndDate()<now))
			bExpired = ETrue;
		
		// remove from list if it has expired
		if (bExpired) 
			{
			iCertInfos.Remove(index);
			}
		else index++;
		}
	}
