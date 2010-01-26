// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <e32uid.h>
#include <networking/vj.h>
#include "ncpip.h"
#include "PPPLOG.H"
#include "PPP_VER.H"
#include <networking/pppprog.h>
#include <networking/pppdebug.h>
#include <commsdattypeinfov1_1.h>
#include <comms-infras/ss_subconnprov.h>
#include <networking/pppbase.h>
#include <networking/pppconfig.h>

using namespace ESock;

#if defined(__VC32__) && (_MSC_VER < 1300)
 #define PMF(x) x
#else
 #define PMF(x) &x
#endif

_LIT(KVjComp, "vjcomp");
#ifdef __FLOG_ACTIVE
_LIT8(KNif,"Ppp");
_LIT8(KPPPBinderIP4,"IP4");
#endif

//
// PPP for ESock
//

#ifdef __VC32__
// warning C4355: 'this' : used in base member initializer list
#pragma warning (disable:4355)
#endif

CPppBinderIp4::CPppBinderIp4(CPppLcp* aLcp)
	:   MPppFsm(aLcp, EPppPhaseNetwork, KPppIdIpcp),
        iPppNifSubConnectionFlow(aLcp),
		iIpRecvr(this, PMF(CPppBinderIp4::RecvIp), PMF(CPppBinderIp4::SendFlowOn), aLcp, EPppPhaseNetwork,
			KPppIdIp, PMF(CPppBinderIp4::IpFrameError), PMF(CPppBinderIp4::IpKillProtocol)),
		iVjCompTcpRecvr(this, PMF(CPppBinderIp4::RecvVjCompTcp), PMF(CPppBinderIp4::SendFlowOn), aLcp, EPppPhaseNetwork,
			KPppIdVjCompTcp, PMF(CPppBinderIp4::VjCompTcpFrameError), PMF(CPppBinderIp4::VjCompTcpKillProtocol)),
		iVjUncompTcpRecvr(this, PMF(CPppBinderIp4::RecvVjUncompTcp), PMF(CPppBinderIp4::SendFlowOn), aLcp, EPppPhaseNetwork,
			KPppIdVjUncompTcp, PMF(CPppBinderIp4::VjUncompTcpFrameError), PMF(CPppBinderIp4::VjUncompTcpKillProtocol))
	{
	__DECLARE_FSM_NAME(_S("IPCP"));
    __FLOG_OPEN(KNif, KPPPBinderIP4);
    __FLOG_2(_L8("this:%08x\tCPppBinderIp4::CPppBinderIp4(CPppLcp& %08x)"), this, &iPppNifSubConnectionFlow);

	}
#ifdef __VC32__
#pragma warning (default:4355)
#endif

CPppBinderIp4* CPppBinderIp4::NewL(CPppLcp* aLcp)
	{
	CPppBinderIp4* pppBinderIp4 = new(ELeave) CPppBinderIp4(aLcp);
	CleanupStack::PushL(pppBinderIp4);
	pppBinderIp4->ConstructL();
	CleanupStack::Pop(pppBinderIp4);
	return pppBinderIp4;
	}

CPppBinderIp4::~CPppBinderIp4()
	{
	CObjectCon* con;
	TInt		i;

	Deregister();
	iIpRecvr.Deregister();
	iVjCompTcpRecvr.Deregister();
	iVjUncompTcpRecvr.Deregister();
	delete iSendCallBack;

	if (iVJCompressor)
		{
		iVJCompressor->Close();
		iVJCompressor = NULL;
		}

	if (iVJDecompressor)
		{
		iVJDecompressor->Close();
		iVJDecompressor = NULL;
		}

	//
	// Delete all the Containers
	//
	if(iVJCompressorCon)
		{
		con = iVJCompressorCon;

		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
			iVJCompressorCon = NULL;
		}


	if(iVJDeCompressorCon)
		{
		con = iVJDeCompressorCon;
		for(i=0 ; i<con->Count() ; ++i)
			CNifFactory::ControlledDelete((*con)[i]);
		iVJDeCompressorCon = NULL;
		}

	iSendQ.Free();
    __FLOG_CLOSE;
	}


