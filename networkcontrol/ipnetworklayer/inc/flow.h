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
 @file deftflow.h
*/

#ifndef IPSHIMFLOW_H_INCLUDED_
#define IPSHIMFLOW_H_INCLUDED_

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <comms-infras/ss_protflow.h>
#include <cflog.h>

#if defined __CFLOG_ACTIVE || defined SYMBIAN_TRACE_ENABLE
// CommsDebugUtility logging tags. Use them to enable tracing for ReferenceSCPR
_LIT8(KIPProtoTag1,"IPProto");
_LIT8(KIPProtoTag2,"IPProtoFlow");
#endif

class CIPShimIfBase;
class MNifIfUser;
class CIPShimIncoming;

class CIPShimSubConnectionFlow;
class CAsyncBinderClose : public CAsyncOneShot
/**
Class used to asyncronously close binders.

Closing a binder can result in it being destroyed, so this asynchronous callback is
used to avoid destroying a binder in the same stack frame as a binder Error() upcall.
*/
	{
public:
	explicit CAsyncBinderClose(CIPShimSubConnectionFlow& aFlow);
	void RunL();

private:
	CIPShimSubConnectionFlow& iFlow;
	};

class CIPProtoBinder;

//-=====================================================================
// IPShim PINT and PINT factory
//-=====================================================================
class CIPShimProtocolIntf : public ESock::CProtocolIntfBase
/**
@internalTechnology
*/
	{
    friend class CIPShimSubConnectionFlow;
	friend class CIPShimIfBase;

public:
	CIPShimProtocolIntf(ESock::CProtocolIntfFactoryBase& aFactory,const Messages::TNodeId& aCprId);

	// from CProtocolIntfBase
	virtual void DoFlowCreated(ESock::CSubConnectionFlowBase& aFlow);
	virtual void DoFlowBeingDeleted(ESock::CSubConnectionFlowBase& aFlow);

	void OpenRoute();
	void CloseRoute();

	~CIPShimProtocolIntf();

private:
	CIPShimIfBase* FindOrCreateNifL(const TDesC8& aProtocol, const TConnectionInfo& aConnectionInfo);
	void NifDisappearing(CIPShimIfBase* aNif);

private:
	RPointerArray<CIPShimIfBase> iNifs;
	};

class CIPShimProtocolIntfFactory : public ESock::CProtocolIntfFactoryBase
/**
@internalTechnology
*/
	{
public:
	static CIPShimProtocolIntfFactory* NewL(TUid aFactoryId, ESock::CProtocolIntfFactoryContainer& aParentContainer);
	virtual ESock::CProtocolIntfBase* DoCreateProtocolIntfL(ESock::TFactoryQueryBase& aQuery);
	CIPShimProtocolIntfFactory(TUid aFactoryId, ESock::CProtocolIntfFactoryContainer& aParentContainer);
	};


class CIPShimSubConnectionFlow : public ESock::CSubConnectionFlowBase,
                                 public ESock::MFlowBinderControl,
                                 public ESock::MLowerControl
