// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "NifPdpTsy.h"
#include "QosTestNcp.h"
#include "QoSTestLog.h"
#include <PcktCs.H>


//
// CNifPDPContextTsy Class
//
CNifPDPContextTsy* CNifPDPContextTsy::NewL(CQoSTestNcp* aNif, TDes8& aConfig)
	{
	CNifPDPContextTsy* self = NewLC(aNif, aConfig);
	CleanupStack::Pop();
	return self;
	};
CNifPDPContextTsy* CNifPDPContextTsy::NewLC(CQoSTestNcp* aNif, TDes8& aConfig)
	{
	CNifPDPContextTsy* self = new(ELeave) CNifPDPContextTsy(aNif, aConfig);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};

CNifPDPContextTsy::CNifPDPContextTsy(CQoSTestNcp* aNif, TDes8& aConfig)
	:CNifPDPContextBase(aNif, aConfig)
	{
	};
CNifPDPContextTsy::~CNifPDPContextTsy()
	{
	};
void CNifPDPContextTsy::ConstructL()
	{
	iDataChannelPckg = new (ELeave) RPacketContext::TDataChannelV2Pckg(iDataChannel);
	iRel99ContextConfigPckg = new (ELeave) TPckg<RPacketContext::TContextConfigGPRS>(iRel99ContextConfig);
#ifdef SYMBIAN_NETWORKING_UMTSR5  
	iReqR5QosPckg = new (ELeave) TPckg<RPacketQoS::TQoSR5Requested>(iReqR5Qos);
	iNegR5QosPckg = new (ELeave) TPckg<RPacketQoS::TQoSR5Negotiated>(iNegR5Qos);
#else
	iReqQosPckg = new (ELeave) TPckg<RPacketQoS::TQoSR99_R4Requested>(iReqQos);
	iNegQosPckg = new (ELeave) TPckg<RPacketQoS::TQoSR99_R4Negotiated>(iNegQos);
#endif
// SYMBIAN_NETWORKING_UMTSR5 


	iFilterPckg = new (ELeave) TPckg<RPacketContext::TPacketFilterV2>(iFilter);
	
	// init struct for initialize context 
	iRel99ContextConfig.iAccessPointName = _L8("SYMBIAN EMPLOYEE INTRANET");;//DPCKTTSY_ACCESS_POINT2;	
	iRel99ContextConfig.iPdpAddress = _L8("A WAP PORTAL");//DPCKTTSY_PDP_ADDRESS2;
	iRel99ContextConfig.iPdpType = RPacketContext::EPdpTypePPP;//SIM_PACKET_PROTOCOL;
	iRel99ContextConfig.iUseEdge = ETrue;
	iRel99ContextConfig.iPdpCompression = 0x0fffffff;

	// Set QoS Profile Params
	//0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 
	//0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02, 0x08, 0x04, 0x02, 0x04, 0x02
#ifdef SYMBIAN_NETWORKING_UMTSR5
	iReqR5Qos.iReqTrafficClass                 = (RPacketQoS::TTrafficClass)( 4);
	iReqR5Qos.iMinTrafficClass					= (RPacketQoS::TTrafficClass)2;	
	iReqR5Qos.iReqDeliveryOrderReqd            = (RPacketQoS::TDeliveryOrder)( 4);
	iReqR5Qos.iMinDeliveryOrderReqd			= (RPacketQoS::TDeliveryOrder)2;
	iReqR5Qos.iReqDeliverErroneousSDU          = (RPacketQoS::TErroneousSDUDelivery)(8 );
	iReqR5Qos.iMinDeliverErroneousSDU			= (RPacketQoS::TErroneousSDUDelivery)4;	
	iReqR5Qos.iReqMaxSDUSize                   = (1500 );
	iReqR5Qos.iMinAcceptableMaxSDUSize			= 100;
	iReqR5Qos.iReqMaxRate.iUplinkRate          = ( 1000);
	iReqR5Qos.iReqMaxRate.iDownlinkRate		= 1000;	
	iReqR5Qos.iMinAcceptableMaxRate.iUplinkRate = ( 500);
	iReqR5Qos.iMinAcceptableMaxRate.iDownlinkRate = 500;
	iReqR5Qos.iReqBER                          = (RPacketQoS::TBitErrorRatio)(4 );
	iReqR5Qos.iMaxBER							= (RPacketQoS::TBitErrorRatio)2;
	iReqR5Qos.iReqSDUErrorRatio                = (RPacketQoS::TSDUErrorRatio)(8 );
	iReqR5Qos.iMaxSDUErrorRatio				= (RPacketQoS::TSDUErrorRatio)4;
	iReqR5Qos.iReqTrafficHandlingPriority      = (RPacketQoS::TTrafficHandlingPriority)(2 );
	iReqR5Qos.iMinTrafficHandlingPriority		= (RPacketQoS::TTrafficHandlingPriority)4;	
	iReqR5Qos.iReqTransferDelay                = (1000 );
	iReqR5Qos.iMaxTransferDelay				= 100;
	iReqR5Qos.iReqGuaranteedRate.iUplinkRate   = (1000 );
	iReqR5Qos.iReqGuaranteedRate.iDownlinkRate	= 1000;
	iReqR5Qos.iMinGuaranteedRate.iUplinkRate	= (500 );
	iReqR5Qos.iMinGuaranteedRate.iDownlinkRate	= 500;

	iReqR5Qos.iSourceStatisticsDescriptor  = (RPacketQoS::TSourceStatisticsDescriptor)(1);
	iReqR5Qos.iSignallingIndication        = 0 ; 
#else	
	iReqQos.iReqTrafficClass                 = (RPacketQoS::TTrafficClass)( 4);
	iReqQos.iMinTrafficClass					= (RPacketQoS::TTrafficClass)2;	
	iReqQos.iReqDeliveryOrderReqd            = (RPacketQoS::TDeliveryOrder)( 4);
	iReqQos.iMinDeliveryOrderReqd			= (RPacketQoS::TDeliveryOrder)2;
	iReqQos.iReqDeliverErroneousSDU          = (RPacketQoS::TErroneousSDUDelivery)(8 );
	iReqQos.iMinDeliverErroneousSDU			= (RPacketQoS::TErroneousSDUDelivery)4;	
	iReqQos.iReqMaxSDUSize                   = (1500 );
	iReqQos.iMinAcceptableMaxSDUSize			= 100;
	iReqQos.iReqMaxRate.iUplinkRate          = ( 1000);
	iReqQos.iReqMaxRate.iDownlinkRate		= 1000;	
	iReqQos.iMinAcceptableMaxRate.iUplinkRate = ( 500);
	iReqQos.iMinAcceptableMaxRate.iDownlinkRate = 500;
	iReqQos.iReqBER                          = (RPacketQoS::TBitErrorRatio)(4 );
	iReqQos.iMaxBER							= (RPacketQoS::TBitErrorRatio)2;
	iReqQos.iReqSDUErrorRatio                = (RPacketQoS::TSDUErrorRatio)(8 );
	iReqQos.iMaxSDUErrorRatio				= (RPacketQoS::TSDUErrorRatio)4;
	iReqQos.iReqTrafficHandlingPriority      = (RPacketQoS::TTrafficHandlingPriority)(2 );
	iReqQos.iMinTrafficHandlingPriority		= (RPacketQoS::TTrafficHandlingPriority)4;	
	iReqQos.iReqTransferDelay                = (1000 );
	iReqQos.iMaxTransferDelay				= 100;
	iReqQos.iReqGuaranteedRate.iUplinkRate   = (1000 );
	iReqQos.iReqGuaranteedRate.iDownlinkRate	= 1000;
	iReqQos.iMinGuaranteedRate.iUplinkRate	= (500 );
	iReqQos.iMinGuaranteedRate.iDownlinkRate	= 500;
#endif 
// SYMBIAN_NETWORKING_UMTSR5 
			
	
	// Instantiate the Qos & PDP monitor
	iQosMonitor = CNifQosMonitorTsy::NewL(this);
	iContextMonitor = CNifPDPMonitorTsy::NewL(this);

	CActiveScheduler::Add(this);
	};
