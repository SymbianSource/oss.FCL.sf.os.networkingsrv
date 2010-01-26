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
// This is part of an ECOM plug-in
// 
//

#include "IPProtoTierManagerSelector.h"
#include "IPProtoTierManagerFactory.h"
#include "IPProtoMCpr.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <commsdattypesv1_1.h> // CommsDat
#include <es_connpref.h>	//TConnIdList
#include <commdbconnpref.h>	//TCommDbConnPref & TCommDbConnPrefMulti

#include <comms-infras/esock_params_internal.h>  //TConnCSRParams
#include <comms-infras/ss_mcprnodemessages.h> //RConnPrefList and Iterator
#include <comms-infras/esock_params.h>
#include <comms-infras/ss_metaconnprov_internal.h>
#include <comms-infras/simpleselectorbase.h>
#include <commsdattypesv1_1_partner.h>

#ifdef _DEBUG
#define KIpProtoTierMgrTag KESockMetaConnectionTag
_LIT8(KIpProtoTierMgrSubTag, "ipprototiermgr");
#endif

using namespace ESock;
using namespace CommsDat;

const TInt KCommsdatLinkFlag    =  0x80000000;


//Panic codes
_LIT(KIpProtoSelectorPanic, "IpProtoSelector");
enum TIpProtoSelectorPanic
	{
	EExpectedAccessPointAwareSystem = 0,
	EUnExpectedSelectionPreferences = 1,
	EUnExpectedNumberOfAccessPoints = 2
    };

TUint FindIPProtoAccessPointForIAPL(TUint aIap, CMDBSession* aDbs)
	{
	CCDAccessPointRecord* apRec = static_cast<CCDAccessPointRecord*>(CCDTierRecord::RecordFactoryL(KCDTIdAccessPointRecord));
	CleanupStack::PushL(apRec);
	apRec->iCprConfig = aIap;
	apRec->iTier = CIPProtoTierManagerFactory::iUid | KCommsdatLinkFlag;
	
	TUint ap = 0;
	TBool found = apRec->FindL(*aDbs);
	if (found)
		{
		ap = apRec->iRecordTag;
		}
	else
		{
		ap = TierManagerUtils::ReadDefaultAccessPointL(TUid::Uid(CIPProtoTierManagerFactory::iUid), *aDbs);
		}
	CleanupStack::PopAndDestroy(apRec);
	
	return ap;
	}


CMetaConnectionProviderBase* AIpProtoSelectorBase::FindOrCreateProviderL(TUint aIapToFind, TUint aAccessPointToCreate)
	{
	//Find factory
	TUid mCprUid = TierManagerUtils::ReadMCprUidL(aAccessPointToCreate,*iDbs);
	CMetaConnectionProviderFactoryBase* factory = static_cast<CMetaConnectionProviderFactoryBase*>(iMetaContainer.FindOrCreateFactoryL(mCprUid));

	//Create the provider
	TUid tierId = TUid::Uid(iTierRecord->iRecordTag);
	TUid tierImplUid = TUid::Uid(iTierRecord->iTierImplUid);
	TProviderInfo providerInfo(tierId,aIapToFind);
	TMetaConnectionFactoryQuery query(providerInfo,tierImplUid);
	CMetaConnectionProviderBase* provider = static_cast<CMetaConnectionProviderBase*>(factory->Find(query));
	if (provider==NULL)
		{
		//If this is an attach selection, the provider must be found, so leave with an error
		if (iSelectionPrefs.Scope()&TSelectionPrefs::ESelectFromExisting)
	    	{
			//For legacy reasons only, we need to return KErrCouldNotConnect instead of KErrNotFound
			//for leagcy attach (legacy attach uses EConnPrefCommDb).
			User::Leave(iSelectionPrefs.Prefs().ExtensionId() == TConnPref::EConnPrefCommDb ? KErrCouldNotConnect : KErrNotFound);
	       	}

       	//For construction of IpProto providers (construction only) we use the instance field of TProviderInfo
       	//to carry the IAP id. The IAP id is necessary fo the provider to initialise itself from the database.
       	TMetaConnectionFactoryQuery query(TProviderInfo(tierId,aAccessPointToCreate,(TAny*)aIapToFind),tierImplUid);
       	provider = static_cast<CMetaConnectionProviderBase*>(factory->CreateObjectL(query));
   		}

	if (!(iSelectionPrefs.Scope()&TSelectionPrefs::ESelectFromExisting))
		{
		CIPProtoMetaConnectionProvider *ipprotomcpr = static_cast<CIPProtoMetaConnectionProvider *>(provider);
		if (ipprotomcpr->iIapLocked)
			User::Leave(KErrPermissionDenied);

		TSecureId sid;
		ASubSessionPlatsecApiExt platsecext(iSelectionPrefs.SubSessionUniqueId());
		if (platsecext.SecureId(sid) == KErrNone)
			{
			CCommsDatIapView* iapView = CCommsDatIapView::NewLC(aIapToFind);

			TUint32 iapsid;
			iapView->GetIntL(KCDTIdIAPAppSid, iapsid);

			if (sid.iId == iapsid && iapsid != 0)
				ipprotomcpr->iIapLocked = ETrue;
			CleanupStack::PopAndDestroy(iapView);
			}
		}
	provider->IncrementBlockingDestroy();
	return provider;
	}