/**
Construct the Link Protocol Object
*/
void CPppBinderIp4::ConstructL()
	{
    const CIPConfig* ncpConfig = Flow()->GetNcpConfig();
    if (NULL == ncpConfig)
        {
        User::Leave(KErrCorrupt);
        }

	Register();
	iIpRecvr.Register();
	iVjCompTcpRecvr.Register();
	iVjUncompTcpRecvr.Register();

	TCallBack scb(SendCallBack, this);
	iSendCallBack = new(ELeave) CAsyncCallBack(scb, KIpcpSendPriority);

	iVJCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();
	iVJDeCompressorCon = (iPppLcp->ContainerForDlls())->CreateL();

	FsmConstructL();
	FsmOpen();

	// Create a unique interface name
	TBuf<KCommsDbSvrMaxColumnNameLength> port(ncpConfig->GetPortName());

	if (port.Length() != 0)
		{
		port.LowerCase();
		iIfName.Format(_L("ipcp::%S"), &port);
		}
	else
    	{
    	iIfName.Format(_L("ipcp[0x%08x]"), this);
    	}
	}

// from MLowerDataSender
/**
Called by the IP stack to send a packet.
The packet is first VJ compressed if compression was negotiated.

@param aPacket MBuf chain containing packet
@param aSource ignored
@return 1 for a successful send, 0 to tell upper layer to flow off
*/
MLowerDataSender::TSendResult CPppBinderIp4::Send(RMBufChain& aPacket)
	{
	RMBufPacket packet;
	packet.Assign(aPacket);

	TInt Protocol;
	if (iVJCompressor)
		{
		Protocol = iVJCompressor->VJCompressFrame(packet);
		return SendProtFrame(packet, Protocol );
		}
	else
		{
		//	Send normal IP frame
		return SendProtFrame(packet, KPppIdIp);
		}
	}

// from MLowerControl
TInt CPppBinderIp4::GetName(TDes& aName)
    {
    __FLOG(_L8("CPppNifBinder4:\tGetName()"));

	aName.Copy(iIfName);
	return KErrNone;
    }

TInt CPppBinderIp4::BlockFlow(MLowerControl::TBlockOption /*aOption*/)
    {
    iLowerFlowOn = ESendBlocked;
    return KErrNone;
    }

TInt CPppBinderIp4::GetConfig(TBinderConfig& aConfig)
    {
    TBinderConfig4* config = TBinderConfig::Cast<TBinderConfig4>(aConfig);
    
   	if(config == NULL)
   		{
   		return KErrNotSupported;
   		}	

   	config->iFamily = KAfInet;

    config->iInfo.iFeatures = KPppIpIfaceFeatures | KIfIsDialup;		/* Feature flags */

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

    config->iAddress.SetAddress(iLocalAddr);		/* Interface IP address. */
    config->iNetMask.SetAddress(iNetworkMask);	/* IP netmask. */
    config->iBrdAddr.SetAddress(iRemoteAddr);	/* IP broadcast address. */
    config->iDefGate.SetAddress(iRemoteAddr ? iRemoteAddr : iLocalAddr);		/* IP default gateway or peer address (if known). */
    config->iNameSer1.SetAddress(iPrimaryDns);	/* IP primary name server (if any). */
    config->iNameSer2.SetAddress(iSecondaryDns);	/* IP secondary name server (if any). */
    return KErrNone;
    }

/**
Called by the TCP/IP stack to get/set PPP NIF parameters.

@param aLevel Must be KSOLInterface
@param aName Option name
@param aOption Data for the given option
@param aSource ignored
*/
TInt CPppBinderIp4::Control(TUint aLevel, TUint aName, TDes8& /*aOption*/)
    {
    __FLOG(_L8("CPppNifBinder4::\tControl()"));

	if (aLevel==KSOLInterface)
		{
		switch (aName)
			{
#ifdef _DEBUG
		// Forces PPP to renegotiate the connection
		case KSoIfForceRenegotiatePPP:
			{
			FsmOpen();
			return KErrNone;
			}
#endif

		case KSoIfHardwareAddr:
		default:
			break;
			}
		}
	return KErrNotSupported;
	}

