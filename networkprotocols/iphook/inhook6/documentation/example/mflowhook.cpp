// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This file is for doxygen documentation only, used as
// @dontinclude mflowhook.cpp
// @skip pattern
// @until //-
//

#include <flow.h>

class CFlowHookExample : public CBase, public MFlowHook
	{
public:
	void Open();
	TInt ReadyL(TPacketHead &aHead);
	TInt ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &aInfo);
	void Close();
	TInt iGood4;
	TInt iGood6;
	TInt iBad;
	TInt iUgly;
private:
	~CFlowHookExample();
	TInt iRefs;
	};
//-
CFlowHookExample::~CFlowHookExample()
	{
	}
//-
void CFlowHookExample::Open()
	{
	iRefs += 1;
	}
//-
void CFlowHookExample::Close()
	{
	if (--iRefs < 0)
		delete this;
	}
//-
TInt CFlowHookExample::ReadyL(TPacketHead &)
	{
	return EFlow_READY;
	}
//-
TInt CFlowHookExample::ApplyL(RMBufSendPacket &aPacket, RMBufSendInfo &)
	{
	TIpHeader *const ip = ((RMBufPacketPeek &)aPacket).GetIpHeader();
	if (ip == NULL)
		{
		iBad += 1;	// Bad packet
		}
	else if (ip->ip4.Version() == 4)
		{
		iGood4 += 1;	// IPv4 packet
		}
	else if (ip->ip6.Version() == 6)
		{
		iGood6 += 1;	// IPv6 packet
		}
	else
		{
		iUgly += 1; // Some other? Bad?
		}
	return KErrNone;
	}
//-
