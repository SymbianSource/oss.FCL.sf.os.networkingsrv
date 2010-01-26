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
Mode:RC4 with weak encryption
Protocol: TLS
Tests covered:
	1.Generation of weak keys are also being tested
	2.MAC computations
	3.Encryption
	4.Decryption
*/
TVerdict CTlsProvTestActive::TestProvider_6_0L( CTlsProvStep* aStep )
	{

	
	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;

	INFO_PRINTF1(_L("1"));
	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,/*IsTls?*/ETrue,/*IsExport?*/ETrue,aStep);

	//Want client authentication?
	PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

	//Any dialogs
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 0x19;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;	

	
	//dummy flag untuill the token panic is resolved
	RFs fs;
	RFile file;
	fs.Connect();

	TBuf8<800> boom;
	file.Open(fs, 
				aStep->iServerCertChain,  
	 			EFileShareAny|EFileRead);
	file.Read(boom);
	
	iStatus = KRequestPending;
	CX509Certificate* serverCert;
	HBufC8* servrc = boom.AllocL();
	PtrProvider->VerifyServerCertificate(servrc->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("2"));

	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 0x19;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;

	iStatus = KRequestPending;		
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("3"));


	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 3;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	
	iStatus = KRequestPending;
	HBufC8* clntkeyexchang;
	if(!PtrSession)
   {
   	iLogInfo.Copy(_L("	6.1:  !PtrSession"));
		return EFail;
   }


	PtrSession->ClientKeyExchange(clntkeyexchang,iStatus);
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("4"));

	if(PtrSession)
   {
   	iLogInfo.Copy(_L("	6.1:  EncryptAndDecryptL"));
		TVerdict ver = (EncryptAndDecryptL(PtrSession,aStep));
      delete PtrProvider;
      delete PtrSession;
      return ver;
   }
	else
   {
      delete PtrProvider;
      delete PtrSession;
		return EFail;
   }

	}

/*
Mode:RC4 with weak encryption
Protocol: SSL
Tests covered:
	1.Generation of weak keys are also being tested
	2.MAC computations
	3.Encryption
	4.Decryption
*/

TVerdict CTlsProvTestActive::TestProvider_6_1L( CTlsProvStep* aStep )
	{
	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;

	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,/*IsTls?*/EFalse,/*IsExport?*/ETrue,aStep);

	//Want client authentication?
	PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

	//Any dialogs
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 3;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KSSL3_0;	

	
	//Init start
	RFs fs;
	RFile file;
	fs.Connect();	
	TBuf8<1000> boom;
	file.Open(fs, 
				aStep->iServerCertChain,  
	 			EFileShareAny|EFileRead);
	file.Read(boom);
	
	iStatus = KRequestPending;
	CX509Certificate* serverCert;
	HBufC8* servrc = boom.AllocL();
	PtrProvider->VerifyServerCertificate(servrc->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();

	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 0x19;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	//Init end




	iStatus = KRequestPending;	
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();



	//Init start
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 3;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;	
	iStatus = KRequestPending;
	HBufC8* clntkeyexchang;
	if(!PtrSession)
   {
   	iLogInfo.Copy(_L("	6.1:  !PtrSession"));
		return EFail;
   }
	PtrSession->ClientKeyExchange(clntkeyexchang,iStatus);
	SetActive();
	CActiveScheduler::Start();
	//Init end

	TInt errr = iStatus.Int();

	if(PtrSession && !errr)
   {
   	iLogInfo.Copy(_L("	6.1:  EncryptAndDecryptL"));
		TVerdict ver = (EncryptAndDecryptL(PtrSession,aStep));
      delete PtrProvider;
      delete PtrSession;
      return ver;
   }
	else
   {
   	iLogInfo.Format(_L("	6.1:  Fail %d"), errr);
      delete PtrProvider;
      delete PtrSession;
		return EFail;
   }
	}


/*
Mode:DES with strong encryption
Protocol: TLS
Tests covered:
	1.MAC computations
	2.Encryption
	3.Decryption
*/
TVerdict CTlsProvTestActive::TestProvider_6_2L( CTlsProvStep* aStep )
	{
	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;

	INFO_PRINTF1(_L("1"));
	iLogInfo.Copy(_L("	6.2:  InitProviderL"));
	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,/*IsTls?*/ETrue,/*IsExport?*/EFalse,aStep);

	//Want client authentication?
	PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

	//Any dialogs
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 9;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KTLS1_0;	

	
	//Init start
	RFs fs;
	RFile file;
	fs.Connect();	
	TBuf8<1000> boom;
	file.Open(fs, 
				aStep->iServerCertChain,  
	 			EFileShareAny|EFileRead);
	file.Read(boom);
	
	iStatus = KRequestPending;
	CX509Certificate* serverCert;
	HBufC8* servrc = boom.AllocL();
	PtrProvider->VerifyServerCertificate(servrc->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	iLogInfo.Copy(_L("	6.2:  IPtrProvider->VerifyServerCertificate"));
	INFO_PRINTF1(_L("2"));

	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 0x19;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	//Init end



	iStatus = KRequestPending;	
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();
	INFO_PRINTF1(_L("3"));

	iLogInfo.Copy(_L("	6.2:  IPtrProvider->CreateL"));

	//Init start
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 9;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;	
	iStatus = KRequestPending;
	HBufC8* clntkeyexchang;
	if(!PtrSession)
   {
		iLogInfo.Copy(_L("	6.2:  !PtrSession"));
      delete PtrProvider;
		return EFail;
   }
	PtrSession->ClientKeyExchange(clntkeyexchang,iStatus);
	SetActive();
	CActiveScheduler::Start();
	//Init end
	INFO_PRINTF1(_L("4"));


	if(PtrSession)
   {
   	INFO_PRINTF1(_L("5"));
   	iLogInfo.Copy(_L("	6.2:  EncryptAndDecryptL"));
		TVerdict ver = (EncryptAndDecryptL(PtrSession,aStep));
      delete PtrProvider;
      delete PtrSession;
      return ver;
   }
	else
   {
      delete PtrProvider;
		return EFail;
   }

	}


