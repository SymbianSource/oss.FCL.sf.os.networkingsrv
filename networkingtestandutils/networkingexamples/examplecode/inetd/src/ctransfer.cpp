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

#include <ctransfer.h>

//------------------------------------------------------------------------------//
// --- Class CTransfer ---------------------------------------------------------//
//------------------------------------------------------------------------------//

CTransfer::CTransfer(RSocket& aSocket, const TName& aProgram, const TName& aProgramOptions) :  CActive(EPriorityStandard), iSocket(aSocket)
//
// C'tor
//
	{
	iProgram = aProgram;
	iProgOptions = aProgramOptions;
	}

CTransfer::~CTransfer()
// 
// D'tor
//
	{
	Cancel();
	iSocket.Close();
	delete iTimer;
	}

CTransfer* CTransfer::NewL(RSocket& aSocket, const TName& aProgram, const TName& aProgramOptions)
//
// Build a new transfer object
//
	{
	CTransfer* self = new(ELeave) CTransfer(aSocket, aProgram, aProgramOptions);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CTransfer::ConstructL()
// 
// Symbian second construction step
//
	{
	iTimer = CTransferTimer::NewL(this);
	}

void CTransfer::TransferL()
//
// Transfer socket to new program
//
	{
	TName socketName;

	// 1: Create a new process
	CleanupClosePushL(iProcess);
	User::LeaveIfError(iProcess.Create(iProgram, iProgOptions));

	// 2: Get socket name
	User::LeaveIfError(iSocket.Name(socketName));

	// 3: Pass socket name as parameter for newly created process
	User::LeaveIfError(iProcess.SetParameter(KSlotParameter, socketName));

	// 4: Rendezvous newly created process
	iProcess.Rendezvous(iStatus);

	// 5: Resume newly created process
	iProcess.Resume();

	// 6: Wait for Rendezvous and set timer
	CActiveScheduler::Add(this);
	SetActive();
	iTimer->After(KWaitTime);
	}

void CTransfer::RunL()
//
// Verify transfer result
//
	{
	TInt ret;
	TProtocolDesc protocolInfo;

	// 1: Cancel timer
	iTimer->Cancel();
	
	// 2: Verify status is OK
	if(!iStatus.Int())
		{
		// 3: Verify transfer result
		ret = iSocket.Info(protocolInfo);
		switch(ret)
			{
			case KErrNone:
				// Transfer failed
				iSocket.Close();
				break;

			case KErrBadHandle:
				// Transfer succeed
				break;

			default:
				// Error
				break;
			}
		}
	}

void CTransfer::Stop()
//
// Stop socket transfer. (Used by CTransferTimer)
//
	{
	Cancel();
	}


void CTransfer::DoCancel()
//
// Cancel rendezvous
//
	{
	// We do not check return here. So we do not report any error.
	(void)iProcess.RendezvousCancel(iStatus);	
	CleanupStack::PopAndDestroy(&iProcess);
	}

//------------------------------------------------------------------------------//
// --- Class CTransferTimer ----------------------------------------------------//
//------------------------------------------------------------------------------//

CTransferTimer::CTransferTimer(CTransfer* aTransfer) : CTimer(EPriorityStandard)
//
// C'tor
// 				 
	{
	iTransfer = aTransfer;
	}

CTransferTimer* CTransferTimer::NewL(CTransfer* aTransfer)
// 
// Build a new Transfer Timer
//
	{
	CTransferTimer* self = new(ELeave)CTransferTimer(aTransfer);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

void CTransferTimer::ConstructL()
// 
// Symbian second construction step
//
	{
	CTimer::ConstructL();
	CActiveScheduler::Add(this);
	}

void CTransferTimer::RunL()
//
// Process too long to react so we stop
// 
	{
	iTransfer->Stop();
	}

void CTransferTimer::DoCancel()
//
// DoCancel() implementation
//
	{
	}
