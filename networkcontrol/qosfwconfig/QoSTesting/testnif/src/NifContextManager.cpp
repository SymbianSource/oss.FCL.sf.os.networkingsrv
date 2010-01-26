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

#include "umtsnif.h"

#include "log-r6.h"

CNifContextManager::CNifContextManager()
    {}

CNifContextManager::~CNifContextManager()
    {
	LOG(Log::Printf(_L("CNifContextManager::~CNifContextManager : Nif context manager going down\n")));
	for(TInt cid = 0; cid < (TInt)KMaximumNumberOfContextsPerAPN; cid++) 
        {
		delete iContextDelete[cid];
		if(iContextTable[cid])
			ReleaseContext((TContextId)cid);
        //delete iContextTable[cid];
        }
    }

void CNifContextManager::ConstructL(CUmtsNif *aNif)
    {
	iNif = aNif;
	iFirst = TRUE;
	iNumberOfContexts = 0;
	iLastCid = 0;
	
	for(TInt cid = 0; cid < (TInt)KMaximumNumberOfContextsPerAPN; cid++) 
        {
		iContextTable[cid] = NULL;
		iContextDelete[cid] = NULL;
        }
		
    }


TInt CNifContextManager::CreateContext(TContextId& aId)
    {
	//TContextReasonCode OperationStatus;
	TInt OperationStatus;

	TInt8 cid = 0;
	
	if(	!iNif->Controller()->RequestNewContextResource() ||
       iNumberOfContexts == KMaximumNumberOfContextsPerAPN) 
        {
		LOG(Log::Printf(_L("Open Secondary Context Failed: Maximum number of contexts open\n")));
		OperationStatus = KErrNotSupported;
        }
	else 
        {
		if(iFirst) 
            {
			//for(cid = 0; cid < KMaximumNumberOfContextsPerAPN && iContextTable[cid] != NULL; cid++); // Find the first free slot

			cid = 0; // Always for primary
			iLastCid = cid;

			TRAPD(ret,iContextTable[cid] = CNifContext::NewPrimaryL(iNif,cid));
			if(ret != KErrNone)
                {
				LOG(Log::Printf(_L("CNifContextManager::CreateContext : Open Primary Context Failed: <%d>\n"),ret));		
				delete iContextTable[cid];
				iContextTable[cid] = NULL;	
				
				iNif->Controller()->ReleaseContextResource();
				
				return ret;
                }

            }
		else 
            {
			cid = 0;
			// Loop to first slot taken to get an existing name in TSY we can use to create a new secondary below
			while(cid < (TInt)KMaximumNumberOfContextsPerAPN)
                {
				if(	iContextTable[cid] != NULL &&
                   iContextTable[cid]->Usable())
					break;
				cid++;
                }			
				
			TBuf<65> existingName;
			
			__ASSERT_DEBUG(iContextTable[cid], User::Panic(_L("No context"), 0));
			
			existingName = iContextTable[cid]->ContextName();
			
			cid = iLastCid;
			TInt8 dbgCheck = 0;
			while(dbgCheck < (TInt)KMaximumNumberOfContextsPerAPN)
                {
				if(cid == (TInt8)KMaximumNumberOfContextsPerAPN) // Looped over
					cid = 0;
				if(iContextTable[cid] == NULL)	// Found an empty slot
					break;
				cid++;		// Neeext..
				dbgCheck++;	// Checks that we won't loop until inifinity...				
                }
			
			iLastCid = cid;
						
			__ASSERT_DEBUG(cid < (TInt8)KMaximumNumberOfContextsPerAPN , User::Panic(_L("No room for context"), 0));

			TRAPD(ret,iContextTable[cid] = CNifContext::NewSecondaryL(iNif,cid,existingName));
			if(ret != KErrNone)
                {
				LOG(Log::Printf(_L("CNifContextManager::CreateContext : Open Secondary Context Failed: <%d>\n"),ret));
				
				iNif->Controller()->ReleaseContextResource();

				return ret;
                }
            }	
		iNumberOfContexts++;
		iFirst = FALSE;
		aId = cid;
		return KErrNone;
        }
	return (TInt)OperationStatus;
    }

