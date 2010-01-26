// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <c32comm.h>
#include <test/testexecutelog.h>
#include "TSecureConnection.h"


// PDD names for the physical device drivers that are loaded in wins or arm
#if defined (__WINS__)
#define PDD_NAME		_L("ECDRV")
#else
#define PDD_NAME		_L("EUART1")
#define PDD2_NAME		_L("EUART2")
#define PDD3_NAME		_L("EUART3")
#define PDD4_NAME		_L("EUART4")
#endif
#define LDD_NAME		_L("ECOMM")

_LIT8(KUserAgent, "SimpleHTTPClient (1.0)");
_LIT8(KAccept, "*/*");

CTSecureConnectionStep::CTSecureConnectionStep(const TDesC& aStepName)
	{
	SetTestStepName( aStepName );
	}
	
CTSecureConnectionStep::~CTSecureConnectionStep()
	{
	}

TVerdict CTSecureConnectionStep::doTestStepPreambleL()
	{	
	INFO_PRINTF1(_L("CTSecureConnectionStep Check Start"));
	
	InitCommsL();
	User::LeaveIfError(iFs.Connect());
	ConvertDlgConfigL();
	
	iScheduler=new(ELeave) CActiveScheduler;
	CActiveScheduler::Install(iScheduler);

	SetTestStepResult(EPass);
	return TestStepResult();
	}

TVerdict CTSecureConnectionStep::doTestStepL()
	{
	TPtrC url;				
	if(!GetStringFromConfig(ConfigSection(), KUrl, url))
		{
		ERR_PRINTF1(_L("Missing url"));
		SetTestStepError(KErrBadName);
		SetTestStepResult(EFail);
		}
		
	iHttp=CSimpleHttpClient::NewL(this);
	iHttp->StartClientL(url);
	
	return TestStepResult();
	}

TVerdict CTSecureConnectionStep::doTestStepPostambleL()
	{
	INFO_PRINTF1(_L("CTSecureConnectionStep Check Done"));
	iFs.Close();
	delete iScheduler;
	return TestStepResult();
	}

void CTSecureConnectionStep::WriteDialogRecordL(RFileWriteStream& aStream, TSecurityDialogOperation aOp, const TDesC& aLabelSpec,
											 const TDesC& aResponse1, const TDesC& aResponse2)
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

void CTSecureConnectionStep::InitCommsL()
	{
	// StartC32
	TInt ret = User::LoadPhysicalDevice(PDD_NAME);
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);

#ifndef __WINS__
	ret = User::LoadPhysicalDevice(PDD2_NAME);
	ret = User::LoadPhysicalDevice(PDD3_NAME);
	ret = User::LoadPhysicalDevice(PDD4_NAME);
#endif

	ret = User::LoadLogicalDevice(LDD_NAME);
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);
	ret = StartC32();
	User::LeaveIfError(ret == KErrAlreadyExists?KErrNone:ret);		
	}
	
void CTSecureConnectionStep::ConvertDlgConfigL()
	{
	//Clean the response files
	CFileMan* fileMan = CFileMan::NewL(iFs);
	CleanupStack::PushL(fileMan);
	
	TDriveUnit sysDrive (RFs::GetSystemDrive());
	TDriveName sysDriveName (sysDrive.Name());
	
	TBuf<128> datFile (sysDriveName);
	datFile.Append(KTSecDlgOutputFile);
	TInt err = fileMan->Delete(datFile);
	if (err != KErrNotFound)
		User::LeaveIfError(err);
	
	datFile.Copy(sysDriveName);
	datFile.Append(KTSecDlgInputFile);
	
	err = fileMan->Delete(datFile);
	if (err != KErrNotFound)
		User::LeaveIfError(err);
	
	CleanupStack::PopAndDestroy(fileMan);
	
	//Create the response input file
	RFileWriteStream stream;
	err = stream.Open(iFs, datFile, EFileWrite | EFileShareExclusive);
	if (err == KErrNotFound)
		{
		err = stream.Create(iFs, datFile, EFileWrite | EFileShareExclusive);
		}
	User::LeaveIfError(err);
	stream.PushL();
	
	// Read the dialog response data
	TPtrC res1;
	if (GetStringFromConfig(ConfigSection(), KServerAuthenticationResponse, res1))
		{
		WriteDialogRecordL(stream, EServerAuthenticationFailure, _L("Server Authentication Dialog"), res1, KNullDesC);			
		}	

	TPtrC res3, res4;
	if (GetStringFromConfig(ConfigSection(), KClientAuthenticationResponse1, res3) && GetStringFromConfig(ConfigSection(), KClientAuthenticationResponse2, res4))
		{
		WriteDialogRecordL(stream, ESecureConnection, _L("Client Authentication Dialog"), res3, res4);
		}		
	
	TPtrC res5;
	if (!GetStringFromConfig(ConfigSection(), KKeyStorePassPhrase, res5))
		{
		res5.Set(_L("pinkcloud"));				
		}
	WriteDialogRecordL(stream, EEnterPIN, _L("Key store passphrase"), res5, KNullDesC);	

	stream.CommitL();
	CleanupStack::PopAndDestroy(); // stream		
	}
	
