// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// PPP CFProtocol Provisioning Message inline methods.
// CPppLcpConfig methods
// 
//

/**
 @file
 @internalTechnology
*/

CPppLcpConfig::TPppLinkMode CPppLcpConfig::GetIfServerMode() const
    {
    return iIfServerMode;
    }

TInt CPppLcpConfig::GetIfCallbackType() const
    {
    return iIfCallbackType;
    }

const TDesC& CPppLcpConfig::GetIfParams() const
    {
    return iIfParams;
    }

const TDesC& CPppLcpConfig::GetIfCallbackInfo() const
    {
    return iIfCallbackInfo;
    }

TBool CPppLcpConfig::GetIfCallbackEnabled() const
    {
    return iIfCallbackEnabled;
    }

TBool CPppLcpConfig::GetEnableLcpExtensions() const
    {
    return iEnableLcpExtensions;
    }

TBool CPppLcpConfig::GetEnableSwComp() const
    {
    return iEnableSwComp;
    }
    
void CPppLcpConfig::SetIfServerMode(CPppLcpConfig::TPppLinkMode aIfServerMode)
    {
    iIfServerMode = aIfServerMode;
    }

void CPppLcpConfig::SetIfCallbackType(TInt aIfCallbackType)
    {
    iIfCallbackType = aIfCallbackType;
    }

void CPppLcpConfig::SetIfParams(HBufC* aIfParams)
    {
    iIfParams.Close();
   	iIfParams.Assign(aIfParams);    
    }

void CPppLcpConfig::SetIfCallbackInfo(HBufC* aIfCallbackInfo)
    {
    iIfCallbackInfo.Close();
   	iIfCallbackInfo.Assign(aIfCallbackInfo);         
    }

void CPppLcpConfig::SetIfCallbackEnabled(TBool aIfCallbackEnabled)
    {
    iIfCallbackEnabled = aIfCallbackEnabled;
    }

void CPppLcpConfig::SetEnableLcpExtensions(TBool aEnableLcpExtensions)
    {
    iEnableLcpExtensions = aEnableLcpExtensions;
    }

void CPppLcpConfig::SetEnableSwComp(TBool aEnableSwComp)
    {
    iEnableSwComp = aEnableSwComp;
    }

#if defined (_DEBUG)
const TDesC& CPppLcpConfig::GetISPName() const
    {
    return iISPName;
    }

void CPppLcpConfig::SetISPName(HBufC* aISPName)
    {
    iISPName.Close();
   	iISPName.Assign(aISPName);
    }

#endif       







//-=========================================================
// CPppAuthConfig methods
//-=========================================================

TBool CPppAuthConfig::GetServiceEnableSwComp() const
    {
    return iServiceEnableSwComp;
    }

void CPppAuthConfig::SetServiceEnableSwComp(TBool aServiceEnableSwComp)
    {
    iServiceEnableSwComp = aServiceEnableSwComp;
    }



const TDesC8& CPppProvisionInfo::ExcessData() const { return iExcessData; }

TInt CPppProvisionInfo::IsDialIn() const { return iIsDialIn; }

TAny* CPppProvisionInfo::NotificationData() const { return iNotificationData; }

TInt CPppProvisionInfo::SetExcessData(const TDesC8& aData)
	{
	iExcessData.Close();
	return iExcessData.Create(aData);
	}

void CPppProvisionInfo::SetIsDialIn(TInt aValue)
	{
	iIsDialIn = aValue;
	}

void CPppProvisionInfo::SetNotificationData(TAny* aNotificationData)
	{
	iNotificationData = aNotificationData;
	}


const TName& CPppTsyConfig::TsyName() const
	{
	return iTsyName;
	}

void CPppTsyConfig::SetTsyName(const TName& aTsyName)
	{
	iTsyName.Copy(aTsyName);
	}