//
//CIpProtoProviderSelector

void CIpProtoProviderSelector::RunL()
	// The dialogue has been presented.
	// Normally completes with KErrNone or KErrCancel
	// Could, however, complete with another system error e.g. KErrOutOfMemory
	{
	TInt error = iStatus.Int();
	__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CIpProtoProviderSelector %08x::\tRunL() Err%d Ap%d"),this, error, SelectedIAP()));
    User::LeaveIfError(error);

    ASSERT(SelectedIAP()); //Should not be 0 now.
    
    TConnPref& pref = iSelectionPrefs.Prefs();
    const TConnIdList& list = static_cast<const TConnIdList&>(pref);
    TUint accessPoint = list.Get(0);
    
    //finish the selection which the prompt was for
    CMetaConnectionProviderBase* ipprotoMCPr = FindOrCreateProviderL(SelectedIAP(), FindIPProtoAccessPointForIAPL(SelectedIAP(), iDbs));
    TLayerSelectionInfo* selectionInfo = const_cast<TLayerSelectionInfo*>(
            static_cast<const TLayerSelectionInfo*>(ipprotoMCPr->AccessPointConfig().FindExtension(
            		STypeId::CreateSTypeId(TLayerSelectionInfo::EUid, TLayerSelectionInfo::ETypeId))));
    __ASSERT_ALWAYS(selectionInfo, User::Panic(_L("ipproto"), CorePanics::KPanicUnexpectedExecutionPath));
    
    if ((TUint)-1 == selectionInfo->CustomSelectionPolicy())
    	{
    	/*
		 * This means that the default IPProto AP is used from the meshpreface.cfg file,
		 * which has a -1 as the CustomSelectionPolicy (the reason for that is that we don't know
		 * which will be the default IAP, default Link level AP, so that's why we prompted for that).
		 * This is the place and time we know how to overwrite the AccessPoint information of the MCPr
		 * meaning we know the selected IAP. So the -1 value can be overwritten with the selected recordID,
		 * or tagID of the Link level AP.
		 */
		selectionInfo->iCustomSelectionPolicy = SelectedIAP();
    	selectionInfo->iCprConfig = SelectedIAP();
    	}
    
    iSelectionNotify.SelectComplete(this,ipprotoMCPr);
	//iSelectionNotify.SelectComplete(this,FindOrCreateProviderL(SelectedIAP(), FindIPProtoAccessPointForIAPL(SelectedIAP(), iDbs)));
	
    //store the information that we have already prompted the user
    TPromptingSelectionPrefsExt* promptingExt = const_cast<TPromptingSelectionPrefsExt*>(
            static_cast<const TPromptingSelectionPrefsExt*>(iNetworkMCpr->AccessPointConfig().FindExtension(
                    TPromptingSelectionPrefsExt::TypeId())));
	if (promptingExt == NULL)
		{
		/*
		 * the extension is not existing yet on the AccessPointConfig of the NetMCPr. Append a new one
		 * to indicate that we have already prompted the user. So if the configuration contains 2
		 * ConnectionPreferences record without any default IAP and with PROPTING dialog preference then
		 * the prompt dialog won't be invoked in the 2nd time but only during the re-selection, if it's needed.
		 */
	    RMetaExtensionContainer mec;
	    mec.Open(iNetworkMCpr->AccessPointConfig());
	    CleanupClosePushL(mec);
	    
	    TConnIdList dummyList;
		TPromptingSelectionPrefsExt* ext = new (ELeave) TPromptingSelectionPrefsExt(TUid::Uid(iTierRecord->iRecordTag),dummyList); //it's an empty list
		CleanupStack::PushL(ext);
		mec.AppendExtensionL(ext);
        CleanupStack::Pop(ext);

		iNetworkMCpr->AccessPointConfig().Close();
		iNetworkMCpr->AccessPointConfig().Open(mec);
		CleanupStack::PopAndDestroy(&mec);
		}
	else if (!promptingExt->iPromptingInProgress)
	    {
	    promptingExt->iPromptingInProgress = ETrue;
	    }
	else
		{
		/* the extension is existing already on the AccessPointConfig of the NetMCPr.
		 * This means that this is was the 2nd time we have prompted. Mark the extension
		 * to indicate that we don't have any more choices...
		 */
		promptingExt->iPromptingInProgress = EFalse;
		}
	
	//Complete the selection request
	iSelectionNotify.SelectComplete(this,NULL);
	}

TInt CIpProtoProviderSelector::RunError(TInt aError)
	{
	iSelectionNotify.Error(this,aError);
	return KErrNone;
	}

