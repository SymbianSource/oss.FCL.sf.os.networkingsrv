// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// IP Connection Provider state definitions.
// 
//

/**
 @file
 @internalComponent
*/

#define SYMBIAN_NETWORKING_UPS

#include "IPCpr.h"
#include "ipcpr_states.h"
#include "IPMessages.h"
#include <gsmerror.h>
#include <comms-infras/ss_nodemessages.h>
#include <comms-infras/ss_nodemessages_internal_esock.h>
#include <comms-infras/ss_datamonitoringprovider.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <comms-infras/ss_nodemessages_scpr.h>
#include <tcprecvwin.h>
#include <cs_genevent.h>
#include <comms-infras/ss_protopt.h>
#include <in_sock.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace IpCprActivities;
using namespace IpCprStates;

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
using namespace TcpAdaptiveReceiveWindow;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

const TInt KNifEMCompatibilityLayerEntireSubConnectionUid = 0;
const TInt KNifEMCompatibilityLayerFakeSubConnectionId = 1;

DEFINE_SMELEMENT( TAwaitingPolicyParams, NetStateMachine::MState, IpCprStates::TContext)
TBool IpCprStates::TAwaitingPolicyParams::Accept()
    {
	return iContext.iMessage.IsMessage<TCFIPMessage::TPolicyParams>();
    }
/**
Security policies
*/

_LIT_SECURITY_POLICY_C1(KIpCprStopSecurityPolicy, ECapabilityNetworkControl);
    
/**
IpCpr Stop capability check transition
*/
DEFINE_SMELEMENT( TCheckStopCapabilities, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TCheckStopCapabilities::DoL()
    {
    MPlatsecApiExt* platsec = reinterpret_cast<MPlatsecApiExt*>(address_cast<TNodeId>(iContext.iSender).Node().FetchNodeInterfaceL(MPlatsecApiExt::KInterfaceId));
    TInt err = platsec->CheckPolicy(KIpCprStopSecurityPolicy);
	User::LeaveIfError(err);
    }

/**

*/
DEFINE_SMELEMENT( TSendPolicyParams, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendPolicyParams::DoL()
    {
	TCFIPMessage::TPolicyParams& policyParams = message_cast<TCFIPMessage::TPolicyParams>(iContext.iMessage);
	// change originator to current ipcpr
//	policyParams.iSender = iContext.Node()();

	RNodeInterface* ctrlProvider = iContext.Node().ControlProvider();


	if(ctrlProvider)
		{
		ctrlProvider->PostMessage(iContext.NodeId(), policyParams);
		}
	else
		{
		User::Leave(KErrCorrupt);
		}
    }

DEFINE_SMELEMENT( TAwaitingSubConnDataTransferred, NetStateMachine::MState, IpCprStates::TContext)
TBool IpCprStates::TAwaitingSubConnDataTransferred::Accept()
	{
	return(iContext.iMessage.IsMessage<TCFMessage::TSubConnDataTransferred>());
	}

DEFINE_SMELEMENT( TProcessSubConnDataTransferred, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TProcessSubConnDataTransferred::DoL()
	{
	TCFMessage::TSubConnDataTransferred& msg = message_cast<TCFMessage::TSubConnDataTransferred>(iContext.iMessage);

	RNodeInterface* ctrlClient = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::ECtrl));
	if(ctrlClient) // Ditch the notifications if there's no one to send to.
		{
		TTime now;
		now.UniversalTime();

		if(msg.iValue1 == KNifEMCompatibilityLayerEntireSubConnectionUid)
			{
		    TSubConnectionClosedEvent wholeConnEvent;
		    wholeConnEvent.iSubConnectionUniqueId = KNifEMCompatibilityLayerEntireSubConnectionUid ;
		    wholeConnEvent.iTotalUplinkDataVolume = msg.iValue2;
		    wholeConnEvent.iTotalDownlinkDataVolume = msg.iValue3;
		    wholeConnEvent.iTimeClosed = now;
			TCFInternalEsock::TSubConnectionClosedEvent wholeConnMsg(wholeConnEvent);
			ctrlClient->PostMessage(iContext.NodeId(), wholeConnMsg);
			}
		else
			{
			TSubConnectionClosedEvent defaultSubConnEvent;
			defaultSubConnEvent.iSubConnectionUniqueId = KNifEMCompatibilityLayerEntireSubConnectionUid;
			defaultSubConnEvent.iTotalUplinkDataVolume = msg.iValue2;
			defaultSubConnEvent.iTotalDownlinkDataVolume = msg.iValue3;
			defaultSubConnEvent.iTimeClosed = now;
			TCFInternalEsock::TSubConnectionClosedEvent subConnMsg(defaultSubConnEvent);
			ctrlClient->PostMessage(iContext.NodeId(), subConnMsg);
			}
		}
	}


DEFINE_SMELEMENT( TSendInitialSubConnectionOpenedEvent, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendInitialSubConnectionOpenedEvent::DoL()
	{
	RNodeInterface* ctrlClient = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::ECtrl));
	if(ctrlClient) // Ditch the notifications if there's no one to send to.
		{
	    TSubConnectionOpenedEvent wholeConnEvent;
	    wholeConnEvent.iSubConnectionUniqueId = KNifEMCompatibilityLayerEntireSubConnectionUid ;
		TCFInternalEsock::TSubConnectionOpenedEvent wholeConnMsg(wholeConnEvent);
		ctrlClient->PostMessage(iContext.NodeId(), wholeConnMsg);
		}
	}

