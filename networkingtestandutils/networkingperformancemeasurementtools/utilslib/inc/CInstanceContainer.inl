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
// instanceManager.inl
// CInstanceContainer class definition
// 
//

template <class TInstance> 
CInstanceContainer<TInstance>::CInstanceContainer() : iDefaultInstance(NULL) 
{
}

template <class TInstance> 
CInstanceContainer<TInstance>::~CInstanceContainer() 
{
    Remove(true);
}

// add an instance
template <class TInstance> 
bool CInstanceContainer<TInstance>::Add(TInstance &aInstance, bool aIsDefault)
{
    // check/manage default session
    if (aIsDefault)
    {
        if (iDefaultInstance)
        {
            // delete &aInstance;
            printf("default instance already exists (ie. can't create another one) = 0x%08x\n", iDefaultInstance);
			return false;
        }
        else
            iDefaultInstance = &aInstance;
    }
    iInstances.push_back(&aInstance);
	return true;
}

// get instance - default
template <class TInstance> 
TInstance *CInstanceContainer<TInstance>::Get()
{
    if (iDefaultInstance)
        return Get(iDefaultInstance);

	printf("default instance does not exist = 0x%08x\n", iDefaultInstance);
	return NULL;
}

// get instance - confirm specified instance has been previously registered (ie. exists)
template <class TInstance> 
TInstance *CInstanceContainer<TInstance>::Get(TInstance &aInstance)
{
    // find matching registered instance
	std::vector<TInstance *>::iterator instanceIterator;
    for (instanceIterator = iInstances.begin(); instanceIterator != iInstances.end(); instanceIterator++)
    {
        if (*instanceIterator == &aInstance)
            return &aInstance;
    }
    printf("no matching registered instance found, instance = %08x\n", &aInstance);
	return NULL;
}

// get instance - from CCall reference
template <class TInstance> 
TInstance *CInstanceContainer<TInstance>::Get(const CCall &aCall)
{
    // get instance handle
    TInstance *instance;
    if (aCall.Get("handle", (int &) instance))
    {
        if (instance == NULL)
            instance = Get(*iDefaultInstance);         // verify instance has been registered
    }

    // return instance
    if (instance)
        return instance;

    printf("no default instance, and missing/invalid parameters; \"handle\"\n");
	return NULL;
}

// remove instance - from CCall reference
template <class TInstance> 
bool CInstanceContainer<TInstance>::Remove(const CCall &aCall)
{
    return Remove(*Get(aCall));
}

// remove instance - default or all
template <class TInstance> 
bool CInstanceContainer<TInstance>::Remove(bool aRemoveAll)
{
    if (aRemoveAll)
    {
        while (iInstances.size() > 0)
            Remove(**iInstances.begin());
		return true;
    }
	else if (iDefaultInstance)
		return Remove(*iDefaultInstance);
	return false;
}

// remove instance - from instance handle
template <class TInstance> 
bool CInstanceContainer<TInstance>::Remove(const TInstance &aInstance)
{
    // remove registered instance
	std::vector<TInstance *>::iterator instanceIterator;
    for (instanceIterator = iInstances.begin(); instanceIterator != iInstances.end(); instanceIterator++)
    {
        if (*instanceIterator == &aInstance)
        {
            delete &aInstance;
            if (*instanceIterator == iDefaultInstance)
                iDefaultInstance = NULL;
            iInstances.erase(instanceIterator);
            return true;
        }
    }

    printf("no matching registered instance found, instance = %08x\n", &aInstance);
	return false;
}