void CIpProtoProviderSelector::InvokeDialogL(ISelectionNotify& aSelectionNotify,
		                                     TUint aCprConfig)
	{
	TConnectionPrefs prefsForPromt;
	TierManagerUtils::GetPrefsFromConnPrefRecL(aCprConfig, *iDbs, prefsForPromt);
	
	PromptUserL(aSelectionNotify, prefsForPromt);
	}

void CIpProtoProviderSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	ASSERT(iDbs);
	ASSERT(iTierRecord);
	TUint32 defaultAccessPoint = iTierRecord->iDefaultAccessPoint;

	//Must be set
	__ASSERT_DEBUG(defaultAccessPoint!=0,User::Panic(KIpProtoSelectorPanic,EExpectedAccessPointAwareSystem));
	User::LeaveIfError(defaultAccessPoint!=0? KErrNone : KErrCorrupt);

	TConnPref& pref = iSelectionPrefs.Prefs();
	//This selector receives only EConnPrefIdList at this layer
	__ASSERT_DEBUG(pref.ExtensionId() == TConnPref::EConnPrefIdList, User::Panic(KIpProtoSelectorPanic,EUnExpectedSelectionPreferences));

	__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CIpProtoProviderSelector %08x::\tSelectL() Using TConnIdList"),this));
	const TConnIdList& list = static_cast<const TConnIdList&>(pref);

	TUint apElemId = 0;
	TUint cprConfig = 0;
	TUint custSelPol = 0;
	
	TInt count = list.Count();
	for (TInt i = 0; i < count; i++)
		{
		TUint accessPoint = list.Get(i);
		
		cprConfig  = TierManagerUtils::ReadCprConfigL(accessPoint, *iDbs);
		custSelPol = TierManagerUtils::ReadCustomSelectionPolicyIdL(accessPoint, *iDbs);
		
		if ( (KCDTIdConnectionPrefsRecord == (KCDMaskShowRecordType & cprConfig)) ||
			 ((TUint)-1 == custSelPol) )
			{
			/* 1.)
			 * There is a full TMDBelementID in the cprConfig field of the given AP. 
			 * This means that the related ConnPref record is a prompting record so
			 * this AP is a so-called 'prompring AP. Invoke a dialog prompt.
			 * 
			 * 2.)
			 * The default IPProto AP is used form the meshpreface.cfg file, where
			 * there isn't any connectionPrefs record in the database. We should invoke
			 * a dialog in this case too to get back the tagID of the Linke level AP, which
			 * is the same as recordID of the used IAP.
			 * 
			 */
			
		    TPromptingSelectionPrefsExt* promptingExt = const_cast<TPromptingSelectionPrefsExt*>(
		            static_cast<const TPromptingSelectionPrefsExt*>(iNetworkMCpr->AccessPointConfig().FindExtension(
		                    TPromptingSelectionPrefsExt::TypeId())));
		    if (promptingExt)
				{
				/* our NetMCPr has a prompting extension already. This can mean 2 things:
				 * 1.) We are still in the selection process and we have 2 prompting APs on
				 * 	   the IPProto level. In this case now (2nd time) don't invoke any further
				 *     dialog but store the information in the NetMCPr that we have still 1
				 *     possibility for the re-selection scenario.
				 * 2.) This is the re-selection scenario, so we have to invoke the prompt
				 *     dialog.
				 */
				
				if (!iReselection)
					{
					//the 1.) paragraph is true
					TConnIdList list;
					list.Append(accessPoint);
					
					promptingExt->iPrefs = list;
					promptingExt->iTierId = TUid::Uid(iTierRecord->iRecordTag);
					promptingExt->iPromptingInProgress = ETrue;
					}
				else
					{
					//the 2.) paragraph is true
					//invoke a dialog
					InvokeDialogL(aSelectionNotify, cprConfig);
					return; //selection will continue in the RunL
					}
				}
			else
				{
				//invoke a dialog
				InvokeDialogL(aSelectionNotify, cprConfig);
				return; //selection will continue in the RunL
				}
			}
		else
			{
			/*
			 * business as usual
			 */
			TPromptingSelectionPrefsExt* promptingExt = const_cast<TPromptingSelectionPrefsExt*>(
                static_cast<const TPromptingSelectionPrefsExt*>(iNetworkMCpr->AccessPointConfig().FindExtension(
                    TPromptingSelectionPrefsExt::TypeId())));
    		if (promptingExt != NULL)
				{
				/* There is a prompting extension in the AccessPointConfig of our
				 * Network MCPr. This means that there are 2 ConnPref records in the
				 * database. The first one is a prompting one (that's why the extension is there
				 * - to indicate that we have already prompted). But the 2nd record is
				 * not a prompting record, so the mapped IPProto AP is _NOT_ a prompting AP.
				 * Continue the selection as usual, and mark the extension - it's not
				 * needed anymore because if there will be a re-selection we can't use
				 * the extension as the 2nd IPProto AP is not a promting one.
				 */
				promptingExt->iPromptingInProgress = EFalse;
				}
			
			//apElemId = TierManagerUtils::ReadCustomSelectionPolicyIdL(accessPoint,*iDbs);
			apElemId = TierManagerUtils::ReadCprConfigL(accessPoint,*iDbs);
			aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(apElemId,accessPoint));
			}
		} //for

	//Complete the selection request
	aSelectionNotify.SelectComplete(this,NULL);
	}


