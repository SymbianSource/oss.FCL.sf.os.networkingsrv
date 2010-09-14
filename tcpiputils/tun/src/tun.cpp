// Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
// implementation of Inbound and Outbound hook
// 
//

/**
 @file
 @internalTechnology 
 */

#include <udp_hdr.h>
#include <in_chk.h>
#include <es_prot_internal.h>
#include "tun.pan"
#include "tun.h"

_LIT(KProtocolTunName, "tun");

void Panic(TTunPanic aPanic)
    {
    User::Panic(_L("Tun panic"), aPanic);
    }


// ============================ MEMBER FUNCTIONS ==============================

// ----------------------------------------------------------------------------
// CProtocolTun::CProtocolTun
// C++ default constructor can NOT contain any code, that
// might leave.
// ----------------------------------------------------------------------------
//
CProtocolTun::CProtocolTun ()
    {}

// Destructor
CProtocolTun::~CProtocolTun ()
    {
    NetworkService()->Protocol()->Unbind((CProtocolBase*)iFlowinfo,0);
    }

// ----------------------------------------------------------------------------
// CProtocolTun::NewL
// ----------------------------------------------------------------------------
//
CProtocolTun* CProtocolTun::NewL ()
    {
    CProtocolTun* self = new (ELeave) CProtocolTun();
    CleanupStack::PushL(self);
    self -> ConstructL ();
    CleanupStack::Pop();
    return self;
    }

// ----------------------------------------------------------------------------
// CProtocolTun::ConstructL
// Initializes the CProtocolTun
// ----------------------------------------------------------------------------
//
void CProtocolTun::ConstructL ()
    {
    iFlowinfo = new (ELeave) CTunFlowInfo();
    iSapInstance = new (ELeave) CSapTun();
    iSapInstance->iFlowInfo= iFlowinfo;
    iSapInstance->iProtocol=this;
    }

// ----------------------------------------------------------------------------
// CProtocolTun::NetworkAttachedL
// Binds the hooks (inbound, outbound flowhook and forward) to the IP6. 
// ----------------------------------------------------------------------------
//
void CProtocolTun::NetworkAttachedL ()
    {
    // Outbound hook
    NetworkService()->BindL ((CProtocolBase*) this, BindFlowHook());
    }

// ----------------------------------------------------------------------------
// CProtocolTun::NetworkDetached
// Unbind the hooks.
// ----------------------------------------------------------------------------
//
void CProtocolTun::NetworkDetached ()
    {
    // Do Nothing
    // as the destructor does the rest 
    }

// ----------------------------------------------------------------------------
// CProtocolTun::Identify
// Provide identification information to the caller.
// ----------------------------------------------------------------------------
//
void CProtocolTun::Identify (TServerProtocolDesc & aEntry)
    {
    aEntry.iName = KProtocolTunName;
    aEntry.iAddrFamily = KAfInet | KAfInet6;
    aEntry.iSockType = KSockDatagram;
    aEntry.iProtocol = KProtocolTUN;
    aEntry.iVersion = TVersion (1, 0, 0);
    aEntry.iByteOrder = EBigEndian;
    aEntry.iServiceInfo=KSIDatagram | KSIConnectionLess;
    aEntry.iNamingServices = 0;
    aEntry.iSecurity = KSocketNoSecurity;
    aEntry.iServiceTypeInfo=0;
    aEntry.iMessageSize = 0xffff;
    aEntry.iServiceTypeInfo = ESocketSupport | EInterface;
    aEntry.iNumSockets = KUnlimitedSockets;
    }

void CProtocolTun::Identify (TServerProtocolDesc * aDesc) const
{
Identify (*aDesc);
}

// ----------------------------------------------------------------------------
// CProtocolTun::ApplyL
// This is the handler for forwarding packets.
// Udp encapsulation for the packets from the Virtual tunnel nif
// ----------------------------------------------------------------------------
//
TInt CProtocolTun::ApplyL (RMBufHookPacket& /*aPacket*/, RMBufRecvInfo& /*aInfo*/)
    {
    return KIp6Hook_PASS;
    };

