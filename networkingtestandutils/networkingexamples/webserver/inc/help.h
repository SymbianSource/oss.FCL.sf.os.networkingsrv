// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// help.h - http server help module
//



/**
 @internalComponent
*/
#ifndef __HELP_H
#define __HELP_H

#include <apgtask.h>

class CHelpTask : public CBase {
public:
    void ConstructL(RWsSession* awsSession, const CEikApplication* aEikApp);
    ~CHelpTask();

    void DisplayL();
    void Close();

private:

    TBool LocateHelpTask(TApaTask& aTask);

    TBool     iHelpAppLaunched;
    TThreadId iHelpAppThreadId;

    TApaTask*   iTask;
    RWsSession* iwsSession;

    TFileName   iHelpFileName;

};

#endif
