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
// Started by MWT, June 1997
// 
//

#include <pingeng.h>
#include <e32cons.h>
#include <c32comm.h>
#include <nifman.h>

#ifdef __WINS__
#define CDRV1_PATH _L("ECDRV")
#define COMM_PATH _L("ECOMM")
#endif

LOCAL_C TInt ProgramL();

const TInt  KHistoryBufferSize = 20;

_LIT(KPrompt, "Command>");

const TInt KPromptLength = 8;
const TInt KQuitReturn = -100;
const TInt KHelpReturn = -101;

class CCircList : public CBase
	{
public:
	CCircList(TInt aLength);
	~CCircList();
	TInt Add(const TDesC& aLine);
	const HBufC& operator[](TInt anIndex) const;
	TInt Count() const;

private:
	RArray<HBufC*> iBufPtrArray;
	TInt iMaxLength;
	};

class CPingTestKeyStroke;
class CPingTestUi : public CBase, public MPingNotificationHandler
    {
public:
	static CPingTestUi* NewL();
	void StartL(const TPingOptions& aOptions);
	~CPingTestUi();
	
	virtual void Pinging(const TNameRecord& aRecord, TInt aBytes) const;
	virtual void Sent() const;
	virtual void Reply(const TInetAddr& aFrom, TInt aBytes, TInt aSeq, TTimeIntervalMicroSeconds32 aTime) const;
	virtual void Icmp4Message(const TInetAddr& aFrom, TInt aType, TInt aCode, const TDesC8& aRestOfIt) const;
	virtual void Icmp6Message(const TInetAddr& aFrom, TInt aType, TInt aCode) const;
	virtual void Finished(const TNameRecord& aRecord, TInt aNrTransmitted, TInt aNrReceived, TInt aNrDuplicates, TInt aMin, TInt aMax, TInt aSum, TInt aError);

	void SetParams(TBool aFloodFormat);
	void SetKeyStrokeActive();

	void KeyStroke();
	void KeyStrokeDoCancel();
	void AddToHistory(const TDesC& aLine);
	void DisplayHistory(TInt aLine, TInt aPromptLen, TDes& aDes) const;
	inline CConsoleBase& Console() { return *iConsole; }

	TInt NrReceivedPackets() const;

private:
	void ConstructL();
	void InitialiseL();

private:
	TBool iFloodFormat;
	CConsoleBase* iConsole;
	CPingTestKeyStroke* iKeyHandler;
	CCircList* iHistory;
	CPingEng* iEngine;
	TInt iNrReceivedPackets;
	};

inline TInt CPingTestUi::NrReceivedPackets() const
	{
	
	return iNrReceivedPackets;
	}

class CPingTestKeyStroke : public CActive
	{
public:
	CPingTestKeyStroke(CPingTestUi& aUi);
	~CPingTestKeyStroke();
	void ReStart();
	void RunL();
	void DoCancel();
private:
	CPingTestUi& iUi;
	};

class TPingTestParser
	{
public:
	// returns EFalse if the user wants to exit, ETrue otherwise
	TBool ParseCommandLine(CPingTestUi& aUI);
	TInt ParseCLArguments(TDes& aCommandLine,CPingTestUi& aUI);
	
	const TPingOptions& Options() const;
	const TPtrC& BadArgument() const;

private:
	TPingOptions iOptions;
	TPtrC iCLArgument;
	};

const TPtrC& TPingTestParser::BadArgument() const
	{
	
	return iCLArgument;
	}


GLDEF_C TInt E32Main()
	{
	
	__UHEAP_MARK;
	// Standard stuff
	CTrapCleanup* trap = CTrapCleanup::New();
	if(trap==NULL)
		return KErrNoMemory;

	TRAPD(err, ProgramL());
	
	delete trap;
	__UHEAP_MARKEND;
	return err;
	}

