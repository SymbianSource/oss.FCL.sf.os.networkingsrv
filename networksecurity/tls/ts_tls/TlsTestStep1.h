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
// TlsTstStep1.h
// This is the header file for Tls test 01
// 
//


#if (!defined __STEP_001_01_H__)
#define __STEP_001_01_H__

class CTestStepT_Tls : public CTestStepTls
{
public:
	CTestStepT_Tls();
	~CTestStepT_Tls();

	virtual enum TVerdict doTestStepL( void );
	void TLSTestL();

};

class CTlsRenegotiateTest : public CTestStepT_Tls
{
public:
	CTlsRenegotiateTest();
	virtual enum TVerdict doTestStepL( void );
};

class CTlsCancelRecvTest : public CTestStepT_Tls
{
public:
	CTlsCancelRecvTest();
	virtual enum TVerdict doTestStepL( void );
};

class CTlsOpenConnection : public CTestStepT_Tls
{
public:
	CTlsOpenConnection();
	virtual enum TVerdict doTestStepL( void );

	TBuf<128>	iAddress;
	TInt		iPortNum;
	TInetAddr	iInetAddr;

};

class CTlsCloseConnection : public CTestStepT_Tls
{
public:
	CTlsCloseConnection();
	virtual enum TVerdict doTestStepL( void );
};

class CTlsFailSuiteSelection : public CTestStepT_Tls
{
public:
	CTlsFailSuiteSelection();
	virtual enum TVerdict doTestStepL( void );

	TBuf<128>				iAddress;
	TInt					iPortNum;
	TInetAddr				iInetAddr;
	TBuf8<KCipherBufSize>	iCipherSuites;
private:
	CSecureSocket*			iSecureSocket;
};

class CTlsOldGetOptsTest : public CTestStepT_Tls
{
public:
	CTlsOldGetOptsTest();
	virtual enum TVerdict doTestStepL( void );
};


class CTestStepDialogMode_Tls : public CTestStepTls
{
public:
	CTestStepDialogMode_Tls();
	virtual enum TVerdict doTestStepL( void );

};


#endif //(__STEP_001_01_H__)
