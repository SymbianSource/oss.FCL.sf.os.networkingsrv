// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//

/**
 @file
 @internalTechnology
 @prototype
*/

#include "agentmessages.h"
#include "agentscpr.h"
#include "CAgentAdapter.h"
#include <comms-infras/linkmessages.h>


#ifdef _DEBUG
// Panic category for "absolutely impossible!" vanilla ASSERT()-type panics from this module
// (if it could happen through user error then you should give it an explicit, documented, category + code)
_LIT(KSpecAssert_NifManAgtPrCAgtM, "NifManAgtPrCAgtM");
#endif

using namespace Meta;
using namespace Messages;
using namespace MeshMachine;
using namespace ESock;


START_ATTRIBUTE_TABLE( CAgentProvisionInfo, CAgentProvisionInfo::EUid, CAgentProvisionInfo::ETypeId )
END_ATTRIBUTE_TABLE()

START_ATTRIBUTE_TABLE( CCredentialsConfig, CCredentialsConfig::EUid, CCredentialsConfig::ETypeId )
END_ATTRIBUTE_TABLE()

EXPORT_C CAgentProvisionInfo::~CAgentProvisionInfo()
   {
   iAgentName.Close();
   delete iCredentials;
   }


/**
Used internally by the AgentSCPr when the MCPr has not provided a
CAgentNotificationHandler
*/
CAgentNotificationHandler* CAgentNotificationHandler::NewL()
    {
    return new (ELeave)CAgentNotificationHandler ();
    }


/**
Initialises the CAgentNotificationHandler with the Agent SCPr it should use to communicate
with the Agent
*/
void CAgentNotificationHandler::Initialise (CAgentSubConnectionProvider* aAgentSCPr)
   {
   iAgentSCPr = aAgentSCPr;
   }


/**
C'tor
*/
EXPORT_C CAgentNotificationHandler::CAgentNotificationHandler ()
   {
   }


/**
Appends extension provisioning information to the SCPrs iAccessPointConfig member
*/
EXPORT_C void CAgentNotificationHandler::AppendExtensionL(const Meta::SMetaData* aExtension)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 1));
   RMetaExtensionContainer mec;
   mec.Open(iAgentSCPr->iAccessPointConfig);
   CleanupClosePushL(mec);
   mec.AppendExtensionL(aExtension);
   
   iAgentSCPr->iAccessPointConfig.Close();
   iAgentSCPr->iAccessPointConfig.Open(mec);
   CleanupStack::PopAndDestroy(&mec);
   
   AnonPostMessageToFlow(TCFDataClient::TProvisionConfig(iAgentSCPr->iAccessPointConfig).CRef());
   }

/**
Gets an extension from the SCPrs iAccessPointConfig member
@return A pointer to the extension or NULL if the specified extension type could not be found
*/
EXPORT_C const Meta::SMetaData* CAgentNotificationHandler::GetExtension(const Meta::STypeId& aType) const
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 3));
   return iAgentSCPr->AccessPointConfig().FindExtension(aType);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::ReadPortName (TDes& aPortName)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 4));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->ReadPortName (aPortName);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::ReadIfParams (TDes& aIfParams)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 5));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->ReadIfParams (aIfParams);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::ReadIfNetworks (TDes& aIfNetworks)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 6));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->ReadIfNetworks (aIfNetworks);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::ReadExcessData (TDes8& aBuffer)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 7));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->ReadExcessData (aBuffer);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::ReadNifName (TDes& aNifName)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 8));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->ReadNifName (aNifName);
   }


/**
Reads information via the Agent
*/
EXPORT_C TInt CAgentNotificationHandler::QueryIsDialIn()
   {
   return NotificationToAgent(EFlowToAgentEventTypeQueryIsDialIn, NULL);
   }


/**
*/
EXPORT_C TInt CAgentNotificationHandler::NotificationToAgent (TFlowToAgentEventType aEvent, TAny* aInfo /*=NULL*/)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 9));
   return iAgentSCPr->AgentProvisionInfo()->AgentAdapter()->NotificationToAgent(aEvent, aInfo);
   }



