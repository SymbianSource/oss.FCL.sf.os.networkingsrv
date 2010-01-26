/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file PPPLCP.h
 @internalComponent 
*/

#ifndef PPPLCP_H_INCLUDED_
#define PPPLCP_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/nifif.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <networking/pppdef.h>
#include <networking/pppconfig.h>		// for CCPppLcpConfig::TPppLinkMode
#include <comms-infras/agentmessages.h>	// for CAgentProvisionInfo
#include <comms-infras/commsdebugutility.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifvar_internal.h>
#endif


////////////////////////////////////////////////////////////////////////////////
// PPP LCP - The heart of PPP
////////////////////////////////////////////////////////////////////////////////

//const TUint KPppIdEsocl = 0x10000; // outside PPP range and Esock forbids zero.

class CPppAcp;
class CPppAuthentication;
class CPppMsCbcpClient;	// CSW

// Callback IETF request types (see draft RFC for PPP-callback)
typedef enum	// CSW
	{
	ECallbackIETFRequestType0=0,	// defined by IETF draft RFC
	ECallbackIETFRequestType1=1,
	ECallbackIETFRequestType2=2,
	ECallbackIETFRequestType3=3,
	ECallbackIETFRequestType4=4,
	ECallbackIETFRequestType5=5,
	// and also
	ECallbackIETFRequestTypeMSCBCP=6	// from MS rejected draft
	} TCallbackIETFRequestType;

// String literals for protocol name used during flow binding
_LIT8(KProtocol4, "ip");
_LIT8(KDescIcmp, "icmp");
_LIT8(KProtocol6, "ip6");

class CPppCcp;
class CPppLrd;
class CPppFlowBase;
class MPppRecvr;
class CPppCompressor;
class CPppDeCompressor;
class RPppOptionList;
#if defined (__FLOG_ACTIVE)
class CPppLog;
#endif

class CPppBinderIp4;
class CPppBinderIp6;
class CPppLinkBase;
class TSigLinkSubType;
class CIPConfig;
class CPppLcpConfig;
class CHDLCConfig;
class CCredentialsConfig;
class CPPPAuthConfig;
class CPppConfig;
class CAgentProvisionInfo;
class CPppProvisionInfo;

class CPppLcp : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl,
                public ESock::MUpperDataReceiver, public ESock::MUpperControl,
                public MPppFsm, public MPppOptionsExtender
