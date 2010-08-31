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
// tcp.h - TCP protocol for IPv6/IPv4
// TCP protocol class declarations for IPv6/IPv4.
//



/**
 @file tcp.h
 @internalComponent
*/

#ifndef __TCP_H__
#define __TCP_H__

#include "in_trans.h"
#include <ip6_hdr.h>
#include <tcp_hdr.h>
#include <in_chk.h>
#include "frag.h"
#include "inet6log.h"
#include <in6_opt.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <in_sock.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

// Define the following macro here. Do NOT use macro statement in mmp file to define this.
//	The macro will be used in source files to confirm the right modifed header file is used.
//	Using macro statement in mmp file disables the trick.
//#define FJ_USE_VARIANT_TIMER

#ifdef FJ_USE_VARIANT_TIMER
#include <FjVariantTimer.h>
#endif

//
// Constants affecting protocol performance
//
const TUint KOneSecondUs      = 1000000;    //< Help for converting longer times to microseconds

const TUint KTcpMaximumWindow = 0x3fffffff;  //< Maximum receive window size
const TUint KTcpMinimumWindow =      1024;  //< Minimum receive window size
const TUint KTcpDefaultRcvWnd = 48 * 1024;  //< Default receive window size
const TUint KTcpDefaultSndWnd = 16 * 1024;  //< Default send window size

const TUint KTcpDefaultMSS    =     65535;  //< By default, MSS is not limited by user
const TUint KTcpStandardMSS   =       536;  //< Internet standard MSS
const TUint KTcpMinimumMSS    =        64;  //< Minimum acceptable MSS.
const TUint KTcpMaxTransmit   =         2;  //< Transmit at most this many segments at one time.

const TUint KTcpMinRTO        =   1000000;  //< us (1s)
const TUint KTcpMaxRTO        =  60000000;  //< us (60s)
const TUint KTcpInitialRTO    =   3000000;  //< us (3s)
const TUint KTcpSrttSmooth    =         8;  //< alpha = 1/8
const TUint KTcpMdevSmooth    =         4;  //< beta  = 1/4
const TUint KTcpRTO_K         =         4;  //< RTO = SRTT = 4 * RTTVAR

const TUint KTcpAckDelay      =    200000;  //< us (200ms)
const TUint KTcpMsl2Delay     =  60000000;  //< us (2x30s)

const TUint KTcpSynRetries    = 5;          //< Maximum retransmit attempts during connect.
const TUint KTcpMaxRetries1   = 3;          //< Maximum retransmit attempts before MTU reduction.
const TUint KTcpMaxRetries2   = 12;	    //< Maximum retransmit attempts during transmission.

const TUint KTcpReordering    = 3;          //< Worst case packet reordering in network.
const TUint KTcpInitialCwnd   = 2;          //< Initial congestion window
const TUint KTcpLtxWindow     = 2;          //< Limited transmit window (2 segments)


const TUint KTcpNumKeepAlives = 8;	    //< Number of keepalive probes before quitting.
const TUint KTcpKeepAliveRxmt = 75;	    //< Interval for retransmitted keepalives (seconds).
const TUint KTcpKeepAliveIntv = 2 * 3600;   //< Default interval between keepalives (seconds => 2 h).
const TUint KTcpKeepAliveTH   = 30;		//< Minimum time between triggered KeepAlives (seconds)

const TInt KTcpFinPersistency = 3;		//< Default for tcp_fin_persistency.
const TInt KTcpMaxLingerTime = 1800;	//< Max Linger Timeout in seconds (= 30 min).

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
const TUint KTcpDefaultRcvMaxWnd = 0x40000; // TCP Receive Max window = 262144
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

//
// Maximum number of remembered urgent pointers. If the number of
// separate pending urgent bytes exceeds this number the urgent
// data bytes will start appearing in the received data (as if
// inlined).
//
// Discussion: We use a fairly light-weight method of removing
// out-of-band data from the data stream that involves no extra
// copying. BSD does it the hard way but still sometimes leaves
// droppings in the data stream. So, why bother? Increasing the
// number below reduces the probability of an out-of-band character
// appearing in the data stream. The cost is 4*n bytes state
// variable space. -ML
//
const TInt KTcpUpMax         = 5;


typedef TInet6Checksum<TInet6HeaderTCP> TTcpPacket;


/**
 * TCP timer
 */
