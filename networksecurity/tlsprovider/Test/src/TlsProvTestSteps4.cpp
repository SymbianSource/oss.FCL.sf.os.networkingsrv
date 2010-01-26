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



#define KTLSCachingTimeout  200 

TVerdict CTlsProvTestActive::doTest4_0L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	4.0:  problems with test data files") );
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
			
	iStatus = KRequestPending;
	CX509Certificate* cert;
	sessionObj->ServerCertificate( cert, iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	
	
	if ( KErrNone != iStatus.Int() )
		{
		iLogInfo.Copy( _L("	4.0:  CTLSSession::ServerCertificte - returned with error") );
		return EFail;
		}
	
	//compare retrieved cert with original one:
	if ( cert->IsEqualL( *serverCert ) == EFalse  ) 
		{
		iLogInfo.Copy( _L("	4.0:  retrieved certificate doesn't match") );
		return EFail;
		}
	
	iLogInfo.Copy( _L("	4.0:  OK") );
	
	delete tlsProvider;	
	delete sessionObj;
	
	return EPass;
	
	} 	



TVerdict CTlsProvTestActive::doTest4_1L( CTlsProvStep* aStep )
	{
	
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
		
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.1:  problems with test data files") );
		return EFail;
		}
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	tlsCryptoAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes->iProposedProtocol.iMinor = 0; 
		
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
	tlsCryptoAttributes->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.1:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.1:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
			
	TTLSSessionId sessionId;
	
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
		
	SetActive();
	CActiveScheduler::Start();
	
	
	
	if ( KErrNone != iStatus.Int() )
		{
		iLogInfo.Copy( _L("	4.1:  CTLSProvider::GetSession - returned with error") );
		return EFail;
		}
	
	if (  0 != sessionId.Compare( 
								tlsCryptoAttributes->iSessionNameAndID.iSessionId ) ) 
		{
		iLogInfo.Copy( _L("	4.1:  CTLSProvider::GetSession - BAD session id") );
		return EFail;
		}
		
		
	iLogInfo.Copy( _L("	4.1:  OK") );
	
			
	return EPass;
	
	} 

	

TVerdict CTlsProvTestActive::doTest4_2L( CTlsProvStep* aStep )
	{
	
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider2 = CTLSProvider::ConnectL();
			
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	CTLSSession* sessionObj2;
			
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
	
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.2:  problems with test data files (1)") );
		return EFail;
		}
			
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
			
		
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.2:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.2:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
			
		
	//
	// entry 2
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLServerRnd"),aStep->iServerRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLClientRnd"),aStep->iClientRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLServerCert"),aStep->iServerCertChain);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLHandshakeMsgsServer"),aStep->iHandshakeMsgsServer);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLSrvFinishedCheckOutput"),aStep->iSrvFinishedCheckOutput);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLPremaster"),aStep->iPremaster);
	
	RFs fs;
	if ( KErrNone != fs.Connect() )
		{
		iLogInfo.Copy( _L("	4.2:  problems with file system (2)") );
		return EFail;
		}
		
	CFileMan* fMan = CFileMan::NewL( fs );
	
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> fileName (sysDrive.Name());
	fileName.Append(_L("\\Premaster.bin"));
	
	fMan->Delete(fileName);
	fMan->Copy( aStep->iPremaster,	fileName, CFileMan::EOverWrite );
	fs.Close();	
			
	
	RArray<TTLSCipherSuite> userCipherSuiteList2;
	iStatus = KRequestPending;
	tlsProvider2->CipherSuitesL( userCipherSuiteList2 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	CTlsCryptoAttributes* tlsCryptoAttributes2 = tlsProvider2->Attributes();
		
	tlsCryptoAttributes2->iSessionNameAndID.iServerName.iAddress.Copy( KServer1 );
	tlsCryptoAttributes2->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes2->iSessionNameAndID.iSessionId.Copy( KSessionId2 );
	 
	
	tlsCryptoAttributes2->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes2->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes2->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes2->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes2->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes2->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes2->iProposedProtocol.iMinor = 0; 
	
		
	tlsCryptoAttributes2->iPublicKeyParams->iKeyType = ERsa;
	
	tlsCryptoAttributes2->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes2->iDialogNonAttendedMode = ETrue;
	
		
	delete encServerCert;
	encServerCert = NULL;
	
	res = aStep->ReadDataForCreateL(tlsCryptoAttributes2, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.2:  problems with test data files (2)") );
		return EFail;
		}
	
		
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider2->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	
	iStatus = KRequestPending;
	tlsProvider2->CreateL( sessionObj2,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj2) ) 
		{
		iLogInfo.Copy( _L("	4.2:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj2->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.2:  CTLSSession::ClientKeyExchange failed (4)") );
		return EInconclusive;
		}
		
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	if( EFalse == CacheSessionL( aStep, sessionObj2) )
		return EInconclusive;
	
	
	// entry 2 inserted
	//
	
	//
			
		
	TTLSSessionId sessionId;
	
	CTLSProvider* tlsProvider3 = CTLSProvider::ConnectL();
	
	RArray<TTLSCipherSuite> userCipherSuiteList3;
	iStatus = KRequestPending;
	tlsProvider3->CipherSuitesL( userCipherSuiteList3 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	CTlsCryptoAttributes* tlsCryptoAttributes3 = tlsProvider3->Attributes();
		
	tlsCryptoAttributes3->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes3->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes3->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes3->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes3->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes3->iProposedProtocol.iMinor = 0; 
	TTLSSessionNameAndID sessionNameAndID;
	sessionNameAndID.iServerName.iAddress.Copy( KServer1 );
	sessionNameAndID.iServerName.iPort = 10;
			
		
	iStatus = KRequestPending;
	tlsProvider3->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
		
	if ( KErrNone != iStatus.Int() )
		{
		iLogInfo.Copy( _L("	4.2:  CTLSProvider::GetSession - returned with error") );
		return EFail;
		}
	
	
	if (  0 != sessionId.Compare( tlsCryptoAttributes2->iSessionNameAndID.iSessionId ) ) 
		{
		iLogInfo.Copy( _L("	4.2:  CTLSProvider::GetSession - BAD session id") );
		return EFail;
		}
		
		
	return EPass;
	
	} 	

