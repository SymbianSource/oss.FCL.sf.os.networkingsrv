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
// networkconfigextensionprovision.cpp
// 
//
//

#include "netcfgextprov.h"
#include <commsdattypeinfov1_1.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManNetCfgExtn, "NifManNetCfgExtP");
#endif

EXPORT_C CNetCfgExtProvision* CNetCfgExtProvision::NewL()
	{
	return new(ELeave) CNetCfgExtProvision;
	}

EXPORT_C void CNetCfgExtProvision::InitialiseConfigL(ESock::CCommsDatIapView* aIapView)
	{
	__ASSERT_DEBUG(aIapView, User::Panic(KSpecAssert_NifManNetCfgExtn, 1));

	HBufC* name;

	iIap = aIapView->IapId();
	aIapView->GetIntL(CommsDat::KCDTIdIAPNetwork, iNetworkId);

	aIapView->GetTableCommonTextFieldL(ESock::CCommsDatIapView::EConfigDaemonName, name);
	iConfigDaemonName.Copy(name->Des());
	delete name;
	name = NULL;

	aIapView->GetTableCommonTextFieldL(ESock::CCommsDatIapView::EConfigDaemonManagerName, name);
	iConfigDaemonManagerName.Copy(name->Des());
	delete name;
	name = NULL;
	}

EXPORT_C CNetCfgExtProvision::~CNetCfgExtProvision()
	{
	}


START_ATTRIBUTE_TABLE( CNetCfgExtProvision, CNetCfgExtProvision::EUid, CNetCfgExtProvision::ETypeId )
END_ATTRIBUTE_TABLE()

