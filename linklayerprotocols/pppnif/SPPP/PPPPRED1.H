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

/**
 @file
 @internalComponent 
*/

#if !defined(__PPPPRED1_H__)
#define __PPPPRED1_H__

#include "pppbase.h"
#include "in_sock.h"
#include "in_iface.h"
#include "nifman.h"

class MCpred1Recvr : public MPppRecvr
	{
public:
	MCpred1Recvr(CPppLcp* aLcp);
	virtual TBool RecvFrame(RMBufChain& aPacket);
	};



#endif // __PPPPRED1_H