//
//CIpProtoProviderSelectorConnPrefList
CIpProtoProviderSelectorConnPrefList::CIpProtoProviderSelectorConnPrefList(const ESock::RConnPrefList& aSelectionPreferences)
	:	AIpProtoSelectorBase()
	,	iSelectionPrefList(aSelectionPreferences)
	,	iTierId(TUid::Uid(CIPProtoTierManagerFactory::iUid))
	,	iLastAccessPoint(0)
		{
		}

void CIpProtoProviderSelectorConnPrefList::RunL()
	// The dialogue has been presented.
	// Normally completes with KErrNone or KErrCancel
	// Could, however, complete with another system error e.g. KErrOutOfMemory
	{
	TInt error = iStatus.Int();
	__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CIpProtoProviderSelector %08x::\tRunL() Err%d Ap%d"),this, error, SelectedIAP()));
    User::LeaveIfError(error);

    ASSERT(SelectedIAP()); //Should not be 0 now.
    
    //finish the selection which the prompt was for
	iSelectionNotify.SelectComplete(this,FindOrCreateProviderL(SelectedIAP(), iLastAccessPoint));
	
    //--------------------------------------------------
    //continue the loop which we started in the SelectL
    //--------------------------------------------------
    ESock::RConnPrefList::TIter<TConnAPPref> iterAP = iSelectionPrefList.getIter<TConnAPPref>();
    
	CCDAccessPointRecord* apRec;
	TUid tierId;
	TInt cprConfig = 0;
	
//This iterator runs through all the instances of TConnAPPref
	while(!iterAP.IsEnd())
		{
		//Access point is retrived from preference
		TUint accessPoint = iterAP->GetAP();
		//The record relating to this access point is retrived
		apRec = TierManagerUtils::LoadAccessPointRecordL(accessPoint,*iDbs);
		//A check is done to confirm the access point is related to this tier
		//tierId = TUid::Uid(apRec->iTier);
		tierId = TierManagerUtils::MapElementIdToTagId(apRec->iTier, *iDbs);
		
		delete apRec;
		if(iTierId == tierId)
			{
			//The iap related to the access point is retrived from the record
			//This iap must be created at the link layer, within the method
			//FindOrCreateProviderL the provider created is provisioned with this
			//iap, this means when the next tier is created the appropriate iap will
			//be created
			
			cprConfig =  TierManagerUtils::ReadCprConfigL(accessPoint,*iDbs);
			
			if (KCDTIdConnectionPrefsRecord == (KCDMaskShowRecordType & cprConfig))
				{
				/* There is a full TMDBelementID in the cprConfig field of the given AP.
				 * This means that the related ConnPref record is a prompting record so
				 * this AP is a so-called 'prompring AP. As we are in the RunL method
				 * this means that we've invoked already a dilog prompt so here we cannot
				 * invoke another one. Instead store the information that we have still
				 * possibilies in the case of a reconnection.
				 */
				TConnIdList list;
				list.Append(accessPoint);
				
				RMetaExtensionContainer mec;
				mec.Open(iNetworkMCpr->AccessPointConfig());
				CleanupClosePushL(mec);
				
				TPromptingSelectionPrefsExt* ext = new (ELeave) TPromptingSelectionPrefsExt(TUid::Uid(iTierRecord->iRecordTag),list); //it's an empty list
				CleanupStack::PushL(ext);
				mec.AppendExtensionL(ext);
				CleanupStack::Pop(ext);
				
				iNetworkMCpr->AccessPointConfig().Close();
				iNetworkMCpr->AccessPointConfig().Open(mec);
				CleanupStack::PopAndDestroy(&mec);
				}
			else
				{
				iSelectionNotify.SelectComplete(this,FindOrCreateProviderL(cprConfig, accessPoint));
				}
			//The peference is no longer needed so is deleted and removed
			delete iterAP.Remove();
			}
		else
			{
			iterAP++;
			}
		}
	iSelectionNotify.SelectComplete(this,NULL);
	}

TInt CIpProtoProviderSelectorConnPrefList::RunError(TInt aError)
	{
	iSelectionNotify.Error(this,aError);
	return KErrNone;
	}

