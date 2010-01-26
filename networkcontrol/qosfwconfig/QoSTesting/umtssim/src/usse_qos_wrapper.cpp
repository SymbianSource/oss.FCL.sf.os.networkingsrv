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
// usse_qos_wrapper.cpp - wrapper for server side handling of RPacketQoS calls
//

#include "us_cliserv.h"
#include "usse_qos_wrapper.h"
#include "usse_simulator.h"
#include "log.h"

/********************
    * CPacketQoSApiWrapper class
    *
    * Works as a wrapper for passing requests originated from RPacketQoS
    * to the state machine owned by server. The class is subsession
    * specific and and client information (like notification requests)
    * is stored here.
    */

CPacketQoSApiWrapper* CPacketQoSApiWrapper::NewL(CUmtsSimServSession* aSession)
    {
	// const RMessage& message = aSession->Message();
    // Changed because of migration to Client/Server V2 API
    const RMessage2& message = aSession->iMessage;
	const TAny* name = message.Ptr0();
	const TAny* context = message.Ptr2();
	TUmtsSimServQoSOpenMode mode = (TUmtsSimServQoSOpenMode) message.Int1();

    CPacketQoSApiWrapper* self = new (ELeave) CPacketQoSApiWrapper(aSession);
	self->CObject::Dec(); // allow destruction if ConstructL leaves
    CleanupStack::PushL(self);
    self->ConstructL(mode, name, context);
    CleanupStack::Pop();
	self->CObject::Inc();
    return self;
    }

CPacketQoSApiWrapper::CPacketQoSApiWrapper(CUmtsSimServSession* aSession)
	: iSetProfileParametersA(EFalse), iGetProfileParametersA(EFalse),
          iNotifyProfileChangedA(EFalse), iSession(aSession),
          iSimulator(aSession->GetSimulator()), iQoS(NULL)
    {
	// nothing to do here
    }

CPacketQoSApiWrapper::~CPacketQoSApiWrapper()
    {
	CloseWrapper();
    }

void CPacketQoSApiWrapper::ConstructL(TUmtsSimServQoSOpenMode aMode,
									  const TAny* /* aName */, const TAny* /* aContext */)
    {
    // This is called by method: CPacketQoSApiWrapper* CPacketQoSApiWrapper::NewL
    // 
    // self->ConstructL(mode, name, context);
    // 
    // With following declarations:
    // 
    // const TAny* name = message.Ptr0();
    // const TAny* context = message.Ptr2();
    // 
	// first check whether to accept or not
	TInt status, delay; // delay not really used
	iSimulator->CheckRequest(EUmtsSimServCreatePacketQoSSubSession, status, delay);
	User::LeaveIfError(status);

	// get our context
	TBuf<65> context;
	// TRAPD(err, iSession->iMessage.ReadL(aContext, context));
	// Changed because of migration to Client/Server V2 API
	TRAPD(err, iSession->Message().ReadL(2, context));
	if(err != KErrNone)
        {
		iSession->PanicClient(EBadDescriptor);
		User::Leave(err);
        }

	switch(aMode)
        {
            case EUmtsSimServOpenNewPacketQoS:
            {
			TBuf<65> name;
			User::LeaveIfError(iSimulator->NewPacketQoS(context, name, iQoS));

			// TRAPD(err, iSession->iMessage.WriteL(aName, name));
			// Changed because of migration to Client/Server V2 API
			TRAPD(err, iSession->Message().WriteL(0, name));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }
            }
            break;
            case EUmtsSimServOpenExistingPacketQoS:
            {
			TBuf<65> name;
			// TRAPD(err, iSession->iMessage.ReadL(aName, name));
			// Changed because of migration to Client/Server V2 API
			TRAPD(err, iSession->Message().ReadL(0, name));
			if(err != KErrNone)
                {
				iSession->PanicClient(EBadDescriptor);
				User::Leave(err);
                }

			User::LeaveIfError(iSimulator->GetPacketQoS(context, name, iQoS, ETrue));
            }
            break;
            default:
                User::Leave(KErrArgument);
                break;
        }

	_LIT(KLogNew, "RPacketQoS::Open() * ApiWrapper created");
	LogWithName(KLogNew);
    }

