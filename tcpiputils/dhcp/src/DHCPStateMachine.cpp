// Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the DHCP statemachine helper functions
// 
//

/**
 @file DHCPStateMachine.cpp
 @internalTechnology
*/

#include "DHCPStates.h"
#include "DHCPStatesDebug.h"
#include "DHCPControl.h"
#include "ExpireTimer.h"
#include "DHCPMsg.h"
#include <e32math.h>
#include "DHCPServer.h"
#include "in6_opt.h"
#include "es_sock.h"

CDHCPStateMachine::~CDHCPStateMachine()
/**
  * Destructor of the If base class
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::~CDHCPStateMachine")));
	Cancel();
	UnloadConfigurationFile();
	delete iDhcpMessage;
	delete iMessageSender;
	delete iTimer;
	delete iHostName;
	delete iDomainName;
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	iSavedExtraParameters.Close();
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
	iClientId.Close();
	iSocket.Close();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	iSvrSocket.Close();
	delete iDNSInformation;
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	}

void CDHCPStateMachine::ConstructL()
/**
  * Creates socket and connections for the object
  *
  *
  *	@internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::ConstructL")));
	LoadConfigurationFile();
#ifdef _DEBUG
	// let's set debug properties to something
	//  so they can be read immediately..
	CDHCPStateMachine* const & iStateMachine = this;
	CDHCPStateMachine* const & iDhcpStateMachine = iStateMachine;
	DHCP_DEBUG_PUBLISH_READY(DHCPDebug::ENotReady);
	DHCP_DEBUG_PUBLISH_STATE(DHCPDebug::EStateUnknown);
#endif

	iTimer = CExpireTimer::NewL();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	if(!iServerImpl) // Assemble client Ids only for DHCP client implementation
		{
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	InitialiseSocketL();
	AssembleClientIDsL();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
		}
	else
		InitialiseServerSocketL();
#endif // SYMBIAN_NETWORKING_DHCPSERVER			
	}

void CDHCPStateMachine::Start(MStateMachineNotify* aStateMachineNotify)
/**
  * The Start function
  *
  * Starts the statemachine task
  *
  * @internalTechnology
  */
{
   SetLastError( KErrNone );
   iReceiving = EFalse;
   iHistory = 0;
	SetActiveEvent(iFirstState);
	if ( !aStateMachineNotify )
		{//no notifier specified => leave the current one unchanged
		aStateMachineNotify = iStateMachineNotify;
		}
	CStateMachine::Start(NULL, NULL, aStateMachineNotify);
}

void CDHCPStateMachine::CloseNSendMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType)
/**
  * Handles sending of packets for this object
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::CloseNSendMsgL")));
   	PrepareToSendL(aEAddressType);
  	iMessageSender->Cancel();
	
	GetServerAddress(iSocketAddr);
	AddScopeToAddrL(iSocketAddr);

#ifdef _DEBUG
	THostName addrDes;
	iSocketAddr.Output(addrDes);	
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		_L("CDHCPStateMachine::CloseNSendMsgL - Sending message to %S%%%d"), &addrDes, iSocketAddr.Scope()));
#endif

	iSocket.SendTo(iDhcpMessage->Message(), iSocketAddr, 0, aStatus);
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void CDHCPStateMachine::CloseNSendServerMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType)
/**
  * Handles sending of packets for this object
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::CloseNSendMsgL")));


   	PrepareToSendServerMsgL(aEAddressType);
  	iMessageSender->Cancel();

	GetClientAddress(iSrvSocketAddr);
	AddScopeToClientAddrL(iSrvSocketAddr);

#ifdef _DEBUG
	THostName addrDes;
	iSrvSocketAddr.Output(addrDes);	
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		_L("CDHCPStateMachine::CloseNSendServerMsgL - Sending message to %S%%%d"), &addrDes, iSrvSocketAddr.Scope()));
#endif

	iSvrSocket.SendTo(iDhcpMessage->Message(), iSrvSocketAddr, 0, aStatus);
	}
	
void CDHCPStateMachine::GetClientAddress( TInetAddr& /*aAddress*/ )
/**
  * GetClientAddress
  *
  * Null implementation
  *
  * @internalTechnology	
*/
	{
	}	
	
#ifdef SYMBIAN_DNS_PROXY	
TInetAddr CDHCPStateMachine::GetListenerAddress()
	{
	return iCurrentAddress;
	}
#endif
		
