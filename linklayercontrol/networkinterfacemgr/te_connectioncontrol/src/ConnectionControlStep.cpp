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
// Implements the test classes for the Nifman connection control mechanism.
// 
//

/**
 @file
 @internalComponent
*/

#include <s32file.h>
#include <testexecutelog.h>
#include <es_enum.h>
#include <nifman.h>
#include <in_iface.h>
#include <commdbconnpref.h>
#include <c32comm.h> // uniquely for the call to StartC32WithCMISuppressions
#include "ConnectionControlStep.h"
#include "../configdaemon/include/ProgressNotifications.h"
#include "../configdaemon/include/SampleIoctlInterface.h"
#include <nifman.h>
#include <networking/pppdebug.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif

/** The tolerances on the idle timer. */
const TInt KLastSocketClosedTimeoutDelta = 5;

/** 
 * Strings put in the CDMA_IWF_NAME field of the CDMA2000 packet
 * service table to define desired behavior of the test 
 * configuration daemon.
 */
_LIT(KDoNotDisableTimers,             "DoNotDisableTimers");
_LIT(KForcePPPRenegotiation,          "ForcePPPRenegotiation");
_LIT(KOnRegisterOK,                   "OnRegisterOK");
_LIT(KOnRegisterError,                "OnRegisterError");
_LIT(KOnRegisterDie,                  "OnRegisterDie");
_LIT(KOnRegisterHang,                 "OnRegisterHang");
_LIT(KOnDeregisterOKStop,             "OnDeregisterOKStop");
_LIT(KOnDeregisterOKPreserve,         "OnDeregisterOKPreserve");
_LIT(KOnDeregisterError,              "OnDeregisterError");
_LIT(KOnDeregisterDie,                "OnDeregisterDie");
_LIT(KOnDeregisterHang,               "OnDeregisterHang");
_LIT(KDaemonSuccessfullyDeregistered, "DaemonSuccessfullyDeregistered");
_LIT(KOnDeregisterReportCause,        "OnDeregisterReportCause");


CConnectionControlProgress::CConnectionControlProgress()
	: CActive(CActive::EPriorityStandard)
/**
 * Constructs the progress active object.
 *
 * @internalComponent
 */
	{
	}
	
CConnectionControlProgress::~CConnectionControlProgress()
/**
 * Destroys the progress active object.
 *
 * @internalComponent
 */
	{
	iConnection = NULL;
	iDestination = NULL;
	}

void CConnectionControlProgress::RunL()
/**
 * Progress notification from the connection. Propagates it to the event subscriber.
 *
 * @internalComponent
 *
 * @leave Doesn't leave.
 */
	{	
	if (iDestination)
		{
		TInt rc = iStatus.Int();
		if (KErrNone == rc)
			rc = iProgressBuf().iError; 
		iDestination->DoOnProgressNotification(iProgressBuf().iStage, rc);			
		}
	}

void CConnectionControlProgress::DoCancel()
/**
 * Cancels an outstanding progress request.
 *
 * @internalComponent
 */
	{
	if (iConnection)
		iConnection->CancelProgressNotification();	
	}
	
void CConnectionControlProgress::ProgressNotification(
	MProgressDestination* aDestination,
	RConnection* aConnection)
/**
 * Issues an asynchronous progress request.
 * 	
 * @internalComponent
 * 	
 * @param	aDestination	Destination for progress notifications.
 * @param	aConnection		Opened connection to be used to issue the request.
 */
	{
	iDestination = aDestination;
	iConnection = aConnection;
	
	if (iConnection)
		{	
		iConnection->ProgressNotification(iProgressBuf, iStatus);
		SetActive();
		}		
	}

CConnectionControlAllInterface::CConnectionControlAllInterface()
	: CActive(CActive::EPriorityStandard)
/**
 * Constructs the all interface active object.
 *
 * @internalComponentf
 */
	{
	}
	
CConnectionControlAllInterface::~CConnectionControlAllInterface()
/**
 * Destroys the all interface active object. Cancels any outstanding asynchronous request.
 *
 * @internalComponent
 */
	{
	iConnection = NULL;
	iDestination = NULL;	
	}

void CConnectionControlAllInterface::RunL()
	{
/**
 * All interface notification from the connection. Propagates it to the event destination.
 *
 * @internalComponent
 */
	if (iDestination) 
		iDestination->DoOnAllInterfaceNotification(iAllInterfaceBuf().iConnectionInfo.iIapId, iAllInterfaceBuf().iState, iStatus.Int());
	}
	
void CConnectionControlAllInterface::DoCancel()
/**
 * Cancels an outstanding all interface request.
 *
 * @internalComponent
 */
	{
	if (iConnection)
		iConnection->CancelAllInterfaceNotification();
	}
	
void CConnectionControlAllInterface::AllInterfaceNotification(
	MAllInterfaceDestination* aDestination, 
	RConnection* aConnection)
/**
 * Cancels an outstanding all interface request.
 *
 * @internalComponent
 *
 * @param aDestination Destination for all interface notifications.
 * @param aConnection Opened connection to be used to issue the request.
 */
	{
	iDestination = aDestination;
	iConnection = aConnection;
	
	if (iConnection)
		{	
		iConnection->AllInterfaceNotification(iAllInterfaceBuf, iStatus);
		SetActive();
		}		
	}
	
CConnectionControlTimer* CConnectionControlTimer::NewL(
	MTimerDestination* aTimerDestination)	
