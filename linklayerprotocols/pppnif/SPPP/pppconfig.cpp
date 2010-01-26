// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PPP CFProtocol Provisioning Message class implementations
// 
//

/**
 @file
 @internalComponent
*/

#include "pppmcpr.h"
#include <comms-infras/ss_tiermanagerutils.h>
#include <comms-infras/ss_log.h>
#include <networking/pppconfig.h>

using namespace ESock;

    
//-=========================================================
// CPppLcpConfig methods
//-=========================================================
EXPORT_C CPppLcpConfig* CPppLcpConfig::NewLC(ESock::CCommsDatIapView* aIapView)
	{
	CPppLcpConfig* self = new (ELeave) CPppLcpConfig;
	CleanupStack::PushL(self);
	self->InitialiseConfigL(aIapView);
	return self;
	}

void CPppLcpConfig::InitialiseConfigL(ESock::CCommsDatIapView* aIapView)
    {
	HBufC* buf = NULL;
	TUint32 uintDBVar = 0;
	TBool boolDBVar;
	TInt err = KErrNone;
	    
#if defined (_DEBUG)
	err = aIapView->GetText(KCDTIdDialOutISPRecord | KCDTIdRecordName, buf);
    SetISPName(buf);
#endif       
    //setters
    
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdIfServerMode, boolDBVar);
    if (KErrNone != err)
		{
		__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read IfServerMode from database. Defaulting to EPppLinkIsClient")); )
		}		
	
	if (!boolDBVar)
		{
		SetIfServerMode(CPppLcpConfig::EPppLinkIsClient);
		}	
	else
		{
		SetIfServerMode(CPppLcpConfig::EPppLinkIsServer);
		}
	
    err = aIapView->GetText(KCDTIdIfParams, buf);
    if (KErrNone == err)
    	{
		ASSERT(buf);
    	SetIfParams(buf);	// transfers ownership of buf
		buf = NULL;
    	}
	else 
		{
		ASSERT(buf == NULL);
		}
    
    //
    // Netdial Callback information
    //
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdIfCallbackEnabled, boolDBVar);
    if (err != KErrNone)
    	{
    	__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read IfCallbackEnabled from database. Callback config will be skipped")); )    	
    	}    	
    SetIfCallbackEnabled(boolDBVar);
    	
    if (boolDBVar)
    	{    	    	
    	uintDBVar = 0;
    	err = aIapView->GetInt(KCDTIdIfCallbackType, uintDBVar);
    	if (KErrNone != err)
    		{
    		__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read IfCallbackType from database.")); )
    		User::Leave(err);
    		}
    	SetIfCallbackType(uintDBVar);
    	
    	err = aIapView->GetText(KCDTIdIfCallbackInfo, buf);
    	if (KErrNone != err)
    		{
			ASSERT(buf == NULL);
    		__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read IfCallbackType from database.")); )
    		User::Leave(err);
    		}
		else
			{
			ASSERT(buf);
	    	SetIfCallbackInfo(buf);	// transfers ownership of buf
			buf = NULL;
    		}
    	}
    
        
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdEnableLCPExtension, boolDBVar);
    if (err != KErrNone)
    	{
    	__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read EnableLcpExtensions from database. Defaulting to false.")); )    		
    	}
    SetEnableLcpExtensions(boolDBVar);
        
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdEnableSwComp, boolDBVar);
    if (KErrNone != err)
    	{
    	__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPPPMetaConnectionProvider:\tSetAccessPointConfigFromDbL() - failed to read EnableSwComp from database. Defaulting to false.")); ) 	    		    		
    	}
    SetEnableSwComp(boolDBVar);

	// IsDialIn value is provisioned by the Agent Provider.
    }
    
//-=========================================================
// CPppAuthConfig methods
//-=========================================================
EXPORT_C CPppAuthConfig* CPppAuthConfig::NewLC(ESock::CCommsDatIapView* aIapView)
	{
	CPppAuthConfig* self = new (ELeave) CPppAuthConfig;
	CleanupStack::PushL(self);
	self->InitialiseConfigL(aIapView);
	return self;
	}

void CPppAuthConfig::InitialiseConfigL(ESock::CCommsDatIapView* aIapView)
    {
    TBool boolDBVar = EFalse;
    TInt err = aIapView->GetBool(KCDTIdEnableSwComp, boolDBVar);
    SetServiceEnableSwComp(boolDBVar);
        
    }
    

//-=========================================================
// CPppProvisionInfo methods
//-=========================================================
EXPORT_C CPppProvisionInfo::~CPppProvisionInfo()
   {
	iExcessData.Close();
   }

//-=========================================================
// CPppTsyConfig methods
//-=========================================================
EXPORT_C CPppTsyConfig* CPppTsyConfig::NewLC(ESock::CCommsDatIapView* aIapView)
	{
	CPppTsyConfig* self = new (ELeave) CPppTsyConfig;
	CleanupStack::PushL(self);
	self->InitialiseConfigL(aIapView);
	return self;
	}

void CPppTsyConfig::InitialiseConfigL(ESock::CCommsDatIapView* aIapView)
	{
	HBufC* buf = NULL;
	if (KErrNone == aIapView->GetText(KCDTIdTsyName, buf))
		{
		iTsyName.Copy(*buf);		
		__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPppTSYConfig [this=%08x]::InitialiseConfigL() KCDTIdTsyName=%s"), this, &iTsyName));
		}
	else
		{
		__CFLOG_VAR((KPppMcprTag, KPppMcprSubTag, _L8("CPppTSYConfig [this=%08x]::InitialiseConfigL() Couldn't read TSY name, skipping..."), this, &iTsyName));
		}
	delete buf; 	
	}

//
// Attribute table for CPppConfig
//

EXPORT_START_ATTRIBUTE_TABLE_AND_FN( CPppLcpConfig, CPppLcpConfig::EUid, CPppLcpConfig::ETypeId )
// No attributes registered as no serialisation takes place.
END_ATTRIBUTE_TABLE()

EXPORT_START_ATTRIBUTE_TABLE_AND_FN( CPppAuthConfig, CPppAuthConfig::EUid, CPppAuthConfig::ETypeId )
// No attributes registered as no serialisation takes place.
END_ATTRIBUTE_TABLE()

EXPORT_START_ATTRIBUTE_TABLE_AND_FN( CPppProvisionInfo, CPppProvisionInfo::EUid, CPppProvisionInfo::ETypeId)
// No attribute definitions because we're not serialising/de-serialising
END_ATTRIBUTE_TABLE()

EXPORT_START_ATTRIBUTE_TABLE_AND_FN( CPppTsyConfig, CPppTsyConfig::EUid, CPppTsyConfig::ETypeId)
// No attribute definitions because we're not serialising/de-serialising
END_ATTRIBUTE_TABLE()
