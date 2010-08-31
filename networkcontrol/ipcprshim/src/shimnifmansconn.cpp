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
// SHIMSCPR.CPP
// This is part of an ECOM plug-in
// 
//

#include <ss_std.h>
#include <comms-infras/ss_log.h>
#include "shimnifmansconn.h"

START_ATTRIBUTE_TABLE(TShimConnectionInfo, KShimCommsUid, Meta::KNetMetaTypeAny)
END_ATTRIBUTE_TABLE()

TInt CNifManSubConnectionShim::AsyncDestructorCb(TAny* aInstance)
	{
	CNifManSubConnectionShim* nifManSubConnection= reinterpret_cast<CNifManSubConnectionShim*>(aInstance);
	delete nifManSubConnection;
	return KErrNone;
	}

/**
Create a new CNifManSubConnectionShim to act as a mux/demux for subconnections
*/
CNifManSubConnectionShim::CNifManSubConnectionShim (CConnectionProviderShim& aProviderShim)
   :iSubConnectionsUniqueId(0), iConnectionProvider(&aProviderShim),
   iAsyncDestructor(CActive::EPriorityStandard + 1)
	{
	__CFLOG_VAR((KShimScprTag, KShimScprSubTag, _L8("CNifManSubConnectionShim [this=%08x]:\tCNifManSubConnectionShim() [MConnectionDataClient=%08x]"),
	   this, (MConnectionDataClient*)this));
	iAsyncDestructor.Set(TCallBack(AsyncDestructorCb, this));
	}

/**
D'tor
*/
CNifManSubConnectionShim::~CNifManSubConnectionShim()
	{
	__CFLOG_VAR((KShimScprTag, KShimScprSubTag, _L8("CNifManSubConnectionShim::~CNifManSubConnectionShim() %08x"), this));
	iShimClients.ResetAndDestroy();
	delete iConnDataTransferShim;
	}


void CNifManSubConnectionShim::DeleteAsync()
	{
	if ( !iAsyncDestructor.IsActive() )
		{
		iAsyncDestructor.CallBack();
		}
    }

TInt CNifManSubConnectionShim::FindClient(const CConnection& aConnection)
	{
	TInt max = iShimClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
		CSubConnectionLinkShimClient* client = iShimClients[i];
		if ( client->Match(aConnection) )
			{
			return i;
			}
		}
	return KErrNotFound;
	}
	
void CNifManSubConnectionShim::ConnectionJoiningL(const CConnection& aConnection)
	{//create a new CSubConnectionLinkShimClient for the joining conection
	TInt i = FindClient(aConnection);
	if ( i == KErrNotFound )
		{
		CSubConnectionLinkShimClient* client = new (ELeave)CSubConnectionLinkShimClient(aConnection,*this);
		CleanupStack::PushL(client);
		//create data transfer object if not created yet
		CreateDataTransferL();
		iConnDataTransferShim->RegisterClientL(*client);
		TInt ret = iShimClients.Append(client);
		if (ret != KErrNone)
			{
			iConnDataTransferShim->DeRegisterClient(*client);
			User::Leave(ret);
			}
		CleanupStack::Pop(client);
		}
	}
	
void CNifManSubConnectionShim::ConnectionLeaving(const CConnection& aConnection)
	{//destroy a CSubConnectionLinkShimClient belonging to leaving conection
	TInt i = FindClient(aConnection);
	if ( i >= 0 )
		{
		CSubConnectionLinkShimClient* client = iShimClients[i];
		iShimClients.Remove(i);
		delete client;
		}
	}

void CNifManSubConnectionShim::ConnectionGoingDown(CConnectionProviderBase& /*aConnProvider*/)
	{
	__CFLOG_VAR((KShimScprTag, KShimScprSubTag, _L8("CNifManSubConnectionShim %08x:\tConnectionGoingDown() Id %d"), 
						 this, iSubConnectionsUniqueId));
						 
   // The ConnectionProvider has told us its going down so we delete ourselves, clearing
   // the pointer to it so our d'tor doesn't make any calls on it.
	iConnectionProvider = NULL;
	delete this;
	}
	
void CNifManSubConnectionShim::ConnectionError(TInt /*aStage*/, TInt /*aError*/)
	{
	}
	
void CNifManSubConnectionShim::Notify(TNotify /*aNotifyType*/,  CConnectionProviderBase* /*aConnProvider*/, TInt /*aError*/, const CConNotificationEvent* /*aConNotificationEvent*/)
	{
	}