LOCAL_C TInt ProgramL()
	{
	
	TInt ret = 0;

	CActiveScheduler* as = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(as);
	CActiveScheduler::Install(as);

	HBufC *argsBuf = HBufC::NewMaxLC(512);
	TPtr args(argsBuf->Des());
	User::CommandLine(args);

	CPingTestUi* ui=CPingTestUi::NewL();
	CleanupStack::PushL(ui);

	TPingTestParser parser;
	if (args.Length()!=0)
		{
		TInt res = parser.ParseCLArguments(args,*ui);
		if (res != KErrNone)
			{
			ui->Console().Printf(_L("Invalid argument %S - result %d\n"), &parser.BadArgument(), res);
			}
		else
			{
			ui->SetParams(parser.Options().iInterval.Int()<200000);
			ui->SetKeyStrokeActive();
			ui->StartL(parser.Options());
			CActiveScheduler::Start();
			}
	    ui->Console().Getch();
		}
	else
		for(;;)
		{
			ret = parser.ParseCommandLine(*ui);
			if(ret == KQuitReturn)
				{
				break;
				}
			if(ret != KHelpReturn)
			{
				ui->SetParams(parser.Options().iInterval.Int()<200000);
				ui->SetKeyStrokeActive();
				ui->StartL(parser.Options());
				CActiveScheduler::Start();
			}
		}

	ret = ui->NrReceivedPackets() == 0;

	CleanupStack::PopAndDestroy(ui);
	CleanupStack::PopAndDestroy(argsBuf);
	CleanupStack::PopAndDestroy(as);

	return ret;
	}

CPingTestUi* CPingTestUi::NewL()
//
// Create new test UI
//
	{

	CPingTestUi* ui = new (ELeave) CPingTestUi;
	CleanupStack::PushL(ui);
	ui->ConstructL();
	CleanupStack::Pop(ui);
	return ui;
	}

void CPingTestUi::InitialiseL()
//
// Ensure stuff is loaded etc
//
	{
	
#ifndef __EPOC32__
	User::LoadPhysicalDevice(CDRV1_PATH);    
    	User::LoadLogicalDevice(COMM_PATH);
#endif

	
//	User::LeaveIfError(Nifman::CheckIniConfig());
	}


void CPingTestUi::ConstructL()
//
// Contruct engine and console
//
	{
	
	iConsole = Console::NewL(_L("Ping"),TSize(KConsFullScreen,KConsFullScreen));
	_LIT(KPhbkSyncCMI, "phbsync.cmi");
    	(void)StartC32WithCMISuppressions(KPhbkSyncCMI);
	
	iEngine=CPingEng::NewL(*this);
	iKeyHandler = new (ELeave) CPingTestKeyStroke(*this);
	iHistory = new (ELeave) CCircList(KHistoryBufferSize);
		
	InitialiseL();
	}

CPingTestUi::~CPingTestUi()
//
// Delete console
//
	{
	
	delete iHistory;
	delete iKeyHandler;
	delete iConsole;
	delete iEngine;
	}

void CPingTestUi::StartL(const TPingOptions& aOptions)
	{
	
	iEngine->StartL(aOptions);
	}

void CPingTestUi::AddToHistory(const TDesC& aLine)
//
// Add line to history buffer
//
	{
	
	iHistory->Add(aLine);
	}
	
void CPingTestUi::DisplayHistory(TInt aLine, TInt aPromptLen, TDes& aDes) const
	{
	
	aDes.SetLength(0);
	if(!iHistory->Count())
		{
		return;
		}
	const HBufC& line=(*iHistory)[aLine];
	iConsole->SetPos(aPromptLen);
	iConsole->ClearToEndOfLine();
	iConsole->Write(line);
	aDes.Append(line);
	}

void CPingTestUi::Pinging(const TNameRecord& aRecord, TInt aBytes) const
//
//	Who are we pinging ??
//
	{

	TName ipaddr;
	TInetAddr& addr = (TInetAddr&)aRecord.iAddr;
	addr.Output(ipaddr);

	if(aRecord.iName.Length())
		{
		iConsole->Printf(_L("Pinging %S [%S] with %d bytes of data\n"), &aRecord.iName, &ipaddr, aBytes);
		}
	else
		{
		iConsole->Printf(_L("Pinging %S with %d bytes of data\n"), &ipaddr, aBytes);
		}
	}