TVerdict CTlsProvTestActive::doTest4_3L( CTlsProvStep* aStep )
	{
	
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerRnd"),aStep->iServerRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientRnd"),aStep->iClientRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerCert"),aStep->iServerCertChain);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("HandshakeMsgsServer"),aStep->iHandshakeMsgsServer);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SrvFinishedCheckOutput"),aStep->iSrvFinishedCheckOutput);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("Premaster"),aStep->iPremaster);
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
		
	TTLSSessionNameAndID sessionNameAndID;
	sessionNameAndID.iServerName.iAddress.Copy( KServer3 );
	sessionNameAndID.iServerName.iPort = 20;
			
	TTLSSessionId sessionId;
	
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( 0 != sessionId.Length()  )
		{
		iLogInfo.Copy( _L("	4.3:  CTLSProvider::GetSession - wrong error code returned") );
		return EFail;
		}
		
	// now lets insert some entries and try again
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iPort = 10;
		 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	tlsCryptoAttributes->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 5; 
	
		
	tlsCryptoAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes->iProposedProtocol.iMinor = 1; 
	
	
	tlsCryptoAttributes->iPublicKeyParams = NULL;
	
			
	HBufC8* encServerCert = NULL;
	
		
	TInt res = aStep->ReadDataForCreateL(tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.3:  problems with test data files") );
		return EFail;
		}
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
			
	tlsCryptoAttributes->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer1 );
	tlsCryptoAttributes->iSessionNameAndID.iSessionId.Append( KSessionId1 );
	
	CTLSSession* sessionObj = NULL;
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	delete serverCert;
	serverCert = NULL;
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.3:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.3:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
		
	// now lets try to retrieve invalid session
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 != sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.3:  CTLSProvider::GetSession - wrong error code returned") );
		return EFail;
		}
	
	iLogInfo.Copy( _L("	4.3:  OK") );
	
	return EPass;
	
	} 	
	

