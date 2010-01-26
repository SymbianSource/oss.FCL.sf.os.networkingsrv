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
// instanceManager.h
// CInstanceContainer class decleration
// - functionality;
// a) container for references to instances of an arbitrary type
// b) default instance mechanism
// - possible future enhancements;
// a) linked list - instead of vector which can suffer performance penalties for removing & finding elements
// b) garbage collection - automatic timed removal of un-referenced instances
// 
//

#ifndef CINSTANCE_CONTAINER_H
#define CINSTANCE_CONTAINER_H

#include <vector>
#include "ccall.h"

template<class TInstance> class CInstanceContainer
{
public:
    CInstanceContainer();
	~CInstanceContainer();

    bool Add(TInstance &aInstance, bool aIsDefault);
    
    TInstance *Get();
    TInstance *Get(TInstance &aInstance);
    TInstance *Get(const CCall &aCall);

    bool Remove(const CCall &aCall);
    bool Remove(bool aRemoveAll = false);
    bool Remove(const TInstance &aInstance);

private:
	// container of instances - for consistency, also includes the default instance handle
	std::vector<TInstance *> iInstances;
	// default instance - exists so that caller don't need to pass a dynamic instance value (aka no session based handle)
    TInstance *iDefaultInstance;
};

#include "CInstanceContainer.inl"
#endif // CINSTANCE_CONTAINER_H
