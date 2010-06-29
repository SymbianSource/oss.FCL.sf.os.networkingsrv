
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
// Implementation file for the CConnectionSelectorShim
// 
//

/**
 @file
 @internalComponent
*/


#include <nifman.h>
#include <ss_glob.h>
#ifndef SYMBIAN_NETWORKING_UMTSR5
#include <comms-infras/nifif.h>
#endif //SYMBIAN_NETWORKING_UMTSR5
#include <connpref.h>
#include <cdblen.h>
#include <esockmessages.h>
#include "shimcprfactory.h"
#include "connectionSelectorShim.h"
#include "shimcpr.h"

using namespace ESock;

//PREQ399_REMOVE
#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
#include <nullagtprog.h>
#endif
//SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE

TInt CConnectionSelectorShim::AsyncDestructorCb(TAny* aInstance)
	{
	CConnectionSelectorShim* selector = reinterpret_cast<CConnectionSelectorShim*>(aInstance);
	delete selector;
	return KErrNone;
	}

CConnectionSelectorShim::CConnectionSelectorShim(ISelectionNotify& aNotify) : 
 iNotify(aNotify), 
 iAsyncDestructor(CActive::EPriorityStandard + 1),
 iConnProvider(NULL)
/** 
C'tor.

The priority of the async destructor is specifically set one higher than normal
to cater for the case whereby an RConnection is started asynchronously and
immediately closed.  This ensures that the async destructor is called before
NetCon begins the selection procedure, and hence can cancel the NetCon request
before it starts.  This causes immediate cancellation of the connection start
procedure.  Otherwise, NetCon gets in first, the connection comes all the way
and then times out in the short timer.

@param aNotify the ISelectionNotify that should be notified about the
selection progress.
*/
    { 
    __CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCConnectionSelectorShim"), this));   
    iAsyncDestructor.Set(TCallBack(AsyncDestructorCb, this));
    }        
    
CConnectionSelectorShim::~CConnectionSelectorShim()
    {
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\t~CConnectionSelectorShim"), this));

	iNotify.Detach(); //"PrevLayer"::Detach() will be called only once.

	delete iSelectorSession;

	iSelectionInfo.Close();
    }


void CConnectionSelectorShim::DeleteAsync()
	{
	if (!iAsyncDestructor.IsActive())
		{
		if (iConnProvider)
			{
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tDeleteAsync() RelaseRef(this)"), this));
			iConnProvider->ReleaseRef(this);
			}
		iAsyncDestructor.CallBack();
		}
    }

#ifdef SYMBIAN_NETWORKING_UMTSR5
void CConnectionSelectorShim::SetFactoryIface(MIAPLockInfo * aFactoryIface)
	{
	iFactoryIface = aFactoryIface;
	}

#endif // #ifdef SYMBIAN_NETWORKING_UMTSR5

TInt CConnectionSelectorShim::Cancel()
	{
	return Cancel(KErrCancel,NULL);
	}
	
TInt CConnectionSelectorShim::Cancel(TInt aReason, const RMessage2* aMessage)
	{
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCancel(aReason %d)"), this, aReason));
	TInt error = KErrNone;
	if (iSelectorSession)
		{
		error = iSelectorSession->Stop(aReason, aMessage);
		delete iSelectorSession;
		iSelectorSession = NULL;
		}
	
	//"PrevLayer"::Detach() will be called only once.
	//It is important to call iNotify.Detach() after all progress notifications
	//resulting from NifSession::Stop() have been passed up towards the CConnection.
	iNotify.Detach();
	DeleteAsync();
	return error;
	}
	
