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
// The DHCPStateMachine header file
// 
//

/**
 @file DHCPStateMachine.h
*/

#ifndef DHCPSTATEMACHINE_H
#define DHCPSTATEMACHINE_H

#include <e32base.h>
#include <comms-infras/statemachine.h>

#include "MsgSender.h"
#include "DHCP_Std.h"

class CExpireTimer;
class CDHCPMessageHeader;
class CDHCPState;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
enum TSvrState
	{
	ESvrBinding,
	ESvrWaitForAnyDHCPMsgs,
	ESvrWaitForDiscoverInform,
	ESvrEnd
	};

enum TSvrSpecificState
	{
	ESvrDiscoverInProgress,
	ESvrInformInProgress,
	ESvrRenewInProgress,
	ESvrRebindInProgress,
	ESvrDeclineInProgress,
	ESvrReleaseInProgress,
	};
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	
class CDHCPStateMachine : public CStateMachine, public MMSListener
/**
  * Implements helper function abstractions
  *
  * @internalTechnology
  */
	{
friend class CDHCPControl;
friend class CDHCPDb;
friend class CDHCPAddressAcquisition;
friend class CDHCPSelect;
friend class CDHCPRebootConfirm;
friend class CDHCPInformationConfig;
friend class CDHCPRequest;
friend class CDHCPRenew;
friend class CDHCPRebind;
friend class CDHCPWaitForClientMsgs;

public:
   enum EAddressType
      {
      EAllAvailableServers,
      EUnicast
      };

	enum EInitialisationContext
		{
		EFirstCall,
		ESubsequentCalls
		};

public:
	~CDHCPStateMachine();

	void Cancel();

	virtual void GetServerAddress( TInetAddr& aAddress ) = 0;
  	virtual void SetCurrentAddress(const TInetAddr& aCurrentAddress, const TInetAddr& aSubnetMask) = 0;
	void SetCurrentAddress(const TInetAddr& aCurrentAddress);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	virtual void GetClientAddress(TInetAddr& aAddress);
	virtual void InitServerStateMachineL(MStateMachineNotify* aStateMachineNotify);
	virtual void InitServerBinding(MStateMachineNotify* aStateMachineNotify);	
	virtual void CloseNSendServerMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType);
	void CloseNSendSvrMsgL(TTimeIntervalSeconds aSecs, TInt aMaxRetryCount, CDHCPStateMachine::EAddressType aEAddressType);
	virtual CDHCPState* ReceiveOnPort67L( TRequestStatus* aStatus ) = 0;
	virtual void InitialiseServerSocketL() = 0;
   	virtual void CreateOfferMsgL() = 0;
   	virtual void HandleRequestMsgL() = 0;
   	virtual void HandleInformMsgL() = 0;   	
   	void SetClientIdentified(TBool aClientIdentified);
   	TBool IsClientIdentified();
   	void SetServerState(TBool aServerImpl);
   	void FetchServerAddressL();
	TBool CheckNetworkId();
	TInt BindServerInterface();
	TInetAddr GetInterfaceServerGlobalAddress();
	void SetDNSInformation(TDes8* aDNSInfo);
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	// returns unspecified address if no global address present
	TInetAddr GetInterfaceGlobalAddress();
	
	TBool DoesInterfaceKnowAnyDNSServers();

   //dhcpIPX tasks
   virtual void StartInitL(MStateMachineNotify* aStateMachineNotify, EInitialisationContext aInitialisationContext, TInt aUserTimeOut = 0 ) = 0;
   virtual void StartInformL(MStateMachineNotify* aStateMachineNotify, TBool aStaticAddress) = 0;
   virtual void StartRebootL(MStateMachineNotify* aStateMachineNotify) = 0;
   virtual void StartRenewL(MStateMachineNotify* aStateMachineNotify, TInt aUserTimeOut) = 0;
   virtual void StartDeclineL(MStateMachineNotify* aStateMachineNotify) = 0;
   virtual void StartReleaseL(MStateMachineNotify* aStateMachineNotify) = 0;
   virtual void StartRebindL(MStateMachineNotify* aStateMachineNotify) = 0;

   virtual void RemoveConfiguredAddress(const TInetAddr *aInetAddr = NULL ) = 0;
   void RemoveConfiguredAddress( const TSoInet6InterfaceInfo& aSoInet6InterfaceInfo );

   //interface used by CDHCPXXStates derivatives
	virtual void CloseNSendMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType);
   void CloseNSendMsgL(TTimeIntervalSeconds aSecs, TInt aMaxRetryCount, CDHCPStateMachine::EAddressType aEAddressType);
   virtual CDHCPState* ReceiveL( TRequestStatus* aStatus ) = 0;
   void CancelMessageSender();

   virtual void InitialiseSocketL() = 0;
   virtual void CreateDiscoverMsgL() = 0;                //discover IP4 solicit IP6
   virtual void CreateOfferAcceptanceRequestMsgL() = 0;  //request after offer IP4 advertise IP6
   virtual void CreateRebootRequestMsgL() = 0;           //reboot IP4 confirm IP6
   virtual void CreateInformMsgL() = 0;                  //no IP address is required (static address IP4 stateless neg. IP6)
   virtual void CreateDeclineMsgL() = 0;
   virtual void CreateRenewRequestMsgL() = 0;
   virtual void CreateRebindRequestMsgL() = 0;
   virtual void CreateReleaseMsgL() = 0;
   virtual void HandleOfferL() = 0;                      //after discover IP4 solicit IP6 or rebind(IP4/IP6)
   virtual CDHCPState* HandleReplyL( TRequestStatus* aStatus ) = 0;//after request
   TInt BindToSource();
   virtual void BindSocketForUnicastL() = 0;

   //interface used by both CDHCPXXStates derivatives & CDHCPXXControl
   void StartTimer(TTimeIntervalSeconds aSeconds, MExpireTimer& aExpireTimer);
   void StartTimer( TTimeIntervalMicroSeconds32 aMicroSeconds, MExpireTimer& aExpireTimer);
   void CancelTimer();
   TBool TimerActive() const;
   TBool IsGettingCfgInfoOnly() const;
   void SetAsyncCancelHandler(CDHCPState* aStateOwner);

   virtual TInt MSReportError(TInt aError);
   
	CDHCPMessageHeader* Message() const;
	const TName& InterfaceName() const
		{
		return iInterfaceName;
		}
	void SetIdle( TBool aIdle );
	TBool Idle();
	void SetFastTimeoutDuringInform();
	TBool FastTimeoutDuringInform();
	void SetCompleteClientRequestTrue();
	void SetCompleteClientRequestFalse();
	TBool CompleteClientRequest();
