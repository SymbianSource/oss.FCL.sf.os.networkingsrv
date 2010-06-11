// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// SHIMCPRFACTORY.H
// 
//

#include <ecom/ecom.h>
#include <implementationproxy.h>
#include "shimcprfactory.h"	// for CConnectionProviderFactoryShim

#include "connectionSelectorShim.h"
#include "shimcpr.h"
#include "shimnifmansconn.h"
#include "idquerynetmsg.h"
#include <nifman.h>
#ifdef SYMBIAN_NETWORKING_UMTSR5
#include <esockmessages.h>
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5	


const TUint KShimConnectionProviderImplementationUid = 0x102070FF;
/**
Data required for instantiating ECOM Plugin
*/
const TImplementationProxy ImplementationTable[] = 
	{
	IMPLEMENTATION_PROXY_ENTRY(KShimConnectionProviderImplementationUid, CConnectionProviderFactoryShim::NewL),
	};


/**
ECOM Implementation Factory
*/
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);

    return ImplementationTable;
    }

CConnectionProviderFactoryShim* CConnectionProviderFactoryShim::NewL(TAny* aConstructionParameters)
	{
	CConnectionProviderFactoryShim* ptr = new (ELeave) CConnectionProviderFactoryShim(KShimConnectionProviderFactoryId, *(reinterpret_cast<CConnectionFactoryContainer*>(aConstructionParameters)));
	CleanupStack::PushL(ptr);
	ptr->ConstructL();
	CleanupStack::Pop(ptr);
	return ptr;
	}
   
void CConnectionProviderFactoryShim::ConstructL()
	{//create a provider session for global tasks (enumerate connections/all interface notification....)
    iNifmanSession = Nif::NewConnectionL(this, 0);
    iNifmanSession->AllInterfaceNotificationL();
	}
// Destructor
CConnectionProviderFactoryShim::~CConnectionProviderFactoryShim()
	{
	delete iNifmanSession;
	}
   
// Constructor
CConnectionProviderFactoryShim::CConnectionProviderFactoryShim(TUint aFactoryId, CConnectionFactoryContainer& aParentContainer) :
	CConnectionProviderFactoryBase( aFactoryId, aParentContainer )
	{
	}

#ifdef SYMBIAN_NETWORKING_UMTSR5
// Sets the IAP lock status
void CConnectionProviderFactoryShim::SetIAPLockStatus(TBool aLockStatus, TInt aLockedIAP)
	{
	iIsIAPLocked = aLockStatus;
	iLockedIAP = aLockedIAP;
	}
// Gets the IAP lock status
void CConnectionProviderFactoryShim::GetIAPLockStatus(TBool &aLockStatus, TInt &aLockedIAP)
	{
	aLockStatus = iIsIAPLocked;
	aLockedIAP = iLockedIAP;
	}
	
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5	

CConnectionProviderBase* CConnectionProviderFactoryShim::DoCreateProviderL()
	{
#ifdef SYMBIAN_NETWORKING_UMTSR5	
	CConnectionProviderShim * shimProv = CConnectionProviderShim::NewL(*this);
	shimProv->SetFactoryIfactory(this);
	return shimProv;
#else
	return CConnectionProviderShim::NewL(*this);
#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5		
	}
   
MProviderSelector* CConnectionProviderFactoryShim::DoSelectProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage )
	{
	//create self-destructing object
	CConnectionSelectorShim* selector = new CConnectionSelectorShim(aSelectionNotify);
	if (!selector)
		{
		aSelectionNotify.SelectComplete(NULL,KErrNoMemory);
		aSelectionNotify.Detach();
		}
	else 
		{
		#ifdef SYMBIAN_NETWORKING_UMTSR5	
		selector->SetFactoryIface(this);	
		#endif
		if ( selector->Select(aPreferences, aMessage) != KErrNone )
			{
			selector = NULL;
			}	
		}
		
		
	return selector;
	}
   