TVerdict CTlsProvTestActive::doTest4_4L( CTlsProvStep* aStep )
	{
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
				
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider2 = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider3 = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider4 = CTLSProvider::ConnectL();
		
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	CTLSSession* sessionObj2;
	CTLSSession* sessionObj3;
	CTLSSession* sessionObj4;
		
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
	
	HBufC8* encServerCert = NULL;
	
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.4:  problems with test data files (1)") );
		return EFail;
		}
			
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
		
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.4:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
			
	//
	// entry 2 - diferent sever but same session id
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	RArray<TTLSCipherSuite> userCipherSuiteList2;
	iStatus = KRequestPending;
	tlsProvider2->CipherSuitesL( userCipherSuiteList2 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	CTlsCryptoAttributes* tlsCryptoAttributes2 = tlsProvider2->Attributes();
		
	delete encServerCert;
	encServerCert = NULL;
	res = StandardTestInitL(aStep, tlsCryptoAttributes2, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.4:  problems with test data files (2)") );
		return EFail;
		}
	tlsCryptoAttributes2->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider2->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	
	iStatus = KRequestPending;
	tlsProvider2->CreateL( sessionObj2,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj2) )
		{
		iLogInfo.Copy( _L("	4.4:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj2->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
			
	if( EFalse == CacheSessionL( aStep, sessionObj2) )
		return EInconclusive;
	
	// entry 2 
	//
	
	//
	// entry 3
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	RArray<TTLSCipherSuite> userCipherSuiteList3;
	iStatus = KRequestPending;
	tlsProvider3->CipherSuitesL( userCipherSuiteList3 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
		
	CTlsCryptoAttributes* tlsCryptoAttributes3 = tlsProvider3->Attributes();
	
	delete encServerCert;
	encServerCert = NULL;
		
	res = StandardTestInitL(aStep, tlsCryptoAttributes3, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.4:  problems with test data files (3)") );
		return EFail;
		}
	tlsCryptoAttributes3->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	tlsCryptoAttributes3->iSessionNameAndID.iSessionId.Copy( KSessionId3 );	
	
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider3->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	
	iStatus = KRequestPending;
	tlsProvider3->CreateL( sessionObj3,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj3) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj3->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj3) )
		return EInconclusive;
	
	// entry 3 inserted
	//
	
	//
	// entry 4
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLServerRnd"),aStep->iServerRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLClientRnd"),aStep->iClientRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLKeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLServerCert"),aStep->iServerCertChain);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLHandshakeMsgsServer"),aStep->iHandshakeMsgsServer);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLSrvFinishedCheckOutput"),aStep->iSrvFinishedCheckOutput);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLPremaster"),aStep->iPremaster);
	
	RFs fs;
	if ( KErrNone != fs.Connect() )
		{
		iLogInfo.Copy( _L("	4.4:  problems with file system (4)") );
		return EFail;
		}
		
	CFileMan* fMan = CFileMan::NewL( fs );
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> fileName (sysDrive.Name());
	fileName.Append(_L("\\Premaster.bin"));
	
	fMan->Delete(fileName);
	fMan->Copy( aStep->iPremaster,	fileName, CFileMan::EOverWrite );
	fs.Close();	
			
	
	RArray<TTLSCipherSuite> userCipherSuiteList4;
	iStatus = KRequestPending;
	tlsProvider4->CipherSuitesL( userCipherSuiteList4 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	CTlsCryptoAttributes* tlsCryptoAttributes4 = tlsProvider4->Attributes();
		
	tlsCryptoAttributes4->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	tlsCryptoAttributes4->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes4->iSessionNameAndID.iSessionId.Append( KSessionId4 );
	 
	
	tlsCryptoAttributes4->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes4->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes4->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes4->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes4->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes4->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes4->iProposedProtocol.iMinor = 0; 
	
		
	tlsCryptoAttributes4->iPublicKeyParams->iKeyType = ERsa;
	
	tlsCryptoAttributes4->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes4->iDialogNonAttendedMode = ETrue;
	
	
	delete encServerCert;
	encServerCert = NULL;
	
	res = aStep->ReadDataForCreateL(tlsCryptoAttributes4, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.4:  problems with test data files (4)") );
		return EFail;
		}
	
		
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider4->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	
	iStatus = KRequestPending;
	tlsProvider4->CreateL( sessionObj4,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj4) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj4->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.4:  CTLSSession::ClientKeyExchange failed (4)") );
		return EInconclusive;
		}
		
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	if( EFalse == CacheSessionL( aStep, sessionObj4) )
		return EInconclusive;
	
	
	// entry 4 inserted
	//
	
		
	TTLSSessionId sessionId;		
	//case A:
	
	
	iStatus = KRequestPending;
	tlsProvider4->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId, 
		iStatus );
		
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 != sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.4:  A: CTLSProvider::GetSession - found session: BAD") );
		return EFail;
		}
		
	//case B:
		
	CTLSProvider* tlsProvider5 = CTLSProvider::ConnectL();
	
	
	CTlsCryptoAttributes* tlsCryptoAttributes5 = tlsProvider5->Attributes();
		
	
	tlsCryptoAttributes5->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes5->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes5->iProposedProtocol.iMajor = 2;
	tlsCryptoAttributes5->iProposedProtocol.iMinor = 0; 
	
	TTLSSessionNameAndID sessionNameAndID;
	sessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	sessionNameAndID.iServerName.iPort = 10;
			
	iStatus = KRequestPending;
	tlsProvider5->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( 0 != sessionId.Length()  )
		{
		iLogInfo.Copy( _L("	4.4: B: CTLSProvider::GetSession - found session: BAD") );
		return EFail;
		}
		
	
	//case C (positive):
		
	tlsCryptoAttributes5->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes5->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes5->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes5->iNegotiatedProtocol.iMinor = 0; 
	
	tlsCryptoAttributes5->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes5->iProposedProtocol.iMinor = 0; 
	
	iStatus = KRequestPending;
	tlsProvider5->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 == sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.4:  C: CTLSProvider::GetSession - failed") );
		return EFail;
		}
	
	tlsCryptoAttributes5->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes5->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes5->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes5->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes5->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes5->iProposedProtocol.iMinor = 1; 
	
	iStatus = KRequestPending;
	tlsProvider5->GetSessionL(
		sessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 == sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.4:  D: CTLSProvider::GetSession - failed") );
		return EFail;
		}
	
	
	iLogInfo.Copy( _L("	4.4:  OK") );	
	
	delete tlsProvider;	
	delete tlsProvider2;
	delete tlsProvider3;
	delete tlsProvider4;
	delete tlsProvider5;
	delete sessionObj;
	delete sessionObj2;	
	delete sessionObj3;	
	delete sessionObj4;	
		
	return EPass;
	
	} 	
	


