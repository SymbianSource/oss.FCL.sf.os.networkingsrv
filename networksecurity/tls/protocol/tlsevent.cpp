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
// SSL3.0/TLS1.0 Asynchronous events implementation file.
// 
//

/**
 @file
*/
  
#include "tlsevent.h"

TBool CTlsEvent::AcceptRecord( TInt aRecordType ) const
/**
 * This is a virtual method that determines if a record protocol's content type
 * should be accepted.
 */
{
	LOG(Log::Printf(_L("CTlsEvent::AcceptRecord(). record type = %d"), aRecordType ));
   (void)aRecordType;
	return EFalse;
}
