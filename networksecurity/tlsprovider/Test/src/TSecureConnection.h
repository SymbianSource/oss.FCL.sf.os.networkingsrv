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

#ifndef __SECURECONNECTION_H__
#define __SECURECONNECTION_H__

#include <test/testexecutestepbase.h>
#include <securesocket.h>
#include <secdlgimpldefs.h>
#include <http.h>

//_LIT( KSecureConnection, "SecureConnection" );
_LIT( KUrl, "url" );
_LIT( KServerAuthenticationResponse, "SAResponse" );
_LIT( KClientAuthenticationResponse1, "CAResponse1" );
_LIT( KClientAuthenticationResponse2, "CAResponse2" );
_LIT( KKeyStorePassPhrase, "KeyStorePassPhrase" );

/*
//Diglog input data
_LIT( KServerAuthenticationDlg, "ServerAuthDlg");
_LIT( KSADescription, "Server Authentication Dialog");
_LIT( KCTSecureConnectionStepDlg, "ClientAuthDlg");
_LIT( KCADescription, "Client Authentication Dialog");
_LIT( KKeyStorePassPhrase, "KeyStorePassPhrase");
_LIT( KDlgResponse1, "Response1");
_LIT( KDlgResponse2, "Response2");
*/
// t_secdlg filenames
_LIT(KTSecDlgInputFile, "\\t_secdlg_in.dat");
_LIT(KTSecDlgOutputFile, "\\t_secdlg_out.dat");


class CCommunicate;
class CSimpleHttpClient;
class CTSecureConnectionStep : public CTestStep
{
public:
	CTSecureConnectionStep(const TDesC& aStepName);
	~CTSecureConnectionStep();
	TVerdict doTestStepPreambleL();
	TVerdict doTestStepL();
	TVerdict doTestStepPostambleL();
	void ReceiveTestResult(const TInt& aErrorCode, TVerdict aVerdict);
	
private:
	void WriteDialogRecordL(RFileWriteStream& aStream, 
							TSecurityDialogOperation aOp, 
							const TDesC& aLabelSpec,
							const TDesC& aResponse1, 
							const TDesC& aResponse2);
	static void InitCommsL();
	void ConvertDlgConfigL();
private:
	CSimpleHttpClient* iHttp;
	RFs iFs;
	CActiveScheduler* iScheduler;
};

class CHttpEventHandler : public CBase, public MHTTPTransactionCallback
	{
public:
	static CHttpEventHandler* NewL(CTSecureConnectionStep* aStep);
	static CHttpEventHandler* NewLC(CTSecureConnectionStep* aStep);
	
	// methods from MHTTPTransactionCallback
	virtual void MHFRunL(RHTTPTransaction aTransaction, const THTTPEvent& aEvent);
	virtual TInt MHFRunError(TInt aError, RHTTPTransaction aTransaction, const THTTPEvent& aEvent);
private:
	void ConstructL(CTSecureConnectionStep* aStep);
	MHTTPDataSupplier* iRespBody;
	CTSecureConnectionStep* iTestStep;	
	};
	
class CSimpleHttpClient : public CBase
	{
public:
	static CSimpleHttpClient* NewL(CTSecureConnectionStep* aStep);
	static CSimpleHttpClient* NewLC(CTSecureConnectionStep* aStep);
	virtual ~CSimpleHttpClient();
	void StartClientL(TDesC& aUrl);
protected:
	void ConstructL(CTSecureConnectionStep* aStep);
private:
	void InvokeHttpMethodL(const TDesC8& aUri, RStringF aMethod);
	void SetHeaderL(RHTTPHeaders aHeaders, TInt aHdrField, const TDesC8& aHdrValue);
private:
	RHTTPSession iSess;
	RHTTPTransaction iTrans;
	CHttpEventHandler* iTransObs;
	CTSecureConnectionStep* iTestStep;
	};
	
#endif



