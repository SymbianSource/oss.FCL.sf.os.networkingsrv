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

#include "ServiceNotifications.h"
#include "UMTSNifController.h"

#include "log-r6.h"


CEtelServiceNotificationRequest::CEtelServiceNotificationRequest() : CActive(EPriorityStandard)
    {}

CEtelServiceNotificationRequest::~CEtelServiceNotificationRequest()
    {	
	if(IsActive())
		Cancel();
    }
void CEtelServiceNotificationRequest::DoCancel()
    {
	iNifController->PacketService().CancelAsyncRequest(iNotifierCode); // Temporarily commented
	iStatus=KErrNone;	
    }

void CEtelServiceNotificationRequest::ConstructL(CUmtsNifController *aNifController)
    {
	CActiveScheduler::Add(this); 	
	iNifController = aNifController;
    }


CEtelServiceStatusChange::CEtelServiceStatusChange() : iServiceStatusPtr(iServiceStatus)
    {
	iNotifierCode = EPacketNotifyStatusChange;
    }

void CEtelServiceStatusChange::RunL()
    {		
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelServiceStatusChange::RunL() START\n")));
#endif
	iNifController->iStatus = iServiceStatus;
	
	// Contoller should take action if necessary
	
	iNifController->PacketService().NotifyStatusChange(iStatus,iServiceStatus);	
	SetActive();
#ifdef _RUNL_DEBUG
	LOG(Log::Printf(_L("CEtelServiceStatusChange::RunL() STOP\n")));
#endif
	return;
    }

void CEtelServiceStatusChange::Start()
    {	
	LOG(Log::Printf(_L("CEtelServiceStatusChange::Start(): starting PacketService status listener\n")));
	iNifController->PacketService().NotifyStatusChange(iStatus,iServiceStatus);
	SetActive();

	return;
    }