#ifdef FJ_USE_VARIANT_TIMER
class CTcpTimer : public DCM::CVariantTimer
#else
class CTcpTimer : public CTimer
#endif
	{
public:
#ifdef FJ_USE_VARIANT_TIMER
	CTcpTimer(TCallBack& aCallBack, TInt aPriority = KInet6DefaultPriority)
    : DCM::CVariantTimer(aPriority), iCallBack(aCallBack.iFunction, aCallBack.iPtr)
#else
  CTcpTimer(TCallBack& aCallBack, TInt aPriority = KInet6DefaultPriority)
		: CTimer(aPriority), iCallBack(aCallBack.iFunction, aCallBack.iPtr)
#endif
		{ CActiveScheduler::Add(this); }
	virtual void RunL() { iCallBack.CallBack(); }
	virtual void InitL() { ConstructL(); }
	void Start(TUint aMicroSeconds)
		{
		if (!IsActive())
			After(aMicroSeconds);
		}

	void Restart(TUint aMicroSeconds)
		{
		if (IsActive())
			Cancel();
		After(aMicroSeconds);
		}

	private:
		TCallBack iCallBack;
	};


/**
 * TCP reassembly queue
 */
class RMBufTcpFrag : public RMBufFrag
	{
public:
	TUint Offset();
	TUint FragmentLength();
	void Join(RMBufChain& aChain);
	};

typedef RMBufFragQ<RMBufTcpFrag> RMBufTcpFragQ;


/**
 * TCP Protocol
 */
class CProtocolTCP6 : public CProtocolInet6Transport
	{
public:
	CProtocolTCP6();
	CProtocolTCP6& operator=(const CProtocolTCP6&);
	virtual ~CProtocolTCP6();
	virtual CServProviderBase *NewSAPL(TUint aProtocol);
	virtual void InitL(TDesC& aTag);
	virtual void StartL();
	virtual void Identify(TServerProtocolDesc *) const;
	//virtual TInt GetOption(TUint level,TUint name,TDes8 &option,CProtocolBase* aSourceProtocol=NULL);
	//virtual TInt SetOption(TUint level, TUint aName,const TDesC8 &option,CProtocolBase* aSourceProtocol=NULL);
	virtual TInt Send(RMBufChain& aPDU,CProtocolBase* aSourceProtocol=NULL);
	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);
	static void Describe(TServerProtocolDesc&);
	TUint32 RandomSequence();

	TInt SendControlSegment(RFlowContext *aFlow,
		const TSockAddr& aSrcAddr, const TSockAddr& aDstAddr,
		TUint8 aFlags, TTcpSeqNum aSeq, TTcpSeqNum aAck,
		TUint32 aWnd = 0, TUint32 aUP = 0);

	//
	// TCP Configuration Parameters
	//
	TUint MSS() const               { return iMSS; }
	TUint RecvBuf() const           { return iRecvBuf; }
	TUint SendBuf() const           { return iSendBuf; }
	TUint MinRTO() const            { return iMinRTO; }
	TUint MaxRTO() const            { return iMaxRTO; }
	TUint InitialRTO() const        { return iInitialRTO; }
	TUint SrttSmooth() const        { return iSrttSmooth; }
	TUint MdevSmooth() const        { return iMdevSmooth; }
	TUint RTO_K() const             { return iRTO_K; }
	TUint RTO_G() const             { return iRTO_G; }
	TUint ClockGranularity() const  { return iClockGranularity; }
	TUint AckDelay() const          { return iAckDelay; }
	TUint Msl2Delay() const         { return iMsl2Delay; }
	TUint SynRetries() const        { return iSynRetries; }
	TUint Retries1() const          { return iRetries1; }
	TUint Retries2() const          { return iRetries2; }
	TUint ProbeStyle() const        { return iProbeStyle; }
	TUint InitialCwnd() const       { return iInitialCwnd; }
	TUint LtxWindow() const         { return iLtxWindow; }
	TUint RFC2414() const           { return iRFC2414; }
	TUint Reordering() const        { return iReordering; }
	TUint MaxBurst() const          { return iMaxBurst; }
	TUint TimeStamps() const        { return iTimeStamps; }
	TUint Sack() const              { return iSack; }
	TUint LocalTimeWait() const     { return iLocalTimeWait; }
	TUint StrictNagle() const       { return iStrictNagle; }
	TUint PushAck() const           { return iPushAck; }
	TUint FRTO() const				{ return iFRTO; }
	TUint DSack() const				{ return iSack && iDSack; }
	TUint KeepAliveIntv() const		{ return iKeepAliveIntv; }
	TUint KeepAliveRxmt() const		{ return iKeepAliveRxmt; }
	TUint NumKeepAlives() const		{ return iNumKeepAlives; }
	TUint DstCache() const			{ return iDstCache; }
	TUint Ecn() const				{ return iEcn; }
	TUint FinPersistency() const	{ return iFinPersistency; }
	TUint SpuriousRtoResponse() const { return iSpuriousRtoResponse; }
	TInt WinScale()					{ return iWinScale; }
	TInt AlignOpt() const 			{ return iAlignOpt; }

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//Function to set and get receive window
	void SetRecvWin(TUint aRecvWin) { iRecvBuf = aRecvWin;}
	TUint GetRecvWinSize()			{ return iRecvBuf;	 }
	TUint RecvMaxWnd()              { return iTcpMaxRecvWin;}
	TUint RecvBufFromIniFile()      { return iRecvBufFromIniFile; }
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

