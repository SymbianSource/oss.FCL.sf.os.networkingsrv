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
// usse_simulator.h - header for umts simulator state machine
//

#ifndef _USSE_SIMULATOR_H__
#define _USSE_SIMULATOR_H__

#define USSE_DEBUG_CHECKS

#include "usse_packet.h"
#include "usse_server.h"
#include "usse_qos.h"

// 11 here is supposedly defined in some documentation
const TInt KUMTSSimMaxContexts = 11;

_LIT(KUMTSSimConfFile, "Y:\\system\\data\\UmtsSim_Server_simconf.ini");
_LIT(KUMTSSimVersion, "UMTSSim version 0.9.0 beta 3");

//**********************************
// TSimNifList
//**********************************

class TSimNifList
    {
    private:
      struct TContextEntry
          {
            TContextEntry* iNext;
            const CPacketContext *iContext;
          };
      struct TListEntry
          {
            TListEntry* iNext;
            TContextEntry* iContexts;
            TInt iContextCount;
          };
    public:
      TSimNifList();
      ~TSimNifList();

      void AddNewPrimary(const CPacketContext* aContext);
      void AddNewSecondary(const CPacketContext* aContext, const CPacketContext* aFather);
      void Remove(const CPacketContext* aContext);

      TInt GetNifCount();
      TInt GetContextCount(const CPacketContext* aContext); // in nif specified by parameter

      TInt GetNifInfo(TInt aIndex, RPacketService::TNifInfoV2& aNifInfo);
      TInt GetContextName(TInt aIndex, TDes& aName, const CPacketContext* aContext); // in nif specified by parameter

    private:
      TListEntry* iNifs;
      TInt iNifCount;
    };

//**********************************
// CUmtsSimulator
//**********************************

class CSimTimer;
class CSimRequestManager;
class CSimEventController;
class CUmtsSimulator : public CBase
    {
      // types
    public:
      typedef void (*TCallback)(TAny*);
      enum TUmtsRequestStatus { EURStatusNormal = 0, EURStatusFail, EURStatusConfirm };

    public:
      // construct/destruct
      static CUmtsSimulator* NewL();
      ~CUmtsSimulator();
    private:
      void ConstructL();
      CUmtsSimulator();

      static void RequestShutdown(TAny *aSelf);

    public:
      CPacketService* GetPacketService() const; // cannot fail
      TInt GetPacketContext(const TDesC& aName, CPacketContext*& aContext, TBool aRefCount) const;
      TInt NewPrimaryPacketContext(TDes& aName, CPacketContext*& aContext);
      TInt NewSecondaryPacketContext(const TDesC& aOldContext, TDes& aName, CPacketContext*& aContext);
      TInt RemovePacketContext(const TDesC& aName);
      TInt GetPacketQoS(const TDesC& aContext, const TDesC& aName, CPacketQoS*& aQoS, TBool aRefCount) const;
      TInt NewPacketQoS(const TDesC& aContext, TDes& aName, CPacketQoS*& aQoS);
      TInt RemovePacketQoS(const TDesC& aContext, const TDesC& aName);

      void ContextStateChanged(void); // method to be called by any CPacketContext when status changes
      // (handles related packet service status changes)

      TSimNifList& GetNifList();

      TSglQueIter<CUmtsSimServSession>& AllocSessionIterator();
      void FreeSessionIterator(TSglQueIter<CUmtsSimServSession>& aIterator);

      TSglQueIter<CPacketContext>& AllocContextIterator();
      void FreeContextIterator(TSglQueIter<CPacketContext>& aIterator);

      void AddSession(CUmtsSimServSession& aSession);
      void RemoveSession(CUmtsSimServSession& aSession);

      CSimTimer* GetSimTimer() { return iSimTimer; }

      // For Reconfigure* -methods aFilename should contain at least path and filename. If it also contains
      // drive letter, then that drive is searched first, but if not found, other drives also in standard
      // order (Y-A, Z). The return code is informational and requires no action.
      TInt ReconfigureReqMan(const TDesC& aFilename);
      TInt ConfigureRequestHandler(const TDesC& aCfg); // (re)configure single handler
      TUmtsRequestStatus CheckRequest(TUint aRequestCode, TInt& aReturnStatus, TInt& aDelay_ms);

      // For Reconfigure* -methods aFilename should contain at least path and filename. If it also contains
      // drive letter, then that drive is searched first, but if not found, other drives also in standard
      // order (Y-A, Z). The return code is informational and requires no action.
      TInt ReconfigureEventCtrl(const TDesC& aFilename);
      TInt ConfigureEvent(const TDesC& aCfg); // add single event
      CSimEventController* GetEventCtrl();

      void Log(const TDesC& aEntry);
      void Log(const TDesC& aEntry, const TDesC& aData);

      void DebugCheck(void); // note call order in DebugChecks!

    private:
      void SafeLog(const TDesC& aEntry);
      TInt DoNewPacketContext(TDes& aName, CPacketContext*& aContext);
      TInt RemoveQoSOfContext(CPacketContext* aContext);
      TInt FindFile(const TDesC& aHintPathAndFile, TDes& aFullOrErrorMsg);

    public:
      // simulation methods (actions for event controller)
      TInt ActDeactivateContext(const TDesC* aParams, TAny* aOptionalParam);
      TInt ActSuspendContext(const TDesC* aParams, TAny* aOptionalParam);
      TInt ActResumeContext(const TDesC* aParams, TAny* aOptionalParam); // re-activate suspended context
      TInt ActConfigureRequest(const TDesC* aCfgLine, TAny* aOptionalParam); // (re)configure request handler
      TInt ActSetEventTrigger(const TDesC* aParams, TAny* aOptionalParam);   // redefine what event triggers
      TInt ActDoNothing(const TDesC*, TAny*);

    private:
      CPacketService* iPacketService;
      TSglQue<CPacketContext> iPacketContexts;
      TSglQue<CPacketQoS> iPacketQoSs;
      TSglQue<CUmtsSimServSession> iSessions;

      TBool iContextNamePool[KUMTSSimMaxContexts];
      TInt iQoSNamePool;

      TSimNifList iNifs;
      CSimTimer* iSimTimer;
      CSimRequestManager* iRequestManager;
      CSimEventController* iEventController;

      TUint iLogIndex;

      TBool iShutdownActive;
      TInt iShutdownRequest;
    };


