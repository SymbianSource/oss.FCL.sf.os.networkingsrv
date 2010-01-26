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
// Implements a Session of a Symbian OS server for the RConfigDaemon API
// 
//

/**
 @file DHCPSess.cpp
 @internalTechnology
*/

#include "DHCPSess.h"
#include "DHCPIP4Control.h"
#include "DHCPIP6Control.h"
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
#include "DHCPIP4ServerControl.h"
#endif // SYMBIAN_NETWORKING_DHCPSERVER
#include "DHCPServer.h"
#include "DHCPDb.h"
#ifdef SYMBIAN_NETWORKING_PLATSEC
#include <comms-infras/rconfigdaemonmess.h>
#else
#include <comms-infras\cs_daemonmess.h>
#endif
#include "DHCP_Std.h"
#include "NetCfgExtDhcpControl.h"

CDHCPSession::~CDHCPSession()
/**
 *
 * Destructor
 *
 * @internalTechnology
 *
 */
	{
	iDHCPIfs.ResetAndDestroy();

	DHCPServer()->Close(this);
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPSession::~CDHCPSession")));
	}

CDHCPSession::CDHCPSession() :
	iConfigType( CDHCPControl::EConfigToBeDecided )
	{
	}

void CDHCPSession::ServiceL(const RMessage2& aMessage)
/**
 * Called when a message is received from NIFMAN to configure
 * or query the connection
 *
 * @internalTechnology
 * @param	aMessage	Message received from the If
 * @leave Does not leave (As DHCP Server does not provide Error() method)
 */
	{
	TRAPD(r,DoServiceL(aMessage));
	if (r!=KErrNone)
		{
		__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Error: CDHCPSession::DoServiceL left with %d"), r));
		if (!iMessage.IsNull())
			iMessage.Complete(r);
		}
	}

void CDHCPSession::DoServiceL(const RMessage2& aMessage)
/**
 * Called when a message is received from NIFMAN to configure
 * or query the connection. We save a copy of the message so that
 * we can complete it safely later once processing is done, note we 
 * do not store cancel messages here - we only have one message stored
 * at once, and each message is completed before the next is stored.
 *
 * @internalTechnology
 * @param	aMessage	Message received from the If
 * @leave KErrNotSupported or other leave code from ConfigureL, ControL or IoctlL
 */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPSession::ServiceL")));

	switch (aMessage.Function())
		{
		case EConfigDaemonDeregister:
		if ( iDHCPIfs.Count())
			{
			iDHCPIfs[0]->HandleClientRequestL(aMessage);
			}
		else
			{
			aMessage.Complete(KErrNone); //must be before the rest to avoid deadlock with ESOCK
			}
		break;
		case EConfigDaemonConfigure:
			ASSERT(iMessage.IsNull());
			iMessage = aMessage;
			ConfigureL(iMessage);
			break;
		case EConfigDaemonIoctl:
			ASSERT(iMessage.IsNull());
			iMessage = aMessage;
			IoctlL(iMessage);
			break;
		case EConfigDaemonControl://control is used for internal 
         //NetworkConfigurationExtensionDhcp <-> Dhcp server communication
			ControlL(aMessage);
			break;
      	case EConfigDaemonCancel:
			aMessage.Complete(KErrNone); //must be before the rest to avoid deadlock with ESOCK
			for (TInt i=0 ; i < iDHCPIfs.Count() ; ++i)
				{
				iDHCPIfs[i]->Cancel();
				}
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPSession::Cancel completed")));
        	break;
		default:
			ASSERT(iMessage.IsNull());
			iMessage = aMessage;
			User::Leave(KErrNotSupported);
		}
	}

void CDHCPSession::ControlL(const RMessage2& aMessage)
/** control is used for internal 
 *  NetworkConfigurationExtensionDhcp <-> Dhcp server communication
 *
 * @internalComponent
 * @param	aMessage	Message received from the If
 */
   {
   //it could be a dynamic configuration overwriting static commDb setting
   //in case we are to decide wheter to start with IP address acquisition or info only
   TUint optionName = aMessage.Int1();
   switch (optionName)
		{
		case KConnControlConfigureNoIPAddress:
			iConfigType = CDHCPControl::EConfigNoIPAddress;
			aMessage.Complete(KErrNone);
			break;
		case KConnControlConfigureIPAddress:
			iConfigType = CDHCPControl::EConfigIPAddress;
			aMessage.Complete(KErrNone);
			break;
		default:
			{
			User::Leave(KErrNotSupported);
			}
		};
   }

