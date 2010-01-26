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

#include <e32hal.h>	// UserHal::MachineInfo()
#include <in_sock.h> // IPv6 enhanced in_sock.h: KProtocolInet6Ip
#include <in6_if.h>	// KSoIface*, KIf*
#include "ncpip6.h"	// This
#include "ncpip.h" // for KSlashChar
#include "PPPLOG.H"
#include "PppProg.h"
#include <commsdattypeinfov1_1.h>
#include <networking/pppconfig.h>

using namespace ESock;

#if defined(__VC32__) && (_MSC_VER < 1300)
 #define PMF(x) x
#else
 #define PMF(x) &x
#endif

#ifdef __FLOG_ACTIVE
_LIT8(KNif,"Ppp");
_LIT8(KPPPBinderIP6,"IP6");
#endif

#pragma warning (disable:4355)
CPppBinderIp6::CPppBinderIp6(CPppLcp* aLcp)
	:   MPppFsm(aLcp, EPppPhaseNetwork, KPppIdIp6cp),
        iPppNifSubConnectionFlow(aLcp),
	    iIpRecvr(this, PMF(CPppBinderIp6::RecvIp), PMF(CPppBinderIp6::SendFlowOn), aLcp, EPppPhaseNetwork,
			KPppIdIp6, PMF(CPppBinderIp6::Ip6FrameError), PMF(CPppBinderIp6::Ip6KillProtocol))
{
#if EPOC_SDK <= 0x06000000
	__DECLARE_NAME(_S("CPppBinderIp6"));
#endif
	__DECLARE_FSM_NAME(_S("IP6CP"));
    __FLOG_OPEN(KNif, KPPPBinderIP6);
    __FLOG_2(_L8("this:%08x\tCPppBinderIp4::CPppBinderIp6(CPppLcp& %08x)"), this, &iPppNifSubConnectionFlow);

}
#pragma warning (default:4355)

CPppBinderIp6* CPppBinderIp6::NewL(CPppLcp* aLcp)
	{
	CPppBinderIp6* pppBinderIp6 = new(ELeave) CPppBinderIp6(aLcp);
	CleanupStack::PushL(pppBinderIp6);
	pppBinderIp6->ConstructL();
	CleanupStack::Pop(pppBinderIp6);
	return pppBinderIp6;
	}

CPppBinderIp6::~CPppBinderIp6()
{
	Deregister();
	iIpRecvr.Deregister();

	delete iSendCallBack;
	iSendQ.Free();
    __FLOG_CLOSE;
}

void CPppBinderIp6::ConstructL()
    {
    const CIPConfig* ncpConfig = Flow()->GetNcpConfig();
    if (NULL == ncpConfig)
        {
        User::Leave(KErrCorrupt);
        }

	Register();
	iIpRecvr.Register();

	TCallBack scb(SendCallBack, this);
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KIp6cpSendPriority);
	FsmConstructL();
	FsmOpen();

	// Create a unique interface name
	TBuf<KCommsDbSvrMaxColumnNameLength> port(ncpConfig->GetPortName());;
	if (port.Length() != 0)
		{
		port.LowerCase();
		iIfName.Format(_L("ipcp6::%S"), &port);
		}
	else
    	{
    	iIfName.Format(_L("ipcp6[0x%08x]"), this);
    	}
    }

MLowerDataSender* CPppBinderIp6::BindL(MUpperDataReceiver& aUpperReceiver, MUpperControl& aControl)
{
    __FLOG_1(_L8("CPppBinderIp6::Bind(MUpperDataReceiver %08x"), &aUpperReceiver);
	if(iUpperControl)
		User::Leave(KErrInUse);
    iUpperControl = &aControl;
	iUpperReceiver = &aUpperReceiver;
	return this;
}

void CPppBinderIp6::UnBind(MUpperDataReceiver& aUpperReceiver, MUpperControl& aUpperControl)
	{
    __FLOG(_L8("CDummyNifBinder6:\tUnbind()"));
    (void)aUpperReceiver;
    (void)aUpperControl;
    ASSERT(&aUpperReceiver == iUpperReceiver);
    ASSERT(&aUpperControl == iUpperControl);
    iUpperReceiver = NULL;
    iUpperControl = NULL;
	}

TBool CPppBinderIp6::MatchesUpperControl(const MUpperControl* aUpperControl) const
	{
	return iUpperControl == aUpperControl;
	}

TInt CPppBinderIp6::Control(TUint /*aLevel*/, TUint /*aName*/, TDes8& /*aOption*/)
{
	return KErrNotSupported;
}

