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
// Implements the DHCP control plain
// 
//

/**
 @file DHCPControl.cpp
 @internalTechnology
*/

#include "DHCPControl.h"
#include "DHCPStates.h"
#include "DHCPStateMachine.h"
#include "DHCPConfigListener.h"
#include "DNSUpdateIf.h"
#include "DHCPDb.h"
#include "DHCPMsg.h"
#include "NetCfgExtDhcpControl.h"
#include <nifman.h>
#include <comms-infras/es_config.h>
#ifdef _DEBUG
#include "DHCPServer.h"
#endif

#include "DHCPStatesDebug.h"

#ifdef SYMBIAN_NETWORKING_PLATSEC
#include <comms-infras/rconfigdaemonmess.h>
#else
#include <comms-infras\cs_daemonmess.h>
#endif

#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
#include "dhcphwaddrmanager.h"
const TInt KEightBit = 8;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <nifman_internal.h>
#endif


CDHCPControl::~CDHCPControl()
	{
  	CompleteClientMessage( KErrCancel ); // complete all. must be before the rest to avoid deadlock with ESOCK
	delete iDhcpStateMachine;
	delete iTimer;
	iConnection.Close();
	delete iDhcpDb;
	iValidMsg.Close();
	delete iDhcpConfigListener;
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	delete iDNSRawOption;
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION
	delete iDhcpHwAddrManager;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif 	// SYMBIAN_NETWORKING_DHCPSERVER	
	}

//_LIT(KIp6Interface1,"ipcp6");
//_LIT(KIp6Interface2,"eth6");

void CDHCPControl::FindInterfaceNameL(const TConnectionInfo& aInfo, TInt aFamily)
/**
  * Finds the interface name by querying
  * the connection for the name
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::FindInterfaceName")));

	TConnectionInfoBuf connInfo = aInfo;
	// store IAP for later so we can access CommDB
	iDhcpDb->iIapId = connInfo().iIapId;
	User::LeaveIfError(iConnection.Open(iEsock));
	User::LeaveIfError(iConnection.Attach(connInfo, RConnection::EAttachTypeMonitor));
	TInt err = KErrNotFound;	// to ensure we can check that we found connection info
	
	TPckgBuf<TConnInterfaceName> name;
	name().iIndex=1;
	//this is somethig unheard of in OO environment that we cannot get hold of a simple object relations
	//forced to do this horrible stuff
	RSocket socket;
	User::LeaveIfError(socket.Open(iEsock, KAfInet, KSockDatagram, KProtocolInetUdp,iConnection));
	// make socket invisible for interface counting
	User::LeaveIfError(socket.SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0));
	
	CleanupClosePushL( socket );
	TPckgBuf<TSoInet6InterfaceInfo> info;
	while ( err == KErrNotFound && iConnection.Control(KCOLProvider, KConnGetInterfaceName, name) == KErrNone )
		{
		TSoInet6InterfaceInfo& q = info();
		User::LeaveIfError(socket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl, 0));
		do
			{
			User::LeaveIfError(socket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, info));
			}
		while ( q.iName != name().iName );
		
		if ( q.iState == EIfUp &&
			q.iAddress.Family() == KAfInet6 && !q.iAddress.IsV4Mapped() )
			{
			if ( (TUint)aFamily == KAfInet6 )
				{
				err = KErrNone;
				}
			}
		else if ( (TUint)aFamily == KAfInet )
			{
			err = KErrNone;
			}
#if 0
			switch (aFamily)
			{
			case KAfInet6:
				if ( name().iName.FindF( KIp6Interface1 ) != KErrNotFound ||
					name().iName.FindF( KIp6Interface2 ) != KErrNotFound )
					{
					err = KErrNone;
					}
				break;
			case KAfInet:
				if ( name().iName.FindF( KIp6Interface1 ) == KErrNotFound &&
					name().iName.FindF( KIp6Interface2 ) == KErrNotFound )
					{
					err = KErrNone;
					}
				break;
			};
#endif
			name().iIndex++;
		}
	User::LeaveIfError(err);	// make sure we found something useful
	CleanupStack::PopAndDestroy( 1 ); //socket
	iInterfaceName = name().iName;
	}

void CDHCPControl::Cancel()
{
   CompleteClientMessage( KErrCancel ); //cancel all. must be before the rest to avoid deadlock with ESOCK
    if(iDhcpStateMachine)
        {
        iDhcpStateMachine->Cancel();
        }
   
   iInitStartedByRenew = EFalse;
}

TBool CDHCPControl::CompleteClientMessage(TInt aError, TInt aFunctionToCancel)
/** 
  * If necessary then complete client.
  *   If complete is performed, true is returned
  */
	{
	if (iMessage &&
		 !iMessage->IsNull() &&
		  ( aFunctionToCancel == -1 || aFunctionToCancel == iMessage->Function() || EConfigDaemonDeregister == iMessage->Function() ) )
		{
		iMessage->Complete(aError);
		iMessage = NULL;
		return ETrue;
		}
	return EFalse;
	}
	


