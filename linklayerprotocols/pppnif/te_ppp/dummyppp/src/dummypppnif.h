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
 

#include <udp_hdr.h>
#include "PPPLCP.H"
#include "PPPBASE.H"
#include "VJ.H"
#include "PPPCCP.H"
#include "ncpip.h"
#include "ncpip6.h"


const TInt KDummyPPPSendChannel = 0;
const TInt KDummyPPPRecvChannel = 1;

//
// For queing to send to PPP 
//
const TInt KDummyPPPSendPriority = 10;

//
// For responding to PPP timers
//
const TInt KDummyPPPMaxRestartConfig = 10;
const TInt KDummyPPPWaitTimeConfig = 3000;
const TInt KDummyPPPFsmRequestMaxTimeout = 60000;

//
// For negotiating PAP for PPP
//
const TUint8 KPppPapRequest = 1;
const TUint8 KPppPapAck = 2;
const TUint8 KPppPapNak = 3;
//
// For responding to PAP in PPP timers
//
const TInt KDummyPPPPapWaitTime = 3000;
const TInt KDummyPPPPapRetries = 4;

class CDummyPPPLink;
class CDummyNifLog;

//
// DUMMY_PPP Finite State Machine for responding to PPP
//

enum TDummyPPPToPPPFsmState
	{
	EDummyPPPFsmInitial, EDummyPPPFsmStarting,
	EDummyPPPFsmClosed, EDummyPPPFsmStopped,
	EDummyPPPFsmStopping, EDummyPPPFsmClosing,
	EDummyPPPFsmReqSent, EDummyPPPFsmAckRecvd,
	EDummyPPPFsmAckSent, EDummyPPPFsmOpened,
	EDummyPPPFsmPapAuthReqSent,EDummyPPPFsmPapAuthAckRecvd,
	EDummyPPPFsmPapAckSent,EDummyPPPIPCPReqSent,
	EDummyPPPFsmIPOpen
	};

enum TDummyPPPState
	{ EDummyPPPLinkClosed, EDummyPPPLinkConnecting, EDummyPPPLinkOpen, EDummyPPPLinkDisconnecting, EDummyPPPLinkReconnecting };

class CDummyPPPIf : public CNifIfBase //, public CProtocolBase
	{
public:
	CDummyPPPIf(CDummyPPPLink* aLink);
	
	void CreateL();
	void CloseLog();
	void End();

    virtual void BindL(TAny *aId);
	
	virtual TInt Send(RMBufChain& aPacket, TAny* aPppId);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Control(TUint, TUint, TDes8&, TAny*);
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	 

	//Added to set value of iSendFlowOn
	void SetFlowOnFlag(TBool aFlag);

	//Added to send to PPP interface directly
	TInt SwitchIPHeaders(RMBufChain& aPacket);
	void SendtoPPP(RMBufChain& aPacket);
	TInt ProcessPPPPacket(RMBufChain& aPacket);
	TInt DecodePacket(RMBufChain& aPacket);
	
	void ProcessConfig();
	void ProcessTerminate();
	void SetupConfigRequest(RPppOptionList& aRequestList);
	void SetupSendPPP();
	void InitialiseConfigRequestL();
	void SendConfigReply(RPppOptionList& aOptList,TUint8 aType);
	void SendConfigRequest();
	void SendTerminateAck();
	void SendDummyPPPIdentification();

	void InitializeIPCPConfigRequestL();
	void ProcessPAPandIPCP();
	void SendPapRequest();
	void SendPapAck(RPppOptionList& aReplyList);
	void SendPapNak(RPppOptionList& aReplyList);
	TInt CheckRecvPapReq();
	void SendIPCPDummyNak();
	void SendIPCPReq();
	void CheckRecvIPCPReq(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList);

	void SetState(TDummyPPPToPPPFsmState aState);
	void SetNewId();
	void SetPppId(TUint aProt);
	void InitRestartCountForConfig();
	void CheckRecvConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList);
	void CheckRecvConfigAck(RPppOptionList& aReplyList);
	void CheckRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	void CheckRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	void OpenFSM();
	void CloseFSM();
	void ChangeStateforTimerComplete();

	inline TBool CheckMaxFailureExceeded(){return ((iDummyPPPMaxFailureCount == 0)?TRUE:FALSE); }
	inline void  DecrementDummyPPPMaxFailureCount(){iDummyPPPMaxFailureCount--;}

	TInt PresetAddr(TUint32& aAddr, const TDesC& aVarName);
 
	CProtocolBase* iProtocol;
	MPppOptionsExtender* iPppOptions;
	CDummyPPPLink* iLink;

protected:

	//TUint32 iLocalAddressBase;
	TUint32 iLocalAddress;
	// Negotiated information
	TUint32 iPrimaryDns;
	TUint32 iSecondaryDns;