// from MUpperControl
/**
Notifies the TCP/IP stack that it may start sending data.
*/
void CPppBinderIp4::SendFlowOn()
    {
    __FLOG_1(_L("IPCP Flow On iUpperFlowOn=%d"), iUpperFlowOn);
    // unblock call from lower layer, therefore immediately unblock
	iLowerFlowOn = ESendAccepted;
    // Ensure Send queue is empty before reopening upper flow
	if (!iSendQ.IsEmpty())
		iSendCallBack->CallBack();
	if (iSendQ.IsEmpty())
		{
		iUpperFlowOn = ESendAccepted ;
        __FLOG(_L("Start Sending to IP from Ready()"));
        iUpperControl->StartSending();
		}
    }


void CPppBinderIp4::Error(TInt aError)
    {
    iUpperControl->Error(aError);
    }

/**
Binds NCPIP to the TCP/IP stack.

@param aId Pointer to network stack object (CProtocolBase*)
*/
MLowerDataSender* CPppBinderIp4::BindL(MUpperDataReceiver& aUpperReceiver, MUpperControl& aControl)
	{
    __FLOG_1(_L8("CPppBinderIp4:\tBind(MUpperDataReceiver %08x)"), &aUpperReceiver);
	if(iUpperControl)
		User::Leave(KErrInUse);
    iUpperControl = &aControl;
	iUpperReceiver = &aUpperReceiver;
	return this;
	}

void CPppBinderIp4::UnBind(MUpperDataReceiver& aUpperReceiver, MUpperControl& aUpperControl)
	{
	(void)aUpperReceiver;
	(void)aUpperControl;
    __FLOG(_L8("CDummyNifBinder4:\tUnbind()"));
    ASSERT(&aUpperReceiver == iUpperReceiver);
    ASSERT(&aUpperControl == iUpperControl);
    iUpperReceiver = NULL;
    iUpperControl = NULL;
	}

TBool CPppBinderIp4::MatchesUpperControl(const MUpperControl* aUpperControl) const
/**
Comparison routine used by Flow to determine which binder is being referred to
in an MFlowBinderControl::Unbind() call.

@param aUpperControl MUpperControl object to compare against this binder

@return ETrue if this binder is associated with aUpperControl, else EFalse.
*/
	{
	return iUpperControl == aUpperControl;
	}

/**
Static function called asynchronously to send packets out the link.

@param aCProtocol Pointer to this NCPIP object
@return 0
*/
TInt CPppBinderIp4::SendCallBack(TAny* aCProtocol)
	{
	((CPppBinderIp4*)aCProtocol)->DoSend();
	return 0;
	}

/**
Dequeues packets and sends them out the link.
*/
void CPppBinderIp4::DoSend()
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
		if (iLowerFlowOn&&!iUpperFlowOn)
			{
			iUpperFlowOn = ESendAccepted;
			LOG( Flow()->iLogger->Printf(_L("StartSending to IP from DoSend()")); )
			Flow()->StartSending();
			}
		}
	}


//
// NCP Upcalls from FSM
//

/**
Opens the layer below
*/
TInt CPppBinderIp4::FsmLayerStarted()
	{
	return iPppLcp->PppOpen();
	}

/**
Closes the layer below
*/
void CPppBinderIp4::FsmLayerFinished(TInt aReason)
	{
	iPppLcp->PppClose(aReason);
    Flow()->Progress(EPppProgressLinkDown, KErrNone);
	}

