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
//

#include "UmtsNifControlIf.h"
#include "umtsnif.h"
#include "NifContext.h"
#include "ContextNotifications.h"

#include "log-r6.h"


CNifContext* CNifContext::NewPrimaryL(CUmtsNif* aNif,TContextId aNifContextId) 
    {
	CNifContext* cx = new (ELeave)CNifContext();
	CleanupStack::PushL(cx);
	cx->InitL(aNif,aNifContextId);	
	CleanupStack::Pop(1);
	
	return cx;
    }

CNifContext* CNifContext::NewSecondaryL(CUmtsNif* aNif,TContextId aNifContextId,TDes& aExistingName) 
    {
	CNifContext* cx = new (ELeave)CNifContext();
	CleanupStack::PushL(cx);
	cx->InitL(aNif,aNifContextId,aExistingName);
	CleanupStack::Pop(1);
	return cx;
    }

CNifContext::CNifContext() : iContextHandle(),iContextQoS()
    {
	//iContextConfigListener = NULL;
	iContextStatusListener = NULL;
	iQoSProfileChangeListener = NULL;

	iCreated = EFalse;
	iManualQosRequest = EFalse;

	ResetPendingQoS();	
    }

void CNifContext::InitL(CUmtsNif* aNif,TContextId aNifContextId) 
    {
	iContextParameters = new (ELeave)TContextParameters();
	
	TInt ret = 0;
	Parameters().iContextInfo.iStatus = RPacketContext::EStatusUnknown;
	Parameters().iContextType = EPrimaryContext;

	iContextId = aNifContextId;
	iNif = aNif;
	iContextHandle = new (ELeave) RPacketContext();
	iContextQoS = new (ELeave) RPacketQoS();	
	
	
	// TSY-Context Handle
	ret = iContextHandle->OpenNewContext(iNif->Controller()->PacketService(),iProxyId);
	if(ret != KErrNone) 
        {
		LOG(Log::Printf(_L("CNifContext::InitL: OpenNewContext() failed with code <%d>"),ret));
		iContextHandle->Close();
		User::LeaveIfError(ret);
        }
	
	iContextName = iProxyId;
	
	iUsable = ETrue; // The context exists in Etel and Nif
	
	// TSY-QoS Handle
	TBuf<65> ProfileName;
	ret = iContextQoS->OpenNewQoS(*iContextHandle,ProfileName);
	if(ret != KErrNone) 
        {
		LOG(Log::Printf(_L("CNifContext::InitL: OpenNewContext() failed with code <%d>"),ret));
		iContextHandle->Close();
		iContextQoS->Close();
		User::LeaveIfError(ret);
        }
	iRequest = CEtelRequest::NewL();			
	iRequest->ConstructL(this);

	// Create Packet Interface 
	iPacketIf = iNif->Controller()->iPEPManager->NewPacketInterfaceL(this,iProxyId);	

	StartListenersL();
    }

void CNifContext::InitL(CUmtsNif* aNif,TContextId aNifContextId,TDes& aExistingName) 
    {
	iContextParameters = new (ELeave)TContextParameters();

	Parameters().iContextInfo.iStatus = RPacketContext::EStatusUnknown;
	Parameters().iContextType = ESecondaryContext;
	
	iContextId = aNifContextId;
	iNif = aNif;
	iContextHandle = new (ELeave) RPacketContext();
	iContextQoS = new (ELeave) RPacketQoS();	
	
	
	User::LeaveIfError(iContextHandle->OpenNewSecondaryContext(iNif->Controller()->PacketService(), aExistingName, iProxyId));
	iContextName = iProxyId;

	TBuf<65> ProfileName;
	User::LeaveIfError(iContextQoS->OpenNewQoS(*iContextHandle,ProfileName));

	iUsable = ETrue; // The context exists in Etel and Nif

	// Etel request object
	iRequest = CEtelRequest::NewL();		
	iRequest->ConstructL(this);
	
	// Packet Interface from PEP
	// ** Here we should check if this succeeds..**
	iPacketIf = iNif->Controller()->iPEPManager->NewPacketInterfaceL(this,iProxyId);
	// 

	StartListenersL();
    }

