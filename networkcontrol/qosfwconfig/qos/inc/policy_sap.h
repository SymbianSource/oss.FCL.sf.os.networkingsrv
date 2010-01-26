// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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



/**
 @internalComponent
*/
#ifndef __POLICY_SAP_H__
#define __POLICY_SAP_H__

#define QOS_PLATSEC_CAPABILITY

#include <e32std.h>
#include <e32base.h>
#include <es_prot.h>
#include <es_mbuf.h>

#include "pfqos.h"	// .. only for KPfqoEventAll !!?

enum TPfqosPanic
	{
	// Only one actually used (preserve the old number (3) in case it's documented somewhere)
	EPfqosPanic_NoData = 3,
	EPfqosPanic_NullPointer = 4,
	};

void Panic(TPfqosPanic aPanic);

//
//	Listener and Registered are set into iFlags of those SAP's
//	which are ready to listen PFQOS messages.
//
const TUint KProviderKey_Listener				= 0x0001;
const TUint KProviderKey_RegisteredQoSConf		= 0x0002;
const TUint KProviderKey_RegisteredQoSFailure	= 0x0004;
const TUint KProviderKey_RegisteredQOSAdapt		= 0x0008;
const TUint KProviderKey_ExpectInput			= KPfqosEventAll;

// Forward declaration
class CProtocolQoS;
class TPfqosMessage;

// Provider class
class CQoSProvider : public CServProviderBase
	{
	friend class CProtocolQoS;
public:
	CQoSProvider(CProtocolQoS& aProtocol);
	~CQoSProvider();

	void Start();
	void LocalName(TSockAddr& aAddr) const;
	TInt SetLocalName(TSockAddr& aAddr);
	void RemName(TSockAddr& aAddr) const;
	TInt SetRemName(TSockAddr& aAddr);
	TInt GetOption(TUint aLevel,TUint aName,TDes8& aOption)const;
	void Ioctl(TUint aLevel,TUint aName,TDes8* aOption);
	void CancelIoctl(TUint aLevel,TUint aName);
	TInt SetOption(TUint aLevel,TUint aName,const TDesC8 &aOption);
	TUint Write(const TDesC8& aDesc,TUint aOptions, TSockAddr* aAddr=NULL);
	void GetData(TDes8& aDesc,TUint aOptions,TSockAddr* aAddr=NULL);
	void ActiveOpen();
	void ActiveOpen(const TDesC8& aConnectionData);
	TInt PassiveOpen(TUint aQueSize);
	TInt PassiveOpen(TUint aQueSize,const TDesC8& aConnectionData);
	void Shutdown(TCloseType aOption);
	void Shutdown(TCloseType aOption,const TDesC8& aDisconnectionData);
	void AutoBind();
	void Deliver(TPfqosMessage& aMsg);
	
#ifdef QOS_PLATSEC_CAPABILITY
	TInt SecurityCheck(MProvdSecurityChecker *aSecurityChecker);
#endif

protected:
	CProtocolQoS& iProtocol;
	TDblQueLink iSAPlink;
	RMBufPktQ iRecvQ;
	TUint iFlags;
	// Currently following are only for statistics and logging
	TInt iQueuedBytes;		// Queued total bytes waiting for GetData
	TInt iQueuedPackets;	// Queued packets waiting for GetData
	};

#endif
