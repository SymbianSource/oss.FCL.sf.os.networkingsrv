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
// T_TraceRt.CPP
// Started by MWT, June 1997
// 
//

#include <TRENG.H>
#include <e32cons.h>
#include <c32comm.h>
#include <nifman.h>
#include <f32file.h>

#ifdef __WINS__
#define CDRV1_PATH _L("ECDRV")
#define COMM_PATH _L("ECOMM")
#endif

LOCAL_C TInt Program();

const TInt  KHistoryBufferSize = 20;

#define KPrompt Command>
const TInt KPromptLength = 8;

class CHBufCirBuf : public CCirBuf<HBufC*>
	{
public:
	static CHBufCirBuf* NewL(TInt aLength);
	~CHBufCirBuf();
	TInt Add(const TDesC& aLine);
	HBufC*& operator[](TInt anIndex) const;
	};

class CTraceRtTestKeyStroke;
class CTraceRtTestUi : public CTraceRtEng, public MTraceRtNotificationHandler
	{
public:
	static CTraceRtTestUi* NewL();
	void ConstructL();
	void InitialiseL();
	~CTraceRtTestUi();

	virtual void Starting(const TNameRecord& aTo, TInt aTtl, TInt aDataLen);
	virtual void Probe(TInt aTtl);
	virtual void Reply(TInt aNo, TInt aDelta, TUint aId);
	virtual void FromHost(const TNameRecord& aHost);
	virtual void Finished(TInt aError);

	void SetKeyStrokeActive();
	void KeyStroke();
	void KeyStrokeDoCancel();
	void AddToHistory(const TDesC& aLine);
	void DisplayHistory(TInt aLine, TInt aPromptLen, TDes& aDes) const;
	inline CConsoleBase& Console() { return *iConsole; }

private:
	CConsoleBase* iConsole;
	CTraceRtTestKeyStroke* iKeyHandler;
	CHBufCirBuf* iHistory;
	};

class CTraceRtTestKeyStroke : public CActive
	{
friend class CTraceRtTestUi;
public:
	CTraceRtTestKeyStroke(CTraceRtTestUi& aUi);
	~CTraceRtTestKeyStroke();
	void RunL();
	void DoCancel();
private:
	CTraceRtTestUi* iUi;
	};

class TTestTraceRtOptions : public TTraceRtOptions
	{
public:
	TInt ParseCommandLine(CTraceRtTestUi& aUI);
	};


GLDEF_C TInt E32Main()
	{

	__UHEAP_MARK;
	// Stardard stuff
	CTrapCleanup* cleanupStack = CTrapCleanup::New();
	if(!cleanupStack)
		{
		return KErrNoMemory;
		}

	TRAPD(err, Program());

	delete cleanupStack;
	__UHEAP_MARKEND;
	return err;
	}

LOCAL_C TInt Program()
	{

	TInt ret = 0;

	CActiveScheduler* as = new(ELeave) CActiveScheduler;
	CleanupStack::PushL(as);
	CActiveScheduler::Install(as);

	CTraceRtTestUi* ui = CTraceRtTestUi::NewL();
	CleanupStack::PushL(ui);

	FOREVER
		{

		TTestTraceRtOptions options;
		if((ret=options.ParseCommandLine(*ui))!=KErrNone)
			{
			break;
			}
	
		ui->SetKeyStrokeActive();
		ui->Start(options);
		CActiveScheduler::Start();
		}

	CleanupStack::PopAndDestroy(ui);
	CleanupStack::PopAndDestroy(as);

	return ret;
	}

CTraceRtTestUi* CTraceRtTestUi::NewL()
//
// Create new test UI
//
	{

	CTraceRtTestUi* ui = new (ELeave) CTraceRtTestUi;
	CleanupStack::PushL(ui);
	ui->ConstructL();
	CleanupStack::Pop();
	return ui;
	}

void CTraceRtTestUi::InitialiseL()
//
// Ensure stuff is loaded etc
//
	{

#ifdef __WINS__
    User::LoadPhysicalDevice(CDRV1_PATH);    
    User::LoadLogicalDevice(COMM_PATH);
#endif
//	User::LeaveIfError(Nifman::CheckIniConfig());
	}


