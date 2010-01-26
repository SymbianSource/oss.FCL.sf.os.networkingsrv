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
 
#include "dummypppnif.h"
#include "DummyPPPNifVar.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <es_prot_internal.h>
#endif

/*
 * Factory functions...
 * Build this file with its factory when you want a normal dummy nif which will
 * echo back udp packets and respond to commands
 */

EXPORT_C CNifFactory* NewInterfaceFactoryL()
	{
	return new(ELeave) CDummyPPPIfFactory;
	}
void CDummyPPPIfFactory::InstallL()
	{
	}

CNifIfBase* CDummyPPPIfFactory::NewInterfaceL(const TDesC& /*aName*/)
	{	
	CDummyPPPLink* s = new(ELeave) CDummyPPPLink(*this);
	
	//CDummyPPPIf* s = new(ELeave) CDummyPPPIf(sLink);
	
	CleanupStack::PushL(s);
	s->TimerConstructL(ESocketTimerPriority);
	CleanupStack::Pop();
	return s;

	}

TInt CDummyPPPIfFactory::Info(TNifIfInfo& aInfo, TInt /*aIndex*/) const
	{
	CDummyPPPLink::FillInInfo(aInfo,(TAny*)this);

	return 1;
	}