void CDHCPSession::IoctlL(const RMessage2& aMessage)
/**
 * Extracts data from the message to
 * determine which CDHCPIf to query for the DHCP 
 * server address that has configured its interface
 *
 * @internalComponent
 * @leave KErrNotReady, or leave code in HandleClientRequestL
 */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPSession::Ioctl")));

#ifdef __DHCP_HEAP_CHECK_CONTROL
    if(aMessage.Int1() & KDhcpMemDbgIoctl & ~KConnWriteUserDataBit)
    	{
		HandleHeapDebug(aMessage);
		return;
		}
#endif
	if (iDHCPIfs.Count())
		{
		// Send messages to the first control object.
		//  (corresponding to the first value in the IfNetworks commsdat field).
		//
		// This means having IfNetworks "ip,ip6" will allow the client only to call Ioctls on IP4 DHCP
		//      .. and ..    IfNetworks "ip6,ip" will allow the client only to call Ioctls on IP6 DHCP
		//
		// Specifically addressing IP4 and IP6 Ioctls will require rework to the config daemon interface in nifman
		//
		iDHCPIfs[0]->HandleClientRequestL(aMessage);
		}
	else
		{
		User::Leave(KErrNotReady);
		}
	}
	
void CDHCPSession::HandleHeapDebug(const RMessage2& aMessage)
/**
  * Receives client requests for Heap Debug.
  * 
  * Heap Debug Ioctl messages are handled at session level. That allows debug command to be issued
  * immediately after creation of te session.
  * @internalTechnology
  */
	{
//-- perform heap control from the client side.
//-- Enabled for debug builds only.
#ifdef __DHCP_HEAP_CHECK_CONTROL
	TUint optionName = aMessage.Int1();
	TInt length      = aMessage.Int3();
    TInt nResult     = KErrNone;

    if(optionName & KDhcpMemDbgIoctl & ~KConnWriteUserDataBit)
    	{
    
        //-- the parameter should be TUint and it is a heap debug control parameter
        //-- usually it is a counter. 
        if(length > static_cast<TInt>(sizeof(TUint)))
        	{
            nResult = KErrArgument; //-- wrong parameter type
            }
        else
        	{
            //-- read IOCTL parameter
            TDhcpMemDbgParamBuf ctlParamBuf;
            aMessage.Read(2, ctlParamBuf);
            TInt ctlParam = ctlParamBuf();

            //-- perform IOCTL heap control functon
            switch(optionName & ~KDhcpMemDbgIoctl)
            	{
                case KDHCP_DbgMarkHeap:    
                    //-- Mark the start of heap cell checking for the current thread's heap
                    __UHEAP_MARK;
                    nResult = KErrNone;
                	break; 

                case KDHCP_DbgCheckHeap:   
                    //-- Check the current number of allocated heap cells for the current thread's heap. 
                    //-- ctlParam is the expected number of allocated cells at the current nested level 
                    __UHEAP_CHECK(ctlParam);
                    nResult = KErrNone;
                	break; 

                case KDHCP_DbgMarkEnd:
                    //-- Mark the end of heap cell checking at the current nested level for the current thread's heap
                    //-- ctlParam is the number of allocated heap cells expected.
                    __UHEAP_MARKENDC(ctlParam);
                    nResult = KErrNone;
                	break; 

                case KDHCP_DbgFailNext:
                    //-- Simulate a heap allocation failure for the current thread's heap.
                    //-- ctlParam is the rate of failure. If <= 0, reset.
                    if(ctlParam <= 0)
                      __UHEAP_RESET;
                    else
                      __UHEAP_FAILNEXT(ctlParam);
                    nResult = KErrNone;
                	break; 

                case KDHCP_DbgFlags:
                    //-- Simulate different error conditions in DHCP server
					CDHCPServer::DebugFlags() = ctlParam;
                    nResult = KErrNone;
                	break; 
                  
                default:
                    nResult = KErrArgument; //-- wrong function
           	 	}//switch
        	}//if(length > sizeof(TUint))

        aMessage.Complete(nResult);
    	}
#else
	aMessage.Complete(KErrNotSupported);
#endif

	}