void CPingTestUi::Reply(const TInetAddr& aFrom, TInt aBytes, TInt aIcmpSeq, TTimeIntervalMicroSeconds32 aTime) const
//
// Reply from remote host
//
	{

	if(iFloodFormat)
		{
		iConsole->Write(_L("."));
		return;
		}

	TName inetaddr;
	aFrom.Output(inetaddr);

	if(aTime.Int()<15000)
		{
		iConsole->Printf(_L("Reply from %S  len=%d seq %d time<15ms\n"), &inetaddr, aBytes, aIcmpSeq);
		}
	else
		{
		iConsole->Printf(_L("Reply from %S  len=%d seq %d time=%dms\n"), &inetaddr, aBytes, aIcmpSeq, aTime.Int()/1000);
		}
	}

void CPingTestUi::Sent() const
//
//
//
	{

	if(iFloodFormat)
		{
		iConsole->Write(_L("\b"));
		}
	}

void CPingTestUi::Icmp4Message(const TInetAddr& aAddr, TInt aType, TInt aCode, const TDesC8& aData) const
//
//
//
	{

	TBuf<39> inetaddr;
	aAddr.Output(inetaddr);
	iConsole->Printf(_L("Reply from %S: len=%d "), &inetaddr, aData.Length()+4);

	switch(aType)
		{
	case KIPv4PingTypeEchoReply:
		iConsole->Printf(_L("Echo Reply\n"));
		break;

	case KIPv4PingTypeUnreachable:
		switch(aCode)
			{
		case KIPv4PingCodeUnreachNet:
			iConsole->Printf(_L("Destination Net Unreachable\n"));
			break;
		case KIPv4PingCodeUnreachHost:
			iConsole->Printf(_L("Destination Host Unreachable\n"));
			break;
		case KIPv4PingCodeUnreachProtocol:
			iConsole->Printf(_L("Destination Protocol Unreachable\n"));
			break;
		case KIPv4PingCodeUnreachPort:
			iConsole->Printf(_L("Destination Port Unreachable\n"));
			break;
		case KIPv4PingCodeUnreachNeedFrag:
			iConsole->Printf(_L("Fragmentation Needed and DF Set\n"));
			break;
		case KIPv4PingCodeUnreachSrcRouteFail:
			iConsole->Printf(_L("Source Route Failed\n"));
			break;
		case KIPv4PingCodeUnreachNetUnknown:
			iConsole->Printf(_L("Destination Network Unknown\n"));
			break;
		case KIPv4PingCodeUnreachHostUnknown:
			iConsole->Printf(_L("Destination Host Unknown\n"));
			break;
		case KIPv4PingCodeUnreachSrcHostIsolated:
			iConsole->Printf(_L("Source Host Isolated\n"));
			break;
		case KIPv4PingCodeUnreachNetProhibited:
			iConsole->Printf(_L("Destination Network Prohibited\n"));
			break;
		case KIPv4PingCodeUnreachHostProhibited:
			iConsole->Printf(_L("Destination Host Prohibited\n"));
			break;
		case KIPv4PingCodeUnreachNetTOS:
			iConsole->Printf(_L("Network Unreachable for TOS\n"));
			break;
		case KIPv4PingCodeUnreachHostTOS:
			iConsole->Printf(_L("Host Unreachable for TOS\n"));
			break;
		case KIPv4PingCodeUnreachProhibited:
			iConsole->Printf(_L("Prohibited by Filtering\n"));
			break;
		case KIPv4PingCodeUnreachPrecVolation:
			iConsole->Printf(_L("Precedence Violation\n"));
			break;
		case KIPv4PingCodeUnreachPrecCutoff:
			iConsole->Printf(_L("Precedence Cutoff in Effect\n"));
			break;

		default:
			iConsole->Printf(_L("Dest Unreachable, Bad Code: %d\n"), aCode);
			break;
			}
		break;

	case KIPv4PingTypeSourceQuench:
		iConsole->Printf(_L("Source Quench\n"));
		break;

	case KIPv4PingTypeRedirect:
		switch(aCode) 
			{
		case KIPv4PingCodeRedirectNet:
			iConsole->Printf(_L("Redirect Network"));
			break;
		case KIPv4PingCodeRedirectHost:
			iConsole->Printf(_L("Redirect Host"));
			break;
		case KIPv4PingCodeRedirectNetTOS:
			iConsole->Printf(_L("Redirect Type of Service and Network"));
			break;
		case KIPv4PingCodeRedirectHostTOS:
			iConsole->Printf(_L("Redirect Type of Service and Host"));
			break;
		default:
			iConsole->Printf(_L("Redirect, Bad Code: %d"), aCode);
			break;
			}
		iConsole->Printf(_L("(New addr: %d.%d.%d.%d)\n"), aData[0], aData[1], aData[2], aData[3]);
		break;

	case KIPv4PingTypeTimeExceeded:
		switch(aCode)
			{
		case KIPv4PingCodeExceedInTransit:
			iConsole->Printf(_L("Time to live exceeded\n"));
			break;
		case KIPv4PingCodeExceedInReasm:
			iConsole->Printf(_L("Frag reassembly time exceeded\n"));
			break;
		default:
			iConsole->Printf(_L("Time exceeded, Bad Code: %d\n"), aCode);
			break;
			}
		break;
	case KIPv4PingTypeBadParameter:
		iConsole->Printf(_L("Parameter problem: pointer = 0x%02x\n"), *(TUint*)aData.Ptr());
		break;
	default:
		iConsole->Printf(_L("Bad ICMPv4 type: %d\n"), aType);
		}
	}

