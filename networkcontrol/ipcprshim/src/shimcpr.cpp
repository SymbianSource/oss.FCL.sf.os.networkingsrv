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
//

#include <comms-infras/nifif.h>
#include <esockmessages.h>
#include <nifman.h>
#include "shimcpr.h"
#include "shimconnsettings.h"
#include "es_prot.h"
#include "shimnifmansconn.h"
#include "ss_glob.h"
#include "connectionSelectorShim.h"

using namespace ESock;

//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
#include <networking/umtsgprs_subconnprovfactory.h>
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE


CConnectionProviderShim* CConnectionProviderShim::NewL(CConnectionProviderFactoryBase& aFactory)
	{
	CConnectionProviderShim* self = new (ELeave) CConnectionProviderShim(aFactory);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CConnectionProviderShim::ConstructL()
	{
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tConstructL"), this));
	// Do nothing, atleast for now;	
	}

CConnectionProviderShim::~CConnectionProviderShim()
	{
	delete iConnectionSettings;
	
	// As soon as the provider has a valid iProvider pointer the ownership has passed to this provider
   	delete iProvider;

	iSelectors.Close();
	iConnections.Close();
	iNifManSubConnections.Close();
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\t~CConnectionProviderShim"), this));
	}
	
void CConnectionProviderShim::AddRefL(CConnectionSelectorShim* aSelector)
	{
	TInt index = iSelectors.Find(aSelector);
	if (-1 == index)
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tAddRef(aSelector %x) adding selector"), this, aSelector));	
		iSelectors.AppendL(aSelector);
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tAddRef(aSelector %x) selector already known"), this, aSelector));	
		}
	}
	

void CConnectionProviderShim::ReleaseRef(CConnectionSelectorShim* aSelector)
	{	
	TInt index = iSelectors.Find(aSelector);
	if (-1 != index)
		{		
		iSelectors[index]->SetProviderNull();
		iSelectors.Remove(index);
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tReleaseRef(aSelector %x) removng selector"), this, aSelector));
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tReleaseRef(aSelector %x) selector not known"), this, aSelector));
		}
	
	if (ShouldIDeleteNow())
		{
		DeleteMeNow();
		}
	}
	
// CConnectionProviderBase virtuals
TBool CConnectionProviderShim::ShouldIDeleteNow()
	{		
	TInt selectors = iSelectors.Count();
	if (0 == selectors && !iHasAnyControlClientJoined)
		{
		return ETrue;
		}
		
	return EFalse;	
	}

void CConnectionProviderShim::DoStartL(Meta::SMetaData& aParams, const RMessagePtr2* aMessage)
	{
#ifdef _DEBUG
	Meta::STypeId tid = aParams.GetTypeId();
	ASSERT( tid.iUid == TUid::Uid( KESockMessagesImplementationUid ) && tid.iType == EESockMessageConnStart);
#endif
	
	CConnStart& p = static_cast<CConnStart&>(aParams);
	if ( p.ConnPrefs() )
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoStartL() with prefs"), this));
		iProvider->StartL(p.StartType(), *p.ConnPrefs(), aMessage);
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoStartL() without prefs"), this));
		iProvider->StartL(p.StartType(), aMessage);
		}
	}
   
void CConnectionProviderShim::DoDataClientJoiningL(MConnectionDataClient& aDataClient)
	{
	(void)aDataClient;
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoDataClientJoiningL(aDataClient %x) [iDataClients.Count()=%d]"), this, &aDataClient, iDataClients.Count()));
	}
	
void CConnectionProviderShim::DoDataClientLeaving(MConnectionDataClient& aDataClient)
	{
	(void)aDataClient;
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoDataClientLeaving(aDataClient %x) [iDataClients.Count()=%d]"), this, &aDataClient, iDataClients.Count()));
	}
	
void CConnectionProviderShim::DoControlClientJoiningL(MConnectionControlClient& aControlClient)
	{
	(void)aControlClient;
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoControlClientJoiningL(aControlClient %x) [iControlClients.Count()=%d]"), this, &aControlClient, iControlClients.Count()));
	}
	
