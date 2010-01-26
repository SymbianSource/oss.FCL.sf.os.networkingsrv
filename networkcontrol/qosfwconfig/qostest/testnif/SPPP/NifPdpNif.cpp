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

#include "NifPdpNif.h"
#include "QoSTestLog.h"
#include "etelqos.h"



//
// CNifPDPContextTNif Class
//
CNifPDPContextTNif* CNifPDPContextTNif::NewL(CQoSTestNcp* aNif, TDes8& aConfig)
	{
	CNifPDPContextTNif* self = NewLC(aNif, aConfig);
	CleanupStack::Pop();
	return self;

	}
CNifPDPContextTNif* CNifPDPContextTNif::NewLC(CQoSTestNcp* aNif, TDes8& aConfig)
	{
	CNifPDPContextTNif* self = new(ELeave) CNifPDPContextTNif(aNif, aConfig);
	CleanupStack::PushL(self);
	self->ConstructL();
	return self;
	};

CNifPDPContextTNif::CNifPDPContextTNif(CQoSTestNcp* aNif, TDes8& aConfig)
	:CNifPDPContextBase(aNif, aConfig)
	{
	};
CNifPDPContextTNif::~CNifPDPContextTNif()
	{
	// Delete the Timer.
	TimerDelete();
	Cancel();
	iSblpParams.ResetAndDestroy();
	};
void CNifPDPContextTNif::TimerComplete(TInt /*aStatus*/)
	{
	// Trigger the Active object.
	SetActive();
	TRequestStatus* theStatus=&iStatus;
	User::RequestComplete(theStatus, KErrNone);	
	};
void CNifPDPContextTNif::DoCancel()
	{
	TimerCancel();
	};
void CNifPDPContextTNif::ConstructL()
	{
	// Instantiate the Qos monitor, if we need it for testnif level simulation.
	//TODO:iQosMonitor = CNifQosMonitorNif::NewL(this);
	//TODO:iContextMonitor = CNifPDPMonitorNif::NewL(this);

	// Initialise the section in configuration file.
	iSectionName.Format(KSectionNameFormat, ContextId());
	if(iNif->CfgFile()->Section(iSectionName)==NULL)
		User::Leave(KErrNotFound);
	
	// Construct the Timer for Asynchronous Call
	TimerConstructL(KDefaultTimerPriority);
	CActiveScheduler::Add(this);

	};
TInt CNifPDPContextTNif::HandlePDPCreate(TDes8& aConfig)
	{
	RPacketContext::TProtocolType pdpType;
	iNif->FirstExistingPDPContext()->ContextPDPType(pdpType);

	// Context creation is synchronous call.
	const TDesC8& contextName=CfgFile()->ItemValue(KContextName, KDefaultName);
	iContextName.Copy(contextName);

	RPacketContext::TGSNAddress name;
	iNif->FirstExistingPDPContext()->ContextAPNName(name);

	// Update the return data.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	opt.iContextConfig.SetPdpType(pdpType);
	opt.iContextType=ESecondaryContext;
	opt.iContextConfig.SetAccessPointName(name);
	opt.iReasonCode = KErrNone;				// we need to set this to success IDO

	// Update the local data copy.	
	iParameter.iContextConfig.SetPdpType(pdpType);
	iParameter.iContextType=ESecondaryContext;
	iParameter.iContextConfig.SetAccessPointName(name);

	//TODO: We need to start a timer as well to simulate the creation
	// if telephony decides the creation is Asyn call.

	// Start Timer to new PDP .
	TimerCancel();
	TimerAfter(2000*1000);

	return KErrNone;
	};