void CTraceRtTestUi::ConstructL()
//
// Contruct base object and console
//
	{
	
	iKeyHandler = new (ELeave) CTraceRtTestKeyStroke(*this);
	iHistory = CHBufCirBuf::NewL(KHistoryBufferSize);
	iConsole = Console::NewL(_L("Trace Route"),TSize(KConsFullScreen,KConsFullScreen));
	InitialiseL();
	CTraceRtEng::ConstructL(*this);
	}

CTraceRtTestUi::~CTraceRtTestUi()
//
// Delete console
//
	{
	 
	delete iHistory;
	delete iKeyHandler;
	delete iConsole;
	}
	
void CTraceRtTestUi::AddToHistory(const TDesC& aLine)
//
// Add line to history buffer
//
	{
	
	iHistory->Add(aLine);
	}
	
void CTraceRtTestUi::DisplayHistory(TInt aLine, TInt aPromptLen, TDes& aDes) const
	{
	
	aDes.SetLength(0);
	if(!iHistory->Count())
		{
		return;
		}
	const HBufC* line=(*iHistory)[aLine];
	iConsole->SetPos(aPromptLen);
	iConsole->ClearToEndOfLine();
	iConsole->Write(*line);
	aDes.Append(*line);
	}

void CTraceRtTestUi::Starting(const TNameRecord& aTo, TInt aTtl, TInt)
//
// Starting trace route to remote host
//
	{

	TName ipaddr;
	TInetAddr& addr = (TInetAddr&)aTo.iAddr;
	addr.Output(ipaddr);
	
	if(aTo.iName.Length())
		{
		iConsole->Printf(_L("Tracing route to %S [%S]\n\r"), &aTo.iName, &ipaddr);
		}
	else
		{
		iConsole->Printf(_L("Tracing route to %S\n\r"), &ipaddr);
		}
	iConsole->Printf(_L("Over a maximum of %d hops\n\r\n\r"), aTtl);
	}

void CTraceRtTestUi::Probe(TInt aTtl)
//
// First line of output
//
	{
	
	iConsole->Printf(_L("%3d "), aTtl);
	}

void CTraceRtTestUi::Reply(TInt, TInt aDelta, TUint aId)
//
// A reply from a remote host
//
	{

	TName buf;

	if(aId==KTraceRtCodeTimeout)
		{
		buf.Append(_L("     "));
		}
	else if(aDelta<10)
		{
		buf.Append(_L(" <10 "));
		}
	else
		{
		buf.AppendFormat(_L("%4d "), aDelta);
		}

	switch(aId)
		{
	case KTraceRtCodeUnreachNet: buf.AppendFormat(_L("!N ")); break;
	case KTraceRtCodeUnreachHost:  buf.AppendFormat(_L("!H ")); break;
	case KTraceRtCodeUnreachProtocol:  buf.AppendFormat(_L("!@ ")); break;
	case KTraceRtCodeUnreachPort:  buf.AppendFormat(_L("!?  ")); break;
	case KTraceRtCodeUnreachNeedFrag:  buf.AppendFormat(_L("!F ")); break;
	case KTraceRtCodeUnreachSrcRouteFail:  buf.AppendFormat(_L("!R ")); break;
	case KTraceRtCodeUnreachNetUnknown:  buf.AppendFormat(_L("!U ")); break;
	case KTraceRtCodeUnreachHostUnknown:  buf.AppendFormat(_L("!J ")); break;
	case KTraceRtCodeUnreachSrcHostIsolated:  buf.AppendFormat(_L("!I ")); break;
	case KTraceRtCodeUnreachNetProhibited:  buf.AppendFormat(_L("!X ")); break;
	case KTraceRtCodeUnreachHostProhibited:  buf.AppendFormat(_L("!Z ")); break;
	case KTraceRtCodeUnreachNetTOS:  buf.AppendFormat(_L("!T ")); break;
	case KTraceRtCodeUnreachHostTOS:  buf.AppendFormat(_L("!Q ")); break;
	case KTraceRtCodeUnreachProhibited:  buf.AppendFormat(_L("!Y ")); break;
	case KTraceRtCodeUnreachPrecVolation:  buf.AppendFormat(_L("!V ")); break;
	case KTraceRtCodeUnreachPrecCutoff:  buf.AppendFormat(_L("!N ")); break;
	case KTraceRtCodeTimeout:  buf.AppendFormat(_L("*  ")); break;
	default:  buf.AppendFormat(_L("   ")); break;
		}
	iConsole->Printf(buf);
	}

