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


#include <comms-infras/ss_log.h>
#include <elements/sm_core.h>
#include <in_sock.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/ss_nodeinterfaces.h>

#include "agentmcpr.h"
#include "CAgentAdapter.h"
#include "agentmcpravailabilitytesting.h"
#include "agentqueryconnsettingsimpl.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCgntm, "NifManAgtPrCgntm");
#endif


#ifdef _DEBUG
#define KAgentMCprTag KESockMetaConnectionTag
_LIT8(KAgentMCprSubTag, "agentmcpr");
#endif

using namespace ESock;
using namespace Messages;
using namespace NetStateMachine;
using namespace CommsDat;


/**
Constructor for the Agent Meta Connection Provider
@param aFactory The parent factory which created this MCPr
*/
EXPORT_C CAgentMetaConnectionProvider::CAgentMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory,
                                                                    const ESock::TProviderInfo& aProviderInfo,
                                                                    const MeshMachine::TNodeActivityMap& aActivityMap)
	:	CCoreMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap),
		TIfStaticFetcherNearestInHierarchy(this)
	{
	iQueryConnSettingsImpl = NULL;
	}


/**
D'tor
*/
EXPORT_C CAgentMetaConnectionProvider::~CAgentMetaConnectionProvider()
	{
	CAgentMetaConnectionProvider::CancelAvailabilityMonitoring(); //Don't call virtual for obvious reasons!
    DestroyIapView();
	delete iQueryConnSettingsImpl;
	}

EXPORT_C void CAgentMetaConnectionProvider::ConstructL()
	{
	CCoreMetaConnectionProvider::ConstructL();
    ProvisionAgentInfoL();
	}

/**
*/
void CAgentMetaConnectionProvider::ProvisionAgentInfoL()
    {
    __CFLOG_VAR((KAgentMCprTag, KAgentMCprSubTag, _L8("CAgentMetaConnectionProvider %08x:\tProvisionAgentInfoL()"), this));

    // It is possible that an AgentProvisionInfo structure has previously been allocated
    // We create a new one anyway and override it, meaning the reference on the old one can
    // possibly be released (handled for us) resulting in its deletion 
    
    RMetaExtensionContainer mec;
    mec.Open(AccessPointConfig());
    CleanupClosePushL(mec);

    CAgentProvisionInfo* agentProvisionInfo = new (ELeave) CAgentProvisionInfo;
    CleanupStack::PushL(agentProvisionInfo);
    mec.AppendExtensionL(agentProvisionInfo);
	CleanupStack::Pop(agentProvisionInfo);

    HBufC* agentName(NULL);

    OpenIapViewL();
    IapView()->GetTextL(KCDTIdBearerAgent, agentName);
    __ASSERT_DEBUG(agentName, User::Panic(KSpecAssert_NifManAgtPrCgntm, 1));


    agentProvisionInfo->SetAgentName(*agentName);
    delete agentName;

    agentProvisionInfo->SetIapId(IapView()->IapId());
    TUint32 netId;
    IapView()->GetIntL(KCDTIdIAPNetwork,netId);
    agentProvisionInfo->SetNetworkId(netId);
    agentProvisionInfo->SetBearerSet(0xFF);

    TUint32 reconnectAttempts = 0;
    IapView()->GetIntL(KCDTIdMaxConnectionAttempts, reconnectAttempts);
    agentProvisionInfo->SetReconnectAttempts(reconnectAttempts);


    agentProvisionInfo->SetCredentials(CreateCredentialsInfoL());
    iAgentMCPrDoneWithIapView = ETrue;
    CloseIapView();
    
    // Safely reached the end, commit the changes
	AccessPointConfig().Close();
    AccessPointConfig().Open(mec);
    CleanupStack::PopAndDestroy(&mec);
    }


/**
Create the Credentials structure in the Agent Provisioning Info, and populate it
with initial values from CommsDat.

Assumes that IapView() has been setup.
*/
CCredentialsConfig* CAgentMetaConnectionProvider::CreateCredentialsInfoL()
	{
	__ASSERT_DEBUG(IapView(), User::Panic(KSpecAssert_NifManAgtPrCgntm, 2));
	CCredentialsConfig* pCredentialsConfig = CCredentialsConfig::NewLC(IapView());
	CleanupStack::Pop(pCredentialsConfig);
	return pCredentialsConfig;
	}


