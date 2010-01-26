// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// usse_simulator.cpp - umts simulator state machine implementattion
//

#include <e32math.h>
#include <f32file.h>
#include "usse_simulator.h"
#include "usse_control_wrapper.h"
#include "usse_qos_wrapper.h"
#include "log.h"

/* **************************
    * CUmtsSimulator class
    * ************************** */

CUmtsSimulator* CUmtsSimulator::NewL()
    {
    CUmtsSimulator* self = new (ELeave) CUmtsSimulator();
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
    return self;
    }

CUmtsSimulator::CUmtsSimulator()
	: iPacketService(NULL), iPacketContexts(_FOFF(CPacketContext, iLink)), iPacketQoSs(_FOFF(CPacketQoS, iLink)),
          iSessions(_FOFF(CUmtsSimServSession, iSglLink)), iQoSNamePool(100), iSimTimer(NULL), iRequestManager(NULL),
          iEventController(NULL), iLogIndex(0), iShutdownActive(EFalse)
    {
	for(TInt i = 0; i < KUMTSSimMaxContexts; i++)
		iContextNamePool[i] = EFalse;
    }

void CUmtsSimulator::ConstructL()
    {
	iPacketService = CPacketService::NewL(this);
	iSimTimer = CSimTimer::NewL();
	iRequestManager = CSimRequestManager::NewL(this);
	iEventController = CSimEventController::NewL(this);

	_LIT(KLogSimulatorUp, "UMTSSim: CUmtsSimulator instance constructed!");
	SafeLog(KLogSimulatorUp);
	_LIT(KLogSimulatorPrefix, "UMTSSim: ");
	TBuf<100> buf;
	buf = KLogSimulatorPrefix;
	buf.Append(KUMTSSimVersion);
	SafeLog(buf);

	ReconfigureReqMan(KUMTSSimConfFile);
	ReconfigureEventCtrl(KUMTSSimConfFile);
    }

CUmtsSimulator::~CUmtsSimulator()
    {
	_LIT(KLogSimulatorDown, "UMTSSim: CUmtsSimulator instance closing!");
	SafeLog(KLogSimulatorDown);

	if(iShutdownActive)
		iSimTimer->RemoveRequest(iShutdownRequest);

	if(iEventController) delete iEventController;
	if(iRequestManager) delete iRequestManager;
	if(iSimTimer) delete iSimTimer;
	if(iPacketService) delete iPacketService;
    }

void CUmtsSimulator::RequestShutdown(TAny *aSelf)
    {
	CUmtsSimulator *self = (CUmtsSimulator *) aSelf;

	self->iShutdownActive = EFalse; // for destructor
	CActiveScheduler::Stop(); // shut down the server
    }

CPacketService* CUmtsSimulator::GetPacketService() const
    {
	return iPacketService;
    }

TInt CUmtsSimulator::GetPacketContext(const TDesC& aName, CPacketContext*& aContext, TBool aRefCount) const
    {
	aContext = NULL;

	TSglQueIter<CPacketContext> iter(((CUmtsSimulator*)this)->iPacketContexts);
	iter.SetToFirst();
	CPacketContext* context = iter++;
	while(context)
        {
		if(!(context->GetName().Compare(aName)))
            {
			if(aRefCount) context->IncRefCount();
			aContext = context;
			return KErrNone;
            }
		context = iter++;
        }
	return KErrNotFound;
    }

TInt CUmtsSimulator::DoNewPacketContext(TDes& aName, CPacketContext*& aContext)
    {
	aContext = NULL;

	for(TInt index = 0; index < KUMTSSimMaxContexts; index++)
        {
		if(!iContextNamePool[index])
            {
			TRAPD(err, aContext = CPacketContext::NewL(this));
			if(err != KErrNone)
                {
				aContext = NULL;
				return err;
                }

			iContextNamePool[index] = ETrue;
			aName.Num(index+1);
			aContext->SetName(aName);

			aContext->IncRefCount();
			iPacketContexts.AddLast(*aContext);

			// add default qos profile to created context
			CPacketQoS* qos = NULL;
			TBuf<64> qosname;
			err = NewPacketQoS(aContext->GetName(), qosname, qos);
			if(err != KErrNone)
                {
				RemovePacketContext(aContext->GetName());
				return err;
                }
			aContext->SetPendingQoS(qos);
			qos->DecRefCount(); // SetPendingQoS incs ref count
			err = qos->SetToDefault();
			if(err != KErrNone) {
            RemovePacketContext(aContext->GetName()); // qos is also removed
            return err;
			}

			return KErrNone;
            }
        }

	return KErrOverflow; // bad error code?
    }

TInt CUmtsSimulator::NewPrimaryPacketContext(TDes& aName, CPacketContext*& aContext)
    {
	TInt err = DoNewPacketContext(aName, aContext);
	if(err != KErrNone)
        {
		aContext = NULL;
		return err;
        }
	iNifs.AddNewPrimary(aContext);
	iPacketService->InformContextAdded(aName);

	DebugCheck();

	return KErrNone;
    }

TInt CUmtsSimulator::NewSecondaryPacketContext(const TDesC& aOldContext, TDes &aName, CPacketContext*& aContext)
    {
	aContext = NULL;

	CPacketContext* oldContext = NULL;
	TInt err = GetPacketContext(aOldContext, oldContext, EFalse);
	if(err != KErrNone)
		return err;

	CPacketContext* context = NULL;
	TBuf<65> name;
	err = DoNewPacketContext(name, context);
	if(err != KErrNone)
		return err;

	err = context->CopyFrom(*oldContext);
	if(err != KErrNone)
        {
		RemovePacketContext(name);
		return err;
        }

	aContext = context;
	aName = name;

	iNifs.AddNewSecondary(context, oldContext);

	iPacketService->InformContextAdded(aName);

	DebugCheck();

	return KErrNone;
    }

TInt CUmtsSimulator::RemovePacketContext(const TDesC& aName)
    {
	DebugCheck(); // after deletion state of caller might be wrong for a while => called here

	CPacketContext* context;
	TInt err = GetPacketContext(aName, context, EFalse);
	if(err != KErrNone)
		return err;

	context->DecRefCount();
	if(context->GetRefCount() == 0)
        {
		// first we must remove all qoses of context to be deleted
		if(RemoveQoSOfContext(context) != KErrNone)
			User::Invariant();

		TLex lexer(aName);
		TInt name;
		if(lexer.Val(name) != KErrNone)
			User::Invariant(); // context was found so name should be ok => should not have failed
		if(name < 1 || name > KUMTSSimMaxContexts || !iContextNamePool[name-1])
			User::Invariant();
		iContextNamePool[name-1] = EFalse;

		iNifs.Remove(context);
		iPacketContexts.Remove(*context); // context was found => should not fail
		delete context;
        }

	return KErrNone;
    }

TInt CUmtsSimulator::RemoveQoSOfContext(CPacketContext* aContext)
    {
	// we could go directly thru QoS objects and mark them deleted (remove context ref. etc)
	// chosen way is more general though

	TSglQueIter<CUmtsSimServSession>& iterator = AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketQoSWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// because there is a bug in CObjectIx we need to hack here
			CPacketQoSApiWrapper* ptr = (CPacketQoSApiWrapper*)(*session->iPacketQoSWrappers)[idx];
			if(ptr == NULL)
                {
                //				_LIT(KLogBug, "ERROR: Simulator::RemoveQoSOfContext() found CObjectIx bug");
				//SafeLog(KLogBug);
				count++;
                }
			else
				ptr->SimContextDeleted(aContext);
            }
		session = iterator++;
        }
	FreeSessionIterator(iterator);

	// last we need to clear references from context itself:
	aContext->DropNegotiatedQoS();
	aContext->DropPendingQoS();

	DebugCheck();

	return KErrNone;
    }

TInt CUmtsSimulator::GetPacketQoS(const TDesC& aContext, const TDesC& aName, CPacketQoS*& aQoS, TBool aRefCount) const
    {
	aQoS = NULL;

	CPacketContext* context = NULL;
	TInt err = GetPacketContext(aContext, context, EFalse);
	if(err != KErrNone)
        {
		return err;
        }

	TSglQueIter<CPacketQoS> iter(((CUmtsSimulator*)this)->iPacketQoSs);
	iter.SetToFirst();
	CPacketQoS* qos = iter++;
	while(qos)
        {
		if(!(qos->GetName().Compare(aName)))
            {
			if(qos->GetContext() != context)
				return KErrCorrupt;
			if(aRefCount) qos->IncRefCount();
			aQoS = qos;
			return KErrNone;
            }
		qos = iter++;
        }
	return KErrNotFound;
    }

