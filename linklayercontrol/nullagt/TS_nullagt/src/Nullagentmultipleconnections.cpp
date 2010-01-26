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
#include "commdbconnpref.h"
#include "in_sock.h"

CTestStepNullAgtMultipleConnections::CTestStepNullAgtMultipleConnections(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtMultipleConnections::doTestStepL(void)
{	
	__UHEAP_MARK;

	TInt r;                // the result of various operations
	TRequestStatus status; // status of asynchronous ops

	// connection paraphanelia, two of each
	RSocketServ server1, server2;
	RConnection connection1, connection2;
	RSocket socket1, socket2;

	TInetAddr dest;
	dest.SetAddress(KDummyNifLocalAddressBase + 4);
	dest.SetPort(KPortNo);

	TBuf8<KBufferLength> buffer;

	// as ever we need a socket server to create connections...
	r = server1.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server1);
	// let's have two
	r = server2.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server2);
	
	// create the first (default connection using commdb settings)
	// Which according to connetion preferences, the one with ranking 1
	// is IAP 5
	r = connection1.Open(server1, KAfInet);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(connection1);

	// create the second connection (overrides this time)
	// on the same IAP, ie IAP5
	r = connection2.Open(server2, KAfInet);
	TESTEL(r == KErrNone, r);
	// Don't push this connection as it will be stopped by connection1

	// create the overrides
	TCommDbConnPref prefs;
	prefs.SetIapId(5);
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

	connection1.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	// start the connection with the overrides
	connection2.Start(prefs, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	// open a socket over the first connection
	r = socket1.Open(server1, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(socket1);
	TESTL(socket1.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	// set the source port number
	r = socket1.SetLocalPort(KPortNo);
	TESTEL(r == KErrNone, r);
		
	// open a socket over the second connection
	r = socket2.Open(server2, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(socket2);
	TESTL(socket2.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	// set the source port number - otherwise will panic cos it's zero
	r = socket2.SetLocalPort(KPortNo);
	TESTEL(r == KErrNone, r);

	// build some data to send on the socket
	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) 0x8;
	buffer[1] = (TUint8) 0x0;
	buffer[2] = (TUint8) 0xF7;
	buffer[3] = (TUint8) 0xFF;

	// send the data out the first socket
	socket1.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	buffer.Zero();
	// I expect to get the data looped back from the dummy NIF
	socket1.RecvFrom(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	if (status.Int() == KErrNone)
	{
		// if the receive times out and we access buffer we get a panic
		TESTL(buffer[0] == 0x08);
		TESTL(buffer[1] == 0x00);
		TESTL(buffer[2] == 0xF7);
		TESTL(buffer[3] == 0xFF);
	}

	// send the same data out over the second socket
	socket2.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	buffer.Zero();
	// again, I expect it to be looped back by the dummy NIF
	socket2.RecvFrom(buffer, dest, 0, status);
	User::WaitForRequest(status);

	if (status.Int() == KErrNone)
	{
		// if the receive times out and we access buffer we get a panic
		TESTL(buffer[0] == 0x08);
		TESTL(buffer[1] == 0x00);
		TESTL(buffer[2] == 0xF7);
		TESTL(buffer[3] == 0xFF);
	}

	// close down both of the sockets
	socket2.Shutdown(RSocket::ENormal, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	CleanupStack::Pop();
	socket1.Shutdown(RSocket::ENormal, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	CleanupStack::Pop();

	// Stop only connection1, as this points to the same IAP
	// as connection2
	r = connection1.Stop();
	TESTEL(r == KErrNone, r);
	CleanupStack::Pop(); 

	// close the socket servers
	server2.Close();
	CleanupStack::Pop();
	server1.Close();
	CleanupStack::Pop();

	__UHEAP_MARKEND;

	return iTestStepResult;
}	
