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
// This code implements a simple ip echo server over TCP.
// Its goal is to point out the symbian socket transfer mechanism.
// This program tries to get an accepted connection throught its
// arguments. This implies it has been called and passed a valid
// connected socket.
// Then it receives packets it sends them back until the connection
// is closed.
// Note: only connected mode implemented
// 
//

#include <e32base.h>
#include <e32cons.h>
#include <es_sock.h>
#include <in_sock.h>

LOCAL_D CConsoleBase* console;

// Main 
void MainL()
	{
	
	// This is the slot used to transfer the socket.
	// It has been defined by our own protocol.
	const TInt KSocketParameterSlot = 8;

	
	console->Printf(_L("IP-Echo\n"));

	// Wait for key
	console->Printf(_L("[ press any key ]\n"));
	console->Getch();	// Get and ignore character

	/* Parse command line 
	Enable us to determine some options
	Note: Not implemented in this test case.
	*/
	

	/* Connection part 
	Manage to see if we have a handle waiting for us
		==> means we have a socket or data already
	Options: Enable waiting mode on our own as a
	standalone ip echo server.
	*/ 
	
	// Connect to socket server
	console->Printf(_L("Connect RSocketServ ... "));
	RSocketServ socketServ;
	CleanupClosePushL(socketServ);
	User::LeaveIfError(socketServ.Connect());
	console->Printf(_L("OK\n"));
	
	
	// Note: Not implemented in this test case
	// RConnection creation <== only when we do not grab a socket 
	

	// Socket received at creation
	console->Printf(_L("Grab back the waiting socket ... \n"));

	// Get socket name
	console->Printf(_L("Look for name in slot ... "));
	TName passedSocketName;
	TInt ret = User::GetDesParameter(KSocketParameterSlot, passedSocketName);
	if(ret != KErrNone)
		{
		if(ret == KErrNotFound)
			{
			console->Printf(_L("Not found\n"));
			}
		else
			{
			console->Printf(_L("User::GetDesParameter(%d, passedSocketName) ==> ret %d\n"), KSocketParameterSlot, ret);
			User::LeaveIfError(ret);
			}
		}
	console->Printf(_L("Found \n"));

	
	/* Transfer Part 
	Do transfert 
	*/
	console->Printf(_L("Transfer socket %S... "), &passedSocketName);
	RSocket socket;
	User::LeaveIfError(socket.Transfer(socketServ, passedSocketName));
	console->Printf(_L("OK\n"));

	// Signal Rendez-vous
	// RProcess::Rendezvous(KErrNone); <== Should be here, but we will use it later for test
	
	// Verify handle on socket after transfer
	console->Printf(_L("Check socket transfer ... "));
	TProtocolDesc protocol;
	ret = socket.Info(protocol);
	TBool result = ETrue;
	switch(ret)
		{
		case KErrBadHandle:
			console->Printf(_L("Failed\n"));
			result = EFalse;
			break;

		case KErrNone:
			console->Printf(_L("Succeed\n"));
			result = ETrue;
			break;

		default:
			console->Printf(_L("RSocket::Info(protocol) ==> Error %d\n"), ret);
			User::LeaveIfError(ret);
		}

	if(result)
		{
		/* Grab data if necessary 
		Concern connected mode only.
		Note: only connected mode implemeted in this test case.
		*/ 

		CleanupClosePushL(socket);
		
		// Ask for receive
		console->Printf(_L("Receive data from socket ... "));
		TBuf8<KSocketDefaultBufferSize> buffer;
		TRequestStatus status;
		buffer.Zero();
		socket.Recv(buffer, 0, status);	// We use Recv, so we will wait until buffer is full.
						// Maybe, RecvOneOrMore would be better in real situation.
		RProcess::Rendezvous(KErrNone); // Rendez-vous here for test
		User::WaitForRequest(status);
		ret = status.Int();
		
		while(ret != KErrEof)	// Loop while socket active
			{
			if(ret != KErrNone)
				{
				console->Printf(_L("RSocket::Recv(buffer, 0, status) ==> Error %d\n"), ret);
				User::LeaveIfError(ret);
				}
			console->Printf(_L("OK\n"));

			/* Identify data 
			Look at the IP header
			Determine if we should discard the packet
			Do manipulation on the packet
			Send it back
			 *
			It is where the complete RFC 2075 should be implemented.
			
			In this test case we simply decremented the ttl.
			We did not check if it was a icmp packet, nor do we
			look at the ip header history.
			*/ 
			console->Printf(_L("Send packet back ...\n"));

			// Decrement TTL
			console->Printf(_L(" ... Decrement ttl ... "));
			TInt ttl;
			User::LeaveIfError(socket.GetOpt(KSoIpTTL, KSolInetIp, ttl));
			User::LeaveIfError(socket.SetOpt(KSoIpTTL, KSolInetIp, ttl - 1));
			console->Printf(_L("OK\n"));

			// Send packet back
			console->Printf(_L(" ... Send data to socket ... "));
			socket.Send(buffer, 0, status);
			User::WaitForRequest(status);
			User::LeaveIfError(status.Int());
			console->Printf(_L("OK\n"));
			
			// Grab next data
			console->Printf(_L("Receive packet ... "));
			buffer.Zero();
			socket.Recv(buffer, 0, status);	// We use Recv, so we will wait until buffer is full.
							// Maybe, RecvOneOrMore would be better in real situation.
			User::WaitForRequest(status);
			ret = status.Int();
			}
		console->Printf(_L("Received EOF\n"));
	
		/* Closing part 
		Close all opened stuff
		Options: Look here for daemon mode
		*/ 
		CleanupStack::PopAndDestroy(static_cast<TAny*>(&socket));
		}

	// Close socket server
	CleanupStack::PopAndDestroy(static_cast<TAny*>(&socketServ));
	}

// Console harness
void ConsoleMainL()
	{
	
	// Get a console
	console = Console::NewL(_L("IP-Echo"), TSize(KConsFullScreen, KConsFullScreen));
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
	__ASSERT_ALWAYS(!ret, User::Panic(_L("IP-Echo"), ret));
	delete cleanupStack;
	__UHEAP_MARKEND;
	return 0;
	}