/**
Signals up event to next layer above
*/
void CPppBinderIp4::FsmLayerUp()
	{
	// PPP is up. Inform the stakeholders.
	// Note:
    // It is important to signal Link Up first, then Flow On.
    // Some clients, (e.g SPUD) may make assumptions as to the order of these notifications.
    // Until LinkLayer Up is received, SPUD assumes that the NIF is not ready.
    // Sending Flow On before LinkLayer Up may cause these clients to make the wrong conclusions and misbehave.

	iPppLcp->NcpUp(); // Inform control path: Link is up. 
    SendFlowOn(); // Inform data path: ready to process data.

	if(!iPppLcp->QueryExternalIPConfiguration())
        {
        // Only send the LinkUp progress if we're not
        // doing Mobile IP.  If we are doing MIP then
        // the LinkUp progress should be generated in
        // the MIP plugin module after it has finished
        // its own negotiation
        Flow()->Progress(EPppProgressLinkUp, KErrNone);
        Flow()->BinderLinkUp(CPppLcp::EPppIp4);
        }
	}

/**
Signals down event to next layer above
*/
void CPppBinderIp4::FsmLayerDown(TInt aReason)
	{
	LOG( iPppLcp->iLogger->Printf(_L("CPppBinderIp4::FsmLayerDown reason[%d]."),aReason); )

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



/**
Fills in Config Request to be sent
*/
void CPppBinderIp4::FsmFillinConfigRequestL(RPppOptionList& aRequestList)
	{
	const CIPConfig* ncpConfig = Flow()->GetNcpConfig();

	// See whether we are configured to request a dynamic IP address. If so, make
	// sure we get one by setting iLocalAddr to 0.0.0.0
	if(!ncpConfig->GetIpAddrFromServer())
		{
		LOG(iPppLcp->iLogger->Printf(_L("Requesting static IP address"));)
		iLocalAddr = ncpConfig->GetIpAddress();
		}
	else
		{
		LOG(iPppLcp->iLogger->Printf(_L("Requesting dynamic IP address"));)
		iLocalAddr = KInetAddrNone;
		}

	// See whether we are configured to request a dynamic DNS address. If so, make
	// sure we get one by setting iPrimaryDns and iSecondaryDns to 0.0.0.0
	if(!ncpConfig->GetIp4DNSAddrFromServer())
		{	// Static addresses
		LOG(iPppLcp->iLogger->Printf(_L("Requesting static DNS address"));)
		iPrimaryDns = ncpConfig->GetIp4NameServer1();
		iSecondaryDns = ncpConfig->GetIp4NameServer2();
		}
	else
		{	// set to 0 for dynamic address request
		LOG(iPppLcp->iLogger->Printf(_L("Requesting dynamic DNS address"));)
		iPrimaryDns = KInetAddrNone;
		iSecondaryDns = KInetAddrNone;
		}
	iPrimaryNbns = KInetAddrNone;
	iSecondaryNbns = KInetAddrNone;

	iNetworkMask = ncpConfig->GetIpNetMask();
	if(iNetworkMask)
		{
		TUint32 mask=~1u, bits=iNetworkMask;
		while((bits&1)==0)
			{
			bits>>=1;
			mask<<=1;
			}
		iNetworkMask = mask;
		}

    iRemoteAddr = ncpConfig->GetIpGateway(); // Field use differently in server mode (GPRS Ip gateway)

    iVJCompressionOn = ncpConfig->GetEnableIpHeaderComp();
    if (iVJCompressionOn)
		{
		_LIT(KVJCompDll, "vjcomp.dll");
		iVJCompressionOn = iPppLcp->DoesDllExist(KVJCompDll);
		}

	if (iVJCompressionOn)
		{
		const TUint32 CompressionOptions = (KPppIdVjCompTcp << 16) | (KDesiredVjSlot << 8) | KVjCompSlotId;
		aRequestList.CreateAndAddL(KPppIpcpOptIpCompressionProtocol, CompressionOptions);
		}

	if(!iPppLcp->QueryExternalIPConfiguration())
		{
		// Don't ask for IP address if we get it with another protocol.
		aRequestList.CreateAndAddL(KPppIpcpOptIpAddress, iLocalAddr);
		}

	if (ncpConfig->GetIp4DNSAddrFromServer())
		{
		aRequestList.CreateAndAddL(KPppIpcpOptPrimaryDnsAddress, iPrimaryDns);
	//	aRequestList.CreateAndAddL(KPppIpcpOptPrimaryNbnsAddress, iPrimaryNbns);
		aRequestList.CreateAndAddL(KPppIpcpOptSecondaryDnsAddress, iSecondaryDns);
	//	aRequestList.CreateAndAddL(KPppIpcpOptSecondaryNbnsAddress, iSecondaryNbns);
		}
	// Nbns negotiation removed to speed things up
	//
	}

/**
Checks options in a received config request.
Each option is added to the appropriate Ack, Nak or Rej list.
Upcall from the FSM.

@param aRequestList LCP options to check
@param aAckList Acked LCP options
@param aNakList Naked LCP options
@param aRejList Rejected LCP options
*/
void CPppBinderIp4::FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)
	{
	RPppOption opt;
	while (aRequestList.Remove(opt))
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
		    if (iVJCompressionOn)
				{
				TUint16	CompressionProtocol = 0;  // Invalid compression protocol
				TUint Length = opt.ValueLength();
				if (Length >= 2)
					CompressionProtocol = BigEndian::Get16(opt.ValuePtr());

				if (CompressionProtocol == KPppIdVjCompTcp)
					{
					//
					//	VJ Compression
					//
					TUint8*	Options = opt.ValuePtr();
					Options += 2;

					if (*Options >= KMinVjSlot && *Options <= KDesiredVjSlot)
						{
						iMaxVJSlots = *Options++;
						iCompressConnId = *Options++;
						//
						//	If these values are OK then ACK
						//
						aAckList.Append(opt);
						}
					else
						{
						aNakList.Append(opt);
						}
					}
				else
					{
					// Unknown compression protocol
					aRejList.Append(opt);
					}
				}
			aRejList.Append(opt);
			break;
		case KPppIpcpOptIpAddress:
			{
			const TUint32 addr = BigEndian::Get32(opt.ValuePtr());

			if ((iPppLcp->PppLinkMode()== CPppLcpConfig::EPppLinkIsServer)
			    && (addr != iRemoteAddr)
			    && (iRemoteAddr != KInetAddrNone))
				{
				// If peer doesn't know its correct IP address but we do, tell it
				BigEndian::Put32(opt.ValuePtr(), iRemoteAddr);
				aNakList.Append(opt);
				}
			else if ((addr == KInetAddrNone) &&
					 ((iPppLcp->PppLinkMode() != CPppLcpConfig::EPppLinkIsServer) ||
					 ((iPppLcp->PppLinkMode() == CPppLcpConfig::EPppLinkIsServer) && (iRemoteAddr == KInetAddrNone))))
				{
				// If peer doesn't know its IP address and neither do we, reject the request
				aRejList.Append(opt);
				}
			else
				{
				// Peer knows its own IP address and we have no reason to believe its wrong
				aAckList.Append(opt);
				}
			break;
			}
		case KPppIpcpOptIpAddresses:
			{
			const TUint32 remoteAddr = BigEndian::Get32(opt.ValuePtr());
			const TUint32 addr = BigEndian::Get32(opt.ValuePtr()+4);

			// TODO: This negotiation isn't very robust
	        if (remoteAddr == KInetAddrNone)
		        {
				if (iPppLcp->PppLinkMode()!= CPppLcpConfig::EPppLinkIsServer)
					{
					aRejList.Append(opt);
					}
				else
					{
					BigEndian::Put32(opt.ValuePtr(), iRemoteAddr);
					aNakList.Append(opt);
					}
		        }
			else if (addr == KInetAddrNone)
	            {
	            // It doesn't matter how we found our local address; if we know it, tell the peer
				if (iLocalAddr != KInetAddrNone)
					{
					BigEndian::Put32(opt.ValuePtr()+4, iLocalAddr);
					}
				aNakList.Append(opt);
				}
			else
	            {
				aAckList.Append(opt);
	            }
			break;
			}
		case KPppIpcpOptPrimaryDnsAddress:
			{
			const TUint32 reqDns = BigEndian::Get32(opt.ValuePtr());
			if (reqDns != KInetAddrNone)
				{
				// Accept whatever DNS address the remote wants
				aAckList.Append(opt);
				}
			else
				{
				if ((iPppLcp->PppLinkMode()== CPppLcpConfig::EPppLinkIsServer) && (iPrimaryDns != KInetAddrNone))
					{
					// Give it our DNS address if we have one for it
					BigEndian::Put32(opt.ValuePtr(), iPrimaryDns);
					aNakList.Append(opt);
					}
				else
					{
					// We don't have any DNS addresses to give the peer
					aRejList.Append(opt);
					}
				}
			break;
			}
		case KPppIpcpOptSecondaryDnsAddress:
			{
			const TUint32 reqDns = BigEndian::Get32(opt.ValuePtr());
			if (reqDns != KInetAddrNone)
				{
				// Accept whatever DNS address the remote wants
				aAckList.Append(opt);
				}
			else
				{
				if ((iPppLcp->PppLinkMode()== CPppLcpConfig::EPppLinkIsServer) && (iSecondaryDns != KInetAddrNone))
					{
					// Give it our DNS address if we have one for it
					BigEndian::Put32(opt.ValuePtr(), iSecondaryDns);
					aNakList.Append(opt);
					}
				else
					{
					// We don't have any DNS addresses to give the peer
					aRejList.Append(opt);
					}
				}
			break;
			}
		default:
			aRejList.Append(opt);
			break;
			}
		}
	}

