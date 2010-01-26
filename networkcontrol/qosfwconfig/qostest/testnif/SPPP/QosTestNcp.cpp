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
// CQoSTestNcp.CPP
// 
//

#include "QosTestNcp.h"
#include <qos_if.h>
#include "NifPdpTsy.h"
#include "NifPdpNif.h"
#include "QoSTestLog.h"
#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <udp_hdr.h>
#include <tcp_hdr.h>

//
// CQoSTestNcp Class
//
CQoSTestNcp::CQoSTestNcp(CQosTestLcp* aLcp, MLowerControl& alowerControl, CPppLcp::TPppProtocol aProtocolType)
:CQoSTestEventBase(),
 iDefaultPrimary(iDefaultPrimaryObj),
 iLcp(aLcp),
 iLowerControl(alowerControl),
 iProtocolType(aProtocolType)
	{
	}

CQoSTestNcp* CQoSTestNcp::NewL(CQosTestLcp* aLcp, MLowerControl& alowerControl, CPppLcp::TPppProtocol aProtocolType)
    {
    CQoSTestNcp* newNcp = new (ELeave) CQoSTestNcp(aLcp, alowerControl, aProtocolType);
    CleanupStack::PushL(newNcp);
    newNcp->ConstructL();
    CleanupStack::Pop(newNcp);
    return newNcp;
    }



//-=========================================================
// MLowerControl methods
// - they're virtuals, hence no inlines.
//-=========================================================
TInt CQoSTestNcp::GetName(TDes& aName)
    {
    return iLowerControl.GetName(aName);
    }

TInt CQoSTestNcp::BlockFlow(TBlockOption aOption)
    {
    return iLowerControl.BlockFlow(aOption);
    }

TInt CQoSTestNcp::GetConfig(TBinderConfig& aConfig)
    {
    return iLowerControl.GetConfig(aConfig);
    }


