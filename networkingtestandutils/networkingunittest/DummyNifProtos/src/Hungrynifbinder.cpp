// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Implementation file for the NIF Protocol2
// 
//

/**
 @file HungryNifBinder.cpp
*/

#include <e32std.h>
#include "dummynifvar.h"
#include "hungrynifbinder.h"
#include "HungryNifFlow.h"
#include <es_mbuf.h>

using namespace ESock;

//
// CHungryNifBinder4 //
//

CHungryNifBinder4::CHungryNifBinder4(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow) : CDummyNifBinder4(aHungryNifSubConnectionFlow)
	{}

CHungryNifBinder4* CHungryNifBinder4::NewL(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow)
	{
	return new (ELeave) CHungryNifBinder4(aHungryNifSubConnectionFlow);
	}

TInt CHungryNifBinder4::GetName(TDes& aName)
/**
Called from upper layer to retrieve the binder name.

@param aName populated with name
@return KErrNone on success, else a system wide error code.
*/
	{
    __FLOG(_L8("CHungryNifBinder4:\tGetName()"));

	// This name matches the NIF-based DummyNif to match any potential
	// test code expectations on the name.
	aName.Format(_L("hungrynif[0x%08x]"), this);
	
	return KErrNone;
	}

MLowerDataSender::TSendResult CHungryNifBinder4::Send(RMBufChain& aData)
/**
Entry point for receiving IPv4 outgoing data

@param aData MBuf chain containing data to send
@return an indication of whether the upper layer should block.
*/
	{
	// Acknowledge without actually sending
	aData.Free();
	return ESendAccepted;
	}

//
// CDummyNifBinder6 //
//

CHungryNifBinder6::CHungryNifBinder6(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow) : CDummyNifBinder6(aHungryNifSubConnectionFlow)
	{}

CHungryNifBinder6* CHungryNifBinder6::NewL(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow)
	{
	return new (ELeave) CHungryNifBinder6(aHungryNifSubConnectionFlow);
	}

MLowerDataSender::TSendResult CHungryNifBinder6::Send(RMBufChain& /*aData*/)
	{
	// Acknowledge without actually sending
	return ESendAccepted;
	}

TInt CHungryNifBinder6::GetName(TDes& aName)
/**
Called from upper layer to retrieve the binder name.

@param aName populated with name
@return KErrNone on success, else a system wide error code.
*/
	{
    __FLOG(_L8("CHungryNifBinder6:\tGetName()"));

	// This name matches the NIF-based DummyNif to match any potential
	// test code expectations on the name.
	aName.Format(_L("hungrynif6[0x%08x]"), this);
	
	return KErrNone;
	}

