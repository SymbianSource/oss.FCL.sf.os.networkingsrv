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
// umtsniflink.cpp
// Representation of a context in the Nif
//

#ifndef _NIF_CONTEXT_
#define _NIF_CONTEXT_


#include "uscl_packet.h"
#include "uscl_qos.h"

#include "packetinterface.h"
#include <UmtsNifControlIf.h>

#include "contextnotifications.h"

class CUmtsNif;
class CEtelRequest;

class CNifContext : public CBase, public MNifContextNotify
    {
      friend class CNifContextManager;
    public:
      ~CNifContext();
      static CNifContext* NewPrimaryL(CUmtsNif* aNif,TContextId aNifContextId);
      static CNifContext* NewSecondaryL(CUmtsNif* aNif,TContextId aNifContextId,TDes& aExistingName);
	
      inline RPacketQoS& ContextQoS()									const;
      inline RPacketContext& ContextHandle()							const;
      inline TBool FlowOn()											const;
      inline TContextParameters& Parameters()							const;
      inline TName ContextName()										const;
      inline TContextId ContextId()									const; 
      inline CUmtsNif& Nif()											const;
      inline RPacketContext::TContextStatus Status()					const;
      inline TContextType ContextType()								const;
      inline TBool Created()											const;
      inline TBool Usable()											const;
      inline void SetStatus(RPacketContext::TContextStatus aStatus);
      inline void SetConfig(RPacketContext::TContextConfigGPRS& aConfig);
      inline TBool IdenticalQoS(RPacketQoS::TQoSR5Negotiated aQoS1,RPacketQoS::TQoSR5Negotiated aQoS2) const;
      inline void SetCreated();
      TBool Usable();
      inline CPacketInterface& PacketIf() const;
      void ContextInternalEvent(RPacketContext::TContextStatus aContextStatus);
      void ContextInternalEvent(RPacketQoS::TQoSR5Negotiated aContextStatus);
      void NetworkInitiatedDeletion();
      void ContextBlocked();
      void FillEvent(TContextParameters &aEvent);
      void FillEventQoS(TContextParameters &aEvent);
      void FillEventTFT(TContextParameters &aEvent);

      // QoS notification related
      TBool PendingModification();
      TBool GetPendingQoS(RPacketQoS::TQoSR5Negotiated& aQoS);
      void ResetPendingQoS();
      void SetPendingQoS(RPacketQoS::TQoSR5Negotiated aQoS);
	
      //void AddToQueue(RMBufChain &aPacket);
      void Send(RMBufChain &aPacket);
      TInt StartListenersL();
      TInt IssueRequest(TContextParameters *aParameters,TInt8 aOperation);
	
      void UnblockIndication();					// From MNifContextNotify
      void BlockIndication();						// From MNifContextNotify
      void ProcessPacket(RMBufChain& aPacket);	// From MNifContextNotify

      void PrintEvent(TUint aEventCode, TContextParameters& aParameters);	
      
      TInt SetIMCNSubsystemflag(TBool aIMCNSubsystemflag);
      TInt GetIMCNSubsystemflag(TBool &aIMCNSubsystemflag) const;

    protected:
      CNifContext();
      void InitL(CUmtsNif* aNif,TContextId aNifContextId);
      void InitL(CUmtsNif* aNif,TContextId aNifContextId,TDes& aExistingName);
	
    private:
      RPacketContext *iContextHandle;	// Context session handle
      RPacketQoS *iContextQoS;		// Context QoS-session handle	
      TBuf<65> iProxyId;				// From TSY
      CPacketInterface *iPacketIf;	// Interface used to send/receive packets on a context
      TName iContextName;				// Name given by TSY-module	
      TContextId iContextId;			// Id in the Nif
      CUmtsNif *iNif;					// Parent Nif
      CEtelRequest *iRequest;			// Object to handle Etel requests
      TContextParameters *iContextParameters;	
      // Notification listeners START
      CEtelContextNotificationRequest *iContextConfigListener;
      CEtelContextNotificationRequest *iContextStatusListener;
      CEtelContextNotificationRequest *iQoSProfileChangeListener;
      // Notification listeners END
      TBool iUsable;
      TBool iCreated;
      TBool iQoSPending;
      RPacketQoS::TQoSR5Negotiated iPendingQoS;
      // A boolean variable to mark if the setqos request is from user (or network?)	
      TBool iManualQosRequest;
      // IMS flag      
      TBool iIMCNSubsystemflag;
    };


