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

#include "NifPdpBase.h"
#include "QosTestNcp.h"

//
// CNifPDPContextBase Class
//
CNifPDPContextBase::CNifPDPContextBase(CQoSTestNcp* aNif, TDes8& aConfig)
	:CActive(EPriorityStandard), iNif(aNif)
	{
	// Set the local copy PDP Context Type & ID, PDP Type and access name
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	iParameter.iContextType = opt.iContextType;
	iParameter.iContextInfo.iContextId = opt.iContextInfo.iContextId;
	RPacketContext::TProtocolType pdpType;
	opt.iContextConfig.GetPdpType(pdpType);
	iParameter.iContextConfig.SetPdpType(pdpType);
	RPacketContext::TGSNAddress name;
	opt.iContextConfig.GetAccessPointName(name);
	iParameter.iContextConfig.SetAccessPointName(name);

	// Copy the request Qos, primary context need this.
	CopyQosReqParameter(opt);
	iPending = EFalse;

	// used to flag whether or not to generate context deleted event
	iNifmanInitiatedDelete = EFalse;
	};

CNifPDPContextBase::~CNifPDPContextBase()
	{
	delete iContextMonitor;
	iContextMonitor=NULL;
	delete iQosMonitor;
	iQosMonitor=NULL;
	};

TInt CNifPDPContextBase::HandleControl(TUint aName, TDes8& aConfig)
	{
	TInt ret;
		switch (aName)
			{
			case KContextCreate:
				//Set the Operation Command.
				iCommand = KContextCreate;
				ret = HandlePDPCreate(aConfig);
				break;
			case KContextDelete:
				//Set the Operation Command.
				iCommand = KContextDelete;
				ret = HandlePDPDelete(aConfig);
				break;
			case KContextQoSSet:
				//Set the Operation Command.
				iCommand = KContextQoSSet;
				ret = HandleQosSet(aConfig);
				break;
			case KContextActivate:
				//Set the Operation Command.
				iCommand = KContextActivate;
				ret = HandleContextActivate(aConfig);
				break;
			case KContextModifyActive:
				//Set the Operation Command.
				iCommand = KContextModifyActive;
				ret = HandleModifyActive(aConfig);
				break;
			case KContextTFTModify:
				iCommand = KContextTFTModify;
				ret = HandleTFTModify(aConfig);
				break;
			default:
				return KErrNotSupported;
			}
		return ret;
	};


void CNifPDPContextBase::RunL()
	{
		switch(iCommand)
		{
		//TODO:The secondary PDP context creation is sync or async?
		//There is a change request from Nokia, it seems telephony can not
		// make it.Check it!
		case KStartupPrimaryContextCreation:
			PdpPrimaryComplete();
			break;
		case KContextCreate:
			PDPCreateComplete();
			break;
		case KContextDelete:
			PdpDeleteComplete();
			break;
		case KContextActivate:
			PdpActivationComplete();
			//iCommand = 150;
			break;
		case KContextQoSSet:
			PdpQosSetComplete();
			break;
		case KContextModifyActive:
			PdpModifyActiveComplete();
			break;
		case KContextTFTModify:
			PdpTFTModifyComplete();
			break;
		default:
			//PdpTriggerEvent();
			break ;
		}
	};
void CNifPDPContextBase::CopyQosReqParameter(TContextParameters& aParameter)
	{
   	#ifdef SYMBIAN_NETWORKING_UMTSR5  
   		RPacketQoS::TQoSR5Requested qosR5;
   		aParameter.iContextConfig.GetUMTSQoSReq(qosR5);
   		iParameter.iContextConfig.SetUMTSQoSReq(qosR5);
   	#else
   		RPacketQoS::TQoSR99_R4Requested qos;
   		aParameter.iContextConfig.GetUMTSQoSReq(qos);
   		iParameter.iContextConfig.SetUMTSQoSReq(qos);
   	#endif 
   	// SYMBIAN_NETWORKING_UMTSR5 
	};
	
