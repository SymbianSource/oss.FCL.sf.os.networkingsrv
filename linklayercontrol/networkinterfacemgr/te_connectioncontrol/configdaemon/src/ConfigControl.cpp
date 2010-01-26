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
// Implements the config daemon controller
// 
//

/**
 @file
 @internalComponent
*/


#include <nifman.h>
#include <commdbconnpref.h>
#include "Config_Std.h"
#include "ConfigControl.h"
#include "ProgressNotifications.h"
#include "SampleIoctlInterface.h"
#include "networking/pppdebug.h"
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif


/** The simulated registration delay. */
const TInt KRegistrationTimeout = 5 * 1000000;
/** The simulated deregistration delay. */
const TInt KDeregistrationTimeout = 5 * 1000000;
/** The simulated dormant mode delay. */
const TInt KDormantTimeout = 2 * 1000000;		
/** The simulated ioctl delay. */
const TInt KIoctlTimeout = 2 * 1000000;		

/** 
 * Strings put in the CDMA_IWF_NAME field of the CDMA2000 packet
 * service table to define desired behavior of the test 
 * configuration daemon.
 */
_LIT(KDoNotDisableTimers,             "DoNotDisableTimers");
_LIT(KForcePPPRenegotiation,          "ForcePPPRenegotiation");
_LIT(KOnRegisterError,                "OnRegisterError");
_LIT(KOnRegisterDie,                  "OnRegisterDie");
_LIT(KOnRegisterHang,                 "OnRegisterHang");
_LIT(KOnDeregisterOKPreserve,         "OnDeregisterOKPreserve");
_LIT(KOnDeregisterError,              "OnDeregisterError");
_LIT(KOnDeregisterDie,                "OnDeregisterDie");
_LIT(KOnDeregisterHang,               "OnDeregisterHang");
_LIT(KOnExitDormantModeDontNotify,    "OnExitDormantModeDontNotify");
_LIT(KDaemonSuccessfullyDeregistered, "DaemonSuccessfullyDeregistered");
_LIT(KOnDeregisterReportCause,        "OnDeregisterReportCause");

CIoctlHandler* CIoctlHandler::NewL(
	RSocketServ& aEsock)
/**
 * Two phase construction method. Creates and configures a new CIoctlHander instance.
 *
 * @internalComponent
 *
 * @param aEsock Reference to a connected socket server.
 * @leave KErrNoMemory if there is insufficient memory to create the object.
 */	 
	{
	CIoctlHandler* instance = new (ELeave) CIoctlHandler(aEsock);
	CleanupStack::PushL(instance);
	instance->ConstructL();
	CleanupStack::Pop(instance);
	return instance;	
	}

CIoctlHandler::CIoctlHandler(RSocketServ& /*aEsock*/) :
	CTimer(CActive::EPriorityStandard)
/**
 * Creates a new CIoctlHander instance.
 *
 * @internalComponent
 *
 * @param aEsock Reference to a connected socket server.
 */	 
	 {
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::CIoctlHandler Start"));
	
	CActiveScheduler::Add(this);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::CIoctlHandler End"));
	 }

CIoctlHandler::~CIoctlHandler()
/**
 * Destructor. Cancels any outstanding asynchronous requests.
 *
 * @internalComponent
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::~CIoctlHandler Start"));

	Cancel();
	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::~CIoctlHandler End"));
	}

void CIoctlHandler::ConstructL()
/**
 * Second phase construction, initializes the timer active object.
 *
 * @internalComponent
 *
 * @leave KErrNoMemory if there is insufficient memory to create the object.
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConstructL Start"));

	CTimer::ConstructL();

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConstructL End"));
	}

void CIoctlHandler::IoctlL(TInt aName, const RMessage2& aMessage)
/**
 * Processes an Ioctl requesst for ConfigDaemon.
 *
 * @internalComponent
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::IoctlL Start"));

	if (aName == KSampleDaemonOptionName1)
		{
		iMessage = aMessage;
		After(KIoctlTimeout);
		}
	else
		{
		iMessage.Complete(KErrNotSupported);				
		}

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::IoctlL End"));
	}

void CIoctlHandler::RunL()
/**
 * RunL for CIoctlHandler.
 *
 * @internalComponent
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::RunL Start"));

	if (iStatus.Int() != KErrNone)
		{
		iMessage.Complete(iStatus.Int());
		return;
		}

	TPckgBuf<TSampleDaemonParameter1> argPkg;
	iMessage.ReadL(2, argPkg);
	argPkg().iSomeValue++;
	argPkg().iSomeBuf.Copy(_L8("hello"));
	iMessage.Write(2, argPkg);
	iMessage.Complete(KErrNone);
			
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CIoctlHandler::RunL End"));
	}

void CIoctlHandler::DoCancel()
/**
 * Active object DoCancel for CIoctlHandler.
 *
 * @internalComponent
 */
	{
	CTimer::Cancel();
	}