void CNifPDPContextTsy::DoCancel()
	{
	//Find out the outstanding request.
	if (iSubCommand!=ECommandNone)
	{ 
		switch(iSubCommand)
			{
		//	case EInitialise:
		//		break;
			case ESetConfig:
				iContext.CancelAsyncRequest(EPacketContextSetConfigCancel);
				break;
			case ESetQos:
				iQos.CancelAsyncRequest(EPacketQoSSetProfileParams);
				break;	
			case EActivation:
				iContext.CancelAsyncRequest(EPacketContextActivateCancel);
				break;
			case EGetQosProfile:
				iQos.CancelAsyncRequest(EPacketQoSGetProfileParams);
				break;
			case ELoanCommport:
				iContext.CancelAsyncRequest(EPacketContextLoanCommPortCancel);
				break;
			case EDeletion:
				iContext.CancelAsyncRequest(EPacketContextDeleteCancel);
				break;
			case EDeactivation:
				iContext.CancelAsyncRequest(EPacketContextDeactivateCancel);
				break;
			case EAddTFTFilter:
				iContext.CancelAsyncRequest(EPacketContextAddPacketFilterCancel);
				break;
			case ERemoveTFTFilter:
				iContext.CancelAsyncRequest(EPacketContextRemovePacketFilterCancel);
				break;
			case EModifyActivation:
				iContext.CancelAsyncRequest(EPacketContextModifyActiveContextCancel);
			default:;
			}
		}
	if (iContextMonitor)
		iContextMonitor->Cancel();
	if (iQosMonitor)
		iQosMonitor->Cancel();
	};

