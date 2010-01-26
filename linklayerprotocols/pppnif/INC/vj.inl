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

#if !defined(__VJ_INL_)
#define __VJ_INL_

/**
Sets a flag indicating that the stored object contains a valid TCP/IP header.

@see IsValid()
*/
inline void TVJCompHdr::MarkValid()
	{
	iConnectionId |= KVJValidFlag;
	}

/**
Determines if the stored TCP/IP header contains valid data.

@see MarkValid()

@return ETrue if the object contains valid headers
*/
inline TBool TVJCompHdr::IsValid() const
	{
	return !!(iConnectionId & KVJValidFlag);
	}

/**
Sets the pointer to the next object in the linked list.

@param aNextPtr Another TVJCompHdr object
*/
inline void TVJCompHdr::SetNextPtr(TVJCompHdr* aNextPtr)
	{
	iNextPtr = aNextPtr;
	}

/**
Returns the next object in the linked list.

@return The next TVJCompHdr object
*/
TVJCompHdr* TVJCompHdr::NextPtr() const
	{
	return iNextPtr;
	}

/**
Sets the VJ connection number.
Also clears the valid flag as a side effect.

@param aConnection VJ connection number (0..255)
*/
inline void TVJCompHdr::SetConnectionNumber(TUint aConnection)
	{
	iConnectionId = aConnection;
	}

/**
Returns the VJ connection number.

@return VJ connection number
*/
inline TUint TVJCompHdr::ConnectionNumber() const
	{
	return iConnectionId & 0xff;	// Strip KVJValidFlag
	}


#endif // __VJ_INL_