/**
Loads the VJ compressor DLL and creates a compressor object.

@return VJ compressor object
*/
CVJCompressorIf* CPppBinderIp4::LoadVJCompressorL()
	{
	CVJCompFactory* Factory=NULL;
	CVJCompressorIf*	VJCompressor;

	Factory = (CVJCompFactory*)FindPppFactoryL(KVjComp, TUid::Uid(KSharedLibraryUidValue), *iVJCompressorCon);

	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
	VJCompressor = Factory->NewVJCompressorL(iMaxVJSlots, iCompressConnId);
	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return VJCompressor;
	}

/**
Loads the VJ decompressor DLL and creates a decompressor object.

@return VJ decompressor object
*/
CVJDeCompressorIf* CPppBinderIp4::LoadVJDeCompressorL()
	{
	CVJCompFactory*		Factory=NULL;
	CVJDeCompressorIf*	VJDeCompressor;

	Factory = (CVJCompFactory*)FindPppFactoryL(KVjComp, TUid::Uid(KSharedLibraryUidValue), *iVJDeCompressorCon);
	CleanupStack::PushL(TCleanupItem(CNifFactory::Cleanup, Factory));
	VJDeCompressor = Factory->NewVJDeCompressorL(iMaxVJSlots);
	CleanupStack::PopAndDestroy(); // Close extra reference on Factory

	return VJDeCompressor;
	}


