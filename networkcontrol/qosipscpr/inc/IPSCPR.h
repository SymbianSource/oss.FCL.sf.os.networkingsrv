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
// IP SubConnection Provider class definition.
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPSCPR_H
#define SYMBIAN_IPSCPR_H

#include <comms-infras/ss_subconnprov.h>
#include <comms-infras/corescpr.h>
#include <comms-infras/corescprstates.h>

#include <cs_subconevents.h>
#include <cs_subconparams.h>

#include <ip_subconparams.h>
#include <networking/umtsextn.h>
#include <networking/imsextn.h>
#include <networking/pfqoslib.h>
#include <networking/sblpextn.h>
#include <comms-infras/eintsock.h>

#include "IPSCPRFactory.h"
#include "asyncwriter.h"
#include "ipdeftbasescpr.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <networking/qoslib_internal.h>
#endif

class CQoSMsgWriter;
class CQoSMsgReader;
class TUmtsR5QoSParameters;
class TImsParameter;
class CSubConQosR5ParamSet;
class CSubConIMSExtParamSet;

namespace QoSIpSCprStates
    {
    class TAddClientToQoSChannel;
    class TRemoveClientToQoSChannel;
    class TRemoveLeavingClientFromQoSChannel;
    class TSetParameters;
    }

class CIpSubConnectionProvider : public CIpSubConnectionProviderBase
/** Non-default/reserved IP subconnection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class CIpDefaultSubConnectionProviderFactory;
    friend class QoSIpSCprStates::TAddClientToQoSChannel;
    friend class QoSIpSCprStates::TRemoveClientToQoSChannel;
    friend class QoSIpSCprStates::TRemoveLeavingClientFromQoSChannel;
    friend class QoSIpSCprStates::TSetParameters;
    friend class CQoSSocketOpener;    
public:
    typedef CIpDefaultSubConnectionProviderFactory FactoryType;
	virtual ~CIpSubConnectionProvider();


private:
    enum
        {
        KParameterRelInvalid = 0,
        KParameterRelGeneric = 1,
        KParameterRel4Rel99 = 4,
        KParameterRel5 = 5,
        };

protected:
    //-====================================
    //Construction methods bundle - accessible thru the factory only
    //-====================================
    CIpSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory);
	static CIpSubConnectionProvider* NewL(CIpDefaultSubConnectionProviderFactory& aFactory);
	void ConstructL();
    void ResetPrtExtensions();

    //-====================================
    //ACFNode overrides
    //-====================================
    virtual Messages::RNodeInterface* NewClientInterfaceL(const Messages::TClientType& aClientType, TAny* aClientInfo = NULL);
    void Received(MeshMachine::TNodeContextBase& aContext);
    void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

    //-====================================
    //Callbacks from MESH.
    //-====================================
    void SetQoSParametersL();
    RInternalSocket& InternalSocket();
    void InternalSocketOpened(TInt aErr);    
	
    //-====================================
    //Callbacks from QoSMsg.
    //-====================================
public:
	// Messages from PRT
	void ProcessPRTMsg(TPfqosMessage& aMsg);
	void ProcessPRTError(TPfqosMessage& aMsg, TInt aError);
#ifdef _DEBUG
	void ProcessPRTError(TInt aMsgType, TInt aError);
#endif

private:
    //-====================================
    //Custom methods
    //-====================================
	void DataClientJoiningL(RIPDataClientNodeInterface& aDataClient);
	void DataClientLeaving(RIPDataClientNodeInterface& aDataClient);

	// Messages to PRT
	void SendOpenExistingL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendCreateL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendCloseL();
	void SendJoinL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendLeaveL(const TInetAddr &aSrcAddr, const TInetAddr &aDstAddr, TUint32 aIapId, TUint32 aProtocol);
	void SendSetQoSL();


	// Internal Processing
	void ProcessPRTEventL(TPfqosMessage& aMsg);
	void ProcessPRTReplyL(TPfqosMessage& aMsg);

	void ConvertParametersFromESockL(const ESock::RCFParameterFamilyBundleC& aParameterBundle);
	void ConvertParametersFromQOSL(TPfqosMessage& aMsg, CSubConGenEventParamsGranted* aEvent);

	RIPDataClientNodeInterface* DetermineClient(const TPfqosMessage& aMsg);
	void MapGenericParamsFromESockToPRTL(const CSubConQosGenericParamSet& generic) const;
	void MapGenericParamsFromPRTToESockL(CSubConQosGenericParamSet& generic) const;
	void MapExtensionParamsFromESockToPRTL(const CSubConQosIPLinkR99ParamSet& extension, TUmtsQoSParameters& params);
	void MapExtensionParamsFromESockToPRTL(const CSubConSBLPR5ExtensionParamSet& extension, CSblpParameters& params) const;
	void MapExtensionParamsFromESockToPRTL(const CSubConQosR5ParamSet& aExtension, TUmtsR5QoSParameters& aParams);
	void MapExtensionParamsFromESockToPRTL(const CSubConImsExtParamSet& aExtension, TImsParameter& aParams);
    CSubConExtensionParameterSet* MapFromUmtsR5ExtensionL (const CUmtsR5QoSPolicy* aPolicy);
    CSubConExtensionParameterSet* MapFromImsExtensionL (const CImsPolicy* aPolicy);

	void ConvertTQoSIntoCQoSParamsL(const TQoSParameters& aParameter);
	void ConvertCQoSIntoTQoSParamsL(TQoSParameters& aParameters) const;

private:
	/** Internal Socket */
	RInternalSocket iSocket;
        TInt iSocketError;	

	/** PRT message writer */
	CQoSMsgWriter* iWriter;

	/** PRT message reader */
	CQoSMsgReader* iReader;

	/** QoS channel Id */
	TInt iChannelId;

	/** Process UID */
	TUidType iUid;

	/** QoS Parameters */
    CQoSParameters* iPrtParameters;

	/** QoS Extensions */
    TQoSExtensionQueue iPrtExtensions;

	/** QoS Parameters Set */
	TBool iParametersSet;
	TInt  iParameterRelease;

	/**Class for doing async writes */
	CAsyncWriter* iAsyncWriter;
    };


#endif //SYMBIAN_IPSCPR_H
