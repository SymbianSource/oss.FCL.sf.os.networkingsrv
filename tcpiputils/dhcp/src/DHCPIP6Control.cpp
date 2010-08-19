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
// Implements the DHCP IP6 highest level control plain
// 
//

/**
 @file DHCPIP6Control.cpp
 @internalTechnology
*/

#include "DHCPIP6Control.h"
#include "DHCPIP6States.h"
#include "DHCPServer.h"
#include "DHCPDb.h"
#include "NetCfgExtDhcpControl.h"
#include "DhcpIP6Msg.h"
#include "ExpireTimer.h"
#include "DomainNameDecoder.h"
#include <comms-infras/es_config.h>
#include "NetCfgExtnDhcpMsg.h"
#include <nifman.h>
#include <f32file.h>
#include <comms-infras/metatype.h>
#include <comms-infras/metadata.h>
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include "dhcphwaddrmanager.h"
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
using namespace DHCPv6;

struct TDHCPv6Persistent : public Meta::SMetaData
{
	TDHCPv6Persistent( SIdentityAssociationConfigInfoNA& aIdentityAssociationConfigInfoNA ) :
		iIdentityAssociationConfigInfoNA( &aIdentityAssociationConfigInfoNA )
		{
		iTaskStartedAt.HomeTime();
		}
	TTime iTaskStartedAt;
	SIdentityAssociationConfigInfoNA* iIdentityAssociationConfigInfoNA;

	DATA_VTABLE
};

START_ATTRIBUTE_TABLE( TDHCPv6Persistent, KDHCPv6Persinstence, KDHCPv6PersinstenceId )
	REGISTER_ATTRIBUTE( TDHCPv6Persistent, iIdentityAssociationConfigInfoNA, TMetaObjectPtr<SIdentityAssociationConfigInfoNA> )
	REGISTER_ATTRIBUTE( TDHCPv6Persistent, iTaskStartedAt, TMetaTime )
END_ATTRIBUTE_TABLE()


CDHCPIP6Control::~CDHCPIP6Control()
	{
	}

void CDHCPIP6Control::ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage)
/**
  * Open and attach to the RConnection
  *
  * @internalTechnology
  */
	{
   CDHCPControl::ConfigureL( aInfo, aMessage );
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::ConfigureL")));
	
	iDhcpDb = new (ELeave) CDHCPDb( TPtrC( SERVICE_IP6_DNS_ADDR_FROM_SERVER ),
													TPtrC( SERVICE_IP6_NAME_SERVER1 ),
													TPtrC( SERVICE_IP6_NAME_SERVER2 )); //for both version for the time being see the class TDHCPIP4Db comments
	TRAPD( err, FindInterfaceNameL(aInfo,KAfInet6) );
	if( err == KErrNone )
		{
		TBuf8<KMaxName> tempBuf;
		tempBuf.Copy( iInterfaceName );
		__CFLOG_1(KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::ConfigureL - FindInterfaceNameL found \"%S\""), &tempBuf);
		}
	else
		{
		__CFLOG_1(KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::ConfigureL - FindInterfaceNameL failed with error %d"), err);
		User::Leave( err );
		}
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	iDhcpHwAddrManager = CDhcpHwAddrManager::NewL();
	iDhcpStateMachine = CDHCPIP6StateMachine::NewL(iEsock, iConnection, iInterfaceName,iDhcpHwAddrManager);
#else
	iDhcpStateMachine = CDHCPIP6StateMachine::NewL(iEsock, iConnection, iInterfaceName);
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
	
	CDHCPIP6StateMachine* sm6 = static_cast<CDHCPIP6StateMachine*>(iDhcpStateMachine);
 	TDHCPv6Persistent pers(sm6->iInterfaceConfigInfo.iSIdentityAssociationConfigInfoNA);
	TBool bStaticAddress = !iDhcpDb->ReadL(*iDhcpStateMachine, pers);
	//initialise relevant data
	TInt pos = 0;
	const SIPAddressInfo* info = pers.iIdentityAssociationConfigInfoNA->GetValidAddressInfo( pos );
	TTimeIntervalSeconds seconds = info ? static_cast<TInt>(info->iValidLifeTime) : 0 ;
	iDhcpDb->iLeaseExpiresAt = pers.iTaskStartedAt + seconds;
	//the rest of the data we'll get back in reply msg
	CDHCPControl::ConfigureL(bStaticAddress);	
	}

