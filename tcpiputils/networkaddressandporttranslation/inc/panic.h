// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @prototype
*/

#ifndef __NAPT_PANIC_H__
#define __NAPT_PANIC_H__

#include <in_sock.h>
#include <ext_hdr.h>
#include <es_prot.h>



enum TNAPTPanic
	{
	ENAPTPanic_BadBind,
	ENAPTPanic_BadHeader,
	ENAPTPanic_BadCall,
	ENAPTPanic_BadIndex,
	ENAPTPanic_IoctlFailed,
	ENAPTPanic_CorruptPacketIn,
	ENAPTPanic_Ip6NotSupported
	};

GLREF_C void Panic(TNAPTPanic aPanic);




#endif //__NAPT_PANIC_H__
