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
// Implementation file for the Nif ProtocolIntf
// 
//

/**
 @file DummyNifFlow.cpp
*/

#include <e32std.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <elements/nm_messages_base.h>
#include <elements/nm_messages_child.h>

#include "ss_subconnflow.h"
#include <comms-infras/ss_log.h>
#include "DummyNifFlow.h"
#include "Dummynifbinder.h"
#include "DummyProvision.h"
#include "HungryNifFlow.h"			// for KHungryNifFlowImplementationUid

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;

_LIT8(KNif,"DummyNif");

// Restore when required:
//
// _LIT8(KDummyFlow,"Flow");

CDummyNifSubConnectionFlowFactory* CDummyNifSubConnectionFlowFactory::NewL(TAny* aConstructionParameters)
/**
Constructs a Default SubConnection Flow Factory

@param aConstructionParameters construction data passed by ECOM

@returns pointer to a constructed factory
*/
	{
	CDummyNifSubConnectionFlowFactory* ptr = new (ELeave) CDummyNifSubConnectionFlowFactory(TUid::Uid(KDummyNifFlowImplementationUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
	return ptr;
	}


CDummyNifSubConnectionFlowFactory::CDummyNifSubConnectionFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
:	CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
Default SubConnection Flow Factory Constructor

@param aFactoryId ECOM Implementation Id
@param aParentContainer Object Owner
*/
	{
	}


CSubConnectionFlowBase* CDummyNifSubConnectionFlowFactory::DoCreateFlowL(CProtocolIntfBase* aProtocolIntf, TFactoryQueryBase& aQuery)
	{
	const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
	return CDummyNifSubConnectionFlow::NewL(*this, query.iSCprId, aProtocolIntf);
	}


//=======================================================================================
// CDummyNifSubConnectionFlow
//

CDummyNifSubConnectionFlow::CDummyNifSubConnectionFlow(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
:	CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf)
	{
    __FLOG_OPEN(KCFNodeTag, KNif);
    LOG_NODE_CREATE(KNif, CDummyNifSubConnectionFlow);
    }

CDummyNifSubConnectionFlow::~CDummyNifSubConnectionFlow()
/**
Destroys 'this'
*/
    {
    ASSERT(iBinder4 == NULL);
    ASSERT(iBinder6 == NULL);
    LOG_NODE_DESTROY(KNif, CDummyNifSubConnectionFlow);
    __FLOG_CLOSE;
    }

CDummyNifSubConnectionFlow* CDummyNifSubConnectionFlow::NewL(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
	{
	CDummyNifSubConnectionFlow* flow = new (ELeave) CDummyNifSubConnectionFlow(aFactory, aSubConnId, aProtocolIntf);
	return flow;
	}

// Methods called from Binders

void CDummyNifSubConnectionFlow::FlowDown(TInt aError, TInt aAction /*= MNifIfNotify::EDisconnect*/)
/**
Binder requesting a FlowDown() message be sent to SCPR
*/
    {
	PostFlowDownMessage(aError, aAction);
	iAgentProvision = NULL;
    }

void CDummyNifSubConnectionFlow::Progress(TInt aStage, TInt aError)
/**
Binder requesting a Progress() message be sent to SCPR
*/
    {
    PostProgressMessage(aStage, aError);
    }

const TDummyIp6Provision* CDummyNifSubConnectionFlow::Ip6Provision() const
/**
Return a pointer to the IPv6 provisioning structure
*/
	{
	ASSERT(iProvision);
	return &iProvision->iIp6Provision;
	}

const TDummyAgentProvision* CDummyNifSubConnectionFlow::AgentProvision() const
/**
Return a pointer to the Agent provisioning structure
*/
	{
	ASSERT(iAgentProvision);
	return iAgentProvision;
	}

//-=========================================================
// Messages::ANode methods
//-=========================================================

void CDummyNifSubConnectionFlow::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
/**
Method called on incoming SCPR messages

@param aCFMessage message base
*/
    {
    #ifdef __FLOG_ACTIVE
	    TAny* senderAddr = NULL;
	    const TNodeId* senderNodeId = address_cast<TNodeId>(&aSender);
	    if(senderNodeId)
	    	{
	    	senderAddr = senderNodeId->Ptr();
	    	}
	    	
	    __FLOG_4(_L8("CDummyNifSubConnectionFlow(%x):\tReceivedL() Sender Id: %x, Msg Realm: %X, Msg ID: %d"), this, senderAddr, aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
	#endif

    CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);
	if(aMessage.IsMessage<TEBase::TCancel>())
		{
		// Start flow or any other operation is synchronous and can't be cancelled so ignore this message...
		}
	else if (aMessage.IsMessage<TEBase::TError>())
		{
		SubConnectionError(static_cast<TEBase::TError&>(aMessage).iValue);
		}
	else if (TEChild::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TEChild::TDestroy::EId :
			Destroy();
			break;
		default:
//TODO - logging
			ASSERT(EFalse);
			}
		}
	else if (TCFDataClient::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TCFDataClient::TStart::EId :
			StartFlowL();
			break;
		case TCFDataClient::TStop::EId :
			StopFlow(static_cast<TCFDataClient::TStop&>(aMessage).iValue);
			break;
		case TCFDataClient::TProvisionConfig::EId:
			ProvisionConfig(static_cast<TCFDataClient::TProvisionConfig&>(aMessage).iConfig);
			break;
		case TCFDataClient::TBindTo::EId:
            {
			TCFDataClient::TBindTo& bindToReq = message_cast<TCFDataClient::TBindTo>(aMessage);
			if (!bindToReq.iNodeId.IsNull())
				{
				User::Leave(KErrNotSupported);
				}
			RClientInterface::OpenPostMessageClose(Id(), aSender, TCFDataClient::TBindToComplete().CRef());
            }
			break;
		default:
//TODO - logging
			ASSERT(EFalse);
			}
		}
	else		// realm != TCFMessage::ERealmId
		{
		Panic(EUnexpectedMessage);
		}
    }

//
// Methods for handling received SCPR messages
//

void CDummyNifSubConnectionFlow::StartFlowL()
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow:\tStartFlowL()"));
    if (iDisableStart)
    	{
    	User::Leave(KErrCouldNotConnect);
    	}

    if (iFlowStarted)
    	{
    	//Flow can be started multiple times.
    	//For example a start may fail on an upper layer in which case the upper layer will not be able
    	//to stop our layer and we need to accept start again.
    	PostProgressMessage(KLinkLayerOpen, KErrNone);
    	PostDataClientStartedMessage();
    	return;
    	}

    iFlowStarted = ETrue;

	// If the processing of the ProvisionConfig message failed earlier, send the error response
	// here to the StartFlow message as there is no response to ProvisionConfig.

	User::LeaveIfError(iSavedError);

	AgentProvisionConfigL();		// retrieve and validate the Agent provisioning information

	PostProgressMessage(KLinkLayerOpen, KErrNone);
	PostDataClientStartedMessage();

	if (iBinder4)
   	    {
		iBinder4->BinderReady();
       	}
	if (iBinder6)
		{
		iBinder6->BinderReady();
		}
	}