void CPingTestUi::Icmp6Message(const TInetAddr& aAddr, TInt aType, TInt aCode) const
	{
	
	TBuf<39> inetaddr;
	aAddr.Output(inetaddr);
	iConsole->Printf(_L("Reply from %S: "), &inetaddr);

	switch(aType)
		{
		case KIPv6PingTypeEchoReply:
			iConsole->Printf(_L("Echo Reply\n"));
			break;

		case KIPv6PingTypeUnreachable:
			switch (aType)
				{
			case KIPv6PingCodeNoRoute:
				iConsole->Printf(_L("No route to destination\n"));
				break;
			case KIPv6PingCodeAdminProhibited:
				iConsole->Printf(_L("Communication administratively prohibited\n"));
				break;
			case KIPv6PingCodeAddressUnreachable:
				iConsole->Printf(_L("Address unreachable\n"));
				break;
			case KIPv6PingCodePortUnreachable:
				iConsole->Printf(_L("Port unreachable\n"));
				break;
			default:
				iConsole->Printf(_L("Unreachable, bad code: %d\n"), aCode);
				break;
				}
			break;

		case KIPv6PingTypePacketTooBig:
			iConsole->Printf(_L("Packet too big"));
			break;

		case KIPv6PingTypeTimeExeeded:
			switch (aType)
				{
			case KIPv6PingCodeHLExeeded:
				iConsole->Printf(_L("Hop limit exceeded in transit\n"));
				break;
			case KIPv6PingCodeFragReassemblyExeeded:
				iConsole->Printf(_L("Fragment reassembly time exceeded\n"));
				break;
			default:
				iConsole->Printf(_L("Time exceeded, bad code: %d\n"), aCode);
				}
			break;

		case KIPv6PingTypeParamProblem:
			switch(aCode)
				{
			case KIPv6PingCodeErroneousHeader:
				iConsole->Printf(_L("Erroneous header field encountered\n"));
				break;
			case KIPv6PingCodeNextHeaderUnrecognised:
				iConsole->Printf(_L("Unrecognised next header type encountered\n"));
				break;
			case KIPv6PingCodeIPv6OptionUnrecognised:
				iConsole->Printf(_L("Unrecognised IPv6 option encountered\n"));
				break;
			default:
				iConsole->Printf(_L("Parameter problem, bad code: %d\n"), aCode);
				break;
				}
			break;

		default:
			iConsole->Printf(_L("Bad ICMPv6 type, value: %d\n"), aType);
		}
	}

