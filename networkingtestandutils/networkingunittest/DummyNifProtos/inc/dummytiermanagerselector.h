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

#ifndef DUMMY_TIER_MANAGER_SELECTOR_H
#define DUMMY_TIER_MANAGER_SELECTOR_H

#include <comms-infras/simpleselectorbase.h>

//
//TAgentSelectorFactory
class TDummySelectorFactory
/**
@internalComponent
@prototype
*/
	{
public:
	static ESock::MProviderSelector* NewSelectorL(const Meta::SMetaData& aSelectionPreferences);
	};

#endif
// DUMMY_TIER_MANAGER_SELECTOR_H

