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


#ifndef __NTRAS_H__
#define __NTRAS_H__

#include <nifvar.h>

/**
@internalComponent
*/
_LIT(RAS_COMMSPEED,"CommSpeed");
_LIT(RAS_COMMDATA,"CommData");
_LIT(RAS_COMMHANDSHAKE,"CommHandshake");
_LIT(RAS_AUTHENTICATENAME,"AuthenticateName");
_LIT(RAS_AUTHENTICATEPASSWORD,"AuthenticatePassword");

_LIT(RAS_YESTOCONNECT,"YesToConnect");
_LIT(RAS_YESTORECONNECT,"YesToReconnect");

/**
@internalComponent
*/
enum ENtRasProgress
	{
	ERasUninitialised           = KMinAgtProgress,
	ERasNoConnection,
	ERasStartingSelection,
	ERasFinishedSelection,
	ERasConnectionOpen          = KConnectionOpen,
	ERasStartingHangUp,
	ERasFinishedHangUp          = KConnectionClosed
	};

#endif
