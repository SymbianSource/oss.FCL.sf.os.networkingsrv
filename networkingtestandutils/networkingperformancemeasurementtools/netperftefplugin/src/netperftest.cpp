// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// netperfte specific test steps called from TEF script.
// 
//

/**
 @file
 @internalTechnology
*/
 
#include <stdlib.h>
#include <string.h>
#include <profiler.h>
#include <test/testexecutelog.h>
#include <in_sock.h>
#include "netperftest.h"
#include "netperfsender.h"
#include "netperfreceiver.h"
#include "cpusponge.h"

#include <commsdat.h>
#include <commdbconnpref.h>
#include <commsdattypesv1_1.h>
#include <metadatabase.h>


CIperfTestSetupReceiver::CIperfTestSetupReceiver(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestSetupReceiver);
	}

CIperfTestSetupReceiver::~CIperfTestSetupReceiver()
	{
	}


using namespace CommsDat;

TVerdict CIperfTestSetupReceiver::doTestStepL()
	{
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
	CMDBSession* iDb = CMDBSession::NewL(KCDVersion1_2);
#else
	CMDBSession* iDb = CMDBSession::NewL(KCDVersion1_1);
#endif
	CleanupStack::PushL(iDb);
	
	CMDBRecordSet<CCDIAPRecord> *iapRecordSet = new(ELeave) CMDBRecordSet<CCDIAPRecord>(KCDTIdIAPRecord);
	CleanupStack::PushL(iapRecordSet);

	iapRecordSet->LoadL(*iDb);
	
	INFO_PRINTF1(_L(" IAP#\tName"));
	
	//The table has been loaded
	TInt i(0);
	while (i < iapRecordSet->iRecords.Count())
		{
		CCDIAPRecord* rec = static_cast<CCDIAPRecord*>(iapRecordSet->iRecords[i]);
		const TDesC & a = rec->iRecordName;
		TInt id = rec->RecordId();
		INFO_PRINTF3(_L(" %d\t\t%S"),id,&a);
		i++;
		}
		
	CleanupStack::PopAndDestroy(iapRecordSet);
	CleanupStack::PopAndDestroy(iDb);
	
	TPtrC temp_string;

	// IAP,SNAP,ReceiveProtocol,SamplingPeriodInMilliseconds,PacketSizeInBytes,ReceivedPort,EchoReceived
	TInt iap = ReadIapL();
	TInt snap = ReadSnapL();
	TBool receiveTcp=EFalse;
	TInt testDuration_sec = ReadTestDurationL();
	TInt samplingPeriod_ms = ReadSamplingPeriodL();
	TInt packetSizeInBytes = ReadPacketSizeInBytesL();	
	TInt sendThroughputInKilobitsPerSecond=100;
	TInt receivePort=5001; // iperf default
	TBool echoReceived=EFalse;
	
	
	// IAP,SNAP,ReceiveProtocol,SamplingPeriodInMilliseconds,PacketSizeInBytes,ReceivedPort,EchoReceived
	GetStringFromConfig( ConfigSection(), _L("ReceiveProtocol"), temp_string );
	if (temp_string.Compare(_L("UDP"))==0)
		{
		INFO_PRINTF1(_L("Receiving in UDP mode.."));
		}
	else if (temp_string.Compare(_L("TCP"))==0)
		{
		receiveTcp=ETrue;
		INFO_PRINTF1(_L("Receiving in TCP mode.."));			
		}
	else
		{
		INFO_PRINTF1(_L("ReceiveProtocol must be TCP or UDP"));
		User::Leave(KErrArgument);
		}
	
	GetIntFromConfig( ConfigSection(), _L("ReceivePort"), receivePort);
	INFO_PRINTF2(_L("Listening on port %d"), receivePort);

	if (GetBoolFromConfig( ConfigSection(), _L("EchoReceivedData"), echoReceived) && echoReceived)
		{
		INFO_PRINTF1(_L("Will echo received data."));
		}
	else
		{
		INFO_PRINTF1(_L("Will discard received data."));
		}

	GetIntFromConfig( ConfigSection(), _L("SendThroughputInKilobitsPerSecond"), sendThroughputInKilobitsPerSecond);

	CIperfReceiver* ipr = new(ELeave)CIperfReceiver(*iServer, Logger());
	CleanupStack::PushL(ipr);
	
	// IAP,ReceiveProtocol,SamplingPeriodInMilliseconds,PacketSizeInBytes,ReceivedPort,EchoReceived
	ipr->SetIap(iap);
	ipr->SetSnap(snap);
	ipr->SetTcpMode(receiveTcp);
	ipr->SetTestDuration_sec(testDuration_sec);
	ipr->SetSamplingPeriod_ms(samplingPeriod_ms);
	ipr->SetPacketSizeInBytes(packetSizeInBytes);
	ipr->SetThroughputInKilobitsPerSecond(sendThroughputInKilobitsPerSecond);
	ipr->SetReceivePort(receivePort);
	ipr->SetEchoReceivedData(echoReceived);
	
	ipr->SpawnWorkerThreadL(EPriorityMore,_L("Netperf-Receiver"));
	
	User::LeaveIfError(iServer->Workers().Add(ipr, ConfigSection() ) );
	CleanupStack::Pop(ipr);
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}





