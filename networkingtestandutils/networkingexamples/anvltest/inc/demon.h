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
//

#ifndef __DEMON_H
#define __DEMON_H

#include <e32base.h>

class MDemonMain
    {
public:
    virtual void Write(const TDesC &aDes) = 0;
    virtual void ShowError(TInt aId) = 0;
    TInt virtual CheckResult(const TDesC &aText, TInt aResult) = 0;
    
    virtual void HandleCommandL(TInt aCommand) = 0;
    virtual void Start(int) = 0;   // Start Engine
    virtual void Stop() = 0;    // Stop Engine

private:
	
	int ipVersion;

    };

class CDemonEngineBase : public CBase
    {
public:
    virtual ~CDemonEngineBase() {};
    virtual void ConstructL() = 0;
    virtual void HandleCommandL(TInt aCommand) = 0;

protected:

    CDemonEngineBase(MDemonMain *aMain, int ipVersionSelected) : iMain(aMain) 
	{
		ipVersion = ipVersionSelected;
	}

    MDemonMain *iMain;
	
private:

	int ipVersion;

    };

class ENGINE
    {
public:

    static CDemonEngineBase *NewL(MDemonMain *aMain, int ipVersionSelected);

private:

    int ipVersion;    

};

#endif
