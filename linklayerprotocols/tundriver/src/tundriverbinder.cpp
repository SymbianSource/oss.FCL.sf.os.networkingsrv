/**
*   Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
*   All rights reserved.
*   This component and the accompanying materials are made available
*   under the terms of "Eclipse Public License v1.0"
*   which accompanies this distribution, and is available
*   at the URL "http://www.eclipse.org/legal/epl-v10.html".
*   
*   Initial Contributors:
*   Nokia Corporation - initial contribution.
*   
*   Contributors:
*
*   Description:
*   Ipv4 and Ipv6 Binder configuration source file.
* 
*
*/

/**
 @file tundriverbinder.cpp
 @internalTechnology
 */

#include <e32std.h>
#include <eui_addr.h>
#include <ip4_hdr.h>
#include <udp_hdr.h>
#include <in_chk.h>
#include <in_iface.h>
#include <comms-infras/nifif.h>
#include <in_sock.h>
#include <flow.h>
#include "tundriverbinder.h"
#include "tundriverprovision.h"

using namespace ESock;

#ifdef _DEBUG
_LIT8(KNif,"TunDriver");
_LIT8(KBinder4,"Binder4");
#ifdef IPV6SUPPORT
_LIT8(KBinder6,"Binder6");
#endif
#endif

//////////////////////
// CTunDriverBinder //
//////////////////////

CTunDriverBinder::CTunDriverBinder(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow) : iTunDriverSubConnectionFlow(aTunDriverSubConnectionFlow)
/*
* Default constructor for Binder Base Class.
*/
        {
        }

MLowerDataSender* CTunDriverBinder::Bind(MUpperDataReceiver& /*aUpperReceiver*/, MUpperControl& /*aUpperControl*/)
/**
* Virtual Function, Implementation is binder specific (IPv4 or IPv6)
* If This Bind function is called, then there is an error in binding.
* @param Not used here
* @return NULL.
*/
    {
    return NULL;
    }

void CTunDriverBinder::Unbind(MUpperDataReceiver& /*aUpperReceiver*/, MUpperControl& /*aUpperControl*/)
/**
* Virtual Function, Implementation is binder specific (IPv4 or IPv6)
* @param Not used here
*/
    {
    }

void CTunDriverBinder::SetPort(TUint aPort)
/**
* CTunDriverBinder::SetPort is used to set port common for Ip4Binder and Ip6Binder
* @param aPort
* @return
*/
    {
    iPort = aPort;
    }

TUint CTunDriverBinder::GetPort()
/**
* CTunDriverBinder::GetPort is used to obtain port common for Ip4Binder and Ip6Binder
* @param Not used here
* @return iPort
*/
    {
    return iPort;
    }

TBool CTunDriverBinder::MatchesUpperControl(ESock::MUpperControl* /*aUpperControl*/) const
/**
* Virtual Function, Implementation is binder specific (IPv4 or IPv6)
* If This MatchesUpperControl function is called, then there is an error in binding.
* @param Not used here
* @return EFalse.
*/
    {
    return EFalse;
    }

