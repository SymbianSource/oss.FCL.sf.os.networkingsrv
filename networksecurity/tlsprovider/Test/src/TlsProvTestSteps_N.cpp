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
#include <e32property.h>

_LIT(KYes,"Yes");
_LIT(KInputFile, "\\t_secdlg_in.dat");
_LIT(KOutputFile, "\\t_secdlg_out.dat");

_LIT8(EAPString,"client EAP encryption");

#define KPRF1Length 128
#define KPRF2Length 64
#define KMaxBufData 0x12C // 300 length

extern TInt ReadTestDataL( CTlsCryptoAttributes*& aTlsCryptoAttributes, HBufC8*& aSrvCert,
				   CTlsProvStep*& aTestStepPtr);

extern TInt ReadDataFromFiles( CTlsCryptoAttributes* aTlsCryptoAttributes, 
						HBufC8*& aSrvCert);

static TBool AreCipherSuitesSame(const TTLSCipherSuite& aLeft, const TTLSCipherSuite& aRight)
/**
	Identity relation compares the two suplied cipher suites.

	@param	aLeft			Left cipher suite to compare.
	@param	aRight			Right cipher suite to compare.
	@return					ETrue if the ciphers have the same
							identity, i.e. the same low and high
							bytes; EFalse otherwise.
 */
	{
	return (aLeft.iLoByte == aRight.iLoByte) && (aLeft.iHiByte == aRight.iHiByte);
	}

static TBool SuitesMatchExpected(
	const RArray<TTLSCipherSuite>& aSuites, TBool aOneMissing)
/**
	Predicate function determines if the supplied array of
	cipher suites, which should have been retrieved with
	CTlsProvider::CipherSuitesL(), match the expected suites.
	
	@param	aSuites			Set of cipher suites retrieved
							from the TLS provider.
	@param	aOneMissing		Whether or not exactly one item
							is missing.  This is used by
							CTlsProvTestActive::doTest1_0L
							and CTlsProvTestActive::doTest1_1L
							to test failure cases.
	@return					ETrue if the supplied suites match
							the expected set (including being in
							the right order;) EFalse otherwise.
	@see CTlsProvTestActive::doTest1_0L
	@see CTlsProvTestActive::doTest1_1L
 */
	{
	// This order is taken from the TLS Protocol Design Document.
	// See INC075426.
	
	const TInt KExpectedCount = 14;
	const TTLSCipherSuite expectedCiphers[KExpectedCount] =
		{
		{0x00, 0x35},	// TLS_RSA_WITH_AES_256_CBC_SHA
		{0x00, 0x2F},	// TLS_RSA_WITH_AES_128_CBC_SHA
		{0x00, 0x0A},	// SSL_RSA_WITH_3DES_EDE_CBC_SHA
		{0x00, 0x16},	// SSL_DHE_RSA_WITH_3DES_EDE_CBC_SHA
		{0x00, 0x13},	// SSL_DHE_DSS_WITH_3DES_EDE_CBC_SHA
		{0x00, 0x05},	// SSL_RSA_WITH_RC4_128_SHA
		{0x00, 0x04},	// SSL_RSA_WITH_RC4_128_MD5
		{0x00, 0x09},	// SSL_RSA_WITH_DES_CBC_SHA
		{0x00, 0x12},	// SSL_DHE_DSS_WITH_DES_CBC_SHA
		{0x00, 0x08},	// SSL_RSA_EXPORT_WITH_DES40_CBC_SHA
		{0x00, 0x03},	// SSL_RSA_EXPORT_WITH_RC4_MD5
		{0x00, 0x11},	// SSL_DHE_DSS_EXPORT_WITH_DES40_CBC_SHA
		{0x00, 0x14},	// SSL_DHE_RSA_EXPORT_WITH_DES40_CBC_SHA
		{0x00, 0x19}	// TLS_DH_anon_EXPORT_WITH_DES40_CBC_SHA
		};

	if (aSuites.Count() != KExpectedCount - (aOneMissing ? 1 : 0))
		return EFail;

	TInt i;			// VC6 doesn't support for scope
	if (! aOneMissing)
		{
		// ensure expected ciphers returned, and in expected order.
		for (i = KExpectedCount - 1; i >= 0; --i)
			{
			if (!(aSuites[i] == expectedCiphers[i]))
				return EFalse;
			}		
		}
	else
		{
		// work out which cipher suite is missing from the
		// returned list.  (This is quadratic.)
		TInt missingIdx = KMinTInt;	// avoid used when uninitialized warning
		TInt missingCount = 0;
		for (i = KExpectedCount - 1; i >= 0; --i)
			{
			TIdentityRelation<TTLSCipherSuite> idRel(AreCipherSuitesSame);
			if (aSuites.Find(expectedCiphers[i], idRel) == KErrNotFound)
				{
				missingIdx = i;
				++missingCount;
				}
			}
		if (missingCount != 1)
			return EFalse;
		
		// check the returned suites match the expected order
		for (i = 0; i < missingIdx; ++i)
			{
			if (!(aSuites[i] == expectedCiphers[i]))
				return EFalse;
			}
		for (i = missingIdx; i < KExpectedCount - 1; ++i)
			{
			if (!(aSuites[i] == expectedCiphers[i + 1]))
				return EFalse;
			}
		}
	
	return ETrue;
	}

