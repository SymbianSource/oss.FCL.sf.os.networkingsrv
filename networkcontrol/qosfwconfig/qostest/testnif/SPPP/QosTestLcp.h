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
// QosTestLcp
// 
//

/**
 @file
 @internalComponent
*/

#ifndef QOSTESTLCP_H
#define QOSTESTLCP_H

#include <e32base.h>
#include <e32std.h>
#include <e32hashtab.h>
#include <ss_subconnflow.h>
#include "PPPLCP.H"

class CQoSTestNcp;
class CQosTestLcp : public CPppLcp
/**
*/
	{
public:
	static CQosTestLcp* NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);
	virtual ~CQosTestLcp();

	// call from Ncp when it has no context left
	void NcpDown();
	void QoSBinderLinkUp(CPppLcp::TPppProtocol aProtocol);

    //-=========================================================
    // MFlowBinderControl methods
    //-=========================================================
    virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl);
	virtual void Unbind(ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);

protected:
    CQosTestLcp(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

private:
	virtual void StartFlowL();
	void BinderLinkUp(CPppLcp::TPppProtocol aProtocol);

private:
	CQoSTestNcp* iQoSTestNCP4;
	CQoSTestNcp* iQoSTestNCP6;
	};


#endif
// QOSTESTLCP_H