TInt CUmtsSimulator::NewPacketQoS(const TDesC& aContext, TDes& aName, CPacketQoS*& aQoS)
    {
	CPacketContext* context = NULL;
	TInt err = GetPacketContext(aContext, context, EFalse);
	if(err != KErrNone)
        {
		aQoS = NULL;
		return err;
        }

	TRAP(err, aQoS = CPacketQoS::NewL(this, context));
	if(err != KErrNone)
        {
		aQoS = NULL;
		return err;
        }
	aName.Num(iQoSNamePool++);
	aQoS->SetName(aName);

	aQoS->IncRefCount();

	iPacketQoSs.AddLast(*aQoS);

	DebugCheck();

	return KErrNone;
    }

TInt CUmtsSimulator::RemovePacketQoS(const TDesC& aContext, const TDesC& aName)
    {
	DebugCheck(); // after deletion state of caller might be wrong for a while => called here

	CPacketQoS* qos;
	TInt err = GetPacketQoS(aContext, aName, qos, EFalse);
	if(err != KErrNone)
		return err;

	qos->DecRefCount();
	if(qos->GetRefCount() == 0) {
    iPacketQoSs.Remove(*qos); // qos was found => should not fail
    delete qos;
	}

	return KErrNone;
    }

void CUmtsSimulator::ContextStateChanged()
    {
	// NOTE: this method changes service status from unattached to something else if there
	// is context activity; this matters only as long as context operations are not sanity-checked

	RPacketService::TStatus status = (iPacketService->iStatus == RPacketService::EStatusUnattached)?
		(RPacketService::EStatusUnattached) : (RPacketService::EStatusAttached);

	TSglQueIter<CPacketContext> iter(((CUmtsSimulator*)this)->iPacketContexts);
	iter.SetToFirst();
	CPacketContext* context = iter++;
	while(context)
        {
		if(context->iStatus == RPacketContext::EStatusActive)
            {
			status = RPacketService::EStatusActive;
			break;
            }
		else if(context->iStatus == RPacketContext::EStatusSuspended)
            {
			status = RPacketService::EStatusSuspended;
            }
		context = iter++;
        }

	iPacketService->PossibleStatusUpdate(status);
    }

TSimNifList& CUmtsSimulator::GetNifList()
    {
	return iNifs;
    }

TSglQueIter<CUmtsSimServSession>& CUmtsSimulator::AllocSessionIterator()
    {
	return *(new TSglQueIter<CUmtsSimServSession>(iSessions));
    }

void CUmtsSimulator::FreeSessionIterator(TSglQueIter<CUmtsSimServSession>& aIterator)
    {
	delete &aIterator;
    }

TSglQueIter<CPacketContext>& CUmtsSimulator::AllocContextIterator()
    {
	return *(new TSglQueIter<CPacketContext>(iPacketContexts));
    }

void CUmtsSimulator::FreeContextIterator(TSglQueIter<CPacketContext>& aIterator)
    {
	delete &aIterator;
    }

void CUmtsSimulator::AddSession(CUmtsSimServSession& aSession)
    {
	if(iShutdownActive)
        {
		iSimTimer->RemoveRequest(iShutdownRequest);
		iShutdownActive = EFalse;
        }

	iSessions.AddLast(aSession);

	DebugCheck();
    }

void CUmtsSimulator::RemoveSession(CUmtsSimServSession& aSession)
    {
	iSessions.Remove(aSession);

	if(iSessions.IsEmpty())
        {
		// order server shutdown in 60 seconds
		TRAPD(err, iShutdownRequest = iSimTimer->RequestCallAfterL(60*1000000,
		      RequestShutdown, this));
        if (err != KErrNone)
            {
            LOG(Log::Printf(_L("iSimTimer->RequestCallAfterL: %d"), err));
            }
		iShutdownActive = ETrue;
        }

	// DebugCheck(); -- callers state may be wrong
    }

TInt CUmtsSimulator::ActDeactivateContext(const TDesC* aParams, TAny* aOptionalParam)
    {
	if(aOptionalParam != NULL || aParams == NULL)
		return KErrArgument;

	TLex lex(*aParams);
	TInt err = KErrNone;
	TPtrC name;
	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'c': // name of the context
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				lex.SkipCharacters();
				if(lex.TokenLength() > 0)
					name.Set(lex.MarkedToken());
				else
					err = KErrArgument;
                }
                break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		if(name.Length() < 1)
			return KErrArgument;

		CPacketContext* context = NULL;
		if(GetPacketContext(name, context, EFalse) != KErrNone)
			return KErrNotFound;

		_LIT(KLogConDeact, "UMTSSim: Controller deactivating context");
		_LIT(KLogName, "Context = ");
		TBuf<65> namebuf;
		namebuf = KLogName;
		namebuf.Append(name);
		Log(KLogConDeact, namebuf);

		context->Deactivate();

		return KErrNone;
        }
	return err;
    }

TInt CUmtsSimulator::ActSuspendContext(const TDesC* aParams, TAny* aOptionalParam)
    {
	if(aOptionalParam != NULL || aParams == NULL)
		return KErrArgument;

	TLex lex(*aParams);
	TInt err = KErrNone;
	TPtrC name;
	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'c': // name of the context
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				lex.SkipCharacters();
				if(lex.TokenLength() > 0)
					name.Set(lex.MarkedToken());
				else
					err = KErrArgument;
                }
                break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		if(name.Length() < 1)
			return KErrArgument;

		CPacketContext* context = NULL;
		if(GetPacketContext(name, context, EFalse) != KErrNone)
			return KErrNotFound;

		_LIT(KLogConDeact, "UMTSSim: Controller suspending context");
		_LIT(KLogName, "Context = ");
		TBuf<65> namebuf;
		namebuf = KLogName;
		namebuf.Append(name);
		Log(KLogConDeact, namebuf);

		return context->Suspend();
        }
	return err;
    }

TInt CUmtsSimulator::ActResumeContext(const TDesC* aParams, TAny* aOptionalParam)
    {
	if(aOptionalParam != NULL || aParams == NULL)
		return KErrArgument;

	TLex lex(*aParams);
	TInt err = KErrNone;
	TPtrC name;
	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'c': // name of the context
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				lex.SkipCharacters();
				if(lex.TokenLength() > 0)
					name.Set(lex.MarkedToken());
				else
					err = KErrArgument;
                }
                break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		if(name.Length() < 1)
			return KErrArgument;

		CPacketContext* context = NULL;
		if(GetPacketContext(name, context, EFalse) != KErrNone)
			return KErrNotFound;

		_LIT(KLogConDeact, "UMTSSim: Controller resuming context");
		_LIT(KLogName, "Context = ");
		TBuf<65> namebuf;
		namebuf = KLogName;
		namebuf.Append(name);
		Log(KLogConDeact, namebuf);

		return context->Resume();
        }
	return err;
    }

TInt CUmtsSimulator::ActSetEventTrigger(const TDesC* aParams, TAny* aOptionalParam)
    {
	if(aOptionalParam != NULL || aParams == NULL)
		return KErrArgument;

	TInt eventId = 0;
	TInt triggerId = 0;

	TLex lex(*aParams);
	TInt err = KErrNone;
	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'i': // id of event to trigger another
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				err = lex.Val(eventId);
				if(err != KErrNone) continue;
				lex.Mark();
                }
                break;
                case 't': // id of event to be triggered
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				err = lex.Val(triggerId);
				if(err != KErrNone) continue;
				lex.Mark();
                }
                break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		if(eventId < 1)
			return KErrArgument;

		_LIT(KLogSetTri, "UMTSSim: Controller setting event trigger");
		_LIT(KLogCfg, "Cfg = \"");
		_LIT(KLogCfgEnd, "\"");
		HBufC* paramBuf = NULL; 
		TRAPD(err, paramBuf = HBufC::NewL(aParams->Length() + 8));
        if (err != KErrNone)
            {
            LOG(Log::Printf(_L("HBufC::NewL error: %d"), err));
            }
		TPtr paramPtr = paramBuf->Des();
		paramPtr.Append(KLogCfg);
		paramPtr.Append(*aParams);
		paramPtr.Append(KLogCfgEnd);
		Log(KLogSetTri, paramPtr);
		delete paramBuf;

		return iEventController->SetTrigger(eventId, triggerId);
        }
	return err;
    }

TInt CUmtsSimulator::ActDoNothing(const TDesC*, TAny*)
    {
	return KErrNone;
    }

