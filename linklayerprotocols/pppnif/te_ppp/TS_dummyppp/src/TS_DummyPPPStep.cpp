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
// This contains TS_RConnectionStep class which is
// the base class for all the RConnection multihoming test steps.
// Contains methods which simplify the API to RConnection, RSocket and 
// RHostResolver to be used by derived classes. This abstraction means 
// that the API can continue to evolve as it has done for a while without 
// impacting a large number of test cases - just alter this parent class.
// 
//

/**
 @file TS_DummyPPPStep.cpp
*/
#include "TS_DummyPPPStep.h"
#include "commdbconnpref.h"
#include "DummyPPPNifVar.h"
#include <c32root.h>

const TUint KBufferLength = 512;

// Constructor
TS_DummyPPPStep::TS_DummyPPPStep()
{
}

// Destructor
TS_DummyPPPStep::~TS_DummyPPPStep()
	{ 
	}
		
// Method to Copy a file to another file. 
void TS_DummyPPPStep::copyFileL (const TDesC& anOld,const TDesC& aNew) 
	{

	// create a fileserver
	RFs  FileSystem;

	// connect to file server
	TInt returnCode=FileSystem.Connect();

	// create a file manager
	CFileMan * fileMan = CFileMan::NewL( FileSystem );

	CleanupStack::PushL(fileMan);

	if (returnCode != KErrNone )
		{
		User::Leave(returnCode);
		}

	// parse the filenames
	TParse Source;
	returnCode = Source.Set( anOld, NULL, NULL );
	if ( returnCode != KErrNone )
		{
		User::Leave(returnCode);
		}
 
	// parse the filenames
	TParse Target;
	returnCode = Target.Set( aNew, NULL, NULL );
	if ( returnCode != KErrNone )
		{
		User::Leave(returnCode);
		}

	// do the copy
	returnCode=fileMan->Copy(Source.FullName(), 
		Target.FullName(), CFileMan::EOverWrite);

	if ( returnCode != KErrNone )
		{
			User::Leave(returnCode);
		}

	CleanupStack::PopAndDestroy(fileMan);
	// close the file system
	FileSystem.Close();
	}

enum TVerdict TS_DummyPPPStep::doTestStepPreambleL(void)
{
	enum TVerdict result = EPass;

	/*
	 * Load the config from the .ini file
	 */

	if (KErrNone != ReadIniFile())
	    {
		result = EFail;
		}
	
	iDummyNifSendAddr.SetAddress(KDummyPppNifLocalAddressBase + 4);
	iDummyNifSendAddr.SetPort(iEchoPortNum);
	/*
	 * Wait for all of the interfaces to close down
	 */

	RSocketServ ss;
	TInt ret=ss.Connect();
	if(ret!=KErrNone)
	    {
		Log(_L("Unable to connect to SocketServer, returned %d"), ret);
		return EFail;
	    }
	
	CleanupClosePushL(ss);

	if (KErrNone == WaitForAllInterfacesToCloseL(ss))
	    {
		result = EPass;
		}
	else
	    {
		result = EFail;
		}

	ss.Close();
	CleanupStack::Pop();

	return result;
}

