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
* Defines the test steps for the connection control mechanism.
* 
*
*/



/**
 @file
 @internalComponent
*/

#ifndef __CONNECTION_CONTROL_STEP_H__
#define __CONNECTION_CONTROL_STEP_H__


#include <testexecutestepbase.h>
#include <commdb.h>
#include <es_sock.h>
#include <es_enum.h>
#include <in_sock.h>
#include <metadatabase.h>
#include <commsdattypesv1_1.h>
using namespace CommsDat;

class MProgressDestination
/**
 * Notification interface for the CConnectionControlProgress class.
 *
 * @internalComponent
 */
	{
public:
	virtual void DoOnProgressNotification(TInt aStage, TInt aError) = 0;
	};


class CConnectionControlProgress : public CActive
/**
 * Progress active object class. Issues an ProgressNotification call on a given connection
 * and reports the callbacks to the notification destination.
 *
 * @internalComponent
 */
	{
public:
	CConnectionControlProgress();
	~CConnectionControlProgress();

	void ProgressNotification(MProgressDestination* aProgressDestination, RConnection* aConnection);

	void DoCancel();
	void RunL();
	
protected:
private:
	TNifProgressBuf iProgressBuf;
	RConnection* iConnection;
	MProgressDestination* iDestination;
	}; 


class MAllInterfaceDestination
/**
 * Notification interface for the CConnectionControlAllInterface class.
 *
 * @internalComponent
 */
	{
public:
	virtual void DoOnAllInterfaceNotification(TInt aIapId, TConnInterfaceState aState, TInt aError) = 0;
	};


class CConnectionControlAllInterface : public CActive
/**
 * All interface active object class. Issues an AllInterfaceNotification call on a given connection
 * and reports the callbacks to the notification destination.
 *
 * @internalComponent
 */
	{
public:
	CConnectionControlAllInterface();
	~CConnectionControlAllInterface();

	void AllInterfaceNotification(MAllInterfaceDestination* aDestination, RConnection* aConnection);

	void DoCancel();
	void RunL();
	
protected:
private:
	TInterfaceNotificationBuf iAllInterfaceBuf;
	RConnection* iConnection;
	MAllInterfaceDestination* iDestination;
	}; 


class MTimerDestination
/**
 * Notification interface for the CConnectionControlTimer class.
 *
 * @internalComponent
 */
	{
public:
	virtual void DoOnTimerExpired(TInt aError) = 0;
	};


class CConnectionControlTimer : public CTimer
/**
 * Timer active object class. Issues an After timer call and reports the callbacks to the 
 * notification destination.
 *
 * @internalComponent
 */
	{
public:
	static CConnectionControlTimer* NewL(MTimerDestination* aTimerDestination);
	CConnectionControlTimer(MTimerDestination* aTimerDestination);
	~CConnectionControlTimer();

	void RunL();
	
protected:
private:
	MTimerDestination* iTimerDestination;
	}; 


