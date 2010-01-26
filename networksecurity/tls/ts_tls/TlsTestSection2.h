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
// Tls Test section 2
// This is the header file for the Tls test section 2
// 
//


#if (!defined __TEST_STEP2_H__)
#define __TEST_STEP2_H__

class CTlsTestSection2_1 : public CTestStepTls
{
public:
	CTlsTestSection2_1();
	~CTlsTestSection2_1();

	virtual enum TVerdict doTestStepL( void );

private:
	TBuf<KMaxFileName> iOldSslAdaptor;
	TBool iCommDbModified;

	void ModifyCommDbL(const TDesC& aProtocolName, TBool aBreak);
};

_LIT(KBadSslLibrary, "nonexistent.dll");

#endif //(__TEST_STEP2_H__)