private:

	void DoSendtoPPP();
	static TInt SendCallBack(TAny* aCProtocol);
	CAsyncCallBack*	iSendCallBack;

	TInt iSendLoWat;
	TInt iSendHiWat;
	TInt iSendNumBufs;
	TBool iSendFlowOn;
	TUint iFlags;
	
	//RMBufChain iSendPkt;

	RMBufPktQ iRecvQ;
	RMBufQ iRecvPkt;
	RMBuf* iRecvMBuf;
	
	TUint iDummyPPPId; //to pass to Lcp for Sending
	TCommConfig iOrigConfig;

	//static TInt iNumOfIfs;
	//TInt iIfNum;
	TInterfaceName iIfName;
	CDummyNifLog* logUser;

	//DUMMY_PPP to PPP Fsm variables
	TDummyPPPToPPPFsmState iDummyPPPState;
	TUint8 iDummyPPPRequestId;
	TUint8 iCodeRec;
	TUint8 iIdRec;
	TInt iLenRec;
	TUint iDummyPPPLocMagicNumber;
	TUint iPPPRemMagicNumber;
	RPppOptionList iDummyPPPRequestList;
	TUint8 iCurrentDummyPPPId; //For both LCP/PAP Negotiation
	TInt iDummyPPPMaxFailureCount;
	TUint iSendPppId;	

	// RFC 1661 4.6
	TInt iDummyPPPRestartCount;
	TInt iDummyPPPWaitTime;

    //For PAP Negotiation
	TInt	iDummyPPPTryCount;

	//For IPCP Negotiation
	TUint32 iRemoteAddr;
	TUint32 iLocalAddr;

	RMBufPacket ipktRecvPPP;

	RMBufPacket *ptrTransPktPPP;

	};



class CDummyPPPLink : public CNifIfLink, public MTimer //from public CPppUmtsLink
	{

public:
	CDummyPPPLink(CNifIfFactory& aFactory);
	~CDummyPPPLink();

	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual void OpenL(); // Open/Close from LCP
	void LinkDown(TInt aStatus);
	virtual TInt Start(); 
	virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction);	 
	virtual void IfProgress(TInt aResult);
    virtual CNifIfBase* GetBinderL(const TDesC& aName); 
	virtual void Restart(CNifIfBase* aIf);
	virtual void TimerComplete(TInt aStatus);
	static void FillInInfo(TNifIfInfo& aInfo, TAny* aPtr);
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Info(TNifIfInfo& aInfo) const;

public:
	CDummyPPPIf* iNifIf;
	MNifIfNotify* iMNifIfNotify;
	
	RMBufPktQ	iSendQ;
	TDummyPPPState iState;
	//TUint iFlags;
	};
 
class CDummyPPPIfFactory : public CNifIfFactory
	{
protected:
	virtual void InstallL();
	virtual CNifIfBase* NewInterfaceL(const TDesC& aName);
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const;
	};

 
const TInt KHexDumpWidth = 16;

class CDummyNifLog : public CBase
	{
public:
	static void Write(const TDesC& aDes);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	static void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth = KHexDumpWidth);
	void Dump(RMBufChain& aPacket, TInt aChannel);
	void DumpBytes(const TText* aMargin, const TUint8* aPtr, TInt aLen);

	static const TText* StateToText(TDummyPPPToPPPFsmState aState);

	void DumpChapType(const TUint8* aPtr);

	void LogUserData(RMBufChain& aPacket, TInt aChannel);
	void Initialize();

	const TText* LcpCodeToText(TUint aValue);
	const TText* LcpOptToText(TUint aValue);
	const TText* FsmCodeToText(TUint aValue);
	const TText* ProtocolToText(TUint aValue);
	const TText* CallbackOpToText(TUint aValue);
	const TText* PapCodeToText(TUint aValue);
	const TText* IpProtocolToText(TUint aValue);
	const TText* CbcpCodeToText(TUint aValue);
	const TText* ChapCodeToText(TUint aValue);
	const TText* IpcpCodeToText(TUint aValue);
	const TText* Ip6cpCodeToText(TUint aValue);
	const TText* CcpCodeToText(TUint aValue);
	TInt DumpLcp(TPtrC8& aDes);
	TInt DumpPap(TPtrC8& aDes);
	TInt DumpChap(TPtrC8& aDes);
	TInt DumpCbcp(TPtrC8& aDes);
	TInt DumpLcpOption(TPtrC8& aDes);
	TInt DumpIpcp(TPtrC8& aDes);
	TInt DumpIpcpOption(TPtrC8& aDes);
	TInt DumpIp6(TPtrC8& aDes);
	TInt DumpIp(TPtrC8& aDes);
	TInt DumpIp6cp(TPtrC8& aDes);
	TInt DumpVjCompTcp(TPtrC8& aDes);
	TInt DumpCcp(TPtrC8& aDes);
	TInt DumpCcpOption(TPtrC8& aDes);
	TInt DumpVjUncompTcp(TPtrC8& aDes);
	TInt DumpTcp(TPtrC8& aDes, TUint32 aSrcA, TUint32 aDstA, TInt aLength);
	TInt DumpIcmp(TPtrC8& aDes, TInt aLength);
	TInt DumpUdp(TPtrC8& aDes, TUint32 /* aSrcA */, TUint32 /*aDstA */, TInt aLength);
	
	
	TUint32 iSentData;
	TUint32 iRecvdData;
	};



// various things that will get set up on each interface by dummy nifs
// obviously this is a common network mask....
_LIT(KNetworkMask, "255.255.255.0");
// will be added to the address base to make the broadcast address...
const TUint KBroadcastAddressSuffix = 255;
// some arbitrary num to add to the base to give the default gateway machine...
const TUint KDefaultGatewayAddressSuffix = 10;
// some arbitrary num to add to the base to give the secondary dns server...
const TUint KSecondaryDnsAddressSuffix = 11;
// obviously all the above addresses are totally arbitrary to a certain extent... :-)

 
