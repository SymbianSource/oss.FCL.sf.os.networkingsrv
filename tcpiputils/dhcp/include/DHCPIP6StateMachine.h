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
// The DHCPIP6StateMachine header file
// 
//

/**
 @file DHCPIP6StateMachine.h
*/

#ifndef DHCPIP6STATEMACHINE_H
#define DHCPIP6STATEMACHINE_H

#include "DHCPStateMachine.h"
#include "DHCPIP6MsgSender.h"
#include "DHCPIP6IA.h"

class CDHCPIP6State;
class CDHCPIP6MessageSender;
class CDhcpIP6MessageReader;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
class CDHCPIP6StateMachine : public CDHCPStateMachine
/**
  * Implements helper function & starts DHCPv6 tasks (SOLICITATION,INFORM,....)
  *
  * @internalTechnology
  */
	{
friend class CDHCPIP6Control;
friend class CDHCPIP6ListenToNeighbor;
friend class CDHCPIP6Select;
friend class CDHCPIP6ReplyNoBinding;
friend class CDHCPIP6Renew;
friend class CDHCPIP6Reconfigure;
friend class CDHCPIP6Rebind;

public:
   /*tasks the statemachine could be started for
   enum ETask
      {                 //Initiated by:   Reason:
      ESolicitation,    //Client          IP Address & Info (DNS servers,.....
      EInformRequest,   //Client          Info only
      EConfirm,         //Client          Using the same address after reboot, return from sleep, change IAP,....
      EConfigExchange,  //Client          After lease time expired
      EReconfigure,     //Server          Information given to client's changed
      ERebind,          //Client          Renew message didn't work
      EDecline,         //Client          IP Address not suitable
      ERelease          //Client          IP Address not needed any more
      };
   */

public:
	~CDHCPIP6StateMachine();
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	static CDHCPIP6StateMachine* NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager);
#else
	static CDHCPIP6StateMachine* NewL(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
   //interface used by CDHCPIP6Control
	virtual void GetServerAddress( TInetAddr& aAddress );
  	virtual void SetCurrentAddress(const TInetAddr& aCurrentAddress, const TInetAddr& aSubnetMask);
  	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER  	
	virtual void CreateOfferMsgL();
	virtual void HandleRequestMsgL();
	virtual void HandleInformMsgL();	
	virtual CDHCPState* ReceiveOnPort67L( TRequestStatus* aStatus );
	virtual void CloseNSendServerMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType);
	virtual void InitialiseServerSocketL();
#endif // SYMBIAN_NETWORKING_DHCPSERVER
   //dhcpIPX tasks
	virtual void StartInitL(MStateMachineNotify* aStateMachineNotify, EInitialisationContext aInitialisationContext, TInt aUserTimeOut = 0 );//Starts Solicitation
	virtual void StartInformL(MStateMachineNotify* aStateMachineNotify, TBool aStaticAddress);//Starts InformRequest
	virtual void StartRebootL(MStateMachineNotify* aStateMachineNotify);//Starts Confirm
   virtual void StartRenewL(MStateMachineNotify* aStateMachineNotify, TInt aUserTimeOut);
	virtual void StartRebindL(MStateMachineNotify* aStateMachineNotify);
	virtual void StartDeclineL(MStateMachineNotify* aStateMachineNotify);
	virtual void StartReleaseL(MStateMachineNotify* aStateMachineNotify);
   //dhcpIP6 special task
	void StartReconfigureL(MStateMachineNotify* aStateMachineNotify);

   virtual void RemoveConfiguredAddress(const TInetAddr *aInetAddr = NULL);
   virtual CDHCPState* ReceiveL( TRequestStatus* aStatus );
   void CancelMessageReceiver();
	virtual void CloseNSendMsgL(TRequestStatus& aStatus, CDHCPStateMachine::EAddressType aEAddressType);

	//interface used by CDHCPIP6States derivatives
   void CreateMessageL();
   void CreateCommonMsgStart();
   void CreateCommonMsgEndL();
	virtual void InitialiseSocketL();
   //message creation functions                       //IP6 naming
   //--------------------------------------------------------------------
	virtual void CreateDiscoverMsgL();                 //solicit message
	virtual void CreateOfferAcceptanceRequestMsgL();   //request
	virtual void CreateRebootRequestMsgL();            //confirm
	virtual void CreateInformMsgL();                   //inform-request
	virtual void CreateDeclineMsgL();                  //decline
	virtual void CreateRenewRequestMsgL();             //renew
   virtual void CreateRebindRequestMsgL();            //rebind
	virtual void CreateReleaseMsgL();                  //release
	virtual void HandleOfferL();                       //handle advertisement
	virtual CDHCPState* HandleReplyL( TRequestStatus* aStatus );//handle final reply

   virtual void BindSocketForUnicastL();

   TUint GetMessageType() const;
   TBool CheckXid() const;

	CDHCPIP6MessageSender* MessageSender() const;
	DHCPv6::CDHCPMessageHeaderIP6* DhcpMessage() const;

	void ConfigureInterfaceL( TInt aPos );
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
	void StartInformL(MStateMachineNotify* aStateMachineNotify);
