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
// TunnelMCPR.CPP
// Tunnel MCPR
// 
//

/**
 @file
 @internalComponent
*/

#include "tunnelProvision.h"
#include <comms-infras/ss_tiermanagerutils.h>
#include <commsdattypeinfov1_1.h>

using namespace ESock;
using namespace CommsDat;

void TTunnelProvision::InitialiseConfigL(CCommsDatIapView* aIapView)
    {
	iInfo.iLocalIap = aIapView->IapId();					// IAP

	TUint32 val;

    aIapView->GetIntL(KCDTIdIAPNetwork, val);					// Network Id
	iInfo.iLocalNetwork = val;

    TInt err;
	err = aIapView->GetInt(KCDTIdVPNIAPRecord, val); // VPN IAP
    if(err == KErrNotFound)
    	val = 0; /* Hard code to 0 for SNAP, should be backed up by appropriate configuration*/
    else if(err != KErrNone)
    	User::Leave(err);
    val = (val & KCDMaskShowRecordId) >> 8;
    
	// form the Tunnel CFProtocol name
	_LIT(KNifNameFormat, "tunnelnif[0x%08x][%d:%d]");
	
	iInfo.iIfName.Format(KNifNameFormat, this, iInfo.iLocalIap, val);
    }

//
// Attribute table for provisioning structure passed to CFProtocol
//

START_ATTRIBUTE_TABLE(TTunnelProvision, TTunnelProvision::EUid, TTunnelProvision::ETypeId)
// No attributes defined - no serialisation performed
END_ATTRIBUTE_TABLE()
