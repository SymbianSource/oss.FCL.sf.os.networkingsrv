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
// IPProtoDeMux  class definitions.
// Also contains classes related to the demux
// 
//

/**
 @file
 @internalComponent
*/


#ifndef IPPROTODEMUX_H
#define IPPROTODEMUX_H

#include <e32base.h>
#include <in_sock.h>

#include <comms-infras/ss_flowbinders.h>
#include "nif.h"

class CIPShimSubConnectionFlow;
class CIPShimIfBase;

class CIPProtoBinder : 
	//interfaces for NIF
	public ESock::MLowerControl, public ESock::MLowerDataSender, 
	//interfaces for the flow below
	public ESock::MUpperControl, public ESock::MUpperDataReceiver
    {
public:
	friend class CIPShimSubConnectionFlow;
	friend class CIPShimIfBase4; 
	friend class CIPShimIfBase6;
	
	static CIPProtoBinder* NewL(CIPShimSubConnectionFlow& aFlow, const TDesC8& aProtocolName);
	void StartL();

	/**
	   Called from the nif above
	*/
	void BindL(CIPShimIfBase* aNif);
	void Unbind();

	/**
	   Called from the flow
	*/
	void BindToLowerFlowL(ESock::MFlowBinderControl& aLowerBinderControl);
	void UnbindFromLowerFlow();
public:

    //-=========================================
    // MUpperDataReceiver methods
    //-=========================================        
 	virtual void Process(RMBufChain& aData);
 
    //-=========================================
    // MUpperControl methods
    //-=========================================        
	virtual void StartSending();
	virtual void Error(TInt anError);
	
    //-=========================================
    // MLowerDataSender methods
    //-========================================= 
	virtual TSendResult Send(RMBufChain& aData);    	

    //-=========================================
    // MLowerControl methods
    //-========================================= 
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(TBlockOption aOption);
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);

    //-========================================= 
	inline CIPShimSubConnectionFlow& Flow();

	void SetProtocolName(const TDesC8& aProtocol);
	const TDesC8& ProtocolName();
	
	
private:	
	CIPProtoBinder(CIPShimSubConnectionFlow& aFlow);
	~CIPProtoBinder();
	
    //-=========================================
    // members
    //-=========================================      	
private:
	ESock::MFlowBinderControl* iBinderControl;
	ESock::MLowerDataSender* iLowerDataSender;
    ESock::MLowerControl*    iLowerControl;
    
    CIPShimSubConnectionFlow& iFlow;
    CIPShimIfBase* iNif;

	TBuf8<0x20> iProtocolName;
	TBool iMarkedForClosure:1;
    };


CIPShimSubConnectionFlow& CIPProtoBinder::Flow()
	{
	return iFlow;
	}

#endif //IPPROTODEMUX_H