TInt TS_DummyPPPStep::ReadIniFile(void)
/*
 * Opens the .ini file, reads the contents into some class members
 * and then closes the file again
 * @return system wide error code
 */
{
	TInt result = KErrNone;

	TPtrC filename(_L("ts_dummy.ini"));

	// this opens the file and loads everything in
	LoadConfig(filename);

	// set the IAP values in the class members
	_LIT(KIapSection, "IAP");
	_LIT(KDummyNifIapKey, "DummyPPPNifIapNumber");
	_LIT(KDummyNifLongTimeoutIapKey, "DummyPPPNifLongTimeoutIapNumber");
	_LIT(KNtRasIapKey, "NtRasIapNumber");
	_LIT(KBadNtRasIapKey, "BadNtRasIapNumber");
	_LIT(KMissingNifIapKey, "MissingNifIapNumber");

	if (!(GetIntFromConfig(KIapSection, KDummyNifIapKey, iDummyNifIap)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KIapSection, KDummyNifLongTimeoutIapKey, iDummyNifLongTimeoutIap)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KIapSection, KNtRasIapKey, iNtRasIap)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KIapSection, KBadNtRasIapKey, iBadNtRasIap)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KIapSection, KMissingNifIapKey, iMissingNifIap)))
		result = KErrNotFound;

	// set the timeout values in the class members
	_LIT(KTimeoutSection, "Timeouts");
	_LIT(KShortTimeoutKey, "ShortTimeout");
	_LIT(KMediumTimeoutKey, "MediumTimeout");
	_LIT(KLongTimeoutKey, "LongTimeout");
	
	if (!(GetIntFromConfig(KTimeoutSection, KShortTimeoutKey, iShortTimeout)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KTimeoutSection, KMediumTimeoutKey, iMediumTimeout)))
		result = KErrNotFound;
	if (!(GetIntFromConfig(KTimeoutSection, KLongTimeoutKey, iLongTimeout)))
		result = KErrNotFound;
	
	// other general set up stuff
	_LIT(KGeneralSection, "General");
	_LIT(KEchoPortKey, "EchoPort");
	if (!(GetIntFromConfig(KGeneralSection, KEchoPortKey, iEchoPortNum)))
		result = KErrNotFound;

	return result;
}

TInt TS_DummyPPPStep::OpenSocketServer(RSocketServ& ss)
/* 
 * Opens the socket server that is pointed to
 * @param ss the socket server to open
 * @return system wide error code
 */
    {
	return (ss.Connect());
    }

void TS_DummyPPPStep::CloseSocketServer(RSocketServ& ss)
/*
 * Closes the socket server that is pointed to
 * @param ss the socket server to close
 * @return system wide error code
 */
{
	ss.Close(); // Close() inherited from RHandleBase?
}

TInt TS_DummyPPPStep::OpenConnection(RConnection& conn, RSocketServ& ss)
/*
 * Open the connection using the socket server too
 * @param conn the connection to open
 * @param ss the socket server within which the connection is to be opened
 * @return system wide error code
 */
{
	return (conn.Open(ss));
}

TInt TS_DummyPPPStep::OpenConnection(RConnection& conn, RSocketServ& ss, TName& name)
/*
 * Open the connection (passing the name provided)
 * @param conn - the connection to open
 * @param ss - the socket server within which the connection is to be opened
 * @return system wide error code
 */
{
	return (conn.Open(ss, name));
}

TInt TS_DummyPPPStep::StartConnection(RConnection& conn)
/*
 * Start a connection using the default comm db settings
 * @param conn the connection to start
 * @return system wide error code
 */
{
	TRequestStatus status;
	conn.Start(status);
	User::WaitForRequest(status);
	return status.Int();
}

TInt TS_DummyPPPStep::StartConnectionSynchronous(RConnection& conn)
/*
 * Start a connection using the default comm db settings (using the synchronous call)
 * @param conn - the connection to start
 * @return system wide error code
 */
{
	return (conn.Start());
}

void TS_DummyPPPStep::StartConnectionAsynchronous(RConnection& conn, TRequestStatus& status)
/*
 * Start a connection using the default comm db settings (returning before it completes)
 * @param conn - the connection to start
 * @param system wide error code
 */
{
	conn.Start(status);
}

TInt TS_DummyPPPStep::StartConnectionWithOverrides(RConnection& conn, TInt iap)
/*
 * Start a connection on the IAP as specified by the param, do this by 
 * building some database overrides.
 * @param conn - the connection to start
 * @param iap - which IAP to create the connection over
 * @return system wide error code
 */
{
	TRequestStatus status;
	
	TCommDbConnPref prefs;
	prefs.SetIapId(iap);
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	conn.Start(prefs, status);
	
	User::WaitForRequest(status);

	return status.Int();
}

TInt TS_DummyPPPStep::StartConnectionWithOverridesSynchronous(RConnection& conn, TInt iap)
/*
 * Start a connection on the IAP as specified by the param, do this by building some 
 * database overrides. Use the synchronous start method
 * @param conn - the connection to start
 * @param iap - which IAP to create the connection over
 * @return system wide error code
 */
{
	TCommDbConnPref prefs;
	prefs.SetIapId(iap);
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	return (conn.Start(prefs));
}

