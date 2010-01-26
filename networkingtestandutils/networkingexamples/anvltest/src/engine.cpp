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

#include "anvltest.h"
#include "engine.h"
#include "anvllog.h"
#include "listener.h"

CDemonEngineBase *ENGINE::NewL(MDemonMain *aMain, int ipVersionSelected)
    {	
    return new (ELeave) CAnvltestEngine(aMain, ipVersionSelected);
    
	}

CAnvltestEngine::~CAnvltestEngine()
    {
    delete iListener;
    }


void CAnvltestEngine::ConstructL()
    {

    //
    // Start Socket Reader activity
    //
    CheckResultL(_L("Active Scheduler"), CActiveScheduler::Current() == NULL);

    iListener = new (ELeave) CAnvltestListener(this ,this->ipVersion);
    iListener->ConstructL();
	
    }

    
void CAnvltestEngine::HandleCommandL(TInt /*aCommand*/)
    {
#if 0   
    switch(aCommand)
        {
        case EAnvltestDump:
            if (iListener)
                iListener->HandleCommandL(aCommand);
            else
                iMain->Write(_L("Listener not active.\n"));
            break;

        default:
            // No own commands, pass the buck to other modules
            if (iListener)
                iListener->HandleCommandL(aCommand);
        }
#endif
    }

//
// CAnvltestEngine::CheckResult
//  Output success or fail message, returns the error code
//
TInt CAnvltestEngine::CheckResult(const TDesC &aText, TInt aResult)
    {
    return iMain->CheckResult(aText, aResult);
    }

void CAnvltestEngine::CheckResultL(const TDesC &aText, TInt aResult)
    {
    if (CheckResult(aText, aResult) != KErrNone)
        User::Leave(aResult);
    }

int CAnvltestEngine::GetIpVersion(void)
{
	return ipVersion;	
}
void CAnvltestEngine::ShowText(const TDesC &aText)
    {
    iMain->Write(aText);
    }

void CAnvltestEngine::WriteText(TRefByValue<const TDesC16> aFmt,...)
    {
    VA_LIST aList;
    VA_START(aList, aFmt);
    WriteTextList(aFmt, aList);
    }

void CAnvltestEngine::WriteTextList(const TDesC &aFmt,VA_LIST aList)
    {
    TBuf<250> str;
    
    str.AppendFormatList(aFmt, aList);
    iMain->Write(str);
    }

