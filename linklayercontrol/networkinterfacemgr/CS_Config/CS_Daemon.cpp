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
//

#include "CS_Daemon.h"
#include "CS_DaemonMess.h"

const TInt KConfigDaemonMajorVersionNumber = 2;
const TInt KConfigDaemonMinorVersionNumber = 0;
const TInt KConfigDaemonBuildVersionNumber = 0;

 	
TVersion RConfigDaemon::Version() const
/**
 * The RConfigDaemon::Version method
 *
 * Extract the version of the API
 *
 * @internalTechnology
 *
 * @return	Version of the API
 */
	{
	return TVersion(KConfigDaemonMajorVersionNumber,KConfigDaemonMinorVersionNumber,KConfigDaemonBuildVersionNumber);
	}

void RConfigDaemon::Configure(const TDes8& aInfo, TRequestStatus& aStatus )
/**
 * The RConfigDaemon::Connect method
 *
 * Connect the Handle to the Server
 * Must be called before all other methods except Version()
 *
 * @internalTechnology
 *
 * @param aInfo The startup info for the connection
 * @param aStatus The request status of the active object to be completed
 *
 * @return	KErrNone if successful, otherwise the error that occurred
 */
	{
   	SendReceive(EConfigDaemonConfigure,TIpcArgs(&aInfo), aStatus);
	}

void RConfigDaemon::LinkLayerDown()
/**
 * Informs the daemon of the start of link layer renegotiation. 
 *
 * @internalTechnology
 *
 * @param aStatus Active object iStatus to complete
 */
	{
   	Send(EConfigDaemonLinkLayerDown);		
	}	

void RConfigDaemon::LinkLayerUp()
/**
 * Informs the daemon of the completion of link layer renegotiation. 
 *
 * @internalTechnology
 *
 * @param aStatus Active object iStatus to complete
 */
	{
   	Send(EConfigDaemonLinkLayerUp);		
	}	

void RConfigDaemon::Deregister(TInt aCause, TDes8* aActionStatus, TRequestStatus& aStatus)
/**
 * Issues a deregistration request. 
 *
 * @internalTechnology
 * @param aCause Specifies what caused the deregister call (idle timer or Stop call)	
 * @param aActionStatus	Returns the action to be executed by Nifman 
 *						(shut down the NIF and the agent or keep them up)
 * @param aStatus Active object iStatus to complete
 */
	{
   	SendReceive(EConfigDaemonDeregister, TIpcArgs(aCause, aActionStatus), aStatus);		
	}	

void RConfigDaemon::ProgressNotification(TDaemonProgressBuf& aProgressBuf, TRequestStatus& aStatus)
/**
 * Issues a progress notification request. This request will be completed when the daemon has 
 * something to report.
 *
 * @internalTechnology
 * @param aProgressBuf Returns the progress information
 * @param aStatus Active object iStatus to complete
 */
	{
	SendReceive(EConfigDaemonProgress, TIpcArgs(&aProgressBuf), aStatus);	
	}	

void RConfigDaemon::Ioctl(TUint aOptionLevel, TUint aOptionName, TRequestStatus& aStatus, TDes8* aDes)
/**
  * Issues an asynchronous command. Various option's levels & names could be supported
  * by the loaded daemon
  *
  * @internalTechnology
  * @return TInt An error code
  * @param aOptionLevel - Control level
  * @param aOptionName - Name of control request
  * @param aStatus - active object iStatus to complete
  * @param aOption - Buffer for data to be retrieved
  */
	{
  	SendReceive(EConfigDaemonIoctl, TIpcArgs(aOptionLevel, aOptionName, aDes, aDes ? aDes->MaxLength() : 0), aStatus);
	}

void RConfigDaemon::Cancel(TRequestStatus& aStatus)
/**
  * Cancels current request asynchronously => 
  * just one request at the time could be issued see
  * CNifConfigurationControl::StoreClientStatus functinon in NIFConfigurationControl.cpp
  *
  * @internalTechnology
  * @param aStatus - active object iStatus to complete
  * @return TInt An error code
  */
	{
   //!!This async call assumes there will be always place in the daemon queue to send the 
   //cancel message. The msg queue is 10 requests long, each daemon processes one request at the
   //time => dangerous situation occures when there'r running more than 3 different links, each
   //configured by daemon, each having outstanding request 
   //and we cancel all of them at the same time. Very, very unlikely situation.
   	SendReceive(EConfigDaemonCancel, TIpcArgs(), aStatus);
	}

void RConfigDaemon::Cancel(TUint aOpMask, TRequestStatus& aStatus)
/**
 * Cancels an asynchronous operation. The operation to be canceled is specified 
 * in the mask.
 *
 * @internalTechnology
 * @param aOpMask Specifies the operation to be canceled
 * @param aStatus active object iStatus to complete
 */
	{
   	SendReceive(EConfigDaemonCancelMask, TIpcArgs(aOpMask), aStatus);	
	}


