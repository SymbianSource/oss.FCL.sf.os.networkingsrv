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
// RawIPMCPR.CPP
// RawIP MCPR
// 
//

/**
 @file
 @internalComponent
*/
#include "LinkProvision.h"
#include <comms-infras/ss_tiermanagerutils.h>
#include <commsdattypesv1_1.h> // CommsDat
#include <comms-infras/ss_log.h> // KESockConnectionTag
#include "agentmcpr.h"


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCLnkP, "NifManAgtPrCLnkP");
#endif

#ifdef __CFLOG_ACTIVE
#define KLinkProvisionTag KESockConnectionTag
_LIT8(KLinkProvisionSubTag, "linkprovision");
#endif

using namespace CommsDat;
using namespace ESock;

//-=========================================================
// CBCAProvision methods
//-=========================================================
EXPORT_C CBCAProvision* CBCAProvision::NewLC(ESock::CCommsDatIapView* aIapView)
	{
	CBCAProvision* self = new (ELeave) CBCAProvision;
	CleanupStack::PushL(self);
	self->InitialiseConfigL(aIapView);
	return self;
	}

EXPORT_C CBCAProvision::~CBCAProvision()
    {
    iBCAStack.Close();
    iBCAName.Close();
    iPortName.Close();
    }

void CBCAProvision::InitialiseConfigL(CCommsDatIapView* aIapView)
    {
	HBufC* buf = NULL;
	TUint32 uintDBVar = 0;
	
	//-====================================================
	//LENIENT PROVISION
	//-====================================================
	//With some provision parameters (e.g.: portName), rawIpMCPR 
	//tries to be lenient as technically they can be supplied by
	//someone further in the chain (e.g.: GPRS), if not found in the db.
	TRAPD(getErr, aIapView->GetTextL(KCDTIdPortName, buf));
    if (getErr == KErrNone )
        {
        __ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCLnkP, 1));
    	SetPortName(*buf);
    	delete buf;        
        }
    else if (getErr != KErrNotFound )
    	{
    	User::Leave(getErr);
    	}

	
	// =================================================
	// Mandatory Provision
	// =================================================
    SetBcaStackAndName(aIapView);

	TInt err = aIapView->GetInt(KCDTIdCommRole, uintDBVar);
	if (err != KErrNone)
		{
		uintDBVar = 0;  // default role is DTE
		}
    SetCommRole((uintDBVar & KModemCommRoleDCE) ? ECommRoleDCE : ECommRoleDTE);
    
	// Excess data is provisioned by the Agent Provider.

	uintDBVar = 0;
	err = aIapView->GetInt(KCDTIdHandshaking, uintDBVar);
	if (err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CBCAProvision:\tInitialiseConfigL() - error %d reading handshake value from database\r\n"), err); )
		User::Leave(err);
		}

    SetHandShaking(uintDBVar);
    SetIAPid(aIapView->IapId());
    }

void CBCAProvision::SetBcaStackAndName(CCommsDatIapView* aIapView)
/**
Setup BCA related fields

@param aIapView IAP View object used to read CommsDat fields
@param aHDLCConfig provisioning object to populate with retrieved information.
*/
	{
	const TChar stackDelimiter(',');	

	HBufC* bcaStack = NULL;
	TInt err = aIapView->GetText(KCDTIdBCAStack, bcaStack);
	TInt length = 0;
	if (err != KErrNone || (length = bcaStack->Length()) == 0)
    	{
		_LIT(KBcaName,"C32Bca");
		SetBCAName(KBcaName);
        SetBCAStack(KBcaName);
		delete bcaStack;
        return;
    	}
    	
	TInt delimiterLoc = bcaStack->Locate(stackDelimiter);
	
	if(delimiterLoc > 0)
		{
        SetBCAName(bcaStack->Left(delimiterLoc));
        SetBCAStack(bcaStack->Right(length-delimiterLoc-1));
		}
	else
		{
		SetBCAName(*bcaStack);
        SetBCAStack(*bcaStack);
		}
	delete bcaStack;
	}

//-=========================================================
// CIPConfig methods
//-=========================================================
EXPORT_C CIPConfig* CIPConfig::NewLC(ESock::CCommsDatIapView* aIapView)
	{
	CIPConfig* self = new (ELeave) CIPConfig;
	CleanupStack::PushL(self);
	self->InitialiseConfigL(aIapView);
	return self;
	}

EXPORT_C CIPConfig* CIPConfig::NewFromGPRSOutLC(ESock::CCommsDatIapView* aIapView)
	{
	CIPConfig* self = new (ELeave) CIPConfig;
	CleanupStack::PushL(self);
	self->InitialiseConfigFromGPRSOutL(aIapView);
	return self;
	}

