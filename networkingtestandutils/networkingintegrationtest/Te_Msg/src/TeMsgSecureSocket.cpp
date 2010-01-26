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
 @file TeMsgSecureSocket.cpp
*/

#include "TeMsgSecureSocket.h"
#include "TeMsgStep.h"
#include <securesocket.h>

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif


_LIT(KSSLProtocol,			"tls1.0");

CTestSecureSocket* CTestSecureSocket::NewL(CTestStep& aTestStep, RSocket& aSocket, const TDesC8& aDnsName)
	{
	CTestSecureSocket*	self=new(ELeave) CTestSecureSocket(aTestStep, aSocket);
	CleanupStack::PushL(self);
	self->ConstructL(aDnsName);
	CleanupStack::Pop();
	return self;
	}

CTestSecureSocket::CTestSecureSocket(CTestStep& aTestStep, RSocket& aSocket)
: CActive(EPriorityStandard)
, iState(EStartClientHandshake)
, iTestStep(aTestStep)
, iSocket(aSocket)
, iDnsName(NULL)
	{
	}

CTestSecureSocket::~CTestSecureSocket()
	{
#ifdef NODIALOGS
	iSemaphore.Close();
#endif

	delete iDnsName;
	iDnsName=NULL;
	}

void CTestSecureSocket::ConstructL(const TDesC8& aDnsName)
	{
#ifdef NODIALOGS
	// create a global semaphore that the dialog server searches for to
	// decide if "trust" dialogs should be displayed or not
	TInt err = iSemaphore.CreateGlobal( KSemaphoreName, 0 );
	if ( err != KErrNone )
		{
		iTestStep.ERR_PRINTF1(_L("Semaphore creation failed."));
		iTestStep.SetTestStepResult(EFail);
		User::Leave(err);
		}
#endif

	iDnsName = aDnsName.AllocL();
	iSecureSocket = CSecureSocket::NewL(iSocket, KSSLProtocol);

	TBuf8<2> cipherBuf;
	cipherBuf.SetMax();

	cipherBuf[0] = (TUint8) 0x00;
	cipherBuf[1] = (TUint8) 0x03;

	// Set cipher suites
	iSecureSocket->SetAvailableCipherSuites( cipherBuf );

	// Set Option
	iSecureSocket->SetOpt(KSoSSLDomainName,KSolInetSSL, iDnsName->Des());

	CActiveScheduler::Add(this);
	}

void CTestSecureSocket::StartHandshake()
	{
	iTotalBytesRead = 0;

	// Start client handshake
	iSecureSocket->StartClientHandshake(iStatus);
	iState=EStartClientHandshake;
	SetActive();
	}

void CTestSecureSocket::DoCancel()
	{
	}

void CTestSecureSocket::RunL()
	{
	switch ( iState )
		{
	case EStartClientHandshake:
		{
		TInt result = iStatus.Int();

		if (result != KErrNone)
			{
			// Set the test result EFail if starting client handshake has failed
			iTestStep.INFO_PRINTF2(_L("StartHandShaking fails with error %d"), result);
			iTestStep.SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}

		THTTPMessage *iMyHTTP = new (ELeave) THTTPMessage;
		CleanupStack::PushL(iMyHTTP);

		// Form a HTTP request packet
		iMyHTTP->Method(_L8("GET"));
		TBuf8<50> page;

		page.Copy(_L("/index.html"));
		iMyHTTP->URI(page);
		iMyHTTP->AddHeaderField(_L8("Connection"),_L8("close"));
		iMyHTTP->AddHeaderField(_L8("User-Agent"),_L8("SSL_TEST"));
		iMyHTTP->AddHeaderField(_L8("Accept-Encoding"));
		iMyHTTP->AddHeaderField(_L8("Accept"),_L8("*/*"));
		iMyHTTP->GetHeader(iBuffer);
		CleanupStack::PopAndDestroy(iMyHTTP);

		// Send the HTTP request
		iSecureSocket->Send(iBuffer, iStatus);

		iState = ESentData;
		SetActive();
		break;
		}
	case ESentData:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			// Set the test result EFail if sending request has failed
			iTestStep.INFO_PRINTF2(_L("Sending data via secure socket fails with error %d"), result);
			iTestStep.SetTestStepResult(EFail);
			CActiveScheduler::Stop();
			break;
			}
		else
			{
			// Set the test result EPass if sending the request has passed
			iTestStep.INFO_PRINTF2(_L("Send data via secure socket succeeds with %d"), result);
			iTestStep.SetTestStepResult(EPass);
			iBuffer.Zero();
			// Wait for receiving the data
			iSecureSocket->Recv(iBuffer, iStatus);
			iState = ERecvData;
			SetActive();
			break;
			}
		}
	case ERecvData:
		{
		TInt result = iStatus.Int();
		if (result != KErrNone)
			{
			if ( iStatus!=KErrEof )
				{
				// Set the test result EFail if receiving has failed
				iTestStep.INFO_PRINTF2(_L("Receiving data via secure socket fails with error %d"), result);
				iTestStep.SetTestStepResult(EFail);
				CActiveScheduler::Stop();
				break;
				}
			else
				{
				if (iBuffer.Length())
					{
					// Set the test result EPass if receiving has succeeded and reaches the end of file
					iTestStep.INFO_PRINTF2(_L("Receiving the end of data via secure socket succeeds with %d"), result);
					iTestStep.SetTestStepResult(EPass);
					CActiveScheduler::Stop();
					break;
					}
				}
			}

		// Continue reaching till it receives the end of file
		iSecureSocket->Recv(iBuffer, iStatus);
		iState = ERecvData;
		SetActive();
		break;
		}
		}
	}





