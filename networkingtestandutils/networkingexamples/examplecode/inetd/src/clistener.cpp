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
//

#include <clistener.h>

//------------------------------------------------------------------------------//
// --- Class CListenerOptions --------------------------------------------------//
//------------------------------------------------------------------------------//

CListenerOptions* CListenerOptions::NewL(TPtrC8 aConfiguration)
// 
// Construct a new ListenerOptions object
//
	{
	CListenerOptions* self = new(ELeave) CListenerOptions;
	CleanupStack::PushL(self);
	self->ConstructL(aConfiguration);
	CleanupStack::Pop(self);
	return self;
	}

void CListenerOptions::ConstructL(TPtrC8 aConfiguration)
// 
// Symbian second construction step
//
	{
	TLex8 lex;
	TUint port;
	TName protocolName;

	lex.Assign(aConfiguration);


	// Set Service
	lex.Mark();
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		iService.Copy(lex.MarkedToken());
		}
	else
		{
		// Bad configuration file. We exit in test case.
		User::Leave(KErrAbort);
		}
	lex.SkipSpaceAndMark();

	// Set socket type
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		TLex8 tempLex;
		tempLex.Assign(lex.MarkedToken());
		tempLex.Val(iSockType);
		}
	else
		{
		// Bad configuration file. We exit in test case.
		User::Leave(KErrAbort);
		}
	lex.SkipSpaceAndMark();

	// Set protocol name
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		protocolName.Copy(lex.MarkedToken());
		}
	else
		{
		// Bad configuration file. We exit in test case.
		User::Leave(KErrAbort);
		}
	lex.SkipSpaceAndMark();
	
	// Determine protocol number
	if(protocolName == _L("TCP"))
		{
		iProtocol = KProtocolInetTcp;
		}
	else
		{
		if(protocolName == _L("UDP"))
			{
			iProtocol = KProtocolInetUdp;
			}
		}

	// Set port
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		TLex8 tempLex;
		tempLex.Assign(lex.MarkedToken());
		tempLex.Val(port);
		}
	else
		{
		// Bad configuration file. We exit in test case.
		User::Leave(KErrAbort);
		}
	lex.SkipSpaceAndMark();
	iBindAddr = TInetAddr(KInetAddrAny, port);

	// Set program name
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		iProgramName.Copy(lex.MarkedToken());
		}
	else
		{
		// Bad configuration file. We exit in test case.
		User::Leave(KErrAbort);
		}
	lex.SkipSpaceAndMark();

	// Set program option
	lex.SkipCharacters();
	if(lex.TokenLength() != 0)
		{
		iProgramName.Copy(lex.MarkedToken());
		}
	}

TInetAddr CListenerOptions::Addr()
// 
// Return address where to bind
// 
	{
	return iBindAddr;
	}

void CListenerOptions::SetAddress(const TInetAddr& aAddr)
//
// Method to set address we want to bind to
// 
	{
	iBindAddr = aAddr;
	}

TUint CListenerOptions::Family()
// 
// Return Address family
// 
	{
	return iBindAddr.Family();
	}

void CListenerOptions::SetFamily(TUint aFamily)
// 
// Set address family
// 
	{
	iBindAddr.SetFamily(aFamily);
	}

TInt CListenerOptions::Port()
// 
// Get Port where to bind
// 
	{
	return iBindAddr.Port();
	}

void CListenerOptions::SetPort(TInt aPort)
//
// Set port where to bind
// 
	{
	iBindAddr.SetPort(aPort);
	}

TName CListenerOptions::ProgramName()
//
// Return program name to launch
// 
	{
	return iProgramName;
	}

void CListenerOptions::SetProgramName(const TName& aName)
//
// Set program to launch name
// 
	{
	iProgramName = aName;
	}

TName CListenerOptions::ProgramOptions()
// 
// Return command line arguments options
// 
	{
	return iProgramOptions;
	}

void CListenerOptions::SetProgramOptions(const TName& aOptions)
// 
// Set command line arguments options
// 
	{
	iProgramOptions = aOptions;
	}

TUint CListenerOptions::Protocol()
// 
// Return socket protocol
// 
	{
	return iProtocol;
	}

void CListenerOptions::SetProtocol(TUint aProtocol)
//
// Set socket protocol
//
	{
	iProtocol = aProtocol;
	}

TUint CListenerOptions::SockType()
//
// Return socket type
//
	{
	return iSockType;
	}