void TS_DummyPPPStep::StartConnectionWithOverridesAsynchronous(RConnection& conn, TCommDbConnPref& aPrefs, TInt iap, TRequestStatus& status)
/*
 * Start a connection on the IAP as specified by the param - return before it completes
 * @param conn - the connection to start
 * @param iap - which IAP to create the connection over
 * @return system wide error code
 */
{
	aPrefs.SetIapId(iap);
	aPrefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	conn.Start(aPrefs, status);
}



void TS_DummyPPPStep::CloseConnection(RConnection& conn)
/*
 * Close a connection
 * @param conn the connection to close
 * @return system wide error code
 */
{
	conn.Close();
}


TInt TS_DummyPPPStep::StopConnection(RConnection& conn)
/* 
 * Force the connection to stop and remove the interface no matter what other connections are using it
 * @param conn the connection to stop
 * @return system wide error code
 */
{
	return (conn.Stop());
}

TInt TS_DummyPPPStep::EnumerateConnections(RConnection& conn, TUint& num)
/*
 * Read how many connections (==interfaces?) exist at the moment
 * @param conn - to be used to read the count
 * @param num - on completion holds the number of connections
 * @return system wide error code
 */
{
	return (conn.EnumerateConnections(num));
}

TInt TS_DummyPPPStep::GetTimeoutValues(RConnection& conn, TTimeoutValues &timeouts)
/*
 * Read the timeout values from commdb
 * @param conn - the connection to read the values from
 * @param timeouts - where to put the timeout vals
 * @return system wide error code
 */
{
	TInt ret = KErrNone;
	TInt tmp;
	if (KErrNone != (tmp = conn.GetIntSetting(_L("ModemBearer\\LastSessionClosedTimeout"), timeouts.iShortTimeout)))
		ret = tmp;
	if (KErrNone != (tmp = conn.GetIntSetting(_L("ModemBearer\\LastSocketClosedTimeout"), timeouts.iMediumTimeout)))
		ret = tmp;
	if (KErrNone != (tmp = conn.GetIntSetting(_L("ModemBearer\\LastSocketActivityTimeout"), timeouts.iLongTimeout)))
		ret = tmp;

	return ret;
}

TInt TS_DummyPPPStep::AttachNormal(RConnection& conn, const TDesC8& info)
/*
 * Attach the connection supplied to the one described by the info
 * @param conn - the connection to be attached to something
 * @param info - the details of the connection to attach to
 * @return system wide error code
 */
{
	return conn.Attach(info, RConnection::EAttachTypeNormal);
}

TInt TS_DummyPPPStep::AttachMonitor(RConnection& conn, const TDesC8& info)
/*
 * Attach the connection supplied to the one described by the info (as a monitor)
 * @param conn - the connection to be attached to something
 * @param info - the details of the connection to attach to
 * @return system wide error code
 */
{
	return conn.Attach(info, RConnection::EAttachTypeMonitor);
}

void TS_DummyPPPStep::ProgressNotification(RConnection& conn, TRequestStatus& status, 
											  TNifProgressBuf& progress, TUint aSelectedProgress)
/*
 * Request notification when the progress changed to that supplied
 * @param conn - the connection we are interested in the progress of
 * @param status - the status variable which should be updated eventually
 * @param aSelectedProgress - the progress we want
 * @return system wide error code
 */
{
	conn.ProgressNotification(progress, status, aSelectedProgress);
}

void TS_DummyPPPStep::ProgressNotification(RConnection& conn, TRequestStatus& status, 
											  TNifProgressBuf& progress)
/*
 * Request notification of the next progress change
 * @param conn - the connection whose status we are interested in
 * @param status - the status variable that should be updated
 * @return system wide error code
 */
{
	conn.ProgressNotification(progress, status);
}

void TS_DummyPPPStep::CancelProgressNotification(RConnection& conn)
/*
 * Cancel the notification of progress changes
 * @param conn - the connection to cancel progress notification for
 * @return system wide error code
 */
{
	conn.CancelProgressNotification();
}

TInt TS_DummyPPPStep::Progress(RConnection& conn, TNifProgress& progress)
/*
 * Read the current progress state
 * @param conn - the connection to read the progress of
 * @param progress - where to put the read progress
 * @return system wide error code
 */
{
	return conn.Progress(progress);
}

