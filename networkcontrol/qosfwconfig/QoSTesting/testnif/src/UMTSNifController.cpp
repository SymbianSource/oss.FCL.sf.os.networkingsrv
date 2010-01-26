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

#include "e32std.h"


#include "UMTSNifLink.h"
#include "UMTSNifController.h"
#include "servicenotifications.h"

#include "log-r6.h"

CUmtsNifController* CUmtsNifController::NewL(CUmtsNifIfFactory &aFactory)
    {
	CUmtsNifController *temp = new (ELeave)CUmtsNifController(aFactory);
	CleanupStack::PushL(temp);
	temp->ConstructL();
	CleanupStack::Pop();

	LOG(Log::Printf(_L("CUmtsNifController::NewL - end\n")));

	return temp;
	//return new (ELeave)CUmtsNifController(aFactory);
    }

CUmtsNifController::CUmtsNifController(CUmtsNifIfFactory &aFactory) : iFactory(aFactory)
    {
	LOG(Log::Printf(_L("CUmtsNifController::CUmtsNifController - Creating Umtsnif Controller\n")));
	
	TRAPD(err,{
    User::LeaveIfError(iPacketServer.Connect());
    User::LeaveIfError(iPacketService.Open(iPacketServer));
	});
	if(err != KErrNone)
        {
		LOG(Log::Printf(_L("Trap error from CUmtsNifController::CUmtsNifController(), %d\n"), err));
		return;
        }

	iContextCount = 0;
    }

void CUmtsNifController::ConstructL()
    {
	LOG(Log::Printf(_L("CUmtsNifController::ConstructL\n")));
	// Start service specific notification listeners
	iServiceStatusListener = new (ELeave) CEtelServiceStatusChange();
	CleanupStack::PushL(iServiceStatusListener);
	iServiceStatusListener->ConstructL(this);
	CleanupStack::Pop();
	iServiceStatusListener->Start();

	// Create the PEPs and connections to signalling proxies 
	iPEPManager = CPEPManager::NewL();

	// Link layer list init
	iLinkLayers.SetOffset(_FOFF(CUmtsNifLink, iLink)); 
	LOG(Log::Printf(_L("CUmtsNifController::ConstructL - end\n")));
    }

CUmtsNifLink* CUmtsNifController::CreateNewUmtsLinkL()
    {
	LOG(Log::Printf(_L("CUmtsNifController::CreateNewUmtsNifL Nif : Controller module creating a new Nif\n")));
	CUmtsNifLink* p = new (ELeave) CUmtsNifLink(iFactory);
	CleanupStack::PushL(p);
	p->ConstructL(this);
	CleanupStack::Pop();
	iLinkLayers.AddLast(*p);
	return p;
    }
CUmtsNifController::~CUmtsNifController()
    {
	LOG(Log::Printf(_L("CUmtsNifController::~CUmtsNifController : Destroying Umtsnif manager\n")));
	if (iServiceStatusListener)
		delete iServiceStatusListener;
	iPacketService.Close();
	iPacketServer.Close();
	delete iPEPManager;
    }


TBool CUmtsNifController::RequestNewContextResource()
    {
	if(iContextCount < 11) // Fetch this from Nif's ini file
        {
		iContextCount++;
		return ETrue;
        }
	return EFalse;	// No more room
    }

TBool CUmtsNifController::ReleaseContextResource()
    {
	__ASSERT_DEBUG(iContextCount != 0,User::Panic(_L("Invalid release!"), 0));
	iContextCount--;
	
	return ETrue;
    }
