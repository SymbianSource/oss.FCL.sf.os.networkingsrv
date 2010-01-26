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



/**
 @internalComponent
*/
#ifndef __QOS_FAMILY_H__
#define __QOS_FAMILY_H__


#include <es_prot.h>

class CProtocolFamilyQoS : public CProtocolFamilyBase
	{
public:
	CProtocolFamilyQoS();
	~CProtocolFamilyQoS();
public:
	TInt Install();
	TInt Remove();
	CProtocolBase *NewProtocolL(TUint aSockType, TUint aProtocol);
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	};

#endif
