/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/



/**
 @file SHIMCLIENT.H
 @internalComponent
*/

#if !defined(__SHIMCLIENT_H__)
#define __SHIMCLIENT_H__

#include <e32def.h>
#include <e32base.h>
#include <comms-infras/metadata.h>

class TNifProgress;
class MShimControlClient
/**
 the former CSubConnection down-calls (interface seen by ESOCK and used to comunicate sub-connection
 related calls made via RConnection using either id or index.
 
 @internalComponent
 */
	{
public:
	//former CConnection::StopSubConnectionL(const RMessage2& aMessage);
	virtual TBool StopL(const RMessage2& aMessage) = 0;
	// Former calls from CConnection::GetSubConnectionInfo
	virtual TInt GetSubConnectionInfo(const RMessage2& aMessage) = 0;
	//	Former Calls from RConnection via CSubConnection
	virtual TInt GetCurrentProgress(TNifProgress& aProgress) = 0;
	virtual TBool DataTransferredL(const RMessage2& aMessage) = 0;
	virtual TBool DataTransferredCancel(const RMessage2& aMessage) = 0;
	virtual TBool RequestSubConnectionProgressNotificationL(const RMessage2& aMessage) = 0;
	virtual TBool CancelSubConnectionProgressNotification(const RMessage2& aMessage) = 0;
	virtual TBool DataSentNotificationRequestL(const RMessage2& aMessage) = 0;
	virtual TBool DataSentNotificationCancel(const RMessage2& aMessage) = 0;
	virtual TBool DataReceivedNotificationRequestL(const RMessage2& aMessage) = 0;
	virtual TBool DataReceivedNotificationCancel(const RMessage2& aMessage) = 0;
	virtual TBool IsSubConnectionActiveRequestL(const RMessage2& aMessage) = 0;
	virtual TBool IsSubConnectionActiveCancel(const RMessage2& aMessage) = 0;
	
	virtual TInt ReturnCode() const = 0;
	};
	
#endif // __SHIMCLIENT_H__