TInt CUmtsSimulator::ActConfigureRequest(const TDesC* aCfgLine, TAny* aOptionalParam)
    {
	if(aOptionalParam != NULL || aCfgLine == NULL)
		return KErrArgument;

	_LIT(KLogConConf, "UMTSSim: Controller configuring request handler");
	_LIT(KLogCfg, "Cfg = \"");
	_LIT(KLogCfgEnd, "\"");
	HBufC* paramBuf = 0;
	TRAPD(err, paramBuf = HBufC::NewL(aCfgLine->Length() + 8));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), err));
        }
	TPtr paramPtr = paramBuf->Des();
	paramPtr.Append(KLogCfg);
	paramPtr.Append(*aCfgLine);
	paramPtr.Append(KLogCfgEnd);
	Log(KLogConConf, paramPtr);
	delete paramBuf;

	return iRequestManager->HandleRequestLine(*aCfgLine);
    }

CUmtsSimulator::TUmtsRequestStatus CUmtsSimulator::CheckRequest(TUint aRequestCode,
																TInt& aReturnStatus,
																TInt& aDelay_ms)
    {
	CSimRequestManager::CRequestHandler* handler = iRequestManager->GetHandler(aRequestCode);
	if(!handler)
		handler = iRequestManager->GetDefaultHandler();

	return handler->Request(aReturnStatus, aDelay_ms);
    }

TInt CUmtsSimulator::ReconfigureReqMan(const TDesC& aFilename)
    {
	_LIT(KLogManagerFail, "ERROR: RequestManager configuration failed");

	TBuf<255> file;
	TInt err = FindFile(aFilename, file);
	if(err != KErrNone)
        {
		Log(KLogManagerFail, file);
		return err;
        }

	err = iRequestManager->Configure(file);
	_LIT(KLogCfg, "File = \"");
	_LIT(KLogCfgEnd, "\"");
	HBufC* paramBuf = 0; 
	TRAPD(trap_err, paramBuf = HBufC::NewL(file.Length() + 9));
    if (trap_err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), trap_err));
        }
	TPtr paramPtr = paramBuf->Des();
	paramPtr.Append(KLogCfg);
	paramPtr.Append(file);
	paramPtr.Append(KLogCfgEnd);
	if(err != KErrNone)
        {
		Log(KLogManagerFail, paramPtr);
		_LIT(KLogManagerErr, "ERROR: Error code was ");
		TBuf<40> buf;
		buf = KLogManagerErr;
		buf.AppendNum(err);
		Log(buf);
        }
	else
        {
		_LIT(KLogManagerOk, "UMTSSim: RequestManager configured.");
		Log(KLogManagerOk, paramPtr);
        }
	delete paramBuf;
	return err;
    }

TInt CUmtsSimulator::ReconfigureEventCtrl(const TDesC& aFilename)
    {
	_LIT(KLogCtrlFail, "ERROR: EventController configuration failed");

	TBuf<255> file;
	TInt err = FindFile(aFilename, file);
	if(err != KErrNone)
        {
		Log(KLogCtrlFail, file);
		return err;
        }

	err = iEventController->Configure(file);
	_LIT(KLogCfg, "File = \"");
	_LIT(KLogCfgEnd, "\"");
	HBufC* paramBuf = 0;
	TRAPD(trap_err, paramBuf = HBufC::NewL(file.Length() + 9));
    if (trap_err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), trap_err));
        }
	TPtr paramPtr = paramBuf->Des();
	paramPtr.Append(KLogCfg);
	paramPtr.Append(file);
	paramPtr.Append(KLogCfgEnd);
	if(err != KErrNone)
        {
		Log(KLogCtrlFail, paramPtr);
		_LIT(KLogCtrlErr, "ERROR: Error code was ");
		TBuf<40> buf;
		buf = KLogCtrlErr;
		buf.AppendNum(err);
		Log(buf);
        }
	else
        {
		_LIT(KLogCtrlOk, "UMTSSim: EventController configured.");
		Log(KLogCtrlOk, paramPtr);
        }
	delete paramBuf;
	return err;
    }

TInt CUmtsSimulator::FindFile(const TDesC& aHintPathAndFile, TDes& aFullOrErrorMsg)
    {
	TParse parser;
	_LIT(KDefault, "Y:");
	TInt err = parser.SetNoWild(aHintPathAndFile, NULL, &(KDefault()));
	if(err != KErrNone)
        {
		_LIT(KErrParsing, "Error parsing given filename [");
		_LIT(KErr2, "] \"");
		_LIT(KErr3, "\"");
		aFullOrErrorMsg = KErrParsing();
		aFullOrErrorMsg.AppendNum(err);
		aFullOrErrorMsg.Append(KErr2);
		aFullOrErrorMsg.Append(aHintPathAndFile.Left
                               (aFullOrErrorMsg.MaxLength()-aFullOrErrorMsg.Length()-1));
		aFullOrErrorMsg.Append(KErr3);
		return err;
        }

	RFs fs;
	err = fs.Connect();
	if(err != KErrNone)
        {
		fs.Close();
		_LIT(KErrOpen, "Error opening FileServer session");
		aFullOrErrorMsg = KErrOpen();
		return err;
        }

	TFindFile finder(fs);
	err = finder.FindByDir(parser.NameAndExt(), parser.DriveAndPath());

	if(err == KErrNone)
        {
		aFullOrErrorMsg = finder.File();
        }
	else
        {
		_LIT(KErrNF, "[");
		_LIT(KErr2, "] Could not find file \"");
		_LIT(KErr3, "\" on any drive");

		aFullOrErrorMsg = KErrNF();
		aFullOrErrorMsg.AppendNum(err);
		aFullOrErrorMsg.Append(KErr2);
		aFullOrErrorMsg.Append(parser.FullName().Right(parser.FullName().Length()-2).Left
                               (aFullOrErrorMsg.MaxLength()-aFullOrErrorMsg.Length()-KErr3().Length()));
		aFullOrErrorMsg.Append(KErr3);
        }

	fs.Close();
	return err;
    }

TInt CUmtsSimulator::ConfigureRequestHandler(const TDesC& aCfg)
    {
	_LIT(KLogCfg2, "Cfg = \"");
	_LIT(KLogCfg3, "\"");
	HBufC* buf = NULL;
	TRAPD(err1,buf = HBufC::NewL(aCfg.Length() + 8));
    if (err1 != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), err1));
        }
	//buf = HBufC::NewL(aCfg.Length() + 8);
	TPtr bufPtr = buf->Des();
	bufPtr = KLogCfg2;
	bufPtr.Append(aCfg);
	bufPtr.Append(KLogCfg3);

	TInt err = iRequestManager->HandleRequestLine(aCfg);
	if(err != KErrNone)
        {
		_LIT(KLogCfgErr, "ERROR: RequestHandler configuration failed");
		Log(KLogCfgErr, bufPtr);
        }
	else
        {
		_LIT(KLogCfgSingle, "UMTSSim: Configured single RequestHandler");
		Log(KLogCfgSingle, bufPtr);
        }

	delete buf;
	return err;
    }

TInt CUmtsSimulator::ConfigureEvent(const TDesC& aCfg)
    {
	_LIT(KLogCfg2, "Cfg = \"");
	_LIT(KLogCfg3, "\"");
	HBufC* buf = 0;
	TRAPD(trap_err, buf = HBufC::NewL(aCfg.Length() + 8));
    if (trap_err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), trap_err));
        }
	TPtr bufPtr = buf->Des();
	bufPtr = KLogCfg2;
	bufPtr.Append(aCfg);
	bufPtr.Append(KLogCfg3);

	TInt err = iEventController->HandleEventLine(aCfg);
	if(err == KErrAlreadyExists)
        {
		_LIT(KLogCfgErr, "ERROR: Event configuration failed (id in use)");
		Log(KLogCfgErr, bufPtr);
        }
	else if(err != KErrNone)
        {
		_LIT(KLogCfgErr, "ERROR: Event configuration failed");
		Log(KLogCfgErr, bufPtr);
        }
	else
        {
		_LIT(KLogCfgSingle, "UMTSSim: Configured single Event");
		Log(KLogCfgSingle, bufPtr);
        }

	delete buf;
	return err;
    }

CSimEventController* CUmtsSimulator::GetEventCtrl()
    {
	return iEventController;
    }

void CUmtsSimulator::SafeLog(const TDesC& aEntry)
    {
	LOG(Log::Printf(aEntry));
    }