DEFINE_SMELEMENT( TSendSubsequentSubConnectionOpenedEvent, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendSubsequentSubConnectionOpenedEvent::DoL()
	{
	RNodeInterface* ctrlClient = iContext.Node().GetFirstClient<TDefaultClientMatchPolicy>(TClientType(TCFClientType::ECtrl));
	if(ctrlClient) // Ditch the notifications if there's no one to send to.
		{
    	// If it's the default subconnection then use the fake subconnection id.
    	// Otherwise cook one up from the pointer of the SCPR.
    	TUint subConnectionUniqueId = KNifEMCompatibilityLayerFakeSubConnectionId;
    	TUint dataClientCount = iContext.Node().CountClients<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData));
    	if(dataClientCount > 1)
    		{
    		subConnectionUniqueId = reinterpret_cast<TUint>(address_cast<TNodeId>(iContext.iSender).Ptr());
    		}

	    TSubConnectionOpenedEvent defaultSubConnEvent;
	    defaultSubConnEvent.iSubConnectionUniqueId = subConnectionUniqueId;
	    TCFInternalEsock::TSubConnectionOpenedEvent subConnMsg(defaultSubConnEvent);
	    ctrlClient->PostMessage(iContext.NodeId(), subConnMsg);
    	}
	}


#ifndef SYMBIAN_NETWORKING_UPS

_LIT_SECURITY_POLICY_C1(KIpCprStartSecurityPolicy, ECapabilityNetworkServices);