void CDHCPIP6Control::BindingFinishedL()
 	{
	CDHCPControl::BindingFinishedL();
	//see case EDeclineInitialisedInProgress as to how the persistent
	//data is written
  	}

void CDHCPIP6Control::TaskCompleteL(TInt aError)
/**
  * Signals the end of a task
  * and decides what we should do when
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
				 _L8("CDHCPIP6Control::TaskCompleteL (%d) with error = %d") ,
				 iState, aError));

	DhcpStateMachine()->Cancel();
	TState prevState = iState;
	switch (iState)
		{
		case EDeclineInitialisedInProgress:
			{
			TDHCPv6Persistent pers(DhcpStateMachine()->iInterfaceConfigInfo.iSIdentityAssociationConfigInfoNA);
			//get relevant data from statemachine
			pers.iTaskStartedAt = DhcpStateMachine()->iStartedAquisitionAt;
			TRAPD( ret, iDhcpDb->WriteL(*iDhcpStateMachine, pers) );
			if (ret!=KErrNone)
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::TaskCompleteL error: %d"),ret));
				}
			iState = EInitialised;
			break;
			}
		case EReconfigureInProgress:
			//in the case the following fn leave we've run out of memory => we will not respond
			//to the reconfigure message. However we can still use our address(es) and wait
			//for renew timer to expire
			if ( aError == KErrNone )
				{
				CDHCPMessageHeaderIP6* v6Msg = DhcpStateMachine()->DhcpMessage();
				COptionNode* pNode = v6Msg->GetOptions().FindOption( EReconfMsg );
				if (pNode)
					{
					TInt nCode = pNode->GetBigEndian();
					switch ( nCode )
						{
						case EReconfigureInformRequest:
							DhcpStateMachine()->StartInformL( this, EFalse /*static address is either already set or not used*/ );
							iState = EInformInProgress;
							//cancel the renew timer (if any) after succesfully starting the above tasks
							iTimer->Cancel();
							break;
						case EReconfigureRenew:
							DhcpStateMachine()->StartRenewL( this,0 );
							iState = ERenewInProgress;
							//cancel the renew timer (if any) after succesfully starting the above tasks
							iTimer->Cancel();
							break;
						default:
							// keep restarting reconfigure
							DhcpStateMachine()->StartReconfigureL( this );
						}
					}
				else
					{
					// keep restarting reconfigure
					DhcpStateMachine()->StartReconfigureL( this );
					}
			}
			else
				{
				//keep restarting reconfigure until a valid reconfigure arrives or
				//renew timer expires or interface goes down
				DhcpStateMachine()->StartReconfigureL( this );
				}
			break;
		case ERebindInProgress:
		case EInitInProgress:
			iTimer->Cancel();

			if ( aError != KErrNone )
				{//decline all
				DhcpStateMachine()->iInterfaceConfigInfo.SetAddressStatus( KAddrIndexAll, EMarkForDecline );
				}
			iState = EInitInProgress; //take the same action as for EInitInProgress
			CDHCPControl::TaskCompleteL(aError);
			break;
		case ERenewInProgress:
			iTimer->Cancel();
			if (KErrNone != aError)
				{
				//Renew process has failed => start rebind
				DhcpStateMachine()->StartRebindL( this );
				iState = ERebindInProgress;
				break;
				}
			//fall through if no error
		default:
			{
			CDHCPControl::TaskCompleteL(aError);
			}
		};
	//if the statemachine history indicates CDHCPState::EBinding and aError==KErrNone we have to check whether to
	//decline any addresses that haven't been verified by the stack as valid ones. If so we initiate decline
	//to decline the invalid addresses. In case at least one address is valid we go to reconfigure state after the decline.
	//=> amend UML and doc accordingly.
	if (DhcpStateMachine()->History() & CDHCPState::EBinding && aError == KErrNone )
		{//see CDHCPIP6StateMachine::StartDeclineL
		DhcpStateMachine()->StartDeclineL( this );
		iState = EDeclineInitialisedInProgress;
		//not changing CDHCPIP6Control state we are initialised and timer is running
		}
	else if ( iState == EInitialised || (iState == EEnd && prevState == EInformInProgress))
		{//we don't want this to leave
#if defined(DHCP_RECONFIGURE_NO_AUTHENTICATION)
		//we don't support reconfigure until we've sorted out authentication
		TRAPD(ret, DhcpStateMachine()->StartReconfigureL( this ));
		if (ret!=KErrNone)
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::TaskCompleteL error: %d"),ret));
			}
		iState = EReconfigureInProgress;