#ifdef SYMBIAN_DNS_PROXY	
	TInetAddr GetListenerAddress();
#endif // SYMBIAN_DNS_PROXY
	
protected:
   
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
   CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl);
#else
   CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#else   
   CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName);
#endif //  SYMBIAN_NETWORKING_DHCPSERVER
   void ConstructL();
   void Start(MStateMachineNotify* aStateMachineNotify);

   virtual void DoCancel();

   virtual void AssembleClientIDsL() = 0;
   void FetchHWAddress();

   void ConfigureInterfaceL( const TSoInet6InterfaceInfo& aInterfaceInfo );
   virtual void PrepareToSendL(CDHCPStateMachine::EAddressType aEAddressType) = 0;
#ifdef SYMBIAN_NETWORKING_DHCPSERVER   
   	virtual void PrepareToSendServerMsgL(CDHCPStateMachine::EAddressType aEAddressType) = 0;
	void AddScopeToClientAddrL(TInetAddr& addr);
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
   virtual void AssignAddresses( TInetAddr& aDest, const TInetAddr& aSrc ) const = 0;
   TUint32 GetNetworkIdL() const;
   void AddScopeToAddrL(TInetAddr& addr);

protected:
	const TName& iInterfaceName;
	RSocketServ& iEsock;
	RConnection& iConnection;
	CDHCPMessageHeader* iDhcpMessage;
	CMessageSender* iMessageSender;
	CExpireTimer* iTimer;

	TSockAddr iHardwareAddr;
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	TSockAddr iClientHWAddr;
	TInetAddr iInformClientAddr;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	CDhcpHwAddrManager* iDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
	
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	CDHCPState* iAsyncCancelHandler; //iAsyncCancelHandler is set by the DHCP State that assumes ownership of the RequestStatus object of CDHCPStateMachine 

