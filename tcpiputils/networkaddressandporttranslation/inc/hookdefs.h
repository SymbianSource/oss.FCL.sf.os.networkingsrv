// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// hookdef.h
// 
//

/**
 @file 
 @internalComponent
 @prototype
*/

#ifndef __HOOKDEFS_H__
#define __HOOKDEFS_H__

#include <ip6_hook.h>
#include <in_bind.h>
#include <in6_event.h>
#include "family.h"
#include <in_sock.h>
#include "translationtable.h"
#include <posthook.h>
#include <es_ini.h>
#include "panic.h"
#include "naptinterface.h"
#include "naptconfigtable.h"


/*
------------------------------------------------------------------------------------
					INCOMING PACKETS FROM PUBLIC INTERFACE TO PRIVATE


								ip stack

								/	  \
                              /         \
      						/             \
      |forward hook to change route|	 |Inbound Hook napt hook|
               			/					  \	
            	  	  /						    \
                    /						      \	
   			      /							        \   		
 			    /                                     \ 
        ----------                             -------------
		private interface						public interface
-------------------------------------------------------------------------------------

*/

/*
------------------------------------------------------------------------------------
				OUTCOMING PACKETS FROM PRIVATE INTERFACE TO PUBLIC


								ip stack

								/	  \
                              /         \
      						/             \
      		  |forward hook napt hook|      \ 
               			/					  \	
            	  	  /						    \
                    /						      \	
   			      /							        \   		
 			    /                                     \ 
        ----------                             -------------
		private interface						public interface
-------------------------------------------------------------------------------------

*/

//Definitions of ICMP messages. These messages are not supported by stack therefore
//defining them in this file.Since NAPT will forward packets which are coming there is
//no good reason to support only icmp messages which are supported by Stack.

const TUint8 KInetICMP_Information_Request= 15;
const TUint8 KInetICMP_Information_Reply= 16;
const TUint8 KInet4ICMP_AddressMask_Request= 17;
const TUint8 KInet4ICMP_AddressMask_Reply= 18;
const TUint8 KInet4ICMP_DNS_Request= 37;
const TUint8 KInet4ICMP_DNS_Reply= 38;

//ICMP CODES
const TUint8 KInet4ICMP_CODE_DF = 4;

const TUint KProtocolNAPT = 1000;

//Number of Service access points allowed
const TUint16 KMaxNumSap = 	5;

class CProtocolNapt;
class CNaptTimer;


NONSHARABLE_CLASS(CSapNapt) : public CServProviderBase
/** 
*  Class Derived from CServProviderBase. 
*  Inherit virtual functions from CServProviderBase.This Class is socket support for this 
*  Hook (protocol).Protocol or Hook could be manipulated by the Socket using socket options.
*  This class will be used to take downlink information from the private client.The SAP will 
*  called for all the Socket opened by the socket. SAP is 1 to 1 with Socket.
**/
	{

public:
friend class CProtocolNapt;
	CSapNapt();
	~CSapNapt();
	void Ioctl(TUint /*level*/,TUint /* name*/,TDes8* /* anOption*/);
	
	void Start();
	void Shutdown(TCloseType option);
	void LocalName(TSockAddr& anAddr) const; 
	TInt SetLocalName(TSockAddr& anAddr); 
	void RemName(TSockAddr& anAddr) const;
	TInt SetRemName(TSockAddr& anAddr);
	TInt GetOption(TUint /*level*/,TUint /*name*/,TDes8& /*anOption*/)const;	
	void CancelIoctl(TUint /*aLevel*/,TUint /*aName*/);
	TInt SetOption(TUint aLevel,TUint aName,const TDesC8& anOption); 
	void ActiveOpen();
	void ActiveOpen(const TDesC8& /*aConnectionData*/);
	TInt PassiveOpen(TUint /*aQueSize*/);
	TInt PassiveOpen(TUint /*aQueSize*/,const TDesC8& /*aConnectionData*/);
	void Shutdown(TCloseType /*option*/,const TDesC8& /*aDisconnectionData*/);
	void AutoBind();
  	TInt SecurityCheck(MProvdSecurityChecker* aChecker);
  	TNaptConfigInfo* GetConfigInfo();
  	void SetConfigInfo(TNaptConfigInfo *cinfo);

private:
    void DoBindToL(TInt aStatus);
	CProtocolNapt* iProtocol;
	CNaptClientConfigMgr* iConfigMgr;
	TNaptConfigInfo* iConfigInfo;
	};  //End Class CProtocolSap
	
	
