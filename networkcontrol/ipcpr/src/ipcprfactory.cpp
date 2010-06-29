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
// This is part of an ECOM plug-in
// 
//

#include <implementationproxy.h>
#include "ipcprfactory.h"	// CIPNetworkProviderFactory
#include "ipcprprovider.h"
#include <es_sock.h>        // KCommsNetworkLayerId
#include <ss_glob.h>
#include <shimcprfactory.h>
#include <esockmessages.h>
#include <commdbconnpref.h> // TConnPref
#include <commsdattypesv1_1.h> // CommsDat
#include <es_connpref.h>
#include <in_sock.h> //KAfInet

using namespace CommsDat;
using namespace ESock;

const TInt KIPConnectionProviderImplementationUid=0x102070EF;

/**
Data required for instantiating ECOM Plugin
*/
const TImplementationProxy ImplementationTable[] = 
	{
	IMPLEMENTATION_PROXY_ENTRY(KIPConnectionProviderImplementationUid, CIPNetworkProviderFactory::NewL)
	};

/**
ECOM Implementation Factory
*/
EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
    {
    aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
    return ImplementationTable;
   }

CIPNetworkProviderFactory* CIPNetworkProviderFactory::NewL(TAny* aParentContainer)
	{
 	return new (ELeave) CIPNetworkProviderFactory(KIPConnectionProviderFactoryId, *(reinterpret_cast<CConnectionFactoryContainer*>(aParentContainer)));
	}
   
CIPNetworkProviderFactory::CIPNetworkProviderFactory(TUint aFactoryId, CConnectionFactoryContainer& aParentContainer)
	: CConnectionProviderFactoryBase(aFactoryId,aParentContainer)
	{
	}
   
CConnectionProviderBase* CIPNetworkProviderFactory::DoCreateProviderL()
	{
    return CIPNetworkConnectionProvider::NewL(*this);
	}

MProviderSelector* CIPNetworkProviderFactory::DoSelectProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* aMessage )
	{
	//create self destructing object to select a provider
	CIPConnectionSelector* selector = new CIPConnectionSelector(aSelectionNotify,*this);
	TInt error;
	if (selector == 0)
		{
		error = KErrNoMemory;
		}
	else
		{
		error = selector->Select(aPreferences, aMessage);
		}

	if (error != KErrNone)
		{
		aSelectionNotify.SelectComplete(0, error);
		selector = NULL; //The selector will delete itself.
		}

 
	return selector;
  	}

MProviderSelector* CIPNetworkProviderFactory::DoSelectNextLayerProvider( Meta::SMetaData& aPreferences, ISelectionNotify& aSelectionNotify, const RMessagePtr2* /*aMessage*/ )
	{//at the moment always uses the CConnectionProviderFactoryShim::SelectProviderL
   (void)aPreferences;
   (void)aSelectionNotify;
   return NULL;
	}
	
void CIPNetworkProviderFactory::DoEnumerateConnectionsL(RPointerArray<TConnectionInfo>& aConnectionInfoPtrArray)
	{
	CConnectionFactoryContainer* connectionFactories = SockManGlobals::Get()->iConnectionFactories;
	ASSERT(connectionFactories);
	CConnectionProviderFactoryBase* factory = connectionFactories->FindFactory(KShimConnectionProviderFactoryId);
	ASSERT(factory);
	factory->EnumerateConnectionsL(aConnectionInfoPtrArray);
	}


//CIPConnectionSelector--
TInt CIPConnectionSelector::Cancel()
	{
	return Cancel(KErrCancel,NULL);
	}

TInt CIPConnectionSelector::Cancel(TInt aReason, const RMessage2* aMessage)
	{
	CActive::Cancel(); // There may be an outstanding selection request.

    //CIPConnectionSelector will be deleted from Detach().
    //Detach will always be called as a result of Cancel() in the same call stack,
    //but only after all progress notifications have been passed up towards the
    //CConnection.
    TInt ret = KErrNotReady;
    if(iNextLayerSelector !=NULL)
    	{
    	ret = iNextLayerSelector->Cancel(aReason, aMessage);
    	}

	iNotify.Detach(); //"PrevLayer"::Detach() will be called only once in the same call stack.
	return ret;
	}