/*
Mode:DES with strong encryption
Protocol: SSL
Tests covered:
	1.MAC computations
	2.Encryption
	3.Decryption
*/
TVerdict CTlsProvTestActive::TestProvider_6_3L( CTlsProvStep* aStep )
	{
	CTLSProvider* PtrProvider = 0;
	CTLSSession* PtrSession;
	CTlsCryptoAttributes* PtrTlsCryptoAttributes;

	INFO_PRINTF1(_L("1"));
	iLogInfo.Copy(_L("	6.3:  InitProviderL"));
	InitProviderL(PtrProvider,PtrSession,PtrTlsCryptoAttributes,/*IsTls?*/EFalse,/*IsExport?*/EFalse,aStep);

	//Want client authentication?
	PtrTlsCryptoAttributes->iClientAuthenticate = EFalse;

	//Any dialogs
	PtrTlsCryptoAttributes->iDialogNonAttendedMode = ETrue;

	//Required ciphersuite
	TTLSCipherSuite	 CipherSuite;
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 9;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	PtrTlsCryptoAttributes->iNegotiatedProtocol = KSSL3_0;	

	
	//Init start
	RFs fs;
	RFile file;
	fs.Connect();	
	TBuf8<1000> boom;
	file.Open(fs, 
				aStep->iServerCertChain,  
	 			EFileShareAny|EFileRead);
	file.Read(boom);
	
	iStatus = KRequestPending;
	CX509Certificate* serverCert;
	HBufC8* servrc = boom.AllocL();
	PtrProvider->VerifyServerCertificate(servrc->Des(), serverCert, iStatus);
	SetActive();
	CActiveScheduler::Start();
	iLogInfo.Copy(_L("	6.3:  IPtrProvider->VerifyServerCertificate"));
	INFO_PRINTF1(_L("2"));

	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 0x19;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;
	//Init end

	iStatus = KRequestPending;	
	PtrProvider->CreateL(PtrSession,iStatus);
	SetActive();
	CActiveScheduler::Start();

	iLogInfo.Copy(_L("	6.3:  IPtrProvider->CreateL"));

	//Init start
	CipherSuite.iHiByte = 0;
	CipherSuite.iLoByte = 9;
	PtrTlsCryptoAttributes->iCurrentCipherSuite = CipherSuite;	
	iStatus = KRequestPending;
	HBufC8* clntkeyexchang;
	INFO_PRINTF1(_L("3"));
	if(!PtrSession)
   {
      delete PtrProvider;
		iLogInfo.Copy(_L("	6.3:  !PtrSession"));
		return EFail;
   }
	PtrSession->ClientKeyExchange(clntkeyexchang,iStatus);
	SetActive();
	CActiveScheduler::Start();
	//Init end


	if(PtrSession)
   {
   	INFO_PRINTF1(_L("4"));
   	iLogInfo.Copy(_L("	6.3:  EncryptAndDecryptL"));
		TVerdict ver = (EncryptAndDecryptL(PtrSession,aStep));
      delete PtrProvider;
      delete PtrSession;
      return ver;
   }
	else
   {
      delete PtrProvider;
		return EFail;
   }

	}



