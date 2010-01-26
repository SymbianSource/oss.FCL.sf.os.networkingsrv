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

#ifndef _USSE_PACKET_WRAPPER_H__
#define _USSE_PACKET_WRAPPER_H__

#include "us_cliserv.h"
#include "uscl_packet.h"
#include "usse_server.h"
#include "usse_packet.h"


// *************************
// CPacketServiceApiWrapper
// for message delivery & wrapping
// *************************

class CPacketServiceApiWrapper : public CObject
    {
    public:
      static CPacketServiceApiWrapper* NewL(CUmtsSimServSession* aSession);
      ~CPacketServiceApiWrapper();
      void CloseWrapper();

    private:
      CPacketServiceApiWrapper(CUmtsSimServSession* aSession);
      void ConstructL();

    public:
      const RMessage2& Message() const;

    public:
      // FROM ETEL PACKET DATA API / PACKET SERVICE
      // NOTE: Some of these methods may leave!
      void AttachA();
      void AttachACancel();
      void DetachA();
      void DetachACancel();

      void GetStatusS();
      void NotifyStatusChangeA();
      void NotifyStatusChangeACancel();

      void NotifyContextAddedA();
      void NotifyContextAddedACancel();

      void EnumerateContextsA();
      void EnumerateContextsACancel();
      void GetContextInfoA();
      void GetContextInfoACancel();

      void EnumerateNifsA();
      void EnumerateNifsACancel();
      void GetNifInfoA();
      void GetNifInfoACancel();
      void EnumerateContextsInNifA();
      void EnumerateContextsInNifACancel();
      void GetContextNameInNifA();
      void GetContextNameInNifACancel();

    public:
      // FROM SIMULATOR
      static void SimAttachAReady(TInt aStatus, TAny* aSelf);
      static void SimDetachAReady(TInt aStatus, TAny* aSelf);

      static void SimGetStatusSReady(TInt aStatus, RPacketService::TStatus aSStatus, TAny* aSelf);

      void SimStatusUpdated(RPacketService::TStatus aStatus);
      void SimContextAdded(const TDesC& aId);

      static void SimEnumerateContextsAReady(TInt aStatus, TInt aCount, TInt aMax, TAny* aSelf);
      static void SimGetContextInfoAReady(TInt aStatus, const TDesC* aCName,
                                          RPacketContext::TContextStatus aCStatus, TAny* aSelf);

      static void SimEnumerateNifsAReady(TInt aStatus, TInt aCount, TAny* aSelf);
      static void SimGetNifInfoAReady(TInt aStatus, const TDesC8* aInfo, TAny* aSelf);
      static void SimEnumerateContextsInNifAReady(TInt aStatus, TInt aCount, TAny* aSelf);
      static void SimGetContextNameInNifAReady(TInt aStatus, const TDesC* aName, TAny* aSelf);

      // note call order in DebugChecks!
      void DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession);

    private:
      // the following make it possible to attach and detach, async and sync, all at the same time
      // -- probably that doesn't make too much sense :)
      TBool iAttachA;
      TUint iAttachAReq;
      RMessage2 iAttachAMessage;
      TBool iDetachA;
      TUint iDetachAReq;
      RMessage2 iDetachAMessage;

      TBool iGetStatusS;
      TUint iGetStatusSReq;
      RMessage2 iGetStatusSMessage;

      TBool iNotifyStatusA;
      RMessage2 iNotifyStatusAMessage;

      TBool iNotifyContextAddedA;
      RMessage2 iNotifyContextAddedAMessage;

      TBool iEnumerateContextsA;
      TBool iGetContextInfoA;
      TUint iEnumerateContextsAReq;
      TUint iGetContextInfoAReq;
      RMessage2 iEnumerateContextsAMessage;
      RMessage2 iGetContextInfoAMessage;

      TBool iEnumerateNifsA;
      TBool iGetNifInfoA;
      TBool iEnumerateContextsInNifA;
      TBool iGetContextNameInNifA;
      TUint iEnumerateNifsAReq;
      TUint iGetNifInfoAReq;
      TUint iEnumerateContextsInNifAReq;
      TUint iGetContextNameInNifAReq;
      RMessage2 iEnumerateNifsAMessage;
      RMessage2 iGetNifInfoAMessage;
      RMessage2 iEnumerateContextsInNifAMessage;
      RMessage2 iGetContextNameInNifAMessage;

    protected:
      CUmtsSimServSession* iSession; //session owning us
      CUmtsSimulator* iSimulator;    //umts simulator state machine
    };

// *************************
// CPacketContextApiWrapper
// for message delivery & wrapping
// *************************