TInt CNifPDPContextTNif::HandlePDPDelete(TDes8& aConfig)
	{
	// Get the Activation Duration.
	TInt deleteDuration;

	const CTestConfigItem* item = CfgFile()->Item(KContextDeleteEntry);
	TInt err=CTestConfig::GetElement(item->Value(), KStdDelimiter, 1, deleteDuration);		// The 3rd parameter (3) represents the index of the variable on the config file line
	if (err!=KErrNone)
		deleteDuration=KDefaultDeletionTime;
//leteDuration=KDefaultDeletionTime;
	// Start Timer to delete the PDP context.
	TimerCancel();
	TimerAfter(deleteDuration*1000);
	// Update the return data.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	opt.iReasonCode = KErrNone;				// we need to set this to success IDO
	// Update the local data copy.	
	iParameter.iReasonCode = KErrNone;
	return KErrNone;
	};
TInt CNifPDPContextTNif::HandleQosSet(TDes8& aConfig)
	{
	// Update local record.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	CopyQosReqParameter(opt);

	// Get the Qos Profile Name.
	const TDesC8& name=CfgFile()->ItemValue(KQosProfileName, KDefaultName);
	iQosProfileName.Copy(name);

	// Get the Qos Setting Duration.
	TInt qosDuration;
	const CTestConfigItem* item = CfgFile()->Item(KContextQosSetEntry);
	TInt err=CTestConfig::GetElement(item->Value(), KStdDelimiter, 1, qosDuration);		// The 3rd parameter (3) represents the index of the variable on the config file line
	if (err!=KErrNone)
		qosDuration=KDefaultQosSettingTime;

	// Start Timer to set Qos.
	TimerCancel();
	TimerAfter(qosDuration*1000);	

	// Update the return data.
	opt.iReasonCode = KErrNone; 				// we need to set this to success 
	// Update the local data copy.	
	iParameter.iReasonCode = KErrNone;
	return KErrNone;
	};
TInt CNifPDPContextTNif::HandleContextActivate(TDes8& aConfig)
	{
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	if (iParameter.iContextInfo.iStatus ==  RPacketContext::EStatusActivating)
	{
		opt.iReasonCode = KErrInUse;				// we need to set this to success 
		return KErrGeneral;
	}
	else if(iParameter.iContextInfo.iStatus ==  RPacketContext::EStatusActive)
	{
		opt.iReasonCode = KErrInUse;
		return KErrGeneral;
	}
	
	// Get the Activation Duration.
	TInt activateDuration;
	const CTestConfigItem* item = CfgFile()->Item(KContextActivateEntry);
	TInt err=CTestConfig::GetElement(item->Value(), KStdDelimiter, 1, activateDuration);		// The 3rd parameter (3) represents the index of the variable on the config file line
	if (err!=KErrNone)
		activateDuration=KDefaultActivationTime;

	// Start Timer to activate the PDP context.
	TimerCancel();
	TimerAfter(activateDuration*1000);

	// Update the return data.
	opt.iReasonCode = KErrNone;				// we need to set this to success IDO
	opt.iContextInfo.iStatus = RPacketContext::EStatusActivating;
	// Update the local data copy.	
	iParameter.iReasonCode = KErrNone;
	
	return KErrNone;
	};
void CNifPDPContextTNif::ActivatePrimaryContext(TDes8& aConfig)
	{
	iCommand=KStartupPrimaryContextCreation;  // change made by ido & jongon
	HandleContextActivate(aConfig);
	};
TInt CNifPDPContextTNif::HandleModifyActive(TDes8& aConfig)
	{
	// Get the Activation Duration.
	TInt modifyActivate;
	const CTestConfigItem* item = CfgFile()->Item(KContextModifyActivate);
	TInt err=CTestConfig::GetElement(item->Value(), KStdDelimiter, 1, modifyActivate);		// The 3rd parameter (3) represents the index of the variable on the config file line
	if (err!=KErrNone)
		modifyActivate=KDefaultModifyActivateTime;

	// Start Timer to modfidy activation
	TimerCancel();
	TimerAfter(modifyActivate*1000);
	
	// Update the return data.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	opt.iReasonCode = KErrNone;				// we need to set this to success IDO
	// Update the local data copy.	
	iParameter.iReasonCode = KErrNone;
	
	return KErrNone;
	};
