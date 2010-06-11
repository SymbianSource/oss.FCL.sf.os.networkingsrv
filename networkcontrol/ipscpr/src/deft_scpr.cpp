// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation file for the IP SubConnection Provider
// 
//

/**
 @file
*/

#include <e32std.h>
#include <e32test.h>
#include <ss_glob.h>
#include "deft_scpr.h"
#include "ipscprlog.h"
#ifdef SYMBIAN_NETWORKING_UMTSR5
#include "MAppIdInfo.h"
#include <networking/qos3gpp_subconparams.h>
#include <ip_subconparams.h>
#endif //SYMBIAN_NETWORKING_UMTSR5

#ifdef SYMBIAN_NETWORKING_UMTSR5
	const TUint32 KSIPSecureId  = 270490934;
	const TUint32 KDHCPSecureId = 270522821;
	const TUint32 KDNSSecureId  = 268437634;
#endif //SYMBIAN_NETWORKING_UMTSR5

CEmptySubConnectionProvider::~CEmptySubConnectionProvider()
	{
	__IPCPRLOG(IpCprLog::Printf(_L("~CEmptySubConnectionProvider [this=%08x]"), this));
	if (iConnectionProvider)
		{
		iConnectionProvider->Leave(*this);
		}
	}

void CEmptySubConnectionProvider::DoControlClientJoiningL(MSubConnectionControlClient& aControlClient)
	{
	(void)aControlClient;
	__IPCPRLOG(IpCprLog::Printf(_L("CEmptySubConnectionProvider [this=%08x]:\tDoControlClientJoiningL() [iControlClients.Count=%d] [aControlClient=%08x]"), this, iControlClients.Count(), &aControlClient));
	}
	
void CEmptySubConnectionProvider::DoControlClientLeaving(MSubConnectionControlClient& aControlClient)
	{
	(void)aControlClient;
	__IPCPRLOG(IpCprLog::Printf(_L("CEmptySubConnectionProvider [this=%08x]:\tDoControlClientLeaving() [iControlClients.Count=%d] [aControlClient=%08x]"), this, iControlClients.Count(), &aControlClient));
	}

void CEmptySubConnectionProvider::EnumerateClientsL(TUint& aCount, TDes8& aDes, CConnectionProviderBase::TEnumClients aClientType)
	{
	STypeId tid = STypeId::CreateSTypeId(KConnectionClientExtUid,EConnectionClientDesc);
	TInt max = iControlClients.Count();
	for ( TInt n = 0; n < max; n++ )
		{
		MConnectionClientDesc* intf = reinterpret_cast<MConnectionClientDesc*>(iControlClients[n]->FetchInterfaceInstanceL(*this,tid));
		if ( intf )
			{
			TConnectionProcessInfo cinfo;
			cinfo.GetInfoL(aClientType,aCount, *intf, aDes);
			}
		}
	STypeId tid2 = STypeId::CreateSTypeId(KConnectionClientExtUid,EConnectionEnumerateClients);
	max = iDataClients.Count();
	for ( TInt n = 0; n < max; n++ )
		{
		MConnectionEnumerateClients* intf = reinterpret_cast<MConnectionEnumerateClients*>(iDataClients[n]->FetchInterfaceInstanceL(*this,tid2));
		if ( intf )
			{
			intf->EnumerateClientsL(aCount,aDes,aClientType);
			}
		}
	}

void CEmptySubConnectionProvider::ConnectionError(TInt /*aStage*/, TInt aError)
	{//it's comming from connection at the same level so forward it sideways
	//with an origin EConnection
	TInt max = iDataClients.Count();
	for ( TInt n = max - 1; n >= 0; n-- )
		{
		iDataClients[n]->SubConnectionError(*this, MSubConnectionDataClient::EConnection, aError);
		}
	}

MConnectionDataClient* CEmptySubConnectionProvider::DoSelfConnectionDataClient()
	{
	return this;
	}


// Methods to be overriden be derived subconnection provider
void CDefaultSubConnectionProvider::DoControlClientJoiningL(MSubConnectionControlClient& aControlClient)
	{
	(void)aControlClient;
   __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider::DoControlClientJoiningL [this=%08x]"), this));
   if (NULL != NextLayer())
      {
      __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP exists - joined with provider")));
      }
   else
      {
      __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP doesn't exists - not joined")));
      }
	
	}
	
