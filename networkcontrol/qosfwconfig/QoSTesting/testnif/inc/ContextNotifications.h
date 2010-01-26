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
// This file contains active objects which are used for receiving context
// related notifications from Etel
//

#ifndef _CONTEXT_NOTIFICATIONS_
#define _CONTEXT_NOTIFICATIONS_

#include <e32base.h>

#if MM_ETEL_API
#include "uscl_packet.h"	// Simulator
#include "uscl_qos.h"		// Simulator
#include "uscl_pcktcs.h"	// Simulator
#endif 

class CNifContext;

//
// Common base class for listeners
//
class CEtelContextNotificationRequest : public CActive 
    {
    public:
      CEtelContextNotificationRequest();
      ~CEtelContextNotificationRequest();	
      void ConstructL(CNifContext *aContext);
      virtual void Start() = 0;


      // Temp ->
      void PrintEvent(TUint aEventCode, const TContextParameters& aParameters);	
      void PrintEventType(TUint aEventCode, const TContextParameters& aParameters);
      void PrintContextInfo(const TContextInfo& aContextInfo);
      void PrintContextConfig(const TContextConfig& aContextConfig);
      void PrintContextNegotiatedQoS(TUint aEventCode, const TContextParameters& aParameters);
      void PrintContextTFT(TUint aEventCode, const TContextParameters& aParameters);
      // <- Temp

    protected:
	
      void DoCancel();	

      CNifContext *iContext;					// Pointer to parent context object 
      TUmtsSimServRqstCancels iNotifierCode;	// Used when object is destroyed to cancel corrent Etel-call
    };

//
// Listen to RPacketContext::NotifyConfigChanged()
//
class CEtelContextConfigChanged : public CEtelContextNotificationRequest 
    {
    public:
      CEtelContextConfigChanged();
      void Start();
    private:
      void RunL();	
	
      // Context configuration
      TPckg<RPacketContext::TContextConfigGPRS> iUMTSConfigPtr;
      RPacketContext::TContextConfigGPRS iUMTSConfig;	
    };

//
// Listen to RPacketContext::NotifyStatusChanged()
//
class CEtelContextStatusChanged : public CEtelContextNotificationRequest 
    {
    public:
      CEtelContextStatusChanged();
      void Start();
    private:
      void RunL();
	
      // Context status
      RPacketContext::TContextStatus iContextStatus;
    };

//
// Listen to RPacketQoS::NotifyProfileChanged()
//
class CEtelContextQoSChanged : public CEtelContextNotificationRequest 
    {
    public:
      CEtelContextQoSChanged();
      void Start();
    private:
      void RunL();
	
      // UMTS Negotiated QoS
      TPckg<RPacketQoS::TQoSR5Negotiated> iUMTSQoSNegPtr;
      RPacketQoS::TQoSR5Negotiated iUMTSQoSNeg;
    };

#endif
