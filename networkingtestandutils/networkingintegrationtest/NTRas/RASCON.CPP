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

#include "rasdef.h"
#include <cdblen.h>

CRasConnect::CRasConnect(CNtRas* aNtRas)
	: iNtRas(aNtRas)
	{
	__DECLARE_NAME(_S("CRasConnect"));
	}

CRasConnect::~CRasConnect()
	{

	if(iCommOpen)
		{
		Disconnect();
		CommClose();
		}
	CommDelete();
	TimerDelete();
	delete iChat;
	}

CRasConnect* CRasConnect::NewL(CNtRas* aRas)
	{
	CRasConnect* p = new (ELeave) CRasConnect(aRas);
	CleanupStack::PushL(p);
	p->ConstructL();
	CleanupStack::Pop();
	return p;
	}

void CRasConnect::ConstructL()
	{

	CommConstructL(KCommReadPriority,KCommWritePriority);
	TimerConstructL(KCommTimerPrioiry);
	iState = ERasConClosed;
	iChat = new (ELeave) CCommChatter(this, KChatterPriority);
	iChat->CreateL(KChatBufferSize);
	iNtRas->ReadInt(NIF_IFSERVERMODE, iServerMode);

	if(iServerMode)
		iString=CCommChatString::NewL(_L8("CLIENT"), EFalse);
	else
		iString=CCommChatString::NewL(_L8("SERVER"), EFalse);

	iChat->AddString(iString);
	TFileName csy;
	TName port;

	TBuf<KCommsDbSvrMaxColumnNameLength> columnName=TPtrC(MODEM);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(MODEM_CSY_NAME));
	User::LeaveIfError(iNtRas->ReadDes(columnName, csy));

	columnName.SetLength(0);
	columnName=TPtrC(MODEM);
	columnName.Append(TChar(KSlashChar));
	columnName.Append(TPtrC(MODEM_PORT_NAME));
	User::LeaveIfError(iNtRas->ReadDes(columnName, port));

	TInt err;
	err = CommOpen(csy, port, ECommShared);
	if (err==KErrNotFound)
		{
		Util::CommInit();
		err = CommOpen(csy, port, ECommShared);
		}
	User::LeaveIfError(err);
	iCommOpen=ETrue;
	StartL();
	}

void CRasConnect::StartL()
	{
	User::LeaveIfError(InitComm());
	iCommPort.ResetBuffers();
	Disconnect(); // Will drop the lines
	TimerAfter(KCommLineDropTime);
	iState = ERasConDropLines;
	}

TPtrC8 CRasConnect::ExcessData()
	{
	return TPtrC8(iRxBuf.Ptr()+iRxBufOffset, iRxBuf.Length()-iRxBufOffset);
	}

void CRasConnect::CommReadComplete(TInt aStatus)
	{
	if (aStatus!=KErrNone)
		{
		Complete(aStatus);
		return;
		}
	for (iRxBufOffset=0; iRxBufOffset<iRxBuf.Length() && iState!=ERasConConnected; iRxBufOffset++)
		iChat->AddChar(iRxBuf[iRxBufOffset]);
	if (iState==ERasConConnected)
		{
		Complete(KErrNone);
		}
	else
		{
		CommReadOneOrMore(iRxBuf);
		iRxBufOffset = 0;
		}
	}

void CRasConnect::CommWriteComplete(TInt aStatus)
	{
	if (aStatus!=KErrNone)
		{
		Complete(aStatus);
		return;
		}
	else if (iState==ERasConConnecting)
		{
		iTries = 0;
		SendToken();
		}
	}

void CRasConnect::TimerComplete(TInt /*aStatus */)
	{
	if (iState == ERasConDropLines)
		{
		TInt ret;
		if (ret = InitComm(), ret!=KErrNone)
			{
			Complete(ret);
			return;
			}

		iCommPort.SetSignalsToMark(KSignalRTS | KSignalDTR);

		if(iServerMode)
			{
			CommReadOneOrMore(iRxBuf);
			}
		else
			{
		    CommWriteReady();
		    TimerAfter(KCommTimeout);
			}
		iState = ERasConConnecting;
		}
	else
		{
		Complete(KErrTimedOut);
		}
	}

void CRasConnect::ChatStringMatch(CCommChatString* /*aString */)
	{
	if(iState==ERasConTokenSent)
		{
	    iState = ERasConConnected;
		if(iServerMode)
			iRxBufOffset=0;
		}
	else if(iServerMode && iState!=ERasConTokenSent && !CommIsWriting())
	    {
		iChat->RemoveString(iString);
		delete iString;
        TRAPD(res, iString=CCommChatString::NewL(_L8("~"), EFalse);)
		if(res!=KErrNone)  // Comment out here
			{
			Complete(res);
			return;
			}
		CommWriteReady();
		}
	}

void CRasConnect::ChatTimeout()
	{
	SendToken();
	}

void CRasConnect::SendToken()
	{
	
	// Changed to work on PPPD machines, comment line below and uncomment block for
	// NTRas to work
	//iNtRas->ConnectComplete(KErrNone);
	
	iState = ERasConTokenSent;
	if (iTries++>=KChatMaxTries)
		{
		Complete(KErrCouldNotConnect);
		return;
		}
	if(iServerMode)
		CommWrite(_L8("SERVER"));
	else
	    CommWrite(_L8("CLIENT"));

	if(!CommIsReading())
	    CommReadOneOrMore(iRxBuf);
	iChat->StartTimer(KChatTimeout);
	//*/
	}

void CRasConnect::Complete(TInt aStatus)
	{
	CommCancel();
	TimerCancel();
	iChat->StopTimer();
	iNtRas->ConnectComplete(aStatus);
	}

void CRasConnect::Disconnect()
	{

	CommCancel();

	//TInt ret;
	TCommConfig cbuf;
	TCommConfigV01 &cfg=cbuf();
	iCommPort.Config(cbuf);
	cfg.iHandshake = KConfigFreeRTS	| KConfigFreeDTR;
	iCommPort.SetConfig(cbuf);
	iCommPort.SetSignalsToSpace(KSignalRTS | KSignalDTR);
	}

TInt CRasConnect::InitComm()
	{
	TInt ret;
	TCommConfig cbuf;
	TCommConfigV01 &cfg=cbuf();
	iCommPort.Config(cbuf);

	TUint32 spd;
	iNtRas->ReadInt(RAS_COMMSPEED, spd);
	ret = Util::GetCommSpeed(spd, cfg.iRate);	
	
	TBuf<32> buf;
	buf.SetMax();
	iNtRas->ReadDes(RAS_COMMDATA, buf);
	if (ret = Util::GetCommFormat(buf, cfg.iDataBits, cfg.iStopBits, cfg.iParity), ret!=KErrNone)
		return ret;
	
	buf.SetMax();
	iNtRas->ReadDes(RAS_COMMHANDSHAKE, buf);
	if (ret = Util::GetCommHandshake(buf, cfg.iHandshake), ret!=KErrNone)
		return ret;

	if (ret = iCommPort.SetConfig(cbuf), ret!=KErrNone)
		return ret;

	return KErrNone;
	}
