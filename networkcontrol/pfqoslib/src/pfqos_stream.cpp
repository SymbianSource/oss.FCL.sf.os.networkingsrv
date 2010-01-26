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
//


#include <in_sock.h>

#include "pfqos_stream.h"

EXPORT_C CPfqosStream::CPfqosStream() : iSendBuf(0,0)
    {
    iBuf=NULL;
    }

EXPORT_C void CPfqosStream::ConstructL(TUint aBufSize)
    {
    iBuf = HBufC8::NewL(aBufSize);
    TPtr8 tmp(iBuf->Des());
    iSendBuf.Set(tmp);
    }
    
EXPORT_C CPfqosStream* CPfqosStream::NewL(TUint aBufSize)
    {
    CPfqosStream* msg = new (ELeave) CPfqosStream();
    CleanupStack::PushL(msg);
    msg->ConstructL(aBufSize);
    CleanupStack::Pop();
    return msg;
    }

EXPORT_C CPfqosStream::~CPfqosStream()
    {
    delete iBuf;
    }
    

EXPORT_C void CPfqosStream::Init(TUint8 aMsgType, TUint32 aSeq)
    {
    struct pfqos_msg msg;
    msg.pfqos_msg_version = KPfqosMsgV1;
    msg.pfqos_msg_type = aMsgType;
    msg.pfqos_msg_errno = 0;
    msg.pfqos_msg_reserved = 0;
    msg.pfqos_msg_seq = aSeq;
    msg.pfqos_msg_pid = 0;
    // All policies added by QoS API are dynamic!
    msg.pfqos_msg_options = KPfqosOptionDynamic; 
    
    iSendBuf.Zero();
    iSendBuf.SetLength(0);
    iLength = msg.pfqos_msg_len = sizeof(struct pfqos_msg) / 8;
    iSendBuf.Copy((TUint8 *)&msg, sizeof(struct pfqos_msg));
    }


EXPORT_C void CPfqosStream::AddSelector(TUint8 aProtocol, 
    const TUidType& aUid, TUint32 aPolicyType, TUint32 aIapId, 
    TUint32 aPriority, const TDesC& aName)
    {
    struct pfqos_selector ext;
    const int byte_len = sizeof(struct pfqos_selector);
    ext.pfqos_selector_len = (byte_len + 7) / 8;
    ext.pfqos_ext_type = EPfqosExtSelector;
    ext.protocol = aProtocol;
    ext.uid1 = aUid[0].iUid;
    ext.uid2 = aUid[1].iUid;
    ext.uid3 = aUid[2].iUid;
    ext.iap_id = aIapId;
    ext.policy_type = aPolicyType;
    ext.priority = aPriority;
    ext.reserved = 0;
    
    TPtr8 namePtr((TUint8*)ext.name, 0, KMaxName);
    if (namePtr.MaxLength() >= aName.Length())
        {
        namePtr.Copy(aName);
        }
    namePtr.ZeroTerminate();
    
    iSendBuf.Append((TUint8*)&ext, sizeof(ext));
    iSendBuf.AppendFill(0, ext.pfqos_selector_len * 8 - byte_len);
    iLength = (TUint16)(iLength + ext.pfqos_selector_len);
    }


EXPORT_C void CPfqosStream::AddChannel(TUint32 aChannelId)
    {
    struct pfqos_channel ext;
    const int byte_len = sizeof(pfqos_channel);
    ext.pfqos_channel_len = (byte_len + 7) / 8;
    ext.pfqos_ext_type = EPfqosExtChannel;
    ext.channel_id = aChannelId;
    
    iSendBuf.Append((TUint8*)&ext, sizeof(ext));
    iSendBuf.AppendFill(0, ext.pfqos_channel_len * 8 - byte_len);
    iLength = (TUint16)(iLength + ext.pfqos_channel_len);
    }


EXPORT_C void CPfqosStream::AddConfigFile(const TDesC& aName)
    {
    struct pfqos_config_file ext;
    
    const int byte_len = sizeof(pfqos_config_file);
    ext.pfqos_config_file_len = (byte_len + 7) / 8;
    ext.pfqos_ext_type = EPfqosExtConfigFile;
    TPtr8 nameptr((TUint8*)ext.filename, 0, KMaxFileName);
    if (nameptr.MaxLength() >= aName.Length())
        {
        nameptr.Copy(aName);
        }
    nameptr.ZeroTerminate();
    ext.reserved = 0;
    
    iSendBuf.Append((TUint8*)&ext, sizeof(ext));
    iSendBuf.AppendFill(0, ext.pfqos_config_file_len * 8 - byte_len);
    iLength = (TUint16)(iLength + ext.pfqos_config_file_len);
    }