void CDefaultSubConnectionProvider::DoDataClientJoiningL(MSubConnectionDataClient& aDataClient)
	{
#ifdef SYMBIAN_NETWORKING_UMTSR5
	// This piece of code is being added as per the requirements of PREQ 635 of dedicated PDP signalling 
	// context. When Primary PDP Context is created Network will return back the code saying whether it 
	// accepts the request to be dedicated signalling context or not. If Network decided to allow for dedicated
	// signalling context then UmtsGprs SCPR add the information into the iParameterBundle of the UmtsGprsScpr 
	
	// However, if the User doesnt do RSubConnection::SetParamter(EAttachDefault), which actually the user shouldn't do
	// then we have to Get the parameter from the Next Layer, which again might or might not be UmtsGprs SubConnection Provider.
	// Nevertheless if we call SetParameter() on iNextLayer then SetParameters across all the Layers below will get called, and 
	// since I knew that if any of the layer below is UmtsGprsSCpr, it will update the IMCN Signalling flag in the 
	// parameter Bundle which I will pass with the SetParameter()
	
	// We Initialize this variable everytime so that it can it can point to proper next layer 
	// everytime. Just a safety measure because it will anyway be initialised by the code written 
	// below
	iSubConNextLayer = NULL;
	if (NULL == iNextLayer)
		{
		// Find the Next Layer , otherwise iSubConNextLayer will be NULL by default.
		CConnectionProviderBase* lowerConnectionProvider = iConnectionProvider->NextLayer();
   		if (lowerConnectionProvider)
       		{
			TUint nextLayerFactoryId = lowerConnectionProvider->CanDoSubConnection(RSubConnection::EAttachToDefault);
			if (nextLayerFactoryId != 0)
	   			{	
           		TSockManData* sockManData = SockManGlobals::Get();
           		CSubConnectionFactoryContainer* subConnectionFactories = sockManData->iSubConnectionFactories;
   	            XSubConnectionFactoryQuery query(lowerConnectionProvider, RSubConnection::EAttachToDefault);
       		    iSubConNextLayer = subConnectionFactories->FindOrCreateProviderL(nextLayerFactoryId, query);
	   			}	
			        	
       		}
   		}
	else
		{
		// We have a NextLayer, Point iSubConNextLayer to the same
		iSubConNextLayer = iNextLayer;
		}

	// Get The parameter Bundle for this SubConnectionProvider Instance. if Not available create one
	// and Get the parameters from the Next Lyer
	CSubConParameterBundle *tempBundle = NULL;
	if (iSubConNextLayer != NULL)
		{
		tempBundle = iParameterBundle != NULL? iParameterBundle :CSubConParameterBundle::NewL();
		TRAP_IGNORE(iSubConNextLayer->SetParametersL(*tempBundle));
		}
	
	// Get the family if Available
	CSubConParameterFamily *imcnFamily = tempBundle != NULL ? tempBundle->FindFamily(KSubConnContextDescrParamsFamily) : NULL;
	if (imcnFamily)
		{
		// Find the family, Look for the IMCN value, using CSubConImsExtParamSet defined in Qos3gpp
		CSubConImsExtParamSet *imcnSigParams = static_cast<CSubConImsExtParamSet*>
		(imcnFamily->FindExtensionSet(STypeId::CreateSTypeId(KSubConIPParamsUid,KSubConImsExtParamsType), 
		CSubConParameterFamily::EGranted));
		// If Family contains IMCN Signalling Parameters
		if (imcnSigParams&& imcnSigParams->GetImsSignallingIndicator())	
			{
    		// Check and Delete
			if (tempBundle != iParameterBundle)
				{
				tempBundle->Close();
				tempBundle=NULL;
				}
			// Fetch the interface from the connection provider
			STypeId typeID = STypeId::CreateSTypeId(KConnectionAppInfoInterfaceId,0); // IP Conenction provider factory Uid
			TAny* intf = iConnectionProvider->FetchInterfaceInstanceL(typeID);
			if (!intf)
				{
				// We are not able to get the AppSId so leaving
				User::Leave(KErrNotSupported);
				}
			MConnectionAppIdInfo  *appIdIP = static_cast<MConnectionAppIdInfo*>(intf);
			TUint32 appSecureId = appIdIP->GetAppSecureId();
			
			// if socket is being opened by any application other than SIP, DHCP, and DNS we need to
			// restrict the socket from being Created.
		
			
			if ( ( appSecureId==KSIPSecureId || iAppId == KSIPSecureId)  || 
			     (appSecureId==KDHCPSecureId || iAppId == KDHCPSecureId)  || 
			     (appSecureId==KDNSSecureId || iAppId == KDNSSecureId) )
				{
				aDataClient.JoinComplete(*this);
				}
		    else
	            {
	            User::Leave(KErrPermissionDenied);
	            }	     
			} // if (imcnSigParams&& imcnSigParams->GetImsSignallingIndicator())	
		else
		    {
		    if (iConnectionProvider->IsLayerUp())
		        {
		         aDataClient.JoinComplete(*this);
		        }

	     	}
		} // if (imcnFamily)
		// Check and Delete
		if (tempBundle != iParameterBundle && tempBundle!=NULL)
		   {
		    tempBundle->Close();
	       }
#else 
			
		if (iConnectionProvider->IsLayerUp())
		  {
		   aDataClient.JoinComplete(*this);
		  }
#endif 	// #ifdef SYMBIAN_NETWORKING_UMTSR5

	}

