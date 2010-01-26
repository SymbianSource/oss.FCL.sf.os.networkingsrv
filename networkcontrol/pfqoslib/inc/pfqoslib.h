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
// @file pfqoslib.h
// Header file for PF_QOS messages
// @internalTechnology
// @released
//

#ifndef __PFQOSLIB_H__
#define __PFQOSLIB_H__

#include <e32std.h>
#include <in_sock.h>
#include <es_mbuf.h>

#include <networking/pfqos.h>
#include <networking/qosparameters.h>

class CSelectorBase;

/** 
 * Map the basic PFQOS_MSG V1 structures into Classes with
 * constructors for initialized content.
 *
 * @internalTechnology 
 */
class T_pfqos_msg : public pfqos_msg
    {
    public:
    IMPORT_C T_pfqos_msg(TUint8 aMsgType, TUint32 aSeq = 0);
    };

/** @internalTechnology */
class T_pfqos_selector : public pfqos_selector
    {
    public:
    IMPORT_C T_pfqos_selector(CSelectorBase* aSelector);
    };

/** @internalTechnology */
class T_pfqos_address : public pfqos_address
    {
    public:
    IMPORT_C T_pfqos_address(TUint aType, TUint16 aPortMax);
    };

/** @internalTechnology */
class T_pfqos_module : public pfqos_modulespec
    {
    public:
    IMPORT_C T_pfqos_module(TUint aProtocolId, TUint32 aFlags, const TDesC& aName, const TDesC& aPath, TInt aDataLen);
    };

/** @internalTechnology */
class T_pfqos_flowspec : public pfqos_flowspec
    {
    public:
    IMPORT_C T_pfqos_flowspec(const TQoSParameters& aParameters);
    };

/** @internalTechnology */
class T_pfqos_event : public pfqos_event
    {
    public:
    IMPORT_C T_pfqos_event(TUint8 aType, TUint16 aEventType, TUint16 aValue);
    };

/** @internalTechnology */
class T_pfqos_channel : public pfqos_channel
    {
    public:
    IMPORT_C T_pfqos_channel(TUint32 aChannelId);
    };

/** @internalTechnology */
class T_pfqos_configure : public pfqos_configure
    {
    public:
    IMPORT_C T_pfqos_configure(TUint16 aProtocolId);
    };

/**
 * Internal representation of PfqosMessage
 *
 * @internalTechnology 
 */
class TPfqosBase
    {
    public:
    const struct pfqos_msg *iMsg;
    IMPORT_C TPfqosBase();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aTotal) const;
    };

/** @internalTechnology */
class TPfqosSelector
    {
    public:
    const struct pfqos_selector *iExt;
    IMPORT_C TPfqosSelector();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    IMPORT_C TInt GetUid(TCheckedUid& aUid);
    };

/** @internalTechnology */
class TPfqosAddress
    {
    public:
    const struct pfqos_address *iExt;
    const TInetAddr *iAddr;
    const TInetAddr *iPrefix;
    IMPORT_C TPfqosAddress();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

class CExtension;

/** @internalTechnology */
class TPfqosModule
    {
    public:
    const struct pfqos_modulespec *iExt;
    TPtrC8 iData;
    IMPORT_C TPfqosModule();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    TSglQueLink iNext;
    };

/** @internalTechnology */
class TPfqosFlowSpec
    {
    public:
    const struct pfqos_flowspec *iExt;
    IMPORT_C TPfqosFlowSpec();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

/** @internalTechnology */
class TPfqosEvent
    {
    public:
    const struct pfqos_event *iExt;
    IMPORT_C TPfqosEvent();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

/** @internalTechnology */
class TPfqosChannel
    {
    public:
    struct pfqos_channel *iExt;
    IMPORT_C TPfqosChannel();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

/** @internalTechnology */
class TPfqosConfigure
    {
    public:
    const struct pfqos_configure *iExt;
    IMPORT_C TPfqosConfigure();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

/** @internalTechnology */
class TPfqosConfigFile
    {
    public:
    const struct pfqos_config_file *iExt;
    IMPORT_C TPfqosConfigFile();
    IMPORT_C TUint Length() const;
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;
    };

/** @internalTechnology */
class CPfqosPolicyData : public CBase
    {
    public:
    IMPORT_C ~CPfqosPolicyData();
    IMPORT_C static CPfqosPolicyData* NewL(TInt aType);
    IMPORT_C static CPfqosPolicyData* NewL(TInt aType, const TUint8 *aBuf,TInt aLength);
    IMPORT_C TUint Length() const;
    IMPORT_C TInt Size() const;
    IMPORT_C void CopyL(const TUint8 *aBuf,TInt aLength);
    IMPORT_C TDesC8& Data();
    IMPORT_C TInt Type();
    IMPORT_C TInt ByteStream(RMBufChain &aPacket, TInt aOffset) const;

    TSglQueLink iNext;

    private:

    IMPORT_C CPfqosPolicyData(TInt aType);
    IMPORT_C void ConstructL();
    TInt    iType;
    TPtr8    iBufPtr;
    HBufC8* iBuf;
    };



/**
 * Internal representation of PF_QOS message
 *
 * This class is either misnamed (should not be a T-class) or
 * misdesigned. The instance of this object owns dynamically
 * allocated heap objects in member variables iModuleList and
 * iExtentions. Thus, the destructor of this class must always
 * be executed.
 *
 * @internalTechnology 
 */
class TPfqosMessage
    {
    public:
    IMPORT_C TPfqosMessage(const TDesC8& aMsg);
    IMPORT_C TPfqosMessage();
    IMPORT_C ~TPfqosMessage();
    IMPORT_C void AddModuleL(T_pfqos_module& aModule, const TDesC8& aConfigData);
    IMPORT_C void AddExtensionL(const TDesC8& aExtension, TInt aType);
    IMPORT_C TUint ExtensionLength();
    IMPORT_C TUint ModuleLength();
    IMPORT_C TUint16 Length64();
    IMPORT_C void ByteStreamL(RMBufChain &aPacket);
    IMPORT_C void Add(TPfqosModule& aModule);
    IMPORT_C void AddExtension(CPfqosPolicyData& aExtension);
    IMPORT_C TInt SetQoSParameters(TQoSParameters& aParameters) const;
    IMPORT_C void RemovePolicyData(); // Remove all policy data (flowspec, modules, extensions, event, configure)

    public:
    TInt iError;
    TPfqosBase iBase;
    TPfqosSelector iSelector;
    TPfqosAddress iSrcAddr;
    TPfqosAddress iDstAddr;
    TPfqosFlowSpec iFlowSpec;
    TPfqosEvent iEvent;
    TPfqosChannel iChannel;
    TPfqosConfigure iConfigure;
    TPfqosConfigFile iConfigFile;
    TUint iNumModules;
    TSglQue<TPfqosModule> iModuleList;
    TUint iNumExtensions;
    TSglQue<CPfqosPolicyData> iExtensions;
    };


#endif