TInt CIPConnectionSelector::Select(Meta::SMetaData& aPreferences, const RMessagePtr2* aMessage)
    {
	__FLOG_OPEN(KIpcprTag, KIpcprSubTag);
	__FLOG_1(_L8("CIPConnectionSelector::Select() %08x"), this);

	STypeId tId = STypeId::CreateSTypeId(aPreferences.GetTypeId());
    ASSERT(tId.iUid.iUid == KESockMessagesImplementationUid);
	ASSERT(tId.iType == EESockMessageConnStart);	

	if (aMessage)
		iSelectMessage = *aMessage; // aMessage will be passed on to shim
#ifdef SYMBIAN_NETWORKING_UMTSR5	
     //Here secure Id of application is stored and will be kept with  
    if(!iSelectMessage.IsNull())
     {
		iAppSecureId=iSelectMessage.SecureId();
     }
#endif // SYMBIAN_NETWORKING_UMTSR5	            
 
    TRAPD(r, SelectL(aPreferences));
	
	if (r!=KErrNone && iNextLayerSelector==NULL)
		{
		__FLOG_1(_L8("Error during selection of current  - should detech now %08x"), this);
 		Detach();
		return r;
 		}
 		
 	TRAP(r,SelectLinkLayerL());
 	if (r != KErrNone)
 		{
 		__FLOG_1(_L8("Error during select of link layer - detach should be called by the link layer %08x"), this);
 		}
	
	return r;
	}

void CIPConnectionSelector::SelectL(Meta::SMetaData& aPreferences)
	{
	ASSERT(iDbs==0);
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	iDbs = CMDBSession::NewL(KCDVersion1_2);
#else
	iDbs = CMDBSession::NewL(KCDVersion1_1);
#endif

	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	iDbs->SetAttributeMask( ECDHidden | ECDPrivate );
	
	ASSERT(iConnStart==0);
	iConnStart = CConnStart::NewL();
	iConnStart->Copy(aPreferences);

	// Get "defaultSnap" and "promptForSnap" from CommsDat.
    CCDGlobalSettingsRecord* gs = LoadGlobalSettingsRecordLC();
	TBool promptForSnap = gs->iPromptForSnap;
	iAPid = gs->iDefaultSnap; // Unless reassigned, iAPid becomes the default access point.
	CleanupStack::PopAndDestroy(gs);

	__FLOG_STMT(_LIT(K, "SelectL() Prompt%d Def%d"));
	__FLOG_2(K, promptForSnap, iAPid);

	if (iAPid != 0)
    	// System is access point aware.
    	{
		TConnStartType selectType(iConnStart->StartType());
		TConnPref* selectPrefs = iConnStart->ConnPrefs();
		
		if (selectType == EConnStartImplicit ||
			selectPrefs == 0 || selectPrefs->ExtensionId() == TConnPref::EConnPrefUnknown)
			// Use default access point or dialogue if enabled.
			{
			__FLOG_STMT(_LIT(K, "SelectL() Default Type%d Prefs%d"));
			__FLOG_2(K, selectType, selectPrefs);

			// Use the default access point unless promptForSnap is ETrue in which case prompt
			// for the access point.
			if (promptForSnap)
				{
				User::LeaveIfError(iDlgServ.Connect());
				iDlgServ.AccessPointConnection(iAPid,KAfInet,iStatus);
				SetActive();
				return; // Don't do selection until RunL() gets the dialogue results.
				}

			CCDIAPPrioritySelectionPolicyRecord* policy = LoadPolicyRecordLC(iAPid);
			FillListL(*policy);
			CleanupStack::PopAndDestroy(policy);
			}
		else if (selectPrefs && selectPrefs->ExtensionId() == TConnPref::EConnPrefSnap)
			// Use access point id from preferences.
			{
			iAPid = static_cast<const TCommSnapPref*>(selectPrefs)->Snap();

			__FLOG_STMT(_LIT(K, "SelectL() Type%d TConnPrefSnap AccessPoint%d "));
			__FLOG_2(K, selectType, iAPid);

			CCDIAPPrioritySelectionPolicyRecord* policy = LoadPolicyRecordLC(iAPid);
			FillListL(*policy);
			CleanupStack::PopAndDestroy(policy);
			}
		}
	}