void CConnectionProviderShim::DoControlClientLeaving(MConnectionControlClient& aControlClient)
	{
	(void)aControlClient;
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoControlClientLeaving(aControlClient %x) [iControlClients.Count()=%d]"), this, &aControlClient, iControlClients.Count()));
	}
	
TInt CConnectionProviderShim::DoStop(TInt aError, const RMessagePtr2* aMessage)
	{
#ifdef SYMBIAN_NETWORKING_UMTSR5
	// when RConn::Stop() happens, that means one is releasing the connection, so the IAP associated with
	// the Lock should go away. The code below will set the status of IAP locked to false, so that any further
	// Start can lock it 			
	iFactoryShim->SetIAPLockStatus(EFalse,-1);
#endif // SYMBIAN_NETWORKING_UMTSR5			
	return iProvider->Stop( aError, aMessage );
	}
   
void CConnectionProviderShim::DoProgressL(Meta::SMetaData& aParams) const
	{
#ifdef _DEBUG
	Meta::STypeId tid = aParams.GetTypeId();
#endif
	ASSERT( tid.iUid == TUid::Uid( KESockMessagesImplementationUid ) && tid.iType == EESockMessageCommsNifBuffer);
	TMetaNifProgressBuf& p = static_cast<TMetaNifProgressBuf&>(aParams);
	iProvider->ProgressL( p.iNifProgressBuf );
	}
   
void CConnectionProviderShim::DoLastProgressError(Meta::SMetaData& aBuffer)
	{
#ifdef _DEBUG
	Meta::STypeId tid = aBuffer.GetTypeId();
#endif
	ASSERT( tid.iUid == TUid::Uid( KESockMessagesImplementationUid ) && tid.iType == EESockMessageCommsNifBuffer);
	TMetaNifProgressBuf& p = static_cast<TMetaNifProgressBuf&>(aBuffer);
	iProvider->LastProgressError( p.iNifProgressBuf );
	}
   
void CConnectionProviderShim::DoRequestServiceChangeNotificationL()
	{
	iProvider->RequestServiceChangeNotificationL();
	}
   
void CConnectionProviderShim::DoCancelServiceChangeNotification()
	{
	iProvider->CancelServiceChangeNotification();
	}
   
void CConnectionProviderShim::DoControlL(TUint aOptionLevel, TUint aOptionName, Meta::SMetaData& aOption, const RMessagePtr2* aMessage)
	{
#ifdef _DEBUG
	Meta::STypeId tid = aOption.GetTypeId();
#endif
	ASSERT( tid.iUid == TUid::Uid( KESockMessagesImplementationUid ) && tid.iType == EESockMessageCommsDes8);
	::TMetaDes8& p = static_cast< ::TMetaDes8& >(aOption);
	ASSERT(p.iDes);
	iProvider->ControlL(aOptionLevel, aOptionName, *((TDes8*)(p.iDes)), aMessage);
	}
   
TInt CConnectionProviderShim::DoAllSubConnectionNotificationEnable()
	{
	return iProvider->AllSubConnectionNotificationEnable();
	}
   
TInt CConnectionProviderShim::DoCancelAllSubConnectionNotification()
	{
	return iProvider->CancelAllSubConnectionNotification();
	}

void CConnectionProviderShim::DoSendIoctlMessageL(const RMessage2& aMessage)
	{
	iProvider->SendIoctlMessageL(aMessage);
	}
   
void CConnectionProviderShim::DoSendCancelIoctl()
	{
   	iProvider->SendCancelIoctl();	
	}

TInt CConnectionProviderShim::DoEnumerateSubConnectionsL(TUint& aCount)
	{
	return iProvider->EnumerateSubConnections(aCount);
	}

TUint CConnectionProviderShim::DoEnumerateClientsL(HBufC8*& /*aClientInfoBuffer*/, TEnumClients /*aClientType*/)
	{
	return 0;
	}

CConnectionSettings& CConnectionProviderShim::DoSettingsAccessL()
	{
	ASSERT(iProvider);
	if (iConnectionSettings == NULL)
    	{
    	iConnectionSettings = new (ELeave)CConnectionSettingsShim(*iProvider);	    
    	}
	return *iConnectionSettings;
	}
	
