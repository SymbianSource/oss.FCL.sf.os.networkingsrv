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
//

/**
 @file TestSteps.cpp
*/


#include <e32math.h>
#include <c32comm.h>
#include <in_sock.h>


#include <test/testexecutelog.h>
#include "TestSteps.h"
#include "te_pppsize_server.h"

//
// Construction/Destruction
//

CPPPMinMaxMMU::CPPPMinMaxMMU()
	{
	SetTestStepName(KPPPMinMaxMMU);
	}

CPPPMinMaxMMU::~CPPPMinMaxMMU()
	{ }

void CPPPMinMaxMMU::CommInitL(TBool aEnhanced)
	{
    TInt err;
    TPtrC pdd_name, ldd_name,ldd_fname;
	INFO_PRINTF1(_L("Loading Comms driver"));
#if defined (__WINS__)
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("WinsPDDname"), pdd_name));
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("WinsLDDname"), ldd_name));
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("WinsLDDFname"), ldd_fname));
#else
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("ThumbPDDname"), pdd_name));
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("ThumbLDDname"), ldd_name));
	TEST(GetStringFromConfig(_L("CommsDrivers"), _L("ThumbLDDFname"), ldd_fname));
#endif
    err=User::LoadPhysicalDevice(pdd_name);
    if (err!=KErrNone && err!=KErrAlreadyExists) 
		{
		User::Leave(err);
		}

    if (aEnhanced) 
		{
		err=User::LoadLogicalDevice(ldd_fname);
		}
    else 
		{
		err=User::LoadLogicalDevice(ldd_name);
		}
    if (err!=KErrNone && err!=KErrAlreadyExists)
		{
		User::Leave(err);
		}
	}

TVerdict CPPPMinMaxMMU::doTestStepL()
	{
	const TInt KMaxMMU = 4000;
	const TInt KMinMMU = 1;

	TBuf8<KMaxMMU> *sendBuf = new(ELeave) TBuf8<KMaxMMU>();
	CleanupStack::PushL(sendBuf);
	
	TBuf8<KMaxMMU> *recvBuf = new(ELeave) TBuf8<KMaxMMU>();
	CleanupStack::PushL(recvBuf);
	
	TBuf8<KMaxMMU> *recvBuf2 = new(ELeave) TBuf8<KMaxMMU>();
	CleanupStack::PushL(recvBuf2);

    //initialize COMM
    //CommInitL(EFalse);
    
	//Start Comms server
// 	_LIT(KPhbkSyncCMI, "phbsync.cmi");
//    (void)StartC32WithCMISuppressions(KPhbkSyncCMI);
	
	SetTestStepResult(EPass);
    INFO_PRINTF1(_L("Starting: Socket Server\n"));
    
    RSocketServ ss;

    // Start the socket server
    TEST(KErrNone == ss.Connect());
    RSocket sock;
    TRequestStatus sta;
	INFO_PRINTF1(_L("Open a socket\n"));
    
    // Get the test address
    TInetAddr RemAddr(7);
    TPtrC testAddr;
	TEST(GetStringFromConfig(_L("AddressInfo"), _L("TestPPPIPAddr"), testAddr));
	
    if(testAddr.Length())
		{
		RemAddr.Input(testAddr); 
		}
    else
		{
		INFO_PRINTF1(_L("Test FAILED\nMissing address information in config file: "));
		return EFail;
		}
    
    // Open a socket
    TEST(KErrNone == sock.Open(ss, KAfInet, KSockStream, KProtocolInetTcp)); 
    INFO_PRINTF1(_L("Connecting Socket to:"));
	
    TBuf<30> printAddr;
    RemAddr.Output(printAddr);
	
    INFO_PRINTF1(printAddr);
	
    // Connect a socket
    sock.Connect(RemAddr,sta);
	
    // Wait for Connect to complete
    User::WaitForRequest(sta);
    TEST(sta.Int() == 0);
    
    TInt iterEnd;
	GetIntFromConfig(_L("MMUInfo"), _L("TestPPPmaxMMU"), iterEnd);
    if (iterEnd > KMaxMMU)
		iterEnd = KMaxMMU;
	
    TInt iterStart;
	GetIntFromConfig(_L("MMUInfo"), _L("TestPPPminMMU"), iterStart);
    if (iterStart < KMinMMU)
		iterStart = KMinMMU;
	
    TInt i,j;
    TSockXfrLength recvLen;
    INFO_PRINTF1(_L("Send/Recv frames"));
    for(j=iterStart;j<=iterEnd;j++)
		{
		sendBuf->Zero();
		
		for (i=0;i<j;i++)
			sendBuf->Append(Math::Random() & 0x7f);
		
		INFO_PRINTF2(_L("Sending Packet of Size: %d"),j);
		// Send data to echo port
		sock.Write(*sendBuf,sta);
		User::WaitForRequest(sta);
		TEST(sta.Int() == 0);
		i=0;
		recvBuf->Zero();
		
		while(i<j)
			{
			// Receive data from echo port
			sock.RecvOneOrMore(*recvBuf2,0,sta,recvLen); 
			User::WaitForRequest(sta);
			TEST(sta.Int() == 0);
			i += recvBuf2->Length();
			recvBuf->Append(*recvBuf2);
			}
		
		TEST(KErrNone == recvBuf->Compare(*sendBuf));
		INFO_PRINTF1(_L("   Received echoed Packet"));
		}
    
    sock.Shutdown(RSocket::EStopOutput,sta);
    User::WaitForRequest(sta);
    TEST(sta.Int() == 0);
    sock.Close();
    ss.Close();

	CleanupStack::PopAndDestroy(3);

	return TestStepResult();
	}

