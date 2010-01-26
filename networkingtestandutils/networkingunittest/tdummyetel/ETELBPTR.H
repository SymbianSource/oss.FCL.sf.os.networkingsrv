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

#if !defined(__ETELBPTR_H__)
#define __ETELBPTR_H__

#if !defined(__ETELBGSM_H__)
#include "ETelBgsm.h"
#endif

class CBasicPtrHolder : public CBase
/**
Contains the TPtr's used by asynchronous client-side functions.
Used by Client-side code to ensure BC by keeping size of Rxxx classes constant even if a 
new function has to be added.
@internalComponent
*/
	{
public:
	static CBasicPtrHolder* NewL(const TInt aSize);
	~CBasicPtrHolder();
public:
	TPtr8& Ptr(const TInt aIndex);
protected:
	CBasicPtrHolder();
	virtual void ConstructL(const TInt aSize);	
protected:
	CArrayFixFlat<TPtr8>* iPtrArray;
public:
	TInt iGetNetworkInfoValue;
	RBasicGsmPhone::TBscNetworkId iNetworkIdValue;
	RBasicGsmPhone::TNetworkSetMode iNetworkMode;
	TInt iMsgStoreIndex;
	TInt iSetAlarmIndex;
	TInt iGetAlarmIndex;
	TInt iPhoneBookInfoIndex;
	};


#endif