TInt CQoSTestNcp::Control(TUint aLevel, TUint aName, TDes8& aOption)
	{
	TInt ret;
	if (aLevel==KSOLInterface)
		{
		ret = CQoSTestEventBase::Control(aLevel, aName, aOption);
		if (ret != KErrNotSupported)
			return ret;  // mean we already take care to this call in the in harite classes
			
		_LIT(KUmtsPlugInName, "guqos");
		//LOG(_LIT(string2,"\n Get Network Infomation");)
 		#ifdef SYMBIAN_NETWORKING_UMTSR5  
 			RPacketQoS::TQoSR5Requested reqR5Qos;
 		#else
 			RPacketQoS::TQoSR99_R4Requested reqQos;
 		#endif 
 		// SYMBIAN_NETWORKING_UMTSR5 

		TContextParameters& opt = *(TContextParameters*)aOption.Ptr();
		TSoIfControllerInfo& info = *(TSoIfControllerInfo*)aOption.Ptr();
		TContextId contextId = opt.iContextInfo.iContextId;

		switch (aName)
			{
			case KSoIfControllerPlugIn:		// Get required plug-in 
				{
				LOG(_LIT(string1,"Control(): Get plug-in module name");)
				LOG(PdpLog::Write(string1);)				
				info.iPlugIn = KUmtsPlugInName;
				info.iProtocolId = 360;		// Magic number!!
				return KErrNone;
				}

			//case KSoIfGetNetworkInfo:
				//LOG(PdpLog::Write(string2);)
				// Current GuQos is not asking for this.	
				// TODO: Get the network ID from Commdb
				//return KErrNone;

			case KNifSetDefaultQoS:
				LOG(_LIT(string3,"Control(): Set Default QoS Parameters");)
				LOG(PdpLog::Write(string3);)				
 				#ifdef SYMBIAN_NETWORKING_UMTSR5  
 					opt.iContextConfig.GetUMTSQoSReq(reqR5Qos);
 					iDefaultPrimary().iContextConfig.SetUMTSQoSReq(reqR5Qos);
 				#else
 					opt.iContextConfig.GetUMTSQoSReq(reqQos);
 					iDefaultPrimary().iContextConfig.SetUMTSQoSReq(reqQos);
 				#endif 
 				// SYMBIAN_NETWORKING_UMTSR5
				return KErrNone;

			case KContextCreate:
				{
				contextId = FindAvailId();
				if (contextId == 0x4f) // mean we reach the max context in the system so error 
					{
					LOG(_LIT(string11,"Control(): Create Secondary PDP Context failed : reach max context num");)
					LOG(PdpLog::Printf(string11);)				
					opt.iReasonCode = KErrNotSupported;
					return KErrGeneral;
					}
					
				opt.iContextInfo.iContextId = contextId;
				// Instaniate approriate Secondary PDP context instance				
				LOG(_LIT(string4,"Control(): Initiate create Secondary PDP Context %d");)
				LOG(PdpLog::Printf(string4, contextId);)

#ifdef _NIFSIMTSY
				CNifPDPContextTsy* context=0;
				TRAPD(res, context=CNifPDPContextTsy::NewL(this, aOption));
				LOG(_LIT(string,"\n Create Secondary PDP (Tsy) Context Error: %d");)
#else
				CNifPDPContextTNif* context=0;
				TRAPD(res, context = CNifPDPContextTNif::NewL(this, aOption));
#endif
				if (res==KErrNone)
					{
					AddPDPContext(context);
					return context->HandleControl(aName, aOption);
					}
				else
					{
					LOG(_LIT(string,"Control(): Create Secondary PDP Context error %d");)
					LOG(PdpLog::Printf(string, res);)				
					return res;
					}
				}
				
			case KContextDelete:
				{
				LOG(_LIT(string5,"Control(): Initiate Delete PDP Context %d");)
				LOG(PdpLog::Printf(string5, contextId);)
				break;
				}

			case KContextQoSSet:
				LOG(_LIT(string6,"Control(): Initiate Set QoS of PDP Context %d");)
				LOG(PdpLog::Printf(string6, contextId);)
				break;

			case KContextActivate:
				{
				LOG(_LIT(string7,"Control(): Initiate Activate PDP Context %d");)
				LOG(PdpLog::Printf(string7, contextId);)
				// Find specific context
				CNifPDPContextBase* context = LookForPDPContext(opt.iContextInfo.iContextId);
				// Store time of context activation
				context->iTimeActivated.UniversalTime();
				break;
				}

			case KContextModifyActive:
				LOG(_LIT(string8,"Control(): Initiate Modify Active PDP Context %d");)
				LOG(PdpLog::Printf(string8, contextId);)				
				break;

			case KContextTFTModify:
				LOG(_LIT(string9,"Control(): Initiate Modify TFT of PDP Context %d");)
				LOG(PdpLog::Printf(string9, contextId);)				
				break;
				
			default:
				return KErrNotSupported;
			}
			
		//contextId = opt.iContextInfo.iContextId;
		CNifPDPContextBase* pdpContext = LookForPDPContext(contextId);
		if (pdpContext)
			{
			//if (aName == KContextDelete) 
			//	FreeId( contextId);
			pdpContext->iPending = ETrue;
			return pdpContext->HandleControl(aName, aOption);
			}
		else
			{
			LOG(_LIT(string1,"Control(): PDP Context Id %d Not Found");)
			LOG(PdpLog::Printf(string1, contextId);)				
			opt.iReasonCode = KErrArgument; // we need to set this so the Qos will know why it fail 
			return KErrNotFound;
			}
		}
	return iLowerControl.Control( aLevel, aName, aOption);
	};


