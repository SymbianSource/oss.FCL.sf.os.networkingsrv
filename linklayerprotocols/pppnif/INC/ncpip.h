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
// ncpip4.H
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined(__NCPIP4_H__)
#define __NCPIP4_H__

#include <in_iface.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/commsdebugutility.h>
#include <comms-infras/es_protbinder.h>
#include <networking/ppplcp.h>
#include <networking/pppbase.h>

const TUint KPppIpIfaceFeatures =
//		KIfIsLoopback |
		KIfIsPointToPoint |
//		KIfCanBroadcast |
//		KIfCanMulticast	|
//		KIfCanSetMTU |
//		KIfHasHardwareAddr |
//		KIfCanSetHardwareAddr |
		0;

const TUint KIpcpSendPriority = 10;
//const TUint KPppIdIp = 0x0021;	// MOVED TO VJ.H

const TUint KPppIdIpcp = 0x8021;
const TUint KPppIdCompressed = 0x00fd;
const TUint KPppIdHeadComp = 0x004f;
const TUint KPppIdHeadCompCp = 0x804f;

const TUint8 KPppIpcpOptIpAddresses = 1;
const TUint8 KPppIpcpOptIpCompressionProtocol = 2;
const TUint8 KPppIpcpOptIpAddress = 3;
const TUint8 KPppIpcpOptPrimaryDnsAddress = 129;
const TUint8 KPppIpcpOptSecondaryDnsAddress = 131;
// NetBios
const TUint8 KPppIpcpOptPrimaryNbnsAddress = 130;
const TUint8 KPppIpcpOptSecondaryNbnsAddress = 132;

const TUint KSlashChar='\\';

class MIpcpIpRecvr : public MPppRecvr
	{
public:
	MIpcpIpRecvr(CPppLcp* aLcp);
	virtual TBool RecvFrame(RMBufChain& aPacket);
	virtual void  FrameError();
	virtual void  KillProtocol();
	};

class MIpcpTcpVjCompRecvr : public MPppRecvr
	{
public:
	MIpcpTcpVjCompRecvr(CPppLcp* aLcp);
	virtual TBool RecvFrame(RMBufChain& aPacket);
	virtual void  FrameError();
	virtual void  KillProtocol();
	};

class MIpcpTcpVjUncompRecvr : public MPppRecvr
	{
public:
	MIpcpTcpVjUncompRecvr(CPppLcp* aLcp);
	virtual TBool RecvFrame(RMBufChain& aPacket);
	virtual void  FrameError();
	virtual void  KillProtocol();
	};

class CVJCompressorIf;
class CVJDeCompressorIf;

class CPppLcp;
class CPppNifSubConnectionFlow;

NONSHARABLE_CLASS(CPppBinderIp4) : public CBase, public MPppFsm,
								   public ESock::MLowerDataSender, public ESock::MLowerControl
/**
Implements IPCP and support for IP datagrams (RFC 1332)

@internalComponent
*/
	{
public:
   	static CPppBinderIp4* NewL(CPppLcp* aLcp);
	~CPppBinderIp4();

    //-=========================================================
    // MLowerDataSender methods
    //-=========================================================
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aPdu);
	
    //-=========================================================
	// MLowerControl methods
    //-=========================================================
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(ESock::MLowerControl::TBlockOption /*aOption*/);
    virtual TInt GetConfig(TBinderConfig& aConfig);
    virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);

    //-=========================================================
    // Callthrough from MFlowBinderControl instance
    //-=========================================================
    MLowerDataSender* BindL(ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aControl);
    void UnBind(ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aControl);

	//
	TBool MatchesUpperControl(const ESock::MUpperControl* aUpperControl) const;
	
	void SendFlowOn();
    void Error(TInt aError);
	TInt Notification(TAgentToNifEventType aEvent);

	//
	void RecvIp(RMBufChain& aPacket);
	void RecvVjCompTcp(RMBufChain& aPacket);
	void RecvVjUncompTcp(RMBufChain& aPacket);
	void IpFrameError();
	void VjCompTcpFrameError();
	void VjUncompTcpFrameError();
	void IpKillProtocol();
	void VjCompTcpKillProtocol();
	void VjUncompTcpKillProtocol();

	//

protected:

	// PPP FSM Upcalls
	virtual TInt FsmLayerStarted();
	virtual void FsmLayerFinished(TInt aReason=KErrNone);
	virtual void FsmLayerUp();
	virtual void FsmLayerDown(TInt aReason=KErrNone);
	virtual void FsmFillinConfigRequestL(RPppOptionList& aRequestList);
	virtual void FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList);
	virtual void FsmApplyConfigRequest(RPppOptionList& aRequestList);
	virtual void FsmRecvConfigAck(RPppOptionList& aReplyList);
	virtual void FsmRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	virtual void FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	virtual void FsmTerminationPhaseComplete();
	virtual TBool FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
	virtual void KillProtocol();
private:
   	void  ConstructL();
   	CPppBinderIp4(CPppLcp* aLcp);

	MLowerDataSender::TSendResult SendProtFrame(RMBufChain& aPacket, TUint aProtocol);
	static TInt SendCallBack(TAny* aCProtocol);
	void DoSend();
	CVJCompressorIf*	LoadVJCompressorL();
	CVJDeCompressorIf*	LoadVJDeCompressorL();
	
	inline CPppLcp* Flow();

    __FLOG_DECLARATION_MEMBER;
private:
	//
    CPppLcp* iPppNifSubConnectionFlow;

	ESock::MUpperDataReceiver* iUpperReceiver;
    ESock::MUpperControl* iUpperControl;

    TBool iAuthenticated;
	RMBufPktQ iSendQ;
	CAsyncCallBack* iSendCallBack;

	TInterfaceName iIfName;

	//
	ESock::MLowerDataSender::TSendResult iLowerFlowOn;
	ESock::MLowerDataSender::TSendResult iUpperFlowOn;
	//
	TPppExtraRecvr<CPppBinderIp4> iIpRecvr;
	TPppExtraRecvr<CPppBinderIp4> iVjCompTcpRecvr;
	TPppExtraRecvr<CPppBinderIp4> iVjUncompTcpRecvr;

	// Negotiated information
	TUint32 iPrimaryDns;
	TUint32 iSecondaryDns;

	/** Held in case of a future NetBios protocol */
	TUint32 iPrimaryNbns;
	
	/** Held in case of a future NetBios protocol */
	TUint32 iSecondaryNbns;

	TUint32 iRemoteAddr;
	TUint32 iLocalAddr;
	TUint32 iNetworkMask;
	CVJDeCompressorIf*	iVJDecompressor;
	CVJCompressorIf*	iVJCompressor;
	CObjectCon*			iVJCompressorCon;
	CObjectCon*			iVJDeCompressorCon;
	
	/** ETrue if the user enabled VJ compression */
	TBool				iVJCompressionOn;
	
	TUint				iMaxVJSlots;
	TBool				iCompressConnId;
	};

//
// Inline functions
//

CPppLcp* CPppBinderIp4::Flow()
	{
	return iPppNifSubConnectionFlow;
	}


#endif // __PPPBINDERIP4_H__
