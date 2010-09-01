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
// sap.h - Packet Probe Hook
//



/**
 @internalComponent
*/
#ifndef __PROBE_SAP_H
#define __PROBE_SAP_H

#include "prt.h"
#include "family.h"


class CProviderProbe : public CServProviderBase
{
	friend class CProtocolProbe;
public:
	CProviderProbe(CProtocolProbe* aProtocol);
	virtual ~CProviderProbe();

	//These might be useful
	virtual void Start();
	virtual TInt GetOption(TUint level,TUint name,TDes8& anOption) const;
	virtual TInt SetOption(TUint level,TUint name,const TDesC8 &anOption);
	virtual void Ioctl(TUint level,TUint name,TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel,TUint aName);

	virtual TUint Write(const TDesC8& aDesc,TUint options, TSockAddr* aAddr=NULL);
	virtual void GetData(TDes8& aDesc,TUint options,TSockAddr* anAddr=NULL);

	void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol);

	//These do nothing
	virtual void LocalName(TSockAddr& /*anAddr*/) const
		{ Panic(EProbePanic_NotSupported); }
	inline virtual void RemName(TSockAddr& /*anAddr*/) const
		{ Panic(EProbePanic_NotSupported); }
	inline virtual void ActiveOpen()
		{ Panic(EProbePanic_NotSupported); }
	inline virtual void ActiveOpen(const TDesC8& /*aConnectionData*/)
		{ Panic(EProbePanic_NotSupported); }
	virtual void Shutdown(TCloseType aOption);
	virtual void Shutdown(TCloseType aOption, const TDesC8& aDisconnectionData);
	virtual void AutoBind();
	virtual TInt SetLocalName(TSockAddr& /*anAddr*/);

	inline virtual TInt SetRemName(TSockAddr& /*anAddr*/)
		{ Panic(EProbePanic_NotSupported); return 0; }
	inline virtual TInt PassiveOpen(TUint /*aQueSize*/)
		{ Panic(EProbePanic_NotSupported); return 0; }
	inline virtual TInt PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/)
		{ Panic(EProbePanic_NotSupported); return 0; }

	void Error(TInt aError, TUint aOperationMask);
	inline TBool FatalState() { return (iErrorMask & (MSocketNotify::EErrorFatal|MSocketNotify::EErrorConnect)) != 0; }
#ifdef TCPIP6_CAPABILITY
	TInt SecurityCheck(MProvdSecurityChecker *aChecker);
#endif
protected:
	CProtocolProbe *const iProtocol;
	CProviderProbe *iNext;
	//
	// IsReceiving is intended for the protocol side. It is can be used by
	// the protocol side "Deliver" method to ask if this SAP is willing to
	// receive packets associated with the specified id.
	virtual TBool IsReceiving(const RMBufPktInfo &info);

	RMBufPktQ iRecvQ;
	//
	// iQueueLimit is used to control how much buffered data is allowed
	// to be in the iRecvQ, before "congestion" control hits. The value counts
	// bytes in iRecvQ in following way
	// - if iQueueLimit < 0, then incoming packet is dropped (= "congestion")
	// - if iQueueLimit >= 0, then incoming packet is added into iRecvQ, and
	//   the length of the packet is subtracted from the iQueueLimit. When
	//   GetData removes the packet from the queue, the length is added back
	//   to iQueueLimit.
	// Thus, if left as initial value (= 0), only one packet at time can be
	// queued. If initialized to 8000, then at most 8000 bytes and 1 packet
	// can be queued at any point.
	TInt iQueueLimit;
	TInt iPacketsDropped;	// Count packets dropped due "congestion"
	TUint iInputStopped:1;
	TUint iErrorMask;
	};

#endif
