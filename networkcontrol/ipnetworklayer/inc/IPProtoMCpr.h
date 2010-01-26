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
// IPProto MCPR
// 
//

/**
 @file
 @internalComponent
*/

#ifndef SYMBIAN_IPPROTOMCPR_H
#define SYMBIAN_IPPROTOMCPR_H

#include <comms-infras/ss_coreprstates.h>
#include <comms-infras/coremcpr.h>
#include <comms-infras/coremcprstates.h>
#include "ItfInfoConfigExt.h"
#include "IPProtoMCprFactory.h"

//
//CIPProtoMetaConnectionProvider
namespace CommsDat
{
	class CMDBSession;
}
NONSHARABLE_CLASS(CIPProtoMetaConnectionProvider) : public CCoreMetaConnectionProvider
    {
    friend class CIPProtoMetaConnectionProviderFactory;

protected:
	//NewL only for the factory
    static CIPProtoMetaConnectionProvider* NewL(ESock::CMetaConnectionProviderFactoryBase& aFactory,
	                                            const ESock::TProviderInfo& aProviderInfo);

    CIPProtoMetaConnectionProvider(ESock::CMetaConnectionProviderFactoryBase& aFactory,
                                   const ESock::TProviderInfo& aProviderInfo,
                                   const MeshMachine::TNodeActivityMap& aActivityMap);

	virtual ~CIPProtoMetaConnectionProvider();
	void ConstructL();
	virtual void ReceivedL(const Messages::TRuntimeCtxId& aSender, const Messages::TNodeId& aRecipient, Messages::TSignatureBase& aMessage);

private:
	void SetConfigL(); //Reads idle timers, interface info, etc from CommsDat.

public:
	TBool iIapLocked;
    };

namespace IPProtoMCprActivities
{
	//Activity Map provided by IPProtoMCPr to be used by MCprs.
	DECLARE_ACTIVITY_MAP(ipProtoActivitiesMCpr)
}

#endif //SYMBIAN_IPPROTOMCPR_H