void CConnectionProviderShim::ConnectionJoiningL( const CConnection& aConnection )
	{
	if (iConnections.Find(&aConnection) < 0)
		{
		iConnections.AppendL(&aConnection);
		}
	TInt count = iNifManSubConnections.Count();
	for ( TInt n = 0; n < count; n++ )
		{//here we exactly know what we are dealing with hence the cast
		CNifManSubConnectionShim* c = static_cast<CNifManSubConnectionShim*>(iNifManSubConnections[n]);
		MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(c);
		subint->ConnectionJoiningL(aConnection);
		}
	}
	
void CConnectionProviderShim::ConnectionLeaving( const CConnection& aConnection )
	{
	TInt count = iNifManSubConnections.Count();
	for ( TInt n = 0; n < count; n++ )
		{//here we exactly know what we are dealing with hence the cast
		CNifManSubConnectionShim* c = static_cast<CNifManSubConnectionShim*>(iNifManSubConnections[n]);
		MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(c);
		subint->ConnectionLeaving(aConnection);
		}
	TInt n = iConnections.Find(&aConnection);
	if (n >= 0)
		{
		iConnections.Remove(n);
		}
	}

void CConnectionProviderShim::DoConnectionControlActivityL( TControlActivity aControlActivity, const Meta::SMetaData* aData, const RMessagePtr2* aMessage )
	{
	switch (aControlActivity)
		{
		case EAttachNormal:
		case EAttachMonitor:
			iProvider->AttachToConnectionL(iConnectionInfo.Right(KConnInfoPart), aControlActivity == EAttachMonitor, aMessage);
			if ( aData == NULL )
				{
				break;
				}
			aControlActivity = EConnJoin; //and fall through
		case EConnJoin:
			if ( !aData || !aData->IsTypeOf( STypeId::CreateSTypeId(KShimCommsUid, Meta::KNetMetaTypeAny) ) )
				{
				User::Leave(KErrArgument);
				}
			ConnectionJoiningL(static_cast<const TShimConnectionInfo*>(aData)->iConnection);
			break;
		case EConnLeave:
			if ( !aData || !aData->IsTypeOf( STypeId::CreateSTypeId(KShimCommsUid, Meta::KNetMetaTypeAny) ) )
				{
				User::Leave(KErrArgument);
				}
			ConnectionLeaving(static_cast<const TShimConnectionInfo*>(aData)->iConnection);
			break;
		case ESetUsageProfile:
			{
			ASSERT(aData);
	#ifdef _DEBUG
			Meta::STypeId tid = aData->GetTypeId();
	#endif
			ASSERT( tid.iUid == TUid::Uid( KESockMessagesImplementationUid ) && tid.iType == EESockMessageCommsDes8);
			const ::TMetaDes8* p = static_cast<const ::TMetaDes8*>(aData);
			ASSERT(p->iDes);
			const TUint* profile = reinterpret_cast<const TUint*>(p->iDes->Ptr());
			if (iUsageProfile != *profile)
				{
				__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tSetting usage profile %x -> %x"),
						this, iUsageProfile & 0xff, *profile & 0xff));
				iUsageProfile = *profile;
				iProvider->SetUsageProfile(*profile);
				}
			break;
			}
		};
	}
	
CNifManSubConnectionShim* CConnectionProviderShim::FindSubConnection( TSubConnectionUniqueId aId )
	{
	CNifManSubConnectionShim* client = NULL;
	TInt count = iNifManSubConnections.Count();
	for ( TInt n = 0; n < count; n++ )
		{//here we exactly know what we are dealing with hence the cast
		CNifManSubConnectionShim* c = static_cast<CNifManSubConnectionShim*>(iNifManSubConnections[n]);
		MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(c);
		if (subint->Id() == aId )
			{
			client = c;
			break;
			}
		}
	return client;
	}
	
