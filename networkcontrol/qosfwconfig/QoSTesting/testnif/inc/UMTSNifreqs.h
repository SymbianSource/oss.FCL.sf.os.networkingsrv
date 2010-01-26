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
 
#ifndef __UMTSREQS_H__
#define __UMTSREQS_H__

#include "umtsnif.h"

#include "log-r6.h"

#include "uscl_packet.h"
#include "uscl_qos.h"
#include "uscl_pcktcs.h"

#ifdef SYMBIAN_NETWORKING_UMTSR5
#include <umtsextn.h>
#else
#include <umtsapi.h>
#endif // SYMBIAN_NETWORKING_UMTSR5


class CUmtsNif;
class CNifContext;
class CEtelSubRequest;
class CEtelRequest : public CActive 
    {
      friend class CEtelSubRequest;
    public:
      CEtelRequest();
      ~CEtelRequest();
	
      static CEtelRequest *NewL();
      void ConstructL(CNifContext *aContext);

      //TContextReasonCode IssueRequestL(TContextParameters *aParameters,TInt8 aOperation);
      TInt IssueRequestL(TContextParameters *aParameters,TInt8 aOperation);
	
      CNifContext *iContext;
      TBool Free() { return iFree; };

      inline void SubrequestReady();

      inline TInt8 Operation() const;
      
      // for ims context
      void CreateFileIMS();
      // for umtsr5 context
      void CreateFileUMTSR5();

    protected:	
      void DoCancel();
      void RunL();

      // Handlers to build Response events to GUQoS-module
      void HandleDeleteRequestResponse();
      void HandleActivateRequestResponse();
	
      void HandleContextQoSSetRequestResponse();
      void HandleContextTFTModifyRequestResponse();
      void HandleModifyActiveRequestResponse();
      // 
      void HandleStartupPrimaryContextRequestResponse();
	
      void HandleContextCreateSecondPhaseRequestResponse();

	
      void PrintEvent(TUint aEventCode, const TContextParameters& aParameters);
      void PrintEventType(TUint aEventCode, const TContextParameters& aParameters);
      void PrintContextInfo(const TContextInfo& aContextInfo);
      void PrintContextConfig(const TContextConfig& aContextConfig);
      void PrintContextNegotiatedQoS(TUint aEventCode, const TContextParameters& aParameters);
      void PrintContextTFT(TUint aEventCode, const TContextParameters& aParameters);

      void FillEvent(TContextParameters &aEvent);
      void FillEventQoS(TContextParameters &aEvent);
      void FillEventTFT(TContextParameters &aEvent);
	
	

    private:
	
      enum THandlerState // Internal states types for this object 
          {
          KHandlerFetchParameters,
          KHandlerBuildResponse
          };	
      THandlerState iHandlerState;

      TInt8 iOperation;	
	
      TBool iFree;						// Should this be a semaphore?
		
      CEtelSubRequest		*iSubRequest;	// Subrequest started to wrap multiple 
      // asynchronous calls to Etel
      TContextParameters	*iResponse;		// For active objects serving this object to fill

    };

// Parent class for all objects wrapping more than one Etel-request
class CEtelSubRequest : public CActive
    {
      friend class CEtelRequest;
    public:
      CEtelSubRequest();
      ~CEtelSubRequest();
      void ConstructL(CEtelRequest *aParent);	 // Make object ready for DoTask() call	
    protected:
      void DoCancel();
      virtual void DoTask(TContextParameters *aContextParameters) = 0;// Starts the request group execution
      void JumpToRunl(TInt aError);
    public:
      CEtelRequest *iParent;
      TRequestStatus *iCallerStatus;

      TContextParameters *iContextParameters;
	
      TUmtsSimServRqstCancels iLastCancelCode; // For cancelling

      RPacketContext::TDataChannelV2 iGPDSContextId;
      RPacketContext::TDataChannelV2Pckg iGPDSContextIdPtr;
    };


