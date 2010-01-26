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

#include "iptiermanagerselector.h"
#include "iptiermanagerfactory.h"
#include <comms-infras/ss_log.h>
#include <ss_glob.h>
#include <comms-infras/ss_metaconnprov_internal.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <commsdattypesv1_1.h> // CommsDat
#include <es_connpref.h>	//TConnIdList
#include <commsdattypesv1_1_partner.h>
#include <es_enum_internal.h>

//IpProto Tier Manager's UID.
//Since meaning of the selection policy id changes with
//the legacy flavor it is difficult to derive the next layer
//tier's id dynamically when supporting all of the legacy scenarios.
//At the same time it is known that for all of these legacy
//setups this is and always will be the only next layer tier manager.
static const TUid KLegacyNextLayerTierId = { 0x10281DF0 }; //CIPProtoTierManagerFactory::iUid


#ifdef __CFLOG_ACTIVE
#define KIpTierMgrTag KESockMetaConnectionTag
_LIT8(KIpTierMgrSubTag, "iptiermanager");
#endif

using namespace ESock;
using namespace CommsDat;

//Panic codes
_LIT(KIpSelectorPanic, "IpSelector");
enum TIpSelectorPanic
	{
	EExpectedAccessPointAwareSystem = 0,     //
	EUnExpectedSelectionPreferences = 1,
    };


//
//CIpProviderSelector
NONSHARABLE_CLASS(CIpProviderSelector) : public CSimplePromptingSelectorBase
/** Simple selector object.
	May need to prompt the user in the future, currently not prompting.

@internalComponent
@prototype
*/
	{
public:
	static const TUint KUid = 0x10283021;

public:
	explicit CIpProviderSelector(const Meta::SMetaData& aSelectionPreferences)
		:	CSimplePromptingSelectorBase(KUid, aSelectionPreferences)
		{
		}

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aAccessPoint);
	
	// From CActive
	virtual void RunL();
	virtual TInt RunError(TInt aError);
	};


void CIpProviderSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	ASSERT(iDbs);
	ASSERT(iTierRecord);
	TUint32 defaultAccessPoint = iTierRecord->iDefaultAccessPoint;

	if (iSelectionPrefs.IsEmpty())
		{
    	//Implicit case on the new setup
    	ASSERT(iSelectionPrefs.Flags()==0);
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelector %08x::\tSelectL() Using Default AP:%d"),this,defaultAccessPoint));
		TBool promptUser = iTierRecord->iPromptUser;
		//TUint selectionScope = iSelectionPrefs.Scope();
        
        /**
        enum TSelectionScope
    		{
    		ENone               = 0x00, //For initialisation only
    		ESelectFromExisting = 0x01, //If not set, means the opposite (select even if does not exist, create).
    		ESelectWholePlane   = 0x02, //If not set, means the opposite (select only the top provider).
    		ERequestCommsBinder = 0x04  //If not set, means the opposite (do not request comms binder).
    		};
		*/
        
        
		if (promptUser/* &&
		    selectionScope != TSelectionPrefs::ERequestCommsBinder*/)
		//this means that we are not prompting when this is an implicit connection
		//Assumption: the selectionScope is TSelectionPrefs::ERequestCommsBinder if the Select 
		//            message is because of an ImplicitFlowRequest
			{
			PromptUserL(aSelectionNotify);
			}
		else
			{
			aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(defaultAccessPoint));
			aSelectionNotify.SelectComplete(this,NULL);
			}
		return; //Don't do any more selection (or untill the dialog returns)
		}

	const TConnPref& prefs = iSelectionPrefs.Prefs();
	if (prefs.ExtensionId() == TConnPref::EConnPrefProviderInfo)
		{
    	ASSERT(iSelectionPrefs.Scope() & TSelectionPrefs::ESelectFromExisting); //This is always attach
		const TConnProviderInfo& connProvInfo = static_cast<const TConnProviderInfoPref&>(prefs).Info();
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelector %08x::\tSelectL() Using TConnProviderInfoPref, AP:%d"),this,connProvInfo.iInfo[1]));
		aSelectionNotify.SelectComplete(this,FindProviderL(connProvInfo.iInfo[1],(TAny*)connProvInfo.iInfo[2]));
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	if (prefs.ExtensionId() == TConnPref::EConnPrefSnap)
		{
		/*
		 * NOTE: Here the accessPoint variable contains the received SNAP preference which is 
		 * ----- an AccessPoint RECORD ID!!!!!!
		 *       As the interpretation of AccessPoints is based on TagID in the ESock code this id
		 *       has to be 'translated' to a tag ID.
		 * 
		 * PRE-CONDITION: CommsDat should generate TagIds to each network level AccessPoint!!!!!
		 * --------------
		 */
		TUint tempAaccessPoint = static_cast<const TConnSnapPref&>(prefs).Snap();
		TInt accessPoint = TierManagerUtils::ConvertSNAPPrefToTagIdL(tempAaccessPoint, *iDbs);
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelector %08x::\tSelectL() Using TConnPrefSnap, AP:%d"),this,accessPoint));
		aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(accessPoint));
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	if (prefs.ExtensionId() == TConnPref::EConnPrefIdList)
		{
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelector %08x::\tSelectL() Using TConnIdList"),this));
		const TConnIdList& list = static_cast<const TConnIdList&>(prefs);
		TInt count = list.Count();
		for (TInt i = 0; i < count; i++)
			{
			aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(list.Get(i)));
			}
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	//In this selector we must have the new preferences
	__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("ERROR: CIpProviderSelector %08x::\tSelectL() Unexpected selection preferences"),this));
	User::Panic(KIpSelectorPanic,EUnExpectedSelectionPreferences);
	}

