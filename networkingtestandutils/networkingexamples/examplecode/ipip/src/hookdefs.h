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
//

#ifndef __HOOKDEFS_H__
#define __HOOKDEFS_H__

#include <ip6_hook.h>
#include <in_bind.h>

#include "policy.h"

class CProtocol_t;
class CFlowHook : public CBase, public MFlowHook
	{
public:
	static CFlowHook* NewLC(TPacketHead &aHead, CIpIpActionSpec& aAction, CProtocol_t& aProtocol);
	~CFlowHook() {}

	void Open();
	TInt ReadyL(TPacketHead& aHead);
	TInt ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo);
	void Close();

private:
	CFlowHook(TPacketHead &aHead, CIpIpActionSpec& aAction, CProtocol_t& aProtocol);

private:
	TPacketHead iPacketHead;
	CIpIpActionSpec& iAction;
	CProtocol_t& iProtocol;
	TInt iAccessCount;
	};

class CSap_t : public CServProviderBase
	{
public:
	CSap_t(CProtocol_t& aProtocol);

	void Ioctl(TUint level,TUint name,TDes8* anOption);
	
	void Start() {}
	void Shutdown(TCloseType option) { (void)option; }

	void LocalName(TSockAddr& anAddr) const {  (void)anAddr;  Panic(EIpipPanic_BadCall); }
	TInt SetLocalName(TSockAddr& anAddr) { (void)anAddr;  Panic(EIpipPanic_BadCall); return KErrNone; }
	void RemName(TSockAddr& anAddr) const { (void)anAddr;  Panic(EIpipPanic_BadCall); }
	TInt SetRemName(TSockAddr& anAddr) { (void)anAddr;  Panic(EIpipPanic_BadCall); return KErrNone; }
	TInt GetOption(TUint /*level*/,TUint /*name*/,TDes8& /*anOption*/)const { Panic(EIpipPanic_BadCall); return KErrNone; }
	
	void CancelIoctl(TUint /*aLevel*/,TUint /*aName*/) { Panic(EIpipPanic_BadCall); }
	TInt SetOption(TUint /*level*/,TUint /*name*/,const TDesC8 &/*anOption*/) { Panic(EIpipPanic_BadCall); return KErrNone; }
	void ActiveOpen() { Panic(EIpipPanic_BadCall); }
	void ActiveOpen(const TDesC8& /*aConnectionData*/) { Panic(EIpipPanic_BadCall); }
	TInt PassiveOpen(TUint /*aQueSize*/) { Panic(EIpipPanic_BadCall); return KErrNone; }
	TInt PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/) { Panic(EIpipPanic_BadCall); return KErrNone; }
	
	void Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/) { Panic(EIpipPanic_BadCall); }
	void AutoBind() { Panic(EIpipPanic_BadCall); }

private:
	CProtocol_t& iProtocol;
	};

class CProtocol_t : public CIp6Hook
	{
public:
	static CProtocol_t* NewL();
	~CProtocol_t();

	void Identify(TServerProtocolDesc* aProtocolDesc)const;
	TInt ApplyL(RMBufHookPacket &aPacket, RMBufRecvInfo &aInfo);
	void BindL(CProtocolBase* protocol, TUint id);
	void BindToL(CProtocolBase* protocol);
	MFlowHook* OpenL(TPacketHead &aHead, CFlowContext *aFlow);
	CServProviderBase* NewSAPL(TUint aProtocol);

	void Ioctl(TDesC8& aOption);
private:
	CProtocol_t() {}
	void CreateL();
	
friend class CFlowHook;
	void RemoveFlowFromList(const CFlowHook* aFlow);

private:
	CProtocolInet6Binder* iIPProtocol;
	RPointerArray<CFlowHook> iHookList;
	CPolicyHolder* iPolicyHolder;
	};

class CProtocolFamily_t : public CProtocolFamilyBase
	{
public:
	CProtocolFamily_t();
	~CProtocolFamily_t();
	TInt Install();
	TInt Remove();
	TUint ProtocolList(TServerProtocolDesc *& aProtocolList);
	CProtocolBase* NewProtocolL(TUint /*aSockType*/, TUint aProtocol);
	};

#endif //__HOOKDEFS_H__