CConfigControl* CConfigControl::NewL(
	RSocketServ& aEsock)
/**
 * Two phase construction method. Creates and configures a new CConfigControl instance.
 *
 * @internalComponent
 *
 * @param aEsock Reference to a connected socket server.
 * @leave KErrNoMemory if there is insufficient memory to create the object.
 */	 
	{
	CConfigControl* instance = new (ELeave) CConfigControl(aEsock);
	CleanupStack::PushL(instance);
	instance->ConstructL();
	CleanupStack::Pop(instance);
	return instance;	
	}

CConfigControl::CConfigControl(RSocketServ& aEsock) :
	CTimer(CActive::EPriorityStandard),
	iEsock(aEsock)
/**
 * Creates a new CConfigControl instance.
 *
 * @internalComponent
 *
 * @param aEsock Reference to a connected socket server.
 */	 
	 {
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::CConfigControl Start"));

	CActiveScheduler::Add(this);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::CConfigControl End"));
	 }

CConfigControl::~CConfigControl()
/**
 * Destructor. Cancels any outstanding asynchronous requests, un-initializes the CommDb connection and
 * closes the Esock connection.
 *
 * @internalComponent
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::~CConfigControl Start"));

	UnInitializeDb();
	UnInitializeConnection();

	Cancel();
	
	if (iIoctlHandler)
		delete iIoctlHandler;
	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::~CConfigControl End"));
	}

void CConfigControl::ConstructL()
/**
 * Second phase construction, initializes the timer active object.
 *
 * @internalComponent
 *
 * @leave KErrNoMemory if there is insufficient memory to create the object.
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConstructL Start"));

	iState = EIdle;
	iProgressOutstanding = EFalse;
	iProgressValid = EFalse;	
	
	CTimer::ConstructL();

    iIoctlHandler = CIoctlHandler::NewL(iEsock);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConstructL End"));
	}

void CConfigControl::InitializeDbL(
	TInt aIapId)
/**
 * Initializes the CommDb fields.
 *
 * @internalComponent
 *
 * @leave If any of the called methods leaves.
 */
	{
	iSession = CMDBSession::NewL(KCDVersion1_2);

	iIap = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	iIap->SetRecordId(aIapId);
	iIap->LoadL(*iSession);

	iService = static_cast<CCDCDMA2000PacketServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdCDMA2000PacketServiceRecord));
	iService->SetRecordId(iIap->iService);
	iService->LoadL(*iSession);

	GetDbServiceParamsL();
	}

void CConfigControl::UnInitializeDb()
/**
 * Un-initializes the CommDb connection.
 *
 * @internalComponent
 */
	{
	delete iService;
	iService = NULL;

	delete iIap;
	iIap = NULL;

	delete iSession;
	iSession = NULL;
	}