void CPppBinderIp6::SendFlowOn()
{
	iLowerFlowOn = ESendAccepted;

	if (!iSendQ.IsEmpty())
	{
		iSendCallBack->CallBack();
	}

	if (iSendQ.IsEmpty() && iUpperControl)
	{
		iUpperFlowOn = ESendAccepted;
		iUpperControl->StartSending();
	}
}

void CPppBinderIp6::Error(TInt aError)
    {
    iUpperControl->Error(aError);
    }


TInt CPppBinderIp6::SendCallBack(TAny* aCProtocol)
{
	((CPppBinderIp6*)aCProtocol)->DoSend();
	return 0;
}

void CPppBinderIp6::FsmTerminationPhaseComplete()
{
}

void CPppBinderIp6::DoSend()
{
	if (FsmIsThisLayerOpen())
	{
		RMBufPacket pkt;

		while (iSendQ.Remove(pkt))
		{
			RMBufPktInfo*info = pkt.Unpack();
			TPppAddr addr;

			addr = info->iDstAddr;
            TUint protocol = addr.GetProtocol();
            pkt.Pack();

           	if (Flow()->Send(pkt, protocol) <= 0)
			    {
			    LOG( Flow()->iLogger->Printf(_L("IPCP Flow Off")); )
			    iLowerFlowOn = ESendBlocked;
			    break;
			    }
		}

		if (iLowerFlowOn && !iUpperFlowOn)
		{
			iUpperFlowOn = ESendAccepted;
			LOG( Flow()->iLogger->Printf(_L("StartSending to IP from DoSend()")); )
			Flow()->StartSending();
		}
	}
}

MLowerDataSender::TSendResult CPppBinderIp6::Send(RMBufChain& aPacket)
	{

#if EPOC_SDK==0x06000000
	iPppLcp->StartInactiveTimer();
#endif

	RMBufPacket packet;
	packet.Assign(aPacket);
	RMBufPktInfo* info = packet.Unpack();

	TPppAddr addr;
	addr.SetProtocol(KPppIdIp6);
	info->iDstAddr = addr;

	packet.Pack();

	iSendQ.Append(packet);
	iSendCallBack->CallBack();

	if (!FsmIsThisLayerOpen() || !iLowerFlowOn)
		{
		iUpperFlowOn = ESendBlocked;
		}

	return iUpperFlowOn;
	}

//-=========================================================
// MLowerControl methods
//-=========================================================
TInt CPppBinderIp6::GetName(TDes& aName)
    {
    __FLOG(_L8("CPppNifBinder6:\tGetName()"));

	aName.Copy(iIfName);

	return KErrNone;
    }

TInt CPppBinderIp6::BlockFlow(MLowerControl::TBlockOption /*aOption*/)
    {
    iLowerFlowOn = ESendBlocked;
    return KErrNone;
    }

TInt CPppBinderIp6::GetConfig(TBinderConfig& aConfig)
	{
    TBinderConfig6* config = TBinderConfig::Cast<TBinderConfig6>(aConfig);
    
   	if(config == NULL)
   		{
   		return KErrNotSupported;
   		}	
	
	config->iFamily = KAfInet6;
	
	config->iInfo.iFeatures = KIfIsPointToPoint | KIfCanMulticast | KIfIsDialup;		/* Feature flags */
	
	TInt rxsz, txsz;
	if (FsmIsThisLayerOpen())
		{
		iPppLcp->PppLink()->GetSendRecvSize(rxsz, txsz);
		config->iInfo.iMtu = txsz==0 ? KPppDefaultFrameSize : txsz;
		config->iInfo.iRMtu = rxsz==0 ? KPppDefaultFrameSize : rxsz;
		config->iInfo.iSpeedMetric = iPppLcp->PppLink()->SpeedMetric() / 1024;
		}
	else
		{
		config->iInfo.iMtu = KPppDefaultFrameSize;
		config->iInfo.iRMtu = KPppDefaultFrameSize;
		config->iInfo.iSpeedMetric = 0;
		}
	iPppLcp->SetMaxTransferSize(config->iInfo.iMtu);
	
	TEui64Addr* ifId = (TEui64Addr*)&config->iLocalId;
	
	ifId->Init();
	ifId->SetAddress(iLocalIfId);
		
	ifId = (TEui64Addr*)&config->iRemoteId;
	ifId->Init();
	ifId->SetAddress(iRemoteIfId);
	
	// Setup static DNS address if required
	
	if (!iPrimaryDns.IsUnspecified())
		{
		config->iNameSer1.SetAddress(iPrimaryDns);
		if (!iSecondaryDns.IsUnspecified())
			config->iNameSer2.SetAddress(iSecondaryDns);
		}   
	return KErrNone;
    }

// ################################################################

TInt CPppBinderIp6::FsmLayerStarted()
{
	iPppLcp->PppOpen();
	return KErrNone;
}

