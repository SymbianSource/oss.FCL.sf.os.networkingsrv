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
// Protocol (PAP) - RFC 1334.
// 
//

/**
 @file
 @brief Source file for the implementation of Password Authentication
 @internalComponent 
*/

#include "ppppap.h"
#include "PppProg.h"
#include "PPPAUTH.H"
#include "PPPConfig.h"


//
CPppPap::CPppPap()
	{
	}

CPppPap::~CPppPap()
	{
	if (iPppLcp != 0)
		TimerDelete();
	}

void CPppPap::InitL(CPppLcp* aLcp)
	{
	CPppAuthentication::InitL(aLcp);
	TimerConstructL(KPppFsmTimerPriority);
	Register();
	}

void CPppPap::AuthenticateComplete(TInt aStatus)
	{
	ASSERT(iPppLcp != 0);

	if (aStatus==KErrNone)
		{
		//ignore Error, if fails it will time out 
		//and try again anyway.
		TRAP_IGNORE(SendAuthRequestL(ETrue));
		}
	else
		DoFail(aStatus);
	}

void CPppPap::LowerLayerUp()
	{
	ASSERT(iPppLcp != 0);

	if (iFlags & KPppApIsClient)
		{
		if (IsAuthenticateRequestDone())
			{
			//ignore Error, if fails it will time out 
			//and try again anyway.
			TRAP_IGNORE(SendAuthRequestL(ETrue));
			}
		else
			AuthenticateRequest();
		}
	}

void CPppPap::LowerLayerDown(TInt /*aStatus*/)
	{
	ASSERT(iPppLcp != 0);

	TimerCancel();
	}

void CPppPap::TimerComplete(TInt /*aStatus*/)
	{
	ASSERT(iPppLcp != 0);

	if (iTryCount>0)
		{
		//ignore Error, if fails it will time out 
		//and try again anyway.
		TRAP_IGNORE(SendAuthRequestL());
		}
	}

void CPppPap::SendAuthRequestL(TBool aNewRequest)
	{
    const CCredentialsConfig* credentials = iPppLcp->GetCredentials();
#ifdef _UNICODE
	const TDesC& username = credentials->GetUserName();
	HBufC8& user = *HBufC8::NewLC(username.Length());
	user.Des().Copy(username);

	const TDesC& password = credentials->GetPassword();
	HBufC8& pass = *HBufC8::NewLC(password.Length());
 	pass.Des().Copy(password);
#else
	TPtrC8 user(credentials->GetUserName()), pass(credentials->GetPassword());
#endif

	RMBufPacket pkt;
	RMBufPktInfo* info = NULL;
	TInt len = 4+user.Length()+1+pass.Length()+1;
	TRAPD(err, pkt.AllocL(len));
	if (err!=KErrNone)
		return;
	TRAP(err, info = pkt.NewInfoL());
	if (err!=KErrNone)
		{
		pkt.Free();
		return;
		}
	TUint8* ptr = pkt.First()->Ptr();
	*ptr++ = KPppPapRequest;
	if (aNewRequest)
		{
		iTryCount = KPppPapRetries;
		if (++iCurrentId==0)
			++iCurrentId;
		}
	*ptr++ = iCurrentId;
	BigEndian::Put16(ptr, (TUint16)len);
	ptr += 2;
	*ptr++ = (TUint8)user.Length();	
	Mem::Copy(ptr, (TUint8*)user.Ptr(), user.Length());
	ptr += user.Length();
	*ptr++ = (TUint8)pass.Length();	
	Mem::Copy(ptr, (TUint8*)pass.Ptr(), pass.Length());
	ptr += pass.Length();
	info->iLength = len;	
	TPppAddr::Cast((info->iDstAddr)).SetProtocol(iPppId);
	pkt.Pack();
	SendFrame(pkt);
	TimerCancel();
	TimerAfter(KPppPapWaitTime*1000);
	--iTryCount;

#ifdef _UNICODE
	CleanupStack::PopAndDestroy(&pass);
	CleanupStack::PopAndDestroy(&user);
#endif
	}


TBool CPppPap::RecvFrame(RMBufChain& aPacket)
	{
	ASSERT(iPppLcp != 0);

	RMBufPacket pkt;
	pkt.Assign(aPacket);
	pkt.Unpack();
	RMBufPktInfo* info = pkt.Info();

	TUint8 op;
	TUint8 id;
	TInt len;

	if (IsInactive())
		{
		pkt.Free();
		return EFalse;
		}
	
	// Extract and drop LCP header
	pkt.Align(4);
	TUint8* ptr = pkt.First()->Ptr();
	op = *ptr++;
	id = *ptr++;
	len = BigEndian::Get16(ptr);
	
	// Check packet length is OK
	if (info->iLength<len)
		{
		// Too short!
		pkt.Free();
		return EFalse;
		}
	else if (info->iLength>len)
		pkt.TrimEnd(len);
	
	pkt.TrimStart(4);

	switch (op)
		{
	case KPppPapRequest:
		{
		TInt n;
		TPtrC8 user;
		TPtrC8 pass;
		n = *ptr++;
		user.Set(ptr, n);  ptr += n;
		n = *ptr++;
		pass.Set(ptr, n);  ptr += n;
		}
	case KPppPapAck:
		if (id==iCurrentId)
			{
			TimerCancel();
			DoSucceed();
			if(iPppLcp->CallbackEnabled() && iPppLcp->CallbackRequestType() != ECallbackIETFRequestTypeMSCBCP)
				{
				iPppLcp->CallbackGrantedAndAuthenticated();
				iPppLcp->TerminateLink(MNifIfNotify::ECallBack);	// all done, shut it all down
				}
			}
		break;
	case KPppPapNak:
		if (id==iCurrentId)
			{
			TimerCancel();
			DoFail(KErrIfAuthenticationFailure);
			}
		break;
		}
	pkt.Free();
	return EFalse;
	}