TVerdict CTlsProvTestActive::doTest4_5L( CTlsProvStep* aStep )
	{
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd );
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj;
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
	HBufC8* encServerCert = NULL;	
		
	TInt res =  StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.5:  problems with test data files") );
		return EFail;
		}
	tlsCryptoAttributes->iSessionNameAndID.iSessionId.Copy( KSessionId2 );
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	4.5:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.5:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
	
		
	TTLSSessionNameAndID sessionNameAndId;
	
	sessionNameAndId.iServerName.iAddress.Copy( KServer1 );
	sessionNameAndId.iServerName.iPort = 10;
	sessionNameAndId.iSessionId.Append( KSessionId2 );
	 
	
	iStatus = KRequestPending;
	tlsProvider->ClearSessionCacheL( 
			sessionNameAndId, 
			iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	if ( (KErrNone != iStatus.Int()) )
		{
		iLogInfo.Copy( _L("	4.5:  CTLSProvider::ClearSessionCache - failed") );
		return EFail;
		}
	
	
	TTLSSessionId sessionId;
	
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		sessionNameAndId.iServerName,
		sessionId,
		iStatus );
		
		
	SetActive();
	CActiveScheduler::Start();
		
	if ( ( 0 != sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.5:  session cleared but STILL in cache") );
		return EFail;
		}
	
	
	iLogInfo.Copy( _L("	4.5:  OK") );
	
	
	return EPass;
	
	} 	