CMetaConnectionProviderBase* CIpProviderSelector::FindOrCreateProviderL(TUint aAccessPoint)
    {
    ASSERT(aAccessPoint); //Should not be 0 now.
    
    CMetaConnectionProviderBase* provider = NULL;
    
    TUint selectionScope = iSelectionPrefs.Scope();
    
    if (selectionScope == TSelectionPrefs::ERequestCommsBinder)
    //create only a top level MCPr
        {
    	//Find factory
    	TUid mCprUid = TierManagerUtils::ReadMCprUidL(aAccessPoint,*iDbs);
    	CMetaConnectionProviderFactoryBase* factory = static_cast<CMetaConnectionProviderFactoryBase*>(iMetaContainer.FindOrCreateFactoryL(mCprUid));

    	//Create the provider
    	TUid tierId = TUid::Uid(iTierRecord->iRecordTag);
    	TUid tierImplUid = TUid::Uid(iTierRecord->iTierImplUid);
    	TMetaConnectionFactoryQuery query (TProviderInfo(tierId,aAccessPoint),tierImplUid);
	    
    	//We are not looking for an existng provider, we always create a new one.
		provider = static_cast<CMetaConnectionProviderBase*>(factory->CreateObjectL(query));
		//CleanupStack::PushL(provider);

		//Change the provider info so that this provider is never found by other selections
		const TProviderInfo& pi = provider->ProviderInfo();
    	//This provider will never be found
		provider->SetProviderInfo(TProviderInfo(pi.TierId(),pi.APId(),&provider));
		provider->IncrementBlockingDestroy();
        }
    else
    //this behaviour is the original one...
        {
        provider = ASimpleSelectorBase::FindOrCreateProviderL(aAccessPoint);
        }
        
    return provider;
    }

void CIpProviderSelector::RunL()
	// The dialogue has been presented.
	// Normally completes with KErrNone or KErrCancel
	// Could, however, complete with another system error e.g. KErrOutOfMemory
	{
	TInt error = iStatus.Int();
	__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelector %08x::\tRunL() Err%d Ap%d"),this, error, iSelectedAPId));
    User::LeaveIfError(error);

    ASSERT(iSelectedAPId); //Should not be 0 now.
	iSelectionNotify.SelectComplete(this,FindOrCreateProviderL(iSelectedAPId));
	iSelectionNotify.SelectComplete(this,NULL);
	}

TInt CIpProviderSelector::RunError(TInt aError)
	{
	iSelectionNotify.Error(this,aError);
	return KErrNone;
	}


//
//CLegacyPrefsSelector - handles legacy preferences by passing them down
NONSHARABLE_CLASS(CLegacyPrefsSelector) : public CBase, public ASimpleSelectorBase
/** Simple selector object for the Ip layer.

@internalComponent
@prototype
*/
	{
public:
	static const TUint KUid = 0x10283020;

public:
	explicit CLegacyPrefsSelector(const Meta::SMetaData& aSelectionPreferences)
		:	ASimpleSelectorBase(KUid, aSelectionPreferences)
		{
		}
    virtual ~CLegacyPrefsSelector()
    {}

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aAccessPoint);
	};

void CLegacyPrefsSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	ASSERT(iDbs);
	ASSERT(iTierRecord);
	TUint32 defaultAccessPoint = iTierRecord->iDefaultAccessPoint;

	//Since we do not understand the preferences, we create a provider based on a default Access Point,
	//like in the implicit case, but we also pass the preferences down.

	__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CLegacyPrefsSelector %08x::\tSelectL() Using Default AP:%d"),this,defaultAccessPoint));

	aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(defaultAccessPoint));
	aSelectionNotify.SelectComplete(this,NULL);
	}