EXPORT_C CIPConfig::~CIPConfig()
    {
    iPortName.Close();
    }
    
void CIPConfig::InitialiseConfigL(ESock::CCommsDatIapView* aIapView)
    {
    CommsDat::TMDBElementId serviceTypeTable = aIapView->GetServiceTableType();
    switch (serviceTypeTable)
        {
        case KCDTIdDialOutISPRecord:
            InitialiseConfigFromDialOutL(aIapView);
            break;
        case KCDTIdOutgoingGprsRecord:
            InitialiseConfigFromGPRSOutL(aIapView);
            break;
        default:
            User::Leave(KErrCorrupt);
        };
    }


void CIPConfig::InitialiseConfigFromDialOutL(ESock::CCommsDatIapView* aIapView)
    {	
	TUint32 addr = 0;
	TIp6Addr addr6 = {0};
	TBool boolDBVar = EFalse;
	TInt err = KErrNone;

    err = aIapView->GetBool(KCDTIdIpAddrFromServer, boolDBVar);
    if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromDialOutL() - error %d reading IpAddressFromServer value from database\r\n"), err); )		
		}
    SetIpAddrFromServer(boolDBVar);
    if (!boolDBVar)
        {        
        if (KErrNone == CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdIpAddr, addr))
        	{
        	SetIpAddress(addr);
        	}
        }
    else
    	{
    	SetIpAddress(KInetAddrNone);
    	}
    
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdIpDNSAddrFromServer, boolDBVar);
    SetIp4DNSAddrFromServer(boolDBVar);
    if (!boolDBVar)
        {
        addr = 0;
        CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdIpNameServer1, addr);
        SetIp4NameServer1(addr);
        
        addr = 0;
        CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdIpNameServer2, addr);
        SetIp4NameServer2(addr);
        }
    else
    	{
    	SetIp4NameServer1(KInetAddrNone);
        SetIp4NameServer2(KInetAddrNone);	
    	}
    
    boolDBVar = EFalse;    
    err = aIapView->GetBool(KCDTIdIp6DNSAddrFromServer, boolDBVar);
    SetIp6DNSAddrFromServer(boolDBVar);
    if (!boolDBVar)
        {
        CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdIp6NameServer1, addr6);	
        SetIp6NameServer1(addr6);
                
        CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdIp6NameServer2, addr6);
        SetIp6NameServer2(addr6);
        }    
    else
    	{
    	SetIp6NameServer1(KInet6AddrNone);
        SetIp6NameServer2(KInet6AddrNone);	
    	}
        
    addr = 0;
    CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdIpNetMask, addr);
    SetIpNetMask(addr);
    
    addr = 0;
    CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdIpGateway, addr);
    SetIpGateway(addr);
    
    boolDBVar = EFalse;
	err = aIapView->GetBool(KCDTIdEnableIpHeaderComp, boolDBVar);
	if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromDialOutL() - error %d reading Header Compression value from database\r\n"), err); )
		}
    SetEnableIpHeaderComp(boolDBVar);  
	InitialiseCommonConfigL(aIapView);      
    }