/**
Opens a ref counted Iap View. This must be closed with a call to CloseIapView
once the view has been finished with.
@return A pointer to the populated CCommsDatIapView.
*/
EXPORT_C CCommsDatIapView* CAgentMetaConnectionProvider::OpenIapViewL()
	{
	if (iIapView == NULL)
    	{
    	iIapView = CCommsDatIapView::NewL(ProviderInfo().APId());
    	}

    iIapViewRefCount++;

	return iIapView;
	}


/**
Opens a ref counted Iap View adding the returned object to the cleanup stack.
This must be closed with a call to CloseIapView once the view has been finished
with.
@return A pointer to the populated CCommsDatIapView.
*/
EXPORT_C CCommsDatIapView* CAgentMetaConnectionProvider::OpenIapViewLC()
	{
	CCommsDatIapView* iapView = OpenIapViewL();
	CleanupStack::PushL(TCleanupItem(CleanupCloseIapView, this));
	return iapView;
	}


/**
The cleanup stack method used to cleanly release the Iap View
@note Removal of the item from the cleanup stack may not always cause deletion
of the object. It still remains ref counted, and is only releasable once both
the ref count has reached zero and the AgentMCPr base class has had a chance
to use it for Provisioning.
*/
EXPORT_C void CAgentMetaConnectionProvider::CleanupCloseIapView(TAny* aThis)
	{
	reinterpret_cast<CAgentMetaConnectionProvider*>(aThis)->CloseIapView();
	}


/**
Closes a reference to the Iap View.
@note The Iap View may not be deleted after this call. It is only deleted once
both the ref count has reached zero and the AgentMCPr base class has had a
chance to use it for Provisioning.
*/
EXPORT_C void CAgentMetaConnectionProvider::CloseIapView()
	{
	if (iAgentMCPrDoneWithIapView && --iIapViewRefCount == 0)
    	{
    	DestroyIapView();
        }
	}

void CAgentMetaConnectionProvider::DestroyIapView()
	{
	delete iIapView;
	iIapView = NULL;
	}


/**
Gets a pointer to the current Iap View.
@return A pointer to the current Iap View.
@note A call to OpenIapViewL or OpenIapViewLC must be made prior to calling
this method to ensure the returned value is valid.
*/
EXPORT_C CCommsDatIapView* CAgentMetaConnectionProvider::IapView()
	{
	__ASSERT_DEBUG(iIapView, User::Panic(KSpecAssert_NifManAgtPrCgntm, 4));
	return iIapView;
	}

/**
Sets the Agent Notification Handler to be used by the AgentSCPr
@param aNotificationHandler - Pointer to an implementation of the
CAgentNotificationHandler class. This must exist for the lifetime of the MCPr.
*/
EXPORT_C void CAgentMetaConnectionProvider::SetAgentNotificationHandlerL (CAgentNotificationHandler* aNotificationHandler)
   {
	CAgentProvisionInfo* agentProvisionInfo = const_cast<CAgentProvisionInfo*>(static_cast<const CAgentProvisionInfo*>(AccessPointConfig().FindExtension(CAgentProvisionInfo::TypeId())));
	if (aNotificationHandler || agentProvisionInfo)
	    {
       	if (agentProvisionInfo==NULL)
       		{
       		RMetaExtensionContainer mec;
       		mec.Open(AccessPointConfig());
       		CleanupClosePushL(mec);
       		
       	    agentProvisionInfo = new (ELeave) CAgentProvisionInfo;
       	    CleanupStack::PushL(agentProvisionInfo);
       		mec.AppendExtensionL(agentProvisionInfo);
       		CleanupStack::Pop(agentProvisionInfo);
       		
       		AccessPointConfig().Close();
       		AccessPointConfig().Open(mec);
       		
            CleanupStack::PopAndDestroy(&mec);
       		}

        agentProvisionInfo->SetAgentNotificationHandler(aNotificationHandler);
	    }
    }