// Define methods from the interface specified by MConnectionNotify
void CConnectionProviderShim::ProgressNotification(TInt aStage, TInt aError)
	{
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tProgressNotification(%d, %d)"), this, aStage, aError));
	
	TInt max = iControlClients.Count();
	for (TInt n = max - 1 ; n >= 0 ; n--)
		{
		MConnectionControlClient* client = iControlClients[n];
		client->ProgressNotification(aStage, aError);
		//check whether the client dissapeared or not
		if ( n < iControlClients.Count() && client == iControlClients[n] )
			{
			if (aStage == KLinkLayerOpen)
				{
				iControlClients[n]->LayerUp(aError);
				}
			else if (aError != KErrNone)
				{
				iControlClients[n]->ConnectionError(aStage, aError);
				}
			}
		}
	if (aStage == KConnectionUninitialised && !iIsAlreadyUninitialised /* Guard against receiving KConnectionUninitialised multiple times*/)
		{						
		iIsAlreadyUninitialised = ETrue;
		TInt selectors = iSelectors.Count();
		for (TInt i = 0; i < selectors; ++i)
			{
			iSelectors[i]->SetProviderNull();
			}		
		DeleteMeNow();		
		}
	}

void CConnectionProviderShim::LinkLayerOpen(TInt /*aError*/)
/**
Notification from the connection provider - ignored.

*/
	{	
	}
   
void CConnectionProviderShim::LinkLayerClosed(TInt /*aError*/)
/**
Notification from the connection provider - not currently used.

*/
	{
	User::Invariant();
	}

void CConnectionProviderShim::SelectionComplete(TInt aError, const TDesC8& /*aSelectionInfo*/)
	{//obsolete call selection done by factories this could happen only on Attach
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tSelectionComplete(%d)"), this, aError));
	ProgressNotification(KFinishedSelection, aError);
	}
   
void CConnectionProviderShim::ConnectionError(TInt /*aError*/)
/**
Notification from the connection provider - not currently used.

*/
	{
	User::Invariant();
	}
   
void CConnectionProviderShim::ProgressNotification(TSubConnectionUniqueId aSubConnectionUniqueId, TInt aStage, TInt aError)
	{
	MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(FindSubConnection(aSubConnectionUniqueId));
	if (subint)
		{//here we exactly know what we are dealing with hence the cast
		subint->ProgressNotification(aStage, aError, KNullDesC8);
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tProgressNotification(subConnectionId %d, %d, %d) subconnection ID lookup error occured"),
					 this, aSubConnectionUniqueId, aStage, aError));
		}
	}
   
void CConnectionProviderShim::ServiceChangeNotification(TUint32 aId, const TDesC& aType)
	{
	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
		iControlClients[i]->ServiceChangeNotification(aId, aType);
		}
	}
   
void CConnectionProviderShim::InterfaceStateChangeNotification(TDesC8& /*aInfo*/)
	{//served through factories
	ASSERT( NULL );
	}
   
void CConnectionProviderShim::NotifyDataSent(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aUplinkVolume)
	{//here we exactly know what we are dealing with hence the cast
	MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(FindSubConnection(aSubConnectionUniqueId));
	if (subint)
		{
		subint->NotifyDataSent(aUplinkVolume,0);
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tNotifyDataSent(subConnectionId %d, aUplinkVolume %d) subconnection ID lookup error %d occured"),
					 this, aSubConnectionUniqueId, aUplinkVolume, KErrNotFound));
		}
	}
   
void CConnectionProviderShim::NotifyDataReceived(TSubConnectionUniqueId aSubConnectionUniqueId, TUint aDownlinkVolume)
	{//here we exactly know what we are dealing with hence the cast
	MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(FindSubConnection(aSubConnectionUniqueId));
	if (subint)
		{
		subint->NotifyDataReceived(aDownlinkVolume,0);
		}
	else
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tNotifyDataReceived(SubConnectionId %d, aDownLinkVolume %d); subconnection ID lookup error %d occured"),
					 this, aSubConnectionUniqueId, aDownlinkVolume, KErrNotFound));
		}
	}

