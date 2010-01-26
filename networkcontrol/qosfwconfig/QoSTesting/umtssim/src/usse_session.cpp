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
// usse_session.cpp - implementation of server side object of client-server -session with umts simulator server
//

#include "us_cliserv.h"
#include "usse_server.h"
#include "usse_simulator.h"
#include "usse_packet_wrapper.h"
#include "usse_control_wrapper.h"
#include "usse_qos_wrapper.h"

/* **************************
    * CUmtsSimServSession class
    *
    * Represents a server session
    * Contains object index and container in case that sessions will need
    *     "own" data in the future. Now these only have wrappers.
    * ************************** */

//CUmtsSimServSession* CUmtsSimServSession::NewL(RThread& aClient,
//                                               CUmtsSimServServer* aServer,
//                                               CUmtsSimulator* aSimulator)
// Changed because of migration to Client/Server V2 API
CUmtsSimServSession* CUmtsSimServSession::NewL( CUmtsSimServServer* aServer,
                                                CUmtsSimulator*     aSimulator )
    {
    CUmtsSimServSession* self=new (ELeave) CUmtsSimServSession();
    CleanupStack::PushL(self);
    self->ConstructL(aServer, aSimulator);
    CleanupStack::Pop();
    return self;
    }


// C++ constructor
CUmtsSimServSession::CUmtsSimServSession()
    : CSession2(), iContainer(0), iPacketServiceWrappers(0), iPacketContextWrappers(0),
          iPacketQoSWrappers(0), iControlWrappers(0), iResourceCount(0), iSessionRemoved(EFalse)
    {
    __DECLARE_NAME(_S("CUmtsSimServSession"));
    }

// C++ destructor
CUmtsSimServSession::~CUmtsSimServSession()
    {
    // just-in-case session was not closed properly
	TInt code = 0;
	if(!iSessionRemoved)
        {
		code |= 256;
		iSimulator->RemoveSession(*this);
		iSessionRemoved = ETrue;
        }
    if(iPacketServiceWrappers) { code |= 1; delete iPacketServiceWrappers; }
    if(iPacketContextWrappers) { code |= 2; delete iPacketContextWrappers; }
	if(iPacketQoSWrappers) { code |= 4; delete iPacketQoSWrappers; }
	if(iControlWrappers) { code |= 8; delete iControlWrappers; }
	if(iContainer) { code |= 16; iUmtsSimSvr->DeleteContainer(iContainer); }

	if(code != 0)
        {
		_LIT(KLogWarningText, "ERROR: Session::~Session() found objects open, code = ");
		TBuf<60> logText;
		logText.Append(KLogWarningText);
		logText.AppendNum(code, EHex);
		iSimulator->Log(logText);
        }
    }


// second-phase C++ constructor
void CUmtsSimServSession::ConstructL(CUmtsSimServServer* aServer, CUmtsSimulator* aSimulator)
    {
    // second-phase construct base class
    // CSession::CreateL(*aServer);
    // Changed because of migration to Client/Server V2 API
    // CSession2::CreateL();
    //
    // CSession::CreateL was public, but CSession2::CreateL is private, 
    // therefore it can't be called from CUmtsSimServSession anymore.
    // 
    iUmtsSimSvr=aServer;
	iSimulator=aSimulator;

    // create new object index
    iPacketServiceWrappers=CObjectIx::NewL();
	iPacketContextWrappers=CObjectIx::NewL();
	iPacketQoSWrappers=CObjectIx::NewL();
	iControlWrappers=CObjectIx::NewL();
    // initialize container using the index in the server
    iContainer=iUmtsSimSvr->NewContainerL();

	// initialize resource counting
	ResourceCountMarkStart();
    }

// utility
// return resources used
TInt CUmtsSimServSession::CountResources(void)
    {
	return iResourceCount;
    }

// return a packet service api wrapper corresponding given handle
CPacketServiceApiWrapper* CUmtsSimServSession::PacketServiceWrapperFromHandle(TUint aHandle)
    {
    CPacketServiceApiWrapper* wrapper = (CPacketServiceApiWrapper*) iPacketServiceWrappers->At(aHandle);
    if(wrapper == NULL)
        {
		_LIT(KLogPSWFHText, "PANIC: Session::PacketServiceWrapperFromHandle() found no wrapper");
		iSimulator->Log(KLogPSWFHText);
		PanicClient(EBadSubsessionHandle);
        }

    #ifdef USSE_DEBUG_CHECKS
	TInt epocErrors = 0;
	TInt count = iPacketServiceWrappers->ActiveCount();
	TInt found = 0;
	for(TInt i = 0; i < count; i++)
        {
		CPacketServiceApiWrapper* w = (CPacketServiceApiWrapper*) (*iPacketServiceWrappers)[i];
		if(w == NULL)
            {
			epocErrors++;
			count++;
            }
		else if(w == wrapper)
            {
			found++;
			wrapper->DebugCheck(iSimulator, this);
            }
        }
	if(found != 1)
		User::Invariant();

	if(epocErrors)
        {
        //		_LIT(KLogBug, "ERROR: Session::PacketServiceWrapperFromHandle() found CObjectIx bug");
		//iSimulator->Log(KLogBug);
        }
    #endif

    return wrapper;
    }

