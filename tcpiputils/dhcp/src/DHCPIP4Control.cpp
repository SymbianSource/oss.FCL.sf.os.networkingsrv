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
// Implements the DHCP IP4 control plain
// 
//

/**
 @file DHCPIP4Control.cpp
 @internalTechnology
*/

#include "DHCPIP4Control.h"
#include "DHCPIP4States.h"
#include "DHCPIP4Msg.h"
#include "DHCPServer.h"
#include "DHCPConfigListener.h"
#include "DHCPDb.h"
#include "ExpireTimer.h"
#include "NetCfgExtDhcpControl.h"
#include "DomainNameDecoder.h"
#include <nifman.h>
#include <comms-infras/es_config.h>
#include "NetCfgExtnDhcpMsg.h"
#include <f32file.h>
#include <comms-infras/metatype.h>
#include <comms-infras/metadata.h>
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
#include "DHCPIP4Msg.h" 
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include "dhcphwaddrmanager.h"
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
using namespace DHCPv4;
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS			

struct TDHCPv4Persistent : public Meta::SMetaData
{
	TDHCPv4Persistent() : 
		iTaskStartedAt( 0 ),
		iCurrentAddress(KInetAddrNone,KInetPortNone),
		iLeaseTime(0)
		{
		}

	TTime iTaskStartedAt;
	TInetAddr iCurrentAddress;
	TUint32 iLeaseTime;

	DATA_VTABLE
};

START_ATTRIBUTE_TABLE( TDHCPv4Persistent, KDHCPv4Persinstence, KDHCPv4PersinstenceId )
	REGISTER_ATTRIBUTE( TDHCPv4Persistent, iTaskStartedAt, TMetaTime )
	REGISTER_ATTRIBUTE( TDHCPv4Persistent, iCurrentAddress, TMeta<TInetAddr> )
	REGISTER_ATTRIBUTE( TDHCPv4Persistent, iLeaseTime, TMetaNumber )
END_ATTRIBUTE_TABLE()

CDHCPIP4Control::~CDHCPIP4Control()
	{
	}

void CDHCPIP4Control::ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage)
/**
  * Open and attach to the RConnection
  *
  * @internalTechnology
  */
	{
   CDHCPControl::ConfigureL( aInfo, aMessage );
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4Control::ConfigureL")));
	
	iDhcpDb = new (ELeave) CDHCPDb( TPtrC( SERVICE_IP_DNS_ADDR_FROM_SERVER ),
													TPtrC( SERVICE_IP_NAME_SERVER1 ),
													TPtrC( SERVICE_IP_NAME_SERVER2 )); //for both version for the time being see the class TDHCPIP4Db coments
	FindInterfaceNameL(aInfo,KAfInet);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	iDhcpStateMachine = CDHCPIP4StateMachine::NewL(iEsock, iConnection, iInterfaceName,iDHCPServerImpl);
#else
	iDhcpHwAddrManager = CDhcpHwAddrManager::NewL();
	iDhcpStateMachine = CDHCPIP4StateMachine::NewL(iEsock, iConnection, iInterfaceName,iDhcpHwAddrManager, iDHCPServerImpl);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#else 
	iDhcpStateMachine = CDHCPIP4StateMachine::NewL(iEsock, iConnection, iInterfaceName);
#endif 	// SYMBIAN_NETWORKING_DHCPSERVER		
	TDHCPv4Persistent pers;
	iStaticAddress = !iDhcpDb->ReadL(*iDhcpStateMachine, pers);


	//initialise relevant data
	TTimeIntervalSeconds seconds = static_cast<TInt>(pers.iLeaseTime);
	iDhcpDb->iLeaseExpiresAt = pers.iTaskStartedAt + seconds;
	DhcpStateMachine()->iLeaseTime = pers.iLeaseTime;
	iDhcpStateMachine->SetCurrentAddress( pers.iCurrentAddress );
	DhcpStateMachine()->iMaxRetryCount = 2; //Max retry count for the first run is set to 2


	TInetAddr existingGlobalAddr = iDhcpStateMachine->GetInterfaceGlobalAddress();

	if( ! existingGlobalAddr.IsUnspecified() )
		{
		// there was an existing address on the interface, regardless of commsdat preference..
		//  so force inform.
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Interface already has global address. Forcing inform mode.")));
		iDhcpStateMachine->SetCurrentAddress( existingGlobalAddr );
		iStaticAddress = ETrue;
		}
	else
		{
		if(iStaticAddress)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Interface has a static address from commsdat. Proceeding in inform mode.")));
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Interface has no global address yet. Proceeding in discover mode.")));
			}
		}

	CDHCPControl::ConfigureL(iStaticAddress);
	}