void CDHCPControl::BindingFinishedL()
   {
   //cancel any pending timer
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::BindingFinishedL")));
	iTimer->Cancel();
	UpdateDns(iDhcpStateMachine->iHostName, iDhcpStateMachine->iDomainName);
	iDhcpStateMachine->iStartedAquisitionAt.HomeTime(); //remember acquisition time
	
	/** A compromise til we have implemented the "lifetime" option (#42) for
	  *  stateless configuration mode - if iRenewalTimeT1 hasn't been read (because we
	  *   didn't read IA information), we don't attempt a renew later.
	  *    This prevents entry to the renew functionality which acts as though we are
	  *     in stateful mode (attempting a full renew / rebind / re-lease)
	  */
	if(iDhcpStateMachine->iRenewalTimeT1)
		{
		//Start renewal timer
		iTimer->After(static_cast<TTimeIntervalSeconds>(iDhcpStateMachine->iRenewalTimeT1), *this);
		}
	SaveMessageBufferForLaterReference();
	ServiceAnyOutstandingIoctlL();
	CompleteClientConfigureMessage(KErrNone);
	DHCP_DEBUG_PUBLISH_READY(DHCPDebug::EReady);
	}
 
 
void CDHCPControl::ServiceAnyOutstandingIoctlL()
	{
	if(iMessage && !iMessage->IsNull() && (iMessage->Function() == EConfigDaemonIoctl || iMessage->Function() == EConfigDaemonDeregister))
		{
		TInt err = KErrNone;
		
		// Run the request again, this time service it (because iMessage is set).  We need
		// to check to make sure the IOCTL is not a renew which has caused a reinitialisation
		// - we don't want to restart the IOCTL!.
		if( ( ( iState != EInitInProgress ) && ( iState != EInformInProgress ) ) || !iInitStartedByRenew )
			{
			TRAP(err,HandleClientRequestL(*iMessage));
			}
		iInitStartedByRenew = EFalse;
		
		CompleteClientIoctlMessage(err);
		}
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
void CDHCPControl::ServiceAnyOutstandingServerIoctlL()
	{
	if(iMessage && !iMessage->IsNull() && iMessage->Function() == EConfigDaemonIoctl)
		{
		// run the request again, this time service it (because iMessage is set)
		TRAPD(err,HandleClientRequestL(*iMessage));
		CompleteServerIoctlMessage(err);
		}
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER			
	
void CDHCPControl::SaveMessageBufferForLaterReference()
	{
	//save the msg buffer in case user wants to retrieve any info
	TPtr8 messageBuf = iDhcpStateMachine->Message()->Message().Des();
	TInt len = messageBuf.Length();
	TInt err = KErrNone;
	
	if ( iValidMsg.MaxLength() < len )
			{
			//we cannot use any other buffer (e.g ~CMessageSender one) unfortunately since the
			//message options are valid over potential renew/rebind initalised manualy, by
			//timeout or reconfigure.
			iValidMsg.Close();
			err = iValidMsg.Create(len);
			}
		//if we cannot create the iValidMsg then, well, bad luck
		if ( err == KErrNone )
			{
			iValidMsg.Copy(messageBuf); 
			}
		else
			{
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::SaveMessageBufferForLaterReference Error %d: couldn't store received message"),err));
			}
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
		if (iDhcpStateMachine->iDhcpInformAckPending && iMessage)
			{
			TRAPD(err,InformCompleteRequestHandlerL());
			CompleteClientIoctlMessage(err);
			iDhcpStateMachine->iDhcpInformAckPending=EFalse;
			}	
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
	if(iDHCPServerImpl)
		{
		if(iDNSRawOption)
			{
			TPtr8 ptr = iDNSRawOption->Des();
			HandleSetRawOptionCodeL(&ptr);
			}
		}
#endif // 	SYMBIAN_NETWORKING_DHCPSERVER
	}
   
   
TBool CDHCPControl::OnCompletion( CStateMachine* aStateMachine )
/**
  * OnCompletion function
  *
  * if this method returns ETrue then aStateMachine deletes itself.
  * In this case it does not ever return ETrue.
  * Called upon completion or when suspended.
  *
  * @see CStateMachine::iSuspendRequest comment
  * @internalTechnology
  */
	{
	TRAPD(err, TaskCompleteL(aStateMachine->LastError()));
	if (err != KErrNone)
		{
		// complete the client if this release was prompted by them
		if(CompleteClientConfigureMessage(err))
			{
			// i.e. only if client is waiting for a configure action (not ioctl)

			if(err != KErrNoMemory )
				{
				// by doing this we will make sockets get errored as there is no src addr
				iDhcpStateMachine->RemoveConfiguredAddress();
				iValidMsg.Close();
				}
			}
		}
	return EFalse; //Always!!!
	}

void CDHCPControl::SaveAndHandleClientRequestL(const RMessage2& aMessage,TUint aOptionName,TInt aValue )
	{
	TInt deferRequest=0;
	
	deferRequest = CDHCPControl::HandleClientRequestL(aOptionName,aValue);
	
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::SaveAndHandleClientRequestL")));
		
	iMessage = &aMessage;
	if( !deferRequest )
		{
		CompleteClientIoctlMessage(KErrNone);
		}
	else
 		{
 		if (deferRequest < KErrNone )
 			{
 			//return the err Value and  complete immediately
			CompleteClientIoctlMessage(deferRequest);
			}
		}
	}
	

void CDHCPControl::HandleClientRequestL(const RMessage2& aMessage)
/**
  * Receives client requests from RConnection.
  * 
  * This is the base implementation that is called
  * to service client requests.  It handles the 
  * reading and writing of data into the message
  * and passes control to the derived implementation
  * to provide the correct info, then completes the message
  * when done.  If there is an error the message
  * is completed by the session.
  *
  * DEFERRING:
  *
  * When the request is first made, and it's possible to defer the request.
  *  This is done by setting iMessage to the message.
  *
  * @internalTechnology
  */
	{
	if (aMessage.Function() == EConfigDaemonDeregister)
		{
		iDhcpDaemonDeregister = ETrue;
		SaveAndHandleClientRequestL(aMessage,KConnAddrRelease);	
		return;	
		}
	
	if(aMessage.Function() != EConfigDaemonIoctl ||
	   iMessage != 0 && iMessage != &aMessage)
		{
		User::Leave(KErrUnknown);
		}
	
	TUint optionName = aMessage.Int1();
	TInt length      = aMessage.Int3();
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
	TInt desLength= aMessage.GetDesLength(2);
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS

#if _DEBUG
    if(optionName & KDhcpInterfaceDbgIoctl)
    	{
    	HandleInterfaceDebugL(aMessage);
    	}
	else
#endif
	if (length>0)
		{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER			
			//Only below commands are supported by DHCP server implementation.
			//Any other commands are not supported.
			if(iDHCPServerImpl)
				{
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
				if(optionName != KConnSetDhcpRawOptionData )
#else
				if((optionName != KConnSetDhcpRawOptionData ) && (optionName != KConnDhcpSetHwAddressParams))
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
					{
					CompleteClientIoctlMessage(KErrNotSupported);
					}
				}
#endif // SYMBIAN_NETWORKING_DHCPSERVER	
		
		if(optionName == KConnAddrRenew)
			{
			//processing renew with user defined timeout
			
			TInt secValue;
			TPckg<TInt> val(secValue);
			aMessage.ReadL(2, val,0);

			SaveAndHandleClientRequestL(aMessage, optionName,secValue);	
			}
		else
			{
			HBufC8* buffer = HBufC8::NewMaxLC(length);
			TPtr8 ptr = buffer->Des();
			aMessage.ReadL(2, ptr);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
			ptr.SetLength(desLength);
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
			
			TInt deferRequest =  HandleClientRequestL(optionName, &ptr);

			iMessage = &aMessage;
			if( !deferRequest )
				{
				// request was serviced.. complete immediately
				aMessage.WriteL(2, ptr);
				CompleteClientIoctlMessage(KErrNone);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
				iDhcpStateMachine->iDhcpInformAckPending=EFalse;
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS					
				}
			else
				{
				if (deferRequest < KErrNone)
					{
					//return the err Value and  complete immediately
					CompleteClientIoctlMessage(deferRequest);
					}
				}	
			CleanupStack::PopAndDestroy(buffer);
			}
		}
	else
		{
		SaveAndHandleClientRequestL(aMessage,optionName);
		}
	}

void CDHCPControl::HandleInterfaceDebugL(const RMessage2& aMessage)
/**
  * Receives client requests for Debug on this interface.
  * 
  * @internalTechnology
  */
	{
#ifdef _DEBUG
//-- perform debug control from the client side.
//-- Enabled for debug builds only.
	TUint optionName = aMessage.Int1();
	TInt length      = aMessage.Int3();
    TInt nResult     = KErrNone;

    if(optionName & KDhcpInterfaceDbgIoctl)
    	{
        
        TPckgBuf<TInt> ctlParamBuf;
        TInt ctlParam = 0;
        
        //-- read IOCTL parameter if appropriate
        if(optionName & KConnReadUserDataBit)
            {
            //-- the parameter should be TUint as it is a debug control parameter
            if(length != static_cast<TInt>(sizeof(TUint)))
    			{
        		nResult = KErrArgument; //-- wrong parameter type
        		}
        	else
        		{
            	aMessage.ReadL(2, ctlParamBuf);
				ctlParam = ctlParamBuf();
				}
			}
		
		if(nResult == KErrNone)
		{
	        //-- perform IOCTL functon
	        switch(optionName)
	        	{
	            case KDHCP_GetPubSubMonitorHandle:    

	                //-- easy handle.. state machine pointer.
	                ctlParam = (TInt)iDhcpStateMachine;

	                nResult = KErrNone;
	            	break;

	            default:
	                nResult = KErrArgument; //-- wrong function
	       	 	}//switch

			//-- write IOCTL result if appropriate
			if(optionName & KConnWriteUserDataBit)
				{
				ctlParamBuf() = ctlParam;
				aMessage.WriteL(2,ctlParamBuf);
				}
			
    	}
		
        aMessage.Complete(nResult);
    	}
#else
	aMessage.Complete(KErrNotSupported);
#endif
	}



void CDHCPControl::ConfigureL(const TConnectionInfo& /*aInfo*/, const RMessage2* aMessage)
/**
  * Open and attach to the RConnection
  *
  * @internalTechnology
  */
	{
	// This ConfigureL is called at the start of the derived class ConfigureL.

	iMessage = aMessage;
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::ConfigureL")));
	
	delete iDhcpStateMachine;
	iDhcpStateMachine = NULL;
	delete iDhcpDb;
	iDhcpDb = NULL;
	
	delete iTimer;
	iTimer = NULL;
	iTimer = CExpireTimer::NewL();
	}

void CDHCPControl::ConfigureL( TBool aStaticAddress )
	{
	// This ConfigureL is called at the end of the derived class ConfigureL.
	// We create the listener here as by now the interface name should be set.
	// Also, this is before the state m/c is started so we are already
	// registered for linklocal events.
	if (!iDhcpConfigListener)
		{
		// we will continue normal processing if we fail on construction in any way
		TRAP_IGNORE(iDhcpConfigListener = CDHCPConfigListener::NewL(iInterfaceName, *this));
		}

	// if we have ipAddrFromServer = EFalse in commDB then we have read the static address 
	// and must therefore inform of static ipaddress to the dhcp server. The ReadCommDbL
	// returns the value of the ipAddrFromServer field
	//iConfigType = EConfigNoIPAddress;
	if ((aStaticAddress) || iConfigType == EConfigNoIPAddress)
		{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER			
		if(iDHCPServerImpl) 
			{
			iDhcpStateMachine->iSvrState = ESvrBinding;
			CompleteClientConfigureMessage(KErrNone);
			}
		else 
			{
#endif // SYMBIAN_NETWORKING_DHCPSERVER		
		// We must find out if any information critical to connection start completion is not yet known..
		if(InformNegotiationIsRequiredForConnectionStartCompletion())
			{
			// .. if that's the case we must try to find it by DHCP before we complete the connection start.
			iDhcpStateMachine->StartInformL(this,aStaticAddress);
			iState = EInformInProgress;
			}
		else
			{
			// .. otherwise we can defer negotiations til a client specifically needs to access an option
			//  (e.g. SIP server address)
			iState = EDeferredInform;
			CompleteClientConfigureMessage(KErrNone);
			}
#ifdef SYMBIAN_NETWORKING_DHCPSERVER			
			}
#endif // SYMBIAN_NETWORKING_DHCPSERVER					
		}
	else
		{
		TTime timeNow;
 		timeNow.HomeTime();
	   	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Lease expires at: %Ld. Time now: %Ld"),iDhcpDb->iLeaseExpiresAt.Int64(),timeNow.Int64()));
 		if (iDhcpDb->iLeaseExpiresAt>timeNow)
			{	
#ifdef _DEBUG
			if (CDHCPServer::DebugFlags() & KDHCP_ForceDiscovery)
				{
				iDhcpStateMachine->StartInitL(this, iConfigType == EConfigIPAddress ? CDHCPStateMachine::ESubsequentCalls : CDHCPStateMachine::EFirstCall);
				iState = EInitInProgress;
				}
			else 
				{
				iDhcpStateMachine->StartRebootL(this);
				iState = EInitInProgress;
				}
#else
			iDhcpStateMachine->StartRebootL(this);
			iState = EInitInProgress;
#endif
			}
		else 
			{
			// lease has already expired or we didn't have one...
			// so we must do discovery but can request the known ip address if there is one...
			iDhcpStateMachine->StartInitL(this, iConfigType == EConfigIPAddress ? CDHCPStateMachine::ESubsequentCalls : CDHCPStateMachine::EFirstCall);
			iState = EInitInProgress;
			}
		}

	DHCP_DEBUG_PUBLISH_READY(DHCPDebug::ENotReady);
	}


TBool CDHCPControl::InformNegotiationIsRequiredForConnectionStartCompletion(void) const
	{
	// We assume that we already know that we don't need to use DHCP to find an ip address
	//  This is reasonable as this method shouldn't be called from a discover/request control path.
	
	// Check if it's DHCP's job to find DNS addresses..
	if(iDhcpStateMachine->iNameServerAddressesFromServer &&
	   ! iDhcpStateMachine->DoesInterfaceKnowAnyDNSServers() )
		{
		iDhcpStateMachine->SetFastTimeoutDuringInform();
		return ETrue;
		}
	
	return EFalse;
	}

TBool CDHCPControl::ShouldInformAfterFailedInit(void)
	{
	return EFalse;
	}

void CDHCPControl::TimerExpired()
/**
  * Called by the timer to signal that the timer has expired
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::TimerExpired()")));

	switch (iState)
		{
		case EInitialised:
     	case EReconfigureInProgress://this enum should really be changed in derived class
                                  //to EInitialised in case 
                                  //!iDhcpStateMachine->IsGettingCfgInfoOnly() so we could assert but....
			//This is renew timeout. Start renew process and rebind timeout.
        {
    
	    	iDhcpStateMachine->Cancel();
	    	iDhcpStateMachine->iMaxRetryCount = 3; 
			TRAPD(err, iDhcpStateMachine->StartRenewL(this,0));  //If server doesn't respond after 3 attempts, a rebind is initiated..Pls not Ipv6 calculates its own retry values
			if (err != KErrNone)
				{
				//This might happen only due to lack of memory. Retry after timeout.
				iTimer->After(TTimeIntervalSeconds(KFailTimeOut), *this);
				return;	
				}
			iState = ERenewInProgress;
   			iTimer->After(TTimeIntervalSeconds(iDhcpStateMachine->iRebindTimeT2 - iDhcpStateMachine->iRenewalTimeT1 + 1), *this);
			break;
        }
		case ERenewInProgress:
			//This is rebind timeout. Start rebind process and final lease timeout.
        {
        	iDhcpStateMachine->Cancel();
			TRAPD(err, iDhcpStateMachine->StartRebindL(this));  //If server doesn't respond after 3 attempts, dhcp moves on to the INIT state..Pls not Ipv6 calculates its own retry values
			if (err != KErrNone)
				{
				//This might happen only due to lack of memory. Retry after timeout.
				iTimer->After(TTimeIntervalSeconds(KFailTimeOut), *this);
				return;	
				}
			//Rebind timeout
			iTimer->After(TTimeIntervalSeconds(iDhcpStateMachine->iLeaseTime - iDhcpStateMachine->iRebindTimeT2 + 1), *this);
			iState = ERebindInProgress;
			iDhcpStateMachine->iTaskStartedAt = 0;
			break;
        }
		case ERebindInProgress:
			//This is lease timeout. 
			//Remove configured address (from TCPIP6 stack) and start discovery process.
        {
			iDhcpStateMachine->RemoveConfiguredAddress();
         	iDhcpStateMachine->Cancel();
         	iValidMsg.Close();
         	iDhcpStateMachine->iMaxRetryCount = 2;
         	TRAPD(err, iDhcpStateMachine->StartInitL(this,CDHCPStateMachine::ESubsequentCalls));
			if (err != KErrNone)
				{
				//This might happen only due to lack of memory. Retry after timeout.
				iTimer->After(TTimeIntervalSeconds(KFailTimeOut), *this);
				return;
				}
			iState = EInitInProgress;
			break;
        }
		default:
			_LIT(KDhcpPanicReason, "Timer expired in unexpected state");
			User::Panic(KDhcpPanicReason, KErrNotSupported);
		}
	}

void CDHCPControl::TaskCompleteL(TInt aError)
/**
  * Signals the end of a task
  * and decides what we should do when
  *
  * @internalTechnology
  */
	{
	// cancel possibly working message sender & socket activity and delete current states
	iDhcpStateMachine->Cancel();
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		         _L8("CDHCPControl::TaskCompleteL (%d) with error = %d") ,
		         iState, aError));
	if ( aError == KErrServerTerminated )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		         _L8("CDHCPControl::TaskCompleteL server terminated => complete client request & going to idle")));
		iState = EEnd;
		CompleteClientConfigureMessage(aError);
		CompleteClientIoctlMessage(aError);
		return;
		}
	switch (iState)
		{
		case EInitInProgress:
			if (KErrTimedOut == aError)
				{
				// Listen for Link Local address.
				// DHCP server is timed out so we unblock our client.
				if (iDhcpConfigListener && iDhcpConfigListener->HaveLinkLocal())
					{
					CompleteClientConfigureMessage(KErrNone);
					// don't complete any outstanding ioctl yet..
					}
				}

			if (iDhcpStateMachine->Idle())
				{
				ServiceAnyOutstandingIoctlL();
				CompleteClientConfigureMessage(KErrNone);
				iState = EEnd;
				return;
				}

			if (iDhcpStateMachine->CompleteClientRequest())	// 'O' flag true in RA
				{
				CompleteClientConfigureMessage(KErrNone);
				iDhcpStateMachine->SetCompleteClientRequestFalse();
				iDhcpStateMachine->StartInformL(this, EFalse);
				return;
				}

			if (KErrNone != aError)
				{
				iDhcpStateMachine->SetLastError(aError);
				if (iDhcpStateMachine->History() & CDHCPState::EBinding)
					{
					// ARP failed, duplicate address found so cannot use assigned one. Send DHCPDECLINE.
					// After decline task being finished, we will complete client request with error.
					iDhcpStateMachine->iMaxRetryCount = 3; 
					iDhcpStateMachine->StartDeclineL(this);
					iState = EDeclineInProgress;

					return;
					}
				//We received either NACK or not any DHCP server replied. Retry it.
				if(ShouldInformAfterFailedInit())
					{
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
					         _L8("CDHCPControl::TaskCompleteL starting Inform because Init failed...")));
					iDhcpStateMachine->StartInformL(this, /*aStaticAddress=*/ ETrue);
					iState = EInformInProgress;
					}
				else
					{
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
					         _L8("CDHCPControl::TaskCompleteL restarting Init because Init failed...")));					
					iDhcpStateMachine->iMaxRetryCount = 2;         
					iDhcpStateMachine->StartInitL(this,CDHCPStateMachine::ESubsequentCalls);
					}

				return;
				}

			// we're bound
         	BindingFinishedL();
			iState = EInitialised;
			break;

		case ERenewInProgress:
			if (KErrNone != aError)
				{
				//Complete client request with error if there is any
				CompleteClientConfigureMessage(aError); //unlikely
				CompleteClientIoctlMessage(aError);
				//Renew process has failed.
				return;
				}

			// we're bound
         	BindingFinishedL();
			iState = EInitialised;
			break;

		case ERebindInProgress:
			if (KErrNone != aError)
				{
				//Complete client request with error if there is any
				CompleteClientConfigureMessage(aError); //unlikely
				CompleteClientIoctlMessage(aError);
				//Renew process has failed. We wait for rebind timer (which is already running)
				//to expire to start rebind process.
				return;
				}

			// we're bound
			iTimer->Cancel();
         	BindingFinishedL();
			iState = EInitialised;
			break;

		case EDeclineInProgress:
			//Decline message has been sent. Retry discovery.
			CompleteClientConfigureMessage(aError);
			iDhcpStateMachine->StartInitL(this,CDHCPStateMachine::ESubsequentCalls);
			iState = EInitInProgress;
			break;

		case EReleaseInProgress:
			if (! iDhcpDaemonDeregister)  
				{
				iDhcpStateMachine->RemoveConfiguredAddress();
				iDhcpDaemonDeregister = EFalse;
				}
			ServiceAnyOutstandingIoctlL();
			CompleteClientConfigureMessage(KErrNone);
			iState = EEnd;
			break;
		case EInformInProgress:
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS
			if ( aError == KErrTimedOut && iDhcpStateMachine->iDhcpInformAckPending)
				{
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, 
		         _L8("CDHCPControl::Inforrm Request TaskCompleteL requests timed out => complete client request & going to idle")));
				iState = EEnd;
				TRAPD(err,InformCompleteRequestHandlerL());
				CompleteClientIoctlMessage(err);
				CompleteClientConfigureMessage(err);
				return;
				}
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS		
			SaveMessageBufferForLaterReference();
			ServiceAnyOutstandingIoctlL();
			CompleteClientConfigureMessage(KErrNone);
			DHCP_DEBUG_PUBLISH_READY(DHCPDebug::EReady);
			iState = EEnd;
			break;
		default:
			__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::TaskCompleteL not supported state %d"),iState));
			User::Leave(KErrNotSupported);
		}

	}