/**
Applies options in a received config request (that was ACK'd).
Upcall from the FSM.

@param aRequestList NCPIP options to use
*/
void CPppBinderIp4::FsmApplyConfigRequest(RPppOptionList& aRequestList)
	{
	TMBufPktQIter iter(aRequestList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			//
			// Load the VJ DLL
			//
			if (iVJCompressor != NULL)
				{
				iVJCompressor->Close();
				iVJCompressor = NULL;
				}

			TRAPD( Ret, iVJCompressor = LoadVJCompressorL();)
			if (Ret != KErrNone)
				{
				//
				// Couldn't load the DLL for some reason!!!
				// We're in trouble now, the only reason I can
				// forsee is Out of memory PRR 18/3/98
				//
				FsmAbort(Ret);
				}
			break;
		case KPppIpcpOptIpAddress:
			iRemoteAddr = BigEndian::Get32(opt.ValuePtr());
			if (iRemoteAddr == KInetAddrNone)
				FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptIpAddresses:
			iRemoteAddr = BigEndian::Get32(opt.ValuePtr());
			if (iRemoteAddr == KInetAddrNone)
				FsmAbort(KErrNotReady);
			iLocalAddr = BigEndian::Get32(opt.ValuePtr()+4);
			if (iLocalAddr == KInetAddrNone)
				FsmAbort(KErrNotReady);
			break;
//		case KPppIpcpOptPrimaryDnsAddress:
//			iPrimaryDns = BigEndian::Get32(opt.ValuePtr());
//			break;
//		case KPppIpcpOptSecondaryDnsAddress:
//			iSecondaryDns = BigEndian::Get32(opt.ValuePtr());
//			break;
		default:
			break;
			}
		}
	}