TInt TS_DummyPPPStep::LastProgressError(RConnection& conn, TNifProgress& progress)
/*
 * Get the last progress error
 * @param conn - the connection we are interested in
 * @param progress - where to put the progress value (the error)
 * @return system wide error code
 */
{
	return conn.LastProgressError(progress);
}

TInt TS_DummyPPPStep::GetConnectionInfo(RConnection& conn, TUint index, TDes8& aConnectionInfo)
/*
 * Read back the information about a particular connection (using the enumeration functionality)
 * @param conn - the connection to use to access the info
 * @param index - the index of the connection we want to find out about
 * @param aConnectionInfo - where to write the info into
 * @return system wide error code
 */
{
	return conn.GetConnectionInfo(index, aConnectionInfo);
}

TInt TS_DummyPPPStep::GetInfo(RConnection& conn, TPckgBuf<TConnectionInfo>& info)
/*
 * Read the info about this connection from comm db
 * @param conn - the connection to read info about
 * @param info - where to write the info to
 * @return system wide error code
 */
{
	TInt ret = KErrNone;
	ret = conn.GetIntSetting(_L("IAP\\Id"), info().iIapId);
	if (ret != KErrNone)
		return ret;
	return conn.GetIntSetting(_L("IAP\\IAPNetwork"), info().iNetId);
}

TInt TS_DummyPPPStep::OpenUdpSocketL(RSocket& sock, RSocketServ& ss)
/*
 * Open a UDP socket within the supplied socket server. No connection is 
 * specified so let the system implicitly decide which to use
 * @param sock - the socket to open
 * @param ss - the socket server
 * @return system wide error code
 */
{
	TInt err = KErrNone;
	TInt ret = KErrNone;
	
	err = sock.Open(ss, KAfInet, KSockDatagram, KProtocolInetUdp);
	TESTEL(KErrNone == ret, ret);
	if (err != KErrNone)
		ret = err;
	
	err = sock.SetOpt(KSoReuseAddr, KSolInetIp, 1);
	TESTEL(err == KErrNone, err);
	if (err != KErrNone)
		ret = err;

	err = sock.SetLocalPort(iEchoPortNum); 
	TESTEL(err == KErrNone, err);
	if (err != KErrNone)
		ret = err;

	return ret;
}

TInt TS_DummyPPPStep::OpenUdpSocketExplicitL(RSocket& sock, RSocketServ& ss, RConnection& conn)
/*
 * Open a UDP socket with the supplied socket server. Associate it 
 * with the specified connection.
 * @param sock - the socket to open
 * @param ss - the socket server
 * @param conn - the connection to associate it with
 * @return system wide error code
 */
{
	TInt err;
	TInt ret = KErrNone;
	err = sock.Open(ss, KAfInet, KSockDatagram, KProtocolInetUdp, conn);
	TESTEL(KErrNone == err, err);
	if (err != KErrNone)
		ret = err;

	err = sock.SetOpt(KSoReuseAddr, KSolInetIp, 1); 
	TESTEL(err == KErrNone, err);
	if (err != KErrNone)
		ret = err;

	err = sock.SetLocalPort(iEchoPortNum); 
	TESTEL(err == KErrNone, err);
	if (err != KErrNone)
		ret = err;
	
	return ret;
}

void TS_DummyPPPStep::DestroyUdpSocket(RSocket& sock)
/*
 * Destroys the supplied socket
 * @param sock - the socket to destroy
 * @return system wide error code
 */
{
	sock.Close();
}