void CDummyNifSubConnectionFlow::StopFlow(TInt aError)
	{
    __FLOG_1(_L8("CDummyNifSubConnectionFlow:\tStopFlow(%d)"), aError);

	PostProgressMessage(KLinkLayerClosed, aError);
    iFlowStarted = EFalse;
	PostFlowDownMessage(aError);
	iAgentProvision = NULL;
	}


/*
Provisioning description for Dummy CFProtocol Flow:

- on receipt of the ProvisionConfig message, the pointer contained within it is stored
  in iAccessPointConfig and the information contained within the iAccessPointConfig
  array is validated:
	- CDummyProtoProvision must be present.  It is added by the DummyProtoMCPr and populated from CommsDat.
	  It is a 'C' class to take advantage of zero initialisation.
  Any errors are saved in iSavedError - there is no response to ProvisionConfig, so an error
  response is sent later to StartFlow message.

- on receipt of TCFDataClient::TStart:
  - iSavedError is checked and, if non-zero, an Error message is sent to the SCPr
  - TDummyAgentProvision must be present.  It is added by the DummyAgentHandler and populated via calls
	  to the Agent.  It is a 'T' class because it requires no zero initialisation.  If missing,
	  an Error message is signalled to the SCPr.
*/

void CDummyNifSubConnectionFlow::ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData)
/**
Save the provisioning structure pointer passed in the TProvisionConfig message

@param aData pointer to provisioning structure
*/
	{
	__FLOG_0(_L8("CDummyNifSubConnectionFlow:\tProvisionConfig message received"));

	iAccessPointConfig.Close();
	iAccessPointConfig.Open(aConfigData);

	const CDummyProtoProvision* provision = static_cast<const CDummyProtoProvision*>(AccessPointConfig().FindExtension(
			STypeId::CreateSTypeId(CDummyProtoProvision::EUid, CDummyProtoProvision::ETypeId)));
	if (provision == NULL)
		{
		__FLOG_0(_L8("CDummyNifSubConnectionFlow:\tProvisionConfigL() - DummyProto config not found"));
		iSavedError = KErrCorrupt;
		}

	// Save pointer to provisioning information
	iProvision = provision;
	}

void CDummyNifSubConnectionFlow::AgentProvisionConfigL()
/**
Validate and process the Agent provisioning information.

This information is not valid when TProvisionConfig message is received, but only
becomes valid at TCFDataClient::TStart.
*/
	{
    const TDummyAgentProvision* agentProvision = static_cast<const TDummyAgentProvision*>(AccessPointConfig().FindExtension(
    		STypeId::CreateSTypeId(TDummyAgentProvision::EUid, TDummyAgentProvision::ETypeId)));
    if (agentProvision == NULL)
        {
        __FLOG_0(_L8("CDummyNifSubConnectionFlow:\tProvisionConfigL() - DummyProto Agent config not found"));
		User::Leave(KErrCorrupt);
        }

	ASSERT(iAgentProvision == NULL);
	iAgentProvision = agentProvision;
	}

