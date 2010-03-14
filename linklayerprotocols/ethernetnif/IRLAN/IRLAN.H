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
// Interface to IR-LAN
// 
//

/**
 @file
 @internalComponent 
*/

#if !defined(__IRLAN_H__)
#define __IRLAN_H__

#define _MKLOG
//#define __TRACEWIN__

#include <ir_sock.h>
#include "IRLANDAT.H"

#include <comms-infras/nifif.h>
#include <es_prot.h>

const TUint KIrlanMajorVersionNumber=1; //< Constant specifies Major version number.
const TUint KIrlanMinorVersionNumber=0; //< Constant specifies Minor version number.
const TUint KIrlanBuildVersionNumber=7; //< Constant specifies Build version number.

class CInternalSocket;
class CInternalNetDB;
class CInternalHostResolver;
class CIrlanControlEngine;

/**
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanPktDrvFactory) : public CPktDrvFactory
{
public:
	CIrlanPktDrvFactory();
	virtual CPktDrvBase* NewDriverL(CLANLinkCommon* aParent);
  	virtual TVersion Version() const;
};
  
inline CIrlanPktDrvFactory::CIrlanPktDrvFactory()
{
}

/**
Specific packet drivers are implemented as separate DLLs.
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanPktDrv) : public CPktDrvBase
{
public: 
	CIrlanPktDrv(CPktDrvFactory& aFactory);
	virtual ~CIrlanPktDrv();
	virtual void ConstructL(CLANLinkCommon* aParent);
	virtual TInt TestSend(TDesC8& /*aBuffer*/){return KErrNone;};
	virtual TInt StartInterface();
	virtual TInt StopInterface();
	virtual TInt ResetInterface();
	virtual TInt SetRxMode(TRxMode AMode); 
	virtual TInt GetRxMode() const;
	virtual TInt AccessType();
	virtual TInt ReleaseType();
	virtual TInt SetInterfaceAddress(const THWAddr&);
	virtual TUint8* GetInterfaceAddress()const;
	virtual TInt GetMulticastList(const THWAddr* aAddr, TInt& n) const;
	virtual TInt SetMulticastList(const THWAddr* aAddr, TInt n);
	virtual TInt InterfacePowerUp();
	virtual TInt InterfacePowerDown();
	virtual TInt InterfaceSleep();
	virtual TInt InterfaceResume();
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const; //< need to instantiate pure virtuals from CNifIfBase ETHER_802 
    virtual void FoundMACAddr();
	virtual TInt Send(RMBufChain& aPdu);
	virtual TInt Notification(enum TAgentToNifEventType aEvent, void* aInfo);
	virtual TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0);

	// Other functions 
	void LinkLayerUp();
	void Process(RMBufChain& aPdu);
private:
	CIrlanPktDrv();
	
private:
	CEther802LLCBase* iMACLayer;
	CIrlanControlEngine *iControl;
};

class CIrlanSender;
class CIrlanReceiver;
class CIrlanControlEngineTimers;

