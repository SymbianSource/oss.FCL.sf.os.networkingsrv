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

#if !defined(__LOG_H__)
#define __LOG_H__

#include <comms-infras/commsdebugutility.h>

#ifdef _DEBUG
#define LOG(a)		a
#else
#define LOG(a)
#endif

#ifdef _DEBUG
class Log
	{
public:
	static void Write(const TDesC& aDes);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	};
#endif
#endif