TVerdict CTlsProvTestActive::doTest4_6L( CTlsProvStep* aStep )
	{
	CTlsTestRandom rnd;
	
	SetThreadRandomL( &rnd ); 
	
	RArray<TTLSCipherSuite> userCipherSuiteList;
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider2 = CTLSProvider::ConnectL();
	CTLSProvider* tlsProvider3 = CTLSProvider::ConnectL();
	
	CTLSSession* sessionObj;
	CTLSSession* sessionObj2;
	CTLSSession* sessionObj3;
	
		
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
				
	
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();
		
	HBufC8* encServerCert = NULL;
		
	TInt res =  StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.6:  problems with test data files") );
		return EFail;
		}
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
	HBufC8* clientKeyExch = NULL;
	
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) ) 
		{
		iLogInfo.Copy( _L("	4.6:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.6:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
	
	
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
		
		
				
	//
	// entry 2 
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
		
	
	CTlsCryptoAttributes* tlsCryptoAttributes2 = tlsProvider2->Attributes();
	
	delete encServerCert;
	encServerCert = NULL;	
		
	res = StandardTestInitL(aStep, tlsCryptoAttributes2, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.6:  problems with test data files") );
		return EFail;
		}
	tlsCryptoAttributes2->iSessionNameAndID.iSessionId.Copy( KSessionId2 );
	
	RArray<TTLSCipherSuite> userCipherSuiteList2;
	iStatus = KRequestPending;
	tlsProvider2->CipherSuitesL( userCipherSuiteList2 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider2->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
		
	iStatus = KRequestPending;
	tlsProvider2->CreateL( sessionObj2,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj2) ) 
		{
		iLogInfo.Copy( _L("	4.6:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
	iStatus = KRequestPending;
	sessionObj2->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	4.6:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	
	if( EFalse == CacheSessionL( aStep, sessionObj2) )
		return EInconclusive;
	
	// entry 2 
	//
	
	
	//
	// entry 3
	
	delete clientKeyExch;	
	clientKeyExch = NULL;
	
	RArray<TTLSCipherSuite> userCipherSuiteList3;
		
	iStatus = KRequestPending;
	tlsProvider3->CipherSuitesL( userCipherSuiteList3 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
		
	
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerRnd2"),aStep->iServerRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientRnd2"),aStep->iClientRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams1_2"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams2_2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams3_2"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerCert2"),aStep->iServerCertChain);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientKeyExch2"),aStep->iClientKeyExch);
		
	CTlsCryptoAttributes* tlsCryptoAttributes3 = tlsProvider3->Attributes();
	
	tlsCryptoAttributes3->iClientAuthenticate = EFalse;
	tlsCryptoAttributes3->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes3->iSessionNameAndID.iServerName.iAddress.Copy( KServer1 );
	tlsCryptoAttributes3->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes3->iSessionNameAndID.iSessionId.Append( KSessionId3 );
	 
	
	tlsCryptoAttributes3->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes3->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes3->iCurrentCipherSuite.iLoByte = 0x16;
	
		
	tlsCryptoAttributes3->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes3->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes3->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes3->iProposedProtocol.iMinor = 1; 
		
		
	tlsCryptoAttributes3->iPublicKeyParams->iKeyType = EDHE;
		
	
	tlsCryptoAttributes3->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes3->iDialogNonAttendedMode = ETrue;
	
	delete encServerCert;
	encServerCert = NULL;	
	
	res = aStep->ReadDataForCreateL(tlsCryptoAttributes3, encServerCert);
			
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.6:  problems with test data files") );
		return EFail;
		}
	
	delete serverCert;
	serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider3->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	iStatus = KRequestPending;
	tlsProvider3->CreateL( sessionObj3,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	HBufC8* clientKeyExchFromFile = NULL;
	res = aStep->ReadClientKeyExchL( clientKeyExchFromFile );
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	4.6:  problems with test data files (client key exchange)") );
		return EFail;
		}
			
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj3) )
		{
		iLogInfo.Copy( _L("	4.6:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj3->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
		
		
	// check that ClientKeyExch is not NULL
	if (  0 != clientKeyExchFromFile->Compare( clientKeyExch->Des() )  )
		{
		iLogInfo.Copy( _L("	4.6:  Client key exchange BAD (3, DHE)") );
		return EInconclusive;
		}
	
	// session should have been cached up to now but not set to resumable yet -
	// and it's left unresumable to check whether will be cleared after cache 
	// entries lifetime timeout
	
	//  entry 3
	//
				
	// before timeout:
	
	TTLSSessionId sessionId;
	
	iStatus = KRequestPending;
	tlsProvider2->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId,
		iStatus );
		
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 == sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.6:  Before timeout: CTLSProvider::GetSession - returned nothing") );
		return EFail;
		}
		
		
	// wait for timeout:
	RTimer timer;
	timer.CreateLocal();
	TTimeIntervalMicroSeconds32 waitTime( 1000000*(KTLSCachingTimeout + 5));
	timer.After( iStatus, waitTime);
	SetActive();
	CActiveScheduler::Start();	
	
	// after timeout
	sessionId.Zero();	
	
	iStatus = KRequestPending;
	tlsProvider2->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId, 
		iStatus );
		
		
	SetActive();
	CActiveScheduler::Start();
	
	if ( ( 0 != sessionId.Length() ) )
		{
		iLogInfo.Copy( _L("	4.6:  After timeout: CTLSProvider::GetSession - cache NOT empty") );
		return EFail;
		}
	
		
	iLogInfo.Copy( _L("	4.6:  OK") );	
	
	delete tlsProvider;	
	delete tlsProvider2;
	delete tlsProvider3;
	
	delete sessionObj;
	delete sessionObj2;		
	delete sessionObj3;		
	
	return EPass;
			
	} 
	

