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
// This contains CTestCase which is the base class for all the TestCase DLLs
// 
//

// EPOC includes
#include <e32base.h>
#include <commdb.h>
#include <f32file.h>
#include <es_sock.h>

#include <securesocketinterface.h>
#include <securesocket.h>

// Test system includes
#include <networking/log.h>
#include <networking/teststep.h>
#include <networking/testsuite.h>
#include "TeststepTls.h"


class TLog16Overflow :public TDes16Overflow
	{
public:
	// TDes16Overflow pure virtual
	virtual void Overflow(TDes16& aDes);
	};

CTestStepTls::CTestStepTls() : iTestType( TLS_TEST_NORMAL )
/**
 * Constructor
 */
{
}

CTestStepTls::~CTestStepTls()
/**
 * Destructor
 */
{
}

void TLog16Overflow::Overflow(TDes16& /*aDes*/)
/** 
 * This function is called if the format text overflows the internal buffer.
 */
{
	User::Panic(_L("Log output buffer overflow"),1);
}

TPtrC CTestStepTls::EpocErrorToText(TInt aError)
{
	switch (aError)
		{
	case KErrSSLNoSharedCipher:
		return _L("KErrSSLNoSharedCipher");
	case KErrSSLSocketBusy			:
		return _L("KErrSSLSocketBusy");
	case KErrSSLInvalidCipherSuite:
		return _L("KErrSSLInvalidCipherSuite");
	case KErrSSLInvalidCert:
		return _L("KErrSSLInvalidCert");
	case KErrSSLNoClientCert:
		return _L("KErrSSLNoClientCert");
	case KErrSSLUnsupportedKeySize:
		return _L("KErrSSLUnsupportedKeySize");
	case KErrSSLUnsupportedKey:
		return _L("KErrSSLUnsupportedKey");
	case KErrSSLBadRecordHeader:
		return _L("KErrSSLBadRecordHeader");
	case KErrSSLBadProtocolVersion:
		return _L("KErrSSLBadProtocolVersion");
	case KErrSSL2ServerOnly:
		return _L("KErrSSL2ServerOnly");
	case KErrSSLUnexpectedMessage:
		return _L("KErrSSLUnexpectedMessage");
	case KErrSSLUnsupportedCipher:
		return _L("KErrSSLUnsupportedCipher");
	case KErrSSLBadMAC:
		return _L("KErrSSLBadMAC");
//	case KErrSSLReceivedAlert:
//		return _L("KErrSSLReceivedAlert");
	case KErrSSLRecvNotSupportedHS:
		return _L("KErrSSLRecvNotSupportedHS");
	case KErrSSLHSRecordFieldTooBig:
		return _L("KErrSSLHSRecordFieldTooBig");
	case KErrSSLRecordHeaderTooBig:
		return _L("KErrSSLRecordHeaderTooBig");
//	case KErrSSLSendDataTooBig:
//		return _L("KErrSSLSendDataTooBig");
	case KErrSSLNoCertificate:
		return _L("KErrSSLNoCertificate");
	case KErrSSLInvalidHash:
		return _L("KErrSSLInvalidHash");
	case KErrSSLSendCanceled:
		return _L("KErrSSLSendCanceled");
	case KErrSSLRecvCanceled:
		return _L("KErrSSLRecvCanceled");
	case KErrSSLHandshakeCanceled:
		return _L("KErrSSLHandshakeCanceled");
	case KErrSSLWriteFailed:
		return _L("KErrSSLWriteFailed");
	case KErrSSLFailedToLoad:
		return _L("KErrSSLFailedToLoad");
	case KErrSSLDisconnectIndication	:
		return _L("KErrSSLDisconnectIndication");
	case KErrSSLDllLeave:
		return _L("KErrSSLDllLeave");
	case KErrSSLAlertCloseNotify				:
		return _L("KErrSSLAlertCloseNotify");
	case KErrSSLAlertUnexpectedMessage:
		return _L("KErrSSLAlertUnexpectedMessage");
//	case KErrSSLAlertBadRecordMac:
//		return _L("KErrSSLAlertBadRecordMac");
	case KErrSSLAlertDecryptionFailed:
		return _L("KErrSSLAlertDecryptionFailed");
	case KErrSSLAlertRecordOverflow:
		return _L("KErrSSLAlertRecordOverflow");
	case KErrSSLAlertDecompressionFailure:
		return _L("KErrSSLAlertDecompressionFailure");
	case KErrSSLAlertHandshakeFailure:
		return _L("KErrSSLAlertHandshakeFailure");
	case KErrSSLAlertNoCertificate:
		return _L("KErrSSLAlertNoCertificate");
	case KErrSSLAlertBadCertificate:
		return _L("KErrSSLAlertBadCertificate");
	case KErrSSLAlertUnsupportedCertificate:
		return _L("KErrSSLAlertUnsupportedCertificate");
	case KErrSSLAlertCertificateRevoked:
		return _L("KErrSSLAlertCertificateRevoked");
	case KErrSSLAlertCertificateExpired:
		return _L("KErrSSLAlertCertificateExpired");
	case KErrSSLAlertCertificateUnknown:
		return _L("KErrSSLAlertCertificateUnknown");
	case KErrSSLAlertIllegalParameter:
		return _L("KErrSSLAlertIllegalParameter");
	case KErrSSLAlertUnknownCA:
		return _L("KErrSSLAlertUnknownCA");
	case KErrSSLAlertAccessDenied:
		return _L("KErrSSLAlertAccessDenied");
	case KErrSSLAlertDecodeError:
		return _L("KErrSSLAlertDecodeError");
	case KErrSSLAlertDecryptError:
		return _L("KErrSSLAlertDecryptError");
	case KErrSSLAlertExportRestriction:
		return _L("KErrSSLAlertExportRestriction");
	case KErrSSLAlertProtocolVersion:
		return _L("KErrSSLAlertProtocolVersion");
	case KErrSSLAlertInsufficientSecurity:
		return _L("KErrSSLAlertInsufficientSecurity");
	case KErrSSLAlertInternalError:
		return _L("KErrSSLAlertInternalError");
	case KErrSSLAlertUserCanceled:
		return _L("KErrSSLAlertUserCanceled");
	case KErrSSLAlertNoRenegotiation:
		return _L("KErrSSLAlertNoRenegotiation");
	default:
		return CTestStep::EpocErrorToText( aError );
	}
}


