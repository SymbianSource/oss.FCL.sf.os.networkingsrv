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

#include "usse_packet.h"
#include "usse_simulator.h"
#include "usse_packet_wrapper.h"
#include "log.h"

/* **************************
    * CPacketContext
    * ************************** */

CPacketContext* CPacketContext::NewL(CUmtsSimulator* aSimulator)
    {
	CPacketContext* self = new (ELeave) CPacketContext(aSimulator);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketContext::CPacketContext(CUmtsSimulator* aSimulator)
	: iRefCount(0), iStatus(RPacketContext::EStatusInactive), iConfigGPRS(NULL), iSimulator(aSimulator),
          iActivation2ndPhase(EFalse)
    {
	for(TInt i = 0; i < 8; i++)
		iPacketFilters[i] = NULL;
    }

void CPacketContext::ConstructL()
    {
	// nothing to do
    }

CPacketContext::~CPacketContext()
    {
	if(iActivation2ndPhase)
        {
		iSimulator->GetSimTimer()->RemoveRequest(iActivation2ndPhaseReq);
		iActivation2ndPhase = EFalse;
        }

	if(iQoSNegotiated != NULL || iQoSPending != NULL)
		User::Invariant(); // should not exist anymore

	if(iConfigGPRS)
		delete iConfigGPRS;

	for(TInt i = 0; i < 8; i++)
		if(iPacketFilters[i])
			delete iPacketFilters[i];
    }

TInt CPacketContext::CopyFrom(const CPacketContext& aContext)
    {
	if(iConfigGPRS)
        {
		delete iConfigGPRS;
		iConfigGPRS = NULL;
        }
	if(aContext.iConfigGPRS)
        {
		TRAPD(err, iConfigGPRS = new RPacketContext::TContextConfigGPRS(*aContext.iConfigGPRS));
		if(err != KErrNone || !iConfigGPRS)
			return KErrNoMemory;
        }

	return KErrNone;
    }

void CPacketContext::SetName(const TDesC& aName)
    {
	iName = aName;
    }

const TDesC& CPacketContext::GetName() const
    {
	return iName;
    }

TUint CPacketContext::RequestContextInitialisation(void (*aCallback)(TInt,const TDesC8&,TAny*), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextInitialiseContextA, simStatus, simDelay);

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, 
	      CPacketContext::RequestContextInitialisationReady, 
	      param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestContextInitialisationCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestContextInitialisationReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt,const TDesC8&,TAny*) = (void(*)(TInt,const TDesC8&,TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		_LIT(KDeprecated, "(deprecated)");
		_LIT(KChannel, "Channel");
		RPacketContext::TDataChannelV2 channel;
		channel.iChannelId = KChannel;
		channel.iChannelId.Append(self->GetName());
		channel.iCsy = KDeprecated;
		channel.iPort = KDeprecated;
		RPacketContext::TDataChannelV2Pckg channelPckg(channel);
		callback(KErrNone, channelPckg, param[2]);
		delete [] param;
        }
	else
		User::Invariant();
    }

TUint CPacketContext::RequestDeletion(void (*aCallback)(TAny* aParam), TAny* aParam)
    {
	iStatus = RPacketContext::EStatusUnknown;
	InformStatusChange();

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(200000, 
	      aCallback, aParam));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret; 
    }

void CPacketContext::RequestDeletionCancel(TUint aRequest)
    {
	iSimulator->GetSimTimer()->RemoveRequest(aRequest); // no params to be deleted
    }

void CPacketContext::FinalizeDeletion(void)
    {
	iStatus = RPacketContext::EStatusDeleted;
	InformStatusChange();
    }

TUint CPacketContext::RequestConfigurationL(TPacketDataConfigBase* aConfig,
										    void (*aCallback)(TInt aStatus, TAny* aParam),
										    TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextSetConfigA, simStatus, simDelay);

	TAny** param = new TAny*[6];
	param[0] = (TAny*) aCallback;
	param[1] = aParam;
	param[2] = this;
	param[4] = (TAny*) (TInt) requestOk;
	param[5] = (TAny*) simStatus;

	switch(aConfig->ExtensionId())
        {
            case TPacketDataConfigBase::KConfigGPRS:
                param[3] = new RPacketContext::TContextConfigGPRS(*(RPacketContext::TContextConfigGPRS*)aConfig);
                break;
            default:
            {
			_LIT(KLogUnSup, "WARNING: Context::RequestConfigurationL() found an unsupported extension id");
			iSimulator->Log(KLogUnSup);
			delete param;
			User::Leave(KErrNotSupported);
            }
        }

	return iSimulator->GetSimTimer()->RequestCallAfterL(simDelay*1000,
                                                        CPacketContext::RequestConfigurationReady, param);
    }

