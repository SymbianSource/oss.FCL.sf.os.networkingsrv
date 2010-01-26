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
 
#ifndef __UMTSNIFCONTROLLER__
#define __UMTSNIFCONTROLLER__

#include <comms-infras/nifif.h>

#include "uscl_packet.h"
#include "pepmanager.h"


class CEtelServiceNotificationRequest;

class CUmtsNif;
class CUmtsNifLink;
class CUmtsNifIfFactory;
class CUmtsNifController : public CBase
    {
      friend class CUmtsNifIfFactory;
    public:	

      TBool RequestNewContextResource();
      TBool ReleaseContextResource();

      RPacketService& PacketService() { return iPacketService; }
      RUmtsSimServ& PacketServer() { return iPacketServer; }

      RUmtsSimServ  iPacketServer;
      RPacketService iPacketService;

      CPEPManager *iPEPManager;

      RPacketService::TStatus iStatus;

    protected:
      static CUmtsNifController* NewL(CUmtsNifIfFactory &aFactory);
      CUmtsNifController(CUmtsNifIfFactory &aFactory);
      void ConstructL();
      CUmtsNifLink* CreateNewUmtsLinkL();
      ~CUmtsNifController();

      CUmtsNifIfFactory &iFactory;	

      CEtelServiceNotificationRequest* iServiceStatusListener;

      TSglQue<CUmtsNifLink> iLinkLayers; // List of link layer objects
	
      TInt8 iContextCount;
    };

#endif
