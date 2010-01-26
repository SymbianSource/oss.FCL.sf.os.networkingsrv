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
//

#include <in_sock.h>
#include <c32comm.h>
#include <e32base.h>
#include <es_sock.h>
#include <tftpeng.h>
#include <nifman.h>

// Device driver names
#ifndef __EPOC32__
#define PDD_NAME _L("ECDRV")
#define LDD_NAME _L("ECOMM")
#else
#define PDD_NAME _L("EUART1")
#define LDD_NAME _L("ECOMM")
#endif

LOCAL_C void InitCommsL();
LOCAL_C void ProgramL();

//temporary const for TFTP server on own machine
_LIT(KInetTempAddr, "10.159.24.32");
//const for file to be moved
_LIT(KFileToMove, "test.cpp");

class CCommandParser: public CAsyncOneShot, public MTftpNotifier 
	{

public:
	~CCommandParser();
	static CCommandParser* NewL(CConsoleBase& aConsole);

protected:
    CCommandParser(CConsoleBase& aConsole);
    void ConstructL(); 
    virtual void RunL();
	virtual void OpComplete(TOpComp aResult, TInt aOp);
	virtual void ProgressNotification();
private:
	CConsoleBase& iConsole;
	CTftpEngine* iEngine;
    };

GLDEF_C TInt E32Main()
	{
	__UHEAP_MARK;
	CTrapCleanup* trap = CTrapCleanup::New();
	if(trap==NULL)
		return KErrNoMemory;

	TInt err = KErrNone;

	TRAP(err, ProgramL());

	delete trap;
	__UHEAP_MARKEND;
	return err;
	}

//
// Set up Epoc environment. Put in separate function so we can
// let heap allocations fail and leave, trapping them all in 
// one place above.
//
LOCAL_C void ProgramL()
	{
	CActiveScheduler* pS = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(pS);

	//initialization for COMM port (TFTP server on comm2)
	InitCommsL();
	
    CActiveScheduler::Install(pS);
	//create console for output
	CConsoleBase* console = Console::NewL(_L("TFTP"),TSize(KConsFullScreen,KConsFullScreen));
	CleanupStack::PushL(console);
	CCommandParser* compars= CCommandParser::NewL(*console);
	compars->Call();
	   
	CActiveScheduler::Start();

	delete compars;
	CleanupStack::PopAndDestroy(console);
	CleanupStack::PopAndDestroy(pS);
	}

LOCAL_C void InitCommsL()
//
// Initialisation code - loads the serial LDD and PDD
// starts the comm subsystem (for EPOC32 builds)
// On a full EPOC implementation, this code would not
// be required because higher level components (EIKON) 
// automatically start the services.
//
	{
	// Load the physical device driver
	// The OS will automatically append .PDD and 
	// search /System/Libs on all drives.
	TInt r=User::LoadPhysicalDevice(PDD_NAME);
	if (r != KErrNone && r!= KErrAlreadyExists)
		User::Leave(r);
	//test(r==KErrNone || r==KErrAlreadyExists);

	// Similarly for the Logical device driver
	r=User::LoadLogicalDevice(LDD_NAME);
	if (r != KErrNone && r != KErrAlreadyExists)
		User::Leave(r);
	//test(r==KErrNone|| r==KErrAlreadyExists);

//	User::LeaveIfError(Nifman::CheckIniConfig());
	}

CCommandParser::~CCommandParser()
//
// C++ D'tor
//
    {
	delete iEngine;
	}

CCommandParser::CCommandParser(CConsoleBase& aConsole)
    : CAsyncOneShot(0), iConsole(aConsole)
//
// C++ C'tor
//
    {
    }

void CCommandParser::ConstructL()
//
// E32 C'tor
//
    {
	iEngine = CTftpEngine::NewL(0, *this); 
    }

void CCommandParser::RunL()
//
// Run method
//
    {
	iConsole.Printf(_L("TFTP>"));
	TBuf<0x100> command;
	TKeyCode key;
	while((key=iConsole.Getch())!=EKeyEnter)
		{
		if(command.Length()>=0x100)
			User::Beep(440, 500000);
		else if(key == EKeyBackspace)
			{
			if(command.Length())
				{
				iConsole.Printf(_L("%c"), key);
				command.SetLength(command.Length()-1);
				}
			}
		else
			{
			iConsole.Printf(_L("%c"), key);
			command.Append(TChar(key));
			}
		}

	iConsole.Printf(_L("\n Command %S\n"), &command);
	if(!command.CompareF(_L("quit")))
		CActiveScheduler::Stop();
	else if(!command.CompareF(_L("get")))

		iEngine->Get(KInetTempAddr,KFileToMove);
	else if(!command.CompareF(_L("put")))
		iEngine->Put(KInetTempAddr,KFileToMove);
	else
		{
		iConsole.Printf(_L("Unknown Function\n"));
		Call();
		}
	}

void CCommandParser::OpComplete(TOpComp aResult, TInt aOp)
//
// Operation completed
//
	{
	iConsole.Printf(_L("\nOperation %d completed %d\n"), aOp, aResult);
	Call();
	}

void CCommandParser::ProgressNotification()
//
// Progress indication
//
	{
	iConsole.Printf(_L("."));
	}	

CCommandParser* CCommandParser::NewL(CConsoleBase& aConsl)
//
// Create and connect client with leaving
//
    {
    CCommandParser* p = new (ELeave) CCommandParser(aConsl);
    CleanupStack::PushL(p);
    p->ConstructL();
    CleanupStack::Pop(p);
    return p;
    }





