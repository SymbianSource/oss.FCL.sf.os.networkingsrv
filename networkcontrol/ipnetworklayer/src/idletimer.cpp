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
//

#include <e32base.h>
#include "IPProtoCPR.h"

/**
 defines a base implenetation of the timers that may be incorporated into a linkcpr
 */

#include <comms-infras/ss_log.h>
#include <comms-infras/metatype.h>
#include "idletimer.h"


START_ATTRIBUTE_TABLE(TPacketActivity, TPacketActivity::iUid, TPacketActivity::iId)
	REGISTER_ATTRIBUTE(TPacketActivity, iPacketActivity, TMeta<TBool*>)
END_ATTRIBUTE_TABLE()

START_ATTRIBUTE_TABLE(TIdleTimerValues, TIdleTimerValues::iUid, TIdleTimerValues::iId)
	REGISTER_ATTRIBUTE(TIdleTimerValues, iShortTimer, TMetaNumber)
	REGISTER_ATTRIBUTE(TIdleTimerValues, iMediumTimer, TMetaNumber)
	REGISTER_ATTRIBUTE(TIdleTimerValues, iLongTimer, TMetaNumber)
END_ATTRIBUTE_TABLE()
