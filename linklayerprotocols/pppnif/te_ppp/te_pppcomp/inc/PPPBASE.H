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
//

/**
 @file
 @internalComponent 
*/

#if !defined(__PPPBASE_H__)
#define __PPPBASE_H__

#include <nifvar.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifvar_internal.h>
#endif
#include <nifmbuf.h>
#include <nifutl.h>

#include <networking/pppdef.h>
#include <networking/pppsock.h>
#include <networking/pppcomp.h>

#include <etelpckt.h>

_LIT(PPP_INI_FILE,"ppp.ini");
_LIT(PPP_SERVER_INI_FILE,"pppd.ini");

enum TPppPanic
	{
	EPppPanic_PPPHDLCIsAlreadyActive,
	EPppPanic_PPPDBReadCallback,
	EPppPanic_PPPNoMemory,
	EPppPanic_PPPNotSupported,
	EPppPanic_PacketHeaderTooBig,
    EPppPanic_PPPInvalidCallback,
    EPppPanic_PPPNoCallbackInfo,
    EPppPanic_PPPLcpRequired,
    EPppPanic_PPPNoNifIfNotify,
    EPppPanic_IETFCalbackInvalid,
    EPppPanic_IETFNoCalback,
    EPppPanic_IncorrectNcpNotif
	};

//
// Panic
//

static inline void PppPanic(TPppPanic aPanic)
//
// Panic the Protocol
//
	{
   _LIT(KPppDesc, "PPP");
	User::Panic(KPppDesc,aPanic);
	}


//
// PPP Internal LCP Constants
//

const TInt KPppDefaultFrameSize = 1500;

enum TMSCBCPAction
	{
	EMSCBCPActionRequireClientSpecifiedNumber  = ECallbackActionMSCBCPRequireClientSpecifiedNumber,
	EMSCBCPActionAcceptServerSpecifiedNumber   = ECallbackActionMSCBCPAcceptServerSpecifiedNumber,
	EMSCBCPActionOverrideServerSpecifiedNumber = ECallbackActionMSCBCPOverrideServerSpecifiedNumber
	};

//

class TPppFcs16;
class TPppFcs32;
class MPppRecvr;
class MPppFsm;
class CPppLcp;
class CPppLinkBase;
class CNifFactory;

//
// Other simple PPP Frame Receivers
//

/**
Adds extra PPP Recv channels to a class T
*/
template <class T>
class TPppExtraRecvr : public MPppRecvr
	{
public:
	TPppExtraRecvr<T>(T* aClass, void (T::*aRecvFn)(RMBufChain&), void (T::*aFlowFn)(), 
		CPppLcp* aLcp, TPppPhase aPhase, TUint aId, void (T::*aFrameErrFn)(), void (T::*aKillFn)())
		: MPppRecvr(aLcp, aPhase, aId), iClass(aClass), iRecvFn(aRecvFn), iFlowFn(aFlowFn), 
			iFrameErrFn(aFrameErrFn), iKillFn(aKillFn){}
		virtual void FrameError()
		{ (iClass->*iFrameErrFn)(); }
		virtual void KillProtocol()
		{ (iClass->*iKillFn)(); }

	virtual TBool RecvFrame(RMBufChain& aPacket)
		{ (iClass->*iRecvFn)(aPacket); return ETrue; };
	virtual void FlowOn()
		{ (iClass->*iFlowFn)(); }
private:
	T* iClass;
	void (T::*iRecvFn)(RMBufChain&);
	void (T::*iFlowFn)();
	void (T::*iFrameErrFn)();
	void (T::*iKillFn)();
	};

//
// PPP Options
//

class RPppOption : public RMBufChain
	{
public:
	RPppOption();
	RPppOption(RMBufChain& aChain);
	inline const RPppOption& operator=(const RMBufChain& aChain);
	void SetL(TUint8 aType, const TAny* aPtr, TInt aLen);
	TUint8 OptType() const;
	void SetType(const TUint8 aType);
	TInt ValueLength() const;
	void SetValueLength(TInt aLen);
	TUint8* ValuePtr();
	const TUint8* ValuePtr() const;
	inline static RPppOption& Cast(RMBufChain& aChain);
	inline static const RPppOption& Cast(const RMBufChain& aChain);
	// These probably belong in the MBuf manager
	TInt MaxLength() const;	//this needs to be accessible because SetValueLength does not reallocate memory if user increases length
private:
	TInt SetLength(TInt aLen);
	};

enum TPppOptResponse
	{
	EPppOptAck,
	EPppOptNak,
	EPppOptReject
	};

class MPppOptionsExtender;
class MPppOptionHandler
	{
	friend class MPppOptionsExtender;
protected:
	MPppOptionHandler();
	void OptRegister(MPppOptionsExtender* aExtender, const TUint8* aOptList, TInt aNumOpts);
	virtual void OptNegotiationStarted();
	virtual void OptNegotiationAborted();
	virtual void OptNegotiationComplete();
	virtual void OptFillinConfigRequestL(RPppOptionList& aRequestList);
	virtual TPppOptResponse OptCheckConfigRequest(RPppOption& aOption);
	virtual void OptApplyConfigRequest(RPppOption& aOption);
	virtual void OptRecvConfigAck(RPppOption& aOption);
	virtual void OptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList);
	virtual void OptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList);
