// Copyright (c) 2000-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Contains method definitions for EtelMM subsession RMobileLine
// 
//

// From core API
#include "ETELEXT.H"

// Multimode header files
#include "ETELMM.H"
#include "mm_hold.h"


/************************************************************************/
//
//  RMobileLine
//
/************************************************************************/

EXPORT_C RMobileLine::RMobileLine()
	: iMmPtrHolder(NULL)
	{
	}

EXPORT_C void RMobileLine::ConstructL()
	{
	RLine::ConstructL();
	__ASSERT_ALWAYS(iMmPtrHolder==NULL,PanicClient(EEtelPanicHandleNotClosed));
	iMmPtrHolder = CMobileLinePtrHolder::NewL(CMobileLinePtrHolder::EMaxNumberLinePtrSlots,CMobileLinePtrHolder::EMaxNumberLinePtrCSlots);
	}

EXPORT_C void RMobileLine::Destruct()
	{
	RLine::Destruct();
	delete iMmPtrHolder;
	iMmPtrHolder = NULL;
	}

EXPORT_C TInt RMobileLine::GetMobileLineStatus(RMobileCall::TMobileCallStatus& aStatus) const
/**
 * This method returns the current status of a mobile line 
 *
 * \retval aStatus Will contain the retrieved line state
 * \return KErrNone
 */
	{
	TPckg<RMobileCall::TMobileCallStatus> ptr1(aStatus);
	return Get(EMobileLineGetMobileLineStatus, ptr1);
	}

EXPORT_C void RMobileLine::NotifyMobileLineStatusChange(TRequestStatus& aReqStatus, RMobileCall::TMobileCallStatus& aStatus) const
/**
 * This notification completes when the mobile line's state changes
 *
 * \retval aStatus Will contain the new line state
 */
	{
	__ASSERT_ALWAYS(iMmPtrHolder!=NULL,PanicClient(EEtelPanicNullHandle));

	TPtr8& ptr1=iMmPtrHolder->Set(CMobileLinePtrHolder::ESlot1LineNotifyMobileLineStatusChange, aStatus);

	Get(EMobileLineNotifyMobileLineStatusChange, aReqStatus, ptr1);
	}

