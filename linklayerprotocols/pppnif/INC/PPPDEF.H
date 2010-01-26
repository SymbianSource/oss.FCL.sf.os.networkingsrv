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

#ifndef __PPPDEF_H__
#define __PPPDEF_H__

#include <nifutl.h>
#include <nifmbuf.h>
#include <networking/pppsock.h>

#if !defined INLINE
#if defined( _DEBUG )
#define	INLINE
#else
#define	INLINE	inline
#endif
#endif

class CPppLcp;
class CNifFactory;
class RPppOptionList;

//
// PPP Interface to Link protocols - accepts frames from Lcp's demux
//

class MPppRecvr
	{
public:
	MPppRecvr(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId=KPppIdUnknown);
// The following constructor and Init function is a provisional fix,
// since according to Symbian Coding Standard M classes (mixins) are
// not supposed to have state - data members - and should contain no
// costructors or destructors.  MPppRecvr is not in fact a valid
// "mixin class", because it does not fulfill these requirements.
	void Init(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId=KPppIdUnknown);
	virtual ~MPppRecvr();
	//
	inline TInt SendFrame(RMBufChain& aPacket);
	inline void SetId(TUint aPppId);
	inline void SetPhase(TPppPhase aPhase);
	void Register(TUint aPpdId=KPppIdAsIs);
	void Reregister(TUint aPpdId, TPppPhase aPhase);
	void Deregister();
	void Deque();


/**
   Delivers a data link frame to this receiver for processing.
   @param aPacket [in] A reference to the RMBufChain object containing
   the data link frame.
   @return EFalse
*/
   virtual TBool RecvFrame(RMBufChain& aPacket)=0;

/**
   Notifies that a CRC error has been detected in the data link frame
   that has been received.
*/
	virtual void FrameError()=0;

/**
   Deactivates this receiver object, preventing any processing of the
   data link frames being received.
*/
	virtual void KillProtocol()=0;

	IMPORT_C virtual void FlowOn();

/**
   Notifies this receiver that the lower layer is up, i.e. that the
   PPP link is now in a phase when this receiver is active and that
   this receiver should get ready for processing the data frames being
   received, as the data frames being received will be delivered to
   it.
*/
	virtual void LowerLayerUp();

/**
   Notifies this receiver that the lower layer is down, i.e. that the
   PPP link is now in a phase when this receiver is inactive and the
   data frames being received will not be delivered to it.
   @param aStatus [in] A status code that may indicate a special
   reason for which this receiver was deactivated.
*/
	virtual void LowerLayerDown(TInt aStatus);
protected:
	CNifFactory* FindPppFactoryL(const TDesC& aFilename, TUid aUid2, CObjectCon& aCon);
	MPppRecvr();
//	virtual void PhaseChanged(TPppPhase aOldPhase, TPppPhase aNewPhase, TInt aReason)=0;
protected:
	TUint iPppId;
	TPppPhase iActivePhase;
	CPppLcp* iPppLcp;
	TInt iPppAbortCode;
private:
	friend class CPppLcp;
	TDblQueLink iPppRecvrListLink;
	};

class RPppOption;
class TPppFcs32;
class MPppOptionHandler;

//
// PPP Options
//

class RPppOptionList : public RMBufPktQ
	{
public:
	void SetL(RMBufChain& aList, TInt aLength=0);
	TInt CopyL(RMBufChain& aPacket, TInt aHdrLen=0) const;
	TInt RemoveOption(RPppOption& aOption);
	TInt ReplaceOption(RPppOption& aOption);
	TInt RemoveOptions(RPppOptionList& aRejectList);
	TInt ReplaceOptions(RPppOptionList& aReplaceList);
	void CreatePacketL(RMBufPacket& aPacket, TUint aPppId, TUint8 aType, TUint8 aId, TBool aCreateEmptyPacket = EFalse);
	void CreateAndAddL(TUint8 aType);
	void CreateAndAddL(TUint8 aType, TUint8 aValue);
	void CreateAndAddL(TUint8 aType, TUint16 aValue);
	void CreateAndAddL(TUint8 aType, TUint32 aValue);
	void CreateAndAddL(TUint8 aType, const TUint8*  aBuf, TInt aLen );	// CSW
	void CreateAndAddL(TUint8 aType, TPtrC8& aValue);
	void Crc32(TPppFcs32& aFcs, TBool aInitFcs=ETrue);
	TBool EqualTo(RPppOptionList& aList);
	TBool IsSubsetOf(RPppOptionList& aList);
	};

