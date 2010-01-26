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
// uscl_packetservice.cpp - implementation of RPacketService class
//

#include "us_cliserv.h"
#include "uscl_pcktcs.h"
#include "uscl_packet.h"
#include "uscl_packet_internal.h"

//********************************
//RPacketService
//********************************

// construct/destruct
EXPORT_C RPacketService::RPacketService() : iData(NULL)
    {
	// nothing here -- constructing done in Open
    }

EXPORT_C RPacketService::~RPacketService()
    {
	if(iData)
        {
		// could also call Close, but is RUmtsSimServ still up?
		delete iData;
		iData = NULL;
        }
    }

//  create new subsession with the simulator server
EXPORT_C TInt RPacketService::Open(RUmtsSimServ &aServer )
    {
	// TAny* p[KMaxMessageArguments];
	// TInt err = CreateSubSession(aServer,EUmtsSimServCreatePacketServiceSubSession,&p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	TInt err = CreateSubSession(aServer,EUmtsSimServCreatePacketServiceSubSession,args);
	if(err != KErrNone) return err;

	if(!iData)
        {
		TRAPD(r,iData = CPacketServiceInternalData::NewL());
		if(r != KErrNone)
            {
			iData = NULL;
			return r;
            }
        }
	iData->iSimServ = &aServer;

    return KErrNone;
    }

// close a subsession
EXPORT_C void RPacketService::Close()
    {
	if(iData)
        {
		delete iData;
		iData = NULL;
        }

    RSubSessionBase::CloseSubSession(EUmtsSimServClosePacketServiceSubSession);
    }


// attach to/detach from network
EXPORT_C void RPacketService::Attach(TRequestStatus& aStatus) const
    {
    // SendReceive(EUmtsSimServPacketServiceAttachA, 0, aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    SendReceive(EUmtsSimServPacketServiceAttachA, args, aStatus);
    }

EXPORT_C void RPacketService::Detach(TRequestStatus& aStatus) const
    {
    // SendReceive(EUmtsSimServPacketServiceDetachA, args, aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    SendReceive(EUmtsSimServPacketServiceDetachA, args, aStatus);
    }


// get the packet service system status -- what should this return???
EXPORT_C TInt RPacketService::GetStatus(TStatus& aPacketServiceStatus) const
    {
    TPckg<TStatus> pckg(aPacketServiceStatus);

    // TAny* p[KMaxMessageArguments];
    // p[0]=(TAny *) &pckg;
    // TInt res = SendReceive(EUmtsSimServPacketServiceGetStatusS, &p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &pckg );
    return SendReceive(EUmtsSimServPacketServiceGetStatusS, args);
    }

EXPORT_C void RPacketService::NotifyStatusChange(TRequestStatus& aStatus,
										         TStatus& aPacketServiceStatus) const
    {
	if(iData->iNotifyStatusChangePckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketNotifyStatusChange);
        }
	iData->iNotifyStatusChangePckg = new TPckg<TStatus> (aPacketServiceStatus);

    // TAny* p[KMaxMessageArguments];
	// p[0] = iData->iNotifyStatusChangePckg;
    // SendReceive(EUmtsSimServPacketServiceNotifyStatusChangeA, &p[0], aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( iData->iNotifyStatusChangePckg );
    SendReceive(EUmtsSimServPacketServiceNotifyStatusChangeA, args, aStatus);
    }


// order notification of added contexts
EXPORT_C void RPacketService::NotifyContextAdded(TRequestStatus& aStatus, TDes& aContextId) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = &aContextId;
	// SendReceive(EUmtsSimServPacketServiceNotifyContextAddedA, &p[0], aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aContextId );
    SendReceive(EUmtsSimServPacketServiceNotifyContextAddedA, args, aStatus);
    }


