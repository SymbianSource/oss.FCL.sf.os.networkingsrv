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

/**
 @file
 @internalComponent 
*/

#if !defined(__VJDECOMP_INL_)
#define __VJDECOMP_INL_

inline void CVJDeCompressor::SetFlag(const TUint aFlag)
/**
Set an internal flag.

@param aFlag Flag value to set
*/
	{
	iFlags |= aFlag;
	return;
	}
	
inline void CVJDeCompressor::ClearFlag(const TUint aFlag)
/**
Clear an internal flag.

@param aFlag Flag value to clear
*/
	{
	iFlags &= ~aFlag;
	return;
	}

inline TBool CVJDeCompressor::TestFlag(const TUint aFlag)
/**
Test if an internal flag is set.

@param aFlag Flag value to test

@return Status of flag
*/
	{
	return (iFlags & aFlag);
	}

#endif // __VJDECOMP_INL_
