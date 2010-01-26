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

/**
 @file
 @internalTechnology
 @prototype
*/


#ifndef SYMBIAN_AGENTCPR_H
#define SYMBIAN_AGENTCPR_H

#include <comms-infras/corecpr.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>
#include <comms-infras/ss_commsdataobject.h>
#include <comms-infras/cagentadapter.h>

class CAgentProvisionInfo;
class CAgentQueryConnSettingsImpl;


namespace ESock
{
	class MPlatsecApiExt;
	class MLinkCprServiceChangeNotificationApiExt;
}

namespace AgentCprStates
{
	DECLARE_EXPORT_ACTIVITY_MAP(agentCprActivities)
}

class CAgentConnectionProvider : public CCoreConnectionProvider,
	private ESock::MLegacyControlApiExt,
    public ITFHIERARCHY_LINK_3(CAgentConnectionProvider, CCoreConnectionProvider,
    	ESock::MLegacyControlApiExt,
		ESock::MQueryConnSettingsApiExt,
    	ESock::MLinkCprServiceChangeNotificationApiExt)
	{
public:
	typedef ITFHIERARCHY_LINK_3(CAgentConnectionProvider, CCoreConnectionProvider,
		ESock::MLegacyControlApiExt,
		ESock::MQueryConnSettingsApiExt,
    	ESock::MLinkCprServiceChangeNotificationApiExt) TIfStaticFetcherNearestInHierarchy;

public:
    IMPORT_C static CAgentConnectionProvider* NewL(ESock::CConnectionProviderFactoryBase& aFactory);
    IMPORT_C ~CAgentConnectionProvider();

    void ReturnInterfacePtrL(ESock::MLegacyControlApiExt*& aInterface);
	void ReturnInterfacePtrL(ESock::MQueryConnSettingsApiExt*& aInterface);
    void ReturnInterfacePtrL(ESock::MLinkCprServiceChangeNotificationApiExt*& aInterface);

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
public:
	const CAgentProvisionInfo* AgentProvisionInfo() const;
#else
protected:
	const CAgentProvisionInfo* AgentProvisionInfo() const;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	

protected:
    CAgentConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory);

    CAgentConnectionProvider(ESock::CConnectionProviderFactoryBase& aFactory, const MeshMachine::TNodeActivityMap& aActivityMap);
    // From Messages::ANode Interface
    virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

protected:

	// From MLegacyControlApiExt Interface
	virtual TInt ControlL(TUint aOptionLevel, TUint aOptionName, TDes8& aOption, ESock::MPlatsecApiExt* aPlatsecItf);

protected:
    CAgentQueryConnSettingsImpl* iQueryConnSettingsImpl;
	};


#endif
// SYMBIAN_AGENTCPR_H

