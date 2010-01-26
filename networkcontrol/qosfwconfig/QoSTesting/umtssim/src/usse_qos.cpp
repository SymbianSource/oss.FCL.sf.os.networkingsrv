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

#include "usse_qos.h"
#include "usse_qos_wrapper.h"
#include "usse_simulator.h"
#include "us_cliserv.h"
#include "log.h"

/* **************************
    * CPacketQoS
    * ************************** */

CPacketQoS* CPacketQoS::NewL(CUmtsSimulator* aSimulator, CPacketContext* aContext)
    {
	CPacketQoS* self = new (ELeave) CPacketQoS(aSimulator, aContext);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketQoS::CPacketQoS(CUmtsSimulator* aSimulator, CPacketContext* aContext)
	: iRefCount(0), iSimulator(aSimulator), iContext(aContext), iParamsUMTS(NULL)
    {
	// nothing to do
    }

void CPacketQoS::ConstructL()
    {
	// nothing to do
    }

CPacketQoS::~CPacketQoS()
    {
	if(iParamsUMTS)
		delete iParamsUMTS;
    }

TInt CPacketQoS::SetToDefault()
    {
	if(iParamsUMTS)
		delete iParamsUMTS;

	iParamsUMTS = new RPacketQoS::TQoSR5Negotiated(); // could fail?
	iParamsUMTS->iTrafficClass = RPacketQoS::ETrafficClassUnspecified;
	iParamsUMTS->iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderUnspecified;
	iParamsUMTS->iDeliverErroneousSDU = RPacketQoS::EErroneousSDUDeliveryUnspecified;
	iParamsUMTS->iMaxSDUSize = 0;
	iParamsUMTS->iMaxRate.iDownlinkRate = 0;
	iParamsUMTS->iMaxRate.iUplinkRate = 0;
	iParamsUMTS->iBER = RPacketQoS::EBERUnspecified;
	iParamsUMTS->iSDUErrorRatio = RPacketQoS::ESDUErrorRatioUnspecified;
	iParamsUMTS->iTrafficHandlingPriority = RPacketQoS::ETrafficPriorityUnspecified;
	iParamsUMTS->iTransferDelay = 0;
	iParamsUMTS->iGuaranteedRate.iDownlinkRate = 0;
	iParamsUMTS->iGuaranteedRate.iUplinkRate = 0;
	iParamsUMTS->iSignallingIndication = EFalse;
	iParamsUMTS->iSourceStatisticsDescriptor = RPacketQoS::ESourceStatisticsDescriptorUnknown;

	return KErrNone;
    }

TInt CPacketQoS::SetToCopy(const CPacketQoS& aQoS)
    {
	if(iParamsUMTS)
		delete iParamsUMTS;
	iParamsUMTS = new RPacketQoS::TQoSR5Negotiated(*aQoS.iParamsUMTS);
	return KErrNone;
    }

void CPacketQoS::SetName(const TDesC& aName)
    {
	iName = aName;
    }

const TDesC& CPacketQoS::GetName() const
    {
	return iName;
    }

CPacketContext* CPacketQoS::GetContext() const
    {
	return iContext;
    }

TUint CPacketQoS::RequestSettingProfileParametersL(TPacketDataConfigBase* aParameters,
												   void (*aCallback)(TInt aStatus, TAny* aParam),
												   TAny* aParam)
    {
	TInt simStatus = 0;
	TInt simDelay = 0;
	CUmtsSimulator::TUmtsRequestStatus requestOk =
		iSimulator->CheckRequest(EUmtsSimServPacketQoSSetProfileParametersA, simStatus, simDelay);

	TAny** param = new TAny*[6];
	param[0] = (TAny*) aCallback;
	param[1] = aParam;
	param[2] = this;
	param[4] = (TAny*) (TInt) requestOk;
	param[5] = (TAny*) simStatus;

	switch(aParameters->ExtensionId())
        {
            //case TPacketDataConfigBase::KConfigRel99Rel4:
            case TPacketDataConfigBase::KConfigRel5:
                param[3] = new RPacketQoS::TQoSR5Requested(*(RPacketQoS::TQoSR5Requested*)aParameters);
                break;
            default:
            {
			_LIT(KLogUnSup, "WARNING: QoS::RequestSettingProfileParametersL() found an unsupported extension id");
			iSimulator->Log(KLogUnSup);
			delete param;
			User::Leave(KErrNotSupported);
            }
        }

	return iSimulator->GetSimTimer()->RequestCallAfterL(100000, CPacketQoS::RequestSettingProfileParametersReady, param);
    }

void CPacketQoS::RequestSettingProfileParametersCancel(TUint aRequest)
    {
	TAny** param = (TAny**) iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	switch(((TPacketDataConfigBase*)param[3])->ExtensionId())
        {
            //case TPacketDataConfigBase::KConfigRel99Rel4:
            case TPacketDataConfigBase::KConfigRel5:
                delete (RPacketQoS::TQoSR5Requested*) param[3];
                break;
            default:
                User::Invariant();
        }
	delete [] param;
    }

void CPacketQoS::RequestSettingProfileParametersReady(TAny* aParam)
    {
	TInt status = KErrNone;

	TAny** param = (TAny**) aParam;
	void (*callback)(TInt, TAny*) = (void(*)(TInt,TAny*)) param[0];
	CPacketQoS* self = (CPacketQoS*) param[2];
	CUmtsSimulator::TUmtsRequestStatus requestOk = (CUmtsSimulator::TUmtsRequestStatus) (TInt) param[4];
	TInt simStatus = (TInt) param[5];

	switch(((TPacketDataConfigBase*)param[3])->ExtensionId())
        {
            //case TPacketDataConfigBase::KConfigRel99Rel4:
            case TPacketDataConfigBase::KConfigRel5:
            {
			RPacketQoS::TQoSR5Requested* requested = (RPacketQoS::TQoSR5Requested*) param[3];

			if(requestOk == CUmtsSimulator::EURStatusNormal)
                {
				self->DoSetProfileParameters(requested);
				self->iContext->SetPendingQoS(self);
                }
			else if(requestOk == CUmtsSimulator::EURStatusFail)
                {
				status = simStatus;
                }
			else
				User::Invariant();

			delete requested;
            }
            break;
            default:
                User::Invariant();
        }

	callback(status, param[1]);
	delete [] param;
    }

TUint CPacketQoS::RequestGettingProfileParameters(void (*aCallback)(TInt aStatus, const TDesC8& aParameters, TAny* aParam), TAny* aParam)
    {
	TAny** param = new TAny*[3];
	param[0] = (TAny*) aCallback;
	param[1] = aParam;
	param[2] = this;

	TUint ret = 0;
	TRAPD(err, ret = iSimulator->GetSimTimer()->RequestCallAfterL(100000, 
          CPacketQoS::RequestGettingProfileParametersReady, param));
    if (err != KErrNone)
        {
        LOG(Log::Printf(_L("iSimulator->GetSimTimer()->RequestCallAfterL \
error: %d"), err));
        }
	return ret;
    }

void CPacketQoS::RequestGettingProfileParametersCancel(TUint aRequest)
    {
	TAny** param = (TAny**) iSimulator->GetSimTimer()->RemoveRequest(aRequest);
	delete [] param;
    }

void CPacketQoS::RequestGettingProfileParametersReady(TAny* aParam)
    {
	TInt status = KErrNone;

	TAny** param = (TAny**) aParam;
	void (*callback)(TInt, const TDesC8&, TAny*) = (void(*)(TInt, const TDesC8&, TAny*)) param[0];
	CPacketQoS* self = (CPacketQoS*) param[2];

	CPacketQoS* neg = NULL;
	status = self->iContext->GetNegotiatedQoS(neg);
	if(status == KErrNone)
        {
		if(neg->iParamsUMTS)
            {
			TPckgC<RPacketQoS::TQoSR5Negotiated> paramsPckg(*neg->iParamsUMTS);
			callback(status, paramsPckg, param[1]);
            }
		else
			status = KErrNotReady;
        }
	delete [] param;
    }

void CPacketQoS::DoSetProfileParameters(RPacketQoS::TQoSR5Requested* aRequested)
    {
	if(iParamsUMTS)
		delete iParamsUMTS;
	iParamsUMTS = new RPacketQoS::TQoSR5Negotiated();
	iParamsUMTS->iTrafficClass = aRequested->iReqTrafficClass;
	iParamsUMTS->iDeliveryOrderReqd = aRequested->iReqDeliveryOrderReqd;
	iParamsUMTS->iDeliverErroneousSDU = aRequested->iReqDeliverErroneousSDU;
	iParamsUMTS->iMaxSDUSize = aRequested->iReqMaxSDUSize;
	iParamsUMTS->iMaxRate = aRequested->iReqMaxRate;
	iParamsUMTS->iBER = aRequested->iReqBER;
	iParamsUMTS->iSDUErrorRatio = aRequested->iReqSDUErrorRatio;
	iParamsUMTS->iTrafficHandlingPriority = aRequested->iReqTrafficHandlingPriority;
	iParamsUMTS->iTransferDelay = aRequested->iReqTransferDelay;
	iParamsUMTS->iGuaranteedRate = aRequested->iReqGuaranteedRate;
	iParamsUMTS->iSignallingIndication = aRequested->iSignallingIndication;
	iParamsUMTS->iSourceStatisticsDescriptor = aRequested->iSourceStatisticsDescriptor;
    }

TInt CPacketQoS::NotifyProfileToWrapper(CPacketQoSApiWrapper* aWrapper)
    {
	if(iParamsUMTS == NULL)
		return KErrNotFound;

	TPckgC<RPacketQoS::TQoSR5Negotiated> paramsPckg(*iParamsUMTS);
	aWrapper->SimProfileChanged(iContext, paramsPckg);
	return KErrNone;
    }


void CPacketQoS::DebugCheck(CUmtsSimulator*
                            #ifdef USSE_DEBUG_CHECKS
							aSimulator
                            #endif
							)
    {
    #ifdef USSE_DEBUG_CHECKS
	if(aSimulator != iSimulator)
		User::Invariant();
	if(iContext)
		iContext->DebugCheck(iSimulator);
    #endif
    }