void CPacketContext::RequestConfigurationCancel(TUint aRequest)
    {
	TAny** param = (TAny**) iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	switch(((TPacketDataConfigBase*)param[3])->ExtensionId())
        {
            case TPacketDataConfigBase::KConfigGPRS:
                delete (RPacketContext::TContextConfigGPRS*) param[3];
                break;
            default:
                User::Invariant();
        }
	delete [] param;
    }

void CPacketContext::RequestConfigurationReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	void (*callback)(TInt, TAny*) = (void(*)(TInt,TAny*)) param[0];
	CPacketContext* self = (CPacketContext*) param[2];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[4];
	TInt simStatus = (TInt) param[5];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		switch(((TPacketDataConfigBase*)param[3])->ExtensionId())
            {
                case TPacketDataConfigBase::KConfigGPRS:
                {
				if(self->iConfigGPRS)
					delete self->iConfigGPRS;
				self->iConfigGPRS = (RPacketContext::TContextConfigGPRS*) param[3];
                }
                break;
                default:
                    User::Invariant();
            }
		callback(KErrNone, param[1]);
		delete [] param;

		self->InformConfigChange();
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
        {
		delete (RPacketContext::TContextConfigGPRS*) param[3];
		callback(simStatus, param[1]);
		delete [] param;
        }
	else
		User::Invariant();
    }

TUint CPacketContext::RequestGettingConfig(void (*aCallback)(TInt aStatus, const TDesC8& aConfig, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextGetConfigA, simStatus, simDelay);

	TAny** param = new TAny*[5];
	param[0] = (TAny*) aCallback;
	param[1] = aParam;
	param[2] = this;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, CPacketContext::RequestGettingConfigReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestGettingConfigCancel(TUint aRequest)
    {
	TAny** param = (TAny**) iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestGettingConfigReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	void (*callback)(TInt, const TDesC8&, TAny*) = (void(*)(TInt, const TDesC8&, TAny*)) param[0];
	CPacketContext* self = (CPacketContext*) param[2];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(self->iConfigGPRS)
            {
			TPckgC<RPacketContext::TContextConfigGPRS> configPckg(*self->iConfigGPRS);
			callback(KErrNone, configPckg, param[1]);
            }
        }
	else
		User::Invariant();

	delete [] param;
    }

TUint CPacketContext::RequestActivation(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextActivateA, simStatus, simDelay);

	iStatus = RPacketContext::EStatusActivating;
	InformStatusChange();

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, CPacketContext::RequestActivationReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestActivationCancel(TUint aRequest)
    {
	iStatus = RPacketContext::EStatusInactive;
	InformStatusChange();

	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestActivationReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt,TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];
	TInt simStatus = (TInt) param[4];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		// we should also check context configuration
		TInt err = self->NegotiateQoS();
		if(err != KErrNone)
            {
			callback(err, param[2]);
			delete [] param;

			self->iStatus = RPacketContext::EStatusUnknown;
			self->InformStatusChange();
            }
		else
            {
			self->InformProfileChange();

			callback(KErrNone, param[2]);
			delete [] param;

			if(self->iActivation2ndPhase)
                {
				_LIT(KLog2ndPh, "WARNING: Context::RequestActivationReady() found already active 2ndPhase");
				self->iSimulator->Log(KLog2ndPh);
				// User::Invariant();
				return;
                }

			TRAPD(err, self->iActivation2ndPhaseReq = 
			      self->iSimulator->GetSimTimer()->RequestCallAfterL(2000000, 
			      CPacketContext::RequestActivation2ndPhaseReady, self));
            if (err != KErrNone)
                {
                LOG(Log::Printf(_L("self->iSimulator->GetSimTimer()->\
RequestCallAfterL error: %d"), err));
                }
			self->iActivation2ndPhase = ETrue;
            }
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
        {
		callback(simStatus, param[2]);
		delete [] param;

		self->iStatus = RPacketContext::EStatusUnknown;
		self->InformStatusChange();
        }
	else
		User::Invariant();
    }

void CPacketContext::RequestActivation2ndPhaseReady(TAny* aParam)
    {
	CPacketContext* self = (CPacketContext*) aParam;

	self->iActivation2ndPhase = EFalse;
	self->iStatus = RPacketContext::EStatusActive;
	self->InformStatusChange();

	_LIT(KLogCom2, "RPacketContext::Activate() * 2nd phase completed, now active");
	_LIT(KLogName, "Context = ");
	TBuf<65> namebuf;
	namebuf = KLogName;
	namebuf.Append(self->iName);
	self->iSimulator->Log(KLogCom2, namebuf);
    }

