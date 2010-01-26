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

#include <networking/vj.h>


// Force export of non-mangled name
extern "C"
    {

EXPORT_C CVJCompFactory* NewVJCompFactoryL(void)
	{
	return new (ELeave) CVJFactory;
	}

	}
	
//
// CVJFactory definitions
//

CVJFactory::CVJFactory()
	{}

void CVJFactory::InstallL()
	{}

CVJCompressorIf* CVJFactory::NewVJCompressorL( TInt aSlots, TInt aCompressConnId)
/**
Create and construct a new CVJCompressor object.

@param aSlots Maximum number of VJ connections
@param aCompressConnId Whether to compress the connection number 

@return New CVJCompressor object
*/
	{
	CVJCompressor* Comp = new (ELeave) CVJCompressor();

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Comp));
	Comp->ConstructL(this, aSlots, aCompressConnId);
	CleanupStack::Pop();

	return Comp;
	}

CVJDeCompressorIf* CVJFactory::NewVJDeCompressorL( TInt aSlots )
/**
Create and construct a new CVJDeCompressor object.

@param aSlots Value of the highest VJ connection number to be received

@return New CVJDeCompressor object
*/
	{
	CVJDeCompressor* DeComp = new (ELeave) CVJDeCompressor();

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, DeComp));
	DeComp->ConstructL(this, aSlots);
	CleanupStack::Pop();

	return DeComp;
	}

//
// TVJCompHdr definitions
//

TVJCompHdr::TVJCompHdr()
	:
	iNextPtr(NULL), iConnectionId(0)
	{
	    // Ignore lint "not initialized" warning about iIPHeader and iTCPHeader
	}

void TVJCompHdr::StoreTCPIPHeader(ThdrIP* aHeader)
/**
Stores the TCP/IP header.

@param aHeader TCP/IP packet header
*/
	{
	iIPHeader = *aHeader;
	iTCPHeader = *(ThdrTCP::NetPtr(aHeader));
	iTCPHeader.NetSetChecksum(0);
	MarkValid();
	}

void TVJCompHdr::RetrieveTCPIPHeader(ThdrIP* aIPHeader, ThdrTCP* aTCPHeader)
/**
Retrieves the TCP/IP header.

@param aIPHeader Returns the IP packet header
@param aTCPHeader Returns the TCP packet header
*/
	{
	*aIPHeader = iIPHeader;
	*aTCPHeader = iTCPHeader;
	}