void CDHCPSession::ConfigureL(const RMessage2& aMessage)
/**
 * Starts dhcp configuration for
 * the connection specified in the RMessage.
 *
 * @internalComponent
 * @Leave KErrNoMemory If new connection object memory allocation or
 * startup of object fails.
 */
	{
	__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("CDHCPSession::Configure")));

	if (iDHCPIfs.Count())
		{
		User::Leave(KErrInUse);
		}
	else
		{
		TConnectionInfoBuf configInfo;
		aMessage.Read(0, configInfo);
		CreateControlL(configInfo);

		// if this leaves, then the client is notified by the connection
		// start failing (presumably with KErrNoMemory), however there will
		// be a null pointer for the state machine. No probs, we just
		// make sure that any client ioctl requests following their
		// failed connection attempt, start as safe by checking the state machine ptr
		// before handling the client request

		// We only care about the completion status of the first control object,
		//  because this is the only one who can accept ioctls later.
		//
		// This means that with IfNetworks "ip,ip6", connection start will wait for completion of IP4 DHCP
		//     .. and ..        IfNetworks "ip6,ip", connection start will wait for completion of IP6 DHCP
		//
		// It also means having IfNetworks "ip,ip6" will allow the client only to call Ioctls on IP4 DHCP
		//        .. and ..     IfNetworks "ip6,ip" will allow the client only to call Ioctls on IP6 DHCP
		//
		// Specifically addressing IP4 and IP6 Ioctls will require rework to the config daemon interface in nifman
		//
		// So only send a message (for completion purposes) to the last control
		//  object that we create.
		//
		if(iDHCPIfs.Count())
			{
			iDHCPIfs[0]->ConfigureL(configInfo(), &aMessage);
			for(TInt i=1; i<iDHCPIfs.Count(); ++i)
				{
				iDHCPIfs[i]->ConfigureL(configInfo(), 0);
				}
			}
		}
	}

void CDHCPSession::CreateControlL(const TConnectionInfoBuf& aConfigInfo)
/**
  * Create the control objects to handle configuration for the connection.
  *
  * @note In the future when IPv6 support is added, this function will have to
  * read commDB to find out how to create control object
  *
  * @internalTechnology
  */
	{
	CDHCPDb dhcpDb( aConfigInfo().iIapId );
	RArray<int> families;
	CleanupClosePushL(families);
	dhcpDb.GetAddressFamiliesL(families);
#ifdef SYMBIAN_NETWORKING_DHCPSERVER	
	TBool IsServerImpl = EFalse;	
#endif // SYMBIAN_NETWORKING_DHCPSERVER					
	TInt iMax=families.Count();
	for(TInt i=0;i<iMax;++i)
		{
		CDHCPControl * newInst = NULL;  // assigned to null to avoid 'used before initialised' warning from smart armv5 compiler for default case switch
		switch ( families[i] )
			{
			case KAfInet6:
				newInst = new(ELeave)CDHCPIP6Control(DHCPServer()->ESock(),static_cast<CDHCPControl::TConfigType>(iConfigType));
				break;
			case KAfInet:
#ifdef SYMBIAN_NETWORKING_DHCPSERVER
				// Check if the DHCP server implementation is to be used.
				// If CheckIfDHCPServerImplEnabledL() leaves while reading the commsdat entries,
				// then we assume we dont require DHCP server implementation			
				TRAPD(err, IsServerImpl = dhcpDb.CheckIfDHCPServerImplEnabledL());
							
				if(IsServerImpl && err == KErrNone)
					{
					newInst = new(ELeave)CDHCPIP4ServerControl(DHCPServer()->ESock(),static_cast<CDHCPControl::TConfigType>(iConfigType));
					newInst->iDHCPServerImpl = ETrue;
					}
				else
#endif // SYMBIAN_NETWORKING_DHCPSERVER				
				newInst = new(ELeave)CDHCPIP4Control(DHCPServer()->ESock(),static_cast<CDHCPControl::TConfigType>(iConfigType));
				break;
			default:
				__CFLOG_VAR((KLogSubSysDHCP, KLogCode, _L8("Unrecognised address family %d on interface. Aborting."), families[i]));
				User::Leave( KErrNotSupported );
			};
		CleanupStack::PushL(newInst); // in case the array push fails
		iDHCPIfs.AppendL(newInst); // give new object to array
		CleanupStack::Pop(newInst); // now owned by array so remove from cleanup stack
		}
	families.Close();// R class objects should call close before destruction to free allocated resources
	CleanupStack::PopAndDestroy(&families);
	return;
	}
