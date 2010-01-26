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

#ifndef __ENGINE_H
#define __ENGINE_H

#include <s32file.h>
#include <es_sock.h>
#include <in_sock.h>

#include "demon.h"

class CAnvltestListener;

class CAnvltestEngine : public CDemonEngineBase
    {
public:

    CAnvltestEngine(MDemonMain *aMain,int ipVersionSelected) : CDemonEngineBase(aMain, ipVersionSelected) 
	{ 
		ipVersion = ipVersionSelected;
	}

    ~CAnvltestEngine();
    void ConstructL();
    void HandleCommandL(TInt aCommand);
    void TestPrint(void);
    void WriteText(TRefByValue<const TDesC16> aFmt,...);
    void WriteTextList(const TDesC &aFmt,VA_LIST aList);
    void CheckResultL(const TDesC &aText, TInt aResult);
    TInt CheckResult(const TDesC &aText, TInt aResult);
    void ShowText(const TDesC &aText);
    CAnvltestListener *iListener;

	int GetIpVersion(void);	
	int ipVersion;

    };

#endif // __ENGINE_H