void CTSecureConnectionStep::ReceiveTestResult(const TInt& aErrorCode, TVerdict aVerdict)
	{
	SetTestStepError(aErrorCode);
	SetTestStepResult(aVerdict);	
	}

CSimpleHttpClient* CSimpleHttpClient::NewLC(CTSecureConnectionStep* aStep)
	{
	CSimpleHttpClient* me = new(ELeave) CSimpleHttpClient;
	CleanupStack::PushL(me);
	me->ConstructL(aStep);
	return me;
	}

CSimpleHttpClient* CSimpleHttpClient::NewL(CTSecureConnectionStep* aStep)
	{
	CSimpleHttpClient* me = NewLC(aStep);
	CleanupStack::Pop(me);
	return me;
	}
	
CSimpleHttpClient::~CSimpleHttpClient()
	{
	iSess.Close();
	delete iTransObs;
	}
	
void CSimpleHttpClient::ConstructL(CTSecureConnectionStep* aStep)
	{
	// Open the RHTTPSession
	iSess.OpenL();	
	iTransObs = CHttpEventHandler::NewL(aStep);
	iTestStep=aStep;
	}

void CSimpleHttpClient::StartClientL(TDesC& aUrl)
	{
	RStringPool strP = iSess.StringPool();
	RStringF method;
	method = strP.StringF(HTTP::EGET,RHTTPSession::GetTable());
	
	// Start the method off
	TBuf8<256> url8;
	url8.Copy(aUrl);
	InvokeHttpMethodL(url8, method);
	}

void CSimpleHttpClient::InvokeHttpMethodL(const TDesC8& aUri, RStringF aMethod)
	{
	TUriParser8 uri; 
	uri.Parse(aUri);
	iTrans = iSess.OpenTransactionL(uri, *iTransObs, aMethod);
	RHTTPHeaders hdr = iTrans.Request().GetHeaderCollection();

	SetHeaderL(hdr, HTTP::EUserAgent, KUserAgent);
	SetHeaderL(hdr, HTTP::EAccept, KAccept);
	iTrans.SubmitL();

	CActiveScheduler::Start();
	}

void CSimpleHttpClient::SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue)
	{
	RStringF valStr = iSess.StringPool().OpenFStringL(aHdrValue);
	THTTPHdrVal val(valStr);
	aHeaders.SetFieldL(iSess.StringPool().StringF(aHdrField,RHTTPSession::GetTable()), val);
	valStr.Close();
	}

CHttpEventHandler* CHttpEventHandler::NewLC(CTSecureConnectionStep* aStep)
	{
	CHttpEventHandler* me = new(ELeave)CHttpEventHandler();
	CleanupStack::PushL(me);
	me->ConstructL(aStep);
	return me;
	}

CHttpEventHandler* CHttpEventHandler::NewL(CTSecureConnectionStep* aStep)
	{
	CHttpEventHandler* me = NewLC(aStep);
	CleanupStack::Pop(me);
	return me;
	}
void CHttpEventHandler::ConstructL(CTSecureConnectionStep* aStep)
	{	
	iTestStep=aStep;
	}
	
void CHttpEventHandler::MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent)
	{
	switch (aEvent.iStatus)
		{
		case THTTPEvent::EGotResponseHeaders:
			{
			} break;
		case THTTPEvent::EGotResponseBodyData:
			{
			iRespBody = aTransaction.Response().Body();
			iRespBody->ReleaseData();
			} break;
		case THTTPEvent::EResponseComplete:
			{
			} break;
		case THTTPEvent::ESucceeded:
			{
			aTransaction.Close();
			CActiveScheduler::Stop();
			} break;
		case THTTPEvent::EFailed:
			{
			aTransaction.Close();
			CActiveScheduler::Stop();
			} break;
		case THTTPEvent::ERedirectedPermanently:
			{
			} break;
		case THTTPEvent::ERedirectedTemporarily:
			{
			} break;
		case THTTPEvent::ERedirectRequiresConfirmation:
 			{
 			aTransaction.Close();
 			CActiveScheduler::Stop();
 			} break;
		default:
			{
			if (aEvent.iStatus < 0)
				{
				iTestStep->ReceiveTestResult(aEvent.iStatus, EFail);
				aTransaction.Close();
				CActiveScheduler::Stop();
				}
			} break;
		}
	}

TInt CHttpEventHandler::MHFRunError(TInt /*aError*/, RHTTPTransaction /*aTransaction*/, const THTTPEvent& /*aEvent*/)
	{
	return KErrNone;
	}


