// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "vjlog.h"
#include <vj.h>


extern "C"
    {
    IMPORT_C CVJCompFactory * NewVJCompFactoryL();	// Force export 
    }

EXPORT_C CVJCompFactory* NewVJCompFactoryL()
	{

	return new (ELeave) CVJFactory;
	}

//
// CVJFactory definitions
//

CVJFactory::CVJFactory()
	{}

void CVJFactory::InstallL()
	{}

CVJCompressorIf* CVJFactory::NewVJCompressorL( TInt aSlots, TInt aCompressConnId)
	{
	CVJCompressor* Comp = new (ELeave) CVJCompressor(aSlots, aCompressConnId);

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Comp));
	Comp->ConstructL(this, aSlots, aCompressConnId);
	CleanupStack::Pop();

	return Comp;
	}

CVJDeCompressorIf* CVJFactory::NewVJDeCompressorL( TInt aSlots )
	{
	CVJDeCompressor* DeComp = new (ELeave) CVJDeCompressor(aSlots);

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, DeComp));
	DeComp->ConstructL(this, aSlots);
	CleanupStack::Pop();

	return DeComp;
	}

//
// TVJCompHdr definitions
//

void TVJCompHdr::SetNextPtr(TVJCompHdr* aNextPtr)
	{
	iNextPtr = aNextPtr;
	}

TVJCompHdr * TVJCompHdr::NextPtr()
	{
	return iNextPtr;
	}

void TVJCompHdr::SetConnectionNumber(TUint aConnection)
	{
	iConnectionId = aConnection;
	}

TUint TVJCompHdr::ConnectionNumber()
	{
	return iConnectionId;
	}

TVJCompHdr::TVJCompHdr()
	:
	iConnectionId(0)
	{
	}

void TVJCompHdr::StoreTCPIPHeader(ThdrIP * aHeader)
	{
	iIPHeader = *aHeader;
	iTCPHeader = *(ThdrTCP::NetPtr(aHeader));
	iTCPHeader.NetSetChecksum(0);
	return;
	}

void TVJCompHdr::RetrieveTCPIPHeader(ThdrIP* aIPHeader, ThdrTCP* aTCPHeader)
	{
	*aIPHeader = iIPHeader;
	*aTCPHeader = iTCPHeader;
	return;
	}
