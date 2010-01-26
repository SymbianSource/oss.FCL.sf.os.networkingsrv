// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <E32def.h>
#include <E32std.h>
#include "UMTSNifdll.h"
#include "UMTSNif.h"
#include "UMTSNifController.h"

#include "log-r6.h"

extern "C" { IMPORT_C CNifFactory *Install(); }

EXPORT_C CNifFactory *Install()
	{
	return new (ELeave) CUmtsNifIfFactory;
	}

//
// DLL entry point
//

//
// DLL 
//
TInt CUmtsNifIfFactory::Info(TNifIfInfo& aInfo, TInt aIndex) const
	{		
	// Should there be something here? Nobody seems to call this..
	// 
	__ASSERT_DEBUG(0, User::Panic(_L("Factory onfo called!"), 0));
	switch(aIndex)
        {
            case 0:
                CUmtsNif::FillInInfo(aInfo);
                break;
        }
	return 1;
	}

void CUmtsNifIfFactory::InstallL()
	{
	iController = CUmtsNifController::NewL(*this);
	}

CNifIfBase* CUmtsNifIfFactory::NewInterfaceL(const TDesC& aName)
	{
	__ASSERT_DEBUG(iController, User::Panic(_L("No Nif controller"), 0));	

	_LIT(KNifName, "testnif"); // Change once whole Nif is re-baptized to umtsnif
	if(aName.CompareF(KNifName))
		User::Leave(KErrNotSupported);

	CUmtsNifLink* newInterface = iController->CreateNewUmtsLinkL();

	return newInterface;		
	}

CUmtsNifIfFactory::~CUmtsNifIfFactory()
    {
	LOG(Log::Printf(_L("CUmtsNifIfFactory::~CUmtsNifIfFactory : Nif factory going down \n")));
	delete iController; // Remove common Nif -managementlayer
    }
