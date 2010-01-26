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
//

/**
 @file
 @internalComponent
*/


#include "agenttiermanagerselector.h"
#include "agenttiermanagerfactory.h"

#include <comms-infras/ss_log.h>
#include <commsdattypesv1_1.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <es_connpref.h>	//TConnIdList

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <commsdattypesv1_1_partner.h>
#include <es_enum_internal.h>
#endif

#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrgntrm, "NifManAgtPrgntrm");
#endif

#ifdef __CFLOG_ACTIVE
#define KAgentTierMgrTag KESockMetaConnectionTag
_LIT8(KAgentTierMgrSubTag, "agenttiermgr");
#endif // __CFLOG_ACTIVE


using namespace ESock;
using namespace CommsDat;

//Panic codes
_LIT(KAgentSelectorPanic, "AgentSelector");
enum TAgentSelectorPanic
	{
	EExpectedAccessPointAwareSystem = 0,     //
	EUnExpectedSelectionPreferences = 1,
    };


//
//CAgentProviderSelector
class CAgentProviderSelector : public CBase, public ASimpleSelectorBase
/** Link connection selector.
	Simple selector object for the link layer.

@internalComponent
@prototype
*/
	{
public:
	static const TUint KUid = 0x10283022;

public:
	explicit CAgentProviderSelector(const Meta::SMetaData& aSelectionPreferences)
		:	ASimpleSelectorBase(KUid, aSelectionPreferences)
		{
		}

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	//CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aAccessPoint);
	};


void CAgentProviderSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	__ASSERT_DEBUG(iDbs, User::Panic(KSpecAssert_NifManAgtPrgntrm, 1));
	__ASSERT_DEBUG(iTierRecord, User::Panic(KSpecAssert_NifManAgtPrgntrm, 2));
	TUint32 defaultAccessPoint = iTierRecord->iDefaultAccessPoint;

	//This selector deals only with the AccessPoint aware system!
	__ASSERT_DEBUG(defaultAccessPoint!=0,User::Panic(KAgentSelectorPanic,EExpectedAccessPointAwareSystem));

	if (iSelectionPrefs.IsEmpty())
		{
    	//Implicit case on the new setup
		__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentProviderSelector %08x::\tSelectL() Using Default AP:%d"),this,defaultAccessPoint));
		aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(defaultAccessPoint));
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	const TConnPref& prefs = iSelectionPrefs.Prefs();
	if (prefs.ExtensionId() == TConnPref::EConnPrefProviderInfo)
		{
    	__ASSERT_DEBUG(iSelectionPrefs.Scope() & TSelectionPrefs::ESelectFromExisting, User::Panic(KSpecAssert_NifManAgtPrgntrm, 3)); //This is always attach
		const TConnProviderInfo& connProvInfo = static_cast<const TConnProviderInfoPref&>(prefs).Info();
		__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentProviderSelector %08x::\tSelectL() Using TConnProviderInfoPref, AP:%d"),this,connProvInfo.iInfo[1]));
		aSelectionNotify.SelectComplete(this,FindProviderL(connProvInfo.iInfo[1],(TAny*)connProvInfo.iInfo[2]));
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	if (prefs.ExtensionId() == TConnPref::EConnPrefSnap)
		{
		TUint accessPoint = static_cast<const TConnSnapPref&>(prefs).Snap();
		__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentProviderSelector %08x::\tSelectL() Using TConnPrefSnap, AP:%d"),this,accessPoint));
		aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(accessPoint));
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	if (prefs.ExtensionId() == TConnPref::EConnPrefIdList)
		{
		__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentProviderSelector %08x::\tSelectL() Using TConnIdList"),this));
		const TConnIdList& list = static_cast<const TConnIdList&>(prefs);
		TInt count = list.Count();
		for (TInt i = 0; i < count; i++)
			{
			aSelectionNotify.SelectComplete(this,FindOrCreateProviderL(list.Get(i)));
			}
		aSelectionNotify.SelectComplete(this,NULL);
		return;
		}

	//In this selector we _must_ have the new preferences, otherwise it means that
	//a critical, non-recoverable mistake has occured before when this selector has been picked.
	__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("ERROR: CAgentProviderSelector %08x::\tSelectL() Unexpected selection preferences"),this));
	User::Panic(KAgentSelectorPanic,EUnExpectedSelectionPreferences);
	}




//
// TAgentSelectorFactory::NewSelectorL - This fn matches a selector
MProviderSelector* TAgentSelectorFactory::NewSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	__CFLOG_VAR((KAgentTierMgrTag, KAgentTierMgrSubTag, _L8("CAgentMetaCprSelectorBase::\tNewL()")));
	__ASSERT_DEBUG(aSelectionPreferences.IsTypeOf(TSelectionPrefs::TypeId()), User::Panic(KSpecAssert_NifManAgtPrgntrm, 4));
	CMDBSession* dbs = CMDBSession::NewLC(KCDVersion1_2);
	ASimpleSelectorBase* self = new (ELeave) CAgentProviderSelector(aSelectionPreferences);
	CleanupStack::PushL(self);
	__ASSERT_DEBUG(self->iTierRecord==NULL, User::Panic(KSpecAssert_NifManAgtPrgntrm, 5));
	self->iTierRecord = TierManagerUtils::LoadTierRecordL(TUid::Uid(CAgentTierManagerFactory::iUid),*dbs);
	CleanupStack::Pop(self);
	CleanupStack::Pop(dbs);
	__ASSERT_DEBUG(self->iDbs==NULL, User::Panic(KSpecAssert_NifManAgtPrgntrm, 6));
	self->iDbs = dbs;
	return self;
	}


