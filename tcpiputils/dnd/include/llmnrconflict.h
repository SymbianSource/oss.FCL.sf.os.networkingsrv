// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// llmnrconflict.h - Interface definitions LLMNR name conflict notification
// Interface definitions for the LLMNR name conflict notification
//



/**
 @file llmnrconflict.h
 @publishedPartner
 @released
*/

#ifndef __LLMNRCONFLICT_H__
#define __LLMNRCONFLICT_H__

#include <es_sock.h>

/**
* The notification UID
*/
TInt KLlmnrConflictNotifyUid = 0x101F6D23;

class TLlmnrConflictName
	{
public:
	inline TBool IsSafe() const;
	THostName	iName;				//< The host name.
	};

class TLlmnrConflictNotify : public TLlmnrConflictName
	/**
	* Conflict notification message format.
	*
	* The base iName contains the link local name which is conflict
	* with some other host on some interface assotiated with the
	* indicated IAP (iIAPId).
	*/
	{
public:

	TUint32		iIAPId;				//< ID of selected IAP
	TUint32		iNetworkId;			//< ID of Network to which IAP belongs
	};
	
/**
* The reply from notifier only has the replacement host name.
*
* The the nofitier completes with KErrNone, the reply message can contain
* a the new name. If this new name is also colliding with someone, another
* notify is issued.
*
* If the notifier does not want LLMNR to try an alternate name, it should
* either complete with some error (KErrCancel for example), or KErrNone
* with empty replacement name (iName.Length() == 0).
*/
typedef TLlmnrConflictName TLlmnrConflictReply;


TBool TLlmnrConflictName::IsSafe() const
	/**
	* Test if iName really is a TBuf derived descriptor.
	*/
	{
	// This test is defined, because this structure is passed as a parameter,
	// and any sender of the message can crash the receiver by placing tailored "garbage"
	// on TBuf descriptor. The receiver has no official way of validating that TBuf
	// descriptor is correct, and the garbage can be such that any reference to the
	// content of TBuf will cause panic (for example place into iTypeLength field a
	// value that implies TPtr and NULL into actual buffer address).
	//
	// This method gives some (but not 100%) confidence that accessing the iName
	// does not cause a panic. This test works on assumtion that first TInt of
	// the descriptor holds "type length" information, and that type of a TBuf is
	// 0.
	//
	// Something like this sbould be provided by the descriptor class itself!
	//
	return ((iName.Length() | 0x30000000) == (TInt &)iName &&	// ... is really a TBuf?
			iName.Length() <= iName.MaxLength() &&				// ... length & maxlength seem reasonable
			iName.MaxSize() >= (sizeof(THostName) - 2*sizeof(TInt)));
	}

#endif