//**********************************
// CSimTimer
//**********************************

class CSimTimer : public CTimer
    {
      class CRequestQueue;
      friend class CRequestQueue;
      // types
    private:
      // *** TRequest
      struct TRequest
          {
            friend class CRequestQueue;

            TTime iTime;							// when
            CUmtsSimulator::TCallback iCallback;	// what
            TAny* iParam;

            TUint GetHandle() { return iHandle; };
            TBool operator<(const TRequest& aRequest);
          private:
            TRequest* iNext;
            TUint iHandle;
          };

      // *** CRequestQueue
      class CRequestQueue : public CBase
          {
          public:
            static CRequestQueue* NewL();
            ~CRequestQueue();
          protected:
            CRequestQueue();
            void ConstructL();

          public:
            TUint AddL(TRequest* aRequest);
            TRequest* GetFirst();
            TRequest* GetRequest(TUint aHandle);
            TRequest* Remove(TUint aHandle);

          private:
            TRequest* iRequests; // will point to element before first one
            TUint iCount; // all time count
          };


      // construction
    public:
      static CSimTimer* NewL();
      ~CSimTimer();
    protected:
      CSimTimer();
      void ConstructL();

      // request api
    public:
      // RequestCallAfterL guarantees that given callback is executed later asynchronously
      // (meaning that call to RequestCallAfterL returns before callback is called)
      TInt RequestCallAfterL(TTimeIntervalMicroSeconds32 aPeriod, CUmtsSimulator::TCallback aCallback, TAny* aParam);
      TAny* RemoveRequest(TInt aRequest);

      // from CTimer (make protected)
    protected:
      void After(TTimeIntervalMicroSeconds32 aInterval) { CTimer::After(aInterval); }
      void At(const TTime& aTime) { CTimer::At(aTime); }

      // from CActive
    public:
      void RunL();

    private:
      CRequestQueue* iRequests;
    };


//**********************************
// CSimRequestManager
//**********************************

