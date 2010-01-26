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
// IPProto demultiplexor implementation
// 
//

/**
 @file
 @internalComponent
*/

#include <nifmbuf.h>
#include "IPProtoDeMux.h"

CIPProtoBinder::CIPProtoBinder(CIPShimSubConnectionFlow& aFlow)
	:iBinderControl(NULL),
	 iLowerControl(NULL),
	 iFlow(aFlow),
	 iNif(NULL)
	{    
	
	}


CIPProtoBinder* CIPProtoBinder::NewL(CIPShimSubConnectionFlow& aFlow, const TDesC8& aProtocolName)
	{
	CIPProtoBinder* me = new (ELeave) CIPProtoBinder(aFlow);
	me->SetProtocolName(aProtocolName);
    return me;
	}
	
void CIPProtoBinder::StartL()
	{
	ASSERT(iNif);
	iNif->StartL();
	}

CIPProtoBinder::~CIPProtoBinder()
	{
	ASSERT(iNif == NULL); // should have been unbound by this stage
	}

void CIPProtoBinder::BindL(CIPShimIfBase* aNif)
	{
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tBindL(%08x)"), this, aNif); 
	ASSERT(iNif == NULL); // shouldn't be binding twice
	iNif = aNif;
	}

void CIPProtoBinder::Unbind()
	{
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tUnbind()"), this); 
	ASSERT(iNif);
	iNif = NULL;
	}

void CIPProtoBinder::BindToLowerFlowL(ESock::MFlowBinderControl& aLowerBinderControl)
	{
	ASSERT(iNif);

	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L8("CIPProtoBinder %08x:\tBindToLowerFlowL(): protocol '%S'"), this, &iNif->ProtocolName()); 

	iBinderControl = &aLowerBinderControl;
	iLowerControl = iBinderControl->GetControlL(iNif->ProtocolName());
	iLowerDataSender = iBinderControl->BindL(iNif->ProtocolName(), this, this);
	}	
	
void CIPProtoBinder::UnbindFromLowerFlow()
	{
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tUnbindFromLowerFlow()"), this); 

	if (!iBinderControl)
		return;
	
	iBinderControl->Unbind(this, this);
	iBinderControl = NULL;

	iLowerControl = NULL;
	iLowerDataSender = NULL;
	}

//-=========================================
// MUpperDataReceiver methods
//-=========================================        
void CIPProtoBinder::Process(RMBufChain& aData)
	{
	if (iNif)
		{
		ASSERT(iNif->iUpperProtocol);
		iNif->iUpperProtocol->Process(aData, reinterpret_cast<CProtocolBase*>(iNif));
		}
	}

//-=========================================
// MUpperControl methods
//-=========================================        
void CIPProtoBinder::StartSending()
	{
	__CFLOG_1(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tStartSending()"), this);
	Flow().BinderReady();	
	iNif->StartSending();
	}
	
void CIPProtoBinder::Error(TInt aError)
	{
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tError(%d)"), this, aError);
	Flow().BinderError(aError, this);
	}
//-=========================================
// MLowerDataSender methods
//-========================================= 
ESock::MLowerDataSender::TSendResult CIPProtoBinder::Send(RMBufChain& aData)
	{
	return iLowerDataSender->Send(aData);
	}

//-=========================================
// MLowerControl methods
//-========================================= 
TInt CIPProtoBinder::GetName(TDes& aName)
	{
	return iLowerControl->GetName(aName);
	}
	
TInt CIPProtoBinder::BlockFlow(TBlockOption aOption)
	{
	__CFLOG_2(KIPProtoTag1, KIPProtoTag2, _L("CIPProtoBinder %08x:\tBlockFlow(%d)"), this, aOption);
	return iLowerControl->BlockFlow(aOption);
	}
TInt CIPProtoBinder::GetConfig(TBinderConfig& aConfig)
	{
	return iLowerControl->GetConfig(aConfig);
	}
	
TInt CIPProtoBinder::Control(TUint aLevel, TUint aName, TDes8& aOption)
	{
	return iLowerControl->Control(aLevel, aName, aOption);
	}

//-========================================= 
void CIPProtoBinder::SetProtocolName(const TDesC8& aProtocol)
	{
	iProtocolName.Copy(aProtocol);
	}


const TDesC8& CIPProtoBinder::ProtocolName()
	{
	ASSERT(iProtocolName.Length());
	return iProtocolName;
	}
