// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the DHCP Message base
// 
//

/**
 @file DHCPMsg.cpp
 @internalTechnology
*/

#include "DHCPMsg.h"

	
#ifdef __FLOG_ACTIVE
void CDHCPMessageHeader::Dump()
	{
	iRecord.Dump( _L("DHCP"), _L("DHCP")  );
	}
#endif
	
	
void CDHCPMessageHeader::InitialiseL()
/**
  * Initialise the message, wipe previous info
  * if the descriptor is being reused
  *
  * @internalTechnology
  */
	{
   RemoveAllOptions();
	TPtr8 ptr(iMsg->Des());
	ptr.FillZ(ptr.MaxLength());	
	ptr.SetLength(0);
	iRecord.InitialiseL(ptr);
	}