void CConnectionProviderShim::SubConnectionEvent(const TSubConnectionEvent& aSubConnectionEvent)
/**
Upcall from connection provider indicating that a subconnection event has occured

@param aSubConnectionEvent The event that occured
@note Even if the not client is listening for subconnection events, we still need them to manage the array of subconnections by listening for started and stopped events on each subconnection
@note Events are not handled on a per-subconnection basis - all events are distributed to anyone who uses the AllSubConnectionNotification() method, so no subconnection/subinterface involvement is necessary
*/
	{
	TSubConnectionEventType eventType(aSubConnectionEvent.iEventType);
	TInt ret = KErrNone;
	
	TSockManData* sockManData = SockManGlobals::Get();
	ASSERT(sockManData != NULL);
	
	CNifManSubConnectionShim* c = NULL;
	switch(eventType)
		{
		case ESubConnectionOpened:
		    {			
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tSubConnectionEvent() ESubConnectionOpened"), this));
			c = FindSubConnection(aSubConnectionEvent.iSubConnectionUniqueId);
			if (c == NULL)
				{
				// no instance yet for particular Id, create one
				TRAP(ret,c = new (ELeave)CNifManSubConnectionShim(*this));
				if(ret!=KErrNone)	// nothing we can do here, no point in throwing error back to event source - nothing they can do about it
					{
					// Array integrity will be broken here - may cause errors later with events on this subconnection if we recover
					__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tERROR: Subconnection opened event; could not create CSubInterface - possible OOM condition - subconnection ID %d, error %d - subinterface array integrity failure"),
							    this, aSubConnectionEvent.iSubConnectionUniqueId, ret));
					break;
					}
				ret = iNifManSubConnections.Append(c);
				if (ret != KErrNone)
					{
						__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tERROR: could not append CNifManSubConnectionShim to the list - subconnection ID %d, error %d - subinterface array integrity failure"),
							    this, aSubConnectionEvent.iSubConnectionUniqueId, ret));
						c->DeleteAsync();
						break;
					}
				}
			MSubInterfaceShim* subint = static_cast<MSubInterfaceShim*>(c);
			subint->SetSubConnectionUniqueId(aSubConnectionEvent.iSubConnectionUniqueId);
			
			TInt count = iConnections.Count();
			for (TInt i = count - 1; i >= 0; i--)
				{
				TRAP(ret,c->ConnectionJoiningL(*iConnections[i]));
				if(ret!=KErrNone)	// nothing we can do here, no point in throwing error back to event source - nothing they can do about it
					{
					// Array integrity will be broken here - may cause errors later with events on this subconnection if we recover
					__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tERROR: connection joining subconnection opened event; - possible OOM condition - subconnection ID %d, error %d - subinterface array integrity failure"),
								 this, aSubConnectionEvent.iSubConnectionUniqueId, ret));
					break;
					}
				}
		    }
			break;

		case ESubConnectionClosed:
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tSubConnectionEvent() ESubConnectionClosed"), this));
			c = FindSubConnection(aSubConnectionEvent.iSubConnectionUniqueId);
			if(!c)		// then there was an error (probably that we couldn't find the object in the array) - write error to log - not much else we can do
				{
				__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tERROR: Subconnection closed event; could not find subconnection ID %d, lookup error %d occured"),
						    this, aSubConnectionEvent.iSubConnectionUniqueId, KErrNotFound));
				}
			else
				{
				iDeleteAsynchOnly = ETrue;
				TInt n = iNifManSubConnections.Find(c);
				if (n >= 0)
					{
					iNifManSubConnections.Remove(n);
					}
				c->DeleteAsync();
				}
			break;

		default:
			// do nothing; we only care about open and close events
			break;
		}	
	
	// Rebroadcast all events to all attached control clients
	TInt max = iControlClients.Count();
	for (TInt i = max - 1; i >= 0 ; i--)
		{
		iControlClients[i]->SubConnectionEvent(NULL, aSubConnectionEvent);
		}

	CheckDeleteThis();

	}

CConnectionProvdBase* CConnectionProviderShim::GetNifSession()
    {
    return iProvider;
    }
    