TInt CNifContext::StartListenersL()
    {
	// iCreated = ETrue; // Moved to after the context is notified activation to GUQoS so we won't send unrelevant events
	// Not needed..
    /*	
        iContextConfigListener = new (ELeave)CEtelContextConfigChanged();
        iContextConfigListener->ConstructL(this);
        iContextConfigListener->Start();
        */
	
	iContextStatusListener = new (ELeave)CEtelContextStatusChanged();
	iContextStatusListener->ConstructL(this);
	iContextStatusListener->Start();

	iQoSProfileChangeListener = new (ELeave)CEtelContextQoSChanged();
	iQoSProfileChangeListener->ConstructL(this);
	iQoSProfileChangeListener->Start();

	return KErrNone;
    }

CNifContext::~CNifContext()
    {
	LOG(Log::Printf(_L("<%s>[Context %d] CNifContext::~CNifContext start"),iNif->Name().PtrZ(),iContextId));
	delete iContextParameters;
	//delete iContextConfigListener;	// Not needed..
	delete iContextStatusListener;	
	delete iQoSProfileChangeListener;
	
	delete iRequest;	
	iContextQoS->Close();
	iContextHandle->Close();
	delete iContextQoS;
	delete iContextHandle;
	delete iPacketIf;
	LOG(Log::Printf(_L("<%s>[Context %d] CNifContext::~CNifContext finish"),iNif->Name().PtrZ(),iContextId));
    }

//TContextReasonCode CNifContext::IssueRequest(TContextParameters *aParameters,TInt8 aOperation)
TInt CNifContext::IssueRequest(TContextParameters *aParameters,TInt8 aOperation)
    {
	//TContextReasonCode result = KContextOk;
	TInt result = KErrNone;
	
	// A boolean variable to mark if the setqos request is from user (or network?)	
	if ( aOperation == (TInt8)KContextQoSSet )
		iManualQosRequest = ETrue;
	else
		iManualQosRequest = EFalse;

	TRAPD(ret,result = iRequest->IssueRequestL(aParameters,aOperation));
	if(ret != KErrNone)
		return ret;
	
	return result;
    }

/*void CNifContext::AddToQueue(RMBufChain &aPacket)
    {
	iPacketIf->AddToQueue(aPacket);
    }*/
void CNifContext::Send(RMBufChain &aPacket)
    {
	iPacketIf->Send(aPacket);
    }


//
// Unblock and block-indications for packetinterface
//
void CNifContext::UnblockIndication()
    {
	TContextInfo info;
	// TRequestStatus status; // for local use
	//TContextReasonCode operationCode = KContextOk;
	TInt operationCode = KErrNone;

	info.iStatus = Status();
	info.iContextId = ContextId();
    
	// Build response event
    //	TContextParameters UnblockEvent;
	
	TContextParameters* UnblockEvent = NULL;
	TRAPD(err, UnblockEvent = new (ELeave) TContextParameters());
	if (err)
		return;

	UnblockEvent->iReasonCode = operationCode;
	UnblockEvent->iContextInfo = info;
	UnblockEvent->iContextType = ContextType();

	TPckg<TContextParameters> event(*UnblockEvent);
		
	// Event built
	// Send unblocked event to GUQoS
	if(Nif().EventsOn() && Created() && Usable())
        {		
		LOG(Log::Printf(_L("<%s> Sending unblocked event for [Context %d]"),Nif().Name().PtrZ(),UnblockEvent->iContextInfo.iContextId));	
		Nif().EventHandler().Event((CProtocolBase*)&Nif(),KContextUnblockedEvent,event);	// Notify the upper layer
        }
	// Without signalling module or events of, call StartSending() 
	else if (Created() && Usable())
        {
		Nif().Network()->StartSending((CProtocolBase*)&Nif());
        }
	delete UnblockEvent;
	UnblockEvent=NULL;
    }