CIperfTestSetupSender::CIperfTestSetupSender(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestSetupSender);
	}

CIperfTestSetupSender::~CIperfTestSetupSender()
	{
	}

TVerdict CIperfTestSetupSender::doTestStepL()
	{
	TPtrC temp_string;

	// IAP,SNAP,SendProtocol,SamplingPeriodInMilliseconds,SendThroughputInKilobitsPerSecond,PacketSizeInBytes,SendPeriodInMilliseconds,SendToAddress,SendToPort
	TInt iap = ReadIapL();
	TInt snap = ReadSnapL();
	TBool sendTcp=EFalse;
	TInt testDuration_sec = ReadTestDurationL();
	TInt samplingPeriod_ms = ReadSamplingPeriodL();
	TInt sendThroughputInKilobitsPerSecond=100;
	TInt packetSizeInBytes = ReadPacketSizeInBytesL();
	TInetAddr sendTo;
	TBool useLowerLayerPacketGenerator = EFalse;
	
	
	// IAP,SNAP,SendProtocol,SamplingPeriodInMilliseconds,SendThroughputInKilobitsPerSecond,PacketSizeInBytes,SendPeriodInMilliseconds,SendToAddress,SendToPort
	if ( GetStringFromConfig( ConfigSection(), _L("SendProtocol"), temp_string ) )
		{
		if (temp_string.Compare(_L("UDP"))==0)
			{
			INFO_PRINTF1(_L("Sending in UDP mode.."));			
			}
		else if (temp_string.Compare(_L("TCP"))==0)
			{
			sendTcp=ETrue;
			INFO_PRINTF1(_L("Sending in TCP mode.."));			
			}
		else
			{
			INFO_PRINTF1(_L("SendProtocol must be TCP or UDP"));
			User::Leave(KErrArgument);
			}
		}

	GetIntFromConfig( ConfigSection(), _L("SendThroughputInKilobitsPerSecond"), sendThroughputInKilobitsPerSecond);

	if (GetStringFromConfig( ConfigSection(), _L("SendToAddress"), temp_string))
		{
		TInt port=5001;
		GetIntFromConfig( ConfigSection(), _L("SendToPort"), port);
		if (sendTo.Input(temp_string))
			{
			INFO_PRINTF2(_L("Invalid SendToAddress [%s] specified!"),temp_string.Ptr());
			User::Leave(KErrArgument);
			}
		sendTo.SetPort(port);
		}

	GetBoolFromConfig( ConfigSection(), _L("UseLowerLayerPacketGenerator"), useLowerLayerPacketGenerator);

	CIperfSender* ips = new(ELeave)CIperfSender(*iServer, Logger());
	CleanupStack::PushL(ips);
	
	// IAP,SNAP,SendProtocol,SamplingPeriodInMilliseconds,SendThroughputInKilobitsPerSecond,PacketSizeInBytes,SendPeriodInMilliseconds,SendToAddress,SendToPort
	ips->SetIap(iap);
	ips->SetSnap(snap);
	ips->SetTcpMode(sendTcp);
	ips->SetTestDuration_sec(testDuration_sec);
	ips->SetSamplingPeriod_ms(samplingPeriod_ms);
	ips->SetThroughputInKilobitsPerSecond(sendThroughputInKilobitsPerSecond);
	ips->SetPacketSizeInBytes(packetSizeInBytes);
	ips->SetSendToSockAddr(sendTo);
	ips->SetUseLowerLayerPacketGenerator(useLowerLayerPacketGenerator);
	
	ips->SpawnWorkerThreadL(EPriorityNormal,_L("Netperf-Sender"));
	
	User::LeaveIfError(iServer->Workers().Add(ips, ConfigSection() ) );
	CleanupStack::Pop(ips);
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}





CIperfTestSetupCpuSponge::CIperfTestSetupCpuSponge(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestSetupCpuSponge);
	}

