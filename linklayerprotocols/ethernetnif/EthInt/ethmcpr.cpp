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
// Ethernet MCPR
// 
//

/**
 @file
 @internalComponent
*/

#include <comms-infras/ss_log.h>
#include <comms-infras/agentmcpractivities.h>
#include <comms-infras/coremcpractivities.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/ss_msgintercept.h>

#include "ethmcpr.h"
#include "EthProvision.h"

#define KEthMCprTag KESockMetaConnectionTag

using namespace Messages;
using namespace MeshMachine;
using namespace ESock;
using namespace NetStateMachine;
using namespace MCprActivities;
using namespace EthMCprStates;



// No Bearer Activity
namespace EthMCPRNoBearerActivity
{
DECLARE_DEFINE_NODEACTIVITY(ECFActivityNoBearer, EthMCPRNoBearer, TCFControlProvider::TNoBearer)
	FIRST_NODEACTIVITY_ENTRY(CoreNetStates::TAwaitingNoBearer, MeshMachine::TNoTag)
	LAST_NODEACTIVITY_ENTRY(KNoTag, CoreNetStates::TSendBearer)
NODEACTIVITY_END()
}



// Activity Map
namespace EthMCprStates
{
DEFINE_EXPORT_ACTIVITY_MAP(stateMap)
	ACTIVITY_MAP_ENTRY(EthMCPRNoBearerActivity, EthMCPRNoBearer)
ACTIVITY_MAP_END_BASE(AgentMCprActivities, agentMCprActivities)
} // namespace EthMCprStates

//-=========================================================
//
//CEthMetaConnectionProvider implementation
//
//-=========================================================

