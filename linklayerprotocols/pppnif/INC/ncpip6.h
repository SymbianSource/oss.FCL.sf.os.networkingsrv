// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __NCPIP6_H__
#define __NCPIP6_H__

#include <eui_addr.h>	// TE64Addr
#include <comms-infras/ss_protflow.h>
#include <comms-infras/es_protbinder.h>
#include <networking/ppplcp.h>
#include <networking/pppbase.h>
#include <comms-infras/commsdebugutility.h>

const TUint KPpp6MajorVersionNumber=0;
const TUint KPpp6MinorVersionNumber=1;
const TUint KPpp6BuildVersionNumber=1;

const TUint KIp6cpSendPriority = 10;

const TUint KPppIdIp6cp = 0x8057;
const TUint KPppIdIp6 = 0x0057;

const TUint8 KPppIp6cpOptInterfaceIdentifier = 1;
const TUint8 KPppIp6cpOptCompressionProtocol = 2;

NONSHARABLE_CLASS(CPppBinderIp6) : public CBase, public MPppFsm,
								   public ESock::MLowerDataSender, public ESock::MLowerControl
/**
Implements IPCP and support for IP datagrams (RFC 1332)

@internalComponent
*/
	{
public:
   	static CPppBinderIp6* NewL(CPppLcp* aLcp);
	~CPppBinderIp6();

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
    ESock::MLowerDataSender* BindL(ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aControl);
    void UnBind(ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aControl);

	TBool MatchesUpperControl(const ESock::MUpperControl* aUpperControl) const;

#if EPOC_SDK >= 0x06010000
	TInt Notification(TAgentToNifEventType aEvent);
#endif

	void SendFlowOn();
    void Error(TInt aError);
	
	void RecvIp(RMBufChain& aPacket);
	void Ip6FrameError();
	void Ip6KillProtocol();

protected:
	virtual TInt FsmLayerStarted();
	virtual void FsmLayerFinished(TInt aReason = KErrNone);
	virtual void FsmLayerUp();
	virtual void FsmLayerDown(TInt aReason = KErrNone);
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
	static TInt SendCallBack(TAny* aCProtocol);
	void DoSend();

private:
   	CPppBinderIp6(CPppLcp* aLcp);
	void  ConstructL();
	
	inline CPppLcp* Flow();

private:
    //-=========================================================
    // Layer infrastructure
    //-=========================================================
    CPppLcp* iPppNifSubConnectionFlow;

	ESock::MUpperDataReceiver* iUpperReceiver;
    ESock::MUpperControl* iUpperControl;

//	CProtocolBase* iNetwork;

	RMBufPktQ iSendQ;
	CAsyncCallBack* iSendCallBack;

	ESock::MLowerDataSender::TSendResult iLowerFlowOn;
	ESock::MLowerDataSender::TSendResult iUpperFlowOn;

	TPppExtraRecvr<CPppBinderIp6> iIpRecvr;

	TE64Addr iLocalIfId;
	TE64Addr iRemoteIfId;
	TIp6Addr iPrimaryDns;
	TIp6Addr iSecondaryDns;

	TInterfaceName iIfName;

    __FLOG_DECLARATION_MEMBER;

};

//
// Inline functions
//

CPppLcp* CPppBinderIp6::Flow()
	{
	return iPppNifSubConnectionFlow;
	}


#endif
