/**
* Copyright (c) 2002-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file NI_LOG.H
 @internalComponent
*/


#if !defined(__NI_LOG_H__)
#define __NI_LOG_H__

#include <comms-infras/commsdebugutility.h>

#if defined(__FLOG_ACTIVE)


/**
@internalComponent
*/
	#ifndef LOG
		#define LOG(MSG) MSG
	#endif

	#if defined (DETAILED_LOG)

/**
@internalComponent
*/
		#ifndef LOG_DETAILED
			#define LOG_DETAILED(MSG) MSG
		#endif
	#else
		#ifndef LOG_DETAILED
			#define LOG_DETAILED(MSG)
		#endif
	#endif

/**
@internalComponent
*/
	_LIT(KNifmanLogFolder,"Nifman");
	_LIT(KNifmanLogFile,"Nifman.txt");

/**
@internalComponent
*/
	_LIT(KEndOfLine,"\r\n");

/**
@internalComponent
*/
	const TInt KNifmanLogHexDumpWidth = 16;

	class NifmanLog
/**
@internalComponent
*/
		{

	public:
	    static void Printf(TRefByValue<const TDesC> aFmt, ...);
	    static void Printf(TRefByValue<const TDesC8> aFmt, ...);
		};

#else

	#define LOG(MSG)
	#define LOG_DETAILED(MSG)

#endif

#endif // __NI_LOG_H__