CQoSTestNcp::~CQoSTestNcp()
	{

#ifdef _NIFSIMTSY
	RecoverETel();
#else
	delete iConfigFile;
	iConfigFile=NULL;
	iFs.Close();
#endif

	delete iNetworkMonitor;
	iNetworkMonitor=NULL;

	if(iContexts)
		{
		// send context shutdown events for any remaining contexts
		while (iContexts->Count() != 0)
		{
			TUint index = iContexts->Count() - 1; // the indexing is zero based

			// Send a subconnection closed event for this context
			TSubConnectionClosedEvent connectionClosedEvent;
			connectionClosedEvent.iTotalUplinkDataVolume = iContexts->At(index)->iDataRecvSize;
			connectionClosedEvent.iTotalDownlinkDataVolume = iContexts->At(index)->iDataSentSize;
			connectionClosedEvent.iTimeClosed.UniversalTime();
			connectionClosedEvent.iSubConnectionUniqueId = iContexts->At(index)->ContextId();
			TPckg<TSubConnectionClosedEvent> connectionClosedEventPtr(connectionClosedEvent);
            //REMEK -> here no good!
			//iNotify->NifEvent(ESubConnectionEvent, ESubConnectionClosed, connectionClosedEventPtr);

			CNifPDPContextBase* temp = iContexts->At(index);
			iContexts->Delete(index); // delete it from the array
			delete temp;              // delete it from the heap	
		}

		delete iContexts;
		iContexts = NULL;
		}

	if (iNextAvailableId)
		delete iNextAvailableId;

	};

void  CQoSTestNcp::ConstructL()
	{
	// Construct the PDP context array.
	iContexts=new (ELeave) CArrayFixFlat<CNifPDPContextBase*>(KDefaultGranularity);

	// init all context available
	TBool IdAvail= false;
	iNextAvailableId =  new(ELeave) CArrayFixFlat<TBool>(20);
	for (TInt i=0;i<20;i++)	iNextAvailableId->AppendL(IdAvail);

	// Construct the Test Configuration File instance
	User::LeaveIfError(iFs.Connect());
	iConfigFile=CTestConfig::NewLC(iFs,KConfigFileDir,KConfigFilename);	
	CleanupStack::Pop();

#ifdef _NIFSIMTSY	
	iNetworkMonitor=CNifNetworkMonitorTsy::NewL(this);	
	
	TRAPD(ret,InitETelL());
	if (ret!=KErrNone)
		RecoverETel();
	
#else // NifSIMNif
	// if any init to file mode 
#endif	
	};

const TInt KMaxContextPool = 12;
TUint8 
CQoSTestNcp::FindAvailId(void)
	{
	TUint8 offset=0x4f;
	
	for(offset=1 ; offset < KMaxContextPool ; offset++)
		if ((*iNextAvailableId)[offset] == false )
		{
			(*iNextAvailableId)[offset] = true;
			return offset;
		}
	return offset;
	}

void 
CQoSTestNcp::FreeId(TInt contextId)
	{
	(*iNextAvailableId)[contextId] = false;
	}

void CQoSTestNcp::GetPhoneInfoL(const TDesC& aLoadedTsyName, RTelServer::TPhoneInfo& aInfo)
//
//	Assumes aloadedTsyName has no ".TSY" appendage
//	Finds the phone information for the TSY just loaded. 
//	Assumes just one phone in that TSY - or that every phone in it is equally useful.
//
	{

	TInt count;
	User::LeaveIfError(iTelServer.EnumeratePhones(count));
	if (count<=0)
		User::Leave(KErrNotFound);
	TBool found=EFalse;
	for (TInt i=0; i<count; i++)
		{
		TBuf<KCommsDbSvrMaxFieldLength> currentTsyName;
		User::LeaveIfError(iTelServer.GetTsyName(i,currentTsyName));		
		TInt r=currentTsyName.Locate('.');
		if (r!=KErrNotFound)
			currentTsyName.SetLength(r);
		if (currentTsyName.CompareF(aLoadedTsyName)==KErrNone)
			{
			User::LeaveIfError(iTelServer.GetPhoneInfo(i,aInfo));
			found=ETrue;
			break;
			}	
		}
	if (!found)
		User::Leave(KErrNotFound);
	};


void CQoSTestNcp::AddPDPContext(CNifPDPContextBase* aContext)
{
	TRAPD(res, iContexts->AppendL(aContext););

	if (res == KErrNone)
	{
		// Send a subconnection opened event for the context
		TSubConnectionOpenedEvent connectionOpenedEvent;
		connectionOpenedEvent.iSubConnectionUniqueId = aContext->ContextId();
		TPckg<TSubConnectionOpenedEvent> connectionOpenedEventPtr(connectionOpenedEvent);
		//iNotify->NifEvent(ESubConnectionEvent, ESubConnectionOpened, connectionOpenedEventPtr);
	}
	else
	{	
		LOG(_LIT(string1,"AddPDPContext(): iContexts->AppendL() failed with error %d");)
		LOG(PdpLog::Printf(string1, res);)	
		// AppendL failed so cleanup thus avoiding potential memory leak here
		delete aContext;
	}
};

