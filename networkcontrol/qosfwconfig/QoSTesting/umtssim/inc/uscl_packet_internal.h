// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef _USCL_PACKET_INTERNAL_H__
#define _USCL_PACKET_INTERNAL_H__

#include "uscl_packet.h"

// ******************
// FOR PACKET SERVICE
// ******************

class CPacketServiceInternalData : public CBase
    {
      friend class RPacketService;
    public:
      static CPacketServiceInternalData* NewL();
      ~CPacketServiceInternalData();

    private:
      CPacketServiceInternalData();
      void ConstructL() {}

    public:
      RSessionBase* GetServer() const;

    private:
      RUmtsSimServ* iSimServ;
      TPckg<RPacketService::TStatus>* iNotifyStatusChangePckg;
      TPckg<TInt>* iEnumerateContextsPckg1;
      TPckg<TInt>* iEnumerateContextsPckg2;
      TPckg<RPacketService::TContextInfo>* iGetContextInfoPckg;
      TPckg<TInt>* iEnumerateNifsPckg;
      TPckg<TInt>* iEnumerateContextsInNifPckg;
    };

// ******************
// FOR PACKET CONTEXT
// ******************

class CPacketContextInternalData : public CBase
    {
      friend class RPacketContext;
    public:
      static CPacketContextInternalData* NewL();
      ~CPacketContextInternalData();

    private:
      CPacketContextInternalData();
      void ConstructL() {}

    public:
      RSessionBase* GetServer() const;
      const TDesC& GetName() const;

    private:
      RSessionBase* iSimServ;
      TPckg<RPacketContext::TContextStatus>* iNotifyStatusChangePckg;
      TPckg<TInt>* iEnumeratePacketFiltersPckg;
      TBuf<65> iName; // context identifier got from server in Open*-call
    };

#endif
