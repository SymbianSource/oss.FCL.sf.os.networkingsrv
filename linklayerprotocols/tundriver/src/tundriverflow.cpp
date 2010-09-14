/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Flow implementation for data path for tunnel driver.
* 
*
*/

/**
 @file tundriverflow.cpp
 @internalTechnology
*/

#include <e32std.h>
#include <ecom/ecom.h>
#include <ecom/implementationproxy.h>
#include <elements/nm_messages_base.h>
#include <elements/nm_messages_child.h>

#include "ss_subconnflow.h"
#include <comms-infras/ss_log.h>
#include "tundriverflow.h"
#include "tundriverbinder.h"
#include "tundriverprovision.h"

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;

#ifdef SYMBIAN_TRACE_ENABLE
_LIT8(KNif,"tundriver");
#endif

CTunDriverSubConnectionFlowFactory* CTunDriverSubConnectionFlowFactory::NewL(TAny* aConstructionParameters)
/**
* CTunDriverSubConnectionFlowFactory::NewL constructs a Default SubConnection Flow Factory
* @param aConstructionParameters construction data passed by ECOM
* @returns pointer to a constructed factory object.
*/
    {
    CTunDriverSubConnectionFlowFactory* ptr = new (ELeave) CTunDriverSubConnectionFlowFactory(TUid::Uid(KTunDriverFlowImplementationUid), *(reinterpret_cast<CSubConnectionFlowFactoryContainer*>(aConstructionParameters)));
    return ptr;
    }


CTunDriverSubConnectionFlowFactory::CTunDriverSubConnectionFlowFactory(TUid aFactoryId, CSubConnectionFlowFactoryContainer& aParentContainer)
:   CSubConnectionFlowFactoryBase(aFactoryId, aParentContainer)
/**
* CTunDriverSubConnectionFlowFactory::CTunDriverSubConnectionFlowFactory is a Default SubConnection Flow Factory
* @param aFactoryId ECOM Implementation Id
* @param aParentContainer Object Owner
*/
    {
    }

CSubConnectionFlowBase* CTunDriverSubConnectionFlowFactory::DoCreateFlowL(CProtocolIntfBase* aProtocolIntf, TFactoryQueryBase& aQuery)
/**
* CTunDriverSubConnectionFlowFactory::DoCreateFlowL will create SubConnection Flow.
* @param aFactoryId ECOM Implementation Id
* @param aParentContainer Object Owner
*/
    {
    const TDefaultFlowFactoryQuery& query = static_cast<const TDefaultFlowFactoryQuery&>(aQuery);
    return CTunDriverSubConnectionFlow::NewL(*this, query.iSCprId, aProtocolIntf);
    }

CTunDriverSubConnectionFlow::CTunDriverSubConnectionFlow(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
:   CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf)
/**
* CTunDriverSubConnectionFlow::CTunDriverSubConnectionFlow is a Default Constructor
* @param aFactoryId ECOM Implementation Id
* @param aParentContainer Object Owner
*/
    {
    __FLOG_OPEN(KCFNodeTag, KNif);
    LOG_NODE_CREATE(KNif, CTunDriverSubConnectionFlow);
    }

CTunDriverSubConnectionFlow::~CTunDriverSubConnectionFlow()
/**
* CTunDriverSubConnectionFlowFactory::~CTunDriverSubConnectionFlow is a Default destructor
* @param aFactoryId ECOM Implementation Id
* @param aParentContainer Object Owner
*/
    {
    delete iBinder4;
    iBinder4 = NULL;
#ifdef IPV6SUPPORT
    delete iBinder6;
    iBinder6 = NULL;
#endif
    LOG_NODE_DESTROY(KNif, CTunDriverSubConnectionFlow);
    __FLOG_CLOSE;
    }