void CDefaultSubConnectionProvider::DoDataClientLeaving(MSubConnectionDataClient& aDataClient)
	{
	aDataClient.LeaveComplete(*this);
	}
	
void CDefaultSubConnectionProvider::DoSourceAddressUpdate(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aSource*/)
	{
	}
	
void CDefaultSubConnectionProvider::DoDestinationAddressUpdate(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aDestination*/)
	{
	}
	
void CDefaultSubConnectionProvider::DoDataClientRouted(MSubConnectionDataClient& /*aDataClient*/, const TSockAddr& /*aSource*/, const TSockAddr& /*aDestination*/, const TDesC8& /*aConnectionInfo*/)
	{
	}
	
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
void CDefaultSubConnectionProvider::DoParametersAboutToBeSetL(CSubConParameterBundle& aParameterBundle)
	{
   __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider::DoParametersAboutToBeSetL [this=%08x]"), this));
   if (NULL != NextLayer())
      {
      __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP exists - forwarding the request")));
      iNextLayer->SetParametersL(aParameterBundle);
      }
   else
      {
      __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP doesn't exists - the request not supported")));
      User::Leave(KErrNotSupported);        
      }
   }

#else
void CDefaultSubConnectionProvider::DoParametersAboutToBeSetL(CSubConParameterBundle& /*)aParameterBundle*/)
	{//this could potentially fetch a current parameters from GuQoS and return them back as granted ones	
	User::Leave(KErrNotSupported);
	}
#endif
	
#ifdef SYMBIAN_NETWORKING_UMTSR5	
TInt CDefaultSubConnectionProvider::DoControl(TUint aOptionLevel, TUint /*aOptionName*/, TDes8& /*aOption*/)
#else
TInt CDefaultSubConnectionProvider::DoControl(TUint /*aOptionLevel*/, TUint /*aOptionName*/, TDes8& /*aOption*/)
#endif //#ifdef SYMBIAN_NETWORKING_UMTSR5
	{
#ifdef SYMBIAN_NETWORKING_UMTSR5
//This control is used to send application secure ID of Active Connection. This Id 
//will be used to for adding socket as data client to the subconnection.
	iAppId=aOptionLevel;
	return KErrNone;
#else
	return KErrNotSupported;
#endif //#ifdef SYMBIAN_NETWORKING_UMTSR5
	}
	
void CDefaultSubConnectionProvider::DoStartL()
	{
   __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider::DoStartL [this=%08x]"), this));
	}
	
void CDefaultSubConnectionProvider::DoStop()
	{
   __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider::DoStop [this=%08x]"), this));
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
   if (iNextLayer)
      {
      __IPCPRLOG(IpCprLog::Printf(_L("Leaving Lower subconnection provider")));
      iNextLayer->Leave(*this);
      iNextLayer = NULL;
      }
#endif
	}
	
CSubConnectionProviderBase* CDefaultSubConnectionProvider::DoNextLayer()
	{
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
   __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tDoNextLayer()"), this));
   if (NULL == iNextLayer)
      {
      CConnectionProviderBase* lowerConnectionProvider = iConnectionProvider->NextLayer();
      if (!lowerConnectionProvider)
         {
         //This could denote the connection isn't started and perhaps should be from here,
         // but since the selection isn't separated from startup, we don't have enough
         // information to do it here.
         __IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tDoNextLayer() Connection Provider is missing its NextLayer"), this));
         return NULL;   // No Lower layer,so leaving rhostresolver without connection.
         }
		                                 
		TUint nextLayerFactoryId = lowerConnectionProvider->CanDoSubConnection(RSubConnection::EAttachToDefault);
      TRAP_IGNORE(
      
   		if (nextLayerFactoryId != 0)
   		   {
            //'This' not started yet. The lower layer unknown
            //This is as much as we can delay with resolving the subconnection stack
            TSockManData* sockManData = SockManGlobals::Get();
            CSubConnectionFactoryContainer* subConnectionFactories = sockManData->iSubConnectionFactories;
      
            XSubConnectionFactoryQuery query(lowerConnectionProvider, RSubConnection::EAttachToDefault);
               iNextLayer = subConnectionFactories->FindOrCreateProviderL(nextLayerFactoryId, query);
   		   }
   		
         if (iNextLayer)
            {
            __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP found.. Joining")));
            iNextLayer->JoinL(*this);
            }
         else
            {
            __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP not found.. Continuing")));
            }
         );
         
      }
      
   return iNextLayer;

#else
   __IPCPRLOG(IpCprLog::Printf(_L("Lower subconnection provider for IP not supported")));
	return NULL;
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS
	}
	