class MPppOptionsExtender
	{
public:
	MPppOptionsExtender();
	void ExtOptRegister(MPppOptionHandler* aHandler);
	void ExtOptDeregister(MPppOptionHandler* aHandler);
	void ExtOptNegotiationStarted();
	void ExtOptNegotiationAborted();
	void ExtOptNegotiationComplete();
	void ExtOptFillinConfigRequestL(RPppOptionList& aRequestList);
	void ExtOptCheckConfigRequest(RPppOption& aOption, RPppOptionList &aAckList, RPppOptionList &aNakList, RPppOptionList &aRejList);
	void ExtOptApplyConfigRequest(RPppOption& aOption);
	void ExtOptRecvConfigAck(RPppOption& aOption);
	void ExtOptRecvConfigNak(RPppOption& aOption, RPppOptionList& aReqList);
	void ExtOptRecvConfigReject(RPppOption& aOption, RPppOptionList& aReqList);
	MPppOptionHandler* ExtOptLookup(TUint8 aOptId);
private:
	TDblQue<MPppOptionHandler> iExtOptHandlerList;
	};

//
// PPP Finite State Machine
//

enum TPppFsmState
	{
	EPppFsmInitial, EPppFsmStarting,
	EPppFsmClosed, EPppFsmStopped,
	EPppFsmClosing, EPppFsmStopping,
	EPppFsmReqSent, EPppFsmAckRecvd,
	EPppFsmAckSent, EPppFsmOpened
	};

class MPppFsm : public MPppRecvr, public MTimer
	{
public:
	MPppFsm(CPppLcp* aPppLcp, TPppPhase aPhase, TUint aPppId);
	virtual ~MPppFsm();
	void FsmConstructL();
	// Open the state machine
	TInt FsmOpen();
	// Close the state machine
	void FsmClose(TInt aReason);
	// Abort, similar to close, but enters stopped/closed states
	void FsmAbort(TInt aReason);
	// Enquire if this layer is in the Opened state
	TBool FsmIsThisLayerOpen();
	TUint8 FsmNewId();
	void FsmRejectPacket(RMBufChain& aPacket, TUint aReason=KPppLcpCodeReject, TUint aPppId=KPppIdAsIs);
	void TerminateLink();
	inline void ChangeTimers(TBool aLengthen);
	inline void ConnectionPersist(TBool aPersist);


protected:
	// Open the layer below
	virtual TInt FsmLayerStarted()=0;
	// Close the layer below
	virtual void FsmLayerFinished(TInt aReason=KErrNone)=0;
	// Signal up event to next layer above
	virtual void FsmLayerUp()=0;
	// Signal down event to next layer above
	virtual void FsmLayerDown(TInt aReason=KErrNone)=0;
	// Fillin Config Request to be sent
	virtual void FsmFillinConfigRequestL(RPppOptionList& aRequestList)=0;
	// Check options in a recvd config request
	virtual void FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList)=0;
	// Apply options in a recvd config request - no-one has complained
	virtual void FsmApplyConfigRequest(RPppOptionList& aRequestList)=0;
	// Recvd a Config Ack
	virtual void FsmRecvConfigAck(RPppOptionList& aReplyList)=0;
	// Recvd a Config Nak - The associated original request is in aReqList
	virtual void FsmRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList)=0;
	// Recvd a Config Reject - The associated original request is in aReqList
	virtual void FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList)=0;
	// Recvd an unrecognised opcode - has a default implementation
	virtual void FsmTerminationPhaseComplete()=0;
	virtual TBool FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);	
	IMPORT_C virtual void KillProtocol();
	virtual TBool FsmAckOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);
	virtual TBool FsmRejectOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);
	virtual TBool FsmConfigRequestOptionsValid(RPppOptionList& aList);

	//PG Moved to protected so it can be called by CPppCcp::KillProtocol
	void SetState(TPppFsmState aState);

 	//SL Added to return current state for CPppLcp::LinkRecv
 	inline TPppFsmState FsmState() const;

	TUint8* NewPacket(RMBufPacket& aPkt, TUint aLength);
 
