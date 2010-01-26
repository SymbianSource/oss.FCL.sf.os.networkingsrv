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

#ifndef _USSE_CONTROL_WRAPPER_H__
#define _USSE_CONTROL_WRAPPER_H__

#include "usse_server.h"

// *************************
// CControlApiWrapper
// for message delivery & wrapping
// *************************

class CControlApiWrapper : public CObject
    {
    public:
      static CControlApiWrapper* NewL(CUmtsSimServSession* aSession);
      ~CControlApiWrapper();
      void CloseWrapper();

    private:
      CControlApiWrapper(CUmtsSimServSession* aSession);
      void ConstructL();

    public:
      const RMessage2& Message() const;

    public:
      // FROM SIMULATION CONTROL API
      void NotifyAllA();
      void NotifyAllACancel();

      void ReconfigureSimulatorS();
      void ConfigureRequestHandlerS();
      void ConfigureEventS();

    public:
      // FROM SIMULATOR
      void Log(const TDesC& aEntry);

      // note call order in DebugChecks!
      void DebugCheck(CUmtsSimulator* aSimulator, CUmtsSimServSession* aSession);

    private:
      TBool iNotifyAllA;
      RMessage2 iNotifyAllAMessage;

    private:
      CUmtsSimServSession* iSession; //session owning us
      CUmtsSimulator* iSimulator;    //umts simulator state machine
    };

#endif
