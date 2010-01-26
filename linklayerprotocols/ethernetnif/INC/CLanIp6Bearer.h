// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// header file for the CLanIp6Bearer implementation
// class, a derived from CLanxBearer.
// History
// 15/11/01 Started by Julian Skidmore.
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined( CLanIp6Bearer_H )
#define CLanIp6Bearer_H

#include "CLanxBearer.h"
#include <eui_addr.h>
#include "EthProvision.h"

/**
@internalComponent
*/
const TUint8 KMulticastPrefix[2] = {0x33, 0x33};


/**
All of this is stored in network byte order.
@internalComponent
*/
struct TIpv6Header
{
	TUint8 GetVersion() { return (TUint8)((iVersionClassHi>>4)&0xff);}
	TUint8 GetClass() { return (TUint8)( ((iVersionClassHi<<4)| (iClassLoFlowHi>>4)) &0xff); }
	TUint8 iVersionClassHi; // The Upper nybble provides version.
	TUint8 iClassLoFlowHi;
	TUint16 iFlowLo; // Two bytes, but it doesn't start on a word boundary.
	TUint16 iPayloadLength;
	TUint8 iNextHeader;
	TUint8 iHopLimit;
	union
		{
		TUint8 iSourceAddrB[16];
		TUint16 iSourceAddrW[8];
		TUint32 iSourceAddrL[4];
		};
	union
		{
		TUint8 iDestAddrB[16];
		TUint16 iDestAddrW[8];
		TUint32 iDestAddrL[4];
		};
};


/**
@internalComponent
*/
NONSHARABLE_CLASS(CLanIp6Bearer) : public CLanxBearer
{
public:
	CLanIp6Bearer(CLANLinkCommon* aLink);
	virtual void ConstructL();

	// MLowerControl
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption);
	virtual TInt GetConfig(TBinderConfig& aConfig);

	// MLowerDataSender
	virtual MLowerDataSender::TSendResult Send(RMBufChain& aPdu);

	//Additional methods.
	// Perhaps need different version of this
	virtual TBool WantsProtocol(TUint16 aProtocolCode,const TUint8* aPayload);
	virtual void Process(RMBufChain& aPdu,TAny* aLLC);
	virtual void UpdateMACAddr();

	virtual const TDesC8& ProtocolName() const;
	
	// Support for provisioning
	virtual void SetProvisionL(const Meta::SMetaData* aProvision);

private:
	void ResolveMulticastIp6(TSockAddr& aDstAddr,RMBufChain& aPdu);
	void ShiftLinkLayerAddress(TSockAddr& aDstAddr);
	void ReadCommDbLanSettingsL();

	enum {KIPProtocol=0x800, KIP6Protocol=6};
	// 32-bit addresses in IPv4.
	TInetAddr iIpAddr;
	TEui64Addr iEuiMac; // yet another variant on the theme.
	TIp6Addr iLocalAddr;
	TIp6Addr iPrimaryDns;
	TIp6Addr iSecondaryDns;

	const TLanIp6Provision* iProvision;		// Provisioning information from SCPR
};

inline CLanIp6Bearer::CLanIp6Bearer(CLANLinkCommon* aLink) : 
   CLanxBearer(aLink)
{
}


#endif
