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
// usse_packetservice.cpp - implementation of server side correspondent for RPacketService
// (note that there is only one CPacketService, but there is one (or even more) RPacketService per client)
//

#include "usse_packet.h"
#include "usse_simulator.h"
#include "usse_server.h"
#include "usse_packet_wrapper.h"
#include "log.h"

/* **************************
    * CPacketService
    * ************************** */

CPacketService* CPacketService::NewL(CUmtsSimulator* aSimulator)
    {
	CPacketService* self = new (ELeave) CPacketService(aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketService::CPacketService(CUmtsSimulator* aSimulator)
	: iStatus(RPacketService::EStatusUnattached), iSimulator(aSimulator)
    {
	// nothing to do
    }

void CPacketService::ConstructL()
    {
	// nothing to do
    }

CPacketService::~CPacketService()
    {
	// nothing to do
    }

TUint CPacketService::RequestAttachment(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simDelay = 0;
	TInt simStatus = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketServiceAttachA, simStatus, simDelay);

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, CPacketService::RequestAttachmentReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret; 
    }

void CPacketService::RequestAttachmentCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestAttachmentReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt,TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];
	TInt simStatus = (TInt) param[4];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(self->iStatus != RPacketService::EStatusAttached)
            {
			self->iStatus = RPacketService::EStatusAttached;
			self->InformStatusChange();

			callback(KErrNone, param[2]);
            }
		else
			callback(KErrAlreadyExists, param[2]);
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
		callback(simStatus, param[2]);
	else
		User::Invariant();

	delete [] param;
    }

TUint CPacketService::RequestDetachment(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simDelay = 0;
	TInt simStatus = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketServiceDetachA, simStatus, simDelay);

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, CPacketService::RequestDetachmentReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret; 
    }

void CPacketService::RequestDetachmentCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestDetachmentReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt, TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];
	TInt simStatus = (TInt) param[4];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(self->iStatus != RPacketService::EStatusUnattached)
            {
			self->iStatus = RPacketService::EStatusUnattached;
			self->InformStatusChange();

			callback(KErrNone, param[2]);
            }
		else
			callback(KErrAlreadyExists, param[2]);
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
		callback(simStatus, param[2]);
	else
		User::Invariant();

	delete [] param;
    }

TUint CPacketService::RequestGettingStatus(void (*aCallback)(TInt aStatus, RPacketService::TStatus aSStatus, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestGettingStatusReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestGettingStatusCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestGettingStatusReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, RPacketService::TStatus, TAny*) = (void(*)(TInt,RPacketService::TStatus,TAny*)) param[1];

	callback(KErrNone, self->iStatus, param[2]);
	delete [] param;
    }

TUint CPacketService::RequestEnumeratingContexts(void (*aCallback)(TInt, TInt, TInt, TAny*), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestEnumeratingContextsReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestEnumeratingContextsCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestEnumeratingContextsReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, TInt, TInt, TAny*) = (void(*)(TInt, TInt, TInt, TAny*)) param[1];

	TSglQueIter<CPacketContext>& iterator = self->iSimulator->AllocContextIterator();
	TInt count = 0;
	iterator.SetToFirst();
	CPacketContext* context = iterator++;
	while(context)
        {
		count++;
		context = iterator++;
        }
	self->iSimulator->FreeContextIterator(iterator);

	callback(KErrNone, count, KUMTSSimMaxContexts, param[2]);
	delete [] param;
    }

TUint CPacketService::RequestGettingContextInfo(TInt aIndex, void (*aCallback)(TInt, const TDesC*, RPacketContext::TContextStatus, TAny*), TAny* aParam)
    {
	TAny** param = new TAny*[4];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) aIndex;
	
	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestGettingContextInfoReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestGettingContextInfoCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestGettingContextInfoReady(TAny *aParam)
    {
	TInt status = KErrNone;
	CPacketContext* context = NULL;

	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, const TDesC*, RPacketContext::TContextStatus, TAny*) = 
		(void(*)(TInt, const TDesC*, RPacketContext::TContextStatus, TAny*)) param[1];
	TInt index = (TInt) param[3];

	if(index < 0)
		status = KErrArgument;
	else
        {
		TSglQueIter<CPacketContext>& iterator = self->iSimulator->AllocContextIterator();
		iterator.SetToFirst();
		do
            {
			context = iterator++;
			if(!context)
                {
				status = KErrArgument;
				break;
                }
            } while(index--);
		self->iSimulator->FreeContextIterator(iterator);
        }

	if(status != KErrNone)
		callback(status, NULL, RPacketContext::EStatusUnknown, param[2]);
	else
		callback(status, &(context->GetName()), context->GetStatus(), param[2]);

	delete [] param;
    }

TUint CPacketService::RequestEnumeratingNifs(void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	
	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestEnumeratingNifsReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestEnumeratingNifsCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestEnumeratingNifsReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, TInt, TAny*) = (void(*)(TInt, TInt, TAny*)) param[1];

	callback(KErrNone, self->iSimulator->GetNifList().GetNifCount(), param[2]);
	delete [] param;
    }

TUint CPacketService::RequestGettingNifInfo(TInt aIndex, void (*aCallback)(TInt aStatus, const TDesC8* aInfo, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[4];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) aIndex;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestGettingNifInfoReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestGettingNifInfoCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketService::RequestGettingNifInfoReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, const TDesC8*, TAny*) = (void(*)(TInt, const TDesC8*, TAny*)) param[1];
	TInt index = (TInt) param[3];

	RPacketService::TNifInfoV2 info;
	TInt status = self->iSimulator->GetNifList().GetNifInfo(index, info);
	if(status != KErrNone)
		callback(status, NULL, param[2]);
	else
        {
		RPacketService::TNifInfoV2Pckg infoPckg(info);
		callback(status, &infoPckg, param[2]);
        }
	delete [] param;
    }