void CDHCPIP4Control::Cancel()
	{
	iClientShouldCompleteWhenLinkLocalCreated = EFalse;

	// Call the base class.
	CDHCPControl::Cancel();
   }

void CDHCPIP4Control::BindingFinishedL()
	{
	CDHCPControl::BindingFinishedL();
	TDHCPv4Persistent pers;
	//get relevant data from statemachine
	pers.iTaskStartedAt = DhcpStateMachine()->iStartedAquisitionAt;
	pers.iCurrentAddress = DhcpStateMachine()->iCurrentAddress;
	pers.iLeaseTime = DhcpStateMachine()->iLeaseTime;
	TRAPD( ret, iDhcpDb->WriteL(*iDhcpStateMachine, pers) );
	if (ret!=KErrNone)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4Control::BindingFinishedL error: %d"),ret));
		}
	}

void CDHCPIP4Control::TimerExpired()
/**
  * Called by the timer to signal that the timer has expired
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4Control::TimerExpired()")));

    CDHCPControl::TimerExpired();
	
	// In response to the rebind operation timing out, try to create
	// a link local to allow local addressing (may not be supported
	// by current interface's IPv4 link local option).  We cannot
	// create the link local earlier (e.g., after a renew attempt
	// times-out) because the link local will not be usable until
	// the DHCP assigned address has been removed (randomly
	// generated addresses are prioritised below non-randomly
	// generated addresses during selection in the TCP/IP stack).
	if( ( iState == EInitInProgress ) && iDhcpConfigListener && !iDhcpConfigListener->HaveLinkLocal() )
		{
		DhcpStateMachine()->CreateIPv4LinkLocal();
		}
	}

TInt CDHCPIP4Control::HandleClientRequestL(TUint aName, TInt aValue)
/**
  * Handles client requests made through RConnection
  * are handled here.  Currently its for renewing the lease time 
  * where client passes in a required timeout 
  * @see CDHCPControl::HandleClientRequestL
  *
  * @internalTechnology
  */
	{
	TBool deferAllowed = !iMessage;
	TBool defer = EFalse;
	
  	defer = CDHCPControl::HandleClientRequestL(aName, aValue);

	if (aName == KConnAddrRenew && iState == ERenewInProgress && deferAllowed)
		{//Rebind timeout
		TTimeIntervalSeconds timeOut;
		if(aValue > 0)
			{
			timeOut = aValue;
			}
		else
			{
			timeOut = (DhcpStateMachine()->iRebindTimeT2 - DhcpStateMachine()->iRenewalTimeT1 + 1);
			}
      	iTimer->After(timeOut, *this);
      	defer = ETrue;
		}
	
	return defer;
	}	

TInt CDHCPIP4Control::HandleClientRequestL(TUint aName)
/**
  * Handles client requests specific for IP4
  *
  * @see CDHCPControl::HandleClientRequestL
  * @internalTechnology
  */
	{
	return CDHCPIP4Control::HandleClientRequestL(aName,0);
	}