void CIpProtoProviderSelectorConnPrefList::SelectL(ISelectionNotify& aSelectionNotify)
	{
	iDbs = CMDBSession::NewL(KCDVersion1_2);
	iTierRecord = TierManagerUtils::LoadTierRecordL(iTierId,*iDbs);

	ASSERT(iTierRecord);

	CCDAccessPointRecord* apRec;
	TUid tierId;

	ESock::RConnPrefList::TIter<TConnCSRPref> iterCSR = iSelectionPrefList.getIter<TConnCSRPref>();
	ASSERT(iterCSR[0] != NULL || iterCSR[1] == NULL );
	iSelectionPrefs.SetScope(iterCSR[0]->Scope());
	iSelectionPrefs.SetFlags(iterCSR[0]->Flags());
	iSelectionPrefs.SetSubSessionUniqueId(iterCSR[0]->SubSessionUniqueId());
	
	TInt cprConfig = 0;
	
	ESock::RConnPrefList::TIter<TConnAPPref> iterAP = iSelectionPrefList.getIter<TConnAPPref>();

	//This iterator runs through all the instances of TConnAPPref
	while(!iterAP.IsEnd())
		{
		//Access point is retrived from preference
		TUint accessPoint = iterAP->GetAP();
		//The record relating to this access point is retrived
		apRec = TierManagerUtils::LoadAccessPointRecordL(accessPoint,*iDbs);
		//A check is done to confirm the access point is related to this tier
		//tierId = TUid::Uid(apRec->iTier);
		tierId = TierManagerUtils::MapElementIdToTagId(apRec->iTier, *iDbs);
		
		delete apRec;
		if(iTierId == tierId)
			{
			//The iap related to the access point is retrived from the record
			//This iap must be created at the link layer, within the method
			//FindOrCreateProviderL the provider created is provisioned with this
			//iap, this means when the next tier is created the appropriate iap will
			//be created
			
			cprConfig =  TierManagerUtils::ReadCprConfigL(accessPoint,*iDbs);
			
			if (KCDTIdConnectionPrefsRecord == (KCDMaskShowRecordType & cprConfig))
				{
				/* There is a full TMDBelementID in the cprConfig field of the given AP.
				 * This means that the related ConnPref record is a prompting record so
				 * this AP is a so-called 'prompring AP. Invoke a dialog prompt.
				 */
				//invoke a dialog
				
				TConnectionPrefs prefsForPromt;
				TierManagerUtils::GetPrefsFromConnPrefRecL(cprConfig, *iDbs, prefsForPromt);
				
				PromptUserL(aSelectionNotify, prefsForPromt);
				
				/* Remove the last processed preference so the loop in the
				 * RunL can continue from the beginning of the list
				 */
				delete iterAP.Remove();
				iLastAccessPoint = accessPoint;
				return; //selection will continue in the RunL
				}
			else
				{
				aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(cprConfig, accessPoint));
				}
			//The peference is no longer needed so is deleted and removed
			delete iterAP.Remove();
			}
		else
			{
			iterAP++;
			}
		}
	aSelectionNotify.SelectComplete(this,NULL);
	}


//
//CLinkPrefsSelector
CLinkPrefsSelector::~CLinkPrefsSelector()
	{
	iPrefsList.Reset();
	iPrefsList.Close();
	}

void CLinkPrefsSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	ASSERT(iDbs);
	ASSERT(iTierRecord);
	ASSERT(iLegacyMCpr); //This selector depends on the legacy MCpr (client) being set!
	iSelectionNotify = aSelectionNotify; //Store notify interface for asynch processing
	TUint32 defaultAccessPoint = iTierRecord->iDefaultAccessPoint;

	//Must be set
	__ASSERT_DEBUG(defaultAccessPoint!=0,User::Panic(KIpProtoSelectorPanic,EExpectedAccessPointAwareSystem));
	User::LeaveIfError(defaultAccessPoint!=0? KErrNone : KErrCorrupt);

	if (!iSelectionPrefs.IsEmpty() && iSelectionPrefs.Prefs().ExtensionId() == TConnPref::EConnPrefCommDb)
		{ //There is only ever going to be one AP (Ip layer) as a result of these prefs.
		//IAP based attach should follow this branch as well and takes the same route as
		//ordinary selection (at this layer only)
		TCommDbConnPref connPrefs = static_cast<const TCommDbConnPref&>(iSelectionPrefs.Prefs());
		__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CLinkPrefsSelector %08x::\tSelectL() Using TCommDbConnPref, IAP:%d"),this,connPrefs.IapId()));
		OverrideLegacyPrefs(connPrefs,1);
		iPrefsList.AppendL(connPrefs);
		}
	else if (!iSelectionPrefs.IsEmpty() && iSelectionPrefs.Prefs().ExtensionId() == TConnPref::EConnPrefCommDbMulti)
		{ //There may be multiple APs (Ip layer) as a result of these prefs.
		const TCommDbMultiConnPref& connPrefsMulti = static_cast<const TCommDbMultiConnPref&>(iSelectionPrefs.Prefs());
		TInt attempts = const_cast<TCommDbMultiConnPref&>(connPrefsMulti).ConnectionAttempts();
		ASSERT(attempts >= 0 && attempts <= TCommDbMultiConnPref::KMaxMultiConnPrefCount); //Sanity check
		__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CLinkPrefsSelector %08x::\tSelectL() Using TCommDbMultiConnPref, attempts:%d"),this,attempts));
		//Be careful, TCommDbMultiConnPref takes a special index that starts from 1!
		for (TInt i = 1; i <= attempts; i++)
			{
			TCommDbConnPref connPrefs;
			User::LeaveIfError(const_cast<TCommDbMultiConnPref&>(connPrefsMulti).GetPreference(i,connPrefs));
			OverrideLegacyPrefs(connPrefs,i);
			iPrefsList.AppendL(connPrefs);
			}
		}
	else
		{
		//We must have link layer prefs here!
		User::Panic(KIpProtoSelectorPanic,EUnExpectedSelectionPreferences);
		}

	//From this point on we attempt to mimic the legacy netcon/nifman selection logic.
	//All preferences are now stored in the iPrefsList and for our legacy scenarios
	//the list can not be empty.
	ASSERT(iPrefsList.Count()>0);
	User::LeaveIfError(iPrefsList.Count()>0? KErrNone : KErrArgument);
	ProcessPrefsFromListL();
	}