// get info about contexts
EXPORT_C void RPacketService::EnumerateContexts(TRequestStatus& aStatus, TInt& aCount, TInt& aMaxAllowed) const
    {
	if(iData->iEnumerateContextsPckg1)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketEnumerateContexts);
        }
	iData->iEnumerateContextsPckg1 = new TPckg<TInt> (aCount);
	iData->iEnumerateContextsPckg2 = new TPckg<TInt> (aMaxAllowed);

	// TAny* p[KMaxMessageArguments];
	// p[0] = iData->iEnumerateContextsPckg1;
	// p[1] = iData->iEnumerateContextsPckg2;
    // SendReceive(EUmtsSimServPacketServiceEnumerateContextsA, &p[0], aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( iData->iEnumerateContextsPckg1, 
                   iData->iEnumerateContextsPckg2 );
    SendReceive(EUmtsSimServPacketServiceEnumerateContextsA, args, aStatus);
    }

EXPORT_C void RPacketService::GetContextInfo(TRequestStatus & aStatus, TInt aIndex, TContextInfo& aInfo) const
    {
	if(iData->iGetContextInfoPckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketGetContextInfo);
        }
	iData->iGetContextInfoPckg = new TPckg<TContextInfo> (aInfo);

	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) aIndex;
	// p[1] = iData->iGetContextInfoPckg;
	// SendReceive(EUmtsSimServPacketServiceGetContextInfoA, &p[0], aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( aIndex, 
                   iData->iGetContextInfoPckg );
    SendReceive(EUmtsSimServPacketServiceGetContextInfoA, args, aStatus);
    }


// get info about nifs and contexts in them
EXPORT_C void RPacketService::EnumerateNifs(TRequestStatus& aStatus, TInt& aCount) const
    {
	if(iData->iEnumerateNifsPckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketEnumerateNifs);
        }
	iData->iEnumerateNifsPckg = new TPckg<TInt> (aCount);

	// TAny* p[KMaxMessageArguments];
	// p[0] = iData->iEnumerateNifsPckg;
	// SendReceive(EUmtsSimServPacketServiceEnumerateNifsA, &p[0], aStatus);
	// Changed because of migration to Client/Server V2 API
	TIpcArgs args( iData->iEnumerateNifsPckg );
	SendReceive(EUmtsSimServPacketServiceEnumerateNifsA, args, aStatus);
    }

EXPORT_C void RPacketService::GetNifInfo(TRequestStatus& aStatus, TInt aIndex, TDes8& aNifInfo) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) aIndex;
	// p[1] = &aNifInfo;
	// SendReceive(EUmtsSimServPacketServiceGetNifInfoA, &p[0], aStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( aIndex, 
                   &aNifInfo );
	SendReceive(EUmtsSimServPacketServiceGetNifInfoA, args, aStatus);
    }

EXPORT_C void RPacketService::EnumerateContextsInNif(TRequestStatus& aRequestStatus,
													 const TDesC& aExistingContextName, TInt& aCount) const
    {
	if(iData->iEnumerateContextsInNifPckg)
        {
		// in case there is already a request on we cancel it
		CancelAsyncRequest(EPacketEnumerateContextsInNif);
        }
	iData->iEnumerateContextsInNifPckg = new TPckg<TInt> (aCount);

	// TAny* p[KMaxMessageArguments];
	// p[0] = iData->iEnumerateContextsInNifPckg;
	// p[1] = (TAny*) &aExistingContextName;
	// SendReceive(EUmtsSimServPacketServiceEnumerateContextsInNifA, &p[0], aRequestStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( iData->iEnumerateContextsInNifPckg, 
                   &aExistingContextName );
	SendReceive(EUmtsSimServPacketServiceEnumerateContextsInNifA, args, aRequestStatus);
    }

EXPORT_C void RPacketService::GetContextNameInNif(TRequestStatus& aRequestStatus,
												  const TDesC& aExistingContextName,
												  TInt aIndex, TDes& aContextName) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) aIndex;
	// p[1] = (TAny*) &aExistingContextName;
	// p[2] = &aContextName;
	// SendReceive(EUmtsSimServPacketServiceGetContextNameInNifA, &p[0], aRequestStatus);
	// Changed because of migration to Client/Server V2 API
    TIpcArgs args( aIndex, 
                   &aExistingContextName, 
                   &aContextName );
	SendReceive(EUmtsSimServPacketServiceGetContextNameInNifA, args, aRequestStatus);
    }