protected:
	TInt iNumOptions;
	const TUint8* iOptionList;
private:
	TDblQueLink iOptHandlerLink;
	};

//
// PPP Finite State Machine
//

const TInt KPppFsmTerminateTimeout = 2500;
const TInt KPppFsmLengthenedTerminateTimeout = 30000;		// 30 secs
const TInt KPppFsmTerminateRetries = 10;
const TInt KPppFsmRequestTimeout = 3000;
const TInt KPppFsmRequestMaxTimeout = 60000;
const TInt KPppFsmLengthenedRequestTimeout = 40000;			// 40 secs
const TInt KPppFsmRequestRetries = 10;
const TInt KPppFsmTimerPriority = 10;

#define __DECLARE_FSM_NAME(t) __iFsmName=t

const TInt KPppFsmNonConvergeLimit = 4;
const TUint KPppRequestIdAnswered = 0x80000000;

//
// PPP Link Protocols
//

/**
Calculates the CCITT CRC-16 used by PPP in HDLC framing.
@internalTechnology
*/
class TPppFcs16
	{
public:
	inline TPppFcs16();
	inline void Init();
	void Calc(const TUint8* aPtr, const TUint8* aEnd);
	inline void CalcByte(TUint8 aByte);
	inline TUint16 Fcs() const;
	inline TBool IsGood() const;
	
private:
	// WINS can't handle static const members initialized here, so use an enum
	enum
		{
		/** Initial FCS value */
		KPppInitFcs16 = 0xffff,
		/** Good final FCS value */
		KPppGoodFcs16 = 0xf0b8
		};
	/** CRC value */
	TUint16 iFcs;		
	/** Fast CRC calculation table */
	static const TUint16 FcsTable[256];
	};

/**
Calculates the CCITT CRC-32 used as an option by PPP in HDLC framing.
@internalTechnology
*/
class TPppFcs32
	{
public:
	inline TPppFcs32();
	inline void Init();
	void Calc(const TUint8* aPtr, const TUint8* aEnd);
	inline void CalcByte(TUint8 aByte);
	inline TUint32 Fcs() const;
	inline TBool IsGood() const;
	
private:
	// WINS can't handle static const members initialized here, so use an enum
	enum
		{
		/** Initial FCS value */
		KPppInitFcs32 = 0xffffffff,
		/** Good final FCS value */
		KPppGoodFcs32 = 0xdebb20e3
		};
	/** CRC value */
	TUint32 iFcs;		
	/** Fast CRC calculation table */
	static const TUint32 FcsTable[256];
	};

/*
// Support for PPP components as external DLLs
class CPppExtDllRef : public CAsyncOneShot
	{
public:
	CPppDllRef();
	~CPppDllRef();
	void UnloadMe();
protected:
	virtual void RunL();
private:
	RLibrary iDll;
	TDblQueLink iLink;
	};

class MPppExtDll
	{
public:
	IMPORT_C virtual ~MPppExtDll();
	virtual void DllInstallL()=0;
	virtual void DllRemove()=0;
private:
	friend class CPppExtDllRef;
	CPppExtDllRef* iPppDllRef;
	};
*/

//
// Other simple PPP Frame Receivers
//

/**
Base class for link protocols
@internalTechnology
*/
class CPppLinkBase : public CBase
	{
public:
	virtual ~CPppLinkBase();
	// Implemented by link protocols
	virtual void CreateL()=0;
	virtual TInt Send(RMBufChain& aPacket, TUint aPppId=KPppIdAsIs)=0;
	virtual void OpenL()=0; // Open/Close from LCP
	virtual void Close()=0;
	virtual void StartL()=0;
	virtual void Stop(TInt aReason, TBool aLinkDown=ETrue)=0;
	virtual void GetSendRecvSize(TInt& aMaxRecvSize, TInt& aMaxSendSize)=0;
	virtual void GetDataTransfer(RPacketContext::TDataVolume&)=0;
	virtual TInt SpeedMetric()=0;
	inline void NewCompressor(const CPppCompressor* aCompressor);
	inline void NewDeCompressor(const CPppDeCompressor* aDeCompressor);
	CPppLinkBase* PppCreateLinkL(CPppLcp* aLcp, const TDesC& aNifName);
protected:
	CPppLinkBase(CPppLcp* aLcp);
	// Used by link protocols
	inline void NotifyLinkUp();
	inline void NotifyLinkDown(TInt aReason);
	inline void DeliverToLcp(RMBufChain& aPacket);
protected:
	CPppLcp* iPppLcp;
	CPppCompressor* iPppCompressor;
	CPppDeCompressor* iPppDecompressor;
	};

#include <networking/pppbase.inl>

#endif