#if EPOC_SDK >= 0x06010000
void CPppBinderIp6::FsmLayerFinished(TInt aReason)
#else
void CPppBinderIp6::FsmLayerFinished(TInt /*aReason*/)
#endif
{
#if EPOC_SDK >= 0x06010000
	iPppLcp->PppClose(aReason);
#else
	iPppLcp->PppClose();
#endif
    Flow()->Progress(EPppProgressLinkDown, KErrNone);
}

void CPppBinderIp6::FsmLayerUp()
{
	// PPP is up. Inform the stakeholders.
	// Note:
    // It is important to signal Link Up first, then Flow On.
    // Some clients, (e.g SPUD) may make assumptions as to the order of these notifications.
    // Until LinkLayer Up is received, SPUD assumes that the NIF is not ready.
    // Sending Flow On before LinkLayer Up may cause these clients to make the wrong conclusions and misbehave.
//	Flow()->FlowUp(); // Inform control path: Link is up.
	iPppLcp->NcpUp(); // Inform control path: Link is up. 
    SendFlowOn(); // Inform data path: ready to process data.

    Flow()->Progress(EPppProgressLinkUp, KErrNone);
    Flow()->BinderLinkUp(CPppLcp::EPppIp6);
}

void CPppBinderIp6::FsmLayerDown(TInt aReason)
{
	LOG( iPppLcp->iLogger->Printf(_L("NCPIP6::FsmLayerDown reason[%d]. "),aReason); )

	// Mobile IP Inter-PDSN Handoff support.
	if(KErrNone == aReason)
		{
		Flow()->FlowDown(aReason, MNifIfNotify::ENoAction);
		return;
		}


	// If the layer is down due to an error, it means that the FSM is terminating.

	// RFC1661 compliant Termination sequence support:
 	if(iTerminateRequestEnabled || iTerminateAckEnabled)
		{
		// LCP signals the Down event on all NCPs:
 		// We don't want to signal to Nifman from here, because LCP is not finished yet.
 		// We let LCP handle disconnection notification to Nifman
		return;
		}

	// Legacy shutdown support:
	// Notify SCPR.
	// When legacy shutdown is no longer required, this code can be safely removed.
	if(KErrCommsLineFail == aReason && FsmIsThisLayerOpen())
	    {
	    // This is a legacy EReconnect scenario
		Flow()->FlowDown(aReason, MNifIfNotify::EReconnect);
	    }
    else
        {
    	Flow()->FlowDown(aReason, MNifIfNotify::EDisconnect);
        }

    Flow()->Progress(EPppProgressLinkDown, aReason);
}


void CPppBinderIp6::FsmFillinConfigRequestL(RPppOptionList& aReqList)
{
	const CIPConfig* ncpConfig = Flow()->GetNcpConfig();

	// See whether we are configured to request a dynamic DNS address.
	if(!ncpConfig->GetIp6DNSAddrFromServer())
		{
		// Setup static DNS addresses from CommDb
		LOG(iPppLcp->iLogger->Printf(_L("Configuring static IPv6 DNS addresses"));)
		iPrimaryDns = ncpConfig->GetIp6NameServer1();
		iSecondaryDns = ncpConfig->GetIp6NameServer2();
		}
	else
		{
		// Ensure that static DNS addresses are set as unspecified,
		// so they are not used in Control(KSoIfConfig).
		iPrimaryDns = KInet6AddrNone;
		iSecondaryDns = KInet6AddrNone;
		}

	RPppOption opt;
	TMachineInfoV1Buf machineInfo;

	//
	// Use the 64 bit id of MARM machines as our interface id
	//
	UserHal::MachineInfo(machineInfo);
	iLocalIfId.SetAddr(machineInfo().iMachineUniqueId);
	iLocalIfId.SetUniversalBit(0);

	//
	// In WINS environment the id is zero which is no-no
	//
	if (iLocalIfId.IsZero())
	{
#ifdef NCPIP6_STATIC_ID
		const TUint8 constantId[8] = { 0x1c, 0xe5, 0xb8, 0x4d, 0xd6, 0xfb, 0x10, 0x63 };
		iLocalIfId.SetAddr(constantId, sizeof (constantId));
#else
		iLocalIfId.SetAddrRandomNZ();
#endif
	}

	opt.SetL (KPppIp6cpOptInterfaceIdentifier, iLocalIfId.AddrPtr(), iLocalIfId.AddrLen());
	aReqList.Append(opt);
}

