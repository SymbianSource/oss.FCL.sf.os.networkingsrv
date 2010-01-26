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

CTestStepNullAgtNotifications::CTestStepNullAgtNotifications(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtNotifications::doTestStepL(void)
{
	__UHEAP_MARK;

	TRequestStatus status; // status of asynchronous ops
	TInt r;                // the result of various operations

	RSocketServ server;    // connection paraphanelia
	RConnection connection;
	RSocket socket;

	TInetAddr dest;
	dest.SetAddress(KDummyNifLocalAddressBase + 4);
	dest.SetPort(KDummyNifCmdPort);

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

	// to send commands to the dummy nif we need to send pseudo-traffic which 
	// it will interpret as a command... hence all the socket stuff that follows

	// open a udp socket
	r = socket.Open(server, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(socket);
	TESTL(socket.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	// set the source port number - otherwise will panic cos it's zero
	r = socket.SetLocalPort(KDummyNifCmdPort);
	TESTEL(r == KErrNone, r);

	// put the command we want into the udp payload...
	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) KSendNotification;

	// send the data out over the socket
	socket.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	buffer.Zero();
	// I expect to get the data looped back from the dummy NIF
	socket.RecvFrom(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	// check what we got back somehow
	TESTEL(buffer[1] != (unsigned char) KErrGeneral, buffer[1]);
	
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