class CSimRequestManager : public CBase
    {
      // types
    public:
      class CRequestHandler : public CBase
          {
            // types
          protected:
            enum THandlerStatus { EHStatusDefault = 0, EHStatusPermit, EHStatusRefuse, EHStatusConfirm };

            // construction
          public:
            static CRequestHandler* NewL(TUint aRequestCode, CUmtsSimulator* aSimulator);
            virtual ~CRequestHandler();
          protected:
            CRequestHandler(TUint aRequestCode, CUmtsSimulator* aSimulator);
            void ConstructL(void);

          public:
            // Request() tells how server should answer to client
            virtual CUmtsSimulator::TUmtsRequestStatus Request(TInt& aReturnStatus, TInt& aDelay_ms);
            // Following methods modify state of the handler
            virtual void Skip(TInt aN); // Skip() is equivalent to calling Request aN times
            virtual void Lock(void);    // Lock() prevents Request() and Skip() from modifying handler state
            virtual void Unlock(void);  // Unlock() allows handler state to be modified by Request() and Skip()
            virtual void Permit(void);  // Permit() forces requests to succeed until further command
            virtual void Refuse(void);  // Refuse() forces requests to fail until further command
            virtual void Confirm(void); // Confirm() forces requests to be confirmed thru Control api
            virtual void Default(void); // Default() cancels any previous Permit, Refuce or Confirm calls
            virtual void SetDelay(TInt aDelay_ms); // sets delay to be returned in Request()
            virtual void SetProbability(TReal aProb); // sets probability of success for default-mode
            virtual void SetFailStatus(TInt aCode); // sets status given on failed request
            virtual void Triggers(TInt aId); // makes request trigger (or cancel if aId < 0) specified event
            // (use aId == 0 to cancel triggering)

            virtual TBool CheckForMatch(TUint aRequestCode);

          protected:
            TUint iRequestCode;
            THandlerStatus iHandlerStatus;
            TBool iLocked;
            TInt iDelay; // in milliseconds
            TReal iProbability;
            TInt64 iRandSeed;
            TInt iFailStatus;
            TInt iTriggerId;

            CUmtsSimulator* iSimulator;
          };

      // construction
    public:
      static CSimRequestManager* NewL(CUmtsSimulator* aSimulator);
      ~CSimRequestManager();
    protected:
      CSimRequestManager(CUmtsSimulator* aSimulator);
      void ConstructL(void);

      void Clear(void);

    public:
      // Configure reads manager settings from file. Note that all state is lost and existing
      // handlers are all deleted. On failure Configure leaves manager to clear state.
      TInt Configure(const TDesC& aFilename);
      TInt HandleRequestLine(const TDesC& aLine);

      CRequestHandler* GetDefaultHandler(void) const;
      void SetDefaultHandler(CRequestHandler* aHandler); // passes ownership to manager

      // It should be noticed that GetHandler doesn't return DefaultHandler if there is
      // no specialized handler for given request, but returns NULL. This is to prevent
      // unwanted behaviour when (possibly) modifying the returned handler.
      CRequestHandler* GetHandler(TUint aRequestCode);

      // SetHandler passes ownership to manager. Use it also to remove handler by passing NULL.
      void SetHandler(TUint aRequestCode, CRequestHandler* aHandler);

    protected:
      TInt GetHandlerIndex(TUint aRequestCode);

      TInt GetRequestCode(const TDesC& aName, TUint& aCode);

    protected:
      RPointerArray<CRequestHandler> iHandlers;
      CRequestHandler* iDefaultHandler;
      CUmtsSimulator* iSimulator;
    };


//**********************************
// CSimEventController
//**********************************

class CSimEventController : public CBase
    {
      // types
    public:
      typedef TInt (CUmtsSimulator::* TAction)(const TDesC* aParams, TAny* aOptionalParam);
    protected:
      class CEvent : public CBase
          {
          public:
            static CEvent* NewL(TAction aAction, CUmtsSimulator* aSimulator);
            virtual ~CEvent(); // guaranteed to cancel everything
          protected:
            CEvent(TAction aAction, CUmtsSimulator* aSimulator);
            void ConstructL(void);

          public:
            void SetIdL(TInt aId); // leaves if aId < 1
            TInt GetId() { return iId; }

            void SetParams(const TDesC& aParams); // parameters given to action
            void SetDelay(TInt aDelay_ms);		  // delay from trigger to action
            void Triggers(TInt aId); // makes evewnt trigger (or cancel if aId < 0) specified event
            // (use aId == 0 to cancel triggering)
            TInt Trigger(TAny* aOptionalParam);   // trigger this event giving extra parameters (or NULL)
            void Cancel();						  // cancels previous Trigger()-call

          public:
            // called from timer
            static void Callback(TAny* aSelf);

          protected:
            TInt iId;
            TAction iAction;
            HBufC* iParams;
            TAny* iOptionalParam;

            TBool iActive;
            TInt iRequest;
            TInt iDelay_ms;
            TInt iTriggerId;

            CUmtsSimulator* iSimulator;
          };

      // construction
    public:
      static CSimEventController* NewL(CUmtsSimulator* aSimulator);
      ~CSimEventController();
    protected:
      CSimEventController(CUmtsSimulator* aSimulator);
      void ConstructL(void);

      void Clear(void);

    public:
      // Configure reads controller settings from file. Note that all state is lost and existing
      // events are all deleted. On failure Configure leaves controller to clear state.
      TInt Configure(const TDesC& aFilename);
      TInt HandleEventLine(const TDesC& aLine);
      TInt SetTrigger(TInt aEventId, TInt aTriggerId); // sets/cancels event trigger/cancel another

      TInt Trigger(TInt aId, TAny* aOptionalParam);
      TInt Cancel(TInt aId);

    protected:
      TInt AddEvent(CEvent* aEvent); // passes ownership
      TInt GetEvent(TInt aId, CEvent*& aEvent);

      TInt GetAction(const TDesC& aName, TAction& aAction);

    protected:
      RPointerArray<CEvent> iEvents;
      CUmtsSimulator* iSimulator;
    };

#endif