void CPacketQoSApiWrapper::CloseWrapper()
    {
	_LIT(KLogClose, "?::?() * PacketQoSApiWrapper closing");
	LogWithName(KLogClose);

	// Should the associated RMessages be .Completed()?
	// .. for now they are not but I don't know which approach causes more problems on the client side..

	// first cancel notifications
	if(iNotifyProfileChangedA) iNotifyProfileChangedA = EFalse;

	if(iSetProfileParametersA) { iSetProfileParametersA = EFalse; iQoS->RequestSettingProfileParametersCancel(iSetProfileParametersAReq); }
	if(iGetProfileParametersA) { iGetProfileParametersA = EFalse; iQoS->RequestGettingProfileParametersCancel(iGetProfileParametersAReq); }

	if(iQoS)
        {
		
        //		User::LeaveIfError(iSimulator->RemovePacketQoS(iQoS->GetContext()->GetName(), iQoS->GetName()));
		TRAPD(err,iSimulator->RemovePacketQoS(iQoS->GetContext()->GetName(), iQoS->GetName()));
		if(err)
            {
			_LIT(KLogNew2, "CPacketQoSApiWrapper::CloseWrapper * err");
			LogWithName(KLogNew2);
		
            }
		iQoS = NULL;
        }

	iSimulator->DebugCheck();
    }

// utility: get message from session
const RMessage2& CPacketQoSApiWrapper::Message() const
    {
    // return iSession->Message();
    // Changed because of migration to Client/Server V2 API
    return iSession->iMessage;
    }


// ETEL PACKET DATA API / PACKET QOS

TInt CPacketQoSApiWrapper::TrappedMethodL(RMessage2& aMsg, TInt aLength)
    {
    TAny* buf =User::AllocLC(aLength); 
    
    TPtr8 bufDes((TUint8*)buf, aLength);
    aMsg.ReadL(0, bufDes);

    // Warning: usage of buf is dependent on implementation of TPckg
    TRAPD(err, iSetProfileParametersAReq = 
          iQoS->RequestSettingProfileParametersL((TPacketDataConfigBase*) buf,
          CPacketQoSApiWrapper::SimSetProfileParametersAReady, 
          this));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iQoS->RequestSettingProfileParametersL error: %d")
            , err));
        }
    // NOTE: this is down here to allow RequestSettingProfileParametersL leave without bad consequences
    iSetProfileParametersA = ETrue;

    CleanupStack::PopAndDestroy(); // free "buf"
    
    return err;
    }

// Set/Get QoS profile parameters
void CPacketQoSApiWrapper::SetProfileParametersA()
    {
    _LIT(KLogReq, "RPacketQoS::SetProfileParameters() * Requesting in Wrapper (A)");
    LogWithName(KLogReq);

    iSetProfileParametersAMessage = Message();
    RMessage2& msg = iSetProfileParametersAMessage;

    TInt length = msg.GetDesLength(0);
    if(length < 0)
        {
        iSession->PanicClient(EBadDescriptor);
        return; // err..
        }

    TRAPD(err1, TrappedMethodL(msg, length));

    _LIT(KLogEventErr, "ERROR: CPacketQoSApiWrapper::SetProfileParametersA - Allocation failed");
    if(err1 != KErrNone)
        {
        iSimulator->Log(KLogEventErr);
        }
    }