void CUmtsSimulator::Log(const TDesC& aEntry)
    {
	SafeLog(aEntry);

	// prefix log entry with a single hex number
	const TDesC* entry = &aEntry;
	HBufC* prefixEntry = HBufC::New(aEntry.Length()+3);
	if(prefixEntry)
        {
		TPtr16 prePtr = prefixEntry->Des();
		prePtr.Num((iLogIndex++)%16, EHex);
		_LIT(KLogFill, ": ");
		prePtr.Append(KLogFill);
		prePtr.Append(aEntry);
		entry = prefixEntry;
        }

	// inform all simulation control instances about event
	TSglQueIter<CUmtsSimServSession>& iterator = AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iControlWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// because there is a bug in CObjectIx we need to hack here
			CControlApiWrapper* ptr = (CControlApiWrapper*)(*session->iControlWrappers)[idx];
			if(ptr == NULL)
                {
                //				_LIT(KLogBug, "ERROR: Simulator::Log() found CObjectIx bug");
				//SafeLog(KLogBug);
				count++;
                }
			else
				ptr->Log(*entry);

			// ((CControlApiWrapper*)(*session->iControlWrappers)[idx])->Log(*entry);
            }
		session = iterator++;
        }
	FreeSessionIterator(iterator);

	if(prefixEntry)
		delete prefixEntry;
    }

void CUmtsSimulator::Log(const TDesC& aEntry, const TDesC& aData)
    {
	HBufC* combined = HBufC::New(aEntry.Length()+aData.Length()+5);
	if(combined == NULL)
        {
		_LIT(KLogFailed, "ERROR: Simulator::Log() failed buffer allocation");
		Log(KLogFailed);
		Log(aEntry);
		Log(aData);
        }
	else
        {
		TPtr16 ptr = combined->Des();
		ptr = aEntry;
		ptr.Append(' ');
		ptr.Append('[');
		ptr.Append(aData);
		ptr.Append(']');
		Log(ptr);
		delete combined;
        }
    }

void CUmtsSimulator::DebugCheck()
    {
    #ifdef USSE_DEBUG_CHECKS
	TSglQueIter<CUmtsSimServSession>& iterator = AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		session->DebugCheck(this);
		session = iterator++;
        }
	FreeSessionIterator(iterator);
    #endif
    }


/* **************************
    * CSimTimer class
    * ************************** */

