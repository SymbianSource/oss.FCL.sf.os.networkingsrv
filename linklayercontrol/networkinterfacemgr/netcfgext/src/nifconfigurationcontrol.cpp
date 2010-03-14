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
// Implements generic base for NIFMAN side of configuration daemon
// 
//

/**
 @file NIFConfigurationControl.cpp
 @internalAll
*/
#include <comms-infras/ss_log.h>
#include "nifnullconfiguration.h"
#include <comms-infras/nifif.h>
#include <metadatabase.h>
#include <commsdattypeinfov1_1.h>
#include <cdbcols.h>
#include <cdblen.h>

#ifdef __CFLOG_ACTIVE
_LIT8(KNifConfigControlSubTag, "NifConfigControl");
#endif

EXPORT_C CNifConfigurationControl* CNifConfigurationControl::NewL(MNifIfNotify& aNifIfNotify)
/**
 NewL - accesses comm db to find out which derivative of CNifConfigurationControl class 
 to create to handle the subsequent configuration and communication with configurator.
 @internalAll
 @version 0.02
 @param aNifIfNotify[in] represents the connection the instance is dealing with
 @return a new CNifConfigurationControl derivative
**/
	{
	const TUid KUidNifConfigurationIf = {0x101FEBE1}; //interface ID
 	TBuf8<KCommsDbSvrMaxFieldLength> serverName;
	TInt err = aNifIfNotify.ReadDes(TPtrC(KCDTypeNameConfigDaemonManagerName), serverName);
	
	if (err!=KErrNone && err!=KErrNotFound)
		{
		User::Leave(err);
		}
	if (serverName.Length() == 0 || err == KErrNotFound)
  		{
  		__FLOG_STATIC0(KESockSubConnectionTag, KNifConfigControlSubTag,
					   _L8("CNifConfigurationControl::NewL - creating CNifNullConfiguration object"));
		// default behaviour with old CommDB or connection not using config daemons...
  		return new(ELeave)CNifNullConfiguration(aNifIfNotify);
  		}
  	else
  		{
  		__FLOG_STATIC0(KESockSubConnectionTag, KNifConfigControlSubTag,
					   _L8("CNifConfigurationControl::NewL - creating CNifConfigurationIf object"));
    	TEComResolverParams teComResolverParams;
    	teComResolverParams.SetDataType(serverName);
    	TAny* pImplem = REComSession::CreateImplementationL(KUidNifConfigurationIf, 
		_FOFF(CNifConfigurationIf, iDtor_ID_Key), &aNifIfNotify, teComResolverParams);
    	return reinterpret_cast<CNifConfigurationIf*>(pImplem);
 		}
  	}

void CNifNullConfiguration::RunL()
/**
 RunL - called when request completes does nothing
 @internalAll
 @version 0.01
 @see CActive::RunL
**/
	{
	__FLOG(_L8("CNifNullConfiguration::RunL")); 
	}

void CNifNullConfiguration::CancelControl()
/**
 CancelControl - cancels request asynchronously to avoid deadlock
 @internalAll
 @version 0.01
**/
   {
   __FLOG(_L8("CNifNullConfiguration::CancelControl")); 
   }
   
void CNifNullConfiguration::AsyncDelete()
/**
 AsyncDelete - controls deletion asynchronously to avoid deadlock
 @internalAll
 @version 0.01
**/
   {
   __FLOG(_L8("CNifNullConfiguration::AsyncDelete")); 
   // in this case there is no asynchronous stuff to worry about...
   // so we jsut go away nicely.
   delete this;
   }

void CNifNullConfiguration::ConfigureNetworkL()
/**
 ConfigureNetworkL - signals nothing even if it should signal KLinkLayerOpen with KErrNone
 @internalAll
 @version 0.01
**/
	{
	__FLOG(_L8("CNifNullConfiguration::ConfigureNetworkL")); 
	}

void CNifNullConfiguration::LinkLayerDown()
/**
 LinkLayerDown - does nothing.
 @internalAll
 @version 0.01
**/
	{
	__FLOG(_L8("CNifNullConfiguration::LinkLayerDown")); 
	}

void CNifNullConfiguration::LinkLayerUp()
/**
 LinkLayerUp - does nothing.
 @internalAll
 @version 0.01
**/
	{
	__FLOG(_L8("CNifNullConfiguration::LinkLayerUp")); 
	}

void CNifNullConfiguration::Deregister(
	TInt /* aCause */)
/**
 Generates the KConfigDaemonStartingDeregistration and 
 KConfigDaemonFinishedDeregistrationStop progress notifications, 
 this indicates to CNifAgentRef that the NIF is to be stopped.
 @internalAll
**/
	{
	iNifIfNotify->IfProgress(KConfigDaemonStartingDeregistration, KErrNone);
	iNifIfNotify->IfProgress(KConfigDaemonFinishedDeregistrationStop, KErrNone);
	}
	
void CNifNullConfiguration::SendIoctlMessageL(const ESock::RLegacyResponseMsg& aMessage)
/**
 * SendIoctlMessageL forwards Ioctl request to the daemon and activates the AO to wait for response
 * 
 @internalAll
 @version 0.01
 @param aMessage[in] a message to be processed (it's the caller's responsibility to forward just Ioctl
 *                   messages)
**/
	{
	__FLOG(_L8("CNifNullConfiguration::SendIoctlMessageL")); 
   	aMessage.Complete(KErrNotSupported);
	}

void CNifNullConfiguration::DoCancel()
/**
 DoCancel - does nothing
 @internalAll
 @version 0.01
 @see CActive::DoCancel
**/
	{
	__FLOG(_L8("CNifNullConfiguration::DoCancel()"));
	}

void CNifNullConfiguration::EventNotification(TNetworkAdaptorEventType /*aEventType*/, TUint /*aEvent*/, const TDesC8& /*aEventData*/, TAny* /*aSource*/)
/**
 Notification - does nothing.
 @internalAll
 @version 0.01
**/
	{
	__FLOG(_L8("CNifNullConfiguration::EventNotification")); 
	}

