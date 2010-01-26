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
* CNifIfBase and CProtocolBase shim layer functionality
* 
*
*/



/**
 @file nif.h
*/

#if !defined(NIF_H_INCLUDED)
#define NIF_H_INCLUDED

#include <e32base.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/nifif.h>
#include <comms-infras/es_protbinder.h>
#include <comms-infras/ss_flowbinders.h>
#include "notify.h"
#include "flow.h"
#include <networking/ipaddrinfoparams.h>
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifif_internal.h>
#include <comms-infras/nifprvar_internal.h>
#endif

class MNifIfUser;
class CIPShimNotify;

class CHookAddressInfo;
class CIPProtoBinder;

const TInt KMaxProtocolNameSize = 16;

_LIT8(KProtocolIp, "ip");
_LIT8(KProtocolIp6, "ip6");


class CIPShimIfBase : public CNifIfBase,
	//interfaces for the flow below
	public ESock::MUpperControl, public ESock::MUpperDataReceiver
	
	{
/**
Class that represents a NIF to the TCP/IP stack and presents and presents an
upperdatareceiver to the flows below


@internalComponent
*/
public:
    friend class CIPShimSubConnectionFlow;

public:
	static CIPShimIfBase* NewL(const TDesC8& aProtocol, CIPShimProtocolIntf *iProtIntf);
	~CIPShimIfBase();
	
	void StartL();
	void BindToL(CIPProtoBinder* aIPProtoBinder);
	void UnbindFrom(CIPProtoBinder* aIPProtoBinder);

	inline void SetProtocolIntf(CIPShimProtocolIntf *aIntf);
	void CleanupInterface(TInt aError);
	void Release(TInt aError);
	
	const TConnectionInfo& ConnectionInfo();
	void SetConnectionInfo(const TConnectionInfo& aConnectionInfo);
	const TDesC8& ProtocolName();

	CIPShimNotify* const ShimNotify() const { return iShimNotify; };

	// IP4/IP6 specific required derivations
	virtual void GetConfigFirstTime() = 0;

    //-=========================================
    // MUpperDataReceiver methods
    //-=========================================        
 	void Process(RMBufChain& aData);
 
    //-=========================================
    // MUpperControl methods
    //-=========================================        
	void StartSending();
	void Error(TInt anError);


	CProtocolBase* iUpperProtocol;
	
	CIPShimSubConnectionFlow& Flow();
	CIPShimProtocolIntf* ProtcolIntf();
	
	void AddIpAddrInfoL(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo);
	void RemoveIpAddrInfo(CIPProtoBinder* aBinder, CSubConIPAddressInfoParamSet::TSubConIPAddressInfo& aAddrInfo);
	void RemoveIpAddrInfo(CIPProtoBinder* aBinder);
	void RemoveInterfaceName(CIPProtoBinder* aBinder);
protected:
	CIPShimIfBase(const TDesC8& aProtocolName);
	void ConstructL();

	// from CNifIfBase
	void BindL(TAny *aId);
	TInt State();
	TInt Control(TUint aLevel,TUint aName,TDes8& aOption, TAny* aSource=0);
	void Info(TNifIfInfo& aInfo) const;
	TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	TInt Notification(TAgentToNifEventType aEvent, void * aInfo);

	// IP4/IP6 specific required derivations
	virtual TInt ServiceInfoControl(TDes8& aOption, TUint aName) = 0;
	virtual TInt ServiceConfigControl(TDes8& aOption) = 0;
	
	RPointerArray<CIPProtoBinder> iProtoBinders;
	

private:
	TInt ServiceHwAddrControl(TDes8& aOption);
	TInt ServiceConnInfo(TDes8& aOption);
	
private:
	TBuf8<KMaxProtocolNameSize> iProtocolName;	// "ip" or "ip6"
	TConnectionInfo iConnectionInfo;
	
	CIPShimNotify* iShimNotify;                 // note: CNifIfBase::iNotify is an alias of this
	mutable TBool iInterfaceNameRecorded;		// Used by Info() which is inherited const from CNifIfBase
	TBool iBinderReady;	
	MNifIfUser* iNifUser;
	
	CHookAddressInfo* iHookAddressInfo;
	CIPShimProtocolIntf *iProtIntf; // needed for cleanup
	};

    

void CIPShimIfBase::SetProtocolIntf(CIPShimProtocolIntf *aIntf)
	{
	iProtIntf = aIntf;
	}
	

#endif
//NIF_H_INCLUDED