/**
 * Constructs and configures a new CConnectionControlTimer object.
 *
 * @internalComponent
 *
 * @param aTimerDestination	Target or subscriber for timer notifications.
 * @leave Leaves from CConnectionControlTimer::ConstructL.
 */
	{
	CConnectionControlTimer* self = new (ELeave)CConnectionControlTimer(aTimerDestination);
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CConnectionControlTimer::CConnectionControlTimer(
	MTimerDestination* aTimerDestination)
	: CTimer(CActive::EPriorityStandard),
	iTimerDestination(aTimerDestination)
/**
 * Constructs the timer active object.
 *
 * @internalComponent
 *
 * @param aTimerDestination	Target or subscriber for timer notifications.
 */
	{
	}
	
CConnectionControlTimer::~CConnectionControlTimer()
/**
 * Destroys the timer active object. Cancels any outstanding timer request.
 *
 * @internalComponent
 */
	{
	iTimerDestination = NULL;
	}

void CConnectionControlTimer::RunL()
/**
 * Timeout notification, propagated to the destination.
 *
 * @internalComponent
 *
 * @leave Doesn't leave.
 */
	{
	if (iTimerDestination)
		iTimerDestination->DoOnTimerExpired(iStatus.Int());
	}

CConnectionControlStep::CConnectionControlStep()
	: CTestStep()
/**
 * Constructs the connection control base object.
 *
 * @internalComponent
 */
	{
	}

CConnectionControlStep::~CConnectionControlStep()
/**
 * Destructor, deletes all dynamic member variables. 
 *
 * @internalComponent
 */
	{
	// delete the active objects first
	iAllInterface->Cancel();
	delete iAllInterface;
	iAllInterface = NULL;
	
	iProgress->Cancel();
	delete iProgress;
	iProgress = NULL;

	iTimer->Cancel();
	delete iTimer;
	iTimer = NULL;	

	CloseDb();
	UnInitializeConnection();
	}
	
void CConnectionControlStep::ConstructL()
/**
 * Second phase construction. Instantiate all active objects.
 *
 * @internalComponent
 *
 * @leave Leaves if out of memory.
 */
	{	
	iProgress = new (ELeave) CConnectionControlProgress(); 
	iAllInterface = new (ELeave) CConnectionControlAllInterface(); 
	iTimer = CConnectionControlTimer::NewL(this);
	}

TVerdict CConnectionControlStep::doTestStepPreambleL()
/**
 * Implements the pure virtual doTestStepPreambleL defined in CTestStep. 
 * Used in this case it is used to disable the phone book synchronizer
 * which will cause the tests to fail.
 *
 * @internalComponent
 *
 * @return EPass if successful.
 * @leave Doesn't leave.
 */
	{	
	_LIT(KPhbkSyncCMI, "phbsync.cmi");
	TInt err = StartC32WithCMISuppressions(KPhbkSyncCMI);
	TESTL(err == KErrNone || err == KErrAlreadyExists);
	INFO_PRINTF1(_L("Test Step Preamble: disabled Phonebook Synchronizer."));	
	return EPass;
	}

TVerdict CConnectionControlStep::doTestStepL()
/**
 * Implements the pure virtual doTestStepL defined in CTestStep. 
 * Allows the base class to execute before any of the derived 
 * tests is called.
 *
 * @internalComponent
 *
 * @leave If any of the called methods leaves.
 */
	{	
	// get the configuration and initialize the CommDb
	// the configuration is only valid here and these steps could not 
	// have been done in ConstructL
	GetConfigurationL();
	
	// open the database
	OpenDbL();
	
	// set test configuration in the database so that the 
	// daemon may read it
	WriteTestConfigToDbL();
	
	// run the test
	return RunTestStepL();
	}
	
void CConnectionControlStep::GetConfigurationL()
/**
 * Retrieves the common configuration values from the INI file.
 *
 * @internalComponent
 *
 * @leave KErrNotFound if any of the configuration values cannot be read.
 *		  System error if the IP address format is incorrect.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::GetConfigurationL - Start"));				

	GetBoolFromConfig(ConfigSection(), KDoNotDisableTimers, iDoNotDisableTimers);
	GetBoolFromConfig(ConfigSection(), KForcePPPRenegotiation, iForcePPPRenegotiation);
	GetBoolFromConfig(ConfigSection(), KOnRegisterOK, iOnRegisterOK);
	GetBoolFromConfig(ConfigSection(), KOnRegisterError, iOnRegisterError);
	GetBoolFromConfig(ConfigSection(), KOnRegisterDie, iOnRegisterDie);
	GetBoolFromConfig(ConfigSection(), KOnRegisterHang, iOnRegisterHang);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterOKStop, iOnDeregisterOKStop);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterOKPreserve, iOnDeregisterOKPreserve);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterError, iOnDeregisterError);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterDie, iOnDeregisterDie);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterHang, iOnDeregisterHang);
	GetBoolFromConfig(ConfigSection(), KOnDeregisterReportCause, iOnDeregisterReportCause);

	TPtrC strExpectedRegIpAddress;
	if (!GetIntFromConfig(ConfigSection(), _L("IAPID"), iIapId) ||
		!GetStringFromConfig(ConfigSection(), _L("ExpectedRegIPAddress"), strExpectedRegIpAddress) ||
		!GetIntFromConfig(ConfigSection(), _L("LastSocketClosedTimeout"), iLastSocketClosedTimeout) ||
		!GetIntFromConfig(ConfigSection(), _L("LastActivityTimeout"), iLastActivityTimeout) ||
		!GetIntFromConfig(ConfigSection(), _L("LastSessionClosedTimeout"), iLastSessionClosedTimeout) ||
		!GetIntFromConfig(ConfigSection(), _L("ProgressNotificationTimeout"), iProgressNotificationTimeout))
		{
		INFO_PRINTF1(_L("CConnectionControlStep::GetConfigurationL - Failed to read configuration file"));		
		User::Leave(KErrNotFound);
		}	
			
	
	TInt rc = iExpectedRegIpAddress.Input(strExpectedRegIpAddress);
  	if (rc != KErrNone)
  		{
  		INFO_PRINTF2(_L("CConnectionControlStep::GetConfigurationL - Invalid IP address format"), rc);		
  		User::Leave(rc);
  		}

	INFO_PRINTF1(_L("CConnectionControlStep::GetConfigurationL - End"));				
	}
	
void CConnectionControlStep::OpenDbL()
/**
 * Initializes the CommDb views using the IAP ID read from the configuration file.
 *
 * @internalComponent
 *
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::OpenDbL - Start"));				
	iSession = CMDBSession::NewL(KCDVersion1_2);
	iIap = static_cast<CCDIAPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIAPRecord));
	iIap->SetRecordId(iIapId);
	iIap->LoadL(*iSession);
	
	
	// service
	if (iService == 0)
		{
    	const TDesC& servType = iIap->iServiceType;

		if (servType.CompareF(TPtrC(KCDTypeNameDialOutISP))==0)
			{
			iService = static_cast<CCDDialOutISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialOutISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDialInISP))==0)
			{
			iService = static_cast<CCDDialInISPRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDialInISPRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameLANService))==0)
			{
			iService = static_cast<CCDLANServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameVPNService))==0)
			{
			iService = static_cast<CCDVPNServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVPNServiceRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameOutgoingWCDMA))==0)
			{
			iService = static_cast<CCDOutgoingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdOutgoingGprsRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameIncomingWCDMA))==0)
			{
			iService = static_cast<CCDIncomingGprsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdIncomingGprsRecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameCDMA2000PacketService))==0)
			{
			iService = static_cast<CCDCDMA2000PacketServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdCDMA2000PacketServiceRecord));
			}
		/*else if (servType.CompareF(TPtrC(WHAT IS THIS NAME?))==0)
			{
			iService = static_cast<CCDWCDMAPacketServiceRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWCDMAPacketServiceRecord));
			}*/
		else if (servType.CompareF(TPtrC(KCDTypeNameDefaultWCDMA))==0)
			{
			iService = static_cast<CCDDefaultWCDMARecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDefaultWCDMARecord));
			}
		else if (servType.CompareF(TPtrC(KCDTypeNameDefaultCDMA2000Settings))==0)
			{
			iService = static_cast<CCDDefaultCDMA2000SettingsRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdDefaultCDMA2000SettingsRecord));
			}
		else
			{
        	INFO_PRINTF1(_L("CConnectionControlStep::OpenDbL() - service type does not match any supported type"));	
			User::Leave(KErrBadName);
			}
		iService->SetRecordId(iIap->iService);
		}
	iService->LoadL(*iSession);



	// bearer
	if (iBearer == 0)
		{
		const TDesC& bearType = iIap->iBearerType;

		if (bearType.CompareF(TPtrC(KCDTypeNameModemBearer))==0)
			{
			iBearer = static_cast<CCDModemBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdModemBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameLANBearer))==0)
			{
			iBearer = static_cast<CCDLANBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdLANBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameVirtualBearer))==0)
			{
			iBearer = static_cast<CCDVirtualBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdVirtualBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameWAPSMSBearer))==0)
			{
			iBearer = static_cast<CCDWAPSMSBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWAPSMSBearerRecord));
			}
		else if (bearType.CompareF(TPtrC(KCDTypeNameWAPIPBearer))==0)
			{
			iBearer = static_cast<CCDWAPIPBearerRecord*>(CCDRecordBase::RecordFactoryL(KCDTIdWAPIPBearerRecord));
			}
		else
			{
        	INFO_PRINTF1(_L("CConnectionControlStep::OpenDbL() - bearer type does not match any supported type"));	
			User::Leave(KErrBadName);
			}
		iBearer->SetRecordId(iIap->iBearer);
		}
	iBearer->LoadL(*iSession);

	
	INFO_PRINTF1(_L("CConnectionControlStep::OpenDbL - End"));				
	}
	
void CConnectionControlStep::CloseDb()
/**
 * Un-initializes the CommDb views and database objects.
 *
 * @internalComponent
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::CloseDb - Start"));				

	delete iBearer;
	iBearer = NULL;

	delete iService;
	iService = NULL;

	delete iIap;
	iIap = NULL;

	delete iSession;
	iSession = NULL;

	INFO_PRINTF1(_L("CConnectionControlStep::CloseDb - End"));				
	}
	
TBool CConnectionControlStep::DaemonSuccessfullyDeregisteredL()
/**
 * Checks that the daemon reset the CDMA_IWF_NAME field in commdb 
 * indicating successful deregistration.
 *
 * @internalComponent
 *
 * @leave KErrBadHandle if the view is invalid.
 *		  System errors if any of the called methods leaves.
 */
	{
	TBuf<255> strDaemonResult;	

	if (!iService)
		{
		INFO_PRINTF1(_L("CConnectionControlStep::GetDbIpAdressL - Invalid service object"));		
		User::Leave(KErrBadHandle);
		}

	CCDCDMA2000PacketServiceRecord* cdma = static_cast<CCDCDMA2000PacketServiceRecord*>(iService);
	strDaemonResult = cdma->iIwfName;
	
	if (strDaemonResult.Compare(KDaemonSuccessfullyDeregistered) == 0)
		return ETrue;
	else
		return EFalse;
	}	
	