CSimTimer* CSimTimer::NewL()
    {
	CSimTimer* self = new (ELeave) CSimTimer();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimTimer::CSimTimer() : CTimer(CActive::EPriorityStandard)
    {
	// nothing to do
    }

CSimTimer::~CSimTimer()
    {
	Cancel();
	delete iRequests;
    }

void CSimTimer::ConstructL()
    {
	iRequests = CRequestQueue::NewL();

	CTimer::ConstructL();
	CActiveScheduler::Add(this);
    }

void CSimTimer::RunL()
    {
	// Used timing method At() is not documented to give error when delay is small
	// (or negative), but that seems to be the case. Accepting KErrUnderflow may
	// not seem nice but it is necessary, as RequestCallAfter is guarantereed to
	// execute callback asynchronously. (This is necessary because requests are
	// made in situations where associated callbacks cannot be executed safely.)
	if(iStatus != KErrNone && iStatus != KErrUnderflow)
        {
		User::Invariant();
        }

	TRequest* request = iRequests->GetFirst();
	iRequests->Remove(request->GetHandle());

	TRequest* newRequest = iRequests->GetFirst();
	if(newRequest)
		At(newRequest->iTime);

	request->iCallback(request->iParam);
	delete request;
    }

TInt CSimTimer::RequestCallAfterL(TTimeIntervalMicroSeconds32 aPeriod, CUmtsSimulator::TCallback aCallback, TAny* aParam)
    {
	TRequest* request = new TRequest;
	request->iCallback = aCallback;
	request->iParam = aParam;

	request->iTime.HomeTime();
	request->iTime += aPeriod;

	TUint handle = iRequests->AddL(request);
	if(request == iRequests->GetFirst())
        {
		Cancel();
		At(request->iTime);
        }
	return handle;
    }

TAny* CSimTimer::RemoveRequest(TInt aRequest)
    {
	TRequest* oldFirst = iRequests->GetFirst();
	TRequest* request = iRequests->Remove(aRequest);

	if(oldFirst == request)
        {
		Cancel();
		TRequest* newFirst = iRequests->GetFirst();
		if(newFirst)
			At(newFirst->iTime);
        }

	TAny* ret = request->iParam;
	delete request;
	return ret;
    }


/* **************************
    * TRequest class
    * ************************** */

TBool CSimTimer::TRequest::operator<(const TRequest& aRequest)
    {
	return iTime < aRequest.iTime;
    }


/* **************************
    * CRequestQueue class
    * ************************** */

CSimTimer::CRequestQueue* CSimTimer::CRequestQueue::NewL()
    {
	CRequestQueue* self = new (ELeave) CRequestQueue();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimTimer::CRequestQueue::CRequestQueue() : iRequests(0), iCount(1)
    {
	// nothing to do
    }

CSimTimer::CRequestQueue::~CRequestQueue()
    {
	TRequest* req = iRequests;
	while(req)
        {
		TRequest* temp = req->iNext;
		delete req;
		req = temp;
        }
    }

void CSimTimer::CRequestQueue::ConstructL()
    {
	iRequests = new TRequest;
	iRequests->iCallback = 0;
	iRequests->iParam = 0;
	iRequests->iHandle = 0;
	iRequests->iNext = 0;
    }

CSimTimer::TRequest* CSimTimer::CRequestQueue::GetFirst()
    {
	return iRequests->iNext;
    }

TUint CSimTimer::CRequestQueue::AddL(TRequest* aRequest)
    {
	aRequest->iHandle = iCount++;
	TRequest* req0 = iRequests;
	TRequest* req1 = req0->iNext;
	while(req1)
        {
		if(*aRequest < *req1)
            {
			req0->iNext = aRequest;
			aRequest->iNext = req1;
			return aRequest->iHandle;
            }
		req0 = req1;
		req1 = req1->iNext;
        }
	req0->iNext = aRequest;
	aRequest->iNext = 0;
	return aRequest->iHandle;
    }

CSimTimer::TRequest* CSimTimer::CRequestQueue::Remove(TUint aHandle)
    {
	TRequest* req0 = iRequests;
	TRequest* req1 = req0->iNext;
	while(req1)
        {
		if(req1->iHandle == aHandle)
            {
			req0->iNext = req1->iNext;
			return req1;
            }
		req0 = req1;
		req1 = req1->iNext;
        }
	return NULL;
    }

/* **************************
    * TSimNifList class
    * ************************** */

TSimNifList::TSimNifList() : iNifs(NULL), iNifCount(0) {}

TSimNifList::~TSimNifList()
    {
	TListEntry* nif = iNifs;
	while(nif)
        {
		TContextEntry* context = nif->iContexts;
		while(context)
            {
			TContextEntry* next = context->iNext;
			delete context;
			context = next;
            }
		TListEntry* next = nif->iNext;
		delete nif;
		nif = next;
        }
    }

void TSimNifList::AddNewPrimary(const CPacketContext* aContext)
    {
	TContextEntry* nifContext = new TContextEntry;
	nifContext->iContext = aContext;
	nifContext->iNext = NULL;
	TListEntry* nif = new TListEntry;
	nif->iContexts = nifContext;
	nif->iNext = iNifs;
	nif->iContextCount = 1;
	iNifs = nif;
	iNifCount++;
    }

void TSimNifList::AddNewSecondary(const CPacketContext* aContext, const CPacketContext* aFather)
    {
	TListEntry* nif = iNifs;
	while(nif)
        {
		TContextEntry* context = nif->iContexts;
		while(context)
            {
			if(context->iContext == aFather)
                {
				TContextEntry* child = new TContextEntry;
				child->iContext = aContext;
				child->iNext = context->iNext;
				context->iNext = child;
				nif->iContextCount++;
				return;
                }
			context = context->iNext;
            }
		nif = nif->iNext;
        }

	User::Invariant(); // server-side error
    }

void TSimNifList::Remove(const CPacketContext* aContext)
    {
	TListEntry* nif = iNifs;
	TListEntry** prevNif = &iNifs;
	while(nif)
        {
		TContextEntry* context = nif->iContexts;
		TContextEntry** prevContext = &(nif->iContexts);
		while(context)
            {
			if(context->iContext == aContext)
                {
				*prevContext = context->iNext;
				delete context;

				if(!(--(nif->iContextCount)))
                    {
					iNifCount--;
					*prevNif = nif->iNext;
					delete nif;
                    }

				return;
                }
			prevContext = &((*prevContext)->iNext);
			context = context->iNext;
            }
		prevNif = &((*prevNif)->iNext);
		nif = nif->iNext;
        }

	// NOTE: If context creation fails (out-of-memory etc.) this method might be called as cleanup
	// before context was added here -- so it is NOT error if nothing is found!
	// User::Invariant(); // server-side error
    }

TInt TSimNifList::GetNifCount()
    {
	return iNifCount;
    }

TInt TSimNifList::GetContextCount(const CPacketContext* aContext)
    {
	TListEntry* nif = iNifs;
	while(nif)
        {
		TContextEntry* context = nif->iContexts;
		while(context)
            {
			if(context->iContext == aContext)
                {
				return nif->iContextCount;
                }
			context = context->iNext;
            }
		nif = nif->iNext;
        }

	User::Invariant(); // server-side error
	return -1; // never reached
    }

TInt TSimNifList::GetNifInfo(TInt aIndex, RPacketService::TNifInfoV2& aNifInfo)
    {
	if( aIndex < 0 ) return KErrArgument;

	TListEntry* nif = iNifs;
	while((--aIndex) >= 0 && nif != NULL)
		nif = nif->iNext;
	if(!nif) return KErrArgument;

	aNifInfo.iContextName = nif->iContexts->iContext->GetName();
	aNifInfo.iNumberOfContexts = nif->iContextCount;
	aNifInfo.iNifStatus = RPacketContext::EStatusInactive;

	TContextEntry* context = nif->iContexts;
	while(context)
        {
		switch(context->iContext->GetStatus())
            {
                case RPacketContext::EStatusActivating:
                case RPacketContext::EStatusActive:
                case RPacketContext::EStatusSuspended:
                    aNifInfo.iNifStatus = RPacketContext::EStatusActive;
                    context = NULL;
                    continue;
                    // unknown and deactivating could be handled otherwise also?
                case RPacketContext::EStatusUnknown:
                case RPacketContext::EStatusDeactivating:
                case RPacketContext::EStatusInactive:
                case RPacketContext::EStatusDeleted:
                    break;
                default:
                    User::Invariant();
            }

		context = context->iNext;
        }

	_LIT8(KAddressDefaultDebug, "3.14.15.92");
	aNifInfo.iPdpAddress = KAddressDefaultDebug;
	aNifInfo.iContextType = RPacketService::EInternalContext;

	return KErrNone;
    }

TInt TSimNifList::GetContextName(TInt aIndex, TDes& aName, const CPacketContext* aContext)
    {
	TListEntry* nif = iNifs;
	while(nif)
        {
		TContextEntry* context = nif->iContexts;
		while(context)
            {
			if(context->iContext == aContext)
                {
				if(aIndex < 0 || aIndex >= nif->iContextCount)
					return KErrArgument;
				TContextEntry* ccount = nif->iContexts;
				while((--aIndex) >= 0)
					ccount = ccount->iNext; // shouldn't fail
				aName = ccount->iContext->GetName();
				return KErrNone;
                }
			context = context->iNext;
            }
		nif = nif->iNext;
        }

	User::Invariant(); // server-side error
	return KErrGeneral; // never reached
    }


/* **************************
    * CSimRequestManager class
    * ************************** */

CSimRequestManager* CSimRequestManager::NewL(CUmtsSimulator* aSimulator)
    {
	CSimRequestManager* self = new (ELeave) CSimRequestManager(aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimRequestManager::CSimRequestManager(CUmtsSimulator* aSimulator)
	: iDefaultHandler(NULL), iSimulator(aSimulator)
    {
	// nothing to do
    }

CSimRequestManager::~CSimRequestManager()
    {
	iHandlers.ResetAndDestroy();

	if(iDefaultHandler)
        {
		delete iDefaultHandler;
		iDefaultHandler = NULL;
        }
    }

void CSimRequestManager::ConstructL()
    {
	iDefaultHandler = CRequestHandler::NewL(0, iSimulator);
    }

void CSimRequestManager::Clear()
    {
	SetDefaultHandler(NULL);
	iHandlers.ResetAndDestroy();
    }

TInt CSimRequestManager::Configure(const TDesC& aFilename)
    {
	Clear();

	RFs fileserver;
	TInt err = fileserver.Connect();
	if(err != KErrNone)
		return err;
	RFile file;
	err = file.Open(fileserver, aFilename, EFileStreamText|EFileRead|EFileShareReadersOnly);
	if(err != KErrNone)
        {
		fileserver.Close();
		return err;
        }
	TFileText textFile;
	textFile.Set(file);

	_LIT(KSectionReqMan, "!Section-ReqMan");
	TBool section = EFalse;

	TBuf<200> buffer; // intentionally smaller than 256 so that we get an error with long lines
	TInt line = 1;
	err = textFile.Read(buffer);
	while(err == KErrNone)
        {
		if(buffer == KSectionReqMan)
			section = ETrue;
		else {
        if(section)
			{
            if(buffer.Left(9) == KSectionReqMan().Left(9)) // "!Section-" but not ReqMan
                section = EFalse;
            else
				{
                err = HandleRequestLine(buffer);
                if(err != KErrNone) break;
				}
			}
		}

		err = textFile.Read(buffer);
		line++;
        }

	file.Close();
	fileserver.Close();

	if(err != KErrEof)
        {
		_LIT(KLogError, "ERROR: Error in configuration file on line ");
		TBuf<50> error;
		error = KLogError;
		error.AppendNum(line);
		iSimulator->Log(error, buffer);

		Clear();
		return err;
        }

	return KErrNone;
    }

CSimRequestManager::CRequestHandler* CSimRequestManager::GetDefaultHandler() const
    {
	return iDefaultHandler;
    }

void CSimRequestManager::SetDefaultHandler(CSimRequestManager::CRequestHandler* aHandler)
    {
	if(iDefaultHandler)
        {
		delete iDefaultHandler;
		iDefaultHandler = NULL;
        }
	if(aHandler)
		iDefaultHandler = aHandler;
	else
        {
		TRAPD(err, iDefaultHandler = CRequestHandler::NewL(0, iSimulator));
        if (err != KErrNone)
            {
            LOG(Log::Printf(_L("CRequestHandler::NewL error: %d"), err));
            }
        }
    }

void CSimRequestManager::SetHandler(TUint aRequestCode, CSimRequestManager::CRequestHandler* aHandler)
    {
	if(aHandler && aHandler->CheckForMatch(aRequestCode) == EFalse)
		User::Invariant();

	TInt index = GetHandlerIndex(aRequestCode);
	if(index == KErrNotFound)
        {
		// do nothing if handler not found and new handler == NULL
		if(aHandler && iHandlers.Append(aHandler) != KErrNone)
			User::Invariant();
        }
	else
        {
		CRequestHandler* old = iHandlers[index];
		if(aHandler)
			iHandlers[index] = aHandler;
		else
			iHandlers.Remove(index);
		delete old;
        }
    }

CSimRequestManager::CRequestHandler* CSimRequestManager::GetHandler(TUint aRequestCode)
    {
	TInt index = GetHandlerIndex(aRequestCode);
	if(index == KErrNotFound)
		return NULL;
	return iHandlers[index];
    }

TInt CSimRequestManager::GetHandlerIndex(TUint aRequestCode)
    {
	TInt count = iHandlers.Count();
	for(TInt i = 0; i < count; i++)
		if(iHandlers[i]->CheckForMatch(aRequestCode))
			return i;
	return KErrNotFound;
    }

TInt CSimRequestManager::HandleRequestLine(const TDesC& aLine)
    {
	TLex lex(aLine);
	if(lex.Peek().IsSpace())
		lex.SkipSpaceAndMark();
	if(lex.Eos() || lex.Peek() == '#')
		return KErrNone;

	lex.SkipCharacters(); // move char pos over context.id -field
	TUint code = 0;
	TInt err = GetRequestCode(lex.MarkedToken(), code);
	if(err != KErrNone) return err;

	CSimRequestManager::CRequestHandler* handler = NULL;
	TRAP(err, handler = CSimRequestManager::CRequestHandler::NewL(code, iSimulator));
	if(err != KErrNone) return err;

	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'p': // set probability
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt prob = 0;
				err = lex.Val(prob);
				if(err != KErrNone) continue;

				TReal probReal = TReal(prob)/100.0;
				if(probReal < 0 || probReal > 1)
                    {
					err = KErrArgument;
					continue;
                    }
				handler->SetProbability(probReal);

				lex.Mark();
                }
                break;
                case 'd': // set delay
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt delay = 0;
				err = lex.Val(delay);
				if(err != KErrNone) continue;

				handler->SetDelay(delay);

				lex.Mark();
                }
                break;
                case 'f': // set request status returned when want to fail
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt code = 0;
				err = lex.Val(code);
				if(err != KErrNone) continue;

				handler->SetFailStatus(code);

				lex.Mark();
                }
                break;
                case 't': // makes this request to trigger/cancel an event
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt id = 0;
				err = lex.Val(id);
				if(err != KErrNone) continue;

				handler->Triggers(id);

				lex.Mark();
                }
                break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		if(code)
			SetHandler(code, handler);
		else
			SetDefaultHandler(handler);
		return KErrNone;
        }
	delete handler;
	return err;
    }

