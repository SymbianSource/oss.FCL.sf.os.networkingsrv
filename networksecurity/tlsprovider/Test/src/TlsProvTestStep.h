/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file TlsProvTestStep.h
*/
#if (!defined __TLSPROV_STEP_H__)
#define __TLSPROV_STEP_H__
#include <test/testexecutestepbase.h>
#include "TlsProvServer.h"

#include "tlstypedef.h"
#include "tlsprovinterface.h"
#include <random.h>
#include <s32file.h> 
#include <secdlgimpldefs.h>

// common constants 
#define KServer1  _L8("192.168.30.2")
#define KServer2  _L8("192.168.10.11")

#define KServer3  _L8("192.168.10.210")

#define KSupportedCipherCount  (11+1+2)   //Including test ciphers

#define KSessionId1 _L8("11111111112222222222333333333322")
#define KSessionId2 _L8("222222222233333333334444444444")
#define KSessionId3 _L8("33333333334444444444555555555522")
#define KSessionId4 _L8("444444444455555555556666666666")
#define KSessionId5 _L8("55555555556666666666777777777722")
#define KSessionId6 _L8("66666666667777777777888888888822")

// forward declarations:
class CTlsProvStep;

class CTlsProvTestActive : public CActive
	{
public:
	CTlsProvTestActive( CTestExecuteLogger& aLogger );
	~CTlsProvTestActive();
	
	
	TVerdict doTest2_0L( CTlsProvStep* aStep );	
	TVerdict doTest4_0L( CTlsProvStep* aStep );
	TVerdict doTest4_1L( CTlsProvStep* aStep );
	TVerdict doTest4_2L( CTlsProvStep* aStep );
	TVerdict doTest4_3L( CTlsProvStep* aStep );
	TVerdict doTest4_4L( CTlsProvStep* aStep );
	TVerdict doTest4_5L( CTlsProvStep* aStep );
	TVerdict doTest4_6L( CTlsProvStep* aStep );	
	TVerdict doTest5_2L( CTlsProvStep* aStep );
	TVerdict doTest5_3L( CTlsProvStep* aStep );
	TVerdict doTest5_4L( CTlsProvStep* aStep );
	TVerdict doTest5_5L( CTlsProvStep* aStep );
	
	TVerdict doTest7_0L( CTlsProvStep* aStep );
	TVerdict doTest7_1L( CTlsProvStep* aStep );
	TVerdict doTest7_2L( CTlsProvStep* aStep );
	TVerdict doTest7_3L( CTlsProvStep* aStep );
		

	
	TVerdict doTest8_1L( CTlsProvStep* aStep );
	
	TVerdict doTest9_0L( CTlsProvStep* aStep );
	
	TVerdict doTest10_0L( CTlsProvStep* aStep );
	TVerdict doTest10_1L( CTlsProvStep* aStep );

	//*************************************Provider Tests*****************************************
	
	//Tests for obtaining the list of ciphersuites
	TVerdict doTest1_0L( CTlsProvStep* aStep );
	TVerdict doTest1_1L( CTlsProvStep* aStep );

	//Tests for invalid paramers from handshake, selecting a token, certificate,session resumption, etc
	TVerdict TestProvider_3_0L( CTlsProvStep* aStep );
	TVerdict TestProvider_3_1L( CTlsProvStep* aStep );
	TVerdict TestProvider_3_2L( CTlsProvStep* aStep );
	TVerdict TestProvider_3_3L( CTlsProvStep* aStep );

	//Tests for verifying a server certificate in both valid and invalid modes
	TVerdict doTest5_0L( CTlsProvStep* aStep );
	TVerdict doTest5_1L( CTlsProvStep* aStep );
	
	//Tests for Encryption, Decryption, MAC computation and export Key generation
	TVerdict TestProvider_6_0L( CTlsProvStep* aStep );
	TVerdict TestProvider_6_1L( CTlsProvStep* aStep );
	TVerdict TestProvider_6_2L( CTlsProvStep* aStep );
	TVerdict TestProvider_6_3L( CTlsProvStep* aStep );
	TVerdict EncryptAndDecryptL(CTLSSession* aPtrTlsSession, CTlsProvStep* aStep);

	
	//Helper functions
	void ConfigureTLS(TBool aIsExport, CTlsProvStep* aStep);
	void ConfigureSSL(TBool aIsExport, CTlsProvStep* aStep);
	TVerdict InitProviderL(CTLSProvider*& aPtrProvider,CTLSSession*& aPtrSession,CTlsCryptoAttributes*& aTlsCryptoAttributes,
							TBool aIsTls, TBool aIsExport,CTlsProvStep* aStep);
	TInt StandardTestInitL( CTlsProvStep* aStep,
							CTlsCryptoAttributes* tlsCryptoAttributes, 
							HBufC8*& aEncServerCert);
	TBool CacheSessionL(CTlsProvStep* aStep, 
						CTLSSession* aSessionObj);

	void DeleteFileL();
	void WriteDialogRecordL(RFileWriteStream& aStream, TSecurityDialogOperation aOp, const TDesC& aLabelSpec,const TDesC& aResponse1, const TDesC& aResponse2);	
	
	//********************************************************************************************
	
	
	//Active
	void DoCancel() { return; };
	void RunL();
	void AddDNL();
	
   CTestExecuteLogger& Logger(){return iLogger;}
	//Log buffer
	TBuf<150> iLogInfo;
   CTestExecuteLogger& iLogger;
   TBool iDialogNonAttendedMode;
   TPtrC iExpectedResult;
   RFs iFs;
   RFile iFile;
	RPointerArray<HBufC8> iDNs;
   TPtrC iIssuerName;
   TPtrC iKeyDerivation128;
   TPtrC iKeyDerivation64;
private:
	
	};