TInt CNifPDPContextTNif::HandleTFTModify(TDes8& aConfig)
	{
	// Get the Activation Duration.
	RPacketContext::TPacketFilterV2 iFilter;

	TInt tftmodify;
	const CTestConfigItem* item = CfgFile()->Item(KContextTFTModifyEntry);
	TInt err=CTestConfig::GetElement(item->Value(), KStdDelimiter, 1, tftmodify);		// The 3rd parameter (3) represents the index of the variable on the config file line
	if (err!=KErrNone)
		tftmodify=KDefaultTFTModifyTime;

	// Start Timer to modfidy activation
	TimerCancel();
	TimerAfter(tftmodify*1000);
	// Update the return data.
	TContextParameters& opt = *(TContextParameters*)aConfig.Ptr();
	opt.iReasonCode = KErrNone;				// we need to set this to success IDO
	// Update the local data copy.	
	
//	iParameter = opt;
	iParameter.iReasonCode = KErrNone;
	iParameter.iTFTOperationCode = opt.iTFTOperationCode;
	/**
	The individual operations will be performed according to the 
	operation code provided
	*/
	TTFTInfo aTFT,bTFT;
	if (iParameter.iTFTOperationCode == KAddFilters) 
		{
		iParameter.iContextConfig.GetTFTInfo(aTFT);

		opt.iContextConfig.GetTFTInfo(bTFT);
		bTFT.SetToFirst();
		bTFT.NextPacketFilter(iFilter);
		aTFT.AddPacketFilter(iFilter);
		iParameter.iContextConfig.SetTFTInfo(aTFT);	
		// update the Result
		opt.iReasonCode = KErrNone;
		iParameter.iReasonCode = KErrNone; 
		}
	else if (iParameter.iTFTOperationCode == KRemoveFilters) 
		{
		iParameter.iContextConfig.GetTFTInfo(aTFT);

		opt.iContextConfig.GetTFTInfo(bTFT);
		bTFT.SetToFirst();
		bTFT.NextPacketFilter(iFilter);
		aTFT.RemovePacketFilter(iFilter);
		iParameter.iContextConfig.SetTFTInfo(aTFT);	
		// update the Result
		opt.iReasonCode = KErrNone;
		iParameter.iReasonCode = KErrNone;
		}
	else if (iParameter.iTFTOperationCode == KAddSblpParameter)
		{
		
		iParameter.iContextConfig.GetTFTInfo(aTFT);

		opt.iContextConfig.GetTFTInfo(bTFT);
		bTFT.SetToFirst();
		bTFT.NextPacketFilter(iFilter);
		aTFT.AddPacketFilter(iFilter);
		/**
		These addition is for handling SBLP Parameters inside the TFT as per the 
		requirements of PREQ634.
		Currently only one SblpParameters ( consisting of one MAT and multiple associated
		Flow Identifires ). Later when R6 will be implemented this code needs to change to
		support the Multiple SBLP Parameter. However the infrastructure of Multiple SBLP is 
		already in place as iSblpParams uses RPointerArray
		*/
		RPacketContext::CTFTMediaAuthorizationV3 * aMAT = 	RPacketContext::CTFTMediaAuthorizationV3::NewL();
		/**
		Store it in RPointerArray to get it cleaned which destructing
		*/
		iSblpParams.AppendL(aMAT);
		/**
		Get the SBLP Parameters
		*/ 
		bTFT.GetSblpToken(*aMAT);
		/**
		Add SBLP Parameters to bTft; 		
		*/
		aTFT.AddSblpToken(iSblpParams[0]->iAuthorizationToken,iSblpParams[0]->iFlowIds);	
		iParameter.iContextConfig.SetTFTInfo(aTFT);	
		// update the Result
		opt.iReasonCode = KErrNone;
		iParameter.iReasonCode = KErrNone;
		
				
		//checking the values against PDPContext.txt and return result accordingly
		// Initialise the section in configuration file.
		TBuf8<KMaxName>		iSectionName1;   // SBLP section names
		
		TBool matFound = ETrue;
		TBool validToken = ETrue;
     	for (TInt sblpContextId = 0; matFound; sblpContextId++)
       	{
			
			iSectionName1.Format(KSBLPSectionNameFormat, sblpContextId);
			if(iNif->CfgFile()->Section(iSectionName1)==NULL)
			{
				matFound = EFalse;
				break;	
			}
			validToken = ETrue;
       		//comparing Authorisation token, if doesnt match exit
			TInt numberOfFlowIdsReceived = iSblpParams[0]->iFlowIds.Count();			
			
			const CTestConfigSection* sblpCfgSection = iNif->CfgFile()->Section(iSectionName1);
			const TDesC8& authorizationToken=sblpCfgSection->ItemValue(KAuthorizationToken,KDefaultAuthorizationToken);	
			if(authorizationToken.Compare(iSblpParams[0]->iAuthorizationToken) != 0)
				{
				LOG(_LIT(string1,"Media Authorisation does not match\n"));
				LOG(PdpLog::Printf(string1);)
				validToken = EFalse;
				}
				else
				{
				  							
				    if(numberOfFlowIdsReceived > 0)
					  {

						TInt numberOfTokensInFile=sblpCfgSection->ItemValue(KFlowIdCount,KDefaultFlowIdCount);				
						if(numberOfTokensInFile != numberOfFlowIdsReceived )
							{
							LOG(_LIT(string1,"\n Number of FlowIds received does not match with the Config file count");)
							LOG(PdpLog::Printf(string1);)
							validToken = EFalse;							
							}
						else
							{
				  			    
				  				for (TInt sblpFlowId = 0; sblpFlowId < numberOfTokensInFile; sblpFlowId++)
       			  				{
       			  					
       			  					TInt mediaComponentNumber,ipFlowNumber;
									TBuf8<KMaxName>	iFlowId;
									iFlowId.Format(KFlowIdentifier,sblpFlowId);
									const CTestConfigItem* item =  sblpCfgSection->Item(iFlowId);
       			  					if(item)
									{
       			  					
       			  					TInt ret=CTestConfig::GetElement(item->Value(),KStdDelimiter,0,mediaComponentNumber);
								 
									if(ret!=KErrNone)
				 					{
				 					LOG(_LIT(string1,"WARNING ERROR IN CONFIGURATION FILE PARSING - SBLP::mediaComponentNumber\n"));
				 					LOG(PdpLog::Printf(string1);)
				 					mediaComponentNumber = KDefaultFlowIdValues;
				 					}
				 					
									ret=CTestConfig::GetElement(item->Value(),KStdDelimiter,1,ipFlowNumber);
									if(ret!=KErrNone)
				 					{
				 					LOG(_LIT(string1,"WARNING ERROR IN CONFIGURATION FILE PARSING - SBLP::ipFlowNumber\n"));
				 					LOG(PdpLog::Printf(string1);)
				 					ipFlowNumber = KDefaultFlowIdValues;
									} 
									
									if( (mediaComponentNumber != iSblpParams[0]->iFlowIds[sblpFlowId].iMediaComponentNumber)
										|| (ipFlowNumber != iSblpParams[0]->iFlowIds[sblpFlowId].iIPFlowNumber) )
										{
											LOG(_LIT(string1,"\n Number of FlowIds received doesnt match with the Config file count");)
											LOG(PdpLog::Printf(string1);)
											validToken = EFalse;
																															
										}
										
									}
									
       			  				}
       								
							}//end else
					   }//end if
			
				if(validToken)
				{
					matFound=EFalse;
				}
		
			
	
		}//end else
		
       	}
		if(validToken == EFalse)
		{
			opt.iReasonCode = KErrNotFound;
			iParameter.iReasonCode = KErrNotFound;
		}

		
		}
		
		
		
	
		else if (iParameter.iTFTOperationCode == KRemoveSblpParameter)
		{
		iParameter.iContextConfig.GetTFTInfo(aTFT);
		opt.iContextConfig.GetTFTInfo(bTFT);
		bTFT.SetToFirst();
		/**
		In case of Multiple SBLP Parameters as per the requirements of R6. Then this 
		implementation will change and will remove SBLP Parameters as per the facility
		provided by the umtsif.dll
		*/
		aTFT.RemovSblpToken();
		iParameter.iContextConfig.SetTFTInfo(aTFT);	
		// update the Result
		opt.iReasonCode = KErrNone;
		iParameter.iReasonCode = KErrNone;
		}
	else
		{
		/**
		No implemetation yet 
		To be determined
		*/
		}
	return KErrNone;
	}

