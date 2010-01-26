/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header for Connection Settings used by CDialogProcessor
* and DBAccess Classes
* 
*
*/



/**
 @file CONNECTIONSETTINGS.H
 @internalTechnology
*/


#ifndef __CONNECTIONSETTINGS_H__
#define __CONNECTIONSETTINGS_H__

#include <d32comm.h>
#include <commdb.h> //todo move the constants
#include <commsdattypesv1_1.h>
using namespace CommsDat;

const TUint KCharT='T';
const TUint KCharP='P';
const TUint KSlashChar='\\';

const TUint KDCDHandshakeMask=KConfigObeyDCD|KConfigFailDCD;
const TUint KDSRFailHandshakeMask=KConfigFailDSR;
const TUint KCTSHandshakeMask=KConfigObeyCTS|KConfigFailCTS;
const TUint KNoHandshakingMask=~(KConfigObeyXoff | KConfigSendXoff | KConfigObeyCTS | 
		KConfigFailCTS | KConfigObeyDSR | KConfigFailDSR | KConfigObeyDCD | KConfigFailDCD );

class TConnectionSettings
/**
@internalTechnology
*/
	{
public:
	IMPORT_C TConnectionSettings();
public:
	TUint32 iRank;
	TCommDbConnectionDirection iDirection;
	TCommDbDialogPref iDialogPref;
	TUint32 iBearerSet;
	TUint32 iIAPId;
	TUint32 iServiceId;
	TBuf<KCommsDbSvrMaxColumnNameLength> iServiceType;
	TUint32 iBearerId;
	TBuf<KCommsDbSvrMaxFieldLength> iBearerType;
	TUint32 iLocationId;
	TUint32 iChargeCardId;
	};

#endif