#ifdef _LOG
	static void LogPacket(char aDir, RMBufChain& aPacket, RMBufPktInfo *info = 0, TInt aOffset = 0);
#endif

private:

	TUint	iMSS;
	TUint   iRecvBuf;
	TUint   iSendBuf;
	TUint   iMinRTO;
	TUint   iMaxRTO;
	TUint   iInitialRTO;
	TUint   iSrttSmooth;
	TUint   iMdevSmooth;
	TUint   iRTO_G;
	TUint   iRTO_K;
	TUint   iMaxBurst;
	TUint   iAckDelay;
	TUint   iSynRetries;
	TUint   iRetries1;
	TUint   iRetries2;
	TUint   iProbeStyle;
	TUint   iClockGranularity;
	TUint   iMsl2Delay;
	TUint   iInitialCwnd;
	TUint   iLtxWindow;
	TUint   iReordering;
	TUint	iKeepAliveIntv;
	TUint	iKeepAliveRxmt;
	TUint	iNumKeepAlives;
	TUint	iFinPersistency;
	TInt8   iWinScale;		//< value of "tcp_winscale" option: -1 ... 7

	// Flags
	TUint  	iTimeStamps:1;
	TUint  	iSack:1;
	TUint  	iLocalTimeWait:1;
	TUint  	iStrictNagle:1;
	TUint  	iRFC2414:1;
	TUint 	iPushAck:1;
	TUint 	iFRTO:1;
	TUint 	iDSack:1;
	TUint  	iDstCache:1;
	TUint	iAlignOpt:1;	//< Set if TCP options should be aligned using NOP option.

	// Ecn has 3 reasonable settings: 0 = disable, 1 = enable with ECT(1), 2 = enable with ECT(0).
	// Old specification used only ECT(0), so there may be routers out there that only understand
	// ECT(0) but not ECT(1).
	TUint	iEcn:2;
	
	// 8 possible values should be enough for spurious response alternatives.
	TUint	iSpuriousRtoResponse:3;

	TUint32 iRandomIncrement;
	RMBufAllocator iBufAllocator;
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	TUint iTcpMaxRecvWin;
	TUint iRecvBufFromIniFile;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	};


/**
 * TCP Socket Provider.
	*/
class CProviderTCP6 : public CProviderInet6Transport
	{
	friend class CProtocolTCP6;

public:
	CProviderTCP6(CProtocolInet6Base* aProtocol);
	virtual ~CProviderTCP6();
	virtual void InitL();
	virtual void Start();
	virtual TInt GetOption(TUint level,TUint name,TDes8 &anOption) const;
	virtual void Ioctl(TUint level,TUint name,TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel, TUint aName);
	virtual TInt SetOption(TUint level,TUint name, const TDesC8 &anOption);
	virtual TInt SetRemName(TSockAddr &aAddr);
	virtual void Shutdown(TCloseType option);
	virtual void ActiveOpen();
	virtual TInt PassiveOpen(TUint aQueSize);
	virtual void ErrorExpire(TInt aError);
	virtual void CanSend();

	// PRTv1.0 send and receive methods
	virtual TUint Write(const TDesC8 &aDesc,TUint options, TSockAddr* aAddr=NULL);
	virtual void GetData(TDes8 &aDesc,TUint options,TSockAddr *aAddr=NULL);

	// PRTv1.5 send and receive methods
	virtual TInt Write(RMBufChain& aData, TUint aOptions, TSockAddr* anAddr=NULL);
	virtual TInt GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* anAddr=NULL);

	// Parent socket methods
	TInt CreateChild(CProviderTCP6*& aSAP);
	void DetachChild(CProviderTCP6* aSAP);
	TInt CompleteChildConnect(CProviderTCP6* aSAP);
	inline void SetChildDeleted(TBool aDeleted);

	virtual void Process(RMBufChain& aPacket, CProtocolBase *aSourceProtocol = NULL);
	CProtocolTCP6* Protocol() const { return (CProtocolTCP6*)iProtocol; }

	virtual void IcmpError(TInt aError, TUint aOperationMask, TInt aType, TInt aCode,
		const TInetAddr& aSrcAddr, const TInetAddr& aDstAddr, const TInetAddr& aErrAddr);

	inline void LingerTimeout();

	virtual TInt CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic);

