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
*   Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file tundriverflow.h
 @internalTechnology
*/

#ifndef TUNDRIVERFLOW_H
#define TUNDRIVERFLOW_H

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/nifif.h>
#include "tundriverbinder.h"
#include "tundriverprovision.h"

/**
Tun Driver SubConnFlow Implementation UID
*/
const TInt KTunDriverFlowImplementationUid = 0x10281C3C;

// String literals for protocol name used during flow binding
_LIT8(KProtocol4, "ip");
#ifdef IPV6SUPPORT
_LIT8(KProtocol6, "ip6");
#endif

class CTunDriverSubConnectionFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
*/
    {
public:
    static CTunDriverSubConnectionFlowFactory* NewL(TAny* aConstructionParameters);
    virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
protected:
    CTunDriverSubConnectionFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);
    };

class CTunDriverBinder4;
#ifdef IPV6SUPPORT
class CTunDriverBinder6;
#endif
class CTunDriverSubConnectionFlow : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
/**
*/
    {
public:
    static CTunDriverSubConnectionFlow* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

    // from Messages::ANode (via CSubConnectionFlowBase)
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

    // Methods called from Binders
    void FlowDown(TInt aError, TInt aAction = MNifIfNotify::EDisconnect);
    void Progress(TInt aStage, TInt aError);
    const TTunDriverIp4Provision* Ip4Provision() const;
#ifdef IPV6SUPPORT
    const TTunDriverIp6Provision* Ip6Provision() const;
#endif

    // Functions for dealing with SCPR messages
    void StartFlowL();
    void CancelFlow(TInt aError);
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
    CTunDriverSubConnectionFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
    virtual ~CTunDriverSubConnectionFlow();

    // CSubConnectionFlowBase
    virtual ESock::MFlowBinderControl* DoGetBinderControlL();

    // Utilities for posting SCPR messages
    void PostProgressMessage(TInt aStage, TInt aError);
    void PostDataClientStartedMessage();
    void PostFlowDownMessage(TInt aError, TInt aAction = MNifIfNotify::EDisconnect);
    void MaybePostDataClientIdle();

private:
    void ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData);

private:
    CTunDriverBinder4* iBinder4;
#ifdef IPV6SUPPORT
    CTunDriverBinder6* iBinder6;
#endif
    const CTunDriverProtoProvision* iProvision;      // cached pointer to provisioning structure in SCPR (in control side memory)
    TInt iSavedError;                           // errors during processing of ProvisionConfig message
    enum TMeshMachineFlowState
            {
            EStopped,
            EStarting,
            EStarted,
            EStopping,
            };
    TMeshMachineFlowState iMMState;
    __FLOG_DECLARATION_MEMBER;
public:
    };

#endif // TUNDRIVERFLOW_H
