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

#ifndef _USSE_PACKET_H__
#define _USSE_PACKET_H__

#include "uscl_packet.h"


//**********************************
// CPacketService
//**********************************
class CUmtsSimulator;
class CPacketQoS;
class CPacketService : public CBase
    {
      friend class CUmtsSimulator;
      // construct/destruct
    public:
      static CPacketService* NewL(CUmtsSimulator* aSimulator);
      ~CPacketService();
    private:
      CPacketService(CUmtsSimulator* aSimulator);
      void ConstructL();

    public:
      // FROM WRAPPER
      TUint RequestAttachment(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestAttachmentCancel(TUint aRequest);
      TUint RequestDetachment(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestDetachmentCancel(TUint aRequest);

      TUint RequestGettingStatus(void (*aCallback)(TInt aStatus, RPacketService::TStatus aSStatus, TAny* aParam), TAny* aParam);
      void RequestGettingStatusCancel(TUint aRequest);

      TUint RequestEnumeratingContexts(void (*aCallback)(TInt aStatus, TInt aCount, TInt aMax,
                                                         TAny* aParam), TAny* aParam);
      void RequestEnumeratingContextsCancel(TUint aRequest);
      TUint RequestGettingContextInfo(TInt aIndex, void (*aCallback)(TInt aStatus, const TDesC* aCName,
                                                                     RPacketContext::TContextStatus aCStatus, TAny* aParam), TAny* aParam);
      void RequestGettingContextInfoCancel(TUint aRequest);

      TUint RequestEnumeratingNifs(void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam), TAny* aParam);
      void RequestEnumeratingNifsCancel(TUint aRequest);
      TUint RequestGettingNifInfo(TInt aIndex, void (*aCallback)(TInt aStatus, const TDesC8* aInfo, TAny* aParam), TAny* aParam);
      void RequestGettingNifInfoCancel(TUint aRequest);
      TUint RequestEnumeratingContextsInNif(const TDesC* aContextName, void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam), TAny* aParam);
      void RequestEnumeratingContextsInNifCancel(TUint aRequest);
      TUint RequestGettingContextNameInNif(TInt aIndex, const TDesC* aContextName, void (*aCallback)(TInt aStatus, const TDesC* aName, TAny* aParam), TAny* aParam);
      void RequestGettingContextNameInNifCancel(TUint aRequest);

    public:
      // FROM SIMULATOR
      static void RequestAttachmentReady(TAny* aParam);
      static void RequestDetachmentReady(TAny* aParam);

      static void RequestGettingStatusReady(TAny* aParam);

      static void RequestEnumeratingContextsReady(TAny* aParam);
      static void RequestGettingContextInfoReady(TAny* aParam);

      static void RequestEnumeratingNifsReady(TAny* aParam);
      static void RequestGettingNifInfoReady(TAny* aParam);
      static void RequestEnumeratingContextsInNifReady(TAny* aParam);
      static void RequestGettingContextNameInNifReady(TAny* aParam);

      void DebugCheck(CUmtsSimulator* aSimulator); // note call order in DebugChecks!

    public:
      RPacketService::TStatus GetStatus() const { return iStatus; };
      void PossibleStatusUpdate(RPacketService::TStatus aStatus); // called by simulator when context
      // states change (suspended etc)

    private:
      void InformStatusChange();
      void InformContextAdded(const TDesC& aId);

    private:
      RPacketService::TStatus iStatus;

      CUmtsSimulator* iSimulator;
    };


//**********************************
// CPacketContext
//**********************************