void CConnectionControlStep::WriteTestConfigToDbL()
/**
 * Write the test configuration to commdb.
 *
 * @internalComponent
 *
 * @leave System errors if any of the called methods leaves.
 */
	{	
	if (!iService)
		{
		INFO_PRINTF1(_L("CConnectionControlStep::WriteTestConfigToDbL - Invalid service object"));		
		User::Leave(KErrBadHandle);
		}

	CCDCDMA2000PacketServiceRecord* cdma = static_cast<CCDCDMA2000PacketServiceRecord*>(iService);
	cdma->RefreshL(*iSession);

	// put in the CDMA_IWF_NAME field the desired behavior of the 
	// test configuration daemon
	if (iDoNotDisableTimers)
		{
		cdma->iIwfName.SetMaxLengthL(KDoNotDisableTimers.BufferSize);
		cdma->iIwfName = KDoNotDisableTimers;
		}
	else if (iForcePPPRenegotiation)
		{
		cdma->iIwfName.SetMaxLengthL(KForcePPPRenegotiation.BufferSize);
		cdma->iIwfName = KForcePPPRenegotiation;
		}
	else if (iOnRegisterOK)
		{
		cdma->iIwfName.SetMaxLengthL(KOnRegisterOK.BufferSize);
		cdma->iIwfName = KOnRegisterOK;
		}
	else if (iOnRegisterError)
		{
		cdma->iIwfName.SetMaxLengthL(KOnRegisterError.BufferSize);
		cdma->iIwfName = KOnRegisterError;
		}
	else if (iOnRegisterDie)
		{
		cdma->iIwfName.SetMaxLengthL(KOnRegisterDie.BufferSize);
		cdma->iIwfName = KOnRegisterDie;
		}
	else if (iOnRegisterHang)   
		{
		cdma->iIwfName.SetMaxLengthL(KOnRegisterHang.BufferSize);
		cdma->iIwfName = KOnRegisterHang;
		}
	else if (iOnDeregisterOKStop)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterOKStop.BufferSize);
		cdma->iIwfName = KOnDeregisterOKStop;
		}
	else if (iOnDeregisterOKPreserve)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterOKPreserve.BufferSize);
		cdma->iIwfName = KOnDeregisterOKPreserve;
		}
	else if (iOnDeregisterError)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterError.BufferSize);
		cdma->iIwfName = KOnDeregisterError;
		}
    else if (iOnDeregisterDie)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterDie.BufferSize);
		cdma->iIwfName = KOnDeregisterDie;
		}
	else if (iOnDeregisterHang)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterHang.BufferSize);
		cdma->iIwfName = KOnDeregisterHang;
		}
	else if (iOnDeregisterReportCause)
		{
		cdma->iIwfName.SetMaxLengthL(KOnDeregisterReportCause.BufferSize);
		cdma->iIwfName = KOnDeregisterReportCause;
		}
	else 
		{
		INFO_PRINTF1(_L("CConnectionControlStep::WriteTestConfigToDbL - One of OnRegisterOK, OnRegisterError, etc. must be set."));				
		User::Leave(KErrGeneral);		
		}

	cdma->ModifyL(*iSession);

	if (!iBearer)
		{
		INFO_PRINTF1(_L("CConnectionControlStep::WriteTestConfigToDbL - Invalid bearer object"));		
		User::Leave(KErrBadHandle);
		}

	CCDLANBearerRecord* lan = static_cast<CCDLANBearerRecord*>(iBearer);
	lan->RefreshL(*iSession);

	lan->iLastSocketClosedTimeout = iLastSocketClosedTimeout;
	lan->iLastSocketActivityTimeout	= iLastActivityTimeout;
	lan->iLastSessionClosedTimeout = iLastSessionClosedTimeout;

	lan->ModifyL(*iSession);
	}
	
void CConnectionControlStep::UpdateDaemonBehaviourFieldL(const TDesC& aDaemonBehaviour)
/**
 * Change the behaviour of the test configuration daemon.
 *
 * @internalComponent
 *
 * @leave System errors if any of the called methods leaves.
 */
	{	
	if (!iService)
		{
		INFO_PRINTF1(_L("CConnectionControlStep::UpdateDaemonBehaviourFieldL - Invalid view object"));		
		User::Leave(KErrBadHandle);
		}

	CCDCDMA2000PacketServiceRecord* cdma = static_cast<CCDCDMA2000PacketServiceRecord*>(iService);
	cdma->RefreshL(*iSession);

	cdma->iIwfName = aDaemonBehaviour;

	cdma->ModifyL(*iSession);
	}

TInt CConnectionControlStep::AttachToConn(RConnection& aSourceConn, RConnection& aSinkConn, TBool aAsMonitor)
/**
 * Attaches to the specified interface.
 *
 * @internalComponent
 * 
 * @param aSourceConn The interface to attach to (attachment target).
 * @param aSinkConn The interface to be attached to the target.
 * @param aAsMonitor If ETrue, attaches the aSinkConn as monitor
 * @return KErrNone on success, or a system-wide error code.
 */	
	{	
	TInt rc = aSinkConn.Open(iEsock); 
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::AttachToConn - Failed to open sink connection [%d]"), rc);				
		return rc;				
		}
	
	TConnectionInfo sourceConnInfo;
	TInt idxUnused;
	rc = GetConnectionInfo(iIapId, sourceConnInfo, idxUnused, aSourceConn);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::AttachToConn - GetConnectionInfo on source connection failed [%d]"), rc);
		return rc;							
		}
	
	TPckg<TConnectionInfo> sourceConnInfoBuf(sourceConnInfo);
	if(aAsMonitor)
		{
		rc = aSinkConn.Attach(sourceConnInfoBuf,RConnection::EAttachTypeMonitor);
		}
	else
		{
		rc = aSinkConn.Attach(sourceConnInfoBuf,RConnection::EAttachTypeNormal);
		}
		
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::AttachToConn - RConnection::Attach failed [%d]"), rc);
		}
	return rc;
	}
	

TInt CConnectionControlStep::StartConnection(RConnection& aConnection)
/**
 * Starts the specified interface
 *
 * @internalComponent
 * 
 * @param aConnection The interface to start
 * @return KErrNone on success, or a system-wide error code.
 */
	{
	TInt rc = iEsock.Connect();
	if(KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::StartConnection - Failed to connect to Esock [%d]"), rc);				
		return rc;
		}
		
	rc = aConnection.Open(iEsock);
	if(KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::StartConnection - Failed to Open Connection [%d]"), rc);				
		return rc;
		}
				
	TCommDbConnPref connPref;
	connPref.SetIapId(iIapId);
	connPref.SetDialogPreference(ECommDbDialogPrefDoNotPrompt);

	// start the connection, this will bring up everything
	rc = aConnection.Start(connPref);
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::StartConnection - Failed to start connection [%d]"), rc);				
		}
	return rc;
	}

TInt CConnectionControlStep::InitializeConnectionL()
/**
 * Sets up the ESOCK connection and retrieves the interface name.
 *
 * @internalComponent
 *
 * @leave System errors if any of the called methods leaves.
 */
	{	
	INFO_PRINTF1(_L("CConnectionControlStep::InitializeConnectionL - Start"));				

	TInt rc = StartConnection(iConnection);
	if(KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::InitializeConnectionL - StartConnection failed [%d]"), rc);				
		return rc;
		}
	
	// retrieve the connection information
	rc = GetInterfaceName(iIapId, iInterfaceName);
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::InitializeConnectionL - Failed to get the interface name [%d]"), rc);				
		return rc;				
		}
		
	INFO_PRINTF1(_L("CConnectionControlStep::InitializeConnectionL - End"));				

	return KErrNone;
	}
	
void CConnectionControlStep::UnInitializeConnection()
/**
 * Un-initializes the ESOCK connection.
 *
 * @internalComponent
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::UnInitializeConnection - Start"));				

	if (iConnection.SubSessionHandle())
		{
		iConnection.Stop(RConnection::EStopNormal);
		iConnection.Close();
		}
		
	if (iEsock.Handle())
		iEsock.Close();
		
	INFO_PRINTF1(_L("CConnectionControlStep::UnInitializeConnection - End"));				
	}


TInt CConnectionControlStep::GetConnectionInfo(
	TInt aIapId,
	TConnectionInfo& aInfo,
	TInt& aIndex)
/**
 * Retrieves the default test connection information and index for a given IAP ID.
 *
 * @internalComponent
 *
 * @param aIapId IAP ID to be searched.
 * @param aInfo Returns the connection info.
 * @param aIndex Returns the connection index.
 * @return KErrNotFound if the specified IAP ID cannot be found, KErrNone otherwise.
 */	{
	return GetConnectionInfo(aIapId, aInfo, aIndex, iConnection);
	}
	

TInt CConnectionControlStep::GetConnectionInfo(
	TInt aIapId,
	TConnectionInfo& aInfo,
	TInt& aIndex,
	RConnection& aConn)
/**
 * Retrieves the connection information and index for a given IAP ID.
 *
 * @internalComponent
 *
 * @param aIapId IAP ID to be searched.
 * @param aInfo Returns the connection info.
 * @param aIndex Returns the connection index.
 * @param aConn the connection to get information about
 * @return KErrNotFound if the specified IAP ID cannot be found, KErrNone otherwise.
 */
	{
	TConnectionInfoBuf bufConnInfo = aInfo;

	TUint count;
	TInt rc = aConn.EnumerateConnections(count);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::GetConnectionInfo - Failed to enumerate connections [%d]"), rc);				
		return rc;				
		}
	
	// search the IAP ID
	TUint i = 0;
	rc = KErrNotFound;	
	for (i = 1; i <= count; ++i)
		{
		rc = aConn.GetConnectionInfo(i, bufConnInfo);
		if (rc != KErrNone)
			{
			INFO_PRINTF2(_L("CConnectionControlStep::GetConnectionInfo - Failed to get connection info [%d]"), rc);				
			return rc;				
			}
		
		if (bufConnInfo().iIapId == (TUint)aIapId)
			{
			// found it, store the info and exit
			aIndex = i;
			aInfo = bufConnInfo();
			rc = KErrNone;
			break;
			}
		}
	
	return rc;	
	}	

