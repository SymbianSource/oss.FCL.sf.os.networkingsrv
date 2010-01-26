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
// service_notifications.h
//
 
#ifndef _SERVICE_NOTIFICATIONS_
#define _SERVICE_NOTIFICATIONS_

#include <e32base.h>

#include "uscl_packet.h"
#include "uscl_qos.h"
#include "uscl_pcktcs.h"
class CUmtsNifController;

class CEtelServiceNotificationRequest : public CActive 
    {
    public:
      CEtelServiceNotificationRequest();
      ~CEtelServiceNotificationRequest();	
      void ConstructL(CUmtsNifController *aNifController);
      virtual void Start() = 0;
    protected:
	
      void DoCancel();	

      CUmtsNifController *iNifController;
      TUmtsSimServRqstCancels iNotifierCode;
    };

// Configuration change notification listener
class CEtelServiceStatusChange : public CEtelServiceNotificationRequest
    {
    public:
      CEtelServiceStatusChange();
      void Start();
    private:
      void RunL();	
	
      TPckg<RPacketService::TStatus> iServiceStatusPtr;
      RPacketService::TStatus iServiceStatus;	
    };


#endif