void CConfigControl::GetDbServiceParamsL()
/**
 * Retrieves the required service parameters.
 *
 * @internalComponent
 *
 * @leave If any of the called methods leaves.
 */
	{
	TInt rc;
	TBuf<255> address;
	iService->RefreshL(*iSession);

	iDaemonConfiguration = iService->iIwfName;
	
	address = iService->iIpAddr;
	rc = iAddress.Input(address);
	if (rc != KErrNone)
		{
		__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetDbServiceParamsL - Invalid IP address format"));
		User::Leave(rc);
		}

	// So we know that we're not just picking up a static address.
	iAddress.SetAddress(iAddress.Address() + 1);

	address = iService->iIpNetMask;
	rc = iSubnetMask.Input(address);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetDbServiceParamsL - Invalid netmask format [%d]"), rc);
		User::Leave(rc);
		}

	address = iService->iIpGateway;
	rc = iDefGateway.Input(address);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetDbServiceParamsL - Invalid default gateway format [%d]"), rc);
		User::Leave(rc);
		}

	address = iService->iIpNameServer1;
	rc = iNameServer1.Input(address);	
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetDbServiceParamsL - Invalid name server format [%d]"), rc);
		User::Leave(rc);
		}

	address = iService->iIpNameServer2;
	rc = iNameServer2.Input(address);	
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetDbServiceParamsL - Invalid name server format [%d]"), rc);
		User::Leave(rc);
		}

	// set up the broadcast address		
	TInetAddr broadcast;
	broadcast.SubNetBroadcast(iAddress, iSubnetMask);
	iBroadcastAddress = broadcast.Address();		
	}

void CConfigControl::SetDaemonSuccessfullyDeregiseredInDbL()
/**
 * Sets SERVICE_IP_ADDR CommDb field to the specified value.
 *
 * @internalComponent
 *
 * @param aIpAddress Returns the IP address read from CommDb.
 * @leave KErrBadHandle if the view is invalid.
 *		  System errors if any of the called methods leaves.
 */
	{
	if (!iService)
		{
		__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::SetDaemonSuccessInDb - Invalid service object"));
		User::Leave(KErrBadHandle);
		}

	iService->iIwfName = KDaemonSuccessfullyDeregistered;
	iService->ModifyL(*iSession);
	}	

void CConfigControl::InitializeConnectionL(
	const TConnectionInfo& aInfo)
/**
 * Sets up the ESOCK connection, retrieves the interface name and the hardware address.
 *
 * @internalComponent
 * 
 * @leave System errors - if any of the called methods leaves.
 */
	{	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Start"));				

	TInt rc = iEsock.Connect();
	if (KErrNone != rc)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to connect to the ESOCK server [%d]"), rc);				
		User::Leave(rc);				
		}
	
	rc = iConnection.Open(iEsock);
	if (KErrNone != rc)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to open connection [%d]"), rc);				
		User::Leave(rc);				
		}
	
	// start the connection, this will bring up everything
	TConnectionInfoBuf infoBuf = aInfo;
	
	// Attach as Monitor - we need access to the interface for control purposes only, and don't want to affect Nifman idle timers. 
	// If we attach with EAttachTypeNormal, we prevent Nifman from ever switching to Short (Last Session Closed) Idle Timer.
	rc = iConnection.Attach(infoBuf,RConnection::EAttachTypeMonitor);
	if (KErrNone != rc)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to start connection [%d]"), rc);				
		User::Leave(rc);				
		}
		
	rc = iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to open socket [%d]"), rc);
		User::Leave(rc);
		}
		
	// retrieve the connection information		
	rc = GetInterfaceName(aInfo, iInterfaceName);
	if (KErrNone != rc)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to get the interface name [%d]"), rc);				
		User::Leave(rc);
		}
		
	TSoInetInterfaceInfo info;
	rc = GetInterfaceInfo(iInterfaceName, info);
	if (KErrNone != rc)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to get the interface info [%d]"), rc);				
		User::Leave(rc);
		}
	
	iHardwareAddr = info.iHwAddr;
		
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - End"));				
	}

void CConfigControl::UnInitializeConnection()
/**
 * Closes the ESOCK connection.
 *
 * @internalComponent
 */
	{	
	iSocket.Close();
	iConnection.Close();
	}