private:
    RMBufAllocator iBufAllocator;
	// Connection state
	enum TTcpStateEnum
		{
		ETcpInitial      = 0x0001,
		ETcpListen       = 0x0002,
		ETcpSynSent      = 0x0004,
		ETcpSynReceived  = 0x0008,
		ETcpEstablished  = 0x0010,
		ETcpFinWait1     = 0x0020,
		ETcpFinWait2     = 0x0040,
		ETcpCloseWait    = 0x0080,
		ETcpClosing      = 0x0100,
		ETcpLastAck      = 0x0200,
		ETcpTimeWait     = 0x0400,
		ETcpClosed       = 0x0800,
		ETcpConnect      = 0x1000
		} iState;

	//
	//.    Send Window Management
	//
	//      SND.UNA - send unacknowledged
	//      SND.NXT - send next
	//      SND.WND - send window
	//      SND.UP  - send urgent pointer
	//      SND.WL1 - segment sequence number used for last window update
	//      SND.WL2 - segment acknowledgment number used for last window update
	//      ISS     - initial send sequence number
	//
	struct TTcpSendSequence
		{
		TTcpSeqNum UNA;
		TTcpSeqNum NXT;
		TTcpSeqNum WL1;
		TTcpSeqNum WL2;
		TTcpSeqNum UP;
		TUint32    WND;
		} iSND;

	//
	//    Receive Window Management
	//
	//      RCV.NXT - receive next
	//      RCV.WND - receive window
	//      RCV.UP  - receive urgent pointer
	//      IRS     - initial receive sequence number
	//
	struct TTcpRecvSequence
		{
		TTcpSeqNum NXT;
		//TTcpSeqNum UP;
		TUint32    WND;
		} iRCV;
	TTcpSeqNum        iFinSeq;

	// Window updates
	TUint32           iFreeWindow;
	TUint32           iAdvertisedWindow;

	// Retransmission control
	TTcpSeqNum	    iLastAck;
	TTcpSeqNum	    iTransmitSeq;
	TTcpSeqNum	    iRecoverSeq;
	TTcpSeqNum	    iSendHigh;
	TUint		    iDupAcks;
	TUint		    iLastWnd;

	// Queue management
	RMBufAsyncPktQ	iSendQ;
	RMBufAsyncPktQ  iRecvQ;
	RMBufTcpFragQ	iFragQ;
	RMBufSockQ	    iSockOutQ;
	RMBufSockQ	    iSockInQ;
	TUint		    iSockOutQLen;
	TUint		    iSockInQLen;
	TUint		    iSockOutBufSize;
	TUint		    iSockInBufSize;
	TUint			iNewData;
	TUint           iPending;
	

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	
	//Window size for startup case
	TUint32 iTcpMaxRecvWin;
    //New window set by the bearer in case of window shrink
	TUint32 iNewTcpWindow;
	//Size of buffer read by the application from the TCP receive buffer
	//but is not transparent while advertising a new TCP window to the sender.	
	TUint32 iHiddenFreeWindow;
	//  Size of Window Shrink
	TUint32 iShrinkedWindowSize;
	// Window size set by user. This will override the default values for the bearers
	TBool   iWindowSetByUser;
	//Flag for socket startup case. No tcp window expand/shrink in this case.
	TBool iSocketStartupCase;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

	// Maximum Segment Sizes
	TUint           iMSS;   //< Maximum set by user
	TUint		    iSMSS;  //< Send MSS
	TUint		    iRMSS;  //< Receive MSS

	// Asynchronous events
	CAsyncCallBack  *iTransmitter;
	CTcpTimer	    *iDelayAckTimer;
	CTcpTimer	    *iRetransTimer;
	CTcpTimer	    *iLingerTimer;

	// RTT timing
	TTime		    iStartTime;		  //< Time at the beginning of connection.
	TUint32	    	iTimeStamp;		  //< Last time stamp taken
	TTcpSeqNum	    iTimingSeq;
	TUint32	    	iTsRecent;

	// RTO calculation
	TUint32	    iRTO;
	TUint32	    iSRTT;
	TUint32	    iMDEV;
	TUint		iBackoff;

	// Delay spike detection
	TUint32     iLastTimeout;         //< Timestamp of last RTO
	
	// Keep-Alive triggering
	TUint32		iLastTriggeredKeepAlive; //< Last triggered keep-alive

	// Congestion control
	TUint32	    iCwnd;
	TUint32     iLwnd;
	TUint32	    iSsthresh;
	TTcpSeqNum  iQuenchSeq;           //< Store right window edge at source quench
	TTcpSeqNum	iPartialSeq;

	// TCP options
	TTcpOptions	iOptions;

	// Server socket state
	TUint		    iListenQueueSize;	  //< Listen queue size.
	TUint		    iConnectCount;	  //< Number active connect attempts;
	CProviderTCP6	*iParent;		  //< Parent socket
	CProviderTCP6   **iListenQueue;       //< Array holding pointers to child sockets
	TBool			iChildDeleted;		//< Flag for notifying parent that a child was deleted by SocketServer

	// Urgent data handling
	TTcpSeqNum        iUpArray[KTcpUpMax];
	TInt              iUpIndex;
	TInt              iUpCount;

	// Large peek offset
	TInt              iCopyOutOffset;

	// SACK book keeping
	SequenceBlockQueue  iSacked;

	// FACK book keeping
	TUint32           iRetranData;

	// Needed state information for F-RTO/DCLOR retransmission
	TUint32			iFRTOsent;  //< True, if rto was sent and no acks have yet arrived
	TTcpSeqNum		iRealSendHigh;  //< iSendHigh is not real with SACK

	// -1=linger disabled, >=0 linger enabled with given time in seconds.
	TInt			iLinger;

	// Window scaling factor for the send window, advertised by the other end.
	TUint8			iSndWscale:4;

	// Window scaling factor for receive window, based on ini settings.
	TUint8			iRcvWscale:4;

	TUint iRetryAck;  // to keep count of the ACKs that inform missing segments

	// Flags
	struct TTcpFlags
		{
		// Additional TCP state
		TUint32		iStarted:1;		  //< Protocol has been started
		TUint32	    iFastRetransMode:1;   //< We're in fast retransmit mode
		TUint32	    iTransmitPending:1;   //< We have segments waiting for flow
		TUint32	    iRetransmitPending:1; //< We have a retransmission waiting for flow
		TUint32     iPeerHasReneged:1;    //< The peer has reneged and might do it again.
		TUint32	    iTiming:1;            //< We're timing a segment round trip
		TUint32     iCloseNotified:1;     //< Application has been notified of received FIN.
		TUint32     iUrgentMode:1;        //< Application is in urgent mode.
		TUint32     iNextIsUrgent:1;      //< Next Write() call contains urgent data.
		TUint32     iFinReceived:1;       //< We have received a FIN from the peer
		TUint32     iDataSentIoctl:1;     //< KIoctlTcpNotifyDataSent ioctl is active
		TUint32     iCompleteRecv:1;      //< Force RSocket::Recv() to complete (urgent data ahead)
		TUint32     iNotifyUrgent:1;      //< Notify application of urgent data.
		TUint32     iDoPMTUD:1;           //< Do path MTU discovery?
		TUint32	    iHaveKeepAlive:1;	  //< Keep-Alive option is set.
		TUint32     iHaveTriggeredKeepAlive:1; //< Triggered Keep-Alive option is set.
		TUint32	    iEcnHaveCongestion:1; //< ECN receiver has got CE bit, but not yet CWR.
		TUint32	    iEcnSendCWR:1;	  //< ECN sender has got ECE. Next seg should have CWR set.
		TUint32	    iKeepInterfaceUp:1;	  //< Storage for KeepInterfaceUp during connection establishment.

		// Enabled TCP options
		TUint32	    iUseTimeStamps:1;     //< We're using timestamps for timing round trips
		TUint32	    iSackOk:1;            //< We're using selective acknowledgements
		TUint32     iOobInline:1;         //< Send out-of-band data inline.
		TUint32     iNoDelay:1;           //< Disable Nagle.
		TUint32	    iCork:1;		  //< Send only full-sized segments.
		TUint32	    iEcn:1;		  //< We're using Explicit Congestion Notification.
		} iFlags;