TInt TS_DummyPPPStep::TestUdpDataPath(RSocket& sock, TSockAddr& dest)
/*
 * Should send data out and receive it back
 */
{
	TRequestStatus status;

	TBuf8<KBufferLength> buffer;

	// build some data to send on the socket
	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) 0x08; // ICMP type = 8
	buffer[1] = (TUint8) 0x00; // ICMP code = 0
	buffer[2] = (TUint8) 0xF7; // ICMP checksum high byte
	buffer[3] = (TUint8) 0xFF; // ICMP checksum low byte
	buffer[13] = (TUint8) 0xFE;
	// NB the rest of the buffer is zero
	// hence the checksum (0xFFFF - 0x800) since 0x8
	// is the only non-zero element of the buffer

	// send the data out over the socket
	sock.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);

	if (status!= KErrNone)
		return status.Int();

	buffer.Zero();
	// I expect to get the data looped back from the dummy NIF
	sock.RecvFrom(buffer, dest, 0, status);
	User::WaitForRequest(status);

	// don't use TESTEL here as the test case may be expecting us to fail and that's ok...

	if (status != KErrNone)
		return status.Int();
	else
	{   // check that what we sent is what we got back
		// if the receive times out and we access buffer we get a panic
		TEST(buffer[0] == 0x08);
		TEST(buffer[1] == 0x00);
		TEST(buffer[2] == 0xF7);
		TEST(buffer[3] == 0xFF);
	}

	return KErrNone;
}


TInt TS_DummyPPPStep::NumberOfInterfacesL(RSocketServ& ss)
/*
 * Return a count of interfaces in this socket server.
 * Can be used even if this socket server has no open connections (because it 
 * temporarily opens one!).
 * @param ss - the interesting socket server to count in
 * @return number of interfaces, or it might leave
 */
{
	TInt err;
	RConnection conn;
	TUint numOfConnections;

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);

	CloseConnection(conn);
	CleanupStack::Pop();

	return numOfConnections;
}

TInt TS_DummyPPPStep::TimeUntilRequestComplete(TRequestStatus& status, TTimeIntervalSeconds& timeElapsed)
/*
 * Wait for the status to change. Count how long it takes.
 * @param status - the status which we need to wait for it to change
 * @param timeElapsed - the time it took (in seconds)
 * @return system wide error code
 */
{
	TTime time1, time2;

	time1.HomeTime();

	User::WaitForRequest(status);
		
	time2.HomeTime();
	time2.SecondsFrom(time1, timeElapsed);

	return (status.Int());
}

TInt TS_DummyPPPStep::RequestInterfaceDownL(RConnection& conn, RSocketServ& ss)
{
	TInt err = KErrNone;
	TRequestStatus status;

	TBuf8<KBufferLength> buffer;

	RSocket sock;
	TInetAddr dest;

	err = OpenUdpSocketExplicitL(sock, ss, conn);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(sock);

	//err = sock.SetLocalPort(KDummyNifCmdPort);
	//TESTEL(KErrNone == err, err);

	dest.SetAddress(KDummyPppNifLocalAddressBase + 4);
	dest.SetPort(KDummyPppNifCmdPort);

	buffer.SetMax();
	buffer.FillZ();
	buffer[0] = (TUint8) KForceDisconnect;

	sock.SendTo(buffer, dest, 0, status);
	User::WaitForRequest(status);
	TESTEL(KErrNone == status.Int(), status.Int());

	buffer.Zero();
	// I expect some sort of response
	// this fails - possibly ok as the interface might have gone already??
	//sock.RecvFrom(buffer, dest, 0, status);
	//User::WaitForRequest(status);
	//TESTEL(KErrNone == status.Int(), status.Int());

	DestroyUdpSocket(sock);
	CleanupStack::Pop();

	return err;
}

TInt TS_DummyPPPStep::WaitForAllInterfacesToCloseL(RSocketServ& ss)
/*
 * Sit around waiting for all the interfaces to die (essentially poll every now 
 * and again until there are zero interfaces in this socket server left)
 * @return system wide error code
 */
{
	TInt err;
	TUint numOfConnections;
	TUint count = 0;

	RConnection conn;

	err = OpenConnection(conn, ss);
	TESTEL(KErrNone == err, err);
	CleanupClosePushL(conn);

	err = EnumerateConnections(conn, numOfConnections);
	TESTEL(KErrNone == err, err);

	while ((0 != numOfConnections) && (count < 60))
	{
		count++;
		User::After(1000000); // wait a bit, a second sounds good
		err = EnumerateConnections(conn, numOfConnections);
	}

	CloseConnection(conn);
	CleanupStack::Pop();

	if (0 == numOfConnections)
		return KErrNone;

	// Note: adding an "else" to the previous "if" seems to generate an "unreachable code" error in WinS
	return KErrTimedOut;
}
