// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <ss_glob.h>
#include <commsdattypesv1_1.h>
#include <metadatabase.h>

#include "tunnelagentcpr.h"
#include "tunnelagentcprstates.h"
#include "agentmessages.h"
#include <comms-infras/ss_tiermanagerutils.h>

#include <comms-infras/ss_connprov.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCtnlg, "NifManAgtPrCtnlg");
#endif

//#ifdef __CFLOG_ACTIVE
//#define KTunnelAgentCprTag KESockConnectionTag
//_LIT8(KTunnelAgentCprSubTag, "tunnelagentcprstates");
//#endif

_LIT (KTunnelAgentCPRPanic,"TunnelAgentCPRPanic");

using namespace TunnelAgentCprStates;
using namespace ESock;
using namespace Messages;



EXPORT_DEFINE_SMELEMENT(TJoinRealIAP, NetStateMachine::MStateTransition, TunnelAgentCprStates::TContext)
EXPORT_C void TJoinRealIAP::DoL()
	{
    //__CFLOG_VAR((KTunnelAgentCprTag, KTunnelAgentCprSubTag, _L8("TBindToRealIAP::DoL()")));
    __ASSERT_DEBUG(iContext.iNodeActivity, User::Panic(KSpecAssert_NifManAgtPrCtnlg, 1));

	// 1) Get the realIAPId
	TUint32 realIapId, myIapId,snaprecid,snapid=0;

	MQueryConnSettingsApiExt* queryItf;
	iContext.Node().ReturnInterfacePtrL(queryItf);

	CommsDat::TMDBElementId elementId = (KCDTIdRealIAP);
	TInt err = queryItf->GetInt(elementId, realIapId, NULL);
	if (err == KErrNotFound) 
	        realIapId = 0;
	
        CommsDat::TMDBElementId elementId2 = (KCDTIdVPNSNAPRecord);
	TInt err2 = queryItf->GetInt(elementId2, snaprecid,NULL);
	if(err2 == KErrNone)
	    {
	    CMDBSession* dbs = CMDBSession::NewLC(KCDVersion1_2);
	    snapid = TierManagerUtils::ConvertSNAPPrefToTagIdL(snaprecid,*dbs);
	    CleanupStack::PopAndDestroy(dbs);
	    }

	// compare it with MY IAP id, make sure they are different
	CommsDat::TMDBElementId myElementId = (KCDTIdIAPRecord | KCDTIdRecordTag);
	err = queryItf->GetInt(myElementId, myIapId, NULL);
	if(myIapId == realIapId)
		{
		_LIT(KPanicNoTunnelService, "No Tunnel Service");
		User::Panic(KPanicNoTunnelService, ETunnelAgentCprNoTunnelService);
		}

	// 2) Bind to the real AP
	TUint apid = realIapId ? realIapId : snapid;
	XConnectionProviderInfoQuery query(apid);
	
	CConnectionFactoryContainer* connectionFactories = SockManGlobals::Get()->iConnectionFactories;
	__ASSERT_DEBUG(connectionFactories, User::Panic(KSpecAssert_NifManAgtPrCtnlg, 2));

	CConnectionProviderBase* provider = static_cast<CConnectionProviderBase*>(connectionFactories->Find(query));
	
	TNodeId realIapNodeId = provider->Id();

	TNodeId thisNodeId = iContext.Node().NodeId();
	
	//SP are peers added with TClientType ((TCFClientType::EServProvider, CFClientType::EActive))
	RNodeInterface* sp = iContext.Node().AddClientL(realIapNodeId, TClientType(TCFClientType::EServProvider, TCFClientType::EActive));
		
	__ASSERT_DEBUG(sp != NULL, User::Panic(KTunnelAgentCPRPanic, ETunnelAgentCprNoServiceProvider));
 	iContext.iNodeActivity->PostRequestTo(
 			realIapNodeId, 
 			TCFServiceProvider::TJoinRequest(iContext.NodeId(), TClientType(TCFClientType::ECtrl)).CRef()
 			);
	}