void CDHCPStateMachine::InitServerStateMachineL(MStateMachineNotify* /*aStateMachineNotify*/)
/**
  * InitServerStateMachineL
  *
  * Null implementation
  *
  * @internalTechnology
  */
	{
	}	
	
void CDHCPStateMachine::InitServerBinding(MStateMachineNotify* /*aStateMachineNotify*/)		
	{
	
	}
	
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPStateMachine::CloseNSendMsgL(TTimeIntervalSeconds aSecs, TInt aMaxRetryCount, CDHCPStateMachine::EAddressType aEAddressType)
/**
  * Handles sending of packets for this object
  *
  * @internalTechnology
  *
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::CloseNSendMsgL")));
   	PrepareToSendL(aEAddressType);
	iMessageSender->Cancel();
	
	TInetAddr addr;		
	GetServerAddress( addr );
	AddScopeToAddrL(addr);
	
#ifdef _DEBUG
	THostName addrDes;
	addr.Output(addrDes);	
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		_L("CDHCPStateMachine::CloseNSendMsgL - Sending message to %S%%%d"), &addrDes, addr.Scope()));
#endif

	if (FastTimeoutDuringInform())	// true only when InformNegotiationIsRequiredForConnectionStartCompletion is ETrue
		{
		iMessageSender->SendL(addr, iDhcpMessage->Message(), (aSecs.Int() * KMicrosecondsInSecs) / 14, aMaxRetryCount);
		}
	else
		{
		iMessageSender->SendL(addr, iDhcpMessage->Message(), aSecs.Int() * KMicrosecondsInSecs, aMaxRetryCount);	
		}
	}

TInt CDHCPStateMachine::MSReportError(TInt aError)
/**
  * Report an error properly
  *
  * @internalTechnology
  */
	{
	Cancel();

	SetLastError(aError);
	OnCompletion();

	return aError;
	}

void CDHCPStateMachine::Cancel()
/**
  * Cancel the state machine's activities
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::Cancel")));
	CancelMessageSender();
	iSocket.Close();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	iSvrSocket.Close();
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	CancelTimer();
	CStateMachine::Cancel(KErrNone);
	delete iFirstState;
	iFirstState = NULL;
	iFastTimeout = EFalse;	// reset iFastTimeout
	iMaxRetryCount = KInfinity;//reset
	}

void CDHCPStateMachine::DoCancel()
/**
  * Implements a default docancel for the connection object
  *
  * @internalTechnology
  */
	{
	// we have to cancel send and recv independently as cancelAll()
	// doesn't satisfy us, only supporting read, write, ioctl, connect,
	// accept and shutdown...:-(
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::DoCancel")));
	if (iSocket.SubSessionHandle())
		{
		// check that the socket is open
		// and if it is then we need to cancel things on it
		iSocket.CancelRecv();
		}
#ifdef SYMBIAN_NETWORKING_DHCPSERVER		
	if(iSvrSocket.SubSessionHandle())
		{
		iSvrSocket.CancelRecv();
		}
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
	/*	iAsyncCancelHandler is set by the DHCP State that assumes ownership of the 
		RequestStatus object of CDHCPStateMachine 				 
	*/
	 if(iAsyncCancelHandler)
		{
		iAsyncCancelHandler->Cancel();
		}
	}
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void CDHCPStateMachine::FetchServerAddressL()
/**
  * This function is used to get the server's address (interface address)
  *
  */
	{
	if(!iSvrSocket.SubSessionHandle())
		InitialiseServerSocketL();
	
	TPckgBuf<TSoInetInterfaceInfo> opt;
	while (iSvrSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == iInterfaceName)
			{
			iCurrentAddress = opt().iAddress;
			break;
			}
		}
   }
   
void CDHCPStateMachine::SetDNSInformation(TDes8* aDNSInfo)	
	{
	delete iDNSInformation;
	iDNSInformation = NULL;
	
	iDNSInformation = HBufC8::NewL(aDNSInfo->Length());
	iDNSInformation->Des() = *aDNSInfo;
	}   
	
