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
// This code implements a really simple inetd program.
// Its purpose is to enlight symbian socket transfer principle.
// It uses a configuration file: inetd.conf (place in '\System\Data\'
// for this test case). (A)
// From it, it creates listeners (active objects), each one 
// representing a listening connection as specified in the configuration
// file. (B)
// When a connection is accepted, the listener will launch the appropriate
// program and passed it to the connected socket. (C)
// (A)  | inetd.conf |
// |               (B)               +-----------+      (C)     +-----------+
// |  ---------------------------->  | CListener |  --------->  | CTransfer |
// |	                                      |	        (C)     +-----------+
// |	                                      | ------------->  | CTransfer |
// |	                                      |         (C)
// |	              (B)               +-----------+   (C)     +-----------+
// |  ---------------------------->     | CListener |  ------>  | CTransfer |
// |	                                      |         (C)
// This implementation is not about efficiency as we create a new
// process for each accepted connection. We could have coded it in
// other ways like by using threads or active objects but we decided
// to keep the test simple in order to clearly show the
// RSocket::Transfer(...) method.
// In the case of a real and used inetd implementation, it would be
// definitly better to improve the application launching for simple
// services like ipecho.
// In order to do this, the use of either Dlls, call to other programs or
// code inside inetd would have to be defined and thought in relation with
// the service purpose.
// For example, it would be more efficient to have the ipecho code inside 
// inetd as an active object, because it is simple and not a lot used. By
// doing so we will save ressources in avoiding the creation of another
// process.
// But in the case of a web server, it would be better to have one process
// launched the first time and then pass to it connections throught a 
// client//server messaging protocol defined for this purpose.
// 
//

#include <e32base.h>
#include <e32cons.h>
#include <e32std.h>
#include <es_sock.h>
#include <in_sock.h>
#include <e32cmn.h>
#include <f32file.h>
#include <s32file.h>
#include <commdbconnpref.h>
#include <clistener.h>

LOCAL_D CConsoleBase* console;


_LIT(KConfigFileName, "inetd.conf");		// Configuration file name.
_LIT(KConfigFileDir, "\\System\\Data\\");	// Configuration file dir (Note: Should look at
						// platform security structure in real
						// implementation).
						// It has been defined by our own protocol.
const TUint KRunTime = 50000000;		// Time before server will stop. (test option).
const TInt KNbListeners = 25;			// Number of listeners. Typically, it represents
						// the number maximum of services our inetd can handle.
						// for each listener.
const TInt KLineSize = 100;			// Size of a line in the configuration file.

/** Object from this class will be used in test case to stop the
active scheduler after a time KRunTime;
 */
class CInetdExitTimer : public CTimer
	{
	public:
		static CInetdExitTimer* NewL();

	protected:
		CInetdExitTimer();
		void ConstructL();
		void RunL();
	};


// Function to read configuration file and create listeners
GLDEF_C void ReadConfigFileL(	RFile& aConfigFile, 
				TListeners& aListeners, 
				RSocketServ& aSocketServ, 
				RConnection& aConnect);

// Function to delete a RPointerArray correctly if we leave
LOCAL_C void DestroyPointerArray(RPointerArray<CListener>* aListeners)
	{
	aListeners->ResetAndDestroy();
	aListeners->Close();
	}