CConnDataTransfer& CDefaultSubConnectionProvider::DoDataTransferL()
	{
	User::Leave(KErrNotSupported);
	//unreachable code
	return iNextLayer->DataTransferL();
 	}
	
//MConnectionDataClient
TAny* CDefaultSubConnectionProvider::FetchInterfaceInstanceL(CConnectionProviderBase& /*aProvider*/, const STypeId& aTid)
	{
	return (aTid == STypeId::CreateSTypeId(KConnectionClientExtUid,EConnectionEnumerateClients)) ? static_cast<MConnectionEnumerateClients*>(this) : NULL;
	}
	
void CDefaultSubConnectionProvider::ConnectionGoingDown(CConnectionProviderBase& /*aConnProvider*/)
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tConnectionGoingDown()"), this));
	iConnectionProvider = NULL;
	DeleteMeNow();
	}
	
void CDefaultSubConnectionProvider::Notify(MConnectionDataClient::TNotify aNotifyType, CConnectionProviderBase* /*aConnProvider*/, TInt aError, const CConNotificationEvent* /*aConNotificationEvent*/)
	{
	int count = iControlClients.Count();	
	for (int i = count - 1; i >= 0; --i)
		{		
		iControlClients[i]->SubConnectionEvent(*this, aNotifyType, aError, NULL);
		}
	if (aNotifyType == ENotifyLayerUp)
		{//complete outstanding data client joins
		TInt max = iDataClients.Count();
		for ( TInt n = max - 1; n >= 0; n-- )
			{
			if (aError == KErrNone)
				{
				iDataClients[n]->JoinComplete(*this);
				}
			else
				{
				iDataClients[n]->JoinFailed(*this,aError);
				}
			}
		}
	}
	
void CDefaultSubConnectionProvider::AttachToNext(CSubConnectionProviderBase* /*aSubConnProvider*/)
	{
	}

CDefaultSubConnectionProvider::~CDefaultSubConnectionProvider ()
   {
	__IPCPRLOG(IpCprLog::Printf(_L("~CDefaultSubConnectionProvider [this=%08x]"), this));
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
   if (iNextLayer) 
      {
      iNextLayer->Leave (*this);
      }
#endif	
   }
	
	
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
void CDefaultSubConnectionProvider::DoControlClientLeaving(MSubConnectionControlClient& aControlClient)
   {
   (void)aControlClient;
	__IPCPRLOG(IpCprLog::Printf(_L("CDefaultSubConnectionProvider [this=%08x]:\tDoControlClientLeaving() [iControlClients.Count=%d] [aControlClient=%08x]"), this, iControlClients.Count(), &aControlClient));

    // Note: control client count == 1 because the the client has not been removed yet
    if (iNextLayer && iControlClients.Count() == 1 && iDataClients.Count() == 0) 
      {
      iNextLayer->Leave (*this);
      iNextLayer = NULL;
      }
   }

void CDefaultSubConnectionProvider::SubConnectionEvent(CSubConnectionProviderBase& /*aSubConnProvider*/, MConnectionDataClient::TNotify /*aNotifyType*/, TInt /*aError*/, const CSubConNotificationEvent* aEvent)
    {
    NotifyClientEvent(*aEvent);
    }	
    
    
void CDefaultSubConnectionProvider::SubConnectionGoingDown(CSubConnectionProviderBase& /*aSubConnProvider*/)
    {
	TInt max = iControlClients.Count();
	for ( TInt n = max - 1; n >= 0; n-- )
		{
		iControlClients[n]->SubConnectionGoingDown(*this);
		}
    }
    
    
void CDefaultSubConnectionProvider::LayerUp(CSubConnectionProviderBase& /*aSubConnProvider*/, TInt /*aError*/)
   {
   }
   
void CDefaultSubConnectionProvider::IncomingConnection(CSubConnectionProviderBase* /*aSubConnProvider*/, CSubConParameterBundle* /*aParameterBundle*/, TInt /*aError*/)
   {
   }
    
#endif
// SYMBIAN_NETWORKING_3GPPDEFAULTQOS