//
// PEP-modules call this when the used pipe blocks
//
void CNifContext::BlockIndication()
    {
	TContextInfo info;
	// TRequestStatus status; // for local use
	//TContextReasonCode operationCode = KContextOk;
	TInt operationCode = KErrNone;

	info.iStatus = Status();
	info.iContextId = ContextId();
    
	// Build response event
    //	TContextParameters BlockEvent;
	TContextParameters* BlockEvent = NULL;
	TRAPD(err, BlockEvent = new (ELeave) TContextParameters());
	if (err)
		return;
	
	BlockEvent->iReasonCode = operationCode;
	BlockEvent->iContextInfo = info;
	BlockEvent->iContextType = ContextType();

	TPckg<TContextParameters> event(*BlockEvent);
		
	// Event built
	// Send Blocked event to GUQoS
	if(Nif().EventsOn() && Created() && Usable())
        {		
		LOG(Log::Printf(_L("<%s> Sending blocked event for [Context %d]"),Nif().Name().PtrZ(),BlockEvent->iContextInfo.iContextId));		
		Nif().EventHandler().Event((CProtocolBase*)&Nif(),KContextBlockedEvent,event);	// Notify the upper layer
        }
	// The stack will receive the block info via Send() -method's return value
	// when sending packets.
	delete BlockEvent;
	BlockEvent=NULL;
    }

//
// Packet interface will call this when it has received a packet
void CNifContext::ProcessPacket(RMBufChain& aPacket)
    {
	Nif().ReceivePacket(aPacket);
    }

TBool CNifContext::Usable()
    {
	return iUsable;
    }

//
// Indicate QoS has changed
// 
void CNifContext::ContextInternalEvent(RPacketQoS::TQoSR5Negotiated aContextQoS)
    {
	// Next we should check if the context is still up and running
	if(Nif().EventsOn() && Created() && Usable()) // Change the created to indicate active in GuQoS
        {		
		if(!PendingModification())
            {
			// If we had a pending QoS it is now invalid anyway..
			ResetPendingQoS();
			
			RPacketQoS::TQoSR5Negotiated currentQoS;
			Parameters().iContextConfig.GetUMTSQoSNeg(currentQoS);
			if(!IdenticalQoS(aContextQoS,currentQoS))
                {
				Parameters().iContextConfig.SetUMTSQoSNeg(aContextQoS);
				
                //				TContextParameters contextQoSChangeEvent;	
				TContextParameters* contextQoSChangeEvent = NULL;
				TRAPD(err, contextQoSChangeEvent = new (ELeave) TContextParameters());
				if (err)
					return;

				FillEvent(*contextQoSChangeEvent);		// Standard info		
				FillEventQoS(*contextQoSChangeEvent);	// QoS	
				FillEventTFT(*contextQoSChangeEvent);	// TFT
				
				TPckg<TContextParameters> event(*contextQoSChangeEvent);
				
				// A boolean variable to mark if the setqos request is from user (or network?)	
				// if this is from the user do not generate any event but if it is by the network
				// then generate the event
				if ( iManualQosRequest )
                    {
					LOG(Log::Printf(_L("<%s> Sending KContextParametersChangeEvent event for [Context %d]"),Nif().Name().PtrZ(),contextQoSChangeEvent->iContextInfo.iContextId));	
					Nif().EventHandler().Event((CProtocolBase*)&(Nif()),KContextParametersChangeEvent,event);			
					iManualQosRequest = EFalse;
                    }
				delete contextQoSChangeEvent;
				contextQoSChangeEvent=NULL;
                }
            }
		else
            {			
			SetPendingQoS(aContextQoS);											
            }
        }

	return;
    }

//
// 
// 
void CNifContext::ContextInternalEvent(RPacketContext::TContextStatus aContextStatus)
    {
	// Next we should check if the context is still up and running
	if(Usable())
        {		
		// Network initiated deletion of the context
		if(	aContextStatus == RPacketContext::EStatusInactive)
            {
			NetworkInitiatedDeletion();
            }
		else if(aContextStatus == RPacketContext::EStatusDeleted)
            {
			//NetworkInitiatedDeletion();
            }
		// The context is suspended or starting to go down..
		else if(aContextStatus == RPacketContext::EStatusSuspended	||	
				aContextStatus == RPacketContext::EStatusDeactivating) 
            {			
			PacketIf().Block();	// Block it
            }
        }
	return;
    }

