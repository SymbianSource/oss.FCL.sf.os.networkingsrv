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
// Implements a Session of a Symbian OS server for the test 
// configuration daemon.
// 
//

/**
 @file
 @internalComponent
*/


#include "ConfigSess.h"
#include "ConfigServer.h"
#include "ConfigControl.h"
#include "Config_Std.h"
#include <comms-infras/rconfigdaemonmess.h>

CConfigSession::~CConfigSession()
/**
 * Destructor, closes the server.
 *
 * @internalComponent
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::~CConfigSession Start"));
	
	ConfigServer()->Close(this);

	delete iConfigIf;
	iConfigIf = NULL;
	
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::~CConfigSession End"));
	}

void CConfigSession::ServiceL(
	const RMessage2& aMessage)
/**
 * Called when a message is received from the interface.
 *
 * @internalComponent
 *
 * @param aMessageMessage received from the interface.
 * @leave Leaves from any of the called methods.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ServiceL Start"));

	switch (aMessage.Function())
		{
	case EConfigDaemonConfigure:
		ConfigureL(aMessage);
		break;
    case EConfigDaemonLinkLayerDown:
		LinkLayerDownL(aMessage);
    	break;
    case EConfigDaemonLinkLayerUp:
		LinkLayerUpL(aMessage);
    	break;
    case EConfigDaemonDeregister:
		DeregisterL(aMessage);
    	break;
    case EConfigDaemonProgress:
		ProgressL(aMessage);
    	break;
	case EConfigDaemonIoctl:
		IoctlL(aMessage);
		break;
    case EConfigDaemonCancel:
    	Cancel(aMessage);
        break;
    case EConfigDaemonCancelMask:
		CancelMask(aMessage);
        break;
	default:
		User::Leave(KErrNotSupported);
		break;
		}

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ServiceL End"));
	}

void CConfigSession::ConfigureL(
	const RMessage2& aMessage)
/**
 * This function is used to start the configuration for
 * the connection specified in the RMessage.
 *
 * @internalComponent
 *
 * @param aMessage	
 * @leave KErrInUse if previously called. Leaves from CConfigControl::ConfigureL.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ConfigureL Start"));

	if (iConfigIf)
		User::Leave(KErrInUse);

   	TConnectionInfoBuf configInfo;
   	aMessage.Read(0, configInfo);
	iConfigIf = CreateControlL();
	iConfigIf->ConfigureL(configInfo(), aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ConfigureL End"));		
	}

void CConfigSession::LinkLayerDownL(
	const RMessage2& aMessage)
/**
 * Start of link-layer renegotiation notification.
 *
 * @internalComponent
 *
 * @param aMessage	
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::LinkLayerDown Start"));

	if (!iConfigIf)
		User::Leave(KErrNotReady);

	iConfigIf->LinkLayerDownL(aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::LinkLayerDown End"));		
	}	

void CConfigSession::LinkLayerUpL(
	const RMessage2& aMessage)
/**
 * End of link-layer renegotiation notification.
 *
 * @internalComponent
 *
 * @param aMessage	
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::LinkLayerUp Start"));

	if (!iConfigIf)
		User::Leave(KErrNotReady);

	iConfigIf->LinkLayerUpL(aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::LinkLayerUp End"));		
	}	

void CConfigSession::DeregisterL(
	const RMessage2& aMessage)
/**
 * Asynchronous deregistration request.
 *
 * @internalComponent
 *
 * @param aMessage	
 * @leave KErrNotReady If ConfigureL was not called previously. Leaves from CConfigControl::DeregisterL.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::DeregisterL Start"));

	if (!iConfigIf)
		User::Leave(KErrNotReady);

	iConfigIf->DeregisterL(aMessage.Int0(), aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::DeregisterL End"));		
	}	

void CConfigSession::ProgressL(
	const RMessage2& aMessage)
/**
 * Asynchronous progress notification registration.
 *
 * @internalComponent
 *
 * @param aMessage Message containing the type parameter, the buffer length and the buffer.
 * @leave Leaves from CConfigControl::ProgressL.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ProgressL Start"));

	if (!iConfigIf)
		User::Leave(KErrNotReady);

	iConfigIf->ProgressL(aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::ProgressL End"));		
	}	
	
void CConfigSession::IoctlL(
	const RMessage2& aMessage)
/**
 * Asynchronous request to start an action.
 *
 * @internalComponent
 *
 * @param aMessage Message containing the type parameter and the request status.
 * @leave Leaves from CConfigControl::IoctlL.
 */
	{
	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::IoctlL Start"));

	if (!iConfigIf)
		User::Leave(KErrNotReady);

	iConfigIf->IoctlL(aMessage.Int1(), aMessage);

	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::IoctlL End"));
	}

void CConfigSession::Cancel(
	const RMessage2& aMessage)
/**
 * Asynchronous request to cancel an existing asynchronous operation.
 *
 * @internalComponent
 *
 * @param aMessage Message containing the request status.
 */
	{
  	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::Cancel Start"));

    if (iConfigIf)
      	iConfigIf->CancelRequest();

    aMessage.Complete(KErrNone); 
	    
  	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::Cancel End"));
	}		
	
void CConfigSession::CancelMask(
	const RMessage2& aMessage)
/**
 * Synchronous request to cancel an existing asynchronous operation.
 *
 * @internalComponent
 *
 * @param aMessage Message containing the operation mask and the request status.
 */
	{
  	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::CancelMaskL Start"));

    if (iConfigIf)
      	iConfigIf->CancelMask(aMessage.Int0());

    aMessage.Complete(KErrNone); 
	    
  	__FLOG_STATIC(KConfigLoggingTag1(), KConfigLoggingTag2(), _L("CConfigSession::CancelMaskL End"));
	}		
	
CConfigControl* CConfigSession::CreateControlL()
/**
 * Create the control object to handle configuration for the connection.
 *
 * @internalComponent
 *
 * @return A new CConfigControl object.
 * @leave Leaves from CConfigControl::NewL.
 */
	{
    return CConfigControl::NewL(ConfigServer()->ESock());
	}

