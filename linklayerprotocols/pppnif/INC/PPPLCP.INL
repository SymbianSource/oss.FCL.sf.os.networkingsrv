// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

/**
 @file
 @internalComponent 
*/

inline const CIPConfig* CPppLcp::GetNcpConfig() const
    {
    return iPppNcpConfig;
    }
    
inline const CPppLcpConfig* CPppLcp::GetLcpConfig() const
    {
    return iPppLcpConfig;
    }
    
inline const CBCAProvision* CPppLcp::GetBCAProvision() const
    {
    return iBCAProvision;
    }    

inline const CCredentialsConfig* CPppLcp::GetCredentials() const
    {
    return iCredentials;
    }

inline const CPppAuthConfig* CPppLcp::GetAuthConfig() const
    {
    return iPppAuthConfig;
    }

inline const CAgentProvisionInfo* CPppLcp::GetAgentConfig() const
	{
	return iAgentProvision;
	}

inline const CPppProvisionInfo* CPppLcp::GetPppAgentConfig() const
	{
	return iPppAgentProvision;
	}

inline void CPppLcp::CallbackRequestGranted()	
	{
	iCallbackRequestGranted = ETrue;
	}

inline CPppLcpConfig::TPppLinkMode CPppLcp::PppLinkMode() const
	{
	return iLinkMode;
	}
	
/** Set the MTU */
inline void CPppLcp::SetMaxTransferSize(TInt aLength)
	{
	iMaxSendSize = aLength;
	}

/** Return the MTU */
inline TInt CPppLcp::MaxTransferSize() const
	{
	return iMaxSendSize;
	}

/** Set the MRU */
inline void CPppLcp::SetMaxReceiveSize(TInt aSize)
	{
	iMaxRecvSize = aSize;
	}
	
/** Return the MRU */
inline TInt CPppLcp::MaxReceiveSize() const
	{
	return iMaxRecvSize;
	}

inline const TDesC8& CPppLcp::GetExcessData() const
	{
	if (GetPppAgentConfig())
		{
		return GetPppAgentConfig()->ExcessData();
		}
	else
		{
		return KNullDesC8;
		}
	}
