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
// ipsec.h - IPv6/IPv4 IPSEC security policy protocol family
//



/**
 @internalComponent
*/
#ifndef __IPSEC_H__
#define __IPSEC_H__

#include <es_prot.h>
#include <es_mbuf.h>

const TUint KProtocolInetHook	= 0x103;	// a temp assignment

const TUint KAfIpsec			= 0x0801;	// a temp assignment

/**
* @capability ECapabilityNetworkControl		Required for opening 'pfkey' sockets.
* @ref RSocket::Open()
*/
const TUint KProtocolKey		= 0x101;	// a temp assignment

/**
* @capability ECapabilityNetworkControl		Required for opening 'secpol' sockets.
* @ref RSocket::Open()
*/
const TUint KProtocolSecpol		= KProtocolInetHook;	// SECPOL needs to use hook id!

enum TIpsecPanic
	{
	EIpsecPanic_NoData,
	EIpsecPanic_DeleteSA		// Attempt to delete non-existent SA
	};

void Panic(TIpsecPanic);


class MAssociationManager;
class IPSEC
	{
public:
	static CProtocolBase *NewSecpolL();
	static CProtocolBase *NewPfkeyL();
	static void IdentifySecpol(TServerProtocolDesc &aEntry);
	static void IdentifyPfkey(TServerProtocolDesc &aEntry);
	static MAssociationManager *FindAssociationManager(const CProtocolBase *aProtocol, TUint aId);
	};

class CProviderIpsecBase : public CServProviderBase
	/**
	* The common base class for SECPOL and PFKEY socket provider.
	*
	* This class provides the default implementations for the
	* required functions, which are not used for anything in
	* SECPOL and PFKEY SAP.
	*/
	{
public:
	CProviderIpsecBase();
	~CProviderIpsecBase();

	// Virtual provider base class stuff, that must be present
	// (most of these don't make any sense with the IPSEC/Policy Socket)
	void LocalName(TSockAddr& anAddr) const;
	TInt SetLocalName(TSockAddr& anAddr);
	void RemName(TSockAddr& anAddr) const;
	TInt SetRemName(TSockAddr& anAddr);
	TInt GetOption(TUint level,TUint name,TDes8& anOption)const;
	void Ioctl(TUint level,TUint name,TDes8* anOption);
	void CancelIoctl(TUint aLevel,TUint aName);
	TInt SetOption(TUint level,TUint name,const TDesC8 &anOption);
	void ActiveOpen();
	void ActiveOpen(const TDesC8& aConnectionData);
	TInt PassiveOpen(TUint aQueSize);
	TInt PassiveOpen(TUint aQueSize,const TDesC8& aConnectionData);
	void Shutdown(TCloseType option);
	void Shutdown(TCloseType option,const TDesC8& aDisconnectionData);
	void AutoBind();

	void GetData(TDes8& aDesc,TUint options,TSockAddr* anAddr);
	void Deliver(RMBufChain& aPacket);

	TInt SecurityCheck(MProvdSecurityChecker *aChecker);

public:
	// ...for the associated protocol class.
	TDblQueLink iSAPlink;			//< SAP collection under the protocol
	TUint iListening;				//< Non-Zero, when queuing packets is allowed.
protected:
	// Provide the receive queue for the SAP
	RMBufPktQ iRecvQ;				//< Messages waiting for delivery to application.

	/**
	* Receive queue limit.
	* The queue limit is used to control how much buffered data is allowed
	* to be in the iRecvQ, before "congestion" control hits. The value counts
	* bytes in iRecvQ in following way:
	*
	* @li	if iQueueLimit < 0, then incoming packet is dropped (= "congestion")
	* @li	if iQueueLimit >= 0, then incoming packet is added into iRecvQ, and
	*		the length of the packet is subtracted from the iQueueLimit. When
	*		GetData removes the packet from the queue, the length is added back
	*		to iQueueLimit.
	*
	* Thus, if left as initial value (= 0), only one packet at time can be
	* queued. If initialized to 8000, then at most 8000 bytes and 1 packet
	* can be queued at any point.
	*
	* Currently only enforced for the Policy Socket. PFKEY messages are
	* too important to drop, and PFKEY clients MUST read the the
	* socket.
	*/
	TInt iQueueLimit;
	};

#endif
