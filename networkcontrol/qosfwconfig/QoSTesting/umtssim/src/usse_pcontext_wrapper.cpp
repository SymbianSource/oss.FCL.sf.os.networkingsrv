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
// usse_pcontext_wrapper.cpp - wrapper for server side handling of RPacketContext calls//
//


#include "us_cliserv.h"
#include "usse_packet_wrapper.h"
#include "usse_simulator.h"
#include "log.h"

/********************
    * CPacketContextApiWrapper class
    *
    * Works as a wrapper for passing requests originated from RPacketContext
    * to the state machine owned by server. The class is subsession
    * specific and and client information (like notification requests)
    * is stored here.
    */

CPacketContextApiWrapper* CPacketContextApiWrapper::NewL(CUmtsSimServSession* aSession)
    {
    // const RMessage& message = aSession->Message();
    // Changed because of migration to Client/Server V2 API
	const RMessage2& message = aSession->iMessage;
	const TAny* name = message.Ptr0();
	TUmtsSimServContextOpenMode mode = (TUmtsSimServContextOpenMode) message.Int1();
	const TAny* ptr2 = message.Ptr2();

    CPacketContextApiWrapper* self = new (ELeave) CPacketContextApiWrapper(aSession);
	self->CObject::Dec(); // allow destruction if ConstructL leaves
    CleanupStack::PushL(self);
    self->ConstructL(mode, name, ptr2);
    CleanupStack::Pop();
	self->CObject::Inc();
    return self;
    }

CPacketContextApiWrapper::CPacketContextApiWrapper(CUmtsSimServSession* aSession)
	: iInitialiseContextA(EFalse), iDeleteA(EFalse), iSetConfigA(EFalse), iGetConfigA(EFalse),
          iNotifyConfigChangedA(EFalse), iActivateA(EFalse), iDeactivateA(EFalse), iGetStatusS(EFalse),
          iNotifyStatusA(EFalse), iEnumeratePacketFiltersA(EFalse), iGetPacketFilterInfoA(EFalse),
          iAddPacketFilterA(EFalse), iRemovePacketFilterA(EFalse), iModifyActiveContextA(EFalse),
          iSession(aSession), iSimulator(aSession->GetSimulator()), iContext(NULL)
    {
	// nothing to do here
    }

CPacketContextApiWrapper::~CPacketContextApiWrapper()
    {
	CloseWrapper();
    }

void CPacketContextApiWrapper::ConstructL(TUmtsSimServContextOpenMode aMode,
										  const TAny* /* aName */, const TAny* /* aPtr */)
    {
	// first check whether to accept or not
	TInt status, delay; // delay not really used
	iSimulator->CheckRequest(EUmtsSimServCreatePacketContextSubSession, status, delay);
	User::LeaveIfError(status);

	switch(aMode)
        {
            case EUmtsSimServOpenNewPacketContext:
            {
			TBuf<65> name;
			User::LeaveIfError(iSimulator->NewPrimaryPacketContext(name, iContext));

			// TRAPD(err, iSession->WriteL(aName, name));
            // Changed because of migration to Client/Server V2 API
            TRAPD(err, iSession->Message().WriteL(0, name));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }
            }
            break;
            case EUmtsSimServOpenNewSecondaryPacketContext:
            {
			TBuf<65> oldContext;
			// TRAPD(err, iSession->ReadL(aPtr, oldContext));
            // Changed because of migration to Client/Server V2 API
            TRAPD(err, iSession->Message().ReadL(2, oldContext));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }

			TBuf<65> name;
			User::LeaveIfError(iSimulator->NewSecondaryPacketContext(oldContext, name, iContext));

			// TRAP(err, iSession->WriteL(aName, name));
            // Changed because of migration to Client/Server V2 API
            TRAP(err, iSession->Message().WriteL(0, name));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }
            }
            break;
            case EUmtsSimServOpenExistingPacketContext:
            {
			TBuf<65> name;
			// TRAPD(err, iSession->ReadL(aName, name));
            // Changed because of migration to Client/Server V2 API
            TRAPD(err, iSession->Message().ReadL(0, name));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }

			User::LeaveIfError(iSimulator->GetPacketContext(name, iContext, ETrue));
            }
            break;
            default:
                User::Leave(KErrArgument);
                break;
        }

	_LIT(KLogNew, "RPacketContext::Open() * ApiWrapper created");
	LogWithName(KLogNew);
    }