CTunDriverBinder4* CTunDriverBinder4::NewL(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow)
/**
* CTunDriverBinder4::NewL
* This function is invoked by the flow. This CTunDriverBinder4 is IPv4 specific.
* @param CTunDriverSubConnectionFlow
* @return CTunDriverSubConnectionFlow.
*/
    {
    CTunDriverBinder4* self = new (ELeave)CTunDriverBinder4(aTunDriverSubConnectionFlow);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CTunDriverBinder4::ConstructL()
/**
* CTunDriverBinder4::ConstructL is the second-phase constructor.
* This function is invoked by NewL.
* This function initializes the asynchronous callback functions for sending and receiving packets.
* @param 
* @return.
*/
    {
    TCallBack scb(SendCallBack, this);
    iSendCallBack = new(ELeave) CAsyncCallBack(scb, EPriorityNormal);

    TCallBack rcb(RecvCallBack, this);
    iRecvCallBack = new(ELeave) CAsyncCallBack(rcb, EPriorityNormal);
    }

CTunDriverBinder4::CTunDriverBinder4(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow) : CTunDriverBinder(aTunDriverSubConnectionFlow),iTunDriverSubConnectionFlow(aTunDriverSubConnectionFlow)
/**
* CTunDriverBinder4::CTunDriverBinder4 is the default constructor.
* This function is invoked by NewL.
* This function initializes localaddress to 0.0.0.0.
* @param 
* @return.
*/
        {
        __FLOG_OPEN(KNif, KBinder4);
        iLocalAddress = KInetAddrNone;
        __FLOG_3(_L8("CTunDriverBinder4 %08x:\tCTunDriverBinder4(CTunDriverSubConnectionFlow& %08x): iLocalAddress %08x"), this, &aTunDriverSubConnectionFlow, iLocalAddress.Address());
        }

CTunDriverBinder4::~CTunDriverBinder4()
/**
* CTunDriverBinder4::CTunDriverBinder4 is the default destructor.
* This function frees the RMBufPkt queues and 
* deletes the asynchrounous call back pointers initialized at the time of construction.
* @param 
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder4 %08x:\t~CTunDriverBinder4()"), this);
    iRecvQ.Free();
    iSendQ.Free();
    delete iRecvCallBack;
    delete iSendCallBack;
    }

MLowerDataSender* CTunDriverBinder4::Bind(MUpperDataReceiver& aUpperReceiver, MUpperControl& aUpperControl)
/**
* CTunDriverBinder4::Bind Binds upper CommsFramwork Protocol to this CommsFramwork Protocol.
* @param aUpperReceiver A pointer to Upper layer Receive class
* @param aUpperControl A pointer to Upper layer control class
* @return this.
*/
    {
    __FLOG_3(_L8("CTunDriverBinder4 %08x:\tBind(aUpperReceiver %08x, aUpperControl %08x)"), this, &aUpperReceiver, &aUpperControl);
    iUpperReceiver = &aUpperReceiver;
    iUpperControl = &aUpperControl;
    return this;
    }

void CTunDriverBinder4::Unbind(MUpperDataReceiver& /*aUpperReceiver*/, MUpperControl& /*aUpperControl*/)
/**
* CTunDriverBinder4::UnBind UnBinds the assocication of CommsFramwork Protocol from this CommsFramwork Protocol.
* @param aUpperReceiver A pointer to Upper layer Receive class
* @param aUpperControl A pointer to Upper layer control class
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder4 %08x:\tUnbind()"), this);
    iUpperReceiver = NULL;
    iUpperControl = NULL;
    }

void CTunDriverBinder4::StartSending()
/**
* CTunDriverBinder4::StartSending is signaled to the upperlayer that Binder is ready to send packets.
* This function will be called only once when the interface is started.
* @param 
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder4 %08x:\tBinderReady()"), this);
    iUpperControl->StartSending();
    }

TBool CTunDriverBinder4::MatchesUpperControl(ESock::MUpperControl* aUpperControl) const
/**
* CTunDriverBinder4::MatchesUpperControl returns whether this binder is associated with the
* MUpperControl object passed as argument.
* This function will be called while Unbinding the flow.
* @param aUpperControl upper layer to match against
* @return ETrue on a match else EFalse.
*/
    {
    return (aUpperControl == iUpperControl);
    }

MLowerDataSender::TSendResult CTunDriverBinder4::Send(RMBufChain& aData)
/**
* CTunDriverBinder4::Send places the stack packet in SendQueue and breaks the call.
* Then asynchrounous callback function is invoked.
* @param aData MBuf chain containing data to send
* @return ESendAccepted.
*/
    {
    iSendQ.Append(aData);
    iSendCallBack->CallBack();
    return ESendAccepted;
    }

TInt CTunDriverBinder4::GetConfig(TBinderConfig& aConfig)
/**
* CTunDriverBinder4::GetConfig populates the interface IPv4 configuration information.
* This function is invoked by the upperlayer
* @param aConfig structure.
* @return KErrNone if IPv4 binderconfiguration exists.
* @return KErrNotSupported otherwise. 
*/ 
    {
    TBinderConfig4* config = TBinderConfig::Cast<TBinderConfig4>(aConfig);

    if(config == NULL)
        {
        return KErrNotSupported;
        }
    const TTunDriverIp4Provision* ip4Provision = Flow()->Ip4Provision();
    // Setup config
    iLocalAddress = ip4Provision->LocalAddr();
    config->iInfo.iFeatures = KIfCanBroadcast | KIfCanMulticast;
    config->iInfo.iMtu = KMTU;
    config->iInfo.iRMtu = KMTU;
    config->iInfo.iSpeedMetric = KSpeedMetric;
    config->iFamily = KAfInet;

    __FLOG_2(_L8("CTunDriverBinder4 %08x:\tGetConfig(): iLocalAddress %08x"), this, iLocalAddress.Address());
    config->iAddress.SetAddress(iLocalAddress.Address());


    TInetAddr address = ip4Provision->DefGateway();
    config->iDefGate.SetAddress(address.Address());
    // primary DNS, just make same as default gateway
    address = ip4Provision->PrimaryDns();
    config->iNameSer1.SetAddress(address.Address());
    // secondary DNS
    address = ip4Provision->SecondaryDns();
    config->iNameSer2.SetAddress(address.Address());
    return KErrNone;
    }

TInt CTunDriverBinder::Control(TUint aLevel, TUint aName, TDes8& aOption)
/**
* CTunDriverBinder::Control is to check wether the packet is intended for tundriver or not.
* Called from the upperlayer to identify the special control functionality.
* Also the sets the port number.
* @param aLevel
* @param aName
* @param aOption
* @return KErrNone if packet is intended for Ipv4 tundriver.
* @return KErrNotSupported otherwise. 
*/ 
    {
    __FLOG_3(_L("CTunDriverBinder %08x:\tControl(aLevel %x, aName %x)"), this, aLevel, aName);
    if((aLevel == KSolInetIp) && (aName == KSoTunnelPort))
        {
        TInetAddr& localAddress = *(TInetAddr*) aOption.Ptr();
        SetPort(localAddress.Port());
        return KErrNone;
        }
    return KErrNotSupported;
    }

TInt CTunDriverBinder4::GetName(TDes& aName)
/**
* CTunDriverBinder4::GetName is to retreive the Binder4 Name.
* Called from the upperlayer. aName is filled with Binder4 Name. 
* @param aName
* @return KErrNone if packet is intended for Ipv4 tundriver.
* @return KErrNotSupported otherwise. 
*/
    {
    aName.Format(_L("TunDriver4[0x%08x]"), this);
    __FLOG_2(_L("CTunDriverBinder4 %08x:\tGetName(): %S"), this, &aName);
    return KErrNone;
    }

TInt CTunDriverBinder4::RecvCallBack(TAny* aCProtocol)
/**
* CTunDriverBinder4::RecvCallBack is asynchronous callback function to send the packets to IP Layer.
* Will be invoked whenever there is a packet in the RMBufPkt Receive Buffer.
* @param aCProtocol contains the this object.
* @return KErrNone 
*/
    {
    ((CTunDriverBinder4*)aCProtocol)->DoProcess();
    return KErrNone;
    }

TInt CTunDriverBinder4::SendCallBack(TAny* aCProtocol)
/**
* CTunDriverBinder4::SendCallBack is asynchronous callback function to send the packets to IP Layer.
* Will be invoked whenever there is a packet in the RMBufPkt Send Buffer.
* DoSend will process the packets if required and place the packet in the RMBufPkt Receive Queue 
* @param aCProtocol contains the this object.
* @return KErrNone 
*/
    {
    ((CTunDriverBinder4*)aCProtocol)->DoSend();
    return KErrNone;
    }

void CTunDriverBinder4::DoSend()
/**
* CTunDriverBinder4::DoSend will retreive the packet from RMBufPkt Send Buffer.
* Process the packets if required and puts the packet back in Receive Buffer.
* Will be invoked by CTunDriverBinder4::SendCallback
* @param 
* @return  
*/
    {
    RMBufPacket send;
    RMBufPacket recv;
    while (iSendQ.Remove(send))
        {
        TunnelProcessing(send,recv);
        iRecvQ.Append(recv);
        iRecvCallBack->CallBack();
        send.Free();
        }
    }

void CTunDriverBinder4::DoProcess()
/**
* CTunDriverBinder4::DoProcess will retreive the packet from RMBufPkt Receive Buffer.
* send the packet to IP Layer for processing the packet.
* Will be invoked by CTunDriverBinder4::ReceiveCallback
* @param 
* @return  
*/
    {
    RMBufPacket packet;
    while (iRecvQ.Remove(packet))
        {
        iUpperReceiver->Process(packet);
        }
    }

void CTunDriverBinder4::TunnelProcessing(RMBufPacket& aPacket, RMBufPacket& aRecv )
/**
* CTunDriverBinder4::TunnelProcessing will encapsulate with IPv4 Header if the packet is to be tunneled.
* IP Number and port number will be configure by TUN Client.
* If the IPPacket contains TOS Option set with 192, Packet is encapsulated else packet will be send as is.
* Will be invoked by CTunDriverBinder4::DoSend
* @param aPacket contains the raw packet.
* @param aRecv will contain the payload and the encapsulated IPv4 Header.
* @return  
*/
    {
    TInt  ret;
    TUint origPktLen;

    TInet6HeaderIP4* lip4 = (TInet6HeaderIP4*) aPacket.First()->Next()->Ptr();
    TInt outerHdrLen = TInet6HeaderIP4::MinHeaderLength()
                       + TInet6HeaderUDP::MinHeaderLength();
    
    if((lip4->TOS() & KTUNDriverTos) == KTUNDriverTos)
        {
        RMBufChain outerHdr;
        aRecv.Assign(aPacket);
        RMBufPktInfo *info = aRecv.Unpack();
        aRecv.SetInfo(info);
        
        origPktLen = aRecv.Length();
        TRAP(ret, outerHdr.AllocL(outerHdrLen));
        if (ret != KErrNone)
            {
            RDebug::Printf("\nVirtual tunnel nif: tunnel processing: Error in Allocating the outer IP header.");
            aRecv.Free();
            }

        aRecv.Prepend(outerHdr);
        info->iLength += outerHdrLen;

        TInet6Checksum<TInet6HeaderUDP> outerUdp(aRecv, 20);
        if (outerUdp.iHdr == NULL)
            {
            RDebug::Printf("\nVirtual tunnel nif: tunnel processing: Error in Allocating the outer UDP header.");
            aRecv.Free();
            }

        outerUdp.iHdr->SetSrcPort(GetPort());
        outerUdp.iHdr->SetDstPort(GetPort());
        outerUdp.iHdr->SetLength(origPktLen + TInet6HeaderUDP::MinHeaderLength());
        outerUdp.iHdr->SetChecksum(0);       // Compute checksum

        TInet6Checksum<TInet6HeaderIP4> outerIP(aRecv);
        outerIP.iHdr->Init();
        outerIP.iHdr->SetHeaderLength(TInet6HeaderIP4::MinHeaderLength());
        outerIP.iHdr->SetTotalLength(origPktLen + outerHdrLen);
        outerIP.iHdr->SetTOS(1);
        outerIP.iHdr->SetProtocol(KProtocolInetUdp);
        outerIP.iHdr->SetSrcAddr(iLocalAddress.Address()) ;
        outerIP.iHdr->SetDstAddr(iLocalAddress.Address());
        outerIP.ComputeChecksum();
        aRecv.Pack();
        }
    }
#ifdef IPV6SUPPORT
CTunDriverBinder6* CTunDriverBinder6::NewL(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow)
/**
* CTunDriverBinder6::NewL
* This function is invoked by the flow. This CTunDriverBinder4 is IPv4 specific.
* @param CTunDriverSubConnectionFlow
* @return CTunDriverSubConnectionFlow.
*/
    {
    CTunDriverBinder6* self = new (ELeave)CTunDriverBinder6(aTunDriverSubConnectionFlow);
    CleanupStack::PushL(self);
    self->ConstructL();
    CleanupStack::Pop(self);
    return self;
    }

void CTunDriverBinder6::ConstructL()
/**
* CTunDriverBinder4::ConstructL is the second-phase constructor.
* This function is invoked by NewL.
* This function initializes the asynchronous callback functions for sending and receiving packets.
* @param 
* @return.
*/
    {
    TCallBack scb(SendCallBack, this);
    iSendCallBack = new(ELeave) CAsyncCallBack(scb, EPriorityNormal);

    TCallBack rcb(RecvCallBack, this);
    iRecvCallBack = new(ELeave) CAsyncCallBack(rcb, EPriorityNormal);
    }

CTunDriverBinder6::CTunDriverBinder6(CTunDriverSubConnectionFlow& aTunDriverSubConnectionFlow) : CTunDriverBinder(aTunDriverSubConnectionFlow),iTunDriverSubConnectionFlow(aTunDriverSubConnectionFlow)
/**
* CTunDriverBinder4::CTunDriverBinder4 is the default constructor.
* This function is invoked by NewL.
* This function initializes localaddress to 0.0.0.0.
* @param 
* @return.
*/        
    {
    __FLOG_OPEN(KNif, KBinder6);
    iLocalAddress.SetAddress(KInet6AddrNone);
    __FLOG_3(_L8("CTunDriverBinder6 %08x:\tCTunDriverBinder6(CTunDriverSubConnectionFlow& %08x): iLocalAddress %08x"), this, &aTunDriverSubConnectionFlow, iLocalAddress.Address());
    }

CTunDriverBinder6::~CTunDriverBinder6()
/**
* CTunDriverBinder4::CTunDriverBinder4 is the default destructor.
* This function frees the RMBufPkt queues and 
* deletes the asynchrounous call back pointers initialized at the time of construction.
* @param 
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder6 %08x:\t~CTunDriverBinder6()"), this);
    iRecvQ.Free();
    iSendQ.Free();
    delete iRecvCallBack;
    delete iSendCallBack;
    }

MLowerDataSender* CTunDriverBinder6::Bind(MUpperDataReceiver& aUpperReceiver, MUpperControl& aUpperControl)
/**
* CTunDriverBinder4::Bind Binds upper CommsFramwork Protocol to this CommsFramwork Protocol.
* @param aUpperReceiver A pointer to Upper layer Receive class
* @param aUpperControl A pointer to Upper layer control class
* @return this.
*/
    {
    __FLOG_3(_L8("CTunDriverBinder6 %08x:\tBind(aUpperReceiver %08x, aUpperControl %08x)"), this, &aUpperReceiver, &aUpperControl);
    iUpperReceiver = &aUpperReceiver;
    iUpperControl = &aUpperControl;
    return this;
    }

void CTunDriverBinder6::Unbind(MUpperDataReceiver& /*aUpperReceiver*/, MUpperControl& /*aUpperControl*/)
/**
* CTunDriverBinder4::UnBind UnBinds the assocication of CommsFramwork Protocol from this CommsFramwork Protocol.
* @param aUpperReceiver A pointer to Upper layer Receive class
* @param aUpperControl A pointer to Upper layer control class
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder6 %08x:\tUnbind()"), this);
    iUpperReceiver = NULL;
    iUpperControl = NULL;
    }

void CTunDriverBinder6::StartSending()
/**
* CTunDriverBinder4::StartSending is signaled to the upperlayer that Binder is ready to send packets.
* This function will be called only once when the interface is started.
* @param 
* @return.
*/
    {
    __FLOG_1(_L8("CTunDriverBinder6 %08x:\tBinderReady()"), this);
    iUpperControl->StartSending();
    }

TBool CTunDriverBinder6::MatchesUpperControl(ESock::MUpperControl* aUpperControl) const
/**
* CTunDriverBinder4::MatchesUpperControl returns whether this binder is associated with the
* MUpperControl object passed as argument.
* This function will be called while Unbinding the flow.
* @param aUpperControl upper layer to match against
* @return ETrue on a match else EFalse.
*/
    {
    return (aUpperControl == iUpperControl);
    }

MLowerDataSender::TSendResult CTunDriverBinder6::Send(RMBufChain& aData)
/**
* CTunDriverBinder4::Send places the stack packet in SendQueue and breaks the call.
* Then asynchrounous callback function is invoked.
* @param aData MBuf chain containing data to send
* @return ESendAccepted.
*/
    {
    iSendQ.Append(aData);
    iSendCallBack->CallBack();
    return ESendAccepted;
    }

TInt CTunDriverBinder6::GetConfig(TBinderConfig& aConfig)
/**
* CTunDriverBinder4::GetConfig populates the interface IPv4 configuration information.
* This function is invoked by the upperlayer
* @param aConfig structure.
* @return KErrNone if IPv4 binderconfiguration exists.
* @return KErrNotSupported otherwise. 
*/
    {
    TBinderConfig6* config = TBinderConfig::Cast<TBinderConfig6>(aConfig);

    if(config == NULL)
        {
        return KErrNotSupported;
        }
    const TTunDriverIp6Provision* ip6Provision = Flow()->Ip6Provision();
    // Setup config
    iLocalAddress = ip6Provision->LocalAddr();
    config->iInfo.iFeatures = KIfCanBroadcast | KIfCanMulticast;
    config->iInfo.iMtu = KMTU;
    config->iInfo.iRMtu = KMTU;
    config->iInfo.iSpeedMetric = KSpeedMetric;
    config->iFamily = KAfInet6;

    __FLOG_2(_L8("CTunDriverBinder6 %08x:\tGetConfig(): iLocalAddress %08x"), this, iLocalAddress.Address());

    // primary DNS, just make same as default gateway
    TInetAddr address = ip6Provision->PrimaryDns();
    config->iNameSer1 = address;
    // secondary DNS
    address = ip6Provision->SecondaryDns();
    config->iNameSer2 = address;
    return KErrNone;
    }

TInt CTunDriverBinder6::GetName(TDes& aName)
/**
* CTunDriverBinder4::GetName is to retreive the Binder4 Name.
* Called from the upperlayer. aName is filled with Binder4 Name. 
* @param aName
* @return KErrNone if packet is intended for Ipv4 tundriver.
* @return KErrNotSupported otherwise. 
*/
    {
    aName.Format(_L("TunDriver6[0x%08x]"), this);
    __FLOG_2(_L("CTunDriverBinder6 %08x:\tGetName(): %S"), this, &aName);
    return KErrNone;
    }

TInt CTunDriverBinder6::RecvCallBack(TAny* aCProtocol)
/**
* CTunDriverBinder4::RecvCallBack is asynchronous callback function to send the packets to IP Layer.
* Will be invoked whenever there is a packet in the RMBufPkt Receive Buffer.
* @param aCProtocol contains the this object.
* @return KErrNone 
*/
    {
    ((CTunDriverBinder6*)aCProtocol)->DoProcess();
    return KErrNone;
    }

TInt CTunDriverBinder6::SendCallBack(TAny* aCProtocol)
/**
* CTunDriverBinder4::SendCallBack is asynchronous callback function to send the packets to IP Layer.
* Will be invoked whenever there is a packet in the RMBufPkt Send Buffer.
* DoSend will process the packets if required and place the packet in the RMBufPkt Receive Queue 
* @param aCProtocol contains the this object.
* @return KErrNone 
*/
    {
    ((CTunDriverBinder6*)aCProtocol)->DoSend();
    return KErrNone;
    }

void CTunDriverBinder6::DoSend()
/**
* CTunDriverBinder4::DoSend will retreive the packet from RMBufPkt Send Buffer.
* Process the packets if required and puts the packet back in Receive Buffer.
* Will be invoked by CTunDriverBinder4::SendCallback
* @param 
* @return  
*/
    {
    RMBufPacket send;
    RMBufPacket recv;
    while (iSendQ.Remove(send))
        {
        TunnelProcessing(send,recv);
        iRecvQ.Append(recv);
        iRecvCallBack->CallBack();
        send.Free();
        }
    }

void CTunDriverBinder6::DoProcess()
/**
* CTunDriverBinder4::DoProcess will retreive the packet from RMBufPkt Receive Buffer.
* send the packet to IP Layer for processing the packet.
* Will be invoked by CTunDriverBinder4::ReceiveCallback
* @param 
* @return  
*/
    {
    RMBufPacket packet;
    while (iRecvQ.Remove(packet))
        {
        iUpperReceiver->Process(packet);
        }
    }

void CTunDriverBinder6::TunnelProcessing(RMBufPacket& aPacket, RMBufPacket& aRecv )
/**
* CTunDriverBinder4::TunnelProcessing will encapsulate with IPv4 Header if the packet is to be tunneled.
* IP Number and port number will be configure by TUN Client.
* If the IPPacket contains TOS Option set with 192, Packet is encapsulated else packet will be send as is.
* Will be invoked by CTunDriverBinder4::DoSend
* @param aPacket contains the raw packet.
* @param aRecv will contain the payload and the encapsulated IPv4 Header.
* @return  
*/
    {
    TInt  ret;
    TUint origPktLen;

    TInet6HeaderIP* lip6 = (TInet6HeaderIP*) aPacket.First()->Next()->Ptr();
    TInt outerHdrLen = TInet6HeaderIP4::MinHeaderLength()
                       + TInet6HeaderUDP::MinHeaderLength();
    
    if((lip6->HopLimit() & KTUNDriverTos) == KTUNDriverTos)        
        {
        RMBufChain outerHdr;
        aRecv.Assign(aPacket);
        RMBufPktInfo *info = aRecv.Unpack();
        aRecv.SetInfo(info);
        
        origPktLen = aRecv.Length();
        TRAP(ret, outerHdr.AllocL(outerHdrLen));
        if (ret != KErrNone)
            {
            RDebug::Printf("\nVirtual tunnel nif: tunnel processing: Error in Allocating the outer IP header.");
            aRecv.Free();
            }

        TInet6HeaderIP *hdrBuf = (TInet6HeaderIP*) outerHdr.First()->Buffer();
        aRecv.Prepend(outerHdr);
        info->iLength += outerHdrLen;

        TInet6Checksum<TInet6HeaderUDP> outerUdp(aRecv, 20);
        if (outerUdp.iHdr == NULL)
            {
            RDebug::Printf("\nVirtual tunnel nif: tunnel processing: Error in Allocating the outer UDP header.");
            aRecv.Free();
            }

        outerUdp.iHdr->SetSrcPort(GetPort());
        outerUdp.iHdr->SetDstPort(GetPort());
        outerUdp.iHdr->SetLength(origPktLen + TInet6HeaderUDP::MinHeaderLength());
        outerUdp.iHdr->SetChecksum(0);

        TInet6Checksum<TInet6HeaderIP> outerIP(aRecv);
        outerIP.iHdr->Init();
        outerIP.iHdr->SetHopLimit(1);
        outerIP.iHdr->SetNextHeader(KProtocolInetUdp);
        outerIP.iHdr->SetSrcAddr(iLocalAddress.Ip6Address());
        outerIP.iHdr->SetDstAddr(iLocalAddress.Ip6Address());
        aRecv.Pack();
        }
    }
    
CTunDriverSubConnectionFlow* CTunDriverBinder6::Flow()
    {
    return &iTunDriverSubConnectionFlow;
    }

#endif