CNifPDPContextBase* CQoSTestNcp::LookForPDPContext(TContextId aNextId)
	{
	TInt i;
	for (i=0;i<iContexts->Count();i++)
		{
		if (iContexts->At(i)->ContextId()==aNextId)
			return iContexts->At(i);
		}
	return NULL;
	};

CNifPDPContextBase* CQoSTestNcp::PrimaryPDPContext()
	{
	TInt i;
	for (i=0;i<iContexts->Count();i++)
		if (iContexts->At(i)->ContextType()==EPrimaryContext)
			return iContexts->At(i);
	return NULL;
	}
	
CNifPDPContextBase* CQoSTestNcp::FirstExistingPDPContext()
	{
	return iContexts->At(0);
	}

void CQoSTestNcp::RemovePDPContext(CNifPDPContextBase* aContext)
	{
	// send a subconnection closed event to nifman
	TSubConnectionClosedEvent connectionClosedEvent;
	connectionClosedEvent.iTotalUplinkDataVolume = aContext->iDataRecvSize;
	connectionClosedEvent.iTotalDownlinkDataVolume = aContext->iDataSentSize;
	connectionClosedEvent.iTimeClosed.UniversalTime();
	connectionClosedEvent.iSubConnectionUniqueId = aContext->ContextId();
	TPckg<TSubConnectionClosedEvent> connectionClosedEventPtr(connectionClosedEvent);
	//iNotify->NifEvent(ESubConnectionEvent, ESubConnectionClosed, connectionClosedEventPtr);

	// send a context deleted event to guqos if it doesn't know already
	if (aContext->IsNifmanInitiatedDelete())
		{
		TPckg<TContextParameters> paraPckg(aContext->iParameter);
		MNifEvent* nifevent;
		nifevent = iNifEvent.iEvent;
		nifevent->Event((CProtocolBase*)this, KContextDeleteEvent, paraPckg);
		}
	
	// delete the context from our array here in the test nif
	TInt i;
	for (i=0;i<iContexts->Count();i++)
		if (iContexts->At(i)==aContext)
			{
				FreeId( aContext->iParameter.iContextInfo.iContextId); 
				
				CNifPDPContextBase* temp = iContexts->At(i);
				iContexts->Delete(i); // delete it from the array
				delete temp;          // delete it from the heap
				
				break;
			}

	// have we any contexts left? if not potentially there are none left in this nif
	if (iContexts->Count() == 0)
		{
		// notify LCP of this
		iLcp->NcpDown();
		}

}

TInt CQoSTestNcp::StartPrimaryContext()
	{
	
	// Assign a Context ID for primary context.
	TContextId contextId = FindAvailId();//IDOCHANGE ID ContextId();
	iDefaultPrimary().iContextInfo.iContextId = contextId;

	// Set Primary Context Type.
	iDefaultPrimary().iContextType = EPrimaryContext;
 

	// TODO:What type of Pdp context need to be created? which NCPIP? From CommDb.
	// TODO:Get default configuration from Commdb (APN name, Pdp Type?)
	RPacketContext::TProtocolType pdpType = (RPacketContext::TProtocolType)3;
	iDefaultPrimary().iContextConfig.SetPdpType(pdpType);
	RPacketContext::TGSNAddress name;
	iDefaultPrimary().iContextConfig.SetAccessPointName(name);

	// Instaniate approriate PDP context instance.
	LOG(_LIT(string1,"Creating Primary PDP Context %d");)
	LOG(PdpLog::Printf(string1, contextId);)
	
#ifdef _NIFSIMTSY
		CNifPDPContextTsy* aTsyContext=0;
#else
		CNifPDPContextTNif* aNifContext=0;
#endif
	
#ifdef _NIFSIMTSY
	TRAPD(res, aTsyContext=CNifPDPContextTsy::NewL(this, iDefaultPrimary));
	if (res==KErrNone)
		{
		AddPDPContext(aTsyContext);
		LOG(_LIT(string1,"Create Primary PDP Tsy Context Successfully");)
		LOG(PdpLog::Write(string1);)				
		aTsyContext->ActivatePrimaryContext(iDefaultPrimary);
		}
	else
		{
		LOG(_LIT(string1,"\n Create Primary PDP Tsy Context Error: %d");)
		LOG(PdpLog::Printf(string1, res);)				
		}
#else
	TRAPD(res, aNifContext = CNifPDPContextTNif::NewL(this, iDefaultPrimary));
	if (res == KErrNone)
		{
		AddPDPContext(aNifContext);
		LOG(_LIT(string1,"Create Primary PDP Context successful");)
		LOG(PdpLog::Write(string1);)				
		aNifContext->ActivatePrimaryContext(iDefaultPrimary);
		}
	else
		{
		LOG(_LIT(string1,"Create Primary PDP Context error %d");)
		LOG(PdpLog::Printf(string1, res);)				
		}
#endif

	return KErrNone;
	}