TUint CPacketContext::RequestDeactivation(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextDeactivateA, simStatus, simDelay);

	iStatus = RPacketContext::EStatusDeactivating;
	InformStatusChange();

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, CPacketContext::RequestDeactivationReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;	
    }

void CPacketContext::RequestDeactivationCancel(TUint aRequest)
    {
	iStatus = RPacketContext::EStatusUnknown;
	InformStatusChange();

	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestDeactivationReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt,TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];
	TInt simStatus = (TInt) param[4];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(self->iActivation2ndPhase)
            {
			self->iSimulator->GetSimTimer()->RemoveRequest(self->iActivation2ndPhaseReq);
			self->iActivation2ndPhase = EFalse;
            }

		self->Deactivate();

		callback(KErrNone, param[2]);
		delete [] param;
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
        {
		self->DropNegotiatedQoS(); // this might not be necessary, gives clearer result though

		self->iStatus = RPacketContext::EStatusUnknown;
		self->InformStatusChange();

		callback(simStatus, param[2]);
		delete [] param;
        }
	else
		User::Invariant();
    }

void CPacketContext::Deactivate(void)
    {
	if(iStatus != RPacketContext::EStatusInactive)
        {
		DropNegotiatedQoS();
		iStatus = RPacketContext::EStatusInactive;
		InformStatusChange();
        }
    }

TInt CPacketContext::Suspend(void)
    {
	if(iStatus == RPacketContext::EStatusSuspended)
        {
		return KErrNone; // KErrAlreadyExists?
        }
	else if(iStatus != RPacketContext::EStatusActive)
        {
		return KErrGeneral;
        }

	iStatus = RPacketContext::EStatusSuspended;
	InformStatusChange();

	return KErrNone;
    }

TInt CPacketContext::Resume(void)
    {
	if(iStatus == RPacketContext::EStatusActive)
        {
		return KErrNone; // KErrAlreadyExists?
        }
	else if(iStatus != RPacketContext::EStatusSuspended)
        {
		return KErrGeneral;
        }

	iStatus = RPacketContext::EStatusActive;
	InformStatusChange();

	return KErrNone;
    }

TUint CPacketContext::RequestGettingStatus(void (*aCallback)(TInt aStatus, RPacketContext::TContextStatus aCStatus, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	
	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketContext::RequestGettingStatusReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestGettingStatusCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestGettingStatusReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, RPacketContext::TContextStatus, TAny*) = (void(*)(TInt,RPacketContext::TContextStatus,TAny*)) param[1];

	callback(KErrNone, self->iStatus, param[2]);
	delete [] param;
    }

TUint CPacketContext::RequestEnumeratingPacketFilters(void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	
	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketContext::RequestEnumeratingPacketFiltersReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestEnumeratingPacketFiltersCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestEnumeratingPacketFiltersReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TInt, TAny*) = (void(*)(TInt, TInt, TAny*)) param[1];

	TInt filters = 0;
	for(TInt i = 0; i < 8; i++)
		if(self->iPacketFilters[i]) filters++;

	callback(KErrNone, filters, param[2]);
	delete [] param;
    }

TUint CPacketContext::RequestGettingPacketFilterInfo(TInt aIndex, void (*aCallback)(TInt aStatus, const TDesC8& aInfo, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[4];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) aIndex;
	
	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketContext::RequestGettingPacketFilterInfoReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestGettingPacketFilterInfoCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestGettingPacketFilterInfoReady(TAny* aParam)
    {
	TInt status = KErrNone;
	RPacketContext::TPacketFilterV2* filter = NULL;

	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, const TDesC8&, TAny*) = (void(*)(TInt, const TDesC8&, TAny*)) param[1];
	TInt index = (TInt) param[3];

	if(index < 0)
		status = KErrArgument;
	else
        {
		for(TInt i = 0; i < 8; i++)
            {
			if(self->iPacketFilters[i]) --index;
			if(index == -1)
                {
				filter = self->iPacketFilters[i];
				break;
                }
            }
		if(index != -1)
			status = KErrArgument;
        }

    RPacketContext::TPacketFilterV2Pckg filterPckg(*filter);
    callback(status, filterPckg, param[2]);
	delete [] param;
    }

TUint CPacketContext::RequestAddingPacketFilter(RPacketContext::TPacketFilterV2* aInfo, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextAddPacketFilterA, simStatus, simDelay);

	TAny** param = new TAny*[6];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = new RPacketContext::TPacketFilterV2(*aInfo);
	param[4] = (TAny*) (TInt) requestOk;
	param[5] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketContext::RequestAddingPacketFilterReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestAddingPacketFilterCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	RPacketContext::TPacketFilterV2* filter = (RPacketContext::TPacketFilterV2*)((TAny**)param)[3];
	delete filter;
	delete [] param;
    }

