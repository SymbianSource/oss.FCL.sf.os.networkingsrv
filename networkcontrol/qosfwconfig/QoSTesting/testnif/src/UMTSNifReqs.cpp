// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "UMTSNif.h"
#include "UMTSNifReqs.h"

#include <f32file.h>
#include <s32file.h>

#include "log-r6.h"

/*  
 *  CEtelRequest: Represents an asynchronous request to Etel 
 *  Uses CEtelSubRequest -objects to wrap multiple asynchronous 
 *  calls to Etel together.
 */
CEtelRequest *CEtelRequest::NewL()
    {
    CEtelRequest *req = new (ELeave)CEtelRequest();
    return req;
    }

CEtelRequest::CEtelRequest() : CActive(EPriorityStandard)
    {
    }

void CEtelRequest::ConstructL(CNifContext *aContext)
    {
    CActiveScheduler::Add(this); 
    iFree = TRUE;
    iHandlerState = KHandlerFetchParameters;
    
    iSubRequest = NULL;
    iContext = aContext;

    iResponse = new (ELeave) TContextParameters();
    }

CEtelRequest::~CEtelRequest()
    {
    delete iSubRequest;
    iSubRequest = NULL;
    delete iResponse;
    iResponse = NULL;
    if(IsActive())
        {
        Cancel();
        }
    }

TInt CEtelRequest::IssueRequestL(TContextParameters *aParameters, 
                                 TInt8 aOperation)
    {
    TInt8 ContextId = aParameters->iContextInfo.iContextId;

    switch(aOperation)
        {        
        case KStartupPrimaryContextCreation:
            {
            // Just in case signalling module GuQoS is impatient
            iFree = FALSE; 

            iOperation = aOperation;

            iResponse->iContextConfig.Set(aParameters->iContextConfig);
            // Start the operation wrapper 
            iSubRequest = new (ELeave)CEtelContextStartupCreate();
            iSubRequest->ConstructL(this);

            iResponse->iContextInfo.iContextId = ContextId;
            iSubRequest->DoTask(iResponse);
            // Started

            SetActive(); // Go to wait

            LOG(Log::Printf(
                _L("<%s>[Context %d] Nif starting a new primary context"),
                iContext->Nif().Name().PtrZ(), 
                ContextId));

            break;
            }
        case KContextCreate:
            {
            if(!iFree)
                {
                __ASSERT_DEBUG(0, User::Panic(_L("Create busy"), 0));
                }
            iFree = FALSE; // Only one requests allowed        
            iOperation = aOperation;

            iSubRequest = new (ELeave)CEtelContextCreateSecondPhase();
            iSubRequest->ConstructL(this);

            iResponse->iContextInfo.iContextId = ContextId;
            iSubRequest->DoTask(iResponse);
            // Started

            SetActive(); // Go to wait

            LOG(Log::Printf(_L
                ("<%s>[Context %d] Starting 2nd phase of context creation"),
                iContext->Nif().Name().PtrZ(),
                ContextId));
            break;
            }
        case KContextActivate:
            {
            if(!iFree)
                {
                LOG(Log::Printf(_L("<%s>[Context %d] Issue Activate Request failed, another request pending"),iContext->Nif().Name().PtrZ(),ContextId));
                return KErrInUse;
                }
            iFree = FALSE; // Only one requests allowed

            iOperation = aOperation;

            // Start the operation wrapper for context Activation
            iSubRequest = new (ELeave)CEtelContextActivate();
            iSubRequest->ConstructL(this);

            iResponse->iContextConfig.Reset();

            iResponse->iContextInfo.iContextId = ContextId;
            iSubRequest->DoTask(iResponse);
            // Started

            SetActive(); // Go to wait

            LOG(Log::Printf(_L("<%s>[Context %d] Activate request issued"),
                            iContext->Nif().Name().PtrZ(), 
                            ContextId));        
            break;
            }    
        case KContextModifyActive:
            {
            if(!iFree)
                {
                LOG(Log::Printf(_L("<%s>[Context %d] Issue Modify Active Context Request failed, another request pending"),iContext->Nif().Name().PtrZ(),ContextId));
                return KErrInUse;
                }
            iFree = FALSE; // Only one request allowed

            iOperation = aOperation;

            // Start the operation wrapper for active context modification
            iSubRequest = new (ELeave)CEtelContextModifyActive();
            iSubRequest->ConstructL(this);

            iResponse->iContextConfig.Reset();

            iResponse->iContextInfo.iContextId = ContextId;
            iSubRequest->DoTask(iResponse);
            // Started 

            SetActive();        

            LOG(Log::Printf(_L("<%s>[Context %d] Modify Active request issued"), iContext->Nif().Name().PtrZ(), ContextId));
            break;
            }
        case KContextQoSSet:
            {
            if(!iFree)
                {
                LOG(Log::Printf( _L("<%s>[Context %d] Issue Set QoS Request failed, another request pending"),iContext->Nif().Name().PtrZ(),ContextId));
                return KErrInUse;
                }
            iFree = FALSE; // Only one pending requests allowed per context

            iOperation = aOperation;

            iResponse->iContextConfig.Set(aParameters->iContextConfig);
            // Start the wrapper class for QoS setting
            iSubRequest = new (ELeave)CEtelSetQoS();        
            iSubRequest->ConstructL(this);        
            iResponse->iContextInfo.iContextId = ContextId;
            iSubRequest->DoTask(iResponse);
            // Started

            // Wait for asynchronous RPacketContext::SetQoS() to finish
            SetActive(); 
        
            LOG(Log::Printf(
                _L("<%s>[Context %d] Set QoS request issued"),
                iContext->Nif().Name().PtrZ(),
                ContextId));
            break;
            }
        case KContextTFTModify :
            {
            if(!iFree)
                {
                LOG(Log::Printf(_L("<%s>[Context %d] Issue TFT Modify Request failed, another request pending"),iContext->Nif().Name().PtrZ(),ContextId));
                return KErrInUse;
                }
            iFree = FALSE; // Only one requests allowed

            iOperation = aOperation;

            iResponse->iContextConfig.Set(aParameters->iContextConfig);
            iResponse->iTFTOperationCode = aParameters->iTFTOperationCode;
            // Start the wrapper class for TFT operation
            iSubRequest = new (ELeave)CEtelContextTFTOperation();
            iSubRequest->ConstructL(this);

            iSubRequest->DoTask(iResponse);

            LOG(Log::Printf(
                _L("<%s>[Context %d] Modify TFT request issued"),
                iContext->Nif().Name().PtrZ(), 
                ContextId));
            SetActive(); // Wait...
            break;
            }
        default:
            {
            __ASSERT_DEBUG(0, User::Panic(_L("Unknown operation"), 0));
            break;
            }
        }
    return KErrNone;
    }

void CEtelRequest::DoCancel()
    {        
    if(iSubRequest)
        {
        iSubRequest->Cancel();
        }
    iOperation = NULL;
    iFree = TRUE;    // Welcome next request in
    iStatus = KErrNone;
    }

/* 
 *  Method called when the child active object finishes
 */