void CQoSTestNcp::PrimaryContextReady()
	{
	LOG(_LIT(string1,"Primary Context Ready");)
	LOG(PdpLog::Write(string1);)			
	iIsPrimaryContextReady=ETrue;
	if (iIsPPPReady)
		{
		PrimaryPDPContext()->EventPrimaryActive();
		iLcp->QoSBinderLinkUp(iProtocolType);
		}
	}

void CQoSTestNcp::PppNegotiationComplete()
	{
	LOG(_LIT(string1,"PPP Negotiation Complete");)
	LOG(PdpLog::Write(string1);)			
	


#ifdef _NIFSIMTSY	
	// when we want to be notify we need to start it 
	//LOG(_LIT(string2,"\n Network Status Monitor started  ");)
	//LOG(PdpLog::Write(string2);)			
	//iNetworkMonitor->StartToMonitor();
#endif // NifSIMNif

	iIsPPPReady = ETrue;
	if (iIsPrimaryContextReady)
		{
		PrimaryContextReady();
		}
	}


/**
 * Return the number of sub-connections within this connection
 * @param none
 */
TUint CQoSTestNcp::EnumerateSubConnections()
	{
	return iContexts->Count();
	}
	
/**
 * Store information about a specified subConnection in the supplied TSubConnectionInfo&
 * @param aSubConnectionInfo On return, contains the class with all its members filled in 
 * @param aIndex A number between one and the total number of subconnections in this connection
 * @return Error code KErrNone
 */
TInt CQoSTestNcp::GetSubConnectionInfo(const TUint aIndex, TSubConnectionInfo& aSubConnectionInfo)
	{
	aSubConnectionInfo.iSubConnectionUniqueId = iContexts->At(aIndex)->ContextId();

	// Test harness is only of use using connections supporting sub-connections i.e gprs and wcdma
	// Therefore certain fields in this dummy test nif are hardcoded to represent gprs connections
	aSubConnectionInfo.iConnectionType = EConnectionGPRS;

	aSubConnectionInfo.iTimeStarted = iContexts->At(aIndex)->TimeContextActivated();
	
	return KErrNone;
	}

/**
 * Store information about a specified subConnection in the supplied TSubConnectionInfo&
 * @param aSubConnectionInfo On return, contains the class with all its members filled in. Passed in with the subconnection ID already filled in.
 * @return Error code KErrNone
 */
TInt CQoSTestNcp::GetSubConnectionInfo(TSubConnectionInfo& aSubConnectionInfo)
	{
	TInt index = LookForIndex(aSubConnectionInfo.iSubConnectionUniqueId);
	if (index<0)
		return index;
	return GetSubConnectionInfo(index, aSubConnectionInfo);
	}

/**
 * Given a subconnection ID find the position of it in the array of contexts
 * @param aSubConnectionUniqueId The unique ID which is to be searched for
 * @return An index into the array or KErrNotFound
 */	