TInt CConnectionSelectorShim::Select(Meta::SMetaData& aPreferences, const RMessagePtr2* aMessage)
/** Implements CConnectorSelector::Select (async). The shim layer (this layer)
delegates the selection to NIFMAN. NIFMAN cannot be asked just to perform the
selection, as the selection is tighly coupled with starting connection provider.

@param aPreferences connection preferences.
*/
    {
    //Assume this is a TConnStart (as defined in ss_connprov.h)
    STypeId type = aPreferences.GetTypeId();
    ASSERT(type.iUid.iUid == KESockMessagesImplementationUid);
	ASSERT(type.iType == EESockMessageConnStart);

   	const TConnPref* connPrefs = NULL;
   	TConnStartType stype = EConnStartImplicit;
   	
   	if (type.iType == EESockMessageConnStart)
 		{
 		connPrefs = reinterpret_cast<CConnStart&>(aPreferences).ConnPrefs();
 		stype = reinterpret_cast<CConnStart&>(aPreferences).StartType();
	    if (connPrefs && connPrefs->ExtensionId() == TConnPref::EConnPrefSnap)
	    	{
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tSelect() with not supported preferences EConnPrefSnap"), this));
	    	iNotify.SelectComplete(NULL, KErrNotSupported);
	   	    DeleteAsync();
	   	    return KErrNotSupported;
	    	}
 		}

#ifdef SYMBIAN_NETWORKING_UMTSR5 
     	
		// The code here will check the secure ID of the application which is expected with the aMessage Ptr.
		// The logic used here is that if a General purpose application locks an IAP then no other application
		// should be allowed to start the connection ans should return with KErrAccessDenied.
	    if(aMessage!=NULL)
	       {
	       iAppSecureId = aMessage->SecureId();
	       }
 #endif   //SYMBIAN_NETWORKING_UMTSR5 
    
    TRAPD(ret,
	iSelectorSession = Nif::NewConnectionL(this, 0);
    if (connPrefs)
    	{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tSelect() with preferences, iSelectorSession %x"), this, iSelectorSession));
	    iSelectorSession->StartL(stype, *connPrefs, aMessage );		
    	}
    else
    	{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tSelect() no preferences, iSelectorSession %x"), this, iSelectorSession));
	    iSelectorSession->StartL(stype, aMessage );		
    	}
    );
    if (ret != KErrNone)
    	{
    	iNotify.SelectComplete(NULL, ret);
   	    DeleteAsync();
    	}
    return ret;
    }
    

void CConnectionSelectorShim::SelectionComplete(TInt aError, const TDesC8& aSelectionInfo)
/** Called by NIFMAN when the selection (previously triggered by CConnectionSelectorShim::SelectL)
completes the selection. There are two methods NIFMAN will use to indicate progress to its client. This
method and the generic ::ProgressNotification. This method forwards selection complete to
this->ProgressNotification(KFinishedSelection).

@param aError operation result.
@param aSelectionInfo selection info (to be converted to TSoIfConnectionInfo {iIAPId; iNetworkId;})
*/
    {
    __CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tSelectionComplete(), aError: %d"), this, aError));
    iSelectionInfo.Close();
    TInt err;
    if((err = iSelectionInfo.Create(aSelectionInfo)) != KErrNone)
    	{
    	aError = err;
    	}
#ifdef SYMBIAN_NETWORKING_UMTSR5
    // The following line will extract connection information which is sent by nifman after agent selection.The information contains IAP , iNetworkId
	//and connection information.

    Mem::Copy(&iConnectionInfo, aSelectionInfo.Ptr(),sizeof(iConnectionInfo));
#endif
    ProgressNotification(KFinishedSelection, aError);
    }

void CConnectionSelectorShim::CreateProviderL()
	{
	ASSERT(iSelectionInfo.Length());//this should only happen after the agent's been selected
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() iConnProvider %x"), this, iConnProvider));
 	if (iConnProvider) 
 		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() iConnProvider->GetNifSession() %x"), this, iConnProvider->GetNifSession()));	
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() iConnProvider->ConnectionInfo() %S"), this, &iConnProvider->ConnectionInfo()));
 		}
	//if we have a provider already and it's the existing one we have to check whether the conn info still matches since there could've been
	//a reconnection
	if ( iConnProvider && iConnProvider->GetNifSession() && iConnProvider->ConnectionInfo() != iSelectionInfo )
		{
        __CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() -> ReleaseRef()"), this));
	    iConnProvider->ReleaseRef(this);
	    iConnProvider = NULL;
		}
	if ( !iConnProvider )
		{
	    TSockManData* sockManData = SockManGlobals::Get();
		ASSERT(sockManData);
		
		CConnectionFactoryContainer* connectionFactories = sockManData->iConnectionFactories;
		ASSERT(connectionFactories);
		
		CConnectionProviderFactoryBase* factory = connectionFactories->FindFactory(KShimConnectionProviderFactoryId);
		ASSERT(factory);
		if (IsConnectionStoppingL())
			{
			// We must force the creation of a new provider because the old one is on its way out.
			// This is required to ensure that we do not receive progresses generated as the connection
			// comes down.  We are only interested in progresses generated when the connection subsequently
			// starts coming up again.  Upper layers starting a connection just as it is coming down will
			// otherwise receive these progresses.  In particular, progresses with an error will otherwise
			// be mistaken for an indication that the connection startup failed.
	    		iConnProvider = (CConnectionProviderShim*)factory->CreateProviderL();
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() - connection stopping, iConnProvider %x"), this, iConnProvider));
			}
		else
			{
			XConnectionFactoryQueryInfo query(NULL, iSelectionInfo);
			iConnProvider = (CConnectionProviderShim*)factory->FindOrCreateProviderL(query);
			__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tCreateProviderL() - found/created iConnProvider %x"), this, iConnProvider));
			}
	    //and keep provider up during the selection
	    iConnProvider->AddRefL(this);
		}
	}