/**
Applies the options received in a Config Ack
Upcall from the FSM.

@param aReplyList NCPIP options to use
*/
void CPppBinderIp4::FsmRecvConfigAck(RPppOptionList& aReplyList)
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			if (iVJDecompressor != NULL)
				{
				iVJDecompressor->Close();
				iVJDecompressor = NULL;
				}

			TRAPD(ret, iVJDecompressor = LoadVJDeCompressorL();)
			if (ret != KErrNone)
				{
				//
				// Couldn't load the DLL for some reason!!!
				//
				iPppLcp->Stop(ret, MNifIfNotify::EDisconnect);
				}
			break;
		case KPppIpcpOptIpAddress:
			iLocalAddr = BigEndian::Get32(opt.ValuePtr());
			if (iLocalAddr == KInetAddrNone)
				FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iPrimaryDns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iSecondaryDns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptPrimaryNbnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iPrimaryNbns = BigEndian::Get32(opt.ValuePtr());
			break;
		case KPppIpcpOptSecondaryNbnsAddress:
			if(BigEndian::Get32(opt.ValuePtr()))
			    iSecondaryNbns = BigEndian::Get32(opt.ValuePtr());
			break;
		default:
			break;
			}
		}
	}

/**
Modifies request after receiving a Config Nak
Upcall from the FSM.

@param aReplyList NAK'd NCPIP options
@param aReqList The associated original request to be modified
*/
void CPppBinderIp4::FsmRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList)
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptIpAddress:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			aReqList.ReplaceOption(opt);
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			aReqList.ReplaceOption(opt);
			break;
		default:
			aReqList.ReplaceOption(opt);
			break;
			}
		}
	}

/**
Modifies request after receiving a Config Reject
Upcall from the FSM.

@param aReplyList NAK'd NCPIP options
@param aReqList The associated original request to be modified
*/
void CPppBinderIp4::FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList)
	{
	TMBufPktQIter iter(aReplyList);
	RPppOption opt;
	while (opt = iter++, !opt.IsEmpty())
		{
		switch (opt.OptType())
			{
		case KPppIpcpOptIpCompressionProtocol:
			aReqList.RemoveOption(opt);
			break;
		case KPppIpcpOptIpAddress:
			aReqList.RemoveOption(opt);
			FsmAbort(KErrNotReady);
			break;
		case KPppIpcpOptPrimaryDnsAddress:
			aReqList.RemoveOption(opt);
			break;
		case KPppIpcpOptSecondaryDnsAddress:
			aReqList.RemoveOption(opt);
			break;
		default:
			aReqList.RemoveOption(opt);
			break;
			}
		}
	}

void CPppBinderIp4::KillProtocol()
	{
	// This call would cause all NCPs to fail to connect
	// iPppLcp->Stop(KErrCouldNotConnect,MNifIfNotify::EDisconnect);

	// This shuts down only this NCP
	FsmAbort(KErrCouldNotConnect);
	}

/**
Received an unrecognised opcode - has a default implementation
*/
TBool CPppBinderIp4::FsmRecvUnknownCode(TUint8 /*aCode*/, TUint8 /*aId*/, TInt /*aLength*/, RMBufChain& /*aPacket*/)
	{
	return EFalse;
	}