class CConnectionControlStep : public CTestStep, public MProgressDestination, public MAllInterfaceDestination, public MTimerDestination
/**
 * Base class for all connection control tests.
 *
 * @internalComponent
 */
	{
public:
	// standard test step methods
	CConnectionControlStep();
	~CConnectionControlStep();
	void ConstructL();
	TVerdict doTestStepPreambleL();
	enum TVerdict doTestStepL();	
/**
 * Should contain the actual test step code. 
 *
 * @pre		The configuration is read and the CommDb views are initialized
 * @leave 	In case of abnormal situations. Any expected leaves shopuld be trapped
 *			and a fail verdict code should be returned
 */
	virtual TVerdict RunTestStepL() = 0;
	void DoOnProgressNotification(TInt aStage, TInt aError);
	void DoOnAllInterfaceNotification(TInt aIapId, TConnInterfaceState aState, TInt aError);
	void DoOnTimerExpired(TInt aError);
	
protected:
	// database utilities
	void GetConfigurationL();
	void OpenDbL();
	void CloseDb();
	void WriteTestConfigToDbL();
	void UpdateDaemonBehaviourFieldL(const TDesC& aDaemonBehaviour);
	TBool IpAddressMatchesExpectedRegistrationAddressL();
	TBool DaemonSuccessfullyDeregisteredL();
	
	// connection utilities
	TInt AttachToConn(RConnection& aConnSource, RConnection& aConnSink, TBool aAsMonitor = ETrue);
	TInt StartConnection(RConnection& aConn);
	TInt InitializeConnectionL();
	void UnInitializeConnection();
	
	// connection information utilities
	TInt GetConnectionInfo(TInt aIapId, TConnectionInfo& aInfo, TInt& aIndex);
	TInt GetConnectionInfo(TInt aIapId, TConnectionInfo& aInfo, TInt& aIndex,RConnection& aConn); 
	TInt GetInterfaceName(TInt aIapId, TName& aInterfaceName);	
	TInt GetInterfaceInfoL(const TName& aInterfaceName,	TSoInetInterfaceInfo& aInfo);
	TInt GetIpAddressL(const TName& aInterfaceName, TInetAddr& aIpAddress);
	
	// progress notification utilities
	TInt WaitForAnyNotificationL(TInt& aProgressStage, TInt& aProgressError, TConnInterfaceState& aInterfaceState, TInt& aInterfaceError, TInt aTimeout, TBool aProgressEnabled = ETrue, TBool aAllInterfaceEnabled = ETrue);	
	TInt WaitForProgressStageL(TInt aProgressStage, TInt aProgressError, TInt aTimeout);
	TInt DoSuccessfulRegistrationL();
	
	// test daemon configuration values
	/** Only one of the following should be set. Retrieved from the .ini file. */
	/** If ETrue, the test daemon will be configured to not disable timers. */
	TBool iDoNotDisableTimers;
	/** If ETrue, the test daemon will force PPP to renegotiate the link during registration. */
	TBool iForcePPPRenegotiation;
	/** If ETrue, the test daemon will be configured to successfully register and deregister. */
	TBool iOnRegisterOK;
	/** If ETrue, the test daemon will report an error on registration. */
	TBool iOnRegisterError;
	/** If ETrue, the test daemon will terminate on regsitration. */
	TBool iOnRegisterDie;
	/** If ETrue, the test daemon will hang on a regsitration. */
	TBool iOnRegisterHang;
	/** If ETrue, the test daemon will deregister normally and tell Nifman to tear down the interface. */
	TBool iOnDeregisterOKStop;
	/** If ETrue, the test daemon will deregister normally and tell Nifman to maintain the interface -> go into dormant mode. */
	TBool iOnDeregisterOKPreserve;
	/** If ETrue, the test daemon will report an error on registration. */
	TBool iOnDeregisterError;
	/** If ETrue, the test daemon will terminate on registration. */
	TBool iOnDeregisterDie;
	/** If ETrue, the test daemon will hang on registration. */
	TBool iOnDeregisterHang;
	/** If ETrue, the test daemon will report progress notification with deregistration cause */
	TBool iOnDeregisterReportCause;
	/** The IAP id to use. Read from the .ini file. */
	TInt iIapId;		
	/** The IP address expected to be set by the test configuration daemon. Read from the .ini file. */
	TInetAddr iExpectedRegIpAddress;
	/** The timeout to use when waiting for progress notifications. Read from the .ini file. */
	TInt iProgressNotificationTimeout;
	/** The last socket closed timeout to use. Read from the .ini file. */
	TInt iLastSocketClosedTimeout;
	/** The last activity timeout to use. Read from the .ini file. */
	TInt iLastActivityTimeout;	
	/** The Last Session closed timeout to use. Read from the .ini file. */
	TInt iLastSessionClosedTimeout;
	
	/** The name of the interface. Obtained dynamically. */
	TName iInterfaceName;

	// active objects
	/** Used to monitor progress notifications. */
	CConnectionControlProgress* iProgress;	
	/** Used to monitor interface notifications. */
	CConnectionControlAllInterface* iAllInterface;	
	/** Timer used to time-out when waiting for progress notifications. */
	CConnectionControlTimer* iTimer;	

	// ESOCK objects
	/** Handle to esock. */
	RSocketServ iEsock;
	/** The connection. */
	RConnection iConnection;

private:
	// CommsDat objects
	/** Handle to commsdat database */
	CMDBSession* iSession;
	/** Handle to the IAP record */
	CCDIAPRecord* iIap;
	/** Handle to the service record */
	CCDRecordBase* iService;
	/** Handle to the bearer record */
	CCDRecordBase* iBearer;
	
	// stae for the progress notification code
	/** ETrue if the timer expired when waiting for a progress notification. */
	TBool iTimerExpired;
	/** Expected progress stage. */
	TInt iExpectedProgressStage;
	/** Expected interface state. */
	TConnInterfaceState iExpectedInterfaceState;
	/** Progress error. */
	TInt iProgressError;

	/** Actual reached progress stage. */
	TInt iReachedProgressStage;
	/** Actual reached interface state. */
	TConnInterfaceState iReachedInterfaceState;
	/** Interface error if any. */
	TInt iInterfaceError;
	/** For time-out errors. */
	TInt iAsyncError;
	}; 