void CPingTestUi::SetParams(TBool aFloodFormat)
//
// 
//
	{
	
	iFloodFormat=aFloodFormat;
	}

void CPingTestUi::Finished(const TNameRecord& aRecord, TInt aNrTransmitted, TInt aNrReceived, TInt, TInt aMin, TInt aMax, TInt aSum, TInt aError)
//
// Pinger finished
//
	{

	iConsole->Printf(_L("\n"));
	if(aError!=KErrNone)
		{
		iConsole->Printf(_L("Error %d\n"),aError);
		}

	if(aNrTransmitted)
		{

		
		TInt loss = aNrReceived>=aNrTransmitted ? 0 : (aNrTransmitted-aNrReceived)*100/aNrTransmitted;

		iConsole->Printf(_L("Statistics %S\n"), &aRecord.iName);
		iConsole->Printf(_L("%d transmitted %d received %d%% packet loss\n"), aNrTransmitted, aNrReceived, loss);

		if(aNrReceived)
			{
			TInt avg = aSum/aNrReceived/1000;
			iConsole->Printf(_L("Round-trip %d min %d avg %d max\n"), aMin/1000, avg, aMax/1000);
			}

		iNrReceivedPackets = aNrReceived;
		}
	else
		{
		iNrReceivedPackets = 0;
		}
	iKeyHandler->Cancel();
	CActiveScheduler::Stop();
	}	

void CPingTestUi::KeyStroke()
//
// Key was pressed
//
	{
	
	if(iKeyHandler->iStatus==KErrNone)
		{

	    	if(iConsole->KeyCode()==EKeyEscape)
			{
			iConsole->Printf(_L("\nAborted\n"));
			iEngine->CancelAndFinished();
			return;
			}

		}
	SetKeyStrokeActive();
	}

void CPingTestUi::SetKeyStrokeActive()
//
//
//
	{
	
	iConsole->Read(iKeyHandler->iStatus);
	iKeyHandler->ReStart();
	}

void CPingTestUi::KeyStrokeDoCancel()
//
// Cancel the read
//
	{

	iConsole->ReadCancel();
	}

CPingTestKeyStroke::CPingTestKeyStroke(CPingTestUi& aUi)
//
// Key reader
//
	: CActive(0), iUi(aUi)
	{
	
	CActiveScheduler::Add(this);
	}

CPingTestKeyStroke::~CPingTestKeyStroke()
//
// Destruct means cancel
//
	{

	Cancel();
	}
	
void CPingTestKeyStroke::RunL()
//
// Key pressed
//
	{

	iUi.KeyStroke();
	}

void CPingTestKeyStroke::DoCancel()
//
// Cancel key stroke
//
	{

	iUi.KeyStrokeDoCancel();
	}

void CPingTestKeyStroke::ReStart()
	{
	
	SetActive();
	}

const TPingOptions& TPingTestParser::Options() const
	{
	
	return iOptions;
	}