void CPacketContextApiWrapper::CloseWrapper()
    {
	_LIT(KLogClose, "?::?() * PacketContextApiWrapper closing");
	LogWithName(KLogClose);

	// Should the associated RMessages be .Completed()?
	// .. for now they are not but I don't know which approach causes more problems on the client side..

	// first cancel notifications
	if(iNotifyConfigChangedA) iNotifyConfigChangedA = EFalse;
	if(iNotifyStatusA) iNotifyStatusA = EFalse;

	if(iInitialiseContextA) { iInitialiseContextA = EFalse; iContext->RequestContextInitialisationCancel(iInitialiseContextAReq); }
	if(iDeleteA) { iDeleteA = EFalse; iContext->RequestDeletionCancel(iDeleteAReq); }
	if(iSetConfigA) { iSetConfigA = EFalse; iContext->RequestConfigurationCancel(iSetConfigAReq); }
	if(iGetConfigA) { iGetConfigA = EFalse; iContext->RequestGettingConfigCancel(iGetConfigAReq); }
	if(iActivateA) { iActivateA = EFalse; iContext->RequestActivationCancel(iActivateAReq); }
	if(iDeactivateA) { iDeactivateA = EFalse; iContext->RequestDeactivationCancel(iDeactivateAReq); }
	if(iGetStatusS) { iGetStatusS = EFalse; iContext->RequestGettingStatusCancel(iGetStatusSReq); }
	if(iEnumeratePacketFiltersA) { iEnumeratePacketFiltersA = EFalse; iContext->RequestEnumeratingPacketFiltersCancel(iEnumeratePacketFiltersAReq); }
	if(iGetPacketFilterInfoA) { iGetPacketFilterInfoA = EFalse; iContext->RequestGettingPacketFilterInfoCancel(iGetPacketFilterInfoAReq); }
	if(iAddPacketFilterA) { iAddPacketFilterA = EFalse; iContext->RequestAddingPacketFilterCancel(iAddPacketFilterAReq); }
	if(iRemovePacketFilterA) { iRemovePacketFilterA = EFalse; iContext->RequestRemovingPacketFilterCancel(iRemovePacketFilterAReq); }
	if(iModifyActiveContextA) { iModifyActiveContextA = EFalse; iContext->RequestModifyingActiveContextCancel(iModifyActiveContextAReq); }

	if(iContext)
        {
		//User::LeaveIfError(iSimulator->RemovePacketContext(iContext->GetName()));
		TRAPD(err, iSimulator->RemovePacketContext(iContext->GetName()));
		if(err)
            {
			_LIT(KLogNew1, "CPacketContextApiWrapper::CloseWrapper() * err");
			LogWithName(KLogNew1);
            }

		iContext = NULL;
        }

	iSimulator->DebugCheck();
    }

// utility: get message from session
const RMessage2& CPacketContextApiWrapper::Message() const
    {
    // return iSession->Message();
    // Changed because of migration to Client/Server V2 API
    return iSession->iMessage;
    }


// ETEL PACKET DATA API / PACKET CONTEXT

