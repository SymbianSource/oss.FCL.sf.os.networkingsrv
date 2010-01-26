

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



TVerdict CTlsProvTestActive::doTest7_0L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	7.0:  problems with test data files") );
		return EFail;
		}
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	
		
		
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
		iLogInfo.Copy( _L("	7.0:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	7.0:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	HBufC8* verifySrvFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	res = aStep->ReadDataForSrvFinishedL(verifySrvFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	problems with test data files (for server finished check)") );
		return EInconclusive;
		}
	
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig );
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig );
						
	md5Dig->Reset();
	md5Dig->Update( verifySrvFinInput->Des() );
			
	shaDig->Reset();
	shaDig->Update( verifySrvFinInput->Des() );
			
	iStatus = KRequestPending;
	sessionObj->VerifyServerFinishedMsgL( 
		md5Dig,
		shaDig,
		finshedMsg->Des(), 
		iStatus) ;
	
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy( 2, md5Dig );		
				
	if ( iStatus.Int() != KErrNone )
		{
		iLogInfo.Copy( _L("	7.0 CTLSSession::VerifyServerFinishedMsgL failed") );
		return EFail;
		}
			
			
	iLogInfo.Copy( _L("	7.0:  OK") );
	
			
	return EPass;
	}


TVerdict CTlsProvTestActive::doTest7_1L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	7.1:  problems with test data files") );
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
		iLogInfo.Copy( _L("	7.1:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	7.1:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	HBufC8* verifySrvFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	res = aStep->ReadDataForSrvFinishedL(verifySrvFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	problems with test data files (for server finished check)") );
		return EInconclusive;
		}
	
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig );
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig );
						
	md5Dig->Reset();
	md5Dig->Update( verifySrvFinInput->Des() );
			
	shaDig->Reset();
	shaDig->Update( verifySrvFinInput->Des() );
			
	iStatus = KRequestPending;
	sessionObj->VerifyServerFinishedMsgL( 
		md5Dig,
		shaDig,
		finshedMsg->Des(), 
		iStatus) ;
	
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy( 2, md5Dig );		
				
	if ( iStatus.Int() != KErrNone )
		{
		iLogInfo.Copy( _L("	7.1 CTLSSession::VerifyServerFinishedMsgL failed") );
		return EFail;
		}
			
			
	iLogInfo.Copy( _L("	7.1:  OK") );
	
			
	return EPass;

	}



TVerdict CTlsProvTestActive::doTest7_2L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	7.2:  problems with test data files") );
		return EFail;
		}
		
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("HandshakeMsgsClient"),aStep->iHandshakeMsgsClient);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClntFinishedCheckOutput"),aStep->iClntFinishedCheckOutput);
	
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer2 );
	
		
		
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
		iLogInfo.Copy( _L("	7.2:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	7.2:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	HBufC8* clntFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	res = aStep->ReadDataForClntFinishedL(clntFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	problems with test data files (for client finished check)") );
		return EInconclusive;
		}
	
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig );
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig );
						
	md5Dig->Reset();
	md5Dig->Update( clntFinInput->Des() );
			
	shaDig->Reset();
	shaDig->Update( clntFinInput->Des() );
			
	HBufC8* output = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientFinishedMsgL(
		md5Dig,
		shaDig,
		output, 
		iStatus) ;
			
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy( 2, md5Dig );		
				
	if ( iStatus.Int() != KErrNone || (0 != output->Compare(finshedMsg->Des()) ) )
		{
		iLogInfo.Copy( _L("	7.2 CTLSSession::ClientFinishedMsgL failed") );
		return EFail;
		}
			
			
	iLogInfo.Copy( _L("	7.2:  OK") );
				
	return EPass;
			
	}



TVerdict CTlsProvTestActive::doTest7_3L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	7.3:  problems with test data files") );
		return EFail;
		}
		
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("HandshakeMsgsClient"),aStep->iHandshakeMsgsClient);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClntFinishedCheckOutput"),aStep->iClntFinishedCheckOutput);
	
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
		iLogInfo.Copy( _L("	7.3:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	7.3:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	HBufC8* clntFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	res = aStep->ReadDataForClntFinishedL(clntFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	problems with test data files (for client finished check)") );
		return EInconclusive;
		}
	
	CMessageDigest* md5Dig = NULL;
	CMessageDigest* shaDig = NULL;
	
	md5Dig = CMD5::NewL(); 
	CleanupStack::PushL( md5Dig );
	shaDig = CSHA1::NewL(); 
	CleanupStack::PushL( shaDig );
						
	md5Dig->Reset();
	md5Dig->Update( clntFinInput->Des() );
			
	shaDig->Reset();
	shaDig->Update( clntFinInput->Des() );
			
	HBufC8* output = NULL;
	iStatus = KRequestPending;
	sessionObj->ClientFinishedMsgL(
		md5Dig,
		shaDig,
		output, 
		iStatus) ;
			
	SetActive();
	CActiveScheduler::Start();
	
	CleanupStack::PopAndDestroy( 2, md5Dig );		
				
	if ( iStatus.Int() != KErrNone || (0 != output->Compare(finshedMsg->Des()) ) )
		{
		iLogInfo.Copy( _L("	7.3 CTLSSession::ClientFinishedMsgL failed") );
		return EFail;
		}
			
			
	iLogInfo.Copy( _L("	7.3:  OK") );
				
	return EPass;
	
	}