// Handle to contexts for the Nif
CNifContext* CNifContextManager::Context(TUint8 aContextId)
    {
	ASSERT(iContextTable[aContextId]);
	return iContextTable[aContextId];
    }

//
// Number of contexts under this manager
//
TUint8 CNifContextManager::ContextCount()
    {
	return iNumberOfContexts;
    }

//
// Delete a context with given id
//
TInt CNifContextManager::Delete(TUint8 aContextId)
    {
	if(!iContextTable[aContextId]) 
		return KErrNotFound;

	ASSERT(iContextTable[aContextId]);

	if(iContextTable[aContextId]->Usable())	// Do not delete if already going down..
        {
		iContextTable[aContextId]->iRequest->Cancel();	// Do cancel before deletion
        // GUQoS is not interested in getting events on the context

		iContextTable[aContextId]->iUsable = EFalse;	// Mark so that this will be ignored from now on.
		
		iContextDelete[aContextId] = new (ELeave) CEtelContextDelete();
		TRAPD(err, iContextDelete[aContextId]->ConstructL(Context(aContextId),this));
        if (err != KErrNone)
            {
            LOG(Log::Printf(_L("iContextDelete[aContextId]->ConstructL error:\
 %d"), err));
            }
		iContextDelete[aContextId]->Start();
        }
	
	return KErrNone;
    }


//
// All the contexts in one UmtsNif => CNifContextManager share the config
//
void CNifContextManager::SetCommonConfig(RPacketContext::TContextConfigGPRS& aContextConfig)
    { 	
	iContextConfig = aContextConfig; 
    };
TInt CNifContextManager::GetCommonConfig(RPacketContext::TContextConfigGPRS& aContextConfig)
    { 
	aContextConfig = iContextConfig;
	return KErrNone;
    };

//
// Internal, used by CEtelRequest, when handling asynchronous context deletion's final step
// Destroys all resources used by the contexts
//
TInt CNifContextManager::ReleaseContext(TInt8 aContextId)
    {
	LOG(Log::Printf(_L("<%s>[Context %d] Context deleted\n"),iNif->Name().PtrZ(),aContextId));
		
	iContextDelete[aContextId] = NULL;
	delete iContextTable[aContextId];
	iContextTable[aContextId] = NULL;
		
	iNif->Controller()->ReleaseContextResource(); // Common for whole UMTSNif
			
	iNumberOfContexts--;	// Internal to this manager
		
	// Deletion of last context causes the Nif to go down
	if(ContextCount() == 0) 
        {
		iNif->LastContextDown(); // Indicate the Nif
        }
	return KErrNone;
    }

void CEtelContextDelete::RunL()
    {
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextDelete::RunL() START\n")));
#endif
	if(iStatus != KErrNone) // In case something fails, just kill the context
        {		
		iContextManager->ReleaseContext(iContext->ContextId());	
		delete this;	
        }
	else
        {	
		switch(iRequestState)
            {	
                case EDeleteState :
                {
                iContext->ContextHandle().Delete(iStatus);
                iNotifierCode = EPacketContextDelete;
                iRequestState = EReleaseState;
                SetActive();
                }
                break;
                case EReleaseState :
                {
                iContextManager->ReleaseContext(iContext->ContextId());	
                delete this;
                }
                break;
                default :
                    ASSERT(0 != 0);	// Replace with unexpected panic indication.
                    break;
            }
        }

#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextDelete::RunL() STOP\n")));
#endif
	return;
    }


CEtelContextDelete::CEtelContextDelete() : CActive(EPriorityStandard)
    {
	iCancellable = ETrue;
    }

CEtelContextDelete::~CEtelContextDelete()
    {	
	if(IsActive())
		Cancel();	
    }