void CLinkPrefsSelector::OverrideLegacyPrefs(TCommDbConnPref& aConnPrefs, TInt aRank)
	{
	//A short guide to legacy overrides, in chronological order.
	//1) If direction preferences == ECommDbConnectionDirectionUnknown,
	//use ECommDbConnectionDirectionOutgoing, otherwise keep the prefs
	//2) If iap == 0,
	//GetPreferedIap(direction,attempt)
	//3) If iap == 0 and ECommDbDialogPrefUnknown,
	//set the dialog preference to ECommDbDialogPrefPrompt
	//4) If dialog preference == ECommDbDialogPrefUnknown,
	//use setting from the CCDTierRecord, otherwise keep the prefs
	//5) If bearer preferences == ECommDbBearerUnknown,
	//use the one from the IAP record, oterwise keep the prefs
	//6) If dialog preference == ECommDbDialogPrefDoNotPrompt and iap == 0,
	//use default from the CCDTierRecord record
	//Finally, for multiple preferences, or when the dialog is used,
	//we disallow (remove) repetitions.
	//The rest of the legacy selection logic is in the legacy reconnection
	//activity.
	if (aConnPrefs.Direction()==ECommDbConnectionDirectionUnknown)
		{
		aConnPrefs.SetDirection(ECommDbConnectionDirectionOutgoing);
		}
	if (aConnPrefs.IapId()==0)
		{
		//Ignore if the record does not exist, aConnPrefs state should not change
		TRAP_IGNORE(TierManagerUtils::MapRankingAndDirectionToPrefsL(aRank,aConnPrefs.Direction(),aConnPrefs,*iDbs));
		}
	if (aConnPrefs.IapId()==0 && aConnPrefs.DialogPreference()==ECommDbDialogPrefUnknown)
		{
		aConnPrefs.SetDialogPreference(ECommDbDialogPrefPrompt);
		}
	if (aConnPrefs.DialogPreference()==ECommDbDialogPrefUnknown)
		{
		TBool promptUser = iTierRecord->iPromptUser;
		aConnPrefs.SetDialogPreference(promptUser? ECommDbDialogPrefPrompt : ECommDbDialogPrefDoNotPrompt);
		}
	if (aConnPrefs.BearerSet()==ECommDbBearerUnknown)
		{
		}
	if (aConnPrefs.IapId()==0 && aConnPrefs.DialogPreference()==ECommDbDialogPrefDoNotPrompt)
		{
		aConnPrefs.SetIapId(iTierRecord->iDefaultAccessPoint);
		}
	}


void CLinkPrefsSelector::ProcessPrefsFromListL()
	{
	//There may be more than one preference in the list (usually one or two).
	//Some of them may have their dialog preference as ECommDbDialogPrefDoNotPrompt,
	//some may have ECommDbDialogPrefPrompt.

	//To code this selection behaviour as close to the legacy netcon's/nifman's
	//logic as possible, we have taken the following approach:
	//1) We select as much from the preference list as possible without invoking a dialog.
	//2) If the selection is completed (iToBeSelected is empty) we finish, otherwise
	//3) We prompt for the preferences that need to be prompted for
	//4) If the dialog failed (has been cancelled, etc) we finish with an error, otherwise
	//5) If the selection is completed (iToBeSelected is empty) we finish, otherwise
	//6) We find the NetMCpr and store all remaining iToBeSelected entries in its configuration data
	//Later, during legacy reconnection, these stored entries will be used by the legacy NetMCpr
	//to extend its meta tree by nodes that could not be selected here at this time.

	//If there is more than one preference in the queue, we will only ever prompt once, for the first one.
	ASSERT(iPrefsList.Count());

	//Take the first preference (see above for details)
	TCommDbConnPref connPrefs = iPrefsList[0];
	iPrefsList.Remove(0); //We do not want to consider this option again

	__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("CLinkPrefsSelector %08x\tProcessPrefsFromListL DialogPreference: %d"), this, connPrefs.DialogPreference()));

	//We are here only because DialogPreference()!=ECommDbDialogPrefDoNotPrompt
 	if (connPrefs.DialogPreference()==ECommDbDialogPrefDoNotPrompt ||
 		connPrefs.DialogPreference()==ECommDbDialogPrefPromptIfWrongMode)
		{
		SetSelectedIAP(connPrefs.IapId());
		RunL();
		}
	else
		{
		//We have to involve dialog
		User::LeaveIfError(iDlgServ.Connect());
		TConnectionPrefs dialogPrefs;
		dialogPrefs.iRank = 1;
		dialogPrefs.iDirection = connPrefs.Direction();
		dialogPrefs.iBearerSet = connPrefs.BearerSet();
		// pass in iSelectedAPId directly here, as it's being passed in as a
		// TUint32& and so IapConnection may wish to modify it.
		iDlgServ.IapConnection(iSelectedAPId,dialogPrefs,iStatus);
		SetActive();
		iUserPromted = ETrue;
		}
	}

