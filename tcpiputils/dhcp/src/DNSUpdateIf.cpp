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
// Implements an interface for creating a dynamic dns update
// plugin instance.
// 
//

/**
 @file DNSUpdateIf.cpp
 @internalTechnology
*/

#include "DNSUpdateIf.h"

CDnsUpdateIf* CDnsUpdateIf::NewL()
/**
  * Create instance of DDNS update object using ECom.
  * At present just creates a dummy dns update object
  * which does nothing
  *
  * @internalTechnology
  */
   { 
	/*Dummy DNS Update*/
   const TUid KUidDNSUpdateImplementation = {0x101FEBE0};

   // interface implementation ID (read from somewhere maybe?)
   return static_cast<CDnsUpdateIf*>(REComSession::CreateImplementationL( KUidDNSUpdateImplementation, _FOFF(CDnsUpdateIf, iDtor_ID_Key)));
   }