TInt CConnectionControlStep::GetInterfaceName(
	TInt aIapId,
	TName& aInterfaceName)
/**
 * Retrieves the interface name for a given IAP ID.
 *
 * @internalComponent
 *
 * @param aIapId IAP ID to be searched.
 * @param aInterfaceName Returns the interface name.
 * @return System codes if the IAP ID cannot be found, KErrNone otherwise.
 */
	{
	TInt index;
	TConnectionInfo connInfo;
	TInt rc = GetConnectionInfo(aIapId, connInfo, index);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::GetInterfaceName - Failed to retrieve connection information [%d]"), rc);				
		return rc;				
		}
		
	TPckgBuf<TConnInterfaceName> name;
	name().iIndex = index;
	rc = iConnection.Control(KCOLProvider, KConnGetInterfaceName, name);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::InitGetInterfaceName - Failed to get interface name [%d]"), rc);				
		return rc;				
		}

	aInterfaceName = name().iName;
	return rc;
	}

TInt CConnectionControlStep::GetInterfaceInfoL(	
	const TName& aInterfaceName,
	TSoInetInterfaceInfo& aInfo)
/**
 * Retrieves the interface info structure for a given interface name.
 *
 * @internalComponent
 *
 * @param aInterfaceName Interface name to be used.
 * @param aInfo Returns the interface info.
 * @return KErrNotFound if an interface with the given name cannot be found, KErrNone otherwise.
 */
	{
	RSocket socket;
	TInt rc = socket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::GetInterfaceInfoL - Failed to open socket [%d]"), rc);
		return rc;
		}
	CleanupClosePushL(socket);

	rc = socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::GetInterfaceInfoL - Failed to set KSoInetEnumInterfaces option [%d]"), rc);				
		CleanupStack::PopAndDestroy(&socket);
		return rc;				
		}
		
	rc = KErrNotFound;
	TPckgBuf<TSoInetInterfaceInfo> opt;
	while (socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == aInterfaceName)
			{
			// found the interface
			aInfo = opt();
			rc = KErrNone;
			break;
			}
		}

	CleanupStack::PopAndDestroy(&socket);
	return rc;	
	}
		
TInt CConnectionControlStep::GetIpAddressL(
	const TName& aInterfaceName,
	TInetAddr& aIpAddress)
/**
 * Retrieves the IP address associated with a given interface name.
 *
 * @internalComponent
 *
 * @param aInterfaceName Interface name to be used.
 * @param aIpAddress Returns the IP address.
 * @return  System error codes in case of error, KErrNone otherwise.
 */
	{
	TSoInetInterfaceInfo info;
	TInt rc = GetInterfaceInfoL(aInterfaceName, info);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::GetIpAddressL - Failed to get the interface info [%d]"), rc);
		return rc;
		}
	
	aIpAddress = info.iAddress;
	return KErrNone;	 
	}	

void CConnectionControlStep::DoOnProgressNotification(
	TInt aStage, 
	TInt aError)
/**
 * Progress callback from the active object. In case of errors or when the expected 
 * progress stage is reached, it stops the current active scheduler, otherwise re-issues the 
 * progress request.
 *
 * @internalComponent
 *
 * @param aStage Progress stage.
 * @param aError Error code.
 */
	{
	INFO_PRINTF3(_L("CConnectionControlStep::DoOnProgressNotification - Received notification %d error %d"), aStage, aError);		
	
	iReachedProgressStage = aStage; 
	if ((aError != KErrNone) || (aStage == iExpectedProgressStage))
		{
		INFO_PRINTF1(_L("CConnectionControlStep::DoOnProgressNotification - Stopping the scheduler"));		
		iProgressError = aError;
		CActiveScheduler::Stop();
		}
	else
		{
		// re-issue the asynchronous request
		iProgress->ProgressNotification(this, &iConnection); 
		}		
		
	INFO_PRINTF1(_L("CConnectionControlStep::DoOnProgressNotification - End"));		
	}
	
void CConnectionControlStep::DoOnAllInterfaceNotification(
	TInt aIapId, 
	TConnInterfaceState aState,
	TInt aError)
/**
 * All interface callback from the active object. In case of errors or when the expected 
 * interface state is reached, it stops the current active scheduler, otherwise re-issues the 
 * all interface request.
 *
 * @internalComponent
 *
 * @param aIapId Indentifies the interface this notification is for.
 * @param aState Interface state.
 * @param aError Async request completion error code.
 */
	{
	if (((KErrNone != aError) || (iExpectedInterfaceState == aState)) && (aIapId == iIapId))
		{
		INFO_PRINTF1(_L("CConnectionControlStep::DoOnAllInterfaceNotification - Stopping the scheduler"));		
		iReachedInterfaceState = aState;
		iInterfaceError = aError;
		CActiveScheduler::Stop();
		}
	else
		{
		// re-issue the asynchronous request
		iAllInterface->AllInterfaceNotification(this, &iConnection); 
		}			
	}	
		
void CConnectionControlStep::DoOnTimerExpired(
	TInt /* aError */)
/**
 * Timer callback from the active object. If the close connection flag was set it attempts to close
 * the connection and re-issue the timer request, otherwise stops the active scheduler.
 *
 * @internalComponent
 *
 * @param aError Async request completion error code. Not used.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::DoOnTimerExpired - Timer expired"));		

	// indicate that it timed out
	iAsyncError = KErrTimedOut;
	CActiveScheduler::Stop();
	}
	
TInt CConnectionControlStep::WaitForAnyNotificationL(
	TInt& aProgressStage,
	TInt& aProgressError,
	TConnInterfaceState& aInterfaceState,
	TInt& aInterfaceError,
	TInt aTimeout,
	TBool aProgressEnabled, 
	TBool aAllInterfaceEnabled)	
/**
 * Installs a new active scheduler and synchronously waits for one of the following notifications:
 * - progress stage to reach the specified stage
 * - interface state to reach the specified state
 * - timeout
 * The progress and all interface notifications can be enabled/disabled individually.
 *
 * @internalComponent
 *
 * @param	aProgressStage		Expected progress stage considered if aProgressEnabled is ETrue. 
 *		 						Returns the last progress stage reached.
 * @param	aProgressError		Error returned by the asynchronous progress notification.
 * @param	aInterfaceState		Expected interface state considered if aAllInterfaceEnabled is ETrue
 *		 						Returns the last interface stage reached.
 * @param	aInterfaceError		Error returned by the interface.
 * @param	aTimeout			Timeout value in seconds.
 * @param	aProgressEnabled	Enables the progress notification.
 * @param	aAllInterfaceEnabled	Enables the all interface notification.
 * @return	System error code, KErrTimedOut if the timer exited the scheduler.
 * @leave	If any of the called methods leaves
 */
	{
	INFO_PRINTF5(_L("CConnectionControlStep::WaitForAnyNotificationL - Waiting for progress stage %d/error %d or interface state %d with timeout %d sec"), aProgressStage, aProgressError, aInterfaceState, aTimeout);		

	// store the expected values
	iExpectedProgressStage = aProgressStage;
	iReachedProgressStage = KConnectionUninitialised;
	iProgressError = KErrNone;
	iExpectedInterfaceState = aInterfaceState;
	iReachedInterfaceState = EInterfaceDown;
	iInterfaceError = KErrNone;
	
	iAsyncError = KErrNone;

	// save the current scheduler
	CActiveScheduler* pOldScheduler = CActiveScheduler::Current();
	
	// create and install a new active scheduler
	CActiveScheduler* pScheduler = new (ELeave) CActiveScheduler;
	CleanupStack::PushL(pScheduler);
	CActiveScheduler::Install(pScheduler);
	
	// issue the asynchronous requests
	pScheduler->Add(iTimer);	
	iTimer->After(aTimeout * 1000 * 1000);
	
	if (aProgressEnabled)
		{	
		pScheduler->Add(iProgress);
		iProgress->ProgressNotification(this, &iConnection); 
		}
	if (aAllInterfaceEnabled)
		{	
		pScheduler->Add(iAllInterface);
		iAllInterface->AllInterfaceNotification(this, &iConnection); 
		}
			
	// run the scheduler
	CActiveScheduler::Start();

	// make sure the requests are canceled	
	if (aProgressEnabled)
		iProgress->Cancel();
		
	if (aAllInterfaceEnabled)
		iAllInterface->Cancel();

	iTimer->Cancel();
	
	aProgressStage = iReachedProgressStage;
	aProgressError = iProgressError;
	aInterfaceState = iReachedInterfaceState;
	aInterfaceError = iInterfaceError;

	// go back to the old scheduler
	CActiveScheduler::Install(pOldScheduler);		
	CleanupStack::PopAndDestroy(pScheduler);
	
	INFO_PRINTF2(_L("CConnectionControlStep::WaitForAnyNotificationL - Returning %d"), iAsyncError);		
	return iAsyncError;
	}	
	