TBool CConnectionSelectorShim::IsConnectionStoppingL()
	{
	TPckg<TBool> stopping(EFalse);
	iSelectorSession->ControlL(KCOLProvider, KNifSessionGetStopping, stopping, NULL);
	return stopping();
	}

void CConnectionSelectorShim::HandleSelectionL()
	{
	// HandleSelection will set up the provider

	// Set default factory as the CConnectionProviderFactoryShim
	CreateProviderL();
	if (iSelectorSession && !iConnProvider->GetNifSession())
		{			
		iConnProvider->SetConnectionInfo(iSelectionInfo);
		//
		// The factory didn't find an existing provider that matched the query.
		// It's returned a brand new instance of CConnectionProviderShim.
		// Hence we'll call Initialise. this will do 2 things:
		//
		// 1. Create a secure nif(man) session for the provider
		// 2. Call ConnectionControlActivity for the provider so that
		//			
		iConnProvider->InitialiseL(iSelectorSession);
		iConnProvider->SetBlockProgressesL(iSelectorSession);
		iSetProviderNull = ETrue;
		}
	}


#ifdef SYMBIAN_NETWORKING_UMTSR5
void CConnectionSelectorShim::MaybeLockIapL()
	{
	//The value of Secure ID is fetched from DataBase againt IAP.Iap is taken agent selection is over.This will look 
	//into the database if secureID feild is NULL or not if it is NULL then IAP will not be locked
	CMDBSession* cmdbSession;
	CCDIAPRecord* ptrIapRecord;
	ptrIapRecord = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	CleanupStack::PushL(ptrIapRecord);
	
	// Create a new CMDB session Object
	cmdbSession = CMDBSession::NewL(KCDVersion1_1);
	// If successm Get and Load the record.
	CleanupStack::PushL(cmdbSession);

	ptrIapRecord->SetRecordId(iConnectionInfo.iIAPId);
	ptrIapRecord->LoadL(*cmdbSession);
	// Although the SecureID of the application is numerical, we have to take it into the string buffer
	// because of the limitations of the CommsDat, which dont support hexadeciaml values, and truncate 
	// decimal values after 8 digits, for some unknown reasons.
	// Because of the current project schedule, its not feasible to rectify commsdat as of now, so decided
	// to take on string Buffers to be used for Secure Ids from commsdat 
	TUint32 secureId;
	secureId=ptrIapRecord->iAppSid;
 	
	if(secureId==iAppSecureId.iId && iAppSecureId.iId!=NULL)
		{
		// Inform the Factory that the Ids match so IAP will be locked. The IAP number is also given to 
		// the factory so that the application can start connection on other IAPs which are not locked
		// by the program
		iFactoryIface->SetIAPLockStatus(ETrue,iConnectionInfo.iIAPId);
		}
	    
        CleanupStack::Pop(cmdbSession);
	delete cmdbSession;
	CleanupStack::Pop(ptrIapRecord);
	delete ptrIapRecord;
	       
	}
#endif

void CConnectionSelectorShim::ProgressNotification(TInt aStage, TInt aError)
/** 

@param 
*/  
    {
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tProgressNotification(%d, %d)"), 
					 this, aStage, aError));
	
	// In all cases we must pass the new progress up the chain of selectors
	if (aError == KErrNone)
		{
		aError = iError;
		}
	//	
	// Depending on the progress and the progress error, we need to handle situations differently
	// 
	// Normally, if progress has reached KFinishedSelection, we stop propogating the progress
	// up, because, we would have already setup the provide (iProvider) with a new nif session
	// pointing to the same agent as iSelectorSession, hence further messages from agent and
	// nifman will be forwared to the provider.
	//
	// However, there are two exceptions to this situation:
	// We have a provider that's already been initialized with a nif session
	// signified by !iSetProviderNull, in which case, we forward the message up 
	// whether or not we have finished selection. Same goes for situations when 
	// we have progress error, instead of waiting for the provider to propogate the 
	// message upwards, we use iNotify to do that, since the client would expect the
	// message (e.g. Stop) to be completed with error immidiately
	// 

	// Any error means that the selection (and startup of the interface) will be abandoned and reported to the client.
	if (aError != KErrNone)
		{
		iNotify.ProgressNotification(aStage, aError);
		iNotify.SelectComplete(NULL, aError);
		DeleteAsync();
		}
	else if (aStage <= KFinishedSelection || !iSetProviderNull)
		{
		iNotify.ProgressNotification(aStage, aError);
		}

	if (aStage == KFinishedSelection && aError == KErrNone)
		{
		// The selection is complete so we want to reach the situation where the selector (this object) is 
		// joined by the appropriate provider. The provider will be set up with a pointer to the CNifSession 
		// allowing the calls to methods such as EnumerateSubConnections to work between KFinishedSelection and
		// KLinkLayerOpen. The provider will be accessible from the selector until KLinkLayerOpen is reached
		// (when the selector will be destroyed).
		TRAP(aError,HandleSelectionL());
		if (aError == KErrNone)
			{
			iNotify.SelectComplete(iConnProvider, aError);
			}
#ifdef SYMBIAN_NETWORKING_UMTSR5

		// First Check IAP Locked, if yes Return,
		TBool  IapLocked = EFalse;
		TInt	IapNumber = -1;
	
		iFactoryIface->GetIAPLockStatus(IapLocked, IapNumber);
	
		if (IapLocked && IapNumber == iConnectionInfo.iIAPId)
			{
			//iNotify.ProgressNotification(aStage, KErrPermissionDenied);
			iNotify.SelectComplete(iConnProvider, KErrPermissionDenied);
			DeleteAsync();	
			return;
			}
#endif //SYMBIAN_NETWORKING_UMTSR5  

		}
	else if (aStage == KConnectionUninitialised)
		{
		//KConnectionUninitialised means that the interface has been stopped.
		DeleteAsync();
		}
	else if (aStage == KLinkLayerOpen && !iIsLinkLayerOpen)
		{
		// Calling iNotify.LayerUp when we have a provider setup with a nif session already
		// follows from the logic above justifying propogating progress up when we have a 
		// provider setup with a nif session
		iNotify.LayerUp(aError);
		DeleteAsync(); // The interface has successfully started and all responsibility should be taken over by the the provider.
		}

