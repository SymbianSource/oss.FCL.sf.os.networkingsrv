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
// usse_pservice_wrapper.cpp - wrapper for server side handling of RPacketService calls
//

#include "usse_packet.h"
#include "usse_packet_wrapper.h"
#include "usse_simulator.h"

/********************
    * CPacketServiceApiWrapper class
    *
    * Works as a wrapper for passing requests originated from RPacketService
    * to the state machine owned by server. The class is subsession
    * specific and and client information (like notification requests)
    * is stored here.
    */

CPacketServiceApiWrapper* CPacketServiceApiWrapper::NewL(CUmtsSimServSession* aSession)
    {
    CPacketServiceApiWrapper* self = new (ELeave) CPacketServiceApiWrapper(aSession);
	self->CObject::Dec(); // allow destruction if ConstructL leaves
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop();
	self->CObject::Inc();
    return self;
    }

CPacketServiceApiWrapper::CPacketServiceApiWrapper(CUmtsSimServSession* aSession)
	: iAttachA(EFalse), iDetachA(EFalse), iGetStatusS(EFalse), iNotifyStatusA(EFalse),
          iNotifyContextAddedA(EFalse), iEnumerateContextsA(EFalse), iGetContextInfoA(EFalse),
          iEnumerateNifsA(EFalse), iGetNifInfoA(EFalse), iEnumerateContextsInNifA(EFalse),
          iGetContextNameInNifA(EFalse), iSession(aSession), iSimulator(aSession->GetSimulator())
    {
	// nothing here
    }

void CPacketServiceApiWrapper::ConstructL()
    {
	_LIT(KLogNew, "RPacketService::Open() * ApiWrapper created");
	iSimulator->Log(KLogNew);
    }

CPacketServiceApiWrapper::~CPacketServiceApiWrapper()
    {
	CloseWrapper();
    }

void CPacketServiceApiWrapper::CloseWrapper()
    {
	_LIT(KLogClose, "?::?() * PacketServiceApiWrapper closing");
	iSimulator->Log(KLogClose);

	// Should the associated RMessages be .Completed()?
	// .. for now they are not but I don't know which approach causes more problems on the client side..

	// first cancel notifications
	if(iNotifyStatusA) iNotifyStatusA = EFalse;
	if(iNotifyContextAddedA) iNotifyContextAddedA = EFalse;

	CPacketService* service = iSimulator->GetPacketService();
	if(iAttachA) { iAttachA = EFalse; service->RequestAttachmentCancel(iAttachAReq); }
	if(iDetachA) { iDetachA = EFalse; service->RequestDetachmentCancel(iDetachAReq); }
	if(iGetStatusS) { iGetStatusS = EFalse; service->RequestGettingStatusCancel(iGetStatusSReq); }
	if(iEnumerateContextsA) { iEnumerateContextsA = EFalse; service->RequestEnumeratingContextsCancel(iEnumerateContextsAReq); }
	if(iGetContextInfoA) { iGetContextInfoA = EFalse; service->RequestGettingContextInfoCancel(iGetContextInfoAReq); }
	if(iEnumerateNifsA) { iEnumerateNifsA = EFalse; service->RequestEnumeratingNifsCancel(iEnumerateNifsAReq); }
	if(iGetNifInfoA) { iGetNifInfoA = EFalse; service->RequestGettingNifInfoCancel(iGetNifInfoAReq); }
	if(iEnumerateContextsInNifA) { iEnumerateContextsInNifA = EFalse; service->RequestEnumeratingContextsInNifCancel(iEnumerateContextsInNifAReq); }
	if(iGetContextNameInNifA) { iGetContextNameInNifA = EFalse; service->RequestGettingContextNameInNifCancel(iGetContextNameInNifAReq); }
    }

// utility: get message from session
const RMessage2& CPacketServiceApiWrapper::Message() const
    {
    // return iSession->Message();
    // Changed because of migration to Client/Server V2 API
    return iSession->iMessage;
    }


// ETEL PACKET DATA API / PACKET SERVICE

