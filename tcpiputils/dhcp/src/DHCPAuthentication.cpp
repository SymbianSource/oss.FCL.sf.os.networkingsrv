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
// Implements the DHCP Authentication functionality
// 
//

/**
 @file
*/

#include "DHCPAuthentication.h"
#include "DHCPIP6IA.h"

using namespace DHCPv6;

COptionNode* CDHCPOptionAuthentication::NewL()
	{
	return new(ELeave)CDHCPOptionAuthentication();
	}

void CDHCPOptionAuthentication::CheckL( const TInterfaceConfigInfo& /*aInterfaceConfigInfo*/ )
   {
   }

void CDHCPOptionAuthentication::InitL( const TInterfaceConfigInfo& /*aInterfaceConfigInfo*/ )
   {
   }