/**
    Get raw option data (IP4 version).
    @param  pointer to the buffer descriptor for getting option data
*/
void CDHCPIP4Control::HandleGetRawOptionDataL(TDes8* aDes)
{
	TDhcp4RawOptionDataPckg pckg(*aDes);
		
	TUint8 opCode = pckg.OpCode();
	TPtr8 buf(pckg.Buf());
	GetRawOptionDataL(opCode, buf);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	if (!iDhcpStateMachine->iDhcpInformAckPending)   
		{
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	pckg.SetBufLengthL(buf.Length());
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
}

void CDHCPIP4Control::TaskCompleteL( TInt aError )
/**
  * TaskCompleteL function
  *
  * if this method returns ETrue then aStateMachine deletes itself.
  * In this case it does not ever return ETrue.
  * Called upon completion or when suspended.
  *
  * @see CStateMachine::iSuspendRequest comment
  * @internalTechnology
  */
	{
	// In response to the initialisation operation timing out,
	// try to create a link local to allow local addressing (may
	// not be supported by current interface's IPv4 link local
	// option).
	if( ( aError == KErrTimedOut ) &&
		( iState == EInitInProgress ) &&
		!iStaticAddress &&
		iDhcpConfigListener &&
		!iDhcpConfigListener->HaveLinkLocal() )
		{
		if( DhcpStateMachine()->CreateIPv4LinkLocal() == KErrNone )
			{
			iClientShouldCompleteWhenLinkLocalCreated = ETrue;
			//Link Local Address has been taken & return here itself
			CDHCPControl::TaskCompleteL(aError);
			return;
			}
		}
	if((DhcpStateMachine()->iRetryDhcpIpCount >= KDHCPv4MaxRetryCount) && iState == EDeclineInProgress )
		{
		if( DhcpStateMachine()->CreateIPv4LinkLocal() == KErrNone )
			{
			iClientShouldCompleteWhenLinkLocalCreated = ETrue;
			//Link Local Address has been taken & return here itself
 			CDHCPControl::TaskCompleteL(KErrNone);
 			return;
			}
		
		}
	// Call the base class.
	CDHCPControl::TaskCompleteL( aError );
	}
	
void CDHCPIP4Control::LinkLocalCreated()
/**
  * LinkLocalCreated function
  *
  * If iClientShouldCompleteWhenLinkLocalCreated is ETrue we need to complete the
  * client messages as the link local has been created and the interface is now
  * ready for use.
  *
  * @internalTechnology
  */
	{
	if( iClientShouldCompleteWhenLinkLocalCreated )
		{
		iClientShouldCompleteWhenLinkLocalCreated = EFalse;
		
		CompleteClientConfigureMessage(KErrNone);
		// don't complete any outstanding ioctl yet..
		}
	}

void CDHCPIP4Control::GetRawOptionDataL(TUint aOpCode, TPtr8& aPtr)
	{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	if(iValidMsg.Length()==0 && iDHCPServerImpl)
		{
		GetRawOptionDataFromDNSBufL(aPtr);
		return;
		}
#endif // SYMBIAN_NETWORKING_DHCPSERVER
	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr( const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length() );
	msg.iRecord.ParseL(ptr); //no check necessary
	DHCPv4::COptionNode* pOption = msg.iOptions.FindOption(static_cast<TUint8>(aOpCode));
	if (!pOption)
		{
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		CleanupStack::PopAndDestroy();
		TUint8 tempOpt=aOpCode;
		TPtr8 optPtr(&tempOpt,1,1);
		return(RequestInformOrCompleteCallL(optPtr));
#else
		User::Leave(KErrNotFound);
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		}
	ptr.Set(pOption->GetBodyDes());
	if (ptr.Length() > aPtr.MaxLength())
		{
		User::Leave(KErrOverflow);
		}
	aPtr.Copy(ptr);
	CleanupStack::PopAndDestroy();
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP4Control::GetRawOptionDataFromDNSBufL(TPtr8& aPtr)
	{
	if(iValidMsg.Length() == 0)
		{
		if (iDNSRawOption->Des().Length() > aPtr.MaxLength())
			{
			User::Leave(KErrOverflow);
			}
		aPtr.Copy(iDNSRawOption->Des());
		}
	}

void CDHCPIP4Control::HandleSetRawOptionCodeL(TDes8* aDes)
	{
	if(iValidMsg.Length() == 0)
		{
		iDNSRawOption = HBufC8::NewL(aDes->Length());
		iDNSRawOption->Des() = *aDes;
		}
	else
		{
		SetRawOptionCodeL(aDes);	
		}
	}

void CDHCPIP4Control::SetRawOptionCodeL(TDes8* aDes)
	{
	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr( const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length() );
	msg.iRecord.ParseL(ptr); //no check necessary
	
	TDhcp4RawOptionDataPckg pckg(*aDes);
	DHCPv4::TDHCPOptionCodes opcode = static_cast<DHCPv4::TDHCPOptionCodes>(pckg.OpCode());
	
	DHCPv4::COptionNode* pOption = msg.iOptions.FindOption(opcode);
	if (!pOption)
		{
		User::Leave(KErrNotFound);
		}
		
	TPtr8 ptr2(reinterpret_cast<TUint8*>(aDes),aDes->Length());
	
	pOption->SetBody(ptr2);	
	CleanupStack::PopAndDestroy(); // msg
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER

void CDHCPIP4Control::HandleGetSipServerAddrL(TDes8* aDes)
	{	
	if (aDes->Length() < static_cast<TInt>(sizeof(SSipServerAddr)))
		{
		User::Leave(KErrArgument);
		}

	SSipServerAddr* sipServerAddr = 
		reinterpret_cast<SSipServerAddr*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);
	DHCPv4::COptionNode* sipServerOption = msg.iOptions.FindOption(DHCPv4::EDHCPSIPServers);

	if(sipServerOption)
		{
		TUint8* headerPtr = sipServerOption->Ptr();		
		
		TUint encoding = headerPtr[DHCPv4::KDhcpSipEncodingOffset];
		TUint length = headerPtr[DHCPv4::KDhcpSipLengthOffset];
		
		if(encoding != DHCPv4::KDhcpSipEncodingAddresses)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerAddrL: Client trying to fetch a SIP address when only a SIP domain is known")));
			User::Leave( KErrNotFound );
			}
		if((TUint)sipServerAddr->index >= (length / KIp4AddrByteLength))
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerAddrL: Client tried to fetch an out-of-range SIP address %d (I only know about %d)"),(TUint)sipServerAddr->index, (length / KIp4AddrByteLength)));
			User::Leave( KErrNotFound );
			}
		
		TUint addrOffset = sipServerAddr->index * KIp4AddrByteLength;
		
		TUint8* bodyPtr = sipServerOption->GetBodyPtr();
		TUint32 copiedValue;
		// Grab ip address from option body at given offset, +1 used to skip the encoding boolean used for sip
		Mem::Copy(&copiedValue,(bodyPtr + addrOffset+1),4);  
		// swap around ip address as in BigEndian form
		copiedValue = ByteOrder::Swap32(copiedValue); 
		sipServerAddr->address.SetAddress(copiedValue);   
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerAddrL: Client tried to fetch a SIP option but none has (yet) been received")));
		User::Leave(KErrNotFound);
		}	
	CleanupStack::PopAndDestroy();
	}

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
void CDHCPIP4Control::GetDhcpHdrSiaddrL(TDes8& aNxtAddress)
/**
  * The GetDhcpHdrSiaddr function
  *
  * Fetches the Siaddr
  *
  * @param aNxtAddress Next server IPAddress fetched from the DHCP Server
  * @internalTechnology
  */
	{
	if (aNxtAddress.Length() < static_cast<TInt>(sizeof(TConnectionAddress)))
		{
		User::Leave(KErrArgument);
		}
	TConnectionAddress* ptr = (TConnectionAddress*)aNxtAddress.Ptr();
	TInetAddr* addrPtr = new(&(ptr->iAddr))TInetAddr;	
	//ptr->iAddr is just some memory in aDes. There is no guarantee that it will be a
	//valid TInetAddr (or even a valid TDes) so what we do here is just run a constructor
	//on this already valid memory block and we are now guaranteed to have a valid 
	//TInetAddr - NO MEMORY IS ACTUALLY ALLOCATED BY NEW HERE - see base code for more 
	//details	
	
	//Parse the iValidMsg
	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 msgPtr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(msgPtr);
	
	//Extract the server address
	addrPtr->SetAddress(msg.GetSIAddr());
	
	CleanupStack::PopAndDestroy();
	}