/**
PPP Link Control Protocol class.
This class oversees all the other PPP protocol classes, in addition to
implementing the LCP state machine. Objects are instantiated at initialization
or as necessary to implement the protocols:
    CCP, authentication, CBCP, IPCP, IPV6CP
@internalComponent
*/
	{
    friend class CPppLcpFactory;
public:
	enum TPppProtocol { EPppProtocolBegin = 1, EPppProtoNone = EPppProtocolBegin, EPppIp4 , EPppIp6, EPppAuth, EPppProtocolEnd };

	IMPORT_C static CPppLcp* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

    //-=========================================================
    // Methods for handling SCPR messages
    //-=========================================================
	IMPORT_C virtual void SubConnectionGoingDown();
	IMPORT_C virtual void SubConnectionError(TInt aError);
	IMPORT_C virtual void StartFlowL();
	IMPORT_C virtual void StopFlow(TInt aError);

    IMPORT_C virtual void ProvisionConfig(const ESock::RMetaExtensionContainerC& aConfigData);
    IMPORT_C virtual void Destroy();
    IMPORT_C virtual void AuthenticateResponseMessage();
    IMPORT_C virtual void AgentToFlowNotification(TInt aNotificationId);

    //-=========================================================
    // MFlowBinderControl methods
    //-=========================================================
	IMPORT_C virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	IMPORT_C virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	IMPORT_C virtual void Unbind(ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	IMPORT_C virtual ESock::CSubConnectionFlowBase* Flow(); //returns its own flow

protected:
	IMPORT_C CPppLcp(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	IMPORT_C virtual ~CPppLcp();

    //-=========================================================
    // FSM upcalls
    //-=========================================================
	IMPORT_C virtual TInt FsmLayerStarted();
	IMPORT_C virtual void FsmLayerFinished(TInt aReason=KErrNone);
	IMPORT_C virtual void FsmLayerUp();
	IMPORT_C virtual void FsmLayerDown(TInt aReason=KErrNone);
	IMPORT_C virtual void FsmFillinConfigRequestL(RPppOptionList& aRequestList);
	IMPORT_C virtual void FsmCheckConfigRequest(RPppOptionList& aRequestList, RPppOptionList& aAckList, RPppOptionList& aNakList, RPppOptionList& aRejList);
	IMPORT_C virtual void FsmApplyConfigRequest(RPppOptionList& aRequestList);
	IMPORT_C virtual void FsmRecvConfigAck(RPppOptionList& aReplyList);
	IMPORT_C virtual void FsmRecvConfigNak(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	IMPORT_C virtual void FsmRecvConfigReject(RPppOptionList& aReplyList, RPppOptionList& aReqList);
	IMPORT_C virtual void FsmTerminationPhaseComplete();
	IMPORT_C virtual TBool FsmRecvUnknownCode(TUint8 aCode, TUint8 aId, TInt aLength, RMBufChain& aPacket);
	IMPORT_C virtual TBool FsmConfigRequestOptionsValid(RPppOptionList& aList);
	IMPORT_C virtual TBool FsmAckOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);
	IMPORT_C virtual TBool FsmRejectOptionsValid(RPppOptionList& aList, RPppOptionList& aRequestList);

    //-=========================================================
	// Messages::ANode methods
    //-=========================================================
	IMPORT_C void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

    //-=========================================================
    // CSubConnectionFlowBase methods
    //-=========================================================
	IMPORT_C virtual ESock::MFlowBinderControl* DoGetBinderControlL();

public:

    //-=========================================================
    // MUpperDataReceiver methods
    //-=========================================================
	IMPORT_C virtual void Process(RMBufChain& aData);

    //-=========================================================
    // MUpperControl methods
    //-=========================================================
    IMPORT_C virtual void StartSending();
    IMPORT_C virtual void Error(TInt aError);

    //-=========================================================
    // Custom methods
    //-=========================================================
    // provide compatability for classes using old CNifIfLink style API and PPP Internal Stop() calls
    void Stop(TInt aReason, MNifIfNotify::TAction aAction);
    // call from Binder - MLowerDataSender
	ESock::MLowerDataSender::TSendResult Send(RMBufChain& aPdu, TUint aProtocol);

	//
    // Calls from Binder
    //

	IMPORT_C virtual void BinderLinkUp(TPppProtocol aProtocol);		// overriden by CFProtocols derived from CPppLcp

    void Progress(TInt aStage, TInt aError);
	void FlowUp();
	void FlowDown(TInt aError, MNifIfNotify::TAction aAction);

	// Utility methods for retrieving different provisioning information structures
	inline const CIPConfig* GetNcpConfig() const;
	inline const CPppLcpConfig* GetLcpConfig() const;
	inline const CBCAProvision* GetBCAProvision() const;
    inline const CPppAuthConfig* GetAuthConfig() const;
	inline const CAgentProvisionInfo* GetAgentConfig() const;
	inline const CPppProvisionInfo* GetPppAgentConfig() const;

	inline const TDesC8& GetExcessData() const;

	// Retrieve pointer to locally held credentials object
	inline const CCredentialsConfig* GetCredentials() const;

    //-=========================================================
    // For CPPPAuthenticate methods - might be worth moving them elsewhere.
    //-=========================================================
	void Authenticate(CPppAuthentication* aAp);
	void CancelAuthenticate(CPppAuthentication* aAp);

private:
	CPppAuthentication* iAuthenticate;

protected:
	void InitL();

    //-=========================================================
    // Custom methods
    //-=========================================================
	TInt DoStartFlow();
	void DoStopFlow(TInt aReason, MNifIfNotify::TAction aAction);
    void ProvisionConfigL(const ESock::RMetaExtensionContainerC& aConfigData);

private:
	CPppBinderIp4 *iBinder4;
	CPppBinderIp6* iBinder6;

    //-=========================================================
    // PPP Configs cached
    //-=========================================================

	//
	// Pointers to provisioning information objects retrieved from CAccessPointConfig
	//
  	const CBCAProvision* iBCAProvision;
  	const CIPConfig* iPppNcpConfig;
  	const CPppLcpConfig* iPppLcpConfig;
  	const CPppAuthConfig* iPppAuthConfig;
	const CAgentProvisionInfo* iAgentProvision;
	const CPppProvisionInfo* iPppAgentProvision;
	const CCredentialsConfig* iCredentials;

    TBool iInitialised;
    TInt iSavedError;				// saved error (if any) from processing ProvisionConfig message

public:
    //-=========================================================
    // PPP Legacy methods
    //-=========================================================
	inline CPppLinkBase*  PppLink() { return iPppLink; }
	void RegisterRecvr(MPppRecvr* aRecvr);
	void ReregisterRecvr(MPppRecvr* aRecvr);
	void DeregisterRecvr(MPppRecvr* aRecvr);
	void LinkFlowOn();
	void LinkLayerUp();
	void LinkLayerDown(TInt aStatus);
	void PhaseComplete();
	void PhaseAborted(TInt aStatus);
	void PhaseAdvance(TPppPhase aNewPhase);
	void PhaseRetard(TPppPhase aNewPhase, TInt aReason);
	TUint8 SendEchoRequest();
	void SendIdentification();
	inline CPppLcpConfig::TPppLinkMode PppLinkMode() const;
	void NcpUp();
	//
	TInt PppOpen();
	void PppClose(TInt aReason);

	inline TBool DoLcpExts() const { return iDoLcpExts; }
	inline void CallbackRequestGranted();	// CSW
	void TerminateLink(MNifIfNotify::TAction aAction=MNifIfNotify::EDisconnect, TInt aError=KErrNone);
	void PppNewCompressor(const CPppCompressor* aCompressor);
	void PppUnloadCompressor();
	void PppNewDeCompressor(const CPppDeCompressor* aDeCompressor);
	void StopProtocol(TUint aProtocol);
	TBool DoesDllExist(const TDesC& aFilename);
	inline CObjectConIx*	ContainerForDlls() const { return iContainerForDlls; }

	TInt Notification(TAgentToNifEventType aEvent, void * aInfo);

	// Added for callback July 99
	inline TCallbackIETFRequestType CallbackRequestType(){return iCallbackIETFRequestType;}
	inline TBool CallbackEnabled(){return iCallbackEnabled;}

	inline void SetMaxTransferSize(TInt aLength);
	inline TInt MaxTransferSize() const;
	inline void SetMaxReceiveSize(TInt aSize);
	inline TInt MaxReceiveSize() const;

	void CancelRead();

	void CallbackGrantedAndAuthenticated();

	// Returns true if an external IP configuration agent (MIP, DHCP) is active
	TBool QueryExternalIPConfiguration();

   	void LinkRecv(RMBufChain& aPacket);

private:
	static void NewMagicNumberL(TUint& aMagicNumber);
	INLINE void AppendCallbackRequestL(RPppOptionList&  aRequestList );
	INLINE void InitCallbackInfoL();
	INLINE void PhaseAdvancesBeyondAuthenticate();
	INLINE void GetCallbackInfoL();
	INLINE void NewCallbackRequestInfoL( TCallbackIETFRequestType aRequestType );	// CSW

	// Utility functions for posting messages to SCPR

	void PostProgressMessage(TInt aStage, TInt aError);
	void PostDataClientStartedMessage();
	void PostFlowDownMessage(TInt aError);
	void PostErrorMessage(TInt aError);
	void MaybePostDataClientIdle();
	TBool AllNcpsUp();
	// Utility functions for processing incoming SCPR messages
	TInt NotificationMessage(TAgentToNifEventType aNotificationId);

	// Utilities for retrieval of SCPr Provisioning Information
	TInt RetrieveDynamicProvisionInfo();

    // Flogging mechanism
    __FLOG_DECLARATION_MEMBER;
private:

    /** IP Protocol */

    /** The Compression Control Protocol object */
	CPppCcp* iPppCcp;

	/** Number of times PppOpen has been called without a corresponding PppClose */
	TInt iOpenCount;

	/** Current PPP phase */
	TPppPhase iPhase;

	/** Authentication protocol framework object */
	CPppAcp* iPppAcp;

	/** Link quality checking object */
	CPppLrd* iPppLrd;

	/** Lower link-layer object */
	CPppLinkBase* iPppLink;

	/** List of objects that can receive incoming packets */
	TDblQue<MPppRecvr> iRecvrList;

	TUint iLocMagicNumber;
	TUint iRemMagicNumber;	// Or last NAK sent to peer
	TInt iConsecMagic;
	TInt iMaxSendSize;
	TInt iMaxRecvSize;
	HBufC8* iRecvIdentification;
	TTimeIntervalSeconds iRecvTimeRemaining;
	TTime iRecvTimeNotification;
	HBufC8* iRecvTimeRemMessage;
	/** Authentication protocol object */


	/** Whether CBCP configuration has been read */
	TBool	iGotCallbackInfo;

	/** Whether CBCP is enabled */
	TBool	iCallbackEnabled;

	TCallbackAction iCallbackAction;
	TCallbackIETFRequestType iCallbackIETFRequestType;
	TUint8* iCallbackInfo;
	TInt	iCallbackInfoLen;
	TUint8* iCallbackIETFRequestPacket;
	TInt	iCallbackIETFRequestPacketLen;
	TBool	iCallbackRequestGranted;

	/* Callback Control Protocol object */
	CPppMsCbcpClient* iPppMsCbcp;

	/** The action to take once the link goes down */
	MNifIfNotify::TAction iTerminateAction;

	/** The reason the link is going down */
	TInt	iError;

	/** Whether certain extended PPP options from RFC 1570 are to be handled */
	TBool iDoLcpExts;

	TBool iRecursive;
	CObjectConIx*	iContainerForDlls;

	/** Whether authentication has been completed */
	TBool iAuthenticateDone;

	/** The last request received */
	Messages::TNodeSignal::TMessageId iLastRequest;
	TBool iLastRequestErrored;

	/** Whether we are configured to do Mobile IP */
	TBool iDoExternalIPConfiguration;

  	/** Whether we are configured for client or server mode */
  	CPppLcpConfig::TPppLinkMode iLinkMode;

	/*The number of Ncps that have reported being up*/
	TInt iNcpUpCount;

  	enum TMeshMachineFlowState
      	{
      	EStopped,
      	EStarting,
      	EStarted,
      	EStopping,
      	};
	TMeshMachineFlowState iMMState;
	Elements::TStateChange iLastStateChange; //To avoid double reporting
	TBool iSentIdle;

#ifdef __FLOG_ACTIVE
public:
	CPppLog* iLogger;
	TInt iLogLevel;
	
private:
	const TText8* GetMMStateName() const;
#endif
	};

#include <networking/ppplcp.inl>
#endif
