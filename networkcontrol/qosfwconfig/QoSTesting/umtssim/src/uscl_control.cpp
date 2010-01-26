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

#include "us_cliserv.h"
#include "uscl_packet.h"
#include "uscl_control.h"

//********************************
//RControl
//********************************

// construct/destruct
EXPORT_C RControl::RControl()
    {
	// nothing here -- constructing done in Open
    }

EXPORT_C RControl::~RControl()
    {
	// nothing here
    }

//  create new subsession with the simulator server
EXPORT_C TInt RControl::Open(RUmtsSimServ &aServer )
    {
	// TAny* p[KMaxMessageArguments];
	// TInt err = CreateSubSession(aServer,EUmtsSimServCreateControlSubSession,&p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    TInt err = CreateSubSession(aServer, EUmtsSimServCreateControlSubSession, args);
	if(err != KErrNone) return err;
    return KErrNone;
    }

// close a subsession
EXPORT_C void RControl::Close()
    {
    RSubSessionBase::CloseSubSession(EUmtsSimServCloseControlSubSession);
    }

EXPORT_C void RControl::NotifyAll(TRequestStatus& aStatus, TDes& aMsg) const
    {
    // TAny* p[KMaxMessageArguments];
	// p[0] = &aMsg;
	// SendReceive(EUmtsSimServControlNotifyAllA, &p[0], aStatus);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aMsg );
    SendReceive(EUmtsSimServControlNotifyAllA, args, aStatus);
    }

EXPORT_C TInt RControl::ReconfigureSimulator(TInt aFlag) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = NULL;
	// p[1] = (TAny*) aFlag;
	// return SendReceive(EUmtsSimServControlReconfigureSimulatorS, &p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( NULL, aFlag ); 
	return SendReceive(EUmtsSimServControlReconfigureSimulatorS, args);
    }

EXPORT_C TInt RControl::ReconfigureSimulator(const TDesC& aFilename, TInt aFlag) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) &aFilename;
	// p[1] = (TAny*) aFlag;
	// return SendReceive(EUmtsSimServControlReconfigureSimulatorS, &p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aFilename, aFlag ); 
    return SendReceive(EUmtsSimServControlReconfigureSimulatorS, args);
    }

EXPORT_C TInt RControl::ConfigureRequestHandler(const TDesC& aCfgString) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) &aCfgString;
	// return SendReceive(EUmtsSimServControlConfigureRequestHandlerS, &p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aCfgString );
    return SendReceive(EUmtsSimServControlConfigureRequestHandlerS, args);
    }

EXPORT_C TInt RControl::ConfigureEvent(const TDesC& aCfgString) const
    {
	// TAny* p[KMaxMessageArguments];
	// p[0] = (TAny*) &aCfgString;
	// return SendReceive(EUmtsSimServControlConfigureEventS, &p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args( &aCfgString );
    return SendReceive(EUmtsSimServControlConfigureEventS, args);
    }

EXPORT_C void RControl::CancelAsyncRequest(TInt aReqToCancel) const
    {
	if((aReqToCancel & KUmtsSimServRqstMask) != KUmtsSimServRqstControl
       || (aReqToCancel & KUmtsSimServRqstCancelBit) == 0)
        {
		User::Invariant(); // should panic thread here with reasonable reason
		return;
        }

	// SendReceive(aReqToCancel, NULL);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
	SendReceive(aReqToCancel, args);
    }
