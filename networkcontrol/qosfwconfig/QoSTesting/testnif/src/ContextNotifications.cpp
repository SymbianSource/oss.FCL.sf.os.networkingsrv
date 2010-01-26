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

#include "NifContext.h"
#include "umtsnif.h"
#include "ContextNotifications.h"


#include "log-r6.h"

CEtelContextNotificationRequest::CEtelContextNotificationRequest() : CActive(EPriorityStandard + 1)
    {}

CEtelContextNotificationRequest::~CEtelContextNotificationRequest()
    {	
	if(IsActive())
		Cancel();
    }
void CEtelContextNotificationRequest::DoCancel()
    {
	iStatus=KErrNone;		
	if(iNotifierCode == EPacketQoSNotifyProfileChanged)
		iContext->ContextQoS().CancelAsyncRequest(iNotifierCode);
	else
		iContext->ContextHandle().CancelAsyncRequest(iNotifierCode);
    }

void CEtelContextNotificationRequest::ConstructL(CNifContext *aContext)
    {
	CActiveScheduler::Add(this); 	
	iContext = aContext;
    }


CEtelContextStatusChanged::CEtelContextStatusChanged() 
    {
	iNotifierCode = EPacketContextNotifyStatusChange;
    }
void CEtelContextStatusChanged::RunL()
    {		
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextStatusChanged::RunL() START\n")));
#endif
	
	iContext->SetStatus(iContextStatus);

	iContext->ContextInternalEvent(iContextStatus);
	
	iContext->ContextHandle().NotifyStatusChange(iStatus,iContextStatus);

	SetActive();
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextStatusChanged::RunL() STOP\n")));
#endif
	return;
    }

void CEtelContextStatusChanged::Start()
    {
	iContext->ContextHandle().NotifyStatusChange(iStatus,iContextStatus);
	SetActive();

	return;
    }
/*
    CEtelContextConfigChanged::CEtelContextConfigChanged() : iUMTSConfigPtr(iUMTSConfig)
    {
	iNotifierCode = EPacketContextNotifyConfigChanged;
    }
    void CEtelContextConfigChanged::RunL()
    {		
    #ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextConfigChanged::RunL() START\n")));
    #endif
	iContext->SetConfig(iUMTSConfig);
	// Should inform the context
	iContext->ContextHandle().NotifyConfigChanged(iStatus,iUMTSConfigPtr);
	SetActive();

    #ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextConfigChanged::RunL() STOP\n")));
    #endif
	return;
    }

    void CEtelContextConfigChanged::Start()
    {
	iContext->ContextHandle().NotifyConfigChanged(iStatus,iUMTSConfigPtr);
	SetActive();

	return;
    }
    */

CEtelContextQoSChanged::CEtelContextQoSChanged() : iUMTSQoSNegPtr(iUMTSQoSNeg)
    {
	iNotifierCode = EPacketQoSNotifyProfileChanged;
    }
void CEtelContextQoSChanged::RunL()
    {			
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextQoSChanged::RunL() START\n")));
#endif
	
	//iContext->Parameters().iContextConfig.SetUMTSQoSNeg(iUMTSQoSNeg);
	
	// Should inform context
	iContext->ContextInternalEvent(iUMTSQoSNeg);

	// Wait for next
	iContext->ContextQoS().NotifyProfileChanged(iStatus,iUMTSQoSNegPtr);
	SetActive();

#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelContextQoSChanged::RunL() STOP\n")));
#endif

	return;
    }

void CEtelContextQoSChanged::Start()
    {
	iContext->ContextQoS().NotifyProfileChanged(iStatus,iUMTSQoSNegPtr);
	SetActive();

	return;
    }


//
// Logger method
//
#ifdef LOG

void CEtelContextNotificationRequest::PrintEvent(TUint aEventCode, const TContextParameters& aParameters)
    {
	LOG(Log::Printf(_L("************** BEGIN NOTIFICATION EVENT ***************\n")));
	PrintEventType(aEventCode,aParameters);
	PrintContextInfo(aParameters.iContextInfo);
	PrintContextConfig(aParameters.iContextConfig);
	PrintContextNegotiatedQoS(aEventCode,aParameters);
	PrintContextTFT(aEventCode,aParameters);
	LOG(Log::Printf(_L("*************** END NOTIFICATION EVENT ****************\n")));	
    }

