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

#include "TlsProvTestStep.h"
#include <test/testexecutelog.h>
#include <hash.h>

extern TInt ReadTestDataL( CTlsCryptoAttributes*& aTlsCryptoAttributes, HBufC8*& aSrvCert,
				   CTlsProvStep*& aTestStepPtr);

extern TInt ReadDataFromFiles( CTlsCryptoAttributes* aTlsCryptoAttributes, 
						HBufC8*& aSrvCert);


/*
Tests covered:Test for rejecting
	1.Unsupported protocol version
	2.Unsupported ciphersuite
	3.Wrong size server random number
*/
const TTLSProtocolVersion KTest_0 = {4,4}; 
TVerdict CTlsProvTestActive::TestProvider_3_0L( CTlsProvStep*  )
	{

	
	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();

	CTlsCryptoAttributes* TlsCryptoAttributes =  PtrProvider->Attributes();


	TlsCryptoAttributes->iNegotiatedProtocol = KTest_0;
	iStatus = KRequestPending;
	CTLSSession* aPtrTlsSession;
	PtrProvider->CreateL(aPtrTlsSession,iStatus);
	SetActive();
	CActiveScheduler::Start();

	
	if(iStatus.Int() == (TInt)KErrSSLAlertIllegalParameter)
		{
				
		TlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;
		TlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 20;
		TlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 20;
		iStatus = KRequestPending;
		PtrProvider->CreateL(aPtrTlsSession,iStatus);
		SetActive();
		CActiveScheduler::Start();
	
		if(iStatus.Int() == (TInt)KErrSSLAlertIllegalParameter)
			{
			TlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 0;
			TlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 3;
			iStatus = KRequestPending;
			PtrProvider->CreateL(aPtrTlsSession,iStatus);
			SetActive();
			CActiveScheduler::Start();
			if(iStatus.Int() == (TInt)KErrSSLAlertIllegalParameter)
				{
				return EPass;
				}

			}
		}			
		return EFail;
	}

/*
Tests covered:Test for rejecting
	Select a token without client authentication, software token will always be selected
*/

TVerdict CTlsProvTestActive::TestProvider_3_1L( CTlsProvStep* aStep )
	{

	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;

	 
	__UHEAP_MARK;
	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,ETrue,EFalse,aStep);

	
	//Want client authentication?
	PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

	//Any dialogs?
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 3;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;	

	iStatus = KRequestPending;	
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();
	delete PtrProvider;
	if(PtrSession)
		{
		delete PtrSession;
		__UHEAP_MARKEND;
		return EPass;
		}
	else
		return EFail;
	}

/*
Tests covered:
	1.Use client authentication(Provider browses for matching certs and keys and then 
	  selects a token) 
	Check the correct client cert is being selected
*/
TVerdict CTlsProvTestActive::TestProvider_3_2L(CTlsProvStep* aStep)
	{
	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	CTlsCryptoAttributes* PtrTlsCryptoAttributes = tlsProvider->Attributes();;
	
	HBufC8* encServerCert = NULL;
	TInt res = StandardTestInitL(aStep, PtrTlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	3.2:  problems with test data files") );
		return EFail;
		}
	
	__UHEAP_MARK; 
	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,ETrue,EFalse,aStep);

	//Want client authentication? Yes!, Please.
	PtrTlsCryptoAttributes->iClientAuthenticate = ETrue;

	//Any dialogs?
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 3;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;
   PtrTlsCryptoAttributes->isignatureAlgorithm = ERsaSigAlg;

   	AddDNL();
	const TDesC8& issuerName = reinterpret_cast<const TDesC8&>(*iDNs[0]);
	PtrTlsCryptoAttributes->iDistinguishedCANames.Append(&issuerName);

	iDNs.Close();

	iStatus = KRequestPending;	
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();
	TVerdict ret=EFail;
	CX509Certificate* X509ClientCert = 0;
	if(PtrSession)
		{

		iStatus = KRequestPending;	
		PtrSession->ClientCertificate(X509ClientCert,iStatus);
		SetActive();
		CActiveScheduler::Start();

		//to make ccover happy
		CTlsCryptoAttributes* tempAttributes = PtrProvider->Attributes();
		if(tempAttributes->iNegotiatedProtocol != KTLS1_0)
			return EFail;

		PtrProvider->CancelRequest();
		PtrSession->CancelRequest();
		if(X509ClientCert)
			{
			ret=EPass;
			}
      else
         {
   		iLogInfo.Copy( _L("	No Client certificate found") );
         }
		//Check for the subject name too
		delete X509ClientCert;
		delete PtrProvider;
		//delete PtrSession;
		__UHEAP_MARKEND; 
			
		}

		return ret;
	}


/*
Tests covered:
	1.Use client authentication, Create a connection, Store the session and emulate abbrevated connections

  Note: This step tests the provider on its behaviour on resuming sessions and these tests are NOT
		testing the actual session caching functionality of the token. They are only using the functionality.
*/

#define KServer1  _L8("192.168.30.2")
#define KSessionId1 _L8("11111111112222222222333333333322")
TVerdict CTlsProvTestActive::TestProvider_3_3L(CTlsProvStep* aStep)
	{
	
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
		
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	
	SetActive();
	CActiveScheduler::Start();
	
	CTLSSession* sessionObj = NULL;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
			
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	3.3:  problems with test data files") );
		return EFail;
		}
	
				
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	serverCert = CX509Certificate::NewL(encServerCert->Des()); //- remove when VerifyServerCertificate ready
		
	
	
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
		
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.0:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
	
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.0:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;

	//Create a new object here..Try resuming the same session and get the same server cert back

	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;
	

	iStatus = KRequestPending;	
	userCipherSuiteList.Reset();
	PtrProvider->CipherSuitesL( userCipherSuiteList ,iStatus);		
	SetActive();
	CActiveScheduler::Start();
	if(userCipherSuiteList.Count() != KSupportedCipherCount)
		return EFail;


	iStatus = KRequestPending;
	TTLSServerAddr ServerName;
	TTLSSessionId SessionId;
	ServerName.iAddress.Copy( KServer1 );
	ServerName.iPort = 10;
	PtrProvider->GetSessionL(ServerName,SessionId,iStatus);
	SetActive();
	CActiveScheduler::Start();

	//The same session id Should be returned here
	if(SessionId.Compare(KSessionId1) == 0)
		{
		PtrTlsCryptoAttributes = PtrProvider->Attributes();
		InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,ETrue,EFalse,aStep);

		//Want client authentication?
		PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

		//Any dialogs?
		PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

		//Required ciphersuite
		TTLSCipherSuite	 CipherSuite;
		CipherSuite.iHiByte = 0;
		CipherSuite.iLoByte = 3;
		PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
		PtrTlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;	


		//Resume a session
		PtrTlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer1 );
		PtrTlsCryptoAttributes->iSessionNameAndID.iServerName.iPort = 10;
		PtrTlsCryptoAttributes->iSessionNameAndID.iSessionId.Copy(SessionId);
		iStatus = KRequestPending;	
		PtrProvider->CreateL(PtrSession,iStatus);		
		SetActive();
		CActiveScheduler::Start();

		iStatus = KRequestPending;
		CX509Certificate* ReceviedCert;
		ReceviedCert= 0;
		PtrSession->ServerCertificate( ReceviedCert, iStatus );		
		SetActive();
		CActiveScheduler::Start();


		//todo compare retrieved cert with original one:
		if(ReceviedCert && ReceviedCert->IsEqualL(*serverCert)) 
			{		
			return EPass;
			}
		else
			return EFail;	

		}
	return EFail;
	
	} 

	

	
	