TInt TPingTestParser::ParseCLArguments(TDes& aCommandLine,CPingTestUi& aUi)
	{
	iOptions = TPingOptions();	//Reset iOptions;
	iOptions.iDestname = _L("127.0.0.1");
	TLex lex(aCommandLine);

	for(iCLArgument.Set(lex.NextToken()); iCLArgument.Length(); iCLArgument.Set(lex.NextToken()))
		{
		if(iCLArgument.Length()==2)
			{
			if(!iCLArgument.CompareF(_L("-A")))
				{
				iOptions.iResolveAddress=ETrue;
				}
			else
				{
				iOptions.iDestname=iCLArgument;
				}
			if(!iCLArgument.CompareF(_L("-C")))
				{
				iOptions.iPrompt = ETrue;
				}
			else
				{
				iOptions.iPrompt = EFalse;
				}
			}
		else if(iCLArgument.Length()>2)
			{

			TLex val(iCLArgument.Mid(2));
			TInt num;

			TPtrC cmd = iCLArgument.Mid(0,2);

			if(!cmd.CompareF(_L("-N")))
				{
				if(val.Val(iOptions.iNumberOfPings) != KErrNone)
					{
					return KErrArgument;
					}
				else if(iOptions.iNumberOfPings<0)
					{
					return KErrUnderflow;
					}
				}
			else if(!cmd.CompareF(_L("-I")))
				{
				if(val.Val(num) != KErrNone)
					{
					return KErrArgument;
					}
				else if(num<0)
					{
					return KErrUnderflow;
					}
				else
					{
					iOptions.iInterval=num;
					}
				}
			else if(!cmd.CompareF(_L("-W")))
				{
				if(val.Val(num) != KErrNone)
					{
					return KErrArgument;
					}
				else if(num<0)
					{
					return KErrUnderflow;
					}
				else
					{
					iOptions.iWait=num;
					}
				}
			else if(!cmd.CompareF(_L("-S")))
				{
				if(val.Val(iOptions.iPingSize) != KErrNone)
					{
					return KErrArgument;
					}
				else if(iOptions.iPingSize<8)
					{
					return KErrUnderflow;
					}
				}
			else if(!cmd.CompareF(_L("-P")))
				{
				if(val.Val(iOptions.iPreload) != KErrNone)
					{
					return KErrArgument;
					}
				else if(iOptions.iPreload<0)
					{
					return KErrUnderflow;
					}
				}
			else if(!cmd.CompareF(_L("-B")))
				{
				if(val.Val(iOptions.iBacklog) != KErrNone)
					{
					return KErrArgument;
					}
				else if(iOptions.iBacklog<0)
					{
					return KErrUnderflow;
					}
				}
			else if(!cmd.CompareF(_L("-C")))
				{
				// barf if connection override already set:
				if(iOptions.iConnSnap || iOptions.iConnIap)
					{
					return KErrArgument;
					}

				// extract values from argument
				TInt snap=0;
				TInt iap=0;
								
				if(iCLArgument.FindF(_L("SNAP")) == 2)
					{
					TLex val(iCLArgument.Mid(6));
					if(val.Val(snap) != KErrNone)
						{
						return KErrArgument;
						}
					}
				else if(iCLArgument.FindF(_L("IAP")) == 2)
					{
					TLex val(iCLArgument.Mid(5));
					if(val.Val(iap) != KErrNone)
						{
						return KErrArgument;
						}
					}
				else if(val.Val(iap) != KErrNone) // plain number to be interpreted as IAP
					{
					return KErrArgument;
					}
				
				// apply value to options structure, and tell the user we're overriding connection
				if(snap)
					{
					aUi.Console().Printf(_L("Will start connection with SNAP %d\n"), snap);
					iOptions.iConnSnap = snap;
					}
				else if (iap)
					{
					aUi.Console().Printf(_L("Will start connection with IAP %d\n"), iap);
					iOptions.iConnIap = iap;
					}
				else
					{
					// 0 was specified.. what were you thinking?
					User::Leave(-1005);
					return KErrArgument;
					}
				}
			else
				{
				iOptions.iDestname = iCLArgument;
				}
			}
		else
			{
			iOptions.iDestname = iCLArgument;	
			}
		}

	return KErrNone;
	}