TInt CSimRequestManager::GetRequestCode(const TDesC& aName, TUint& aCode)
    {
	TInt length = aName.Length();
	if(length > 60)
		return KErrTooBig;;

	TBuf<60> name = aName;
	name.LowerCase();
	TInt period = name.Locate('.');
	
	if(period == KErrNotFound || period == 0 || period > length-2)
		return KErrCorrupt;

	TPtrC context = name.Left(period);
	TPtrC id = aName.Right(length-period-1); // case sensitive

	TBool isCancel = EFalse;
	if(id.Length() > 6)
        {
		_LIT(KIdCancel, "Cancel");
		if(id.Right(6) == KIdCancel)
            {
			isCancel = ETrue;
			id.Set(id.Left(id.Length()-6));
            }
        }

	_LIT(KContextService, "service");
	_LIT(KContextContext, "context");
	_LIT(KContextQoS, "qos");
	_LIT(KContextDefault, "default");
	if(context == KContextService)
        {
		_LIT(KOpen, "Open");
		_LIT(KClose, "Close");
		_LIT(KNotifyContextAdded, "NotifyContextAdded");
		_LIT(KAttach, "Attach");
		_LIT(KDetach, "Detach");
		_LIT(KGetStatus, "GetStatus");
		_LIT(KNotifyStatusChange, "NotifyStatusChange");
		_LIT(KEnumerateContexts, "EnumerateContexts");
		_LIT(KGetContextInfo, "GetContextInfo");
		_LIT(KEnumerateNifs, "EnumerateNifs");
		_LIT(KGetNifInfo, "GetNifInfo");
		_LIT(KEnumerateContextsInNif, "EnumerateContextsInNif");
		_LIT(KGetContextNameInNif, "GetContextNameInNif");
		if(id == KOpen) aCode = EUmtsSimServCreatePacketServiceSubSession;
		else if(id == KClose) aCode = EUmtsSimServClosePacketServiceSubSession;
		else if(id == KNotifyContextAdded) aCode = EUmtsSimServPacketServiceNotifyContextAddedA;
		else if(id == KAttach) aCode = EUmtsSimServPacketServiceAttachA;
		else if(id == KDetach) aCode = EUmtsSimServPacketServiceDetachA;
		else if(id == KGetStatus) aCode = EUmtsSimServPacketServiceGetStatusS;
		else if(id == KNotifyStatusChange) aCode = EUmtsSimServPacketServiceNotifyStatusChangeA;
		else if(id == KEnumerateContexts) aCode = EUmtsSimServPacketServiceEnumerateContextsA;
		else if(id == KGetContextInfo) aCode = EUmtsSimServPacketServiceGetContextInfoA;
		else if(id == KEnumerateNifs) aCode = EUmtsSimServPacketServiceEnumerateNifsA;
		else if(id == KGetNifInfo) aCode = EUmtsSimServPacketServiceGetNifInfoA;
		else if(id == KEnumerateContextsInNif) aCode = EUmtsSimServPacketServiceEnumerateContextsInNifA;
		else if(id == KGetContextNameInNif) aCode = EUmtsSimServPacketServiceGetContextNameInNifA;
		else return KErrNotFound;
		if(isCancel)
			aCode |= KUmtsSimServRqstCancelBit; // doesn't check sanity here
		return KErrNone;
        }
	else if(context == KContextContext)
        {
		_LIT(KOpenNewContext, "OpenNewContext");
		_LIT(KOpenNewSecondaryContext, "OpenNewSecondaryContext");
		_LIT(KOpenExistingContext, "OpenExistingContext");
		_LIT(KClose, "Close");
		_LIT(KInitialiseContext, "InitialiseContext");
		_LIT(KSetConfig, "SetConfig");
		_LIT(KGetConfig, "GetConfig");
		_LIT(KNotifyConfigChanged, "NotifyConfigChanged");
		_LIT(KActivate, "Activate");
		_LIT(KDeactivate, "Deactivate");
		_LIT(KDelete, "Delete");
		_LIT(KGetStatus, "GetStatus");
		_LIT(KNotifyStatusChange, "NotifyStatusChange");
		_LIT(KEnumeratePacketFilters, "EnumeratePacketFilters");
		_LIT(KGetPacketFilterInfo, "GetPacketFilterInfo");
		_LIT(KAddPacketFilter, "AddPacketFilter");
		_LIT(KRemovePacketFilter, "RemovePacketFilter");
		_LIT(KModifyActiveContext, "ModifyActiveContext");
		if(id == KOpenNewContext) aCode = EUmtsSimServCreatePacketContextSubSession;
		else if(id == KOpenNewSecondaryContext) aCode = EUmtsSimServCreatePacketContextSubSession;
		else if(id == KOpenExistingContext) aCode = EUmtsSimServCreatePacketContextSubSession;
		else if(id == KClose) aCode = EUmtsSimServClosePacketContextSubSession;
		else if(id == KInitialiseContext) aCode = EUmtsSimServPacketContextInitialiseContextA;
		else if(id == KSetConfig) aCode = EUmtsSimServPacketContextSetConfigA;
		else if(id == KGetConfig) aCode = EUmtsSimServPacketContextGetConfigA;
		else if(id == KNotifyConfigChanged) aCode = EUmtsSimServPacketContextNotifyConfigChangedA;
		else if(id == KActivate) aCode = EUmtsSimServPacketContextActivateA;
		else if(id == KDeactivate) aCode = EUmtsSimServPacketContextDeactivateA;
		else if(id == KDelete) aCode = EUmtsSimServPacketContextDeleteA;
		else if(id == KGetStatus) aCode = EUmtsSimServPacketContextGetStatusS;
		else if(id == KNotifyStatusChange) aCode = EUmtsSimServPacketContextNotifyStatusChangeA;
		else if(id == KEnumeratePacketFilters) aCode = EUmtsSimServPacketContextEnumeratePacketFiltersA;
		else if(id == KGetPacketFilterInfo) aCode = EUmtsSimServPacketContextGetPacketFilterInfoA;
		else if(id == KAddPacketFilter) aCode = EUmtsSimServPacketContextAddPacketFilterA;
		else if(id == KRemovePacketFilter) aCode = EUmtsSimServPacketContextRemovePacketFilterA;
		else if(id == KModifyActiveContext) aCode = EUmtsSimServPacketContextModifyActiveContextA;
		else return KErrNotFound;
		if(isCancel)
			aCode |= KUmtsSimServRqstCancelBit; // doesn't check sanity here
		return KErrNone;
        }
	else if(context == KContextQoS)
        {
		_LIT(KOpenNewQoS, "OpenNewQoS");
		_LIT(KOpenExistingQoS, "OpenExistingQoS");
		_LIT(KClose, "Close");
		_LIT(KSetProfileParameters, "SetProfileParameters");
		_LIT(KGetProfileParameters, "GetProfileParameters");
		if(id == KOpenNewQoS) aCode = EUmtsSimServCreatePacketQoSSubSession;
		else if(id == KOpenExistingQoS) aCode = EUmtsSimServCreatePacketQoSSubSession;
		else if(id == KClose) aCode = EUmtsSimServClosePacketQoSSubSession;
		else if(id == KSetProfileParameters) aCode = EUmtsSimServPacketQoSSetProfileParametersA;
		else if(id == KGetProfileParameters) aCode = EUmtsSimServPacketQoSGetProfileParametersA;
		else return KErrNotFound;
		if(isCancel)
			aCode |= KUmtsSimServRqstCancelBit; // doesn't check sanity here
		return KErrNone;
        }
	else if(context == KContextDefault)
        {
		_LIT(KDefault, "default.default");
		if(name == KDefault) aCode = 0; // compares name, not id, so case insensitive
		else return KErrNotFound;
		return KErrNone;
        }

	return KErrNotFound;
    }


/* **************************
    * CRequestHandler class
    * ************************** */

