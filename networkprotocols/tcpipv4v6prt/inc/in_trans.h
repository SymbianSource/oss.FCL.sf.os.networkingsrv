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
// in_trans.h - transport layer protocol base
//



/**
 @internalComponent
*/
#ifndef __IN_TRANS_H__
#define __IN_TRANS_H__

#include "inet.h"
#include "inet6log.h"


#define SYMBIAN_NETWORKING_UPS

enum TProviderMatchEnum
	{
	EMatchNone,
	EMatchLocalPort,
	EMatchServerUnspecAddr,
	EMatchServerSpecAddr,
	EMatchExact,
	EMatchConnection = EMatchExact
	};


class CProviderInet6Transport;
class CProtocolInet6Transport : public CProtocolInet6Base
	{
public:
	CProtocolInet6Transport();
	~CProtocolInet6Transport();

	CProtocolInet6Transport& operator=(const CProtocolInet6Transport&); // Sugar
	virtual void InitL(TDesC& aTag);
	virtual void BindL(CProtocolBase *protocol, TUint id);
	virtual void StartL();
	virtual TInt Send(RMBufChain &aPacket,CProtocolBase* aSourceProtocol=NULL);
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
	virtual CProviderInet6Transport* LocateSap(TProviderMatchEnum aRank, TUint aFamily,
		const TInetAddr& aLocalAddr,
		const TInetAddr& aRemoteAddr = TInetAddr(),
		CProviderInet6Base *aSap = NULL);
#else 
virtual CProviderInet6Transport* LocateSap(TProviderMatchEnum aRank, TUint aFamily,
		const TInetAddr& aLocalAddr,
		const TInetAddr& aRemoteAddr = TInetAddr(),
		CProviderInet6Base *aSap = NULL,TUint32 aSourceIfIndex = 0);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
	virtual TUint AssignAutoPort();

	inline TUint Random(TUint aMax) { return (TUint)Math::Rand(iSeed) % aMax; }	

#ifdef _LOG
	virtual void LogProviders(TUint aPort);
#endif

	TInt GetIniValue(const TDesC &aSection, const TDesC &aName,
						TInt aDefault, TInt aMin, TInt aMax, TBool aBoundMode = EFalse) const;

private:
	TInt64 iSeed;
	};

#ifdef SYMBIAN_NETWORKING_UPS
class MProviderBindings : public MInetBase
	{
	public:
	virtual TBool HasSocket() = 0;
	};

const TUint KApiVer_MProviderBindings = 0;

class CProviderInet6Transport : public CProviderInet6Base, public MProviderBindings
#else
class CProviderInet6Transport : public CProviderInet6Base
#endif
	{
	friend class CProtocolInet6Transport;

public:
	CProviderInet6Transport(CProtocolInet6Base *aProtocol);
	virtual ~CProviderInet6Transport();

	virtual void LocalName(TSockAddr &anAddr) const;
	virtual TInt SetLocalName(TSockAddr &anAddr);
	virtual void RemName(TSockAddr &anAddr) const;
	virtual TInt SetRemName(TSockAddr &anAddr);
	virtual void AutoBind();
	virtual TInt SetOption(TUint aLevel, TUint aName, const TDesC8& aOption);
	virtual TInt GetOption(TUint aLevel, TUint aName, TDes8& aOption) const;

	virtual void CanSend();
	virtual void Error(TInt aError, TUint aOperationMask = MSocketNotify::EErrorAllOperations);
	CProtocolInet6Transport* Protocol() const { return (CProtocolInet6Transport*)iProtocol; }
	virtual void IcmpError(TInt aError, TUint aOperationMask, TInt aType, TInt aCode,
		const TInetAddr& aSrcAddr, const TInetAddr& aDstAddr, const TInetAddr& aErrAddr);

#ifdef SYMBIAN_NETWORKING_UPS
	TBool HasSocket();	
	void *GetApiL(const TDesC8& aApiName, TUint* aVersion);
protected:
	TBool ConnectionInfoSet();
#endif	
protected:

	struct TSockFlags
		{
		TUint16 iRecvClose:1;
		TUint16 iSendClose:1;
		TUint16 iConnected:1;
		TUint16 iFlowStopped:1;
		TUint16 iReuse:1;
		TUint16 iAttached:1;        // If true, ESock has a pointer to this socket
		TUint16 iNotify:1;          // If true, we can notify ESock of events
//		TUint16 iRawMode:1;         // If true, user received UDP packets with IP header
//		TUint16 iHeaderIncluded:1;  // If true, user sends UDP packets with IP header
		TUint16 iReportIcmp:1;      // If true, report ICMP errors to application
//		TUint16 iSynchSend:1;       // If true, block socket write to UDP socket in PENDING and HOLD states
		TUint16 iAddressSet:1;	  // If true, application has bound the socket to a specific IP address
		} iSockFlags;
	TUint iAppFamily;
	TUint iSockFamily;
// UPS support
#ifdef SYMBIAN_NETWORKING_UPS
	TUint		iConnectionInfoReceived:1;
#endif
	};

class RMBufSockQ : public RMBufChain
	{
public:
	void AppendL(const TDesC8& aData, TInt aLength);
	TInt AppendDes(const TDesC8& aData, TInt aLength);
	TInt AppendAtLeast(RMBufChain& aChain, TInt aLength);
	TInt RemoveAtMost(RMBufChain& aChain, TInt aLength);
	static inline RMBufSockQ& Cast(RMBufChain& aChain) { return (RMBufSockQ&)aChain; }
	};

class TDualBufPtr
	{
private:
	enum BufferTypeEnum { ENone, ERMBufChain, EDes8, EDesC8 };

public:
	TDualBufPtr();
	TDualBufPtr(RMBufChain& aChain) : iType(ERMBufChain), iChain(&aChain) { }
	TDualBufPtr(TDes8& aDesc) : iType(EDes8), iDesc(&aDesc) { }
	TDualBufPtr(const TDesC8& aDesc) : iType(EDesC8), iDescC(&aDesc) { }

	void CopyInL(const RMBufChain& aQueue, TInt aOffset, TInt aLength);
	TInt CopyIn(const RMBufChain& aQueue, TInt aOffset, TInt aLength);
	void CopyOut(RMBufChain& aChain, TInt aOffset) const;
	TInt Consume(RMBufChain& aQueue, TInt aLength, RMBufAllocator& aAllocator);
	TInt AppendL(RMBufChain& aQueue, TInt aLength);
	TInt Append(RMBufChain& aQueue, TInt aLength);	//ASSERTS if used with Des type
	void Free();

	inline TInt Length() const { return (iType == ERMBufChain) ? (iChain->IsEmpty() ? 0 : iChain->Length()) : iDesc->Length(); }

private:
	union
		{
		RMBufChain *iChain;
		TDes8 *iDesc;
		const TDesC8 *iDescC;
		};
	BufferTypeEnum iType;
	};

#endif