// Attach to network
void CPacketServiceApiWrapper::AttachA()
    {
	_LIT(KLogReq, "RPacketService::Attach() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

    iAttachA = ETrue;
    iAttachAMessage = Message();
	iAttachAReq = iSimulator->GetPacketService()->RequestAttachment(CPacketServiceApiWrapper::SimAttachAReady, this);
    }

void CPacketServiceApiWrapper::AttachACancel()
    {
    if(!iAttachA) return;

	iSimulator->GetPacketService()->RequestAttachmentCancel(iAttachAReq);
    iAttachAMessage.Complete(KErrCancel);
    iAttachA = EFalse;

	_LIT(KLogCom, "RPacketService::Attach() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

// Detach from network
void CPacketServiceApiWrapper::DetachA()
    {
	_LIT(KLogReq, "RPacketService::Detach() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

    iDetachA = ETrue;
    iDetachAMessage = Message();
	iDetachAReq = iSimulator->GetPacketService()->RequestDetachment(CPacketServiceApiWrapper::SimDetachAReady, this);
    }

void CPacketServiceApiWrapper::DetachACancel()
    {
    if(!iDetachA) return;

	iSimulator->GetPacketService()->RequestDetachmentCancel(iDetachAReq);
    iDetachAMessage.Complete(KErrCancel);
    iDetachA = EFalse;

	_LIT(KLogCom, "RPacketService::Detach() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

// retrieve the current UMTS system status
void CPacketServiceApiWrapper::GetStatusS()
    {
	_LIT(KLogReq, "RPacketService::GetStatus() * Requesting in Wrapper (S)");
	iSimulator->Log(KLogReq);

	iGetStatusS = ETrue;
	iGetStatusSMessage = Message();
	iGetStatusSReq = iSimulator->GetPacketService()->RequestGettingStatus(CPacketServiceApiWrapper::SimGetStatusSReady, this);
    }

void CPacketServiceApiWrapper::NotifyStatusChangeA()
    {
    iNotifyStatusA = ETrue;
    iNotifyStatusAMessage = Message();

	_LIT(KLogReq, "RPacketService::NotifyStatusChange() * Booked in Wrapper (A)");
	iSimulator->Log(KLogReq);
    }

void CPacketServiceApiWrapper::NotifyStatusChangeACancel()
    {
    if(!iNotifyStatusA) return;
    iNotifyStatusAMessage.Complete(KErrCancel);
    iNotifyStatusA = EFalse;

	_LIT(KLogCom, "RPacketService::NotifyStatusChange() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

// deal with contexts
void CPacketServiceApiWrapper::NotifyContextAddedA()
    {
	iNotifyContextAddedA = ETrue;
	iNotifyContextAddedAMessage = Message();

	_LIT(KLogReq, "RPacketService::NotifyContextAdded() * Booked in Wrapper (A)");
	iSimulator->Log(KLogReq);
    }

void CPacketServiceApiWrapper::NotifyContextAddedACancel()
    {
	if(!iNotifyContextAddedA) return;
	iNotifyContextAddedAMessage.Complete(KErrCancel);
	iNotifyContextAddedA = EFalse;

	_LIT(KLogCom, "RPacketService::NotifyContextAdded() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::EnumerateContextsA()
    {
	_LIT(KLogReq, "RPacketService::EnumerateContexts() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iEnumerateContextsA = ETrue;
	iEnumerateContextsAMessage = Message();
	iEnumerateContextsAReq = iSimulator->GetPacketService()->RequestEnumeratingContexts(SimEnumerateContextsAReady, this);
    }

void CPacketServiceApiWrapper::EnumerateContextsACancel()
    {
	if(!iEnumerateContextsA) return;

	iSimulator->GetPacketService()->RequestEnumeratingContextsCancel(iEnumerateContextsAReq);
	iEnumerateContextsAMessage.Complete(KErrCancel);
	iEnumerateContextsA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateContexts() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::GetContextInfoA()
    {
	_LIT(KLogReq, "RPacketService::GetContextInfo() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iGetContextInfoA = ETrue;
	iGetContextInfoAMessage = Message();
	iGetContextInfoAReq = iSimulator->GetPacketService()->RequestGettingContextInfo(iGetContextInfoAMessage.Int0(),
                                                                                    SimGetContextInfoAReady, this);
    }

void CPacketServiceApiWrapper::GetContextInfoACancel()
    {
	if(!iGetContextInfoA) return;

	iSimulator->GetPacketService()->RequestGettingContextInfoCancel(iGetContextInfoAReq);
	iGetContextInfoAMessage.Complete(KErrCancel);
	iGetContextInfoA = EFalse;

	_LIT(KLogCom, "RPacketService::GetContextInfo() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

// deal with nifs and contexts in them
void CPacketServiceApiWrapper::EnumerateNifsA()
    {
	_LIT(KLogReq, "RPacketService::EnumerateNifs() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iEnumerateNifsA = ETrue;
	iEnumerateNifsAMessage = Message();
	iEnumerateNifsAReq = iSimulator->GetPacketService()->RequestEnumeratingNifs(SimEnumerateNifsAReady, this);
    }

void CPacketServiceApiWrapper::EnumerateNifsACancel()
    {
	if(!iEnumerateNifsA) return;

	iSimulator->GetPacketService()->RequestEnumeratingNifsCancel(iEnumerateNifsAReq);
	iEnumerateNifsAMessage.Complete(KErrCancel);
	iEnumerateNifsA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateNifs() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::GetNifInfoA()
    {
	_LIT(KLogReq, "RPacketService::GetNifInfo() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iGetNifInfoA = ETrue;
	iGetNifInfoAMessage = Message();
	iGetNifInfoAReq = iSimulator->GetPacketService()->RequestGettingNifInfo(iGetNifInfoAMessage.Int0(),
                                                                            SimGetNifInfoAReady, this);
    }

void CPacketServiceApiWrapper::GetNifInfoACancel()
    {
	if(!iGetNifInfoA) return;

	iSimulator->GetPacketService()->RequestGettingNifInfoCancel(iGetNifInfoAReq);
	iGetNifInfoAMessage.Complete(KErrCancel);
	iGetNifInfoA = EFalse;

	_LIT(KLogCom, "RPacketService::GetNifInfo() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::EnumerateContextsInNifA()
    {
	_LIT(KLogReq, "RPacketService::EnumerateContextsInNif() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iEnumerateContextsInNifAMessage = Message();

	TBuf<65> context;
	// TRAPD(err, iSession->ReadL(iEnumerateContextsInNifAMessage.Ptr1(), context));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iEnumerateContextsInNifAMessage.ReadL(1, context));
	if(err)
        {
		iSession->PanicClient(EBadDescriptor);
		//iEnumerateContextsInNifAMessage.Complete(err);
		return;
        }

	iEnumerateContextsInNifA = ETrue;
	iEnumerateContextsInNifAReq = iSimulator->GetPacketService()->RequestEnumeratingContextsInNif(
		&context, SimEnumerateContextsInNifAReady, this);
    }

void CPacketServiceApiWrapper::EnumerateContextsInNifACancel()
    {
	if(!iEnumerateContextsInNifA) return;

	iSimulator->GetPacketService()->RequestEnumeratingContextsInNifCancel(iEnumerateContextsInNifAReq);
	iEnumerateContextsInNifAMessage.Complete(KErrCancel);
	iEnumerateContextsInNifA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateContextsInNif() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::GetContextNameInNifA()
    {
	_LIT(KLogReq, "RPacketService::GetContextNameInNif() * Requesting in Wrapper (A)");
	iSimulator->Log(KLogReq);

	iGetContextNameInNifAMessage = Message();

	TBuf<65> context;
	// TRAPD(err, iSession->ReadL(iGetContextNameInNifAMessage.Ptr1(), context));
	// Changed because of migration to Client/Server V2 API
    TRAPD(err, iGetContextNameInNifAMessage.ReadL(1, context));
	if(err)
        {
		iSession->PanicClient(EBadDescriptor);
		//iGetContextNameInNifAMessage.Complete(err);
		return;
        }

	iGetContextNameInNifA = ETrue;
	iGetContextNameInNifAReq = iSimulator->GetPacketService()->RequestGettingContextNameInNif(
		iGetContextNameInNifAMessage.Int0(), &context, SimGetContextNameInNifAReady, this);
    }

void CPacketServiceApiWrapper::GetContextNameInNifACancel()
    {
	if(!iGetContextNameInNifA) return;

	iSimulator->GetPacketService()->RequestGettingContextNameInNifCancel(iGetContextNameInNifAReq);
	iGetContextNameInNifAMessage.Complete(KErrCancel);
	iGetContextNameInNifA = EFalse;

	_LIT(KLogCom, "RPacketService::GetContextNameInNif() * Completed in Wrapper (Cancelled) (A)");
	iSimulator->Log(KLogCom);
    }

// METHODS CALLED FROM SIMULATOR

void CPacketServiceApiWrapper::SimAttachAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;
	self->iAttachAMessage.Complete(aStatus);
	self->iAttachA = EFalse;

	_LIT(KLogCom, "RPacketService::Attach() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimDetachAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;
	self->iDetachAMessage.Complete(aStatus);
	self->iDetachA = EFalse;

	_LIT(KLogCom, "RPacketService::Detach() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimGetStatusSReady(TInt aStatus, RPacketService::TStatus aSStatus, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
	    TPckgC<RPacketService::TStatus> statusPckg(aSStatus);
		// TRAPD(r,self->iGetStatusSMessage.WriteL(self->iGetStatusSMessage.Ptr0(), statusPckg));
		// Changed because of migration to Client/Server V2 API
		TRAPD(r,self->iGetStatusSMessage.WriteL(0, statusPckg));
		if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetStatusSMessage.Complete(aStatus);
	self->iGetStatusS = EFalse;

	_LIT(KLogCom, "RPacketService::GetStatus() * Completed in Wrapper (S)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimStatusUpdated(RPacketService::TStatus aStatus)
    {
	if(!iNotifyStatusA) return;

	TPckgC<RPacketService::TStatus> statusPckg(aStatus);
	// TRAPD(err, iNotifyStatusAMessage.WriteL(iNotifyStatusAMessage.Ptr0(), statusPckg));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iNotifyStatusAMessage.WriteL(0, statusPckg));
	if(err!=KErrNone)
		iSession->PanicClient(EBadDescriptor);

	iNotifyStatusAMessage.Complete(err);
	iNotifyStatusA = EFalse;

	_LIT(KLogCom, "RPacketService::NotifyStatusChange() * Completed in Wrapper (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimContextAdded(const TDesC& aId)
    {
	if(!iNotifyContextAddedA) return;
	// TRAPD(err, iNotifyContextAddedAMessage.WriteL(iNotifyContextAddedAMessage.Ptr0(), aId));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iNotifyContextAddedAMessage.WriteL(0, aId));
	if(err != KErrNone)
		iSession->PanicClient(EBadDescriptor);

	iNotifyContextAddedAMessage.Complete(err);
	iNotifyContextAddedA = EFalse;

	_LIT(KLogCom, "RPacketService::NotifyContextAdded() * Completed in Wrapper (A)");
	iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimEnumerateContextsAReady(TInt aStatus, TInt aCount, TInt aMax, TAny *aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		TPckgC<TInt> countPckg(aCount);
		TPckgC<TInt> maxPckg(aMax);
		// TRAPD(err, self->iEnumerateContextsAMessage.WriteL(self->iEnumerateContextsAMessage.Ptr0(), countPckg));
		// Changed because of migration to Client/Server V2 API
		TRAPD(err, self->iEnumerateContextsAMessage.WriteL(0, countPckg));
		if(err!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
		else
            {
			// TRAP(err, self->iEnumerateContextsAMessage.WriteL(self->iEnumerateContextsAMessage.Ptr1(), maxPckg));
			// Changed because of migration to Client/Server V2 API
			TRAP(err, self->iEnumerateContextsAMessage.WriteL(1, maxPckg));
			if(err!=KErrNone)
				self->iSession->PanicClient(EBadDescriptor);
            }
        }

	self->iEnumerateContextsAMessage.Complete(aStatus);
	self->iEnumerateContextsA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateContexts() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimGetContextInfoAReady(TInt aStatus, const TDesC* aCName,
													   RPacketContext::TContextStatus aCStatus, TAny *aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		RPacketService::TContextInfo info;
		info.iStatus = aCStatus;
		info.iName = *aCName;

		TPckgC<RPacketService::TContextInfo> infoPckg(info);
		// TRAPD(err, self->iGetContextInfoAMessage.WriteL(self->iGetContextInfoAMessage.Ptr1(), infoPckg));
		// Changed because of migration to Client/Server V2 API
		TRAPD(err, self->iGetContextInfoAMessage.WriteL(1, infoPckg));
		if(err!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetContextInfoAMessage.Complete(aStatus);
	self->iGetContextInfoA = EFalse;

	_LIT(KLogCom, "RPacketService::GetContextInfo() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimEnumerateNifsAReady(TInt aStatus, TInt aCount, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		TPckgC<TInt> countPckg(aCount);
		// TRAPD(err, self->iEnumerateNifsAMessage.WriteL(self->iEnumerateNifsAMessage.Ptr0(), countPckg));
		// Changed because of migration to Client/Server V2 API
		TRAPD(err, self->iEnumerateNifsAMessage.WriteL(0, countPckg));
		if(err!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iEnumerateNifsAMessage.Complete(aStatus);
	self->iEnumerateNifsA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateNifs() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimGetNifInfoAReady(TInt aStatus, const TDesC8* aInfo, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		// TRAP(aStatus, self->iGetNifInfoAMessage.WriteL(self->iGetNifInfoAMessage.Ptr1(), *aInfo));
		// Changed because of migration to Client/Server V2 API
		TRAP(aStatus, self->iGetNifInfoAMessage.WriteL(1, *aInfo));
		if(aStatus != KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetNifInfoAMessage.Complete(aStatus);
	self->iGetNifInfoA = EFalse;

	_LIT(KLogCom, "RPacketService::GetNifInfo() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimEnumerateContextsInNifAReady(TInt aStatus, TInt aCount, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		TPckgC<TInt> countPckg(aCount);
		// TRAPD(err, self->iEnumerateContextsInNifAMessage.WriteL(self->iEnumerateContextsInNifAMessage.Ptr0(),
        //                                                        countPckg));
        // Changed because of migration to Client/Server V2 API
		TRAPD(err, self->iEnumerateContextsInNifAMessage.WriteL(0, countPckg));
		if(err!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iEnumerateContextsInNifAMessage.Complete(aStatus);
	self->iEnumerateContextsInNifA = EFalse;

	_LIT(KLogCom, "RPacketService::EnumerateContextsInNif() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

void CPacketServiceApiWrapper::SimGetContextNameInNifAReady(TInt aStatus, const TDesC* aName, TAny* aSelf)
    {
	CPacketServiceApiWrapper* self = (CPacketServiceApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		// TRAP(aStatus, self->iGetContextNameInNifAMessage.WriteL(self->iGetContextNameInNifAMessage.Ptr2(), *aName));
		// Changed because of migration to Client/Server V2 API
		TRAP(aStatus, self->iGetContextNameInNifAMessage.WriteL(2, *aName));
		if(aStatus != KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetContextNameInNifAMessage.Complete(aStatus);
	self->iGetContextNameInNifA = EFalse;

	_LIT(KLogCom, "RPacketService::GetContextNameInNif() * Completed in Wrapper (A)");
	self->iSimulator->Log(KLogCom);
    }

#ifdef USSE_DEBUG_CHECKS
void CPacketServiceApiWrapper::DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession)
    #else
    void CPacketServiceApiWrapper::DebugCheck(CUmtsSimulator*, CUmtsSimServSession*)
    #endif
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(aSession != iSession)
		User::Invariant();
	iSimulator->GetPacketService()->DebugCheck(iSimulator);
    #endif
    }