// return a packet context api wrapper corresponding given handle
CPacketContextApiWrapper* CUmtsSimServSession::PacketContextWrapperFromHandle(TUint aHandle)
    {
    CPacketContextApiWrapper* wrapper = (CPacketContextApiWrapper*) iPacketContextWrappers->At(aHandle);
    if(wrapper == NULL)
        {
		_LIT(KLogPCWFHText, "PANIC: Session::PacketContextWrapperFromHandle() found no wrapper");
		iSimulator->Log(KLogPCWFHText);
		PanicClient(EBadSubsessionHandle);
        }

    #ifdef USSE_DEBUG_CHECKS
	TInt epocErrors = 0;
	TInt count = iPacketContextWrappers->ActiveCount();
	TInt found = 0;
	for(TInt i = 0; i < count; i++)
        {
		CPacketContextApiWrapper* w = (CPacketContextApiWrapper*) (*iPacketContextWrappers)[i];
		if(w == NULL)
            {
			epocErrors++;
			count++;
            }
		else if(w == wrapper)
            {
			found++;
			wrapper->DebugCheck(iSimulator, this);
            }
        }
	if(found != 1)
		User::Invariant();

	if(epocErrors)
        {
        //		_LIT(KLogBug, "ERROR: Session::PacketContextWrapperFromHandle() found CObjectIx bug");
		//iSimulator->Log(KLogBug);
        }
    #endif

    return wrapper;
    }

// return a packet qos api wrapper corresponding given handle
CPacketQoSApiWrapper* CUmtsSimServSession::PacketQoSWrapperFromHandle(TUint aHandle)
    {
    CPacketQoSApiWrapper* wrapper = (CPacketQoSApiWrapper*) iPacketQoSWrappers->At(aHandle);
    if(wrapper == NULL)
        {
		_LIT(KLogPQWFHText, "PANIC: Session::PacketQoSWrapperFromHandle() found no wrapper");
		iSimulator->Log(KLogPQWFHText);
		PanicClient(EBadSubsessionHandle);
        }

    #ifdef USSE_DEBUG_CHECKS
	TInt epocErrors = 0;
	TInt count = iPacketQoSWrappers->ActiveCount();
	TInt found = 0;
	for(TInt i = 0; i < count; i++)
        {
		CPacketQoSApiWrapper* w = (CPacketQoSApiWrapper*) (*iPacketQoSWrappers)[i];
		if(w == NULL)
            {
			epocErrors++;
			count++;
            }
		else if(w == wrapper)
            {
			found++;
			wrapper->DebugCheck(iSimulator, this);
            }
        }
	if(found != 1)
		User::Invariant();

	if(epocErrors)
        {
        //		_LIT(KLogBug, "Error: Session::PacketQoSWrapperFromHandle() found CObjectIx bug");
		//iSimulator->Log(KLogBug);
        }
    #endif

    return wrapper;
    }

// return a simulation control api wrapper corresponding given handle
CControlApiWrapper* CUmtsSimServSession::ControlWrapperFromHandle(TUint aHandle)
    {
    CControlApiWrapper* wrapper = (CControlApiWrapper*) iControlWrappers->At(aHandle);
    if(wrapper == NULL)
        {
		_LIT(KLogCWFHText, "PANIC: Session::ControlWrapperFromHandle() found no wrapper");
		iSimulator->Log(KLogCWFHText);
		PanicClient(EBadSubsessionHandle);
        }

    #ifdef USSE_DEBUG_CHECKS
	TInt epocErrors = 0;
	TInt count = iControlWrappers->ActiveCount();
	TInt found = 0;
	for(TInt i = 0; i < count; i++)
        {
		CControlApiWrapper* w = (CControlApiWrapper*) (*iControlWrappers)[i];
		if(w == NULL)
            {
			epocErrors++;
			count++;
            }
		else if(w == wrapper)
            {
			found++;
			wrapper->DebugCheck(iSimulator, this);
            }
        }
	if(found != 1)
		User::Invariant();

	if(epocErrors)
        {
        //		_LIT(KLogBug, "Error: Session::ControlWrapperFromHandle() found CObjectIx bug");
		//iSimulator->Log(KLogBug);
        }
    #endif

    return wrapper;
    }

// panic the client
void CUmtsSimServSession::PanicClient(TInt aPanic) const
    {
	_LIT(KLogPanicText, "PANIC: Session::PanicClient() called with hex code ");
	TBuf<60> logText;
	logText.Append(KLogPanicText);
	logText.AppendNum(aPanic, EHex);
	iSimulator->Log(logText);

    _LIT(KTxtUmtsSimServSess,"CUmtsSimServSession");
    // Panic(KTxtUmtsSimServSess,aPanic);
    // Changed because of migration to Client/Server V2 API
    iMessage.Panic(KTxtUmtsSimServSess,aPanic);
    // CSession::Panic() panics client - we specify category
    }

// write to the client thread; if unsuccessful, panic the client
void CUmtsSimServSession::Write(const TAny* /* aPtr */, const TDesC& aDes, TInt anOffset)
    {
    // TRAPD(ret,WriteL(aPtr,aDes,anOffset););
    // Changed because of migration to Client/Server V2 API
    TRAPD(ret,Message().WriteL(0, aDes, anOffset););
    if (ret!=KErrNone)
        {
		_LIT(KLogWText, "PANIC: Session::Write() failed");
		iSimulator->Log(KLogWText);
		PanicClient(EBadDescriptor);
        }
    }

// return CPacketService simulator class
CUmtsSimulator* CUmtsSimServSession::GetSimulator() const
    {
	return iSimulator;
    }

// Handle messages for this session
// ServiceL() dispatches requests to the appropriate handler
// Session related messages are handled by CUmtsSimServSession members,
// api requests are delivered via wrapper object of (sub)session


