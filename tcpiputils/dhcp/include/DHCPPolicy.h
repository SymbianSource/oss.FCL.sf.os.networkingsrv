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
// Definitions header file for DHCP Security Policies
// 
//

/**
 @file 
*/

#ifdef SYMBIAN_NETWORKING_PLATSEC
#include <comms-infras/rconfigdaemonmess.h>
#else
#include <comms-infras\cs_daemonmess.h>
#endif

const TUint CDHCPServer::PolicyRangeCount = 2;

const TInt CDHCPServer::PolicyRanges[PolicyRangeCount] = 
	{
	EConfigDaemonConfigure,			// AlwaysPass (connect policy requires NetworkControl + NetworkServices)
	EConfigDaemonDeregister + 1			// fail (to KMaxTInt)
	};

// Index numbers into DHCPServerElements[]

const TInt policyNetwork = 0;
const TInt policyAlwaysPass = 1;

// Mapping of IPCs to policy elements
//
// NOTE: all IPCs marked to always pass once the calls have past the "connection" policy check
//		 (to improve efficiency).

const TUint8 CDHCPServer::PolicyElementsIndex[PolicyRangeCount] = 
	{
	policyAlwaysPass,				// EConfigDaemonConfigure
									// EConfigDaemonIoctl
									// EConfigDaemonCancel
									// EConfigDaemonDeregister
	CPolicyServer::ENotSupported	// EConfigDaemonDeregister + 1 to KMaxTInt
	};

// Individual policy elements

const CPolicyServer::TPolicyElement CDHCPServer::PolicyElements[] = 
	{
	{ _INIT_SECURITY_POLICY_C2(ECapabilityNetworkControl, ECapabilityNetworkServices), CPolicyServer::EFailClient },
	{ _INIT_SECURITY_POLICY_PASS },
	};

// Main policy
	
const CPolicyServer::TPolicy CDHCPServer::Policy =
	{
	policyNetwork, 	//specifies all connect attempts should use policy 0 from CDHCPServer::PolicyElements
	PolicyRangeCount,					
	PolicyRanges,
	PolicyElementsIndex,
	PolicyElements,
	};