void CPacketQoSApiWrapper::SetProfileParametersACancel()
    {
    if(!iSetProfileParametersA) return;

	iQoS->RequestSettingProfileParametersCancel(iSetProfileParametersAReq);
    iSetProfileParametersAMessage.Complete(KErrCancel);
    iSetProfileParametersA = EFalse;

	_LIT(KLogCom, "RPacketQoS::SetProfileParameters() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketQoSApiWrapper::GetProfileParametersA()
    {
	_LIT(KLogReq, "RPacketQoS::GetProfileParameters() * Requesting in Wrapper (A)");
	LogWithName(KLogReq);

    iGetProfileParametersA = ETrue;
    iGetProfileParametersAMessage = Message();
	iGetProfileParametersAReq = iQoS->RequestGettingProfileParameters(CPacketQoSApiWrapper::SimGetProfileParametersAReady, this);
    }

void CPacketQoSApiWrapper::GetProfileParametersACancel()
    {
    if(!iGetProfileParametersA) return;

	iQoS->RequestGettingProfileParametersCancel(iGetProfileParametersAReq);
    iGetProfileParametersAMessage.Complete(KErrCancel);
    iGetProfileParametersA = EFalse;

	_LIT(KLogCom, "RPacketQoS::GetProfileParameters() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

void CPacketQoSApiWrapper::NotifyProfileChangedA()
    {
	iNotifyProfileChangedA = ETrue;
	iNotifyProfileChangedAMessage = Message();

	_LIT(KLogReq, "RPacketQoS::NotifyProfileChanged() * Booked in Wrapper (A)");
	LogWithName(KLogReq);
    }

void CPacketQoSApiWrapper::NotifyProfileChangedACancel()
    {
	if(!iNotifyProfileChangedA) return;
	iNotifyProfileChangedAMessage.Complete(KErrCancel);
	iNotifyProfileChangedA = EFalse;

	_LIT(KLogCom, "RPacketQoS::NotifyProfileChanged() * Completed in Wrapper (Cancelled) (A)");
	LogWithName(KLogCom);
    }

// METHODS CALLED FROM SIMULATOR

void CPacketQoSApiWrapper::SimSetProfileParametersAReady(TInt aStatus, TAny* aSelf)
    {
	CPacketQoSApiWrapper* self = (CPacketQoSApiWrapper*) aSelf;
	self->iSetProfileParametersAMessage.Complete(aStatus);
	self->iSetProfileParametersA = EFalse;

	_LIT(KLogCom, "RPacketQoS::SetProfileParameters() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketQoSApiWrapper::SimGetProfileParametersAReady(TInt aStatus, const TDesC8& aConfig, TAny* aSelf)
    {
	CPacketQoSApiWrapper* self = (CPacketQoSApiWrapper*) aSelf;

	if(aStatus == KErrNone)
        {
		// Error: we should check package types here!
		// TRAPD(r,self->iGetProfileParametersAMessage.WriteL(self->iGetProfileParametersAMessage.Ptr0(), aConfig));
		// Changed because of migration to Client/Server V2 API
		TRAPD(r,self->iGetProfileParametersAMessage.WriteL(0, aConfig));
		if(r!=KErrNone)
			self->iSession->PanicClient(EBadDescriptor);
        }

	self->iGetProfileParametersAMessage.Complete(aStatus);
	self->iGetProfileParametersA = EFalse;

	_LIT(KLogCom, "RPacketQoS::GetProfileParameters() * Completed in Wrapper (A)");
	self->LogWithName(KLogCom);
    }

void CPacketQoSApiWrapper::SimProfileChanged(CPacketContext* aContext, const TDesC8& aParameters)
    {
	// if wrapper closed (SimContextDeleted) iNotifyProfileChangedA == EFalse => iQoS can be NULL
	if(!iNotifyProfileChangedA || aContext != iQoS->GetContext()) return;

	RMessage2& msg = iNotifyProfileChangedAMessage;
	// Error: we should check package types here!
	// TRAPD(r, msg.WriteL(msg.Ptr0(), aParameters));
	// Changed because of migration to Client/Server V2 API
	TRAPD(r, msg.WriteL(0, aParameters));
	if(r != KErrNone)
		iSession->PanicClient(EBadDescriptor);

	msg.Complete(r);
	iNotifyProfileChangedA = EFalse;

	_LIT(KLogCom, "RPacketQoS::NotifyProfileChanged() * Completed in Wrapper (A)");
	LogWithName(KLogCom);
    }

void CPacketQoSApiWrapper::SimContextDeleted(CPacketContext* aContext)
    {
	if(iQoS == NULL || aContext != iQoS->GetContext()) return;

	_LIT(KLogAutoClose, "?::?() * Closing PacketQoSApiWrapper because context was deleted");
	LogWithName(KLogAutoClose);

	CloseWrapper();
    }

void CPacketQoSApiWrapper::LogWithName(const TDesC& aMsg)
    {
	_LIT(KLogName, ", QoS = ");
	_LIT(KLogName2, "Context = ");
	_LIT(KLogDele, "(deleted)");
	_LIT(KLogNull, "(null)");
	TBuf<80> namebuf;
	namebuf = KLogName2;

	if(iQoS && iQoS->GetContext()) namebuf.Append(iQoS->GetContext()->GetName());
	else namebuf.Append(KLogNull);
	namebuf.Append(KLogName);
	if(iQoS) namebuf.Append(iQoS->GetName());
	else namebuf.Append(KLogDele);
	iSimulator->Log(aMsg, namebuf);
    }

#ifdef USSE_DEBUG_CHECKS
void CPacketQoSApiWrapper::DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession)
    #else
    void CPacketQoSApiWrapper::DebugCheck(CUmtsSimulator*, CUmtsSimServSession*)
    #endif
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(aSession != iSession)
		User::Invariant();
	if(iQoS)
		iQoS->DebugCheck(iSimulator);
    #endif
    }