// trap harness for dispatcher
void CUmtsSimServSession::ServiceL(const RMessage2& aMessage)
    {
    // Added because of migration to Client/Server V2 API
    iMessage = aMessage;
	DebugCheck(iSimulator); // "fast" check

    TRAPD(err,DispatchMessageL(aMessage));
    if(err != KErrNone)
        {
		aMessage.Complete(err);

		_LIT(KLogCom, "WARNING: Session::ServiceL() caught exception, message completed");
		_LIT(KLogStat, "Status = ");
		TBuf<30> code;
		code = KLogStat;
		code.AppendNum(err);
		iSimulator->Log(KLogCom, code);
        }

	iSimulator->DebugCheck(); // complete check
    //iMessage = NULL;
    }

// dispatch to right handler
void CUmtsSimServSession::DispatchMessageL(const RMessage2& aMessage)
    {
    // check for session-relative requests
    switch (aMessage.Function())
        {
            case EUmtsSimServCreatePacketServiceSubSession:
            {
			_LIT(KLogPacketServiceSSCreated, "RPacketService::Open() * Msg to Session");
			iSimulator->Log(KLogPacketServiceSSCreated);
			TRAPD(r, NewPacketServiceWrapperL());
			aMessage.Complete(r); // hopefully KErrNone

			_LIT(KLogCom, "RPacketService::Open() * Completed in Session");
			_LIT(KLogStat, "Status = ");
			TBuf<30> code;
			code = KLogStat;
			code.AppendNum(r);
			iSimulator->Log(KLogCom, code);

			return;
            }
            case EUmtsSimServCreatePacketContextSubSession:
            {
			_LIT(KLogPacketContextSSCreated, "RPacketContext::Open() * Msg to Session");
			iSimulator->Log(KLogPacketContextSSCreated);
			TRAPD(r, NewPacketContextWrapperL());
			aMessage.Complete(r); // hopefully KErrNone

			_LIT(KLogCom, "RPacketContext::Open() * Completed in Session");
			_LIT(KLogStat, "Status = ");
			TBuf<30> code;
			code = KLogStat;
			code.AppendNum(r);
			iSimulator->Log(KLogCom, code);

			return;
            }
            case EUmtsSimServCreatePacketQoSSubSession:
            {
			_LIT(KLogPacketQoSSSCreated, "RPacketQoS::Open() * Msg to Session");
			iSimulator->Log(KLogPacketQoSSSCreated);
			TRAPD(r, NewPacketQoSWrapperL());
			aMessage.Complete(r); // hopefully KErrNone

			_LIT(KLogCom, "RPacketQoS::Open() * Completed in Session");
			_LIT(KLogStat, "Status = ");
			TBuf<30> code;
			code = KLogStat;
			code.AppendNum(r);
			iSimulator->Log(KLogCom, code);

			return;
            }
            case EUmtsSimServCreateControlSubSession:
            {
			_LIT(KLogControlSSCreated, "RControl::Open() * Msg to Session");
			iSimulator->Log(KLogControlSSCreated);
			TRAPD(r, NewControlWrapperL());
			aMessage.Complete(r); // hopefully KErrNone

			_LIT(KLogCom, "RControl::Open() * Completed in Session");
			_LIT(KLogStat, "Status = ");
			TBuf<30> code;
			code = KLogStat;
			code.AppendNum(r);
			iSimulator->Log(KLogCom, code);

			return;
            }
            case EUmtsSimServCloseSession:
            {
			_LIT(KLogSessionClosed, "RUmtsSimServ::Close() * Msg to Session");
			iSimulator->Log(KLogSessionClosed);
			CloseSession();
			aMessage.Complete(KErrNone);

			_LIT(KLogCom, "RUmtsSimServ::Close() * Completed in Session");
			iSimulator->Log(KLogCom);

			return;
            }
        }

    // ok, it's either control or wrapper (service, context or qos) relative
	if(KUmtsSimServRqstPacketService == (KUmtsSimServRqstMask & aMessage.Function()))
        {
		// packet service related
	    CPacketServiceApiWrapper* wrapper = PacketServiceWrapperFromHandle(aMessage.Int3());
	    switch (aMessage.Function())
            {
            // subsession control relative stuff
                case EUmtsSimServClosePacketServiceSubSession:
                {
				_LIT(KLogPSSSClosed, "RPacketService::Close() * Msg to Session");
				iSimulator->Log(KLogPSSSClosed);
				DeletePacketServiceWrapper();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::Close() * Completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                // packet service simulation relative stuff
                case EUmtsSimServPacketServiceAttachA:
                {
				_LIT(KLogPSAttachA, "RPacketService::Attach() * Msg to Session (A)");
				iSimulator->Log(KLogPSAttachA);
				wrapper->AttachA();
				return;
                }
                case EUmtsSimServPacketServiceAttachACancel:
                {
				_LIT(KLogPSAttachC, "RPacketService::Attach() * Msg to Session (C)");
				iSimulator->Log(KLogPSAttachC);
				wrapper->AttachACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::Attach() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceDetachA:
                {
				_LIT(KLogPSDetachA, "RPacketService::Detach() * Msg to Session (A)");
				iSimulator->Log(KLogPSDetachA);
				wrapper->DetachA();
				return;
                }
                case EUmtsSimServPacketServiceDetachACancel:
                {
				_LIT(KLogPSDetachC, "RPacketService::Detach() * Msg to Session (C)");
				iSimulator->Log(KLogPSDetachC);
				wrapper->DetachACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::Detach() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceGetStatusS:
                {
				_LIT(KLogPSGetStatus, "RPacketService::GetStatus() * Msg to Session (S)");
				iSimulator->Log(KLogPSGetStatus);
				wrapper->GetStatusS();
				// aMessage.Complete(KErrNone); -- called asynchronoysly (after delay) in wrapper
				return;
                }
                case EUmtsSimServPacketServiceNotifyStatusChangeA:
                {
				_LIT(KLogPSNotifyStatus, "RPacketService::NotifyStatusChange() * Msg to Session (A)");
				iSimulator->Log(KLogPSNotifyStatus);
				wrapper->NotifyStatusChangeA();
				return;
                }
                case EUmtsSimServPacketServiceNotifyStatusChangeACancel:
                {
				_LIT(KLogPSNotifyStatusCancel, "RPacketService::NotifyStatusChange() * Msg to Session (C)");
				iSimulator->Log(KLogPSNotifyStatusCancel);
				wrapper->NotifyStatusChangeACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::NotifyStatusChange() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceNotifyContextAddedA:
                {
				_LIT(KLogPSNotifyContextAddedA, "RPacketService::NotifyContextAdded() * Msg to Session (A)");
				iSimulator->Log(KLogPSNotifyContextAddedA);
				wrapper->NotifyContextAddedA();
				return;
                }
                case EUmtsSimServPacketServiceNotifyContextAddedACancel:
                {
 				_LIT(KLogPSNotifyContextAddedC, "RPacketService::NotifyContextAdded() * Msg to Session (C)");
				iSimulator->Log(KLogPSNotifyContextAddedC);
				wrapper->NotifyContextAddedACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::NotifyContextAdded() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceEnumerateContextsA:
                {
				_LIT(KLogPSEnumerateContextsA, "RPacketService::EnumerateContexts() * Msg to Session (A)");
				iSimulator->Log(KLogPSEnumerateContextsA);
				wrapper->EnumerateContextsA();
				return;
                }
                case EUmtsSimServPacketServiceEnumerateContextsACancel:
                {
				_LIT(KLogPSEnumerateContextsAC, "RPacketService::EnumerateContexts() * Msg to Session (C)");
				iSimulator->Log(KLogPSEnumerateContextsAC);
				wrapper->EnumerateContextsACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::EnumerateContexts() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceGetContextInfoA:
                {
				_LIT(KLogPSGetContextInfoA, "RPacketService::GetContextInfo() * Msg to Session (A)");
				iSimulator->Log(KLogPSGetContextInfoA);
				wrapper->GetContextInfoA();
				return;
                }
                case EUmtsSimServPacketServiceGetContextInfoACancel:
                {
				_LIT(KLogPSGetContextInfoCancel, "RPacketService::GetContextInfo() * Msg to Session (C)");
				iSimulator->Log(KLogPSGetContextInfoCancel);
				wrapper->GetContextInfoACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::GetContextInfo() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceEnumerateNifsA:
                {
				_LIT(KLogPSEnumerateNifsA, "RPacketService::EnumerateNifs() * Msg to Session (A)");
				iSimulator->Log(KLogPSEnumerateNifsA);
				wrapper->EnumerateNifsA();
				return;
                }
                case EUmtsSimServPacketServiceEnumerateNifsACancel:
                {
				_LIT(KLogPSEnumerateNifsAC, "RPacketService::EnumerateNifs() * Msg to Session (C)");
				iSimulator->Log(KLogPSEnumerateNifsAC);
				wrapper->EnumerateNifsACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::EnumerateNifs() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceGetNifInfoA:
                {
				_LIT(KLogPSGetNifInfoA, "RPacketService::GetNifInfo() * Msg to Session (A)");
				iSimulator->Log(KLogPSGetNifInfoA);
				wrapper->GetNifInfoA();
				return;
                }
                case EUmtsSimServPacketServiceGetNifInfoACancel:
                {
				_LIT(KLogPSGetNifInfoAC, "RPacketService::GetNifInfo() * Msg to Session (C)");
				iSimulator->Log(KLogPSGetNifInfoAC);
				wrapper->GetNifInfoACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::GetNifInfo() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceEnumerateContextsInNifA:
                {
				_LIT(KLogPSEnumContInNifA, "RPacketService::EnumerateContextsInNif() * Msg to Session (A)");
				iSimulator->Log(KLogPSEnumContInNifA);
				wrapper->EnumerateContextsInNifA();
				return;
                }
                case EUmtsSimServPacketServiceEnumerateContextsInNifACancel:
                {
				_LIT(KLogPSEnumContInNifAC, "RPacketService::EnumerateContextsInNif() * Msg to Session (C)");
				iSimulator->Log(KLogPSEnumContInNifAC);
				wrapper->EnumerateContextsInNifACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::EnumerateContextsInNif() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketServiceGetContextNameInNifA:
                {
				_LIT(KLogPSGetContextNameInNifA, "RPacketService::GetContextNameInNif() * Msg to Session (A)");
				iSimulator->Log(KLogPSGetContextNameInNifA);
				wrapper->GetContextNameInNifA();
				return;
                }
                case EUmtsSimServPacketServiceGetContextNameInNifACancel:
                {
				_LIT(KLogPSGetCNameInNifAC, "RPacketService::GetContextNameInNif() * Msg to Session (C)");
				iSimulator->Log(KLogPSGetCNameInNifAC);
				wrapper->GetContextNameInNifACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketService::GetContextNameInNif() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                default:
                {
				_LIT(KLogInvText, "PANIC: Session::DispatchMessageL() encountered an invalid PS request");
				iSimulator->Log(KLogInvText);
				PanicClient(EBadRequest);
				return;
                }
            }
        }
	else if(KUmtsSimServRqstPacketContext == (KUmtsSimServRqstMask & aMessage.Function()))
        {
		// packet context related
	    CPacketContextApiWrapper* wrapper = PacketContextWrapperFromHandle(aMessage.Int3());
	    switch (aMessage.Function())
            {
            // subsession control relative stuff
                case EUmtsSimServClosePacketContextSubSession:
                {
				_LIT(KLogPCSSClosed, "RPacketContext::Close() * Msg to Session");
				iSimulator->Log(KLogPCSSClosed);
				DeletePacketContextWrapper();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::Close() * Completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                // packet context simulation relative stuff
                case EUmtsSimServPacketContextInitialiseContextA:
                {
				_LIT(KLogPCInitA, "RPacketContext::InitialiseContext() * Msg to Session (A)");
				iSimulator->Log(KLogPCInitA);
				wrapper->InitialiseContextA();
				return;
                }
                case EUmtsSimServPacketContextInitialiseContextACancel:
                {
				_LIT(KLogPCInitAC, "Session: PacketContext:InitialiseContext() * Msg to Session (C)");
				iSimulator->Log(KLogPCInitAC);
				wrapper->InitialiseContextACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::InitialiseContext() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextDeleteA:
                {
				_LIT(KLogPCDeleteA, "RPacketContext::Delete() * Msg to Session (A)");
				iSimulator->Log(KLogPCDeleteA);
				wrapper->DeleteA();
				return;
                }
                case EUmtsSimServPacketContextDeleteACancel:
                {
				_LIT(KLogPCDeleteC, "RPacketContext::Delete() * Msg to Session (C)");
				iSimulator->Log(KLogPCDeleteC);
				wrapper->DeleteACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::Delete() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                case EUmtsSimServPacketContextSetConfigA:
                {
				_LIT(KLogPCSetConfigA, "RPacketContext::SetConfig() * Msg to Session (A)");
				iSimulator->Log(KLogPCSetConfigA);
				wrapper->SetConfigA();
				return;
                }
                case EUmtsSimServPacketContextSetConfigACancel:
                {
				_LIT(KLogPCSetConfigC, "RPacketContext::SetConfig() * Msg to Session (C)");
				iSimulator->Log(KLogPCSetConfigC);
				wrapper->SetConfigACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::SetConfig() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextGetConfigA:
                {
				_LIT(KLogPCGetConfigA, "RPacketContext::GetConfig() * Msg to Session (A)");
				iSimulator->Log(KLogPCGetConfigA);
				wrapper->GetConfigA();
				return;
                }
                case EUmtsSimServPacketContextGetConfigACancel:
                {
				_LIT(KLogPCGetConfigC, "RPacketContext::GetConfig() * Msg to Session (C)");
				iSimulator->Log(KLogPCGetConfigC);
				wrapper->GetConfigACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::GetConfig() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextNotifyConfigChangedA:
                {
				_LIT(KLogPCNotifyConfigChangedA, "RPacketContext::NotifyConfigChanged() * Msg to Session (A)");
				iSimulator->Log(KLogPCNotifyConfigChangedA);
				wrapper->NotifyConfigChangedA();
				return;
                }
                case EUmtsSimServPacketContextNotifyConfigChangedACancel:
                {
				_LIT(KLogPCNotifyConfigChangedC, "RPacketContext::NotifyConfigChanged() * Msg to Session (C)");
				iSimulator->Log(KLogPCNotifyConfigChangedC);
				wrapper->NotifyConfigChangedACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::NotifyConfigChanged() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextActivateA:
                {
				_LIT(KLogPCActivateA, "RPacketContext::Activate() * Msg to Session (A)");
				iSimulator->Log(KLogPCActivateA);
				wrapper->ActivateA();
				return;
                }
                case EUmtsSimServPacketContextActivateACancel:
                {
				_LIT(KLogPCActivateC, "RPacketContext::Activate() * Msg to Session (C)");
				iSimulator->Log(KLogPCActivateC);
				wrapper->ActivateACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::Activate() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextDeactivateA:
                {
				_LIT(KLogPCDeactivateA, "RPacketContext::Deactivate() * Msg to Session (A)");
				iSimulator->Log(KLogPCDeactivateA);
				wrapper->DeactivateA();
				return;
                }
                case EUmtsSimServPacketContextDeactivateACancel:
                {
				_LIT(KLogPCDeactivateC, "RPacketContext::Deactivate() * Msg to Session (C)");
				iSimulator->Log(KLogPCDeactivateC);
				wrapper->DeactivateACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::Deactivate() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextGetStatusS:
                {
				_LIT(KLogPCGetStatus, "RPacketContext::GetStatus() * Msg to Session (S)");
				iSimulator->Log(KLogPCGetStatus);
				wrapper->GetStatusS();
				// aMessage.Complete(KErrNone); -- called asynchronoysly (after delay) in wrapper
				return;
                }
                case EUmtsSimServPacketContextNotifyStatusChangeA:
                {
				_LIT(KLogPCNotifyStatus, "RPacketContext::NotifyStatusChange() * Msg to Session (A)");
				iSimulator->Log(KLogPCNotifyStatus);
				wrapper->NotifyStatusChangeA();
				return;
                }
                case EUmtsSimServPacketContextNotifyStatusChangeACancel:
                {
				_LIT(KLogPCNotifyStatusCancel, "RPacketContext::NotifyStatusChange() * Msg to Session (C)");
				iSimulator->Log(KLogPCNotifyStatusCancel);
				wrapper->NotifyStatusChangeACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::NotifyStatusChange() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextEnumeratePacketFiltersA:
                {
				_LIT(KLogPCEnumPFA, "RPacketContext::EnumeratePacketFilters() * Msg to Session (A)");
				iSimulator->Log(KLogPCEnumPFA);
				wrapper->EnumeratePacketFiltersA();
				return;
                }
                case EUmtsSimServPacketContextEnumeratePacketFiltersACancel:
                {
				_LIT(KLogPCEnumPFAC, "RPacketContext::EnumeratePacketFilters() * Msg to Session (C)");
				iSimulator->Log(KLogPCEnumPFAC);
				wrapper->EnumeratePacketFiltersACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::EnumeratePacketFilters() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextGetPacketFilterInfoA:
                {
				_LIT(KLogPCGetPFIA, "RPacketContext::GetPacketFilterInfo() * Msg to Session (A)");
				iSimulator->Log(KLogPCGetPFIA);
				wrapper->GetPacketFilterInfoA();
				return;
                }
                case EUmtsSimServPacketContextGetPacketFilterInfoACancel:
                {
				_LIT(KLogPCGetPFIAC, "RPacketContext::GetPacketFilterInfo() * Msg to Session (C)");
				iSimulator->Log(KLogPCGetPFIAC);
				wrapper->GetPacketFilterInfoACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::GetPacketFilterInfo() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextAddPacketFilterA:
                {
				_LIT(KLogPCAddPFA, "RPacketContext::AddPacketFilter() * Msg to Session (A)");
				iSimulator->Log(KLogPCAddPFA);
				wrapper->AddPacketFilterA();
				return;
                }
                case EUmtsSimServPacketContextAddPacketFilterACancel:
                {
				_LIT(KLogPCAddPFAC, "RPacketContext::AddPacketFilter() * Msg to Session (C)");
				iSimulator->Log(KLogPCAddPFAC);
				wrapper->AddPacketFilterACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::AddPacketFilter() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextRemovePacketFilterA:
                {
				_LIT(KLogPCRemovePFA, "RPacketContext::RemovePacketFilter() * Msg to Session (A)");
				iSimulator->Log(KLogPCRemovePFA);
				wrapper->RemovePacketFilterA();
				return;
                }
                case EUmtsSimServPacketContextRemovePacketFilterACancel:
                {
				_LIT(KLogPCRemovePFAC, "RPacketContext::RemovePacketFilter() * Msg to Session (C)");
				iSimulator->Log(KLogPCRemovePFAC);
				wrapper->RemovePacketFilterACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::RemovePacketFilter() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketContextModifyActiveContextA:
                {
				_LIT(KLogPCModifyActiveContextA, "RPacketContext::ModifyActiveContext() * Msg to Session (A)");
				iSimulator->Log(KLogPCModifyActiveContextA);
				wrapper->ModifyActiveContextA();
				return;
                }
                case EUmtsSimServPacketContextModifyActiveContextACancel:
                {
				_LIT(KLogPCModActiveContextAC, "RPacketContext::ModifyActiveContext() * Msg to Session (C)");
				iSimulator->Log(KLogPCModActiveContextAC);
				wrapper->ModifyActiveContextACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketContext::ModifyActiveContext() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                default:
                {
				_LIT(KLogInvText, "PANIC: Session::DispatchMessageL() encountered an invalid PC request");
				iSimulator->Log(KLogInvText);
				PanicClient(EBadRequest);
				return;
                }
            }
        }
	else if(KUmtsSimServRqstPacketQoS == (KUmtsSimServRqstMask & aMessage.Function()))
        {
		// packet qos related
	    CPacketQoSApiWrapper* wrapper = PacketQoSWrapperFromHandle(aMessage.Int3());
	    switch (aMessage.Function())
            {
            // subsession control relative stuff
                case EUmtsSimServClosePacketQoSSubSession:
                {
				_LIT(KLogPQSSClosed, "RPacketQoS::Close() * Msg to Session");
				iSimulator->Log(KLogPQSSClosed);
				DeletePacketQoSWrapper();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketQoS::Close() * Completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                // packet qos simulation relative stuff
                case EUmtsSimServPacketQoSSetProfileParametersA:
                {
				_LIT(KLogPQSetPPA, "RPacketQoS::SetProfileParameters() * Msg to Session (A)");
				iSimulator->Log(KLogPQSetPPA);
				wrapper->SetProfileParametersA();
				return;
                }
                case EUmtsSimServPacketQoSSetProfileParametersACancel:
                {
				_LIT(KLogPQSetPPCancel, "RPacketQoS::SetProfileParameters() * Msg to Session (C)");
				iSimulator->Log(KLogPQSetPPCancel);
				wrapper->SetProfileParametersACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketQoS::SetProfileParameters() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketQoSGetProfileParametersA:
                {
				_LIT(KLogPQGetPPA, "RPacketQoS::GetProfileParameters() * Msg to Session (A)");
				iSimulator->Log(KLogPQGetPPA);
				wrapper->GetProfileParametersA();
				return;
                }
                case EUmtsSimServPacketQoSGetProfileParametersACancel:
                {
				_LIT(KLogPQGetPPCancel, "RPacketQoS::GetProfileParameters() * Msg to Session (C)");
				iSimulator->Log(KLogPQGetPPCancel);
				wrapper->GetProfileParametersACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketQoS::GetProfileParameters() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServPacketQoSNotifyProfileChangedA:
                {
				_LIT(KLogPQNotifyPCA, "RPacketQoS::NotifyProfileChanged() * Msg to Session (A)");
				iSimulator->Log(KLogPQNotifyPCA);
				wrapper->NotifyProfileChangedA();
				return;
                }
                case EUmtsSimServPacketQoSNotifyProfileChangedACancel:
                {
				_LIT(KLogPQNotifyPCAC, "RPacketQoS::NotifyProfileChanged() * Msg to Session (C)");
				iSimulator->Log(KLogPQNotifyPCAC);
				wrapper->NotifyProfileChangedACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RPacketQoS::NotifyProfileChanged() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                default:
                {
				_LIT(KLogInvText, "PANIC: Session::DispatchMessageL() encountered an invalid PQ request");
				iSimulator->Log(KLogInvText);
				PanicClient(EBadRequest);
				return;
                }
            }
        }
	else if(KUmtsSimServRqstControl == (KUmtsSimServRqstMask & aMessage.Function()))
        {
		// simulation control related
		CControlApiWrapper* wrapper = ControlWrapperFromHandle(aMessage.Int3());
		switch (aMessage.Function())
            {
            // subsession control relative stuff
                case EUmtsSimServCloseControlSubSession:
                {
				_LIT(KLogCSSClosed, "RControl::Close() * Msg to Session");
				iSimulator->Log(KLogCSSClosed);
				DeleteControlWrapper();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RControl::Close() * Completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }

                // simulation control relative stuff
                case EUmtsSimServControlNotifyAllA:
                {
				_LIT(KLogCNA, "RControl::NotifyAll() * Msg to Session (A)");
				iSimulator->Log(KLogCNA);
				wrapper->NotifyAllA();
				return;
                }
                case EUmtsSimServControlNotifyAllACancel:
                {
				_LIT(KLogCNAC, "RControl::NotifyAll() * Msg to Session (C)");
				iSimulator->Log(KLogCNAC);
				wrapper->NotifyAllACancel();
				aMessage.Complete(KErrNone);

				_LIT(KLogCom, "RControl::NotifyAll() * Cancel completed in Session");
				iSimulator->Log(KLogCom);

				return;
                }
                case EUmtsSimServControlReconfigureSimulatorS:
                {
				_LIT(KLogRSS, "RControl::ReconfigureSimulator() * Msg to Session (S)");
				iSimulator->Log(KLogRSS);
				wrapper->ReconfigureSimulatorS();
				// aMessage.Complete(KErrNone); -- completed in wrapper
				return;
                }
                case EUmtsSimServControlConfigureRequestHandlerS:
                {
				_LIT(KLogCRHS, "RControl::ConfigureRequestHandler() * Msg to Session (S)");
				iSimulator->Log(KLogCRHS);
				wrapper->ConfigureRequestHandlerS();
				// aMessage.Complete(KErrNone); -- completed in wrapper
				return;
                }
                case EUmtsSimServControlConfigureEventS:
                {
				_LIT(KLogCES, "RControl::ConfigureEvent() * Msg to Session (S)");
				iSimulator->Log(KLogCES);
				wrapper->ConfigureEventS();
				// aMessage.Complete(KErrNone); -- completed in wrapper
				return;
                }
                default:
                {
				_LIT(KLogInvText, "PANIC: Session::DispatchMessageL() encountered an invalid Control request");
				iSimulator->Log(KLogInvText);
				PanicClient(EBadRequest);
				return;
                }
            }
        }
	else
        {
		_LIT(KLogInvText, "PANIC: Session::DispatchMessageL() encountered an ivalid request class id");
		iSimulator->Log(KLogInvText);
		PanicClient(EBadRequest);
        }

	return;
    }



// handlers for session related requests

// close entire session
void CUmtsSimServSession::CloseSession() 
    {
	//remove session from own booking
	iSimulator->RemoveSession(*this);
	iSessionRemoved = ETrue;

	if(CountResources() != 0)
        {
		TInt code = 0;
		if(iPacketServiceWrappers->ActiveCount() != 0) code |= 1;
		if(iPacketContextWrappers->ActiveCount() != 0) code |= 2;
		if(iPacketQoSWrappers->ActiveCount() != 0) code |= 4;
		if(iControlWrappers->ActiveCount() != 0) code |= 8;

		_LIT(KLogErrorText, "ERROR: Session::CloseSession() found wrappers, code = ");
		TBuf<60> logText;
		logText.Append(KLogErrorText);
		logText.AppendNum(code, EHex);
		iSimulator->Log(logText);
        }

	//end resource counting
	ResourceCountMarkEnd(iMessage);

    delete iPacketServiceWrappers;
    delete iPacketContextWrappers;
	delete iPacketQoSWrappers;
	delete iControlWrappers;
    iUmtsSimSvr->DeleteContainer(iContainer);
    // for destructor
    iPacketServiceWrappers = 0;
    iPacketContextWrappers = 0;
	iPacketQoSWrappers = 0;
	iControlWrappers = 0;
    iContainer = 0;
    }

// create a new wrapper (subsession) for packet service, pass back its handle via the message
void CUmtsSimServSession::NewPacketServiceWrapperL()
    {
    CPacketServiceApiWrapper* wrapper = CPacketServiceApiWrapper::NewL(this);
    iContainer->AddL(wrapper); // generates unique id?
    TInt handle = iPacketServiceWrappers->AddL(wrapper);

    // write the handle to client
    TPckg<TInt> handlePckg(handle);
    // TRAPD(res,WriteL(Message().Ptr3(),handlePckg));
    // Changed because of migration to Client/Server V2 API
    TRAPD(res,Message().WriteL(3,handlePckg));
    if(res!=KErrNone)
        {
		iPacketServiceWrappers->Remove(handle);
		PanicClient(EBadDescriptor);
		return;
        }

	iResourceCount++;
    }

// create a new wrapper (subsession) for packet context, pass back its handle via the message
void CUmtsSimServSession::NewPacketContextWrapperL()
    {
    CPacketContextApiWrapper* wrapper = CPacketContextApiWrapper::NewL(this);
    iContainer->AddL(wrapper); // generates unique id?
    TInt handle = iPacketContextWrappers->AddL(wrapper);

    // write the handle to client
    TPckg<TInt> handlePckg(handle);
    // TRAPD(res,WriteL(Message().Ptr3(),handlePckg));
    // Changed because of migration to Client/Server V2 API
    TRAPD(res,Message().WriteL(3,handlePckg));
    if(res!=KErrNone)
        {
		iPacketContextWrappers->Remove(handle);
		PanicClient(EBadDescriptor);
		return;
        }

	iResourceCount++;
    }

// create a new wrapper (subsession) for packet qos, pass back its handle via the message
void CUmtsSimServSession::NewPacketQoSWrapperL()
    {
    CPacketQoSApiWrapper* wrapper = CPacketQoSApiWrapper::NewL(this);
    iContainer->AddL(wrapper); // generates unique id?
    TInt handle = iPacketQoSWrappers->AddL(wrapper);

    // write the handle to client
    TPckg<TInt> handlePckg(handle);
    // TRAPD(res,WriteL(Message().Ptr3(),handlePckg));
    // Changed because of migration to Client/Server V2 API
    TRAPD(res,Message().WriteL(3,handlePckg));
    if(res!=KErrNone)
        {
		iPacketQoSWrappers->Remove(handle);
		PanicClient(EBadDescriptor);
		return;
        }

	iResourceCount++;
    }

// create a new wrapper (subsession) for simulation control, pass back its handle via the message
void CUmtsSimServSession::NewControlWrapperL()
    {
    CControlApiWrapper* wrapper = CControlApiWrapper::NewL(this);
    iContainer->AddL(wrapper); // generates unique id?
    TInt handle = iControlWrappers->AddL(wrapper);

    // write the handle to client
    TPckg<TInt> handlePckg(handle);
    // TRAPD(res,WriteL(Message().Ptr3(),handlePckg));
    // Changed because of migration to Client/Server V2 API
    TRAPD(res,Message().WriteL(3,handlePckg));
    if(res!=KErrNone)
        {
		iControlWrappers->Remove(handle);
		PanicClient(EBadDescriptor);
		return;
        }

	iResourceCount++;
    }


// handler for subsession (wrapper) relative requests

// delete a wrapper by handle
void CUmtsSimServSession::DeletePacketServiceWrapper()
    {
    // TInt handle = Message().Int3();
    // Changed because of migration to Client/Server V2 API
    TInt handle = Message().Int3();
    CPacketServiceApiWrapper* wrapper = PacketServiceWrapperFromHandle(handle);

    // allow cleanup
    wrapper->CloseWrapper();

    iPacketServiceWrappers->Remove(handle);
	iResourceCount--;
    }

void CUmtsSimServSession::DeletePacketContextWrapper()
    {
    // TInt handle = Message().Int3();
    // Changed because of migration to Client/Server V2 API
    TInt handle = Message().Int3();
    CPacketContextApiWrapper* wrapper = PacketContextWrapperFromHandle(handle);

    // allow cleanup
    wrapper->CloseWrapper();

    iPacketContextWrappers->Remove(handle);
	iResourceCount--;
    }

void CUmtsSimServSession::DeletePacketQoSWrapper()
    {
    // TInt handle = Message().Int3();
    // Changed because of migration to Client/Server V2 API
    TInt handle = Message().Int3();
    CPacketQoSApiWrapper* wrapper = PacketQoSWrapperFromHandle(handle);

    // allow cleanup
    wrapper->CloseWrapper();

    iPacketQoSWrappers->Remove(handle);
	iResourceCount--;
    }

void CUmtsSimServSession::DeleteControlWrapper()
    {
    // TInt handle = Message().Int3();
    // Changed because of migration to Client/Server V2 API
    TInt handle = Message().Int3();
    CControlApiWrapper* wrapper = ControlWrapperFromHandle(handle);

    // allow cleanup
    wrapper->CloseWrapper();

    iControlWrappers->Remove(handle);
	iResourceCount--;
    }

void CUmtsSimServSession::DebugCheck(CUmtsSimulator*
                                     #ifdef USSE_DEBUG_CHECKS
									 aSimulator
                                     #endif
									 )
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();

	TInt psc = iPacketServiceWrappers->ActiveCount();
	TInt pcc = iPacketContextWrappers->ActiveCount();
	TInt pqc = iPacketQoSWrappers->ActiveCount();
	TInt cc = iControlWrappers->ActiveCount();

	if(iResourceCount != psc+pcc+pqc+cc)
		User::Invariant();

	//_LIT(KLogBug, "ERROR: Session::DebugCheck() found a CObjectIx bug");

	TInt i = 0;
	for(i = 0; i < psc; i++)
        {
		CPacketServiceApiWrapper* psw = (CPacketServiceApiWrapper*) (*iPacketServiceWrappers)[i];
		if(psw == 0)
            {
			//iSimulator->Log(KLogBug);
			psc++;
            }
		else psw->DebugCheck(iSimulator, this);
        }

	for(i = 0; i < pcc; i++)
        {
		CPacketContextApiWrapper* pcw = (CPacketContextApiWrapper*) (*iPacketContextWrappers)[i];
		if(pcw == 0)
            {
			//iSimulator->Log(KLogBug);
			pcc++;
            }
		else pcw->DebugCheck(iSimulator, this);
        }

	for(i = 0; i < pqc; i++)
        {
		CPacketQoSApiWrapper* pqw = (CPacketQoSApiWrapper*) (*iPacketQoSWrappers)[i];
		if(pqw == 0)
            {
			//iSimulator->Log(KLogBug);
			pqc++;
            }
		else pqw->DebugCheck(iSimulator, this);
        }

	for(i = 0; i < cc; i++)
        {
		CControlApiWrapper* cw = (CControlApiWrapper*) (*iControlWrappers)[i];
		if(cw == 0)
            {
			//iSimulator->Log(KLogBug);
			cc++;
            }
		else cw->DebugCheck(iSimulator, this);
        }
    #endif
    }