CTunDriverSubConnectionFlow* CTunDriverSubConnectionFlow::NewL(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
/**
* CTunDriverSubConnectionFlowFactory::NewL is a first phase Constructor.
* @param aFactoryId ECOM Implementation Id
* @param aParentContainer Object Owner
*/
    {
    CTunDriverSubConnectionFlow* flow = new (ELeave) CTunDriverSubConnectionFlow(aFactory, aSubConnId, aProtocolIntf);
    return flow;
    }

void CTunDriverSubConnectionFlow::FlowDown(TInt aError, TInt aAction /*= MNifIfNotify::EDisconnect*/)
/**
* CTunDriverSubConnectionFlowFactory::FlowDown will post the message.
* @param aError obtained from the upper CF Layer.
* @param aAction and the action message for aError.
*/
    {
    PostFlowDownMessage(aError, aAction);
    }

void CTunDriverSubConnectionFlow::Progress(TInt aStage, TInt aError)
/**
* CTunDriverSubConnectionFlowFactory::Progress Binder requesting a Progress() message be sent to SCPR
* @param aStage and the action message for aError.
* @param aError obtained from the upper CF Layer.
*/
    {
    PostProgressMessage(aStage, aError);
    }

const TTunDriverIp4Provision* CTunDriverSubConnectionFlow::Ip4Provision() const
/**
* CTunDriverSubConnectionFlowFactory::Ip4Provision
* @param 
* @return iIp4Provision.
*/
    {
    ASSERT(iProvision);
    return &iProvision->iIp4Provision;
    }

#ifdef IPV6SUPPORT
const TTunDriverIp6Provision* CTunDriverSubConnectionFlow::Ip6Provision() const
/**
* CTunDriverSubConnectionFlowFactory::Ip6Provision
* @param 
* @return iIp6Provision.
*/
    {
    ASSERT(iProvision);
    return &iProvision->iIp6Provision;
    }
#endif

//-=========================================================
// Messages::ANode methods
//-=========================================================

#define __FRAMEWORK_PRODUCTION_ERRORHANDLING(c,p) (void)((c)||(p,0))
void CTunDriverSubConnectionFlow::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
/**
* CTunDriverSubConnectionFlowFactory::ReceivedL called on incoming SCPR Messages.
* CTunDriverSubConnectionFlow functions will be called Based on message type
* @param aCFMessage message base
*/
    {
    #ifdef __FLOG_ACTIVE
        TAny* senderAddr = NULL;
        const TNodeId* senderNodeId = address_cast<TNodeId>(&aSender);
        __ASSERT_DEBUG(senderNodeId!=NULL,User::LeaveIfError(KErrCorrupt));
        senderAddr = senderNodeId->Ptr();
        __FLOG_4(_L8("CTunDriverSubConnectionFlow(%x):\tReceivedL() Sender Id: %x, Msg Realm: %X, Msg ID: %d"), this, senderAddr, aMessage.MessageId().Realm(), aMessage.MessageId().MessageId());
    #endif

    CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);
    if (aMessage.IsMessage<TEBase::TError>())
        {
        SubConnectionError(static_cast<TEBase::TError&>(aMessage).iValue);
        }
    else if (TEChild::ERealmId == aMessage.MessageId().Realm())
        {
        switch (aMessage.MessageId().MessageId())
            {
        case TEChild::TCancel::EId:
            CancelFlow(KErrCancel);
        case TEChild::TDestroy::EId :
            Destroy();
            break;
        default:
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
            __FRAMEWORK_PRODUCTION_ERRORHANDLING(bindToReq.iNodeId.IsNull(),User::LeaveIfError(KErrNotSupported));
            RClientInterface::OpenPostMessageClose(Id(), aSender, TCFDataClient::TBindToComplete().CRef());
            }
            break;
        default:
            ASSERT(EFalse);
            }
        }
    else
        {
        User::Leave(KErrNotSupported);
        }
    }

void CTunDriverSubConnectionFlow::StartFlowL()
/**
* CTunDriverSubConnectionFlowFactory::StartFlowL starts the flow on receiving the message TStart.
* This function is called from CTunDriverSubConnectionFlowFactory::ReceivedL 
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tStartFlowL()"));
    
    if(iMMState == EStarted)
        {
        //Flow can be started multiple times.
        //For example a start may fail on an upper layer in which case the upper layer will not be able
        //to stop our layer and we need to accept start again.
        PostProgressMessage(KLinkLayerOpen, KErrNone);
        PostDataClientStartedMessage();
        return;
        }
    iMMState = EStarted;

    // If the processing of the ProvisionConfig message failed earlier, send the error response
    // here to the StartFlow message as there is no response to ProvisionConfig.
    User::LeaveIfError(iSavedError);

    PostProgressMessage(KLinkLayerOpen, KErrNone);
    PostDataClientStartedMessage();
    iBinder4->StartSending();
#ifdef IPV6SUPPORT
    if(iBinder6)
        {
        iBinder6->StartSending();
        }
#endif
    }

void CTunDriverSubConnectionFlow::CancelFlow(TInt aError)
/**
* CTunDriverSubConnectionFlowFactory::CancelFlow cancels the flow on receiving the message TCancel.
* It will post the message as KErrCancel.
* This function is called from CTunDriverSubConnectionFlowFactory::ReceivedL 
*/
    {
    __FLOG_1(_L8("CTunDriverSubConnectionFlow:\tCancelFlow(%d)"), aError);

    if(iMMState == EStarted)
        {
        PostFlowDownMessage(aError);
        }
    }