class CProtocolNaptIn;	

 NONSHARABLE_CLASS(CProtocolNapt) : public CProtocolPosthook 
/** 
* Class Derived from CIp6Hook.
* The class will implement all the virtual functions of Base class CIp6Hook (ApplyL).
* The class object will be the Protocol instance will open service access point for that
* Protocol.Service access point will be used to manipulate Protocol using socket option.
* This will Open interested flows for manipulation.
**/

	{
public:
	friend class CProtocolNaptIn;
	static CProtocolNapt* NewL();
	~CProtocolNapt();
	void Identify(TServerProtocolDesc* aProtocolDesc)const;
	TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);
	void BindL(CProtocolBase* protocol, TUint id);
	void BindToL(CProtocolBase* aProtocol);
	CServProviderBase* NewSAPL(TUint aProtocol);
	void ForwardManipulation(RMBufHookPacket& aPacket,RMBufRecvInfo& aInfo);	
    void IcmpHandlerForward(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);
    static void Describe(TServerProtocolDesc& anEntry);
    void NetworkAttachedL();
    virtual MNetworkService *NetworkService()
  	  {
    	return CProtocolPosthook::NetworkService();
  	  }
   
   	void ReadConfigurationFile();
	void ConfigInfo();
	TInt GetIniValue(const TDesC& aSection, const TDesC& aName, TInt aDefault, TInt aMin, TInt aMax);
	TNaptTableMapMgr iNaptMapMgr;
	void CancelSap(CSapNapt *aSap);
private:
	void CreateL();
	MInterfaceManager* iManager;
	
	//table related information
	CNaptTimer* iTimer;
	CProtocolNaptIn* iProtInbound;
	RPointerArray<CSapNapt> iSapList;
	CNaptClientConfigMgr* iConfigMgr; 
	TNaptConfigInfo* iCurConfigInfo;
	
public:
    TBool iBoundFlag;
    CProtocolBase* iProtBase;	
	
	}; //End class CProtocolNapt
	
NONSHARABLE_CLASS(CProtocolNaptIn) : public CIp6Hook
/** 
* Class Derived from CIp6Hook.
* The class will implement all the virtual functions of Base class CIp6Hook (ApplyL).
* The class object will be the Protocol instance will open service access point for that
* Protocol.Service access point will be used to manipulate Protocol using socket option.
* This will Open interested flows for manipulation.
**/
	{
	public:
	CProtocolNaptIn(CProtocolNapt* aNapt);
	~CProtocolNaptIn();
	TInt IncomingPacketManipulation(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);
	TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);
	TInt IcmpHandler(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo) ;
    TInt IcmpHandlerTracert(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo, TNaptConfigInfo** aConfigInfo);
	
	private:	
	CProtocolNapt* iNapt;
	CNaptClientConfigMgr *iConfigMgr;
	
	};

NONSHARABLE_CLASS(CNaptTimer) : public CTimer
	/**
	*  Class CNaptTimer deriver from CTimer.
	*  Timer instance is created during hook attach time.
	*  It will be started only when a first packet needs NAPT translation.
	*  Timer will be stopped only when there are no packets.
	*/
	{

	public:
		static CNaptTimer* NewL(CProtocolNapt* aNaptProtcol);
     	~CNaptTimer();
     	
		void StartTimer();
		void Cancel();
		void RunL();
	
		TInt  iNaptTableScanInterval;
		TInt  iNaptTcpIdleTimeout;
		TInt  iNaptUdpIdleTimeout;
		TInt  iNaptIcmpIdleTimeout;
		TInt  iNaptTcpOpenTimeout;
		TInt  iNaptTcpCloseTimeout;
		
	private:
		CNaptTimer();
		CProtocolNapt*  iNaptProtocol;
		TNaptTableMapMgr* iMapMgrPtr;
		
	
	
	};
	
inline 	 CNaptTimer::CNaptTimer() : CTimer(EPriorityStandard)
 {
 }



#endif //__HOOKDEFS_H__