public: 
   //accessed by CDHCPIP6MessageReader
	RSocket iSocket;
	TInetAddr iSocketAddr;
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	RSocket iSvrSocket;//For the Server Impl
	TInetAddr iSrvSocketAddr;
	TSvrState iSvrState;
	TSvrSpecificState iSvrSpecificState;
	HBufC8* 	iDNSInformation;
#endif // SYMBIAN_NETWORKING_DHCPSERVER	

#ifdef SYMBIAN_DNS_PROXY
	TBuf8<KMaxName>   iProxyHostName;
	TBuf8<KMaxName>   iProxyDomainName;
	TInetAddr   iProxyDnsSrvAddr;
	TInetAddr   iClientStaticAddr;
	TUint32     iOfferedAddress;
	TBuf8<KMaxName>     iClientHostName;
#endif // SYMBIAN_DNS_PROXY
		
protected:
	TDhcpRnd iXid;
	TBool iCfgInfoOnly; //if ETrue then:
							  //for IP4 indicates that a static address is used
							  //for IP6 if set during negotiation it indicates the same as for IP4
							  //			if set by CDHCPIP6Reconfigure states it indicates that the server wants us
							  //			to start inform-request rather than renew request
	TBool iMakeIdle;		  // used only for IP6, when RA recvd, with both 'M' and 'O' flags as false, this flag makes statemachine idle
	TBool iFastTimeout;
	TBool iCompleteClientRequest;	//used only for IP6, set to ETrue when 'O' flag in RA is set
   
	//data read from CommDb &&|| stored in persistent storage
	//the "public" should be replaced by some more inteligent way of dealing with persistent & init data
	//common for IP4 & IP6(maybe EPOC wide persistent storage?)
    //for IP6 it's the first address from the first IA option in the reply message
  	TInetAddr iCurrentAddress;
	TTime iTaskStartedAt;			// Time that task started. If sucessful it is copied to iStartedAquisitionAt
	TTime iStartedAquisitionAt;	// Time that lease period started

	TInetAddr iDefGateway;

	TBool iNameServerAddressesFromServer;
	TInetAddr iNameServer1;
	TInetAddr iNameServer2;

	//DNS client names
	HBufC8*   iHostName;
	HBufC8*   iDomainName;

	TUint32 iRenewalTimeT1;			// number of seconds after iStartedAquisitionAt when we send a renew request
	TUint32 iRebindTimeT2;			// number of seconds after iStartedAquisitionAt when we send a rebind request (only if any renew fails)
	TUint32 iLeaseTime;				// number of seconds after iStartedAquisitionAt when the lease expires

	TBool iReceiving;				//ETrue if we are waiting to receive data from socket

	CDHCPState* iFirstState; //accessed by CDHCPIP6ListenToNeighbor
	RBuf8 iClientId;  //accessed from CDhcpDb
	
	TInt iMaxRetryCount;	//IPv6 calculates its own retry values..so iMaxRetryCount is ignored
	TInt iRetryDhcpIpCount;	//number of retry for DHCP IP in case a DAD is detected.

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	TUint32 iYiaddr;
	TUint32 iCiaddr;
	TUint16 iFlag;							  
	TUint8 iMesgType;							  
	TBool iClientIdentified;
	TBool iServerImpl;
