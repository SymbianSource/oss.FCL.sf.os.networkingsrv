// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

//#include "UMTSNiflink.h"
#include "UMTSNif.h"

#include "log-r6.h"

CUmtsNifLink::CUmtsNifLink(CUmtsNifIfFactory& aFactory) : CNifIfLink(aFactory)
    {
    }

CUmtsNifLink::~CUmtsNifLink() 
    {
	iLink.Deque();		// Remove from Controller's list
	LOG(Log::Printf(_L("Link layer to IAP <%d> going down"),iNifIapId));	

	// CNifIfLink object can be destroyed before the CNifIfBase objects are destroyed.
	// Notify CNifIfBase objects about that here.
	while (!iNcpList.IsEmpty())
        {
		CUmtsNif* nif = iNcpList.First();
		nif->LinkLayerDown();
        }
	iNcpList.Reset();
    }

void CUmtsNifLink::ConstructL(CUmtsNifController *aNifController)
    {
	iNifController = aNifController;

	iNcpList.SetOffset(_FOFF(CUmtsNif, iLink));
	
	iNifIapId = 0;
    }

CNifIfBase* CUmtsNifLink::GetBinderL(const TDesC& aName)
	{
	TName networkName;
	
	if (aName.CompareF(KDescIp6) == 0)
		{
		networkName = KDescIp6;
		CUmtsNif* ncp = new(ELeave) CUmtsNif(this);		
		CleanupStack::PushL(ncp);
		ncp->ConstructL(this,aName);
		CleanupStack::Pop();
		iNcpList.AddLast(*ncp);

		return ncp;
		}
	else if (aName.CompareF(KDescIp) == 0 ||
             aName.CompareF(KDescIcmp) == 0)
		{
		networkName = KDescIp;
		CUmtsNif* ncp = new(ELeave) CUmtsNif(this);
		CleanupStack::PushL(ncp);
		ncp->ConstructL(this,aName);
		CleanupStack::Pop();
		iNcpList.AddLast(*ncp);
		return ncp;
		}
	
	User::Leave(KErrNotSupported);
	
	return KErrNone;
	}
	
TInt CUmtsNifLink::Start()
    {	
	TInt ret = KErrNotFound;
	TDblQueIter<CUmtsNif> iter(iNcpList);
	CUmtsNif *nextNcp = NULL;

	iNotCalled = ETrue;
	
	TUint32 iapId = 0;
	TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(IAP);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(COMMDB_ID));
	ret = iNotify->ReadInt(columnName, iapId); // Should check ret
	iNifIapId = iapId;
	
	while ((nextNcp = iter++) != NULL) // Find the correct link layer with the activated context
        {
		ret = nextNcp->CreateStandAlonePrimaryContext();
		if(ret)
			return ret;
        }
	return KErrNone;
    }


void CUmtsNifLink::Stop(TInt aError,MNifIfNotify::TAction aAction)
    {

	iNotify->IfProgress(KLinkLayerClosed,aError);
	iNotify->LinkLayerDown(aError, aAction);
    }

void CUmtsNifLink::NifDown()
    {
	TDblQueIter<CUmtsNif> iter(iNcpList);
	CUmtsNif *nextNcp = NULL;
	
	while ((nextNcp = iter++) != NULL) 
        {
		if(!nextNcp->Down()) // Found one Nif which is not down
			return;
        }	
	
	// Last Nif under this link went down, inform the agent
	iNotify->LinkLayerDown(KErrNone, MNifIfNotify::EDisconnect);

	iNotify->IfProgress(KLinkLayerClosed, KErrNone);
    }



TInt CUmtsNifLink::Send(RMBufChain& aPacket, TAny*)
    {
	LOG(Log::Printf(_L("UmtsLink<%d> Send called"),iNifIapId)); // Should not happen

	RMBufPacket* packet = (RMBufPacket*)&aPacket;

	packet->Free(); 
	
	return 0;
    }

void CUmtsNifLink::Info(TNifIfInfo& aInfo) const
    {
	//
	// Should there be something here as nobody seems to call this one
	//
	__ASSERT_DEBUG(0, User::Panic(_L("Link layer info called!"), 0));

	_LIT(KUmtsName, "testnif.link."); // Dot added: Nif naming convention now NIFNAME.instanceNumber
	aInfo.iName = KUmtsName;
	aInfo.iName.AppendNum((TInt)iNifIapId);

    }

TInt CUmtsNifLink::Notification(TAgentToNifEventType /*aEvent*/, void* /*aInfo*/)
	{
	
	return KErrNone;
	}
