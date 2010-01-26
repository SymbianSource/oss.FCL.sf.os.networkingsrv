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

#ifndef _USSE_SERVER_H__
#define _USSE_SERVER_H__

#include "uscl_packet.h"

//needed for creating server thread.
const TUint KDefaultHeapSize=0x10000;

// panic reasons
enum TUmtsSimServPanic
    {
	EBadRequest,
	EBadDescriptor,
	EDescriptorNonNumeric,
	EMainSchedulerError,
	ESvrCreateServer,
	ECreateTrapCleanup,
	ELoadServerLibrary,
	EBadSubsessionRemove, 
	EBadSubsessionHandle 
    };

//**********************************
// CUmtsSimServServer
//**********************************

class CUmtsSimulator;

// Server class.
class CUmtsSimServServer : public CServer2
    {
    public:
      enum {EUmtsSimServerPriority=950};
    public:
      static void PanicServer(TUmtsSimServPanic aPanic);
      IMPORT_C static TInt ThreadFunction(TAny*);

      //construct/destruct
      static CUmtsSimServServer* NewL();
      ~CUmtsSimServServer();

    private:
      CUmtsSimServServer();
      void ConstructL();

    public:
      //open/close a session
      // CSharableSession* NewSessionL(const TVersion &aVersion) const;
      // Changed because of migration to Client/Server V2 API
      CSession2* NewSessionL(const TVersion &aVersion, const RMessage2 &aMessage) const;
      void CloseSession();

      //return a object container index which creates an object container for each session
      CObjectCon* NewContainerL();
      void DeleteContainer(CObjectCon* aContainer);

    private:
      //state machine implementation class
      CUmtsSimulator* iSimulator;
      //object container index for creating session specific containers
      CObjectConIx* iContainerIndex;
    };


//**********************************
// CUmtsSimServSession
//**********************************

// for message delivery & wrapping
class CPacketServiceApiWrapper;
class CPacketContextApiWrapper;
class CPacketQoSApiWrapper;
class CControlApiWrapper;

// session for the UMTS Simulator server, to a single client-side session
class CUmtsSimServSession : public CSession2
    {
      friend class CPacketService;
      friend class CPacketContext;
      friend class CUmtsSimulator;
    public:
      //construct/destruct/close session
      // static CUmtsSimServSession* NewL(RThread& aClient,
      //                                  /*const */CUmtsSimServServer* aServer,
      //                                  CUmtsSimulator* aSimulator);
      // Changed because of migration to Client/Server V2 API
      static CUmtsSimServSession* NewL(/*const */CUmtsSimServServer* aServer,
                                       CUmtsSimulator* aSimulator);
      ~CUmtsSimServSession();
      void CloseSession();

    private:
      void ConstructL(CUmtsSimServServer* aServer, CUmtsSimulator* aSimulator);
      // CUmtsSimServSession(RThread& aClient);
      // Changed because of migration to Client/Server V2 API
      CUmtsSimServSession();

    public:
      //service request
      CPacketServiceApiWrapper* PacketServiceWrapperFromHandle(TUint aHandle);
      CPacketContextApiWrapper* PacketContextWrapperFromHandle(TUint aHandle);
      CPacketQoSApiWrapper* PacketQoSWrapperFromHandle(TUint aHandle);
      CControlApiWrapper* ControlWrapperFromHandle(TUint aHandle);

      void ServiceL(const RMessage2& aMessage);
      void DispatchMessageL(const RMessage2& aMessage);

      //create/delete subsession
      void NewPacketServiceWrapperL();
      void NewPacketContextWrapperL();
      void NewPacketQoSWrapperL();
      void NewControlWrapperL();
      void DeletePacketServiceWrapper();
      void DeletePacketContextWrapper();
      void DeletePacketQoSWrapper();
      void DeleteControlWrapper();

      //resource counting
      TInt CountResources();

      //utility
      void Write(const TAny* aPtr,const TDesC& aDes,TInt anOffset=0);
      void PanicClient(TInt aPanic) const;
      CUmtsSimulator* GetSimulator() const;

      void DebugCheck(CUmtsSimulator* aSimulator); // note call order in DebugChecks!

    private:
      CObjectCon* iContainer; // object container for this session, created with server
      // indexes for wrappers specific for unique RPacket<Service|Context> instances
      CObjectIx* iPacketServiceWrappers;
      CObjectIx* iPacketContextWrappers;
      CObjectIx* iPacketQoSWrappers;
      CObjectIx* iControlWrappers;
      CUmtsSimServServer *iUmtsSimSvr; // pointer to owning server
      // this pointer has the right derived-class type, unlike
      // CSession::Server() result
      TInt iResourceCount;
      TBool iSessionRemoved;

      CUmtsSimulator* iSimulator;

    public:
      TSglQueLink iSglLink; // for CUmtsSimulator's linked list

      // Added because of migration to Client/Server V2 API
      RMessage2& Message() { return iMessage; };
      RMessage2 iMessage;
    };

#endif