#endif
		}
	}


TInt CDHCPIP6Control::HandleClientRequestL(TUint aName, TInt aValue)
	{
	TBool deferAllowed = !iMessage;
	
	if (iDhcpStateMachine)
		{
	   	switch (aName)
			{
			case KConnAddrRenew:
				if (deferAllowed && iDhcpStateMachine->IsGettingCfgInfoOnly())
					{
					iTimer->Cancel();
					iDhcpStateMachine->Cancel();
					iDhcpStateMachine->StartInformL(this,EFalse);
					iState = EInformInProgress;
					return ETrue;
					}
				break;
			default:
				break;
			}
		}
	return CDHCPControl::HandleClientRequestL(aName,aValue);		
	}
	
	
TInt CDHCPIP6Control::HandleClientRequestL(TUint aName)
	{
	return CDHCPIP6Control::HandleClientRequestL(aName,0);
	}

void CDHCPIP6Control::GetRawOptionDataL(TUint aOpCode, TPtr8& aPtr )
	{
	HBufC8* buf = NULL;
	DHCPv6::CDHCPMessageHeaderIP6 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr( const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length() );
	msg.iRecord.ParseL(ptr); //no check necessary
    DHCPv6::COptionNode* pOption = msg.GetOptions().FindOption(aOpCode);
    if (!pOption)
      {
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
      // The option is not found try to get the option through DHCP INFORM message 
      //by calling RequestInformOrCompleteCallL
      CleanupStack::PopAndDestroy();
	  TUint opcode = aOpCode;
	  TPtr8 optPtr(reinterpret_cast<TUint8*>(&opcode),1,1);
	  return(RequestInformOrCompleteCallL(optPtr));
#else	  
	  User::Leave(KErrNotFound);  
#endif //SYMBIAN_TCPIPDHCP_UPDATE	  
      }
    ptr.Set(pOption->GetBodyDes());
    if (ptr.Length() > aPtr.MaxLength())
      {
      User::Leave(KErrOverflow);
      }
    aPtr.Copy(ptr);
    CleanupStack::PopAndDestroy();
	}

/**
    Get raw option data (IP6 version).
    @param  pointer to the buffer descriptor for getting option data
*/
void CDHCPIP6Control::HandleGetRawOptionDataL(TDes8* aDes)
	{
	TDhcp6RawOptionDataPckg pckg(*aDes);
		
	TUint16 opCode = pckg.OpCode();
	TPtr8 buf(pckg.Buf());

	GetRawOptionDataL(opCode, buf);
	pckg.SetBufLengthL(buf.Length());
	}

#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
void CDHCPIP6Control::GetDhcpHdrSiaddrL(TDes8& /*aNxtAddress*/)
	{
	//presently empty implementation.
	//PREQ 1647& 1648 implmentation is only for IPv4
	}
	
void CDHCPIP6Control::GetDhcpHdrSnameL(TDes8& /*aHdrSvrName*/)
	{
	//presently empty implementation.
	//PREQ 1647& 1648 implmentation is only for IPv4
	}
void CDHCPIP6Control::HandleGetTftpServerNameL(TDes8& /*aDes*/)
	{
	//presently empty implementation.
	//PREQ 1647& 1648 implmentation is only for IPv4
	}
void CDHCPIP6Control::HandleGetTftpServerAddrL(TDes8& /*aDes*/)
	{
	//presently empty implementation.
	//PREQ 1647& 1648 implmentation is only for IPv4
	}
#endif
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
void CDHCPIP6Control::RequestInformOrCompleteCallL(TDes8& aOpcodePtr)	
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
		//Cancel if there is any outstanding request(DHCP reconfigure will cancelled)
		iDhcpStateMachine->Cancel();
		static_cast<CDHCPIP6StateMachine*>(iDhcpStateMachine)->StartInformL( this);
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
#endif //SYMBIAN_TCPIPDHCP_UPDATE

