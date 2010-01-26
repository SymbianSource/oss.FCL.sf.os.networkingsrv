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
 @brief Header file for the implementation of Password Authentication
 @internalComponent 
*/

#ifndef __PPPPAP_H__
#define __PPPPAP_H__

#include "PPPAUTH.H"

const TInt KPppPapWaitTime = 3000;
const TInt KPppPapRetries = 4;

const TUint8 KPppPapRequest = 1;
const TUint8 KPppPapAck = 2;
const TUint8 KPppPapNak = 3;


NONSHARABLE_CLASS(CPppPap) : public CPppAuthentication, protected MTimer
	{
public:
	static CPppAuthentication* NewL();
	virtual ~CPppPap();

	virtual void InitL(CPppLcp* aLcp);
	virtual void AuthenticateComplete(TInt aStatus);

protected:
	virtual TUint PppId() const;

	// MPppRecvr upcalls
	virtual TBool RecvFrame(RMBufChain& aPacket);
	virtual void LowerLayerUp();
	virtual void LowerLayerDown(TInt aStatus=KErrNone);
	// MTimer upcall
	virtual void TimerComplete(TInt aStatus);

private:
	void SendAuthRequestL(TBool aNewRequest=EFalse);
	CPppPap();

private:
	TInt	iTryCount;
	TUint8	iCurrentId;
	};

/**
   Object factory for CPppPap.
   @leave Standard Symbian OS error codes. e.g. KErrNoMemory.
*/
inline CPppAuthentication* CPppPap::NewL()
	{
	return new(ELeave) CPppPap;
	}

inline TUint CPppPap::PppId() const
	{
	return KPppIdPap;
	}

#endif // ___PPPPAP_H__
