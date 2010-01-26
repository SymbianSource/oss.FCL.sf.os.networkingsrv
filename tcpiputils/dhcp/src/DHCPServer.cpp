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
// Implements a Symbian OS server that exposes the RCDHCP API
// 
//

/**
 @file
 @internalTechnology
*/

#include <e32svr.h>
#include <e32base.h>
#include <e32std.h>
#include "DHCP_Std.h"
#include "DHCPServer.h"
#include "DHCPSess.h"
#include "DHCPIP4Msg.h"
#ifdef SYMBIAN_NETWORKING_PLATSEC
#include <ecom/ecom.h>
#include "DHCPPolicy.h"
#endif

#ifdef _DEBUG
#include <e32property.h>
#endif

/**
 *  Holds current debug options of the server
 *  @internalComponent
 */
TInt CDHCPServer::iDebugFlags(0);

CDHCPServer::~CDHCPServer()
/**
 *
 * Destructor
 *
 * @internalComponent
 */
	{
	__CFLOG_CLOSE;
	__CFLOG_DELETE;
	iEsock.Close();
	delete iTimer;
	}

#ifdef SYMBIAN_NETWORKING_PLATSEC
CDHCPServer::CDHCPServer(): CPolicyServer(EPriorityStandard,Policy,ESharableSessions)
#elif defined(EKA2)
CDHCPServer::CDHCPServer(): CServer2(EPriorityStandard)
#else
//On  EKA1, session is open in one thread and used in another
CDHCPServer::CDHCPServer(): CServer2(EPriorityStandard, ESharableSessions)
#endif
/**
 * The CDHCPServer::CDHCPServer method
 *
 * Constructor
 *
 * @internalComponent
 */
	{
	}
	
CDHCPServer* CDHCPServer::NewL()
/**
 * Creates and start the CServer2 derived server.
 * Called inside the MainL() function.
 *
 * @return - Instance of the server
 */
	{
	CDHCPServer* server = new (ELeave) CDHCPServer();
	CleanupStack::PushL(server);
	server->ConstructL();
	// CServer2 base class call
	server->StartL(KDHCPServerName);
	CleanupStack::Pop(server);
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPServer::NewL")));
	return server;
	}

void CDHCPServer::ConstructL()
/**
  * Second stage construction
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_CREATEL;
	__CFLOG_OPEN;
	User::LeaveIfError(iEsock.Connect());
#ifdef _DEBUG
	DebugFlags() = 0;
#endif
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPServer::ConstructL - connected to ESock")));
	}


CSession2* CDHCPServer::NewSessionL(const TVersion& /*aVersion*/, const RMessage2& /*aMessage*/) const
/**
 *
 * Create a new If on this server
 *
 * @param	&aVersion	Vesion of server
 * @return	A new CDHCP session to be used for the connection
 *
 * @internalTechnology
 */
	{
//	TVersion v(KDHCPSrvMajorVersionNumber,KDHCPSrvMinorVersionNumber,KDHCPSrvBuildVersionNumber);

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPServer::NewSessionL - creating new session...")));

	//	if (!User::QueryVersionSupported(v, aVersion))
//		User::Leave(KErrNotSupported);
	CDHCPSession* ses = CDHCPSession::NewL();
	SessionConnected();
	return ses;
	}
	
void CDHCPServer::SessionConnected() const
/**
  * Called by session to alert server
  * that something has connected so
  * cancel the shutdown timer if there
  * is one running
  *
  */
  	{
	if (iTimer)
		{
		iTimer->Cancel();
		}  	
  	}
	