void CDHCPIP4Control::GetDhcpHdrSnameL(TDes8& aHdrSvrName)
/**
  * The GetDhcpHdrSname function
  *
  * Fetches the Sname
  *
  * @param aHdrSvrName Next server name fetched from the DHCP Server. 
  * @internalTechnology
  */
	{
	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);
	if (msg.GetSName().Length() > aHdrSvrName.MaxLength())
	{
		User::Leave(KErrOverflow);
	}
	aHdrSvrName=msg.GetSName();
	//SetLength otherwise received string length will be 64 by default, truncate to end of string.
	aHdrSvrName.SetLength(aHdrSvrName.Locate('\0'));

	CleanupStack::PopAndDestroy(&msg);
	}
	
void CDHCPIP4Control::HandleGetTftpServerAddrL(TDes8& aDes) 
/**
  * The HandleGetTftpServerAddrL function
  *
  * Fetches the Tftp Server Address
  * @param aDes Buffer returned contains a list of ip addresses each of 4 bytes.
  *
  * @internalTechnology
  */
	{
	if (aDes.Length() < static_cast<TInt>(sizeof(STftpServerAddr)))
		{
		User::Leave(KErrArgument);
		}

	STftpServerAddr* tftpServerAddr = 
		reinterpret_cast<STftpServerAddr*>(const_cast<TUint8*>(aDes.Ptr()));

	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);
	DHCPv4::COptionNode* tftpServerOption = msg.iOptions.FindOption(DHCPv4::EDHCPTftpServers);
	if(tftpServerOption)
		{
		
		TUint8 len=tftpServerOption->GetItemLength()-2;//subtract 2 bytes becz length include message opcode and message opcode length 
		
		TUint addrOffset = tftpServerAddr->index * KIp4AddrByteLength;
		
		if (addrOffset>len)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetTftpServerAddrL: Client tried to fetch a invalid index of Tftp Address which is not present")));
			iDhcpStateMachine->iDhcpInformAckPending=EFalse;
			User::Leave(KErrNotFound);	
			
			}
		
		TUint8* bodyPtr = tftpServerOption->GetBodyPtr();
				
		TUint32 copiedValue;
		// Grab ip address from option body at given offset.
		Mem::Copy(&copiedValue,(bodyPtr + addrOffset),4);  
		// swap around ip address as in BigEndian form/		copiedValue = ByteOrder::Swap32(copiedValue); 
		copiedValue = ByteOrder::Swap32(copiedValue); 
		tftpServerAddr->address.SetAddress(copiedValue);   
		}
	else //check inform to be made or not..or return
		{
		CleanupStack::PopAndDestroy(); //pop buffer ..we are going to return immediately
		TUint8 opt=DHCPv4::EDHCPTftpServers;
		TPtr8 optPtr(&opt,1,1);
		return(RequestInformOrCompleteCallL(optPtr));
		}	
	iDhcpStateMachine->iDhcpInformAckPending=EFalse;
	CleanupStack::PopAndDestroy();
	
}

