// Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// tunnelnif.cpp
//
//

#include <es_mbuf.h>
//#include <flogger.h>

#include <agenterrors.h>

//#include "TunnelNifVar.h"
#include "tunnelFlow.h"
#include "tunnelBinders.h"
#include "tunnelProvision.h"
#include <comms-infras/linkmessages.h>
#include <comms-infras/ss_metaconnprov.h>					// for SAccessPointConfig
#include <comms-infras/commsdebugutility.h>
#include <elements/nm_messages_base.h>
#include <elements/nm_messages_child.h>

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;

/*
 * This sections defines a whole load of constants etc... not very exciting
 */
#if defined (_DEBUG)
	#define LOG(a) a
#else
	#define LOG(a)
#endif

_LIT8(KDescIp6, "ip6");
_LIT8(KDescIp, "ip");

// Note: The "tunnel" logging string has been repeated here but should be unified throughout
// the Tunnel CFProtocol.  The main logging relies on 16-bit RFileLogger calls whereas the
// CFNode logging requires an 8-bit string.  An attempt to make everything 8-bit resulted
// in undefined RFileLogger symbols.
#ifdef SYMBIAN_TRACE_ENABLE
_LIT8(KTunnel, "tunnel");
#endif

/*
 * The Link class
 */

CTunnelFlow::CTunnelFlow(CSubConnectionFlowFactoryBase& aFactory, const TNodeId& aSubConnId, CProtocolIntfBase* aProtocolIntf)
	: CSubConnectionFlowBase(aFactory, aSubConnId, aProtocolIntf)
/**
Constructor.

@param aFactory Reference to the factory which created this object.
@param aSubConnId Id of SubConnection Provider - for sending messages to it.
@param aProtocolIntf Protocol Interface corresponding to the Flow.
*/
    {
    LOG_NODE_CREATE(KTunnel, CTunnelFlow);
    }

CTunnelFlow::~CTunnelFlow()
    {
    LOG_NODE_DESTROY(KTunnel, CTunnelFlow);
    }

TInt CTunnelFlow::Notification(TTunnelAgentMessage::TTunnelSetAddress& aMessage)
    {
	if (iNifIf4)
		{
		return iNifIf4->Notification(aMessage);
		}
	if (iNifIf6)
		{
		return iNifIf6->Notification(aMessage);
		}
	return KErrNotSupported;
	}


// =====================================================================================
// CSubConnectionFlowBase
// =====================================================================================

MFlowBinderControl* CTunnelFlow::DoGetBinderControlL()
	{
	return this;
	}

// =====================================================================================
// MFlowBinderControl methods
// =====================================================================================

MLowerControl* CTunnelFlow::GetControlL(const TDesC8& aProtocol)
	{

	if (aProtocol.CompareF(KDescIp6) == 0)
		{
		if ( iNifIf6 )
			{
			CTunnelNcpLog::Printf(_L("CTunnelFlow:\tGetControlL already bound to %S"), &aProtocol);
			User::Leave(KErrInUse);
			return NULL;
			}
		iNifIf6 = CTunnelNcp6::ConstructL(*this);
		return iNifIf6;
        }
	else if (aProtocol.CompareF(KDescIp) == 0)
	    {
		if ( iNifIf4 )
            {
            CTunnelNcpLog::Printf(_L("CTunnelFlow:\tGetControlL already bound to %S"), &aProtocol);
			User::Leave(KErrInUse);
            return NULL;
            }
        iNifIf4 = CTunnelNcp4::ConstructL(*this);
		return iNifIf4;
        }
	Panic(ETunnelPanic_BadBind);
	return NULL;
	}

MLowerDataSender* CTunnelFlow::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
 * Binds upper CFProtocol to this CFProtocol
 *
 * @param aUpperReceiver A pointer to Upper layer Receive class
 * @param aUpperControl A pointer to Upper layer control class
 */
	{
	MLowerDataSender* lowerDataSender = NULL;
	if (aProtocol.CompareF(KDescIp6) == 0)
		{
		ASSERT(iNifIf6);
		lowerDataSender = iNifIf6->Bind(aReceiver, aControl);
        }
	else if (aProtocol.CompareF(KDescIp) == 0)
		{
		ASSERT(iNifIf4);
		lowerDataSender = iNifIf4->Bind(aReceiver, aControl);
		}
	else
		{
		// GetControlL() should already have been called.
		Panic(ETunnelPanic_BadBind);
		}

	if (lowerDataSender)
    	{
    	iSubConnectionProvider.PostMessage(Id(), TCFControlProvider::TActive().CRef());
    	}

	return lowerDataSender;
	}

void CTunnelFlow::Unbind(MUpperDataReceiver* aUpperReceiver, MUpperControl* aUpperControl)
    {
	if (iNifIf4 && iNifIf4->MatchesUpperControl(aUpperControl))
		{
		iNifIf4->Unbind(aUpperReceiver, aUpperControl);
		delete iNifIf4;
		iNifIf4 = NULL;
		}
	else
	if (iNifIf6 && iNifIf6->MatchesUpperControl(aUpperControl))
		{
		iNifIf6->Unbind(aUpperReceiver, aUpperControl);
		delete iNifIf6;
		iNifIf6 = NULL;
		}
	else
		{
		Panic(ETunnelPanic_BadUnbind);
		}
	MaybePostDataClientIdle();
    }