protected:
	/**
 	Is RFC compliant termination phase for Administrative close enabled?	*/
 	TBool iTerminateRequestEnabled;
 	
 	/**
	Is RFC compliant termination phase for incoming TerminateRequest enabled?	*/
	TBool iTerminateAckEnabled;
	
	/** Why PPP is shutting down */
	TInt iFsmTerminationCauseError;
	 		
private:
	// MPppRecvr upcalls handled by FSM
	IMPORT_C virtual TBool RecvFrame(RMBufChain& aPacket);
	IMPORT_C virtual void FrameError();
  	IMPORT_C virtual void LowerLayerUp();
	IMPORT_C virtual void LowerLayerDown(TInt aStatus=KErrNone);
	// MTimer upcall
	IMPORT_C virtual void TimerComplete(TInt aStatus);
	// Internal functions
	void InitRestartCountForConfig();
	void InitRestartCountForTerminate();
	void ZeroRestartCount();
	void SendConfigRequest();
	void SendInitialConfigRequest();
	void SendConfigRequestAfterNak(RPppOptionList& aOptList);
	void SendConfigRequestAfterReject(RPppOptionList& aOptList);
	void SendConfigReply(RPppOptionList& aOptList, TUint8 aType, TUint8 aId);
	void SendInitialTerminateRequest();
	void SendTerminateRequest();
	void SendTerminateAck(TUint8 aId);
	void ProcessConfig(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
    TBool ProcessEmptyConfigReq();
	void ProcessReject(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
	void ProcessTerminate(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
	void ThisLayerUp();
	void ThisLayerDown();
	void ThisLayerStarted();
	void ThisLayerFinished();
	TInt InitialiseConfigRequest();

	void ReadIniFileL();
	inline TBool MaxFailureExceeded() const;
	inline void  DecrementMaxFailure();
	inline void	 InitMaxFailure();
	
public:
	const TText* __iFsmName;
private:
	TUint8 iCurrentId;
	TUint iTerminateId;
	TUint iRequestId;
	RPppOptionList iRequestList;
	// RFC 1661 4.6
	TInt iMaxRestartConfig;
	TInt iRestartCount;

	TPppFsmState iState;
	
	// RFC 1661 4.6
	TInt iWaitTimeConfig;
	TInt iWaitTime;
	TInt iConsecCfgReq;
	TUint32 iLastCfgReqFcs;

	// RFC 1661 4.6
	TInt iMaxFailureConfig;
	TInt iMaxFailureCount;
	
	//
 	// Termination phase support.
 	// CDMA2000 requires RFC compliant behavior for terminating the PPP link.	
 	//
 	 	
 	/**
 	Maximum TerminateRequest transmissions.	*/
 	TInt iMaxTerminateRequest;
 	
 	/**
 	TerminateRequest retransmission timeout */
 	TInt iTerminateRequestTimeout;
	
	/**
	Terminate Ack timeout period*/
	TInt iTerminateAckTimeout;
	
	
	TBool iLengthenTimers:1;
	TBool iPersist:1;
	/** If MPppFsm is to be terminated.*/
	TBool iFsmTermination:1;	
	/** The physical layer may block on its own retries when trying to send an initial config packet
	which then blocks the LCP from shutting down until attempts to send a disconnect packet have timed
	out. To allow for a quick shutdown in such cases where there is no evidence that the link layer
	is working the EPppFsmStarting state also sets this flag. This is driven by INC85191.
	*/
	TBool iNoEvidenceOfPeer:1;
	
	/**
	Whether restart timer period doubles or not */
	TBool iWaitTimeNoIncrease:1;
	};


inline void MPppFsm::ChangeTimers(TBool aLengthen)
	{
	iLengthenTimers=aLengthen;
	}

inline void MPppFsm::ConnectionPersist(TBool aPersist)
	{
	iPersist=aPersist;
	}

inline TPppFsmState MPppFsm::FsmState() const
	{
	return iState;
	}

inline TBool MPppFsm::MaxFailureExceeded() const
	{
	return iMaxFailureCount == 0;
	}

inline void  MPppFsm::DecrementMaxFailure()
	{
	iMaxFailureCount--;
	}

inline void	 MPppFsm::InitMaxFailure()
	{
	iMaxFailureCount = iMaxFailureConfig;
	}

//
// [lcp]
// PPPEnableMaxFailure = 0:disabled/1:enabled
// PPPMaxFailureCount = 5 default
_LIT(LCPSECTIONNAME,"lcp");
_LIT(PPPMAXFAILUREENTRYNAME_ENABLE,"PPPEnableMaxFailure");
_LIT(PPPMAXFAILUREENTRYNAME_COUNT,"PPPMaxFailureCount");
const TInt KPppMaxFailureDefault = 5;

// [lcp]
// PPPEnableMaxRestart = 0:disabled/1:enabled
// PPPMaxRestartCount = 20 default
_LIT(PPPMAXRESTARTENTRYNAME_ENABLE,"PPPEnableMaxRestart");
_LIT(PPPMAXRESTARTENTRYNAME_COUNT,"PPPMaxRestartCount");

// [lcp]
// PPPEnableRestartTimer = 0:disabled/1:enabled
// PPPRestartPeriod= 3000 default 3 Seconds
_LIT(PPPRESTARTTIMERENTRYNAME_ENABLE,"PPPEnableRestartTimer");
_LIT(PPPRESTARTTIMERENTRYNAME_PERIOD,"PPPRestartPeriod");
_LIT(PPP_RESTARTTIMER_ENTRYNAME_MODE,"PPPRestartTimerMode");


//
// Termination sequence support
// Doesn't follow the macro naming convention deliberately
//

//[lcp]
// PPPEnableMaxTerminateRestart = 0:disabled/1:enabled
_LIT(TERMINATE_REQUEST_ENABLE, "PPPEnableTerminateRequest");

// PPPEnableMaxTerminateRestart = 0:disabled/1:enabled
// PPPMaxTerminateRequestCount: Max TerminateRequests to be sent
_LIT(MAX_TERMINATE_REQUEST_ENABLE, "PPPEnableMaxTerminateRequest");
_LIT(MAX_TERMINATE_REQUEST_COUNT,  "PPPMaxTerminateRequestCount");
 
 
// [lcp]
// PPPEnableTerminateRestartTimer 0:disabled/1:enabled
// PPPTerminateRestartPeriod time to TerminateRequest retransmission
_LIT(TERMINATE_REQUEST_TIMER_ENABLE, "PPPEnableTerminateRequestTimer");
_LIT(TERMINATE_REQUEST_TIMER_PERIOD, "PPPTerminateRequestPeriod");

// [lcp]
// PPPEnableTerminateAck = 0: disabled / 1: enabled
_LIT(TERMINATE_ACK_ENABLE,       "PPPEnableTerminateAck");

// PPPEnableTerminateAckTimer = 0: disabled / 1: enabled
// PPPTerminateAckPeriod the time to wait after sending TerminateAck
_LIT(TERMINATE_ACK_TIMER_ENABLE, "PPPEnableTerminateAckTimer");
_LIT(TERMINATE_ACK_TIMER_PERIOD, "PPPTerminateAckPeriod");

#endif
