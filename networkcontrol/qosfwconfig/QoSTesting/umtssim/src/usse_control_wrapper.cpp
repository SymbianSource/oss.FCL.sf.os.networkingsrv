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
// usse_control_wrapper.cpp - wrapper for server side handling of RControl calls
//
 
#include "usse_control_wrapper.h"
#include "usse_simulator.h"
#include "uscl_control.h"

/********************
    * CControlApiWrapper class
    *
    * This class handles simulation control calls. It is subsession specific.
    */

CControlApiWrapper* CControlApiWrapper::NewL(CUmtsSimServSession* aSession)
    {
    CControlApiWrapper* self = new (ELeave) CControlApiWrapper(aSession);
	self->CObject::Dec(); // allow destruction if ConstructL leaves
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	self->CObject::Inc();
    return self;
    }

CControlApiWrapper::CControlApiWrapper(CUmtsSimServSession* aSession)
	: iNotifyAllA(EFalse), iSession(aSession), iSimulator(aSession->GetSimulator())
    {
	// nothing to do here
    }

void CControlApiWrapper::ConstructL()
    {
	_LIT(KLogNew, "RControl::Open() * ApiWrapper created");
	iSimulator->Log(KLogNew);
    }

CControlApiWrapper::~CControlApiWrapper()
    {
	CloseWrapper();
    }

void CControlApiWrapper::CloseWrapper()
    {
	_LIT(KLogClose, "?::?() * ControlApiWrapper closing");
	iSimulator->Log(KLogClose);
    }

// utility: get message from session
const RMessage2& CControlApiWrapper::Message() const
    {
    // return iSession->Message();
    // Changed because of migration to Client/Server V2 API
    return iSession->iMessage;
    }

// SIMULATION CONTROL API

void CControlApiWrapper::NotifyAllA()
    {
	_LIT(KLogReq, "RControl::NotifyAll() * Booked in Wrapper (A)");
	iSimulator->Log(KLogReq);

    iNotifyAllA = ETrue;
    iNotifyAllAMessage = Message();
    }

void CControlApiWrapper::NotifyAllACancel()
    {
    if(!iNotifyAllA) return;
    iNotifyAllAMessage.Complete(KErrCancel);
    iNotifyAllA = EFalse;

	_LIT(KLogCom, "RControl::NotifyAll() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CControlApiWrapper::ReconfigureSimulatorS()
    {
	_LIT(KLogReq, "RControl::ReconfigureSimulator() * Requesting in Wrapper (S)");
	iSimulator->Log(KLogReq);

	TBuf<200> filename;
	TInt flag = Message().Int1();
	TInt err = KErrNone;
	if(Message().Ptr0() != NULL)
        {
		// TRAP(err, iSession->ReadL(Message().Ptr0(), filename));
        // Changed because of migration to Client/Server V2 API
		TRAP(err, iSession->Message().ReadL(0, filename));
		if(err != KErrNone)
            {
			iSession->PanicClient(EBadDescriptor);
			return;
            }
        }
	else
        {
		filename = KUMTSSimConfFile;
        }

	if(flag & RControl::ECFlagReqMan)
		err = iSimulator->ReconfigureReqMan(filename);
	if(err == KErrNone && (flag & RControl::ECFlagEventCtrl))
		err = iSimulator->ReconfigureEventCtrl(filename);

	Message().Complete(err);

	_LIT(KLogCom, "RControl::ReconfigureSimulator() * Completed in Wrapper (S)");
	iSimulator->Log(KLogCom);
    }

void CControlApiWrapper::ConfigureRequestHandlerS()
    {
	_LIT(KLogReq, "RControl::ConfigureRequestHandler() * Requesting in Wrapper (S)");
	iSimulator->Log(KLogReq);

	TBuf<200> cfg;
	// TRAPD(err, iSession->ReadL(Message().Ptr0(), cfg));
    // Changed because of migration to Client/Server V2 API
    TRAPD(err, iSession->Message().ReadL(0, cfg));
	if(err != KErrNone)
        {
		iSession->PanicClient(EBadDescriptor);
		return;
        }

	err = iSimulator->ConfigureRequestHandler(cfg);
	Message().Complete(err);

	_LIT(KLogCom, "RControl::ConfigureRequestHandler() * Completed in Wrapper (S)");
	iSimulator->Log(KLogCom);
    }

void CControlApiWrapper::ConfigureEventS()
    {
	_LIT(KLogReq, "RControl::ConfigureEvent() * Requesting in Wrapper (S)");
	iSimulator->Log(KLogReq);

	TBuf<200> cfg;
	// TRAPD(err, iSession->ReadL(Message().Ptr0(), cfg));
    // Changed because of migration to Client/Server V2 API
	TRAPD(err, iSession->Message().ReadL(0, cfg));
	if(err != KErrNone)
        {
		iSession->PanicClient(EBadDescriptor);
		return;
        }

	err = iSimulator->ConfigureEvent(cfg);
	Message().Complete(err);

	_LIT(KLogCom, "RControl::ConfigureEvent() * Completed in Wrapper (S)");
	iSimulator->Log(KLogCom);
    }

// called from simulator
void CControlApiWrapper::Log(const TDesC& aEntry)
    {
	if(!iNotifyAllA) return;

	// TRAPD(err, iSession->WriteL(iNotifyAllAMessage.Ptr0(), aEntry));
    // Changed because of migration to Client/Server V2 API
	TRAPD(err, iNotifyAllAMessage.WriteL(0, aEntry));
	if(err != KErrNone && err != KErrOverflow)
		iSession->PanicClient(EBadDescriptor);

	iNotifyAllAMessage.Complete(KErrNone);
	iNotifyAllA = EFalse;

	_LIT(KLogCom, "RControl::NotifyAll() * Completed in Wrapper (A)");
	iSimulator->Log(KLogCom);
    }

#ifdef USSE_DEBUG_CHECKS
void CControlApiWrapper::DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession)
    #else
    void CControlApiWrapper::DebugCheck(CUmtsSimulator*, CUmtsSimServSession*)
    #endif
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(aSession != iSession)
		User::Invariant();
    #endif
    }
