/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* RConfigDaemon Client side header
* Declares the Symbian OS DHCP Client API RConfigDaemon
* 
*
*/



/**
 @file CS_Daemon.h
*/

#ifndef __RCONFIGDAEMON_H__
#define __RCONFIGDAEMON_H__

#include <e32base.h>
#include <e32svr.h>
#include <comms-infras/cs_asyncconnect.h>
#include <comms-infras/rconfigdaemonmess.h>

/**
 * The RConfigDaemon class
 * @internalTechnology
 * Implements the Symbian OS Configuration daemon Client API
 */
class RConfigDaemon : public RAsyncConnectBase
	{
public:
	void Configure(const TDes8& aInfo, TRequestStatus& aStatus);
	void LinkLayerDown();
	void LinkLayerUp();
	void Deregister(TInt aCause, TDes8* aActionStatus, TRequestStatus& aStatus);
	void ProgressNotification(TDaemonProgressBuf& aProgress, TRequestStatus& aStatus);
	void Ioctl(TUint aOptionLevel, TUint aOptionName, TRequestStatus& aStatus, TDes8* aDes=NULL);
	void Cancel(TRequestStatus& aStatus);
	void Cancel(TUint aOpMask, TRequestStatus& aStatus);
	TVersion Version() const;
	};

#endif