void CIPConnectionSelector::FillListL(CCDIAPPrioritySelectionPolicyRecord& aPolicy)
	{
	__FLOG_0(_L("FillListL()"));

	// Make sure we have the TCommIdList.
	
	// Create the new Prefs on the heap so that they are always available 
	// even in the asynchronous promptForSnap Active Object callback
	// The copy of the original Prefs are overwritten here
	// The original Prefs are deleted in esock.  
	// The new Prefs are deleted on destruction of CIPConnectionSelector
	
	iConnStart->SetConnPrefs(NULL);
	iConnStart->SetConnPrefs(new (ELeave) TCommIdList);
	
	// Store Prefs for deletion on destruction of CIPConnectionSelector
	ASSERT(iPrefs==0);
	iPrefs = iConnStart->ConnPrefs();
	
	TCommIdList& list = *static_cast<TCommIdList*>(iPrefs);

	CMDBRecordLink<CCDIAPRecord>* theIap = &aPolicy.iIap1;
	CMDBField<TUint32>* theCount = &aPolicy.iIapCount;
	TInt count = static_cast<TInt>(*theCount);
	if (count > CCDIAPPrioritySelectionPolicyRecord::EMaxNrOfIaps)
		{		
		// The number of IAP's specified is more than allowed. Fix your table :-)
		ASSERT(EFalse);
		count = CCDIAPPrioritySelectionPolicyRecord::EMaxNrOfIaps;
		}	
	for (TInt i = 0; i < count; i++, theIap++)
		{
		TInt theIapNumber = static_cast<TInt>(*theIap);
		ASSERT(theIapNumber>0);
		__FLOG_STMT(_LIT(K, "aList[%d].Append(%d)"));
		__FLOG_2(K, list.Count(), theIapNumber);
		list.Append(theIapNumber);
		}
	}

void CIPConnectionSelector::SelectLinkLayerL()
	{
	CConnectionFactoryContainer* connectionFactories = SockManGlobals::Get()->iConnectionFactories;
	ASSERT(connectionFactories);
	CConnectionProviderFactoryBase* factory = connectionFactories->FindFactory(KShimConnectionProviderFactoryId);
	ASSERT(factory);
	ISelectionNotify selectNotify( this, TSelectionNotify<CIPConnectionSelector>::SelectComplete, 
	                                     TProgressNotify<CIPConnectionSelector>::ProgressNotification,
	                                     TServiceChangeNotify<CIPConnectionSelector>::ServiceChangeNotification,
	                                     TLayerUp<CIPConnectionSelector>::LayerUp,
	                                     TSubConnectionEventTmpl<CIPConnectionSelector>::SubConnectionEvent, NULL);
	selectNotify.RegisterDetach(TDetachNotify<CIPConnectionSelector>::Detach);

	if (iNextLayerSelector!=NULL)
		iNextLayerSelector->Cancel();
	
	// Select next (link) layer's provider.
	ASSERT(iNextLayerSelector==NULL);
	ASSERT(iConnStart!=NULL);

	iNextLayerSelector = factory->SelectProvider(*iConnStart, selectNotify, iSelectMessage.IsNull()? NULL : &iSelectMessage);
    
    if (iNextLayerSelector == NULL)
		{
		User::Leave(KErrGeneral);
		}

    }

