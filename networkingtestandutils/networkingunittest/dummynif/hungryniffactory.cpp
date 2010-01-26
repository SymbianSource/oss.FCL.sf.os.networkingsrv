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
 
#include "hungrynif.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_prot_internal.h>
#endif

/*
 * Factory functions...
 * Build this file with its factory when you want a dummy nif which will just 
 * eat up all the packets it gets - they won't be echoed back at all
 */

EXPORT_C CNifFactory* NewInterfaceFactoryL()
	{
	return new(ELeave) CDummyIfFactory;
	}

void CDummyIfFactory::InstallL()
	{
	}

CNifIfBase* CDummyIfFactory::NewInterfaceL(const TDesC& /*aName*/)
	{	
	CHungryIfLink* s = new(ELeave) CHungryIfLink(*this);
	CleanupStack::PushL(s);
	s->TimerConstructL(ESocketTimerPriority);
	CleanupStack::Pop();
	return s;
	}

TInt CDummyIfFactory::Info(TNifIfInfo& aInfo, TInt /*aIndex*/) const
	{
	CDummyIfLink::FillInInfo(aInfo, (TAny*)this);

	return 1;
	}

// drip functionality (delayed upstream reflection of frames to simulate real-world propogation delay)
// - not needed for hungry nif (downstream only) & thus stubbed to allow linkage
void CDummyIfFactory::SetDripReceiver(TCallBack /*aReceiver*/)
	{
	}
RMBuf* CDummyIfFactory::GetDrip()
    {
    return NULL;
    }
CDummyIfFactory::~CDummyIfFactory()
	{
	delete iNetDelayTimer;
	}
