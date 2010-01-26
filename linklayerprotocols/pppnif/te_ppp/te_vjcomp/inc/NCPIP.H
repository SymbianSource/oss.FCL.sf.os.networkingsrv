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
// DUMMYNCPIP.H
// 
//

#if !defined(__NCPIP_H__)
#define __NCPIP_H__

#include <in_iface.h>
#include "PPPBASE.H"

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

class CPppNcpIp : public CNifIfBase, public MPppFsm
	{
public:
	CPppNcpIp(CPppLcp* aLcp);
	~CPppNcpIp();
	void  ConstructL();
	// Esock
	virtual void BindL(TAny *aId);
	virtual TInt State();
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource);
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	static void FillInInfo(TNifIfInfo& aInfo);

	void SendFlowOn();
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
	
	/* In this dummy version, the LoadVJCompressorL() and UnloadVJCompressorL()
	   are made public functions rather than private */
	CVJCompressorIf*	LoadVJCompressorL(TInt aMaxSlots, TInt aCompressConnId);
	CVJDeCompressorIf*	LoadVJDeCompressorL(TInt aMaxSlots);

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
	virtual TBool FsmOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);
	virtual TBool FsmConfigRequestOptionsValid(RPppOptionList& aList);
private:
	TInt	SendProtFrame(RMBufChain& aPacket, TUint aProtocol);
	static TInt SendCallBack(TAny* aCProtocol);
	void DoSend();
	TInt PresetAddr(TUint32& aAddr, const TDesC& aVarName);


private:
	//
	CProtocolBase* iNetwork;
	RMBufPktQ iSendQ;
	CAsyncCallBack* iSendCallBack;

	TInterfaceName iIfName;

	//
	TBool iLowerFlowOn;
	TBool iUpperFlowOn;
	//
	TPppExtraRecvr<CPppNcpIp> iIpRecvr;
	TPppExtraRecvr<CPppNcpIp> iVjCompTcpRecvr;
	TPppExtraRecvr<CPppNcpIp> iVjUncompTcpRecvr;

	// Negotiated information
	TUint32 iPrimaryDns;
	TUint32 iSecondaryDns;
	TUint32 iPrimaryNbns;	// Held in case of a future NetBios protocol
	TUint32 iSecondaryNbns;	// Held in case of a future NetBios protocol
	TUint32 iRemoteAddr;
	TUint32 iLocalAddr;
	TUint32 iNetworkMask;
	CVJDeCompressorIf*	iVJDecompressor;
	CVJCompressorIf*	iVJCompressor;
	CObjectCon*			iVJCompressorCon;
	CObjectCon*			iVJDeCompressorCon;
	TBool				iVJCompressionOn; // Did the user enable compression ???
	TUint				iMaxVJSlots;
	TBool				iCompressConnId;
	};

#endif
