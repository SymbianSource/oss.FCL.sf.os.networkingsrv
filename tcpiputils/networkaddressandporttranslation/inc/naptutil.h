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
// NAPT utility declaration.
// 
//

/**
 @file
 @internalTechnology
*/
#ifndef __NAPTUTIL__
#define __NAPTUTIL__
#include <e32std.h>
#include <e32base.h>
#include <nifman.h>
#include <metadatabase.h>
#include <commsdattypesv1_1.h>
#include <in_sock.h>

/** 
  *
  * This calss is used to fectdh DHCP server IP address from commsdb and generates client IP for provisioning.
  * @internalTechnology
  *
  */
class TNaptUtil
	{
public:
	static TInt GetClientIp(TInt aIapId, TUint32& aClientIp);
private:
	static void GetClientIpFromCommsL(TInt aIapId, TUint32& aClientIp);
	static void OpenIAPViewLC(CMDBSession*& aSession, CCDIAPRecord*& aIapRecord, TInt aIapId);
	static void InitialServiceLinkL(CMDBSession* aDbSession, CCDIAPRecord* aIapRecord);
	};
#endif //__NAPTUTIL__
	