CMetaConnectionProviderBase* CLegacyPrefsSelector::FindOrCreateProviderL(TUint aAccessPoint)
	{
	ASSERT(aAccessPoint); //Should not be 0 now.

	//Find factory
	TUid mCprUid = TierManagerUtils::ReadMCprUidL(aAccessPoint,*iDbs);
	CMetaConnectionProviderFactoryBase* factory = static_cast<CMetaConnectionProviderFactoryBase*>(iMetaContainer.FindOrCreateFactoryL(mCprUid));

	//Create the provider
	TUid tierId = TUid::Uid(iTierRecord->iRecordTag);
	TUid tierImplUid = TUid::Uid(iTierRecord->iTierImplUid);
	TMetaConnectionFactoryQuery query (TProviderInfo(tierId,aAccessPoint),tierImplUid);

	//We are not looking for an existng provider, we always create a new one.
	//This is the legacy selection (can also be the legacy attach, one layer deep selection)
	CMetaConnectionProviderBase* provider = static_cast<CMetaConnectionProviderBase*>(factory->CreateObjectL(query));
	CleanupStack::PushL(provider);

	//Change the provider info so that this provider is never found by other selections
	const TProviderInfo& pi = provider->ProviderInfo();
	provider->SetProviderInfo(TProviderInfo(pi.TierId(),pi.APId(),&provider)); //This provider will never be found

	//Override the selection policy as we always do for legacy prefs
	ASSERT(provider->AccessPointConfig().FindExtension(TOverridenSelectionPrefsExt::TypeId())==NULL);
	RMetaExtensionContainer mec;
	mec.Open(provider->AccessPointConfig());
	CleanupClosePushL(mec);
	
	TOverridenSelectionPrefsExt* ext = new (ELeave) TOverridenSelectionPrefsExt(KLegacyNextLayerTierId, iSelectionPrefs);
	CleanupStack::PushL(ext);
	mec.AppendExtensionL(ext); //The ownership of the extension goes to AccessPointConfig now.
	CleanupStack::Pop(ext);
	
	provider->AccessPointConfig().Close();
	provider->AccessPointConfig().Open(mec);
	CleanupStack::PopAndDestroy(&mec);

	CleanupStack::Pop(provider);
	provider->IncrementBlockingDestroy();
	return provider;
	}

//
//TIpProviderSelectorFactory::NewSelectorL - mapping prefs to a selector
MProviderSelector* TIpProviderSelectorFactory::NewSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("CIpProviderSelectorBase::\tNewSelectorL()")));
	ASSERT(aSelectionPreferences.IsTypeOf(TSelectionPrefs::TypeId()));
	const ESock::TSelectionPrefs& sp = static_cast<const ESock::TSelectionPrefs&>(aSelectionPreferences);
	ASimpleSelectorBase* self = NULL;
	CBase* selfCleanupItem = NULL;
	
	CMDBSession* dbs = CMDBSession::NewLC(KCDVersion1_2);
	
	// Reveal hidden or private IAP records if a licensee has chosen to protect a record
	// using one of these flags - the API to do this is public so internal components
	// have to support the use of such records.
	dbs->SetAttributeMask( ECDHidden | ECDPrivate );

	TBool prefLegacy = !sp.IsEmpty() && sp.Prefs().ExtensionId()!=TConnPref::EConnPrefSnap
		&& sp.Prefs().ExtensionId()!=TConnPref::EConnPrefIdList && sp.Prefs().ExtensionId()!=TConnPref::EConnPrefProviderInfo;

	//Legacy cfg, legacy prefs or Bravo cfg, legacy prefs (excludes Bravo implicit)
	if (prefLegacy)
		{
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("TIpProviderSelectorFactory::\tNewSelectorL() - Legacy preferences")));
		CLegacyPrefsSelector* selfLegacy = new (ELeave) CLegacyPrefsSelector(aSelectionPreferences);
		selfCleanupItem = selfLegacy;
		self = selfLegacy;
		}
	//Post 399 selection, implicit, AP, SNAP, legacy
	else
		{//AP based attch goes here
		__CFLOG_VAR((KIpTierMgrTag, KIpTierMgrSubTag, _L8("TIpProviderSelectorFactory::\tNewSelectorL() - No Mapping")));
		CIpProviderSelector* ipSelSelf = new (ELeave) CIpProviderSelector(aSelectionPreferences);
		selfCleanupItem = ipSelSelf;
		self = ipSelSelf;
		}


	CleanupStack::PushL(selfCleanupItem);
	// select TierRecord by using CIPTierManager Id not ImplId
	self->iTierRecord = TierManagerUtils::LoadTierRecordL(TUid::Uid(KAfInet),*dbs); //KAfInet =0x0800

	__ASSERT_DEBUG(static_cast<TUint32>(self->iTierRecord->iDefaultAccessPoint)!=0,User::Panic(KIpSelectorPanic,EExpectedAccessPointAwareSystem));

	CleanupStack::Pop(selfCleanupItem);
	CleanupStack::Pop(dbs);
	ASSERT(self->iDbs==NULL);
	self->iDbs = dbs;
	return self;
	}