void CTunDriverSubConnectionFlow::StopFlow(TInt aError)
/**
* CTunDriverSubConnectionFlowFactory::StopFlow stops the flow on receiving the message TStop.
* It will post the message as KLinkLayerClosed and KErrCancel.
* This function is called from CTunDriverSubConnectionFlowFactory::ReceivedL 
*/
    {
    __FLOG_1(_L8("CTunDriverSubConnectionFlow:\tStopFlow(%d)"), aError);

    PostProgressMessage(KLinkLayerClosed, aError);
    iMMState = EStopped;
    PostFlowDownMessage(aError);
    }

void CTunDriverSubConnectionFlow::ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData)
/**
* CTunDriverSubConnectionFlowFactory::Provisionconfig on receipt of the ProvisionConfig message, the pointer contained within it is stored
* in iAccessPointConfig and the information contained within the iAccessPointConfig  array is validated:
* CTunDriverProtoProvision must be present.  It is added by the TunDriverMCPr and populated from CommsDat.
* It is a 'C' class to take advantage of zero initialisation.
* Any errors are saved in iSavedError - there is no response to ProvisionConfig, 
* so an error response is sent later to StartFlow message.
*
* on receipt of TCFDataClient::TStart:
* iSavedError is checked and, if non-zero, an Error message is sent to the SCPr
* TunDriverAgentProvision must be present.  It is added by the TunDriverAgentHandler and populated via calls
* to the Agent.  It is a 'T' class because it requires no zero initialisation.  If missing,
* an Error message is signalled to the SCPr.
* This function is called from CTunDriverSubConnectionFlowFactory::ReceivedL
* @param aData pointer to provisioning structure 
*/
    {
    __FLOG_0(_L8("CTunDriverSubConnectionFlow:\tProvisionConfig message received"));
    iSavedError = KErrNone;
    iAccessPointConfig.Close();
    iAccessPointConfig.Open(aConfigData);

    const CTunDriverProtoProvision* provision = static_cast<const CTunDriverProtoProvision*>(AccessPointConfig().FindExtension(
            STypeId::CreateSTypeId(CTunDriverProtoProvision::EUid, CTunDriverProtoProvision::ETypeId)));
    __FRAMEWORK_PRODUCTION_ERRORHANDLING(provision!=NULL,iSavedError = KErrCorrupt);
    
    iProvision = provision;
    }

void CTunDriverSubConnectionFlow::Destroy()
/**
* CTunDriverSubConnectionFlowFactory::Destroy
* Request from SCPR to destroy
* This function is called from CTunDriverSubConnectionFlowFactory::ReceivedL 
*/
    {
    DeleteThisFlow();
    }

void CTunDriverSubConnectionFlow::SubConnectionGoingDown()
/**
* CTunDriverSubConnectionFlowFactory::SubConnectionGoingDown
* Request from Agemt SCPR
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tSubConnectionGoingDown"));
    }

void CTunDriverSubConnectionFlow::SubConnectionError(TInt /*aError*/)
/**
* CTunDriverSubConnectionFlowFactory::SubConnectionError
* Request from Agemt SCPR
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tSubConnectionError"));
    }

MFlowBinderControl* CTunDriverSubConnectionFlow::DoGetBinderControlL()
/**
* CTunDriverSubConnectionFlowFactory::DoGetBinderControlL Called by upper layer for binding
* Request from Agemt SCPR
* @return MFlowBinderControl instance
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow::DoGetBinderControlL"));
    return this;
    }

//-=========================================================
// MFlowBinderControl methods
//

MLowerControl* CTunDriverSubConnectionFlow::GetControlL(const TDesC8& aProtocol)
/**
* CTunDriverSubConnectionFlow::GetControlL
* Create and return an MLowerControl instance of the specified binder type.
* Called from upper layer during binding procedure.
* @param aProtocol Protocol type of the binder
* @return MLowerControl instance of the protocol type
*/
    {
    MLowerControl* lowerControl = NULL;
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tGetLowerControlL(KProtocol4)"));
    if(aProtocol.Compare(KProtocol4()) == 0)
    		{
    		iBinder4 = CTunDriverBinder4::NewL(*this);
    		lowerControl = iBinder4;
    		}
#ifdef IPV6SUPPORT
    else
    if (aProtocol.CompareF(KProtocol6()) == 0)
        {
        __FLOG(_L8("CTunDriverSubConnectionFlow::GetLowerControlL(KProtocol6)- Should Return KErrNotSupported"));
        iBinder6 = CTunDriverBinder6::NewL(*this);
        lowerControl = iBinder6;        
        }
#endif
    return lowerControl;
    }