void CNifPDPContextBase::EventPrimaryActive()
	{
	//TODO: Not sure if we raise KPrimaryContextCreated here or raise it when the context
	// was created? it is up to the telephony.
	TPckg<TContextParameters> paraPckg(iParameter) ;
	iPending = EFalse;
	iNif->RaiseEvent(KPrimaryContextCreated, paraPckg);
	};

void CNifPDPContextBase::PdpTriggerEvent()
{
	iParameter.iReasonCode=0;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusUnknown;
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iNif->RaiseEvent(KContextParametersChangeEvent, paraPckg);
}

TInt CNifPDPContextBase::StoreDataSent(const TUint aDataSentSize)
	/**
	 * Add the details of this packet onto the current total for the subconnection. This is checked by passing in the port number. If the port number matches that used by this context them the size will be increased.
	 * @param const TUint aDataSentSize The size of the packet
	 * @return Either an error code or if a threshold has been crossed will return the actual amount of data sent since the request for notification (could be larger than the threshold)
	 */	
	{
	iDataSentSize+=aDataSentSize;
	if((iDataSentGranularity>0) && (iDataSentSize-iDataSentNotificationStartSize>=iDataSentGranularity))
		{
		iDataSentGranularity=0;
		return iDataSentSize-iDataSentNotificationStartSize;
		}
	return KErrNone;
	}
	
TInt CNifPDPContextBase::StoreDataReceived(const TUint aDataRecvSize)
	/**
	 * Add the details of this packet onto the current total for the subconnection. This is checked by passing in the port number. If the port number matches that used by this context them the size will be increased.
	 * @param const TUint aDataRecvSize The size of the packet
	 * @return Either an error code or if a threshold has been crossed will return the actual amount of data received since the request for notification (could be larger than the threshold)
	 */		
	{
	iDataRecvSize+=aDataRecvSize;
	if((iDataRecvGranularity>0) && (iDataRecvSize-iDataRecvNotificationStartSize>=iDataRecvGranularity))
		{
		iDataRecvGranularity=0;
		return iDataRecvSize-iDataRecvNotificationStartSize;
		}
	return KErrNone;
	}

TInt CNifPDPContextBase::SetDataSentNotificationGranularity(TUint aGranularity)
	/**
	 * Set the granularity of the notification for data sent on this context
	 * @param aGranularity The granularity you want (in bytes) will affect when the next notification happens
	 * @return A system wide error code
	 */	
	{
	if(iDataSentGranularity==0)
		{
		// First notification request
		iDataSentNotificationStartSize=iDataSentSize;
		}
	iDataSentGranularity=aGranularity;
	if(iDataSentSize-iDataSentNotificationStartSize>=iDataSentGranularity)
		{
		iDataSentGranularity=0;
		return iDataSentSize-iDataSentNotificationStartSize;
		}
	return KErrNone;
	}
	
TInt CNifPDPContextBase::SetDataRecvNotificationGranularity(TUint aGranularity)
	/**
	 * Set the granularity of the notification for data received on this context
	 * @param TUint aGranularity The granularity you want (in bytes) will affect when the next notification happens
	 * @return A system wide error code
	 */	
	{
	if(iDataRecvGranularity==0)
		{
		// First notification request
		iDataRecvNotificationStartSize=iDataRecvSize;
		}
	iDataRecvGranularity=aGranularity;
	if(iDataRecvSize-iDataRecvNotificationStartSize>=iDataRecvGranularity)
		{
		iDataRecvGranularity=0;
		return iDataRecvSize-iDataRecvNotificationStartSize;
		}
	return KErrNone;
	}

TInt CNifPDPContextBase::CancelDataSentNotification()
	/**
	 * Cancel the data sent notification for this context
	 * @return A system wide error code. Presently always KErrNone
	 */		
	{
	iDataSentGranularity=0;
	iDataSentNotificationStartSize=0;
	return KErrNone;
	}
	
TInt CNifPDPContextBase::CancelDataReceivedNotification()
	/**
	 * Cancel the data received notification for this context
	 * @return A system wide error code
	 */	
	{
	iDataRecvGranularity=0;
	iDataRecvNotificationStartSize=0;
	return KErrNone;
	}
	
CNifQosMonitorBase::~CNifQosMonitorBase()
{}