/*
HELPER:
The actual step thta does the encryption and decryption

*/
TVerdict CTlsProvTestActive::EncryptAndDecryptL(CTLSSession* aPtrTlsSession, CTlsProvStep* /*aStep*/)
	{
	
	RFs filesys;
	filesys.Connect();
	RFile fileTmp_t;
	
	TBuf8<1024> TempPrint;
	TDriveUnit sysDrive (filesys.GetSystemDrive());
	TDriveName sysDriveName (sysDrive.Name());
	
	TBuf<128> fileName (sysDriveName);
	fileName.Append(_L("\\data\\ActualAppData.bin"));
	
	TInt result_t = fileTmp_t.Open(filesys, fileName, EFileRead);
		
  	INFO_PRINTF1(_L("EncryptAndDecryptL 1"));
	if(!result_t)
		fileTmp_t.Read( TempPrint );
	fileTmp_t.Close();	

	
	iStatus = KRequestPending;
	HBufC8* Output = NULL;
	TRecordProtocol RecType;
	RecType= EHandshake;
	TInt64 ASeqNumber = 0;
	Output = HBufC8::NewL(TempPrint.Length() + 24);
	aPtrTlsSession->EncryptL( 
		TempPrint, 
		Output, 
		ASeqNumber,RecType) ;
	
  	INFO_PRINTF1(_L("EncryptAndDecryptL 2"));
  	
  	fileName.Copy(sysDriveName);
  	fileName.Append(_L("\\data\\EncryptOutput.bin"));
	
	result_t = fileTmp_t.Open(filesys, fileName, EFileRead);
	
	TBuf8<1024> ActualOutput;	
	if(!result_t)
		fileTmp_t.Read(ActualOutput);
	fileTmp_t.Close();
	if(ActualOutput.Compare(Output->Des()) != 0)
		return EFail;

	//Test Decryption here

  	INFO_PRINTF1(_L("EncryptAndDecryptL 3"));
  	
  	fileName.Copy(sysDriveName);
  	fileName.Append(_L("\\data\\DecryptionInput.bin"));
	
	result_t = fileTmp_t.Open(filesys, fileName, EFileRead);
	
	if(!result_t)
		fileTmp_t.Read( ActualOutput );
	fileTmp_t.Close();


	iStatus = KRequestPending;
	HBufC8* OutputDe = NULL;
	
	RecType= EHandshake;
	TInt64 ASeqNumberDe = 0;
	OutputDe = HBufC8::NewL(ActualOutput.Length());
	aPtrTlsSession->DecryptAndVerifyL( 
		ActualOutput, 
		OutputDe, 
		ASeqNumberDe,RecType) ;

	fileName.Copy (sysDriveName);
	fileName.Append(_L("\\data\\DecryptionOutputNoMac.bin"));
	
	result_t = fileTmp_t.Open(filesys, fileName, EFileRead);
	
	if(!result_t)
		fileTmp_t.Read(ActualOutput);
	fileTmp_t.Close();
	
  	INFO_PRINTF1(_L("EncryptAndDecryptL 4"));
	if(ActualOutput.Compare(OutputDe->Des()) != 0)
   {
	   iLogInfo.Copy(_L("	EncryptDecr Fail"));
		return EFail;
   }
	else
   {
	   iLogInfo.Copy(_L("	EncryptDecr Success"));
		return EPass;
   }
	}

/*
HELPER:
Initializes the TLSProvider
*/
TVerdict CTlsProvTestActive::InitProviderL(CTLSProvider*& aPtrProvider,CTLSSession*& /*aPtrSession*/,
									  CTlsCryptoAttributes*& aTlsCryptoAttributes,
									  TBool aIsTls, TBool aIsExport,CTlsProvStep* aStep)
	{
	RArray<TTLSCipherSuite> UserCipherSuiteList;
	if(!aPtrProvider)
		aPtrProvider = CTLSProvider::ConnectL();

	//Obtain the list of ciphersuites
	iStatus = KRequestPending;
	aPtrProvider->CipherSuitesL(UserCipherSuiteList,iStatus);	
	SetActive();
	CActiveScheduler::Start();

	TInt SelectedCiphers = UserCipherSuiteList.Count();
	if(SelectedCiphers != KSupportedCipherCount) 
		{
		return EFail;			
		}

	
	aTlsCryptoAttributes = aPtrProvider->Attributes();
	if(aIsTls)
		ConfigureTLS(aIsExport,aStep);
	else
		ConfigureSSL(aIsExport,aStep);

	HBufC8* SrvCertificate;
	ReadTestDataL(aTlsCryptoAttributes, SrvCertificate,aStep);
	delete SrvCertificate;
	aTlsCryptoAttributes->iPublicKeyParams->iKeyType = ERsa;
	UserCipherSuiteList.Reset();
	return EPass;
	
	}


/*
HELPER:
Configures the initialization data for TLS
*/
void CTlsProvTestActive::ConfigureTLS(TBool aIsExport, CTlsProvStep* aStep)
	{
	if(aIsExport)
		{
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TLSServerRndExport"),aStep->iServerRnd);
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TLSClientRndExport"),aStep->iClientRnd);
		}
	else
		{
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsServerRnd"),aStep->iServerRnd);
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsClientRnd"),aStep->iClientRnd);
		}
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsKeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsKeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsKeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("TlsServerCert"),aStep->iServerCertChain);	
	}


/*
HELPER:
Configures the initialization data for SSL
*/
void CTlsProvTestActive::ConfigureSSL(TBool aIsExport, CTlsProvStep* aStep)
	{
	if(aIsExport)
		{
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLServerRndExport"),aStep->iServerRnd);
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("SSLClientRndExport"),aStep->iClientRnd);
		}
	else
		{
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerRnd"),aStep->iServerRnd);
		aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ClientRnd"),aStep->iClientRnd);
		}
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams1"),aStep->iKeyParam1);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams2"),aStep->iKeyParam2);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("KeyParams3"),aStep->iKeyParam3);
	aStep->GetStringFromConfig(aStep->ConfigSection(),_L("ServerCert"),aStep->iServerCertChain);
	}
