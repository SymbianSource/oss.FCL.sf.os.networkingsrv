// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// August 2004
// This is a tiny program to put inetd and ipecho implementations
// in action.
// It will launch the inetd program, then try to connect to the ipecho
// server (which will go throught inetd for the hand shaking) and
// finally do a send//receive of a packet.
// It is all user interactive in order to avoid any race conditions between
// processes and keep the code as simple as possible.
// Here is the steps to have it running properly:
// 1. Launch inetdtest.exe
// It will open two windows: inetdtest with a "Press any key" message
// and inetd.
// 2. Inetd should run, so wait for: "Start ..." 
// or any error message because inetdtest cannot continue until inetd 
// is fully launched. (Normally, you would control this by using Rendezvous).
// 3. Once inetd is launched, press a key on inetdtest window.
// This will try to connect through inetd to port 7 (ipecho). It will normally
// launch a third window which is the ipecho program, result of inetd
// execution.
// 4. Press one key on the ipecho window in order to realise the socket transfer.
// 5. Press one key on the inetdtest window in order to send a packet and wait
// for its echo.
// 6. Everything is over now and you should have this:
// - "Press any key" on the ipecho window. If you press it, it will close ipecho.
// - Still "Start ... " on the inetd window or "Press any key". Inetd has been
// implemented in order to stop after a while and produce a "Press any key"
// to terminate. (Note: it can cause error with inetdtest if you are not 
// fast enough to execute all those steps).
// Once you've press any key, it will close inetd.
// - "Press any key" on the inetdtest window. If you press it, it will close
// all three programs.
// 
//

#include <e32base.h>
#include <e32cons.h>
#include <es_sock.h>
#include <in_sock.h>
#include <commdbconnpref.h>

LOCAL_D CConsoleBase* console;

// Main 
void MainL()
	{
	/* Inetd Setup 
	Create a inetd process 
	*/
	RProcess inetd;
	CleanupClosePushL(inetd);
	User::LeaveIfError(inetd.Create(_L("inetd"), _L("")));
	
	// Resume inetd
	inetd.Resume();
	
	// Wait for inetd to be ready
	console->Printf(_L("[ press any key ]"));
	console->Getch();	// Get and ignore character

	
	/* IPEcho Part 
	Connect to socket server 
	*/
	console->Printf(_L("Connect to socket server ... \n"));
	RSocketServ socketServ;
	CleanupClosePushL(socketServ);
	User::LeaveIfError(socketServ.Connect());
	
	// Open RConnection
	console->Printf(_L("Open RConnection ... \n"));
	RConnection connect;
	CleanupClosePushL(connect);
	User::LeaveIfError(connect.Open(socketServ, KConnectionTypeDefault));

	// Start RConnection
	console->Printf(_L("Start RConnection ... \n"));
	TCommDbConnPref prefs;
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	User::LeaveIfError(connect.Start(prefs));	// Handle options here like (ie: No Prompt)

	// Open RSocket
	console->Printf(_L("Open Socket ... \n"));
	RSocket sock;
	CleanupClosePushL(sock);
	User::LeaveIfError(sock.Open(socketServ, KAfInet, KSockStream, KProtocolInetTcp, connect));

	// Connect
	console->Printf(_L("Connect Socket ... \n"));
	TRequestStatus status;
	TInetAddr echoServAddr(KInetAddrLoop, 7);	// 7: TCP port 
	sock.Connect(echoServAddr, status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());

	// Wait for the socket transfer to be over 
	console->Printf(_L("[ press any key ]"));
	console->Getch();	// Get and ignore character
	
	// Send a packet
	console->Printf(_L("Send a packet ... \n"));
	HBufC8* packet = HBufC8::NewL(KSocketDefaultBufferSize);
	CleanupStack::PushL(packet);

	packet->Des().SetMax();
	packet->Des().FillZ();
	sock.Send(packet->Des(), 0, status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());
	
	// Receive it back
	console->Printf(_L("Received it back ... \n"));
	packet->Des().Zero();
	TPtr8 buff = packet->Des();
	sock.Recv(buff, 0, status);
	User::WaitForRequest(status);
	User::LeaveIfError(status.Int());

	
	// Terminates 
	console->Printf(_L("Terminates ... \n"));
	CleanupStack::PopAndDestroy(packet);
	CleanupStack::PopAndDestroy(&sock);
	CleanupStack::PopAndDestroy(&connect);
	CleanupStack::PopAndDestroy(&socketServ);
	CleanupStack::PopAndDestroy(&inetd);
	}

// Console harness
void ConsoleMainL()
	{
	
	// Get a console
	console = Console::NewL(_L("Inetdtest"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(console);

	// Call function
	MainL();

	// Wait for key
	console->Printf(_L("[ press any key ]"));
	console->Getch();	// Get and ignore character

	// Finished with console
	CleanupStack::PopAndDestroy();	// Console
	}

// Cleanup stack harness
GLDEF_C TInt E32Main()
	{
	
	__UHEAP_MARK;
	CTrapCleanup* cleanupStack = CTrapCleanup::New();

	TRAPD(ret, ConsoleMainL());
	__ASSERT_ALWAYS(!ret, User::Panic(_L("Inetd"), ret));
	delete cleanupStack;
	__UHEAP_MARKEND;
	return 0;
	}