void CDHCPIP4Control::HandleGetTftpServerNameL(TDes8& aTftpSvrName)
/**
  * The HandleGetTftpServerName function
  *
  * Fetches the Tftp Server Name
  *
  * @param aTftpSvrName Tftp Server Name fetched from the DHCP Server.
  * @internalTechnology
  */
	{
	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);
	DHCPv4::COptionNode* tftpOption = msg.iOptions.FindOption(DHCPv4::EDHCPTftpServerName);
	if(tftpOption)
		{
		if (aTftpSvrName.MaxLength()<tftpOption->GetItemLength())
			{
			iDhcpStateMachine->iDhcpInformAckPending=EFalse;
			User::Leave(KErrOverflow);
			}
		else	
			{
			aTftpSvrName.Copy(tftpOption->GetBodyDes());
			}
		}
	else 
		{
		//To check if DHCP Sname Option is overloaded
		tftpOption = msg.iOptions.FindOption(DHCPv4::EDHCPOptionOverload);
		if(tftpOption)
			{
			if(	msg.iOptions.GetOptionOverload() == DHCPv4::EDHCPSname)
				{
				GetDhcpHdrSnameL(aTftpSvrName);
				}
			}
		else//now issue Inform Request for option 66 or return now..
			{
			CleanupStack::PopAndDestroy(); //pop buffer ..we are going to return immediately
			TUint8 opt=DHCPv4::EDHCPTftpServerName;
			TPtr8 optPtr(&opt,1,1);
			return(RequestInformOrCompleteCallL(optPtr));
			}
		}
		iDhcpStateMachine->iDhcpInformAckPending=EFalse;
		CleanupStack::PopAndDestroy();	
	}	