void CConfigControl::ConfigureInterfaceL()
/**
 * Set the interface IP address and other params
 * into the TCP/IP6 stack.  
 *
 * @internalComponent
 *
 * @leave Errors from RSocket::SetOpt.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(),KConfigLoggingTag2(), _L("CConfigControl::ConfigureInterfaceL - Start"));
		
	TSoInet6InterfaceInfo interfaceInfo;
	interfaceInfo.iHwAddr = iHardwareAddr;
	interfaceInfo.iAddress.SetV4MappedAddress(iAddress.Address());
	interfaceInfo.iNetMask.SetAddress(iSubnetMask.Address());
	interfaceInfo.iBrdAddr.SetV4MappedAddress(iBroadcastAddress.Address());
	interfaceInfo.iDefGate.SetV4MappedAddress(iDefGateway.Address());
	interfaceInfo.iNameSer1.SetV4MappedAddress(iNameServer1.Address());
	interfaceInfo.iNameSer2.SetV4MappedAddress(iNameServer2.Address());
	interfaceInfo.iName = iInterfaceName;
	interfaceInfo.iDelete = EFalse;
	interfaceInfo.iAlias = EFalse;
	interfaceInfo.iDoId = ETrue;
	interfaceInfo.iState = EIfUp;
	interfaceInfo.iDoState = ETrue;
	interfaceInfo.iDoAnycast = EFalse;
	interfaceInfo.iDoProxy = EFalse;
	
	TPckgBuf<TSoInet6InterfaceInfo> configInfo(interfaceInfo);
	TInt rc = iSocket.SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, configInfo);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureInterfaceL - Failed to set socket KSoInetConfigInterface option [%d]"), rc);
		User::Leave(rc);
		}
		
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureInterfaceL - End"));
	}

void CConfigControl::RemoveConfiguredAddressL()
/**
 * Resets the stack IP address.
 *
 * @see "Implementation of IPv4/IPv6 Basic Socket API for Symbian OS"
 * document for explanation of TSoInet6InterfaceInfo and its use
 *
 * @internalComponent
 *
 * @leave Errors from RSocket::SetOpt.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::RemoveConfiguredAddress - Start"));
		
	TSoInet6InterfaceInfo interfaceInfo;
	interfaceInfo.iHwAddr = iHardwareAddr;
	interfaceInfo.iAddress.SetV4MappedAddress(iAddress.Address());
	interfaceInfo.iName = iInterfaceName;
	interfaceInfo.iDelete = ETrue;
	interfaceInfo.iAlias = EFalse;
	interfaceInfo.iDoId = ETrue;
	interfaceInfo.iState = EIfUp;
	interfaceInfo.iDoState = ETrue;
	
	TPckgBuf<TSoInet6InterfaceInfo> configInfo(interfaceInfo);
	TInt rc = iSocket.SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, configInfo);		
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::RemoveConfiguredAddress - Failed to set socket KSoInetConfigInterface option [%d]"), rc);
		User::Leave(rc);
		}
		
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::RemoveConfiguredAddress - End"));
	}

TInt CConfigControl::GetInterfaceName(
	const TConnectionInfo& aInfo,
	TName& aInterfaceName)
/**
 * Retrieves the interface name for a given IAP ID.
 *
 * @internalComponent
 *
 * @param aIapId IAP ID to be used.
 * @param aInterfaceName 	Returns the interface name.
 * @return System codes if the IAP ID cannot be found, KErrNone otherwise.
 */
	{	
	TConnectionInfoBuf bufConnInfo = aInfo;
	bufConnInfo().iIapId = 0; // To be safe.

	TUint count;
	TInt rc = iConnection.EnumerateConnections(count);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetInterfaceName - Failed to enumerate connections [%d]"), rc);				
		return rc;				
		}
	
	// search for the IAP ID
	TUint i = 0;
	rc = KErrNotFound;	
	for (i = 1; i <= count; ++i)
		{
		rc = iConnection.GetConnectionInfo(i, bufConnInfo);
		if (rc != KErrNone)
			{
			__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::GetInterfaceName - Failed to get connection info [%d]"), rc);				
			return rc;				
			}
		
		if (bufConnInfo().iIapId == aInfo.iIapId)
			{
			// found it, store the info and exit
			rc = KErrNone;
			break;
			}
		}
		
	if (rc != KErrNone)
		return rc;
	
	TPckgBuf<TConnInterfaceName> name;
	name().iIndex = i;
	rc = iConnection.Control(KCOLProvider, KConnGetInterfaceName, name);
	
	if (rc != KErrNone)
		return rc;
		
	aInterfaceName = name().iName;

	return rc;	
	}