void CPppBinderIp6::FsmCheckConfigRequest(RPppOptionList& aReqList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
{
	RPppOption opt;

	while (aReqList.Remove(opt))
	{
		switch (opt.OptType())
		{
		case KPppIp6cpOptCompressionProtocol:
			aRejList.Append(opt);
			break;

		case KPppIp6cpOptInterfaceIdentifier:
			{
				iRemoteIfId.SetAddr(opt.ValuePtr(), opt.ValueLength());

				if (iLocalIfId.Match(iRemoteIfId) || iRemoteIfId.IsZero())
				{
					iRemoteIfId.SetAddrRandomNZButNot(iLocalIfId);
					Mem::Copy(opt.ValuePtr(), iRemoteIfId.AddrPtrC(), iRemoteIfId.AddrLen());
					aNakList.Append(opt);
				}
				else
				{
					aAckList.Append(opt);

				}
			}
			break;

		default:
			aRejList.Append(opt);
			break;
		}
	}
}

void CPppBinderIp6::FsmApplyConfigRequest(RPppOptionList& aReqList)
{
	TMBufPktQIter iter(aReqList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
	{
		switch (opt.OptType())
		{
		case KPppIp6cpOptCompressionProtocol:
			break;

		case KPppIp6cpOptInterfaceIdentifier:
			iRemoteIfId.SetAddr(opt.ValuePtr(), opt.ValueLength());
			break;

		default:
			break;
		}
	}
}

void CPppBinderIp6::FsmRecvConfigAck(RPppOptionList& aRepList)
{
	TMBufPktQIter iter(aRepList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
	{
		switch (opt.OptType())
		{
		case KPppIp6cpOptCompressionProtocol:
			break;

		case KPppIp6cpOptInterfaceIdentifier:
			iLocalIfId.SetAddr(opt.ValuePtr(), opt.ValueLength());
			break;

		default:
			break;
		}
	}
}

void CPppBinderIp6::FsmRecvConfigNak(RPppOptionList& aRepList, RPppOptionList& aReqList)
{
	TMBufPktQIter iter(aRepList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
	{
		switch (opt.OptType())
		{
		case KPppIp6cpOptCompressionProtocol:
			aReqList.ReplaceOption(opt);
			break;

		case KPppIp6cpOptInterfaceIdentifier:
			{
				TE64Addr iIfId(opt.ValuePtr(), opt.ValueLength());

				if (iLocalIfId.Match(iIfId))
				{
					iLocalIfId.SetAddrRandomNZButNot(iIfId);
					Mem::Copy(opt.ValuePtr(), iLocalIfId.AddrPtrC(), iLocalIfId.AddrLen());
				}
				else
				{
					iLocalIfId.SetAddr(iIfId);
				}

				aReqList.ReplaceOption(opt);
			}

			break;

		default:
			aReqList.ReplaceOption(opt);
			break;
		}
	}
}

void CPppBinderIp6::FsmRecvConfigReject(RPppOptionList& aRepList, RPppOptionList& aReqList)
{
	TMBufPktQIter iter(aRepList);
	RPppOption opt;

	while (opt = iter++, !opt.IsEmpty())
	{
		switch (opt.OptType())
		{
		case KPppIp6cpOptCompressionProtocol:
			aReqList.RemoveOption(opt);
			break;

		case KPppIp6cpOptInterfaceIdentifier:
			FsmAbort(KErrNotReady);
			break;

		default:
			aReqList.RemoveOption(opt);
			break;
		}
	}
}

void CPppBinderIp6::KillProtocol()
{
	FsmAbort(KErrCouldNotConnect);
	return;
}

TBool CPppBinderIp6::FsmRecvUnknownCode(TUint8 /*aCode*/, TUint8 /*aId*/, TInt /*aLength*/, RMBufChain& /*aPacket*/)
{
	return EFalse;
}

// ################################################################

void CPppBinderIp6::RecvIp(RMBufChain& aPacket)
{
	if (iUpperReceiver)
		iUpperReceiver->Process(aPacket);
	else
		aPacket.Free();
}

void CPppBinderIp6::Ip6KillProtocol()
{
    __FLOG_2(_L8("this:%08x\tCPppBinderIp6::Ip6KillProtocol() - disconnecting because access was denied"), this, &iPppNifSubConnectionFlow);

    Flow()->FlowDown(KErrAccessDenied, MNifIfNotify::EDisconnect);
	return;
}

void CPppBinderIp6::Ip6FrameError()
{
	return;
}

#if EPOC_SDK >= 0x06010000
TInt CPppBinderIp6::Notification(TAgentToNifEventType aEvent)
{
	if (aEvent==EAgentToNifEventTypeModifyInitialTimer)
	{
		ChangeTimers(ETrue);
	}
	else
	{
		__ASSERT_DEBUG(EFalse, User::Invariant());
	}
	return KErrNone;
}
#endif