void CDHCPIP4Control::RequestInformOrCompleteCallL(TPtr8& aOpcodePtr)	
/**
  * The RequestInformOrCompleteCallL function
  *
  * Checks DHCPINFORM message should be sent or not
  * @param aOpcodePtr  pointer to the opcode list to be updated  in iSavedExtraParameters
  * @internalTechnology
  */
	{

	if (!iDhcpStateMachine->iDhcpInformAckPending)
		{
		if (!iDhcpStateMachine->iSavedExtraParameters.Length())
			{
			iDhcpStateMachine->iSavedExtraParameters.CreateL(aOpcodePtr);
			}
		else
			{
			iDhcpStateMachine->iSavedExtraParameters.ReAllocL(iDhcpStateMachine->iSavedExtraParameters.Length()+aOpcodePtr.Length());
			iDhcpStateMachine->iSavedExtraParameters.Append(aOpcodePtr);
			}	
	
			static_cast<CDHCPIP4StateMachine*>(iDhcpStateMachine)->StartInformL( this );
			iDhcpStateMachine->iDhcpInformAckPending=ETrue;
  			iState = EInformInProgress;
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8(" RequestInformOrCompleteCallL::Client tried to fetch a  option but none has (yet) been received")));
		iDhcpStateMachine->iDhcpInformAckPending=EFalse;
		User::Leave(KErrNotFound);	
		}
	}
	
	

void CDHCPIP4Control::HandleGetMultipleParamsL(TDes8& aDes)
/**
  * The HandleGetMultipleParamsL function
  *
  * Fetches the required raw option parameter buffer.
  * Buffer contains list of parameter options followed by msg opcode, len, data..
  * If any of the parameters are not found, DHCPINFORM message will be sent.
  *	Filled Returned Buffer is as shown in fig..
  * @code
  *		  -----------------------------------------------------
  *		 |No of |      |Data   |      |       |Data   |        |
  *	 	 |OpCode|OP1   |length1|Data1 |  OP2  |Length2|Data2   |
  *		 |      |      |       |      |       |       |        |
  *		  ----------------------------------------------------- 
  * @endcode
  * @param aDes Pointer to the buffer descriptor for getting option data
  * @internalTechnology
  */
	{
	HBufC8 *saveBuf=NULL, *buffer=NULL;
	TUint8 *headerPtr;
	TInt numOfOpcodes=0;
	TInt totalLength=0,opcodeLength=aDes.Length();
	TInt maxLength=aDes.MaxLength();
	TUint8 opcode;
	TBool allFound=ETrue;
	
	DHCPv4::CDHCPMessageHeaderIP4 msgSaved(saveBuf);
	TPtr8 savedPtr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	CleanupClosePushL(msgSaved);
	msgSaved.iRecord.ParseL(savedPtr);


	//The below for loop checks all the required opcode data is present or not
	//The message opcode data is not present in the iValidMsg buffer,the corresponding 
	//opcodes are stored in iCurrentParameters for sending it in DHCPINFORM message
	for(TInt i=0;i<opcodeLength;i++)
		{
		opcode=*(aDes.Ptr()+i); //search required opcode is present or not, one by one
		DHCPv4::COptionNode* findNode = msgSaved.iOptions.FindOption(opcode);
		if (findNode )
			{
			TInt bufLength=findNode->GetItemLength();
			totalLength+=bufLength;
			if ((totalLength+1) > maxLength)
				{
				totalLength-=bufLength;
				continue; //current buffer is too big..so hope next buffer is small
				}
			if (!buffer)
				{
				buffer=HBufC8::NewLC(totalLength + 1);//+1 is extra byte to store number of opcodes
				buffer->Des().Append(KNoChar);
				}
			else
				{
				buffer=buffer->ReAllocL(totalLength + 1);
				CleanupStack::Pop();//buffer as ptr address has changed
		        CleanupStack::PushL(buffer);
				}
			headerPtr = findNode->Ptr(); 
			++numOfOpcodes;
			buffer->Des().Append(headerPtr,bufLength);
			}
		else
			{
			//If atleast one opcode, among the requested, is not found then request through 
			//DHCP INFORM message by calling RequestInformOrCompleteCallL
			allFound=EFalse;
			}
		}
	
	if (allFound ) //everything is present..just return call now itself..
		{
		if ((totalLength + 1) > maxLength)
			{
			User::Leave(KErrOverflow);
			}
		if(buffer) // buffer would be NULL only when aDes.Length = 0 which is a rare scenario. But still check it to avoid NULL pointer dereference.
			{
			aDes.Copy(buffer->Ptr(), buffer->Length());
			TBuf8<1> dummy;
			dummy.FillZ(1);
			dummy[0]=numOfOpcodes;
			aDes.Replace(0,1,dummy);//update number of opcodes collected
			}
		}
	else
		{
		TPtr8 opcodePtr(const_cast<TUint8*>(aDes.Ptr()),opcodeLength);
		opcodePtr.SetLength(opcodeLength);
		RequestInformOrCompleteCallL(opcodePtr);	
		}	
	
	if (buffer)
		{
		CleanupStack::PopAndDestroy(buffer);
		}

	CleanupStack::PopAndDestroy(&msgSaved);

	}


