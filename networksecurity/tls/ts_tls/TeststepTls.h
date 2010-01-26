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
// This contains CTestStepTls which is the base class for all 
// the Tls suite test cases
// 
//

#if (!defined __TLSTESTSTEP_H__)
#define __TLSTESTSTEP_H__

class CTestSuite;
class CTestSuiteTls;


class CTestStepTls : public CTestStep
{
public:

	friend class CController;
	friend class CTLSTest;

	enum TLS_TEST_TYPE
	{
		TLS_TEST_NORMAL = 0,
		TLS_TEST_RENEGOTIATE,
		TLS_TEST_CANCEL_RECV,
		TLS_TEST_OLD_GETOPTS,
	};	

	CTestStepTls();
	~CTestStepTls();

	// pointer to suite which owns this test 
	CTestSuiteTls * iTlsSuite;

	TPtrC EpocErrorToText(TInt aError);
protected:
	enum TLS_TEST_TYPE iTestType;
};


#endif /* __TLSTESTSTEP_H__ */