void CEtelContextNotificationRequest::PrintEventType(TUint aEventCode, const TContextParameters& aParameters)
    {
    TPtrC nif = iContext->Nif().Name();
	switch(aEventCode)
        {
        case KPrimaryContextCreated:
            {
            LOG(Log::Printf(_L("Sending event KPrimaryContextCreated from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextTFTModifiedEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextTFTModifiedEvent from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextQoSSetEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextQoSSetEvent from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextActivateEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextActivateEvent from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextParametersChangeEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextParametersChangeEvent from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        case KSecondaryContextCreated:
            {
            LOG(Log::Printf(_L("Sending event KSecondaryContextCreated from <Nif>/<Context> <%s>/<%d>"),
                &nif,
                aParameters.iContextInfo.iContextId));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("Sending event UNRECOGNIZED")));	
            break;
            }
        }
	// Following if-sentence added only to get rid of WINSCW UREL-warning: 
	// variable / argument 'aParameters' is not used in function
    if (aParameters.iContextInfo.iContextId < 0)
        {
        LOG(Log::Printf(_L("Negative ContextId")));
        }
    }

void CEtelContextNotificationRequest::PrintContextInfo(const TContextInfo& aContextInfo)
    {
	LOG(Log::Printf(_L("** Context status data ** \n")));
	switch(aContextInfo.iStatus)
        {
            case RPacketContext::EStatusUnknown :
                LOG(Log::Printf(_L("\tContext status: EStatusUnknown\n")));
                break;
            case RPacketContext::EStatusInactive :
                LOG(Log::Printf(_L("\tContext status: EStatusInactive\n")));
                break;
            case RPacketContext::EStatusActivating :
                LOG(Log::Printf(_L("\tContext status: EStatusActivating\n")));
                break;
            case RPacketContext::EStatusActive :
                LOG(Log::Printf(_L("\tContext status: EStatusActive\n")));
                break;
            case RPacketContext::EStatusDeactivating :
                LOG(Log::Printf(_L("\tContext status: EStatusDeactivating\n")));
                break;
            case RPacketContext::EStatusSuspended:
                LOG(Log::Printf(_L("\tContext status: EStatusSuspended\n")));
                break;
            case RPacketContext::EStatusDeleted	:
                LOG(Log::Printf(_L("\tContext status: EStatusDeleting\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tContext status: UNRECOGNIZED\n")));
                break;
        }
    }

void CEtelContextNotificationRequest::PrintContextConfig(
    const TContextConfig& aContextConfig)
    {
    LOG(Log::Printf(_L("** Context config data **")));

    RPacketContext::TContextConfigGPRS tempConfig;

    aContextConfig.GetContextConfig(tempConfig);

    RPacketContext::TProtocolAddress pdpAddress;
    aContextConfig.GetPdpAddress(pdpAddress);

    TInetAddr pdpAddr;
    pdpAddr.Copy(pdpAddress);

    TBuf<RPacketContext::KMaxPDPAddressLength> outBuf;
    pdpAddr.Output(outBuf);

    switch(tempConfig.iPdpType)
        {
        case RPacketContext::EPdpTypeIPv4:
            {
            LOG(Log::Printf(_L("\tPdpType: IPv4")));
            break;
            }
        case RPacketContext::EPdpTypeIPv6:
            {
            LOG(Log::Printf(_L("\tPdpType: IPv6")));
            break;
            }
        case RPacketContext::EPdpTypeX25:
            {
            LOG(Log::Printf(_L("\tPdpType: X25")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tPdpType: UNRECOGNIZED")));
            break;
            }
        }

    LOG(Log::Printf(_L8("\tAccess Point Name: <%s>"), 
        tempConfig.iAccessPointName.PtrZ()));
    LOG(Log::Printf(_L("\tPdpAddress: <%s>"), 
        outBuf.PtrZ()));

    if((tempConfig.iPdpCompression == RPacketContext::KPdpDataCompression) &&
       (tempConfig.iPdpCompression == RPacketContext::KPdpHeaderCompression))
        {
        LOG(Log::Printf(_L("\tCompression: Header+Data")));
        }
    else if((tempConfig.iPdpCompression == 
        RPacketContext::KPdpDataCompression))
        {
        LOG(Log::Printf(_L("\tCompression: Data")));
        }
    else if((tempConfig.iPdpCompression == 
        RPacketContext::KPdpHeaderCompression))
        {
        LOG(Log::Printf(_L("\tCompression: Header")));
        }
    else if(tempConfig.iPdpCompression == 0)
        {
        LOG(Log::Printf(_L("\tCompression: None")));
        }
    else
        {
        LOG(Log::Printf(_L("\tCompression: UNRECOGNIZED")));
        }

    switch(tempConfig.iAnonymousAccessReqd)
        {
        case RPacketContext::ENotApplicable:
            {
            LOG(Log::Printf(
                _L("\tAnonymous access required: ENotApplicable")));
            break;
            }
        case RPacketContext::ERequired:
            {
            LOG(Log::Printf(_L("\tAnonymous access required: ERequired")));
            break;
            }
        case RPacketContext::ENotRequired:
            {
            LOG(Log::Printf(
                _L("\tAnonymous access required: ENotRequired")));
            break;
            }
        default:
            {
            LOG(Log::Printf(
                _L("\tAnonymous access required: UNRECOGNIZED")));
            break;
            }
        }
    }

void CEtelContextNotificationRequest::PrintContextNegotiatedQoS(TUint /*aEventCode*/, const TContextParameters& aParameters)
    {
	LOG(Log::Printf(_L("** Context QoS data ** \n")));
	
	RPacketQoS::TQoSR5Negotiated* tempQoS=NULL;
	TRAPD(err, tempQoS = new (ELeave) RPacketQoS::TQoSR5Negotiated());
	if (err)
        {
		LOG(Log::Printf(_L("CEtelContextNotificationRequest::PrintContextNegotiatedQoS - tempQoS allocation failed\n")));
		return;
        }

	aParameters.iContextConfig.GetUMTSQoSNeg(*tempQoS);

	switch(tempQoS->iTrafficClass)
        {
            case RPacketQoS::ETrafficClassUnspecified :
                LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassUnspecified\n")));
                break;
            case RPacketQoS::ETrafficClassConversational :
                LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassConversational\n")));
                break;
            case RPacketQoS::ETrafficClassStreaming :
                LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassStreaming\n")));
                break;
            case RPacketQoS::ETrafficClassInteractive :
                LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassInteractive\n")));
                break;
            case RPacketQoS::ETrafficClassBackground :
                LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassBackground\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tTraffic Class: UNRECOGNIZED\n")));
                break;
        }
	switch(tempQoS->iDeliveryOrderReqd)
        {
            case RPacketQoS::EDeliveryOrderUnspecified :
                LOG(Log::Printf(_L("\tDelivery order: EDeliveryOrderUnspecified\n")));
                break;
            case RPacketQoS::EDeliveryOrderRequired :
                LOG(Log::Printf(_L("\tDelivery order: EDeliveryOrderRequired\n")));
                break;
            case RPacketQoS::EDeliveryOrderNotRequired :
                LOG(Log::Printf(_L("\tDelivery order: EDeliveryOrderNotRequired\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tDelivery order: UNRECOGNIZED\n")));
                break;
        }	
	switch(tempQoS->iDeliverErroneousSDU)
        {
            case RPacketQoS::EErroneousSDUDeliveryUnspecified :
                LOG(Log::Printf(_L("\tDelivery of erroneous: EErroneousSDUDeliveryUnspecified\n")));
                break;
            case RPacketQoS::EErroneousSDUDeliveryRequired :
                LOG(Log::Printf(_L("\tDelivery of erroneous: EErroneousSDUDeliveryRequired\n")));
                break;
            case RPacketQoS::EErroneousSDUDeliveryNotRequired :
                LOG(Log::Printf(_L("\tDelivery of erroneous: EErroneousSDUDeliveryNotRequired\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tDelivery of erroneous: UNRECOGNIZED\n")));
                break;
	
        }

	LOG(Log::Printf(_L("\tMaximum SDU size: %d"),tempQoS->iMaxSDUSize));
	LOG(Log::Printf(_L("\tMaximum data rate uplink: %d"),tempQoS->iMaxRate.iUplinkRate));
	LOG(Log::Printf(_L("\tMaximum data rate downlink: %d"),tempQoS->iMaxRate.iDownlinkRate));
	LOG(Log::Printf(_L("\tGuaranteed data rate uplink: %d"),tempQoS->iGuaranteedRate.iUplinkRate));
	LOG(Log::Printf(_L("\tGuaranteed data rate downlink: %d"),tempQoS->iGuaranteedRate.iDownlinkRate));
	
	
	switch(tempQoS->iBER)
        {
            case RPacketQoS::EBERUnspecified :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBERUnspecified\n")));
                break;
            case RPacketQoS::EBERFivePerHundred :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBERFivePerHundred\n")));
                break;
            case RPacketQoS::EBEROnePerHundred :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerHundred\n")));
                break;
            case RPacketQoS::EBERFivePerThousand :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBERFivePerThousand\n")));
                break;
            case RPacketQoS::EBERFourPerThousand :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBERFourPerThousand\n")));
                break;
            case RPacketQoS::EBEROnePerThousand :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerThousand\n")));
                break;
            case RPacketQoS::EBEROnePerTenThousand :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerTenThousand\n")));
                break;
            case RPacketQoS::EBEROnePerHundredThousand :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerHundredThousand\n")));
                break;
            case RPacketQoS::EBEROnePerMillion :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerMillion\n")));
                break;
            case RPacketQoS::EBERSixPerHundredMillion :
                LOG(Log::Printf(_L("\tBit Error Ratio: EBERSixPerHundredMillion\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tBit Error Ratio: UNRECOGNIZED\n")));
                break;
        }
	
	switch(tempQoS->iSDUErrorRatio)
        {
            case RPacketQoS::ESDUErrorRatioUnspecified :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioUnspecified \n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerTen :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerTen\n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerHundred :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerHundred\n")));
                break;
            case RPacketQoS::ESDUErrorRatioSevenPerThousand :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioSevenPerThousand\n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerThousand :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerThousand\n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerTenThousand :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerTenThousand\n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerHundredThousand :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerHundredThousand\n")));
                break;
            case RPacketQoS::ESDUErrorRatioOnePerMillion :
                LOG(Log::Printf(_L("\tSDU Error Ratio: ESDUErrorRatioOnePerMillion\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tSDU Error Ratio: UNRECOGNIZED\n")));
                break;
        }
	switch(tempQoS->iTrafficHandlingPriority)
        {
            case RPacketQoS::ETrafficPriorityUnspecified :
                LOG(Log::Printf(_L("\tTraffic handling priority: ETrafficPriorityUnspecified\n")));
                break;
            case RPacketQoS::ETrafficPriority1 :
                LOG(Log::Printf(_L("\tTraffic handling priority: ETrafficPriority1\n")));
                break;
            case RPacketQoS::ETrafficPriority2 :
                LOG(Log::Printf(_L("\tTraffic handling priority: ETrafficPriority2\n")));
                break;
            case RPacketQoS::ETrafficPriority3 :
                LOG(Log::Printf(_L("\tTraffic handling priority: ETrafficPriority3\n")));
                break;
            default :
                LOG(Log::Printf(_L("\tTraffic handling priority: UNRECOGNIZED\n")));
                break;
        }
	LOG(Log::Printf(_L("\tTransfer delay: %d"),tempQoS->iTransferDelay));
	
	delete tempQoS;
    }

void CEtelContextNotificationRequest::PrintContextTFT(TUint /*aEventCode*/, const TContextParameters& aParameters)	
    {
	//TTFTInfo tempTFT;
	TTFTInfo* tempTFT = NULL;
	TRAPD(err, tempTFT = new (ELeave) TTFTInfo());
	if (err)
		return;

	RPacketContext::TPacketFilterV2* tempFilter = NULL;
	TRAP(err, tempFilter = new (ELeave) RPacketContext::TPacketFilterV2());
	if(err)
		delete tempTFT;

	aParameters.iContextConfig.GetTFTInfo(*tempTFT);

	LOG(Log::Printf(_L("** Context TFT data ** \n")));
	
	LOG(Log::Printf(_L("\t Number of filters: %d \n"),tempTFT->FilterCount()));
	
	for(TUint Counter = 0; Counter < tempTFT->FilterCount(); Counter++)
        {
        //		RPacketContext::TPacketFilterV2 tempFilter;
		tempFilter->iId = Counter+1;
		
		tempTFT->GetPacketFilter(*tempFilter);

		LOG(Log::Printf(_L("\t\tFilter:\n")));
		LOG(Log::Printf(_L("\t\t\tId: <%d>\n"),tempFilter->iId));
		LOG(Log::Printf(_L("\t\t\tEvaluation precedence: <%d>\n"),tempFilter->iEvaluationPrecedenceIndex));
		
		LOG(Log::Printf(_L("\t\t\tIP:   <%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d>\n"),
                        tempFilter->iSrcAddr[0],
                        tempFilter->iSrcAddr[1],
                        tempFilter->iSrcAddr[2],
                        tempFilter->iSrcAddr[3],
                        tempFilter->iSrcAddr[4],
                        tempFilter->iSrcAddr[5],
                        tempFilter->iSrcAddr[6],
                        tempFilter->iSrcAddr[7],
                        tempFilter->iSrcAddr[8],
                        tempFilter->iSrcAddr[9],
                        tempFilter->iSrcAddr[10],
                        tempFilter->iSrcAddr[11],
                        tempFilter->iSrcAddr[12],
                        tempFilter->iSrcAddr[13],
                        tempFilter->iSrcAddr[14],
                        tempFilter->iSrcAddr[15]));
		LOG(Log::Printf(_L("\t\t\tMask: <%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d>\n"),
                        tempFilter->iSrcAddrSubnetMask[0],
                        tempFilter->iSrcAddrSubnetMask[1],
                        tempFilter->iSrcAddrSubnetMask[2],
                        tempFilter->iSrcAddrSubnetMask[3],
                        tempFilter->iSrcAddrSubnetMask[4],
                        tempFilter->iSrcAddrSubnetMask[5],
                        tempFilter->iSrcAddrSubnetMask[6],
                        tempFilter->iSrcAddrSubnetMask[7],
                        tempFilter->iSrcAddrSubnetMask[8],
                        tempFilter->iSrcAddrSubnetMask[9],
                        tempFilter->iSrcAddrSubnetMask[10],
                        tempFilter->iSrcAddrSubnetMask[11],
                        tempFilter->iSrcAddrSubnetMask[12],
                        tempFilter->iSrcAddrSubnetMask[13],
                        tempFilter->iSrcAddrSubnetMask[14],
                        tempFilter->iSrcAddrSubnetMask[15]));
		
		LOG(Log::Printf(_L("\t\t\tProtocol number/Next Header: <%d>\n"),tempFilter->iProtocolNumberOrNextHeader));
		LOG(Log::Printf(_L("\t\t\tSource port Min: <%d>\n"), tempFilter->iSrcPortMin));
		LOG(Log::Printf(_L("\t\t\tSource port Max: <%d>\n"), tempFilter->iSrcPortMax));
		
		LOG(Log::Printf(_L("\t\t\tDestination port Min: <%d>\n"), tempFilter->iDestPortMin));
		LOG(Log::Printf(_L("\t\t\tDestination port Max: <%d>\n"), tempFilter->iDestPortMax));
		
		LOG(Log::Printf(_L("\t\t\tSPI: <%d>\n"),tempFilter->iIPSecSPI));
		LOG(Log::Printf(_L("\t\t\tTOS: <%d>\n"),tempFilter->iTOSorTrafficClass));
		LOG(Log::Printf(_L("\t\t\tFlow label: <%d>\n"), tempFilter->iFlowLabel));
		
        }
	delete tempTFT;
	delete tempFilter;

    }
#endif
