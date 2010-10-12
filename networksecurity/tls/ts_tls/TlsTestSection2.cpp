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
// This contains TLS test section 2
// 
//

// EPOC includes
#include <e32base.h>

// from t_tls.cpp
#include <e32cons.h>
#include <c32comm.h>
#include <f32file.h>
#include <es_sock.h>

#include <metadatabase.h>
#include <commsdattypesv1_1.h>
using namespace CommsDat;
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <commsdattypesv1_1_partner.h>
#endif
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <commsdattypeinfov1_1_internal.h>
#endif

#include <securesocketinterface.h>
#include <securesocket.h>

// Test system includes
#include <networking/log.h>
#include <networking/teststep.h>

#include "T_TLS_test.h"
#include "TestSuiteTls.h"
#include "TlsTestSection2.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <securesocket_internal.h>
#endif

CTlsTestSection2_1::CTlsTestSection2_1() :
	iCommDbModified(EFalse)
/** 
 * Constructor.
 * Store the name of this test case.
 */

{
	iTestStepName = _L("tls_TestSection2_1");
}


CTlsTestSection2_1::~CTlsTestSection2_1()
/**
 * Destructor
 */
{
}

void CTlsTestSection2_1::ModifyCommDbL(const TDesC& aProtocolName, TBool aBreak)
/**
 * @test Create a special CommDb Database for this test 
 */
    {
	if (aBreak)
		{
		Log(_L("Changing CommDb ssladaptor to non-existent one"));
		}
	else if (!iCommDbModified)
		{
		Log(_L("Attempt to Changing CommDb ssladaptor back to original when not changed"));
		User::Leave(KErrGeneral);
		}
	else
		{
		Log(_L("Changing CommDb ssladaptor back to original"));
		}

	// Create new session to CommsDat
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_2);
#else
	CMDBSession* session = CMDBSession::NewL(KCDVersion1_2);
#endif
	CleanupStack::PushL(session);
	CCDSecureSocketRecord* ptrRecord = static_cast<CCDSecureSocketRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdSSProtoRecord));
	CleanupStack::PushL(ptrRecord);
	ptrRecord->iSSProtoName = aProtocolName;
	User::LeaveIfError(ptrRecord->FindL(*session));

	if (aBreak)
		{
		// need to read existing value before updating value
		iOldSslAdaptor = ptrRecord->iSSProtoLibrary;

		ptrRecord->iSSProtoLibrary = KBadSslLibrary;	
		}
	else
		{
		ptrRecord->iSSProtoLibrary = iOldSslAdaptor;
		}

	ptrRecord->ModifyL(*session);
	iCommDbModified = aBreak;

	CleanupStack::PopAndDestroy(ptrRecord);	
	CleanupStack::PopAndDestroy(session);
	}

// This is the test code for testing use of commdb-specified adaptor
// The script has loaded a commdb with an invalid ProtoLibrary in the 
// SecureSocketTable. So when we attempt to open a secure socket we 
// should get KErrNotFound
TVerdict CTlsTestSection2_1::doTestStepL( )
{
	Log(_L("Starting Bad SSL Adaptor test"));

	// if the test has not left yet it must be a Pass 
	iTestStepResult = EPass;

	__UHEAP_MARK;

	// Create an active scheduler
	CActiveScheduler* myActiveScheduler = new(ELeave) CActiveScheduler();
	CleanupStack::PushL( myActiveScheduler );
	CActiveScheduler::Install( myActiveScheduler );

	// Connect the socket server
	RSocketServ socketServ;
	TInt error = socketServ.Connect();
	TESTEL(error == KErrNone, error);
	CleanupClosePushL(socketServ);

	// Open a socket
	RSocket socket;
	error = socket.Open( socketServ, KAfInet, KSockStream, KProtocolInetTcp );
	TESTEL(error == KErrNone, error);
	CleanupClosePushL(socket);

	// now currupt the ssladaptor to force failure
	ModifyCommDbL(KDefCfgProtocol, ETrue);

	Log(_L("Creating a secure socket"));
	CSecureSocket* secureSocket = NULL;
	TRAP(error, secureSocket = CSecureSocket::NewL( socket, KDefCfgProtocol ) );
	TPtrC errorText = EpocErrorToText(error);
	Log(_L("CSecureSocket::NewL returned %S"), &errorText);
	TESTE(KErrNotFound == error, error);
	if (KErrNone == error)
		{
		secureSocket->Close();
		delete secureSocket;
		}

	// reset the ssladaptor
	ModifyCommDbL(KDefCfgProtocol, EFalse);

	CleanupStack::PopAndDestroy(); // socket
	CleanupStack::PopAndDestroy(); // socketServ

	CActiveScheduler::Install( NULL );
	CleanupStack::PopAndDestroy( myActiveScheduler );

	// unload the securesocket library
	CSecureSocketLibraryLoader::Unload();
	
	__UHEAP_MARKEND;

	return iTestStepResult;
}