void CNifPDPContextTsy::ActivatePrimaryContext(TDes8& /*aConfig*/)
	{

	iCommand=KStartupPrimaryContextCreation;
	// Get the packet service handler and open a new context.
	
	RPacketService& packetNetwork=iNif->PacketService();

	// TODO:We assume the context creation is sync call because of etel.
	// GuQos has two events:KSecondaryContextCreated and KPrimaryContextCreated.
	// seems that GuQos want it async.???
	// Create the Primary using ETEL packet API.
	iContext.OpenNewContext(packetNetwork, iContextName);
	
	//TDesC& primaryName=iNif->FirstExistingPDPContext()->ContextName();
	//TBuf<20> primaryName=iNif->FirstExistingPDPContext()->ContextName();
	//LOG(PdpLog::Write(primaryName);)
	
	// get the initialise parameter or just hard coded them 
	LOG(_LIT(string1,"\n Pdp Primary Async Init");)
	LOG(PdpLog::Write(string1);)

	iContext.InitialiseContext(iStatus, *iDataChannelPckg);
	//User::WaitForRequest(iStatus);
	SetActive();
	iSubCommand=EInitialise; 
	};

void CNifPDPContextTsy::PdpPrimaryComplete()
	{

	LOG(_LIT(string0,"\n Pdp Primary Initialise context"));
	LOG(_LIT(string1,"\n Pdp Primary Qos Setting ");)
	LOG(_LIT(string2,"\n Pdp Primary Activating");)
	LOG(_LIT(string3,"\n Pdp Primary Getting Negotiated Qos");)
	LOG(_LIT(string5,"\n Pdp Primary is ready");)
	LOG(_LIT(string6,"\n Pdp Primary ERROR");)
	//LOG(_LIT(string7,"\n We Starting the Qos Monitor ");)	only in use when starting monitorint qos					
	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Write(string6));
		iParameter.iReasonCode=KErrGeneral;
		iNif->PrimaryContextReady();
		}
	else
		{
		switch(iSubCommand)
			{
	
			case EInitialise:
				// Set the Configuration for this primary context.
				LOG(PdpLog::Write(string0));
			
				iParameter.iContextConfig.SetContextConfig(iRel99ContextConfig);
				iContext.SetConfig(iStatus, *iRel99ContextConfigPckg);
				SetActive();
				iSubCommand=ESetConfig; 	
				break;

			case ESetConfig:
				// Set Qos for the Primary Pdp context
				LOG(PdpLog::Write(string1);)
				if (!iIsQosOpened)
					{ // Open a new qos.					
					iQos.OpenNewQoS(iContext, iQosProfileName);
					iIsQosOpened=ETrue;
					}
								
				// Extract Qos and pack it
				// iParameter.iContextConfig.GetUMTSQoSReq(qos);
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				iQos.SetProfileParameters(iStatus,*iReqR5QosPckg);
			#else
				iQos.SetProfileParameters(iStatus,*iReqQosPckg);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 
	
				//SetActive();
				//iQos.CancelAsyncRequest(EPacketQoSSetProfileParams);
				SetActive();
				iSubCommand=ESetQos;
			
				break;

			case ESetQos:
				// Activate the Primary Pdp context
				LOG(PdpLog::Write(string2);)
				iContext.Activate(iStatus);
	
				SetActive();
				iSubCommand=EActivation;	
				break;

			case EActivation:
				// Get the negotiated the qos
				iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
				LOG(PdpLog::Write(string3);)
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				iQos.GetProfileParameters(iStatus, *iNegR5QosPckg);
			#else
				iQos.GetProfileParameters(iStatus, *iNegQosPckg);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 

				SetActive();
				iSubCommand=EGetQosProfile;
				break;
					
			case EGetQosProfile:
				// Copy the data to the local copy
			#ifdef SYMBIAN_NETWORKING_UMTSR5 
				iParameter.iContextConfig.SetUMTSQoSNeg(iNegR5Qos);
			#else
				iParameter.iContextConfig.SetUMTSQoSNeg(iNegQos);
			#endif 
			// SYMBIAN_NETWORKING_UMTSR5 

				iParameter.iReasonCode=KErrNone;
				LOG(PdpLog::Write(string5);)
				
				iSubCommand=ECommandNone;
				iNif->PrimaryContextReady();		
				
				//iQosMonitor->StartToMonitor();
				//	LOG(PdpLog::Write(string7);)
		
				break;
			default:;
			}
		}
	};