/**
IP Shim flow.

@internalComponent
*/
	{
	friend class CIPShimFlowFactory;
	friend class CIPProtoBinder;

//	struct TBinderPair;


public:
	// Unidirectional internal interface from Binder classes to Flow.
	virtual void BinderReady();
	virtual void SendMessageToSubConnProvider(const Messages::TSignatureBase& aCFMessage);
	virtual const Messages::TNodeId& GetCommsId();
	void OpenRoute();
	void CloseRoute();

	// MIpDataMonitoringNotifications
	virtual void PostConnDataReceivedThresholdReached(TUint aVolume);
	virtual void PostConnDataSentThresholdReached(TUint aVolume);
	virtual void PostSubConnDataReceivedThresholdReached(TUint aVolume);
	virtual void PostSubConnDataSentThresholdReached(TUint aVolume);


    void PostClientIdleIfIdle();
	void BinderError(TInt aError, CIPProtoBinder* aIf);
	void AsyncBinderClose();


    //MFlowBinderControl,
	virtual MLowerControl* GetControlL(const TDesC8& aProtocol);
	virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual void Unbind( ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual CSubConnectionFlowBase* Flow();

    //MLowerControl
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);

protected:
	// from Messages::ANode
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

	CIPShimSubConnectionFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	~CIPShimSubConnectionFlow();
	ESock::MFlowBinderControl* DoGetBinderControlL();

	// Functions for processing SCPR messages.
	virtual void StartFlowL();
	virtual void StopFlow(TInt aError);
	virtual void ProcessProvisionConfigL();
	virtual void Destroy();
	virtual void BindToL(const Messages::TNodeId& aCommsBinder);
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	virtual void UpdateIpAddressInfoL(ESock::TCFScpr::TSetParamsRequest& aParamReq);
#else
	virtual void UpdateIpAddressInfoL(ESock::TCFScpr::TParamsRequest& aParamReq);
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	// Utility functions for sending messages to SCPR
	void PostDataClientStartedIfReady();
	void PostDataClientStoppedIfReady();
	void Error(const TInt aError);


    TInt SetConnectionInfo(const TConnectionInfo& aInfo);
   	void InitialiseDataMonitoringL(CIPShimIfBase* intf);
private:
	static void CleanupBinderListAppend(TAny*);
	static void CleanupSignalNewInterface(TAny*);
	static void CleanupBindToLower(TAny*);
	inline void SetCleanupError(TInt aError);
	inline TInt CleanupError();

	void CreateBinderL(const TDesC8& aProtocol, MFlowBinderControl* aBinderControl);
	TInt SetProtocolList(const TDesC8& aProtocolList);

	void MarkAllBindersForClosure();
	inline void MarkBinderForClosure(CIPProtoBinder* aBinder);
	void CloseMarkedBinders();
	void SignalInterfaceDownOnAllBinders(TInt aError);

	inline CIPShimProtocolIntf* ProtocolIntf();

private:
	RPointerArray<CIPProtoBinder> iBinderList;
    TInt iCleanupError;
    // iAsyncBinderClose is pre-allocated as KErrNoMemory recovery is complex in the situations it is needed
	CAsyncBinderClose iAsyncBinderClose;
	TInt  iStopCode;

public:
	RBuf8 iConnectionInfo;
    TPtrC8 iProtocolList;		// pointer to protocol list string in provisioning information
    TInt iBindCallCount;

private:
    TBool iBinderReady:1;
    TBool iStarting:1;
    TBool iStarted:1;
	TBool iDestroyPending:1;
	};


// =================================================================================
//
// Other inline methods
//

void CIPShimSubConnectionFlow::SetCleanupError(TInt aError)
	{
	iCleanupError = aError;
	}

TInt CIPShimSubConnectionFlow::CleanupError()
	{
	return iCleanupError;
	}

CIPShimProtocolIntf* CIPShimSubConnectionFlow::ProtocolIntf()
	{
	ASSERT(iProtocolIntf);
	return static_cast<CIPShimProtocolIntf*>(iProtocolIntf);
	}

// ==========================================================================================
//
// Flow Factory
//

const TInt KIPShimFlowImplUid = 0x10281C36;

class CIPShimFlowFactory : public ESock::CSubConnectionFlowFactoryBase
/**
IP Shim flow factory.

@internalComponent
*/
	{
public:
	static CIPShimFlowFactory* NewL(TAny* aConstructionParameters);
	virtual ESock::CSubConnectionFlowBase* DoCreateFlowL(ESock::CProtocolIntfBase* aProtocolIntf, ESock::TFactoryQueryBase& aQuery);
	virtual ESock::CProtocolIntfFactoryBase* CreateProtocolIntfFactoryL(ESock::CProtocolIntfFactoryContainer& aParentContainer);
    virtual ~CIPShimFlowFactory();

protected:
	CIPShimFlowFactory(TUid aFactoryId, ESock::CSubConnectionFlowFactoryContainer& aParentContainer);

	};

#endif