private:

	//
	// Private implementation methods
	//
	inline TInt Min(TInt a, TInt b) const;
	inline TUint MinUU(TUint a, TUint b) const;	
	inline TInt MinUS(TUint a, TInt b) const;
	inline TInt MinSU(TInt a, TUint b) const;
	inline TInt Max(TInt a, TInt b) const;

	void Stop();
	void FreeQueues();
	void Close();

	inline void EnterState(TTcpStateEnum aState);
	inline TBool InState(TUint aStateSet) const;

	TInt SendSegment(TUint8 aFlags, TTcpSeqNum aSeq, TUint32 aLen = 0);
	TInt SendDataSegment(TTcpSeqNum aSeq, TBool aNagleOverride = EFalse);

	inline TInt SendSegment(TUint8 aFlags);
	inline void SendDelayACK();
	inline TInt SendReset(TTcpSeqNum aSequence, const TSockAddr& aDstAddr, const TSockAddr& aSrcAddr);
	inline TInt SendReset(TTcpSeqNum aSequence);
	inline TInt SendResetNoSync(TTcpSeqNum aAckSequence, const TSockAddr& aDstAddr, const TSockAddr& aSrcAddr);
	inline TInt SendResetNoSync(TTcpSeqNum aAckSequence);
	inline void SchedTransmit();
	inline void SchedRetransmit();
	inline void ReSchedRetransmit();
	inline void CancelTransmit();
	inline void CancelRetransmit();
	inline void CancelDelayACK();
	inline TUint32 TimeStamp();
	inline TUint32 PathMSS();
	inline TUint32 EffectiveMSS();
	inline TUint32 LinkRMSS();
	inline TInt SockInQOffset(TTcpSeqNum aSeq) const;

	inline TTcpSeqNum UrgentHigh() const;
	inline TInt UrgentOffset() const;
	inline TInt UrgentOffset(TInt aIndex) const;
	inline TInt SockInQLen() const;

	inline TBool CanForwardTransmit();
	inline void SetEcn(TInt aFlag);
	inline TBool IsLandAttack(RMBufRecvInfo *aInfo);

	void RememberUrgentPointer(TTcpSeqNum aUp);
	void ForgetUrgentPointer();
	TInt GetUrgent(TInt& aChar, TUint aOptions);

	void Transmit();
	void ClearRTT();
	void UpdateRTO(TUint32 aRTT);
	void ResetRTO();
	void ResetCwnd(TUint aSMSS);
	void SchedMsl2Wait();
	void ProcessSegments();
	void SendSegments(TBool aNagleOverride = EFalse);
	void RetransmitTimeout();
	TBool RetransmitSegments();
	void ClearSYNSettings();

	/**
	 * Reduce congestion window. The following events may cause this: 1. ICMP Source Quench,
	 * 2. notification from link layer, 3. ECN congestion echo. The method ensures that congestion
	 * window is not reduced more frequently than once in RTT.
	 *
	 * @return ETrue if cwnd was reduced, EFalse if it was not.
	 */
	TBool SourceQuench();

	void SendSYN();
	void CompleteIoctl(TInt aError);
	void Detach();
	void Expire();

	void ReadDestinationCache();
	void StoreDestinationCache();

	//TSW error:JHAA-82JBNG -- FIN retransmission 
 	//Modifying the function to return TBool
	TBool DetachIfDead();
	void DetachFromInterface();

	/**
	 * Check the size of receive buffers and determine if window scaling is needed
	 * on our part.
	 *
	 * @return The scale factor that would be required due to buffer size settings.
	 */
	TUint8 NeedWindowScale();
	
	void SpuriousTimeout(TUint aAcked);

	inline void StoreKeepInterfaceUp();

	// small methods for keep-alive option
	void KeepAliveTimeout();  //< Keep-Alive related timeout has expired.
	void ResetKeepAlives();   //< Resetting keep-alive probe timer when connection becomes idle
	inline TBool CanFireKeepAlives()  //< ETrue when keep-alive probe timer can be activated
		{ return iSND.NXT == iSND.UNA && iSND.WND > 0 && iFlags.iHaveKeepAlive; }
	inline TBool CanTriggerKeepAlive(); //< ETrue if TCP should send triggered keep-alive

	TInt Send(TDualBufPtr& aBuf, TInt aLength, TUint aOptions);
	TInt Recv(TDualBufPtr& aBuf, TInt aLength, TUint aOptions);

	static TInt SenderCallBack(TAny* aProviderTCP);
	static TInt ReceiverCallBack(TAny* aProviderTCP);
	static TInt DelayAckCallBack(TAny* aProviderTCP);
	static TInt TransmitterCallBack(TAny* aProviderTCP);
	static TInt RetransmitterCallBack(TAny* aProviderTCP);
	static TInt LingerTimerCallBack(TAny* aProviderTCP);

