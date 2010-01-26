/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/

/**
 @file te_dndserver.h
*/

#ifndef __TE_DNDSERVER_H__
#define __TE_DNDSERVER_H__

#include <testexecuteserverbase.h>
#include <es_sock.h>

/**
* CDndTestServer class derived from CTestServer
*/

class CDndTestServer : public CTestServer
    {
    public:
        static CDndTestServer* NewL();
        virtual CTestStep* CreateTestStep(const TDesC& aStepName);
        
        //-- public data for using by test steps   
        // !WARNING! ADD A COPY CONSTRUCTOR IF ADDING ANY POINTER VARIABLE
        RHostResolver       iHostResolver;
        RSocketServ         iSocketServ;
                  
    };

#endif //__TE_DNDSERVER_H__