TInt CDHCPIP4Control::InformCompleteRequestHandlerL()
/**
  * The InformCompleteRequestHandlerL function
  * 
  * The function will be called after the DHCPACK message is received, and 
  * iValidMsg buffer has been updated. 
  * Looks for the requested options present or not and appropriately calls 
  * the function handlers
  *
  * @return KErrNone if the required data option is found else corresponding error code.
  * @internalTechnology
  */
	{

	TUint optionName = iMessage->Int1();
	TInt length      = iMessage->Int3();

	HBufC8 *buff=HBufC8::NewMaxLC(length) ;
	TPtr8 ptr(buff->Des());
	iMessage->ReadL(2, ptr);

	
	switch(optionName)
		{
		case KConnGetDhcpRawOptionData:
			{
			HandleGetRawOptionDataL(&ptr);
			iMessage->WriteL(2,ptr);
			break;
			}
		
		case KConnDhcpGetMultipleParams :
			{
			HandleGetMultipleParamsL(ptr);
			iMessage->WriteL(2,ptr);
			break;
			}
	
		case KConnGetTftpServerAddr:
			{
			HandleGetTftpServerAddrL(ptr);
			iMessage->WriteL(2,ptr);
			break;
			}
		
		case KConnGetTftpServerName:
			{
			HandleGetTftpServerNameL(ptr);
			iMessage->WriteL(2,ptr);
			break;
			}
		
		default:
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("InformCompleteRequestHandlerL : default unhandled optino Name")));
			break;	
	
		}
	iDhcpStateMachine->iDhcpInformAckPending=EFalse;
	CleanupStack::PopAndDestroy(buff);
	return KErrNone;
	 
	}
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS


void CDHCPIP4Control::HandleGetSipServerDomainL(TDes8* aDes)
	{
	if (aDes->Length() < static_cast<TInt>(sizeof(SSipServerDomain)))
		{
		User::Leave(KErrArgument);
		}

	SSipServerDomain* sipServerDomain = 
		reinterpret_cast<SSipServerDomain*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv4::CDHCPMessageHeaderIP4 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);
	DHCPv4::COptionNode* sipServerOption = msg.iOptions.FindOption(DHCPv4::EDHCPSIPServers);

	if(sipServerOption)
		{
		TUint8* headerPtr = sipServerOption->Ptr();
		
		TUint encoding = headerPtr[DHCPv4::KDhcpSipEncodingOffset];

		if(encoding != DHCPv4::KDhcpSipEncodingDomains)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerDomainL: Client trying to fetch a SIP domain when only a SIP address is known")));
			User::Leave( KErrNotFound );
			}

		CDomainNameCodec* domainNameDecoder = new(ELeave) CDomainNameCodec();
		CleanupStack::PushL(domainNameDecoder);
		TPtr8 ptr = sipServerOption->GetBodyDes();
		ptr.Set((TUint8*)(ptr.Ptr()+1),ptr.Length()-1,ptr.MaxLength()-1);
		domainNameDecoder->DecodeL(ptr);

		if(sipServerDomain->index < (TInt)domainNameDecoder->NumDomainNames())
			{
			TDomainName domainName = (*domainNameDecoder)[sipServerDomain->index];
			sipServerDomain->domainName.Copy(domainName);
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerDomainL: Client tried to fetch an out-of-range SIP domain %d (I only know about %d)"), sipServerDomain->index , (TInt)domainNameDecoder->NumDomainNames()));
			User::Leave(KErrNotFound);
			}

		CleanupStack::PopAndDestroy();
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("HandleGetSipServerDomainL: Client tried to fetch a SIP option but none has (yet) been received")));
		User::Leave(KErrNotFound);
		}
	CleanupStack::PopAndDestroy();
	}



TBool CDHCPIP4Control::ShouldInformAfterFailedInit(void)
	{
	// if has no global address, should not inform
	return iDhcpStateMachine->GetInterfaceGlobalAddress().IsUnspecified() ? EFalse : ETrue ;
	}

