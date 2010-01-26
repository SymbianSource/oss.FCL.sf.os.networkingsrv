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

#ifndef __PPPBASE_INL__
#define __PPPBASE_INL__

#include <networking/ppplcp.h>

/** @file
@internalComponent */

//
// PPP Link Frame Checksum
//

// 16 bit CRC
inline TPppFcs16::TPppFcs16()
	{
	Init();
	}
	
/**
Returns the current CRC value.
@return CRC-16
*/	
inline TUint16 TPppFcs16::Fcs() const
	{
	return (TUint16)~iFcs;
	}

/**
Sets the CRC value to the initial state before calculation.
*/
inline void TPppFcs16::Init()
	{
	iFcs = (TUint16)KPppInitFcs16;
	}
	
/**
Determines if the calculated CRC is valid.

@return ETrue if the calculated CRC matches the expected value
*/
inline TBool TPppFcs16::IsGood() const
	{
	return iFcs==KPppGoodFcs16;
	}

/**
Incrementally calculates the CRC for the next message byte.

@param aByte Message byte
*/
inline void TPppFcs16::CalcByte(TUint8 aByte)
	{
	iFcs = (TUint16)((iFcs >> 8) ^ FcsTable[(iFcs ^ aByte) & 0xff]);
	}


// 32 bit CRC
inline TPppFcs32::TPppFcs32()
	{
	Init();
	}

/**
Returns the current CRC value.
@return CRC-32
*/	
inline TUint32 TPppFcs32::Fcs() const
	{
	return (TUint32)~iFcs;
	}

/**
Sets the CRC value to the initial state before calculation.
*/
inline void TPppFcs32::Init()
	{
	iFcs = (TUint32)KPppInitFcs32;
	}

/**
Determines if the calculated CRC is valid.

@return ETrue if the calculated CRC matches the expected value
*/
inline TBool TPppFcs32::IsGood() const
	{
	return iFcs==KPppGoodFcs32;
	}

/**
Incrementally calculates the CRC for the next message byte.

@param aByte Message byte
*/
inline void TPppFcs32::CalcByte(TUint8 aByte)
	{
	iFcs = (((iFcs) >> 8) ^ FcsTable[((iFcs) ^ aByte) & 0xff]);
	}

//
// PPP Packet Receiver
//

inline TInt MPppRecvr::SendFrame(RMBufChain& aPacket)
	{ return iPppLcp->PppLink()->Send(aPacket, iPppId); }
inline void MPppRecvr::SetId(TUint aPppId)
	{ iPppId  = aPppId; }
inline void MPppRecvr::SetPhase(TPppPhase aPhase)
	{ iActivePhase  = aPhase; }

//
// PPP Link Protocol Support
//

inline void CPppLinkBase::NotifyLinkUp()
	{ iPppLcp->LinkLayerUp(); }
inline void CPppLinkBase::NotifyLinkDown(TInt aReason)
	{ iPppLcp->LinkLayerDown(aReason); }
inline void CPppLinkBase::DeliverToLcp(RMBufChain& aPacket)
	{ iPppLcp->LinkRecv(aPacket); }
inline void CPppLinkBase::NewCompressor(const CPppCompressor* aCompressor)
	{ 
	  iPppCompressor = (CPppCompressor*)aCompressor;
	}
inline void CPppLinkBase::NewDeCompressor(const CPppDeCompressor* aDeCompressor)
	{ 
		iPppDecompressor = (CPppDeCompressor*)aDeCompressor; 
	}


//
// PPP Options Support
//

inline RPppOption& RPppOption::Cast(RMBufChain& aChain)
	{ return *((RPppOption*)&aChain); }
inline const RPppOption& RPppOption::Cast(const RMBufChain& aChain)
	{ return *((const RPppOption*)&aChain); }
inline const RPppOption& RPppOption::operator=(const RMBufChain& aChain)
	{ iNext = (RMBuf*)aChain.First(); return *this; }

//
// PPP FSM
//

inline TBool MPppFsm::FsmIsThisLayerOpen()
	{ return iState==EPppFsmOpened; }

#endif

