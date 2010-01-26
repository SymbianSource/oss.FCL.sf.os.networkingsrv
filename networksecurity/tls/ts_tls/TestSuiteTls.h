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
// This contains CTestSuiteTls 
// This is the container class for all the TLS test steps
// 
//

#if (!defined __TESTSUITETLS_H__)
#define __TESTSUITETLS_H__

#include <networking/testsuite.h>


class  CTestSuiteTls : public CTestSuite
{
public:
	void InitialiseL( void );
	~CTestSuiteTls();
	TPtrC GetVersion( void );
	void AddTestStepL( CTestStepTls * ptrTestStep );

	RSocketServ iSocketServer;
	RSocket		iSocket;
	CSecureSocket* iSecureSocket;	 
};


#endif /* __TESTSUITETLS_H__ */
