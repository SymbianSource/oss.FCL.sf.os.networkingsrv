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
// res.h - name resolver
//



/**
 @internalComponent
*/
#ifndef __RES_H__
#define __RES_H__

#include <es_prot.h>

const TUint KProtocolInet6Res	= 0xF01;

//	***
//	RES
//	***
//	A minimal static interface for setting up the 'resolver'
//	protocol instance.
//
class CIfManager;
class RES
	{
public:
	static CProtocolBase *NewL(CIfManager *const aInterfacer);
	static void Identify(TServerProtocolDesc &aEntry);
	};
#endif