// State machine to fetch context parameters
class CEtelGetParameters : public CEtelSubRequest 
    {
    public:
      CEtelGetParameters();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();
		
      enum TRequestState // Internal states types for this object 
          {
          EConfigState,		
          EQoSState,
          ETFTState		
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request

      // UTMS Configuration data
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;	
      // UMTS Negotiated QoS data
      TPckg<RPacketQoS::TQoSR5Negotiated> iUMTSQoSNegPtr;
      RPacketQoS::TQoSR5Negotiated iUMTSQoSNeg;
    };

// State machine to set context config 
class CEtelSetConfig : public CEtelSubRequest 
    {
    public:
      CEtelSetConfig();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();

      enum TRequestState // Internal states types for this state machine
          {
          ESetConfigState,
          EGetConfigState
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request

      // UTMS Configuration data, Structure and Pointer
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;	

    };
// State machine to set context config 
class CEtelSetQoS : public CEtelSubRequest 
    {
    public:
      CEtelSetQoS();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();

      enum TRequestState // Internal states types for this object 
          {
          ESetQoSState,
          EGetConfigState,
          EGetQoSState
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request

      // UTMS Requested QoS
      TPckg<RPacketQoS::TQoSR5Requested> iUMTSQoSReqPtr;
      RPacketQoS::TQoSR5Requested iUMTSQoSReq;
    };


// State machine to set context config 
class CEtelContextActivate : public CEtelSubRequest 
    {
    public:
      CEtelContextActivate();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();
	
      enum TRequestState // Internal states types for this object 
          {
          EGetConfigState,
          EGetQoSState
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request	

      // UTMS Configuration data
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;	

      // UTMS Negotiated QoS
      TPckg<RPacketQoS::TQoSR5Negotiated> iUMTSQoSNegPtr;
      RPacketQoS::TQoSR5Negotiated iUMTSQoSNeg;

      // Context status
      RPacketContext::TContextStatus iContextStatus;
    };


class CEtelContextModifyActive : public CEtelSubRequest 
    {
    public:
      CEtelContextModifyActive();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();
    };

class CEtelContextCreateSecondPhase : public CEtelSubRequest 
    {
    public:
      CEtelContextCreateSecondPhase();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();

      enum TRequestState // Internal states types for this object 
          {
          EInitializeContextState	
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request	

    };

class CEtelContextTFTOperation : public CEtelSubRequest 
    {
    public:
      CEtelContextTFTOperation();
      void CreateFileSblpAdded();
      void CreateFileIMS();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();

	
      enum TRequestState 
          {
          KDoTFTTask,
          KDoTFTFetch
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request	
	
      enum TTFTDeleteSubRequestState // 
          {
          ERetrieveFilterInfoState,
          EDeleteFilterState
          };
      TTFTDeleteSubRequestState iTFTDeleteSubRequestState;

      enum TTFTFetchSubState 
          {
          ETFTGetEnumerationFetchState,
          ERetrieveFilterInfoFetchState,
          ECopyRetrievedFilterFetchState
          };
      TTFTFetchSubState iTFTFetchSubState;

      // Filter
      TPckg<RPacketContext::TPacketFilterV2> iFilterPtr;
      RPacketContext::TPacketFilterV2 iFilter;	

      // UTMS Configuration data
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;		

      // UTMS Negotiated QoS
      TPckg<RPacketQoS::TQoSR5Negotiated> iUMTSQoSNegPtr;
      RPacketQoS::TQoSR5Negotiated iUMTSQoSNeg;

      TBool iCancellable;

      TInt iNumberOfFilters;
      TUint iCounter;

      TTFTInfo iTFTInfo;

      TTFTInfo iProcessedFilters;

      RPacketContext::TPacketFilterV2 iLastFilter;
    };


// State machine to create and configure a new primary pdp context
class CEtelContextStartupCreate : public CEtelSubRequest 
    {
    public:
      CEtelContextStartupCreate();
    protected:
      void DoTask(TContextParameters *aContextParameters);
    private:
      void RunL();

      enum TRequestState // Internal states types for this object 
          {
          EInitializeContextState,
          ESetConfigState,
          ESetQoSState,
          EActivateState,
          EGetConfigState,
          EGetQoSState,
          EGetStatusState
          };
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request	

      // UTMS Configuration data
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;	
	
      // UTMS Requested QoS
      TPckg<RPacketQoS::TQoSR5Requested> iUMTSQoSReqPtr;
      RPacketQoS::TQoSR5Requested iUMTSQoSReq;

      // UTMS Negotiated QoS
      TPckg<RPacketQoS::TQoSR5Negotiated> iUMTSQoSNegPtr;
      RPacketQoS::TQoSR5Negotiated iUMTSQoSNeg;

      // Context status
      RPacketContext::TContextStatus iContextStatus;

    };





inline void CEtelRequest::SubrequestReady()
    {
	iOperation = NULL;
	iFree = TRUE;
	delete iSubRequest;
	iSubRequest = NULL;
    }

inline TInt8 CEtelRequest::Operation() const
    {
	return iOperation;
    }
#endif //__UMTSREQS_H__
