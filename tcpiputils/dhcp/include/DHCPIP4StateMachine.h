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
// The DHCPIP4StateMachine header file
// 
//

/**
 @file DHCPIP4StateMachine.h
*/

#ifndef __DHCPIP4STATEMACHINE_H__
#define __DHCPIP4STATEMACHINE_H__

#include "DHCPStateMachine.h"
#include "DHCPIP4Msg.h"

class CDHCPIP4State;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDHCPIP4StateMachine : public CDHCPStateMachine
/**
  * Implements helper function & starts DHCPv4 tasks (INIT,INFORM,....)
  *
  * @internalTechnology
  */
	{
friend class CDHCPIP4Control;
friend class CDHCPIP4Select;
friend class CDHCPIP4HandleClientMsgs;
friend class CDHCPIP4SendRequestResponse;
friend class CDHCPIP4SendInformResponse;

public:
   /* tasks the statemachine could be started for
   enum ETask
      {
      EInit,
      EInform,
      EReboot,
      ERenew,
      ERebind,
      EDecline,
      ERelease
      };
   */

public:
	~CDHCPIP4StateMachine();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER  		
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	static CDHCPIP4StateMachine* NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl);	
#else
	static CDHCPIP4StateMachine* NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#else 	
	static CDHCPIP4StateMachine* NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName);
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	virtual void GetServerAddress( TInetAddr& aAddress );
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	void StartInformL(MStateMachineNotify* aStateMachineNotify);
#endif //SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
  	virtual void SetCurrentAddress(const TInetAddr& aCurrentAddress, const TInetAddr& aSubnetMask);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER  	
	virtual void GetClientAddress(TInetAddr& aAddress);
	virtual CDHCPState* ReceiveOnPort67L( TRequestStatus* aStatus );
	virtual void InitialiseServerSocketL();//For the Server Impl
	virtual void CreateOfferMsgL();
   	virtual void HandleRequestMsgL();
   	virtual void HandleInformMsgL();   	
	virtual void InitServerStateMachineL(MStateMachineNotify* aStateMachineNotify);	
	virtual void InitServerBinding(MStateMachineNotify* aStateMachineNotify);	
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
   virtual CDHCPState* ReceiveL( TRequestStatus* aStatus );
   
   //interface used by CDHCPIP4Control
	virtual void StartInitL(MStateMachineNotify* aStateMachineNotify, EInitialisationContext aInitialisationContext, TInt aUserTimeOut = 0);
	virtual void StartInformL(MStateMachineNotify* aStateMachineNotify, TBool aStaticAddress);
	virtual void StartRebootL(MStateMachineNotify* aStateMachineNotify);
	virtual void StartRenewL(MStateMachineNotify* aStateMachineNotify, TInt aUserTimeOut);
	virtual void StartRebindL(MStateMachineNotify* aStateMachineNotify);
	virtual void StartDeclineL(MStateMachineNotify* aStateMachineNotify);
	virtual void StartReleaseL(MStateMachineNotify* aStateMachineNotify);

	virtual void RemoveConfiguredAddress(const TInetAddr *aInetAddr = NULL);

	//interface used by CDHCPIP4States derivatives
   void CreateMessageL();
   void CreateCommonMsgStart();
   void CreateCommonMsgEndL();
   virtual void InitialiseSocketL();
   virtual void CreateDiscoverMsgL();
   virtual void CreateOfferAcceptanceRequestMsgL();
   virtual void CreateRebootRequestMsgL();
   virtual void CreateInformMsgL();
   virtual void CreateDeclineMsgL();
   virtual void CreateRenewRequestMsgL();
   virtual void CreateRebindRequestMsgL();
   virtual void CreateReleaseMsgL();
   TInt CreateIPv4LinkLocal();
   virtual void HandleOfferL();
   virtual CDHCPState* HandleReplyL( TRequestStatus* aStatus );
  
   virtual void BindSocketForUnicastL();

   TUint8 GetMessageTypeL() const;
	TBool CheckXid() const;

	//interface used by both CDHCPIP4States derivatives & CDHCPIP4Control
	TBool IsUsingStaticAddress() const;
