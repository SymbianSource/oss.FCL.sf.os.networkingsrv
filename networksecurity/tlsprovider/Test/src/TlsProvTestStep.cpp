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

/**
 @file TlsProvTestStep.cpp
*/
#include "TlsProvTestStep.h"
#include <test/testexecutelog.h>


#include <signed.h>
#include <x509cert.h>
#include <pkixcertchain.h>

_LIT(KDialogNonAttendedMode,"DialogNonAttendedMode");
_LIT(KExpectedResult, "ExpectedResult");

// tls test
#include <f32file.h>

CTlsProvTestActive::CTlsProvTestActive( CTestExecuteLogger& aLogger ) : 
   CActive( EPriorityStandard ),
   iLogger( aLogger )
	{
	CActiveScheduler::Add( this );
	User::LeaveIfError(iFs.Connect());
	}

CTlsProvTestActive::~CTlsProvTestActive()
	{
	Cancel();
	iFs.Close();
	}

	
void CTlsProvTestActive::RunL()
	{
	CActiveScheduler::Stop();
	return;
	
	}

//
/*************************************Test Step 1**********************************/
//


CTlsProvStep1::~CTlsProvStep1()
/**
 * Destructor
 */
	{
	}

CTlsProvStep1::CTlsProvStep1()
/**
 * Constructor
 */
	{
	// Call base class method to set up the human readable name for logging
	SetTestStepName(KTlsTestStep1);
	}

TVerdict CTlsProvStep1::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	INFO_PRINTF1(_L("Test Step 1 Preamble"));
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CTlsProvStep1::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Demonstrates reading configuration parameters fom an ini file section
 */
	{

	INFO_PRINTF1(_L("Inside Test Step 1"));
	
	SetTestStepResult(EPass);
	User::After(5000000);
	return TestStepResult();
	
	}

TVerdict CTlsProvStep1::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	INFO_PRINTF1(_L("Test Step 1 Postamble"));
	return TestStepResult();
	}





//
/*************************************Test Step 2**********************************/
//


CTlsProvStep2::~CTlsProvStep2()
/**
 * Destructor
 */
	{
	}

CTlsProvStep2::CTlsProvStep2()
/**
 * Constructor
 */
	{
	// Call base class method to set up the human readable name for logging
	SetTestStepName(KTlsTestStep2);
	}

TVerdict CTlsProvStep2::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	INFO_PRINTF1(_L("Test Step 1 Preamble"));
	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CTlsProvStep2::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Demonstrates reading configuration parameters fom an ini file section
 */
	{

	INFO_PRINTF1(_L("Inside Test Step 2"));
	
	SetTestStepResult(EPass);
	User::After(5000000);
	return TestStepResult();
	
	}

TVerdict CTlsProvStep2::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	INFO_PRINTF1(_L("Test Step 1 Postamble"));
	return TestStepResult();
	}


//
/*************************************Test Steps**********************************/
//


CTlsProvStep::~CTlsProvStep()
/**
 * Destructor
 */
	{
	}

CTlsProvStep::CTlsProvStep(const TDesC& aStepName) 
/**
 * Constructor
 */
	{
	// Call base class method to set up the human readable name for logging
	SetTestStepName( aStepName );
	}