#endif //SYMBIAN_TCPIPDHCP_UPDATE
   /*the below are values from the base class initialised from the IA (iInterfaceConfigInfo)
   used in current transanction (renew or rebind) so that they can be accessed from within the
   CDHCPIP6Renew and CDHCPIP6Rebind classes
	TUint32 iRenewalTimeT1;
	TUint32 iRebindTimeT2;
	TUint32 iLeaseTime;
   */

protected:
   CDHCPIP6StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName);
   void ConstructL();

   virtual void AssembleClientIDsL();
   virtual void PrepareToSendL(CDHCPStateMachine::EAddressType aEAddressType);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER   
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
   CDHCPIP6StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
   virtual void PrepareToSendServerMsgL(CDHCPStateMachine::EAddressType aEAddressType);
#endif // SYMBIAN_NETWORKING_DHCPSERVER
   virtual void AssignAddresses( TInetAddr& aDest, const TInetAddr& aSrc ) const;

   virtual void OnCompletion();

//   virtual void DoCancel();

protected:	
   void SetMessageHeaderL( DHCPv6::TMessageType aMsgType );
	void HandleAckL();

public:
   DHCPv6::TInterfaceConfigInfo iInterfaceConfigInfo;

private:
    CDhcpIP6MessageReader* iMessageReader;
    mutable RBuf8 iServerId;
   	TInt 	iUserRebindTimeout;
	EInitialisationContext iStartInitCalls;	//We need to know when StartInitL is called for the second time.
											//On the second time we will try once to solicit a DHCP server. 
											//If it fails we will complete the client and try harder on subsequent

	};
	
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
inline CDHCPIP6StateMachine::CDHCPIP6StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName,EFalse ), iStartInitCalls(EFirstCall)
/**
  * Constructor of the DHCPIP6StateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#else
inline CDHCPIP6StateMachine::CDHCPIP6StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName, CDhcpHwAddrManager* aDhcpHwAddrManager):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName, aDhcpHwAddrManager, EFalse ), iStartInitCalls(EFirstCall)
/**
  * Constructor of the DHCPIP6StateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#else // SYMBIAN_NETWORKING_DHCPSERVER
	
inline CDHCPIP6StateMachine::CDHCPIP6StateMachine(RSocketServ& aEsock, RConnection& aConnection, const TName& aInterfaceName):
                     CDHCPStateMachine( aEsock, aConnection, aInterfaceName ), iStartInitCalls(EFirstCall)
/**
  * Constructor of the DHCPIP6StateMachine
  *
  * @internalTechnology
  *
  */
	{
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER

inline CDHCPIP6MessageSender* CDHCPIP6StateMachine::MessageSender() const
   {
	return static_cast<CDHCPIP6MessageSender*>(iMessageSender);
   }

/**
  * Accessor method for the current DHCP message
  *
  * @internalTechnology
  *
  */
inline DHCPv6::CDHCPMessageHeaderIP6* CDHCPIP6StateMachine::DhcpMessage() const
	{
	return static_cast<DHCPv6::CDHCPMessageHeaderIP6*>(iDhcpMessage);
	}


#endif