TInt CConfigControl::GetInterfaceInfo(	
	const TName& aInterfaceName,
	TSoInetInterfaceInfo& aInfo)
/**
 * Retrieves the interface info structure for a given interface name.
 *
 * @internalComponent
 *
 * @param aInterfaceName Interface name to be used.
 * @param aInfo Returns the interface info.
 * @return KErrNotFound if an interface with the given name cannot be found, KErrNone otherwise/
 */
	{
	TInt rc = iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	if (rc != KErrNone)
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("GetInterfaceInfoL::GetIpAddress - Failed to set KSoInetEnumInterfaces option [%d]"), rc);				
		CleanupStack::PopAndDestroy();
		return rc;				
		}
		
	rc = KErrNotFound;
	TPckgBuf<TSoInetInterfaceInfo> opt;
	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == aInterfaceName)
			{
			// found the interface
			aInfo = opt();
			rc = KErrNone;
			break;
			}
		}

	return rc;	
	}

void CConfigControl::ConfigureL(
	const TConnectionInfo& aInfo, 
	const RMessage2& aMessage)
/**
 * Asynchronous call to start the IP address configuration.
 *
 * @internalComponent
 *
 * @param aInfo	Connection info to be used when configuring the address.
 * @param aMessage Message to be completed when finished.
 * @leave Leaves from any of the called methods.
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL Start"));

	if (iState != EIdle)
		{	
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL - Invoked with daemon in unexpected state [%d]. Failing with KErrGeneral"), iState);				
		aMessage.Complete(KErrGeneral);
		return;
		}

	iMessage = aMessage;
	iIapId = aInfo.iIapId;
	InitializeDbL(aInfo.iIapId);
	InitializeConnectionL(aInfo);
	
	// optionally disable the idle timers 
	if (iDaemonConfiguration.Compare(KDoNotDisableTimers) != 0)
		{
		TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, ETrue);
		if (KErrNone != rc)
			{
			__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL - Failed to disable the idle timers [%d]"), rc);				
			User::Leave(rc);				
			}
		}

	// optionally force PPP renegotiation - this will only work in
	// debug builds as the hook in Nifman to pass it on
	// to PPP (and the changes in PPP) are only built in 
	// debug builds.
	if (iDaemonConfiguration.Compare(KForcePPPRenegotiation) == 0)
		{
#ifdef _DEBUG
		TBuf8<1> dummyBuf;
		TInt rc = iConnection.Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP, dummyBuf);
		if (rc != KErrNone) 
			{
			__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL - Failed to force PPP renegotiation. The error was: %d. Note: RConnection::Control(KCOLLinkLayer, KSoIfForceRenegotiatePPP) will only work in debug builds as this hook is not compiled-in in release builds."), rc);				
			User::Leave(rc);				
			}
#else
		__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL - Tried to force PPP renegotiation during registration, but can't invoke RConnection::Control(KCOLLinkLayerTestLevel,KSoIfForceRenegotiatePPP); in release builds."));				
		User::Leave(KErrGeneral);				
#endif
		}
		
		
	// wait 
	iState = ERegistering;
	After(KRegistrationTimeout);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::ConfigureL End"));
	}
	
void CConfigControl::LinkLayerDownL(
	const RMessage2& aMessage)
/**
* Handles start of link-layer renegotiation.
*
* @internalComponent
*
* @param aMessage Message to be completed when finished.
* @leave Leaves from any of the called methods.
*/	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::LinkLayerDown Start"));

	DoOnProgressL(KLinkLayerDownNotificationReceivedByConfigDaemon, KErrNone);
	aMessage.Complete(KErrNone);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::LinkLayerDown End"));
	}

void CConfigControl::LinkLayerUpL(
	const RMessage2& aMessage)
/**
* Handles start of link-layer renegotiation.
*
* @internalComponent
*
* @param aMessage Message to be completed when finished.
* @leave Leaves from any of the called methods.
*/	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::LinkLayerUp Start"));

	DoOnProgressL(KLinkLayerUpNotificationReceivedByConfigDaemon, KErrNone);	
	aMessage.Complete(KErrNone);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::LinkLayerUp End"));
	}

void CConfigControl::DeregisterL(
	TUint aCause, 
	const RMessage2& aMessage)