class CTlsProvStep : public CTestStep
	{
public:
	CTlsProvStep(const TDesC& aStepName);
	~CTlsProvStep();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepPostambleL();
	virtual TVerdict doTestStepL();
	TInt ReadDataForCreateL(CTlsCryptoAttributes*& aTlsCryptoAttributes,
							HBufC8*& aSrvCert);
	TInt ReadDataForSrvFinishedL(HBufC8*& aVerifySrvFinInput,
								HBufC8*& aSrvFinishedMsg );
	TInt ReadDataForClntFinishedL(HBufC8*& aClntFinInput,
								HBufC8*& aClntFinishedMsg );
	TInt ReadClientKeyExchL(HBufC8*& aClientKeyExchMsg );
								
public:
	TPtrC iServerRnd;
	TPtrC iClientRnd;
	TPtrC iKeyParam1;
	TPtrC iKeyParam2;
	TPtrC iKeyParam3;
	TPtrC iServerCertChain;
	TPtrC iHandshakeMsgsServer;
	TPtrC iSrvFinishedCheckOutput;
	TPtrC iHandshakeMsgsClient;
	TPtrC iClntFinishedCheckOutput;
	TPtrC iPremaster;
	TPtrC iClientKeyExch;
private:
	TVerdict verdict;
	};


class CTlsTestRandom : public CRandom
	{
public:
	virtual void GenerateBytesL(TDes8& aDest);
	};
	
	
class CTlsProvStep1 : public CTestStep
	{
public:
	CTlsProvStep1();
	~CTlsProvStep1();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepPostambleL();
	virtual TVerdict doTestStepL();
private:
	};

class CTlsProvStep2 : public CTestStep
	{
public:
	CTlsProvStep2();
	~CTlsProvStep2();
	virtual TVerdict doTestStepPreambleL();
	virtual TVerdict doTestStepPostambleL();
	virtual TVerdict doTestStepL();
private:
	
	};
	


_LIT(KTlsTestStep1,"TlsTestStep1");
_LIT(KTlsTestStep2,"TlsTestStep2");

_LIT(KTlsTestStep1_0,"TlsTestStep1.0");
_LIT(KTlsTestStep1_1,"TlsTestStep1.1");

_LIT(KTlsTestStep2_0,"TlsTestStep2.0");
_LIT(KTlsTestStep3_0,"TlsTestStep3.0");
_LIT(KTlsTestStep3_1,"TlsTestStep3.1");
_LIT(KTlsTestStep3_2,"TlsTestStep3.2");
_LIT(KTlsTestStep3_3,"TlsTestStep3.3");

_LIT(KTlsTestStep4_0,"TlsTestStep4.0");
_LIT(KTlsTestStep4_1,"TlsTestStep4.1");
_LIT(KTlsTestStep4_2,"TlsTestStep4.2");
_LIT(KTlsTestStep4_3,"TlsTestStep4.3");
_LIT(KTlsTestStep4_4,"TlsTestStep4.4");
_LIT(KTlsTestStep4_5,"TlsTestStep4.5");
_LIT(KTlsTestStep4_6,"TlsTestStep4.6");

_LIT(KTlsTestStep5_0,"TlsTestStep5.0");
_LIT(KTlsTestStep5_1,"TlsTestStep5.1");

_LIT(KTlsTestStep5_2,"TlsTestStep5.2");
_LIT(KTlsTestStep5_3,"TlsTestStep5.3");
_LIT(KTlsTestStep5_4,"TlsTestStep5.4");
_LIT(KTlsTestStep5_5,"TlsTestStep5.5");
_LIT(KTlsTestStep6_0,"TlsTestStep6.0");
_LIT(KTlsTestStep6_1,"TlsTestStep6.1");
_LIT(KTlsTestStep6_2,"TlsTestStep6.2");
_LIT(KTlsTestStep6_3,"TlsTestStep6.3");

_LIT(KTlsTestStep7_0,"TlsTestStep7.0");
_LIT(KTlsTestStep7_1,"TlsTestStep7.1");
_LIT(KTlsTestStep7_2,"TlsTestStep7.2");
_LIT(KTlsTestStep7_3,"TlsTestStep7.3");
//_LIT(KTlsTestStep8_0,"TlsTestStep8.0"); - covered by 5.2 
_LIT(KTlsTestStep8_1,"TlsTestStep8.1");
//_LIT(KTlsTestStep8_2,"TlsTestStep8.2"); - covered by 5.4
_LIT(KTlsTestStep9_0,"TlsTestStep9.0");

_LIT(KTlsTestStep10_0,"TlsTestStep10.0");
_LIT(KTlsTestStep10_1,"TlsTestStep10.1");

_LIT(KTlsSecureConnectionTestStep,"TSecureConnection");

#endif