TInt CNifPDPContextTsy::HandlePDPCreate(TDes8& aConfig)
	{
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	
	// This method is used to create secondary PDP context only.
	// Get the packet service handler and open a new context.
	
	RPacketService& packetNetwork=iNif->PacketService();

	// Create a secondary Pdp context, Set the same PDP Type 
	// as any of the existing ones.
	RPacketContext::TProtocolType pdpType;
	iNif->FirstExistingPDPContext()->ContextPDPType(pdpType);
	RPacketContext::TGSNAddress name;
	iNif->FirstExistingPDPContext()->ContextAPNName(name);

	// Update the local data.	
	iParameter.iContextConfig.SetPdpType(pdpType);
	iParameter.iContextType=ESecondaryContext;
	iParameter.iContextConfig.SetAccessPointName(name);

	// TODO:We assume the context creation is sync call because of etel.
	// GuQos has two events:KSecondaryContextCreated and KPrimaryContextCreated.
	// seems that GuQos want it async.???
	// Create the Primary using ETEL packet API.

	//TDesC& primaryName=iNif->FirstExistingPDPContext()->ContextName();
	TName primaryName=iNif->FirstExistingPDPContext()->ContextName();
	TInt ret = iContext.OpenNewSecondaryContext(packetNetwork, primaryName, iContextName);
	if (ret != KErrNone)
	{
		opt.iReasonCode = ret;	
		return KErrNone;
	}
	
	// Update the return data
	opt.iContextConfig.SetPdpType(pdpType);
	opt.iContextType=ESecondaryContext;
	opt.iContextConfig.SetAccessPointName(name);
	opt.iReasonCode = KErrNone;
	// we need to issue a InitialiseContext command so the TSY will open the PDP and create the Asunc Event 
	// get the initialise parameter or just hard coded them 
	LOG(_LIT(string1,"\n Pdp Secondary Async Init");)
	LOG(PdpLog::Write(string1);)
	
	
//	RPacketContext::TDataChannelV2Pckg iDataChannelPckg(iDataChannel);
//	iContext.InitialiseContext(iStatus,iDataChannelPckg);

	iContext.InitialiseContext(iStatus, *iDataChannelPckg);
	SetActive();
	iSubCommand=EInitialise; 

	return KErrNone;
	};

TInt CNifPDPContextTsy::HandlePDPDelete(TDes8& aConfig)
	{
	//TODO: confirm if we need deactivation before deletion.Now We do so.
	LOG(_LIT(string1,"\n Pdp Context %d is deactivating");)
	LOG(PdpLog::Printf(string1, ContextId());)	
	if ( iParameter.iContextInfo.iStatus == RPacketContext::EStatusActive )
	{
		iContext.Deactivate(iStatus);
		iSubCommand=EDeactivation;
	}
	else  
	{
		iContext.Delete(iStatus);
		iSubCommand=EDeletion;
	}
		
	// Update the return data
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	opt.iReasonCode = KErrNone;

	SetActive();
	return KErrNone;
	};
TInt CNifPDPContextTsy::HandleQosSet(TDes8& aConfig)
	{
	// Update local record.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	CopyQosReqParameter(opt);

	// Open a new Qos profile, if it does't exist.
	if (!iIsQosOpened)
		{		
		iQos.OpenNewQoS(iContext, iQosProfileName);
		iIsQosOpened=ETrue;
		}
	
	// Extract Qos and pack it
//	RPacketQoS::TQoSR99_R4Requested qos;
//	opt.iContextConfig.GetUMTSQoSReq(qos);
//	TPckg<RPacketQoS::TQoSR99_R4Requested>& qosPckg = *((TPckg<RPacketQoS::TQoSR99_R4Requested>*)&qos);

#ifdef SYMBIAN_NETWORKING_UMTSR5  
	opt.iContextConfig.GetUMTSQoSReq(iReqR5Qos);
	// Call ETel Configuration Setting API
	iQos.SetProfileParameters(iStatus,*iReqR5QosPckg);
#else
	opt.iContextConfig.GetUMTSQoSReq(iReqQos);
	// Call ETel Configuration Setting API
	iQos.SetProfileParameters(iStatus, *iReqQosPckg);
#endif 
// SYMBIAN_NETWORKING_UMTSR5 

	opt.iReasonCode = KErrNone;
	SetActive();
	iSubCommand=ESetQos;
	return KErrNone;
	};