// Main 
void MainL()
	{
	/* Arguments: 
	Not implemented in this test case.
	Note: Implement if necessary
	 */ 
	
	
	/* Connection part: 
	Here we want to open all requirements:
		- RSocketServer connection
		- RSocket
	We also want to start RConnection
	 */ 
	
	// Connect to socket server
	console->Printf(_L("Connect RSocketServ ... "));
	RSocketServ socketServ;
	CleanupClosePushL(socketServ);
	User::LeaveIfError(socketServ.Connect());
	console->Printf(_L("OK\n"));
	
	// Open RConnection
	console->Printf(_L("Open RConnection ... "));
	RConnection connect;
	CleanupClosePushL(connect);
	User::LeaveIfError(connect.Open(socketServ, KConnectionTypeDefault));
	console->Printf(_L("OK\n"));

	// Start RConnection
	console->Printf(_L("Start RConnection ... "));

	/* In a real inetd you would probably want to avoid the IAP prompting.
	You could do it this way:
	
	TCommDbConnPref prefs;
	prefs.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);
	User::LeaveIfError(connect.Start(prefs));	// Handle options here (ie: No Prompt)

	*/
	User::LeaveIfError(connect.Start());
	console->Printf(_L("OK\n"));


	/* Configuration file:
	We read configuration file.
	We will create a listener for each line.
	It will listen and accept connection for the
	specified protocol, then it will launch 
	the appropriate program and continue 
	to listen.
	
	Our basic inetd.conf file is:
		==> ServiceName: Contains the name of an Internet service.
		==> SocketType: Specifies which socket to use.
		==> ProtocolName: Contains the name of an Internet protocol.
		==> ProtocolPort: Specifies which port to use.
		==> ServerName: Contains the name of program to launch.
		==> ServerArgs: Contains program arguments.
	 */
	
	// Connect to file server
	console->Printf(_L("Connect to file server ... "));
	TListeners listeners;
	CleanupStack::PushL(TCleanupItem(TCleanupOperation(DestroyPointerArray), &listeners));
	RFs fileServ;
	CleanupClosePushL(fileServ);
	User::LeaveIfError(fileServ.Connect());
	console->Printf(_L("OK\n"));

	// Search file
	TFindFile ff(fileServ);
	TInt ret = ff.FindByDir(KConfigFileName, KConfigFileDir);
	if(ret != KErrNone)
		{
		if(ret == KErrNotFound)
			{
			console->Printf(_L("Can't find file '%S'\n"), &KConfigFileName);
			}
		else
			{
			User::LeaveIfError(ret);
			}
		}
	else
		{
		// File Open
		console->Printf(_L("Open file ... "));
		RFile configFile;
		CleanupClosePushL(configFile);
		ret = configFile.Open(fileServ, ff.File(), EFileRead);
		if(ret != KErrNone)
			{
			if(ret == KErrNotFound)
				{
				console->Printf(_L("Can't open file '%S'\n"), &KConfigFileName);
				}
			else
				{
				User::LeaveIfError(ret);
				}
			}
		else
			{
			console->Printf(_L("OK\n"));

			// Read File (Here, we read the whole file as we know it's small. Not optimal)
			ReadConfigFileL(configFile, listeners, socketServ, connect);
			}

		// We have Finished with file
		CleanupStack::PopAndDestroy(&configFile);
		}

	// We have finished with file server
	CleanupStack::PopAndDestroy(&fileServ);
	
	// Test here for the number of listeners or any config file error.
	// If 0, then we've got no configuration in file
	console->Printf(_L("NbListeners: %d\n"), listeners.Count());

	if(listeners.Count() != 0)
		{
		/* Launch listeners 
		Start the active scheduler
		 */
		console->Printf(_L("Start ... \n"));
		// Set up timer to stop inetd (Test case implementation)
		CInetdExitTimer* timeToRun = CInetdExitTimer::NewL();
		CleanupStack::PushL(timeToRun);
		timeToRun->After(KRunTime);

		// Start active scheduler for listeners to accept
		CActiveScheduler::Start();

		// End of Inetd, triggered by CInetdExitTimer::RunL()
		console->Printf(_L("Stop\n"));

		CleanupStack::PopAndDestroy(timeToRun);	// Delete timeToRun
		}

	/* Destroy listeners 
	Cancel all pending listeners
	Delete them
	 */
	console->Printf(_L("Delete listeners ... "));
	CleanupStack::PopAndDestroy(&listeners);
	console->Printf(_L("OK\n"));
	
	// Close all opened stuff
	console->Printf(_L("Closing ... "));	
	CleanupStack::PopAndDestroy(&connect);
	CleanupStack::PopAndDestroy(&socketServ);
	console->Printf(_L("End\n"));
	}