class CPacketContextApiWrapper : public CObject
    {
    public:
      static CPacketContextApiWrapper* NewL(CUmtsSimServSession* aSession);
      ~CPacketContextApiWrapper();
      void CloseWrapper();

    private:
      CPacketContextApiWrapper(CUmtsSimServSession* aSession);
      void ConstructL(TUmtsSimServContextOpenMode aMode, const TAny* aName, const TAny* aPtr);

    public:
      const RMessage2& Message() const;

    public:
      // FROM ETEL PACKET DATA API / PACKET CONTEXT
      // NOTE: Some of these methods may leave!
      void InitialiseContextA();
      void InitialiseContextACancel();

      void DeleteA();
      void DeleteACancel();

      void SetConfigA();
      void SetConfigACancel();
      void GetConfigA();
      void GetConfigACancel();

      void NotifyConfigChangedA();
      void NotifyConfigChangedACancel();

      void ActivateA();
      void ActivateACancel();
      void DeactivateA();
      void DeactivateACancel();

      void GetStatusS();
      void NotifyStatusChangeA();
      void NotifyStatusChangeACancel();

      void EnumeratePacketFiltersA();
      void EnumeratePacketFiltersACancel();
      void GetPacketFilterInfoA();
      void GetPacketFilterInfoACancel();
      void AddPacketFilterA();
      void AddPacketFilterACancel();
      void RemovePacketFilterA();
      void RemovePacketFilterACancel();
      void ModifyActiveContextA();
      void ModifyActiveContextACancel();

    public:
      // FROM SIMULATOR
      static void SimInitialiseContextAReady(TInt aStatus, const TDesC8& aChannel, TAny* aSelf);

      static void SimDeleteAReady(TAny* aSelf);

      static void SimSetConfigAReady(TInt aStatus, TAny* aSelf);
      static void SimGetConfigAReady(TInt aStatus, const TDesC8& aConfig, TAny* aSelf);
	
      static void SimActivateAReady(TInt aStatus, TAny* aSelf);
      static void SimDeactivateAReady(TInt aStatus, TAny* aSelf);
	
      static void SimGetStatusSReady(TInt aStatus, RPacketContext::TContextStatus aCStatus, TAny* aSelf);

      void SimStatusUpdated(CPacketContext* aContext, RPacketContext::TContextStatus aStatus);
      void SimConfigChanged(CPacketContext* aContext, const TDesC8& aConfig);

      static void SimEnumeratePacketFiltersAReady(TInt aStatus, TInt aCount, TAny* aSelf);
      static void SimGetPacketFilterInfoAReady(TInt aStatus, const TDesC8& aInfo, TAny* aSelf);
      static void SimAddPacketFilterAReady(TInt aStatus, TAny* aSelf);
      static void SimRemovePacketFilterAReady(TInt aStatus, TAny* aSelf);
      static void SimModifyActiveContextAReady(TInt aStatus, TAny* aSelf);

      // note call order in DebugChecks!
      void DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession);

    private:
      void LogWithName(const TDesC& aMsg); 
      TInt TrappedMethodL(RMessage2& aMsg, TInt aLength);

    private:
      // following stuff permit doing everything (grazy), but costs memory
      // probably should enforce sanity on client requests => could remove most of the stuff here
      TBool iInitialiseContextA;
      TUint iInitialiseContextAReq;
      RMessage2 iInitialiseContextAMessage;

      TBool iDeleteA;
      TUint iDeleteAReq;
      RMessage2 iDeleteAMessage;

      TBool iSetConfigA;
      TUint iSetConfigAReq;
      RMessage2 iSetConfigAMessage;
      TBool iGetConfigA;
      TUint iGetConfigAReq;
      RMessage2 iGetConfigAMessage;

      TBool iNotifyConfigChangedA;
      RMessage2 iNotifyConfigChangedAMessage;

      TBool iActivateA;
      TUint iActivateAReq;
      RMessage2 iActivateAMessage;
      TBool iDeactivateA;
      TUint iDeactivateAReq;
      RMessage2 iDeactivateAMessage;

      TBool iGetStatusS;
      TUint iGetStatusSReq;
      RMessage2 iGetStatusSMessage;

      TBool iNotifyStatusA;
      RMessage2 iNotifyStatusAMessage;

      TBool iEnumeratePacketFiltersA;
      TBool iGetPacketFilterInfoA;
      TBool iAddPacketFilterA;
      TBool iRemovePacketFilterA;
      TBool iModifyActiveContextA;
      TUint iEnumeratePacketFiltersAReq;
      TUint iGetPacketFilterInfoAReq;
      TUint iAddPacketFilterAReq;
      TUint iRemovePacketFilterAReq;
      TUint iModifyActiveContextAReq;
      RMessage2 iEnumeratePacketFiltersAMessage;
      RMessage2 iGetPacketFilterInfoAMessage;
      RMessage2 iAddPacketFilterAMessage;
      RMessage2 iRemovePacketFilterAMessage;
      RMessage2 iModifyActiveContextAMessage;

    protected:
      CUmtsSimServSession* iSession; //session owning us
      CUmtsSimulator* iSimulator;    //umts simulator state machine
      CPacketContext* iContext;      //our context
    };

#endif
