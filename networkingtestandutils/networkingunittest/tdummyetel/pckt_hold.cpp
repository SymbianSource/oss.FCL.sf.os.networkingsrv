// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Packet_HOLD.CPP
// v0.01 JoeF - 10-Aug-01
// 
//

#include "ETELEXT.H"

// ETel Packet header files
#include "pcktptr.h"

CEtelPacketPtrHolder* CEtelPacketPtrHolder::NewL(const TInt aSizeOfPtrArray)
	{
	CEtelPacketPtrHolder* p = new (ELeave) CEtelPacketPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray);
	CleanupStack::Pop();
	return p;
	}

CEtelPacketPtrHolder::CEtelPacketPtrHolder()
	{}

CEtelPacketPtrHolder::~CEtelPacketPtrHolder()
	{
	iPtrArray.Close();
	}

void CEtelPacketPtrHolder::ConstructL(const TInt aSizeOfPtrArray)
	{
	TPtr8 ptr(NULL,0);
	TInt i;
	for (i=0;i<aSizeOfPtrArray;i++)
		User::LeaveIfError(iPtrArray.Append(ptr));
	}

TPtr8& CEtelPacketPtrHolder::Ptr(const TInt aIndex)
	{
	__ASSERT_ALWAYS(aIndex<iPtrArray.Count(),PanicClient(EEtelPanicIndexOutOfRange));
	return iPtrArray[aIndex];
	}

CPacketPtrHolder* CPacketPtrHolder::NewL(const TInt aSizeOfPtrArray)
	{
	CPacketPtrHolder* p = new (ELeave) CPacketPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray);
	CleanupStack::Pop();
	return p;
	}

CPacketPtrHolder::CPacketPtrHolder()
	:CEtelPacketPtrHolder()
	{}

CPacketPtrHolder::~CPacketPtrHolder()
	{
	}

CPacketContextPtrHolder* CPacketContextPtrHolder::NewL(const TInt aSizeOfPtrArray)
	{
	CPacketContextPtrHolder* p = new (ELeave) CPacketContextPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray);
	CleanupStack::Pop();
	return p;
	}

CPacketContextPtrHolder::CPacketContextPtrHolder()
	:CEtelPacketPtrHolder()
	{}

CPacketContextPtrHolder::~CPacketContextPtrHolder()
	{}


CPacketQoSPtrHolder* CPacketQoSPtrHolder::NewL(const TInt aSizeOfPtrArray)
	{
	CPacketQoSPtrHolder* p = new (ELeave) CPacketQoSPtrHolder();
	CleanupStack::PushL(p);
	p->ConstructL(aSizeOfPtrArray);
	CleanupStack::Pop();
	return p;
	}

CPacketQoSPtrHolder::CPacketQoSPtrHolder()
	:CEtelPacketPtrHolder()
	{}

CPacketQoSPtrHolder::~CPacketQoSPtrHolder()
	{}


