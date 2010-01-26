// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// This header defines default bearers.These values must be used
// by the NetMcpr to calculate the TCP receive window for a connection.
// PPP and Ethernet will be using these bearer type.
// 
//

/**
 @file
 @internalAll
 @Released
*/

#ifndef __CF_BEARERS_H_
#define __CF_BEARERS_H_


/**
 * Identify any other bearer which is not
 * defined in the wifibearer.h and etelbearers.h
 */
const TInt KDefaultBearer             = 0;   

/**
 * Identify ethernet bearer
 */
const TInt KEthernetBearer            = 1;   

#endif //__CF_BEARERS_H_