void CTraceRtTestUi::FromHost(const TNameRecord& aHost)
//
// End of line output with host name
//
	{

	TInetAddr& a=TInetAddr::Cast(aHost.iAddr);
	TName ip;
	a.Output(ip);

	if(a.Address()==0)
		{
		iConsole->Printf(_L("\n"));
		}
	else if(aHost.iName.Length()==0)
		{
		iConsole->Printf(_L("%S\n\r"), &ip);
		}
	else
		{
		iConsole->Printf(_L("%S [%S]\n\r"), &aHost.iName, &ip);
		}
	}

void CTraceRtTestUi::Finished(TInt aError)
//
// Trace Complete
//
	{

	if(aError)
		{
		iConsole->Printf(_L("\n\rTrace complete %d\n\r"), aError);
		}
	else
		{
		iConsole->Printf(_L("\n\rTrace complete\n\r"));
		}
	iKeyHandler->Cancel();
	CActiveScheduler::Stop();
	}

void CTraceRtTestUi::KeyStroke()
//
// Key was pressed
//
	{
	
	if(iKeyHandler->iStatus==KErrNone)
		{

		if(iConsole->KeyCode()==EKeyEscape)
			{
			iConsole->Printf(_L("\nAborted\n"));
			CancelAndFinished();
			return;
			}

		}
	SetKeyStrokeActive();
	}

void CTraceRtTestUi::SetKeyStrokeActive()
//
//
//
	{
	
	iConsole->Read(iKeyHandler->iStatus);
	iKeyHandler->SetActive();
	}

void CTraceRtTestUi::KeyStrokeDoCancel()
//
// Cancel the read
//
	{

	iConsole->ReadCancel();
	}

CTraceRtTestKeyStroke::CTraceRtTestKeyStroke(CTraceRtTestUi& aUi)
//
// Key reader
//
	: CActive(0)
	{
	
	iUi = &aUi;
	CActiveScheduler::Add(this);
	}

CTraceRtTestKeyStroke::~CTraceRtTestKeyStroke()
//
// Destruct means cancel
//
	{

	Cancel();
	}
	
void CTraceRtTestKeyStroke::RunL()
//
// Key pressed
//
	{

	iUi->KeyStroke();
	}

void CTraceRtTestKeyStroke::DoCancel()
//
// Cancel key stroke
//
	{

	iUi->KeyStrokeDoCancel();
	}

