/*
* Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of the License "Symbian Foundation License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.symbianfoundation.org/legal/sfl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
*
*/
#ifndef __TEF_IPSEC_CONST_H__
#define __TEF_IPSEC_CONST_H__

#include <e32base.h> 
#include <es_sock.h>
#include <e32cmn.h> 
#include <in_sock.h>
#include <commdbconnpref.h>
#include <comms-infras/es_parameterbundle.h>

#if defined(SYMBIAN_CRYPTO)
#include <cryptostrength.h>
#else
#include <cryptalg.h>
#endif

const TInt KSockBufferLength=7;

//test case name use the following
_LIT(KIPSecTestBypass, "IPSecTestByPassPolicy");
_LIT(KIPSecTestDrop, "IPSecTestDropPolicy");
_LIT(KIPSecTestUMA, "IPSecTestUMAPolicy");
_LIT(KIPSecCoverageTest, "IPSecTestCoverage");

_LIT(KIPSecSendPacketTest, "IPSecSendPacket");
_LIT(KIPSecNATKeepAliveTest, "IPSecNATKeepAlive");
_LIT(KIPSecDPDKeepAliveTest, "IPSecDPDKeepAlive");
_LIT(KIPSecNATTimeOutTest, "IPSecConfNatTimeout");
_LIT(KIPSecDPDTimeOutTest,"IPSecConfDPDTimeout");
_LIT(KIPSecDelConnMgrTest,"IPSecDelConnMgrTest");
_LIT(KIPSecNATRestartTimerTest,"IPSecNATRestartTimerTest");
_LIT(KIPSecAddConnWatcherTest,"IPSecAddConnWatcherTest");
_LIT(KIPSecDelConnWatcherTest,"IPSecDelConnWatcherTest");
_LIT(KIPSecTimerTest,"IPSecTimerTest");

// Commands
_LIT(KCloseConnection,"Close");
_LIT(KSendPackets,	"SendPacket");
_LIT(KDelayCMD, "Delay");
_LIT(KReportEvt, "ReportEvt");
_LIT(KReStartEvtCMD, "ReStartEvt");
//commands use the following 
_LIT(KNewCMD,"New");
_LIT(KTestLoadBypassPolicy, "LoadBypassModePolicy");
_LIT(KTestLoadDropPolicy, "LoadDropModePolicy");
_LIT(KLoadBypasspolicy, "LoadBypassModePolicy");
_LIT(KTestLoadDropModePolicy, "LoadDropModePolicy");
_LIT(KLoadNewDroppolicy, "LoadNewDropModePolicy");
_LIT(KLoadNewBypassPolicy, "LoadNewBypassModePolicy");
_LIT(KUnloadDropPolicy, "UnloadDropModePolicy");
_LIT(KUnloadBypassPolicy, "UnloadBypassModePolicy");
_LIT(KUnloadUMAPolicy, "UnloadUMAModePolicy");
_LIT(KLOadUMAPolicy, "LOadUMAModePolicy");
_LIT(KLoadUMAByPassPolicy, "LoadByPassModePolicy");
_LIT(KUnloadNewBypassPolicy,"UnloadNewBypassModePolicy");
_LIT(KUnloadNewDropPolicy,"UnloadNewDropModePolicy");



_LIT(KCloseConnectionCMD,"Close");
_LIT(KSendPacketsCMD,  "SendPacket");
_LIT(KReportEvtCMD, "ReportEvt");
_LIT(KAddConnWatcherCMD, "AddConnWatcher");
_LIT(KDelConnWatcherCMD, "DelConnWatcher");
_LIT(KDelConnMgrCMD, "DelConnMgr");
_LIT(KIPSecSecondCon, "KIPSecSecondCon");

/// Command parameters
_LIT(KObjectValue, "object_value");
_LIT(KTOSorTrafficClass, "TosOrTrafficClass");
_LIT(KPort, "port");
_LIT(KIapid, "iapid");
_LIT(KDestAddr, "destaddr");
_LIT(KDelay, "Delay");
_LIT(KLocaladdr, "localaddr");
_LIT(KIPSecNATTimeOutValDPDTest, "IPSecNATTimeOutVal1");
_LIT(KIPSecDPDTimeOutValDPDTest,"IPSecDPDTimeOutVal1");
_LIT(KIPSecNATRestartTimerAfter,"IPSecNATRestartTimerAfter");
_LIT(KIPSecDelay,"IPSecDelay");

//parameters use the following
_LIT(KParamObjectValue, "object_value");
_LIT(KParamPort, "port");
_LIT(KParamIapid, "iapid");
_LIT(KParamDestIPAddr, "destaddr");
_LIT(KParamDelay, "Delay");
_LIT(KParamLocaladdr, "localaddr");
_LIT(KParamIPSecNATTimeOutValDPDTest, "IPSecNATTimeOutVal1");
_LIT(KParamIPSecDPDTimeOutValDPDTest,"IPSecDPDTimeOutVal1");
_LIT(KParamIPSecNATRestartTimerAfter,"IPSecNATRestartTimerAfter");
_LIT(KIPSecNATTimeOutValue,"IPSecNATTimeOutVal");
_LIT(KIPSecDPDTimeOutValue,"IPSecDPDTimeOutVal");
_LIT(KParamIPSecDelay,"IPSecDelay");
_LIT(KParamDestIPAddr1, "destaddr1");
_LIT(KParamDestIPAddr2, "destaddr2");
_LIT(KParamLocaladdr1, "localaddr1");
_LIT(KParamLocaladdr2, "localaddr2");
_LIT(KDrive, "drive");
_LIT(KMsvServerPattern,	"!MsvServer*");

#endif /*__TEF_IPSEC_CONST_H__*/
