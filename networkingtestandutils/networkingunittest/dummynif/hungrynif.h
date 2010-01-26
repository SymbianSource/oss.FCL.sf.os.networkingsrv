// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 
#include "dummynif.h"
#include "dummynifvar.h"

class CHungryIfLink : public CDummyIfLink
/**
@internalComponent
*/
	{
public:
	CHungryIfLink(CNifIfFactory& aFactory);
    virtual CNifIfBase* GetBinderL(const TDesC& aName);
	};

class CHungryIf4 : public CDummyIf4
/**
@internalComponent
*/
	{
public:
	CHungryIf4(CHungryIfLink& aLink);
	void Recv(RMBufChain& aPdu);
	};

class CHungryIf6 : public CDummyIf6
/**
@internalComponent
*/
	{
public:
	CHungryIf6(CHungryIfLink& aLink);
	void Recv(RMBufChain& aPdu);
	};