#ifdef SYMBIAN_NETWORKING_3GPPDEFAULTQOS
    else if ( aStage == ENullAgtConnecting && aError == KErrNone )
    	{
#ifdef SYMBIAN_NETWORKING_UMTSR5

	TRAPD(ret,MaybeLockIapL());
#ifdef __CFLOG_ACTIVE	
	if(ret!=KErrNone)
		{
		__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tMaybeLockIap left with  %d"), 
					 this, ret));
		}
#endif
	(void)ret; //TRAP is safe to ignore becuase if MaybeLockIapL: leave the correct behaviour is to leave Iap unlocked.
	//We can safely ignore the trap because the function will only lock IAP. One should not be effected if someth
#endif //SYMBIAN_NETWORKING_UMTSR5  
    	}
    	
#endif //SYMBIAN_NETWORKING_3GPPDEFAULTQOS //PREQ399_REMOVE
    }


void CConnectionSelectorShim::SubConnectionEvent(const TSubConnectionEvent& /*aSubConnectionEvent*/)
/** 

@param 
*/
   {		
   }

void CConnectionSelectorShim::ServiceChangeNotification(TUint32 aId, const TDesC& aType)
/** 

@param 
*/
   {
   iNotify.ServiceChangeNotification(aId, aType);
   }

void CConnectionSelectorShim::LinkLayerOpen(TInt aError)
/** 

@param 
*/
	{
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tLinkLayerOpen(aError %d)"), 
					 this, aError));
	iIsLinkLayerOpen = ETrue;
	if (aError == KErrNone)
		{
		if (iSetProviderNull)
			{
			ASSERT(iConnProvider->GetNifSession());
			iConnProvider->ReleaseRef(this);
			iConnProvider = NULL;
			iSetProviderNull = EFalse;
			}		
		
		DeleteAsync();
		}
	else
		{
		iNotify.SelectComplete(NULL, aError);
		}
    }
    
void CConnectionSelectorShim::SetProviderNull()
	{
	__CFLOG_VAR((KShimCprTag, KShimCprSubTag, _L8("CConnectionSelectorShim %08x:\tSetProviderNull() iConnProvider %x"), this, iConnProvider));
	iConnProvider = NULL;
	}

void CConnectionSelectorShim::ConnectionError(TInt /*aError*/)
/** 

@param 
*/
    {
    //ignore the event
    }


void CConnectionSelectorShim::LinkLayerClosed(TInt /*aError*/)
/** 

@param 
*/
    {
    //ignore the event
    }



void CConnectionSelectorShim::ProgressNotification(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TInt /*aStage*/, TInt /*aError*/)
/** 

@param 
*/
    {
    }

void CConnectionSelectorShim::InterfaceStateChangeNotification(TDesC8& /*aInfo*/)
/** 

@param 
*/
    {
    //ignore the event
    }


void CConnectionSelectorShim::NotifyDataSent(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aUplinkVolume*/)
/** 

@param 
*/
    {
    //ignore the event
    }


void CConnectionSelectorShim::NotifyDataReceived(TSubConnectionUniqueId /*aSubConnectionUniqueId*/, TUint /*aDownlinkVolume*/)
/** 

@param 
*/
    {
    //ignore the event
    }