TInt CDHCPControl::HandleClientRequestL(TUint aName, TDes8* aDes)
/**
  * Handles client requests made through RConnection.
  *
  * Clients can obtain the ip address,
  * the dhcp server ip address, the remaining lease time
  * and also renewing and releasing the lease. Any data from
  * any option can also be returned as raw data using the GetDhcpRawOptionData
  * request.
  *
  * returns True if request could not be serviced immediately (so should be
  *  deferred then serviced later)
  *
  * @internalTechnology
  */
	{
	TTime time;
	time.HomeTime();
	TTimeIntervalSeconds secs;
	
	// currently all below options should block client til DHCP negotiation has finished.
	//   We'll service them properly after that.
	//
	// when requests are needed which return immediately, more complex logic will be required
	//  to decide whether to block client or service immediately.
	//
	// This is only here because it's a decision that should be made here rather than in the calling function
	//
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::HandleClientRequestL state: %d"),iState));
	if ( !iMessage && iValidMsg.Length()==0 && iState == EDeferredInform )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::HandleClientRequestL -> deferred inform now triggering...")));
		iDhcpStateMachine->StartInformL(this, /*aStaticAddress=*/ ETrue);
		iState = EInformInProgress;

		return ETrue;
		}

	if ( !iMessage && iValidMsg.Length()==0 && iState == EInitInProgress )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::HandleClientRequestL EInitInProgress...")));

		return ETrue;
		}
	
	// We're not ready to service any of the following ioctls if for some reason we
	// haven't stored the reply from DHCP (e.g. because we timed out waiting for INFORM response)
	//
	// This is more appropriate than letting the sub-handlers throw something less meaningful
	//
	if (iValidMsg.Length() == 0)
		{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER			
		if(!iDHCPServerImpl)
			{
#endif // 	SYMBIAN_NETWORKING_DHCPSERVER		
		User::Leave(KErrNotReady);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER					
			}
		else
			{
#ifndef SYMBIAN_NETWORKING_ADDRESS_PROVISION
			if(aName != KConnSetDhcpRawOptionData && aName != KConnGetDhcpRawOptionData)
#else
			if(aName != KConnSetDhcpRawOptionData && aName != KConnGetDhcpRawOptionData && aName != KConnDhcpSetHwAddressParams)
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
				{
				User::Leave(KErrNotReady);
				}
			}
		
#endif // SYMBIAN_NETWORKING_DHCPSERVER			
		}
	
	if (iDhcpStateMachine)
		{
		switch (aName)
			{
#ifdef SYMBIAN_NETWORKING_DHCPSERVER			
			case KConnSetDhcpRawOptionData:
					{
					HandleSetRawOptionCodeL(aDes);					
					}
				break;
#endif // SYMBIAN_NETWORKING_DHCPSERVER				
			case KConnGetCurrentAddr:
				{
				if (aDes->Length() < static_cast<TInt>(sizeof(TConnectionAddress)))
					{
					User::Leave(KErrArgument);
					}
				TConnectionAddress* ptr = (TConnectionAddress*)aDes->Ptr();
				

				TInetAddr* addrPtr = new (&(ptr->iAddr))TInetAddr(iDhcpStateMachine->iCurrentAddress);
//				ptr->iAddr is just some memory in aDes. There is no guarantee that it will be a
//				valid TInetAddr (or even a valid TDes) so what we do here is just run a constructor
//				on this already valid memory block and we are now guaranteed to have a valid 
//				TInetAddr - NO MEMORY IS ACTUALLY ALLOCATED BY NEW HERE - see base code for more 
//				details

				break;
				}
			case KConnGetServerAddr:
				{
				if (aDes->Length() < static_cast<TInt>(sizeof(TConnectionAddress)))
					{
					User::Leave(KErrArgument);
					}
				TConnectionAddress* ptr = (TConnectionAddress*)aDes->Ptr();
				TInetAddr* addrPtr = new(&(ptr->iAddr))TInetAddr;
//				ptr->iAddr is just some memory in aDes. There is nno guarantee that it will be a
//				valid TInetAddr (or even a valid TDes) so what we do here is just run a constructor
//				on this already valid memory block and we are now guaranteed to have a valid 
//				TInetAddr - NO MEMORY IS ACTUALLY ALLOCATED BY NEW HERE - see base code for more 
//				details
	
				iDhcpStateMachine->GetServerAddress( *addrPtr );
				break;
				}
			case KConnGetAddrLeaseTimeRemain:
				{
			    if (!iDhcpStateMachine->IsGettingCfgInfoOnly())
					{
					time.SecondsFrom(iDhcpStateMachine->iStartedAquisitionAt, secs);
					
					if (aDes->Length()!=static_cast<TInt>(sizeof(TConnectionLeaseInfo)))
						{
						User::Leave(KErrArgument);
						}
					(*(TConnectionLeaseInfo*)aDes->Ptr()).iSecondsRemaining = iDhcpStateMachine->iLeaseTime-secs.Int();
					}
				break;
				}
			case KConnGetDhcpRawOptionData:
			    //-- DHCP option data format is absolutely different for ip4 and ip6 versions.
			    //-- so, handle this request separately in CDHCPIP4Control and CDHCPIP6Control
			    HandleGetRawOptionDataL(aDes);
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS				
			    if (iDhcpStateMachine->iDhcpInformAckPending)
			    	{
			    	return ETrue;
			    	}
#endif//SYMBIAN_NETWORKING_DHCP_MSG_HEADERS					
			    
			    break;
			
			// SIP server/gateway ?? Which?
			case KConnGetSipServerAddr:
				/*
					Different methods of obtaining the SIP server addresses
					are specified for IPv4 vs. IPv6.
					
					RFC3361 - Dynamic Host Configuration Protocol (DHCP-for-IPv4) 
							  Option for Session Initiation Protocol (SIP) Servers							  
							  ! - Not currently implemented
					
					RFC3319 - Dynamic Host Configuration Protocol (DHCPv6) Options
							  for Session Initiation Protocol (SIP) Servers 							
				*/
				
				HandleGetSipServerAddrL(aDes);
				
				break;
		
			case KConnGetSipServerDomain:
				
				HandleGetSipServerDomainL(aDes);
				break;
#ifdef SYMBIAN_TCPIPDHCP_UPDATE			
			case KConnGetDomainSearchList:
				/*
				Extract the list of domain names during name resolution
				Refer : RFC3646 - Dynamic Host Configuration Protocol (DHCPv6) Options
				for Domain Search List option(option 24)							
				*/		
				HandleGetDomainSearchListL(aDes);
				break;	
				
			case KConnGetDNSServerList:
				/*
				Extract the list of IPv6 address of DNS recursive name server
				Refer : RFC3646 - DNS Recursive Name Server option(option code 23)							
				*/		
				HandleGetDNSServerListL(aDes);
				break;	
#endif // SYMBIAN_TCPIPDHCP_UPDATE				
#ifdef SYMBIAN_NETWORKING_DHCP_MSG_HEADERS				
			case KConnGetDhcpHdrSname:
				{
				/*
				Extract the server hostname 
				**/
				GetDhcpHdrSnameL(*aDes);
				break;
				}
				
			case KConnGetDhcpHdrSiaddr:
				{
				/*
				Extract the server IPAddress 
				**/
				GetDhcpHdrSiaddrL(*aDes);
				break;
				}
				
		
			case KConnGetTftpServerAddr:
				{
				/*
				Extract the TFTP Server Address
				**/	
				HandleGetTftpServerAddrL(*aDes);
				if (iDhcpStateMachine->iDhcpInformAckPending)
			    	{
			    	return ETrue;
			    	}
			    break;	
				}
			
			case KConnGetTftpServerName:
				{
				/*
				Extract Tftp Server Name
				**/
				HandleGetTftpServerNameL(*aDes);
				if (iDhcpStateMachine->iDhcpInformAckPending)
			    	{
			    	return ETrue;
			    	}
				break;
				}

			case KConnDhcpGetMultipleParams :
				{
				HandleGetMultipleParamsL(*aDes);
				if (iDhcpStateMachine->iDhcpInformAckPending) //all options found ..copy the message 
					{
					return  ETrue; //make DHCPINFORM
					}
				break;
				}
#ifdef SYMBIAN_NETWORKING_ADDRESS_PROVISION	
			case KConnDhcpSetHwAddressParams:
				{
					//Extract the Hardware address from the received descriptor and preserve it for future reference.
					Uint64 hwAddress = 0;
					TInt length = aDes->Length();
					if(length > KHwAddrLength)
						{
						// Leave the routine if the hwAddress length is not of the standard
						User::Leave(KErrArgument);
						}
					__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("IOCTL Inserting the MAC address")));
					for(TInt index = 0; index < length; index++)
						{
						hwAddress <<= KEightBit;
						hwAddress += (*aDes)[index];
						__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("%x "), (*aDes)[index]));
						}
					iDhcpHwAddrManager->Insert(hwAddress);
				}
	        	break;