class CPacketContext : public CBase
    {
      friend class CUmtsSimulator;
      // construct/destruct
    public:
      static CPacketContext* NewL(CUmtsSimulator* aSimulator);
      ~CPacketContext();

      TInt CopyFrom(const CPacketContext& aContext);
    private:
      CPacketContext(CUmtsSimulator* aSimulator);
      void ConstructL();

    public:
      void SetName(const TDesC& aName);
      const TDesC& GetName() const;

      void IncRefCount() { iRefCount++; }
      void DecRefCount() { iRefCount--; }
      TInt GetRefCount() { return iRefCount; }

    public:
      // FROM WRAPPER
      TUint RequestContextInitialisation(void (*aCallback)(TInt aStatus, const TDesC8& aChannel, TAny* aParam), TAny* aParam);
      void RequestContextInitialisationCancel(TUint aRequest);
      TUint RequestDeletion(void (*aCallback)(TAny* aParam), TAny* aParam);
      void RequestDeletionCancel(TUint aRequest);
      void FinalizeDeletion(void); // replaces RequestDeletionReady
      TUint RequestConfigurationL(TPacketDataConfigBase* aConfig, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestConfigurationCancel(TUint aRequest);
      TUint RequestGettingConfig(void (*aCallback)(TInt aStatus, const TDesC8& aConfig, TAny* aParam), TAny* aParam);
      void RequestGettingConfigCancel(TUint aRequest);
      TUint RequestActivation(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestActivationCancel(TUint aRequest);
      TUint RequestDeactivation(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestDeactivationCancel(TUint aRequest);

      TUint RequestGettingStatus(void (*aCallback)(TInt aStatus, RPacketContext::TContextStatus aCStatus, TAny* aParam), TAny* aParam);
      void RequestGettingStatusCancel(TUint aRequest);

      TUint RequestEnumeratingPacketFilters(void (*aCallback)(TInt aStatus, TInt aCount, TAny* aParam), TAny* aParam);
      void RequestEnumeratingPacketFiltersCancel(TUint aRequest);
      TUint RequestGettingPacketFilterInfo(TInt aIndex, void (*aCallback)(TInt aStatus, const TDesC8& aInfo, TAny* aParam), TAny* aParam);
      void RequestGettingPacketFilterInfoCancel(TUint aRequest);
      TUint RequestAddingPacketFilter(RPacketContext::TPacketFilterV2* aInfo, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestAddingPacketFilterCancel(TUint aRequest);
      TUint RequestRemovingPacketFilter(TInt aId, void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestRemovingPacketFilterCancel(TUint aRequest);
      TUint RequestModifyingActiveContext(void (*aCallback)(TInt aStatus, TAny* aParam), TAny* aParam);
      void RequestModifyingActiveContextCancel(TUint aRequest);

    public:
      // FROM SIMULATOR
      static void RequestContextInitialisationReady(TAny* aParam);
      // static void RequestDeletionReady(TAny* aParam); -- now called from wrapper, see FinalizeDeletion
      static void RequestConfigurationReady(TAny* aParam);
      static void RequestGettingConfigReady(TAny* aParam);
      static void RequestActivationReady(TAny* aParam);
      static void RequestActivation2ndPhaseReady(TAny* aParam);
      static void RequestDeactivationReady(TAny* aParam);

      static void RequestGettingStatusReady(TAny* aParam);

      static void RequestEnumeratingPacketFiltersReady(TAny* aParam);
      static void RequestGettingPacketFilterInfoReady(TAny* aParam);
      static void RequestAddingPacketFilterReady(TAny* aParam);
      static void RequestRemovingPacketFilterReady(TAny* aParam);
      static void RequestModifyingActiveContextReady(TAny* aParam);

      void DebugCheck(CUmtsSimulator* aSimulator); // note call order in DebugChecks!

    public:
      RPacketContext::TContextStatus GetStatus() const { return iStatus; };

      void SetPendingQoS(CPacketQoS* aQoS);
      TInt GetNegotiatedQoS(CPacketQoS*& aNegotiatedQoS);

    private:
      void InformStatusChange();
      void InformConfigChange();
      void InformProfileChange();

      void Deactivate(void);
      TInt Suspend(void);
      TInt Resume(void);
	
      TInt NegotiateQoS(void); // forms iQoSNegotiated from iQoSPending
      void DropNegotiatedQoS(void); // drops handle to iQoSNegotiated if exists
      void DropPendingQoS(void);    // same to iQoSPending

    public:
      TSglQueLink iLink;
    private:
      TInt iRefCount;
      TBuf<65> iName;
      RPacketContext::TContextStatus iStatus;
      RPacketContext::TContextConfigGPRS* iConfigGPRS;
      RPacketContext::TPacketFilterV2* iPacketFilters[8];

      CPacketQoS* iQoSNegotiated;
      CPacketQoS* iQoSPending;

      CUmtsSimulator* iSimulator;

      TBool iActivation2ndPhase;
      TUint iActivation2ndPhaseReq;
    };

#endif