void CIPConfig::InitialiseConfigFromGPRSOutL(CCommsDatIapView* aIapView)
    {	
	TUint32 addr;
	TIp6Addr addr6;
	TBool boolDBVar = EFalse;
	TInt err = KErrNone;

    err = aIapView->GetBool(KCDTIdWCDMAIPAddrFromServer | KCDTIdOutgoingGprsRecord, boolDBVar);
    if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading IpAddressFromServer value from database\r\n"), err); )		
		}
    SetIpAddrFromServer(boolDBVar);
    //read the address from the DB regardless the IpAddrFromServer flag
    err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdWCDMAIPAddr | KCDTIdOutgoingGprsRecord, addr);
    if (!boolDBVar && err != KErrNone)
        {
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading IpAddress value from database\r\n"), err); )		
        }
    SetIpAddress(addr);
    
    boolDBVar = EFalse;
    err = aIapView->GetBool(KCDTIdWCDMAIPDNSAddrFromServer | KCDTIdOutgoingGprsRecord, boolDBVar);
    if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip4DNSAddrFromServer value from database\r\n"), err); )
		}    
    SetIp4DNSAddrFromServer(boolDBVar);
    //read the address from the DB regardless the Ip4DNSAddrFromServer flag
    err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdWCDMAIPNameServer1 | KCDTIdOutgoingGprsRecord, addr);	
    if (!boolDBVar && err != KErrNone)
        {
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip4DNSAddr value from database\r\n"), err); )		
        }
    
    SetIp4NameServer1(addr);
    err = CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdWCDMAIPNameServer2 | KCDTIdOutgoingGprsRecord, addr);
    if (!boolDBVar && err != KErrNone)
        {
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip4DNSAddr2 value from database\r\n"), err); )		
        }
    
    SetIp4NameServer2(addr);
    
    boolDBVar = EFalse;    
    err = aIapView->GetBool(KCDTIdWCDMAIP6DNSAddrFromServer | KCDTIdOutgoingGprsRecord, boolDBVar);
    if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip6DDNSAddrFromServer value from database\r\n"), err); )
		}      
    SetIp6DNSAddrFromServer(boolDBVar);
    //read the address from the DB regardless the Ip6DNSAddrFromServer flag    
    err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdWCDMAIP6NameServer1 | KCDTIdOutgoingGprsRecord, addr6);	
    if (!boolDBVar && err != KErrNone)
        {
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip6DNSAddr value from database\r\n"), err); )		
        }
    
    SetIp6NameServer1(addr6);
    err = CAgentMetaConnectionProvider::GetIp6Addr(aIapView, KCDTIdWCDMAIP6NameServer2 | KCDTIdOutgoingGprsRecord, addr6);
    if (!boolDBVar && err != KErrNone)
        {
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Ip6DNSAddr2 value from database\r\n"), err); )		
        }
    
    SetIp6NameServer2(addr6);
        
    CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdWCDMAIPNetMask | KCDTIdOutgoingGprsRecord, addr);		
    SetIpNetMask(addr);
    CAgentMetaConnectionProvider::GetIp4Addr(aIapView, KCDTIdWCDMAIPGateway | KCDTIdOutgoingGprsRecord, addr);
    SetIpGateway(addr);
    
    boolDBVar = EFalse;
	err = aIapView->GetBool(KCDTIdWCDMAHeaderCompression| KCDTIdOutgoingGprsRecord, boolDBVar);
	if (KErrNone != err)
		{
		__CFLOG_VAR((KLinkProvisionTag, KLinkProvisionSubTag, _L8("CIPConfig:\tInitialiseConfigFromGPRSOutL() - error %d reading Header Compression value from database\r\n"), err); )
		}
    SetEnableIpHeaderComp(boolDBVar);    
	InitialiseCommonConfigL(aIapView);
    }
    
void CIPConfig::InitialiseCommonConfigL(ESock::CCommsDatIapView* aIapView)
    {
	// Because CommDB doesn't define a Broadcast Address field, we must
	// calculate the broadcast address. This is based on the localAddr
	// and the netMask.
	TInetAddr localAddr(GetIpAddress(), 0);
	TInetAddr netMask(GetIpNetMask(), 0);
	TInetAddr broadcast;
	HBufC* buf = NULL;
		
	broadcast.SubNetBroadcast(localAddr, netMask);
	SetBroadCastAddr(broadcast.Address());
	
	TRAPD(getErr, aIapView->GetTextL(KCDTIdPortName, buf));
    if (getErr == KErrNone )
        {
        __ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCLnkP, 2));
    	SetPortName(*buf);
    	delete buf;        
        }
    else if (getErr != KErrNotFound )
    	{
    	User::Leave(getErr);
    	}	
    }
    
EXPORT_C CRawIpAgentConfig* CRawIpAgentConfig::NewLC(ESock::CCommsDatIapView* aIapView, const TPacketDataConfigBase* aGprsConfig)
	{
	CRawIpAgentConfig* self = new (ELeave) CRawIpAgentConfig(aGprsConfig);
	CleanupStack::PushL(self);
    TUint32 uintDBVar = 0;	
	aIapView->GetIntL(KCDTIdRate, uintDBVar);
	self->iConnectionSpeed = uintDBVar;
	return self;
	}	
    
EXPORT_START_ATTRIBUTE_TABLE_AND_FN(CBCAProvision, CBCAProvision::EUid, CBCAProvision::ETypeId)
// No attributes defined, as no serialisation takes place.
END_ATTRIBUTE_TABLE()

EXPORT_START_ATTRIBUTE_TABLE_AND_FN(CIPConfig, CIPConfig::EUid, CIPConfig::ETypeId)
// No attributes defined, as no serialisation takes place.
END_ATTRIBUTE_TABLE()

EXPORT_START_ATTRIBUTE_TABLE_AND_FN(CRawIpAgentConfig, CRawIpAgentConfig::EUid, CRawIpAgentConfig::ETypeId)
// No attributes defined, as no serialisation takes place.
END_ATTRIBUTE_TABLE()


