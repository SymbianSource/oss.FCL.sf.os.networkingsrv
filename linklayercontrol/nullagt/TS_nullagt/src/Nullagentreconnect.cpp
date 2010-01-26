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

CTestStepNullAgtReconnect::CTestStepNullAgtReconnect(TPtrC aName)
{
	iTestStepName=aName;
}

enum TVerdict CTestStepNullAgtReconnect::doTestStepL(void)
{	
	__UHEAP_MARK;

	TRequestStatus status; // status of asynchronous ops
	TInt r;                // the result of various operations
	
	RSocketServ server;    // connection paraphanelia
	RConnection connection;
	RSocket commandSocket, dataSocket;
	
	TInetAddr commandDest;
	commandDest.SetAddress(KDummyNifLocalAddressBase + 4);
	commandDest.SetPort(KDummyNifCmdPort);
	TInetAddr dataDest;
	dataDest.SetAddress(KDummyNifLocalAddressBase + 4);
	dataDest.SetPort(KPortNo);

	TBuf8<KBufferLength> buffer;

	/* START THE CONNECTION WE WILL USE */

	r = server.Connect();
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(server);

	r = connection.Open(server, KAfInet);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(connection);

	connection.Start(status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	/* OPEN 2 SOCKETS - ONE FOR COMMANDS AND ONE FOR TRAFFIC */

	// to send commands to the dummy nif we need to send pseudo-traffic which 
	// it will interpret as a command... hence all the socket stuff that follows

	// open a udp socket to send the command to the dummy nif
	r = commandSocket.Open(server, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(commandSocket);
	TESTL(commandSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	// set the source port number - otherwise will panic cos it's zero
	r = commandSocket.SetLocalPort(KDummyNifCmdPort);
	TESTEL(r == KErrNone, r);

	// open another socket to send data later on
	r = dataSocket.Open(server, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(r == KErrNone, r);
	CleanupClosePushL(dataSocket);
	TESTL(dataSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1)==KErrNone);
	r = dataSocket.SetLocalPort(KPortNo);
	TESTEL(r == KErrNone, r);

	/* SEND SOME LOOPBACK TRAFFIC */

	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) 0x8;		// ICMP type = 8
	buffer[1] = (TUint8) 0x0;		// ICMP code = 0
	buffer[2] = (TUint8) 0xF7;		// ICMP checksum high byte
	buffer[3] = (TUint8) 0xFF;		// ICMP checksum low byte

	dataSocket.SendTo(buffer, dataDest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	buffer.Zero();
  	dataSocket.RecvFrom(buffer, dataDest, 0, status);
  	User::WaitForRequest(status);
  	TESTEL(status.Int() == KErrNone, status.Int());

	/* SEND THE SPECIAL COMMAND PACKET TO THE DUMMY NIF TO FORCE A RECONNECT */

	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) KForceReconnect;

	commandSocket.SendTo(buffer, commandDest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());

	/* WAIT A WHILE TO LET THE RECONNECT HAPPEN (OR NOT IF DIALOG APPEARS AND USER DOESN'T RESPOND!) */

	User::After(10000000);

	/* SEND MORE LOOPBACK TRAFFIC ON THE DATA SOCKET */
	
	// the idea is that reconnect does not lead to an erroring of the sockets - the stack should 
	// not see that the rug was pulled out.

	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) 0x8;		// ICMP type = 8
	buffer[1] = (TUint8) 0x0;		// ICMP code = 0
	buffer[2] = (TUint8) 0xF7;		// ICMP checksum high byte
	buffer[3] = (TUint8) 0xFF;		// ICMP checksum low byte

	dataSocket.SendTo(buffer, dataDest, 0, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	
	buffer.Zero();
  	dataSocket.RecvFrom(buffer, dataDest, 0, status);
  	User::WaitForRequest(status);
  	TESTEL(status.Int() == KErrNone, status.Int());

	/* CLOSE EVERYTHING DOWN */
	
 	// close the data socket
	dataSocket.Shutdown(RSocket::ENormal, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	CleanupStack::Pop(&dataSocket);

	// close the command socket
	commandSocket.Shutdown(RSocket::ENormal, status);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	CleanupStack::Pop(&commandSocket);

	// force the destruction of the connection - if the reconnect hasn't completed yet it will be cancelled
	r = connection.Stop();
	TESTEL(r == KErrNone, r);
	CleanupStack::Pop(&connection);

	// close the socket server
	server.Close();
	CleanupStack::Pop(&server);

	__UHEAP_MARKEND;

	return iTestStepResult;
}
