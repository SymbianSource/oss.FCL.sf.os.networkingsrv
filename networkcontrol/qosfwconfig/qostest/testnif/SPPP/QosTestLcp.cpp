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


#include "QosTestLcp.h"
#include "QosTestNcp.h"

using namespace ESock;

_LIT(KQoSTestLcpPanicString, "QoSTestLcp");
enum TQoSPppPanicNumber
	{
	EBadGetLowerControProtocol,
	EBadUnbindProtocol,
	EBadBindProtocol
	};

CQosTestLcp* CQosTestLcp::NewL(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
    {
    return new (ELeave) CQosTestLcp(aFactory, aSubConnId, aProtocolIntf);
    }


CQosTestLcp::CQosTestLcp(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf)
  : CPppLcp(aFactory, aSubConnId, aProtocolIntf)
    {
    }

CQosTestLcp::~CQosTestLcp()
    {
    // Binders should have been cleaned up in Unbind().
	ASSERT(iQoSTestNCP4 == NULL);
	ASSERT(iQoSTestNCP6 == NULL);
    }

void CQosTestLcp::NcpDown()
    {
    //The Network has flagged the last NCP going down. commit-suicide.
    //REMEK: INCOMPLETE!! - copy behaviour
    }

// ===========================================================================
//
// MFlowBinderControl methods
//

MLowerControl* CQosTestLcp::GetControlL(const TDesC8& aProtocol)
    {
    MLowerControl* ncpLowerControl = CPppLcp::GetControlL(aProtocol);

    if (aProtocol.CompareF(KProtocol4()) == 0)
    	{
    	return iQoSTestNCP4 = CQoSTestNcp::NewL(this, *ncpLowerControl, EPppIp4);
    	}
    else
    if (aProtocol.CompareF(KProtocol6()) == 0)
    	{
		return iQoSTestNCP6 = CQoSTestNcp::NewL(this, *ncpLowerControl, EPppIp6);
    	}
    else
    	{
    	User::Panic(KQoSTestLcpPanicString, EBadGetLowerControProtocol);
    	}

    return NULL;
    }

MLowerDataSender* CQosTestLcp::BindL(const TDesC8& aProtocol, MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
Override of CPppLcp::BindL() (from MFlowBinderControl::BindL())

This method is here to remember the MUpperControl associated with each IP4/6
CQoSTestNcp instance, so that can identify which instance of the latter to
delete on an Unbind().
*/
	{
    if (aProtocol.CompareF(KProtocol4()) == 0)
    	{
    	ASSERT(iQoSTestNCP4);
    	iQoSTestNCP4->SetUpperControl(aControl);
    	}
    else
    if (aProtocol.CompareF(KProtocol6()) == 0)
    	{
    	ASSERT(iQoSTestNCP6);
    	iQoSTestNCP6->SetUpperControl(aControl);
    	}
    else
    	{
    	User::Panic(KQoSTestLcpPanicString, EBadBindProtocol);
    	}

	return CPppLcp::BindL(aProtocol, aReceiver, aControl);
	}

void CQosTestLcp::Unbind(MUpperDataReceiver* aReceiver, MUpperControl* aControl)
/**
Override of CPppLcp::Unbind() (from MFlowBinderControl::Unbind())

Determine the CQoSTestNcp instance to free up by comparing MUpperControl pointers.
*/
    {
    ASSERT(iQoSTestNCP4 || iQoSTestNCP6);

    if (iQoSTestNCP4 && iQoSTestNCP4->MatchesUpperControl(aControl))
    	{
    	delete iQoSTestNCP4;
    	iQoSTestNCP4 = NULL;
    	}
    else
    if (iQoSTestNCP6 && iQoSTestNCP6->MatchesUpperControl(aControl))
    	{
    	delete iQoSTestNCP6;
    	iQoSTestNCP6 = NULL;
    	}
    else
    	{
    	User::Panic(KQoSTestLcpPanicString, EBadUnbindProtocol);
    	}

    CPppLcp::Unbind(aReceiver, aControl);
    }

// ================================================================================
//
// CPppLcp method overrides
//

void CQosTestLcp::BinderLinkUp(CPppLcp::TPppProtocol aProtocol)
/**
Called when a PPP Binder transmits a progress message.
*/
	{
	switch (aProtocol)
		{
	case EPppIp4:
		ASSERT(iQoSTestNCP4);
    	iQoSTestNCP4->PppNegotiationComplete();
		break;
	case EPppIp6:
		ASSERT(iQoSTestNCP4);
    	iQoSTestNCP6->PppNegotiationComplete();
		break;
	default:
		break;
		}
	}

void CQosTestLcp::QoSBinderLinkUp(CPppLcp::TPppProtocol aProtocol)
    {
    CPppLcp::BinderLinkUp(aProtocol);
    }

void CQosTestLcp::StartFlowL()
	{
	if (iQoSTestNCP4)
		{
    	iQoSTestNCP4->StartPrimaryContext();
		}

	if (iQoSTestNCP6)
		{
    	iQoSTestNCP6->StartPrimaryContext();
		}

	CPppLcp::StartFlowL();
	}

