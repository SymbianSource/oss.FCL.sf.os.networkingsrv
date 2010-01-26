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
//

#ifndef __MODULE_IF_H__
#define __MODULE_IF_H__

#include <ip6_iprt.h>
#include <flow.h>
#include <networking/pfqosparser.h>
#include <networking/qosparameters.h>

// Module flags
const TUint KQoSModuleSignaling         = 0x0001; // Signaling module
const TUint KQoSModulePartialSignaling  = 0x0002; // Partially signaling module (for example, signaled on access network)
const TUint KQoSModuleProvisioning      = 0x0004; // QoS provisioning (for example, DiffServ)
const TUint KQoSModuleBindToInterface   = 0x0008; // Module will be binded to an interface
const TUint KQoSModulePolicer           = 0x0010; // Module supports traffic policing
const TUint KQoSModuleShaper            = 0x0020; // Module supports traffic shaping
const TUint KQoSModuleScheduler         = 0x0060; // Deprecated and not used anymore, Used to Traffic scheduler module 


// QoS negotiation flags for modules
const TUint KQoSModuleSerialize         = 0x0040; // Serialize QoS negotiation (i.e. do not negotiate concurrently with another module)
const TUint KQoSFatalFailure            = 0x0080; // Failure is fatal: do not proceed with QoS negotiation

/**
 * Extension policy data
 *
 * @internalTechnology
 * @released
 */
class TExtensionData
{
public:
    TExtensionData() : iType(-1), iData(NULL,0){};
    TInt    iType;
    TPtrC8    iData;
};

/**
 * MQoSNegotiateEvent is given to a module in CModuleBase::NegotiateL(). 
 * The module must call RequestComplete() 
 * exactly once for each Negotiatel() call.
 *
 * @internalTechnology
 * @released
 */
class MQoSNegotiateEvent
{
public:
    IMPORT_C virtual void RequestComplete(TInt aErrorCode, const TQoSParameters* aParams, const TExtensionData& aExtension=TExtensionData())=0;
};

/**
 *
 *
 * @internalTechnology
 * @released
 */
class MEventInterface
{
public:
    IMPORT_C virtual void BlockFlow(CFlowContext& aFlow)=0;
    IMPORT_C virtual void UnBlockFlow(CFlowContext& aFlow)=0;
    IMPORT_C virtual void NotifyEvent(CFlowContext& aFlow, TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension=TExtensionData())=0;
    IMPORT_C virtual void NotifyEvent(TInt aChannelId, TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension=TExtensionData())=0;
    IMPORT_C virtual CSelectorBase* Lookup(const CFlowContext& aFlow, TUint aType, TUint aPriority, const TDesC& aName=TPtr(NULL,0))=0;
    IMPORT_C virtual CSelectorBase* Lookup(const TInetAddr &aLocal, const TInetAddr &aRemote, TUint aProtocol, 
                                          TUint aLocalPortMax, TUint aRemotePortMax, TUint aType, 
                                          const TUidType& aUid, TUint32 aIapId, TUint aPriority, const TDesC& aName=TPtr(NULL,0))=0;
    IMPORT_C virtual void LoadFileL(const TDesC& aFile)=0;
    IMPORT_C virtual TInt UnLoadFile(const TDesC& aFile)=0;
};


//
// Definitions for Configure(aLevel, aName, aOption, ..).
//
const TUint KSOLQoSModule   = 0xF01; // aLevel

const TUint KSoConfigure    = 0x101; // aName
const TUint KSoCapabilities = 0x102;


/**
 * Module interface: All QoS modules have to implement this interface 
 *
 * @internalTechnology
 * @released
 */
class CModuleBase : public CProtocolBaseUnbind
{
public:
    virtual void InitModuleL(MEventInterface& aEventInterface, CExtension* aData=NULL)=0;
    virtual void BindToIP6L(CProtocolBase* /*protocol*/) {};
    virtual void OpenL(CFlowContext& aFlow, CNifIfBase* aIf)=0;
    virtual void Close(CFlowContext& aFlow)=0;	//lint !e1411	because of CProtocolBase::Close()
    virtual void Negotiate(CFlowContext &aFlow, const TQoSParameters& aParams, MQoSNegotiateEvent& aNotify)=0;
    virtual void Release(CFlowContext& aFlow)=0;
    virtual void OpenChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& aPolicy, MQoSNegotiateEvent& aNotify, CFlowContext& aFlow)=0;
    virtual void CloseChannel(TInt aChannelId)=0;
    virtual void NegotiateChannel(TInt aChannelId, const TQoSParameters& aParams, CExtensionPolicy& aPolicy, MQoSNegotiateEvent& aNotify)=0;
    virtual void Join(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify)=0;
    virtual void Leave(TInt aChannelId, CFlowContext& aFlow, MQoSNegotiateEvent& aNotify)=0;
    virtual TInt ApplyL(TPacketHead &) { return KErrNone; }; // (inbound)
    virtual TInt ApplyL(RMBufSendPacket &, RMBufSendInfo &) { return KErrNone; }; // (outbound)
    virtual TInt Configure(TUint aLevel, TUint aName, TDes8& aOption, TAny* aSource=0)=0;
    virtual void InterfaceAttached(const TDesC& /*aName*/, CNifIfBase* /*aIf*/){};
    virtual void InterfaceDetached(const TDesC& /*aName*/, CNifIfBase* /*aIf*/){};
};

#endif