class CNifContextManager;
class CEtelContextDelete : public CActive
    {
    public:
      CEtelContextDelete();
      ~CEtelContextDelete();
      void ConstructL(CNifContext *aContext,CNifContextManager *aContextManager);
      void Start();

      void JumpToRunl(TInt aError);
    protected:
	
      void DoCancel();
    private:
      void RunL();

      enum TRequestState // Internal states types for this object 
          {	
          EDeleteState,
          EReleaseState,
          };
      CNifContextManager *iContextManager;
      TRequestState iRequestState; // Internal state indicating the state of the wrapped request	
      CNifContext *iContext;
      TUmtsSimServRqstCancels iNotifierCode;
      TBool iCancellable;
    };	

	

inline TContextParameters& CNifContext::Parameters() const 
    { 
	return *iContextParameters;
    } 
inline RPacketContext::TContextStatus CNifContext::Status() const 
    { 
	return Parameters().iContextInfo.iStatus; 
    }
inline TName CNifContext::ContextName() const 
    { 
	return iContextName; 
    }
inline TContextId CNifContext::ContextId() const 
    { 
	return iContextId; 
    }
inline CUmtsNif& CNifContext::Nif() const 
    { 
	return *iNif; 
    }
inline void CNifContext::SetStatus(RPacketContext::TContextStatus aStatus) 
    { 
	Parameters().iContextInfo.iStatus = aStatus; 
    }
inline TContextType CNifContext::ContextType() const 
    { 
	return Parameters().iContextType; 
    } 
inline TBool CNifContext::Created() const
    {
	return iCreated;
    }
inline TBool CNifContext::Usable() const
    {
	return iUsable;
    }

inline void CNifContext::SetConfig(RPacketContext::TContextConfigGPRS& aConfig) 
    { 
	Parameters().iContextConfig.SetContextConfig(aConfig); 
    }

inline TBool CNifContext::FlowOn() const
    { 
	return iPacketIf->ChannelOpen(); 
    }
inline RPacketContext& CNifContext::ContextHandle() const
    { 		
	return *iContextHandle;
    }
inline RPacketQoS& CNifContext::ContextQoS() const
    { 
	return *iContextQoS;
    }
inline CPacketInterface& CNifContext::PacketIf() const
    {
	return *iPacketIf;
    }

inline void CNifContext::SetCreated() 
    {
	iCreated = ETrue;
    }
inline TBool CNifContext::IdenticalQoS(RPacketQoS::TQoSR5Negotiated aQoS1,RPacketQoS::TQoSR5Negotiated aQoS2) const
    {
	if(	aQoS1.iTrafficClass != aQoS2.iTrafficClass ||
       aQoS1.iDeliveryOrderReqd != aQoS2.iDeliveryOrderReqd ||
       aQoS1.iDeliverErroneousSDU != aQoS2.iDeliverErroneousSDU ||
       aQoS1.iMaxSDUSize != aQoS2.iMaxSDUSize ||
		
       aQoS1.iMaxRate.iUplinkRate != aQoS2.iMaxRate.iUplinkRate ||
       aQoS1.iMaxRate.iDownlinkRate != aQoS2.iMaxRate.iDownlinkRate ||
       aQoS1.iBER != aQoS2.iBER ||
       aQoS1.iSDUErrorRatio != aQoS2.iSDUErrorRatio ||
       aQoS1.iTrafficHandlingPriority != aQoS2.iTrafficHandlingPriority ||
       aQoS1.iTransferDelay != aQoS2.iTransferDelay ||
       aQoS1.iGuaranteedRate.iDownlinkRate != aQoS2.iGuaranteedRate.iDownlinkRate ||
       aQoS1.iGuaranteedRate.iUplinkRate != aQoS2.iGuaranteedRate.iUplinkRate)
		return EFalse;
	else
		return ETrue;

    }
#endif