#endif //SYMBIAN_NETWORKING_ADDRESS_PROVISION
#endif // SYMBIAN_NETWORKING_DHCP_MSG_HEADERS					
				
			default:
				User::Leave(KErrNotSupported);
			}
		}
		
		return EFalse; // request was serviced
	}


TInt CDHCPControl::HandleClientRequestL(TUint aName, TInt aValue)
	{
/**
  * Handles client requests made through RConnection
  * are handled here.  Currently aValue is used to set the 
  * user defined renew timeout
  * @internalTechnology
  */
  	TBool deferAllowed = !iMessage;
  	TInt  deferred = EFalse;
  
  	if (iDhcpStateMachine)
		{
	   switch (aName)
		   {
		   case KConnAddrRelease:
			   if (deferAllowed && !iDhcpStateMachine->IsGettingCfgInfoOnly())
				   {
				   iTimer->Cancel();
				   iDhcpStateMachine->Cancel();
				   iDhcpStateMachine->iMaxRetryCount = 3;
				   iValidMsg.Close();
				   
				   // Check to see if we need to ask the DHCP server to release the address
				   // or if we have not yet acquired an address and should cancel any operation
				   // in progress.
				   if( ( iState == EInitialised ) || ( iState == EReconfigureInProgress ) )
				       {
					   iDhcpStateMachine->StartReleaseL( this );
					   iState = EReleaseInProgress;
					   deferred = ETrue;
				       }
				   else
				   	   {
					   iState = EEnd;

					   // Listen for Link Local address.
					   // Assignment process has been cancelled so we unblock our client.
					   if (iDhcpConfigListener && iDhcpConfigListener->HaveLinkLocal())
					       {
					  	   CompleteClientConfigureMessage(KErrNone);
						   }
					   }
				   }
			   if (iDhcpStateMachine->IsGettingCfgInfoOnly())
		       		{
		       		deferred = KErrNotSupported;
		       		}
               break;
           case KConnAddrRenew:
	           if (deferAllowed && !iDhcpStateMachine->IsGettingCfgInfoOnly())
		           {
		           iTimer->Cancel();
		           iDhcpStateMachine->Cancel();
			       iDhcpStateMachine->iMaxRetryCount = 3; //If server doesn't respond after 3 attempts, a rebind is initiated..Pls not Ipv6 calculates its own retry values

				   // Check to see if we need to ask the DHCP server to renew the address
				   // or if we have not yet acquired an address and should start
				   // initialisation again (i.e., the lease has been released).
				   if( ( iState == EInitInProgress ) || ( iState == EReleaseInProgress ) || ( iState == EEnd ) )
					   {
			           iDhcpStateMachine->StartInitL(this,iConfigType == EConfigIPAddress ? CDHCPStateMachine::ESubsequentCalls : CDHCPStateMachine::EFirstCall,aValue);
			           iState = EInitInProgress;
			           iInitStartedByRenew = ETrue;
					   }
				   else
				       {
			           iDhcpStateMachine->StartRenewL(this,aValue);
			           iState = ERenewInProgress;
				       }
				   
				   deferred = ETrue;
		           }
		        if (iDhcpStateMachine->IsGettingCfgInfoOnly())
		       		{
		       		deferred = KErrNotSupported;
		       		} 
		       break;
		   default:
			   User::Leave(KErrNotSupported);
		   }
		}
	else
		{
		// ConfigureL must have left before it could create a state machine
		// object, doing this is the only safe way to stop dereferencing
		// a possibly null pointer.
		User::Leave(KErrAbort);
		}
		
	return deferred;
	}
	