MLowerDataSender* CTunDriverSubConnectionFlow::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
* CTunDriverSubConnectionFlow::BindL Create and return an MLowerDataSender instance of the specified protocol type.
* This is bound to the specified upper layer objects.
* Called from upper layer to bind to this layer.
* @param aProtocol Protocol type of the binder (same as in GetControlL())
* @param aReceiver upper layer's MUpperDataReceiver instance for this binder to associate with
* @param aControl upper layer's MUpperControl instance for this binder to associate with
* @return MLowerDataSender instance
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tBindL()"));

    MLowerDataSender* lowerDataSender = NULL;

    if(aProtocol.CompareF(KProtocol4()) == 0)
    		{
    		lowerDataSender = iBinder4->Bind(*aReceiver, *aControl);
    		}
#ifdef IPV6SUPPORT
    else
    if (aProtocol.CompareF(KProtocol6()) == 0)
        {
        lowerDataSender = iBinder6->Bind(*aReceiver, *aControl);
        }
#endif
    if(lowerDataSender != NULL)
        {
        iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TActive().CRef());
        }
    return lowerDataSender;
    }

void CTunDriverSubConnectionFlow::Unbind(MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
* CTunDriverSubConnectionFlow::Unbind will Unbind from the upper layer.
* Called from the upper layer during unbinding.
* @param aReceiver
* @param aControl
*/
    {
    __FLOG(_L8("CTunDriverSubConnectionFlow:\tUnbind()"));
    if (iBinder4 && iBinder4->MatchesUpperControl(aControl))
            {
            iBinder4->Unbind(*aReceiver, *aControl);
            delete iBinder4;
            iBinder4 = NULL;
            }
#ifdef IPV6SUPPORT
    if (iBinder6 && iBinder6->MatchesUpperControl(aControl))
            {
            iBinder6->Unbind(*aReceiver, *aControl);
            delete iBinder6;
            iBinder6 = NULL;
            }
#endif
    iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
    }

ESock::CSubConnectionFlowBase* CTunDriverSubConnectionFlow::Flow()
/**
* CTunDriverSubConnectionFlow::Flow will Return the flow object corresponding to the MFlowBinderControl
* Called from the UpperLayer get the instance of the flow.
*/
    {
    return this;
    }

void CTunDriverSubConnectionFlow::PostProgressMessage(TInt aStage, TInt aError)
/*
* CTunDriverSubConnectionFlow::PostProgressMessage will send the state change message with the param values.
* 
*/
    {
    iSubConnectionProvider.PostMessage(Id(), TCFMessage::TStateChange(Elements::TStateChange(aStage, aError)).CRef());
    }

void CTunDriverSubConnectionFlow::PostDataClientStartedMessage()
/*
* CTunDriverSubConnectionFlow::PostDataClientStartedMessage will send the started message with the param values.
*/
    {
    iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStarted().CRef());
    }

void CTunDriverSubConnectionFlow::PostFlowDownMessage(TInt aError, TInt aAction /*= MNifIfNotify::EDisconnect*/)
/*
* CTunDriverSubConnectionFlow::PostFlowDownMessage will send the DataClient Down message with the param values.
*/
    {
    if (iMMState == EStarted)
        {
        iMMState = EStopped;
        iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TDataClientGoneDown(aError, aAction).CRef());
        }
    else
        {
        iLastRequestOriginator.ReplyTo(Id(), TCFDataClient::TStopped(aError).CRef());
        }
    }

void CTunDriverSubConnectionFlow::MaybePostDataClientIdle()
/*
* CTunDriverSubConnectionFlow::MaybePostDataClientIdle will send the Idle message with the param values.
*/
    {
    // Can only send DataClientIdle when the upper layer has unbound and the flow is stopped
    if( iBinder4 == NULL
#ifdef IPV6SUPPORT
        || iBinder6 == NULL
#endif
      )
        {
        iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TIdle().CRef());
        }
    }