EXPORT_C void RPacketService::CancelAsyncRequest(TInt aReqToCancel) const
    {
	if((aReqToCancel & KUmtsSimServRqstMask) != KUmtsSimServRqstPacketService
       || (aReqToCancel & KUmtsSimServRqstCancelBit) == 0)
        {
		User::Invariant(); // should panic thread here with reasonable reason
		return;
        }

	// Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	SendReceive(aReqToCancel, args);

	if(aReqToCancel == EUmtsSimServPacketServiceNotifyStatusChangeACancel && iData->iNotifyStatusChangePckg)
        {
		delete iData->iNotifyStatusChangePckg;
		iData->iNotifyStatusChangePckg = NULL;
        }
	if(aReqToCancel == EUmtsSimServPacketServiceEnumerateContextsACancel && iData->iEnumerateContextsPckg1)
        {
		delete iData->iEnumerateContextsPckg1;
		delete iData->iEnumerateContextsPckg2;
		iData->iEnumerateContextsPckg1 = NULL;
		iData->iEnumerateContextsPckg2 = NULL;
        }
	if(aReqToCancel == EUmtsSimServPacketServiceGetContextInfoACancel && iData->iGetContextInfoPckg)
        {
		delete iData->iGetContextInfoPckg;
		iData->iGetContextInfoPckg = NULL;
        }
	if(aReqToCancel == EUmtsSimServPacketServiceEnumerateNifsACancel && iData->iEnumerateNifsPckg)
        {
		delete iData->iEnumerateNifsPckg;
		iData->iEnumerateNifsPckg = NULL;
        }
	if(aReqToCancel == EUmtsSimServPacketServiceEnumerateContextsInNifACancel &&
       iData->iEnumerateContextsInNifPckg)
        {
		delete iData->iEnumerateContextsInNifPckg;
		iData->iEnumerateContextsInNifPckg = NULL;
        }
    }

// *** CPacketServiceInternalData *** wraps data needed by RPacketService in seperately allocated package

CPacketServiceInternalData* CPacketServiceInternalData::NewL()
    {
	CPacketServiceInternalData* self = new (ELeave) CPacketServiceInternalData();
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();
	return self;
    }

CPacketServiceInternalData::CPacketServiceInternalData() : iSimServ(NULL), iNotifyStatusChangePckg(NULL),
                                                               iEnumerateContextsPckg1(NULL), iEnumerateContextsPckg2(NULL), iGetContextInfoPckg(NULL),
                                                               iEnumerateNifsPckg(NULL), iEnumerateContextsInNifPckg(NULL)
    {
	// ConstructL does the work
    }

CPacketServiceInternalData::~CPacketServiceInternalData()
    {
	if(iNotifyStatusChangePckg)
        {
		delete iNotifyStatusChangePckg;
		iNotifyStatusChangePckg = NULL;
        }
	if(iEnumerateContextsInNifPckg)
        {
		delete iEnumerateContextsInNifPckg;
		iEnumerateContextsInNifPckg = NULL;
        }
	if(iEnumerateContextsPckg1)
        {
		delete iEnumerateContextsPckg1;
		delete iEnumerateContextsPckg2;
		iEnumerateContextsPckg1 = NULL;
		iEnumerateContextsPckg2 = NULL;
        }
	if(iEnumerateNifsPckg)
        {
		delete iEnumerateNifsPckg;
		iEnumerateNifsPckg = NULL;
        }
	if(iGetContextInfoPckg)
        {
		delete iGetContextInfoPckg;
		iGetContextInfoPckg = NULL;
        }
    }

RSessionBase* CPacketServiceInternalData::GetServer() const
    {
	return iSimServ;
    }
