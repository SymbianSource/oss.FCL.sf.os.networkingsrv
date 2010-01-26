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
//



/**
 @internalComponent
*/
#ifndef __QOS_PROT_H__
#define __QOS_PROT_H__

#include <posthook.h>	// for CProtocolPosthook and all related IP Hook API definitions.

#include "qoslog.h"		// this header needs _LOG symbol
#include "module_if.h"	// for MEventInterface

class CQoSProvider;
class CFlowHook;
class CInterfaceManager;
class CQoSPolicyMgr;
class CQoSChannelManager;
class CModuleManager;
class TQoSConnId;
class TIpAddress;
class CPolicySelector;
class CModuleSelector;

const TUint KProtocolQoS			= 0x166;	
const TUint KAfPfqos				= 0x802;	
const TUint KQoSMajorVersionNumber  = 0;
const TUint KQoSMinorVersionNumber  = 1;
const TUint KQoSBuildVersionNumber  = 1;


// Hash table size for flows
const TUint KFlowTableSize = 67;

// Default values for configurable options
const TUint KUnloadDelay = 16000000;
_LIT(QOS_INI_DEFAULTFILE, "qospolicies.ini");

// Configurable options that can be tuned using INI-file
class TQoSConfigurableOptions
	{
public:
	TQoSConfigurableOptions() : iUnloadDelay(KUnloadDelay), iPolicyFile(QOS_INI_DEFAULTFILE) {}
public:
	TInt iUnloadDelay;
	TFileName iPolicyFile;
	};

class CProtocolQoS : public CProtocolPosthook, public MEventInterface
	{
public:
	CProtocolQoS();
	~CProtocolQoS();

	CServProviderBase* NewSAPL(TUint aSockType);
	void InitL(TDesC& aTag);
	TInt Send(RMBufChain& aPDU,CProtocolBase* aSourceProtocol=NULL);
	void Identify(TServerProtocolDesc* aProtocolDesc)const;
	static void Identify(TServerProtocolDesc& aDesc);//lint !e1411	because of CIp6Hook::Identity(TServerProtocolDesc *)

	void StartSending(CProtocolBase* iFace);
	MFlowHook *OpenL(TPacketHead &aHead, CFlowContext *aFlow);
	TInt GetFlowOption(TUint aLevel, TUint aName, TDes8 &aOption, const CFlowContext &aFlow) const;
	TInt SetFlowOption(TUint aLevel, TUint aName, const TDesC8 &aOption, CFlowContext &aFlow);
	void InterfaceAttached(const TDesC &aName, CNifIfBase *aIf);
	void InterfaceDetached(const TDesC& aName, CNifIfBase *aIf);
	virtual void NetworkAttachedL();
	virtual void NetworkDetached();

	TInt Event(TUint16 aEvent, CFlowHook& aHook, const TQoSParameters& aSpec, TInt aErrorCode, TInt aExtensionType=-1, const TDesC8& aExtension=TPtrC8(NULL,0));
	TInt Event(TUint16 aEvent, TInt aChannelId, const TQoSParameters& aSpec, TInt aErrorCode, TInt aExtensionType, const TDesC8& aExtension=TPtrC8(NULL,0));

	void Deliver(TPfqosMessage& aMsg, TUint aMask);
	CFlowHook* FindHook(const CFlowContext *aContext);
	void BlockFlow(CFlowContext& aFlow);
	void UnBlockFlow(CFlowContext& aFlow);
	void NotifyEvent(CFlowContext& aFlow, TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension=TExtensionData());
	void NotifyEvent(TInt aChannelId, TInt aEvent, const TQoSParameters* aParams, const TExtensionData& aExtension=TExtensionData());
	CSelectorBase *Lookup(const CFlowContext& aFlow, TUint aType, TUint aPriority, const TDesC& aName=TPtr(NULL,0));
	CSelectorBase* Lookup(const TInetAddr &aLocal, const TInetAddr &aRemote, TUint aProtocol, 
						  TUint aLocalPort, TUint aRemotePort, TUint aType, const TUidType& aUid, TUint32 aIapId,
						  TUint aPriority, const TDesC& aName=TPtr(NULL,0));
	void LoadFileL(const TDesC& aFile);
	TInt UnLoadFile(const TDesC& aFile);
	void CloseFlows();
	CFlowHook* FindHook(const TQoSConnId &aIdent);

	inline CModuleManager* ModuleMgr();
	inline CInterfaceManager* InterfaceMgr();
	inline CQoSPolicyMgr* PolicyMgr();
	inline CQoSChannelManager* ChannelMgr();
	inline const TQoSConfigurableOptions& ConfigOptions() const;

	// Called from CQoSProvider(s) when they receive PF_QOS message
	TInt Exec(TPfqosMessage &aMsg, CQoSProvider *aSrc);
	// Called from CQosPolicyMgr when the selector is removed
	TInt Release(const CPolicySelector* aSel);
protected:
	void Error(TPfqosMessage &aMsg, struct pfqos_msg &aBase, TInt aError);	//lint !e1411	becuase of CProtocolBase::Error(int, CProtocolBase)
	void DumpPolicyL(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, CPolicySelector *sel, CExtensionPolicy* ext=NULL);
	void DumpPolicy(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, CModuleSelector *sel);
	void DumpPolicy(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider *aDst, CExtensionPolicy *sel);
	TInt ExecUpdate(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecAdd(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecDelete(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecGet(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecReject(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecFlush(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecDump(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecConfigure(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecJoin(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecLeave(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecOpenExistingChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecCreateChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecDeleteChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecConfigChannel(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecLoadFile(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TInt ExecUnloadFile(TPfqosMessage &aMsg, struct pfqos_msg &aBase, CQoSProvider* aSrc);
	TUint FlowHashKey(const void* aFlow) const { return (((TUint)aFlow) & 0xff0) % KFlowTableSize; }
	void UpdateFlows();
	void ReadConfigOptions();

private:
	CModuleManager*			iModuleManager;
	CInterfaceManager*		iInterfaceManager;
	CQoSPolicyMgr*			iPolicyMgr;
	CQoSChannelManager*		iChannelMgr;
	TDblQue<CQoSProvider>	iSAPlist;
	TDblQue<CFlowHook>		iFlowTable[KFlowTableSize];
	TQoSConfigurableOptions	iConfigOptions;
	};


#ifdef _LOG
//
// Internal utilities for tracing and logging. Only for DEBUG compile
//
class TLogAddress : public TBuf<70>
	/** Format address%scope#port */
	{
public:
	TLogAddress() {}
	TLogAddress(const TIpAddress& aAddr, TInt aPort);
	TLogAddress(const TInetAddr& aAddr);
	void SetAddr(const TIpAddress& aAddr, TInt aPort);
	void SetAddr(const TInetAddr& aAddr);
	};
#endif

// inline methods
inline CModuleManager* CProtocolQoS::ModuleMgr()
	{ return iModuleManager; };

inline CInterfaceManager* CProtocolQoS::InterfaceMgr()
	{ return iInterfaceManager; };

inline CQoSPolicyMgr* CProtocolQoS::PolicyMgr()
	{ return iPolicyMgr; };

inline CQoSChannelManager* CProtocolQoS::ChannelMgr()
	{ return iChannelMgr; };

inline const TQoSConfigurableOptions& CProtocolQoS::ConfigOptions() const
	{ return iConfigOptions; };

#endif
