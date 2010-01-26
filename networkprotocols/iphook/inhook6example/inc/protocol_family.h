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
// protocol_family.h - generic protocol family class
//

#ifndef __PROTOCOL_FAMILY_H
#define __PROTOCOL_FAMILY_H

#include <es_prot.h>

/**
* @file protocol_family.h
*
* @note
*	This file is only included in protocol_family.cpp. A separate
*	header file is only needed for technical reasons due to doxygen
*	setup (otherwise the content of this would be directly in the
*	cpp file).
* @internalComponent
*/

class CProtocolFamilyGeneric : public CProtocolFamilyBase
	/**
	* Generic protocol family implementation.
	*
	* This class implements the CProtocolFamilyBase assuming that
	* the actual protocol is implemented behind the ProtocolModule
	* specification (API).
	*/
	{
public:
	CProtocolFamilyGeneric();
	~CProtocolFamilyGeneric();
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	CProtocolBase* NewProtocolL(TUint aSockType, TUint aProtocol);
	};

#endif