CIperfTestSetupCpuSponge::~CIperfTestSetupCpuSponge()
	{
	}

TVerdict CIperfTestSetupCpuSponge::doTestStepL()
	{
//	TInt samplingPeriod_ms = ReadSamplingPeriodL();

	CCpuSponge* worker = new(ELeave)CCpuSponge(Logger());
	CleanupStack::PushL(worker);
	
	TInt eaterThreadPriority;
	if (GetIntFromConfig( ConfigSection(), _L("EaterThreadPriority"), eaterThreadPriority))
		{
		worker->SetEaterThreadPriority(eaterThreadPriority);
		}
	
#ifndef __WINS__
	// For real devices we wait until things calm down; no real point on emulator & it just delays debugging
    TUint32 tick = User::NTickCount();
    const TUint32 KPostBootCalmingInterval = 135 * 1000;    // disruption seems to be occurring in the no-unload period, ie first 2 minutes
    INFO_PRINTF2(_L("Now %d seconds since boot"), tick / 1000);
    if(tick < KPostBootCalmingInterval)
        {
        INFO_PRINTF2(_L("Delaying start for %d secs [minimising post-boot disruption]"), (KPostBootCalmingInterval - tick) / 1000);
        do 
            {
            User::After(1000000);
            } while(User::NTickCount() < KPostBootCalmingInterval);
        }
#endif
    
	worker->SpawnWorkerThreadL(EPriorityNormal,_L("Netperf-CpuMeterCtl"));	// sponge will change this when it chooses
	
	User::LeaveIfError(iServer->Workers().Add(worker, ConfigSection() ) );
	CleanupStack::Pop(worker);
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}





CIperfTestStart::CIperfTestStart(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestStart);
	}

CIperfTestStart::~CIperfTestStart()
	{
	}

TVerdict CIperfTestStart::doTestStepL()
	{
	// for all these commands: log on AND await response (on different pubsub key)
	//  then when one of them completes,
	//   cancel both
	//   check results.
	//  this will show if the thread died before / during the execution of the command.
	GetWorkerL()->SendCommandL(KTestWorkerStart);
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}



CIperfTestStop::CIperfTestStop(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestStop);
	}

CIperfTestStop::~CIperfTestStop()
	{
	}

TVerdict CIperfTestStop::doTestStepL()
	{
	GetWorkerL()->SendCommandL(KTestWorkerStop);
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}



CIperfTestReport::CIperfTestReport(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestReport);
	}

CIperfTestReport::~CIperfTestReport()
	{
	}

TVerdict CIperfTestReport::doTestStepL()
	{
//	GetWorkerL()->SendCommandL(KTestWorkerReport);
	GetWorkerL()->ReportL();
	
	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}


CIperfTestDestroy::CIperfTestDestroy(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestDestroy);
	}

CIperfTestDestroy::~CIperfTestDestroy()
	{
	}

TVerdict CIperfTestDestroy::doTestStepL()
	{
	CTestWorker* worker = GetWorkerL();
	worker->ShutdownWorkerThreadL();

	// Remove our NotificationWatcher and Close the thread
	iServer->Workers().Remove(ConfigSection());
	delete worker;

	// return success / failure
	SetTestStepResult( EPass );
	return TestStepResult();
	}


CIperfStartProfile::CIperfStartProfile(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestStartProfiler);
	}
	
TVerdict CIperfStartProfile::doTestStepL()
	{
	TInt ret;
	if ((ret = Profiler::Start()) == KErrNotFound)
	    {
	    _LIT(KProfiler,"profiler");
	    _LIT(KStart,"start -noui");
	    RProcess p;
	    if ((ret = p.Create(KProfiler,KStart)) == KErrNone)
			{
			p.Resume();
			p.Close();
			}
	    }
	INFO_PRINTF2(_L("Profiler::Start() = %d"), ret);
	SetTestStepResult( ret == KErrNone? EPass: EFail );
	return TestStepResult();
	}

CIperfStopProfile::CIperfStopProfile(CIperfTestServer* aOwner)
	: CIperfTestStep(aOwner)
	{
	SetTestStepName(KTestStopProfiler);
	}
	
TVerdict CIperfStopProfile::doTestStepL()
	{
	TInt ret = Profiler::Stop();
	if (ret == KErrNone)
		{
		ret = Profiler::Unload();
		}
	INFO_PRINTF2(_L("Profiler::Stop/Unload() = %d"), ret);
	SetTestStepResult( ret == KErrNone? EPass: EFail );
	return TestStepResult();
	}

// need delete operation that logs in to worker's thread then deletes the worker memory