TBool CDHCPStateMachine::CheckNetworkId()
/**
  * This function compares the NetworkIds of the client and server
  *
  */	
	{
	iInformClientAddr.SetV4MappedAddress(iCiaddr);

	if ((iInformClientAddr.Address() & KInetAddrNetMaskC) == 
			(iCurrentAddress.Address() & KInetAddrNetMaskC))		
		{
		return ETrue;		
		}
	else
		{
		return EFalse;
		}	
	}   
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPStateMachine::FetchHWAddress()
/**
  * Fetches hardware address from the interface
  *
  */
	{
	TPckgBuf<TSoInetInterfaceInfo> opt;
	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == iInterfaceName)
			{
			iHardwareAddr = opt().iHwAddr;
			if(iHardwareAddr.Length() <= KHwAddrOffset)
				{
				// the hardware address came back too short
   		 		//  to be a valid TSockAddr.. so we'll treat it as an empty value
				iHardwareAddr.SetFamily(KAFUnspec);
				iHardwareAddr.SetLength(KHwAddrOffset);
				}
			break;
			}
		}
   }


void CDHCPStateMachine::StartTimer(TTimeIntervalSeconds aSeconds, MExpireTimer& aExpireTimer)
/**
  * Give the tcp/ip6 stack time to perform
  * its gratuitous ARP...
  *
  * @internalTechnology
  */
	{
	CancelTimer();
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::StartTimerL")));

	iTimer->After(aSeconds, aExpireTimer);
	}

void CDHCPStateMachine::StartTimer( TTimeIntervalMicroSeconds32 aMicroSeconds, MExpireTimer& aExpireTimer)
/**
  * Give the tcp/ip6 stack time to perform
  * its gratuitous ARP...
  *
  * @internalTechnology
  */
	{
	CancelTimer();
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::StartTimerL")));

	iTimer->After(aMicroSeconds, aExpireTimer);
	}

void CDHCPStateMachine::RemoveConfiguredAddress( const TSoInet6InterfaceInfo& aSoInet6InterfaceInfo )
/**
  * This function can be called as a result of DAD failing
  * or the lease expiring! It removes the address from the interface
  * inside the TCP/IP6 stack as the address cannot continue to be used.
  *
  * @see "Implementation of IPv4/IPv6 Basic Socket API for Symbian OS"
  * document for explanation of TSoInet6InterfaceInfo and its use
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::RemoveConfiguredAddress")));
	
	TPckgBuf<TSoInet6InterfaceInfo> configInfo(aSoInet6InterfaceInfo);
	// not interested in error
	// how could we attempt to handle it anyway?...keep trying??? i think not...
	// ensure that we have a socket to write down	
	iSocket.Close();	
	TInt error = iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	
    if(error == KErrNone)
		{
		error = iSocket.SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, configInfo);
		if(error == KErrNone)
		    {	        
		    // make socket invisible for interface counting
		    error = iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);
		    if(error != KErrNone)
		        {
		        __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("CDHCPStateMachine::RemoveConfiguredAddress, SetOpt Failed to set KSolInetIp")));   
		        }
		    }
		else
		    {
		    __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("CDHCPStateMachine::RemoveConfiguredAddress,SetOpt Failed to set KsolInetIfCtrl")));   
		    }
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("CDHCPStateMachine::RemoveConfiguredAddress,Socket Open Failed: Due to %d"),error));	
		}
	}

void CDHCPStateMachine::ConfigureInterfaceL( const TSoInetInterfaceInfoExtnDnsSuffix& aInterfaceInfo )
/**
  * Set the interface IP address and other params
  * into the TCP/IP6 stack.
  *
  * What we set depends on the setup
  * in commDB for the service.  If ipAddressFromServer
  * is true then we set the ip address that has
  * been assigned by DHCP, along with the netmask and gateway.
  * If ipAddressFromServer is false, then we set the static ip
  * address as long as it has been okayed by the DHCP server after
  * we have sent an inform.  We will then set the netmask and gateway
  * choosing from those in commDB if they have been given values, or 
  * those returned in the DHCP Server ACK if a zero address is in commDB
  * The same principle applies to DNS Server addresses.
  *
  * @internalTechnology
  */
	{
	
	TPckgBuf<TSoInetInterfaceInfoExtnDnsSuffix> configInfo(aInterfaceInfo);
	

	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::ConfigureInterfaceL - KSoInetConfigInterface")));
	
	User::LeaveIfError(iSocket.SetOpt(KSoInetConfigInterface, KSolInetIfCtrl, configInfo));
	}

