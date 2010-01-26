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


#include <e32base.h>

#include "TestStepLLMNR.h"
//#include "TestSuiteLLMNR.h"

const TInt  KMaxStrLen = 256;

// constructor
CTestStepLLMNR::CTestStepLLMNR(CLlmnrTestServer *apLlmnrTestServer/* = NULL*/) 
               :ipTestServer(apLlmnrTestServer)
    {
    
    }

// destructor
CTestStepLLMNR::~CTestStepLLMNR()
    {
    }


/**
*   Read string from .ini file into 8-bit string descriptor
*
*   @param  aSectName section name descriptor.
*   @param  aKeyName  key name descriptor.  
*   @param  aStr      ref. to the 8-bit descriptor for the result string.  
*
*   @return ETrue on success.
*/
TBool   CTestStepLLMNR::GetIniFileString8(const TDesC &aSectName, const TDesC &aKeyName, TDes8& aStr)
    {
    TBuf<KMaxStrLen>    buff;
    TPtrC               buffPtr(buff);
    
    TBool bRes = GetStringFromConfig(aSectName, aKeyName, buffPtr);
    
    if(bRes)
        aStr.Copy(buffPtr);
    
    return bRes;
    }


/**
*   Read string from .ini file into 16-bit string descriptor
*
*   @param  aSectName section name descriptor.
*   @param  aKeyName  key name descriptor.  
*   @param  aStr      ref. to the 8-bit descriptor for the result string.  
*
*   @return ETrue on success.
*/
TBool   CTestStepLLMNR::GetIniFileString(const TDesC &aSectName, const TDesC &aKeyName, TDes& aStr)
    {
    TPtrC   buffPtr(aStr);
    
    TBool bRes = GetStringFromConfig(aSectName, aKeyName, buffPtr);
    
    if(bRes)
        aStr.Copy(buffPtr);
    
    return bRes;
    }

/**
*   read IP address from .ini file, the address may have either IPv4 or IPv6 format.
*
*   @param  aSectName section name descriptor.
*   @param  aKeyName  key name descriptor.  
*   @param  aInetAddr will contain result.
*
*   @return ETrue on success
*/
TBool   CTestStepLLMNR::GetIniFileIpAddr(const TDesC &aSectName, const TDesC &aKeyName, TInetAddr& aInetAddr)
    {
    TBuf<KMaxStrLen>    buff;
    TPtrC               buffPtr(buff);
    
    TBool bRes = GetStringFromConfig(aSectName, aKeyName, buffPtr);
    if(!bRes) 
        return EFalse;
    
    if(aInetAddr.Input(buffPtr) == KErrNone)
        return ETrue;
    else
        {
        ERR_PRINTF3(_L("Invalid IP address, section:%S key:%S "), &aSectName, &aKeyName );
        return EFalse;
        }
    }