void CEtelContextDelete::DoCancel()
    {
	if(iCancellable)
		iContext->ContextHandle().CancelAsyncRequest(iNotifierCode);
    }

void CEtelContextDelete::ConstructL(CNifContext *aContext,CNifContextManager *aContextManager)
    {
	CActiveScheduler::Add(this); 	
	iContext = aContext;
	iContextManager = aContextManager;
    }

void CEtelContextDelete::Start()
    {		
	// Active -> deactivate first
	if(iContext->Usable())
        {
		if(iContext->Status() == RPacketContext::EStatusActive ||
		   iContext->Status() == RPacketContext::EStatusSuspended)
            {
			iNotifierCode = EPacketContextDeactivate;
			iRequestState = EDeleteState;
			iContext->ContextHandle().Deactivate(iStatus);
            }
		// Not active -> straight to delete
		else
            {
			iNotifierCode = EPacketContextDelete;
			iRequestState = EReleaseState;
			iContext->ContextHandle().Delete(iStatus);
            }
		SetActive(); 
        }
	else
        {
		iRequestState = EReleaseState;
		iCancellable = EFalse;
		JumpToRunl(0);
        }

	return;
    }

void CEtelContextDelete::JumpToRunl(TInt aError)
    {
	SetActive();
	TRequestStatus* statusPtr=&iStatus;
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextDelete::JumpToRunl() Signalling RunL\n")));
#endif
	User::RequestComplete(statusPtr,aError);
    }
			
void CNifContextManager::SendPrimaryContextEvent()
    {
	CNifContext *context = Context(0); // Always use primary which is found from location 0.
    //	TContextParameters primaryContextCreatedEvent;
	TContextParameters* primaryContextCreatedEvent = NULL;
	TRAPD(err, primaryContextCreatedEvent = new (ELeave) TContextParameters());

	if (err)
		{
		LOG(Log::Printf(_L("CNifContextManager::SendPrimaryContextEvent() out of memory for TContextParameters\n")));
		return;
		}

	RPacketContext::TContextStatus iTempStatus;
	context->ContextHandle().GetStatus(iTempStatus);

	TContextInfo info;

	info.iStatus = iTempStatus;
	info.iContextId = context->ContextId();		// Context Id
	primaryContextCreatedEvent->iContextType = context->ContextType();	// Context type
	primaryContextCreatedEvent->iReasonCode = KErrNone;//iResponse.iReasonCode;		// Operation fail / success
	primaryContextCreatedEvent->iContextInfo = info;

	// Ask the contextmanager for the common config

	RPacketContext::TContextConfigGPRS* tempConfig = new (ELeave) RPacketContext::TContextConfigGPRS;

	__ASSERT_ALWAYS(tempConfig != NULL,User::Panic(_L("CNifContextManager::SendPrimaryContextEvent()"),0));
	
	if (tempConfig == NULL)
		{
		delete primaryContextCreatedEvent;
		LOG(Log::Printf(_L("CNifContextManager::SendPrimaryContextEvent() out of memory for TContextConfigGPRS\n")));
		return;
		}

	context->Nif().ContextManager()->GetCommonConfig(*tempConfig);
	primaryContextCreatedEvent->iContextConfig.SetContextConfig(*tempConfig);	// Set context config


	RPacketQoS::TQoSR5Negotiated QoS;
	context->Parameters().iContextConfig.GetUMTSQoSNeg(QoS);
	primaryContextCreatedEvent->iContextConfig.SetUMTSQoSNeg(QoS);


	TPckg<TContextParameters> event(*primaryContextCreatedEvent);
	
	if(context->Nif().EventsOn() && context->Usable())
        {
		context->Nif().EventHandler().Event((CProtocolBase*)&(context->Nif()),KPrimaryContextCreated,event);	// Notify the upper layer
        }
	delete primaryContextCreatedEvent;
	primaryContextCreatedEvent=NULL;

	delete tempConfig;
    }				