#ifdef _LOG
public:
	const TText *TcpState(TUint aState = ~0L);
#endif

	};

	
//
// Private implementation methods
//
inline TInt CProviderTCP6::Min(TInt a, TInt b) const { return (a < b) ? a : b; }
inline TUint CProviderTCP6::MinUU(TUint a, TUint b) const { return (a < b) ? a : b; }
inline TInt CProviderTCP6::MinUS(TUint a, TInt b) const 
	{
	if(a > KMaxTInt16)
		return b;
	else
		return Min(TInt(a), b);
	}
inline TInt CProviderTCP6::MinSU(TInt a, TUint b) const
	{
	if(b > KMaxTInt16)
		return a;
	else
		return Min(a, TInt(b));
	}
inline TInt CProviderTCP6::Max(TInt a, TInt b) const { return (a > b) ? a : b; }

inline void CProviderTCP6::EnterState(TTcpStateEnum aState)
	{
	LOG(if (aState != iState)
		Log::Printf(_L("\ttcp SAP[%u] EnterState(): %s --> %s"),
		(TInt)this, TcpState(), TcpState(aState)));
	iState = aState;
	}

inline TBool CProviderTCP6::InState(TUint aStateSet) const
	{
	return (aStateSet & iState) != 0;
	}


inline TInt CProviderTCP6::SendSegment(TUint8 aFlags)
	{
	return SendSegment(aFlags, iSND.NXT, 0);
	}