TInt TTestTraceRtOptions::ParseCommandLine(CTraceRtTestUi& aUi)
//
//
//
	{

	TInt res;
	TBool help;
	do
		{
		TBuf<0x100> command;
		aUi.Console().Printf(_L("KPrompt"));
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
					++histpos;
					}
				was=key;
				aUi.DisplayHistory(++histpos, KPromptLength, command);
				}
			else if(key == EKeyDownArrow)
				{
				if(was==EKeyUpArrow)
					{
					histpos--;
					}
				was=key;
				aUi.DisplayHistory(histpos--, KPromptLength, command);
				}
			else if(key>=EKeySpace && key<=EKeyDelete)
				{
				aUi.Console().Printf(_L("%c"), key);
				command.Append(TChar(key));
				}
			}
		aUi.Console().Printf(_L("\n"));
		aUi.AddToHistory(command);

		(*this) = TTestTraceRtOptions();
		iDestname = _L("127.0.0.1");
		TLex lex(command);
		res=KErrNone;
		help=EFalse;

		TPtrC ptr;
		for(ptr.Set(lex.NextToken()) ; ptr.Length() ; ptr.Set(lex.NextToken()))
			{

			if(!ptr.CompareF(_L("q")) || !ptr.CompareF(_L("quit")) || !ptr.CompareF(_L("exit")))
				{
				return KErrEof;
				}
			if(!ptr.CompareF(_L("help")))
				{
				help=ETrue;
				}
			else if(ptr.Length()==2)
				{
				if(!ptr.CompareF(_L("-D")))
					{
					iResolveAddress=EFalse;
					}
				else if(!ptr.CompareF(_L("-H")))
					{
					help=ETrue;
					}
				else
					{
					iDestname=ptr;
					}
				if(!ptr.CompareF(_L("-C")))
					{
					iPrompt = ETrue;
					}
				else
					{
					iPrompt = EFalse;
					}
				}
			else if(ptr.Length()>2)
				{

				TLex val(ptr.Mid(2));
				TInt num;

				TPtrC cmd = ptr.Mid(0,2);

				if(!cmd.CompareF(_L("-M")))
					{
					if(val.Val(iMaxTtl) != KErrNone)
						{
						res=KErrArgument;
						}
					else if(iMaxTtl<1)
						{
						res=KErrUnderflow;
						}
					}
				else if(!cmd.CompareF(_L("-W")))
					{
					if(val.Val(num) != KErrNone)
						{
						res=KErrArgument;
						}
					else if(num<0)
						{
						res=KErrUnderflow;
						}
					else
						{
						iWait=num;
						}
					}
				else if(!cmd.CompareF(_L("-V")))
					{
					if(val.Val(iTos) != KErrNone)
						{
						res=KErrArgument;
						}
					else if(iTos<1)
						{
						res=KErrUnderflow;
						}
					else if(iTos>255)
						{
						res=KErrOverflow;
						}
					}
				else if(!cmd.CompareF(_L("-P")))
					{
					if(val.Val(iNrProbes) != KErrNone)
						{
						res=KErrArgument;
						}
					else if(iNrProbes<0)
						{
						res=KErrUnderflow;
						}
					}
				else
					{
					iDestname = ptr;
					}
				}
			else
				{
				iDestname = ptr;
				}

			if(res!=KErrNone || help)
				{
				
				if(!help)
					{
				    	aUi.Console().Printf(_L("Incorrect option %S - result %d\n"), &ptr, res);
					}
				aUi.Console().Printf(_L("Usage: [options] destination\n\nwhere options are\n"));
				aUi.Console().Printf(_L("    -d         do not resolve addresses to hostnames\n"));
				aUi.Console().Printf(_L("    -c         prompt for interface choice\n"));
				aUi.Console().Printf(_L("    -h         print out this screen\n"));
				aUi.Console().Printf(_L("    -m<number> maximum number of hops\n"));
				aUi.Console().Printf(_L("    -w<number> time to wait for replies\n"));
				aUi.Console().Printf(_L("    -v<number> tos\n"));
				aUi.Console().Printf(_L("    -p<number> number of probes\n"));
				aUi.Console().Printf(_L("    quit, q or exit to finish\n\n"));
				
				break;
				}
			}
		} while (res!=KErrNone || help);

    return KErrNone;
	}

CHBufCirBuf* CHBufCirBuf::NewL(TInt aLength)
//
// Create new circular buffer for command line history
//
	{

	CHBufCirBuf*p = new (ELeave) CHBufCirBuf;
	CleanupStack::PushL(p);
	p->SetLengthL(aLength);
	CleanupStack::Pop();
	return p;
	}

CHBufCirBuf::~CHBufCirBuf()
//
// Delete contents
//
	{
	
	while(Count())
		{
		HBufC* buf;
		Remove(&buf);
		delete buf;
		}
	}

TInt CHBufCirBuf::Add(const TDesC& aLine)
//
// Add a new line to the buffer
//
	{

	if(!aLine.Length())
		{
		return KErrNotFound;
		}

	TInt i;
	HBufC* buf=0;
	for(i=Count()-1; i>=0 ; --i)
		{
		if(!buf)
			{
			if(!(*this)[i]->Compare(aLine))
				{
				buf=(*this)[i];
				}
			}
		if(buf)
			{
			if(i==0)
				{
				(*this)[0]=buf;
				return KErrNone;
				}
			else
				{
				(*this)[i]=(*this)[i-1];
				}
			}
		}

	if(Count()==Length())
		{
		Remove(&buf);
		delete buf;
		}

	buf=aLine.Alloc();
	if(!buf)
		{
		return KErrNoMemory;
		}
	
	return CCirBuf<HBufC*>::Add(&buf);
	}

HBufC*& CHBufCirBuf::operator[](TInt aIndex) const
//
// Return index relative to last element added
// doesn't matter if index is out of range because it is wrapped
//
	{

	aIndex=aIndex%iCount;
	if(aIndex<0)
		{
		aIndex=iCount+aIndex;
		}
	++aIndex;

	// Offset to required element
	// Made static to remove GCC warnings.
	static TUint8* offset = iHead-aIndex*iSize;
	if(offset<iPtr)
		{
		offset=PtrSub(iPtrE,iPtr-offset);
		}

	return *(HBufC**)offset;
	}