TInt CDHCPStateMachine::BindToSource()
/**
  * Binds socket to newly assigned address for the interface
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::BindToSourceL")));
	iSocket.Close();	// destroy the old socket
	
	UpdateHistory(CDHCPState::EBindToSource);
	// now start a new one.
	// might be nice if we left here if the socket open fails...but nobody's perfect...
   // PS: cannot leave here the failure doesn't mean exception here see the usage....
	TInt err = iSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	// make socket invisable for interface counting
	if (err == KErrNone)
		{
		err = iSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);
		}
		
    if (err == KErrNone)       //PDEF122482: Enabling the ReUseAddr option.
	    {
	    err = iSocket.SetOpt(KSoReuseAddr, KSolInetIp, 1);
	    }			
	
	if (err == KErrNone)
		{
        TInetAddr bindTo;        
        AssignAddresses( bindTo, iCurrentAddress ); 

        err = iSocket.Bind(bindTo);
        if (err==KErrNone)
	        {
	        // we are finished with this socket, 
	        // release it so as not to hold esock open
	        iSocket.Close();
	        }
		}
	return err;
	}


TUint32 CDHCPStateMachine::GetNetworkIdL() const
	{	
	TUint32 networkId;
	_LIT(KIapNetwork, "IAP\\IAPNetwork");
	User::LeaveIfError(iConnection.GetIntSetting(KIapNetwork, networkId));
	
	return networkId;
	}

void CDHCPStateMachine::AddScopeToAddrL(TInetAddr& addr)
	{
	TPckgBuf<TSoInetIfQuery> queryBuf;
	queryBuf().iName = iInterfaceName;		
	User::LeaveIfError(iSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, queryBuf));
	
	const TUint s = addr.Ip6Address().Scope() - 1;

	if (s < 16)
		{
		addr.SetScope(queryBuf().iZone[s]);
		}
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void CDHCPStateMachine::AddScopeToClientAddrL(TInetAddr& addr)
	{
	TPckgBuf<TSoInetIfQuery> queryBuf;
	queryBuf().iName = iInterfaceName;		
	User::LeaveIfError(iSvrSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, queryBuf));
	
	const TUint s = addr.Ip6Address().Scope() - 1;

	if (s < 16)
		{
		addr.SetScope(queryBuf().iZone[s]);
		}
	}
// Set when the DHCP server implementation is being used	
void CDHCPStateMachine::SetServerState(TBool aServerImpl)	
	{
	iServerImpl = aServerImpl;
	}
	
TInt CDHCPStateMachine::BindServerInterface()
/**
  * Binds socket to newly assigned address for the interface
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::BindServerInterface")));
	iSvrSocket.Close();	// destroy the old socket
	
	// now start a new one.
	TInt err = iSvrSocket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp, iConnection);
	// make socket invisable for interface counting
	if (err == KErrNone)
		{
		TInt err = iSvrSocket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);
		}

	if (err == KErrNone)
		{
        TInetAddr bindTo;        
        AssignAddresses( bindTo, iCurrentAddress );
        err = iSvrSocket.Bind(bindTo);
		}
	return err;
	}

	
TInetAddr CDHCPStateMachine::GetInterfaceServerGlobalAddress()
/**
  * Are any of the addresses on the interface global addresses?
  *  If so, DHCP might decide not to attempt to discover an address.
  *
  * Returns unspecified address if no global address present.
  */
	{
	TPckgBuf<TSoInetInterfaceInfo> opt;
	if (iSvrSocket.SubSessionHandle() == 0)
		{
		InitialiseServerSocketL();
		}
	iSvrSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	__CFLOG_STMT(TBuf<512> addrStr;);
	while (iSvrSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == iInterfaceName)
			{
			TInetAddr& addr = opt().iAddress;
			__CFLOG_STMT(addr.Output(addrStr););
			if ( ! addr.IsLinkLocal())
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("Global address %S found on interface %S"),&addrStr,&iInterfaceName));
				return addr;
				}
			else
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("Linklocal address %S found on interface %S"),&addrStr,&iInterfaceName));
				}
			break;
			}
		}
	return TInetAddr(); // unspecified
	}



		
#endif // SYMBIAN_NETWORKING_DHCPSERVER
	
