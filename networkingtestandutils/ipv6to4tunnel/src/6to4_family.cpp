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
// Name        : 6to4_family.cpp
// Part of     : 6to4 plugin / 6to4.prt
// Implements 6to4 automatic and configured tunnels, see
// RFC 3056 & RFC 2893
// Version     : 0.2
//




// INCLUDE FILES
#include "6to4.h"
#include "6to4_family.h"

// EXTERNAL DATA STRUCTURES
// EXTERNAL FUNCTION PROTOTYPES  
extern "C"
{
IMPORT_C CProtocolFamilyBase *Install (void);
}

// CONSTANTS
// MACROS
// LOCAL CONSTANTS AND MACROS
// MODULE DATA STRUCTURES
class CProtocolFamily6to4 : public CProtocolFamilyBase
	{
	public:
	CProtocolFamily6to4 ();
	~CProtocolFamily6to4 ();
	TInt Install ();
	TInt Remove ();
	TUint ProtocolList (TServerProtocolDesc * &aProtocolList);
	CProtocolBase *NewProtocolL (TUint /* aSockType */ , TUint aProtocol);
	};

// LOCAL FUNCTION PROTOTYPES
// FORWARD DECLARATIONS

// ============================= LOCAL FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// E32Dll () 
// Called by the E32.
// Returns: Error value
// ----------------------------------------------------------------------------
//

// ----------------------------------------------------------------------------
// Install () 
// Called by the E32.
// Returns: A new object CProtocolFamily6to4.
// ----------------------------------------------------------------------------
//
EXPORT_C CProtocolFamilyBase *Install ()
	{
	return new (ELeave) CProtocolFamily6to4;
	}

// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::CProtocolFamily6to4
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
CProtocolFamily6to4::CProtocolFamily6to4 ()
	{
	__DECLARE_NAME (_S ("CProtocolFamily6to4"));
	}

// Destructor
CProtocolFamily6to4::~CProtocolFamily6to4 ()
	{
	}

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::Install
// 
// ----------------------------------------------------------------------------
//
TInt CProtocolFamily6to4::Install ()
	{
	return KErrNone;
	}

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::Remove
// 
// ----------------------------------------------------------------------------
//
TInt CProtocolFamily6to4::Remove ()
	{
	return KErrNone;
	}

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::ProtocolList
// Gets the identification information from the CProtocol6to4
// ----------------------------------------------------------------------------
//
TUint CProtocolFamily6to4::ProtocolList (TServerProtocolDesc * &aProtocolList)
	{
	// This function should be a leaving fn
	// apparently it is OK for it to leave
	TServerProtocolDesc *p = new (ELeave) TServerProtocolDesc[1];

	CProtocol6to4::Identify (p[0]);
	aProtocolList = p;
	return 1;
	}

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::NewProtocolL
// Creates the CProtocol6to4 instance 
// ----------------------------------------------------------------------------
//
CProtocolBase *CProtocolFamily6to4::NewProtocolL (TUint /* aSockType */ ,
												  TUint aProtocol)
	{
	if (aProtocol != KProtocolInet6Ipip)
		User::Leave (KErrNotSupported);

	return new (ELeave) CProtocol6to4;
	}

// ----------------------------------------------------------------------------
// CProtocolFamily6to4::Panic
// 
// ----------------------------------------------------------------------------
//
void Panic (T6to4Panic aPanic)
	{
	User::Panic (_L ("6TO4"), aPanic);
	}

// ========================== OTHER EXPORTED FUNCTIONS ========================

//  End of File  
