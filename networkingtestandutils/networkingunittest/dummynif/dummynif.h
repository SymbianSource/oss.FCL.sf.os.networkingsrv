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
 
#if (!defined __DUMMYNIF_H__)
#define __DUMMYNIF_H__

#include <comms-infras/nifif.h>
#include <nifutl.h>
#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <udp_hdr.h>
#include <in_iface.h>
#include <eui_addr.h>	// TE64Addr
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <comms-infras/nifprvar_internal.h>
#endif

class RMBuf;

// Delay pipe is implemented is implemented as circ buffer of RMBuf*, each being the start of chain that
// the stack transmitted. A NULL terminates a series considered to be simultaneous, the whole queue being
// primed with a few NULLs to get things slow starting (maybe)
const TInt KDelayQuantum = 100 * 1000;
const TInt KDelaySlots = 0;
const TInt KDelayQueueSize = 1000;


class CDummyIfLink;

class CDummyIf4 : public CNifIfBase
/**
IPv4 interface binder
@internalComponent
*/
	{
	friend class CDummyIfLink;
public:
	CDummyIf4(CDummyIfLink& aLink);
	~CDummyIf4();
    virtual void BindL(TAny *aId);
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Control(TUint, TUint, TDes8&, TAny*);
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Recv(RMBufChain& aPdu);
	virtual TInt State();
private:
	void UpdateHeaders(TInet6HeaderIP4* aIp4, TInet6HeaderUDP* aUdp);
	
	static TInt DrainNextDrips(TAny* aSelf);
protected:
	TUint32 iLocalAddressBase;
	TUint32 iLocalAddress;
	CProtocolBase* iProtocol;
	TInterfaceName iIfName;
private:
	CDummyIfLink* iLink;
	};

class CDummyIf6 : public CNifIfBase
/**
IPv6 interface binder
@internalComponent
*/
	{
	friend class CDummyIfLink;
public:
	CDummyIf6(CDummyIfLink& aLink);
	// Inherited virtual methods from CNifIfBase
    virtual void BindL(TAny *aId);
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Control(TUint, TUint, TDes8&, TAny*);
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Recv(RMBufChain& aPdu);
	virtual TInt State();
private:
	void UpdateHeaders(TInet6HeaderIP* aIp6, TInet6HeaderUDP* aUdp);
	void StaticDnsConfiguration();
	TInt PresetAddr(TIp6Addr& aAddr, const TDesC& aVarName);
protected:
	TE64Addr iLocalIfId;
	TE64Addr iRemoteIfId;
	TIp6Addr iPrimaryDns;
	TIp6Addr iSecondaryDns;
	CProtocolBase* iProtocol;
	TInterfaceName iIfName;
private:
	CDummyIfLink* iLink;
	};

class CDummyIfFactory;
class CDummyIfLink : public CNifIfLink, public MTimer
/**
Common link layer class
@internalComponent
*/
	{
public:
	CDummyIfLink(CNifIfFactory& aFactory);
	~CDummyIfLink();
	virtual TInt Send(RMBufChain& aPdu, TAny* aSource=0);
	virtual TInt Start();
	virtual void Stop(TInt aReason, MNifIfNotify::TAction aAction);		
	virtual void AuthenticateComplete(TInt aResult);
	virtual void BindL(TAny *aId);
    virtual CNifIfBase* GetBinderL(const TDesC& aName);
	virtual void TimerComplete(TInt aStatus);
	virtual void Info(TNifIfInfo& aInfo) const;
	virtual TInt Notification(TAgentToNifEventType aEvent, void * aInfo);
	virtual void Restart(CNifIfBase* aIf);
	static void FillInInfo(TNifIfInfo& aInfo, TAny* aPtr);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption, TAny* aSource=0);
	
	CDummyIfFactory& Factory();
public:
	//CProtocolBase* iProtocol;
	CDummyIf4* iNifIf4;
	CDummyIf6* iNifIf6;
	CNifIfFactory* iFactory;
	
private:
	TInt SetNifmanIdleTimeout(const TDesC& aTimeoutToSet, const TDes8& aTimeoutValueBuf);
	};

class CDummyIfFactory : public CNifIfFactory
/**
Dummy NIF factory class
@internalComponent
*/
	{
public:	
	virtual ~CDummyIfFactory();
	void SetDripReceiver(TCallBack aReceiver);
	RMBuf* GetDrip();
	void AddDrip(RMBuf* aDrip);
protected:
	virtual void InstallL();
	virtual CNifIfBase* NewInterfaceL(const TDesC& aName);
	virtual TInt Info(TNifIfInfo& aInfo, TInt aIndex) const;
private:	
	static TInt DripCallback(TAny* aSelf);
protected:
	CPeriodic* iNetDelayTimer;
	CCirBuf<RMBuf*> iDelayPipe;
	TCallBack iDripReceiver;
	};
/**
@internalComponent
*/
const TInt KHexDumpWidth = 16;

class CDummyIfLog : public CBase
/**
@internalComponent
*/
	{
public:
	static void Write(const TDesC& aDes);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	static void HexDump(const TText* aHeader, const TText* aMargin, const TUint8* aPtr, TInt aLen, TInt aWidth = KHexDumpWidth);
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

#endif