TInt CQoSTestNcp::LookForIndex(const TSubConnectionUniqueId aSubConnectionUniqueId)
	{
	for (TInt loopCount=0;loopCount<iContexts->Count();++loopCount)
		{
		// the context ID is signed and so we will lose half the range of unique IDs as generated by the NIF
		if (iContexts->At(loopCount)->ContextId()==static_cast<TInt>(aSubConnectionUniqueId))
			return loopCount;
		}
	return KErrNotFound;
	}
	
/**
 * Add the details of this packet onto the current total for the correct subconnection. This is found by passing in the port number to each in turn. If the port number matches that used by this context them the size will be increased.
 * @param aPortNo The port number used in the packet by GUQoS to identify which context this packet passes over
 * @param aDataSentSize The size of the packet
 */	
void CQoSTestNcp::StoreDataSent(const TUint aPortNo, const TUint aDataSentSize)
{
	CNifPDPContextBase* context = FindContext(static_cast<TContextId>(aPortNo)); // the port number is the context ID
	if (context == 0)
		return;
	TInt ret = context->StoreDataSent(aDataSentSize);
	if(ret>0)
	{	
		// a return greater than zero indicates the threhold has been crossed.
    	// iNotify->NotifyDataSent(context->ContextId(), ret);
    	//REMEK: this method isn't called
    	//REMEK: replace this functionality with proper datamonitoring.
	}
}

/**
 * Add the details of this packet onto the current total for the correct subconnection. Match context looks for a context with appropriate filters in its TFT.
 * @param aPortNo The port number used in the packet by GUQoS to identify which context this packet passes over
 * @param aDataSentSize The size of the packet
 */		
void CQoSTestNcp::StoreDataReceived(RMBufChain& aPacket)
{
	// get the length
	TUint length(0);
	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);
	length = info->iLength;

	// find the correct context
	CNifPDPContextBase* context = MatchContext(aPacket);
	if (context != 0)
	{
		TInt ret = context->StoreDataReceived(length);
		if(ret>0)
		{	
			// a return greater than zero indicates the threhold has been crossed.
    		// iNotify->NotifyDataReceived(context->ContextId(), ret);
        	//REMEK: this method isn't called
        	//REMEK: replace this functionality with proper datamonitoring.    		
		}
	}
}

/**
 * Get the amount of data transferred on a particular context
 * @param aSubConnectionUniqueId The subconnection ID in which you are interested
 * @param aSentBytes Gets filled in with the number of bytes sent
 * @param aReceivedBytes Gets filled in with the number of bytes received
 * @return KErrNone
 */	
TInt CQoSTestNcp::GetDataTransferred(TSubConnectionUniqueId aSubConnectionUniqueId, TUint& aSentBytes, TUint& aReceivedBytes)
	{
	CNifPDPContextBase* context = LookForPDPContext((TContextId)aSubConnectionUniqueId);
	if (context == 0)
		return KErrNotFound;
	aSentBytes = context->iDataSentSize;
	aReceivedBytes = context->iDataRecvSize;
	return KErrNone;
	}
	
/**
 * Set the granularity of the notification for data sent on this context
 * @param aSubConnectionUniqueId The subconnection ID in which you are interested
 * @param aGranularity The granularity you want (in bytes) will affect when the next notification happens
 * @return A system wide error code
 */	
TInt CQoSTestNcp::SetDataSentNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity)
{
	CNifPDPContextBase* context = LookForPDPContext((TContextId)aSubConnectionUniqueId);
	if (context == 0)
		return KErrNotFound;

	TInt ret = context->SetDataSentNotificationGranularity(aGranularity);
	if (ret > 0)
	{
		//iNotify->NotifyDataSent(aSubConnectionUniqueId, ret);
    	//REMEK: this method isn't called
    	//REMEK: replace this functionality with proper datamonitoring.		
		return KErrNone;
	}

	return ret;
}

/**
 * Cancel the data sent notification for this context
 * @param aSubConnectionUniqueId The subconnection ID in which you are interested
 * @return A system wide error code
 */		