void CIPConnectionSelector::SelectComplete(CConnectionProviderBase* aConnProvider, TInt aError)
    {
    CIPNetworkConnectionProvider* connProvider = NULL;
    if (aError == KErrNone)
        {
        ASSERT(aConnProvider);
        XConnectionIPFactoryQuery query(aConnProvider);

        TRAP( aError, connProvider = static_cast<CIPNetworkConnectionProvider*>(iFactory.FindOrCreateProviderL(query)));
        if (aError == KErrNone && connProvider->NextLayer() == NULL)
            {

#ifdef SYMBIAN_NETWORKING_UMTSR5	
			// This piece of code is added to keep the information about the application secure ID in the 
			// IP Connection provider. So that when the information is required form the subconnection provider
			// we can do a fetch interface and get the App Secure ID to decide on to the Socket Blocking			
                
              connProvider->SetAppSecurId(iAppSecureId.iId);            
  
          
#endif // SYMBIAN_NETWORKING_UMTSR5	            

            // The factory returned a new instance - must set the lower layer.
            TRAP(aError,connProvider->JoinNextLayerL(aConnProvider));
            }
        }
    iNotify.SelectComplete(connProvider, aError);
    }
    
void CIPConnectionSelector::ProgressNotification(TInt aStage, TInt aError)
    {
    //The original ISelectionNotifier (iNotify) might be interested in the
    //progress, but we aren't.
    iNotify.ProgressNotification(aStage, aError);
    }

void CIPConnectionSelector::LayerUp(TInt aError)
	{
    iNotify.LayerUp(aError);
	}

void CIPConnectionSelector::SubConnectionEvent(CSubConnectionProviderBase* aSubConnNextLayerProvider, const TSubConnectionEvent& aSubConnectionEvent)
	{
	iNotify.SubConnectionEvent(aSubConnNextLayerProvider, aSubConnectionEvent);
	}
	
void CIPConnectionSelector::ServiceChangeNotification(TUint32 aId, const TDesC& aType)
	{
    //The original ISelectionNotifier (iNotify) might be interested in the
    //notification, but we aren't.
    iNotify.ServiceChangeNotification(aId, aType);
	}

void CIPConnectionSelector::Detach()
	{
	iNextLayerSelector = NULL;
	//Ensure the asynch destructor is ready to use.
	//If its not, then we have probably been already deleted which should never happen.
	//Detach is the only place we should be deleted from.
	ASSERT(!iAsyncDestructor.IsActive());
	__FLOG_1(_L8("CIPConnectionSelector %08x::Detach()"), this);
	iAsyncDestructor.Call();
	}

CIPConnectionSelector::CIPConnectionSelector(ISelectionNotify& aNotify, CIPNetworkProviderFactory& aFactory)
:	CActive(CActive::EPriorityUserInput),
	iNotify(aNotify),
	iFactory(aFactory),
	iAsyncDestructor(CActive::EPriorityLow)
	{
	__FLOG_1(_L8("CIPConnectionSelector %08x::CIPConnectionSelector()"), this);
	CActiveScheduler::Add(this);
	iAsyncDestructor.Set(TCallBack(CIPConnectionSelector::DestroyMyself, this));
	
  	}

TInt CIPConnectionSelector::DestroyMyself(TAny* aSelf)
	{
	delete static_cast<CIPConnectionSelector*>(aSelf);
	return KErrNone;
	}

CIPConnectionSelector::~CIPConnectionSelector()
    {
    __FLOG_CLOSE;
	CActive::Cancel(); // There may be an outstanding selection request.

	// This destructor is private and is meant to be called asynchronously via Detach() or Cancel() only.
	// If is was called from anywhere else, the iNextLayerSelector would not be deleted!
	// Please note that deleting iNextLayerSelector here needs revision on the link layer selectors,
	// and specifically of the shim selector which - in such case - must not call Detach from its
	// synchronous destructor!
	ASSERT(iNextLayerSelector==NULL); // If still a valid pointer - probably not called via Detach() or Cancel().

	delete iDbs;
	
	// Tidy up iConnStart and related objects
	delete iPrefs;
	delete iConnStart;
	
	iDlgServ.Close();

	// Notify detach.
	iNotify.Detach();
    }