void CPacketContext::RequestAddingPacketFilterReady(TAny* aParam)
    {
	TInt status = KErrNone;

	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt, TAny*)) param[1];
	RPacketContext::TPacketFilterV2* filter = (RPacketContext::TPacketFilterV2*) param[3];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[4];
	TInt simStatus = (TInt) param[5];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(filter->iId < 1 || filter->iId > 8)
            {
			delete filter;
			status = KErrArgument;
            }
		else
            {
			if(self->iPacketFilters[filter->iId - 1])
				delete self->iPacketFilters[filter->iId - 1];
			self->iPacketFilters[filter->iId - 1] = filter;
            }
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
        {
		delete filter;
		status = simStatus;
        }
	else
		User::Invariant();

	callback(status, param[2]);
	delete [] param;
    }

TUint CPacketContext::RequestRemovingPacketFilter(TInt aId, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextRemovePacketFilterA, simStatus, simDelay);

	TAny** param = new TAny*[6];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) aId;
	param[4] = (TAny*) (TInt) requestOk;
	param[5] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
	      CPacketContext::RequestRemovingPacketFilterReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestRemovingPacketFilterCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestRemovingPacketFilterReady(TAny* aParam)
    {
	TInt status = KErrNone;
	
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt, TAny*)) param[1];
	TInt id = (TInt) param[3];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[4];
	TInt simStatus = (TInt) param[5];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		if(id < 1 || id > 8)
			status = KErrArgument;
		else
            {
			RPacketContext::TPacketFilterV2* filter = self->iPacketFilters[id-1];
			if(filter)
                {
				delete filter;
				self->iPacketFilters[id-1] = NULL;
                }
			else
				status = KErrNotFound;
            }
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
        {
		status = simStatus;
        }
	else
		User::Invariant();

	callback(status, param[2]);
	delete [] param;
    }

TUint CPacketContext::RequestModifyingActiveContext(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketContextModifyActiveContextA, simStatus, simDelay);

	TAny** param = new TAny*[5];
	param[0] = this;
	param[1] = (TAny*) aCallback;
	param[2] = aParam;
	param[3] = (TAny*) (TInt) requestOk;
	param[4] = (TAny*) simStatus;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(
	      simDelay*1000, 
	      CPacketContext::RequestModifyingActiveContextReady, 
	      param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketContext::RequestModifyingActiveContextCancel(TUint aRequest)
    {
	TAny* param = iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketContext::RequestModifyingActiveContextReady(TAny* aParam)
    {
	TAny** param = (TAny**) aParam;
	CPacketContext* self = (CPacketContext*) param[0];
	void (*callback)(TInt, TAny*) = (void(*)(TInt, TAny*)) param[1];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[3];
	TInt simStatus = (TInt) param[4];

	if(requestOk == CUmtsSimulator::EURStatusNormal)
        {
		// we should also check context configuration
		TInt err = self->NegotiateQoS();
		if(err != KErrNone)
			callback(err, param[2]);
		else
            {
			self->InformProfileChange();
			callback(KErrNone, param[2]);
            }
        }
	else if(requestOk == CUmtsSimulator::EURStatusFail)
		callback(simStatus, param[2]);
	else
		User::Invariant();

	delete [] param;
    }

void CPacketContext::InformStatusChange()
    {
	// we want to inform all wrappers interested in this context
	TSglQueIter<CUmtsSimServSession>& iterator = iSimulator->AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketContextWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// there is a bug in CObjectIx => we need a little hack here
			CPacketContextApiWrapper* ptr = (CPacketContextApiWrapper*)(*session->iPacketContextWrappers)[idx];
			if(ptr == NULL)
                {
				//iSimulator->Log(_L("ERROR: Context::InformStatusChange() found CObjectIx bug"));
				count++;
                }
			else
				ptr->SimStatusUpdated(this, iStatus);

			// ((CPacketContextApiWrapper*)(*session->iPacketContextWrappers)[idx])->SimStatusUpdated(this, iStatus);
            }
		session = iterator++;
        }
	iSimulator->FreeSessionIterator(iterator);

	iSimulator->ContextStateChanged();
    }

