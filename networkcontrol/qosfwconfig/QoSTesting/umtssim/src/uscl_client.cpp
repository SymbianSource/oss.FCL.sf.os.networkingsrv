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
// uscl_client.cpp - implementation of client side server handle for umts simulator session
//

#include "us_cliserv.h"
#include "uscl_packet.h"

// number of message slots.
const TUint kDefaultMessageSlots=32;

// DLL entry point


//********************************
//RUmtsSimServ
//********************************

EXPORT_C RUmtsSimServ::RUmtsSimServ()
    {
    }


// Connect to the  server
EXPORT_C TInt RUmtsSimServ::Connect()
    {
    TInt err = KErrNone;
    for(TInt tries = 0; tries < 2; tries++)
        {
		err = CreateSession(KUmtsSimServerName,Version(),kDefaultMessageSlots);

		if(!err) break; // connected
		if(err != KErrNotFound && err != KErrServerTerminated)
			break; // problems

		err = StartThread(); // try to launch the server

		if(!err) continue; // ok, try to connect again
		if(err == KErrAlreadyExists) continue; // someone else started, ok

		break; // problems
        }

    return(err); 
    }


// Return the version number
EXPORT_C TVersion RUmtsSimServ::Version(void) const
    {
    return(TVersion(KUmtsSimServMajorVersionNumber,
				    KUmtsSimServMinorVersionNumber,
					KUmtsSimServBuildVersionNumber));
    }


// Close the session
EXPORT_C void RUmtsSimServ::Close()
    {
    // TAny *p[KMaxMessageArguments];
    // SendReceive(EUmtsSimServCloseSession,&p[0]);
    // Changed because of migration to Client/Server V2 API
    TIpcArgs args(TIpcArgs::ENothing);   // No arguments
    
    SendReceive(EUmtsSimServCloseSession, args);
    RHandleBase::Close();
    }