/**
 * Asynchronous request to deregister.
 *
 * @internalComponent
 *
 * @param aCause Identifies the cause of the deregistration call. (Unused.)
 * @param aMessage	Message to be completed when finished.
 * @leave Leaves from CCommsDbTableView::ReadTextL.
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL Start"));

	iMessage = aMessage;
			
	// We may need to report back which timeout has caused the deregistration.
	// These notifications are specific to this testsuite. They do not appear in Nifman.
	TInt deregisterCauseProgressNotification(-1); // Invalid progress.
	
	// log the cause
	if (aCause == EConfigDaemonDeregisterCauseTimerLastSessionClosed)
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - EConfigDaemonDeregisterCauseTimerLastSessionClosed"));				
		deregisterCauseProgressNotification = KConfigDaemonStartingDeregistrationTimerShort;
		}
	else if (aCause == EConfigDaemonDeregisterCauseTimerLastSocketClosed)
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - EConfigDaemonDeregisterCauseTimerLastSocketClosed"));				
		deregisterCauseProgressNotification = KConfigDaemonStartingDeregistrationTimerMedium;
		}
	else if (aCause == EConfigDaemonDeregisterCauseTimerLastSocketActivity)
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - EConfigDaemonDeregisterCauseTimerLastSocketActivity"));				
		deregisterCauseProgressNotification = KConfigDaemonStartingDeregistrationTimerLong;
		}
	else if (aCause == EConfigDaemonDeregisterCauseTimerUnknown)
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - ERROR: EConfigDaemonDeregisterCauseTimerUnknown"));				
		User::Leave(KErrArgument);
		}	
	else if (aCause == EConfigDaemonDeregisterCauseStop)
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - EConfigDaemonDeregisterCauseStop"));					
		deregisterCauseProgressNotification = KConfigDaemonStartingDeregistrationStop;
		}
	else if (aCause == EConfigDaemonDeregisterCauseTimer) // This timer cause is deprecated - we should never receive it.
		{
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - ERROR: deprecated cause code [EConfigDaemonDeregisterCauseTimer]"));				
		User::Leave(KErrArgument); 
		}
	else
		{
		__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - Unexpected cause code [%d]"), aCause);					
		User::Leave(KErrArgument);				
		}
		
	// short-cut the process if we haven't previously registered
	if (!(iState == ELinkUp || iState == EDormant))
		{	
		iState = EStopping;
		__FLOG_STATIC0(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - Ignoring deregistration request because we're not registered"));				
		TUint8 deregistrationAction = EConfigDaemonDeregisterActionStop;
		TPtrC8 d(&deregistrationAction,sizeof(deregistrationAction));
		TRAPD(rc,iMessage.WriteL(1, d));
		iMessage.Complete(KErrNone);
		return;
		}

	// get the configuration - it may have changed
	iService->RefreshL(*iSession);
	iDaemonConfiguration = iService->iIwfName;
	
	// optionally disable the idle timers 
	if (iDaemonConfiguration.Compare(KDoNotDisableTimers) != 0)
		{
		TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, ETrue);
		if (KErrNone != rc)
			{
			__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL - Failed to disable the idle timers [%d]"), rc);				
			User::Leave(rc);				
			}
		}
	
	// Report to nifman the Deregistration Cause, if required by the test case.	
	if(iDaemonConfiguration.Compare(KOnDeregisterReportCause) == 0)
		{
		DoOnProgressL(deregisterCauseProgressNotification, KErrNone);
		}	
		
	// wait to deregister
	iState = EDeregistering;
	After(KDeregistrationTimeout);
	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::DeregisterL End"));	
	}
	
void CConfigControl::ProgressL(
	const RMessage2& aMessage)
/**
 * Asynchronous registration for progress notifications.
 *
 * @internalComponent
 *
 * @param aMessage Message to be completed when the next progress notification is available.
 * @leave Leaves from CheckProgressL.
 */	 
	{
	if (iProgressOutstanding)
		// other progress notification call active 
		User::Leave(KErrInUse);
		
	// save the message to be completed
	iProgressMessage = aMessage;
	iProgressOutstanding = ETrue;

	// if any progress notification available - generate that
	CheckProgressL();				
	}
		
