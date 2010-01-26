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
// dummy PPPCCP.H
// we keep the same name because we compile some source file in networking/ppp/ which have an include to pppccp.h
// 
//

#if !defined(__PPPCCP_H__)
#define __PPPCCP_H__

#include <networking/pppbase.h>
#include "PPPCFG.H"

const TUint16 KPppIdCcp = 0x80fd;
const TUint8 KPppCcpResetReq	= 14;	
const TUint8 KPppCcpResetAck	= 15;	

const TUint8 KPppCcpOptOui       = 0;
const TUint8 KPppCcpOptPred1     = 1;
const TUint8 KPppCcpOptPred2     = 2;
const TUint8 KPppCcpOptPuddle    = 3;
const TUint8 KPppCcpOptHP        = 16;
const TUint8 KPppCcpOptStac      = 17;
const TUint8 KPppCcpOptMsoft     = 18;
const TUint8 KPppCcpOptGandalf   = 19;
const TUint8 KPppCcpOptV42bis    = 20;
const TUint8 KPppCcpOptBSDLzw    = 21;
const TUint8 KPppCcpOptMagnalink = 24;
const TUint8 KPppCcpOptDeflate   = 26;
const TUint8 KPppCcpOptReserved  = 255;

const TUint8 KCcpSendPriority    = 10;

//const TUint32 KPppCcpOptMsoftSupport = 1;
const TUint32 KUidPppCompressionModule = 0x10000C47;
const TUint32 KUidUnicodePppCompressionModule = 0x10003d45;
const TUint32 KUidPppMsCompression = 0x10000C49;

class CPppIniData;
class CNifFactory;

/**
Implements the PPP Compression Control Protocol (RFC 1961)
*/
class CPppCcp : public CBase, public MPppFsm
	{
public:
	CPppCcp(CPppLcp* aLcp);
	~CPppCcp();
	static CPppCcp* NewL(CPppLcp* aLcp);
	void  ConstructL();
	void	RemoveRegistration();
	// Esock
	virtual TInt State();
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource);
	virtual void Info(TNifIfInfo& aInfo) const;
	static void FillInInfo(TNifIfInfo& aInfo);

	void SendFlowOn();
	//
	void Recv(RMBufChain& aPacket);
	//
	void KillProtocol();
	void FrameError();

	void UnloadCompressor() ;
	// Added for Predictor
	virtual void ReConfigLink();

	virtual void SendResetRequestL();
	virtual void RemoteCompressorHasReset();
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
	virtual TBool FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
	virtual void FsmTerminationPhaseComplete();
	virtual TBool FsmOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);
	virtual TBool FsmConfigRequestOptionsValid(RPppOptionList& aList);

private:
	static TInt SendCallBack(TAny* aCProtocol);
	void	DoSend();
	void	SendResetAckL(TUint8 aId);
public:
	CPppDeCompressor* LoadDeCompressorL( CPppCompConfig& aPPPCompConfig, TInt aMaxFrameLength);
	CPppCompressor*   LoadCompressorL(	 CPppCompConfig& aPPPCompConfig, TInt aMaxFrameLength);
	void AddToRequestListL(RPppOptionList& aRequestList, TSglQueIter<CPppCompConfig>& aIterator);
private:
	TBool	iSoftwareCompressionOn;
	//
	//TUint				iCurrentCompressor;
	//TUint				iCurrentDeCompressor;
	RMBufPktQ			iSendQ;
	CAsyncCallBack*		iSendCallBack;
	//
	TBool	iLowerFlowOn;
	TBool	iUpperFlowOn;
	//
	TPppExtraRecvr<CPppCcp> iRecvr;
	TUint8	iResetId;	
	
	TSglQue<CPppCompConfig> iCompressorConfig;
	CObjectCon*			iCompressorCon;
	CObjectCon*			iDeCompressorCon;
	CPppCompressor*		iCompressor;
	CPppDeCompressor*	iDeCompressor;
	TBool				iDead;
	CPppIniData*		iIniFilePtr;
	RPppOptionList iRequestWorkList;	
	CObjectConIx*		iContainerForDlls;

};


class CPppCompFactory : public CNifFactory
	{
// Factory base for creating a concrete instance of a CSSLBase
public:
	virtual void InstallL()=0;
	virtual CPppCompressor*   NewPppCompressorL(  CPppCcp* aCcp, TInt aMaxFrameLength, const TUint8* aMode = NULL)=0;
	virtual CPppDeCompressor* NewPppDeCompressorL(CPppCcp* aCcp, TInt aMaxFrameLength, const TUint8* aMode = NULL)=0;
	};

#endif // __PPPCCP_H_