const CTestConfigSection* CNifPDPContextTNif::CfgFile()
	{
	// Get the needed section from the config file
	return iNif->CfgFile()->Section(iSectionName);
	};


void CNifPDPContextTNif::PDPCreateComplete()
{
	
	LOG(_LIT(string1,"Create PDP Context %d completed");)
	LOG(PdpLog::Printf(string1, ContextId());)				
	iParameter.iReasonCode = KErrNone;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusUnknown;	
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iPending = EFalse;
	iNif->RaiseEvent(KSecondaryContextCreated, paraPckg);
}


void CNifPDPContextTNif::PdpDeleteComplete()
	{
	// Get the status from the configratin file, cause this is NIF level simulation
	TInt deleteStatus;
	const CTestConfigItem* item = CfgFile()->Item(KContextDeleteEntry);
	CTestConfig::GetElement(item->Value(), KStdDelimiter, 0, deleteStatus);		// The 3rd parameter (3) represents the index of the variable on the config file line

	LOG(_LIT(string1,"Delete PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(), deleteStatus);)

	iParameter.iReasonCode=deleteStatus;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusDeleted;
	iPending = EFalse;

	// The deletion of PDP context event should only be raised if the deletion request did not come 
	// from GUQoS (ie it came from the network or from nifman). The event will be raised in the 
	// method CQoSTestNcp::RemovePDPContext() as necessary.
	iNif->RemovePDPContext(this);
	};


void CNifPDPContextTNif::PdpActivationComplete()
	{
	// Get the status from the configratin file, cause this is NIF level simulation
	TInt activateStatus;
	const CTestConfigItem* item = CfgFile()->Item(KContextActivateEntry);
	CTestConfig::GetElement(item->Value(), KStdDelimiter, 0, activateStatus);		// The 3rd parameter (3) represents the index of the variable on the config file line
	iParameter.iReasonCode=activateStatus;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
	LOG(_LIT(string1,"Activate PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(), activateStatus);)				

	// Predefined Successful Operation
	if (activateStatus==KErrNone)
		{
		// Get the predefined Negotiate Qos Value from the configuration file.
 		#ifdef SYMBIAN_NETWORKING_UMTSR5  
 			RPacketQoS::TQoSR5Negotiated negR5Qos;
 
 			negR5Qos.iTrafficClass=(RPacketQoS::TTrafficClass)
 				CfgFile()->ItemValue(KQosNegTrafficClass, KDefaultValue);
 			negR5Qos.iDeliveryOrderReqd=(RPacketQoS::TDeliveryOrder)
 				CfgFile()->ItemValue(KQosNegDeliveryOrder, KDefaultValue);
 			negR5Qos.iDeliverErroneousSDU=(RPacketQoS::TErroneousSDUDelivery)
 				CfgFile()->ItemValue(KQosNegErroneousSDUDelivery, KDefaultValue);
 			negR5Qos.iMaxSDUSize=CfgFile()->ItemValue(KQosNegMaxSDUSize, KDefaultValue);
 			negR5Qos.iMaxRate.iUplinkRate=
 				CfgFile()->ItemValue(KQosNegUpBitRate, KDefaultValue);
 			negR5Qos.iMaxRate.iDownlinkRate=
 				CfgFile()->ItemValue(KQosNegDownBitRate, KDefaultValue);
 			negR5Qos.iBER=(RPacketQoS::TBitErrorRatio)
 				CfgFile()->ItemValue(KQosNegBitErrorRatio, KDefaultValue);
 			negR5Qos.iSDUErrorRatio=(RPacketQoS::TSDUErrorRatio)
 				CfgFile()->ItemValue(KQosNegSDUErrorRatio, KDefaultValue);
 			negR5Qos.iTrafficHandlingPriority=(RPacketQoS::TTrafficHandlingPriority)
 				CfgFile()->ItemValue(KQosNegTrafficHandlingPriority, KDefaultValue);
 			negR5Qos.iTransferDelay=CfgFile()->ItemValue(KQosNegTransferDelay, KDefaultValue);
 			negR5Qos.iGuaranteedRate.iUplinkRate=CfgFile()->ItemValue(KQosNegGuaranteedUpRate, KDefaultValue);
 			negR5Qos.iGuaranteedRate.iDownlinkRate=CfgFile()->ItemValue(KQosNegGuaranteedDownRate, KDefaultValue);
 		
 			negR5Qos.iSourceStatisticsDescriptor=(RPacketQoS::TSourceStatisticsDescriptor)
 				CfgFile()->ItemValue(KQosNegSourceStatisticsDescriptor, KDefaultValue);
 			negR5Qos.iSignallingIndication=CfgFile()->ItemValue(KQosNegSignallingIndication, KDefaultValue);
 
 			iParameter.iContextConfig.SetUMTSQoSNeg(negR5Qos);
 		#else
 			RPacketQoS::TQoSR99_R4Negotiated negQos;
 			negQos.iTrafficClass=(RPacketQoS::TTrafficClass)
 				CfgFile()->ItemValue(KQosNegTrafficClass, KDefaultValue);
 			negQos.iDeliveryOrderReqd=(RPacketQoS::TDeliveryOrder)
 				CfgFile()->ItemValue(KQosNegDeliveryOrder, KDefaultValue);
 			negQos.iDeliverErroneousSDU=(RPacketQoS::TErroneousSDUDelivery)
 				CfgFile()->ItemValue(KQosNegErroneousSDUDelivery, KDefaultValue);
 			negQos.iMaxSDUSize=CfgFile()->ItemValue(KQosNegMaxSDUSize, KDefaultValue);
 			negQos.iMaxRate.iUplinkRate=
 				CfgFile()->ItemValue(KQosNegUpBitRate, KDefaultValue);
 			negQos.iMaxRate.iDownlinkRate=
 				CfgFile()->ItemValue(KQosNegDownBitRate, KDefaultValue);
 			negQos.iBER=(RPacketQoS::TBitErrorRatio)
 				CfgFile()->ItemValue(KQosNegBitErrorRatio, KDefaultValue);
 			negQos.iSDUErrorRatio=(RPacketQoS::TSDUErrorRatio)
 				CfgFile()->ItemValue(KQosNegSDUErrorRatio, KDefaultValue);
 			negQos.iTrafficHandlingPriority=(RPacketQoS::TTrafficHandlingPriority)
 				CfgFile()->ItemValue(KQosNegTrafficHandlingPriority, KDefaultValue);
 			negQos.iTransferDelay=CfgFile()->ItemValue(KQosNegTransferDelay, KDefaultValue);
 			negQos.iGuaranteedRate.iUplinkRate=CfgFile()->ItemValue(KQosNegGuaranteedUpRate, KDefaultValue);
 			negQos.iGuaranteedRate.iDownlinkRate=CfgFile()->ItemValue(KQosNegGuaranteedDownRate, KDefaultValue);
 			iParameter.iContextConfig.SetUMTSQoSNeg(negQos);
 		#endif 
 		// SYMBIAN_NETWORKING_UMTSR5 		
 		}
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iPending = EFalse;
	iNif->RaiseEvent(KContextActivateEvent, paraPckg);

	// after we activting we want to simulate ...
//	TimerCancel();
//	TimerAfter(5000*1000);

	};



void CNifPDPContextTNif::PdpQosSetComplete()
	{
	// Get the status from the configratin file, cause this is NIF level simulation
	TInt qosSetStatus;
	const CTestConfigItem* item = CfgFile()->Item(KContextQosSetEntry);
	CTestConfig::GetElement(item->Value(), KStdDelimiter, 0, qosSetStatus);		// The 3rd parameter (3) represents the index of the variable on the config file line
	LOG(_LIT(string1,"Set QoS PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(), qosSetStatus);)				
	iParameter.iReasonCode=qosSetStatus;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusUnknown;
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iPending = EFalse;
	iNif->RaiseEvent(KContextQoSSetEvent, paraPckg);
	};

void CNifPDPContextTNif::PdpModifyActiveComplete()
	{
	// Get the status from the configratin file, cause this is NIF level simulation
	TInt modActStatus;
	const CTestConfigItem* item = CfgFile()->Item(KContextModifyActivate);
	CTestConfig::GetElement(item->Value(), KStdDelimiter, 0, modActStatus);		// The 3rd parameter (3) represents the index of the variable on the config file line
	LOG(_LIT(string1,"Modify Active PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(), modActStatus);)				
	iParameter.iReasonCode=modActStatus;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
	// Raise the event to uplayer
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iPending = EFalse;
	iNif->RaiseEvent(KContextModifyActiveEvent, paraPckg);
	};

void CNifPDPContextTNif::PdpTFTModifyComplete()
	{
	LOG(_LIT(string1,"TFT Modify PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(),iParameter.iContextInfo.iStatus );)				
	iParameter.iReasonCode=KErrNone;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusUnknown;
	// Raise the event to uplayer
	TPckg<TContextParameters> paraPckg(iParameter) ;  
	iPending = EFalse;
	iNif->RaiseEvent(KContextTFTModifiedEvent, paraPckg);
	};

void CNifPDPContextTNif::PdpPrimaryComplete()
	{
	// Get the status from the configratin file, cause this is NIF level simulation
	TInt activateStatus;
	const CTestConfigItem* item = CfgFile()->Item(KContextActivateEntry);
	CTestConfig::GetElement(item->Value(), KStdDelimiter, 0, activateStatus);		// The 3rd parameter (3) represents the index of the variable on the config file line
	
	iParameter.iReasonCode=activateStatus;
	iParameter.iContextInfo.iStatus = RPacketContext::EStatusActive;
	LOG(_LIT(string1,"Activate Primary PDP Context %d completed with error %d");)
	LOG(PdpLog::Printf(string1, ContextId(), activateStatus);)				
	// Predefined Successful Operation
	if (activateStatus==KErrNone)
		{
		// Get the predefined Negotiate Qos Value from the configuration file.
 		#ifdef SYMBIAN_NETWORKING_UMTSR5  
 			RPacketQoS::TQoSR5Negotiated negR5Qos;
 			
 			negR5Qos.iTrafficClass=(RPacketQoS::TTrafficClass)
 				CfgFile()->ItemValue(KQosNegTrafficClass, KDefaultValue);
 			negR5Qos.iDeliveryOrderReqd=(RPacketQoS::TDeliveryOrder)
 				CfgFile()->ItemValue(KQosNegDeliveryOrder, KDefaultValue);
 			negR5Qos.iDeliverErroneousSDU=(RPacketQoS::TErroneousSDUDelivery)
 				CfgFile()->ItemValue(KQosNegErroneousSDUDelivery, KDefaultValue);
 			negR5Qos.iMaxSDUSize=CfgFile()->ItemValue(KQosNegMaxSDUSize, KDefaultValue);
 			negR5Qos.iMaxRate.iUplinkRate=
 				CfgFile()->ItemValue(KQosNegUpBitRate, KDefaultValue);
 			negR5Qos.iMaxRate.iDownlinkRate=
 				CfgFile()->ItemValue(KQosNegDownBitRate, KDefaultValue);
 			negR5Qos.iBER=(RPacketQoS::TBitErrorRatio)
 				CfgFile()->ItemValue(KQosNegBitErrorRatio, KDefaultValue);
 			negR5Qos.iSDUErrorRatio=(RPacketQoS::TSDUErrorRatio)
 				CfgFile()->ItemValue(KQosNegSDUErrorRatio, KDefaultValue);
 			negR5Qos.iTrafficHandlingPriority=(RPacketQoS::TTrafficHandlingPriority)
 				CfgFile()->ItemValue(KQosNegTrafficHandlingPriority, KDefaultValue);
 			negR5Qos.iTransferDelay=CfgFile()->ItemValue(KQosNegTransferDelay, KDefaultValue);
 			negR5Qos.iGuaranteedRate.iUplinkRate=CfgFile()->ItemValue(KQosNegGuaranteedUpRate, KDefaultValue);
 			negR5Qos.iGuaranteedRate.iDownlinkRate=CfgFile()->ItemValue(KQosNegGuaranteedDownRate, KDefaultValue);
 
 			negR5Qos.iSourceStatisticsDescriptor=(RPacketQoS::TSourceStatisticsDescriptor)
 				CfgFile()->ItemValue(KQosNegSourceStatisticsDescriptor, KDefaultValue);
 			negR5Qos.iSignallingIndication=CfgFile()->ItemValue(KQosNegSignallingIndication, KDefaultValue);
 
 			iParameter.iContextConfig.SetUMTSQoSNeg(negR5Qos);
 		#else
 			RPacketQoS::TQoSR99_R4Negotiated negQos;
 			negQos.iTrafficClass=(RPacketQoS::TTrafficClass)
 				CfgFile()->ItemValue(KQosNegTrafficClass, KDefaultValue);
 			negQos.iDeliveryOrderReqd=(RPacketQoS::TDeliveryOrder)
 				CfgFile()->ItemValue(KQosNegDeliveryOrder, KDefaultValue);
 			negQos.iDeliverErroneousSDU=(RPacketQoS::TErroneousSDUDelivery)
 				CfgFile()->ItemValue(KQosNegErroneousSDUDelivery, KDefaultValue);
 			negQos.iMaxSDUSize=CfgFile()->ItemValue(KQosNegMaxSDUSize, KDefaultValue);
 			negQos.iMaxRate.iUplinkRate=
 				CfgFile()->ItemValue(KQosNegUpBitRate, KDefaultValue);
 			negQos.iMaxRate.iDownlinkRate=
 				CfgFile()->ItemValue(KQosNegDownBitRate, KDefaultValue);
 			negQos.iBER=(RPacketQoS::TBitErrorRatio)
 				CfgFile()->ItemValue(KQosNegBitErrorRatio, KDefaultValue);
 			negQos.iSDUErrorRatio=(RPacketQoS::TSDUErrorRatio)
 				CfgFile()->ItemValue(KQosNegSDUErrorRatio, KDefaultValue);
 			negQos.iTrafficHandlingPriority=(RPacketQoS::TTrafficHandlingPriority)
 				CfgFile()->ItemValue(KQosNegTrafficHandlingPriority, KDefaultValue);
 			negQos.iTransferDelay=CfgFile()->ItemValue(KQosNegTransferDelay, KDefaultValue);
 			negQos.iGuaranteedRate.iUplinkRate=CfgFile()->ItemValue(KQosNegGuaranteedUpRate, KDefaultValue);
 			negQos.iGuaranteedRate.iDownlinkRate=CfgFile()->ItemValue(KQosNegGuaranteedDownRate, KDefaultValue);
 			iParameter.iContextConfig.SetUMTSQoSNeg(negQos);
 		#endif 
 		// SYMBIAN_NETWORKING_UMTSR5 	
		}
	else
		{
	
		}
	iNif->PrimaryContextReady();
	};
