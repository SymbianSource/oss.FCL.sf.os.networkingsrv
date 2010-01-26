// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implements the DHCP IP4 Server control class which controls the upper states
// The Upper state machine for DHCP server are
// - Wait For Discover or Inform
// - Wait For Discover / release / decline / renew and rebind request msgs
// 
//

/**
 @file DHCPIP4ServerControl.cpp
 @internalTechnology
*/

#include "DHCPIP4ServerControl.h"
#include "DHCPIP4Msg.h"
#include "DHCPDb.h"
#include "ExpireTimer.h"
#include "DHCPConfigListener.h"
#include "DHCPStatesDebug.h"

//Dns proxy related includes
#ifdef SYMBIAN_DNS_PROXY
#include <dnsproxyupdateif.h>
#endif // SYMBIAN_DNS_PROXY

CDHCPIP4ServerControl::~CDHCPIP4ServerControl()
	{	
#ifdef SYMBIAN_DNS_PROXY
    __CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4ServerControl::~CDHCPIP4ServerControl()")));
    iDnsProxySession.Close();
    delete iDnsProxyPlugin;
#endif // SYMBIAN_DNS_PROXY
	}


void CDHCPIP4ServerControl::ConfigureL(const TConnectionInfo& aInfo, const RMessage2* aMessage)
/**
  * Open and attach to the RConnection
  *
  * @internalTechnology
  */
	{
#ifdef SYMBIAN_DNS_PROXY
	//configure Dns Proxy Server
	ConfigureDnsProxyL(aMessage);
#endif // SYMBIAN_DNS_PROXY
	// use the base class functionality
	CDHCPIP4Control::ConfigureL(aInfo,aMessage);
	// bind the server socket with the static IP address from Comms database
	// we use a bind state  for server to implement timer and continue in the bind state
	// unless the tcpip6 is successful in binding the socket with the IP address
	iDhcpStateMachine->InitServerBinding(this);

	iDhcpStateMachine->iSvrState = ESvrBinding;
	iDhcpStateMachine->SetServerState(ETrue);

#ifdef SYMBIAN_DNS_PROXY
	//read DNS proxy entries from database.should not leave even if host name is not available
	TRAPD(err,iDhcpDb->ReadHostNameL(*iDhcpStateMachine));
	if(err!= KErrNone)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4ServerControl::ConfigureL().Host name not found")));
		}

	if(iDhcpStateMachine->iProxyDomainName.Length())
	    {
	    TRequestStatus aStatus;
		iDnsProxySession.UpdateDomainName(iDhcpStateMachine->iProxyDomainName, aStatus);
	    User::WaitForRequest(aStatus);
	    }

#endif // SYMBIAN_DNS_PROXY

	}

void CDHCPIP4ServerControl::TimerExpired()
/**
  * Called by the timer to signal that the timer has expired
  *
  * @internalTechnology
  */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPIP4ServerControl::TimerExpired()")));

	iDhcpStateMachine->Cancel();
	iDhcpStateMachine->InitServerStateMachineL(this);
	// bind the socket with the session
	iDhcpStateMachine->BindServerInterface();

	// There is a only rebind timer for this simplified DHCP Server implementation
	// After the discover or renew-request a timer until the rebind time is run
	// We service only a single client and the server does not check if the client
	// failed with DAD. Hence if the timer expired after rebind timer then we know we
	// no longer service any client , so we go back to original state (ESvrWaitForDiscoverInform)

	if(iDhcpStateMachine->iSvrState == ESvrWaitForAnyDHCPMsgs)
		{
	   	iDhcpStateMachine->iSvrState = ESvrWaitForDiscoverInform;
		}
	else
		{
		_LIT(KDhcpPanicReason, "Timer expired in unexpected state");
		User::Panic(KDhcpPanicReason, KErrNotSupported);
		}
	}