inline void CProviderTCP6::SendDelayACK()
	{
	iDelayAckTimer->Start(Protocol()->AckDelay());
	}

inline TInt CProviderTCP6::SendReset(TTcpSeqNum aSequence, const TSockAddr& aDstAddr, const TSockAddr& aSrcAddr)
	{
	return Protocol()->SendControlSegment(iFlow.Status() == EFlow_READY ? &iFlow : NULL,
		aDstAddr, aSrcAddr,
		KTcpCtlRST, aSequence, 0);
	}

inline TInt CProviderTCP6::SendReset(TTcpSeqNum aSequence)
	{
	return SendReset(aSequence, iFlow.FlowContext()->RemoteAddr(), iFlow.FlowContext()->LocalAddr());
	}

inline TInt CProviderTCP6::SendResetNoSync(TTcpSeqNum aAckSequence, const TSockAddr& aDstAddr, const TSockAddr& aSrcAddr)
	{
	return Protocol()->SendControlSegment(iFlow.Status() == EFlow_READY ? &iFlow : NULL,
		aDstAddr, aSrcAddr,
		KTcpCtlRST|KTcpCtlACK, 0, aAckSequence);
	}

inline TInt CProviderTCP6::SendResetNoSync(TTcpSeqNum aAckSequence)
	{
	return SendResetNoSync(aAckSequence, iFlow.FlowContext()->RemoteAddr(), iFlow.FlowContext()->LocalAddr());
	}

inline void CProviderTCP6::SchedTransmit()
	{
	iTransmitter->CallBack();
	}

inline void CProviderTCP6::SchedRetransmit()
	{
	iRetransTimer->Start(iRTO);
	}

inline void CProviderTCP6::ReSchedRetransmit()
	{
	iRetransTimer->Restart(iRTO);
	}

inline void CProviderTCP6::CancelTransmit()
	{
	iTransmitter->Cancel();
	iFlags.iTransmitPending = EFalse;
	}

inline void CProviderTCP6::CancelRetransmit()
	{
	iRetransTimer->Cancel();
	iFlags.iRetransmitPending = EFalse;
	}

inline void CProviderTCP6::CancelDelayACK()
	{
	iDelayAckTimer->Cancel();
	}

inline TUint32 CProviderTCP6::TimeStamp()
	{
	TTime now;
	now.UniversalTime();
#ifdef I64LOW
	return I64LOW(now.Int64());
#else
	return (TUint32)now.Int64().GetTInt();
#endif
	}

/**
 * Return maximum segment size allowed by transmission path. Following tradition,
 * the value represents the maximum number of data bytes that can be passed in
 * a TCP segment with no option headers.
 */