TVerdict CTlsProvStep::doTestStepPreambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	if( TestStepName() == KTlsTestStep1_0)
		{
		INFO_PRINTF1(_L("Test Step 1.0 Verify supported ciphers"));
		}
	else if( TestStepName() == KTlsTestStep2_0)
		{
		INFO_PRINTF1(_L("Test Step 2.0 Generation of random bytes"));
		}
	else if( TestStepName() == KTlsTestStep4_0)
		{
		INFO_PRINTF1(_L("Test Step 4.0 Retrieval of cached server certificate"));
		}
	else if( TestStepName() == KTlsTestStep4_1)
		{
		INFO_PRINTF1(_L("Test Step 4.1 Retrieval of valid cached session"));
		}
	else if(TestStepName() == KTlsTestStep4_2)
		{
		INFO_PRINTF1(_L("Test Step 4.2 Retrieval of valid cached session with additional conditions"));
		}
	else if(TestStepName() == KTlsTestStep4_3)
		{
		INFO_PRINTF1(_L("Test Step 4.3 Attempt to retrieve invalid session from cache"));
		}
	else if(TestStepName() == KTlsTestStep4_4)
		{
		INFO_PRINTF1(_L("Test Step 4.4 Attempt to retrieve session from cache with non matching crypto algorithms"));
		}
	else if(TestStepName() == KTlsTestStep4_5)
		{
		INFO_PRINTF1(_L("Test Step 4.5 Cache clearing"));
		}
	else if(TestStepName() == KTlsTestStep4_6)
		{
		INFO_PRINTF1(_L("Test Step 4.6 Periodical cache clearing"));
		}
	else if(TestStepName() == KTlsTestStep5_0)
		{
		INFO_PRINTF1(_L("Test Steps 5.0, Verify server authentication dialog"));
		}
	else if(TestStepName() == KTlsTestStep5_1)
		{
		INFO_PRINTF1(_L("Test Steps 5.1, Verify server authentication dialog"));
		}
	else if(TestStepName() == KTlsTestStep5_2)
		{
		INFO_PRINTF1(_L("Test Steps 5.2, 8.0, 8.2 client cert, computing and veryfying digital signatures: RSA"));
		}
	else if(TestStepName() == KTlsTestStep5_3)
		{
		INFO_PRINTF1(_L("Test Step 5.3 Veryfying signature, RSA, negative test"));
		}
	else if(TestStepName() == KTlsTestStep5_4)
		{
		INFO_PRINTF1(_L("Test Steps 5.4, 8.0, 8.2 client cert, computing and veryfying digital signatures: DSA"));
		}
	else if(TestStepName() == KTlsTestStep5_5)
		{
		INFO_PRINTF1(_L("Test Step 5.5 Veryfying signature, DSA, negative test"));
		}
	else if(TestStepName() == KTlsTestStep7_0)
		{
		INFO_PRINTF1(_L("Test Step 7.0 Verification of Server Finished Check, TLS case"));
		}
	else if(TestStepName() == KTlsTestStep7_1)
		{
		INFO_PRINTF1(_L("Test Step 7.1 Verification of Server Finished Check, SSL case"));
		}
	else if(TestStepName() == KTlsTestStep7_2)
		{
		INFO_PRINTF1(_L("Test Step 7.2 Verification of Client Finished Check, TLS case"));
		}
	else if(TestStepName() == KTlsTestStep7_3)
		{
		INFO_PRINTF1(_L("Test Step 7.3 Verification of Client Finished Check, SSL case"));
		}
	else if(TestStepName() == KTlsTestStep8_1)
		{
		INFO_PRINTF1(_L("Test Step 8.1 Retrieving client cert in non-RSA key exchange case"));
		}
	else if(TestStepName() == KTlsTestStep9_0)
		{
		INFO_PRINTF1(_L("Test Step 9.0 Testing of Cancel functions"));
		}
	else if(TestStepName() == KTlsTestStep10_0)
		{
		INFO_PRINTF1(_L("Test Step 10.0 Testing of Cancel function"));
		}	
	else if(TestStepName() == KTlsTestStep10_1)
		{
		INFO_PRINTF1(_L("Test Step 10.1 Generating EAP String for TLS Protocol"));
		}	
	
	SetTestStepResult(EFail);
	return TestStepResult();	
	}

