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
// This class is an example implementation of a bearer-mobile network session. It is intended as a guide only and does
// not employ any advanced bearer selection algorithms. It is envisaged that users of these classes will want to derive their own
// implementations from CIPNetworkSession. 
// This is part of an ECOM plug-in
// 
//

#include "ipcprprovider.h"	// for CIPNetworkConnectionProvider
#include <comms-infras/ss_log.h>

CIPNetworkConnectionProvider* CIPNetworkConnectionProvider::NewL(CConnectionProviderFactoryBase& aFactory)
	{			
	CIPNetworkConnectionProvider* p = new (ELeave) CIPNetworkConnectionProvider(aFactory);
	CleanupStack::PushL(p);
	p->iMDes = TMetaDes8::NewL(NULL);
	CleanupStack::Pop(p);
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider::NewL() %08x"), p));
	return p;	
	}
	
CIPNetworkConnectionProvider::~CIPNetworkConnectionProvider()
	{
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider   %08x:\t~CIPNetworkConnectionProvider"), this));
	delete iMDes;
	if (iShimCpr)
		{
		iShimCpr->Leave(*this);
		}
	}
	
#ifdef SYMBIAN_NETWORKING_UMTSR5	

TUint32 CIPNetworkConnectionProvider::GetAppSecureId()
	{
	return iAppSecureId;
	}

void CIPNetworkConnectionProvider::SetAppSecurId(TUint32 aSecureId)
	{
	iAppSecureId = aSecureId;
	}
	
TAny* CIPNetworkConnectionProvider::DoFetchInterfaceInstanceL( const STypeId& aTid )
	{
	STypeId typeId = STypeId::CreateSTypeId(KConnectionAppInfoInterfaceId,0);
	if (typeId == aTid)
    	{
    	MConnectionAppIdInfo* ipcpr = static_cast<MConnectionAppIdInfo*>(this);
    	return ipcpr;
    	}
    return NULL;
	}
	
#endif // SYMBIAN_NETWORKING_UMTSR5	
//
// from MConnectionControlClient
void CIPNetworkConnectionProvider::ConnectionGoingDown(CConnectionProviderBase& aConnProvider)
	{
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider %08x:\tConnectionGoingDown(aConnProvider %08x)"), this, &aConnProvider));
	if (&aConnProvider == iShimCpr)
		{
		iShimCpr = NULL;
		DeleteMeNow();
		}
	}

void CIPNetworkConnectionProvider::ProgressNotification(TInt aStage, TInt aError)
	{
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider %08x:\tProgressNotification(aStage %d aError %d)"), this, aStage, aError));
	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
   		{
	   	iControlClients[i]->ProgressNotification(aStage, aError);
		}
	}
   
void CIPNetworkConnectionProvider::ConnectionError(TInt aStage,  TInt aError)
	{
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider %08x:\tConnectionError(aStage %d aError %d)"), this, aStage, aError));
	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
		iControlClients[i]->ConnectionError(aStage, aError);
		}
	max = iDataClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
		iDataClients[i]->ConnectionError(aStage, aError);
		}
  	}
   
void CIPNetworkConnectionProvider::ServiceChangeNotification(TUint32 aId, const TDesC& aType)
	{	
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider %08x:\tServiceChangeNotification(aId %u aType %s)"), this, aId, &aType));
   	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
   		{
	   	iControlClients[i]->ServiceChangeNotification(aId, aType);
		}
  	}   
 
void CIPNetworkConnectionProvider::SubConnectionEvent(CSubConnectionProviderBase* aSubConnNextLayerProvider, const TSubConnectionEvent& aSubConnectionEvent)
	{
	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
	   	iControlClients[i]->SubConnectionEvent(aSubConnNextLayerProvider, aSubConnectionEvent);
		}
	}

void CIPNetworkConnectionProvider::LayerUp(TInt aError)
	{
	TInt max = iControlClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; --i)
		{
		iControlClients[i]->LayerUp(aError);
		}
		
	// broadcast the event to the data clients also, sideways
	max = iDataClients.Count();	
	for (TInt j = max - 1; j >= 0 ; --j)
		{				
		iDataClients[j]->Notify(MConnectionDataClient::ENotifyLayerUp, this, aError, NULL);
		}
   }
   
MConnectionControlClient::TCtlType CIPNetworkConnectionProvider::CtlType() const
	{
	return MConnectionControlClient::ENormal;
	}

//
// from CConnectionProviderBase
void CIPNetworkConnectionProvider::DoDataClientJoiningL(MConnectionDataClient& aDataClient)
	{
	(void)(aDataClient);
	UpdateUsageProfile(NULL);
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider [this=%08x]:\tDoDataClientJoiningL [iDataClients.Count=%d] [aDataClient=%08x]"), this, iDataClients.Count(), &aDataClient));
	}
   