// ----------------------------------------------------------------------------
// CProtocolTun:OpenL
// Outbound Flow hook Open handler for the protocol. 
// ----------------------------------------------------------------------------
//
MFlowHook *CProtocolTun::OpenL (TPacketHead& /*aHead*/, CFlowContext* aFlow)
    {
    // We are interested in this flow, let's create a new local flow instance
    // create a local copy using copy ctor and return the instance
    CTunFlowInfo *info = NULL; 
    CNifIfBase* localNifBase = aFlow->Interface();
    if(IsPortSet())
        {
        TInetAddr localAddr;
        localAddr.SetPort(iAppPortNum);
        TPckg<TInetAddr> pckgLocalAddr(localAddr);
        if(localNifBase->Control(KSolInetIp,KSoTunnelPort,pckgLocalAddr)== KErrNone)
            {
            info = new (ELeave) CTunFlowInfo(iFlowinfo);
            SetCNifBase(localNifBase);
            }
        }
    return info;
    }

// ----------------------------------------------------------------------------
// CProtocolTun::NewSAPL
// Creation of a new SAP instance 
// ----------------------------------------------------------------------------
//
CServProviderBase* CProtocolTun::NewSAPL(TUint /*aProtocol*/)
    {
#if 0
    CSapTun *nsap = new(ELeave) CSapTun();//CSapTun::GetInstanceL();
    nsap->iProtocol=this;
    nsap->iFlowInfo= iFlowinfo;
#endif
    return iSapInstance;
    }

/**
---------------------------------------------------------------------------------------
                                CSapTun
---------------------------------------------------------------------------------------

This class is derived from CServProviderBase.CSapTun is the service class for sockets
loading CProtocolTun.But here only one socket will be able to load protocol.If protocol once
loaded other socket cannot service protocol by opening socket.
 */

//CSapTun* CSapTun::iInstance = NULL;

CSapTun::CSapTun()
    { }
#if 0
CSapTun* CSapTun::GetInstanceL()
    {
    if(!iInstance)
        {
        iInstance = new (ELeave) CSapTun();
        }
    return iInstance;
    }
#endif 
TInt CSapTun::SetOption(TUint aName ,TUint aLevel ,const TDesC8& anOption)
/** 
 * This class is used to set PortNumber information to be used by the hook to perform
 * UDP encapsulation.
 * @param aName  -- KSoTunnelPort
 * @param aLevel --KSolInetIp.
 * return - KErrNone if no value is assigned else KErrPermissionDenied
 **/

    {
    TInt err = KErrNotSupported;
    if((aName == KSoTunnelPort) && (aLevel == KSolInetIp))
        {
        const TUint opt = *reinterpret_cast<const TUint*>(anOption.Ptr());
        iProtocol->SetAppPortNum(opt);
        iFlowInfo->SetAppPortNum(opt);
        err= KErrNone;
        }
    return err;
    }

TInt CSapTun::SecurityCheck(MProvdSecurityChecker* aChecker)
/**
 * Capability check for the TUN Hook sockets.
 *
 * TUN Hook sockets require the NetworkControl capability.
 *
 * @param aChecker The policy checker.
 * @return The result of the policy check.
 */
    {
    //  This method is called when a SAP is created and when a socket is transferred between sessions.  The SAP is
    //required to check whether the originating client process has enough privileges to request services from the SAP.
    //The MProvdSecurityChecker class instance is used to perform security policy checks.  The SAP may choose
    //to perform a security policy check in its SecurityCheck(...) method, or it may choose to store the
    //MProvdSecurityChecker class instance argument and perform checking later (i.e. when subsequent
    //SAP methods are called). 
    _LIT_SECURITY_POLICY_C1(KPolicyNetworkControl, ECapabilityNetworkControl);
    return aChecker->CheckPolicy(KPolicyNetworkControl, "TUN Hook Loading failed.");
    }