CSimRequestManager::CRequestHandler*
    CSimRequestManager::CRequestHandler::NewL(TUint aRequestCode, CUmtsSimulator* aSimulator)
    {
	CSimRequestManager::CRequestHandler* self =
		new (ELeave) CSimRequestManager::CRequestHandler(aRequestCode, aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimRequestManager::CRequestHandler::CRequestHandler(TUint aRequestCode, CUmtsSimulator* aSimulator)
	: iRequestCode(aRequestCode), iHandlerStatus(EHStatusDefault), iLocked(EFalse), iDelay(200),
          iProbability(1.0), iRandSeed(0), iFailStatus(KErrGeneral), iTriggerId(0), iSimulator(aSimulator)
    {
	// nothing to do
    }

CSimRequestManager::CRequestHandler::~CRequestHandler()
    {
	// nothing to do
    }

void CSimRequestManager::CRequestHandler::ConstructL()
    {
	TTime time;
	time.HomeTime();
	iRandSeed = time.Int64();
    }

TBool CSimRequestManager::CRequestHandler::CheckForMatch(TUint aRequestCode)
    {
	return aRequestCode == iRequestCode;
    }

CUmtsSimulator::TUmtsRequestStatus
    CSimRequestManager::CRequestHandler::Request(TInt& aReturnStatus, TInt& aDelay_ms)
    {
	if(iTriggerId != 0)
        {
		TInt err = KErrNone;
		if(iTriggerId < 0)
			err = iSimulator->GetEventCtrl()->Cancel(-iTriggerId);
		else
			err = iSimulator->GetEventCtrl()->Trigger(iTriggerId, NULL);

		_LIT(KLogEventReq, "Req = 0x");
		_LIT(KLogEventEve, ", Event = ");
		TBuf<50> info;
		info = KLogEventReq;
		info.AppendNum(iRequestCode, EHex);
		info.Append(KLogEventEve);
		info.AppendNum(iTriggerId<0 ? -iTriggerId : iTriggerId);
		
		if(err != KErrNone)
            {
			_LIT(KLogEventError, "ERROR: RequestHandler failed triggering/cancelling an event");
			iSimulator->Log(KLogEventError, info);
            }
		else
            {
			if(iTriggerId < 0)
                {
				_LIT(KLogEventCancel, "UMTSSim: RequestHandler cancelled an event");
				iSimulator->Log(KLogEventCancel, info);
                }
			else
                {
				_LIT(KLogEventTrig, "UMTSSim: RequestHandler triggered an event");
				iSimulator->Log(KLogEventTrig, info);
                }
            }
        }
	
	aDelay_ms = iDelay;

	switch(iHandlerStatus)
        {
            case EHStatusPermit:
                break;
            case EHStatusDefault:
            {
			TInt rnd = Math::Rand(iRandSeed);
			rnd = rnd % (137*29); // to create non-uniform rand
			if((TReal)rnd/((TReal)137*29) >= (1.0-iProbability))
				break;
            }
            case EHStatusRefuse:
            {
			_LIT(KLogErrorGenerated, "UMTSSim: RequestHandler generated a failure");
			_LIT(KLogCodePrefix, "Req = 0x");
			TBuf<30> requestNum;
			requestNum = KLogCodePrefix;
			requestNum.AppendNum(iRequestCode, EHex);
			iSimulator->Log(KLogErrorGenerated, requestNum);

			aReturnStatus = iFailStatus;
			return CUmtsSimulator::EURStatusFail;
			//break;
            }
            case EHStatusConfirm:
                aReturnStatus = KErrNone; // should this return iFailStatus so that fail given by confirmation
                // knows what to return to the client?
                return CUmtsSimulator::EURStatusConfirm;
                //break;
            default:
                User::Invariant();
                break;
        }

	aReturnStatus = KErrNone;
	return CUmtsSimulator::EURStatusNormal;
    }

void CSimRequestManager::CRequestHandler::Skip(TInt /*aN*/)
    {
	// does nothing yet
    }

void CSimRequestManager::CRequestHandler::Lock(void)
    {
	iLocked = ETrue;
    }

void CSimRequestManager::CRequestHandler::Unlock(void)
    {
	iLocked = EFalse;
    }

void CSimRequestManager::CRequestHandler::Permit(void)
    {
	iHandlerStatus = EHStatusPermit;
    }

void CSimRequestManager::CRequestHandler::Refuse(void)
    {
	iHandlerStatus = EHStatusRefuse;
    }

void CSimRequestManager::CRequestHandler::Confirm(void)
    {
	iHandlerStatus = EHStatusConfirm;
    }

void CSimRequestManager::CRequestHandler::Default(void)
    {
	iHandlerStatus = EHStatusDefault;
    }

void CSimRequestManager::CRequestHandler::Triggers(TInt aId)
    {
	iTriggerId = aId;
    }

void CSimRequestManager::CRequestHandler::SetDelay(TInt aDelay_ms)
    {
	if(aDelay_ms <= 0)
		User::Invariant();
	iDelay = aDelay_ms;
    }

void CSimRequestManager::CRequestHandler::SetProbability(TReal aProb)
    {
	if(aProb < 0.0 || aProb > 1.0)
		User::Invariant();
	iProbability = aProb;
    }

void CSimRequestManager::CRequestHandler::SetFailStatus(TInt aCode)
    {
	iFailStatus = aCode;
    }


/* **************************
    * CSimEventController class
    * ************************** */

CSimEventController* CSimEventController::NewL(CUmtsSimulator* aSimulator)
    {
	CSimEventController* self = new (ELeave) CSimEventController(aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimEventController::CSimEventController(CUmtsSimulator* aSimulator)
	: iSimulator(aSimulator)
    {
	// nothing to do
    }

CSimEventController::~CSimEventController()
    {
	iEvents.ResetAndDestroy();
    }

void CSimEventController::ConstructL()
    {
	// nothing to do
    }

void CSimEventController::Clear()
    {
	iEvents.ResetAndDestroy();
    }

TInt CSimEventController::Configure(const TDesC& aFilename)
    {
	Clear();

	RFs fileserver;
	TInt err = fileserver.Connect();
	if(err != KErrNone)
		return err;
	RFile file;
	err = file.Open(fileserver, aFilename, EFileStreamText|EFileRead|EFileShareReadersOnly);
	if(err != KErrNone)
        {
		fileserver.Close();
		return err;
        }
	TFileText textFile;
	textFile.Set(file);

	_LIT(KSectionEventCtrl, "!Section-EventCtrl");
	TBool section = EFalse;

	TBuf<200> buffer; // intentionally smaller than 256 so that we get an error with long lines
	TInt line = 1;
	err = textFile.Read(buffer);
	while(err == KErrNone)
        {
		if(buffer == KSectionEventCtrl)
			section = ETrue;
		else {
        if(section)
			{
            if(buffer.Left(9) == KSectionEventCtrl().Left(9)) // "!Section-" but not EventCtrl
                section = EFalse;
            else
				{
                err = HandleEventLine(buffer);
                if(err != KErrNone) break;
				}
			}
		}

		err = textFile.Read(buffer);
		line++;
        }

	file.Close();
	fileserver.Close();

	if(err != KErrEof)
        {
		_LIT(KLogError, "ERROR: Error in configuration file on line ");
		TBuf<50> error;
		error = KLogError;
		error.AppendNum(line);
		iSimulator->Log(error, buffer);

		Clear();
		return err;
        }

	return KErrNone;
    }

TInt CSimEventController::HandleEventLine(const TDesC& aLine)
    {
	TLex lex(aLine);
	if(lex.Peek().IsSpace())
		lex.SkipSpaceAndMark();
	if(lex.Eos() || lex.Peek() == '#')
		return KErrNone;

	lex.SkipCharacters(); // move char pos over action field
	TAction action = NULL;
	TInt err = GetAction(lex.MarkedToken(), action);
	if(err != KErrNone) return err;

	CSimEventController::CEvent* event = NULL;
	TRAP(err, event = CSimEventController::CEvent::NewL(action, iSimulator));
	if(err != KErrNone) return err;

	TBool triggered = EFalse;

	while(err == KErrNone)
        {
		lex.SkipSpaceAndMark();

		switch(lex.Get())
            {
                case '#': // comment, end line
                case 0:
                    err = KErrEof; // Eos actually
                    continue;
                case 'i': // set id
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt id = 0;
				err = lex.Val(id);
				if(err != KErrNone) continue;

				TRAP(err, event->SetIdL(id));
				if(err != KErrNone) continue;

				lex.Mark();
                }
                break;
                case 'd': // set delay
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt delay = 0;
				err = lex.Val(delay);
				if(err != KErrNone) continue;

				event->SetDelay(delay);

				lex.Mark();
                }
                break;
                case 'a': // activate: trigger action immediately (after loop to get right delay)
                {
				lex.Mark();
				triggered = ETrue;
                }
                break;
                case 't': // makes this event to trigger/cancel an event
                {
				lex.Mark();
				if(lex.Peek().IsSpace())
					lex.SkipSpaceAndMark();
				TInt id = 0;
				err = lex.Val(id);
				if(err != KErrNone) continue;

				event->Triggers(id);

				lex.Mark();
                }
                break;
                case '*': // rest of the line is event parameters
                {
				event->SetParams(lex.Remainder());
				err = KErrEof;
				continue;
                }
                //break;
                default:
                    err = KErrCorrupt;
                    continue;
            }
        }

	if(err == KErrEof)
        {
		err = KErrNone;
		if(triggered)
			err = event->Trigger(NULL);
		if(err == KErrNone)
            {
			err = AddEvent(event); // checks also that id is set
			if(err == KErrNone)
				return err;
            }
        }
	delete event;
	return err;
    }

TInt CSimEventController::GetAction(const TDesC& aName, TAction& aAction)
    {
	_LIT(KDeactivateContext, "DeactivateContext");
	_LIT(KSuspendContext, "SuspendContext");
	_LIT(KResumeContext, "ResumeContext");
	_LIT(KConfigureRequest, "ConfigureRequest");
	_LIT(KSetEventTrigger, "SetEventTrigger");
	_LIT(KDoNothing, "DoNothing");
	if(aName == KDeactivateContext)
        {
		aAction = &CUmtsSimulator::ActDeactivateContext;
		return KErrNone;
        }
	else if(aName == KSuspendContext)
        {
		aAction = &CUmtsSimulator::ActSuspendContext;
		return KErrNone;
        }
	else if(aName == KResumeContext)
        {
		aAction = &CUmtsSimulator::ActResumeContext;
		return KErrNone;
        }
	else if(aName == KConfigureRequest)
        {
		aAction = &CUmtsSimulator::ActConfigureRequest;
		return KErrNone;
        }
	else if(aName == KSetEventTrigger)
        {
		aAction = &CUmtsSimulator::ActSetEventTrigger;
		return KErrNone;
        }
	else if(aName == KDoNothing)
        {
		aAction = &CUmtsSimulator::ActDoNothing;
		return KErrNone;
        }
	return KErrNotFound;
    }

TInt CSimEventController::SetTrigger(TInt aEventId, TInt aTriggerId)
    {
	CEvent* event = NULL;
	TInt err = GetEvent(aEventId, event);
	if(err != KErrNone)
		return err;
	event->Triggers(aTriggerId);
	return KErrNone;
    }

TInt CSimEventController::AddEvent(CSimEventController::CEvent* aEvent)
    {
	if(aEvent->GetId() < 1)
		return KErrArgument;

	CEvent* temp = NULL;
	TInt err = GetEvent(aEvent->GetId(), temp);
	if(err == KErrNone)
		return KErrAlreadyExists;
	else if(err != KErrNotFound)
		return KErrGeneral;

	return iEvents.Append(aEvent);
    }

TInt CSimEventController::GetEvent(TInt aId, CSimEventController::CEvent*& aEvent)
    {
	aEvent = NULL;

	TInt count = iEvents.Count();
	for(TInt i = 0; i < count; i++)
		if(iEvents[i]->GetId() == aId)
            {
			aEvent = iEvents[i];
			return KErrNone;
            }
	return KErrNotFound;
    }

TInt CSimEventController::Trigger(TInt aId, TAny* aOptionalParam)
    {
	CEvent* event = NULL;
	TInt err = GetEvent(aId, event);
	if(err != KErrNone)
		return err;
	return event->Trigger(aOptionalParam);
    }

TInt CSimEventController::Cancel(TInt aId)
    {
	CEvent* event = NULL;
	TInt err = GetEvent(aId, event);
	if(err != KErrNone)
		return err;

	event->Cancel();
	return KErrNone;
    }


/* **************************
    * CEvent class
    * ************************** */

CSimEventController::CEvent* CSimEventController::CEvent::NewL(TAction aAction,
															   CUmtsSimulator* aSimulator)
    {
	CEvent* self = new (ELeave) CEvent(aAction, aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CSimEventController::CEvent::CEvent(TAction aAction, CUmtsSimulator* aSimulator)
	: iId(0), iAction(aAction), iParams(NULL), iOptionalParam(NULL), iActive(EFalse),
          iRequest(-1), iDelay_ms(10000), iTriggerId(0), iSimulator(aSimulator)
    {
	// nothing to do
    }

CSimEventController::CEvent::~CEvent()
    {
	Cancel();
	if(iParams)
		delete iParams;
    }

void CSimEventController::CEvent::ConstructL()
    {
	// nothing to do
    }

void CSimEventController::CEvent::SetIdL(TInt aId)
    {
	if(aId < 1)
		User::Leave(KErrArgument);

	iId = aId;
    }

void CSimEventController::CEvent::SetParams(const TDesC& aParams)
    {
	if(iParams != NULL)
        {
		delete iParams;
		iParams = NULL;
        }
	TRAPD(err, iParams = HBufC::NewL(aParams.Length()));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), err));
        }
	*iParams = aParams;
    }