void CDummyNifSubConnectionFlow::Destroy()
/**
Request from SCPR to destroy
*/
	{
	// No-one should be bound to us from above if we are about to disappear.
	ASSERT(iBinder4 == NULL && iBinder6 == NULL);
	DeleteThisFlow();
	}

void CDummyNifSubConnectionFlow::SubConnectionGoingDown()
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow:\tSubConnectionGoingDown"));
	}

void CDummyNifSubConnectionFlow::SubConnectionError(TInt /*aError*/)
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow:\tSubConnectionError"));
	}

MFlowBinderControl* CDummyNifSubConnectionFlow::DoGetBinderControlL()
/**
Return MFlowBinderControl instance.

Called by upper layer for binding

@return MFlowBinderControl instance
*/
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow::DoGetBinderControlL"));
	return this;
	}

//-=========================================================
// MFlowBinderControl methods
//

MLowerControl* CDummyNifSubConnectionFlow::GetControlL(const TDesC8& aProtocol)
/**
Create and return an MLowerControl instance of the specified binder type.

Called from upper layer during binding procedure.

@param aProtocol Protocol type of the binder
@return MLowerControl instance of the protocol type
*/
	{

	MLowerControl* lowerControl = NULL;

	if (aProtocol.CompareF(KProtocol4()) == 0)
		{
        __FLOG(_L8("CDummyNifSubConnectionFlow:\tGetLowerControlL(KProtocol4)"));
		iBinder4 = CDummyNifBinder4::NewL(*this);
		lowerControl = iBinder4;
		}
	else
	if (aProtocol.CompareF(KProtocol6()) == 0)
		{
        __FLOG(_L8("CDummyNifSubConnectionFlow::GetLowerControlL(KProtocol6)"));

        iBinder6 = CDummyNifBinder6::NewL(*this);
		lowerControl = iBinder6;
		}

	ASSERT(lowerControl);
	return lowerControl;
	}

MLowerDataSender* CDummyNifSubConnectionFlow::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
Create and return an MLowerDataSender instance of the specified protocol type.
This is bound to the specified upper layer objects.

Called from upper layer to bind to this layer.

@param aProtocol Protocol type of the binder (same as in GetControlL())
@param aReceiver upper layer's MUpperDataReceiver instance for this binder to associate with
@param aControl upper layer's MUpperControl instance for this binder to associate with
@return MLowerDataSender instance
*/
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow:\tBindL()"));

	MLowerDataSender* lowerDataSender = NULL;

	if (aProtocol.CompareF(KProtocol4()) == 0)
		{
		lowerDataSender = iBinder4->Bind(*aReceiver, *aControl);
		}
	else
	if (aProtocol.CompareF(KProtocol6()) == 0)
		{
		lowerDataSender = iBinder6->Bind(*aReceiver, *aControl);
		}

    iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TActive().CRef());
	ASSERT(lowerDataSender);
	return lowerDataSender;
	}

void CDummyNifSubConnectionFlow::Unbind(MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
Unbind from the upper layer.

Called from the upper layer during unbinding.

@param aReceiver
@param aControl

*/
	{
    __FLOG(_L8("CDummyNifSubConnectionFlow:\tUnbind()"));

	if (iBinder4 && iBinder4->MatchesUpperControl(aControl))
		{
		iBinder4->Unbind(*aReceiver, *aControl);
		delete iBinder4;
		iBinder4 = NULL;
		}
	else if (iBinder6 && iBinder6->MatchesUpperControl(aControl))
		{
		iBinder6->Unbind(*aReceiver, *aControl);
		delete iBinder6;
		iBinder6 = NULL;
		}


    MaybePostDataClientIdle();
	}

ESock::CSubConnectionFlowBase* CDummyNifSubConnectionFlow::Flow()
/**
Return the flow object corresponding to the MFlowBinderControl
*/
	{
	return this;
	}

//
// Utilities for posting SCPR messages
//

void CDummyNifSubConnectionFlow::PostProgressMessage(TInt aStage, TInt aError)
	{
	iSubConnectionProvider.PostMessage(Id(), TCFMessage::TStateChange(Elements::TStateChange(aStage, aError)).CRef());
	}

void CDummyNifSubConnectionFlow::PostDataClientStartedMessage()
	{
	iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStarted().CRef());
	}

void CDummyNifSubConnectionFlow::PostFlowDownMessage(TInt aError, TInt aAction /*= MNifIfNotify::EDisconnect*/)
	{
	if (iFlowStarted)
		{
		iFlowStarted = EFalse;
		iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError, aAction).CRef());
		}
	else
		{
		iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStopped(aError).CRef());
		}
	iAgentProvision = NULL;
	}

void CDummyNifSubConnectionFlow::MaybePostDataClientIdle()
    {
    // Can only send DataClientIdle when the upper layer has unbound and the flow is stopped
	if (iBinder4 == NULL && iBinder6 == NULL)
		{
   		iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
		}
    }

//
// Other utilities
//

void Panic(TDummyNifPanicNum aNum)
	{
	_LIT(KDummyNifPanicString, "DummyProto");
	User::Panic(KDummyNifPanicString, aNum);
	}