#ifdef _DEBUG
	static TInt GetDestPort();
#endif

#ifdef SYMBIAN_DNS_PROXY
	void ReadDhcpMsgParamsL();
#endif // SYMBIAN_DNS_PROXY	

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	void CheckClientMsgL();	
	void HandleRequest();
	void HandleInformL();
	void ProcessDiscoverL();
	void ProcessRequestL();
	void ProcessInformL();
	TUint8 GetClientMessageTypeL() const;
	TUint32 GetIPAddressToOffer();
	TUint32 GetClientIPAddress();
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION	
	TBool ClientHwAddrProvisioned();
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
protected:
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl);
#else
	CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl);
#endif
#else 
	CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName);
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
	
	void ConstructL();

	void ConfigureInterfaceL();
	virtual void PrepareToSendL(CDHCPStateMachine::EAddressType aEAddressType);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	virtual void PrepareToSendServerMsgL(CDHCPStateMachine::EAddressType aEAddressType);
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
   	virtual void AssembleClientIDsL();
	virtual void AssignAddresses( TInetAddr& aDest, const TInetAddr& aSrc ) const;

protected:
	DHCPv4::CDHCPMessageHeaderIP4* DhcpMessage() const;
	void SetMessageHeaderL( DHCPv4::TDHCPv4MessageType aMsgType );
#ifdef 	SYMBIAN_NETWORKING_DHCPSERVER
	void SetMessageHeaderAsServerL( DHCPv4::TDHCPv4MessageType aMsgType );
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	void HandleAckL();
	void CreateFqdnUpdateRequestL();
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
private:
	void ProcessDeclineL();
	TUint32 GenerateClientIPAddress();
    void AddParamRequestOptionL(TUint8 aParamReqValue);
	void CheckClientParamListL();
	void AddMessageOptionsL();
#endif // SYMBIAN_NETWORKING_DHCPSERVER   
private:
   //accessed by CDHCPIP4Control & CDHCPIP6MessageReader
	TInetAddr iDHCPServerAddr; //server address currently used to send/receive
	TUint32 iServerAddress;	// to store the address of the DHCP Server we are dealing with
#ifdef 	SYMBIAN_NETWORKING_DHCPSERVER
	TInetAddr iDHCPClientAddr; //client address 
	TBool iBroadCastFlag;
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
	TPtr8 iIncomingMsgDataPtr;
	TInetAddr iDHCPServerID;
	TInetAddr iBroadcastAddress;
	TInetAddr iSubnetMask;
private:
    void AppendMultipleExtraOptionsParamL(); // Retrieve extra options for dhcp server request from dhcp.ini
    void SplitDomainSearchBufferL(HBufC8* aDomainSearchBuf); // Decryption of option data for dhcp option 119
	};
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
inline CDHCPIP4StateMachine::CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName,TBool aDHCPServerImpl):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName ,aDHCPServerImpl),
                     iIncomingMsgDataPtr( 0, 0 )
#else
inline CDHCPIP4StateMachine::CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager, TBool aDHCPServerImpl):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName , aDhcpHwAddrManager, aDHCPServerImpl),
                     iIncomingMsgDataPtr( 0, 0 )
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
/**
  * Constructor of the DHCPv4StateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
		
#else // SYMBIAN_NETWORKING_DHCPSERVER

inline CDHCPIP4StateMachine::CDHCPIP4StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName ),
                     iIncomingMsgDataPtr( 0, 0 )
/**
  * Constructor of the DHCPv4StateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER

inline DHCPv4::CDHCPMessageHeaderIP4* CDHCPIP4StateMachine::DhcpMessage() const
	{
	return static_cast<DHCPv4::CDHCPMessageHeaderIP4*>(iDhcpMessage);
	}

inline TBool CDHCPIP4StateMachine::IsUsingStaticAddress() const
	{
	return iCfgInfoOnly;
	}
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
inline TUint32 CDHCPIP4StateMachine::GetClientIPAddress()
	{
	DHCPv4::CDHCPMessageHeaderIP4* v4Msg = DhcpMessage();
	return  v4Msg->GetCIAddr();
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER	

#endif