/**
IpCpr Start capability check transition
*/
DEFINE_SMELEMENT( TCheckStartCapabilities, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TCheckStartCapabilities::DoL()
    {
    MPlatsecApiExt* platsec = iContext.iCFMessageSig.FetchPlatsecApiInterfaceL();
    TInt err = platsec->CheckPolicy(KIpCprStartSecurityPolicy);
	User::LeaveIfError(err);
    }

#endif  //SYMBIAN_NETWORKING_UPS


DEFINE_SMELEMENT( TAwaitingSpecialGoneDown, NetStateMachine::MState, IpCprStates::TContext)
TBool IpCprStates::TAwaitingSpecialGoneDown::Accept()
	{
    TCFControlClient::TGoneDown* pGoneDown = message_cast<TCFControlClient::TGoneDown>(&iContext.iMessage);
        if(pGoneDown && //If this is a TCFControlClient::TGoneDown message AND
        iContext.Node().CountClients<TDefaultClientMatchPolicy>(
                        TClientType(TCFClientType::ECtrl, TCFClientType::EAttach)) && //the local node is an attached ipcpr AND
        ((pGoneDown->iValue1 == KErrGprsInsufficientResources) || //the error value suggests the contention management
         (pGoneDown->iValue1 == KErrPacketDataTsyMaxPdpContextsReached) ||
         (pGoneDown->iValue1 == KErrUmtsMaxNumOfContextExceededByNetwork) ||     
         (pGoneDown->iValue1 == KErrUmtsMaxNumOfContextExceededByPhone))
         )
          {
          return ETrue;
          }
        return EFalse;
	}


#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
/*
Constructor for the new parameter set to be sent in parameter bundle while requesting for bearer type.
@param aFamily - Specified the family the parameterset belongs to
	    aType - Specifies the type
@return object of type XBearerInfo
*/
EXPORT_C XBearerInfo* XBearerInfo::NewL(RParameterFamily aFamily, RParameterFamily::TParameterSetType aType)
{
	//call NewL
	XBearerInfo *obj = XBearerInfo::NewL();
	CleanupStack::PushL(obj);
	
	//add the parameterset to the family specified.
	aFamily.AddParameterSetL(obj, aType);
	CleanupStack::Pop(obj);
	
	return obj;	
}

/*
Constructs object of XBearerInfo
@return object of type XBearerInfo
*/
EXPORT_C XBearerInfo* XBearerInfo::NewL()
{
	return new(ELeave)XBearerInfo();
}

//Attribute table for data members of XBearerInfo class. Needed because the class derieves from SMetaData
START_ATTRIBUTE_TABLE(XBearerInfo, XBearerInfo::EUid, XBearerInfo::EId)
	REGISTER_ATTRIBUTE(XBearerInfo, iBearerType, TMetaNumber)
END_ATTRIBUTE_TABLE()

/**
 * State Transition which initialises the Parameter bundle to request for the bearer type.
 */
DEFINE_SMELEMENT(IpCprStates::TInitialiseParams, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TInitialiseParams::DoL()
    {
   	RCFParameterFamilyBundle newBundle;
	//create new bundle
	newBundle.CreateL();
	if(!iContext.Node().iParameterBundle.IsNull())
		{
		iContext.Node().iParameterBundle.Close();
		}
	iContext.Node().iParameterBundle.Open(newBundle);
	//Create new family
	RParameterFamily family = newBundle.CreateFamilyL(KBearerInfo);
	//construct object to be added to the parameterset
	//coverity[returned_pointer]
	// We are trying to initializing the family parameter and returning the pointer to XbearerInfo object.
	XBearerInfo *bearerInfo = XBearerInfo::NewL(family, RParameterFamily::ERequested);
   }

/**
 * State Transition which receives the response message to the bearer type request.
 * Extracts the bearer type and updates it in the NetMCPR
 */
DEFINE_SMELEMENT(IpCprStates::TUpdateProvisionConfigAtStartup, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TUpdateProvisionConfigAtStartup::DoL()
    {
	//Cast the message to response type
    TCFScpr::TGetParamsResponse& paramResponse = message_cast<TCFScpr::TGetParamsResponse>(iContext.iMessage);
	if( (! paramResponse.iFamilyBundle.IsNull()))
		{
		//Open the bundle
		iContext.Node().GetParameterBundle().Open(paramResponse.iFamilyBundle);
		CleanupClosePushL(iContext.Node().GetParameterBundle());
		//Get the family ie. KBearerInfo
		RParameterFamily family = iContext.Node().GetParameterBundle().FindFamily(KBearerInfo);	
		if(!family.IsNull())
			{
			//Find the parameter set
			XBearerInfo *bearerTypePtr = static_cast<XBearerInfo*>(family.FindParameterSet(STypeId::CreateSTypeId(KIpBearerInfoUid, KIpBearerInfoParameterType), RParameterFamily::ERequested));
			if(bearerTypePtr)
				{
				//Set the bearer type in NetMCPR
				CTCPReceiveWindowSize* winSizePtr =	const_cast<CTCPReceiveWindowSize *>(static_cast<const CTCPReceiveWindowSize *>(iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CTCPReceiveWindowSize::ERealmId,CTCPReceiveWindowSize::iId))));
				CSAPSetOpt* protOptPtr = const_cast<CSAPSetOpt *>(static_cast<const CSAPSetOpt *>(iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CSAPSetOpt::EUid,CSAPSetOpt::ETypeId))));
				//Set TCP Receive window value.
				if(winSizePtr && protOptPtr)
					{
					winSizePtr->SetTcpWin(bearerTypePtr->GetBearerType());
					protOptPtr->AddProtocolOptionL(KSolInetTcp, KSoTcpRecvWinAuto, winSizePtr->GetTcpWin());
					protOptPtr->AddProtocolOptionL(KSolInetTcp, KSoTcpMaxRecvWin, winSizePtr->GetTcpMaxWin());
					}		
				}
			}
		// Decrease reference count by calling close of the reference passed in CleanupClosePushL at line no 261
		CleanupStack::PopAndDestroy();
		}
    }