void CDHCPIP4ServerControl::TaskCompleteL(TInt aError)
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
		         _L8("CDHCPIP4ServerControl::TaskCompleteL (%d) with error = %d") ,
		         iDhcpStateMachine->iSvrState, aError));

	// cancel possibly working message sender & socket activity
	// and delete current states cancels the timer
	iDhcpStateMachine->Cancel();
	iTimer->Cancel();
	// we re-intialise the server state to wait for next message (in any state)
	iDhcpStateMachine->InitServerStateMachineL(this);

	if ( aError == KErrServerTerminated )
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode,
		         _L8("CDHCPServerControl::TaskCompleteL server terminated => complete client request & going to idle")));
		iDhcpStateMachine->iSvrState = ESvrEnd;

		CompleteServerConfigureMessage(aError);
		CompleteServerIoctlMessage(aError);
		// we cant wait for client messages any more as the server has terminated
		return;
		}

	switch (iDhcpStateMachine->iSvrState)
		{
		case ESvrBinding:
			// we are done with binding the server socket, now start waiting for client messages
			iDhcpStateMachine->iSvrState = ESvrWaitForDiscoverInform;
			if(iDNSRawOption)
				{
				TPtr8 ptr = iDNSRawOption->Des();
				iDhcpStateMachine->SetDNSInformation(&ptr);
				}

#ifdef SYMBIAN_DNS_PROXY
			//update DNS Proxy database
			iInterfaceAddr = iDhcpStateMachine->GetListenerAddress();
			iInterfaceAddr.Output(iAddrStr);
			if((iInterfaceAddr.Address()) && (iDhcpStateMachine->iProxyHostName.Length()))
                {
                TRequestStatus aStatus;
                iDnsProxyPlugin->AddDbEntry(iDhcpStateMachine->iProxyHostName, iAddrStr, aStatus);
                User::WaitForRequest(aStatus);
                }
			iUpdateDnsProxyDb = ETrue;

#endif // SYMBIAN_DNS_PROXY
			break;

		case ESvrWaitForDiscoverInform:
			// some error , go back to intial state (ESvrWaitForDiscoverInform)
			if (KErrNone != aError)
				{
				// some error, reset to intial server state
				iDhcpStateMachine->iSvrState = ESvrWaitForDiscoverInform;
				}
			else
				{
				// if inform msg processed, go back to initial state
				if(iDhcpStateMachine->iSvrSpecificState == ESvrInformInProgress)
					{
					iDhcpStateMachine->iSvrState = ESvrWaitForDiscoverInform;
#ifdef SYMBIAN_DNS_PROXY
					iDhcpStateMachine->iClientStaticAddr.Output(iAddrStr);
                    if((iDhcpStateMachine->iClientStaticAddr.Address()) && (iDhcpStateMachine->iClientHostName.Length()))
                        {
                        TRequestStatus aStatus;
                        iDnsProxyPlugin->AddDbEntry(iDhcpStateMachine->iClientHostName, iAddrStr, aStatus);
                        User::WaitForRequest(aStatus);
                        }
					
#endif // SYMBIAN_DNS_PROXY
    				}
				else
					{
					// discover was successful, so we now wait for renew, rebind,release, decline
					// or a discover msg from another client (just in case)

#ifdef SYMBIAN_DNS_PROXY
					if(iUpdateDnsProxyDb )
                    	{
                    	iOfferedAddr.SetAddress(iDhcpStateMachine->iOfferedAddress);
						iOfferedAddr.Output(iAddrStr);

                        if((iOfferedAddr.Address()) && (iDhcpStateMachine->iClientHostName.Length()))
                            {
                            TRequestStatus aStatus;
                            iDnsProxyPlugin->AddDbEntry(iDhcpStateMachine->iClientHostName, iAddrStr, aStatus);
                            User::WaitForRequest(aStatus);
                            }

						iUpdateDnsProxyDb = EFalse;
		      	   		}
#endif // SYMBIAN_DNS_PROXY

					iDhcpStateMachine->iSvrState = ESvrWaitForAnyDHCPMsgs;
					iTimer->After(static_cast<TTimeIntervalSeconds>(KDefaultLeaseTime/2 + KDefaultLeaseTime/4), *this);
					}
				iDhcpStateMachine->BindServerInterface();
				SaveMessageBufferForLaterReference();
				DHCP_DEBUG_PUBLISH_READY(DHCPDebug::EReady);
				}

			break;
		// waiting for any DHCP messages like Discover, Renew and rebind requests,
		// decline release except inform message
		case ESvrWaitForAnyDHCPMsgs:
			if (KErrNone != aError)
				{
				//Complete client request with error if there is any
				CompleteServerIoctlMessage(aError);
				iDhcpStateMachine->iSvrState = ESvrWaitForAnyDHCPMsgs;
				}
			else
				{
				switch(iDhcpStateMachine->iSvrSpecificState)
					{
				case ESvrDiscoverInProgress:
				case ESvrRenewInProgress:
				case ESvrRebindInProgress:

					iDhcpStateMachine->iSvrState = ESvrWaitForAnyDHCPMsgs;
					iTimer->After(static_cast<TTimeIntervalSeconds>(KDefaultLeaseTime/2 + KDefaultLeaseTime/4), *this);

					iDhcpStateMachine->BindServerInterface();
					SaveMessageBufferForLaterReference();
					DHCP_DEBUG_PUBLISH_READY(DHCPDebug::EReady);
					break;

				case ESvrReleaseInProgress:
				case ESvrDeclineInProgress:
					// go back to intial state, client released ..

	#ifdef SYMBIAN_DNS_PROXY
					iDhcpStateMachine->iClientStaticAddr.Output(iAddrStr);

					TRequestStatus aStatus;
					iDnsProxyPlugin->RemoveDbEntry(iAddrStr, aStatus);
					User::WaitForRequest(aStatus);

					iUpdateDnsProxyDb = ETrue;
	#endif // SYMBIAN_DNS_PROXY

					iDhcpStateMachine->iSvrState = ESvrWaitForDiscoverInform;
					break;
					}
				}
			break;

		default: // some unknown state may be, so reset the state to receive any message (<- unlikely)
			iDhcpStateMachine->iSvrState = ESvrWaitForAnyDHCPMsgs;
		}
	}
	
#ifdef SYMBIAN_DNS_PROXY	
void CDHCPIP4ServerControl::ConfigureDnsProxyL(const RMessage2* aMessage)
	{
	//load DNS Proxy Server
    TConnectionInfoBuf configInfo;
    TRequestStatus aStatus;
	
	aMessage->Read(0, configInfo);
	User::LeaveIfError(iDnsProxySession.Connect());
	
	iDnsProxySession.ConfigureDnsProxyServer(configInfo, aStatus);
    User::WaitForRequest(aStatus);
	User::LeaveIfError(aStatus.Int());
	
	TUid uid = {0x200215F5};  
	//load proxy plugin  
    TRAPD(err, iDnsProxyPlugin = CDNSProxyUpdateIf::NewL(uid));
    User::LeaveIfError(err);
    }
    
#endif // SYMBIAN_DNS_PROXY