void CLinkPrefsSelector::RunL()
	{
	if (iUserPromted)
		{
    	TInt error = iStatus.Int();
    	if (error!=KErrNone)
    		{
	    	//The dialog has most likely been cancelled, etc
	    	//This means no more selection, complete the selection request as it stands
	    	iSelectionNotify.Error(this,error);
	    	return;
	    	}
	    }

    ASSERT(SelectedIAP()); //Must not be 0 now. Check your commsdat if it is.
    ASSERT(iTierRecord);

    //Report
    iSelectionNotify.SelectComplete(this,FindOrCreateProviderL(SelectedIAP(),FindIPProtoAccessPointForIAPL(SelectedIAP(), iDbs)));

	//Selection has not finished yet, so decide what to do next
	if (iPrefsList.Count()>0)
		{
		ASSERT(iPrefsList.Count()==1); //Well, in the legacy world you could not have more than two..
		if (iUserPromted)
			{ //We have already prompted the user, so store the rest of the preferences
			//into legacy NetMCpr's configuration data.
			ASSERT(iLegacyMCpr->AccessPointConfig().FindExtension(TDeferredSelectionPrefsExt::TypeId())==NULL);
			
			RMetaExtensionContainer mec;
			mec.Open(iLegacyMCpr->AccessPointConfig());
			CleanupClosePushL(mec);
			
			TDeferredSelectionPrefsExt* ext = new (ELeave) TDeferredSelectionPrefsExt(TUid::Uid(iTierRecord->iRecordTag),iPrefsList[0]);
			iPrefsList.Close();
			CleanupStack::PushL(ext);
			mec.AppendExtensionL(ext);
			CleanupStack::Pop(ext);
			
			iLegacyMCpr->AccessPointConfig().Close();
			iLegacyMCpr->AccessPointConfig().Open(mec);
			CleanupStack::PopAndDestroy(&mec);
			}
		else
	    	{
			//We have not prompted the user yet and the selection has not finished,
			//proceed to the next preference.
	    	ProcessPrefsFromListL();
	    	return;
			}
		}

   	//Selection has finished, regardless if we have prompted user or not, finish.
   	iSelectionNotify.SelectComplete(this,NULL); //Complete the selection request
	}

TInt CLinkPrefsSelector::RunError(TInt aError)
	{
	//The iSelectionNotify must not be NULL now since it has been set in SelectL
	iSelectionNotify.Error(this,aError);
	return KErrNone;
	}