/**
Retreives an IPv4 address from a CCommsDatIapView
@param aIapView - A pointer to a CCommsDatIapView to obtain the field from
@param aElementId - The commsdat element Id of the field containing the IPv4 address
@param aAddr - The variable to receive the IPv4 address
*/
EXPORT_C void CAgentMetaConnectionProvider::GetIp4AddrL(CCommsDatIapView* aIapView, TMDBElementId aElementId, TUint32& aAddr)
	{
    User::LeaveIfError(GetIp4Addr(aIapView, aElementId, aAddr));
	}


/**
Non-leaving version of GetIp4AddrL. Retreives an IPv4 address from a CCommsDatIapView.
@param aIapView - A pointer to a CCommsDatIapView to obtain the field from
@param aElementId - The commsdat element Id of the field containing the IPv4 address
@param aAddr - The variable to receive the IPv4 address
@return A system-wide error code. KErrNone on success.
*/
EXPORT_C TInt CAgentMetaConnectionProvider::GetIp4Addr(CCommsDatIapView* aIapView, TMDBElementId aElementId, TUint32& aAddr)
    {
    aAddr = KInetAddrNone;
    HBufC* buf = NULL;
    TInt err = aIapView->GetText(aElementId, buf);
    if (err == KErrNone)
		{
		__ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCgntm, 5));

		TInetAddr addr;
		if ((err = addr.Input(*buf)) == KErrNone)
		    {
		    aAddr = addr.Address();
		    }
    	delete buf;
    	}

#ifdef _DEBUG
	if (err != KErrNone)
		{
		__CFLOG_VAR((KAgentMCprTag, KAgentMCprSubTag, _L8("CAgentMetaConnectionProvider:\tGetIp4Addr(): error %d reading CommsDat field %08x"),
					 err, aElementId));
    	}
#endif

    return err;
    }


/**
Retreives an IPv6 address from a CCommsDatIapView.
@param aIapView - A pointer to a CCommsDatIapView to obtain the field from
@param aElementId - The commsdat element Id of the field containing the IPv6 address
@param aAddr - The variable to receive the IPv6 address
*/
EXPORT_C void CAgentMetaConnectionProvider::GetIp6AddrL(CCommsDatIapView* aIapView, TMDBElementId aElementId, TIp6Addr& aAddr)
	{
    User::LeaveIfError(GetIp6Addr(aIapView, aElementId, aAddr));
	}


/**
Non-leaving version of GetIp6AddrL. Retreives an IPv6 address from a CCommsDatIapView.
@param aIapView - A pointer to a CCommsDatIapView to obtain the field from
@param aElementId - The commsdat element Id of the field containing the IPv6 address
@param aAddr - The variable to receive the IPv6 address
@return A system-wide error code. KErrNone on success.
*/
EXPORT_C TInt CAgentMetaConnectionProvider::GetIp6Addr(CCommsDatIapView* aIapView, TMDBElementId aElementId, TIp6Addr& aAddr)
	{
	aAddr = KInet6AddrNone;
	HBufC* buf = NULL;
	TInt err = aIapView->GetText(aElementId, buf);
	if (err == KErrNone)
    	{
    	__ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCgntm, 6));

    	TInetAddr ip6Addr;
    	if ((err = ip6Addr.Input(*buf)) == KErrNone)
        	{
        	aAddr = ip6Addr.Ip6Address();
        	}
    	delete buf;
    	}

#ifdef _DEBUG
	if (err != KErrNone)
		{
		__CFLOG_VAR((KAgentMCprTag, KAgentMCprSubTag, _L8("CAgentMetaConnectionProvider:\tGetIp6Addr(): error %d reading CommsDat field %08x"),
					 err, aElementId));
    	}
#endif

    return err;
	}


