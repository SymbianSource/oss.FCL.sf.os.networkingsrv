/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file DummyNifflow.h
*/

#ifndef DUMMYNIFFLOW_H_INCLUDED_
#define DUMMYNIFFLOW_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/nifif.h>
#include "Dummynifbinder.h"

class CDummyProtoProvision;
class TDummyIp6Provision;
class TDummyAgentProvision;

/**
Dummy Nif SubConnFlow Implementation UID
*/
const TInt KDummyNifFlowImplementationUid = 0x10281C3B;

// String literals for protocol name used during flow binding
_LIT8(KProtocol4, "ip");
_LIT8(KProtocol6, "ip6");

class CDummyNifSubConnectionFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
*/
	{
public:
	static CDummyNifSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
protected:
	CDummyNifSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
	};

class CDummyNifBinder4;
class CDummyNifBinder6;
class CDummyNifFlowTestingSubscriber;
class CDummyNifSubConnectionFlow : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
/**
*/
	{
	friend class CDummyNifSubConnectionFlowFactory;
public:
	static CDummyNifSubConnectionFlow* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

	// from Messages::ANode (via CSubConnectionFlowBase)
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

	// Methods called from Binders
	void FlowDown(TInt aError, TInt aAction = MNifIfNotify::EDisconnect);
	void Progress(TInt aStage, TInt aError);
	const TDummyIp6Provision* Ip6Provision() const;
	const TDummyAgentProvision* AgentProvision() const;

	// Functions for dealing with SCPR messages
	void StartFlowL();
	void StopFlow(TInt aError);
	void Destroy();
	void SubConnectionGoingDown();
	void SubConnectionError(TInt aError);

	// from MFlowBinderControl
	virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual void Unbind(ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual ESock::CSubConnectionFlowBase* Flow();

protected:
	CDummyNifSubConnectionFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	virtual ~CDummyNifSubConnectionFlow();

	// CSubConnectionFlowBase
	virtual ESock::MFlowBinderControl* DoGetBinderControlL();

	// Utilities for posting SCPR messages
	void PostProgressMessage(TInt aStage, TInt aError);
	void PostDataClientStartedMessage();
	void PostFlowDownMessage(TInt aError, TInt aAction = MNifIfNotify::EDisconnect);
	void MaybePostDataClientIdle();

    inline void SetBinder4(CDummyNifBinder4* aBinder4);
    inline void SetBinder6(CDummyNifBinder6* aBinder4);

private:
	void ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData);
	void AgentProvisionConfigL();

private:
    __FLOG_DECLARATION_MEMBER;

	CDummyNifBinder4* iBinder4;					// IPv4 binder
	CDummyNifBinder6* iBinder6;					// IPv6 binder
	const CDummyProtoProvision* iProvision;		// cached pointer to provisioning structure in SCPR (in control side memory)
	const TDummyAgentProvision* iAgentProvision;// cached pointer to provisioning structure in SCPR (in control side memory)
	TInt iSavedError;							// errors during processing of ProvisionConfig message
	TBool iFlowStarted:1;

public:
	TBool iDisableStart:1;
	};

enum TDummyNifPanicNum
	{
	EUnexpectedMessage
	};

void Panic(TDummyNifPanicNum);

#include "DummyNifFlow.inl"

#endif // DUMMYNIFFLOW_H_INCLUDED_
