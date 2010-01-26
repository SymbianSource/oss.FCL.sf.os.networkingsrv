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
// tunnelnif.h 
// 
//

/**
 @file 
 @internalComponent
*/

#if (!defined TUNNELBINDERS_H__)
#define TUNNELBINDERS_H__

#include <comms-infras/nifif.h>
#include <in6_if.h>
#include <eui_addr.h>	// TE64Addr
#include <comms-infras/ss_flowbinders.h>
#include <comms-infras/ss_nodemessages.h>
#include "tunnelFlow.h"				// for CTunnelNcp::Info()

using namespace ESock;

const TInt KTunnelMtu = 1500;

class CTunnelFlow;

NONSHARABLE_CLASS(CTunnelNcp) : public CBase, public ESock::MLowerDataSender, public ESock::MLowerControl
	{
public:
	// from MLowerControl
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(ESock::MLowerControl::TBlockOption /*aOption*/);

	// from MLowerDataSender
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aPdu);

	// Utility functions called from CTunnelFlow
	ESock::MLowerDataSender* Bind(ESock::MUpperDataReceiver* aUpperReceiver, ESock::MUpperControl* aUpperControl);
	void Unbind(ESock::MUpperDataReceiver* aUpperReceiver, ESock::MUpperControl* aUpperControl);
	TBool MatchesUpperControl(const ESock::MUpperControl* aUpperControl);
	void StartSending();

protected:
	CTunnelNcp(CTunnelFlow& aFlow);
	inline const TTunnelInfo* Info();

protected:
	CTunnelFlow* iFlow;
	
    TInetAddr iNameSer1;
	TInetAddr iNameSer2;

	ESock::MUpperControl* iUpperControl;
	ESock::MUpperDataReceiver* iUpperReceiver;
	};

// ======================================================================================

NONSHARABLE_CLASS(CTunnelNcp4) : public CTunnelNcp
	{
public:
	static CTunnelNcp4* ConstructL(CTunnelFlow& aLink);

	// from MLowerDataSender
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint, TUint, TDes8&);
	//

	virtual TInt Notification(TTunnelAgentMessage::TTunnelSetAddress& aMessage);

private:
	CTunnelNcp4(CTunnelFlow& aLink);

	TUint32 iLocalAddress;
	};

// ======================================================================================

NONSHARABLE_CLASS(CTunnelNcp6) : public CTunnelNcp
	{
public:
	static CTunnelNcp6* ConstructL(CTunnelFlow& aLink);

	// from MLowerDataSender
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint, TUint, TDes8&);
	//

	virtual TInt Notification(TTunnelAgentMessage::TTunnelSetAddress& aMessage);

private:
	CTunnelNcp6(CTunnelFlow& aLink);

	TInetAddr iLocalAddress;
	};

// ======================================================================================

class CTunnelNcpLog : public CBase
	{
public:
	static void Write(const TDesC& aDes);
	static void Printf(TRefByValue<const TDesC> aFmt, ...);
	};

//
// Inline functions
//

inline const TTunnelInfo* CTunnelNcp::Info()
	{
	return iFlow->Info();
	}

#endif
