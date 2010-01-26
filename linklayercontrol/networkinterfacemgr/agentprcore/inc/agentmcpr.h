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


#ifndef SYMBIAN_AGENTMCPR_H
#define SYMBIAN_AGENTMCPR_H

#include <comms-infras/coremcpr.h>
#include <comms-infras/nifagt.h>
#include <comms-infras/agentmessages.h>
#include <metadatabase.h>		// for TMDBElementId
#include <comms-infras/ss_commsdataobject.h>
#include <comms-infras/ss_nodemessages_legacy.h>
#include <comms-infras/ss_nodemessages_legacy_internal.h>
#include <comms-infras/ss_legacyinterfaces.h>

class TIp6Addr;
class CAgentNotificationHandler;
class CAvailabilitySubscriber;
class CAgentQueryConnSettingsImpl;
class CAgentProvisionInfo;

namespace ESock
	{
	class CCommsDatIapView;
	}


class CAgentMetaConnectionProvider : public CCoreMetaConnectionProvider, private ESock::MLinkMCprLegacyDataAccessApiExt,
    public ITFHIERARCHY_LINK_2(CAgentMetaConnectionProvider, CCoreMetaConnectionProvider,
		ESock::MQueryConnSettingsApiExt,
    	ESock::MLinkMCprLegacyDataAccessApiExt)

	{
public:
	typedef ITFHIERARCHY_LINK_2(CAgentMetaConnectionProvider, CCoreMetaConnectionProvider,
		ESock::MQueryConnSettingsApiExt,
    	ESock::MLinkMCprLegacyDataAccessApiExt) TIfStaticFetcherNearestInHierarchy;

public:
	IMPORT_C ~CAgentMetaConnectionProvider();

	void ReturnInterfacePtrL(ESock::MLinkMCprLegacyDataAccessApiExt*& aInterface);
	void ReturnInterfacePtrL(ESock::MQueryConnSettingsApiExt*& aInterface);
	
	// Methods for retrieving and decoding CommsDat fields containing IPv4/v6 addresses.
	//
	// These methods could have been in CCommsDatIapView, but that would have meant ESockSvr
	// linking against insock.lib (IP specific and takes up RAM, even if ESockSvr isn't used
	// for IP at all).  If CCommsDatIapView is moved out of ESockSvr, then these
	// methods should be moved into CCommsDatIapView.
	IMPORT_C static void GetIp4AddrL(ESock::CCommsDatIapView* aIapView, CommsDat::TMDBElementId aElementId, TUint32& aAddr);
	IMPORT_C static TInt GetIp4Addr(ESock::CCommsDatIapView* aIapView, CommsDat::TMDBElementId aElementId, TUint32& aAddr);
	IMPORT_C static void GetIp6AddrL(ESock::CCommsDatIapView* aIapView, CommsDat::TMDBElementId aElementId, TIp6Addr& aAddr);
	IMPORT_C static TInt GetIp6Addr(ESock::CCommsDatIapView* aIapView, CommsDat::TMDBElementId aElementId, TIp6Addr& aAddr);

	
	// From MLinkCprLegacyDataAccessExtApi Interface
	IMPORT_C virtual void GetIntSettingL(const TDesC& aSettingName, TUint32& aValue, ESock::MPlatsecApiExt* aPlatsecItf);
	IMPORT_C virtual void GetBoolSettingL(const TDesC& aSettingName, TBool& aValue, ESock::MPlatsecApiExt* aPlatsecItf);
	IMPORT_C virtual void GetDes8SettingL(const TDesC& aSettingName, TDes8& aValue, ESock::MPlatsecApiExt* aPlatsecItf);
	IMPORT_C virtual void GetDes16SettingL(const TDesC& aSettingName, TDes16& aValue, ESock::MPlatsecApiExt* aPlatsecItf);
	IMPORT_C virtual void GetLongDesSettingL(const TDesC& aSettingName, HBufC*& aValue, ESock::MPlatsecApiExt* aPlatsecItf);

	// Utility method
	TMDBElementId MapFieldNameL(const TDesC& aSettingName, ESock::MPlatsecApiExt* aPlatsecItf);
    TMDBElementId SecondChanceModemBearerLegacyMapFieldNameL(const TDesC& aSettingName, ESock::MPlatsecApiExt* aPlatsecItf);

protected:
	const CAgentProvisionInfo* AgentProvisionInfo();

	IMPORT_C static void CleanupCloseIapView(TAny* aThis);

	IMPORT_C CAgentMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
	                                      const ESock::TProviderInfo& aProviderInfo,
	                                      const MeshMachine::TNodeActivityMap& aActivityMap);

	IMPORT_C ESock::CCommsDatIapView* OpenIapViewL();
	IMPORT_C ESock::CCommsDatIapView* OpenIapViewLC();
	IMPORT_C void CloseIapView();
	IMPORT_C ESock::CCommsDatIapView* IapView();
	void DestroyIapView();

	IMPORT_C void SetAgentNotificationHandlerL(CAgentNotificationHandler* aNotificationHandler);
    IMPORT_C void ConstructL();

    // Messages::ANode Interface - To be implemented by the deriving MCPr
    // virtual TInt ReceivedL(Messages::TSignatureBase& aCFMessage);

	IMPORT_C void StartAvailabilityMonitoringL(const Messages::TNodeCtxId& aAvailabilityActivity);
	IMPORT_C void CancelAvailabilityMonitoring();

private:
    void ProvisionAgentInfoL ();
    CCredentialsConfig* CreateCredentialsInfoL();

private:
    ESock::CCommsDatIapView* iIapView;
    TBool iAgentMCPrDoneWithIapView:1;
    TInt32 iIapViewRefCount;

public:
#ifdef _DEBUG
	CAvailabilitySubscriber* iAvailabilitySubscriber;
#else
	TAny* iReserved; //Place holder to avoid BC between udeb/urel
#endif

	
protected:
    CAgentQueryConnSettingsImpl* iQueryConnSettingsImpl;
	};


#endif
// SYMBIAN_AGENTMCPR_H