TInt CNifPDPContextTsy::HandleTFTModify(TDes8& aConfig)
	{
	// Get the modified TFTFilter information from the parameter.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	// in case we need to delete all GUQos not sending us all the filter and  we neee to take it from iTFTInfo
	// and then remove and delete all are the same .
	if (opt.iTFTOperationCode == KDeleteTFT)
		iModifedTFTInfo.Set(iTFTInfo);
	else 
		opt.iContextConfig.GetTFTInfo(iModifedTFTInfo);
	iModifedTFTInfo.SetToFirst();
	iParameter.iTFTOperationCode=opt.iTFTOperationCode;
//	RPacketContext::TPacketFilterV2 filter;

	if (iModifedTFTInfo.NextPacketFilter(iFilter)==KErrNone)
		{
		//TPckg<RPacketContext::TPacketFilterV2>& filterPckg = *((TPckg<RPacketContext::TPacketFilterV2>*)&filter);

		switch (iParameter.iTFTOperationCode)
			{
		case KAddFilters:
			iCurrentfilter = iFilter ;
			iContext.AddPacketFilter(iStatus, *iFilterPckg);
			// check istatus 
			SetActive();
			iSubCommand=EAddTFTFilter;
			break;

		case KRemoveFilters:
		case KDeleteTFT:
			iCurrentfilter = iFilter ;
			iContext.RemovePacketFilter(iStatus, iFilter.iId);
			SetActive();
			iSubCommand=ERemoveTFTFilter;
			break;

		default:;
			}
		opt.iReasonCode = KErrNone;
		return KErrNone;
		}
	else		
		return KErrNotFound;
	};

TInt CNifPDPContextTsy::HandleContextActivate(TDes8& aConfig)
	{
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	User::After(4000000);
	// Activate the Pdp context
	if (!iIsQosOpened)
	{
		opt.iReasonCode = KErrNotReady;
		iSubCommand=ECommandNone;
		return KErrNone;
	}
	else 
	{
		iContext.Activate(iStatus);
		SetActive();
		//User::WaitForRequest(iStatus);
		opt.iReasonCode = KErrNone;
		iSubCommand=EActivation;
		return KErrNone;
	}
	};
TInt CNifPDPContextTsy::HandleModifyActive(TDes8& aConfig)
	{
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	iContext.ModifyActiveContext(iStatus);
	SetActive();
	opt.iReasonCode = KErrNone;
	iSubCommand=EModifyActivation;
	return KErrNone;
	};

// TSY init the PDP so send evewnt to GUQos,
void CNifPDPContextTsy::PDPCreateComplete()
{
	LOG(_LIT(string1,"\n Pdp Context %d Init secondary Context Status %d");)
	TPckg<TContextParameters> paraPckg(iParameter) ;  // change by ido
	//TPckg<TContextParameters>& paraPckg = *((TPckg<TContextParameters>*)&iParameter);
	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode = KErrArgument ;//KErrContextGeneral;
		iNif->RaiseEvent(KSecondaryContextCreated, paraPckg);
		iSubCommand=ECommandNone;
		}
	else
		{
			LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
			iParameter.iReasonCode=KErrNone;
			iNif->RaiseEvent(KSecondaryContextCreated, paraPckg);
			iSubCommand=ECommandNone;
//			only when we want to be notify we need to init this 
//			iContextMonitor->StartToMonitor();
		}
	iPending = EFalse;
}


void CNifPDPContextTsy::PdpDeleteComplete()
	{
	//TPckg<TContextParameters> paraPckg(iParameter) ;
	if (iStatus != KErrNone)
		{
		LOG(_LIT(string1,"\n Pdp Context %d deletion Status %d");)
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)
		iSubCommand=ECommandNone;
		}
	else
		{
		if (iSubCommand==EDeactivation)
			{
			// Delete the context
			iContext.Delete(iStatus);
			SetActive();
			iSubCommand=EDeletion;
			}
		else
			{
			// ETel has delete the PDP Context, So we detele its instance now.
			// We should not raise the deleteion event here, cos it is started
			// by GuQos.
			iSubCommand=ECommandNone;
			iContext.Close();
			iParameter.iReasonCode=KErrNone;
			iParameter.iContextInfo.iStatus = RPacketContext::EStatusDeleted;
			iPending = EFalse;
			TPckg<TContextParameters> paraPckg(iParameter) ; 
			iNif->RaiseEvent(KContextDeleteEvent, paraPckg);
			iNif->RemovePDPContext(this) ;
			iSubCommand=ECommandNone;
			}
		}
	};

