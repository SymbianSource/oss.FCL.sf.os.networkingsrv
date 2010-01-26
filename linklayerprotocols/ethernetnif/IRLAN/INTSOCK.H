// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Internal Sockets API 
// Written by Mal September 1997
// Based on work by AdamG
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined(__INTSOCK_H__)
#define __INTSOCK_H__

#include <e32std.h>
#include <e32base.h>
#include <es_prot.h>
#include <es_sock.h>

/**
Specifies Internal Max buffer size.
*/
const TUint KInternalMaxBufSize=2048;

enum TPanicCode
{ 
	EPanic_NotReady,
	EPanic_InUse,
	EPanic_StatusRecv,
	EPanic_StatusComplete
};

class CInternalSocketTimers;
/**
Provides internal socket access direct to a SAP 
Based on CServProviderBase
@internalComponent
*/
NONSHARABLE_CLASS(CInternalSocket) : public CServProviderBase, public MSocketNotify
{
private:
	enum TInternalSockState
		{ ENew, EOpen, EConnecting, EConnected, 
		  EListening, EClosing, EClosed, EFault };
public:
	CInternalSocket();
	static CInternalSocket *NewL(CServProviderBase *aSock,MSocketNotify *aNotify);
	~CInternalSocket();
	//CInternalSocket* NewL();
	TInt OpenL(const TDesC& aProtocolName,MSocketNotify* aNotify);

	// inherited from CServProviderBase 
	virtual void Start(void);
	virtual TInt SetLocalName(TSockAddr& aAddr);
	virtual void LocalName(TSockAddr& anAddr)const;
	virtual TInt SetRemName(TSockAddr& aAddr);
	virtual void RemName(TSockAddr& aAddr)const;
	virtual TInt GetOption(TUint level,TUint name,TDes8& anOption)const;
	virtual void Ioctl(TUint level,TUint name,TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel,TUint aName);
	virtual TInt SetOption(TUint,TUint,const TDesC8&);
	virtual TUint Write(const TDesC8 &,TUint,TSockAddr *);
	virtual void GetData(TDes8 &,TUint,TSockAddr *);
	virtual void ActiveOpen(const class TDesC8 &);
	virtual void ActiveOpen(void);
	virtual TInt PassiveOpen(TUint,const TDesC8 &);
	virtual TInt PassiveOpen(TUint);
	virtual void Shutdown(enum CServProviderBase::TCloseType,const TDesC8 &);
	virtual void Shutdown(enum CServProviderBase::TCloseType);
	virtual void AutoBind(void);
	
	TInt Connect(TSockAddr& aAddr);
	TInt WaitForConnect(TUint aQSize);
	void Close();
	TUint Recv(TDes8& aDes,TSockAddr* anAddr,TRequestStatus& aStat,TUint aOptions);
	void RecvFrom(TDes8& aDes, TSockAddr& aAddr,TUint aFlags,TRequestStatus& aStatus);
    TUint Write(RMBufChain& aPdu,TUint options,TRequestStatus& aStat,TSockAddr* anAddr);
	void SendTo(const TDesC8& aDes, const TSockAddr& aAddr,TUint aFlags,TRequestStatus& aStatus);
	void CancelRecv();
	void CancelSend();
	
	void SetNotify(MSocketNotify* aControl){ iNotify=aControl; };
	static TInt InternalSocketTimerExpired(TAny *aCIntSock);
protected:
	virtual void NewData(TUint aCount);
	virtual void CanSend();
	virtual void CanClose(TDelete aDelete=EDelete);
	virtual void Error(TInt anError,TUint anOperationMask=EErrorAllOperations);

	// Pass the following up to layer above via iNotify 
	virtual void ConnectComplete();
    virtual void ConnectComplete(CServProviderBase& aSSP);
	virtual void Disconnect(void);
	virtual void IoctlComplete(TDesC8 *aBuf);
	virtual void ConnectComplete(const TDesC8& aConnectData);
	virtual void ConnectComplete(CServProviderBase& aSSP,const TDesC8& aConnectData);
	virtual void Disconnect(TDesC8& aDisconnectData);
	virtual void CanClose(const TDesC8& aDisconnectData,TDelete aDelete=EDelete);
	virtual void NoBearer(const TDesC8& aConnectionParams);
	virtual void Bearer(const TDesC8& aConnectionInfo);
private:
	void Panic(TPanicCode aPanic);
	void DoRecv();
	void CompleteRecv(TInt aStatus);
	void DoSend();
	void CompleteSend(TInt aStatus);
public:
/** A pointer to the object which signalled it is ready to StartSending. */
	CProtocolBase* iProtocol;
/** A pointer to the object to indicate the server socket. */
	CServProviderBase* iCServSocket;	
private:
	MSocketNotify* iNotify;
	TInternalSockState iState;
	CInternalSocketTimers *iTimers;
	TUint iSockType;

	TUint iSendBufLength;
	HBufC8* iSendBuffer;
	TPtr8 iSendBufPtr;

	TUint iFlags;
	TInt iRecvNotifyCount;
	const TDesC8* iSendDes;
	TUint iSendFlags;
	TSockAddr* iSendAddr;
	TRequestStatus* iSendStat;

	TDes8* iRecvDes;
	TUint iRecvFlags;
	TSockAddr* iRecvAddr;
	TRequestStatus* iRecvStat;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CInternalHostResolver) : public CBase, public MResolverNotify
{
private:
	enum TInternalHRState
		{ ENew, EConnected, EDiscovering,
		  EClosed, EFault };
public:
	CInternalHostResolver();
	~CInternalHostResolver();
	static CInternalHostResolver* NewL();
	TInt OpenL(const TDesC& aProtocolName,MResolverNotify *aEngine);
	// Notifier call back 
	void QueryComplete(TInt aError);
	// Need the following to mimic CHostResolvProvdBase 
	TInt GetByName(TNameRecord& aName);
	void GetByAddress(TNameRecord& aName);
	void SetHostName(TDes8& aNameBuf);
	void GetHostName(TDes8& aNameBuf);
protected:
	void SetNotify(MResolverNotify* aControl){ iNotify=aControl; };
public:
	CHostResolvProvdBase* iHostResolver;
	CProtocolBase* iProtocol;
	/** Set to MResolverNotify ABOVE. */
	MResolverNotify* iNotify;	
private:
	TInternalHRState iState;
	TBool iBusy;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CInternalNetDB) : public CBase , public MResolverNotify
{
private:
	enum TInternalNetDBState
		{ ENew, EOpen, EQuery,
		  EClosed, EFault };
public:
	CInternalNetDB();
	~CInternalNetDB();
	static CInternalNetDB* NewL();
	TInt OpenL(const TDesC& aProtocolName,MResolverNotify *aEngine);

	// Notifier call back 
	void QueryComplete(TInt aError);
	// Need the following to mimic CNetDBProvdBase 
	TInt Query(TDes8& aBuffer);
	void Add(TDes8& aBuffer);
	void Remove(TDes8& aBuffer);
protected:
	void SetNotify(MResolverNotify* aControl){ iNotify=aControl; };
public:
	CProtocolBase* iProtocol;
	CNetDBProvdBase* iNetDatabase;
	/** Set to MResolverNotify ABOVE. */
	MResolverNotify* iNotify;	
private:
	TInternalNetDBState iState;
	TBool iBusy;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CInternalSocketTimers) : public CBase
{
public:
	friend class CInternalSocket;
	virtual ~CInternalSocketTimers();
	static CInternalSocketTimers *NewL(CInternalSocket *aIntSock);
	void StartInternalSocketTimer(TCallBack aCallBack,TInt aTimeout);
	void StopInternalSocketTimer();
	void DoInternalSocketTimerExpired();
private:
	CInternalSocketTimers(CInternalSocket *aIntSock);
private:
	CInternalSocket *iInternalSocket;
	TDeltaTimerEntry iInternalSocketTimer;
	TDeltaTimerEntry *iInternalSocketTimerH;
};

// Added back in as the new stack does not contain sock.h
const TUint KSoConnectPending = 0x0001;	 //< Connect request pending
const TUint KSoIoctlPending = 0x0002; //< Ioctl request pending
const TUint KSoRecvPending = 0x0004; //< Recv/RecvFrom request pending
const TUint KSoSendPending = 0x0008; //< Send/SendTo request pending
const TUint KSoConnectionless = 0x0020;	 //< Does not need to Connect
const TUint KSoCantRecvMore = 0x0100; //< Recvd EOF indication from peer - used with RecvNotifyCount
const TUint KSoCantSendMore = 0x0200; //< Send EOF idication to peer
const TUint KSoSendFlowOff = 0x0400; //< Send is flowed off
const TUint KSoUseCanSend = 0x0800;	//< Use CanSend()
const TUint KSoGracefulClose = 0x1000;	//< Use Graceful close


#endif