MProviderSelector* CConnectionProviderFactoryShim::DoSelectNextLayerProvider( Meta::SMetaData& /*aPreferences*/, ISelectionNotify& /*aSelectionNotify*/, const RMessagePtr2* /*aMessage*/ )
	{	
	return NULL;
	}
	
void CConnectionProviderFactoryShim::DoEnumerateConnectionsL(RPointerArray<TConnectionInfo>& aConnectionInfoPtrArray)
	{
    iNifmanSession->EnumerateConnectionsL(aConnectionInfoPtrArray);
	}

//MConnectionNotify interface to catch the global events
void CConnectionProviderFactoryShim::SelectionComplete(TInt /*aError*/, const TDesC8& /*aSelectionInfo*/)
	{
	
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::ConnectionError(TInt /*aError*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::LinkLayerOpen(TInt /*aError*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::LinkLayerClosed(TInt /*aError*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::ProgressNotification(TInt /*aStage*/, TInt /*aError*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::ProgressNotification(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::ServiceChangeNotification(TUint32 /*aId*/, const TDesC& /*aType*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::InterfaceStateChangeNotification(TDesC8& aInfo)
	{
	TInt count = iConnectionFactoryNotify.Count();
	for (TInt n = 0; n < count; n++)
		{
		iConnectionFactoryNotify[n].InterfaceStateChange(aInfo);
		}
	}
	
void CConnectionProviderFactoryShim::NotifyDataSent(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aUplinkVolume*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::NotifyDataReceived(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aDownlinkVolume*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	
void CConnectionProviderFactoryShim::SubConnectionEvent(const TSubConnectionEvent& /*aSubConnectionEvent*/)
	{
	ASSERT(NULL);//it's never to happen
	}
	

/**
Define the factory query to find out particular CSubConnectionLinkShimClient object based on CConnection address 
*/

class XShimFactoryQuery : public MCommsFactoryQuery
	{
	friend class CConnectionProviderFactoryShim;
	
public:
	explicit XShimFactoryQuery( NetMessages::CTypeIdQuery& aQuery ) :
		iQuery(aQuery),
		iClient(NULL)
		{
		}

protected:
	NetMessages::CTypeIdQuery& iQuery;
	CSubConnectionLinkShimClient* iClient;

public:
	virtual TMatchResult Match( TFactoryObjectInfo& aInfo );
	};

//This method is called for each instance of particular class being managed by CommsFactory framework
MCommsFactoryQuery::TMatchResult XShimFactoryQuery::Match( TFactoryObjectInfo& aInfo )
	{
	CConnectionProviderShim* connectionProvider = static_cast<CConnectionProviderShim*>(aInfo.iInfo.iFactoryObject);
	ASSERT(connectionProvider);
	//call the CNifManSubConnectionShim interface to do the job
	iClient = connectionProvider->QuerySubSessions(iQuery.iTypeId, reinterpret_cast<CConnection*> (iQuery.iHandle));
	
	return iClient ? MCommsFactoryQuery::EMatch : MCommsFactoryQuery::EContinue;
	}
	
/**	Handles incoming messages

@param aNetMessage Messsge reference
@return KErrNone|KErrNotFound|KErrNotSupported
@return if object found passes its pointer to the aNetMessasge object
*/
TInt CConnectionProviderFactoryShim::DoReceiveMessage( NetMessages::CMessage& aNetMessage )
{
STypeId tid = STypeId::CreateSTypeId(NetMessages::KInterfaceUid, NetMessages::ETypeIdQueryId);
NetMessages::CTypeIdQuery& query = static_cast<NetMessages::CTypeIdQuery&>(aNetMessage);
TInt ret = KErrNotSupported;
if (aNetMessage.GetTypeId() == tid && query.iUid == KShimCommsUid)
	{
	XShimFactoryQuery q(query);
	ret = FindObject(q) ? KErrNone : KErrNotFound;
	//carry the result back
	query.iHandle = (TInt)(static_cast<MShimControlClient*>(q.iClient));
	}
return ret;
}


