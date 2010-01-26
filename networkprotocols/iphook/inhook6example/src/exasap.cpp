// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// exasap.cpp - Plugin with SAP example module
//

#include "exasap.h"

#pragma warning (disable:4100)

//
// Applies all the modifications needed for every incoming ICMP packet
//
TInt CProtocolExasap::ApplyL(RMBufHookPacket& /*aPacket*/, RMBufRecvInfo& aInfo)
	/**
	* A forwarded packet detected.
	*
	* This example returns always KIp6Hook_PASS without modifying
	* the packet in any way.
	*/
	{
/** @code */
	return KIp6Hook_PASS;
/** @endcode */
	}

#pragma warning (default:4100)