EXPORT_C void CPfqosStream::AddQoSParameters(const TQoSParameters& 
    aParameters)
    {
    struct pfqos_flowspec ext;
    const int byte_len = sizeof(pfqos_flowspec);

    ext.pfqos_flowspec_len = (byte_len + 7) / 8;
    ext.pfqos_ext_type = EPfqosExtFlowspec;

    // Uplink parameters
    ext.uplink_bandwidth           = aParameters.GetUplinkBandwidth();
    ext.uplink_maximum_burst_size  = aParameters.GetUpLinkMaximumBurstSize();
    ext.uplink_maximum_packet_size = aParameters.GetUpLinkMaximumPacketSize();
    ext.uplink_average_packet_size = aParameters.GetUpLinkAveragePacketSize();
    ext.uplink_delay               = aParameters.GetUpLinkDelay();
    ext.uplink_priority            = static_cast< TUint16 >(
        aParameters.GetUpLinkPriority());
    
    // Downlink parameters
    ext.downlink_bandwidth           = aParameters.GetDownlinkBandwidth();
    ext.downlink_maximum_burst_size  = 
        aParameters.GetDownLinkMaximumBurstSize();
    ext.downlink_maximum_packet_size = 
        aParameters.GetDownLinkMaximumPacketSize();
    ext.downlink_average_packet_size = 
        aParameters.GetDownLinkAveragePacketSize();
    ext.downlink_delay               = aParameters.GetDownLinkDelay();
    ext.downlink_priority            = static_cast< TUint16 >(
        aParameters.GetDownLinkPriority());
    
    ext.flags = aParameters.Flags();
    ext.reserved = 0;

    // name
    ext.name.Copy(aParameters.GetName());

    iSendBuf.Append((TUint8*)&ext, sizeof(ext));
    iSendBuf.AppendFill(0, ext.pfqos_flowspec_len * 8 - byte_len);
    iLength = (TUint16)(iLength + ext.pfqos_flowspec_len);
    }

    
EXPORT_C void CPfqosStream::AddModulespec(TUint32 aProtocolId, TUint32 aFlags,
    const TDesC& aModuleName, const TDesC& aFileName, 
    const TDesC8& aConfigData)
    {
    struct pfqos_modulespec ext;
    const int byte_len = sizeof(pfqos_modulespec)+aConfigData.Length();
        
    ext.pfqos_modulespec_len = (TUint16)((byte_len + 7) / 8);
    ext.pfqos_ext_type = EPfqosExtModulespec;
    ext.protocol_id = aProtocolId;
    ext.flags = aFlags;
    ext.reserved = 0;
    TPtr8 namePtr((TUint8*)ext.name, 0, KMaxName);
    if (namePtr.MaxLength() >= aModuleName.Length())
        {
        namePtr.Copy(aModuleName);
        }

    namePtr.ZeroTerminate();
    TPtr8 fileNamePtr((TUint8*)ext.path, 0, KMaxFileName);
    if (fileNamePtr.MaxLength() >= aFileName.Length())
        {
        fileNamePtr.Copy(aFileName);
        }
    fileNamePtr.ZeroTerminate();
    
    iSendBuf.Append((TUint8*)&ext, sizeof(ext));
    iSendBuf.AppendFill(0, ext.pfqos_modulespec_len * 8 - byte_len);
    iSendBuf.Append(aConfigData);
    
    iLength = (TUint16)(iLength + ext.pfqos_modulespec_len);
    }

    
EXPORT_C void CPfqosStream::AddExtensionPolicy(TDesC8 &aData)
    {
    TInt len = (aData.Length() + 7) / 8;
    
    iSendBuf.Append(aData);
    iLength = (TUint16)(iLength + len);
    }


