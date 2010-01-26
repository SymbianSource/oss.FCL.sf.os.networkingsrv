// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contain the implementation of the class for this null agent test
// 
//

#include "NullAgentTestSteps.h"
#include "dummynifvar.h"
#include "in_sock.h"

CTestStepNullAgtLoopbackTest::CTestStepNullAgtLoopbackTest(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtLoopbackTest::doTestStepL(void)
{
	__UHEAP_MARK;

	TInt r;                // the result of various operations
	TRequestStatus status; // status of asynchronous ops

	RSocketServ server;    // connection paraphanelia
	RConnection connection;
	RSocket socket;

	TInetAddr dest;
	dest.SetAddress(KDummyNifLocalAddressBase + 4);
	dest.SetPort(KPortNo);

	TBuf8<KBufferLength> buffer;

	// connect to the socket server
	r = server.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server);
	
	// this is why we needed a socket server...
	r = connection.Open(server, KAfInet);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(connection);
	
	// start the connection up (outgoing)
	connection.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	// open a udp socket
	r = socket.Open(server, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(socket);
	TESTL(socket.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	// set the source port number - otherwise will panic cos it's zero
	r = socket.SetLocalPort(KPortNo);
	TESTEL(r == KErrNone, r);
		
	// build some data to send on the socket
	// this is an ICMP ping request apparently
	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) 0x8;		// ICMP type = 8
	buffer[1] = (TUint8) 0x0;		// ICMP code = 0
	buffer[2] = (TUint8) 0xF7;		// ICMP checksum high byte
	buffer[3] = (TUint8) 0xFF;		// ICMP checksum low byte
	// NB the rest of the buffer is zero
	// hence the checksum (0xFFFF - 0x800) since 0x8
	// is the only non-zero element of the buffer

	// send the data out over the socket
	socket.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	buffer.Zero();
	// I expect to get the data looped back from the dummy NIF
	socket.RecvFrom(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	// check that what we sent is what we got back
	if (status.Int() == KErrNone)
	{
		// if the receive times out and we access buffer we get a panic
		TEST(buffer[0] == 0x08);
		TEST(buffer[1] == 0x00);
		TEST(buffer[2] == 0xF7);
		TEST(buffer[3] == 0xFF);
	}
	
	// close the socket
	socket.Shutdown(RSocket::ENormal, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	CleanupStack::Pop();

	// force the destruction of the connection
	r = connection.Stop();
	TESTEL(r == KErrNone, r);
	CleanupStack::Pop();

	// close the socket server
	server.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
}