void CConfigControl::IoctlL(
	TInt aName,
	const RMessage2& aMessage)
/**
 * Asynchronous request to start an operation.
 *
 * @internalComponent
 *
 * @param aName	Identifies the operation.
 * @param aMessage Message to be completed when finished.
 * @leave Leaves from RMessage2::ReadL.
 */	 
	{	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::IoctlL Start"));

	iIoctlHandler->IoctlL(aName,aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::IoctlL End"));
	}

void CConfigControl::CancelRequest()
/**
 * Request to cancel an asynchronous request.
 *
 * @internalComponent
 */	 
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::Cancel Start"));
	
	Cancel();
	iMessage.Complete(KErrCancel);
	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::Cancel End"));
	}
		
void CConfigControl::CancelMask(
	const TUint& aMask)
/**
 * Synchronous request to cancel asynchronous operations.
 *
 * @internalComponent
 * 
 * @param aMask	Identifies the asynchronous operation.
 */	 
	{
	if (aMask & KConfigDaemonOpMaskGeneral)
		{
		Cancel();
		iIoctlHandler->Cancel();
		}
		
	if (aMask & KConfigDaemonOpMaskProgress)
		if (iProgressOutstanding && (!iProgressMessage.IsNull()))
			iProgressMessage.Complete(KErrCancel);
	}	

void CConfigControl::DoOnProgressL(
	const TUint& aStage, 
	const TInt& aError) 
/**
 * Generates a progress notification.
 *
 * @internalComponent
 *
 * @param aStage Stage to be reported
 * @param aError Error to be reported
 * @leave Leaves from CConfigControl::CheckProgressL.
 */	 
	{
	// save the progress info
	iProgressInfo.iStage = aStage;
	iProgressInfo.iError = aError;	
	iProgressValid = ETrue;
	
	// generate the progress notification if the cached info is valid
	CheckProgressL();
	}	
	
void CConfigControl::CheckProgressL()	
/**
 * If the cached progress info is valid and a PProgress call is active completes 
 * the progress notification. 
 *
 * @internalComponent
 * 
 * @leave Leaves from RMessage2::WriteL.
 */	 
	{
	if (iProgressOutstanding && iProgressValid)
		{
		TPckg<TDaemonProgress> progressPkg(iProgressInfo);
		iProgressMessage.WriteL(0, progressPkg);
		iProgressMessage.Complete(KErrNone);
		iProgressOutstanding = EFalse;
		iProgressValid = EFalse;			
		}
	}
	
