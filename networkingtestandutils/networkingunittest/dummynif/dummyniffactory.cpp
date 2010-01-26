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
 
#include <es_mbuf.h>
#include "dummynif.h"
#include "dummynifvar.h"

/*
 * Factory functions...
 * Build this file with its factory when you want a normal dummy nif which will
 * echo back udp packets and respond to commands
 */
 
EXPORT_C CNifFactory* NewInterfaceFactoryL()
	{
	return new(ELeave) CDummyIfFactory;
	}

void CDummyIfFactory::InstallL()
	{
	iDelayPipe.SetLengthL(KDelayQueueSize);
	for(TInt prime = 0; prime < KDelaySlots; ++prime)
		{
		RMBuf* stopper = NULL;
		User::LeaveIfError(iDelayPipe.Add(&stopper));
		}
	if(KDelaySlots > 0)
		{
		iNetDelayTimer = CPeriodic::NewL(50000);	// arbitrary stupid high priority, but we really need to run on our ticks & not get delayed until flow-offs & retransmits
		iNetDelayTimer->Start(KDelayQuantum, KDelayQuantum, TCallBack(CDummyIfFactory::DripCallback, this));
		TCallBack callBack(DripCallback, this);
		}
	}
	
void CDummyIfFactory::SetDripReceiver(TCallBack aReceiver)
	{
	iDripReceiver = aReceiver;
	}

TInt CDummyIfFactory::DripCallback(TAny* aSelf)
	{
	CDummyIfFactory* self = (CDummyIfFactory*) aSelf;
	if(self->iDripReceiver.iPtr)
		{
		RMBuf* stopper = NULL;
		self->iDelayPipe.Add(&stopper);	// keep the supply of backstops constant
		self->iDripReceiver.CallBack();
		}
	return 0;
	}

RMBuf* CDummyIfFactory::GetDrip()
	{
	RMBuf* next = NULL;
	iDelayPipe.Remove(&next);
	return next;
	}
	
void CDummyIfFactory::AddDrip(RMBuf* aDrip)
	{
	TInt err = iDelayPipe.Add(&aDrip);
	__ASSERT_ALWAYS(err == 1, User::Panic(_L("DummyIfOver"), 0));
	}
	
	
	
CDummyIfFactory::~CDummyIfFactory()
	{
	delete iNetDelayTimer;
	}
	

CNifIfBase* CDummyIfFactory::NewInterfaceL(const TDesC& /*aName*/)
	{	
	CDummyIfLink* s = new(ELeave) CDummyIfLink(*this);
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
