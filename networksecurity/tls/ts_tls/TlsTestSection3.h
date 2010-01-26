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
// Tls Test section 3
// This is the header file for the Tls test section 3
// 
//


#if !defined(__TLS_TEST_STEP3_H__)
#define __TLS_TEST_STEP3_H__

class CTlsTestSection3_1 : public CTestStepTls
{
public:
	CTlsTestSection3_1();
	~CTlsTestSection3_1();

	virtual enum TVerdict doTestStepL( void );
};


#endif // __TLS_TEST_STEP3_H__