CSubConnectionFlowBase* CTunnelFlow::Flow()
/**
Return the Flow corresponding to the MFlowBinderControl
*/
	{
	return this;
	}

// =====================================================================================
// Messages::ANode
// =====================================================================================

void CTunnelFlow::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
    CSubConnectionFlowBase::ReceivedL(aSender, aRecipient, aMessage);

	if (aMessage.IsMessage<TEBase::TError>())
		{
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
		case TCFDataClient::TProvisionConfig::EId:
			ProvisionConfig(static_cast<TCFDataClient::TProvisionConfig&>(aMessage).iConfig);
			break;
		default:
//TODO - logging
			ASSERT(EFalse);
			}
		}
   	else if (TTunnelAgentMessage::ERealmId == aMessage.MessageId().Realm())
		{
		switch (aMessage.MessageId().MessageId())
			{
		case TTunnelAgentMessage::TTunnelSetAddress::EId:
			{
			Notification(static_cast<TTunnelAgentMessage::TTunnelSetAddress&>(aMessage));
			break;
			}
		default:
			Panic(ETunnelPanic_UnexpectedMessage);
			}
		}
	else	// realm is not TCFMessage or TTunnelAgentMessage
		{
		Panic(ETunnelPanic_UnexpectedMessage);
		}
    }

// =====================================================================================
//
// Methods for handling incoming SCPR messages
//

void CTunnelFlow::StartFlowL()
    {
    // NOTE: according to the NAF docs the sequence should really be StartSending(), then LinkLayerUp() then Progress()
    // for DNS to work.  However, this tunnel NIF doesn't support DNS.
    //
	CTunnelNcpLog::Write(_L("CTunnelFlow:\tStartFlow()"));

	// Process any errors that may have occurred during processing of the ProvisionConfig message earlier.
	// ProvisionConfig has no response, so error the StartFlow here.
	User::LeaveIfError(iSavedError);


	PostDataClientStartedMessage();
	if (iNifIf4)
		{
		iNifIf4->StartSending();
		}
	if (iNifIf6)
		{
		iNifIf6->StartSending();
		}
	iMMState = EStarted;
    }

void CTunnelFlow::StopFlow(TInt aError)
    {
    CTunnelNcpLog::Printf(_L("CTunnelFlow:\tStop(aError %d)"), aError);
   
    PostFlowDownMessage(aError);
    }

void CTunnelFlow::MaybePostDataClientIdle()
    {
	if (iNifIf4 == NULL && iNifIf4 == NULL)
		{
   		iSubConnectionProvider.RNodeInterface::PostMessage(Id(), TCFControlProvider::TIdle().CRef());
		}
    }
/*
Provisioning description for Tunnel CFProtocol Flow:

- on receipt of the TProvisionConfig message, the provisioning information contained within
  the AccessPointConfig array is validated:
	- TTunnelProvision must be present.  It is added by the Tunnel MCPr and populated from CommsDat.  A pointer to it
	  is stored in iProvisionInfo. If missing, TError(TCFDataClient::TStart, KErrCorrupt) message is signalled back
  	  to the SCPr on the next StartFlow message (ProvisionConfig has no response message).
*/


void CTunnelFlow::ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData)
/**
Handle ProvisionConfig message from SCPR.
*/
	{
	iSavedError = KErrNone;
	CTunnelNcpLog::Printf(_L("CTunnelFlow:\tProvisionConfig message received"));
	
	AccessPointConfig().Close();
	AccessPointConfig().Open(aConfigData);
	
    const TTunnelProvision* provision = static_cast<const TTunnelProvision*>(AccessPointConfig().FindExtension(
    		STypeId::CreateSTypeId(TTunnelProvision::EUid, TTunnelProvision::ETypeId)));
    if (provision == NULL)
        {
        CTunnelNcpLog::Printf(_L("CTunnelFlow:\tProcessProvisionConfigL() - no Tunnel configuration"));
		iSavedError = KErrCorrupt;
        }

	ASSERT(iProvisionInfo == NULL);
	iProvisionInfo = &provision->iInfo;
	ASSERT(iProvisionInfo);
	}

void CTunnelFlow::Destroy()
/**
Handle Destroy message from SCPR.
*/
	{
	ASSERT(iNifIf4 == NULL);		// must not still be bound from above before being destroyed
	ASSERT(iNifIf6 == NULL);
	DeleteThisFlow();
	}

//
// Utility functions
//

void CTunnelFlow::PostProgressMessage(TInt aStage, TInt aError)
	{
	iSubConnectionProvider.RNodeInterface::PostMessage(Id(), TCFMessage::TStateChange(Elements::TStateChange(aStage, aError)).CRef());
	}

void Panic(TTunnelPanic aPanic)
	{
	_LIT(KTunnelPanicTag, "Tunnel");
	User::Panic(KTunnelPanicTag, aPanic);
	}