TVerdict CTlsProvStep::doTestStepL()
/**
 * @return - TVerdict code
 * Override of base class pure virtual
 * Demonstrates reading configuration parameters fom an ini file section
 */
	{
	CActiveScheduler* sched=NULL;
	sched=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(sched);	
	CTlsProvTestActive* activeObj = new CTlsProvTestActive( Logger() );

	if(TestStepName() == KTlsTestStep1_0)
		{
		INFO_PRINTF1(_L("Obtain the list of Available cipher suites"));
	
		verdict = activeObj->doTest1_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep1_1)
		{
		INFO_PRINTF1(_L("Obtain the list of Available cipher suites with a simulated token failure"));
	
		verdict = activeObj->doTest1_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep2_0)
		{
		INFO_PRINTF1(_L("Inside Test Step 2.0"));
	
		verdict = activeObj->doTest2_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep3_0)
		{
		INFO_PRINTF1(_L("Inside Test Step 3.0"));
	
		verdict = activeObj->TestProvider_3_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep3_1)
		{
		INFO_PRINTF1(_L("Inside Test Step 3.1"));
	
		verdict = activeObj->TestProvider_3_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep3_2)
		{
		INFO_PRINTF1(_L("Inside Test Step 3.2"));
	
		verdict = activeObj->TestProvider_3_2L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep3_3)
		{
		INFO_PRINTF1(_L("Inside Test Step 3.3"));
	
		verdict = activeObj->TestProvider_3_3L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep4_0)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.0"));
	
		verdict = activeObj->doTest4_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep4_1)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.1"));
	
		verdict = activeObj->doTest4_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep4_2)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.2"));
	
		verdict = activeObj->doTest4_2L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep4_3)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.3"));
	
		verdict = activeObj->doTest4_3L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep4_4)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.4"));
	
		verdict = activeObj->doTest4_4L( this );
		INFO_PRINTF1( activeObj->iLogInfo );		
		}
	else if(TestStepName() == KTlsTestStep4_5)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.5"));
	
		verdict = activeObj->doTest4_5L( this );
		INFO_PRINTF1( activeObj->iLogInfo );		
		}
	else if(TestStepName() == KTlsTestStep4_6)
		{
		INFO_PRINTF1(_L("Inside Test Step 4.6"));
	
		verdict = activeObj->doTest4_6L( this );
		INFO_PRINTF1( activeObj->iLogInfo );		
		}
	else if ( TestStepName() == KTlsTestStep5_0)
		{
		INFO_PRINTF1(_L("Verify a valid certificate"));
		if (EFalse == GetBoolFromConfig(ConfigSection(),KDialogNonAttendedMode,activeObj->iDialogNonAttendedMode))
			{	
			INFO_PRINTF1(_L("DialogNonAttendedMode tag is required to continue the test"));	
			}	

		if (EFalse == GetStringFromConfig(ConfigSection(),KExpectedResult,activeObj->iExpectedResult))
			{	
			INFO_PRINTF1(_L("ExpectedResult tag is required to continue the test"));	
			}	
		verdict = activeObj->doTest5_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep5_1)
		{
		INFO_PRINTF1(_L("Verify a Invalid certificate"));
	
		if (EFalse == GetBoolFromConfig(ConfigSection(),KDialogNonAttendedMode,activeObj->iDialogNonAttendedMode))
			{	
			INFO_PRINTF1(_L("DialogNonAttendedMode tag is required to continue the test"));	
			}	

		if (EFalse == GetStringFromConfig(ConfigSection(),KExpectedResult,activeObj->iExpectedResult))
			{	
			INFO_PRINTF1(_L("ExpectedResult tag is required to continue the test"));	
			}	
		verdict = activeObj->doTest5_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep6_0)
		{
		INFO_PRINTF1(_L("Test Encryption, Decryption and Mac computation with export keys on TLS"));
	
		verdict = activeObj->TestProvider_6_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep6_1)
		{
		INFO_PRINTF1(_L("Test Encryption, Decryption and Mac computation with export keys on SSL"));
	
		verdict = activeObj->TestProvider_6_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep6_2)
		{
		INFO_PRINTF1(_L("Test Encryption, Decryption and Mac computation on TLS"));
	
		verdict = activeObj->TestProvider_6_2L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep6_3)
		{
		INFO_PRINTF1(_L("Test Encryption, Decryption and Mac computation on TLS"));
	
		verdict = activeObj->TestProvider_6_3L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep7_0)
		{
		INFO_PRINTF1(_L("Verify Server Finished Message, TLS case"));
	
		verdict = activeObj->doTest7_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep7_1)
		{
		INFO_PRINTF1(_L("Verify Server Finished Message, SSL case"));
	
		verdict = activeObj->doTest7_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep7_2)
		{
		INFO_PRINTF1(_L("Verify Client Finished Message, TLS case"));
	
		verdict = activeObj->doTest7_2L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if ( TestStepName() == KTlsTestStep7_3)
		{
		INFO_PRINTF1(_L("Verify Client Finished Message, SSL case"));
	
		verdict = activeObj->doTest7_3L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep5_2)
		{
		INFO_PRINTF1(_L("Inside combined Test Steps 5.2 & 8.0"));
	
		verdict = activeObj->doTest5_2L( this );
		INFO_PRINTF1( activeObj->iLogInfo );		
		}
	else if(TestStepName() == KTlsTestStep5_3)
		{
		INFO_PRINTF1(_L("Inside Test Step 5.3"));
	
		verdict = activeObj->doTest5_3L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep5_4)
		{
		INFO_PRINTF1(_L("Inside combined Test Steps 5.4 & 8.2"));
	
		verdict = activeObj->doTest5_4L( this );
		INFO_PRINTF1( activeObj->iLogInfo );
		}
	else if(TestStepName() == KTlsTestStep5_5)
		{
		INFO_PRINTF1(_L("Inside Test Step 5.5"));
	
		verdict = activeObj->doTest5_5L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep8_1)
		{
		INFO_PRINTF1(_L("Inside Test Step 8.1"));
	
		verdict = activeObj->doTest8_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep9_0)
		{
		INFO_PRINTF1(_L("Inside Test Step 9.0"));
	
		verdict = activeObj->doTest9_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep10_0)
		{
		INFO_PRINTF1(_L("Inside Test Step 10.0"));
	
		verdict = activeObj->doTest10_0L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	else if(TestStepName() == KTlsTestStep10_1)
		{
		INFO_PRINTF1(_L("Inside Test Step 10.1"));
	
		verdict = activeObj->doTest10_1L( this );
		INFO_PRINTF1( activeObj->iLogInfo );	
		}
	delete activeObj;	
		
	SetTestStepResult( verdict );	
	return TestStepResult();

		
	}
		

TVerdict CTlsProvStep::doTestStepPostambleL()
/**
 * @return - TVerdict code
 * Override of base class virtual
 */
	{
	if(TestStepName() == KTlsTestStep2_0)
		{
		INFO_PRINTF1(_L("Test Step 2.0 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_0)
		{
		INFO_PRINTF1(_L("Test Step 4.0 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_1)
		{
		INFO_PRINTF1(_L("Test Step 4.1 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_2)
		{
		INFO_PRINTF1(_L("Test Step 4.2 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_3)
		{
		INFO_PRINTF1(_L("Test Step 4.3 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_4)
		{
		INFO_PRINTF1(_L("Test Step 4.4 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_5)
		{
		INFO_PRINTF1(_L("Test Step 4.5 Done"));
		}
	else if(TestStepName() == KTlsTestStep4_6)
		{
		INFO_PRINTF1(_L("Test Step 4.6 Done"));
		}
	else if(TestStepName() == KTlsTestStep5_2)
		{
		INFO_PRINTF1(_L("Test Steps 5.2 (& 8.0, 8.2) Done"));
		}
	else if(TestStepName() == KTlsTestStep5_3)
		{
		INFO_PRINTF1(_L("Test Step 5.3 Done"));
		}
	else if(TestStepName() == KTlsTestStep5_4)
		{
		INFO_PRINTF1(_L("Test Steps 5.4 (& 8.0, 8.2) Done"));
		}
	else if(TestStepName() == KTlsTestStep5_5)
		{
		INFO_PRINTF1(_L("Test Step 5.5 Done"));
		}
	else if(TestStepName() == KTlsTestStep7_0)
		{
		INFO_PRINTF1(_L("Test Step 7.0 Done"));
		}
	else if(TestStepName() == KTlsTestStep7_1)
		{
		INFO_PRINTF1(_L("Test Step 7.1 Done"));
		}
	else if(TestStepName() == KTlsTestStep7_2)
		{
		INFO_PRINTF1(_L("Test Step 7.2 Done"));
		}
	else if(TestStepName() == KTlsTestStep7_3)
		{
		INFO_PRINTF1(_L("Test Step 7.3 Done"));
		}
	else if(TestStepName() == KTlsTestStep8_1)
		{
		INFO_PRINTF1(_L("Test Step 8.1 Done"));
		}
	else if(TestStepName() == KTlsTestStep9_0)
		{
		INFO_PRINTF1(_L("Test Step 9.0 Done"));
		}
	else if(TestStepName() == KTlsTestStep10_0)
		{
		INFO_PRINTF1(_L("Test Step 10.0 Done"));
		}
	else if(TestStepName() == KTlsTestStep10_1)
		{
		INFO_PRINTF1(_L("Test Step 10.1 Done"));
		}
	return TestStepResult();
	}


//Reading test files

TInt CTlsProvStep::ReadDataForCreateL(CTlsCryptoAttributes*& aTlsCryptoAttributes,
									HBufC8*& aSrvCert)
	{
	RFs filesys;
	if ( KErrNone != filesys.Connect() )
		return -1;
	
	TInt res = KErrNone;
	
		
	RFile* file;
	file = new (ELeave) RFile;
	

		
	TBuf8<1000> bufForData;
	
	//
	// server random
	
	res = file->Open(filesys, 
				iServerRnd, 
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aTlsCryptoAttributes->iMasterSecretInput.iServerRandom.Copy(bufForData.Ptr(), 32);
	file->Close();
	
	
	//
	// client random
		
	res = file->Open(filesys, 
				iClientRnd, 
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aTlsCryptoAttributes->iMasterSecretInput.iClientRandom.Copy(bufForData.Ptr(), 32);
	file->Close();
	bufForData.Zero();
	
	
	//
	// server key parameters
	
	res = file->Open(filesys, 
				iKeyParam1, 
	 			EFileShareAny|EFileRead);
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		
		aTlsCryptoAttributes->iPublicKeyParams->iValue1 = bufForData.AllocL();
	
		file->Close();
		bufForData.Zero();
		}
		
	res = file->Open(filesys, 
				iKeyParam2,  
	 			EFileShareAny|EFileRead);
	 			
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		aTlsCryptoAttributes->iPublicKeyParams->iValue2 = bufForData.AllocL();
	
		file->Close();
		bufForData.Zero();
		}

	res = file->Open(filesys, 
				iKeyParam3, 
	 			EFileShareAny|EFileRead);
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		aTlsCryptoAttributes->iPublicKeyParams->iValue3 = bufForData.AllocL();
		bufForData.Zero();
		file->Close();
		}
		
	
	//
	// server cert

	res = file->Open(filesys, 
				iServerCertChain,  
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aSrvCert = bufForData.AllocL();
	
	file->Close();
	
		
	filesys.Close();
	
	return 0;
	}
	
	
TInt CTlsProvStep::ReadDataForClntFinishedL(HBufC8*& aClntFinInput,
								HBufC8*& aClntFinishedMsg )
	{
	RFs filesys;
	if ( KErrNone != filesys.Connect() )
		return -1;
	
	TInt res = KErrNone;
	
		
	RFile* file;
	file = new (ELeave) RFile;


		
	TBuf8<2000> bufForData;
	
	
	//
	// input for client finished check
	
	res = file->Open(filesys, 
				iHandshakeMsgsClient, 
	 			EFileShareAny|EFileRead);
	 			
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aClntFinInput = bufForData.AllocL();
	
	//
	// client finished message
	
	res = file->Open(filesys, 
				iClntFinishedCheckOutput, 
	 			EFileShareAny|EFileRead);
	 			
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aClntFinishedMsg = bufForData.AllocL();
	
	return 0;
	}
								
TInt  CTlsProvStep::ReadDataForSrvFinishedL(HBufC8*& aVerifySrvFinInput,
							HBufC8*& aSrvFinishedMsg )
	{
	
	RFs filesys;
	if ( KErrNone != filesys.Connect() )
		return -1;
	
	TInt res = KErrNone;
	
		
	RFile* file;
	file = new (ELeave) RFile;

		
	TBuf8<2000> bufForData;
	
	//
	// input for server finished check
	
	res = file->Open(filesys, 
				iHandshakeMsgsServer, 
	 			EFileShareAny|EFileRead);
	 			
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aVerifySrvFinInput = bufForData.AllocL();
	
	//
	// server finshed message
	
	res = file->Open(filesys, 
				iSrvFinishedCheckOutput, 
	 			EFileShareAny|EFileRead);
	 			
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aSrvFinishedMsg = bufForData.AllocL();
	
	return 0;
	
	}
	
TInt CTlsProvStep::ReadClientKeyExchL(HBufC8*& aClientKeyExchMsg )
	{
	
	RFs filesys;
	if ( KErrNone != filesys.Connect() )
		return -1;
	
	TInt res = KErrNone;
	
		
	RFile* file;
	file = new (ELeave) RFile;
	
	TBuf8<2000> bufForData;
	
	
	//
	// input for server finished check
	
	res = file->Open(filesys, 
				iClientKeyExch, 
	 			EFileShareAny|EFileRead);
	 			
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aClientKeyExchMsg = bufForData.AllocL();
	
	return 0;
	}

TInt ReadTestDataL( CTlsCryptoAttributes*& aTlsCryptoAttributes, HBufC8*& aSrvCert,
				   CTlsProvStep*& aTestStepPtr	  )
	{
	
	RFs filesys;
	if ( KErrNone != filesys.Connect() )
		return -1;
	
	TInt res = KErrNone;
	
		
	RFile* file;
	file = new (ELeave) RFile;

	
		
	TBuf8<1000> bufForData;
	
	//
	// server random
	
	res = file->Open(filesys, 
				aTestStepPtr->iServerRnd, 
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aTlsCryptoAttributes->iMasterSecretInput.iServerRandom.Copy(bufForData.Ptr(), 32);
	file->Close();
	
	
	//
	// client random
		
	res = file->Open(filesys, 
				aTestStepPtr->iClientRnd, 
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aTlsCryptoAttributes->iMasterSecretInput.iClientRandom.Copy(bufForData.Ptr(), 32);
	file->Close();
	bufForData.Zero();
	
	
	//
	// server key parameters
	//aTlsCryptoAttributes->iPublicKeyParams = new CTLSPublicKeyParams;
	res = file->Open(filesys, 
				aTestStepPtr->iKeyParam1, 
	 			EFileShareAny|EFileRead);
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		
		aTlsCryptoAttributes->iPublicKeyParams->iValue1 = bufForData.AllocL();
	
		file->Close();
		bufForData.Zero();
		}
		
	res = file->Open(filesys, 
				aTestStepPtr->iKeyParam2,  
	 			EFileShareAny|EFileRead);
	 			
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		aTlsCryptoAttributes->iPublicKeyParams->iValue2 = bufForData.AllocL();
	
		file->Close();
		bufForData.Zero();
		}

	res = file->Open(filesys, 
				aTestStepPtr->iKeyParam3, 
	 			EFileShareAny|EFileRead);
	if(!res )
		{
		res = file->Read(bufForData);
		if( KErrNone != res )
			{
			file->Close();
			filesys.Close();
			return -1;
			}
		aTlsCryptoAttributes->iPublicKeyParams->iValue3 = bufForData.AllocL();
		bufForData.Zero();
		file->Close();
		}
		
	
	//
	// server cert

	res = file->Open(filesys, 
				aTestStepPtr->iServerCertChain,  
	 			EFileShareAny|EFileRead);
	if( KErrNone != res )
		{
		filesys.Close();
		return -1;
		}
	res = file->Read(bufForData);
	if( KErrNone != res )
		{
		file->Close();
		filesys.Close();
		return -1;
		}
	
	aSrvCert = bufForData.AllocL();
	
	file->Close();
	
	filesys.Close();
	delete file;
	return 0;
	}



TInt CTlsProvTestActive::StandardTestInitL(
								CTlsProvStep* aStep,
								CTlsCryptoAttributes* tlsCryptoAttributes, 
								HBufC8*& aEncServerCert)
	{
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerRnd"),aStep->iServerRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientRnd"),aStep->iClientRnd);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerCert"),aStep->iServerCertChain);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("HandshakeMsgsServer"),aStep->iHandshakeMsgsServer);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SrvFinishedCheckOutput"),aStep->iSrvFinishedCheckOutput);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("Premaster"),aStep->iPremaster);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("IssuerName"),iIssuerName);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyDerivation128"),iKeyDerivation128);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyDerivation64"),iKeyDerivation64);

	tlsCryptoAttributes->iClientAuthenticate = EFalse;
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iAddress.Copy( KServer1 );
	tlsCryptoAttributes->iSessionNameAndID.iServerName.iPort = 10;
	tlsCryptoAttributes->iSessionNameAndID.iSessionId.Append( KSessionId1 );
	 
	
	tlsCryptoAttributes->iCompressionMethod = ENullCompression;
	tlsCryptoAttributes->iCurrentCipherSuite.iHiByte = 0;
	tlsCryptoAttributes->iCurrentCipherSuite.iLoByte = 3;
	
		
	tlsCryptoAttributes->iNegotiatedProtocol.iMajor = 3;
	tlsCryptoAttributes->iNegotiatedProtocol.iMinor = 1; 
	
	tlsCryptoAttributes->iProposedProtocol.iMajor = 3;
	tlsCryptoAttributes->iProposedProtocol.iMinor = 1; 
		
		
	//tlsCryptoAttributes->iPublicKeyParams = new CTLSPublicKeyParams;
	tlsCryptoAttributes->iPublicKeyParams->iKeyType = ERsa;
		
	
	tlsCryptoAttributes->iClientAuthenticate = EFalse; 
	tlsCryptoAttributes->iDialogNonAttendedMode = ETrue;
	
	TInt res = aStep->ReadDataForCreateL(tlsCryptoAttributes, aEncServerCert);
	
	return res;
	
	}
	
	
TBool CTlsProvTestActive::CacheSessionL(CTlsProvStep* aStep, CTLSSession* aSessionObj)
	{
	HBufC8* verifySrvFinInput = NULL;
	HBufC8* finshedMsg = NULL;
	
	TInt res = aStep->ReadDataForSrvFinishedL(verifySrvFinInput, finshedMsg);
		if( res  < 0 )
		{
		iLogInfo.Copy( _L("	problems with test data files (for server finished check)") );
		return EFalse;
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
	aSessionObj->VerifyServerFinishedMsgL( 
		md5Dig,
		shaDig,
		finshedMsg->Des(), 
		iStatus) ;
	
	SetActive();
	CActiveScheduler::Start();
	
				
	CleanupStack::PopAndDestroy( 2, md5Dig );
	
	if ( iStatus.Int() != KErrNone )
		{
		iLogInfo.Copy( _L("	CTLSSession::VerifyServerFinishedMsgL failed") );
		return EFalse;
		}
		
	return ETrue;
}





void CTlsTestRandom::GenerateBytesL(TDes8& aDestination)
	{
		CSystemRandom* rand = NULL;
		rand = CSystemRandom::NewLC();
				
		rand->GenerateBytesL( aDestination );
				
		CleanupStack::PopAndDestroy( rand );
							
		RFs fs;
		User::LeaveIfError( fs.Connect() );
		
		TBuf8<512> bufForData;	
		RFile* file;
		file=new (ELeave) RFile;
						
		TInt res;
		TDriveUnit sysDrive(fs.GetSystemDrive());
		TDriveName sysDriveName (sysDrive.Name());
		TFileName fileName (sysDriveName);
		fileName.Append(_L("\\DHRandom.bin"));
		
		res = file->Open(fs, fileName, EFileShareAny|EFileRead);
		
		if( (KErrNone == res) && (48 != aDestination.MaxLength()) )
			{
			file->Read(bufForData);
			aDestination.Copy( bufForData);
				
			file->Close();
			fs.Close();
			delete file;
			return;
			}
			
		if( 48 != aDestination.MaxLength() )
			{
			fs.Close();
			delete file;
			return;
			}
			
		fileName.Copy(sysDriveName);
		fileName.Append(_L("\\Premaster.bin"));	
		res = file->Open(fs, fileName, EFileShareAny|EFileRead);
		if ( KErrNone == res )
			{
			
			file->Read(bufForData);
			aDestination.Copy( bufForData);
			aDestination.SetLength( 48 );
				
			file->Close();
										
			}
			
		fs.Close();
		delete file;
		return;
	
	}


