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
// Obsolete TCP/IP header file
// 
//

/**
 @file in_std.h
 @internalComponent
 @deprecated This file originates from an obsolete implementation of the TCP/IP stack
*/

#ifndef __IN_STD_H__
#define __IN_STD_H__

#include <nifmbuf.h>

// Added from INET.H and TCP.H as they are needed in VJCOMP

// Room for all layer of headers
const TUint KInetMaxHeaderSize = 128;

const TUint KTcpFIN = 0x01;
const TUint KTcpSYN = 0x02;
const TUint KTcpRST = 0x04;
const TUint KTcpPSH = 0x08;
const TUint KTcpACK = 0x10;
const TUint KTcpURG = 0x20;
const TUint KTcpECN = 0xc0;     // RFC 3168

class CNifIfBase;
class RMBufRecvInfo : public RMBufPktInfo
	{
public:
	CNifIfBase* iInterface;
	};

#endif
