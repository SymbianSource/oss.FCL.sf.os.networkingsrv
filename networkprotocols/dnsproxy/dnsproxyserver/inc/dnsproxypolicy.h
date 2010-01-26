// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Definitions header file for DNS Proxy Server Security Policies
//



/**
 @file
 @internalComponent
*/

#include "dnsproxyclientconfigparams.h"

const TInt CDnsProxyServer::PolicyRanges[] =
    {  
    EProxyConfigure,
    EProxyAddDb,
    EProxyRemoveDb,
    EProxyUpdateDomainName,
    EProxyConfigureUplink,
    ENotSupported
    };

//No of elements in the array
const TUint CDnsProxyServer::PolicyRangeCount = (sizeof(PolicyRanges) / sizeof(PolicyRanges[0]));

// Index numbers into CDnsProxyServer[]

const TInt policyNetwork = 0;
const TInt policyAlwaysPass = 1;

// Mapping of IPCs to policy elements

const TUint8 CDnsProxyServer::PolicyElementsIndex[PolicyRangeCount] =
	{
	1,								//EProxyConfigure
	2,								//EProxyAddDb
	2,								//EProxyRemoveDb
	2,								//EProxyUpdateDomainName							
	1,    							//EProxyConfigureUplink                          
	CPolicyServer::ENotSupported 	//EProxyConfigureUplink + 1	
	};

// Individual policy elements

const CPolicyServer::TPolicyElement CDnsProxyServer::PolicyElements[] =
	{
	{ _INIT_SECURITY_POLICY_PASS },
	{ _INIT_SECURITY_POLICY_C2(ECapabilityNetworkControl ,ECapabilityNetworkServices), CPolicyServer::EFailClient },
	{ _INIT_SECURITY_POLICY_C1(ECapabilityReadDeviceData), CPolicyServer::EFailClient },
	};

// Main policy

const CPolicyServer::TPolicy CDnsProxyServer::Policy =
	{
	policyNetwork, 	//specifies all connect attempts should use policy 0 from CDnsProxyServer::PolicyElements
	PolicyRangeCount,
	PolicyRanges,
	PolicyElementsIndex,
	PolicyElements,
	};
