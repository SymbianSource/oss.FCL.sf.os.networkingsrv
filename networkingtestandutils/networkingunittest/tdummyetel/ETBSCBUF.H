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

#if !defined(__ET_BSCBUF_H__)
#define __ET_BSCBUF_H__

/**
Number of server-side slots for each notification (greater than 1)
@internalComponent
*/
//
//	RBasicGsmCall
//
const TUint KSlotNumbersGsmCallCapsChange	= 2;

//
//	RBasicGsmPhone
//
const TUint KSlotNumbersPhoneCurrentNetworkChange				= 2;
const TUint KSlotNumbersPhoneNetworkRegistrationStatusChange	= 3;
const TUint KSlotNumbersPhoneIndicatorChange					= 3;

//
//	RGsmPhoneBook
//
const TUint KSlotNumbersPhoneBookEntryChange =5;

//
//	RSmsStorage
//
const TUint KSlotNumbersSmsStorageWrite		= 5;

//
//	RSmsMessaging
//
const TUint KSlotNumbersSmsMessageArrived	= 5;
const TUint KSlotNumbersSmsMessagingRead	= 5;


#endif	  