void CIPNetworkConnectionProvider::DoDataClientLeaving(MConnectionDataClient& aDataClient)
	{
	(void)(aDataClient);
	UpdateUsageProfile(NULL);
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider [this=%08x]:\tDoDataClientLeaving [iDataClients.Count=%d] [aDataClient=%08x]"), this, iDataClients.Count(), &aDataClient));
	}
   
void CIPNetworkConnectionProvider::DoControlClientJoiningL(MConnectionControlClient& aControlClient)
	{
	(void)(aControlClient);
	UpdateUsageProfile(NULL);
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider [this=%08x]:\tDoControlClientJoiningL [iControlClients.Count=%d] [aControlClient=%08x]"), this, iControlClients.Count(), &aControlClient));
  	}
   
void CIPNetworkConnectionProvider::DoControlClientLeaving(MConnectionControlClient& aControlClient)
	{
	UpdateUsageProfile(&aControlClient);
	__CFLOG_VAR((KIpcprTag, KIpcprSubTag, _L8("CIPNetworkConnectionProvider [this=%08x]:\tDoControlClientLeaving [iControlClients.Count=%d] [aControlClient=%08x]"), this, iControlClients.Count(), &aControlClient));
  	}

void CIPNetworkConnectionProvider::UpdateUsageProfile(MConnectionControlClient* aControlClient)
/**
Update the Usage Profile if there has been a change in the RConnection's or Sessions associated
with this provider.
@param aControlClient A control client that should be ignored when calculating the usage profile.
*/
	{
	TUint usageProfile = 0;
	TInt ignoredClient = 0;
	
	TInt max = iControlClients.Count();
	TInt nConnectionMonitorCount = 0;
	TInt nSessionCount = 0;
	for (TInt n = 0; n < max; n++)
		{
		MConnectionControlClient* client = iControlClients[n];
		
		// Ignore the control client passed as argument (it is the one about to leave).
		if (!aControlClient || aControlClient != client)
			{
			MConnectionControlClient::TCtlType type = client->CtlType();
			switch (type)
				{
				case MConnectionControlClient::EMonitor:
					nConnectionMonitorCount++;
					break;
				case MConnectionControlClient::ESession:
					nSessionCount++;
					break;
				}
			}
		else
			{
			ASSERT(ignoredClient == 0);
			ignoredClient = 1;
			}
		}

	// Assert that aControlClient, if specified, is	in iControlClients[]
	ASSERT(aControlClient == NULL || ignoredClient == 1);
	
	if (max - ignoredClient > nConnectionMonitorCount)
		usageProfile |= KConnProfileMedium;

	if (nSessionCount > 0)
		usageProfile |= KConnProfileLong;

	if (iShimCpr)
		{
		TBuf8<sizeof(TUint)> buf;
		buf.AppendNum(usageProfile);

		//Meta derived buffer (iMDes) is only used here to pass
		//the usageProfile down to the shimcpr
		iMDes->iDes = &buf;

		//ESetUsageProfile operation cannot leave in fact=> 
		//=>the trap here just to make leavescan happy
		TRAP_IGNORE(iShimCpr->ConnectionControlActivityL(ESetUsageProfile, iMDes, NULL));
		
		//iMDes->iDes will not be used ever again but we clean it so that it is clear
		iMDes->iDes = NULL;
		}
	}

void CIPNetworkConnectionProvider::DoStartL(Meta::SMetaData& aParams, const RMessagePtr2* aMessage)
	{
	if (iShimCpr)
		{
		iShimCpr->StartL(aParams, aMessage);
		}
	else
		{
		User::Leave(KErrNotReady);
		}
  	}
   	
TInt CIPNetworkConnectionProvider::DoStop(TInt aError, const RMessagePtr2* aMessage)
	{
	return 	iShimCpr ? iShimCpr->Stop(aError, aMessage) : KErrNotReady;
  	}   

void CIPNetworkConnectionProvider::DoProgressL(Meta::SMetaData& aBuffer) const
	{
	if (iShimCpr)
		{
		iShimCpr->ProgressL(aBuffer);
		}
  	}
   
void CIPNetworkConnectionProvider::DoLastProgressError(Meta::SMetaData& aBuffer)
	{
	if (iShimCpr)
		{
		iShimCpr->LastProgressError(aBuffer);
		}
  	}
   
void CIPNetworkConnectionProvider::DoRequestServiceChangeNotificationL()
	{
	if (iShimCpr)
		{
		iShimCpr->RequestServiceChangeNotificationL();
		}
  	}
   