/*
------------------------------------------------------------------------------------------

                        SAP UNUSED FUNTION SECTION
------------------------------------------------------------------------------------------

SAP definion which are not being used.These functions are not doing anything instead they are 
returning nothing from it.

 */

void CSapTun::Ioctl(TUint /*level*/,TUint /*name*/,TDes8*/*anOption*/)
    {}

void CSapTun::Start()
    {}

void CSapTun::Shutdown(TCloseType /*option*/)
    {}

void CSapTun::LocalName(TSockAddr& /*anAddr*/) const
{}

TInt CSapTun::SetLocalName(TSockAddr& /*anAddr*/)
    {
    return KErrNotSupported;
    }

void CSapTun::RemName(TSockAddr& /*anAddr*/) const 
{}

TInt CSapTun::SetRemName(TSockAddr& /*anAddr*/) 
    { 
    return KErrNotSupported;
    }

TInt CSapTun::GetOption(TUint /*aLevel*/, TUint /*aName*/, TDes8& /*aOption*/)const
/** 
 * This implements GetOption method for Napt specific service provider.
 * @param aLevel
 * @param aName
 * @param anOption
 * @return KErrNone in case of success
 **/
{
return KErrNone;
}


void CSapTun::ActiveOpen()
    {}

TInt CSapTun::PassiveOpen(TUint /*aQueSize*/)
    {
    return KErrNotSupported;
    }

void CSapTun::Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/)
    {}

void CSapTun::AutoBind()
    {}

TInt CSapTun::PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/)
    {
    return KErrNotSupported;
    }

void CSapTun::ActiveOpen(const TDesC8& /*aConnectionData*/)
    {}

void CSapTun::CancelIoctl(TUint /*aLevel*/,TUint /*aName*/)
    {}

CSapTun::~CSapTun()
    {
    }

// TUN OUTBOUND FLOW HOOK functions.

// ----------------------------------------------------------------------------
// CTunFlowInfo::CTunFlowInfo
// ----------------------------------------------------------------------------
CTunFlowInfo::CTunFlowInfo ()
    {}

// ----------------------------------------------------------------------------
// CTunFlowInfo::~CTunFlowInfo
// ----------------------------------------------------------------------------
CTunFlowInfo::~CTunFlowInfo ()
    {}

// ----------------------------------------------------------------------------
// CTunFlowInfo::ReadyL
// The stack asks if the flow is ready to send. 
// ----------------------------------------------------------------------------
//
TInt CTunFlowInfo::ReadyL (TPacketHead& /*aHead*/)
    {
    return EFlow_READY;
    }