#ifdef SYMBIAN_TCPIPDHCP_UPDATE 
void CDHCPIP6Control::HandleGetMultipleParamsL(TDes8& aDes)
#else
void CDHCPIP6Control::HandleGetMultipleParamsL(TDes8& /*aDes */)
#endif //SYMBIAN_TCPIPDHCP_UPDATE
/**
  * This function will be called when the application want to retrieve multiple 
  * options from the server by using option ORO(option 6) 
  *
  * @see RFC 3315 sec 22.7
  * @internalTechnology
  */
	{
#ifdef SYMBIAN_TCPIPDHCP_UPDATE	
	HBufC8 *saveBuf=NULL, *buffer=NULL;
	TUint16 *headerPtr;
	TInt numOfOpcodes=0;
	TInt totalLength=0,opcodeLength=aDes.Length();
	TInt maxLength=aDes.MaxLength();
	TUint16 opcode;
	TBool allFound=ETrue;
	
	DHCPv6::CDHCPMessageHeaderIP6 msgSaved(saveBuf);
	TPtr8 savedPtr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	CleanupClosePushL(msgSaved);
	msgSaved.iRecord.ParseL(savedPtr);

	//The below for loop checks all the required opcode data is present or not
	//The message opcode data is not present in the iValidMsg buffer,the corresponding 
	//opcodes are stored in iCurrentParameters for sending it in DHCPINFORM message
	for(TInt i=0;i<opcodeLength;i++)
		{
		opcode=*(aDes.Ptr()+i); //search required opcode is present or not, one by one
		
		DHCPv6::COptionNode* findNode = msgSaved.GetOptions().FindOption(opcode);
		if (findNode )
			{
			//get the opcode length
			TInt bufLength=findNode->GetItemLength();
			totalLength+=bufLength;
			//The opcode buffer length is greater than maximum length through overflow error
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
			headerPtr = reinterpret_cast<TUint16*>(findNode->Ptr()); 
			++numOfOpcodes;
			//Append the opcode information to the buffer
			buffer->Des().Append(reinterpret_cast<const TUint8*>(headerPtr),bufLength);
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
		if ((totalLength + 1) > maxLength || totalLength<=0)
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
		//If the option is not found then trigger the information request
		TPtr8 opcodePtr(const_cast<TUint8*>(aDes.Ptr()),opcodeLength);
		opcodePtr.SetLength(opcodeLength);
		RequestInformOrCompleteCallL(opcodePtr);	
		}	
	
	if (buffer)
		{
		CleanupStack::PopAndDestroy(buffer);
		}

	CleanupStack::PopAndDestroy(&msgSaved);
#endif //SYMBIAN_TCPIPDHCP_UPDATE
	}

TInt CDHCPIP6Control::InformCompleteRequestHandlerL()
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
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
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

	default:
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("InformCompleteRequestHandlerL : default unhandled optino Name")));
		break;	
	
		}
		iDhcpStateMachine->iDhcpInformAckPending=EFalse;
		CleanupStack::PopAndDestroy(buff);
		return KErrNone;
#else
		return EFalse;
#endif //SYMBIAN_TCPIPDHCP_UPDATE
	}
void CDHCPIP6Control::HandleGetSipServerAddrL(TDes8* aDes)
	{	
	SSipServerAddr* sipServerAddr = 
		reinterpret_cast<SSipServerAddr*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv6::CDHCPMessageHeaderIP6 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);

	DHCPv6::COptionList* optionList = &(msg.GetOptions());
	
	CDHCPOptionSipServerAddrs* sipServerAddrs = 
		static_cast<CDHCPOptionSipServerAddrs*>(optionList->FindOption(ESipServerA));
	
	TBool ret = EFalse;

	if(sipServerAddrs)
		{
		ret = sipServerAddrs->GetSipServerAddr(sipServerAddr->index, 
			sipServerAddr->address);
		}

	User::LeaveIfError(ret ? KErrNone : KErrNotFound);
	CleanupStack::PopAndDestroy();
	}

void CDHCPIP6Control::HandleGetSipServerDomainL(TDes8* aDes)
	{	
	SSipServerDomain* sipServerDomain = 
		reinterpret_cast<SSipServerDomain*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv6::CDHCPMessageHeaderIP6 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);

	DHCPv6::COptionList* optionList = &(msg.GetOptions());
	
	CDHCPOptionSipServerDomains* sipServerDomains = 
		static_cast<CDHCPOptionSipServerDomains*>(optionList->FindOption(ESipServerD));

	if(sipServerDomains)
		{
		CDomainNameCodec* domainNameDecoder = new(ELeave) CDomainNameCodec();
		CleanupStack::PushL(domainNameDecoder);
		TPtr8 ptr = sipServerDomains->GetBodyDes();
		domainNameDecoder->DecodeL(ptr);

		if(sipServerDomain->index < (TInt)domainNameDecoder->NumDomainNames())
			{
			TDomainName domainName = (*domainNameDecoder)[sipServerDomain->index];
			sipServerDomain->domainName.Copy(domainName);
			}
		else
			{
			User::Leave(KErrNotFound);
			}
			
		CleanupStack::PopAndDestroy();
		}
	else
		{
		User::Leave(KErrNotFound);
		}
		
	CleanupStack::PopAndDestroy();
	}