void CIPNetworkConnectionProvider::DoCancelServiceChangeNotification()
	{
	if (iShimCpr)
		{
		iShimCpr->CancelServiceChangeNotification();
		}
  	}
   
void CIPNetworkConnectionProvider::DoControlL(TUint aOptionLevel, TUint aOptionName, Meta::SMetaData& aOption, const RMessagePtr2* aMessage)
	{
	if (iShimCpr)
		{
		iShimCpr->ControlL(aOptionLevel, aOptionName, aOption, aMessage);
		}
	else
		{
		User::Leave(KErrNotReady);	
		}
  	}

TInt CIPNetworkConnectionProvider::DoEnumerateSubConnectionsL(TUint& aCount)
	{
	return iShimCpr ? iShimCpr->EnumerateSubConnectionsL(aCount) : KErrNotReady;
	}

TUint CIPNetworkConnectionProvider::DoEnumerateClientsL(HBufC8*& aClientInfoBuffer, TEnumClients aClientType)
/**
Returns information about the clients of this Interface

@param aCount on return contains the number of clients using this Interface
@param aClientInfoBuffer on return contains a TPckg<> containing information about each client
@exception leaves with KErrNoMemory if memory allocation fails
*/
	{	
	const TInt KInfoBufMaxLength = 1024;  //is this large enough?
	TBuf8<KInfoBufMaxLength> infoBuf;

	TUint count = 0;
	STypeId tid = STypeId::CreateSTypeId(KConnectionClientExtUid,EConnectionClientDesc);
	TInt max = iControlClients.Count();
	for ( TInt n = 0; n < max; n++ )
		{
		MConnectionClientDesc* intf = reinterpret_cast<MConnectionClientDesc*>(iControlClients[n]->FetchInterfaceInstanceL(*this,tid));
		if ( intf )
			{
			TConnectionProcessInfo cinfo;
			cinfo.GetInfoL(aClientType, count, *intf, infoBuf);
			}
		}
	STypeId tid2 = STypeId::CreateSTypeId(KConnectionClientExtUid,EConnectionEnumerateClients);
	max = iDataClients.Count();
	for ( TInt n = 0; n < max; n++ )
		{
		MConnectionEnumerateClients* intf = reinterpret_cast<MConnectionEnumerateClients*>(iDataClients[n]->FetchInterfaceInstanceL(*this,tid2));
		if ( intf )
			{
			intf->EnumerateClientsL(count, infoBuf, aClientType);
			}
		}

	aClientInfoBuffer = infoBuf.AllocL();
	return count;
	}

void CIPNetworkConnectionProvider::DoConnectionControlActivityL( CConnectionProviderBase::TControlActivity aControlActivity, const Meta::SMetaData* aData, const RMessagePtr2* aMessage )
	{
	if (iShimCpr)
		{
		iShimCpr->ConnectionControlActivityL(aControlActivity, aData, aMessage);
		}
	}
	
CConnectionSettings& CIPNetworkConnectionProvider::DoSettingsAccessL()
	{
	if (iShimCpr == NULL)
		{
		User::Leave(KErrNotReady);
		}
	return iShimCpr->SettingsAccessL();
	}
	
TInt CIPNetworkConnectionProvider::DoAllSubConnectionNotificationEnable()
	{
	return !iShimCpr ? KErrNotReady : iShimCpr->AllSubConnectionNotificationEnable();
  	}

TInt CIPNetworkConnectionProvider::DoCancelAllSubConnectionNotification()
	{
	return !iShimCpr ? KErrNotReady : iShimCpr->CancelAllSubConnectionNotification();
  	}
   
void CIPNetworkConnectionProvider::DoSendIoctlMessageL(const RMessage2& aMessage)
	{
	if (iShimCpr)
		{
		iShimCpr->SendIoctlMessageL(aMessage);
		}
	else
		{
		User::Leave(KErrNotReady);	
		}
  	}
   
void CIPNetworkConnectionProvider::DoSendCancelIoctl()
	{
	if (iShimCpr)
		{
		iShimCpr->SendCancelIoctl();
		}
  	}
      
TInt CIPNetworkConnectionProvider::DoCanDoSubConnection(RSubConnection::TSubConnType /*aSubConnType*/) const
	{
	return ETrue;
  	}

void CIPNetworkConnectionProvider::DoJoinNextLayerL(CConnectionProviderBase* aNextLayer)
	{
	ASSERT( !iShimCpr );
	ASSERT( aNextLayer );
	iShimCpr = aNextLayer;
    SetConnectionInfo(iShimCpr->ConnectionInfo());
    // join ourselves as a connection control client to the lower provider
    iShimCpr->JoinL(*this);
	}

CConnectionProviderBase* CIPNetworkConnectionProvider::DoNextLayer() const
	{
  	return iShimCpr;
  	}
   
	