/**
Main Irlan control object.  State machine implemented with as an active object.
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanControlEngine) : public CActive , public MResolverNotify , public MSocketNotify
{
public:
	friend class CIrlanSender;
	friend class CIrlanReceiver;
private:
enum TIrlanState
	{
	E_Idle,E_PassiveIdle,E_Query,E_Conn,E_Info,E_Media,E_FilterConfig,E_FilterOperation,
	E_FilterBroadcast,E_Open,E_Wait,E_Arb,E_Data,E_Close,E_Sync,
	};
enum TIrlanMode
	{
	EAccessPoint,EPeerToPeer,EHosted,
	};
public:
	static CIrlanControlEngine *NewL(CIrlanPktDrv* aPktDrv);
	~CIrlanControlEngine();
	void RunL();
	void StartL();
	void DoCancel();
	// Notifier functions for internal sockets to call.
	void NewData(TUint aCount);
	void CanSend();
	TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	void ConnectComplete();
	void ConnectComplete(const TDesC8& aConnectData);
    void ConnectComplete(CServProviderBase& aSSP);
	void ConnectComplete(CServProviderBase& aSSP,const TDesC8& aConnectData);
	void CanClose(TDelete aDelete=EDelete);
	void CanClose(const TDesC8& aDisconnectData,TDelete aDelete=EDelete);
	void Error(TInt anError,TUint anOperationMask=EErrorAllOperations);
	void Disconnect(void);
	void Disconnect(TDesC8& aDisconnectData);
	void IoctlComplete(TDesC8 *aBuf);
	void NoBearer(const TDesC8& aConnectionParams);
	void Bearer(const TDesC8& aConnectionInfo);
	// Notifier functions for internal host resolver/netDB to call. 
	void QueryComplete(TInt aError);
	// Other public functions 
	TUint8* GetInterfaceAddress();
	//	static TInt SendCallBack(TAny* aSource);
	static TInt IrlanControlEngineTimerExpired(TAny *aIrlan);
private:
	CIrlanControlEngine();
	void ConstructL(CIrlanPktDrv* aNotify);
	void ActivateStateMachine(TInt aStat=KErrNone);
	// IrLAN state machine asynchronous actions: 
	TInt AttemptingDiscoveryL();
	TInt PassiveIdleTransitionL();
	TInt QueryRemoteIASL();
	TInt ConnectToProviderL();
	void ParseControlCommand();

	TInt GetInfoCmd();
	TInt GetInfoReply();
	void ParseInfoReply();
	TInt GetMediaCmd();
	TInt GetMediaReply();
	TInt ParseMediaReply(); 
	TInt OpenDataCmd();
	void ParseOpenDataReply();
	TInt GetDirectedFilterConfigCmd();
	void ParseDirectedFilterConfigReply();
	TInt SetDirectedFilterOperationCmd();
	void ParseDirectedFilterOperationReply();
	TInt SetBroadcastFilterOperationCmd();
	void ParseBroadcastFilterOperationReply();
	TInt ConnectToDataChannelL();
	void SendIrlanControlFrame(RMBufChain& aPdu);
	void SendIrlanResponseFrame(RMBufChain& aPdu);
	void QueueWaitForControlCommand();
	void QueueWaitForControlResponse();
	void ProcessReceivedPacketL();

	// Private utility functions 
	CIrlanParameter* LookUpParameter(const TDesC8& aName);
	void HandleErrorL();
	TInt DoIASQuery(const TDesC8& aClassName,
	  const TDesC8& aAttributeName,TUint aRemDevAddr);
	TInt DoControlConnect();
	TInt DoDataConnect();

	// printing routines 
	void PrintState();
	void PrintIASResults();
	void PrintIASError(TInt aError);
	void PrintResultCode(TIrlanQueryResult aResult);
private:
	CIrlanPktDrv* iNotify;
	TUint8 iHWAddr[9];

	HBufC8* iRecvBuffer;
	TUint iRecvBufLength;
	TPtr8 iRecvBufPtr;

	CIrlanControlEngineTimers *iTimers;
	TInt iDiscoveryAttempts;
	TIrlanMode iAccessType;
	TBool iAwaitingResponse;
	TInt iAwaitingCommand;
	TIrlanState iState;
	CIrlanPktDrv *iPktDrv;
	TUint iDiscoveredDevice;
	CIrlanSender* iSender;
	CIrlanReceiver* iReceiver;
	TProtocolDesc iProtInfo;
	CProtocolBase *iProtocol;
	CInternalHostResolver *iHostResolver;
	/** discovery log */
	TNameEntry iLog;		

	CInternalNetDB *iNetDatabase;
	TIASQuery iQuery;
	TIASResponse iResults;
	// listening 
	CInternalSocket *iListenSock;
	CInternalSocket *iAcceptControlSock;

	// sending 
	CInternalSocket *iControlSock;
	CInternalSocket *iDataSock;
	TRequestStatus iSendStat;
	TRequestStatus iRecvStat;
	TUint iOpenRetries;
	TInt iIrlanControlPortNum;
	TInt iIrlanDataPortNum;
	TDblQue<CIrlanParameter> iIrlanParameterList;
	RMBufPktQ iDataSendQ;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanSender) : public CActive
{
public:
	static CIrlanSender* NewL(CIrlanControlEngine* aParent,CInternalSocket* aSock);
	~CIrlanSender();
	void QueueSend(RMBufChain& aPdu);
	void KickSender();
	virtual void RunL();
	virtual void DoCancel();
private:
	CIrlanSender();
	void InitL(CIrlanControlEngine* aParent,CInternalSocket* aSock);
	void SeeIfPacketToSend();
private:
	CIrlanControlEngine* iParent;
	CInternalSocket* iDataSock;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanReceiver) : public CActive
{
public:
	static CIrlanReceiver* NewL(CIrlanControlEngine* aParent,CInternalSocket* aSock);
	~CIrlanReceiver();
	void QueueRead();
	virtual void RunL();
	virtual void DoCancel();
private:
	CIrlanReceiver();
	void InitL(CIrlanControlEngine* aParent,CInternalSocket* aSock);
private:
	CIrlanControlEngine* iParent;
	CInternalSocket* iDataSock;
};

/**
@internalComponent
*/
NONSHARABLE_CLASS(CIrlanControlEngineTimers) : public CBase
{
public:
	friend class CIrlanControlEngine;
	virtual ~CIrlanControlEngineTimers();
	static CIrlanControlEngineTimers *NewL(CIrlanControlEngine *aIrlan);
	void StartIrlanControlEngineTimer(TCallBack aCallBack,TInt aTimeout);
	void StopIrlanControlEngineTimer();
	void DoIrlanControlEngineTimerExpired();
private:
	CIrlanControlEngineTimers(CIrlanControlEngine *aIrlan);
private:
	CIrlanControlEngine *iIrlanControlEngine;
	TDeltaTimerEntry iIrlanControlEngineTimer;
	TDeltaTimerEntry *iIrlanControlEngineTimerH;
};

#endif