TInetAddr CDHCPStateMachine::GetInterfaceGlobalAddress()
/**
  * Are any of the addresses on the interface global addresses?
  *  If so, DHCP might decide not to attempt to discover an address.
  *
  * Returns unspecified address if no global address present.
  */
	{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	if(iServerImpl)
		{
		return GetInterfaceServerGlobalAddress();
		}
	else
		{
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	
	TPckgBuf<TSoInetInterfaceInfo> opt;
	if (iSocket.SubSessionHandle() == 0)
		{
		InitialiseSocketL();
		}
	iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	__CFLOG_STMT(TBuf<512> addrStr;);
	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == iInterfaceName)
			{
			TInetAddr& addr = opt().iAddress;
			__CFLOG_STMT(addr.Output(addrStr););
			if ( ! addr.IsLinkLocal())
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("Global address %S found on interface %S"),&addrStr,&iInterfaceName));
				return addr;
				}
			else
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("Linklocal address %S found on interface %S"),&addrStr,&iInterfaceName));
				}
			break;
			}
		}
	return TInetAddr(); // unspecified
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
		}
#endif // SYMBIAN_NETWORKING_DHCPSERVER				
	}


TBool CDHCPStateMachine::DoesInterfaceKnowAnyDNSServers()
/**
  * Does the interface know of any DNS servers?
  */
	{
	TPckgBuf<TSoInetInterfaceInfo> opt;
	if (iSocket.SubSessionHandle() == 0)
		{
		InitialiseSocketL();
		}
	iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl);
	__CFLOG_STMT(TBuf<512> addrStr;);
	while (iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
		{
		if (opt().iName == iInterfaceName)
			{
			TInetAddr& addr = opt().iNameSer1;
			__CFLOG_STMT(addr.Output(addrStr););
			if ( ! addr.IsUnspecified() )
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("DNS server %S found on interface %S"),&addrStr,&iInterfaceName));
				return ETrue;
				}
			else
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L("No DNS servers already set on interface %S (prior to DHCP)"),&iInterfaceName));
				}
			break;
			}
		}
	return EFalse;
	}


/** 
  * @name LoadConfigurationFile
  *			Opens up the dhcp.ini file and reads the content on to a heap buffer
  * @return - ETrue if the dhcp.ini is successfully parsed to the buffer, EFalse otherwise
  *
  * @internalTechnology
  */
TBool CDHCPStateMachine::LoadConfigurationFile()
    {
    TRAP_IGNORE(iConfig = CESockIniData::NewL(DHCP_INI_DATA));
    return (iConfig != NULL);
    }

/**
  * @name UnloadConfigurationFile 
  *			Frees up memory allocated for reading dhcp.ini file
  *
  * @internalTechnology
  */
void CDHCPStateMachine::UnloadConfigurationFile()
    {
    if (iConfig)
        {
        delete iConfig;
        iConfig = NULL;
        }
    }

/**
 * @name IniRead
 *          General ini file read utility. Makes use of CESockIniData object for parsing
 *          
 * @param   aOptionName   Key name within ini file whose value need to be parsed
 * @param   aOptionValue    Buffer reference to store the parsed output
 * 
 * @return  TInt error values as approriate from the ini parsing framework
 *          
 * @internalTechnology
 */
TInt CDHCPStateMachine::IniRead(const TDesC& aOptionName, TDes8& aOptionValue)
    {
    TPtrC iniValue;
    TBool iniParsed = iConfig->FindVar(KDhcpSection, aOptionName, iniValue);
    if (!iniParsed)
        return KErrNotFound;

    if(aOptionName.CompareF(KDhcpExtraOptions) == KErrNone)
        {
        TLex iniLex(iniValue);
        TChar ch;
        
        while((ch = iniLex.Get()) != 0)
            {
            while ((ch = iniLex.Peek()) != ',')
                iniLex.Inc();
            TLex token(iniLex.MarkedToken());
            
            TUint8 opCode(0);
            token.Val(opCode,EDecimal);

            if ( (opCode > 0) && (opCode < KOpCodeOutOfBounds) )
                aOptionValue.Append(opCode);
            
            iniLex.Inc();
            iniLex.Mark();
            }
        return KErrNone;
        }
    else
        return KErrNotFound;
    }

TDhcpRnd::TDhcpRnd():iXid(0)
/**
  * Constructor for this little random number 
  * class that creates us a random transaction id
  *
  * @internalTechnology
  */
	{
	TTime now;
	now.HomeTime();
	iSeed = now.Int64();
	}

TInt TDhcpRnd::Rnd(TInt aMin, TInt aMax)
/**
  * Utility class function to generated a real
  * random number
  *
  * @internalTechnology
  */
	{
	return Math::Rand(iSeed)%(aMax-aMin+1)+aMin;
	}