// ----------------------------------------------------------------------------
// CTunFlowInfo::ApplyL
// Intial stage where the CTunFlowInfo touches the outgoing packet. 
// ----------------------------------------------------------------------------
TInt CTunFlowInfo::ApplyL(RMBufSendPacket & aPacket, RMBufSendInfo & aInfo)
    {
    const TInetAddr& dest = aInfo.iDstAddr;
    const TInetAddr& src = aInfo.iSrcAddr;

    TInt protocol = aInfo.iProtocol;

    // protocolnum check is added to avoid loopin in the same hook as the 
    // source and destination address will not be changed in the RMBufSendInfo.
    if (dest.Address() == src.Address())
        {
        if (protocol == KProtocolInetIp  )
            {
            TInet6Packet<TInet6HeaderIP4> localIP(aPacket);
            TUint protocol = localIP.iHdr->Protocol();

            if (protocol == KProtocolInetUdp)
                {
                TInet6HeaderUDP* udpHdr = (TInet6HeaderUDP*) localIP.iHdr->EndPtr();
                TUint srcPort = udpHdr->SrcPort();

                if (srcPort == iAppPortNum)
                    {
                    // Ingress traffic forwarded from TUN Client Application. Trim the
                    // outer header and send the original pkt back to the ip stack.
                    TInt outerHdrLen = TInet6HeaderIP4::MinHeaderLength() + TInet6HeaderUDP::MinHeaderLength();

                    // the info length will be updated by TrimStart
                    aPacket.TrimStart(outerHdrLen);
                    
                    TInet6Packet<TInet6HeaderIP4> ip(aPacket);
                    //Update the info Address information
                    TInetAddr::Cast(aInfo.iSrcAddr).SetAddress(ip.iHdr->SrcAddr());
                    TInetAddr::Cast(aInfo.iDstAddr).SetAddress(ip.iHdr->DstAddr());

                    // restart the hook processing from the begining.
                    return KIp6Hook_DONE;
                    }
                }
            }
#ifdef IPV6SUPPORT
        else if (protocol == KProtocolInet6Ip)
            {
            TInet6Packet<TInet6HeaderIP> localIP6(aPacket);
            TInt protocol = localIP6.iHdr->NextHeader();
            if (protocol == KProtocolInetUdp)
                {
                TInet6HeaderUDP* udpHdr =
                (TInet6HeaderUDP*) localIP6.iHdr->EndPtr();
                TUint srcPort = udpHdr->SrcPort();

                if (srcPort == iAppPortNum)
                    {
                    // Ingress traffic forwarded from TUN Client Application. Trim the
                    // outer header and send the original pkt back to the ip stack.
                    TInt outerHdrLen = TInet6HeaderIP::MinHeaderLength()
                    + TInet6HeaderUDP::MinHeaderLength();

                    // the info length will be updated by TrimStart
                    aPacket.TrimStart(outerHdrLen);
                    
                    TInet6Packet<TInet6HeaderIP> ip6(aPacket);
                    //Update the info Address information
                    TInetAddr::Cast(aInfo.iSrcAddr).SetAddress(ip6.iHdr->SrcAddr());
                    TInetAddr::Cast(aInfo.iDstAddr).SetAddress(ip6.iHdr->DstAddr());

                    // restart the hook processing from the begining.
                    return KIp6Hook_DONE;
                    }
                else
                    {
                    return KIp6Hook_PASS;
                    }
                }
            }
#endif //IPV6SUPPORT
        else
            {
            Panic(ETunPanic_BadHeader);
            }
       }
    else 
        {
        // Outgoing packet from the application to the TUN Client Application
        // need to mark this packet to be handled at Forward hook.

        if (protocol == KProtocolInetIp)
            {
            TInet6Packet<TInet6HeaderIP4> ip(aPacket);
            TInt tmpTos = ip.iHdr->TOS();
            tmpTos |= KTunTos;
            ip.iHdr->SetTOS(tmpTos);
            TInet6Checksum<TInet6HeaderIP4> lIp(aPacket);
            lIp.ComputeChecksum(); // recompute checksum as TOS field is updated
            }
#ifdef IPV6SUPPORT
        else if (protocol == KProtocolInet6Ip)
            {
            TInet6Packet<TInet6HeaderIP> ip6(aPacket);
            TInt tmpTrafficClass = ip6.iHdr->TrafficClass();
            tmpTrafficClass |= KTunTos;
            ip6.iHdr->SetTrafficClass(tmpTrafficClass);
            // No need to update checksum in case of IPV6
            }
#endif //IPV6SUPPORT        
        else
            {
            Panic(ETunPanic_BadHeader);
            }
        return KIp6Hook_PASS;
        }
   
    return KIp6Hook_PASS;
    }


// ----------------------------------------------------------------------------
// CTunFlowInfo::Open
// Open the flow, keep reference count.
// ----------------------------------------------------------------------------
//

void CTunFlowInfo::Open ()
    {
    iRef++;
    }

// ----------------------------------------------------------------------------
// CTunFlowInfo::Close
// Close the flow if the reference count has reached zero. Remove the flow 
// from any list it is stored.
// ----------------------------------------------------------------------------
//
void CTunFlowInfo::Close ()
    {
    if (--iRef < 0)
        {
        delete this;
        }
    }