#ifdef SYMBIAN_TCPIPDHCP_UPDATE
void CDHCPIP6Control::HandleGetDomainSearchListL(TDes8* aDes)
/**
  * This function will be called when the application want to retrieve the 
  * list of Domain names from the DomainSearchList option (option 24)by using 
  * KConnGetDomainSearchList variable.
  *
  * @see RFC 3646,
  * @internalTechnology
  */
	{
	SDomainSearchList* domainsearchlist = 
			reinterpret_cast<SDomainSearchList*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv6::CDHCPMessageHeaderIP6 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);

	DHCPv6::COptionList* optionList = &(msg.GetOptions());
	
	CDHCPOptionDomainSearchList* domainsearchlistoption = 
		static_cast<CDHCPOptionDomainSearchList*>(optionList->FindOption(EDomainList));
		
	if(domainsearchlistoption)
		{
		if(domainsearchlist->index >= 0)
			{
			CDomainNameCodec* domainNameDecoder = new(ELeave) CDomainNameCodec();
			CleanupStack::PushL(domainNameDecoder);
			TPtr8 ptr = domainsearchlistoption->GetBodyDes();
			domainNameDecoder->DecodeL(ptr);
		
			if(domainsearchlist->index < (TInt)domainNameDecoder->NumDomainNames())
				{
				TDomainName domainName = (*domainNameDecoder)[domainsearchlist->index];
				domainsearchlist->domainname.Copy(domainName);
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDomainSearchListL -index = %d  domain name  = \"%S\" "), domainsearchlist->index,&(domainsearchlist->domainname)));
				}
			else
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDomainSearchListL: Client tried to fetch an out-of-range domain names %d (I only know about %d)"), domainsearchlist->index , (TInt)domainNameDecoder->NumDomainNames()));
				User::Leave(KErrNotFound);
				}
			
			CleanupStack::PopAndDestroy(domainNameDecoder);
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDomainSearchListL: index is  not >=0 %d "), domainsearchlist->index));
			User::Leave(KErrNotFound);
			}
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDomainSearchListL option is not found ")));
		User::Leave(KErrNotFound);
		}
	
	CleanupStack::PopAndDestroy(&msg);
	}

void CDHCPIP6Control::HandleGetDNSServerListL(TDes8* aDes)
/**
  * This function will be called when the application want to retrieve the 
  * list of IPV6 addresses of DNS recursive name server from the
  * DNS Recursive Name Server option (option 23)by using 
  * KConnGetDNSServerList variable.
  *
  * @see RFC 3646,
  * @internalTechnology
  */	
	{
	SDNSServerAddr* dnsServerAddr = 
			reinterpret_cast<SDNSServerAddr*>(const_cast<TUint8*>(aDes->Ptr()));

	HBufC8* buf = NULL;
	DHCPv6::CDHCPMessageHeaderIP6 msg(buf);
	CleanupClosePushL(msg);
	TPtr8 ptr(const_cast<TUint8*>(iValidMsg.Ptr()), iValidMsg.Length(), iValidMsg.Length());
	msg.iRecord.ParseL(ptr);

	DHCPv6::COptionList* optionList = &(msg.GetOptions());
	
	CDHCPOptionDNSServers* dnsServerAddrsoption = 
		static_cast<CDHCPOptionDNSServers*>(optionList->FindOption(EDNSServers));
	
	TInt ret = KErrNone;
	if(dnsServerAddrsoption)
		{
		if(dnsServerAddr->index >=0)
			{
			ret = dnsServerAddrsoption->GetDomainNameServer(dnsServerAddr->index, 
				dnsServerAddr->addres);
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDNSServerListL -index = %d  DNS server address = \"%S\" "), ret,&(dnsServerAddr->addres)));
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDNSServerListL -index is out of range ")));
			ret = KErrNotFound;
			}
		}
	else
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP6Control::HandleGetDNSServerListL option is not found ")));
		ret = KErrNotFound;	
		}
	
	User::LeaveIfError(ret);
	CleanupStack::PopAndDestroy(&msg);
	
	}
#endif //SYMBIAN_TCPIPDHCP_UPDATE