TVerdict CTlsProvTestActive::doTest1_0L( CTlsProvStep* )
	{
	RArray<TTLSCipherSuite> UserCipherSuiteList;
	
	__UHEAP_MARK;
	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();

	
	iStatus = KRequestPending;
	PtrProvider->CipherSuitesL(UserCipherSuiteList,iStatus);	
	SetActive();
	CActiveScheduler::Start();

	if (! SuitesMatchExpected(UserCipherSuiteList, /* aOneMissing */ EFalse))
		return EFail;
	
	//Now tell the Token code to simulate a failure..use Publish and Subscribe here
    RProperty property;
	const TUid KTestFailUID={0x10201967};   //Uid value is wrong..     
	const TInt KTestFailKey = 1;
	TInt ret = property.Define(KTestFailUID, KTestFailKey,RProperty::EInt,0);
    ret= property.Attach(KTestFailUID, KTestFailKey);
	ret = property.Set(KTestFailUID,KTestFailKey,1);
	if(ret != KErrNone)
	  return EFail;

	PtrProvider->ReConnectL();
	PtrProvider->ReConnectL();

	UserCipherSuiteList.Reset();
	iStatus = KRequestPending;
	PtrProvider->CipherSuitesL(UserCipherSuiteList,iStatus);	
	SetActive();
	CActiveScheduler::Start();

	ret = property.Set(KTestFailUID,KTestFailKey,0);
	property.Close();
	
	// one cipher suite should be missing
	if (! SuitesMatchExpected(UserCipherSuiteList, /* aOneMissing */ ETrue))
		return EFail;	
	
	UserCipherSuiteList.Reset();
	PtrProvider->ReConnectL();
	delete PtrProvider;

	PtrProvider = CTLSProvider::ConnectL();
	PtrProvider->ReConnectL();
	delete PtrProvider;

	__UHEAP_MARKEND;
	
	return EPass;
	}

TVerdict CTlsProvTestActive::doTest1_1L( CTlsProvStep* )
	{
	RArray<TTLSCipherSuite> UserCipherSuiteList;
	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();
	iStatus = KRequestPending;

	//
	//Set the property here so that WIM fails during enumeration

	//
	PtrProvider->CipherSuitesL(UserCipherSuiteList,iStatus);	
	SetActive();
	CActiveScheduler::Start();

	UserCipherSuiteList.Reset();

	PtrProvider->CipherSuitesL(UserCipherSuiteList,iStatus);	
	SetActive();
	CActiveScheduler::Start();

	TVerdict v =
			SuitesMatchExpected(UserCipherSuiteList, /* aOneMissing */ EFalse)
		?	EPass
		:	EFail;
	
	UserCipherSuiteList.Reset();
	return v;
	}


