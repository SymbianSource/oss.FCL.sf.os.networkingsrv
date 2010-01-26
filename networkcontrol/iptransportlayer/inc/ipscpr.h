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
// QoS IP SubConnection Provider class definition.
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

#include <networking/ipaddrinfoparams.h>

#include <networking/umtsextn.h>
#include <networking/imsextn.h>
#include <networking/sblpextn.h>
#include <comms-infras/eintsock.h>

#include "IPSCPRFactory.h"
#include "ipdeftbasescpr.h"

namespace IpSCprStates
    {
    class TAddClientToQosChannel;
    class TRemoveClientFromQoSChannel;
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	class TSendParamsToServiceProvider;
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	class TCreateAddressInfoBundle;
	class TCreateAddressInfoBundleFromJoiningClient;
	class TNoTagOrDoNothingTag;
	class TNoTagOrSendApplyResponseOrErrorTag;
    }

NONSHARABLE_CLASS(CIpSubConnectionProvider) : public CIpSubConnectionProviderBase
/** Non-default/reserved IP subconnection provider

@internalTechnology
@released Since 9.4 */
    {
    friend class CIpSubConnectionProviderFactory;
    friend class IpSCprStates::TAddClientToQosChannel;
    friend class IpSCprStates::TRemoveClientFromQoSChannel;
#ifndef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	friend class IpSCprStates::TSendParamsToServiceProvider;
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
    friend class IpSCprStates::TCreateAddressInfoBundle;
    friend class IpSCprStates::TCreateAddressInfoBundleFromJoiningClient;
	friend class IpSCprStates::TNoTagOrDoNothingTag;
	friend class IpSCprStates::TNoTagOrSendApplyResponseOrErrorTag;
public:
    typedef CIpSubConnectionProviderFactory FactoryType;
	virtual ~CIpSubConnectionProvider();


protected:
    //-====================================
    //Construction methods bundle - accessible thru the factory only
    //-====================================
    CIpSubConnectionProvider(ESock::CSubConnectionProviderFactoryBase& aFactory);
	static CIpSubConnectionProvider* NewL(CIpSubConnectionProviderFactory& aFactory);
	void ConstructL();

    //-====================================
    //ACFNode overrides
    //-====================================
    virtual Messages::RNodeInterface* NewClientInterfaceL(const Messages::TClientType& aClientType, TAny* aClientInfo = NULL);
    void Received(MeshMachine::TNodeContextBase& aContext);
    void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

private:
    //-====================================
    //Custom methods
    //-====================================
	TInt AddressCompletionValidation(RIPDataClientNodeInterface& aDataClient);
	CSubConIPAddressInfoParamSet* InitBundleL();
    };


#endif //SYMBIAN_IPSCPR_H
