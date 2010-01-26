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
 @internalTechnology
 @prototype
*/

#ifndef SYMBIAN_IPPROTO_TIER_MANAGER_SELECTOR_H
#define SYMBIAN_IPPROTO_TIER_MANAGER_SELECTOR_H

//Legacy stuff
#include <commdbconnpref.h>	//TCommDbConnPref & TCommDbConnPrefMulti
#include <comms-infras/simpleselectorbase.h>

#include <comms-infras/ss_mcprnodemessages.h>


//
//TIpProtoProviderSelectorFactory
class TIpProtoProviderSelectorFactory
/**
@internalComponent
@prototype
*/
	{
public:
	static ESock::MProviderSelector* NewSelectorL(const Meta::SMetaData& aSelectionPreferences);
	};

//
//CLinkPrefsSelector
class CLinkPrefsSelector : public CSimplePromptingSelectorBase
/** Link connection selector.

	This selector may use dialog to process some or all of its preferences.
	
	It is declared in the header file only for legacy purposes (so that
	it can be accessed from the IpProto tier manager by type)

@internalComponent
@prototype
*/
	{
public:
	static const TUint KUid = 0x10283018;
	
public:
	explicit CLinkPrefsSelector(const Meta::SMetaData& aSelectionPreferences)
		:	CSimplePromptingSelectorBase(KUid, aSelectionPreferences),
			iUserPromted(EFalse)
		{
		}
	virtual ~CLinkPrefsSelector();

	/**
	 * utility methods to avoid confusion between IAP and AP
	 */
	TInt SelectedIAP() { return iSelectedAPId; }
	void SetSelectedIAP(TInt aSelectedIAP) { iSelectedAPId = aSelectedIAP; }

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	void OverrideLegacyPrefs(TCommDbConnPref& aConnPrefs, TInt aRank);
	void ProcessPrefsFromListL();
	ESock::CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aIapToFind, TUint aAccessPointToCreate);

	// From CActive
	virtual void RunL();
	virtual TInt RunError(TInt aError);

private:
	RArray<TCommDbConnPref> iPrefsList;
	TBool iUserPromted;

public:
	ESock::CMetaConnectionProviderBase* iLegacyMCpr;
	};

//
//AIpProtoSelectorBase
class AIpProtoSelectorBase : /*public CBase, */public CSimplePromptingSelectorBase
	{
public:
	AIpProtoSelectorBase()
	: CSimplePromptingSelectorBase()
	, iReselection(EFalse)
		{
		}
	AIpProtoSelectorBase(const TUint aTypeId, 
						 const Meta::SMetaData& aSelectionPreferences)
		:	CSimplePromptingSelectorBase(aTypeId, aSelectionPreferences)
		,   iReselection(EFalse)
			{
			}
	
	explicit AIpProtoSelectorBase(const Meta::SMetaData& aSelectionPreferences)
		:	CSimplePromptingSelectorBase(aSelectionPreferences)
		,   iReselection(EFalse)
			{
			}
	
	ESock::CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aIapToFind, TUint aAccessPointToCreate);

	/**
	 * utility methods to avoid confusion between IAP and AP
	 */
	TInt SelectedIAP() { return iSelectedAPId; }
	void SetSelectedIAP(TInt aSelectedIAP) { iSelectedAPId = aSelectedIAP; }
	
	ESock::CMetaConnectionProviderBase* iNetworkMCpr;
	TBool iReselection;
	
	};

class CIpProtoProviderSelectorConnPrefList : public AIpProtoSelectorBase
/** Ip Proto connection selector.

	A selector object for the Ip proto layer which takes as input RConnPrefList
	Used only to support post 399 selection on the Ip Proto layer.
	
@internalComponent
*/
	{
public:
	CIpProtoProviderSelectorConnPrefList(const ESock::RConnPrefList& aSelectionPreferences);
	
private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	
	// From CActive
	virtual void RunL();
	virtual TInt RunError(TInt aError);
	
	ESock::RConnPrefList iSelectionPrefList;
	TUid iTierId;
	TInt iLastAccessPoint;
	};


//
//CIpProtoProviderSelector
class CIpProtoProviderSelector : public AIpProtoSelectorBase
/** Ip Proto connection selector.

	Simple selector object for the Ip proto layer.
	Used only to support post 399 selection on the Ip Proto layer.
	Currently does not prompt but could prompt in the future.

@internalComponent
@prototype
*/
	{
	static const TUint KUid = 0x10283019;

public:
	explicit CIpProtoProviderSelector(const Meta::SMetaData& aSelectionPreferences)
	:	AIpProtoSelectorBase(aSelectionPreferences)
		{
		}

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	
	// From CActive
	virtual void RunL();
	virtual TInt RunError(TInt aError);
	
	void InvokeDialogL(ESock::ISelectionNotify& aSelectionNotify,
			           TUint aCprConfig);
	};

#endif //SYMBIAN_IPPROTO_TIER_MANAGER_SELECTOR_H
