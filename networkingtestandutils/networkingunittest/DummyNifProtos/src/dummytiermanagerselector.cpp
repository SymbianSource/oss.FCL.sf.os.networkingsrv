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


#include "dummytiermanagerselector.h"
#include "dummytiermanagerfactory.h"

#include <comms-infras/ss_log.h>
#include <commsdattypesv1_1.h>
#include <comms-infras/ss_tiermanagerutils.h>
#include <es_connpref.h>	//TConnIdList

using namespace ESock;
using namespace CommsDat;

//
//CAgentProviderSelector
class CDummyProviderSelector : public CBase, public ASimpleSelectorBase
/** Link connection selector.
	Simple selector object for the link layer.

@internalComponent
@prototype
*/
	{
public:
	explicit CDummyProviderSelector(const Meta::SMetaData& aSelectionPreferences)
	:	ASimpleSelectorBase(aSelectionPreferences)
		{
		}

private:
	virtual void SelectL(ESock::ISelectionNotify& aSelectionNotify);
	//CMetaConnectionProviderBase* FindOrCreateProviderL(TUint aAccessPoint);
	};


void CDummyProviderSelector::SelectL(ISelectionNotify& aSelectionNotify)
	{
	(void)aSelectionNotify;
	User::Leave(KErrNotSupported);
	}


//
// TAgentSelectorFactory::NewSelectorL - This fn matches a selector
MProviderSelector* TDummySelectorFactory::NewSelectorL(const Meta::SMetaData& aSelectionPreferences)
	{
	(void)aSelectionPreferences;
	User::Leave(KErrNotSupported);
	return NULL;
	}

