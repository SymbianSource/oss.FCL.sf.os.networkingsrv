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
// Contains the implementation of the test cases that predominately use the dummy nif
// as the default interface.
// 
//

/**
 @file TS_DummyPPPNifTestSteps.cpp
*/

#include "TS_DummyPPPNifTestSteps.h"
#include "DummyPPPNifVar.h"
#include "commdbconnpref.h"

#include "connectprog.h"
//New ppp progress states file
#include "PppProg.h"
#include "nullagtprog.h"
#include "in_iface.h"
#include "nifman.h"
#include "inet6err.h"

TS_DummyPPPTest1::TS_DummyPPPTest1(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest1::~TS_DummyPPPTest1()
{
}

// creates a connection and destroys it again
enum TVerdict TS_DummyPPPTest1::doTestStepL(void)
{
	TInt err;

	RConnection conn;
	RSocketServ ss;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest1

TS_DummyPPPTest2::TS_DummyPPPTest2(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest2::~TS_DummyPPPTest2()
{
}

// creates a connection and destroy it again - use the synchronous start
enum TVerdict TS_DummyPPPTest2::doTestStepL(void)
{
	TInt err;

	RConnection conn;
	RSocketServ ss;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionSynchronous(conn);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest2

TS_DummyPPPTest3::TS_DummyPPPTest3(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest3::~TS_DummyPPPTest3()
{
}

// Create a connection using database overrides and then destroy it
enum TVerdict TS_DummyPPPTest3::doTestStepL(void)
{
	TInt err;

	RConnection conn;
	RSocketServ ss;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverrides(conn, iDummyNifIap);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest3

TS_DummyPPPTest4::TS_DummyPPPTest4(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest4::~TS_DummyPPPTest4()
{
}

// Create a connection using database overrides and then destroy it - use the synchronous start
enum TVerdict TS_DummyPPPTest4::doTestStepL(void)
{
	TInt err;

	RConnection conn;
	RSocketServ ss;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverridesSynchronous(conn, iDummyNifIap);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest4

TS_DummyPPPTest5::TS_DummyPPPTest5(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest5::~TS_DummyPPPTest5()
{
}

// Implicitly create a single connection using SendTo()
enum TVerdict TS_DummyPPPTest5::doTestStepL(void)
{
	TInt err;

	RSocketServ ss;
	RSocket sock;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	err = TestUdpDataPath(sock, iDummyNifSendAddr); // this will use SendTo()
	TESTEL(KErrNone == err, err);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest5

TS_DummyPPPTest6::TS_DummyPPPTest6(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest6::~TS_DummyPPPTest6()
{
}

// Explicitly associate a UDP socket to an existing connection
enum TVerdict TS_DummyPPPTest6::doTestStepL(void)
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock, ss, conn);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest6


TS_DummyPPPTest7::TS_DummyPPPTest7(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest7::~TS_DummyPPPTest7()
{
}

enum TVerdict TS_DummyPPPTest7::doTestStepL(void)
/*
 * Ensure that initially chosen interface with implicit connection creation 
 * matches that chosen for later reuses (using UDP sockets)
 */
{
	TInt err;

	RSocketServ ss;
	RSocket sock;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	// this should implicitly create a connection
	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest7

TS_DummyPPPTest8::TS_DummyPPPTest8(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest8::~TS_DummyPPPTest8()
{
}

enum TVerdict TS_DummyPPPTest8::doTestStepL(void)
/*
 * Ensure that initially chosen interface with implicit connection creation 
 * matches that chosen for later reuses (using UDP sockets) after half of the short time out
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock;
	TTimeoutValues timeouts;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	/*
	 * Temporarily create a connection to read the timeout vals from comm db
	 */
		
	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = GetTimeoutValues(conn, timeouts);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);

	/*
	 * Now do the test itself
	 */

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	// this should implicitly create a connection
	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	// wait for half of the short timeout
	User::After((timeouts.iShortTimeout*1000000)/2);

	// should reuse the existing interface
	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	CloseConnection(conn);
	CleanupStack::Pop();

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest8

TS_DummyPPPTest9::TS_DummyPPPTest9(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest9::~TS_DummyPPPTest9()
{
}

enum TVerdict TS_DummyPPPTest9::doTestStepL(void)
/*
 * Send data over two separate sockets using the same connection and interface
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock1, sock2;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);
	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock1, ss, conn);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	err = OpenUdpSocketExplicitL(sock2, ss, conn);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest9

TS_DummyPPPTest10::TS_DummyPPPTest10(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest10::~TS_DummyPPPTest10()
{
}

enum TVerdict TS_DummyPPPTest10::doTestStepL(void)
/*
 * Two sockets (UDP) sending over different sessions (RConnection) to the same interface
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn1, conn2;
	RSocket sock1, sock2;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);
	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);
	err = StartConnection(conn2);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock1, ss, conn1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);
	err = OpenUdpSocketExplicitL(sock2, ss, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);
	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);
	
	DestroyUdpSocket(sock2);
	CleanupStack::Pop();
	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();
	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest10

TS_DummyPPPTest11::TS_DummyPPPTest11(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest11::~TS_DummyPPPTest11()
{
}

enum TVerdict TS_DummyPPPTest11::doTestStepL(void)
/*
 * Two sockets sending over the same interface accessing it from different socket servers
 */
{
	TInt err;

	RSocketServ ss1, ss2;
	RConnection conn1, conn2;
	RSocket sock1, sock2;
	TUint numOfConnections;

	err = OpenSocketServer(ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss1);
	err = OpenSocketServer(ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss2);

	err = OpenConnection(conn1, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);
	err = OpenConnection(conn2, ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);
	err = StartConnection(conn2);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	err = OpenUdpSocketExplicitL(sock1, ss1, conn1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);
	err = OpenUdpSocketExplicitL(sock2, ss2, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);
	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);
	
	DestroyUdpSocket(sock2);
	CleanupStack::Pop();
	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();
	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss2);
	CleanupStack::Pop();

	CloseSocketServer(ss1);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest11


TS_DummyPPPTest12::TS_DummyPPPTest12(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest12::~TS_DummyPPPTest12()
{
}

enum TVerdict TS_DummyPPPTest12::doTestStepL(void)
/*
 * Two sockets sending over the same interface from within different socket servers,
 * implicitly created connections.
 */
{
	TInt err;

	RSocketServ ss1, ss2;
	RConnection conn;
	RSocket sock1, sock2;
	TUint numOfConnections = 0;

	/*
	 * Sort out the first socket
	 */ 
	
	err = OpenSocketServer(ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss1);

	err = OpenUdpSocketL(sock1, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	/*
	 * Sort out the second socket
	 */

	err = OpenSocketServer(ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss2);
	
	err = OpenUdpSocketL(sock2, ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	/*
	 * Try out the traffic paths, both should succeed
	 */ 

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	/*
	 * Try and find out how many interfaces there are
	 */

	err = OpenConnection(conn, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	CloseConnection(conn);
	CleanupStack::Pop();
	
	/*
	 * Now tidy up in reverse order to creation so the order is right for the cleanup stack
	 */

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	CloseSocketServer(ss2);
	CleanupStack::Pop();

	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseSocketServer(ss1);
	CleanupStack::Pop();
	
	return iTestStepResult;
} // TS_DummyPPPTest12

TS_DummyPPPTest13::TS_DummyPPPTest13(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest13::~TS_DummyPPPTest13()
{
}

enum TVerdict TS_DummyPPPTest13::doTestStepL(void)
/*
 * Ensure that Close() effectively pulls down the interface when no other subsessions are 
 * associated with it.
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock;
	TUint numOfConnections = 0;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	User::After(iShortTimeout+1000000); // wait for the interface to be destroyed

	Log(_L("iShortTimeout value =%d + 1000000 wait "),iShortTimeout);

	numOfConnections = NumberOfInterfacesL(ss);
	TESTEL(0 == numOfConnections, numOfConnections);
	
	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest13

TS_DummyPPPTest14::TS_DummyPPPTest14(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest14::~TS_DummyPPPTest14()
{
}

enum TVerdict TS_DummyPPPTest14::doTestStepL(void)
/*
 * Ensure that Close() does not pull down the interface when there are other connections 
 * associated with it.
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn1, conn2;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	/*
	 * Establish the two connections on the same (default) interface
	 */

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);
	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);
	err = StartConnection(conn2);
	TESTEL(KErrNone == err, err);

	/*
	 * See how many interfaces we have created
	 */
	
	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	/*
	 * Close down the first connection
	 */

	CloseConnection(conn1);
	/*
	 * conn1 isn't at top of cleanup stack, hence pop off two then put conn2 back on
	 */
	CleanupStack::Pop(2);
	CleanupClosePushL(conn2);

	/*
	 * Wait for this connection to be destroyed, don't wait too long or the other one will
	 * be destroyed also! (don't wait mediumTimeOut)
	 */
	User::After(iShortTimeout);

	/*
	 * See how many connections we have now
	 */

	err = EnumerateConnections(conn2, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	/*
	 * Close down everything else
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	User::After(iShortTimeout);

	numOfConnections = NumberOfInterfacesL(ss);
	TESTEL(0 == numOfConnections, numOfConnections);

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest14


TS_DummyPPPTest15::TS_DummyPPPTest15(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest15::~TS_DummyPPPTest15()
{
}

enum TVerdict TS_DummyPPPTest15::doTestStepL(void)
/*
 * Ensure that Stop() pulls down the interface when there are no subsessions other
 * than the connection associated with it
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RConnection countingConn;
	TUint numOfConnections = 0;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);
	
	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(countingConn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(countingConn);

	err = EnumerateConnections(countingConn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	err = StopConnection(conn);
	TESTEL(KErrNone == err, err);
	CloseConnection(conn);
	/*
	 * conn isn't at top of cleanup stack hence pop 2 then put countingConn back on
	 */
	CleanupStack::Pop(2);
	CleanupClosePushL(countingConn);

	User::After(iShortTimeout);

	err = EnumerateConnections(countingConn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(0 == numOfConnections, numOfConnections);

	CloseConnection(countingConn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest15

TS_DummyPPPTest16::TS_DummyPPPTest16(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest16::~TS_DummyPPPTest16()
{
}

enum TVerdict TS_DummyPPPTest16::doTestStepL(void)
/*
 * Ensure that Stop() pulls down the interface when there are sockets associated with it
 */
{
	TInt err;
	
	RSocketServ ss;
	RConnection conn;
	RSocket sock;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);
	
	err = StopConnection(conn);
	TESTEL(KErrNone == err, err);
	CloseConnection(conn);
	/*
	 * conn isn't at the top of the cleanup stack hence pop 2 then put sock back on
	 */
	CleanupStack::Pop(2);
	CleanupClosePushL(sock);

	User::After(iShortTimeout);

	numOfConnections = NumberOfInterfacesL(ss);
	TESTEL(0 == numOfConnections, numOfConnections);
	
	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest16


TS_DummyPPPTest17::TS_DummyPPPTest17(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest17::~TS_DummyPPPTest17()
{
}

enum TVerdict TS_DummyPPPTest17::doTestStepL(void)
/*
 * Test sequence of progress notifications for a single explicitly created connection
 */
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverrides(conn, iNtRasIap);
	TESTEL(KErrNone == err, err);

	/*
	 * Using the default interface which should be null agt and dummy ppp nif, hence
	 * use the null agt progress states
	 */

	// wait for KStartingSelection (1000)
	ProgressNotification(conn, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KStartingSelection == progress().iStage, progress().iStage);

	// wait for KFinishedSelection (2000)
	ProgressNotification(conn, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KFinishedSelection == progress().iStage, progress().iStage);

	ProgressNotification(conn, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(ENullAgtConnecting == progress().iStage, progress().iStage);

	ProgressNotification(conn, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(ENullAgtConnected == progress().iStage, progress().iStage);

	/*
	 * Close down
	 */

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest17

TS_DummyPPPTest18::TS_DummyPPPTest18(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest18::~TS_DummyPPPTest18()
{
}

enum TVerdict TS_DummyPPPTest18::doTestStepL(void)
/*
 * Create two connections on the same interface and check that the progress notification 
 * for each is the same
 */
{
	TInt err;
	TRequestStatus startStatus, status;

	RSocketServ ss;
	RConnection conn1, conn2;
	TNifProgressBuf progress;
	TCommDbConnPref prefs;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	/*
	 * Get the first connection going, monitoring the progress
	 */

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	StartConnectionWithOverridesAsynchronous(conn1, prefs, iNtRasIap, startStatus);

	// wait for KStartingSelection (1000)
	ProgressNotification(conn1, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KStartingSelection == progress().iStage, progress().iStage);

	// wait for KFinishedSelection (2000)
	ProgressNotification(conn1, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KFinishedSelection == progress().iStage, progress().iStage);

	// wait for ENullAgtConnecting (2501)
	ProgressNotification(conn1, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(ENullAgtConnecting == progress().iStage, progress().iStage);

	// wait for ENullAgtConnected (3500)
	ProgressNotification(conn1, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(ENullAgtConnected == progress().iStage, progress().iStage);

	//Fix for DEF03548 Changing progress states to match that in pppprog.h
	// wait for EPppProgressLinkUp/KLinkLayerOpen (7000)
	ProgressNotification(conn1, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(EPppProgressLinkUp == progress().iStage, progress().iStage);

	User::WaitForRequest(startStatus);
	TESTEL(startStatus.Int() == KErrNone, startStatus.Int());

	/*
	 * Now start the second connection and see that it is immediately up
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	StartConnectionWithOverridesAsynchronous(conn2, prefs, iNtRasIap, startStatus);

	// wait for KStartingSelection (1000)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KStartingSelection == progress().iStage, progress().iStage);

	// wait for KFinishedSelection (2000)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KFinishedSelection == progress().iStage, progress().iStage);

	//Fix for DEF03548 Changing progress states to match that in pppprog.h
	// wait for EPppProgressLinkUp/KLinkLayerOpen (7000)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(EPppProgressLinkUp == progress().iStage, progress().iStage);

	User::WaitForRequest(startStatus);
	TESTEL(startStatus.Int() == KErrNone, startStatus.Int());

	/*
	 * Tidy everything up
	 */

	err = StopConnection(conn1);
	TESTEL(KErrNone == err, err);
	CloseConnection(conn1);

	// wait for KConfigDaemonStartingDeregistration (8600) (from the null configuration daemon)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KConfigDaemonStartingDeregistration == progress().iStage, progress().iStage);

	// wait for KConfigDaemonFinishedDeregistrationStop (8700) (from the null configuration daemon)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone == progress().iError, progress().iError);
	TESTEL(KConfigDaemonFinishedDeregistrationStop == progress().iStage, progress().iStage);

	// wait for EPppProgressLinkDown (8000)
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrCancel== progress().iError, progress().iError);
	TESTEL(EPppProgressLinkDown == progress().iStage, progress().iStage);

	// wait for KConnectionClosed
	ProgressNotification(conn2, status, progress);
	User::WaitForRequest(status);
	TESTEL(KErrNone== progress().iError, progress().iError);
	TESTEL(KConnectionClosed == progress().iStage, progress().iStage);

	CloseConnection(conn2);
	CleanupStack::Pop();  //cleanup conn2

	CleanupStack::Pop();  // cleanup conn1

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest18


TS_DummyPPPTest19::TS_DummyPPPTest19(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest19::~TS_DummyPPPTest19()
{
}

enum TVerdict TS_DummyPPPTest19::doTestStepL(void)
{
	TInt err;
	
	RSocketServ ss1, ss2;
	RConnection conn1, conn2;
	RSocket sock2;
	TUint numOfConnections;

	err = OpenSocketServer(ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss1);

	err = OpenSocketServer(ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss2);

	err = OpenConnection(conn1, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn2, ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = StartConnectionWithOverrides(conn2, iDummyNifIap);
	TESTEL(KErrNone == err, err);


	err = OpenUdpSocketExplicitL(sock2, ss2, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);
	err = EnumerateConnections(conn2, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss2);
	CleanupStack::Pop();

	CloseSocketServer(ss1);
	CleanupStack::Pop();
	
	return iTestStepResult;
} // TS_DummyPPPTest19



TS_DummyPPPTest20::TS_DummyPPPTest20(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest20::~TS_DummyPPPTest20()
{
}

enum TVerdict TS_DummyPPPTest20::doTestStepL(void)
/*
 * Within a single socket server first implicitly create a connection by using the RSocket method 
 * SendTo() then explicitly create a connection with the default commdb settings.
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock;
	TUint numOfConnections = 0;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	CloseConnection(conn);
	CleanupStack::Pop();

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest20

TS_DummyPPPTest21::TS_DummyPPPTest21(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest21::~TS_DummyPPPTest21()
{
}

enum TVerdict TS_DummyPPPTest21::doTestStepL(void)
/*
 * Test interface to CommDb through RConnection
 */
{
	TInt err;
	
	RSocketServ ss;
	RConnection conn;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);
	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	// read an integer value back from commdb (IAP\Id chosen)
	_LIT(KName1, "IAP\\id");
	TBufC<7> name1(KName1);
	TUint32 intval;
	err = conn.GetIntSetting(name1, intval);
	TESTEL(KErrNone == err, err);

	// read a bool value back from commdb (DialOutISP\IfPromptForAuth chosen)
	_LIT(KName2, "DialOutISP\\IfPromptForAuth");
	TBufC<27> name2(KName2);
	TBool boolval;
	err = conn.GetBoolSetting(name2, boolval);
	TESTEL(KErrNone == err, err);

	// read a descriptor back from commdb (DialOutISP\IfName chosen)
	// this should fail as this field does not exist anymore
	_LIT(KName3, "DialOutISP\\IfName");
	TBufC<18> name3(KName3);
	TBuf<128> desval;
	err = conn.GetDesSetting(name3, desval);
	TESTEL(KErrNotFound == err, err);

	// read a long descriptor back from commdb (DialOutISP\LoginScript chosen)
	_LIT(KName4, "DialOutISP\\LoginScript");
	TBufC<23> name4(KName4);
	TBuf<128> longdesval;
	err = conn.GetLongDesSetting(name4, longdesval);
	TESTEL(KErrNone == err, err);

	// read a descriptor that actually will be successfully read back from the database
	_LIT(KName5, "ModemBearer\\IfName");
	TBufC<21> name5(KName5);
	TBuf16<128> desval16;
	err = conn.GetDesSetting(name5, desval16);
	TESTEL(KErrNone == err, err);
	TBuf8<128> desval8;
	err = conn.GetDesSetting(name5, desval8);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest21

TS_DummyPPPTest22::TS_DummyPPPTest22(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest22::~TS_DummyPPPTest22()
{
}

enum TVerdict TS_DummyPPPTest22::doTestStepL(void)
/*
 * Test reporting of the interface coming down
 */
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	err = RequestInterfaceDownL(conn, ss);
	TESTEL(KErrNone == err, err);

	ProgressNotification(conn, status, progress, KLinkLayerClosed);
	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	TESTEL(progress().iStage == KLinkLayerClosed, progress().iStage);

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest22

TS_DummyPPPTest23::TS_DummyPPPTest23(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest23::~TS_DummyPPPTest23()
{
}

enum TVerdict TS_DummyPPPTest23::doTestStepL(void)
/*
 * Test cancelling of progress notification
 */
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	// use the progress notification to make sure it works
	ProgressNotification(conn, status, progress, KLinkLayerOpen);
	User::WaitForRequest(status);

	// this is the one we are going to cancel
	ProgressNotification(conn, status, progress, KLinkLayerClosed);

	CancelProgressNotification(conn);

	User::WaitForRequest(status);
	TESTEL(KErrCancel == status.Int(), status.Int());

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest23

TS_DummyPPPTest24::TS_DummyPPPTest24(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest24::~TS_DummyPPPTest24()
{
}

enum TVerdict TS_DummyPPPTest24::doTestStepL(void)
/*
 * Duplicate an already existing connection within a single socket server.
 */
{
	TInt err;

	RSocketServ ss;
	RConnection conn1, conn2;
	RSocket sock1, sock2;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	/*
	 * Open the first connection and check it works
	 */

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock1, ss, conn1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	/*
	 * Create the second connection, passing it the name of the first
	 */

	TName name;

	err = conn1.Name(name);
	TESTEL(KErrNone == err, err);

	// With Platform Security, we need to explicitly enable clone opening

	TSecurityPolicyBuf passPolicy(TSecurityPolicy::EAlwaysPass);
	err = conn1.Control(KCOLConnection, KCoEnableCloneOpen, passPolicy);
	Log(_L("Enabling clone open returned %d"), err);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn2, ss, name);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	err = OpenUdpSocketExplicitL(sock2, ss, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();

	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest24

TS_DummyPPPTest25::TS_DummyPPPTest25(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest25::~TS_DummyPPPTest25()
{
}

enum TVerdict TS_DummyPPPTest25::doTestStepL(void)
/* 
 * Duplicate an already existing connection within a different socket server.
 */
{
	TInt err;

	RSocketServ ss1, ss2;
	RConnection conn1, conn2;
	RSocket sock1, sock2;
	TUint numOfConnections;

	err = OpenSocketServer(ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss1);

	err = OpenSocketServer(ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss2);

	/*
	 * Open the first connection and check it works
	 */

	err = OpenConnection(conn1, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock1, ss1, conn1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	/*
	 * Create the second connection, passing it the name of the first
	 */

	TName name;

	err = conn1.Name(name);
	TESTEL(KErrNone == err, err);

	// With Platform Security, we need to explicitly enable clone opening

	TSecurityPolicyBuf passPolicy(TSecurityPolicy::EAlwaysPass);
	err = conn1.Control(KCOLConnection, KCoEnableCloneOpen, passPolicy);
	Log(_L("Enabling clone open returned %d"), err);
	TESTEL(KErrNone == err, err);

	err = OpenConnection(conn2, ss2, name);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	err = OpenUdpSocketExplicitL(sock2, ss2, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();

	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss2);
	CleanupStack::Pop();

	CloseSocketServer(ss1);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest25

TS_DummyPPPTest26::TS_DummyPPPTest26(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest26::~TS_DummyPPPTest26()
{
}

enum TVerdict TS_DummyPPPTest26::doTestStepL(void)
/*
 * Attempt to instantiate a nif that does not exist
 */
{	
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;

	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	ProgressNotification(conn, status, progress, KLinkLayerOpen);

	err = StartConnectionWithOverrides(conn, iMissingNifIap);
	TESTEL(KErrNone == err, err);		// IAP is prompted for and auto prompt says ok

	User::WaitForRequest(status);
	TESTEL(status.Int() == KErrNone, status.Int());
	TESTEL(progress().iStage == KLinkLayerOpen, progress().iStage);


	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest26

/*
 * Test 27
 * TestTwoConnectionsSameNetworkL()
 * Two connections on the same network
 */

TS_DummyPPPTest27::TS_DummyPPPTest27(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest27::~TS_DummyPPPTest27()
{
}

enum TVerdict TS_DummyPPPTest27::doTestStepL(void)
{
	TInt err;

	RSocketServ ss1, ss2;
	RConnection conn1, conn2;
	RSocket sock1, sock2;

	/*
	 * Open the socket servers
	 */

	err = OpenSocketServer(ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss1);

	err = OpenSocketServer(ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss2);

	/*
	 * Open and start the connections
	 */

	err = OpenConnection(conn1, ss1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnectionWithOverrides(conn1, iDummyNifIap);
	TESTEL(KErrNone == err, err);
	
	err = OpenConnection(conn2, ss2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = StartConnectionWithOverrides(conn2, iDummyNifIap);
	TESTEL(KErrNone == err, err);

	/*
	 * Do some strange stuff
	 */

	err = OpenUdpSocketExplicitL(sock1, ss1, conn1);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	err = sock1.SetOpt(KSoNoInterfaceError, KSolInetIp, 1);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock2, ss2, conn2);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock2);

	err = sock2.SetOpt(KSoNoInterfaceError, KSolInetIp, 1);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = StopConnection(conn2);
	TESTEL(KErrNone == err, err);

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNotReady == err, err);

	err = TestUdpDataPath(sock2, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock2);
	CleanupStack::Pop();
 
	DestroyUdpSocket(sock2);
	CleanupStack::Pop();

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss2);
	CleanupStack::Pop();

	CloseSocketServer(ss1);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest27

/*
 * Test 28
 * TestConnectionGoneStateL()
 * Create a connection and a socket, destroy the subsessions, wait a bit, then use the socket again
 * before the interface has been destroyed by nifman (esock will have no record of the IF but 
 * nifman will still be aware of it).
 */

TS_DummyPPPTest28::TS_DummyPPPTest28(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}


TS_DummyPPPTest28::~TS_DummyPPPTest28()
{
}

enum TVerdict TS_DummyPPPTest28::doTestStepL(void)
{
	TInt err;

	RSocketServ ss;
	RConnection conn;
	RSocket sock;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverrides(conn, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	// Use the socket, just for good measure
	err = TestUdpDataPath(sock, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	/*
	 * everything is set up, now do the test
	 * must close both the socket and the connection
	 */

	TInt interfaceIndex;
	TPckg<TInt> indexBuf(interfaceIndex);
	sock.GetOpt(KSoInterfaceIndex, KSolInetIp, indexBuf);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	User::After(100000 /*iShortTimeout/3*/);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	sock.SetOpt(KSoInterfaceIndex, KSolInetIp, indexBuf);

	err = TestUdpDataPath(sock, iDummyNifSendAddr);

	TESTEL(KErrNone == err, err);

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest28


/*
 * Test 29
 * TestAttachL()
 * Attach a new RConnection to an existing connection. Also try to attach to a non-existant interface
 */

TS_DummyPPPTest29::TS_DummyPPPTest29(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}


TS_DummyPPPTest29::~TS_DummyPPPTest29()
{
}

enum TVerdict TS_DummyPPPTest29::doTestStepL(void)
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn1, conn2;
	TPckgBuf<TConnectionInfo> info;
	TUint numOfConnections = 0;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	StartConnectionAsynchronous(conn1, status);
	
	/*
	 * Attempt to attach before the start has completed - should fail
	 */


	err = AttachNormal(conn2, TPtrC8());
	TESTEL(KErrArgument == err, err);

	User::WaitForRequest(status);
	TESTEL(KErrNone == status.Int(), status.Int());

	/*
	 * Attempt to attach to non-existant connection
	 */

	info().iIapId = 9999;
	info().iNetId = 9999;

	err = AttachNormal(conn2, info); // should fail
	TESTEL(KErrCouldNotConnect == err, err);

	/*
	 * Check how many interfaces we have
	 */

	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(1 == numOfConnections, numOfConnections);

	/*
	 * Now for an attach that should succeed
	 */

	err = conn1.GetConnectionInfo(1, info);
	TESTEL(KErrNone == err, err);

	err = AttachNormal(conn2, info);
	TESTEL(KErrNone == err, err);

	TNifProgress progress;
	err = Progress(conn2, progress);
	TESTEL(KErrNone == err, err);
	TESTEL(progress.iStage == KLinkLayerOpen, progress.iStage);
	TESTEL(progress.iError == 0, progress.iError);

	/*
	 * Tidy up
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest29

/*
 * Test 30
 * TestAttachMonitorL()
 * Attach an RConnection to another as a monitor
 */

TS_DummyPPPTest30::TS_DummyPPPTest30(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}


TS_DummyPPPTest30::~TS_DummyPPPTest30()
{
}

enum TVerdict TS_DummyPPPTest30::doTestStepL(void)
{
	TInt err;

	RSocketServ ss;
	RConnection conn1, conn2;
	TPckgBuf<TConnectionInfo> info1, info2;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	/*
	 * So let's attach as monitor
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = GetInfo(conn1, info1);
	TESTEL(KErrNone == err, err);

	err = AttachMonitor(conn2, info1);
	TESTEL(KErrNone == err, err);

	err = GetInfo(conn2, info2);
	TESTEL(KErrNone == err, err);

	/*
	 * Check they are the same interface
	 */

	TESTEL(info1().iIapId == info2().iIapId, info2().iIapId);
	TESTEL(info1().iNetId == info2().iNetId, info2().iNetId);

	/*
	 * Close down
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);

	/*
	 * Now do the whole thing again, but close them in the opposite order, having fun and 
	 * japes with the cleanup stack along the way
	 */

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnection(conn1);
	TESTEL(KErrNone == err, err);

	/*
	 * Right then, let's attach as monitor again
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);

	err = conn1.GetConnectionInfo(1, info1);
	TESTEL(KErrNone == err, err);

	err = AttachMonitor(conn2, info1);
	TESTEL(KErrNone == err, err);

	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn2, numOfConnections);
	TESTEL(KErrNone == err, err);

	err = conn2.GetConnectionInfo(1, info2);
	TESTEL(KErrNone == err, err);

	/*
	 * Check that they are the same interface
	 */

	TESTEL(info1().iIapId == info2().iIapId, info2().iIapId);
	TESTEL(info1().iNetId == info2().iNetId, info2().iIapId);

	/*
	 * Close down, here we go....
	 */

	CloseConnection(conn1);
	// conn1 not at top of cleanup stack, hence pop 2 and push conn2 again
	CleanupStack::Pop(2);
	CleanupClosePushL(conn2);

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest30


/*
 * Test 31
 * TestOOMStartLoopL()
 * Try to perform Start() repeatedly while increasing failnext on esock
 */

TS_DummyPPPTest31::TS_DummyPPPTest31(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest31::~TS_DummyPPPTest31()
{
}

enum TVerdict TS_DummyPPPTest31::doTestStepL(void)
{
	TInt err = KErrGeneral;
	TInt i = 0;
	
	RSocketServ ss;
	RConnection conn, conn2, connMonitor;
	RSocket sock;
	TPckgBuf<TConnectionInfo> info;
	TUint numOfConnections;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	/*
	 * Test OOM conditions when opening connection
	 */

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, OpenConnection(conn, ss));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	CloseConnection(conn);

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);

	/*
	 * Test OOM conditions when starting connection
	 */
	
	i = 0;

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, StartConnectionWithOverrides(conn, iDummyNifIap));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	CloseConnection(conn);
	CleanupStack::Pop();

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);
	
	/*
	 * Test OOM conditions with Attach() normal
	 */

	i = 0;
	
	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);
	
	err = StartConnectionWithOverrides(conn, iDummyNifIap);
	TESTEL(KErrNone == err, err);
	
	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);
	
	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);

	err = conn.GetConnectionInfo(1, info);
	TESTEL(KErrNone == err, err);
	TESTEL(numOfConnections == 1, numOfConnections);

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, AttachNormal(conn2, info));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);

	/*
	 * Test OOM conditions with Attach() monitor
	 */

	i = 0;
	
	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);
	
	err = StartConnectionWithOverrides(conn, iDummyNifIap);
	TESTEL(KErrNone == err, err);
	
	err = OpenConnection(connMonitor, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);
	
	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);

	err = conn.GetConnectionInfo(1, info);
	TESTEL(KErrNone == err, err);
	TESTEL(numOfConnections == 1, numOfConnections);

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, AttachMonitor(connMonitor, info));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	CloseConnection(connMonitor);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	err = WaitForAllInterfacesToCloseL(ss);
	TESTEL(KErrNone == err, err);

	/*
	 * Test OOM conditions when sending over a UDP socket
	 */

	i = 0;

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverrides(conn, iDummyNifIap);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, TestUdpDataPath(sock, iDummyNifSendAddr));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	/*
	 * Test OOM conditions when sending over a UDP socket (implicitly creating connection)
	 */

	i = 0;

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	do
	{
		ss.__DbgFailNext(i);
		TRAP(err, TestUdpDataPath(sock, iDummyNifSendAddr));
		ss.__DbgFailNext(KMaxTInt);
		i++;
	} while (err != KErrNone);

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	/*
	 * All the excitement is over, so close the socket server now
	 */

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest31

/*
 * Test 32
 * TestMediumTimeoutExtensionL()
 * Create a connection, then wait a bit before starting another connection thus 
 * extending the medium timer
 */

TS_DummyPPPTest32::TS_DummyPPPTest32(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest32::~TS_DummyPPPTest32()
{
}

enum TVerdict TS_DummyPPPTest32::doTestStepL(void)
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn1, conn2;
	TTimeIntervalSeconds timeElapsed;
	TInt delta;
	TTimeoutValues timeouts;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnectionWithOverrides(conn1, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);

	err = GetTimeoutValues(conn1, timeouts);
	TESTEL(KErrNone == err, err);

	ProgressNotification(conn1, status, progress, KLinkLayerClosed);

	User::After((timeouts.iMediumTimeout * 1000000) /2); // wait part of the time out

	/*
	 * Hopefully we will extend the timeout by opening another connection
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = StartConnectionWithOverrides(conn2, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);

	err = TimeUntilRequestComplete(status, timeElapsed);
	TESTEL(KErrNone == err, err);

	delta = timeElapsed.Int() - timeouts.iMediumTimeout;
	Log(_L("delta value =%d timeElapsed=%d iMediumTimeout=%d "),delta,timeElapsed.Int(),timeouts.iMediumTimeout);
	TESTEL(delta >= -1 && delta <= 1, delta);

	/*
	 * Tidy up
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();
	
	return iTestStepResult;
} // TS_DummyPPPTest32

/*
 * Test 33
 * TestMediumTimeoutExtensionL()
 * Create a connection, then wait a bit before attaching (normally) another connection thus 
 * extending the medium timer
 */

TS_DummyPPPTest33::TS_DummyPPPTest33(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest33::~TS_DummyPPPTest33()
{
}

enum TVerdict TS_DummyPPPTest33::doTestStepL(void)
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn1, conn2;
	TPckgBuf<TConnectionInfo> info;
	TTimeIntervalSeconds timeElapsed;
	TInt delta;
	TTimeoutValues timeouts;
	TUint numOfConnections;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnectionWithOverrides(conn1, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);

	err = GetTimeoutValues(conn1, timeouts);
	TESTEL(KErrNone == err, err);

	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(numOfConnections == 1, numOfConnections);

	err = conn1.GetConnectionInfo(1, info);
	TESTEL(KErrNone == err, err);

	ProgressNotification(conn1, status, progress, KLinkLayerClosed);

	User::After((timeouts.iMediumTimeout * 1000000) /2); // wait part of the time out

	/*
	 * Hopefully we will extend the timeout by opening another connection
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = AttachNormal(conn2, info);
	TESTEL(KErrNone == err, err);

	err = TimeUntilRequestComplete(status, timeElapsed);
	TESTEL(KErrNone == err, err);

	delta = timeElapsed.Int() - timeouts.iMediumTimeout;
	TESTEL(delta >= -1 && delta <= 1, delta);

	/*
	 * Tidy up
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();
	
	return iTestStepResult;
} // TS_DummyPPPTest33

/*
 * Test 34
 * TestMediumTimeoutExtensionL()
 * Create a connection, then wait a bit before monitor-attaching another connection and checking
 * that this does not extend the medium timer.
 */

TS_DummyPPPTest34::TS_DummyPPPTest34(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest34::~TS_DummyPPPTest34()
{
}

enum TVerdict TS_DummyPPPTest34::doTestStepL(void)
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn1, conn2;
	TPckgBuf<TConnectionInfo> info;
	TTimeIntervalSeconds timeElapsed;
	TInt delta;
	TTimeoutValues timeouts;
	TUint numOfConnections;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn1, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn1);

	err = StartConnectionWithOverrides(conn1, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);

	err = GetTimeoutValues(conn1, timeouts);
	TESTEL(KErrNone == err, err);

	// need to call Enumerate() before GetConnectionInfo() to set up array used there
	err = EnumerateConnections(conn1, numOfConnections);
	TESTEL(KErrNone == err, err);
	TESTEL(numOfConnections == 1, numOfConnections);

	err = conn1.GetConnectionInfo(1, info);
	TESTEL(KErrNone == err, err);

	ProgressNotification(conn1, status, progress, KLinkLayerClosed);

	User::After((timeouts.iMediumTimeout * 1000000) /2); // wait part of the time out

	/*
	 * Hopefully the timer will not be extended by attaching a connection as a monitor
	 */

	err = OpenConnection(conn2, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn2);

	err = AttachMonitor(conn2, info);
	TESTEL(KErrNone == err, err);

	err = TimeUntilRequestComplete(status, timeElapsed);
	TESTEL(KErrNone == err, err);

	delta = timeElapsed.Int() - (timeouts.iMediumTimeout)/2;
	Log(_L("delta value =%d timeElapsed=%d iMediumTimeout=%d "),delta,timeElapsed.Int(),timeouts.iMediumTimeout);
	TESTEL(delta >= -1 && delta <= 1, delta);

	/*
	 * Tidy up
	 */

	CloseConnection(conn2);
	CleanupStack::Pop();

	CloseConnection(conn1);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();
	
	return iTestStepResult;
} // TS_DummyPPPTest34

/*
 * Test 35
 * TestLongTimeoutExtensionL()
 * Open a connection and a socket, wait a bit then send some data over the socket. This should
 * extend the long timer, check it does so.
 */

TS_DummyPPPTest35::TS_DummyPPPTest35(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest35::~TS_DummyPPPTest35()
{
}

enum TVerdict TS_DummyPPPTest35::doTestStepL(void)
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;
	RSocket sock1;
	TTimeIntervalSeconds timeElapsed;
	TTimeoutValues timeouts;
	TNifProgressBuf progress;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnectionWithOverrides(conn, iDummyNifLongTimeoutIap);
	TESTEL(KErrNone == err, err);
	
	err = GetTimeoutValues(conn, timeouts);
	TESTEL(KErrNone == err, err);

	err = OpenUdpSocketExplicitL(sock1, ss, conn);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock1);

	ProgressNotification(conn, status, progress, KLinkLayerClosed);

	User::After((timeouts.iLongTimeout * 1000000) /2); // wait part of the time out

	/*
	 * Hopefully we will extend the timeout by sending data
	 */

	err = TestUdpDataPath(sock1, iDummyNifSendAddr);
	TESTEL(KErrNone == err, err);

	err = TimeUntilRequestComplete(status, timeElapsed);
	TESTEL(KErrNone == err, err);

	TInt delta = timeElapsed.Int() - timeouts.iLongTimeout;
	TESTEL(delta >= -1 && delta <= 1, delta);	

	/*
	 * Tidy up
	 */

	DestroyUdpSocket(sock1);
	CleanupStack::Pop();

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest35


TS_DummyPPPTest36::TS_DummyPPPTest36(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName = aName;
}

TS_DummyPPPTest36::~TS_DummyPPPTest36()
{
}

enum TVerdict TS_DummyPPPTest36::doTestStepL()
/*
 * Use the service notification and cancel service notification methods
 */
{
	TInt err;
	TRequestStatus status;

	RSocketServ ss;
	RConnection conn;

	TUint32 isp;
	TBuf<20> newServiceType;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = StartConnection(conn);
	TESTEL(KErrNone == err, err);

	conn.ServiceChangeNotification(isp, newServiceType, status);

	conn.CancelServiceChangeNotification();

	User::WaitForRequest(status);

	TESTEL(status.Int() == KErrCancel, status.Int());

	CloseConnection(conn);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
}

//Added extra test to test flowon
TS_DummyPPPTest37::TS_DummyPPPTest37(TPtrC aName) : TS_DummyPPPStep()
{
	iTestStepName=aName;
}

TS_DummyPPPTest37::~TS_DummyPPPTest37()
{
}

// Implicitly create a single connection using SendTo()
enum TVerdict TS_DummyPPPTest37::doTestStepL(void)
{
	TInt err;

	RSocketServ ss;
	RSocket sock;

	err = OpenSocketServer(ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(ss);

	err = OpenUdpSocketL(sock, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	for(int iCount =0; iCount<100;iCount++)
	{
		err = TestUdpDataPath(sock, iDummyNifSendAddr); // this will use SendTo()
		TESTEL(KErrNone == err, err);
	}

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	CloseSocketServer(ss);
	CleanupStack::Pop();

	return iTestStepResult;
} // TS_DummyPPPTest37