TBool TPingTestParser::ParseCommandLine(CPingTestUi& aUi)
	{
	
	TInt res = KErrNone;

	do
		{
		TBuf<0x100> command;
		aUi.Console().Printf(KPrompt);
		
		TKeyCode key, was=EKeyNull;
		TInt histpos=-1;
		while((key=aUi.Console().Getch())!=EKeyEnter)
			{
			if(command.Length()>=0x100)
				{
				User::Beep(440, 500000);
				}
			else if(key==EKeyBackspace || key==EKeyLeftArrow || key==EKeyDelete)
				{
				if(command.Length())
					{
					aUi.Console().Printf(_L("\b \b"));
					command.SetLength(command.Length()-1);
					}
				}
			else if(key == EKeyUpArrow)
				{
				if(was==EKeyDownArrow)
					{
					histpos--;
					}
				was=key;
				aUi.DisplayHistory(histpos--, KPromptLength, command);
				}
			else if(key == EKeyDownArrow)
				{
				if(was==EKeyUpArrow)
					{
					++histpos;
					}
				was=key;
				aUi.DisplayHistory(++histpos, KPromptLength, command);
				}
			else if(key>=EKeySpace && key<=EKeyDelete)
				{
				aUi.Console().Printf(_L("%c"), key);
				command.Append(TChar(key));
				}
			}

		aUi.Console().Printf(_L("\n"));
		aUi.AddToHistory(command);

		_LIT(KQuitCommand, "quit");
		_LIT(KQCommand, "q");
		_LIT(KExitCommand, "exit");

		if (command == KQuitCommand || command == KQCommand || command == KExitCommand)
			{
			return KQuitReturn;
			}

		_LIT(KHelpCommand, "help");
		if (command == KHelpCommand)
			{
			aUi.Console().Printf(_L("Usage: [options] destination\n\nwhere options are\n"));
			aUi.Console().Printf(_L("    -a         resolve address to hostname\n"));
			aUi.Console().Printf(_L("    -c         prompt for interface choice\n"));
			aUi.Console().Printf(_L("    -cIAP3     start connection with IAP 3\n"));
			aUi.Console().Printf(_L("    -cSNAP77   start connection with SNAP 77\n"));
			aUi.Console().Printf(_L("    -h         print out this screen\n"));
			aUi.Console().Printf(_L("    -n<number> number of pings\n"));
			aUi.Console().Printf(_L("    -i<number> interval between pings\n"));
			aUi.Console().Printf(_L("    -s<number> number of bytes in request\n"));
			aUi.Console().Printf(_L("    -p<number> preload\n"));
			aUi.Console().Printf(_L("    -w<number> time to wait for replies\n"));
			aUi.Console().Printf(_L("    -b<number> maximum number of unanswered requests\n"));	
			aUi.Console().Printf(_L("    quit, q or exit to finish\n\n"));
			return KHelpReturn;
			}

		res = ParseCLArguments(command,aUi);
		if (res != KErrNone)
			{
			aUi.Console().Printf(_L("Invalid argument %S - result %d\n"), &BadArgument(), res);
			break;
			}
		} while (res!=KErrNone);

    	return ETrue;
	}

//
// Create new circular buffer for command line history
//
CCircList::CCircList(TInt aLength)
: iMaxLength(aLength)
{}

CCircList::~CCircList()
//
// Delete contents
//
	{
	
	for(TInt i = iBufPtrArray.Count()-1; i>=0; --i)
		{
		delete iBufPtrArray[i];
		}
	iBufPtrArray.Close();
	}

TInt CCircList::Add(const TDesC& aLine)
//
// Add a new line to the buffer
//
	{

	if(!aLine.Length())
		{
		return KErrNotFound;
		}

	HBufC* buf=NULL;

	//Check to see if string matches last in array, if so return and don't add it.
	if(iBufPtrArray.Count())
		{
		if(!iBufPtrArray[iBufPtrArray.Count()-1]->Compare(aLine))
			{
			return KErrNone;
			}
		}

	buf=aLine.Alloc();
	if(buf==NULL)
		{
		return KErrNoMemory;		//Ensures we can't put a null pointer into the arrray.
		}
	
	if (iBufPtrArray.Count()>=iMaxLength)	//Check to see if array is now too large,if so remove first element.
		{
		iBufPtrArray.Remove(0);
		}

	return iBufPtrArray.Append(buf);
	}

TInt CCircList::Count() const
	{
	
	return iBufPtrArray.Count();
	}

const HBufC& CCircList::operator[](TInt aIndex) const
//
// Return index relative to last element added
// doesn't matter if index is out of range because we're wrapping it
//
	{
	
	aIndex=aIndex%iBufPtrArray.Count();
	if(aIndex<0)
		{
		aIndex=iBufPtrArray.Count()+aIndex;
		}

	return *iBufPtrArray[aIndex];	//Dont need to check index, since RArray will do bounds checking.
	}
