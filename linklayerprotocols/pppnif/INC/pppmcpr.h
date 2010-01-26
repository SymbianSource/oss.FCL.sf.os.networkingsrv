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
// PPP MCPR
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_PPPMCPR_H
#define SYMBIAN_PPPMCPR_H

#include <comms-infras/ss_mcprnodemessages.h>
#include <networking/pppconfig.h>
#include <comms-infras/agentmcpr.h>
#include "pppmcprfactory.h"

class CCsdAvailabilityListener;

/**
@internalTechnology
@released Since 9.4

PPP meta connection provider
*/
class CPppAgentNotificationHandler;
NONSHARABLE_CLASS(CPppMetaConnectionProvider) : public CAgentMetaConnectionProvider
    {
public:
    typedef CPppMetaConnectionProviderFactory FactoryType;

	static CPppMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory,
	                                        const ESock::TProviderInfo& aProviderInfo);
	virtual ~CPppMetaConnectionProvider();

protected:
    CPppMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
                               const ESock::TProviderInfo& aProviderInfo,
                               const MeshMachine::TNodeActivityMap& aActivityMap);
    void SetAccessPointConfigFromDbL();

protected:
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);
	static void CleanupCloseIapView(TAny* aThis);
    void ConstructL();
    void StartAvailabilityMonitoringL(const Messages::TNodeCtxId& aAvailabilityActivity);
    void CancelAvailabilityMonitoring();

private:
	CPppAgentNotificationHandler* iAgentHandler;
	CCsdAvailabilityListener* iAvailabilityListener;
    };



NONSHARABLE_CLASS(CPppAgentNotificationHandler) : public CAgentNotificationHandler
   {
public:
   static CPppAgentNotificationHandler* NewL();

   virtual void ConnectCompleteL ();

protected:
   CPppAgentNotificationHandler();
   };



#endif //SYMBIAN_PPPMCPR_H
