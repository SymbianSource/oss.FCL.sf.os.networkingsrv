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
 
#ifndef __UMTS_NIF_LINK__
#define __UMTS_NIF_LINK__

#include <comms-infras/nifif.h>
#include "UmtsNifControlIf.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#endif

class CUmtsNifController;
class CUmtsNif;
class CUmtsNifIfFactory;

class CUmtsNifLink : public CNifIfLink
	{
      friend class CUmtsNifController;
    public:
      CUmtsNifLink(CUmtsNifIfFactory& aFactory);
      void ConstructL(CUmtsNifController *aNifController);	
      ~CUmtsNifLink();
      virtual TInt Start();
      virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction);		
      virtual CNifIfBase* GetBinderL(const TDesC& aName);
      virtual void Info(TNifIfInfo& aInfo) const;

      inline virtual void Restart(CNifIfBase*) {};
		
      virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
      virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);  

      CUmtsNifController* Controller() { return iNifController; };

      void NifDown();
		
    protected:
	
    private:
      CUmtsNifController *iNifController;	
      TUint32 iNifIapId;
      TDblQueLink iLink;

      TDblQue<CUmtsNif> iNcpList; // List of Ncps under this link layer: Normally IPv4 and IPv6

      TBool iNotCalled;

	};
#endif