TInt CDHCPControl::HandleClientRequestL(TUint aName)
/**
  * Handles client requests made through RConnection
  * are handled here.  These are obtaining the ip address,
  * the dhcp server ip address, the remaining lease time
  * and also renewing and releasing the lease
  *
  * @internalTechnology
  */
	{
	return CDHCPControl::HandleClientRequestL(aName,0);
	}

void CDHCPControl::UpdateDns(TDesC8* aHostName, TDesC8* aDomainName)
/**
  * UpdateDNS function
  *
  * Poke the DNS update dll to perform its
  * dynamic dns update. However, this is DHCP
  * and we can't handle failure cases for DNS 
  * so we ignore any errors as it's not our job
  *
  * @internalTechnology
  */
	{
	__UHEAP_MARK;
    CDnsUpdateIf* pUpdate = NULL;
	TRAPD(ret,pUpdate = CDnsUpdateIf::NewL();
			  pUpdate->Update(iInterfaceName, aHostName, aDomainName));
	if (ret!=KErrNone)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPControl::UpdateDns error: %d"),ret));
		}
	delete pUpdate;
	REComSession::FinalClose();
	__UHEAP_MARKEND;
	}

#ifdef SYMBIAN_NETWORKING_DHCPSERVER
void  CDHCPControl::HandleSetRawOptionCodeL(TDes8* /*aDes*/)
	{
	}
#endif // SYMBIAN_NETWORKING_DHCPSERVER	