void CDHCPServer::Close(CDHCPSession* /*aDHCPSession*/)
/**
  * Reference counting for the server.
  * When there are no sessions open then
  * we shutdown, unless the only one open
  * is oursleves, then we obviously still shutdown...
  *
  * @internalTechnology
  */
	{
	iSessionIter.SetToFirst();
	
   	TInt count = 0;
   	while (iSessionIter++!=NULL)
   		{
   		count++;
   		}
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPServer::Close - Current Session Count: (%d)") ,count));
		         
		         
	// if we have one session then it is the current one
	// and ok to shutdown...as this function is called
	// before the session is destroyed...
	if (count==1)
		{
		if (!iTimer)
			{
			TRAPD(ret, iTimer = CExpireTimer::NewL());
			if (ret!=KErrNone)
				{
				// OOM so why not just shut down to free some
				// seeing as that's the process that's been set
				// in motion anyway...
				TimerExpired();
				return;
				}
			}			
		// wait for 10 seconds before shutting down completely...
		// in case anyone wants to connect			
		iTimer->After(TTimeIntervalSeconds(KOneSecond/*10*/), *this);
		}	
	REComSession::FinalClose();
	}
	
void CDHCPServer::TimerExpired()
/**
  * CExpireTimer interface function to  alert server timer has popped
  *
  */
  	{
  	// just stop the server if we get here....
	CActiveScheduler::Stop();
  	}

TInt& CDHCPServer::DebugFlags()
/**
  * Provides acess to debug flags
  */
	{
	return iDebugFlags;
	}

LOCAL_C void MainL()
/**
 * Do the funky stuff, and use Rendezvous to signal completion...
 */
	{
	// Leave the hooks in for platform security
#if (defined __DATA_CAGING__)
	RProcess().DataCaging(RProcess::EDataCagingOn);
	RProcess().SecureApi(RProcess::ESecureApiOn);
#endif

#if defined(__WINS__) && !defined(EKA2) //WINS emulator
	User::LeaveIfError(RThread().Rename(KDHCPProcessName));
#elif !defined(EKA2)
	User::LeaveIfError(RProcess().Rename(KDHCPProcessName));
#else //WINCW emulator or any target
	User::LeaveIfError(RProcess::RenameMe(KDHCPProcessName));
#endif

#ifdef _DEBUG
	TInt err = RProperty::Define(KMyPropertyCat, KMyPropertyDestPortv4, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
				TSecurityPolicy(ECapabilityWriteDeviceData));
	if (err == KErrNone)
		{
		// not defined by test code
		RProperty::Define(KMyPropertyCat, KMyPropertyDestPortv6, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
					TSecurityPolicy(ECapabilityWriteDeviceData));
		RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv4, 67);	// set to default values
		RProperty::Set(KMyPropertyCat, KMyPropertyDestPortv6, 547);	// set to default values
		RProperty::Define(KMyPropertyCat, KMyDefaultLeaseTime, RProperty::EInt, TSecurityPolicy(TSecurityPolicy::EAlwaysPass),
					TSecurityPolicy(ECapabilityWriteDeviceData));
		RProperty::Set(KMyPropertyCat, KMyDefaultLeaseTime, 21600);	// Define and set default values for server lease time
		}
#endif

	CActiveScheduler* pScheduler = new(ELeave)CActiveScheduler;
	CActiveScheduler::Install(pScheduler);
	CleanupStack::PushL(pScheduler);
	CDHCPServer* pServer = CDHCPServer::NewL();
	CleanupStack::PushL(pServer);

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("MainL - about to start activescheduler")));
	// Sync with the starter and enter the active scheduler
#if defined(__WINS__) && !defined(EKA2)
	RThread::Rendezvous(KErrNone); //WINS emulator
#else	//WINSCW emulator or any target
	RProcess::Rendezvous(KErrNone);
#endif
	CActiveScheduler::Start();
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("MainL - activescheduler has stopped...")));
	CleanupStack::PopAndDestroy(pServer);
	CleanupStack::PopAndDestroy(pScheduler);
	}


EXPORT_C TInt E32Main()
/**
 * The main thread function, the Ordinal 1!
 * @return - Standard Epoc error code on exit
 */
	{	
__UHEAP_MARK;
	CTrapCleanup* cleanup = CTrapCleanup::New();
	if (cleanup == NULL)
		{
		return KErrNoMemory;
		}
	TInt err = KErrNone;
	TRAP(err,MainL());
	delete cleanup;
__UHEAP_MARKEND;
	return err;
    }
    
#if !defined(EKA2)
GLDEF_C TInt E32Dll(enum TDllReason)
	{
	return 0;
	}
#endif
