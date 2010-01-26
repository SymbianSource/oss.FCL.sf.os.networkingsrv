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
// Network definitions
// Address size of IP4
// 
//

/**
 @file
 @internalComponent 
*/
 
const TInt KProtocolAddressSize=4; 
const TInt KNetworkAddressSize=6;	//< Address size for Ether MAC
const TInt KMacDestAddressOffset=0;
const TInt KMacSrcAddressOffset=6;
const TInt KPacketFrameTypeOffset=12;
const TInt KLLCPacketFrameTypeOffset=20;
const TInt KARPFrameTypeHighByte=0x08;
const TInt KARPFrameTypeLowByte=0x06;
const TInt KIPFrameTypeHighByte=0x08;
const TInt KIPFrameTypeLowByte=0x00;
const TInt KARPMacSize=14;			//< Mac Header Size
const TInt KARPLLCHeaderSize=22;	//< 802.2 LLC Header Size
const TInt KARPRequestSize=28;		//< ARP Request Size
const TInt KTotalARPRequestSize=KARPMacSize+KARPRequestSize;
const TInt KARPLLCPacketLen=60;		//< 802.2 ARP Packet Length including PAD bytes
const TInt KNumARPPackBytesLLC=10;
const TInt KNumARPPackBytesEther=18;
const TInt KARPHardTypeOffset=0;	//< ARP Packet Offsets - from start of data not including MAC or LLC headers
const TInt KARPProtTypeOffset=2;
const TInt KARPHardSizeOffset=4;
const TInt KARPProtSizeOffset=5;
const TInt KARPOperationOffset=6;
const TInt KARPOperationOffsetLowByte=7;
const TInt KARPSenderEtherAddrOffset=8;
const TInt KARPSenderIPAddrOffset=14;
const TInt KARPTargetEtherAddrOffset=18;
const TInt KARPTargetIPAddrOffset=24;
const TInt KIpSourceAddrOffset=12;	//< IP header offsets (from start of IP Header)
const TInt KIpDestAddrOffset=16;
const TInt KIpOptionsAddrOffset=20;
const TInt KARPRequest=1;			//< ARP Packet Types
const TInt KARPReply=2;
//const TUint8 KTestSrcIpAddr[4]={194,200,129,254};
const TUint8 KTestSrcIpAddr[4]={194,129,1,247};