void CNifContext::NetworkInitiatedDeletion()
    {
	Nif().ContextManager()->Delete(ContextId());	// Initiate deletion of this context
	
	if(Nif().EventsOn())	// Send delete indication and start deleting the context
        {
        //		TContextParameters contextDeletedEvent;	
		TContextParameters* contextDeletedEvent = NULL;
		TRAPD(err, contextDeletedEvent = new (ELeave) TContextParameters());
		if (err)
			return;

		FillEvent(*contextDeletedEvent);		// Standard info
		
		TPckg<TContextParameters> event(*contextDeletedEvent);
		
		LOG(Log::Printf(_L("<%s> Sending KContextDeleteEvent event for [Context %d]"),Nif().Name().PtrZ(),contextDeletedEvent->iContextInfo.iContextId));	
		Nif().EventHandler().Event((CProtocolBase*)&(Nif()),KContextDeleteEvent,event);	
		delete contextDeletedEvent;
		contextDeletedEvent=NULL;
        }
    }


void CNifContext::FillEvent(TContextParameters &aEvent)
    {
	TContextInfo info;
	
	aEvent.iTFTOperationCode = 0;

	RPacketContext::TContextStatus iTempStatus;
	ContextHandle().GetStatus(iTempStatus);

	info.iStatus = iTempStatus;
	info.iContextId = ContextId();			// Context Id
	aEvent.iContextType = ContextType();	// Context type
	aEvent.iReasonCode = KErrNone;
	aEvent.iContextInfo = info;

	// Ask the contextmanager for the common config
	RPacketContext::TContextConfigGPRS tempConfig;
	Nif().ContextManager()->GetCommonConfig(tempConfig);
	aEvent.iContextConfig.SetContextConfig(tempConfig);	// Set context config
    }

void CNifContext::FillEventQoS(TContextParameters &aEvent)
    {
	RPacketQoS::TQoSR5Negotiated QoS;
	Parameters().iContextConfig.GetUMTSQoSNeg(QoS);
	aEvent.iContextConfig.SetUMTSQoSNeg(QoS);
    }
void CNifContext::FillEventTFT(TContextParameters &aEvent)
    {
	TTFTInfo TFT;
	Parameters().iContextConfig.GetTFTInfo(TFT);
	aEvent.iContextConfig.SetTFTInfo(TFT);
    }

//
// Returns true if there is a pending modification on this context
//
TBool CNifContext::PendingModification()
    {
	if(	iRequest->Free() &&
       iRequest->Operation() == (TInt8) KContextModifyActive)
		return ETrue;
	else
		return EFalse;
    }
//
// Return true if there is a changed QoS-block for this context. The block is copied 
// into the parameter reference
//
TBool CNifContext::GetPendingQoS(RPacketQoS::TQoSR5Negotiated& aQoS)
    {
	if(iQoSPending)
		aQoS = iPendingQoS;
	return iQoSPending;
    }
//
// Resets the (possible) pending QoS-block
//
void CNifContext::ResetPendingQoS()
    {
	iQoSPending = EFalse;
    }
//
// Sets the pending QoS-block
// This will be used if 
//
void CNifContext::SetPendingQoS(RPacketQoS::TQoSR5Negotiated aQoS)
    {
	iQoSPending = ETrue;
	iPendingQoS = aQoS;
    }
// IMCN flag    
TInt CNifContext::SetIMCNSubsystemflag(TBool aIMCNSubsystemflag)
{
	iIMCNSubsystemflag = aIMCNSubsystemflag;
	return KErrNone;
}

TInt CNifContext::GetIMCNSubsystemflag(TBool &aIMCNSubsystemflag) const
{
	aIMCNSubsystemflag = iIMCNSubsystemflag;
	return KErrNone;
}

