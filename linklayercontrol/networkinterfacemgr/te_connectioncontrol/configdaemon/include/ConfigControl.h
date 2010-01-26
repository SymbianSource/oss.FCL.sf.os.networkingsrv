/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Defines the CConfigControl class and helper classes. CConfigControl 
* implements the core functionality of the test configuration daemon.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __CONFIGCONTROL_H__
#define __CONFIGCONTROL_H__

#include <e32base.h>
#include <es_enum.h>
#include <in_sock.h>
#include <comms-infras/rconfigdaemonmess.h>
#include <metadatabase.h>
#include <commsdattypesv1_1.h>
using namespace CommsDat;

class CIoctlHandler : public CTimer
/**
 * Asynchrounously handles Ioctl requests.
 *
 * @internalComponent
 */
	{
public:
	// constructors/destructors, etc.
	static CIoctlHandler* NewL(RSocketServ& aEsock);
	virtual ~CIoctlHandler();
	
	// handles an Ioctl request 
	void IoctlL(TInt aName, const RMessage2& aMessage);
	
protected:
	// active object
	void RunL();
	void DoCancel();

private:
	// construction
	CIoctlHandler(RSocketServ& aEsock);
	void ConstructL();
	
	/** The message we are to process and complete. (Except for progress messages.)*/
	RMessage2 iMessage;
	};

class CConfigControl : public CTimer
/**
 * Implements all server requests for ConfigDaemon.
 *
 * @internalComponent
 */
	{
public:
	// constructors/destructors, etc.
	static CConfigControl* NewL(RSocketServ& aEsock);
	virtual ~CConfigControl();
		
	// daemon API 
	void ConfigureL(const TConnectionInfo& aInfo, const RMessage2& aMessage);
	void LinkLayerDownL(const RMessage2& aMessage);
	void LinkLayerUpL(const RMessage2& aMessage);
	void DeregisterL(TUint aCause, const RMessage2& aMessage);
	void ProgressL(const RMessage2& aMessage);
	void IoctlL(TInt aName, const RMessage2& aMessage);
	void CancelRequest();
	void CancelMask(const TUint& aMask);
	
protected:
	// active object
	void RunL();
	void DoCancel();
	
private:
	// construction
	CConfigControl(RSocketServ& aEsock);
	void ConstructL();


	/** TConfigControlState - The state of the deamon. */
	enum TConfigControlState
		{
		EIdle,
		ERegistering,
		ELinkUp,
		EDeregistering,
		EDormant,
		EStopping
		};

	// db utilities
	void InitializeDbL(TInt aIapId);
	void UnInitializeDb();
	void GetDbServiceParamsL();
	void SetDaemonSuccessfullyDeregiseredInDbL();

	// connection utilities
	void InitializeConnectionL(const TConnectionInfo& aInfo);
	void UnInitializeConnection();
	void ConfigureInterfaceL();
	void RemoveConfiguredAddressL();
	TInt GetInterfaceName(const TConnectionInfo& aInfo, TName& aInterfaceName);
	TInt GetInterfaceInfo(const TName& aInterfaceName,	TSoInetInterfaceInfo& aInfo);
	
	// progress utilities
	void DoOnProgressL(const TUint& aStage, const TInt& aError);
	void CheckProgressL();	

	// CommsDat objects
	/** The database session. Created in InitializeDbL. Destroyed in UnInitializeDb */
	CMDBSession* iSession;
	/** The IAP record. Created in InitializeDbL. Destroyed in UnInitializeDb */
	CCDIAPRecord* iIap;

	// Esock objects
	/** Esock server. Opened in InitializeConnectionL. Closed in UnInitializeConnection. Closed in UnInitializeConnection. */
	RSocketServ iEsock;
	/** The connection. The connection, in fact, that was started by the test app. We attach to it. Opened in InitializeConnectionL. Closed in UnInitializeConnection. Closed in UnInitializeConnection. */
	RConnection iConnection;
	/** A socket on the connection. Used to configure it/get info about it. Used to set the IP address of the connection.Opened in InitializeConnectionL. Closed in UnInitializeConnection. Closed in UnInitializeConnection. */
	RSocket iSocket;

	// Message related vars
	/** The message we are to process and complete. (Except for progress messages.)*/
	RMessage2 iMessage;
	
	/** Handles ioctl requests. */
	CIoctlHandler* iIoctlHandler;

	/** The progress message we are to process and complete. */
	RMessage2 iProgressMessage;
	/** ETrue if we are already processing a progress notification request. */
	TBool iProgressOutstanding;
	/** Stores a progress notification that we've generated. */
	TDaemonProgress iProgressInfo;
	/** ETrue if iProgressInfo contains valid data. */
	TBool iProgressValid;
	
	/** The test configuration (from commdb). */
	TBuf<255> iDaemonConfiguration;
	
	// CommDb params
	/** The IAP record for the connection. */
	TInt iIapId;
	/** The name of the interface. */
	TName iInterfaceName;
	/** The hardware address of the interface. */
	TSockAddr iHardwareAddr;

	/** The IP address we will set for the interface. */
	TInetAddr iAddress;
	/** The subnet mask address we will set for the interface. */
	TInetAddr iSubnetMask;
	/** The default gateway we will set for the interface. */
	TInetAddr iDefGateway;
	/** The name server (1) we will set for the interface. */
	TInetAddr iNameServer1;
	/** The name server (2) we will set for the interface. */
	TInetAddr iNameServer2;	
	/** The broadcast address we will set for the interface. */
	TInetAddr iBroadcastAddress;
	
	/** The current state of the object */
	TConfigControlState iState;
	};

#endif