/**
Sends a message to the Flow.
The NodeChannelId will be overwritten properly (taking it from the AgentSCPr)
before sending the message to the Flow.
*/
EXPORT_C TInt CAgentNotificationHandler::PostMessageToFlow(const TRuntimeCtxId& aSender, Messages::TSignatureBase& aMessage)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 10));
   return iAgentSCPr->PostMessageToFlow(aSender, aMessage);
   }

/**
Sends a message to the Flow.
*/
EXPORT_C TInt CAgentNotificationHandler::AnonPostMessageToFlow(const Messages::TSignatureBase& aMessage)
   {
   __ASSERT_DEBUG(iAgentSCPr, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 11));
   return iAgentSCPr->PostMessageToFlow(iAgentSCPr->NodeId(), aMessage);
   }

/**
Default implementation of ConnectCompleteL does nothing
*/
EXPORT_C void CAgentNotificationHandler::ConnectCompleteL ()
   {
   }


/**
Default implementation of NotificationFromFlow will forward the notification
to the agent passing a NULL pointer for the aInfo. If a pointer is really needed
by the agent for the type of event received this method must be overidden. The
overiding method MUST make the call to NotificationToAgent()
*/
EXPORT_C TInt CAgentNotificationHandler::NotificationFromFlow (TFlowToAgentEventType aEvent)
   {
   return NotificationToAgent (aEvent, NULL);
   }


EXPORT_C TInt CAgentNotificationHandler::NotificationFromAgent (TAgentToFlowEventType aEvent, TAny* /*aInfo*/)
   {
   TLinkMessage::TAgentToFlowNotification msg(aEvent);

   // Not returning what the legacy NIFs implementation of Notification() would have returned.
   // May have to re-visit this if it causes problems.
   return PostMessageToFlow(TNodeCtxId(KActivityNull, iAgentSCPr->Id()), msg);
   }


/**
Default implementation of ServiceStarted does nothing
*/
EXPORT_C void CAgentNotificationHandler::ServiceStarted ()
   {
   }

EXPORT_C CCredentialsConfig* CCredentialsConfig::NewLC(ESock::CCommsDatIapView* aIapView)
    {
    CCredentialsConfig* config = new (ELeave) CCredentialsConfig();
    CleanupStack::PushL(config);

	HBufC* buf = NULL;
	TInt err = KErrNone;
	CommsDat::TMDBElementId nameElem;
	CommsDat::TMDBElementId passElem;
	CommsDat::TMDBElementId serviceTableType = aIapView->GetServiceTableType();
	//
	// Initially populate credentials structure with fields read from CommsDat.
	// These fields may be altered later if the agent subsequently retrieves authentication
	// information via a different mechanism (e.g. via a dialog prompt to the user).
	//
	switch(serviceTableType)
    	{
    	case KCDTIdDialOutISPRecord:
    	case KCDTIdLANServiceRecord:
    	    nameElem = KCDTIdIfAuthName;
    	    passElem = KCDTIdIfAuthPass;
    	    break;

    	case KCDTIdOutgoingGprsRecord:
    	    nameElem = KCDTIdWCDMAIfAuthName | serviceTableType;
    	    passElem = KCDTIdWCDMAIfAuthPass | serviceTableType;
    	    break;

    	default:
    	    nameElem = KCDTIdIfAuthName;
    	    passElem = KCDTIdIfAuthPass;
    	}

    err = aIapView->GetText(nameElem, buf);
	if (KErrNone != err)
		{
		__ASSERT_DEBUG(buf == NULL, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 12));
		config->SetUserName(KNullDesC);
		}
	else
		{
		__ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 13));
		config->SetUserName(buf);	// transfers ownership of buf
		buf = NULL;
		}

    err = aIapView->GetText(passElem, buf);
	if (err != KErrNone)
		{
		__ASSERT_DEBUG(buf == NULL, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 14));
		config->SetPassword(KNullDesC);
		}
	else
		{
		__ASSERT_DEBUG(buf, User::Panic(KSpecAssert_NifManAgtPrCAgtM, 15));
		config->SetPassword(buf); // transfers ownership of buf
		buf = NULL;
		}
    return config;
    }