void CNifPDPContextTsy::PdpActivationComplete()
	{

	LOG(_LIT(string1,"\n Pdp Context %d Activation Status %d");)
	TPckg<TContextParameters> paraPckg(iParameter) ;  // change by ido
	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode=KErrArgument ;//KErrContextGeneral;
		iPending = EFalse;
		iNif->RaiseEvent(KContextActivateEvent, paraPckg);
		iSubCommand=ECommandNone;
		}
	else
		{
		if (iSubCommand==EActivation)
			{
			//TPckg<RPacketQoS::TQoSR99_R4Negotiated>& qosPckg = *((TPckg<RPacketQoS::TQoSR99_R4Negotiated>*)&iQosParamter);
			iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
		#ifdef SYMBIAN_NETWORKING_UMTSR5 
			iQos.GetProfileParameters(iStatus, *iNegR5QosPckg);
		#else
			iQos.GetProfileParameters(iStatus, *iNegQosPckg);
		#endif 
		// SYMBIAN_NETWORKING_UMTSR5 

			SetActive();
	   		iSubCommand=EGetQosProfile;
			
			}
		else
			{
			LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
			iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
		#ifdef SYMBIAN_NETWORKING_UMTSR5 
			iParameter.iContextConfig.SetUMTSQoSNeg(iNegR5Qos);
		#else
			iParameter.iContextConfig.SetUMTSQoSNeg(iNegQos);
		#endif 
		// SYMBIAN_NETWORKING_UMTSR5 

			iParameter.iReasonCode=KErrNone;
			iPending = EFalse;
			iNif->RaiseEvent(KContextActivateEvent, paraPckg);
			iSubCommand=ECommandNone;
			
			}		
		}
	};
void CNifPDPContextTsy::PdpQosSetComplete()
	{
	LOG(_LIT(string1,"\n Pdp Context %d Qos Setting Status %d");)
	TPckg<TContextParameters> paraPckg(iParameter) ;  // change by ido
	//TPckg<TContextParameters>& paraPckg = *((TPckg<TContextParameters>*)&iParameter);
	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode=KErrArgument ;//KErrContextGeneral;
		iNif->RaiseEvent(KContextQoSSetEvent, paraPckg);
		iSubCommand=ECommandNone;
		}
	else
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode=KErrNone;
		iNif->RaiseEvent(KContextQoSSetEvent, paraPckg);
		if (iSubCommand == ESetQos)
			iSubCommand=ECommandNone;

		//	iQosMonitor->StartToMonitor();
		//	LOG(_LIT(string7,"\n We Starting the Qos Monitor ");)
		//	LOG(PdpLog::Write(string7);)

		}
	iPending = EFalse;
	};


void CNifPDPContextTsy::PdpModifyActiveComplete()
	{
	LOG(_LIT(string1,"\n Pdp Context %d Modify Active Status %d");)
		
	TPckg<TContextParameters> paraPckg(iParameter) ;  // change by ido
	//TPckg<TContextParameters>& paraPckg = *((TPckg<TContextParameters>*)&iParameter);
	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode=KErrArgument ;//KErrContextGeneral;
		iPending = EFalse;
		iNif->RaiseEvent(KContextModifyActiveEvent, paraPckg);
		iSubCommand=ECommandNone;
		}
	else
		{
			if (iSubCommand==EModifyActivation)
			{
			iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
		#ifdef SYMBIAN_NETWORKING_UMTSR5 
			iQos.GetProfileParameters(iStatus, *iNegR5QosPckg);
		#else
			iQos.GetProfileParameters(iStatus, *iNegQosPckg);
		#endif 
		// SYMBIAN_NETWORKING_UMTSR5 

			SetActive();
			iSubCommand=EGetQosProfile;
			}
		else
			{
			LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
			iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
		#ifdef SYMBIAN_NETWORKING_UMTSR5 
			iParameter.iContextConfig.SetUMTSQoSNeg(iNegR5Qos);
		#else
			iParameter.iContextConfig.SetUMTSQoSNeg(iNegQos);
		#endif 
		// SYMBIAN_NETWORKING_UMTSR5 

			iParameter.iReasonCode=KErrNone;
			iPending = EFalse;
			iNif->RaiseEvent(KContextModifyActiveEvent, paraPckg);
			iSubCommand=ECommandNone;
			}		
		}
	};