void CListenerOptions::SetSockType(TUint aSockType)
// 
// Set socket type
// 
	{
	iSockType = aSockType;
	}


//------------------------------------------------------------------------------//
// --- Class CListener ---------------------------------------------------------//
//------------------------------------------------------------------------------//

CListener::CListener(RSocketServ& aSockServ, CListenerOptions* aOptions) : CActive(EPriorityStandard)
//
// C'tor
//
	{
	iSockServ = aSockServ;
	iOptions = aOptions;
	}

CListener::~CListener()
//
// D'tor
//
	{
	Cancel();
	iListenSocket.Close();
	delete iOptions;
	iTransfers.Close();
	}

CListener* CListener::NewL(RSocketServ& aSockServ, RConnection& aConnection, CListenerOptions* aOptions)
//
// Method to construct new listener
// 
	{
	// 0: Create a new object, leave if error (ie: KErrNoMemory) ...
	CListener* self = new(ELeave) CListener(aSockServ, aOptions);

	// 1: Push object on the cleanup stack during initialisation ...
	CleanupStack::PushL(self);

	// 2: Initialize object ...
	self->ConstructL(aConnection);

	// 3: Pop object and return it ...
	CleanupStack::Pop(self);
	return self;
	}

void CListener::ConstructL(RConnection& aConnection)
// 
// Symbian second construction step
//
	{
	TInetAddr bindAddr;
	
	// 1: Open listening socket
	User::LeaveIfError(iListenSocket.Open(iSockServ, iOptions->Family(), iOptions->SockType(), iOptions->Protocol(), aConnection));

	// 2: Bind listening socket
	bindAddr = iOptions->Addr();	
	User::LeaveIfError(iListenSocket.Bind(bindAddr));
	
	// 3: Listen
	User::LeaveIfError(iListenSocket.Listen(KQueueListenSize));

	// 4: Add object to active scheduler
	CActiveScheduler::Add(this);
	}

CListenerOptions* CListener::Options()
//
// Method that return a pointer to this listener's options
//
	{
	return iOptions;
	}

void CListener::AcceptL()
//
// Method that grab the next connect received
//
	{
	// Create a new blank socket for accept
	User::LeaveIfError(iBlankSocket.Open(iSockServ));	

	// Accept
	iListenSocket.Accept(iBlankSocket, iStatus);
	SetActive();
	}

void CListener::RunL()
//
// RunL method that will transfer the connected socket to 
// appropriate program
//
	{
	// Add a new transfer object that will call RSocket::Transfer(...)
	AddTransferL();

	// Grab another connection
	AcceptL();
	}


void CListener::AddTransferL()
//
// Launch a new transfer
//
	{
	TInt pos = 0;
	TBool search = ETrue;

	// We loop in order to see if a previously created transfer
	// is over. If so, we delete it and grab slot back.
	while(search)
		{
		for(pos=0; search && pos < iTransfers.Count(); pos++)
			{
			if(!iTransfers[pos]->IsActive())
				{
				// We found a terminated transfer
				delete iTransfers[pos];	// Delete it
				search = EFalse;	// Stop search
				}
			}

		// Look if we have parse all vector
		if(pos == KNbTransfers)
			{
			// We wait before looping again in order to find a free transfer
			User::After(KWaitTime); // Maximum time before a CTransfer will be stop by its timer
			}
		else
			{
			// We exit loop
			search = EFalse;	// We set seacrh to EFalse for the case there is no CTransfers already running
			}
		}

	// Create a new transfer object
	// Check if it is in new slot
	if(pos == iTransfers.Count())
		{
		iTransfers.Append(CTransfer::NewL(iBlankSocket, iOptions->ProgramName(), iOptions->ProgramOptions()));
		}
	else
		{
		iTransfers[pos] = CTransfer::NewL(iBlankSocket, iOptions->ProgramName(), iOptions->ProgramOptions());
		}

	// Then we launch it to do its job 
	iTransfers[pos]->TransferL();
	}

void CListener::PurgeAllTransfers()
// 
// We want to delete all our transfers
// 
	{
	iTransfers.ResetAndDestroy();
	}

void CListener::DoCancel()
// 
// Cancel all (Look at how to cancel accepted connection
//
	{
	// Delete all transfers
	PurgeAllTransfers();

	// Cancel accept
	iListenSocket.CancelAccept();

	// Terminates
	iBlankSocket.Close();
	}