/**
 * State Transition to update the NetMCPR with bearer type recd. after receiving the modulation change notification.
 * Receives the TPlaneNotification message containting the new bearer.
 */
DEFINE_SMELEMENT(IpCprStates::TUpdateProvisionConfigAtModulation, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TUpdateProvisionConfigAtModulation::DoL()
    {
	
    TInt index = 0;
	//Cast the message to TPlaneNotification
    TCFSubConnControlClient::TPlaneNotification& paramsChanged = message_cast<TCFSubConnControlClient::TPlaneNotification>(iContext.iMessage);
    
	//Obtain the pointer to CEventParamsGranted class from the message
    const CEventParamsGranted * event = static_cast<const CEventParamsGranted*>(paramsChanged.iRefCountOwnedNotification->Ptr());
    
	//Obtain the parameter set
 	const XBearerInfo * msg = static_cast<const XBearerInfo *>(event->FindParameterSet(KBearerInfo, index));
	
	//Set the bearer type at NetMCPR
	CTCPReceiveWindowSize* winSizePtr =	const_cast<CTCPReceiveWindowSize *>(static_cast<const CTCPReceiveWindowSize *>(iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CTCPReceiveWindowSize::ERealmId,CTCPReceiveWindowSize::iId))));
	CSAPSetOpt* protOptPtr = const_cast<CSAPSetOpt *>(static_cast<const CSAPSetOpt *>(iContext.Node().AccessPointConfig().FindExtension(STypeId::CreateSTypeId(CSAPSetOpt::EUid,CSAPSetOpt::ETypeId))));
	
	// Set the TCP receive window and maximum value
	if(winSizePtr && protOptPtr)
		{
		winSizePtr->SetTcpWin((const_cast<XBearerInfo*>(msg))->GetBearerType());
		protOptPtr->UpdateProtocolOption(KSolInetTcp, KSoTcpRecvWinAuto, winSizePtr->GetTcpWin());
		protOptPtr->UpdateProtocolOption(KSolInetTcp, KSoTcpMaxRecvWin, winSizePtr->GetTcpMaxWin());
		}
    
    //Decrease the ref count
    paramsChanged.iRefCountOwnedNotification->Close();
    
	}

/**
 * StateTransition which sends request message containing the Parameter bundle to self.
 * When sent to self, it is sent to the service providers.
 */
DEFINE_SMELEMENT(IpCprStates::TSendParamsToSelf, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendParamsToSelf::DoL()
    {
   	ASSERT( ! iContext.Node().GetParameterBundle().IsNull()); // params initialised already
	iContext.iNodeActivity->PostRequestTo(iContext.NodeId(), TCFScpr::TGetParamsRequest(iContext.Node().GetParameterBundle()).CRef());
    }
    
    
/**
 * StateTransition to send bearer characteristics to data clients. The TCP Receive window size is obtained from 
 * Lookup Table using the bearer type obtained. Send the window size data clients. In this case, it will be sent 
 * before TStartDataClient.
 */
DEFINE_SMELEMENT(TSendTransportNotificationToDataClients, NetStateMachine::MStateTransition, IpCprStates::TContext)
void IpCprStates::TSendTransportNotificationToDataClients::DoL()
	{
		
	//Send TTransportNotification message to the data clients. 
	//Initialise the iter variable which will know all data clients.
	//Exclude data clients that are ELeaving otherwise the PostMessage() below will panic.
	
   	TClientIter<TDefaultClientMatchPolicy> iter = iContext.Node().GetClientIter<TDefaultClientMatchPolicy>(TClientType(TCFClientType::EData), TClientType(0, TCFClientType::ELeaving));
	TInt dataCliNum = NULL;
	RNodeInterface* dataClient = iter[dataCliNum];
	//Initialise message with window sizes
	TCFMessage::TTransportNotification message; 
  
  	while (dataClient)
   		{
		//Post messages to the data client.
 		dataClient->PostMessage(iContext.NodeId(), message);
 		dataCliNum++;
 		dataClient = iter[dataCliNum];
   		}
}

#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