void CNifPDPContextTsy::PdpTFTModifyComplete()
	{
	LOG(_LIT(string1,"\n Pdp Context %d TFT Modify Status %d");)
	TPckg<TContextParameters> paraPckg(iParameter) ;  // change by ido
	//TPckg<TContextParameters>& paraPckg = *((TPckg<TContextParameters>*)&iParameter);

	if (iStatus != KErrNone)
		{
		LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
		iParameter.iReasonCode=KErrArgument ;//KErrContextGeneral;
		iPending = EFalse;
		iNif->RaiseEvent(KContextTFTModifiedEvent, paraPckg);
		iSubCommand=ECommandNone;
		}
	else
		{
	//	RPacketContext::TPacketFilterV2 filter;
		if (iModifedTFTInfo.NextPacketFilter(iFilter)==KErrNone)
			{
			//TPckg<RPacketContext::TPacketFilterV2>& filterPckg 
			//		= *((TPckg<RPacketContext::TPacketFilterV2>*)&filter);
			switch (iParameter.iTFTOperationCode)
				{
			case KAddFilters:
			// first update the iTFTInfo to hold the current etel filters 
				iTFTInfo.AddPacketFilter(iCurrentfilter);
				iCurrentfilter = iFilter ;
			
				iContext.AddPacketFilter(iStatus, *iFilterPckg);
				SetActive();
				iSubCommand=EAddTFTFilter;
				break;
			case KRemoveFilters:
			case KDeleteTFT:
			// first update the iTFTInfo to hold the current etel filters 
				iTFTInfo.RemovePacketFilter(iCurrentfilter);
				iCurrentfilter = iFilter ;

				iContext.RemovePacketFilter(iStatus, iFilter.iId);
				SetActive();
				iSubCommand=ERemoveTFTFilter;
				break;
			default:;
				}
			}
		else		
			{
			if (iParameter.iTFTOperationCode ==  KAddFilters )
				iTFTInfo.AddPacketFilter(iCurrentfilter);
			else // in case of remove or delete all  
				iTFTInfo.RemovePacketFilter(iCurrentfilter);
			iParameter.iContextConfig.SetTFTInfo(iTFTInfo);
			iParameter.iReasonCode=KErrNone;
			iPending = EFalse;
			LOG(PdpLog::Printf(string1, ContextId(), iStatus.Int());)	
			iNif->RaiseEvent(KContextTFTModifiedEvent, paraPckg);
			iSubCommand=ECommandNone;
			}
		}
	};
//
// CNifPDPMonitorTsy Class
//
CNifPDPMonitorTsy* CNifPDPMonitorTsy::NewL(MPacketTsyObserver* aObserver)
	{
	CNifPDPMonitorTsy* self = NewLC(aObserver);
	CleanupStack::Pop();
	return self;
	};
