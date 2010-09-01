// Copyright (c) 2003-2010 Nokia Corporation and/or its subsidiary(-ies).
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
 
#ifndef _T_TLS_CNTRL_H
#define _T_TLS_CNTRL_H

// Accept untrusted certificates without showing a dialog
#define NODIALOGS


#include <e32base.h>
#include <e32cons.h>
#include <f32file.h>
#include <charconv.h>
#include <networking/log.h>
#include <networking/teststep.h>

#include "T_TLS_test.h"
#include "T_Autossl_Const.h"

#include <securesocket.h>


enum TTestSiteParameters
	{
	EIPAddress,			// IP address of site (or www.site.com name)
	EDNSName,			// DNS name of site (checked against names from certificate)
	EPage,				// Page to attempt to retrieve
	EIPPort,			// IP port number
	EMaxTLSVers,		// Max version of TLS client should support
	EMinTLSVers,		// Min version of TLS client should support
	ECipher,			// Cipher that *should* be selected by server from range
	ECipherSuites,		// The range of cipher suites that can be used
	ESimpleGet,			// Use a simple get or full http get request for the page
	ETestEndDelay,		// Number of seconds to delay after test has completed
	ENumSiteParams		// ###Must be last one, used to calc the size of the array needed###
	};

_LIT( KSemaphoreName, "T_AUTOSSL" );

class CController : public CActive
	{
private:
	enum TControllerStates
	{
	EReadNextSite,		// Read a new site description from the file
	EFindFreeTest,		// Find the first free test object and start it off
	EWaitForComplete,	// No more sites to test, wait until all tests have completed
	ETestCompleted		// All tests completed
	};

public:
	// Construct/destruct
	static CController *NewL();
	
	~CController();

	// Start the controller
	void Start( CTestStepTls * aTestStep );
	
private:
	// Construction
	CController();	 
	void ConstructL();

	// Methods from CActive
	void RunL();
	void DoCancel();

	CTestStepTls * iTestStep;
	RTimer	iTimer;
	TInt	iRunState;
	CTLSTest* iTLSTest[ KMaxSSLConnections ];

	TBuf<128>	iAddress;
	TBuf8<256>	iDNSName;
	TInt		iPortNum;
	TBuf<128>	iPage;
	TBuf8<64>	iCipherSuites;
	TInt		iCipher;
	TInt		iSimpleGet;
	TInt		iTestEndDelay;
	TBuf<32>	iProtocol;
	TBool		iUseGenericSocket;
	TBool		iEAPKeyDerivation;
#ifdef HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	TBool      iTLSDialogMode;
	TInt       iTLSDialogModeValue;
	TInt       iExpectedErrorCode;       
#endif  // HTTP_ALLOW_UNTRUSTED_CERTIFICATES
	RSemaphore	iSemaphore;	

	};

#endif