// Console harness
void ConsoleMainL()
	{
	// Get a console
	console = Console::NewL(_L("Inetd"), TSize(KConsFullScreen, KConsFullScreen));
	CleanupStack::PushL(console);

	// Call function
	MainL();

	// Wait for key
	console->Printf(_L("[ press any key ]"));
	console->Getch();	// Get and ignore character

	// Finished with console
	CleanupStack::PopAndDestroy(console);	// Console
	}

// Cleanup stack harness
GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* cleanupStack = CTrapCleanup::New();

	CActiveScheduler* activeScheduler = new CActiveScheduler;
	CActiveScheduler::Install(activeScheduler);

	TRAPD(ret, ConsoleMainL());
	__ASSERT_ALWAYS(!ret, User::Panic(_L("Inetd"), ret));
	delete activeScheduler;
	delete cleanupStack;
	__UHEAP_MARKEND;
	return 0;
	}

// Function to read a line from a buffer
GLDEF_C void ReadConfigFileL(RFile& aConfigFile, TListeners& aListeners, RSocketServ& aSocketServ, RConnection& aConnect)
	{
	TInt fileLen;
	User::LeaveIfError(aConfigFile.Size(fileLen));
	HBufC8* readBuffer = HBufC8::NewL(fileLen);	
	CleanupStack::PushL(readBuffer);
	TPtr8 buff = readBuffer->Des();

	// Read File
	// Here, we read the whole file as we know it's small
	TRequestStatus status;
	aConfigFile.Read(buff, status);
	User::WaitForRequest(status);
	if(status.Int() != KErrNone && status.Int() != KErrEof)
		{
		User::LeaveIfError(status.Int());
		}

	HBufC8* tempBuffer = HBufC8::NewL(KLineSize);
	CleanupStack::PushL(tempBuffer);
	TPtr8 lineBuff = tempBuffer->Des();
	TLex8 lex;
	lex.Assign(buff);

	/* Parse whole stream in to split it in lines
	We discard commented line with #
	 */
	TBool error = EFalse;
	while(!lex.Eos() && !error)
		{
		TBool comment = EFalse;
		if(lex.Peek() == '#')
			{
			// We've got a comment
			comment = ETrue;
			}
		
		TInt nbCharRead = 0;	// To count number of character per line. Used to avoid a buffer overflow
		while(!error && !lex.Eos() && lex.Peek() != '\r' && lex.Peek() != '\n')
			{
			// We look if we are allowed to append character. Otherwise we stop loopping.
			if(nbCharRead < KLineSize)
				{
				lineBuff.Append(lex.Get());
				nbCharRead++;
				}
			else
				{
				error = ETrue;
				}
			}
		if(!comment)
			{
			// We create a new listener
			TInt nbListeners = aListeners.Count();
			if(nbListeners < KNbListeners)
				{
				aListeners.Append(CListener::NewL(aSocketServ, aConnect, CListenerOptions::NewL(lineBuff))); 
				aListeners[nbListeners]->AcceptL();
				}
			}
		lineBuff.Zero();
		if(lex.Peek() == '\r')
			{
			lex.Get();	// Get rid of \r
			}
		lex.Get();	// Get rid of \n
		}
	
	// Delete buffers
	CleanupStack::PopAndDestroy(tempBuffer);	
	CleanupStack::PopAndDestroy(readBuffer);

	if(error)
		{
		// We have a bad line in our configuration file so we delete all listeners
		aListeners.ResetAndDestroy();
		}
	}

//
// Classes Implementations //
//


//------------------------------------------------------------------------------//
// --- Class CInetdExitTimer ---------------------------------------------------//
//------------------------------------------------------------------------------//

CInetdExitTimer::CInetdExitTimer() : CTimer(EPriorityStandard)
// 
// C'tor
//
	{
	}

CInetdExitTimer* CInetdExitTimer::NewL()
//
// Create a new waiter
// 
	{
	CInetdExitTimer* self = new(ELeave) CInetdExitTimer;
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CInetdExitTimer::ConstructL()
// 
// Symbian second construction step
//
	{
	CTimer::ConstructL();
	CActiveScheduler::Add(this);
	}

void CInetdExitTimer::RunL()
//
// We want to stop inetd, so we stop the active scheduler
//
	{
	CActiveScheduler::Stop();
	}