// Initialize context
void CPacketContextApiWrapper::InitialiseContextA()
    {
	_LIT(KLogReq, "RPacketContext::InitialiseContext() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iInitialiseContextA = ETrue;
	iInitialiseContextAMessage = Message();
	iInitialiseContextAReq =
		iContext->RequestContextInitialisation(CPacketContextApiWrapper::SimInitialiseContextAReady, this);
    }

void CPacketContextApiWrapper::InitialiseContextACancel()
    {
	if(!iInitialiseContextA) return;

	iContext->RequestContextInitialisationCancel(iInitialiseContextAReq);
	iInitialiseContextAMessage.Complete(KErrCancel);
	iInitialiseContextA = EFalse;

	_LIT(KLogCom, "RPacketContext::InitialiseContext() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// Delete context
void CPacketContextApiWrapper::DeleteA()
    {
	_LIT(KLogReq, "RPacketContext::Delete() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

    iDeleteA = ETrue;
    iDeleteAMessage = Message();
	iDeleteAReq = iContext->RequestDeletion(CPacketContextApiWrapper::SimDeleteAReady, this);
    }

void CPacketContextApiWrapper::DeleteACancel()
    {
    if(!iDeleteA) return;

	iContext->RequestDeletionCancel(iDeleteAReq);
    iDeleteAMessage.Complete(KErrCancel);
    iDeleteA = EFalse;

	_LIT(KLogCom, "RPacketContext::Delete() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

TInt CPacketContextApiWrapper::TrappedMethodL(RMessage2& aMsg, TInt aLength)
    {
    TAny *buf = User::AllocLC(aLength);

    TPtr8 bufDes((TUint8*)buf, aLength);
    // TRAPD(err2, msg.ReadL(msg.Ptr0(), bufDes));
    // Changed because of migration to Client/Server V2 API
    TRAPD(err2, aMsg.ReadL(0, bufDes));
    if (err2 != KErrNone)
        {
        LOG(Log::Printf(_L("msg.ReadL error: %d"), err2));
        }
    // Warning: usage of buf is dependent on implementation of TPckg
    TRAPD(err, iSetConfigAReq = iContext->RequestConfigurationL(
         (TPacketDataConfigBase*) buf,
         CPacketContextApiWrapper::SimSetConfigAReady, 
         this));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iContext->RequestConfigurationL error: %d"), 
            err));
        }
    // NOTE: this is down here to allow RequestConfigurationL leave without bad consequences
    // if (err == KErrNone)
    iSetConfigA = ETrue;

    CleanupStack::PopAndDestroy(); // free "buf"

    if ((err == KErrNone) && (err2 == KErrNone))
        {
        return KErrNone;
        }
    else if (err != KErrNone)
        {
        return err;
        }
    else
        {
        return err2;
        }
    }

// Configure context
void CPacketContextApiWrapper::SetConfigA()
    {
    _LIT(KLogReq, "RPacketContext::SetConfig() * Requesting in Wrapper (A)");
    LogWithName(KLogReq);

    iSetConfigAMessage = Message();
    RMessage2& msg = iSetConfigAMessage;
    TInt length = msg.GetDesLength(0);
    if(length < 0)
        {
        iSession->PanicClient(EBadDescriptor);
        return; // err..
        }

    TRAPD(err1, TrappedMethodL(msg, length));
    _LIT(KLogEventErr, "ERROR: CPacketContextApiWrapper::SetConfigA - Allocation failed");
    if(err1 != KErrNone)
        {
        iSimulator->Log(KLogEventErr);
        }
    }

void CPacketContextApiWrapper::SetConfigACancel()
    {
    if(!iSetConfigA) return;

	iContext->RequestConfigurationCancel(iSetConfigAReq);
    iSetConfigAMessage.Complete(KErrCancel);
    iSetConfigA = EFalse;

	_LIT(KLogCom, "RPacketContext::SetConfig() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::GetConfigA()
    {
	_LIT(KLogReq, "RPacketContext::GetConfig() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

    iGetConfigA = ETrue;
    iGetConfigAMessage = Message();
	iGetConfigAReq = iContext->RequestGettingConfig(CPacketContextApiWrapper::SimGetConfigAReady, this);
    }

void CPacketContextApiWrapper::GetConfigACancel()
    {
    if(!iGetConfigA) return;

	iContext->RequestGettingConfigCancel(iGetConfigAReq);
    iGetConfigAMessage.Complete(KErrCancel);
    iGetConfigA = EFalse;

	_LIT(KLogCom, "RPacketContext::GetConfig() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// Configuration notification
void CPacketContextApiWrapper::NotifyConfigChangedA()
    {
	iNotifyConfigChangedA = ETrue;
	iNotifyConfigChangedAMessage = Message();

	_LIT(KLogReq, "RPacketContext::NotifyConfigChanged() * Booked in Wrapper (A)");
	LogWithName(KLogReq);
    }

void CPacketContextApiWrapper::NotifyConfigChangedACancel()
    {
	if(!iNotifyConfigChangedA) return;
	iNotifyConfigChangedAMessage.Complete(KErrCancel);
	iNotifyConfigChangedA = EFalse;

	_LIT(KLogCom, "RPacketContext::NotifyConfigChanged() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// Activate context
void CPacketContextApiWrapper::ActivateA()
    {
	_LIT(KLogReq, "RPacketContext::Activate() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

    iActivateA = ETrue;
    iActivateAMessage = Message();
	iActivateAReq = iContext->RequestActivation(CPacketContextApiWrapper::SimActivateAReady, this);
    }

void CPacketContextApiWrapper::ActivateACancel()
    {
    if(!iActivateA) return;

	iContext->RequestActivationCancel(iActivateAReq);
    iActivateAMessage.Complete(KErrCancel);
    iActivateA = EFalse;

	_LIT(KLogCom, "RPacketContext::Activate() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// Deactivate context
void CPacketContextApiWrapper::DeactivateA()
    {
	_LIT(KLogReq, "RPacketContext::Deactivate() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

    iDeactivateA = ETrue;
    iDeactivateAMessage = Message();
	iDeactivateAReq = iContext->RequestDeactivation(CPacketContextApiWrapper::SimDeactivateAReady, this);
    }

void CPacketContextApiWrapper::DeactivateACancel()
    {
    if(!iDeactivateA) return;

	iContext->RequestDeactivationCancel(iDeactivateAReq);
    iDeactivateAMessage.Complete(KErrCancel);
    iDeactivateA = EFalse;

	_LIT(KLogCom, "RPacketContext::Deactivate() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// retrieve the current context status
void CPacketContextApiWrapper::GetStatusS()
    {
	_LIT(KLogReq, "RPacketContext::GetStatus() * Requesting in Wrapper (S)");
	LogWithName(KLogReq);

	iGetStatusS = ETrue;
	iGetStatusSMessage = Message();
	iGetStatusSReq = iContext->RequestGettingStatus(CPacketContextApiWrapper::SimGetStatusSReady, this);
    }

void CPacketContextApiWrapper::NotifyStatusChangeA()
    {
    iNotifyStatusA = ETrue;
    iNotifyStatusAMessage = Message();

	_LIT(KLogReq, "RPacketContext::NotifyStatusChange() * Booked in Wrapper (A)");
	LogWithName(KLogReq);
    }

void CPacketContextApiWrapper::NotifyStatusChangeACancel()
    {
    if(!iNotifyStatusA) return;
    iNotifyStatusAMessage.Complete(KErrCancel);
    iNotifyStatusA = EFalse;

	_LIT(KLogCom, "RPacketContext::NotifyStatusChange() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// manage packet filters
void CPacketContextApiWrapper::EnumeratePacketFiltersA()
    {
	_LIT(KLogReq, "RPacketContext::EnumeratePacketFilters() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iEnumeratePacketFiltersA = ETrue;
	iEnumeratePacketFiltersAMessage = Message();
	iEnumeratePacketFiltersAReq = iContext->RequestEnumeratingPacketFilters(CPacketContextApiWrapper::SimEnumeratePacketFiltersAReady, this);
    }

void CPacketContextApiWrapper::EnumeratePacketFiltersACancel()
    {
	if(!iEnumeratePacketFiltersA) return;

	iContext->RequestEnumeratingPacketFiltersCancel(iEnumeratePacketFiltersAReq);
	iEnumeratePacketFiltersAMessage.Complete(KErrCancel);
	iEnumeratePacketFiltersA = EFalse;

	_LIT(KLogCom, "RPacketContext::EnumeratePacketFilters() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::GetPacketFilterInfoA()
    {
	_LIT(KLogReq, "RPacketContext::GetPacketFilterInfo() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iGetPacketFilterInfoA = ETrue;
	iGetPacketFilterInfoAMessage = Message();
	iGetPacketFilterInfoAReq = iContext->RequestGettingPacketFilterInfo(iGetPacketFilterInfoAMessage.Int1(), CPacketContextApiWrapper::SimGetPacketFilterInfoAReady, this);
    }

void CPacketContextApiWrapper::GetPacketFilterInfoACancel()
    {
	if(!iGetPacketFilterInfoA) return;

	iContext->RequestGettingPacketFilterInfoCancel(iGetPacketFilterInfoAReq);
	iGetPacketFilterInfoAMessage.Complete(KErrCancel);
	iGetPacketFilterInfoA = EFalse;

	_LIT(KLogCom, "RPacketContext::GetPacketFilterInfo() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::AddPacketFilterA()
    {
	_LIT(KLogReq, "RPacketContext::AddPacketFilter() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iAddPacketFilterAMessage = Message();

	TPckgBuf<RPacketContext::TPacketFilterV2> info;
	// TRAPD(err, iSession->ReadL(iAddPacketFilterAMessage.Ptr0(), info));
    // Changed because of migration to Client/Server V2 API
    TRAPD(err, iAddPacketFilterAMessage.ReadL(0, info));
	if(err != KErrNone)
        {
		iSession->PanicClient(EBadDescriptor);
		//iAddPacketFilterAMessage.Complete(err);
		return;
        }

	iAddPacketFilterA = ETrue;
	iAddPacketFilterAReq = iContext->RequestAddingPacketFilter(&(info()), CPacketContextApiWrapper::SimAddPacketFilterAReady, this);
    }

void CPacketContextApiWrapper::AddPacketFilterACancel()
    {
	if(!iAddPacketFilterA) return;

	iContext->RequestAddingPacketFilterCancel(iAddPacketFilterAReq);
	iAddPacketFilterAMessage.Complete(KErrCancel);
	iAddPacketFilterA = EFalse;

	_LIT(KLogCom, "RPacketContext::AddPacketFilter() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::RemovePacketFilterA()
    {
	_LIT(KLogReq, "RPacketContext::RemovePacketFilter() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iRemovePacketFilterA = ETrue;
	iRemovePacketFilterAMessage = Message();
	iRemovePacketFilterAReq = iContext->RequestRemovingPacketFilter(iRemovePacketFilterAMessage.Int0(), CPacketContextApiWrapper::SimRemovePacketFilterAReady, this);
    }

void CPacketContextApiWrapper::RemovePacketFilterACancel()
    {
	if(!iRemovePacketFilterA) return;

	iContext->RequestRemovingPacketFilterCancel(iRemovePacketFilterAReq);
	iRemovePacketFilterAMessage.Complete(KErrCancel);
	iRemovePacketFilterA = EFalse;

	_LIT(KLogCom, "RPacketContext::RemovePacketFilter() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// modify active context
void CPacketContextApiWrapper::ModifyActiveContextA()
    {
	_LIT(KLogReq, "RPacketContext::ModifyActiveContext() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

	iModifyActiveContextA = ETrue;
	iModifyActiveContextAMessage = Message();
	iModifyActiveContextAReq = iContext->RequestModifyingActiveContext(CPacketContextApiWrapper::SimModifyActiveContextAReady, this);
    }

void CPacketContextApiWrapper::ModifyActiveContextACancel()
    {
	if(!iModifyActiveContextA) return;

	iContext->RequestModifyingActiveContextCancel(iModifyActiveContextAReq);
	iModifyActiveContextAMessage.Complete(KErrCancel);
	iModifyActiveContextA = EFalse;

	_LIT(KLogCom, "RPacketContext::ModifyActiveContext() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// METHODS CALLED FROM SIMULATOR

void CPacketContextApiWrapper::SimInitialiseContextAReady(TInt aStatus, const TDesC8& aChannel, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		RMessage2& msg = self->iInitialiseContextAMessage;
		// TRAPD(r, msg.WriteL(msg.Ptr0(), aChannel));
		// Changed because of migration to Client/Server V2 API
		TRAPD(r, msg.WriteL(0, aChannel));
		if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
		else
            {
			// TRAP(r, msg.WriteL(msg.Ptr1(), self->iContext->GetName()));
			// Changed because of migration to Client/Server V2 API
			TRAP(r, msg.WriteL(1, self->iContext->GetName()));
			if(r!=KErrNone)
				self->iSession->PanicClient(EBadDescriptor);
            }
        }

	self->iInitialiseContextAMessage.Complete(aStatus);
	self->iInitialiseContextA = EFalse;

	_LIT(KLogCom, "RPacketContext::InitialiseContext() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimDeleteAReady(TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	self->iContext->FinalizeDeletion();

	self->iDeleteAMessage.Complete(KErrNone);
	self->iDeleteA = EFalse;

	_LIT(KLogCom, "RPacketContext::Delete() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);

	self->CloseWrapper();
    }

void CPacketContextApiWrapper::SimSetConfigAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iSetConfigAMessage.Complete(aStatus);
	self->iSetConfigA = EFalse;

	_LIT(KLogCom, "RPacketContext::SetConfig() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimGetConfigAReady(TInt aStatus, const TDesC8& aConfig, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		// Error: we should check package types here!
		// TRAPD(r, self->iGetConfigAMessage.WriteL(self->iGetConfigAMessage.Ptr0(), aConfig));
		// Changed because of migration to Client/Server V2 API
		TRAPD(r, self->iGetConfigAMessage.WriteL(0, aConfig));
		if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetConfigAMessage.Complete(aStatus);
	self->iGetConfigA = EFalse;

	_LIT(KLogCom, "RPacketContext::GetConfig() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimActivateAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iActivateAMessage.Complete(aStatus);
	self->iActivateA = EFalse;

	_LIT(KLogCom, "RPacketContext::Activate() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimDeactivateAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iDeactivateAMessage.Complete(aStatus);
	self->iDeactivateA = EFalse;

	_LIT(KLogCom, "RPacketContext::Deactivate() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimGetStatusSReady(TInt aStatus, RPacketContext::TContextStatus aCStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
	    TPckgC<RPacketContext::TContextStatus> statusPckg(aCStatus);
	    // TRAPD(r,self->iGetStatusSMessage.WriteL(self->iGetStatusSMessage.Ptr0(), statusPckg));
	    // Changed because of migration to Client/Server V2 API
	    TRAPD(r,self->iGetStatusSMessage.WriteL(0, statusPckg));
	    if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetStatusSMessage.Complete(aStatus);
	self->iGetStatusS = EFalse;

	_LIT(KLogCom, "RPacketContext::GetStatus() * Completed in Wrapper (S)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimStatusUpdated(CPacketContext* aContext, RPacketContext::TContextStatus aStatus)
    {
	// should be safe even if iContext == NULL
	if(!iNotifyStatusA || aContext != iContext) return;

	TPckgC<RPacketContext::TContextStatus> statusPckg(aStatus);
	// TRAPD(err, iNotifyStatusAMessage.WriteL(iNotifyStatusAMessage.Ptr0(), statusPckg));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iNotifyStatusAMessage.WriteL(0, statusPckg));
	if(err!=KErrNone)
		iSession->PanicClient(EBadDescriptor);

	iNotifyStatusAMessage.Complete(err);
	iNotifyStatusA = EFalse;

	_LIT(KLogCom, "RPacketContext::NotifyStatusChange() * Completed in Wrapper (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimConfigChanged(CPacketContext* aContext, const TDesC8& aConfig)
    {
	// should be safe even if iContext == NULL
	if(!iNotifyConfigChangedA || aContext != iContext) return;

	// Error: we should check package types here!
	// TRAPD(err, iNotifyConfigChangedAMessage.WriteL(iNotifyConfigChangedAMessage.Ptr0(), aConfig));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iNotifyConfigChangedAMessage.WriteL(0, aConfig));
	if(err != KErrNone)
		iSession->PanicClient(EBadDescriptor);

	iNotifyConfigChangedAMessage.Complete(err);
	iNotifyConfigChangedA = EFalse;

	_LIT(KLogCom, "RPacketContext::NotifyConfigChanged() * Completed in Wrapper (A)");
	LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimEnumeratePacketFiltersAReady(TInt aStatus, TInt aCount, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		TPckgC<TInt> countPckg(aCount);
		// TRAPD(err, self->iEnumeratePacketFiltersAMessage.WriteL(self->iEnumeratePacketFiltersAMessage.Ptr0(),
        //                                                         countPckg));
        // Changed because of migration to Client/Server V2 API
        TRAPD(err, self->iEnumeratePacketFiltersAMessage.WriteL(0, countPckg));
		if(err!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iEnumeratePacketFiltersAMessage.Complete(aStatus);
	self->iEnumeratePacketFiltersA = EFalse;

	_LIT(KLogCom, "RPacketContext::EnumeratePacketFilters() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimGetPacketFilterInfoAReady(TInt aStatus, const TDesC8& aInfo, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		// TRAPD(r,self->iGetPacketFilterInfoAMessage.WriteL(self->iGetPacketFilterInfoAMessage.Ptr0(), aInfo));
		// Changed because of migration to Client/Server V2 API
		TRAPD(r,self->iGetPacketFilterInfoAMessage.WriteL(0, aInfo));
		if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetPacketFilterInfoAMessage.Complete(aStatus);
	self->iGetPacketFilterInfoA = EFalse;

	_LIT(KLogCom, "RPacketContext::GetPacketFilterInfo() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimAddPacketFilterAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iAddPacketFilterAMessage.Complete(aStatus);
	self->iAddPacketFilterA = EFalse;

	_LIT(KLogCom, "RPacketContext::AddPacketFilter() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimRemovePacketFilterAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iRemovePacketFilterAMessage.Complete(aStatus);
	self->iRemovePacketFilterA = EFalse;

	_LIT(KLogCom, "RPacketContext::RemovePacketFilter() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::SimModifyActiveContextAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketContextApiWrapper* self = (CPacketContextApiWrapper*) aSelf;
	self->iModifyActiveContextAMessage.Complete(aStatus);
	self->iModifyActiveContextA = EFalse;

	_LIT(KLogCom, "RPacketContext::ModifyActiveContext() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketContextApiWrapper::LogWithName(const TDesC& aMsg)
    {
	_LIT(KLogName, "Context = ");
	_LIT(KLogDele, "(deleted)");
	TBuf<65> namebuf;
	namebuf = KLogName;
	if(iContext) namebuf.Append(iContext->GetName());
	else namebuf.Append(KLogDele);
	iSimulator->Log(aMsg, namebuf);
    }

#ifdef USSE_DEBUG_CHECKS
void CPacketContextApiWrapper::DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession)
    #else
    void CPacketContextApiWrapper::DebugCheck(CUmtsSimulator*, CUmtsSimServSession*)
    #endif
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(aSession != iSession)
		User::Invariant();
	if(iContext)
		iContext->DebugCheck(iSimulator);
    #endif
    }
