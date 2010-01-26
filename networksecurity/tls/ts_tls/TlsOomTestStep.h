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
// Tls Oom Test step
// This is the header file for the Tls Out of Memory test 01
// 
//


#if (!defined __TEST_OOM_H__)
#define __TEST_OOM_H__

class CTlsOomTest : public CTestStepTls
{
public:
	CTlsOomTest();
	~CTlsOomTest();

	virtual enum TVerdict doTestStepL( void );
};

class COomTestData 
{

public:
	TBuf<128>	iAddress;
	TInt		iOOMThreshold;
	TInt		iPortNumber;
   TInt     iRunStep;
};
#endif //(__TEST_OOM_H__)
