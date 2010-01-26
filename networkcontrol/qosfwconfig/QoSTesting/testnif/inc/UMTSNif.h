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
 
#ifndef __UTMSNIF_H__
#define __UTMSNIF_H__

#include <in_iface.h>

#include "UMTSNiflink.h"
#include "UMTSNifdll.h"
#include "UMTSNifreqs.h"
#include "UMTSNifController.h"
#include "NifContext.h"

#include <UmtsNifControlIf.h>

#include "uscl_packet.h"
#include "uscl_qos.h"
#include "uscl_pcktcs.h"

#include "packetinterface.h"

//xxx
#include "TestIf.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#endif

const TUint KUmtsNifMajorVersionNumber = 0;
const TUint KUmtsNifMinorVersionNumber = 1;
const TUint KUmtsNifVersionNumber = 1;

const TUint KMajorVersionNumber=0;
const TUint KMinorVersionNumber=1;
const TUint KBuildVersionNumber=1;

_LIT(KDescIp, "ip");
_LIT(KDescIcmp, "icmp");
_LIT(KDescIp6, "ip6");


const TUint KMaximumNumberOfContextsPerAPN = 15; 

const TUint KSlashChar='\\'; // For CommDb access

class CNifContextManager;

class CUmtsNif :  public CNifIfBase 
    {
      friend class CUmtsNifLink;
    public:
      CUmtsNif(CUmtsNifLink* aUmtsNifLink);
      void ConstructL(CUmtsNifLink *aLinkLayer,const TDesC& aNetworkName);
	
      ~CUmtsNif();
	
      virtual void BindL(TAny *aId);
      virtual TInt State();
      virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* aSource = 0);
      virtual void Info(TNifIfInfo& aInfo) const;
      virtual TInt Send(RMBufChain& aPdu, TAny* aSource);
      static void FillInInfo(TNifIfInfo& aInfo);

      void ReceivePacket(RMBufChain& aPacket);
	
      CProtocolBase *Network() const;

      virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);

      TBool EventsOn() { return iEventsOn;}

      CUmtsNifController* Controller() { return iNifController; };
      CNifContextManager* ContextManager() { return iContextManager; };
      MNifIfNotify* Notify() { return iNotify; };
      //TInt8 NifId(void) const;	
      TName Name(void) { return iNifUniqueName; }; 
      TInt LastContextDown();
      TInt CreateStandAlonePrimaryContext();	// For new startup context 	
      void LinkLayerDown();

      TBool Down();
      inline MNifEvent& EventHandler() const;


      // xxx
      void SecondaryContextCreated();
      TUint ControlOption();
      void DropContext();

    protected:
      MNifEvent *iEventHandler;
      // Gprs / Umts 	
      TInt  CreateSecondaryContext(TDes8& aOption);	
      TInt  DeleteContext(TDes8& aOption);
      TInt  ActivateContext(TDes8& aOption);
      TInt  GetContextParameters(TDes8& aOption);
      TInt  SetContextQoS(TDes8& aOption);	
      TInt  ModifyActive(TDes8& aOption);
      //	TInt  ModifyActive1(TDes8& aOption);
      TInt  ContextTFTModify(TDes8& aOption);

	
      TInt  SetDefaultQoS(TDes8& aOption);	// QoS for new startup context 	

      CNifContextManager *iContextManager;
      CUmtsNifLink *iLinkLayer;
	
      TBool iEventsOn;
      TInt8 iPrimaryContextId;
	
      TUint32 iIapId;		// Unique for each NIF-instance
      CUmtsNifController* iNifController;	

    private:

      CProtocolBase* iNetwork;

      MNifIfUser* iNifUser;

      RPacketQoS::TQoSR5Requested iDefaultQoS;
      TBool iDefaultQoSSet;

      TName iNifUniqueName;
      TName iNetworkName;

      TBool iDownFlag;

      TDblQueLink iLink;


      // xxx
      TUint iControlOption;
      TInt8 iContextId;
      /*
          TBool iSetDefaultQoSFail;
          TBool iContextCreateFail;
          TBool iContextDeleteFail;
          TBool iContextActivateFail;
          TBool iContextModifyActiveFail;
          TBool iContextQoSSetFail;
          TBool iContextTFTModifyFail;
          */
    };
	

class CNifContextManager 
    {
      friend class CEtelContextDelete;
    public:
      CNifContextManager();					// Constructor
      ~CNifContextManager();					// Destructor
      void ConstructL(CUmtsNif *aNif);		// Second phase constructor

      TInt CreateContext(TContextId& aId);
      TInt Delete(TUint8 aContextId);			// Deletes a context with given ID	

      CNifContext* Context(TUint8 aContextId);
      TUint8 ContextCount();

      void SetCommonConfig(RPacketContext::TContextConfigGPRS& aContextConfig); 
	

      TInt GetCommonConfig(RPacketContext::TContextConfigGPRS& aContextConfig);

      void SendPrimaryContextEvent();

    private:

      TInt ReleaseContext(TInt8 aContextId);	// Used by delete
	
      CNifContext *iContextTable[KMaximumNumberOfContextsPerAPN];
      CEtelContextDelete *iContextDelete[KMaximumNumberOfContextsPerAPN];

      TUint8 iNumberOfContexts;	// The number of contexts in this Nif
      CUmtsNif *iNif;				// CNifIfBase-object
      TBool iFirst;				// Internal: Indicates whether to create a primary or secondary context
	
      RPacketContext::TContextConfigGPRS iContextConfig; // Common config data for the contexts
	
      TInt8 iLastCid;
    };


inline MNifEvent& CUmtsNif::EventHandler() const
    {
	return *iEventHandler;
    }
#endif //__UTMSNIF_H__