TInt CQoSTestNcp::CancelDataSentNotification(TSubConnectionUniqueId aSubConnectionUniqueId)
{
	CNifPDPContextBase* context = LookForPDPContext((TContextId)aSubConnectionUniqueId);
	if (context == 0)
		return KErrNotFound;

	TInt ret = context->CancelDataSentNotification();
	if (ret > 0)
	{
		//iNotify->NotifyDataReceived(aSubConnectionUniqueId, ret);
    	//REMEK: this method isn't called
    	//REMEK: replace this functionality with proper datamonitoring.		
		return KErrNone;
	}

	return ret;
}
	
/**
 * Set the granularity of the notification for data received on this context
 * @param aSubConnectionUniqueId The subconnection ID in which you are interested
 * @param aGranularity The granularity you want (in bytes) will affect when the next notification happens
 * @return A system wide error code
 */	
TInt CQoSTestNcp::SetDataReceivedNotificationGranularity(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aGranularity)
	{
	CNifPDPContextBase* context = LookForPDPContext((TContextId)aSubConnectionUniqueId);
	if (context == 0)
		return KErrNotFound;
	return context->SetDataRecvNotificationGranularity(aGranularity);
	}
	
/**
 * Cancel the data received notification for this context
 * @param aSubConnectionUniqueId The subconnection ID in which you are interested
 * @return A system wide error code
 */	
TInt CQoSTestNcp::CancelDataReceivedNotification(TSubConnectionUniqueId aSubConnectionUniqueId)
	{
	CNifPDPContextBase* context = LookForPDPContext((TContextId)aSubConnectionUniqueId);
	if (context == 0)
		return KErrNotFound;
	return context->CancelDataReceivedNotification();
	}

CNifPDPContextBase* CQoSTestNcp::FindContext(const TContextId aContextId)
/**
 * Given a context identifier will search the available contexts looking for one that matches. Will return a pointer to the appropriate context. If there is no match then a pointer to the primary will be returned. If there is no primary then something is wrong - a NULL pointer will be returned.
 * @param aContextId The context identifier you want to search for.
 * @return Pointer to a context (or NULL if no match exists which would be a bad situation).
 */
{
	CNifPDPContextBase* primaryContext = 0;

	TUint numOfContexts = iContexts->Count();
	for (TUint loopCount=0; loopCount<numOfContexts; loopCount++)
	{
		if (iContexts->At(loopCount)->ContextId() == aContextId)
		{	// perfect match
			return iContexts->At(loopCount);
		}
		if (iContexts->At(loopCount)->ContextType() == EPrimaryContext)
		{	// take note of the primary incase we don't find an appropriate secondary
			primaryContext = iContexts->At(loopCount);
		}
	}

	LOG(PdpLog::Printf(_L("Didn't find a context with identifier: %d, returning primary"), aContextId);)
	return primaryContext;
}