TInt CConnectionControlStep::WaitForProgressStageL(
	TInt aProgressStage, 
	TInt aProgressError,
	TInt aTimeout)
/**
 * Synchronously waits for the specified progress stage and error. If the 
 * stage is not reached within the specified timeout or the error code for
 * the stage does not match the specified one it returns an error.
 *
 * @internalComponent
 *
 * @param	aProgressStage	Progress stage to be reached.
 * @param	aProgressError	Progress error to be reaported with the stage.
 * @param	aTimeout		Timeout in seconds to wait for the stage.
 * @return	KErrUnknown if the stage was not reached or the error code does not match.
 * @leave	Leaves from WaitForAnyNotificationL.
 */
	{
	// wait for KMinDaemonProgress for a little more than the socket last closed timeout
	TInt progressStage = aProgressStage;
	TConnInterfaceState interfaceState = EInterfaceDown;
	TInt rcProgress = KErrNone;
	TInt rcInterface = KErrNone;	
	TInt rc = WaitForAnyNotificationL(progressStage, rcProgress, interfaceState, rcInterface, 
		aTimeout, ETrue, EFalse);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::WaitForProgressStageL - Failed, error returned [%d]"), rc);
		return rc;
		}
						
	// check if it reached the specified stage
	if (progressStage != aProgressStage)
		{
		INFO_PRINTF1(_L("CConnectionControlProgressStep - Failed, expected stage not reached"));			
		return KErrUnknown;					
		}

	// check if error
	if (rcProgress != aProgressError)
		{
		INFO_PRINTF3(_L("CConnectionControlProgressStep - Failed, expected stage was not reached with expected error [%d/%d]"), aProgressError, rcProgress);			
		return KErrUnknown;					
		}	
		
	return rc;	
	}
	
