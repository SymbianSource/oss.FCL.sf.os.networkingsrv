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
// tunnelnif.h
// 
//

/**
 @file
 @internalComponent
*/

#if (!defined TUNNELFLOW_H__)
#define TUNNELFLOW_H__

#include <comms-infras/nifif.h>
#include <nifutl.h>
#include <in6_if.h>
#include <eui_addr.h>	// TE64Addr
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_flowbinders.h>
#include "tunnelProvision.h"
#include "TunnelAgentHandler.h"

enum TTunnelPanic
	{
	ETunnelPanic_ObjectNotDeleted,
	ETunnelPanic_BadState,
	ETunnelPanic_UnexpectedMessage,
	ETunnelPanic_BadBind,
	ETunnelPanic_BadUnbind
	};

void Panic(TTunnelPanic aPanic);

class CTunnelNcp4;
class CTunnelNcp6;
struct TTunnelInfo;

/**
 *  The link controller.
 *  The purpose of this class in our case is to communicate with
 *  the agent.
 *  It is also responsible for creating and maintaining the NCP
 */
NONSHARABLE_CLASS(CTunnelFlow) : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
	{

public:
	CTunnelFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	~CTunnelFlow();

	//
	// from CSubConnectionFlowBase
	//
	virtual ESock::MFlowBinderControl* DoGetBinderControlL();

	//
	// from MFlowBinderControl
	//
	virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual void Unbind( ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual ESock::CSubConnectionFlowBase* Flow();

	//
	// Other Utility functions
	//
	inline const TTunnelInfo* Info();

protected:
	//
	// from CSubConnectionFlowBase
	//
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

private:
	//
	// Utility functions for processing incoming SCPR messages
	//
	void StartFlowL();
	void StopFlow(TInt aError);
	void ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData);
	void Destroy();
	TInt Notification(TTunnelAgentMessage::TTunnelSetAddress& aMessage);

	//
	// Utility functions for sending messages to SCPR
	//
	void PostProgressMessage(TInt aStage, TInt aError);
	inline void PostDataClientStartedMessage();				// inline as only used once
	inline void PostFlowDownMessage(TInt aError);	// inline as only used once
	void MaybePostDataClientIdle();

private:
	CTunnelNcp4* iNifIf4;						// IP4 binder
	CTunnelNcp6* iNifIf6;						// IP6 binder
  	enum TMeshMachineFlowState
      	{
      	EStopped,
      	EStarting,
      	EStarted,
      	EStopping,
      	};
	TMeshMachineFlowState iMMState;

	// Provisioning information
	const TTunnelInfo* iProvisionInfo;			// pointer to Tunnel provisioning information in AccessPointConfig

	TInt iSavedError;							// an errors saved from processing ProvisionConfig message (until StartFlow)
	};

// ===========================================================================================
//
// Inline functions
//

inline void CTunnelFlow::PostDataClientStartedMessage()
/**
Post a FlowUp message to the SCPR
*/
	{
	iSubConnectionProvider.PostMessage(Id(), ESock::TCFDataClient::TStarted().CRef());
	}

inline void CTunnelFlow::PostFlowDownMessage(TInt aError)
/**
Post a FlowDown message to the SCPR
*/
	{
	iSubConnectionProvider.PostMessage(Id(), ESock::TCFDataClient::TStopped(aError).CRef());
	iMMState = EStopped;
	}

inline const TTunnelInfo* CTunnelFlow::Info()
/**
Retrieve the provisioning information
*/
	{
	ASSERT(iProvisionInfo);
	return iProvisionInfo;
	}

#endif
