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

#include "serial.h"
#include "TestMgr.h"
//
//	Serial Listener Active Object - receive commands on the serial port
//

CSerialListener::CSerialListener(RComm* aCommPort, CTestMgr* aController)
: CActive(CActive::EPriorityStandard), iCommPort(aCommPort), iController(aController)
{
}

CSerialListener* CSerialListener::NewL(RComm* aCommPort, CTestMgr* aController)
{
	CSerialListener* self = new (ELeave) CSerialListener(aCommPort, aController);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();	// self
	CActiveScheduler::Add(self);
	return self;
}

void CSerialListener::ConstructL()
{
}

CSerialListener::~CSerialListener()
{
	Cancel();
}

void CSerialListener::Recv()
{
	// read data from Comm port
	iCommPort->Read(iStatus, iDataBuffer);
	SetActive();
}

void CSerialListener::RunL()
{
	iLastBuffer = iDataBuffer;
	// notify controller we have read something
	iController->Notify(CTestMgr::EReadComplete);
	// start the listener again
	Recv();
}

void CSerialListener::DoCancel()
{
	iCommPort->ReadCancel();
}
//
//	Serial Sender Active Object - send commands on the serial port
//

CSerialSender* CSerialSender::NewL(RComm* aCommPort, CTestMgr* aController)
{
	CSerialSender* self = new (ELeave) CSerialSender(aCommPort, aController);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop();	// self
	CActiveScheduler::Add(self);
	return self;
}

void CSerialSender::ConstructL()
{
}

CSerialSender::~CSerialSender()
{
	Cancel();
}

void CSerialSender::Send(TBuffer&  aBuff)
{
	if (!IsActive())
	{
		// take a local copy of data to send
		iDataBuffer = aBuff;
		// send the data
		iCommPort->Write(iStatus, iDataBuffer);
		SetActive();
	}
}

CSerialSender::CSerialSender(RComm* aCommPort, CTestMgr* aController)
: CActive(CActive::EPriorityStandard), iCommPort(aCommPort), iController(aController)
{
}

void CSerialSender::RunL()
{
	// notify controller that send has finished
	iController->Notify(CTestMgr::EWriteComplete);
}

void CSerialSender::DoCancel()
{
	iCommPort->WriteCancel();
}