TBool CConnectionControlStep::IpAddressMatchesExpectedRegistrationAddressL()
/**
 * Checks the interface IP address and checks to see if it matches the 
 * expected result.
 *
 * @internalComponent
 *
 * @return ETrue if they match.
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::IpAddressMatchesExpectedRegistrationAddressL - Start"));		
	
	// assume they don't match
	TBool rc = EFalse;

	// retrieve and print the current IP address from the stack
	TInetAddr localIpAddress;
	TInt err = GetIpAddressL(iInterfaceName, localIpAddress);	
	if (err != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::IpAddressMatchesExpectedRegistrationAddressL - Failed to retrieve the local IP address [%d]"), rc);				
		User::Leave(err);					
		}
	
	TBuf<255> address;
	localIpAddress.Output(address);
	INFO_PRINTF2(_L("CConnectionControlStep::IpAddressMatchesExpectedRegistrationAddressL - Retrieved local IP address as %s"), address.PtrZ());
	
	// compare the stack IP address with the configured one
	if (localIpAddress.Match(iExpectedRegIpAddress))
		rc = ETrue;
			
	INFO_PRINTF1(_L("CConnectionControlStep::IpAddressMatchesExpectedRegistrationAddressL - End"));		
	return rc;			
	}
	
TInt CConnectionControlStep::DoSuccessfulRegistrationL()
/**
 * Attempts to successfully register. Starts a new connection, waits for KLinkLayerOpen, retrieves 
 * the interface IP address and compares it with the configured IP address.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlStep::DoSuccessfulRegistrationL - Start"));		

	// start the connection 
	TInt rc = InitializeConnectionL();
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::DoSuccessfulRegistrationL - Failed to initialize connection. The error was: [%d]."), rc);				
		return rc;			
		}

	// wait for KLinkLayerOpen 
	rc = WaitForProgressStageL(KLinkLayerOpen, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlStep::DoSuccessfulRegistration - Failed, error waiting for KLinkLayerOpen/KErrNone [%d]"), rc);
		return rc;				
		}

	// retrieve and print the current IP address from the stack
	if (!IpAddressMatchesExpectedRegistrationAddressL())
		rc = KErrGeneral;
		
	INFO_PRINTF1(_L("CConnectionControlStep::DoSuccessfulRegistrationL - End"));		
	return rc;			
	}
	
TVerdict CConnectionControlNullDaemonTest::RunTestStepL()
/**
 * Tests configuration errors in commdb and, consequently, 
 * the behaviour of the null daemon.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlNullDaemonTest - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();
	
	// get the expected error code from configuration
	TInt rc1Expected;
	if (!GetIntFromConfig(ConfigSection(), _L("FirstErrorCode"), rc1Expected))
		{
		INFO_PRINTF1(_L("CConnectionControlNullDaemonTest - Failed to read expected error code from configuration"));			
		SetTestStepResult(EFail);					
		return TestStepResult();							
		}

	// start the connection 
	TInt rc = InitializeConnectionL();
	if (rc != rc1Expected)
		{
		INFO_PRINTF2(_L("CConnectionControlNullDaemonTest - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlNullDaemonTest - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlNullDaemonTest - End"));			
	return TestStepResult();
	}

TVerdict CConnectionControlRegOKStep::RunTestStepL()
/**
 * Tests a successful registration scenario.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlRegOKStep - Start"));		

	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();	
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlRegOKStep - Failed"));
		SetTestStepResult(EFail);					
		} 
	else
		{		
		INFO_PRINTF1(_L("CConnectionControlRegOKStep - Passed"));
		SetTestStepResult(EPass);
		}		

	// clean-up
	UnInitializeConnection();
		
	INFO_PRINTF1(_L("CConnectionControlRegOKStep - End"));			
	return TestStepResult();
	}

TVerdict CConnectionControlRegErrorStep::RunTestStepL()
/**
 * Tests an un-successful registration scenario
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlRegErrorStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	TInt rcExpected;
	if (!GetIntFromConfig(ConfigSection(), _L("FirstErrorCode"), rcExpected))
		{
		INFO_PRINTF1(_L("CConnectionControlRegErrorStep - Failed to read expected error code from configuration"));			
		SetTestStepResult(EFail);					
		return TestStepResult();							
		}
		
	// start the connection 
	TInt rc = InitializeConnectionL();
	
	// we may get the error from here
	if (rc == rcExpected)
		{
		INFO_PRINTF2(_L("CConnectionControlRegErrorStep - Got expected error code (%d) from InitializeConnectionL."), rc);
		SetTestStepResult(EPass);
		return TestStepResult();					
		}
	else if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlRegErrorStep - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// or here. 
	rc = WaitForProgressStageL(KLinkLayerOpen, rcExpected, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlRegErrorStep - Failed, error waiting for KLinkLayerOpen [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlRegErrorStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlRegErrorStep - End"));			
	return TestStepResult();
	}

TVerdict CConnectionControlDeregisterOKStep::RunTestStepL()
/**
 * Tests a successful deregistration scenario.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
	
	// wait for KConfigDaemonFinishedDeregistrationStop 
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterOKStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerClosed/KErrTimedOut 
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterOKStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
			
	// clean-up
	UnInitializeConnection();
				
	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - End"));			
	return TestStepResult();	
	}
	
TVerdict CConnectionControlDeregisterErrorStep::RunTestStepL()
/**
 * Tests an un-successful deregistration scenario.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// get the expected error code from configuration
	TInt rc1Expected;
	if (!GetIntFromConfig(ConfigSection(), _L("FirstErrorCode"), rc1Expected))
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - Failed to read expected error code from configuration"));			
		SetTestStepResult(EFail);					
		return TestStepResult();							
		}
		
	TInt rc2Expected;
	if (!GetIntFromConfig(ConfigSection(), _L("SecondErrorCode"), rc2Expected))
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - Failed to read expected error code from configuration"));			
		SetTestStepResult(EFail);					
		return TestStepResult();							
		}
		
	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
				
	// wait for KConfigDaemonFinishedDeregistrationStop with the expected error code
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, rc1Expected, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterErrorStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop/KErrCouldNotConnect [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerClosed with the expected error code
	rc = WaitForProgressStageL(KLinkLayerClosed, rc2Expected, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterErrorStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// clean-up
	UnInitializeConnection();

	// verify that the daemon did not deregister
	if (DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - It would appear that the daemon erroneously deregistered."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - Passed"));			
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlDeregisterErrorStep - End"));			
	return TestStepResult();	
	}

TVerdict CConnectionControlDeregisterConnectionStep::RunTestStepL()
/**
 * Tests a successful deregistration while simultaneously creating a new socket 
 * on the interface. The socket does not affect the deregistration process and 
 * KLinkLayerClosed is received with KErrTimedOut.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterConnectionStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterOKStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
		
	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterConnectionStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// create a socket
	RSocket socket;
	rc = socket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	CleanupClosePushL(socket);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterConnectionStep - Failed to open socket [%d]"), rc);
		User::Leave(rc);
		}
		
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterConnectionStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop/KErrServerTerminated [%d]"), rc);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket);	
		return TestStepResult();							
		}			

	// wait for KLinkLayerClosed
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterConnectionStep - Failed, error waiting for KLinkLayerClosed/KErrServerTerminated [%d]"), rc);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket); 
		return TestStepResult();							
		}			
		
	INFO_PRINTF1(_L("CConnectionControlDeregisterConnectionStep - Passed"));
	SetTestStepResult(EPass);
	
	CleanupStack::PopAndDestroy(&socket);

	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterConnectionStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterConnectionStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlDeregisterStopAuthoritativeStep::RunTestStepL()
/**
 * Tests a successful deregistration while simultaneously issuing an 
 * RConnection::Stop. The deregistration is expected to be aborted. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterStopAuthoritativeStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterStopAuthoritativeStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
		
	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopAuthoritativeStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// stop the connection
	iConnection.Stop(RConnection::EStopAuthoritative);
	
	// wait for KConfigDaemonFinishedDeregistrationStop/KErrCancel
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrCancel, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopAuthoritativeStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerClosed
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopAuthoritativeStep - Failed, error waiting for KLinkLayerClosed/KErrServerTerminated [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}			
		
	// clean-up
	UnInitializeConnection();

	// verify that the daemon did not deregister
	if (DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterStopAuthoritativeStep - It would appear that the daemon erroneously deregistered."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterStopAuthoritativeStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlDeregisterStopAuthoritativeStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlDeregisterStopNormalStep::RunTestStepL()
/**
 * Tests a successful deregistration while simultaneously issuing an 
 * RConnection::Stop. The deregistration is expected to still succeed. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterStopNormalStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterStopNormalStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
		
	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopNormalStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// stop the connection
	iConnection.Stop(RConnection::EStopNormal);
		
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopNormalStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerClosed
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterStopNormalStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlRegOKStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterStopNormalStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlDeregisterStopNormalStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlDeregisterDormantStep::RunTestStepL()
/**
 * Tests the dormant mode processing. The dormant mode is expected to be entered and exited as expected.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterDormantStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();
		
	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterDormantStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 

	// set the unregistration action - preserve
	UpdateDaemonBehaviourFieldL(KOnDeregisterOKPreserve);
		
	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterDormantStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
			
	// wait for KConfigDaemonFinishedDeregistrationPreserve
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationPreserve, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterDormantStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationPreserve [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// the system is now in dormant mode - wait for it to exit
	rc = WaitForProgressStageL(KConfigDaemonFinishedDormantMode, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterDormantStep - Failed, error waiting for KConfigDaemonFinishedDormantMode [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// set the unregistration action - stop
	UpdateDaemonBehaviourFieldL(KOnDeregisterOKStop);
					
	// the system should be now in link-up state - wait for the idle timer to time out
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterDormantStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterDormantStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterDormantStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlDeregisterDormantStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlDeregisterDormantStep - End"));			
	return TestStepResult();	
	}	
	
TVerdict CConnectionControlCsDaemonProgressStep::RunTestStepL()
/**
 * Tests the progress notifications generated by CS_Daemon.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlCsDaemonProgressStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// start the connection 
	TInt rc = InitializeConnectionL();	
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// wait for KConfigDaemonLoading
	rc = WaitForProgressStageL(KConfigDaemonLoading, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonLoading [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KConfigDaemonLoaded
	rc = WaitForProgressStageL(KConfigDaemonLoaded, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonLoaded [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		
		
	// wait for KConfigDaemonStartingRegistration
	rc = WaitForProgressStageL(KConfigDaemonStartingRegistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonStartingRegistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		

	// wait for KConfigDaemonFinishedRegistration
	rc = WaitForProgressStageL(KConfigDaemonFinishedRegistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonFinishedRegistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		
	
	// wait for KLinkLayerOpen
	rc = WaitForProgressStageL(KLinkLayerOpen, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KLinkLayerOpen [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		
	 
	// set the unregistration action - stop
	UpdateDaemonBehaviourFieldL(KOnDeregisterOKStop);
	
	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iLastSocketClosedTimeout + KLastSocketClosedTimeoutDelta);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		
		
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		
	 
	// wait for KConfigDaemonUnloading
	rc = WaitForProgressStageL(KConfigDaemonUnloading, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonUnloading [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}					
		
	// wait for KConfigDaemonUnloaded
	rc = WaitForProgressStageL(KConfigDaemonUnloaded, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KConfigDaemonUnloaded [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}		

	// wait for KLinkLayerClosed/KErrTimedOut
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlCsDaemonProgressStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}		
		
	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlCsDaemonProgressStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	INFO_PRINTF1(_L("CConnectionControlCsDaemonProgressStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlCsDaemonProgressStep - End"));			
	return TestStepResult();	
	}	
		
TVerdict CConnectionControlProgressStep::RunTestStepL()
/**
 * Tests the progress notification mechanism.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlProgressStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// start the connection 
	TInt rc = InitializeConnectionL();	
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlProgressStep - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// wait for KConfigDaemonCompletedRegistration
	rc = WaitForProgressStageL(KConfigDaemonCompletedRegistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF3(_L("CConnectionControlProgressStep - Failed, error waiting for KConfigDaemonCompletedRegistration[%d], Error [%d]"), KConfigDaemonCompletedRegistration, rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerOpen to finish off
	rc = WaitForProgressStageL(KLinkLayerOpen, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlProgressStep - Failed, error waiting for KLinkLayerOpen [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}

	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlProgressStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlProgressStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlProgressStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlLinkUpStopAuthoritativeStep::RunTestStepL()
/**
 * Tests issuing an RConnection::Stop while in Link Up state. 
 * The deregistration state is expected to be reached.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopAuthoritativeStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlLinkUpStopAuthoritativeStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
	
	// the system is now in Link Up state 
	// set the unregistration action - stop
	UpdateDaemonBehaviourFieldL(KOnDeregisterOKStop);
			
	// stop the connection
	iConnection.Stop(RConnection::EStopAuthoritative);
		
	// wait for KLinkLayerClosed
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrConnectionTerminated, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlLinkUpStopAuthoritativeStep - Failed, error waiting for KLinkLayerClosed [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			

	// clean-up
	UnInitializeConnection();

	// verify that the daemon did not deregister
	if (DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlLinkUpStopAuthoritativeStep - It would appear that the daemon erroneously deregistered."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopAuthoritativeStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopAuthoritativeStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlLinkUpStopNormalStep::RunTestStepL()
/**
 * Tests issuing an RConnection::Stop while in Link Up state. 
 * The deregistration state is expected to be reached.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
	
	// the system is now in Link Up state 
	// set the unregistration action - stop
	UpdateDaemonBehaviourFieldL(KOnDeregisterOKStop);
			
	// stop the connection
	iConnection.Stop(RConnection::EStopNormal);
		
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlLinkUpStopNormalStep - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			

	// clean-up
	UnInitializeConnection();

	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlIoctlStep::RunTestStepL()
/**
 * Tests the Ioctl support.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlIoctlStep - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlIoctlStep - Failed to register"));
		SetTestStepResult(EFail);					
		return TestStepResult();					
		} 
	
	// create and initialize a buffer
	TSampleDaemonParameter1 ioctlParam;
	ioctlParam.iSomeValue = 66;
	ioctlParam.iSomeBuf.Copy(_L8("fish"));

	TSampleDaemonParameter1Pckg pckg(ioctlParam);	
	
	// pass the buffer to the server and check the result
	TRequestStatus status;	
	iConnection.Ioctl(KCOLConfiguration, KSampleDaemonOptionName1, status, (TDes8*)&pckg);
	User::WaitForRequest(status);
	
	if (!(ioctlParam.iSomeValue == 67 && ioctlParam.iSomeBuf.Compare(_L8("hello")) == 0)) 
		{
		INFO_PRINTF1(_L("CConnectionControlIoctlStep - Unexpected result back from Ioctl call."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionControlLinkUpStopNormalStep - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionTimeoutDuringRegistration::RunTestStepL()
/**
 * Tests idle timer timeout during registration.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CCConnectionTimeoutDuringRegistration - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// start the connection 
	TInt rc = InitializeConnectionL();
	if (!(rc == KErrTimedOut || rc == KErrNone))
		{
		INFO_PRINTF2(_L("CConnectionTimeoutDuringRegistration - Expected InitializeConnectionL to fail with KErrTimedOut but got [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionTimeoutDuringRegistration - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionTimeoutDuringRegistration - End"));			
	return TestStepResult();
	}	

TVerdict CConnectionTimeoutDuringDeregistration::RunTestStepL()
/**
 * Tests idle timer timeou during deregistration.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionTimeoutDuringDeregistration - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();	
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionTimeoutDuringDeregistration - Failed"));
		SetTestStepResult(EFail);
		return TestStepResult(); // We must return or else the step panics.					
		} 
	else
		{		
		INFO_PRINTF1(_L("CConnectionTimeoutDuringDeregistration - Passed"));
		SetTestStepResult(EPass);
		}		

	// tell the daemon to not disable timers on deregistration
	UpdateDaemonBehaviourFieldL(KDoNotDisableTimers);

	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionTimeoutDuringDeregistration - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			

	// wait for KLinkLayerClosed/KErrTimedOut
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionTimeoutDuringDeregistration - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionTimeoutDuringDeregistration - Passed"));
	SetTestStepResult(EPass);
		
	INFO_PRINTF1(_L("CConnectionTimeoutDuringDeregistration - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlSimulateInterPDSNHandoff::RunTestStepL()
/**
 * Tests the notification of the start and end of PPP renegotiation to 
 * Nifman and to the daemon. 
 *
 * Note: This test is expected to succeed only in debug builds as 
 * the test hook in PPP to simulate inter-PDSN handoff is only compiled-in 
 * in debug builds. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

#ifndef _DEBUG
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - This test can't be run in release builds as RConnection::Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP) is only supported in debug builds. Failing test purposely."));
	SetTestStepResult(EFail);
#else
	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();	
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - Failed"));
		SetTestStepResult(EFail);					
		return TestStepResult();	
		} 

	// force PPP renegotiation - this will only work in
	// debug builds as the hook in Nifman to pass it on
	// to PPP (and the changes in PPP) are only built in 
	// debug builds.
	TBuf8<1> dummyBuf;
	rc = iConnection.Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP, dummyBuf);
	if (rc != KErrNone) 
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - Unexpected result back from RConnection::Control call."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// make sure the daemon gets the requests - we can tell
	// because it posts the following two non-standard
	// progress notifications
	
	// wait for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerDownNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoff - Failed, error waiting for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			
	
	// wait for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerUpNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoff - Failed, error waiting for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			

	// wait for things to shut down normally

	// wait for KConfigDaemonStartingDeregistration
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoff - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}		
			
	// wait for KConfigDaemonFinishedDeregistrationStop
	rc = WaitForProgressStageL(KConfigDaemonFinishedDeregistrationStop, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoff - Failed, error waiting for KConfigDaemonFinishedDeregistrationStop [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			

	// wait for KLinkLayerClosed/KErrTimedOut
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoff - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - Passed"));
	SetTestStepResult(EPass);
#endif
		
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoff - End"));			
	return TestStepResult();	
	}	
	
TVerdict CConnectionControlSimulateInterPDSNHandoffNullDaemon::RunTestStepL()
/**
 * Tests that the start and end notification of the start and end 
 * of PPP renegotiation to Nifman and to the daemon do not affect 
 * the NULL daemon.
 *
 * Note: This test is expected to succeed only in debug builds as 
 * the test hook in PPP to simulate inter-PDSN handoff is only compiled-in 
 * in debug builds. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

#ifndef _DEBUG
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - This test can't be run in release builds as RConnection::Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP) is only supported in debug builds. Failing test purposely."));
	SetTestStepResult(EFail);
#else
	// start the connection 
	TInt rc = InitializeConnectionL();
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// force PPP renegotiation - this will only work in
	// debug builds as the hook in Nifman to pass it on
	// to PPP (and the changes in PPP) are only built in 
	// debug builds.
	TBuf8<1> dummyBuf;
	rc = iConnection.Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP, dummyBuf);
	if (rc != KErrNone) 
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - Unexpected result back from RConnection::Control call."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// note: there is no way to check programmatically that 
	// the nuil daemon processes these requests, but
	// basically if CConnectionControlSimulateInterPDSNHandoff
	// passes, we know they get passed-off to something and,
	// in this case, that would be the null daemon.

	// wait for things to shut down normally
	
	// wait for KLinkLayerClosed/KErrTimedOut
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - Passed"));
	SetTestStepResult(EPass);
#endif
		
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffNullDaemon - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlSimulateInterPDSNHandoffDuringRegistration::RunTestStepL()
/**
 * Tests the handling of a handoff during registration.
 *
 * Note: This test is expected to succeed only in debug builds as 
 * the test hook in PPP to simulate inter-PDSN handoff is only compiled-in 
 * in debug builds. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

#ifndef _DEBUG
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - This test can't be run in release builds as RConnection::Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP) is only supported in debug builds. Failing test purposely."));
	SetTestStepResult(EFail);
#else
	// start the connection 
	TInt rc = InitializeConnectionL();
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// make sure the daemon gets the requests - we can tell
	// because it posts the following two non-standard
	// progress notifications
	
	// wait for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerDownNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Failed, error waiting for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			
	
	// wait for KLinkLayer + 1/KErrNone
	rc = WaitForProgressStageL(KLinkLayerUpNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Failed, error waiting for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			

	// wait for things to shut down normally

	// wait for KLinkLayerClosed/KErrTimedOut
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - Passed"));
	SetTestStepResult(EPass);
#endif
		
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringRegistration - End"));			
	return TestStepResult();	
	}	

TVerdict CConnectionControlSimulateInterPDSNHandoffDuringDeregistration::RunTestStepL()
/**
 * Tests the handling of a handoff during deregistration.
 *
 * Note: This test is expected to succeed only in debug builds as 
 * the test hook in PPP to simulate inter-PDSN handoff is only compiled-in 
 * in debug builds. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

#ifndef _DEBUG
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - This test can't be run in release builds as RConnection::Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP) is only supported in debug builds. Failing test purposely."));
	SetTestStepResult(EFail);
#else
	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();	
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Failed"));
		SetTestStepResult(EFail);					
		return TestStepResult();	
		} 

	// wait for things to shut down normally (wait for 
	// KConfigDaemonStartingDeregistration)
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Failed, error waiting for KConfigDaemonStartingDeregistration [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}		
			
	// force PPP renegotiation - this will only work in
	// debug builds as the hook in Nifman to pass it on
	// to PPP (and the changes in PPP) are only built in 
	// debug builds.
	TBuf8<1> dummyBuf;
	rc = iConnection.Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP, dummyBuf);
	if (rc != KErrNone) 
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Unexpected result back from RConnection::Control call."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// make sure the daemon gets the requests - we can tell
	// because it posts the following two non-standard
	// progress notifications
	
	// wait for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerDownNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Failed, error waiting for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			
	
	// wait for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerUpNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Failed, error waiting for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			

	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - Passed"));
	SetTestStepResult(EPass);
#endif
		
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringDeregistration - End"));			
	return TestStepResult();	
	}	
	
TVerdict CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest::RunTestStepL()
/**
 * Tests the handling of a handoff during an ioctl request.
 *
 * Note: This test is expected to succeed only in debug builds as 
 * the test hook in PPP to simulate inter-PDSN handoff is only compiled-in 
 * in debug builds. 
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Start"));		
	if (TestStepResult() != EPass)
		return TestStepResult();

#ifndef _DEBUG
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - This test can't be run in release builds as RConnection::Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP) is only supported in debug builds. Failing test purposely."));
	SetTestStepResult(EFail);
#else
	// daemon registration
	TInt rc = DoSuccessfulRegistrationL();	
	if (rc != KErrNone)
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Failed"));
		SetTestStepResult(EFail);
		return TestStepResult();					
		} 

	// issue an ioctl request
	TSampleDaemonParameter1 ioctlParam;
	ioctlParam.iSomeValue = 66;
	ioctlParam.iSomeBuf.Copy(_L8("fish"));
	TSampleDaemonParameter1Pckg pckg(ioctlParam);		
	TRequestStatus status;	
	iConnection.Ioctl(KCOLConfiguration, KSampleDaemonOptionName1, status, (TDes8*)&pckg);
				
	// force PPP renegotiation - this will only work in
	// debug builds as the hook in Nifman to pass it on
	// to PPP (and the changes in PPP) are only built in 
	// debug builds.
	TBuf8<1> dummyBuf;
	rc = iConnection.Control(KCOLLinkLayerTestLevel, KSoIfForceRenegotiatePPP, dummyBuf);
	if (rc != KErrNone) 
		{
		INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Unexpected result back from RConnection::Control call."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// we wait for this AFTER we forced the PPP renegotiation
	// because WaitForProgressStageL will screw-up with this
	// oustanding request. 	
	User::WaitForRequest(status);

	// make sure the daemon gets the requests - we can tell
	// because it posts the following two non-standard
	// progress notifications
	
	// wait for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerDownNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Failed, error waiting for KLinkLayerDownNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();	
		}			
	
	// wait for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone
	rc = WaitForProgressStageL(KLinkLayerUpNotificationReceivedByConfigDaemon, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Failed, error waiting for KLinkLayerUpNotificationReceivedByConfigDaemon/KErrNone [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();						
		}			
		
	// clean-up
	UnInitializeConnection();

	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - Passed"));
	SetTestStepResult(EPass);
#endif
		
	INFO_PRINTF1(_L("CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest - End"));			
	return TestStepResult();
	}
	

/**
 * Tests the correctness of Deregistration Cause parameter passing to Daemon by Nifman
 * on expiry of Short Idle Timer.
 * 
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
TVerdict CConnectionControlDeregisterCauseTimerShortStep::RunTestStepL()
	{	
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerShortStep - Start"));		
	
	//
	// This test step's logic is different from the others', because we need to close all sessions with the interface,
	// but we still need to receive progress notifications.
	// So, 1) we create a "data" session with the interface, which results in Daemon activity.
	//     2) we create a "monitor" session, which we use for testing for correct Progress Notifications. 
	//
	
	//
	// Create the "data" session: we can't reuse the default session, because it cannot be reset to be just a "monitor" later.
	// 
	
	TInt rc = iEsock.Connect();
	if(KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerShortStep - Failed to Connect to Esock. Error[%d]"),rc);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
		
	RConnection dataConn;
	rc =  StartConnection(dataConn);
	if(KErrNone != rc)
		{
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	CleanupClosePushL(dataConn);
	
	// We don't check for successful registration, etc: 
	// 1) This functionality is tested in other test steps.
	// 1) It is not relevant to the test - we just want to see what Nifman does on Idle timer expiry.
	// 2) If the Nifman / Daemon have malfunctioned - we wouldn't be able to deregister correctly and the test would fail. 
	
	//
	// Attach the iConnection as monitor: We just need it for ProgressNotifications. 
	// We must attach it as monitor not to affect Nifman Idle Timers.
	//
	rc = AttachToConn(dataConn, iConnection, ETrue);
	if(KErrNone != rc)
		{
		SetTestStepResult(EFail);
		
		dataConn.Stop();
		CleanupStack::Pop(&dataConn);
		return TestStepResult();
		}
		
	//
	// Make Nifman switch to Short Idle timer.
	//
	dataConn.Close();
	CleanupStack::Pop(&dataConn);
			
	
	//At this point we have no RConnection and no socket objects: Nifman should switch to Short timer mode (last session closed)
	// Wait for Nifman's medium timer to expire and to ask the Daemon to deregister:
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistrationTimerShort, KErrNone, (iLastSessionClosedTimeout + iProgressNotificationTimeout));
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerShortStep - Failed, error waiting for KConfigDaemonStartingDeregistrationTimerShort [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}
	
	// At this point the Daemon is deregistering. Once finished, it instructs Nifman to Close the Nif.
	
	// wait for KLinkLayerClosed/KErrTimedOut 
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerShortStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerShortStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// Cleanup
	UnInitializeConnection();
	
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerShortStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerShortStep - End"));			
	return TestStepResult();	
	}	



TVerdict CConnectionControlDeregisterCauseTimerMediumStep::RunTestStepL()
/**
 * Tests the correctness of Deregistration Cause parameter passing to Daemon by Nifman
 * on expiry of Medium Idle Timer.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerMediumStep - Start"));		
	
	// Bring up the interface
	TInt rc = InitializeConnectionL();
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerMediumStep - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// wait for KConfigDaemonCompletedRegistration
	rc = WaitForProgressStageL(KConfigDaemonCompletedRegistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF3(_L("CConnectionControlDeregisterCauseTimerMediumStep - Failed, error waiting for KConfigDaemonCompletedRegistration[%d], Error [%d]"), KConfigDaemonCompletedRegistration, rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerOpen to finish off
	rc = WaitForProgressStageL(KLinkLayerOpen, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerMediumStep - Failed, error waiting for KLinkLayerOpen [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}
	
	//At this point we have only RConnection object: Nifman should switch to Medium timer mode (last socket closed)
	// Do nothing: Wait for Nifman's medium timer to expire and to ask the Daemon to deregister:
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistrationTimerMedium, KErrNone, (iLastSocketClosedTimeout + iProgressNotificationTimeout));
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerMediumStep - Failed, error waiting for KConfigDaemonStartingDeregistrationTimerMedium [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}
	
	// At this point the Daemon is deregistering. Once finished, it instructs Nifman to Close the Nif.
	
	// wait for KLinkLayerClosed/KErrTimedOut 
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerMediumStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerMediumStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}

	// Cleanup
	UnInitializeConnection();
	
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerMediumStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerMediumStep - End"));			
	return TestStepResult();	
	}
	
TVerdict CConnectionControlDeregisterCauseTimerLongStep::RunTestStepL()
/**
 * Tests the correctness of Deregistration Cause parameter passing to Daemon by Nifman
 * on expiry of Long Idle Timer.
 *
 * @internalComponent
 *
 * @return TVerdict code
 * @leave If any of the called methods leaves.
 */
	{
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerLongStep - Start"));		
	// start the connection
	TInt rc = InitializeConnectionL();
	if (KErrNone != rc)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed to initialize connection. The error was: [%d]."), rc);				
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// Make sure the Daemon registered OK
	// wait for KConfigDaemonCompletedRegistration
	rc = WaitForProgressStageL(KConfigDaemonCompletedRegistration, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF3(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed, error waiting for KConfigDaemonCompletedRegistration[%d], Error [%d]"), KConfigDaemonCompletedRegistration, rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
		
	// wait for KLinkLayerOpen to finish off
	rc = WaitForProgressStageL(KLinkLayerOpen, KErrNone, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed, error waiting for KLinkLayerOpen [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();							
		}
	
	//
	// Make Nifman switch to Long timer: carry out activity on protocol SAP.
	//
	
	RSocket socket;
	rc = socket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed to open socket [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();
		}
	CleanupClosePushL(socket);
	
	// Send data to peer: first get our default gateway address
	TSoInetInterfaceInfo info;
	rc = GetInterfaceInfoL(iInterfaceName, info);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed to get the interface info [%d]"), rc);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket);
		return TestStepResult();
		}
	
	TInetAddr peerUdpDiscardAddr = info.iDefGate;
	if(peerUdpDiscardAddr.IsUnspecified()) // Something was wrong in PPP negotiation?
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed: Peer Address Is Unspecified."));
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket);
		return TestStepResult();
		}
	
	// We don't care if the port is actually open (we may get ICMP error later) - we just need nifman to think
	// that there is socket activity. As long as the packet leaves our interface, we are OK.
	static const TUint KUdpDiscardPort(9); // We want our Send to have the least effect on other machines on the network.
	peerUdpDiscardAddr.SetPort(KUdpDiscardPort);
		
	static const TUint KSendFlagsNone(0);
	static const TInt KDataBufLen(4);
	_LIT8(KData, "data");
	TBufC8<KDataBufLen> dataBuf(KData);
	
	TRequestStatus sendRequest;	
	socket.SendTo(dataBuf, peerUdpDiscardAddr, KSendFlagsNone, sendRequest);
	User::WaitForRequest(sendRequest);	
	if(KErrNone != sendRequest.Int()) // There is no point in proceeding, because likely we haven't reset Nifman Timer.
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Socked Send Failed,[%d]"), sendRequest.Int());
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket);
		return TestStepResult();
		}
	// If the Send has succeeded, there was Packet Activity, and Nifman's idle timer was reset to Long mode.
	
	// Now we don't do anything, and wait for Nifman's long timer to expire and Nifman to ask the Daemon to deregister:
	rc = WaitForProgressStageL(KConfigDaemonStartingDeregistrationTimerLong, KErrNone, (iLastActivityTimeout + iProgressNotificationTimeout));
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed, error waiting for KConfigDaemonStartingDeregistrationTimerLong [%d]"), rc);
		SetTestStepResult(EFail);
		CleanupStack::PopAndDestroy(&socket);
		return TestStepResult();							
		}	
	CleanupStack::PopAndDestroy(&socket);
	
	
	// At this point the Daemon is deregistering. Once finished, it instructs Nifman to Close the Nif.
	// wait for KLinkLayerClosed/KErrTimedOut 
	rc = WaitForProgressStageL(KLinkLayerClosed, KErrTimedOut, iProgressNotificationTimeout);
	if (rc != KErrNone)
		{
		INFO_PRINTF2(_L("CConnectionControlDeregisterCauseTimerLongStep - Failed, error waiting for KLinkLayerClosed/KErrTimedOut [%d]"), rc);
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	// verify that the daemon deregistered
	if (!DaemonSuccessfullyDeregisteredL())
		{
		INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerLongStep - The daemon failed to complete deregistration."));
		SetTestStepResult(EFail);
		return TestStepResult();					
		}
	
	// Cleanup
	UnInitializeConnection();
	
	
	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerLongStep - Passed"));
	SetTestStepResult(EPass);

	INFO_PRINTF1(_L("CConnectionControlDeregisterCauseTimerLongStep - End"));			
	return TestStepResult();	
	}	
	