TUint CPacketService::RequestEnumeratingContextsInNif(const TDesC* aContextName,
													  void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam),
													  TAny* aParam)
    {
	TAny** param = new TAny*[4];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	HBufC* name = NULL; 
	TRAPD(err, name = HBufC::NewL(aContextName->Length()));

	_LIT(KLogEventErr, "ERROR: Allocation of HBufC failed");
	if(err != KErrNone)
        {
		iSimulator->Log(KLogEventErr);
        }




	
	*name = *aContextName;
	param[3] = name;
	
	TUint ret = 0;
	TRAPD(err2, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestEnumeratingContextsInNifReady, param));
    if (err2 != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err2));
        }
	return ret;
    }

void CPacketService::RequestEnumeratingContextsInNifCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	HBufC* name = (HBufC*)((TAny**)param)[3];
	delete name;
	delete [] param;
    }

void CPacketService::RequestEnumeratingContextsInNifReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, TInt, TAny*) = (void(*)(TInt, TInt, TAny*)) param[1];
	HBufC* cname = (HBufC*) param[3];

	CPacketContext* context;
	TInt status = self->iSimulator->GetPacketContext(*cname, context, EFalse);
	if(status != KErrNone)
		callback(status, -1, param[2]);
	else
		callback(status, self->iSimulator->GetNifList().GetContextCount(context), param[2]);
	delete cname;
	delete [] param;
    }

TUint CPacketService::RequestGettingContextNameInNif(TInt aIndex, const TDesC* aContextName,
													 void (*aCallback)(TInt aStatus, const TDesC* aName, TAny* aParam),
													 TAny* aParam)
    {
	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) aIndex;
	HBufC* name = NULL; 
	TRAPD(trap_err, name = HBufC::NewL(aContextName->Length()));
    if (trap_err != KErrNone)
        {
        LOG(Log::Printf(_L("HBufC::NewL error: %d"), trap_err));
        }
	*name = *aContextName;
	param[4] = name;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketService::RequestGettingContextNameInNifReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketService::RequestGettingContextNameInNifCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	HBufC* name = (HBufC*)((TAny**)param)[4];
	delete name;
	delete [] param;
    }

void CPacketService::RequestGettingContextNameInNifReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketService* self = (CPacketService*) param[0];
	void (*callback)(TInt, const TDesC*, TAny*) = (void(*)(TInt, const TDesC*, TAny*)) param[1];
	TInt index = (TInt) param[3];
	HBufC* cname = (HBufC*) param[4];

	CPacketContext* context;
	TInt status = self->iSimulator->GetPacketContext(*cname, context, EFalse);
	if(status != KErrNone)
		callback(status, NULL, param[2]);
	else
        {
		TBuf<65> name;
		status = self->iSimulator->GetNifList().GetContextName(index, name, context);
		if(status != KErrNone)
			callback(status, NULL, param[2]);
		else
			callback(status, &name, param[2]);
        }
	delete cname;
	delete [] param;
    }

void CPacketService::PossibleStatusUpdate(RPacketService::TStatus aStatus)
    {
	if(iStatus != aStatus)
        {
		iStatus = aStatus;
		InformStatusChange();
        }
    }

void CPacketService::InformStatusChange()
    {
	TSglQueIter<CUmtsSimServSession>& iterator = iSimulator->AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketServiceWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// because there is a bug in CObjectIx we need a hack here
			CPacketServiceApiWrapper* ptr = (CPacketServiceApiWrapper*)(*session->iPacketServiceWrappers)[idx];
			if(ptr == NULL)
                {
				//_LIT(KLogBug, "ERROR: Service::InformConfigChanged() found CObjectIx bug");
				//iSimulator->Log(KLogBug);
				count++;
                }
			else
				ptr->SimStatusUpdated(iStatus);

			// ((CPacketServiceApiWrapper*)(*session->iPacketServiceWrappers)[idx])->SimStatusUpdated(iStatus);
            }
		session = iterator++;
        }
	iSimulator->FreeSessionIterator(iterator);
    }

void CPacketService::InformContextAdded(const TDesC& aId)
    {
	TSglQueIter<CUmtsSimServSession>& iterator = iSimulator->AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketServiceWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// because there is a bug in CObjectIx we need a hack here
			CPacketServiceApiWrapper* ptr = (CPacketServiceApiWrapper*)(*session->iPacketServiceWrappers)[idx];
			if(ptr == NULL)
                {
				//_LIT(KLogBug, "ERROR: Service::InformContextAdded() found CObjectIx bug");
				//iSimulator->Log(KLogBug);
				count++;
                }
			else
				ptr->SimContextAdded(aId);

			// ((CPacketServiceApiWrapper*)(*session->iPacketServiceWrappers)[idx])->SimContextAdded(aId);
            }
		session = iterator++;
        }
	iSimulator->FreeSessionIterator(iterator);
    }

void CPacketService::DebugCheck(CUmtsSimulator*
                                #ifdef USSE_DEBUG_CHECKS
								aSimulator
                                #endif
								)
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(aSimulator->GetPacketService() != this)
		User::Invariant();
    #endif
    }
