/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
*
*/



/**
 @file te_dhcpTestServer.h
*/
#if (!defined __DHCPTEST_SERVER_H__)
#define __DHCPTEST_SERVER_H__
#include <test/testexecuteserverbase.h>

#include <es_sock.h>
#include <es_prot.h>
#include <cdbover.h>
#include <commdbconnpref.h>
#include <comms-infras/netcfgextndhcpmsg.h>
//#include "DHCPIP4Msg.h"

class CDhcpTestServer;
namespace CommsDat
	{
	class CCDIAPRecord;
	}

/**
A base class for all DHCP test step classes
*/
class CDhcpTestStepBase : public CTestStep
    {
protected:
    
    TVerdict	doTestStepPreambleL();
    
public:
	void SetTestServer(CDhcpTestServer*);

protected:
	enum TCurAddrType
		{
		KCurAddrNone = 0,
		KCurAddrGlobal = 1,
		KCurAddrIPv4LinkLocal = 2,
		};

protected:
	void Set_UsingIPv6L(TBool);
	void SetIAPToUseL(TInt);
	void MakeIAPPrivateL(TInt);

    TBool   UsingIPv6L(void) const;
    TInt    IAPToUseL(void);
	void	InitialServiceLinkL( CMDBSession* aDbSession, CCDIAPRecord* aIapRecord );
	TBool	IsIAPAddrStaticL();
	TUint	GetCurrentAddressTypesL( RSocket &socket );
    
    TUint   IpAddressFamilyL(void);
    TInt    DhcpGetRawOptionDataL(RConnection &aConn, TDes8& aBufDes, TUint aOptCode);
    void	GetDebugHandleL(RConnection &aConn);
    
    void    ImmediateCompletionTestL(RConnection &aConn);

	TUint   WAIT_FOR_STATE_CHANGE_WITH_TIMEOUTL( TUint aTimeout );
	void    GetProvisioningMacL(TDesC16& aSectioName, const TDesC16& aKeyName, TDes8& aHwAddress);
private:
    CDhcpTestServer * iTestServer;
    
protected:
    TCommDbConnPref iConnPrefs;
    TInt iDebugHandle;
    };

#define LOG_STATEL					{QUERY_STATEL(state);const TText* ss = DHCPDebug::State_Name[state];INFO_PRINTF3(_L(" .. DHCP is in state %d (%s)"),state,ss);}
#define QUERY_STATEL(x)   			DHCP_DEBUG_QUERYL(DHCPDebug::EState + iDebugHandle, x)
#define QUERY_READYL(x)   			DHCP_DEBUG_QUERYL(DHCPDebug::EReadiness + iDebugHandle, x)
#define WAIT_FOR_STATE_CHANGEL(x)	{ \
	DHCPDebug::State state; \
	TRequestStatus propStatus; \
	 \
	while( true ) \
		{ \
		QUERY_STATEL( state ); \
		if( state == x ) \
			{ \
			break; \
			} \
		 \
		DHCP_DEBUG_SUBSCRIBEL(DHCPDebug::EState + iDebugHandle); \
		} \
	}
#define WAIT_FOR_EITHER_STATE_CHANGEL(x, y)	{ \
	DHCPDebug::State state; \
	TRequestStatus propStatus; \
	 \
	while( true ) \
		{ \
		QUERY_STATEL( state ); \
		if( ( state == x ) || ( state == y ) ) \
			{ \
			break; \
			} \
		 \
		DHCP_DEBUG_SUBSCRIBEL(DHCPDebug::EState + iDebugHandle); \
		} \
	}

_LIT( TCPIP_INI_PATH, "c:\\private\\101f7989\\esock\\tcpip.ini" );
_LIT8( TCPIP_INI_IP, "[ip]\r\n" );
_LIT8( TCPIP_INI_IPV4LINKLOCAL, "ipv4linklocal= " );


/**
* A clever little macro to declare the test steps...
*/
#define DECLARE_DHCP_TEST_STEP(nr) \
	_LIT(KDhcpTestStep##nr,#nr); \
class CDhcpTestStep##nr : public CDhcpTestStepBase \
   { \
public: \
	CDhcpTestStep##nr() \
      { \
	  SetTestStepName(KDhcpTestStep##nr); \
      }; \
	  virtual TVerdict doTestStepL(); \
	}

class CDhcpTestServer : public CTestServer
	{
public:
	IMPORT_C TInt NewServer();
	static CDhcpTestServer* NewL();
	virtual CTestStep* CreateTestStep(const TDesC& aStepName);
	
	TBool iUsingIPv6;
	TInt iIAPToUse;
	
private:
	const TPtrC ServerName() const;	
	};

#endif