EXPORT_C void CPfqosStream::AddExtensionHeader(TUint16 aExtension)
    {
    struct pfqos_configure header;
    struct pfqos_extension extension;
    const int byte_len = sizeof(pfqos_extension) + sizeof(pfqos_configure);
    
    header.pfqos_configure_len = (byte_len + 7) / 8;
    header.pfqos_ext_type = EPfqosExtExtension;
    header.protocol_id = 0;
    header.reserved = 0;
    extension.pfqos_ext_len = (byte_len + 7) / 8;
    extension.pfqos_ext_type = EPfqosExtExtension;
    extension.pfqos_extension_type = aExtension;
    
    iSendBuf.Append((TUint8*)&header, sizeof(header));
    iSendBuf.Append((TUint8*)&extension, sizeof(extension));
    iSendBuf.AppendFill(0, header.pfqos_configure_len * 8 - byte_len);
    iLength = (TUint16)(iLength + header.pfqos_configure_len);
    }


EXPORT_C void CPfqosStream::AddSrcAddress(const TInetAddr &anAddr, 
    const TInetAddr &aMask, TUint16 aPortMax)
    {
    AddAddress(anAddr, aMask, EPfqosExtSrcAddress, aPortMax);
    _LIT(KText1, "ADDRESS_SRC");
    __ASSERT_ALWAYS(iLength * 8 == iSendBuf.Length(), User::Panic(KText1, 0));
    }

    
EXPORT_C void CPfqosStream::AddDstAddress(const TInetAddr &anAddr, 
    const TInetAddr &aMask, TUint16 aPortMax)
    {
    AddAddress(anAddr, aMask, EPfqosExtDstAddress, aPortMax);
    _LIT(KText2, "ADDRESS_SRC");
    __ASSERT_ALWAYS(iLength * 8 == iSendBuf.Length(), User::Panic(KText2, 0));
    }


EXPORT_C TInt CPfqosStream::Send(RSocket &aSocket)
    {
    TRequestStatus status;
    TPtrC8 len = TPtrC8((TUint8 *)&iLength, sizeof(iLength));
    iSendBuf.Replace(_FOFF(struct pfqos_msg, pfqos_msg_len), 
        sizeof(((struct pfqos_msg *)0)->pfqos_msg_len), len);
    aSocket.Write(iSendBuf, status);
    User::WaitForRequest(status);
    return status.Int();
    }

    
EXPORT_C void CPfqosStream::Send(RSocket &aSocket, TRequestStatus& aStatus)
    {
    TPtrC8 len = TPtrC8((TUint8 *)&iLength, sizeof(iLength));
    iSendBuf.Replace(_FOFF(struct pfqos_msg, pfqos_msg_len), 
        sizeof(((struct pfqos_msg *)0)->pfqos_msg_len), len);
    aSocket.Write(iSendBuf, aStatus);
    }



EXPORT_C TInt CPfqosStream::Send(RInternalSocket &aSocket)
    {
    TRequestStatus status;
    TPtrC8 len = TPtrC8((TUint8 *)&iLength, sizeof(iLength));
    iSendBuf.Replace(_FOFF(struct pfqos_msg, pfqos_msg_len), 
        sizeof(((struct pfqos_msg *)0)->pfqos_msg_len), len);
    aSocket.Write(iSendBuf, status);
    User::WaitForRequest(status);
    return status.Int();
    }

    
EXPORT_C void CPfqosStream::Send(RInternalSocket &aSocket, TRequestStatus& 
    aStatus)
    {
    TPtrC8 len = TPtrC8((TUint8 *)&iLength, sizeof(iLength));
    iSendBuf.Replace(_FOFF(struct pfqos_msg, pfqos_msg_len), 
        sizeof(((struct pfqos_msg *)0)->pfqos_msg_len), len);
    aSocket.Write(iSendBuf, aStatus);
    }



EXPORT_C void CPfqosStream::AddAddress(const TInetAddr &anAddr, 
    const TInetAddr &aMask, TUint8 aType, TUint16 aPortMax)
    {
    struct pfqos_address address;
    const int byte_len = sizeof(struct pfqos_address) + sizeof(TInetAddr) + 
        sizeof(TInetAddr);

    address.pfqos_address_len = (byte_len + 7) / 8;
    address.pfqos_ext_type = aType;
    address.reserved = 0;
    address.pfqos_port_max = aPortMax;
    iSendBuf.Append((TUint8*)&address, sizeof(address));
    iSendBuf.Append((TUint8*)&anAddr, sizeof(TInetAddr));
    iSendBuf.Append((TUint8*)&aMask, sizeof(TInetAddr));
    iSendBuf.AppendFill(0, address.pfqos_address_len * 8 - byte_len);
    
    iLength = (TUint16)(iLength + address.pfqos_address_len);
    }

    