//


/**
Receives an IP packet from the lower layer.

@param aPacket MBuf chain containing packet
*/
void CPppBinderIp4::RecvIp(RMBufChain& aPacket)
	{
	if (iUpperReceiver)
		iUpperReceiver->Process(aPacket);
	else
		aPacket.Free();
	}


//
// All three of these are grouped together because of the way
// this was intially designed.
// Kill Protocol is called because the protocol was rejected
// in the case of NCPIP I can't see this happening, but if it does
// we need to kill off PPP
//
void CPppBinderIp4::IpKillProtocol()
	{
    __FLOG_2(_L8("this:%08x\tCPppBinderIp4::IpKillProtocol() - disconnecting because access was denied"), this, &iPppNifSubConnectionFlow);

	Flow()->FlowDown(KErrAccessDenied, MNifIfNotify::EDisconnect);
	return;
	}

void CPppBinderIp4::VjCompTcpKillProtocol()
	{
	return;
	}

void CPppBinderIp4::VjUncompTcpKillProtocol()
	{
	return;
	}


void CPppBinderIp4::IpFrameError()
	{
	return;
	}

void CPppBinderIp4::VjCompTcpFrameError()
	{
	if (iVJDecompressor)
		{
		LOG(iPppLcp->iLogger->Printf(_L("CRC CRC CRC \n\n\n\n"));)
		iVJDecompressor->CRCError();
		}
	return;
	}

void CPppBinderIp4::VjUncompTcpFrameError()
	{
	return;
	}

/**
Receives a VJ-compressed IP packet from the lower layer.

@param aPacket MBuf chain containing packet
*/
void CPppBinderIp4::RecvVjCompTcp(RMBufChain& aPacket)
	{
	if (iVJDecompressor)
		{
		if (!iVJDecompressor->DecompVJComp(aPacket))
			{
			aPacket.Free();
			}
		else
			{
			RecvIp(aPacket);
			}
		}
	else
		{
		aPacket.Free();
		}
	return;
	}

/**
Receives a VJ-uncompressed IP packet from the lower layer.

@param aPacket MBuf chain containing packet
*/
void CPppBinderIp4::RecvVjUncompTcp(RMBufChain& aPacket)
	{
	if (iVJDecompressor)
		{
		if (!iVJDecompressor->DecompVJUncomp(aPacket))
			{
			aPacket.Free();
			}
		else
			{
			RecvIp(aPacket);
			}
		}
	else
		{
		aPacket.Free();
		}
	}

void CPppBinderIp4::FsmTerminationPhaseComplete()
	{
	}

/**
Queues a packet as-is onto the send queue and sends it out the link
via an asynchronous callback function.

@param aPacket MBuf chain containing packet
@param aProtocol PPP protocol number
@return 1 for a successful send, 0 to tell upper layer to flow off
*/
MLowerDataSender::TSendResult CPppBinderIp4::SendProtFrame(RMBufChain& aPacket, TUint aProtocol)
	{

	RMBufPktInfo* info = RMBufPacket::PeekInfoInChain(aPacket);

	TPppAddr addr;
	addr.SetProtocol(aProtocol);
	info->iDstAddr = addr;

	iSendQ.Append(aPacket);
	iSendCallBack->CallBack();

	if (!FsmIsThisLayerOpen() || !iLowerFlowOn)
		{
		iUpperFlowOn = ESendBlocked;
		}

	return iUpperFlowOn;
	}

TInt CPppBinderIp4::Notification(TAgentToNifEventType aEvent)
	{
	__ASSERT_DEBUG(aEvent==EAgentToNifEventTypeModifyInitialTimer, PppPanic(EPppPanic_IncorrectNcpNotif));
    if(aEvent==EAgentToNifEventTypeModifyInitialTimer)
        {
        ChangeTimers(ETrue);
        }
	return KErrNone;
	}