#ifdef _DEBUG
EXPORT_C void CAgentMetaConnectionProvider::StartAvailabilityMonitoringL(const TNodeCtxId& aAvailabilityActivity)
	{
	__ASSERT_DEBUG(iAvailabilitySubscriber==NULL, User::Panic(KSpecAssert_NifManAgtPrCgntm, 7)); //Only one start allowed from the availability activity!
	iAvailabilitySubscriber = CAvailabilitySubscriber::NewL(aAvailabilityActivity, ProviderInfo().APId());
	AddClientL(iAvailabilitySubscriber->Id(), TClientType(TClientType::ERegistrar, TCFClientType::EAvailabilityProvider));
#else
EXPORT_C void CAgentMetaConnectionProvider::StartAvailabilityMonitoringL(const Messages::TNodeCtxId& /*aAvailabilityActivity*/)
	{
#endif
	}

EXPORT_C void CAgentMetaConnectionProvider::CancelAvailabilityMonitoring()
	{
#ifdef _DEBUG
	if (iAvailabilitySubscriber)
		{
		RemoveClient(iAvailabilitySubscriber->Id());
		iAvailabilitySubscriber = NULL; //iAvailabilitySubscriber will delete itself when cancelled from the availability activity
		}
#endif
	}

/**
Retrieves the MLinkCprLegacyDataAccessExtApi implementation
@param aInterface Pointer to the interface implementation.
*/
void CAgentMetaConnectionProvider::ReturnInterfacePtrL(MLinkMCprLegacyDataAccessApiExt*& aInterface)
    {
    aInterface = this;
    }

/**
Retrieves the MQueryConnSettingsApiExt implementation
@param aInterface Pointer to the interface implementation.
*/
void CAgentMetaConnectionProvider::ReturnInterfacePtrL(MQueryConnSettingsApiExt*& aInterface)
    {
    if (!iQueryConnSettingsImpl)
    	{
    	const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    	iQueryConnSettingsImpl = new (ELeave)CAgentQueryConnSettingsImpl(*provisionInfo, AccessPointConfig());
    	}
    aInterface = iQueryConnSettingsImpl;
    }

EXPORT_C void CAgentMetaConnectionProvider::GetIntSettingL(const TDesC& aSettingName, TUint32& aValue, MPlatsecApiExt* aPlatsecItf)
    {
    const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    if (!provisionInfo)
        {
        User::Leave (KErrNotReady);
        }

    TMDBElementId elementId = MapFieldNameL(aSettingName, aPlatsecItf);
    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    TInt err = queryItf->GetInt(elementId, aValue, aPlatsecItf);
    if (err == KErrNotFound)
        {
        elementId = SecondChanceModemBearerLegacyMapFieldNameL(aSettingName, aPlatsecItf);
        err = queryItf->GetInt(elementId, aValue, aPlatsecItf);
        }
    User::LeaveIfError(err);
    }


EXPORT_C void CAgentMetaConnectionProvider::GetBoolSettingL(const TDesC& aSettingName, TBool& aValue, MPlatsecApiExt* aPlatsecItf)
    {
    const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    if (!provisionInfo)
        {
        User::Leave (KErrNotReady);
        }

    TMDBElementId elementId = MapFieldNameL(aSettingName, aPlatsecItf);
    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    TInt err = queryItf->GetBool(elementId, aValue, aPlatsecItf);
    if (err == KErrNotFound)
        {
        elementId = SecondChanceModemBearerLegacyMapFieldNameL(aSettingName, aPlatsecItf);
        err = queryItf->GetBool(elementId, aValue, aPlatsecItf);
        }
    User::LeaveIfError(err);
    }


EXPORT_C void CAgentMetaConnectionProvider::GetDes8SettingL(const TDesC& aSettingName, TDes8& aValue, MPlatsecApiExt* aPlatsecItf)
    {
    const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    if (!provisionInfo)
        {
        User::Leave (KErrNotReady);
        }

    TMDBElementId elementId = MapFieldNameL(aSettingName, aPlatsecItf);
    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    HBufC8* value;
    TInt err = queryItf->GetText(elementId, value, aPlatsecItf);
    if (err == KErrNotFound)
        {
        elementId = SecondChanceModemBearerLegacyMapFieldNameL(aSettingName, aPlatsecItf);
        err = queryItf->GetText(elementId, value, aPlatsecItf);
        }
    User::LeaveIfError(err);
    aValue = *value;
    delete value;
    }


EXPORT_C void CAgentMetaConnectionProvider::GetDes16SettingL(const TDesC& aSettingName, TDes16& aValue, MPlatsecApiExt* aPlatsecItf)
    {
    const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    if (!provisionInfo)
        {
        User::Leave (KErrNotReady);
        }

    TMDBElementId elementId = MapFieldNameL(aSettingName, aPlatsecItf);
    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    HBufC* value;
    TInt err = queryItf->GetText(elementId, value, aPlatsecItf);
    if (err == KErrNotFound)
        {
        elementId = SecondChanceModemBearerLegacyMapFieldNameL(aSettingName, aPlatsecItf);
        err = queryItf->GetText(elementId, value, aPlatsecItf);
        }
    User::LeaveIfError(err);
    aValue = *value;
    delete value;
    }


EXPORT_C void CAgentMetaConnectionProvider::GetLongDesSettingL(const TDesC& aSettingName, HBufC*& aValue, MPlatsecApiExt* aPlatsecItf)
    {
    aValue = NULL;

    const CAgentProvisionInfo* provisionInfo = AgentProvisionInfo();
    if (!provisionInfo)
        {
        User::Leave (KErrNotReady);
        }

    TMDBElementId elementId = MapFieldNameL(aSettingName, aPlatsecItf);
    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    TInt err = queryItf->GetText(elementId, aValue, aPlatsecItf);
    if (err == KErrNotFound)
        {
        elementId = SecondChanceModemBearerLegacyMapFieldNameL(aSettingName, aPlatsecItf);
        err = queryItf->GetText(elementId, aValue, aPlatsecItf);
        }
    User::LeaveIfError(err);
    }


TMDBElementId CAgentMetaConnectionProvider::MapFieldNameL(const TDesC& aSettingName, MPlatsecApiExt* aPlatsecItf)
    {
    _LIT(KSlash, "\\");
    TInt pos = aSettingName.Find(KSlash);
    if (pos != KErrNotFound)
        {
        return CommDbShimUtil::GetElementIdFromFieldNameL(aSettingName);
        }

    MQueryConnSettingsApiExt* queryItf;
    ReturnInterfacePtrL(queryItf);

    HBufC16* serviceTable;
    User::LeaveIfError(queryItf->GetText(KCDTIdIAPServiceType, serviceTable, aPlatsecItf));

    const TInt KMaxFieldSize = 2*KCommsDbSvrMaxFieldLength + 1;
    TBuf<KMaxFieldSize> field;

    field = *serviceTable;
    delete serviceTable;

    field.Append(KSlash);
    field.Append(aSettingName);
    return CommDbShimUtil::GetElementIdFromFieldNameL(field);
    }

TMDBElementId CAgentMetaConnectionProvider::SecondChanceModemBearerLegacyMapFieldNameL(const TDesC& aSettingName, MPlatsecApiExt* aPlatsecItf)
    {
    //This nasty looking method re-implements the lagacy behaviour whereat, despite the current IAP not
    //featuring the ModemBearer table (because it represnts LAN for instance) would still serve queries
    //addressed to "ModemBearer\\<field>>" assuming the local equivallent of the ModemBearer (e.g: LanBearer)
    //table is actually meant.
    _LIT(KSlash, "\\");
    TInt pos = aSettingName.Find(KSlash);
    if (pos != KErrNotFound)
        {
        TPtrC left = aSettingName.Left(pos);
        TPtrC right = aSettingName.Mid(pos+1);
        if (left.CompareC(TPtrC(MODEM_BEARER)) == 0)
            {
            HBufC16* bearerTable;

            MQueryConnSettingsApiExt* queryItf;
            ReturnInterfacePtrL(queryItf);

            User::LeaveIfError(queryItf->GetText(KCDTIdIAPBearerType, bearerTable, aPlatsecItf));

            const TInt KMaxFieldSize = 2*KCommsDbSvrMaxFieldLength + 1;
            TBuf<KMaxFieldSize> field;

            field = *bearerTable;
            delete bearerTable;

            field.Append(KSlash);
            field.Append(right);
            return MapFieldNameL(field, aPlatsecItf);
            }
        }
    User::Leave(KErrNotFound);
    return 0;
    }

/**
Retrieves the Agent Provider specific provisioning information as given by the MCpr
transition.

@internalTechnology
*/
const CAgentProvisionInfo* CAgentMetaConnectionProvider::AgentProvisionInfo()
    {
    const CAgentProvisionInfo* agentProvisionInfo = static_cast<const CAgentProvisionInfo*>(AccessPointConfig().FindExtension(CAgentProvisionInfo::TypeId()));
	__ASSERT_DEBUG(agentProvisionInfo, User::Panic(KSpecAssert_NifManAgtPrCgntm, 8));
    return agentProvisionInfo;
    }

