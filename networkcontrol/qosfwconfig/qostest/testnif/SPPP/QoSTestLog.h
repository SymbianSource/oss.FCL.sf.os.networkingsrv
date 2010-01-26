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

#if !defined(QOSTESTLOG_H)
#define QOSTESTLOG_H

#include <comms-infras/commsdebugutility.h>

#if defined (_DEBUG)

#define LOG(a)		a
#else
#define LOG(a)
#endif


class PdpLog
 	{
public:
 	static void Write(const TDesC& aDes);
 	static void Printf(TRefByValue<const TDesC> aFmt, ...);
 	};
 

#endif
//QOSTESTLOG_H