void CNifManSubConnectionShim::AttachToNext(CSubConnectionProviderBase* /*aSubConnProvider*/)
	{
	}

TInt CNifManSubConnectionShim::ProgressNotification(TInt aStage, TInt aError, const TDesC8& aInfo)
/**
Upcall from connection provider via CInterface with notification of new progress stage reached

@param aStage The progress stage that the subconnection has reached
@param aError Any errors that were encountered at this stage
@param aInfo No idea what this is, it's inserted by CInterface and is currently null
@return KErrNone, or one of the system-wide error codes
*/
	{
	__CFLOG_VAR((KShimScprTag, KShimScprSubTag, _L8("CNifManSubConnectionShim %08x:\tProgressNotification(%d, %d) SubConnId: %d"), 
				this, aStage, aError, iSubConnectionsUniqueId));

	TInt max = iShimClients.Count();
	for (TInt i = max - 1 ; i >= 0 ; i--)
		{
		iShimClients[i]->ProgressNotification(aStage, aError, aInfo);
		}

	return KErrNone;
	}

	
TInt CNifManSubConnectionShim::NotifyDataTransferred(TUint aUplinkVolume, TUint aDownlinkVolume)
	{
	return  iConnDataTransferShim ? iConnDataTransferShim->NotifyDataTransferred(aUplinkVolume, aDownlinkVolume) : KErrNone;

	}
	
TInt CNifManSubConnectionShim::NotifyDataSent(TUint aUplinkVolume, TUint /*aCurrentGranularity*/)
	{
	return  iConnDataTransferShim ? iConnDataTransferShim->NotifyDataSent(aUplinkVolume) : KErrNone;

	}
	
TInt CNifManSubConnectionShim::NotifyDataReceived(TUint aDownlinkVolume, TUint /*aCurrentGranularity*/)
	{
	return  iConnDataTransferShim ? iConnDataTransferShim->NotifyDataReceived(aDownlinkVolume) : KErrNone;

	}
	
TSubConnectionUniqueId CNifManSubConnectionShim::Id()
	/**
	Access this subconnections unique id for search purposes
	*/
	{
	return iSubConnectionsUniqueId;
	}
	
void CNifManSubConnectionShim::SetSubConnectionUniqueId( TSubConnectionUniqueId aSubConnectionUniqueId )
	{
	iSubConnectionsUniqueId = aSubConnectionUniqueId;
	}
	
//void CNifManSubConnectionShim::DataClientJoiningL(MSubConnectionDataClient& aDataClient)
//	{
//	aDataClient.JoinComplete(*this);
//	}

//void CNifManSubConnectionShim::DataClientLeaving(MSubConnectionDataClient& aDataClient)
//	{
//	aDataClient.LeaveComplete(*this);
//	}

//void CNifManSubConnectionShim::DoSourceAddressUpdate(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aSource*/)
//	{//do nothing
//	}

//void CNifManSubConnectionShim::DoDestinationAddressUpdate(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aDestination*/)
//	{//do nothing
//	}

//void CNifManSubConnectionShim::DoDataClientRouted(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aSource*/, const TSockAddr& /*aDestination*/, const TDesC8& /*aConnectionInfo*/)
//	{//do nothing
//	}

//void CNifManSubConnectionShim::DoParametersAboutToBeSetL(CSubConParameterBundle& /*aParameterBundle*/)
//	{//do nothing
//	}

//TInt CNifManSubConnectionShim::DoControl(TUint /*aOptionLevel*/, TUint /*aOptionName*/, TDes8& /*aOption*/)
//	{//do nothing
//	return KErrNotSupported;	
//	}
	
//void CNifManSubConnectionShim::DoStartL()
//	{
//	User::Leave(KErrNotSupported);
//	}

//void CNifManSubConnectionShim::DoStop()
//	{
//	}

//CSubConnectionProviderBase* CNifManSubConnectionShim::DoNextLayer()
//	{
//	return NULL;
//	}

CConnDataTransfer& CNifManSubConnectionShim::CreateDataTransferL()
	{
	if (!iConnDataTransferShim)
		{
		iConnDataTransferShim = new (ELeave)CConnDataTransferShim(*this);
		}
	return *iConnDataTransferShim;
	}

//MConnectionDataClient* CNifManSubConnectionShim::DoSelfConnectionDataClient()
//	{
//	return this;
//	}