void CConfigControl::RunL()
/**
 * RunL for CTimer. Indicates that the specified timeout 
 * has elapsed. Executes the configured test scenario.
 *
 * @internalComponent
 * 
 * @param aError Timer error.
 * @leave Leaves from any of the various called methods.
 */
	{
			
	// update the configuration - it may have changed
	iService->RefreshL(*iSession);
	iDaemonConfiguration = iService->iIwfName;

	if (iStatus.Int() != KErrNone)
		{
		iMessage.Complete(iStatus.Int());
		return;
		}
		
	switch (iState)
		{
		case ERegistering:
			if (iDaemonConfiguration.Compare(KOnRegisterError) == 0)
				{
				// simulate registration failure
				
				// re-enable the idle timers 
				TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, EFalse);
				if (KErrNone != rc)
					{
					__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to re-enable the idle timers [%d]"), rc);				
					User::Leave(rc);				
					}

				// complete the message
				iMessage.Complete(KErrCouldNotConnect);
				
				// state remains unchanged
				}	
			else if (iDaemonConfiguration.Compare(KOnRegisterDie) == 0)
				{
				// simulate the server dying
				CActiveScheduler::Stop();
				
				// state remains unchanged
				}	
			else if (iDaemonConfiguration.Compare(KOnRegisterHang) == 0)
				{
				// simulate the case where the daemon goes into 
				// la-la land: do nothing... VERY IMPORTANT NOTE:
				// NIFMAN currently will wait forever for the
				// daemon to complete the request. Only the idle
				// timer could bring it out which we have disabled.
				// This test case is currently commented-out in the 
				// connection control script file.
				}	
			else
				{
				// successful registration
				
				// daemon-specific progress notification
				DoOnProgressL(KConfigDaemonCompletedRegistration, KErrNone);
				
				// set the ip address for the interface
				ConfigureInterfaceL();			
				
				// re-enable the idle timers 
				TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, EFalse);
				if (KErrNone != rc)
					{
					__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to re-enable the idle timers [%d]"), rc);				
					User::Leave(rc);				
					}
				
				// successful registration				
				iMessage.Complete(KErrNone);
				
				// state change
				iState = ELinkUp;			
				}
			break;
			
		case EDeregistering:		 
			if (iDaemonConfiguration.Compare(KOnDeregisterError) == 0)
				{
				// simulate deregistration failure
				
				// re-enable the idle timers 
				TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, EFalse);
				if (KErrNone != rc)
					{
					__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to re-enable the idle timers [%d]"), rc);				
					User::Leave(rc);				
					}

				// complete the message
				iMessage.Complete(KErrCouldNotConnect);
									
				// state remains unchanged
				}	
			else if (iDaemonConfiguration.Compare(KOnDeregisterDie) == 0)
				{
				// server died - terminate the program
				CActiveScheduler::Stop();
				
				// state remains unchanged
				}	
			else if (iDaemonConfiguration.Compare(KOnDeregisterHang) == 0)
				{
				// simulate the case where the daemon goes into 
				// la-la land: do nothing... VERY IMPORTANT NOTE:
				// NIFMAN currently will wait forever for the
				// daemon to complete the request. Only the idle
				// timer could bring it out which we have disabled.
				// This test case is currently commented-out in the 
				// connection control script file.
				}					
			else if (iDaemonConfiguration.Compare(KOnDeregisterOKPreserve) == 0)
				{				
				// simulate going into the dormant state
				
				// daemon-specific progress notification
				DoOnProgressL(KConfigDaemonEnteringFastDormantMode, KErrNone);

				// leave the idle timer disabled
				
				// complete the Deregister async call
				TUint8 deregistrationAction = EConfigDaemonDeregisterActionPreserve;
				TPtrC8 d(&deregistrationAction,sizeof(deregistrationAction));
				TRAPD(rc,iMessage.WriteL(1, d));
				iMessage.Complete(KErrNone);
				
				// state change
				iState = EDormant;										
				
				// to bring us out of the dormant state
				After(KDormantTimeout);
				}
			else
				{									
				// successful deregistration, but stop the interface

				// remove the address from the interface			
				RemoveConfiguredAddressL();

				// put in the DB that we succeded.
				SetDaemonSuccessfullyDeregiseredInDbL();	
				
				// re-enable the idle timers 
				TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, EFalse);
				if (KErrNone != rc)
					{
					__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to re-enable the idle timers [%d]"), rc);				
					User::Leave(rc);				
					}
				
				// complete the Deregister async call
				TUint8 deregistrationAction = EConfigDaemonDeregisterActionStop;
				TPtrC8 d(&deregistrationAction,sizeof(deregistrationAction));
				TRAP(rc,iMessage.WriteL(1, d));
				iMessage.Complete(KErrNone);
										
				// state change
				iState = EStopping;
				}					
			break;
			
		case EDormant:
			if (iDaemonConfiguration.Compare(KOnExitDormantModeDontNotify) == 0)
				{
				// don't provide a notification - just go to link-up
				iState = ELinkUp;
				}
			else
				{
				// re-enable the idle timers 
				TInt rc = iConnection.SetOpt(KCOLProvider, KConnDisableTimers, EFalse);
				if (KErrNone != rc)
					{
					__FLOG_STATIC1(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigControl::InitializeConnectionL - Failed to re-enable the idle timers [%d]"), rc);				
					User::Leave(rc);				
					}
					
				// indicate that the system exited dormant mode
				DoOnProgressL(KConfigDaemonFinishedDormantMode, KErrNone);			
				iState = ELinkUp;
				}
			break;
						
		default:
			// do nothing
			break;
		}				
	}
	
void CConfigControl::DoCancel()
/**
 * Active object DoCancel for CConfigControl.
 *
 * @internalComponent
 */
	{
	CTimer::DoCancel();
	}

