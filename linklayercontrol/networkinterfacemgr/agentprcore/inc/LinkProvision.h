/**
* Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file declaring the RawIP Provisioning structures.
* 
*
*/



/**
 @file
 @internalTechnology
*/

#ifndef LINKPROVISION_H_
#define LINKPROVISION_H_


#include <metadatabase.h>			// for TMDBElementId
#include <comms-infras/agentmessages.h>

#ifdef SYMBIAN_ZERO_COPY_NETWORKING
#include <comms-infras/commsbufpondop.h>
#endif

#include <in_sock.h>
#include <etelpckt.h>

namespace ESock 
	{
	class CCommsDatIapView;
	}

#ifdef SYMBIAN_ZERO_COPY_NETWORKING
struct SCommsPondContext
{
	inline RCommsBufPond GetCommsPond();
};
#endif

class CBCAProvision : public CBase, public Meta::SMetaData
/**
Provisioning information for CRawIPFlow
*/
	{
public:
    enum 
    	{
    	EUid = 0x10281DE6,
    	ETypeId = 1,
    	};

    IMPORT_C static CBCAProvision* NewLC(ESock::CCommsDatIapView* aIapView);
    
    //getters
    inline const TDesC& GetBCAStack() const;
    inline const TDesC& GetBCAName() const;
    inline const TDesC& GetPortName() const;
    inline TUint32 GetIAPid() const;
    inline TCommRole GetCommRole() const;
    inline TUint32 GetHandShaking() const;

#ifdef SYMBIAN_ZERO_COPY_NETWORKING
	inline RCommsBufPond  GetCommsPond();
#endif

    //setters
	inline void  SetBCAStack(const TDesC& aBCAStack);	
	inline void  SetBCAName(const TDesC& aBCAName);	
	inline void  SetIAPid(TUint32 aIAPid);	
	inline void	 SetPortName(const TDesC& aPortName);
	inline void  SetCommRole(TCommRole aCommRole);	
	inline void  SetHandShaking(TUint32 aHandShaking);
	
protected:
    void SetBcaStackAndName(ESock::CCommsDatIapView* aIapView);

	IMPORT_C ~CBCAProvision();
	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);

private:
	RBuf iBCAStack;
	RBuf iBCAName;
	RBuf iPortName;
	TCommRole iCommRole;
	TUint32 iHandShaking;
    TUint32 iapId; //This is to Read IAPID from CommDB.
	               //IAP ID is used in BCA Ioctl method to set IapId for accessing CommDB.
	               //REMEK: and probably BCA shouldn't really need it.

#ifdef SYMBIAN_ZERO_COPY_NETWORKING
	 SCommsPondContext iCommsPondContext;
#endif

public:
    EXPORT_DATA_VTABLE_AND_FN
	};
	
class CIPConfig : public CBase, public Meta::SMetaData
/** 

@internalTechnology
@released Since 9.4
*/
	{
public:
    enum 
    {
    EUid = 0x10281DE6,
    ETypeId = 2,
    };

	IMPORT_C static CIPConfig* NewLC(ESock::CCommsDatIapView* aIapView);
	IMPORT_C static CIPConfig* NewFromGPRSOutLC(ESock::CCommsDatIapView* aIapView);

    //getters	
    inline TUint32 GetIpAddress() const;
    inline TUint32 GetIp4NameServer1() const;
    inline TUint32 GetIp4NameServer2() const;
    inline const TIp6Addr& GetIp6NameServer1() const;
    inline const TIp6Addr& GetIp6NameServer2() const;
    inline TUint32 GetIpNetMask() const;
    inline TUint32 GetIpGateway() const;
    inline TBool   GetIpAddrFromServer() const;
    inline TBool   GetIp4DNSAddrFromServer() const;
    inline TBool   GetIp6DNSAddrFromServer() const;
    inline TBool   GetEnableIpHeaderComp() const;
    inline const TDesC& GetPortName() const;    
    inline TUint32 GetBroadCastAddr() const;    
    
    //setters
    inline void  SetIpAddress(TUint32 aIPAddress);
    inline void  SetIp4NameServer1(TUint32 aIp4NameServer1);
    inline void  SetIp4NameServer2(TUint32 aIp4NameServer2);
    inline void  SetIp6NameServer1(const TIp6Addr& aIp6NameServer1);
    inline void  SetIp6NameServer2(const TIp6Addr& aIp6NameServer2);	
    inline void  SetIpNetMask(TUint32 aIpNetMask);
    inline void  SetIpGateway(TUint32 aIpGateway);
    inline void  SetIpAddrFromServer(TBool aIpAddrFromServer);
    inline void  SetIp4DNSAddrFromServer(TBool aIp4DNSAddrFromServer);
    inline void  SetIp6DNSAddrFromServer(TBool aIp6DNSAddrFromServer);
    inline void  SetEnableIpHeaderComp(TBool aEnableIpHeaderComp);
	inline void  SetPortName(const TDesC& aPortName);
    inline void  SetBroadCastAddr(TUint32 aBroadCast);
	

protected:
	void InitialiseCommonConfigL(ESock::CCommsDatIapView* aIapView);
	void InitialiseConfigFromDialOutL(ESock::CCommsDatIapView* aIapView);
	void InitialiseConfigFromGPRSOutL(ESock::CCommsDatIapView* aIapView);
	void InitialiseConfigL(ESock::CCommsDatIapView* aIapView);
    IMPORT_C ~CIPConfig();

    TUint32  iIpAddress;
    TUint32  iIp4NameServer1;
    TUint32  iIp4NameServer2;
    TIp6Addr iIp6NameServer1;
    TIp6Addr iIp6NameServer2;
    TUint32  iIpNetMask;
    TUint32  iIpGateway;
	RBuf	 iPortName; //to compose unique binder names.
    TUint	 iIpAddrFromServer:1;
    TUint	 iIp4DNSAddrFromServer:1;
    TUint	 iIp6DNSAddrFromServer:1;
    TUint	 iEnableIpHeaderComp:1;
  	TUint32	 iBroadcastAddr;
    
public:
    EXPORT_DATA_VTABLE_AND_FN
};	

	
class CRawIpAgentConfig : public CBase, public Meta::SMetaData
/**
Provisioning information for ConnectionSpeed (usually suplied by agents)
*/
	{
	friend class CRawIpAgentHandler;
public:
    enum 
    {
    EUid = 0x10281DE6,
    ETypeId = 3,
    };
    
    IMPORT_C static CRawIpAgentConfig* NewLC(ESock::CCommsDatIapView* aIapView, 
                const TPacketDataConfigBase* aGprsConfig = NULL);
                	
	TUint iConnectionSpeed;
	const TPacketDataConfigBase* iGprsConfig;
    
public:
    EXPORT_DATA_VTABLE_AND_FN
protected:
    CRawIpAgentConfig(const TPacketDataConfigBase* aGprsConfig)
    :iGprsConfig(aGprsConfig)
    {}

    CRawIpAgentConfig()
    {}
    
    };
	
#include <comms-infras/linkprovision.inl>	

#endif // LINKPROVISION_H_
