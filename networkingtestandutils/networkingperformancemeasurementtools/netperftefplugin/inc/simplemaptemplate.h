// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Simple map class to allow us to store our worker threads by name 
// 
//

/**
 @file
 @internalTechnology
*/
 
#if (!defined SIMPLEMAPTEMPLATE_H)
#define SIMPLEMAPTEMPLATE_H

#include <e32base.h>
#include <e32std.h>

template<class T>
class RPointerDesMap
	{
public:
    inline TInt Add(T* aElement, const TDesC& aName);
    inline T* Find(const TDesC& aName);
    inline void Remove(const TDesC& aName);
    inline void Reset();
    inline void ResetAndDestroy();

private:
    static TBool CompareDescriptors(const TDesC& aLeft, const TDesC& aRight);

private:
    RPointerArray<TDesC> iNames;
    RPointerArray<T> iElements;
	};

template<class T>
inline TBool RPointerDesMap<T>::CompareDescriptors(const TDesC& aLeft, const TDesC& aRight)
    {
    return (aLeft.CompareF(aRight)==0);
    }

template<class T>
inline TInt RPointerDesMap<T>::Add(T* aElement, const TDesC& aName)
    {
    __ASSERT_DEBUG(aElement!=NULL,User::Panic(_L("NULL pointer"),KErrGeneral));

    if (Find(aName) != NULL)
        return KErrAlreadyExists;

    HBufC* copy = aName.Alloc();
    if (copy == NULL)
        return KErrNoMemory;

    TInt error = iNames.Append(copy);
    if (error == KErrNone)
        {
        error = iElements.Append(aElement);
        if (error != KErrNone)
            {
            iNames.Remove(iNames.Count()-1);
            delete copy;
            }
        }
    return error;
    }

template<class T>
inline T* RPointerDesMap<T>::Find(const TDesC& aName)
    {
    TInt idx = iNames.Find(&aName,TIdentityRelation<TDesC>(CompareDescriptors));
    T* element = NULL;
    if (idx>=0)
        {
        element = iElements[idx];
        __ASSERT_DEBUG(element!=NULL,User::Panic(_L("NULL pointer"),KErrGeneral));
        }
    return element;
    }

template<class T>
inline void RPointerDesMap<T>::Remove(const TDesC& aName)
    {
    TInt idx = iNames.Find(&aName,TIdentityRelation<TDesC>(CompareDescriptors));
    __ASSERT_DEBUG(idx>=0,User::Panic(_L("Element does not exist"),KErrGeneral));
    iNames.Remove(idx);
    iElements.Remove(idx);
    }

template<class T>
inline void RPointerDesMap<T>::Reset()
    {
    iNames.Reset();
    iElements.Reset();
    }

template<class T>
inline void RPointerDesMap<T>::ResetAndDestroy()
    {
    iNames.ResetAndDestroy();
    iElements.ResetAndDestroy();
    }


#endif //SIMPLEMAPTEMPLATE_H

