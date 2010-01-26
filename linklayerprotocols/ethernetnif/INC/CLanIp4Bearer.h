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
// header file for the CLanIp4Bearer implementation
// class, a derived from CLanxBearer.
// History
// 15/11/01 Started by Julian Skidmore.
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined( CLanIp4Bearer_H )
#define CLanIp4Bearer_H

#include "CLanxBearer.h"
#include "EthProvision.h"

/**
@internalComponent
*/
const TUint32 KDefaultArpTimeoutSetting = 1800; // 30 minutes, default ARP timeout

/**
@internalComponent
*/
NONSHARABLE_CLASS(CLanIp4Bearer) : public CLanxBearer
{
public:
	CLanIp4Bearer(CLANLinkCommon* aLink);
	virtual void ConstructL();
	//NifIfBase support.
	//virtual TInt State(); Rely on inherited.
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption);
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aPdu);
	
	// we need to override StartSending in CLanIp4Bearer, because of Dhcp.
	virtual void StartSending(CProtocolBase* aProtocol);
	
	// CLanxBearer
	virtual TInt GetConfig(TBinderConfig& aConfig);

	//Additional methods.
	// Perhaps need different version of this
	virtual TBool WantsProtocol(TUint16 aProtocolCode,const TUint8* aPayload);
	virtual void Process(RMBufChain& aPdu,TAny* aLLC);

	// Utilities
	virtual const TDesC8& ProtocolName() const;
	
	// Support for provisioning
	virtual void SetProvisionL(const Meta::SMetaData* aProvision);

private:
	void ReadCommDbLanSettingsL();
	
	enum {KIPProtocol=0x800, KIP4Protocol=4};
	// 32-bit addresses in IPv4.
  	TUint32				iLocalAddr;
  	TUint32				iNetMask;
  	TUint32				iBroadcastAddr;
  	TUint32				iDefGateway;
  	TUint32				iPrimaryDns;
  	TUint32				iSecondaryDns;
  	
  	const TLanIp4Provision* iProvision;
};

inline CLanIp4Bearer::CLanIp4Bearer(CLANLinkCommon* aLink) : 
   CLanxBearer(aLink)
{
}

#endif