void CSimEventController::CEvent::SetDelay(TInt aDelay_ms)
    {
	iDelay_ms = aDelay_ms;
    }

void CSimEventController::CEvent::Triggers(TInt aId)
    {
	iTriggerId = aId;
    }

void CSimEventController::CEvent::Cancel()
    {
	if(!iActive) return;

	iSimulator->GetSimTimer()->RemoveRequest(iRequest);
	iActive = EFalse;
    }

TInt CSimEventController::CEvent::Trigger(TAny* aOptionalParam)
    {
	if(iActive)
		if(aOptionalParam == iOptionalParam)
			return KErrNone;
		else
			return KErrAlreadyExists;

	TRAPD(err, iRequest = iSimulator->GetSimTimer()->RequestCallAfterL(
	      iDelay_ms*1000, Callback, this));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	iOptionalParam = aOptionalParam;
	iActive = ETrue;
	return KErrNone;
    }

void CSimEventController::CEvent::Callback(TAny* aSelf)
    {
	CEvent* self = (CEvent*) aSelf;
	if(!self->iActive || self->iAction == NULL)
		User::Invariant();
	self->iActive = EFalse; // done here so that event can trigger itself

	_LIT(KLogEventExe, "UMTSSim: Event executing");
	_LIT(KLogEventExeId, "Id = ");
	TBuf<50> info;
	info = KLogEventExeId;
	info.AppendNum(self->iId);
	self->iSimulator->Log(KLogEventExe, info);

	if(self->iTriggerId != 0)
        {
		TInt err = KErrNone;
		if(self->iTriggerId < 0)
			err = self->iSimulator->GetEventCtrl()->Cancel(-(self->iTriggerId));
		else
			err = self->iSimulator->GetEventCtrl()->Trigger(self->iTriggerId, NULL);

		_LIT(KLogEventOwn, "Own = ");
		_LIT(KLogEventOth, ", Other = ");
		TBuf<50> info;
		info = KLogEventOwn;
		info.AppendNum(self->iId);
		info.Append(KLogEventOth);
		TInt id = self->iTriggerId;
		info.AppendNum(id<0 ? -id : id);
		
		if(err != KErrNone)
            {
			_LIT(KLogEventError, "ERROR: Event failed triggering/cancelling another event");
			self->iSimulator->Log(KLogEventError, info);
            }
		else
            {
			if(id < 0)
                {
				_LIT(KLogEventCancel, "UMTSSim: Event cancelled another event");
				self->iSimulator->Log(KLogEventCancel, info);
                }
			else
                {
				_LIT(KLogEventTrig, "UMTSSim: Event triggered another event");
				self->iSimulator->Log(KLogEventTrig, info);
                }
            }
        }

	TInt err = (self->iSimulator->*(self->iAction))(self->iParams, self->iOptionalParam);
	if(err != KErrNone)
        {
		_LIT(KLogEventErr, "ERROR: Action originating from EventController failed");
		if(self->iParams)
            {
			_LIT(KLogPar1, "Cfg = \"");
			_LIT(KLogPar2, "\"");
			HBufC* buf = NULL;
			TRAPD(err, buf = HBufC::NewL(self->iParams->Length() + 8));
            if (err != KErrNone)
                {
                LOG(Log::Printf(_L("HBufC::NewL error: %d"), err));
                }
			//buf = HBufC::NewL(self->iParams->Length() + 8);
			TPtr bufPtr = buf->Des();
			bufPtr.Append(KLogPar1);
			bufPtr.Append(*self->iParams);
			bufPtr.Append(KLogPar2);
			self->iSimulator->Log(KLogEventErr, bufPtr);
			delete buf;
            }
		else
            {
			_LIT(KLogNoParams, "(no parameters)");
			self->iSimulator->Log(KLogEventErr, KLogNoParams);
            }
        }
    }