CNifPDPContextBase* CQoSTestNcp::MatchContext(const RMBufChain& aPacket)
/*
 * Given a packet (received from the network) will search the available contexts for one with a TFT that matches. Will return a pointer to the appropriate context. If there is no match then a pointer to the primary will be returned. If there is no primary then something is wrong - a NULL pointer will be returned.
 * @param aPacket The packet to attempt to match to a PDP context.
 * @return Pointer to a context (or NULL if no match exists which would be a bad situation).
 */
{
	TUint dstPort(0);
	// first analyse the packet in more detail, find out where it came from and who it is going to
	TInet6HeaderIP4* ip4 = (TInet6HeaderIP4*) aPacket.First()->Next()->Ptr();
	if (ip4->Version() != 4)
	{	// Strange IP version (probably 6)
		LOG(PdpLog::Printf(_L("Received non-IPv4 packet in IPv4 specific method CPppNcpIp::StoreDataReceived()... ignoring wrt subconnection stats"));)
	}
	else
	{	// IPv4...
		switch (ip4->Protocol())
		{
		case KProtocolInetUdp:
			{
				TInet6HeaderUDP* udp = (TInet6HeaderUDP*) ip4->EndPtr();
				dstPort = udp->DstPort();
				break;
			}
		case KProtocolInetTcp:
			{
				TInet6HeaderTCP* tcp = (TInet6HeaderTCP*) ip4->EndPtr();
				dstPort = tcp->DstPort();
				break;
			}
		default:
			{
			LOG(PdpLog::Printf(_L("Non supported IP based protocol received in CQoSTestNcp::MatchContext()"));)
			break;
			}
		} // end of switch
	}
	
	// now need to look through the contexts for a match (or the primary if no match)
	CNifPDPContextBase* primaryContext = 0;
	TUint numOfContexts = iContexts->Count();
	for (TUint loopCount=0; loopCount<numOfContexts; loopCount++)
	{
		TTFTInfo tft;
		iContexts->At(loopCount)->iParameter.iContextConfig.GetTFTInfo(tft);
		TUint numOfFilters = tft.FilterCount();

		tft.SetToFirst();
		for (TUint filterIter=0; filterIter<numOfFilters; filterIter++)
		{
			RPacketContext::TPacketFilterV2 filter;
			if (tft.NextPacketFilter(filter) != KErrNone)
			{
				LOG(PdpLog::Printf(_L("NextPacketFilter returned an error"));)
			}
			// for now just check the destination port number
			if ((dstPort >= static_cast<TUint>(filter.iDestPortMin)) && (dstPort <= static_cast<TUint>(filter.iDestPortMax))) // casts to remove compile warnings
			{	
				// perfect match
				return iContexts->At(loopCount);
			}
		}

		if (iContexts->At(loopCount)->ContextType() == EPrimaryContext)
		{	
			// take note of the primary incase we don't find an appropriate secondary
			primaryContext = iContexts->At(loopCount);
		}
	}

	LOG(PdpLog::Printf(_L("Didn't match a secondary context with this packet, returning primary"));)
	return primaryContext;
}

#ifdef _NIFSIMTSY

void CQoSTestNcp::RecoverETel()
	{
	iPacketNetwork.Close();
	iPhone.Close();
	if(iIsTsyLoaded)
		{
		// Get TSY Name.
		const TUint KSlashChar='\\';
		TBuf<KCommsDbSvrMaxFieldLength> tsyName;	
		TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(MODEM);
		columnName.Append(TChar(KSlashChar));
		columnName.Append(TPtrC(MODEM_TSY_NAME));	
		iNotify->ReadDes(columnName, tsyName);
		// Unload the TSY
		iTelServer.UnloadPhoneModule(tsyName);
		iIsTsyLoaded=EFalse;
		}
	iTelServer.Close();	// Phone module unloaded automatically		
	};
	
void CQoSTestNcp::InitETelL()
	{
	User::LeaveIfError(iTelServer.Connect());
	//Get TSY Name.
	const TUint KSlashChar='\\';
	TBuf<KCommsDbSvrMaxFieldLength> tsyName =_S("SIM.TSY");	
	TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(MODEM);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(MODEM_TSY_NAME));	
//	iNotify->ReadDes(columnName, tsyName);

	User::LeaveIfError(iTelServer.LoadPhoneModule(tsyName));
	iIsTsyLoaded=ETrue;
	User::LeaveIfError(iTelServer.SetExtendedErrorGranularity(RTelServer::EErrorExtended));

	RTelServer::TPhoneInfo info;
	TInt r=tsyName.Locate('.');
	if (r!=KErrNotFound)
		tsyName.SetLength(r);
	GetPhoneInfoL(tsyName, info);	// TODO:If this leaves, TSY should be unloaded
									// when RTelServer handle is closed.
	User::LeaveIfError(iPhone.Open(iTelServer, info.iName));
	User::LeaveIfError(iPacketNetwork.Open(iPhone));


	RPhone::TStatus status;
	User::LeaveIfError(iPhone.GetStatus(status));
	if (status.iModemDetected==RPhone::EDetectedNotPresent || status.iModemDetected==RPhone::EDetectedUnknown)
		{
		// Use Synchronous Call here to initialise modem.
		iPhone.Initialise();
		}
	};

#endif //_NIFSIMTSY

//
// Utility functions called from CQoSTestLcp
//

void CQoSTestNcp::SetUpperControl(const ESock::MUpperControl* aControl)
	{
	iUpperControl = aControl;
	}
	
TBool CQoSTestNcp::MatchesUpperControl(const ESock::MUpperControl* aControl) const
	{
	return aControl == iUpperControl;
	}