/** Used to define a literal variable. */
#define __TEST_LIT(name) _LIT(K##name, #name)

/** Define a literal from a classname and can be as the keyword class. */
#define __TEST_CLASS(className) __TEST_LIT(className); class className 

/** Define a basic declaration for testClass as a subclass of CConnectionControlStep 
and define a literal from testClass */
#define __DEFINE_CONNECTION_TEST_CLASS(testClass) __TEST_CLASS(testClass) : public CConnectionControlStep \
			{ \
		public: \
			inline testClass() {SetTestStepName(K##testClass);}; \
			virtual TVerdict RunTestStepL(); \
			}; 
			
			
/** Test null configuration daemon behaviour. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlNullDaemonTest)

/** Tests a successful registration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlRegOKStep)

/** Tests an un-successful registration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlRegErrorStep)

/** Tests a successful deregistration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterOKStep)

/** Tests an un-successful deregistration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterErrorStep)

/** Tests a successful deregistration with simultaneous connection attempts. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterConnectionStep)

/** Tests a successful deregistration with simultaneous RConnection::Stop(ENormalStop). */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterStopNormalStep)

/** Tests aborted deregistration with simultaneous RConnection::Stop(EAuthoritativeStop). */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterStopAuthoritativeStep)

/** Tests a successful deregistration with dormant mode. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterDormantStep)

/** Tests the CS_Daemon progress notifications. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlCsDaemonProgressStep)

/** Tests the progress notification mechanism. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlProgressStep)

/** Tests issuing RConnection::Stop(ENormalStop) while in Link Up state. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlLinkUpStopNormalStep)

/** Tests issuing RConnection::Stop(EAuthoritativeStop) while in Link Up state. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlLinkUpStopAuthoritativeStep)

/** Tests issuing RConnection::Ioctl() to the daemon. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlIoctlStep)

/** Tests idle timeout during registration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionTimeoutDuringRegistration)

/** Tests idle timeout during deregistration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionTimeoutDuringDeregistration)

/** Tests PPP renegotiation notifications - simulates PDSN handoff. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlSimulateInterPDSNHandoff)

/** Tests PPP renegotiation notifications - simulates PDSN handoff with null daemon. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlSimulateInterPDSNHandoffNullDaemon)

/** Tests PPP renegotiation notifications - simulates PDSN handoff during registration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlSimulateInterPDSNHandoffDuringRegistration)

/** Tests PPP renegotiation notifications - simulates PDSN handoff during deregistration. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlSimulateInterPDSNHandoffDuringDeregistration)

/** Tests PPP renegotiation notifications - simulates PDSN handoff during an Ioctl request. */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlSimulateInterPDSNHandoffDuringIoctlRequest)

/** Test Nifman Deregistration on Short idle timer expiry parameter passing to Daemon */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterCauseTimerShortStep)

/** Test Nifman Deregistration on Medium idle timer expiry parameter passing to Daemon */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterCauseTimerMediumStep)

/** Test Nifman Deregistration on Long idle timer expiry parameter passing to Daemon */
__DEFINE_CONNECTION_TEST_CLASS(CConnectionControlDeregisterCauseTimerLongStep)

#endif // __CONNECTION_CONTROL_STEP_H__