void CPacketContext::InformConfigChange()
    {
	TPckgC<RPacketContext::TContextConfigGPRS> configPckg(*iConfigGPRS);

	// we want to inform all wrappers interested in this context
	TSglQueIter<CUmtsSimServSession>& iterator = iSimulator->AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketContextWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// there is a bug in CObjectIx => we need a little hack here
			CPacketContextApiWrapper* ptr = (CPacketContextApiWrapper*)(*session->iPacketContextWrappers)[idx];
			if(ptr == NULL)
                {
				
				//iSimulator->Log(_L("ERROR: Context::InformConfigChange() found CObjectIx bug"));
				count++;
                }
			else
				ptr->SimConfigChanged(this, configPckg);

			// ((CPacketContextApiWrapper*)(*session->iPacketContextWrappers)[idx])->SimConfigChanged(this, configPckg);
            }
		session = iterator++;
        }
	iSimulator->FreeSessionIterator(iterator);
    }

void CPacketContext::InformProfileChange()
    {
	if(iQoSNegotiated == NULL)
		User::Invariant(); // maybe we could just return?

	// we want to inform all (qos) wrappers interested in this context
	TSglQueIter<CUmtsSimServSession>& iterator = iSimulator->AllocSessionIterator();
	iterator.SetToFirst();
	CUmtsSimServSession* session = iterator++;
	while(session)
        {
		TInt count = session->iPacketQoSWrappers->ActiveCount();
		for(TInt idx = 0; idx < count; idx++)
            {
			// there is a bug in CObjectIx => we need a little hack here
			CPacketQoSApiWrapper* ptr = (CPacketQoSApiWrapper*)(*session->iPacketQoSWrappers)[idx];
			if(ptr == NULL)
                {
				
				//iSimulator->Log(_L("ERROR: Context::InformProfileChanged() found CObjectIx bug"));
				count++;
                }
			else
				// let iQoSNegotiated offer its profile to wrapper (wrapper checks whether it is
				// interested in 'this' context or not => we just call this for every wrapper)
				if(iQoSNegotiated->NotifyProfileToWrapper(ptr) != KErrNone)
					User::Invariant(); // bit rude yeah
            }
		session = iterator++;
        }
	iSimulator->FreeSessionIterator(iterator);
    }

TInt CPacketContext::NegotiateQoS()
    {
	if(iQoSPending == NULL)
		return KErrNotFound;

	if(iQoSNegotiated == iQoSPending)
		User::Invariant(); // for now negotiated profile should be only used as neg. prof.

	// creating new CPacketQoS is hack here to prevent changing negotiated profile
	// TODO: change this when GetProfileName is implemented!
	TBuf<64> qosname;
	CPacketQoS* newqos;
	TInt err = iSimulator->NewPacketQoS(GetName(), qosname, newqos);
	if(err != KErrNone)
		return err;

	// we should check qos here, for now just accept given profile
	err = newqos->SetToCopy(*iQoSPending);
	if(err != KErrNone)
        {
		if(iSimulator->RemovePacketQoS(GetName(), qosname) != KErrNone)
			User::Invariant();
		return err;
        }

	DropNegotiatedQoS();
	iQoSNegotiated = newqos;

	return KErrNone;
    }

void CPacketContext::SetPendingQoS(CPacketQoS* aQoS)
    {
	if(aQoS == iQoSPending) return;

	DropPendingQoS();
	if(iSimulator->GetPacketQoS(GetName(), aQoS->GetName(), iQoSPending, ETrue) != KErrNone)
		User::Invariant(); // should not fail
    }

void CPacketContext::DropNegotiatedQoS()
    {
	if(iQoSNegotiated)
        {
		if(iSimulator->RemovePacketQoS(GetName(), iQoSNegotiated->GetName()) != KErrNone)
			User::Invariant();
		iQoSNegotiated = NULL;
        }
    }

void CPacketContext::DropPendingQoS()
    {
	if(iQoSPending)
        {
		if(iSimulator->RemovePacketQoS(GetName(), iQoSPending->GetName()) != KErrNone)
			User::Invariant();
		iQoSPending = NULL;
        }
    }

TInt CPacketContext::GetNegotiatedQoS(CPacketQoS*& aNegotiatedQoS)
    {
	aNegotiatedQoS = iQoSNegotiated;
	if(iQoSNegotiated != NULL)
		return KErrNone;
	return KErrNotFound;
    }

void CPacketContext::DebugCheck(CUmtsSimulator*
                                #ifdef USSE_DEBUG_CHECKS
								aSimulator
                                #endif
								)
    {
    #ifdef USSE_DEBUG_CHECKS
	if(iSimulator != aSimulator)
		User::Invariant();

	CPacketContext* ptr;
	if(iSimulator->GetPacketContext(iName, ptr, EFalse) != KErrNone || ptr != this)
		User::Invariant();
    #endif
    }