void CIPConnectionSelector::RunL()
	// The dialogue has been presented.
	// Normally completes with KErrNone or KErrCancel
	// Could, however, complete with another system error e.g. KErrOutOfMemory
	{
	__FLOG_STMT(_LIT(K, "RunL() Err%d Snap%d"));
	__FLOG_2(K, iStatus.Int(), iAPid);

    User::LeaveIfError(iStatus.Int());
    ASSERT(iAPid); //Should not be 0 now.
	CCDIAPPrioritySelectionPolicyRecord* policy = LoadPolicyRecordLC(iAPid);
	FillListL(*policy);
	CleanupStack::PopAndDestroy(policy);
	SelectLinkLayerL();
	}

TInt CIPConnectionSelector::RunError(TInt aError)
	// Either the dialogue, the FillListL() or the SelectLinkLayerL() failed.
	// In each case the selection request is completed with the apropriate result code.
	{
	iNotify.SelectComplete(0, aError);
 	
 	//If we have failed before the call to iNextLayerSelector->Select() or it wasn't successful
 	//we need to initiate the detach sequence by calling Detach().
 	 if (iNextLayerSelector==NULL)
	 	{
	 	Detach(); //It will result in self deletion.
	 	}
	return KErrNone;
	}

void CIPConnectionSelector::DoCancel()
	{
	iDlgServ.CancelAccessPointConnection();
	}

CCDGlobalSettingsRecord* CIPConnectionSelector::LoadGlobalSettingsRecordLC()
	{
	CCDGlobalSettingsRecord* gs = static_cast<CCDGlobalSettingsRecord*>(CCDConnectionPrefsRecord::RecordFactoryL(KCDTIdGlobalSettingsRecord));
	CleanupStack::PushL(gs);
	gs->SetRecordId(1);
	gs->LoadL(*iDbs);
	ASSERT(gs->iDefaultSnap.TypeId() == KCDTIdDefaultSnap); // Panics if built against incorrect CommsDat.
	return gs;
	}

CCDIAPPrioritySelectionPolicyRecord* CIPConnectionSelector::LoadPolicyRecordLC(TInt aAccessPoint)
	{
	// Get access point from CommsDat.
	CCDAccessPointRecord* apRecord = static_cast<CCDAccessPointRecord*>
		(CCDConnectionPrefsRecord::RecordFactoryL(KCDTIdAccessPointRecord));
	CleanupStack::PushL(apRecord);
	apRecord->SetRecordId(aAccessPoint);
	apRecord->LoadL(*iDbs);
	TUint32 policyNumber = apRecord->iSelectionPolicy;
	CleanupStack::PopAndDestroy(apRecord);

	ASSERT((policyNumber & KCDMaskShowRecordType) == KCDTIdIapPrioritySelectionPolicyRecord);

	CCDIAPPrioritySelectionPolicyRecord* policy = static_cast<CCDIAPPrioritySelectionPolicyRecord*>
		(CCDConnectionPrefsRecord::RecordFactoryL(KCDTIdIapPrioritySelectionPolicyRecord));
	CleanupStack::PushL(policy);
	policy->SetElementId(policyNumber);
	policy->LoadL(*iDbs);
	return policy;
	}

MCommsFactoryQuery::TMatchResult XConnectionIPFactoryQuery::Match( TFactoryObjectInfo& aProviderInfo )
	{
	CConnectionProviderBase* prov = static_cast<CConnectionProviderBase*>(aProviderInfo.iInfo.iFactoryObject);
	//if the next layer is the same as the one returned by the shim selection we have a match
	return prov->NextLayer() == iConnectionProviderBase ? EMatch : EContinue;
	}