#endif // 	SYMBIAN_NETWORKING_DHCPSERVER	
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
public:	
	TBool iDhcpInformAckPending;
	RBuf8 iSavedExtraParameters;
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS	
	};
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
inline CDHCPStateMachine::CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl):
                     iInterfaceName(aInterfaceName),
                     iEsock(aEsock),
                     iConnection(aConnection),
                     iCfgInfoOnly( EFalse ),iMaxRetryCount(KInfinity),
					 iMakeIdle (EFalse),
					 iFastTimeout (EFalse),
					 iServerImpl(aDHCPServerImpl),
					 iRetryDhcpIpCount(0)
#else
inline CDHCPStateMachine::CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl):
                     iInterfaceName(aInterfaceName),
                     iEsock(aEsock),
                     iConnection(aConnection),
                     iCfgInfoOnly( EFalse ),iMaxRetryCount(KInfinity),
					 iMakeIdle (EFalse),
					 iFastTimeout (EFalse),
					 iServerImpl(aDHCPServerImpl),
					 iRetryDhcpIpCount(0),
					 iDhcpHwAddrManager(aDhcpHwAddrManager)
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
/**
  * Constructor of the DHCPStateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#else 

inline CDHCPStateMachine::CDHCPStateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName):
	iInterfaceName(aInterfaceName),
	iEsock(aEsock),
	iConnection(aConnection),
	iCfgInfoOnly( EFalse ),iMaxRetryCount(KInfinity),
	iMessageSender(NULL),
	iMakeIdle(EFalse),
	iFastTimeout(EFalse),
	iRetryDhcpIpCount(0)
/**
  * Constructor of the DHCPStateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER
inline CDHCPMessageHeader* CDHCPStateMachine::Message() const
	{
	return iDhcpMessage;
	}
	
inline void CDHCPStateMachine::CancelMessageSender()
   {
	if (iMessageSender != NULL)
		iMessageSender->Cancel();
   }

inline TBool CDHCPStateMachine::TimerActive() const
	{
	return iTimer != NULL ? iTimer->IsActive() : EFalse;
	}

inline void CDHCPStateMachine::CancelTimer()
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPStateMachine::CancelTimer")));

	if (iTimer != NULL)
		iTimer->Cancel();
	}

inline TBool CDHCPStateMachine::IsGettingCfgInfoOnly() const
	{
	return iCfgInfoOnly;
	}

inline void CDHCPStateMachine::SetAsyncCancelHandler(CDHCPState* aStateOwner) 
	{	
	iAsyncCancelHandler =  aStateOwner;
	}	

inline void CDHCPStateMachine::SetCurrentAddress(const TInetAddr& aCurrentAddress)
/**
  * The SetCurrentAddress function
  *
  * Stores ip address
  *
  * @internalTechnology
  */
	{
	iCurrentAddress = aCurrentAddress;
	}

inline void CDHCPStateMachine::SetIdle ( TBool aIdle )
	{
	iMakeIdle = aIdle;
	}

inline TBool CDHCPStateMachine::Idle ()
	{
	return iMakeIdle;
	}

inline void CDHCPStateMachine::SetFastTimeoutDuringInform()
	{
	iFastTimeout = ETrue;
	}

inline TBool CDHCPStateMachine::FastTimeoutDuringInform()
	{
	return iFastTimeout;
	}


inline void CDHCPStateMachine::SetCompleteClientRequestTrue()
	{
	iCompleteClientRequest = ETrue;
	}

inline void CDHCPStateMachine::SetCompleteClientRequestFalse()
	{
	iCompleteClientRequest = EFalse;
	}

inline TBool CDHCPStateMachine::CompleteClientRequest()
	{
	return iCompleteClientRequest;
	}


#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
inline void CDHCPStateMachine::SetClientIdentified(TBool aClientIdentified)
	{
	iClientIdentified = aClientIdentified;
	}

inline TBool CDHCPStateMachine::IsClientIdentified()
	{
	return iClientIdentified;
	}

#endif 	// SYMBIAN_NETWORKING_DHCPSERVER	

#endif