inline TUint32 CProviderTCP6::PathMSS()
	{
	ASSERT(iFlow.FlowContext() != 0);
	return Min(iMSS, iFlow.FlowContext()->PathMtu() - iFlow.FlowContext()->HeaderSize() - KTcpMinHeaderLength);
	}

/**
 * Return effective send MSS. Returns the maximum number of data bytes that can
 * be passed in a TCP segment. Checks both path MTU and MSS advertised by the receiver.
 * and subtracts the number of bytes taken up by TCP options.
 * Finally, ensure that MSS is at most half of the socket output buffer size.
 */
inline TUint32 CProviderTCP6::EffectiveMSS()
	{
	ASSERT(iFlow.FlowContext() != 0);
	return Min(Max(Min(iSMSS, PathMSS()) - iOptions.Length(), KTcpMinimumMSS), iSockOutBufSize>>1);
	}

/**
 * Return maximum segment that can be received through the current network interface.
 * Following tradition, this method returns the maximum number of data bytes assuming
 * the TCP header contains no optons.
 */
inline TUint32 CProviderTCP6::LinkRMSS()
	{
	ASSERT(iFlow.FlowContext() != 0);
	return Min(iMSS, iFlow.FlowContext()->InterfaceRMtu() -
		iFlow.FlowContext()->HeaderSize() - KTcpMinHeaderLength);
	}

/**
 * Return number of bytes in receive queue without out-of-band data.
 */
inline TInt CProviderTCP6::SockInQLen() const
	{
	return iFlags.iOobInline ? iSockInQLen : iSockInQLen - iUpCount;
	}

inline TInt CProviderTCP6::SockInQOffset(TTcpSeqNum aSeq) const
	{
	return aSeq - iRCV.NXT + iSockInQLen;
	}

/**
 * Return sequence number of highest urgent pointer seen so far.
 * Assumes iUpCount > 0.
 */
inline TTcpSeqNum CProviderTCP6::UrgentHigh() const
	{
	return iUpArray[(iUpIndex + iUpCount - 1) % KTcpUpMax];
	}

/**
 * Return offset to pending urgent data. A negative value means
 * there is no pending urgent data.
 */
inline TInt CProviderTCP6::UrgentOffset() const
	{
	return iUpCount ? SockInQOffset(UrgentHigh()) - 1 : -1;
	}

/**
 * Return byte offset of the urgent pointer stored at given index.
 */
inline TInt CProviderTCP6::UrgentOffset(TInt aIndex) const
	{
	return aIndex < iUpCount ? SockInQOffset(iUpArray[(iUpIndex + aIndex) % KTcpUpMax]) - 1 : KMaxTInt;
	}

/**
 * Checks if sending new data is possible without being limited by application or
 * sender or receiver window. This is used for checking whether F-RTO can be applied for sending
 * new data instead of retransmitting after RTO.
 *
 * @return ETrue if some unsent data can be transmitted. 
 */
inline TBool CProviderTCP6::CanForwardTransmit()
	{
	return Min(iSND.WND, iSockOutQLen) > (iSND.NXT - iSND.UNA);
	}


/**
 * Set the status of ECN for this SAP. Two things: set the SAP-specific status flag, and
 * signal the flag to the IP layer (iface.cpp) by using flow options.
 *
 * @param aFlag	0 = disable ECN, 1 = enable ECN with ECT(1), 2 = enable ECN with ECT(0)
 */
inline void CProviderTCP6::SetEcn(TInt aFlag)
	{
	TPckgBuf<TInt> ecnopt(aFlag);
	iFlow.FlowContext()->SetOption(KSolInetIp, KSoIpEcn, ecnopt);
	iFlags.iEcn = (aFlag != 0);
	}

/**
Stores the value of KeepInterfaceUp set for the current flow.
*/
inline void CProviderTCP6::StoreKeepInterfaceUp()
	{
	TPckgBuf<TInt> ifup;
	GetOption(KSolInetIp, KSoKeepInterfaceUp, ifup);
	iFlags.iKeepInterfaceUp = (ifup() != 0) ? 1 : 0;
	}


/**
Returns ETrue, if source and destination have equal IP address and port.
*/
inline TBool CProviderTCP6::IsLandAttack(RMBufRecvInfo *aInfo)
	{
	return TInetAddr::Cast(aInfo->iSrcAddr).CmpAddr(TInetAddr::Cast(aInfo->iDstAddr));
	}

#endif