void CEtelRequest::RunL()
    {
    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelRequest::RunL() START")));
    #endif
    switch(iOperation)
        {
        case KStartupPrimaryContextCreation:
            {
            LOG(Log::Printf(
                _L("<%s>[Context %d] Create new primary context finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));
            HandleStartupPrimaryContextRequestResponse();
            break;
            }
        case KContextActivate:
            {
            LOG(Log::Printf(
                _L("<%s>[Context %d] Activate request finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));
            HandleActivateRequestResponse();
            break;
            }
        case KContextModifyActive:
            {
            LOG(Log::Printf(
                _L("<%s>[Context %d] Modify active request finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));
            HandleModifyActiveRequestResponse();
            break;
            }
        case KContextQoSSet:
            {
            LOG(Log::Printf(
                _L("<%s>[Context %d] Set QoS Request finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));                
            HandleContextQoSSetRequestResponse();
            break;
            }
        case KContextTFTModify:
            {
            LOG(Log::Printf(
                _L("<%s>[Context %d] Modify TFT Request finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));                
            HandleContextTFTModifyRequestResponse();
            break;
            }
        case KContextCreate:
            {
            LOG(Log::Printf(_L
                ("<%s>[Context %d] 2nd phase of context creation finished"),
                iContext->Nif().Name().PtrZ(),
                iResponse->iContextInfo.iContextId));
            HandleContextCreateSecondPhaseRequestResponse();
            break;
            }
        default:
            {
            __ASSERT_DEBUG(0, User::Panic(_L("Unknown operation"), 0));
            break;
            }
        }
    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelRequest::RunL() STOP")));
    #endif
    return;
    }

void CEtelRequest::HandleContextCreateSecondPhaseRequestResponse()
    {    
    // Build response Event
    TContextParameters* secondaryContextCreatedEvent=NULL;
    TRAPD(err, secondaryContextCreatedEvent = 
                  new (ELeave) TContextParameters());
    if(err)
        {
        return;
        }

    CUmtsNif& nif = iContext->Nif();

    FillEvent(*secondaryContextCreatedEvent);

    TPckg<TContextParameters> event(*secondaryContextCreatedEvent);

    SubrequestReady();

    // In case the initialization failed -> delete the context
    if(iStatus != KErrNone)
        {
        nif.ContextManager()->Delete(iContext->ContextId());
        }
    if(nif.EventsOn())
        {
        LOG(PrintEvent(KSecondaryContextCreated, 
                       *secondaryContextCreatedEvent));
        nif.EventHandler().Event((CProtocolBase*)&(nif), 
                                 KSecondaryContextCreated, 
                                 event); // Notify the upper layer
        }

    iContext->SetCreated();
    delete secondaryContextCreatedEvent;
    secondaryContextCreatedEvent=NULL;
    }

void CEtelRequest::HandleStartupPrimaryContextRequestResponse()
    {    
    // Sets the common config all contexts in this Nif can use
    RPacketContext::TContextConfigGPRS tempConfig;
    iResponse->iContextConfig.GetContextConfig(tempConfig);
    iContext->Nif().ContextManager()->SetCommonConfig(tempConfig);
    // Common config set

    // Build response Event
    TContextParameters* primaryContextCreatedEvent=NULL;
    TRAPD(err, primaryContextCreatedEvent=new (ELeave) TContextParameters());
    if(err)
        {
        return;
        }

    FillEvent(*primaryContextCreatedEvent);    // Standard info
    FillEventQoS(*primaryContextCreatedEvent); // QoS

    TPckg<TContextParameters> event(*primaryContextCreatedEvent);    

    SubrequestReady();

    if(iStatus == KErrNone)
        {
        if(iContext->Nif().EventsOn() && iContext->Usable())
            {
            LOG(PrintEvent(KPrimaryContextCreated, 
                           *primaryContextCreatedEvent));
            iContext->Nif().EventHandler().Event(
                (CProtocolBase*)&(iContext->Nif()),
                KPrimaryContextCreated, 
                event); // Notify the upper layer
            }
        LOG(Log::Printf(_L("<%s> Calling StartSending()"), 
                        iContext->Nif().Name().PtrZ()));
        iContext->Nif().Network()->StartSending(
            (CProtocolBase*)&(iContext->Nif())); 

        iContext->Nif().Notify()->LinkLayerUp();
        // EIfProgressLinkUp changed to KLinkLayerOpen
        iContext->Nif().Notify()->IfProgress(KLinkLayerOpen, KErrNone);
        }
    else 
        {
        LOG(Log::Printf(_L("<%s>[Context %d] Sending NegotiationFailed \
signal to Nifman. Error <%d>"), iContext->Nif().Name().PtrZ(),
                         iResponse->iContextInfo.iContextId, 
                         iStatus.Int()));
        // Causes this CNifIfBase to be deleted 
        iContext->Nif().Notify()->NegotiationFailed(&(iContext->Nif()), 
                                                    KErrNone); 
        }
    iContext->SetCreated();

    delete primaryContextCreatedEvent;
    primaryContextCreatedEvent=NULL;
    }
/*
This method is called after the activation is completed in the CEtelRequest::RunL().

The method generates the event to notify the upper layer (GUQOS).
While generating the event, the parameters can also be set. For example, in
the network downgrading parameters case, the downgraded umts parameters are set 
in this method. Same is true for SBLP error code. The control option set by the 
test application (TS_QOS) can be checked and the values can be filled accordingly

Activate is only called during the last phase of the secondary context creation.
This is contrast with the ModifyActive call during secondary context 
configuration (i.e, for the already created secondary context, if SetParameters() 
CSubConnection API is called, then the created context is configured!)

*/
void CEtelRequest::HandleActivateRequestResponse()
    {
    iOperation = NULL;    
    iFree = TRUE;

    // Build response Event
    TContextParameters* activateEvent=NULL;

    TInt err;

    TRAP(err, activateEvent=new (ELeave) TContextParameters());
    if(err)
        {
        return;
        }

    FillEvent(*activateEvent);    // Standard info
    FillEventQoS(*activateEvent); // QoS
    FillEventTFT(*activateEvent); // TFT

    // Print out negotiated rel99 parameters
    if(iContext->Nif().ControlOption() == KPrintRel99)
        {
        RPacketQoS::TQoSR5Requested reqParameters;
        activateEvent->iContextConfig.GetUMTSQoSReq(reqParameters);

        RFs fs;
        RFile file;
        fs.Connect();
        // Just to be sure, delete file
        fs.Delete(KRel99);

        (void)file.Create(fs, KRel99, EFileStreamText);

        HBufC8 *buf=NULL;
        TRAP( err, buf = HBufC8::NewL(512) );

        if ( err )
            {
            delete activateEvent;
            return;
            }

        TPtr8 ptr=buf->Des();
        ptr.AppendFormat(_L8("TrafficClass:\t%d"),reqParameters.iReqTrafficClass);
        ptr.AppendFormat(_L8("DeliveryOrder\t%d"),reqParameters.iReqDeliveryOrderReqd);
        ptr.AppendFormat(_L8("DeliverErroneousSDU\t%d"), reqParameters.iReqDeliverErroneousSDU);
        ptr.AppendFormat(_L8("MaxSDUSize\t%d"), reqParameters.iReqMaxSDUSize);
        ptr.AppendFormat(_L8("MaxRate\t%d"), reqParameters.iReqMaxRate);
        ptr.AppendFormat(_L8("BER\t%d"), reqParameters.iReqBER);
        ptr.AppendFormat(_L8("SDUErrorRatio\t%d"),reqParameters.iReqSDUErrorRatio);
        ptr.AppendFormat(_L8("TrafficHandlingPriority\t%d"), reqParameters.iReqTrafficHandlingPriority);
        ptr.AppendFormat(_L8("TransferDelay\t%d"), reqParameters.iReqTransferDelay);
        ptr.AppendFormat(_L8("GuaranteedRate\t%d"),reqParameters.iReqGuaranteedRate);
        file.Write(*buf);
        file.Close();
        fs.Close();
        delete buf;
        }

    // Downgrade negotiated parameters according to 
    // [UMTSQOSHIGHDEMANDNEGOTIATED]
    if(iContext->Nif().ControlOption() == KDowngrade)
        {
        RPacketQoS::TQoSR5Negotiated newParameters;
        newParameters.iTrafficClass = RPacketQoS::ETrafficClassStreaming;
        newParameters.iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderRequired;
        newParameters.iDeliverErroneousSDU =RPacketQoS::EErroneousSDUDeliveryNotRequired;
        newParameters.iBER = RPacketQoS::EBEROnePerThousand;
        newParameters.iSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTenThousand;
        newParameters.iTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        newParameters.iTransferDelay = 500;
        newParameters.iMaxSDUSize = 1510;
        newParameters.iMaxRate.iUplinkRate = 1500;
        newParameters.iMaxRate.iDownlinkRate = 900;
        newParameters.iGuaranteedRate.iUplinkRate = 450;
        newParameters.iGuaranteedRate.iDownlinkRate = 250;
        activateEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        }

    // Downgrade negotiated parameters
    if(iContext->Nif().ControlOption() == KNetworkDowngrade)
        {
        RPacketQoS::TQoSR5Negotiated newParameters;
        // only the traffic class is downgraded from ETrafficClassInteractive
        // to ETrafficClassBackground
        newParameters.iTrafficClass = RPacketQoS::ETrafficClassBackground;
        newParameters.iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderRequired;
        newParameters.iDeliverErroneousSDU = RPacketQoS::EErroneousSDUDeliveryNotRequired;
        newParameters.iBER = RPacketQoS::EBEROnePerThousand;
        newParameters.iSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTenThousand;
        newParameters.iTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        newParameters.iTransferDelay = 500;
        newParameters.iMaxSDUSize = 1510;
        newParameters.iMaxRate.iUplinkRate = 1500;
        newParameters.iMaxRate.iDownlinkRate = 900;
        newParameters.iGuaranteedRate.iUplinkRate = 450;
        newParameters.iGuaranteedRate.iDownlinkRate = 250;
        activateEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        }

    // upgrade negotiated parameters
    if(iContext->Nif().ControlOption() == KNetworkUpgrade)
        {
        RPacketQoS::TQoSR5Negotiated newParameters;
        newParameters.iTrafficClass = RPacketQoS::ETrafficClassConversational;
        newParameters.iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderRequired;
        newParameters.iDeliverErroneousSDU = RPacketQoS::EErroneousSDUDeliveryNotRequired;
        newParameters.iBER = RPacketQoS::EBEROnePerThousand;
        newParameters.iSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTenThousand;
        newParameters.iTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        newParameters.iTransferDelay = 500;
        newParameters.iMaxSDUSize = 1510;
        newParameters.iMaxRate.iUplinkRate = 1500;
        newParameters.iMaxRate.iDownlinkRate = 900;
        // the guaranteed uplink rate is upgraded
        newParameters.iGuaranteedRate.iUplinkRate = 1111;
        newParameters.iGuaranteedRate.iDownlinkRate = 250;
        activateEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        }

    /*
     *  When KContextActivateRejectSBLP is set by TS_QoS, 
     *  the following if-statement will fill error code 1 in the PCO buffer.
     *  KErrGprsUserAuthenticationFailure is set by method 
     *  CEtelContextActivate::RunL()
     */
    if(iContext->Nif().ControlOption() == KContextActivateRejectSBLP)
        {
        TInt ret = KErrNone;
        const TInt KZero = 0;
        /*
         *  3GPP TS 24.008 v5.12.0:
         *  
         *  When the container identifier indicates Policy Control 
         *  rejection code, the container identifier contents field 
         *  contains a Go interface related cause code from the GGSN 
         *  to the UE (see 3GPP TS 29.207 [100]). The length of 
         *  container identifier contents indicates a length equal 
         *  to one. If the container identifier contents field is 
         *  empty or its actual length is greater than one octect, 
         *  then it shall be ignored by the receiver.
         */
        RPacketContext::TPcoId sblpRejectionCodePcoId(RPacketContext::EEtelPcktPolicyControlRejectionCode); 

        RPacketContext::TContextConfigGPRS configGPRS;
        activateEvent->iContextConfig.GetContextConfig(configGPRS);

        configGPRS.iProtocolConfigOption.iMiscBuffer.SetLength(253);
        configGPRS.iProtocolConfigOption.iMiscBuffer.FillZ();

        TPtr8 pcoBufferPtr(const_cast<TUint8*>
            (configGPRS.iProtocolConfigOption.iMiscBuffer.Ptr()),
             configGPRS.iProtocolConfigOption.iMiscBuffer.Length(),
             configGPRS.iProtocolConfigOption.iMiscBuffer.Length()); 

        pcoBufferPtr.SetLength(KZero);
        TTlvStruct<RPacketContext::TPcoId, RPacketContext::TPcoItemDataLength> pcoTLV(pcoBufferPtr,KZero);
        // Assuming that it has the value 1
        TUint8 errorCode = 1;
        TBuf8<1> sblpCodeBuffer(errorCode);         
        sblpCodeBuffer.SetLength(1); 
        /*
         *  Buffer has to be encoded  as it is outlined by the specification 
         *  -  1 is just an example - it shall have value and be encoded 
         *  as defined in TS; Data portion has to be provided only - setting 
         *  of identification of configuration option and length field is 
         *  provided by TLV class .
         */
        TPtr8 sblpCodePtr(const_cast<TUint8*>(sblpCodeBuffer.Ptr()),
                        sblpCodeBuffer.Length(),sblpCodeBuffer.Length());

        TRAPD(err, ret=pcoTLV.AppendItemL(sblpRejectionCodePcoId,sblpCodePtr));
        if (err !=KErrNone || ret!=KErrNone)
            {
            LOG(Log::Printf(_L("CEtelRequest::HandleActivateRequestResponse() Appending sblpRejectionCodePcoId failed")));
            }
            activateEvent->iContextConfig.SetContextConfig(configGPRS);
        }
        
      
        
    /*
     *  
     */
    if(iContext->Nif().ControlOption() == KNetworkUnsupportedIms)
        {
        /*
        In the KNetworkUnsupportedIms case, the context is not configured to IMS 
        but treated like a normal secondary context. But the network resets the 
        IMS flag value to EFalse
        */
        
        // reset the IMS flag in the nif's active context
        iContext->SetIMCNSubsystemflag(EFalse);
        
        }
        
    /*
     *  
     */
    if(iContext->Nif().ControlOption() == KNetworkUnsupportedUmtsR5)
        {
        /*
        In the KNetworkUnsupportedUmtsR5 case, the context is not configured to UMTSR5 
        but treated like a normal secondary context. But the network resets the 
        UMTS R5 values
        */
        
        RPacketQoS::TQoSR5Negotiated newParameters;
        RPacketQoS::TQoSR5Negotiated negotiatedParameters;
        activateEvent->iContextConfig.GetUMTSQoSNeg(negotiatedParameters);
        
        newParameters.iTrafficClass = negotiatedParameters.iTrafficClass;
        newParameters.iDeliveryOrderReqd = negotiatedParameters.iDeliveryOrderReqd;
        newParameters.iDeliverErroneousSDU = negotiatedParameters.iDeliverErroneousSDU;
        newParameters.iBER = negotiatedParameters.iBER;
        newParameters.iSDUErrorRatio = negotiatedParameters.iSDUErrorRatio;
        newParameters.iTrafficHandlingPriority = negotiatedParameters.iTrafficHandlingPriority;
        newParameters.iTransferDelay = negotiatedParameters.iTransferDelay;
        newParameters.iMaxSDUSize = negotiatedParameters.iMaxSDUSize;
        newParameters.iMaxRate.iUplinkRate = negotiatedParameters.iMaxRate.iUplinkRate;
        newParameters.iMaxRate.iDownlinkRate = negotiatedParameters.iMaxRate.iDownlinkRate;
        newParameters.iGuaranteedRate.iUplinkRate = negotiatedParameters.iGuaranteedRate.iUplinkRate;
        newParameters.iGuaranteedRate.iDownlinkRate = negotiatedParameters.iGuaranteedRate.iDownlinkRate;
        
        // only the r5 parameters are reset!
        newParameters.iSignallingIndication = EFalse;
		newParameters.iSourceStatisticsDescriptor = RPacketQoS::ESourceStatisticsDescriptorUnknown;
		    
        activateEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        
        }
        
        
    /*
    check if the IMCN flag is set for the current context
    
    if it is then it is a dedicated IMS context. create a 
    ims file to notify the test client
    */
    TBool imsflag;
    iContext->GetIMCNSubsystemflag(imsflag);
    if (imsflag)
    	{
    		//create ims file
    		CreateFileIMS();
    		LOG(Log::Printf(_L("CEtelRequest::HandleModifyActiveRequestResponse() Active context is modified to IMS context")));
    	}
    
    
    
    
    // Downgrade r5 negotiated parameters
    if(iContext->Nif().ControlOption() == KNetworkDowngradeR5)
        {
        RPacketQoS::TQoSR5Negotiated newParameters;
        // only the traffic class is downgraded from ETrafficClassInteractive
        // to ETrafficClassBackground
        newParameters.iTrafficClass = RPacketQoS::ETrafficClassBackground;
        newParameters.iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderRequired;
        newParameters.iDeliverErroneousSDU = RPacketQoS::EErroneousSDUDeliveryNotRequired;
        newParameters.iBER = RPacketQoS::EBEROnePerThousand;
        newParameters.iSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTenThousand;
        newParameters.iTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        newParameters.iTransferDelay = 500;
        newParameters.iMaxSDUSize = 1510;
        newParameters.iMaxRate.iUplinkRate = 1500;
        newParameters.iMaxRate.iDownlinkRate = 900;
        newParameters.iGuaranteedRate.iUplinkRate = 450;
        newParameters.iGuaranteedRate.iDownlinkRate = 250;
        newParameters.iSignallingIndication = ETrue;
		newParameters.iSourceStatisticsDescriptor = RPacketQoS::ESourceStatisticsDescriptorUnknown;
		    
        activateEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        }
        
        
     /*
    check if the SI flag is set for the current context
    
    if it is then it is a UMTS r5 context. create a 
    umtsr5 file to notify the test client
    */
    
    RPacketQoS::TQoSR5Negotiated negotiated;
    if(iContext->GetPendingQoS(negotiated))
    	{
    	if (negotiated.iSignallingIndication) // if it is ETrue
    		{
    		//create umtsr5 file
    		CreateFileUMTSR5();
    		LOG(Log::Printf(_L("CEtelRequest::HandleModifyActiveRequestResponse() Active context is modified to UMTSR5 context")));
    		}
    	}
    
        
        
        
        
        
        
        
        
        

    TPckg<TContextParameters> event(*activateEvent);

    SubrequestReady();

    if(iContext->Nif().EventsOn() && iContext->Usable())
        {
        LOG(PrintEvent(KContextActivateEvent,*activateEvent));
        iContext->Nif().EventHandler().Event((CProtocolBase*)&(iContext->Nif()), KContextActivateEvent, event); // Notify the upper layer
        }
    delete activateEvent;
    activateEvent=NULL;
    }

/*
This method is called after the active context modification is completed 
in the CEtelRequest::RunL().

The method generates the event to notify the upper layer (GUQOS).
While generating the event, the parameters can also be set. For example, in
the network downgrading parameters case, the downgraded umts parameters are set 
in this method. The control option set by the test application (TS_QOS) 
can be checked and the values can be filled accordingly

Activate is only called during the last phase of the secondary context creation.
This is contrast with the ModifyActive call during secondary context 
configuration (i.e, for the already created secondary context, if SetParameters() 
CSubConnection API is called, then the created context is configured!)

*/
void CEtelRequest::HandleModifyActiveRequestResponse()
    {
    // Build response Event
    TContextParameters* contextEvent = NULL;
    TRAPD(err, contextEvent = new (ELeave) TContextParameters());
    if (err)
        {
        return;
        }
        

    SubrequestReady();

    // If the activation failed, check if there are 
    // pending QoS profile changes and inform GUQoS
    if(iStatus != KErrNone)
        {
        RPacketQoS::TQoSR5Negotiated qos;
        if(iContext->GetPendingQoS(qos))
            {
            // The case is:
            // - 1) We have issued a modify active operation
            // - 2) The network downgrades the profile
            // - 3) a) the modification fails -> 
            //         send this changed QoS profile to GUQoS
            //      b) the modification succeeds -> 
            //         send that to GUQoS and ignore this one
            FillEvent(*contextEvent);       // Standard info
            FillEventQoS(*contextEvent);    // QoS
            FillEventTFT(*contextEvent);    // TFT
            
            contextEvent->iReasonCode = KErrNone;

            TPckg<TContextParameters> event(*contextEvent);
            
            LOG(Log::Printf(_L("<%s> Sending KContextParametersChangeEvent event for [Context %d]"), iContext->Nif().Name().PtrZ(), 
                          contextEvent->iContextInfo.iContextId));
            iContext->Nif().EventHandler().Event((CProtocolBase*)&(iContext->Nif()),KContextParametersChangeEvent, event);    
            }
        }

    FillEvent(*contextEvent);       // Standard info
    FillEventQoS(*contextEvent);    // QoS
    FillEventTFT(*contextEvent);    // TFT
    

	/*
    *	If the testnif is informed to
    	fail the active context modification
    	return KErrTest.
    	For example , IMS context has to be failed   
    */
    if(iContext->Nif().ControlOption() == KContextModifyActiveFailAsync)
        {
        iStatus = KErrTest;
        }  
    
    
    
    /*
    *  
    */
    if(iContext->Nif().ControlOption() == KNetworkUnsupportedIms)
        {
        /*
        In the KNetworkUnsupportedIms case, the context is not configured to IMS 
        but treated like a normal secondary context. But the network resets the 
        IMS flag value to EFalse
        */
        
        // reset the IMS flag in the nif's active context
        iContext->SetIMCNSubsystemflag(EFalse);
        
        }  
        
        
    /*
     *  
     */
    if(iContext->Nif().ControlOption() == KNetworkUnsupportedUmtsR5)
        {
        /*
        In the KNetworkUnsupportedUmtsR5 case, the context is not configured to UMTSR5 
        but treated like a normal secondary context. But the network resets the 
        UMTS R5 values
        */
        
        RPacketQoS::TQoSR5Negotiated newParameters;
        RPacketQoS::TQoSR5Negotiated negotiatedParameters;
        contextEvent->iContextConfig.GetUMTSQoSNeg(negotiatedParameters);
        
        newParameters.iTrafficClass = negotiatedParameters.iTrafficClass;
        newParameters.iDeliveryOrderReqd = negotiatedParameters.iDeliveryOrderReqd;
        newParameters.iDeliverErroneousSDU = negotiatedParameters.iDeliverErroneousSDU;
        newParameters.iBER = negotiatedParameters.iBER;
        newParameters.iSDUErrorRatio = negotiatedParameters.iSDUErrorRatio;
        newParameters.iTrafficHandlingPriority = negotiatedParameters.iTrafficHandlingPriority;
        newParameters.iTransferDelay = negotiatedParameters.iTransferDelay;
        newParameters.iMaxSDUSize = negotiatedParameters.iMaxSDUSize;
        newParameters.iMaxRate.iUplinkRate = negotiatedParameters.iMaxRate.iUplinkRate;
        newParameters.iMaxRate.iDownlinkRate = negotiatedParameters.iMaxRate.iDownlinkRate;
        newParameters.iGuaranteedRate.iUplinkRate = negotiatedParameters.iGuaranteedRate.iUplinkRate;
        newParameters.iGuaranteedRate.iDownlinkRate = negotiatedParameters.iGuaranteedRate.iDownlinkRate;
        
        // only the r5 parameters are reset!
        newParameters.iSignallingIndication = EFalse;
		newParameters.iSourceStatisticsDescriptor = RPacketQoS::ESourceStatisticsDescriptorUnknown;
		    
        contextEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        
        }    
    
    /*
    check if the IMCN flag is set for the current context
    
    if it is then it is a dedicated IMS context. create a 
    ims file to notify the test client
    */
    TBool imsflag;
    iContext->GetIMCNSubsystemflag(imsflag);
    if (imsflag)
    	{
    		//create ims file
    		CreateFileIMS();
    		LOG(Log::Printf(_L("CEtelRequest::HandleModifyActiveRequestResponse() Active context is modified to IMS context")));
    	}

	
    
    // Downgrade r5 negotiated parameters
    /*
    if(iContext->Nif().ControlOption() == KNetworkDowngradeR5)
        {
        RPacketQoS::TQoSR5Negotiated newParameters;
        // only the traffic class is downgraded from ETrafficClassInteractive
        // to ETrafficClassBackground
        newParameters.iTrafficClass = RPacketQoS::ETrafficClassBackground;
        newParameters.iDeliveryOrderReqd = RPacketQoS::EDeliveryOrderRequired;
        newParameters.iDeliverErroneousSDU = RPacketQoS::EErroneousSDUDeliveryNotRequired;
        newParameters.iBER = RPacketQoS::EBEROnePerThousand;
        newParameters.iSDUErrorRatio = RPacketQoS::ESDUErrorRatioOnePerTenThousand;
        newParameters.iTrafficHandlingPriority = RPacketQoS::ETrafficPriority2;
        newParameters.iTransferDelay = 500;
        newParameters.iMaxSDUSize = 1510;
        newParameters.iMaxRate.iUplinkRate = 1500;
        newParameters.iMaxRate.iDownlinkRate = 900;
        newParameters.iGuaranteedRate.iUplinkRate = 450;
        newParameters.iGuaranteedRate.iDownlinkRate = 250;
        contextEvent->iContextConfig.SetUMTSQoSNeg(newParameters);
        /*
        *   Downgrade the r5 parameters here
        *
        *   to do:
        *
        */
      /*  }
    */
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    TPckg<TContextParameters> event(*contextEvent);

    if(iContext->Nif().EventsOn() && iContext->Usable())
        {
        LOG(PrintEvent(KContextModifyActiveEvent,*contextEvent));
        iContext->Nif().EventHandler().Event((CProtocolBase*)&(iContext->Nif()), KContextModifyActiveEvent, event); // Notify the upper layer
        }
    delete contextEvent;
    contextEvent=NULL;
    }




/* 
 *  Method handles the finished asynchronous 
 *  RPacketQoS::SetProfileParameters() -call
 */
void CEtelRequest::HandleContextQoSSetRequestResponse()
    {
    TContextParameters* QoSSetEvent = NULL;
    TRAPD(err, QoSSetEvent = new (ELeave) TContextParameters());
    if (err)
        {
        return;
        }

    FillEvent(*QoSSetEvent); // Standard info

    TPckg<TContextParameters> event(*QoSSetEvent);

    SubrequestReady();

    if(iContext->Nif().EventsOn() && iContext->Usable())
        {
        LOG(PrintEvent(KContextQoSSetEvent,*QoSSetEvent));
        iContext->Nif().EventHandler().Event(
            (CProtocolBase*)&(iContext->Nif()), 
            KContextQoSSetEvent, 
            event); // Notify the upper layer
        }
    delete QoSSetEvent;
    QoSSetEvent=NULL;
    }

/* 
 *  Method handles the finished asynchronous 
 *  RPacketQoS::SetProfileParameters() -call
 */
void CEtelRequest::HandleContextTFTModifyRequestResponse()
    {
    TContextParameters* TFTModifyEvent = NULL;
    TRAPD(err, TFTModifyEvent = new (ELeave) TContextParameters());
    if (err)
        {
        return;
        }
        
    // Copy TFT-info to context data
    TTFTInfo tfttemp;
    iResponse->iContextConfig.GetTFTInfo(tfttemp);
    iContext->Parameters().iContextConfig.SetTFTInfo(tfttemp);
    // TFT-info Copied

    FillEvent(*TFTModifyEvent); // Standard info    

    TPckg<TContextParameters> event(*TFTModifyEvent);

    SubrequestReady();

    if(iContext->Nif().EventsOn() && iContext->Usable())
        {
        LOG(PrintEvent(KContextTFTModifiedEvent,*TFTModifyEvent));
        iContext->Nif().EventHandler().Event(
            (CProtocolBase*)&(iContext->Nif()),
            KContextTFTModifiedEvent, 
            event); // Notify the upper layer
        }    
    delete TFTModifyEvent;
    TFTModifyEvent=NULL;
    }

/* 
 *  Build the event to send upstairs
 *  Includes:
 */
void CEtelRequest::FillEvent(TContextParameters &aEvent)
    {
    TContextInfo info;

    // TFT operation code. GUQoS checks this for validity
    aEvent.iTFTOperationCode = iResponse->iTFTOperationCode; 

    RPacketContext::TContextStatus iTempStatus;
    iContext->ContextHandle().GetStatus(iTempStatus);

    info.iStatus = iTempStatus;
    info.iContextId = iContext->ContextId();      // Context Id
    aEvent.iContextType = iContext->ContextType();// Context type
    aEvent.iReasonCode = iResponse->iReasonCode;  // Operation fail / success
    aEvent.iContextInfo = info;

    // Ask the contextmanager for the common config
    RPacketContext::TContextConfigGPRS tempConfig;
    iContext->Nif().ContextManager()->GetCommonConfig(tempConfig);
    aEvent.iContextConfig.SetContextConfig(tempConfig); // Set context config
    }

void CEtelRequest::FillEventQoS(TContextParameters &aEvent)
    {
    RPacketQoS::TQoSR5Negotiated QoS;
    iContext->Parameters().iContextConfig.GetUMTSQoSNeg(QoS);
    aEvent.iContextConfig.SetUMTSQoSNeg(QoS);
    }

void CEtelRequest::FillEventTFT(TContextParameters &aEvent)
    {
    TTFTInfo TFT;
    iContext->Parameters().iContextConfig.GetTFTInfo(TFT);
    aEvent.iContextConfig.SetTFTInfo(TFT);
    }

/*
Create the IMS file if the active context is configured or 
new ims context is created
*/
void CEtelRequest::CreateFileIMS()
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, KTestIMS, EFileStreamText);
    if(err == KErrNotFound)
        {
        file.Create(fs, KTestIMS, EFileStreamText);
        }
    
    file.Close();
    fs.Close();
    }

/*
Create the UMTSR5 file if the active context is configured or 
new UMTSR5 context is created
*/
void CEtelRequest::CreateFileUMTSR5()
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, KTestUmtsR5, EFileStreamText);
    if(err == KErrNotFound)
        {
        file.Create(fs, KTestUmtsR5, EFileStreamText);
        }
    
    file.Close();
    fs.Close();
    }
        

/*  
 *  A common base class for all subrequest groups
 */
CEtelSubRequest::CEtelSubRequest() 
    : CActive(EPriorityStandard), 
      iGPDSContextIdPtr(iGPDSContextId)
    {
    }

CEtelSubRequest::~CEtelSubRequest()
    {    
    if(IsActive())
        Cancel();
    }

void CEtelSubRequest::ConstructL(CEtelRequest *aParent)
    {
    CActiveScheduler::Add(this);     
    iParent = aParent;
    iCallerStatus = &(aParent->iStatus); 
    }

void CEtelSubRequest::DoCancel()
    {
    if(iLastCancelCode == EPacketQoSGetProfileParams ||
       iLastCancelCode == EPacketQoSSetProfileParams)
       {
       iParent->iContext->ContextQoS().CancelAsyncRequest(iLastCancelCode);
       }
    else if(iLastCancelCode != 0)
        {
        iParent->iContext->ContextHandle().CancelAsyncRequest(
            iLastCancelCode);
        }
    iStatus=KErrNone;

    LOG(Log::Printf(_L
        ("*** CEtelSubRequest::DoCancel() cancelling the subrequest ****")));
    User::RequestComplete(iCallerStatus,KErrCancel);
    }

void CEtelSubRequest::JumpToRunl(TInt aError)
    {
    SetActive();
    TRequestStatus* statusPtr=&iStatus;
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelSubRequest::JumpToRunl() Signalling RunL")));
#endif
    User::RequestComplete(statusPtr,aError);
    }
    

void CEtelContextTFTOperation::CreateFileSblpAdded()
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, KTestSblpAdd, EFileStreamText);
    if(err == KErrNotFound)
        {
        file.Create(fs, KTestSblpAdd, EFileStreamText);
        }
    
    file.Close();
    fs.Close();
    }
/*
void CEtelContextTFTOperation::CreateFileIMS()
    {
    RFs fs;
    RFile file;
    fs.Connect();
    TInt err = file.Open(fs, KTestIMS, EFileStreamText);
    if(err == KErrNotFound)
        {
        file.Create(fs, KTestIMS, EFileStreamText);
        }
    
    file.Close();
    fs.Close();
    }
    */

/*  
 *  TFT operations wrapper. 
 *  Optional operations:
 *   - KAddFilters   : Add new filters to TFT
 *   - KRemoveFilters: Remove filters from TFT
 *   - KDeleteTFT    : Deletes the whole TFT of a context
 */        
CEtelContextTFTOperation::CEtelContextTFTOperation() 
    : iFilterPtr(iFilter),
      iUMTSConfigPtr(iUMTSConfig),
      iUMTSQoSNegPtr(iUMTSQoSNeg)
    {    
    iCounter = 0;
    iCancellable = ETrue;
    }

void CEtelContextTFTOperation::RunL()
    {
    __ASSERT_DEBUG(!IsActive(), User::Panic(_L("Active panic!"), 0));
    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() START")));
    #endif
    __ASSERT_DEBUG(iStatus == KErrNone, User::Panic(_L("TFT Operation failed!"), 0));

    if(iParent->iContext->Nif().ControlOption() == KContextTFTModifyFailAsync)
        {
        iStatus = KErrTest;
        }

    if(iStatus != KErrNone) // In case something fails
        {
        LOG(Log::Printf(_L("<%s>[Context %d] Failure in TFT opearation"), 
            iParent->iContext->Nif().Name().PtrZ(),
            iParent->iContext->ContextId()));
        
        // Should start a state machine to restore old state
        iContextParameters->iReasonCode = iStatus.Int();
        #ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() calling User::RequestComplete() due to failure")));
        #endif
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else // if(iStatus != KErrNone)
        {
        switch(iRequestState)
            {
            case KDoTFTTask:
                {
                switch(iContextParameters->iTFTOperationCode)
                    {
                    case KAddSblpParameter:
                        {
                        // SBLP implementation for nif
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()Adding sblp")));
                        RPacketContext::CTFTMediaAuthorizationV3 *sblpParams;
                        sblpParams = RPacketContext::CTFTMediaAuthorizationV3::NewL();
                        iTFTInfo.GetSblpToken(*sblpParams);
                        TBuf<255> mat;
                        mat.Copy(sblpParams->iAuthorizationToken);
                        LOG(Log::Printf(_L("media authorization   : %S"),&mat));
                        
                        TInt i;
                        for(i=0; i<sblpParams->iFlowIds.Count();i++)
                            {
                            LOG(Log::Printf(_L("flow id               : <%d>"),
                                sblpParams->iFlowIds.Count()));
                            LOG(Log::Printf(_L("media component number: <%d>"),
                             sblpParams->iFlowIds[i].iMediaComponentNumber));
                            LOG(Log::Printf(_L("flow number           : <%d>"),
                                sblpParams->iFlowIds[i].iIPFlowNumber));
                            }
						// iParent->iContext->ContextHandle().AddMediaAuthorizationL(iStatus, *sblpParams );                            
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()calling User::RequestComplete() after adding sblp finished")));
                        #endif
                        User::RequestComplete(iCallerStatus,KErrNone);
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()STOP")));
                        #endif
                        CreateFileSblpAdded();
                        delete sblpParams;
                        return;
                        } // case KAddSblpParameter:
                    case KRemoveSblpParameter:
                        {
                        // SBLP implementation for nif
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()removing sblp")));
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()calling User::RequestComplete() after removing sblp finished")));
                        #endif
                        User::RequestComplete(iCallerStatus,KErrNone);
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()STOP")));
                        #endif
                        return;
                        } // case KRemoveSblpParameter:
                    case KAddIMCNSubsystemflag:
                        {
                       /*	IMS implementation for nif
                       	*
                        *	IMS value from guqos is read from the passsed TFTInfo structure
                        *	and is kept in the active context (in the testnif)
                        *
                        *	This is checked once the Activate or ModifyActive is called.
                        */
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()Adding IMCN flag")));
                        TBool imcnSubsystemflag;
                        iTFTInfo.GetIMCNSubsystemflag(imcnSubsystemflag);
                        LOG(Log::Printf(_L("IMCN flag value               : <%d>"),imcnSubsystemflag));
                        iParent->iContext->SetIMCNSubsystemflag(imcnSubsystemflag);
                        SetActive();
                        return;
                        } // case KAddIMCNSubsystemflag:                        
                    case KAddFilters:
                        {
                        if(iTFTInfo.NextPacketFilter(iFilter) != KErrNotFound)
                            {
                            iParent->iContext->ContextHandle().AddPacketFilter(iStatus,iFilterPtr);
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() Adding filter <%d>"),iFilter.iId));
                            iLastCancelCode = EPacketContextAddPacketFilter;
                            iCancellable = ETrue;
                            SetActive();
                            return;
                            }
                        else
                            {
                            iRequestState = KDoTFTFetch;
                            iCancellable = EFalse;
                            JumpToRunl(0);
                            return;
                            }
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()STOP")));
                        #endif
                        } // case KAddFilters:
                    case KRemoveFilters:
                        {
                        if(iTFTInfo.NextPacketFilter(iFilter) != 
                            KErrNotFound)
                            {
                            iParent->iContext->ContextHandle().
                                RemovePacketFilter(iStatus,iFilter.iId);
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() Removing filter <%d>"),iFilter.iId));
                            iLastCancelCode = 
                                EPacketContextRemovePacketFilter;
                            iCancellable = ETrue;
                            SetActive();
                            return;
                            }
                        else // No more Filters to remove
                            {
                            iRequestState = KDoTFTFetch;
                            iCancellable = EFalse;
                            JumpToRunl(0);
                            return;
                            }
                        #ifdef _RUNL_DEBUG
                        LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL()STOP")));
                        #endif
                        } // case KRemoveFilters:
                    case KDeleteTFT:
                        {
                        if(iNumberOfFilters < 0) 
                            {
                            __ASSERT_DEBUG(0, User::Panic(
                                _L("Negative iNumberOfFilters"), 0));
                            }
                        // No more filters
                        else if(iCounter == (TUint)iNumberOfFilters)
                            {
                            #ifdef _RUNL_DEBUG
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() calling User::RequestComplete() after delete TFT finished")));
                            #endif
                            User::RequestComplete(iCallerStatus,KErrNone);
                            #ifdef _RUNL_DEBUG
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() STOP")));
                            #endif
                            return;
                            }
                        // Every other RunL() call retrieves filter info id,
                        // every other removes filter retrieved last round
                        switch(iTFTDeleteSubRequestState)
                            {
                            // Retrieve info
                            case ERetrieveFilterInfoState:
                                {
                                iTFTDeleteSubRequestState = 
                                    EDeleteFilterState;
                        
                                iParent->iContext->ContextHandle().
                                    GetPacketFilterInfo(iStatus,
                                                        iCounter++,
                                                        iFilterPtr);
                                iLastCancelCode = 
                                   EPacketContextGetPacketFilterInfo;
                                iCancellable = ETrue;
                                SetActive();
                                return;
                                }
                            // Delete filter
                            case EDeleteFilterState:
                                {
                                iParent->iContext->ContextHandle().
                                    RemovePacketFilter(iStatus,iFilter.iId);
                                iLastCancelCode = 
                                    EPacketContextRemovePacketFilter;
                        
                                iTFTDeleteSubRequestState = 
                                    ERetrieveFilterInfoState;
                                iCancellable = ETrue;
                                SetActive();
                                #ifdef _RUNL_DEBUG
                                LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() STOP")));
                                #endif
                                return;
                                }
                            } // switch(iTFTDeleteSubRequestState)
                            break;
                        } // case KDeleteTFT:
                    } // switch(iContextParameters->iTFTOperationCode)
                } // case KDoTFTTask:
            case KDoTFTFetch:
                {
                // The infamous TFT-retrieval subblock 
                switch(iTFTFetchSubState) 
                    {
                    // Get the number of Filters
                    case ETFTGetEnumerationFetchState:
                        {
                        iParent->iContext->ContextHandle().
                            EnumeratePacketFilters(iStatus,
                                                   iNumberOfFilters);
                        iLastCancelCode = 
                            EPacketContextEnumeratePacketFilters;
                        // Initial state for TFT removal
                        iTFTFetchSubState = ERetrieveFilterInfoFetchState;
                        iCancellable = ETrue;
                        SetActive();
                        
                        TTFTInfo temp; // Reset
                        iContextParameters->iContextConfig.SetTFTInfo(temp);
                        return;
                        }
                    // Copy and fall through
                    case ECopyRetrievedFilterFetchState:
                        {
                        TTFTInfo temp;
                        iContextParameters->iContextConfig.GetTFTInfo(temp);
                        temp.AddPacketFilter(iFilter);
                        iContextParameters->iContextConfig.SetTFTInfo(temp);
                        }
                    // Retrieve info
                    case ERetrieveFilterInfoFetchState:
                        {
                        if(iNumberOfFilters < 0)
                            {
                            __ASSERT_DEBUG(0, User::Panic(
                                _L("Negative iNumberOfFilters"), 0));
                            }
                        // No more filters if TSY
                        else if(iCounter == (TUint)iNumberOfFilters)
                            {
                            #ifdef _RUNL_DEBUG
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() calling User::RequestComplete() after filters are handled")));
                            #endif
                            User::RequestComplete(iCallerStatus, KErrNone);
                            #ifdef _RUNL_DEBUG
                            LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() STOP")));
                            #endif
                            return;
                            }
                        iTFTFetchSubState = ECopyRetrievedFilterFetchState;
                        iParent->iContext->ContextHandle().
                            GetPacketFilterInfo(iStatus,
                                                iCounter++,
                                                iFilterPtr);
                        iLastCancelCode = EPacketContextGetPacketFilterInfo;
                        iCancellable = ETrue;
                        SetActive();
                        return;
                        } // case ERetrieveFilterInfoFetchState:
                    } // switch(iTFTFetchSubState)
                break;
                } // case KDoTFTFetch:
            } // switch(iRequestState)
        } // if(iStatus != KErrNone)

    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextTFTOperation::RunL() STOP")));
    #endif
    }

void CEtelContextTFTOperation::DoTask(TContextParameters *aContextParameters)
    {
    iContextParameters = aContextParameters;
    *iCallerStatus= KRequestPending;

    iContextParameters->iReasonCode = KErrNone;

    if(iContextParameters->iTFTOperationCode == KDeleteTFT)
        {
        iParent->iContext->ContextHandle().EnumeratePacketFilters(iStatus,
            iNumberOfFilters);
        iLastCancelCode = EPacketContextEnumeratePacketFilters;
        iRequestState = KDoTFTTask;
        // Initial state for TFT removal
        iTFTDeleteSubRequestState = ERetrieveFilterInfoState; 
        iCancellable = ETrue;
        SetActive();    
        return;
        }
    else
        {
        iNumberOfFilters = -1; // Needed?
        iContextParameters->iContextConfig.GetTFTInfo(iTFTInfo);
        iCancellable = EFalse;
        JumpToRunl(0);
        return;
        }
    }

/* 
 *  Set QoS request group
 *  Wraps Etel-sequence:
 *  RPacketQoS::SetProfileParameters
 */        
CEtelSetQoS::CEtelSetQoS() 
    : iUMTSQoSReqPtr(iUMTSQoSReq)
    {    
    }

void CEtelSetQoS::RunL()
    {
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelSetQoS::RunL() START")));
#endif
    if(iParent->iContext->Nif().ControlOption() == KContextQoSSetFailAsync)
        {
        iStatus = KErrTest;
        }
    if(iStatus != KErrNone) // In case something fails
        {        
        iContextParameters->iReasonCode = iStatus.Int();
		#ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelSetQoS::RunL() calling User::RequestComplete() die to failure")));
		#endif
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else
        {
		#ifdef _RUNL_DEBUG
        LOG(Log::Printf(
            _L("CEtelSetQoS::RunL() calling User::RequestComplete()")));
		#endif
        User::RequestComplete(iCallerStatus,KErrNone);
        }
	#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelSetQoS::RunL() STOP")));
	#endif
    }

void CEtelSetQoS::DoTask(TContextParameters *aContextParameters)
    {    
    iContextParameters = aContextParameters;    
    *iCallerStatus= KRequestPending;

    iContextParameters->iReasonCode = KErrNone;

    iContextParameters->iContextConfig.GetUMTSQoSReq(iUMTSQoSReq);

    iParent->iContext->ContextQoS().SetProfileParameters(iStatus, iUMTSQoSReqPtr);
    iLastCancelCode = EPacketQoSSetProfileParams;

    SetActive();

    return;
    }

/* 
 *  Activate context request group
 *  Wraps:
 *        RPacketContext::Activate
 */
CEtelContextActivate::CEtelContextActivate() 
    : iUMTSConfigPtr(iUMTSConfig),
      iUMTSQoSNegPtr(iUMTSQoSNeg)
    {
    }

void CEtelContextActivate::RunL()
    {
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextActivate::RunL() START")));
#endif

	if(iParent->iContext->Nif().ControlOption() == KContextActivateFailAsync)
        {
        iStatus = KErrTest;
        }
        
    // When KContextActivateRejectSBLP is set by TS_QoS, 
    // the following if-statement will set KErrGprsUserAuthenticationFailure
    // The PCO buffer is filled by method
    // CEtelRequest::HandleActivateRequestResponse()
    if(iParent->iContext->Nif().ControlOption() == KContextActivateRejectSBLP)
        {
        iStatus = KErrGprsUserAuthenticationFailure;    
        }
        

    if(iStatus != KErrNone) // In case something fails
        {
        iContextParameters->iReasonCode = iStatus.Int();
        #ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextActivate::RunL() calling User::RequestComplete() due to failure")));
        #endif                    
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else
        {
        switch(iRequestState)
            {    
            case EGetConfigState:
                {
                iRequestState = EGetQoSState; 
                iContextParameters->iContextConfig.SetContextConfig(iUMTSConfig);
                iParent->iContext->ContextQoS().GetProfileParameters(iStatus,iUMTSQoSNegPtr);

                iLastCancelCode = EPacketQoSGetProfileParams;

                SetActive();
                break;
                }
            case EGetQoSState:
                {
                // Set QoS in Nif
                iContextParameters->iContextConfig.SetUMTSQoSNeg(iUMTSQoSNeg);
                // Set QoS in Nif
                iParent->iContext->Parameters().iContextConfig.SetUMTSQoSNeg(iUMTSQoSNeg);
                // Get the status, should be Active
                iParent->iContext->ContextHandle().GetStatus(iContextStatus);
                // Set status in Nif
                iParent->iContext->SetStatus(iContextStatus);
                #ifdef _RUNL_DEBUG
                LOG(Log::Printf(_L("CEtelContextActivate::RunL() calling User::RequestComplete()")));
                #endif                                
                User::RequestComplete(iCallerStatus,KErrNone);        
                break;
                }
            }
        }
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextActivate::RunL() STOP")));
#endif
    }

void CEtelContextActivate::DoTask(TContextParameters *aContextParameters)
    {    
    iContextParameters = aContextParameters;
    *iCallerStatus= KRequestPending;    

    iContextParameters->iReasonCode = KErrNone;

    iParent->iContext->ContextHandle().Activate(iStatus);
    iLastCancelCode = EPacketContextActivate;

    SetActive();

    return;
    }

/*  
 *  Modify active context request group
 *  Wraps:
 *        RPacketContext::ModifyActiveContext
 */
CEtelContextModifyActive::CEtelContextModifyActive()
    {
    }

void CEtelContextModifyActive::RunL()
    {
    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextModifyActive::RunL() START")));
    #endif
    if(iParent->iContext->Nif().ControlOption() == KContextModifyActiveFailAsync)
        {
        iStatus = KErrTest;
        }

    // When KContextActivateRejectSBLP is set by TS_QoS, 
    // the following if-statement will set KErrGprsUserAuthenticationFailure
    // The PCO buffer is filled by method
    // CEtelRequest::HandleActivateRequestResponse()
    if(iParent->iContext->Nif().ControlOption() == KContextActivateRejectSBLP)
        {
        iStatus = KErrGprsUserAuthenticationFailure;    
        }

    if(iStatus != KErrNone) // In case something fails
        {        
        iContextParameters->iReasonCode = iStatus.Int();
        #ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextModifyActive::RunL() calling User::RequestComplete() due to failure")));
        #endif                                
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else
        {
        #ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextModifyActive::RunL() calling User::RequestComplete()")));
        #endif                                
        User::RequestComplete(iCallerStatus,KErrNone);
        }
    #ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextModifyActive::RunL() STOP")));
    #endif
    return;
    }

void CEtelContextModifyActive::DoTask(TContextParameters *aContextParameters)
    {    
    iContextParameters = aContextParameters;    
    *iCallerStatus= KRequestPending;

    iContextParameters->iReasonCode = KErrNone;

    iParent->iContext->ContextHandle().ModifyActiveContext(iStatus);
    iLastCancelCode = EPacketContextModifyActiveContext;
    SetActive();

    return;
    }

void CEtelContextStartupCreate::RunL()
    {
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextStartupCreate::RunL() START")));
#endif
    if(iStatus != KErrNone) // In case something fails
        {
        iContextParameters->iReasonCode = iStatus.Int();
#ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextStartupCreate::RunL() calling \
User::RequestComplete() due to failure")));
#endif
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else
        {
        switch(iRequestState)
            {
            case EInitializeContextState:
                {
                iRequestState  = ESetConfigState;
                iContextParameters->iContextConfig.GetContextConfig(
                    iUMTSConfig);

                iParent->iContext->ContextHandle().SetConfig(iStatus,
                    iUMTSConfigPtr);
                iLastCancelCode = EPacketContextSetConfig;
                SetActive(); 
                break;
                }
            case ESetConfigState:
                {                        
                iRequestState = ESetQoSState; 
                iContextParameters->iContextConfig.GetUMTSQoSReq(
                    iUMTSQoSReq);
                iParent->iContext->ContextQoS().SetProfileParameters(iStatus,
                    iUMTSQoSReqPtr);

                iLastCancelCode = EPacketQoSSetProfileParams;

                SetActive();
                break;
                }
            case ESetQoSState:
                {
                iRequestState = EActivateState; 
                iParent->iContext->ContextHandle().GetConfig(iStatus,
                                                             iUMTSConfigPtr);

                iLastCancelCode = EPacketContextGetConfig;

                SetActive();
                break;
                }
            case EActivateState:
                {
                iRequestState = EGetConfigState; 

                iContextParameters->iContextConfig.SetContextConfig(
                    iUMTSConfig);
                iParent->iContext->ContextHandle().Activate(iStatus);

                iLastCancelCode = EPacketContextActivate;

                SetActive();
                break;
                }
            case EGetConfigState:
                {
                iRequestState = EGetQoSState; 
                iContextParameters->iContextConfig.SetContextConfig(
                    iUMTSConfig);
                iParent->iContext->ContextQoS().GetProfileParameters(iStatus,
                    iUMTSQoSNegPtr);
                iLastCancelCode = EPacketQoSGetProfileParams;

                SetActive();
                break;
                }
            case EGetQoSState:
                {
                // Set QoS in Nif
                iContextParameters->iContextConfig.SetUMTSQoSNeg(
                    iUMTSQoSNeg);
                // Set QoS in Nif
                iParent->iContext->Parameters().iContextConfig.SetUMTSQoSNeg(
                    iUMTSQoSNeg); 
                // Get the status, should be Active
                iParent->iContext->ContextHandle().GetStatus(iContextStatus);
                // Set status in Nif
                iParent->iContext->SetStatus(iContextStatus);
#ifdef _RUNL_DEBUG
                LOG(Log::Printf(_L("CEtelContextStartupCreate::RunL() \
calling User::RequestComplete()")));
#endif                                                            
                User::RequestComplete(iCallerStatus,KErrNone);        
                break;
                }
            default:
                {
                __ASSERT_DEBUG(0, User::Panic(_L("Invalid state"), 0));
                break;
                }
            }
        }
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextStartupCreate::RunL() STOP")));
#endif    
    return;
    }

CEtelContextStartupCreate::CEtelContextStartupCreate() 
    : iUMTSConfigPtr(iUMTSConfig),
      iUMTSQoSReqPtr(iUMTSQoSReq),
      iUMTSQoSNegPtr(iUMTSQoSNeg)
    {    
    }

void CEtelContextStartupCreate::DoTask(TContextParameters 
    *aContextParameters)
    {
    iContextParameters = aContextParameters;
    iRequestState = EInitializeContextState;
    *iCallerStatus= KRequestPending;

    iContextParameters->iReasonCode = KErrNone;

    iParent->iContext->ContextHandle().InitialiseContext(iStatus,
        iGPDSContextIdPtr);
    iLastCancelCode = EPacketContextInitialiseContext;
    SetActive(); // Wait for asynchronous request to complete..

    return;
    }

void CEtelContextCreateSecondPhase::RunL()
    {    
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextCreateSecondPhase::RunL() START")));
#endif
    if(iParent->iContext->Nif().ControlOption() == KContextCreateFailAsync)
        {
        iStatus = KErrTest;
        }
    if(iStatus != KErrNone) // In case something fails
        {
        iContextParameters->iReasonCode = iStatus.Int();
#ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextCreateSecondPhase::RunL() calling \
User::RequestComplete() due to failure")));
#endif
        User::RequestComplete(iCallerStatus,iStatus.Int());
        }
    else
        {
#ifdef _RUNL_DEBUG
        LOG(Log::Printf(_L("CEtelContextCreateSecondPhase::RunL() calling \
User::RequestComplete()")));
#endif
        // Secondary context created, write notification to test application
        if(iParent->iContext->Nif().ControlOption() == 
            KNotifySecondaryCreated)
            {
            iParent->iContext->Nif().SecondaryContextCreated();
            }
        User::RequestComplete(iCallerStatus,KErrNone);
        }
#ifdef _RUNL_DEBUG
    LOG(Log::Printf(_L("CEtelContextCreateSecondPhase::RunL() STOP")));
#endif
    }

CEtelContextCreateSecondPhase::CEtelContextCreateSecondPhase()
    {
    }

void CEtelContextCreateSecondPhase::DoTask(TContextParameters 
    *aContextParameters)
    {
    iContextParameters = aContextParameters;
    *iCallerStatus= KRequestPending;

    iContextParameters->iReasonCode = KErrNone;

    iParent->iContext->ContextHandle().InitialiseContext(iStatus,
        iGPDSContextIdPtr); 
    iLastCancelCode = EPacketContextInitialiseContext;
    SetActive(); // Wait for asynchronous request to complete..

    return;
    }

/* 
 *  Logger method
 */
#ifdef LOG

void CEtelRequest::PrintEvent(TUint aEventCode, const TContextParameters& 
    aParameters)
    {
    LOG(Log::Printf(_L("************** BEGIN EVENT ***************")));
    PrintEventType(aEventCode,aParameters);
    PrintContextInfo(aParameters.iContextInfo);
    PrintContextConfig(aParameters.iContextConfig);
    PrintContextNegotiatedQoS(aEventCode,aParameters);
    PrintContextTFT(aEventCode,aParameters);
    LOG(Log::Printf(_L("*************** END EVENT ****************")));
    }

void CEtelRequest::PrintEventType(TUint aEventCode, 
                                  const TContextParameters& aParameters)
    {
    switch(aEventCode)
        {
        case KPrimaryContextCreated:
            {
            LOG(Log::Printf(_L("Sending event KPrimaryContextCreated from \
<Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(),
                             aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextTFTModifiedEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextTFTModifiedEvent from \
<Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(),
                             aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextQoSSetEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextQoSSetEvent from \
<Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(), 
                             aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextActivateEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextActivateEvent from \
<Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(), 
                             aParameters.iContextInfo.iContextId));
            break;
            }
        case KContextParametersChangeEvent:
            {
            LOG(Log::Printf(_L("Sending event KContextParametersChangeEvent \
from <Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(), 
                                  aParameters.iContextInfo.iContextId));
            break;
            }
        case KSecondaryContextCreated:
            {
            LOG(Log::Printf(_L("Sending event KSecondaryContextCreated from \
<Nif>/<Context> <%s>/<%d>"), iContext->Nif().Name().PtrZ(),
                             aParameters.iContextInfo.iContextId));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("Sending event UNRECOGNIZED")));    
            break;
            }
        }
    // Following if-statement added only to get rid of WINSCW UREL-warning:
    // variable / argument 'aParameters' is not used in function
    if (aParameters.iContextInfo.iContextId < 0)
        {
        LOG(Log::Printf(_L("Negative ContextId")));
        }
    }

void CEtelRequest::PrintContextInfo(const TContextInfo& aContextInfo)
    {
    LOG(Log::Printf(_L("** Context status data **")));
    switch(aContextInfo.iStatus)
        {
        case RPacketContext::EStatusUnknown:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusUnknown")));
            break;
            }
        case RPacketContext::EStatusInactive:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusInactive")));
            break;
            }
        case RPacketContext::EStatusActivating:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusActivating")));
            break;
            }
        case RPacketContext::EStatusActive:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusActive")));
            break;
            }
        case RPacketContext::EStatusDeactivating:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusDeactivating")));
            break;
            }
        case RPacketContext::EStatusSuspended:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusSuspended")));
            break;
            }
        case RPacketContext::EStatusDeleted:
            {
            LOG(Log::Printf(_L("\tContext status: EStatusDeleting")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tContext status: UNRECOGNIZED")));
            break;
            }
        }
    }

void CEtelRequest::PrintContextConfig(const TContextConfig& aContextConfig)
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

void CEtelRequest::PrintContextNegotiatedQoS(TUint /*aEventCode*/, 
    const TContextParameters& aParameters)
    {
    LOG(Log::Printf(_L("** Context QoS data **")));

    RPacketQoS::TQoSR5Negotiated tempQoS;
    aParameters.iContextConfig.GetUMTSQoSNeg(tempQoS);

    switch(tempQoS.iTrafficClass)
        {
        case RPacketQoS::ETrafficClassUnspecified:
            {
            LOG(Log::Printf(
                _L("\tTraffic Class: ETrafficClassUnspecified")));
            break;
            }
        case RPacketQoS::ETrafficClassConversational:
            {
            LOG(Log::Printf(
                _L("\tTraffic Class: ETrafficClassConversational")));
            break;
            }
        case RPacketQoS::ETrafficClassStreaming:
            {
            LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassStreaming")));
            break;
            }
        case RPacketQoS::ETrafficClassInteractive:
            {
            LOG(Log::Printf(
                _L("\tTraffic Class: ETrafficClassInteractive")));
            break;
            }
        case RPacketQoS::ETrafficClassBackground:
            {
            LOG(Log::Printf(_L("\tTraffic Class: ETrafficClassBackground")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tTraffic Class: UNRECOGNIZED")));
            break;
            }
        }
    switch(tempQoS.iDeliveryOrderReqd)
        {
        case RPacketQoS::EDeliveryOrderUnspecified:
            {
            LOG(Log::Printf(
                _L("\tDelivery order: EDeliveryOrderUnspecified")));
            break;
            }
        case RPacketQoS::EDeliveryOrderRequired:
            {
            LOG(Log::Printf(_L("\tDelivery order: EDeliveryOrderRequired")));
            break;
            }
        case RPacketQoS::EDeliveryOrderNotRequired:
            {
            LOG(Log::Printf(
                _L("\tDelivery order: EDeliveryOrderNotRequired")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tDelivery order: UNRECOGNIZED")));
            break;
            }
        }
    switch(tempQoS.iDeliverErroneousSDU)
        {
        case RPacketQoS::EErroneousSDUDeliveryUnspecified:
            {
            LOG(Log::Printf(_L("\tDelivery of erroneous: \
EErroneousSDUDeliveryUnspecified")));
            break;
            }
        case RPacketQoS::EErroneousSDUDeliveryRequired:
            {
            LOG(Log::Printf(_L
                ("\tDelivery of erroneous: EErroneousSDUDeliveryRequired")));
            break;
            }
        case RPacketQoS::EErroneousSDUDeliveryNotRequired:
            {
            LOG(Log::Printf(_L("\tDelivery of erroneous: \
EErroneousSDUDeliveryNotRequired")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tDelivery of erroneous: UNRECOGNIZED")));
            break;
            }
        }

    LOG(Log::Printf(_L("\tMaximum SDU size: %d"),tempQoS.iMaxSDUSize));
    LOG(Log::Printf(_L("\tMaximum data rate uplink: %d"),
        tempQoS.iMaxRate.iUplinkRate));
    LOG(Log::Printf(_L("\tMaximum data rate downlink: %d"),
        tempQoS.iMaxRate.iDownlinkRate));
    LOG(Log::Printf(_L("\tGuaranteed data rate uplink: %d"),
        tempQoS.iGuaranteedRate.iUplinkRate));
    LOG(Log::Printf(_L("\tGuaranteed data rate downlink: %d"),
        tempQoS.iGuaranteedRate.iDownlinkRate));

    switch(tempQoS.iBER)
        {
        case RPacketQoS::EBERUnspecified:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBERUnspecified")));
            break;
            }
        case RPacketQoS::EBERFivePerHundred:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBERFivePerHundred")));
            break;
            }
        case RPacketQoS::EBEROnePerHundred:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerHundred")));
            break;
            }
        case RPacketQoS::EBERFivePerThousand:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBERFivePerThousand")));
            break;
            }
        case RPacketQoS::EBERFourPerThousand:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBERFourPerThousand")));
            break;
            }
        case RPacketQoS::EBEROnePerThousand:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerThousand")));
            break;
            }
        case RPacketQoS::EBEROnePerTenThousand:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerTenThousand")));
            break;
            }
        case RPacketQoS::EBEROnePerHundredThousand:
            {
            LOG(Log::Printf(
                _L("\tBit Error Ratio: EBEROnePerHundredThousand")));
            break;
            }
        case RPacketQoS::EBEROnePerMillion:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: EBEROnePerMillion")));
            break;
            }
        case RPacketQoS::EBERSixPerHundredMillion:
            {
            LOG(Log::Printf(
                _L("\tBit Error Ratio: EBERSixPerHundredMillion")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tBit Error Ratio: UNRECOGNIZED")));
            break;
            }
        }

    switch(tempQoS.iSDUErrorRatio)
        {
        case RPacketQoS::ESDUErrorRatioUnspecified:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioUnspecified")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerTen:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioOnePerTen")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerHundred:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioOnePerHundred")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioSevenPerThousand:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioSevenPerThousand")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerThousand:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioOnePerThousand")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerTenThousand:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioOnePerTenThousand")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerHundredThousand:
            {
            LOG(Log::Printf(_L
                ("\tSDU Error Ratio: ESDUErrorRatioOnePerHundredThousand")));
            break;
            }
        case RPacketQoS::ESDUErrorRatioOnePerMillion:
            {
            LOG(Log::Printf(
                _L("\tSDU Error Ratio: ESDUErrorRatioOnePerMillion")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tSDU Error Ratio: UNRECOGNIZED")));
            break;
            }
        }
    switch(tempQoS.iTrafficHandlingPriority)
        {
        case RPacketQoS::ETrafficPriorityUnspecified:
            {
            LOG(Log::Printf(_L("\tTraffic handling priority: \
ETrafficPriorityUnspecified")));
            break;
            }
        case RPacketQoS::ETrafficPriority1:
            {
            LOG(Log::Printf(
                _L("\tTraffic handling priority: ETrafficPriority1")));
            break;
            }
        case RPacketQoS::ETrafficPriority2:
            {
            LOG(Log::Printf(
                _L("\tTraffic handling priority: ETrafficPriority2")));
            break;
            }
        case RPacketQoS::ETrafficPriority3:
            {
            LOG(Log::Printf(
                _L("\tTraffic handling priority: ETrafficPriority3")));
            break;
            }
        default:
            {
            LOG(Log::Printf(
                _L("\tTraffic handling priority: UNRECOGNIZED")));
            break;
            }
        }
    LOG(Log::Printf(_L("\tTransfer delay: %d"),tempQoS.iTransferDelay));
    
    /*
    	UMTS R5 QoS parameters
    */
     switch(tempQoS.iSignallingIndication)
        {
        case ETrue:
            {
            LOG(Log::Printf(_L("\tSignalling Indicator: ETrue")));
            break;
            }
        case EFalse:
            {
            LOG(Log::Printf(_L("\tSignalling Indicator: EFalse")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tSignalling Indicator: UNRECOGNIZED")));
            break;
            }
        }
    switch(tempQoS.iSourceStatisticsDescriptor)
        {
        case RPacketQoS::ESourceStatisticsDescriptorUnknown:
            {
            LOG(Log::Printf(_L("\tSource Statistics Descriptor: ESourceStatisticsDescriptorUnknown")));
            break;
            }
        case RPacketQoS::ESourceStatisticsDescriptorSpeech:
            {
            LOG(Log::Printf(_L("\tSource Statistics Descriptor: ESourceStatisticsDescriptorSpeech")));
            break;
            }
        default:
            {
            LOG(Log::Printf(_L("\tSource Statistics Descriptor: UNRECOGNIZED")));
            break;
            }
        }
    }

void CEtelRequest::PrintContextTFT(TUint /*aEventCode*/, 
    const TContextParameters& aParameters)    
    {
    TTFTInfo tempTFT;

    aParameters.iContextConfig.GetTFTInfo(tempTFT);

    LOG(Log::Printf(_L("** Context TFT data **")));

    LOG(Log::Printf(_L("\t Number of filters: %d"),tempTFT.FilterCount()));

    for(TUint Counter = 0; Counter < tempTFT.FilterCount(); Counter++)
        {
        RPacketContext::TPacketFilterV2 tempFilter;
        tempFilter.iId = Counter+1;

        tempTFT.GetPacketFilter(tempFilter);

        LOG(Log::Printf(_L("\t\tFilter:")));
        LOG(Log::Printf(_L("\t\t\tId: <%d>"),tempFilter.iId));
        LOG(Log::Printf(_L("\t\t\tEvaluation precedence: <%d>"),
            tempFilter.iEvaluationPrecedenceIndex));
        
        LOG(Log::Printf(_L("\t\t\tIP:   \
<%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d>"),
                        tempFilter.iSrcAddr[0],
                        tempFilter.iSrcAddr[1],
                        tempFilter.iSrcAddr[2],
                        tempFilter.iSrcAddr[3],
                        tempFilter.iSrcAddr[4],
                        tempFilter.iSrcAddr[5],
                        tempFilter.iSrcAddr[6],
                        tempFilter.iSrcAddr[7],
                        tempFilter.iSrcAddr[8],
                        tempFilter.iSrcAddr[9],
                        tempFilter.iSrcAddr[10],
                        tempFilter.iSrcAddr[11],
                        tempFilter.iSrcAddr[12],
                        tempFilter.iSrcAddr[13],
                        tempFilter.iSrcAddr[14],
                        tempFilter.iSrcAddr[15]));
        LOG(Log::Printf(_L("\t\t\tMask: \
<%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d.%d>"),
                        tempFilter.iSrcAddrSubnetMask[0],
                        tempFilter.iSrcAddrSubnetMask[1],
                        tempFilter.iSrcAddrSubnetMask[2],
                        tempFilter.iSrcAddrSubnetMask[3],
                        tempFilter.iSrcAddrSubnetMask[4],
                        tempFilter.iSrcAddrSubnetMask[5],
                        tempFilter.iSrcAddrSubnetMask[6],
                        tempFilter.iSrcAddrSubnetMask[7],
                        tempFilter.iSrcAddrSubnetMask[8],
                        tempFilter.iSrcAddrSubnetMask[9],
                        tempFilter.iSrcAddrSubnetMask[10],
                        tempFilter.iSrcAddrSubnetMask[11],
                        tempFilter.iSrcAddrSubnetMask[12],
                        tempFilter.iSrcAddrSubnetMask[13],
                        tempFilter.iSrcAddrSubnetMask[14],
                        tempFilter.iSrcAddrSubnetMask[15]));
        
        LOG(Log::Printf(_L("\t\t\tProtocol number/Next Header: <%d>"),
            tempFilter.iProtocolNumberOrNextHeader));
        LOG(Log::Printf(_L("\t\t\tSource port Min: <%d>"), 
            tempFilter.iSrcPortMin));
        LOG(Log::Printf(_L("\t\t\tSource port Max: <%d>"), 
            tempFilter.iSrcPortMax));
        
        LOG(Log::Printf(_L("\t\t\tDestination port Min: <%d>"), 
            tempFilter.iDestPortMin));
        LOG(Log::Printf(_L("\t\t\tDestination port Max: <%d>"), 
            tempFilter.iDestPortMax));
        
        LOG(Log::Printf(_L("\t\t\tSPI: <%d>"),tempFilter.iIPSecSPI));
        LOG(Log::Printf(_L("\t\t\tTOS: <%d>"),
            tempFilter.iTOSorTrafficClass));
        LOG(Log::Printf(_L("\t\t\tFlow label: <%d>"), 
            tempFilter.iFlowLabel));        
        }
    
    /*
    	SBLP
    */
    LOG(Log::Printf(_L("** SBLP data **")));
    RPacketContext::CTFTMediaAuthorizationV3 *sblpParams;
    sblpParams = RPacketContext::CTFTMediaAuthorizationV3::NewL();
    tempTFT.GetSblpToken(*sblpParams);
    TBuf<255> mat;
    mat.Copy(sblpParams->iAuthorizationToken);
    LOG(Log::Printf(_L("\tMedia Authorization Token: %S"),&mat));
    
    TInt i;
    for(i=0; i<sblpParams->iFlowIds.Count();i++)
        {
        LOG(Log::Printf(_L("\tFlow Id               : <%d>"),
            sblpParams->iFlowIds.Count()));
        LOG(Log::Printf(_L("\tMedia Component Number: <%d>"),
         sblpParams->iFlowIds[i].iMediaComponentNumber));
        LOG(Log::Printf(_L("\tFlow Number           : <%d>"),
            sblpParams->iFlowIds[i].iIPFlowNumber));
        }

    
    /*
    	IMS flag value
    */    
    LOG(Log::Printf(_L("** IMS data **")));
    TBool imsflag;
    tempTFT.GetIMCNSubsystemflag(imsflag);
    switch(imsflag)
		{
		case ETrue:
	    	{
	    	LOG(Log::Printf(_L("\tIM CN Flag: ETrue")));
	    	break;
	    	}
		case EFalse:
	    	{
	    	LOG(Log::Printf(_L("\tIM CN Flag: EFalse")));
	    	break;
	    	}
		default:
	    	{
	    	LOG(Log::Printf(_L("\tIM CN Flag: UNRECOGNIZED")));
	    	break;
	    	}    
		}

    }
#endif