/**

	Since Ip Proto providers need to carry their IAP's identification, we use two Ids
	as parameters to this factory function:
	One is used to find a provider at this layer (IAP id), the other is used to create
	(and provision) a provider at this layer (for legacy selection it is the default
	access point).
*/
CMetaConnectionProviderBase* CLinkPrefsSelector::FindOrCreateProviderL(TUint aIapToFind, TUint aAccessPointToCreate)
	{
	TUid mCprUid = TierManagerUtils::ReadMCprUidL(aAccessPointToCreate,*iDbs);
	CMetaConnectionProviderFactoryBase* factory = static_cast<CMetaConnectionProviderFactoryBase*>(iMetaContainer.FindOrCreateFactoryL(mCprUid));

	//Create the provider
	TUid tierId = TUid::Uid(iTierRecord->iRecordTag);
	TUid tierImplUid = TUid::Uid(iTierRecord->iTierImplUid);
	TProviderInfo providerInfo(tierId,aIapToFind);
	TMetaConnectionFactoryQuery query (providerInfo,tierImplUid);
	CMetaConnectionProviderBase* provider = static_cast<CMetaConnectionProviderBase*>(factory->Find(query));
	TBool providerCreated(EFalse);
	if (provider==NULL)
		{
		//If this is an attach selection, the provider must be found, so leave with an error
		if (iSelectionPrefs.Scope()&TSelectionPrefs::ESelectFromExisting)
	    	{
			//For legacy reasons only, we need to return KErrCouldNotConnect instead of KErrNotFound
			//for leagcy attach (legacy attach uses EConnPrefCommDb).
			User::Leave(iSelectionPrefs.Prefs().ExtensionId() == TConnPref::EConnPrefCommDb ? KErrCouldNotConnect : KErrNotFound);
	       	}

       	//For construction of IpProto providers (construction only) we use the instance field of TProviderInfo
       	//to carry the IAP id. The IAP id is necessary fo the provider to initialise itself from the database.
       	TMetaConnectionFactoryQuery query(TProviderInfo(tierId,aAccessPointToCreate,(TAny*)aIapToFind),tierImplUid);
       	provider = static_cast<CMetaConnectionProviderBase*>(factory->CreateObjectL(query));
       	providerCreated = ETrue; // See below - non obvious conditional late CleanupStack::Pop()
        CleanupStack::PushL(provider);
 		if (aIapToFind != aAccessPointToCreate)
 			{
 			RMetaExtensionContainer mec;
 			mec.Open(provider->AccessPointConfig());
 			CleanupClosePushL(mec);
 			
 			//This IpProto provider must always go straight for the lower layer IAP.
 			//For that we need to override the selection policy.
			TConnIdList list;
			list.Append(aIapToFind);
			TUid nextTierId = TierManagerUtils::ReadTierIdL(aIapToFind,*iDbs);
			TOverridenSelectionPrefsExt* ext = new (ELeave) TOverridenSelectionPrefsExt(nextTierId,TSelectionPrefs(list));
			CleanupStack::PushL(ext);
			mec.AppendExtensionL(ext); //The ownership of the extension goes to AccessPointConfig now.
			CleanupStack::Pop(ext);

			provider->AccessPointConfig().Close();
			provider->AccessPointConfig().Open(mec);
			CleanupStack::PopAndDestroy(&mec);
			}
   		}

	if (!(iSelectionPrefs.Scope()&TSelectionPrefs::ESelectFromExisting))
		{
		CIPProtoMetaConnectionProvider *ipprotomcpr = static_cast<CIPProtoMetaConnectionProvider *>(provider);
		if (ipprotomcpr->iIapLocked)
		    {
			User::Leave(KErrPermissionDenied);
		    }

		TSecureId sid;
		ASubSessionPlatsecApiExt platsecext(iSelectionPrefs.SubSessionUniqueId());
		if (platsecext.SecureId(sid) == KErrNone)
			{
			CCommsDatIapView* iapView = CCommsDatIapView::NewLC(aIapToFind);

			TUint32 iapsid;
			iapView->GetIntL(KCDTIdIAPAppSid, iapsid);

			if (sid.iId == iapsid && iapsid != 0)
				ipprotomcpr->iIapLocked = ETrue;
			CleanupStack::PopAndDestroy(iapView);
			}
		}
	
	if (providerCreated)
	    {
	    // New provider may have been created so remove from the cleanup here (after all the leaving methods)
	    // When maintaining/altering code be aware of keeping the cleanup stack balanced 
        CleanupStack::Pop(provider);
	    }
	
	provider->IncrementBlockingDestroy();
	return provider;
	}


//
//TIpProtoProviderSelectorFactory::NewSelectorL - mapping prefs to a selector
MProviderSelector* TIpProtoProviderSelectorFactory::NewSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("TIpProtoProviderSelectorFactory::\tNewSelectorL()")));
	ASSERT(aSelectionPreferences.IsTypeOf(TSelectionPrefs::TypeId()));
	const ESock::TSelectionPrefs& sp = static_cast<const ESock::TSelectionPrefs&>(aSelectionPreferences);
	ASimpleSelectorBase* self = NULL;
	CBase* selfCleanupItem = NULL;
    
	CMDBSession* dbs = CMDBSession::NewLC(KCDVersion1_2);


	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	dbs->SetAttributeMask( ECDHidden | ECDPrivate );
    
	TBool prefLegacy = !sp.IsEmpty() && sp.Prefs().ExtensionId()!=TConnPref::EConnPrefSnap && sp.Prefs().ExtensionId()!=TConnPref::EConnPrefIdList;
    
	//Legacy selection (including legacy attach)
	if (prefLegacy)
		{
		__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("TIpProtoProviderSelectorFactory::\tNewSelectorL() - Legacy selector")));
		CLinkPrefsSelector* selfLink = new (ELeave) CLinkPrefsSelector(aSelectionPreferences);
		self = selfLink;
		selfCleanupItem = selfLink;
		}
	else
	//399 selection
		{
		__CFLOG_VAR((KIpProtoTierMgrTag, KIpProtoTierMgrSubTag, _L8("TIpProtoProviderSelectorFactory::\tNewSelectorL() - 399 selector")));
		CIpProtoProviderSelector* selfIPProto = new (ELeave) CIpProtoProviderSelector(aSelectionPreferences);
		self = selfIPProto;
		selfCleanupItem = selfIPProto;
		}

	CleanupStack::PushL(selfCleanupItem);
	ASSERT(self->iTierRecord==NULL);
	self->iTierRecord = TierManagerUtils::LoadTierRecordL(TUid::Uid(CIPProtoTierManagerFactory::iUid),*dbs);
	CleanupStack::Pop(selfCleanupItem);
	CleanupStack::Pop(dbs);
	ASSERT(self->iDbs==NULL);
	self->iDbs = dbs;
	return self;
	}