CNifPDPMonitorTsy* CNifPDPMonitorTsy::NewLC(MPacketTsyObserver* aObserver)
	{
	CNifPDPMonitorTsy* self = new(ELeave) CNifPDPMonitorTsy(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};
CNifPDPMonitorTsy::CNifPDPMonitorTsy(MPacketTsyObserver* aObserver):iObserver(aObserver)
	{
	};
void CNifPDPMonitorTsy::ConstructL()
	{
	CActiveScheduler::Add(this);
	};
void CNifPDPMonitorTsy::StartToMonitor()
	{
	iObserver->PacketContext().NotifyStatusChange(iStatus, iContextStatus);
	SetActive();
	};
void CNifPDPMonitorTsy::RunL()
	{
	LOG(_LIT(string1,"\n Pdp Monitoer Context wa called ");)
	TContextParameters para = iObserver->ContextParameters();
	TPckg<TContextParameters> paraPckg(para) ;  // change by ido
	//TPckg<TContextParameters>& paraPckg = *((TPckg<TContextParameters>*)&iParameter);
	//TContextId id=iContext->ContextId();
	//LOG(_LIT(string1,"\n PDP Context %d Monitor Status Changed: %d");)
	//LOG(_LIT(string2,"\n PDP Context %d Monitor Error Status: %d");)

	if (iStatus==KErrNone)
		{	
		switch (iContextStatus)
			{
		//LOG(PdpLog::Printf(string1, id, iContextStatus);)				
		case RPacketContext::EStatusUnknown:
		case RPacketContext::EStatusInactive:
		case RPacketContext::EStatusActivating:
		case RPacketContext::EStatusDeactivating:
			break;
		case RPacketContext::EStatusActive:
			para.iContextInfo.iStatus=RPacketContext::EStatusActive;
			iObserver->NifNcp()->RaiseEvent(KContextUnblockedEvent, paraPckg);
			break;


		case RPacketContext::EStatusSuspended:
			para.iContextInfo.iStatus=RPacketContext::EStatusSuspended;
			iObserver->NifNcp()->RaiseEvent(KContextBlockedEvent, paraPckg);
			break;
		case RPacketContext::EStatusDeleted:
			break;
			}
		}
	else
		{
		//LOG(PdpLog::Printf(string2, id, iStatus);)				
		}
	
	LOG(PdpLog::Printf(string1);)	
	iObserver->PacketContext().NotifyStatusChange(iStatus, iContextStatus);
	SetActive();
	};

void CNifPDPMonitorTsy::DoCancel()
	{
	iObserver->PacketContext().CancelAsyncRequest(EPacketContextNotifyStatusChange);
	};

//
// CNifNetworkMonitorTsy Class
//
CNifNetworkMonitorTsy* CNifNetworkMonitorTsy::NewL(CQoSTestNcp*  aLink)
	{
	CNifNetworkMonitorTsy* self = NewLC(aLink);
	CleanupStack::Pop();
	return self;
	};
CNifNetworkMonitorTsy* CNifNetworkMonitorTsy::NewLC(CQoSTestNcp*  aLink)
	{
	CNifNetworkMonitorTsy* self = new(ELeave) CNifNetworkMonitorTsy(aLink);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};
CNifNetworkMonitorTsy::CNifNetworkMonitorTsy(CQoSTestNcp*  aLink)
	:CNifNetworkMonitorBase(aLink)
	{
	};
void CNifNetworkMonitorTsy::ConstructL()
	{
	CActiveScheduler::Add(this);
	};
void CNifNetworkMonitorTsy::StartToMonitor()
	{
	iLink->PacketService().NotifyStatusChange(iStatus, iNetworkStatus);
	SetActive();
	};
void CNifNetworkMonitorTsy::RunL()
	{
	TNetworkParameters para;
	if (iStatus==KErrNone)
		{
		LOG(_LIT(string1,"\n Network Monitor Network Status Changed: %d");)
		LOG(PdpLog::Printf(string1, iNetworkStatus);)	
		
		para.iNetworkStatus=iNetworkStatus;
		TPckg<TNetworkParameters> paraPckg(para);
		iLink->RaiseEvent(KNetworkStatusEvent, paraPckg);
		}
	else
		{
		LOG(_LIT(string1,"\n Network Monitor Err Status: %d");)
		LOG(PdpLog::Printf(string1, iStatus.Int());)				
		}
	iLink->PacketService().NotifyStatusChange(iStatus, iNetworkStatus);
	SetActive();
	};

void CNifNetworkMonitorTsy::DoCancel()
	{
	iLink->PacketService().CancelAsyncRequest(EPacketNotifyStatusChange);
	};


//
// CNifQoSMonitorTsy Class
//

CNifQosMonitorTsy::CNifQosMonitorTsy(MPacketTsyObserver* aObserver):iObserver(aObserver)
	{
	//	iObserver = aObserver;
#ifdef SYMBIAN_NETWORKING_UMTSR5 
	TRAP_IGNORE(iNegR5QosPckgMonitor = new (ELeave) TPckg<RPacketQoS::TQoSR5Negotiated>(iNegR5QosMonitor));
#else
	TRAP_IGNORE(iNegQosPckgMonitor = new (ELeave) TPckg<RPacketQoS::TQoSR99_R4Negotiated>(iNegQosMonitor));
#endif 
// SYMBIAN_NETWORKING_UMTSR5 
	};


CNifQosMonitorTsy* CNifQosMonitorTsy::NewL(MPacketTsyObserver* aObserver)
	{
	CNifQosMonitorTsy* self = NewLC(aObserver);
	CleanupStack::Pop();
	return self;
	};
CNifQosMonitorTsy* CNifQosMonitorTsy::NewLC(MPacketTsyObserver* aObserver)
	{
	CNifQosMonitorTsy* self = new(ELeave) CNifQosMonitorTsy(aObserver);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};
void CNifQosMonitorTsy::ConstructL()
	{
	CActiveScheduler::Add(this);
	};
void CNifQosMonitorTsy::StartToMonitor()
	{
#ifdef SYMBIAN_NETWORKING_UMTSR5 
	iObserver->PacketQoS().NotifyProfileChanged(iStatus, *iNegR5QosPckgMonitor);
#else
	iObserver->PacketQoS().NotifyProfileChanged(iStatus, *iNegQosPckgMonitor);
#endif 
// SYMBIAN_NETWORKING_UMTSR5 
	SetActive();
	};
void CNifQosMonitorTsy::RunL()
	{
//take care for that 
	LOG(_LIT(string1,"\n Qos Monitor was Called ");)
	LOG(PdpLog::Write(string1);)
					
	//iObserver->PacketQoS().NotifyProfileChanged(iStatus, *iNegQosPckgMonitor);
	//SetActive();
	DoCancel();
	SetActive();
	};

void CNifQosMonitorTsy::DoCancel()
{										      	
	iObserver->PacketQoS().CancelAsyncRequest(EPacketQoSNotifyProfileChanged);
	};