void CConnectionProviderShim::InitialiseL(CConnectionProvdBase* aStarterSession)
	{
	ASSERT(aStarterSession);
	//
	//can't set the provider twice
	//
	ASSERT(iProvider == NULL);	
	iProvider = Nif::NewConnectionL(this, 0);	
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tInitialiseL(aStarterSession %x) iProvider %x"), this, aStarterSession, iProvider));
	//
	// After creating the new nif session (iProvider) we also need to transfer the connection attempt count
	// from the old nif session (aStarterSession) to the new one, since it was the one that initially 
	// started the interface and connection attempt count was incremented in StartL of nif session
	//
	TPckg<TInt> connectionAttempt(-1);
	aStarterSession->ControlL(KCOLProvider, KNifSessionGetConnectionAttempt, connectionAttempt, NULL);
	ASSERT(connectionAttempt() != -1);			
	iProvider->ControlL(KCOLProvider, KNifSessionSetConnectionAttempt, connectionAttempt, NULL);
	//
	// Finally, call ConnectionControlActivityL to ensure that the provider is attached to the connection
	// and progress notifications from the provider end up in the connection object too
	//		
	DoConnectionControlActivityL(EAttachNormal, NULL, NULL);
	
//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	_LIT(KIfNameField, "IfName");
	TBuf<KCommsDbSvrMaxFieldLength> nifname;
	iProvider->GetDes16SettingL(KIfNameField(), nifname);
    if (nifname.CompareF (KSpudName) == 0)
        {
        iSubConnectionType = KUmtsGprsSubConnectionProviderFactoryId;
        }
#endif //SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	iHasAnyControlClientJoined = ETrue;
    }
    
void CConnectionProviderShim::SetBlockProgressesL(CConnectionProvdBase* aStarterSession)
/**
Ensure that the "blocked progress" status of aStarterSession is copied across to iProvider session.

@param aStarterSession session from which to copy "blocked progress" status.
*/
	{
	TPckg<TBool> blockProgresses(EFalse);
	aStarterSession->ControlL(KCOLProvider, KNifSessionGetBlockProgresses, blockProgresses, NULL);
	iProvider->ControlL(KCOLProvider, KNifSessionSetBlockProgresses, blockProgresses, NULL);
	}

void CConnectionProviderShim::DoJoinNextLayerL(CConnectionProviderBase* aNextLayer)
	{
	(void)aNextLayer;
	if ( !iProvider )
		{
		ASSERT( !aNextLayer );
//		AddRef(); 	//The AddRef needs to be here since it follows the logic in the selector shim. 	
		iProvider = Nif::NewConnectionL(this, 0);
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionProviderShim %08x:\tDoJoinNextLayerL(aNextLayer %x) iProvider %x"), this, aNextLayer, iProvider));
		}
	}
	
CConnectionProviderBase* CConnectionProviderShim::DoNextLayer() const
    {
    //This is (by design) the last layer.
    return NULL;
    }
    
    
TInt CConnectionProviderShim::DoCanDoSubConnection(RSubConnection::TSubConnType aSubConnType) const
	{
	(void)aSubConnType;
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	return iSubConnectionType;
#else	
  	return 0;
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS
  	}

/**
Find particular CSubConnectionLinkShimClient object with the pointer to given aConnection

@param aTypeId Id of CNifManSubConnectionShim
@param aConnection aConnection pointer 
@return CSubConnectionLinkShimClient* or NULL
*/
CSubConnectionLinkShimClient* CConnectionProviderShim::QuerySubSessions(TInt32 aTypeId,const CConnection* aConnection)
	{
	 TInt count = iNifManSubConnections.Count();
	 CNifManSubConnectionShim *nifManSubConnection;
	 CSubConnectionLinkShimClient *linkShimClient = NULL;
	 for (TInt i = count - 1; i >= 0; i--)
	 	{
		nifManSubConnection = iNifManSubConnections[i];
		if (nifManSubConnection && nifManSubConnection->Id() == aTypeId)	
			{
			TInt n = 0;
			do
				{
				 linkShimClient = nifManSubConnection->ShimClient(n++);			
				}
			while (linkShimClient && !linkShimClient->Match(*aConnection));
			}
	 	}
	 		
	return linkShimClient;
	}
	
#ifdef SYMBIAN_NETWORKING_UMTSR5
void CConnectionProviderShim::SetFactoryIfactory(CConnectionProviderFactoryShim *aFactoryShim)
	{
	iFactoryShim = aFactoryShim;	
	}
#endif // SYMBIAN_NETWORKING_UMTSR5	


