
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

	
TVerdict CTlsProvTestActive::doTest9_0L( CTlsProvStep* aStep )
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
		iLogInfo.Copy( _L("	9.0:  problems with test data files") );
		return EFail;
		}
		
	CX509Certificate* serverCert = NULL;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	tlsProvider->CancelRequest();
	
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
		
	HBufC8* clientKeyExch = NULL;
	
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	9.0:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
		
		
	iStatus = KRequestPending;
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	sessionObj->CancelRequest();
	
	
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	// check that ClientKeyExch is not NULL
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	9.0:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
		
	HBufC8* verifySrvFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	res = aStep->ReadDataForSrvFinishedL(verifySrvFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	9.0:  problems with test data files (for server finished check)") );
		return EFail;
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
		iLogInfo.Copy( _L("	9.0:  CTLSSession::VerifyServerFinishedMsgL failed") );
		return EInconclusive;
		}
			
	
	RArray<TTLSCipherSuite> userCipherSuiteList2;
	CTLSProvider* tlsProvider2 = CTLSProvider::ConnectL();
	
		
	iStatus = KRequestPending;
	tlsProvider2->CipherSuitesL( userCipherSuiteList2 ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	
			
	CTLSSession* sessionObj2;
	
	CTlsCryptoAttributes* tlsCryptoAttributes2 = tlsProvider2->Attributes();
			
	HBufC8* encServerCert2 = NULL;
			
	res = StandardTestInitL(aStep, tlsCryptoAttributes2, encServerCert2);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	9.0:  problems with test data files (2)") );
		return EFail;
		}		
	
	iStatus = KRequestPending;
	tlsProvider2->CreateL( sessionObj2,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	
	// check that ClientKeyExch is not NULL
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	9.0:  CTLSProvider::Create failed (2)") );
		return EInconclusive;
		}
	iStatus = KRequestPending;
	CX509Certificate* cert;
	sessionObj2->ServerCertificate( cert, iStatus );
	sessionObj2->CancelRequest();
		
	TTLSSessionNameAndID sessionNameAndId;
	
	sessionNameAndId.iServerName.iAddress.Copy( KServer1 );
	sessionNameAndId.iServerName.iPort = 10;
	sessionNameAndId.iSessionId.Append( KSessionId1 );
	
	
	iStatus = KRequestPending;
	tlsProvider2->ClearSessionCacheL( 
			sessionNameAndId, 
			iStatus);
	tlsProvider2->CancelRequest();
	
	
	TTLSSessionId sessionId;
	
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId,
		iStatus );
	tlsProvider->CancelRequest();
	
			
	iStatus = KRequestPending;
	tlsProvider->GetSessionL(
		tlsCryptoAttributes->iSessionNameAndID.iServerName,
		sessionId,
		iStatus );
	SetActive();
	CActiveScheduler::Start();
	
	
	
	if ( KErrNone != iStatus.Int() )
		{
		iLogInfo.Copy( _L("	9.0:  CTLSProvider::GetSession - returned with error") );
		return EFail;
		}
	
			
		
	iLogInfo.Copy( _L("	9.0:  OK") );
	
			
	return EPass;
	
	} 