TVerdict CTlsProvTestActive::doTest5_0L( CTlsProvStep* )
	{
	// Deletes the input and output dat file.
	DeleteFileL();

	RFileWriteStream stream;
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> inputFile (sysDrive.Name());
	inputFile.Append(KInputFile);
	
	TInt err = stream.Open(iFs, inputFile, EFileWrite | EFileShareExclusive);
	if (err == KErrNotFound)
		{
		err = stream.Create(iFs, inputFile, EFileWrite | EFileShareExclusive);
		}
	User::LeaveIfError(err);
	stream.PushL();
	
	WriteDialogRecordL(stream, EServerAuthenticationFailure, _L("Passphrase of the imported key file"), iExpectedResult, KNullDesC);
	stream.CommitL();
	CleanupStack::PopAndDestroy(); // stream

	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();
	
	//
	//Send a certificate we know that will pass
	//

	RFile* file;
	file = new (ELeave) RFile;
	
	TBuf8<1000> bufForData;
	
	TInt res = file->Open(iFs, 
				_L("z:\\testdata\\configs\\tlsprovtestdata\\ClientAuthentication\\ServerCertValid.bin" ), 
	 			EFileShareAny|EFileRead);
	if(!res)
		{
		res = file->Read(bufForData);
		}

	if(res != KErrNone)
		{
		return EFail;
		}
	
	file->Close();
	
	CTlsCryptoAttributes* tlsCryptoAttributes = PtrProvider->Attributes();
	tlsCryptoAttributes->iDialogNonAttendedMode = iDialogNonAttendedMode;

	CX509Certificate* aServerCert;
	iStatus = KRequestPending;
	PtrProvider->VerifyServerCertificate(bufForData,aServerCert,iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	TBool result = KErrAbort;
	if( iExpectedResult.CompareF(KYes) == KErrNone )
		{
		result = KErrNone;	
		}
	if(iStatus.Int() == result)
		{
		return EPass;
		}
	else
		{
		return EFail;
		}
	}



TVerdict CTlsProvTestActive::doTest5_1L( CTlsProvStep* )
	{
	// Deletes the input and output dat file.
	DeleteFileL();

	RFileWriteStream stream;
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TBuf<128> inputFile (sysDrive.Name());
	inputFile.Append(KInputFile);
	
	TInt err = stream.Open(iFs, inputFile, EFileWrite | EFileShareExclusive);
	if (err == KErrNotFound)
		{
		err = stream.Create(iFs, inputFile, EFileWrite | EFileShareExclusive);
		}
	User::LeaveIfError(err);
	stream.PushL();
	
	WriteDialogRecordL(stream, EServerAuthenticationFailure, _L("Passphrase of the imported key file"), iExpectedResult, KNullDesC);
	stream.CommitL();
	CleanupStack::PopAndDestroy(); // stream

	CTLSProvider* PtrProvider = CTLSProvider::ConnectL();

	//
	//Send a certificate we know that will fail
	//
	RFile* file;
	file = new (ELeave) RFile;
	
	(PtrProvider->Attributes())->iDialogNonAttendedMode = iDialogNonAttendedMode;
	
	TBuf8<1000> bufForData;
	
	TInt res = file->Open(iFs, 
				_L("z:\\testdata\\configs\\tlsprovtestdata\\ClientAuthentication\\ServerCertInValid.bin" ),
	 			EFileShareAny|EFileRead);
	if(!res)
		{
		res = file->Read(bufForData);
		}
	if(res != KErrNone)
		{
		return EFail;
		}
	
	file->Close();

	CX509Certificate* aServerCert;
	iStatus = KRequestPending;
	PtrProvider->VerifyServerCertificate(bufForData,aServerCert,iStatus);
	SetActive();
	CActiveScheduler::Start();
	
	TBool result = KErrAbort;
	if( iExpectedResult.CompareF(KYes) == KErrNone )
		{
		result = KErrNone;	
		}
	if(iStatus.Int() == result)
		{
		return EPass;
		}
	else
		{
		return EFail;
		}
	}

void CTlsProvTestActive::WriteDialogRecordL(RFileWriteStream& aStream, TSecurityDialogOperation aOp, const TDesC& aLabelSpec,const TDesC& aResponse1, const TDesC& aResponse2)
	{
	MStreamBuf* streamBuf = aStream.Sink();
	streamBuf->SeekL(MStreamBuf::EWrite, EStreamEnd);

	aStream.WriteInt32L(aOp);
	aStream.WriteInt32L(aLabelSpec.Length());
	aStream.WriteL(aLabelSpec);
	aStream.WriteInt32L(aResponse1.Length());
	aStream.WriteL(aResponse1);
	aStream.WriteInt32L(aResponse2.Length());
	aStream.WriteL(aResponse2);
	}

void CTlsProvTestActive::DeleteFileL()
	{
	CFileMan* fileMan = CFileMan::NewL(iFs);
	CleanupStack::PushL(fileMan);
	
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TDriveName sysDriveName (sysDrive.Name());
	
	TBuf<128> fileName (sysDriveName);
	fileName.Append(KInputFile);
	TInt err = fileMan->Delete(fileName);
	if ( err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
		
	fileName.Copy(sysDriveName);
	fileName.Append(KOutputFile);	
	err = fileMan->Delete(fileName);
	if (err != KErrNotFound && err != KErrNone )
		{
		User::LeaveIfError(err);
		}
	CleanupStack::PopAndDestroy(fileMan);	
	}

TVerdict CTlsProvTestActive::doTest10_0L( CTlsProvStep* )
	{

		CTLSProvider* tlsProvider = CTLSProvider::ConnectL(); 
        RArray<TTLSCipherSuite> userCipherSuiteList; 
        iStatus = KRequestPending; 
        tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);         
        SetActive(); 
        
        INFO_PRINTF1(_L("After SetActive()"));
        tlsProvider->CancelRequest();                 // This line should get stuck waiting forever, until you make the appropriate fixes in the code.
		INFO_PRINTF1(_L("After tlsProvider->CancelRequest();"));
		if(iStatus.Int() != KErrNone && iStatus.Int() != KErrCancel )
			return EFail;
		else
			return EPass;
	
	}

/**
This function call provides the key part of test case TLSProvider-EAPString-001.

@SYMTestCaseID   TLSProvider-EAPString-001.
@SYMTestCaseDesc Generate EAP String and Check the String.
@SYMTestStatus   Implemented
@SYMTestPriority 1
@SYMTestActions  Issue a Request to generate EAP string by calling KeyDerivation API.
@SYMTestExpectedResults Verify the returned string generates 128 bytes and 64 bytes 
						and compare it with the bin file.
@SYMREQ 5464
*/
TVerdict CTlsProvTestActive::doTest10_1L( CTlsProvStep* aStep )
	{		
	CTlsTestRandom rnd;
	SetThreadRandomL( &rnd );
	
	CTLSProvider* tlsProvider = CTLSProvider::ConnectL();
	RArray<TTLSCipherSuite> userCipherSuiteList;
	iStatus = KRequestPending;
	tlsProvider->CipherSuitesL( userCipherSuiteList ,iStatus);	
	SetActive();
	CActiveScheduler::Start();
	userCipherSuiteList.Close();
		
	CTlsCryptoAttributes* tlsCryptoAttributes = tlsProvider->Attributes();			
	HBufC8* encServerCert = NULL;
	TInt res = StandardTestInitL(aStep, tlsCryptoAttributes, encServerCert);
	if( res  < 0 )
		{
		iLogInfo.Copy( _L("	10.1:  problems with test data files") );
		return EFail;
		}	
			
	CX509Certificate* serverCert = 0;
	iStatus = KRequestPending;
	tlsProvider->VerifyServerCertificate( encServerCert->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	serverCert = CX509Certificate::NewL(encServerCert->Des()); //- remove when VerifyServerCertificate ready
	
	CTLSSession* sessionObj = NULL;
	iStatus = KRequestPending;
	tlsProvider->CreateL( sessionObj,
						iStatus);	
	SetActive();
	CActiveScheduler::Start();	
	if ( (iStatus.Int() != KErrNone ) || (NULL == sessionObj) )
		{
		iLogInfo.Copy( _L("	10.1:  CTLSProvider::Create failed") );
		return EInconclusive;
		}
	
	HBufC8* clientKeyExch = NULL;
	iStatus = KRequestPending;
	// ClientKeyExchange to create the master secret key.
	sessionObj->ClientKeyExchange(clientKeyExch, iStatus);
	SetActive();
	CActiveScheduler::Start();
	if ( (NULL == clientKeyExch) || ( 0 == clientKeyExch->Length()) ) 
		{
		iLogInfo.Copy( _L("	10.1:  CTLSSession::ClientKeyExchange failed") );
		return EInconclusive;
		}
	
	TTLSMasterSecretInput masterSecretInput; // Contains the client and server random value.
	HBufC8* keyMaterial = HBufC8::NewL(KPRF1Length+KPRF2Length); // Buffer to hold the 128 + 64 bytes of EAP String.
	TPtr8 keyMaterialPtr = keyMaterial->Des();
	TInt ret = KErrNone;
	// calls KeyDerivation method to generate 128 + 64 bytes EAP String.
	ret = sessionObj->KeyDerivation(EAPString,tlsCryptoAttributes->iMasterSecretInput, keyMaterialPtr);
	if(ret != KErrNone)
		{
		iLogInfo.Copy( _L("	10.1:  KeyDerivation test failed") );				
		return EFail;
		}
	
	TBuf8<KMaxBufData> bufForData;
	res = iFile.Open(iFs,iKeyDerivation128,EFileShareAny|EFileRead);
	if(!res)
		{
		res = iFile.Read(bufForData);
		}
	if(res != KErrNone)
		{
		iFile.Close();	
		return EFail;
		}
	iFile.Close();
	TBuf8<KPRF1Length> eapString128;
	eapString128.Copy(keyMaterialPtr.Left(KPRF1Length));
	// Compare to check whether first 128 bytes EAP string matchs with the test binary data.
	ret = eapString128.Compare(bufForData);
	if(KErrNone == ret)
		{
		iLogInfo.Copy( _L("	10.1: Test data for 128 EAP string matchs") );			
		}
		
	res = iFile.Open(iFs,iKeyDerivation64,EFileShareAny|EFileRead);
	if(!res)
		{
		res = iFile.Read(bufForData);
		}
	if(res != KErrNone)
		{
		iFile.Close();	
		return EFail;
		}
	iFile.Close();
	TBuf8<KPRF2Length> eapString64;	
	eapString64.Copy(keyMaterialPtr.Right(KPRF2Length));
	// Compare to check whether 64 bytes EAP string matchs with the test binary data.
	ret = eapString64.Compare(bufForData);
	if(KErrNone == ret)
		{
		iLogInfo.Copy( _L("	10.1: Test data for 64 EAP string matchs") );			
		}
	
	if( EFalse == CacheSessionL( aStep, sessionObj) )
		return EInconclusive;
	
	CX509Certificate* cert = NULL;
	iStatus = KRequestPending;
	sessionObj->ServerCertificate( cert, iStatus );
	SetActive();
	CActiveScheduler::Start();
	if ( KErrNone != iStatus.Int() )
		{
		iLogInfo.Copy( _L("	10.1:  CTLSSession::ServerCertificte - returned with error") );
		return EFail;
		}
	
	//compare retrieved cert with original one:
	if ( cert->IsEqualL( *serverCert ) == EFalse  ) 
		{
		iLogInfo.Copy( _L("	10.1:  Retrieved certificate doesn't match") );
		return EFail;
		}
	iLogInfo.Copy( _L("	10.1:  OK") );
	
	delete sessionObj; 	
	sessionObj = NULL;
	delete encServerCert; 
	encServerCert = NULL;
	delete serverCert; 	
	serverCert = NULL;
	delete cert; 
	cert = NULL;
	delete keyMaterial;
	keyMaterial = NULL;
	delete clientKeyExch; 
	clientKeyExch = NULL;
	delete sessionObj; 
	sessionObj = NULL;
	delete tlsProvider; 
	tlsProvider = NULL;

	return EPass;	
	}
	