EXPORT_C CEthMetaConnectionProvider* CEthMetaConnectionProvider::NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory, const TProviderInfo& aProviderInfo)
    {
    // coverity[alloc_fn] coverity[alias] coverity[assign]
    CEthMetaConnectionProvider* self = new (ELeave) CEthMetaConnectionProvider(aFactory, aProviderInfo, EthMCprStates::stateMap::Self());
    // coverity[push]
    CleanupStack::PushL(self);
    // coverity[alias] coverity[double_push]
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

EXPORT_C void CEthMetaConnectionProvider::ConstructL()
	{
	CAgentMetaConnectionProvider::ConstructL();
	SetAccessPointConfigFromDbL();
	}

EXPORT_C CEthMetaConnectionProvider::CEthMetaConnectionProvider(CMetaConnectionProviderFactoryBase& aFactory,
    							     const ESock::TProviderInfo& aProviderInfo,
								     const MeshMachine::TNodeActivityMap& aActivityMap)
:	CAgentMetaConnectionProvider(aFactory,aProviderInfo,aActivityMap)
	{
	LOG_NODE_CREATE(KEthMCprTag, CEthMetaConnectionProvider);
	}

EXPORT_C  CEthMetaConnectionProvider::~CEthMetaConnectionProvider()
	{
	LOG_NODE_DESTROY(KEthMCprTag, CEthMetaConnectionProvider);
	}

EXPORT_C void CEthMetaConnectionProvider::ReceivedL(const TRuntimeCtxId& aSender, const TNodeId& aRecipient, TSignatureBase& aMessage)
    {
	__CFLOG_VAR((KEthMCprTag, KEthMCprSubTag, _L8("CEthMetaConnectionProvider [this=%08x]::ReceivedL() aCFMessage=%d"),
	   this, aCFMessage.MessageId().MessageId()));

	ESOCK_DEBUG_MESSAGE_INTERCEPT(aSender, aMessage, aRecipient);

	TNodeContext<CEthMetaConnectionProvider> ctx(*this, aMessage, aSender, aRecipient);
    CCoreMetaConnectionProvider::Received(ctx);
    User::LeaveIfError(ctx.iReturn);
	}

void CEthMetaConnectionProvider::SetAccessPointConfigFromDbL()
    {
	__CFLOG_VAR((KEthMCprFactoryTag, KEthMCprFactorySubTag, _L8("CEthMetaConnectionProvider %08x:\tSetAccessPointConfigFromDbL()"), this));

    RMetaExtensionContainer mec;
    mec.Open(AccessPointConfig());
    CleanupClosePushL(mec);
    
	
	// Open an IAP specific view on CommsDat
	CCommsDatIapView* iapView = OpenIapViewLC();

	// Presumptions:
	// - none of the extensions can already exist in the AccessPointConfig array.  AppendExtensionL()
	//   is presumed to panic if adding the same extension a second time.
	// - if we have added several extensions to the AccessPointConfig array before getting a failure
	//   and leaving, it is presumed that the MCPr will be destroyed and AccessPointConfig destructor
	//   will clean up the extensions immediately afterwards.

    ProvisionLinkConfigL(iapView, mec);
    ProvisionNetworkConfigL(iapView, mec);
    
	CleanupStack::PopAndDestroy(); // CloseIapView();

	AccessPointConfig().Close();
	AccessPointConfig().Open(mec);
    CleanupStack::PopAndDestroy(&mec);
	}



void CEthMetaConnectionProvider::ProvisionLinkConfigL(CCommsDatIapView* aIapView, ESock::RMetaExtensionContainer& aMec)
    {
	TLanLinkProvision* linkProvision = new (ELeave) TLanLinkProvision;
	CleanupStack::PushL(linkProvision);

	// Initialise the link config

	// NOTE:
	// All the LANBearer strings are read as 16-bit values and copied into the
	// 8-bit buffers in the provisioning structure to save memory
	HBufC* buf = NULL;
	TInt err(KErrNone);

	// Config Daemon Name
	err = aIapView->GetText(KCDTIdLANConfigDaemonName, buf);
	if (err == KErrNone  && buf != NULL) 
	    {
    	linkProvision->SetConfigDaemonName(*buf);
    	delete buf;
    	buf= NULL;
	    }
	else if (err == KErrNotFound)
	    {
	    linkProvision->SetConfigDaemonName (KNullDesC);
	    }
	else
	    {
	    User::Leave(err);
	    }

	// LDD Filename
	err = aIapView->GetText(KCDTIdLANBearerLddFilename, buf);
	if (err == KErrNone && buf != NULL) 
	    {
    	linkProvision->SetLddFilename(*buf);
    	delete buf;
    	buf= NULL;
	    }
	else if (err == KErrNotFound)
	    {
	    linkProvision->SetLddFilename (KNullDesC);
	    }
	else
	    {
	    User::Leave(err);
	    }

	// LDD Name
	err = aIapView->GetText(KCDTIdLANBearerLddName, buf);
	if (err == KErrNone && buf != NULL) 
	    {
    	linkProvision->SetLddName(*buf);
    	delete buf;
    	buf= NULL;
	    }
	else if (err == KErrNotFound)
	    {
	    linkProvision->SetLddName (KNullDesC);
	    }
	else
	    {
	    User::Leave(err);
	    }

	// PDD Filename
	err = aIapView->GetText(KCDTIdLANBearerPddFilename, buf);
	if (err == KErrNone && buf != NULL) 
	    {
    	linkProvision->SetPddFilename(*buf);
    	delete buf;
    	buf= NULL;
	    }
	else if (err == KErrNotFound)
	    {
	    linkProvision->SetPddFilename (KNullDesC);
	    }
	else
	    {
	    User::Leave(err);
	    }

	// PDD Name
	err = aIapView->GetText(KCDTIdLANBearerPddName, buf);
	if (err == KErrNone && buf != NULL) 
	    {
    	linkProvision->SetPddName(*buf);
    	delete buf;
    	buf= NULL;
	    }
	else if (err == KErrNotFound)
	    {
	    linkProvision->SetPddName (KNullDesC);
	    }
	else
	    {
	    User::Leave(err);
	    }

	// Packet Driver Name - required info, always error
	aIapView->GetTextL(KCDTIdLANBearerPacketDriverName, buf);
	linkProvision->SetPacketDriverName(*buf);
 	delete buf;

	// Append the provisioning object to the CAccessPointConfig array
 	aMec.AppendExtensionL(linkProvision);
	CleanupStack::Pop(linkProvision);
    }


void CEthMetaConnectionProvider::ProvisionNetworkConfigL(CCommsDatIapView* aIapView, ESock::RMetaExtensionContainer& aMec)
    {
	HBufC* buf = NULL;
    aIapView->GetTextL(KCDTIdLANIfNetworks, buf);
    CleanupStack::PushL(buf);
    if (buf->Length() == 0)
        {
        User::Leave(KErrCorrupt);
        }

    TPtrC16 networks;
    networks.Set(*buf);
    TPtrC16 current;
    TUint32 order = 0;
    TInt commaPos = 0;
    while (commaPos != KErrNotFound)
        {
        commaPos = networks.LocateF(',');
        if (commaPos == KErrNotFound)
            {
            // take all of string
            current.Set(networks);
            }
        else
            {
            current.Set(networks.Ptr(), commaPos);
            }

        if (!current.CompareF(_L("ip")))
            {
            ProvisionIp4ConfigL(aIapView, order, aMec);
            }
        else if (!current.CompareF(_L("ip6")))
            {
            ProvisionIp6ConfigL(aIapView, order, aMec);
            }
        else
            {
            User::Leave(KErrCorrupt);
            }

        order++;
        networks.Set(networks.Mid(commaPos+1));
        }

    CleanupStack::PopAndDestroy(buf);
    }


void CEthMetaConnectionProvider::ProvisionIp4ConfigL(CCommsDatIapView* aIapView, TUint32 aOrder, ESock::RMetaExtensionContainer& aMec)
    {
	TLanIp4Provision* ip4Provision = new (ELeave) TLanIp4Provision;
	CleanupStack::PushL(ip4Provision);

	ip4Provision->SetOrder(aOrder);

	// Read IP address configuration parameters
	TBool serverRequired;
	aIapView->GetBoolL(KCDTIdLANIpAddrFromServer, serverRequired);

	TUint32 addr;
	TInt err;

	ip4Provision->SetLocalAddr(KInetAddrNone);
	ip4Provision->SetNetMask(KInetAddrNone);
	ip4Provision->SetDefGateway(KInetAddrNone);

	if (!serverRequired)
		{
		GetIp4AddrL(aIapView, KCDTIdLANIpAddr, addr);
  		ip4Provision->SetLocalAddr(addr);
		err = GetIp4Addr(aIapView, KCDTIdLANIpNetMask, addr);
		if (err == KErrNone)
			{
			ip4Provision->SetNetMask(addr);
			}

		err = GetIp4Addr(aIapView, KCDTIdLANIpGateway, addr);
		if (err == KErrNone)
		    {
		    ip4Provision->SetDefGateway(addr);
		    }
		else if (err == KErrNotFound || err == KErrArgument)
		    {
		    ip4Provision->SetDefGateway(ip4Provision->LocalAddr());
		    }
		else
		    {
		    User::Leave(err);
		    }

		// Because CommDB doesn't define a Broadcast Address field, we must
		// calculate the broadcast address. This is based on the localAddr
		// and the netMask.

		TInetAddr localAddr(ip4Provision->LocalAddr(), 0);
		TInetAddr netMask(ip4Provision->NetMask(), 0);
		TInetAddr broadcast;
		broadcast.SubNetBroadcast(ip4Provision->LocalAddr(), ip4Provision->NetMask());
		ip4Provision->SetBroadcastAddr(broadcast.Address());
   		}

	ip4Provision->SetPrimaryDns(KInetAddrNone);
	ip4Provision->SetSecondaryDns(KInetAddrNone);

	aIapView->GetBoolL(KCDTIdLANIpDNSAddrFromServer, serverRequired);
  	if (!serverRequired)
  		{
  		err = GetIp4Addr(aIapView, KCDTIdLANIpNameServer1, addr);
  		if (err == KErrNone)
  		    {
  		    ip4Provision->SetPrimaryDns(addr);
		    err = GetIp4Addr(aIapView, KCDTIdLANIpNameServer2, addr);
		    if (err == KErrNone)
		        {
		        ip4Provision->SetSecondaryDns(addr);
		        }
  		    }

  		if (err != KErrNone && err != KErrNotFound && err != KErrArgument)
  		    {
  		    User::Leave(err);
  		    }
     	}

	// Append the provisioning object to the CAccessPointConfig array
  	aMec.AppendExtensionL(ip4Provision);
	CleanupStack::Pop(ip4Provision);
    }


void CEthMetaConnectionProvider::ProvisionIp6ConfigL(CCommsDatIapView* aIapView, TUint32 aOrder, ESock::RMetaExtensionContainer& aMec)
    {
	TLanIp6Provision* ip6Provision = new (ELeave) TLanIp6Provision;
	CleanupStack::PushL(ip6Provision);

	ip6Provision->SetOrder(aOrder);

	// Determine whether static DNS configuration is required.
	TBool dynamicDns = ETrue;

	// By default, Ensure that static DNS addresses are set as unspecified,
	// so they are not used in Control(KSoIfConfig).
	ip6Provision->SetPrimaryDns(KInet6AddrNone);
	ip6Provision->SetSecondaryDns(KInet6AddrNone);

	// Ignore any errors from reading this field - default to dynamicDns = ETrue
	(void) aIapView->GetBool(KCDTIdLANIp6DNSAddrFromServer, dynamicDns);

	if (!dynamicDns)
		{
		// Read static DNS addresses
		TInt err;
		TIp6Addr addr6;
  		err = GetIp6Addr(aIapView, KCDTIdLANIp6NameServer1, addr6);
  		if (err == KErrNone)
  		    {
  		    ip6Provision->SetPrimaryDns(addr6);
		    err = GetIp6Addr(aIapView, KCDTIdLANIp6NameServer2, addr6);
		    if (err == KErrNone)
		        {
		        ip6Provision->SetSecondaryDns(addr6);
		        }
  		    }

  		if (err != KErrNone && err != KErrNotFound && err != KErrArgument)
  		    {
  		    User::Leave(err);
  		    }
		}

	// Append the provisioning object to the CAccessPointConfig array
	aMec.AppendExtensionL(ip6Provision);
	CleanupStack::Pop(ip6Provision);
    }


