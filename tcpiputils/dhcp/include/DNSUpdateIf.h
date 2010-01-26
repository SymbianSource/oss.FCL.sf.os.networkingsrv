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
// The DNS Update abstract plug-in interface
// 
//

/**
 @file DNSUpdateIf.h
*/

#ifndef __DNSUPDATEIF_H__
#define __DNSUPDATEIF_H__

#include <e32base.h>
#include <ecom/ecom.h>

class CDnsUpdateIf : public CBase
	{

public:
	// Wraps ECom object instantitation
	static CDnsUpdateIf* NewL();
	// Wraps ECom object destruction 
	virtual ~CDnsUpdateIf();

	virtual void Update(TDesC& aInterfaceName, TDesC8* aHostName, TDesC8* aDomainName) = 0;

private:
	// Instance identifier key
	TUid iDtor_ID_Key;
	};

//keep the destructor inline even though it's virtual so that 
//the plug-in doesn't have to link against us
inline CDnsUpdateIf::~CDnsUpdateIf()
    {
    REComSession::DestroyedImplementation(iDtor_ID_Key);
    }

#endif
