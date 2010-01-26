/**
* Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* Header file for the Default SubConnection Flow
* 
*
*/



/**
 @file HungryNifBinder.h
*/

#ifndef HUNGRYNIFBINDER_H_INCLUDED_
#define HUNGRYNIFBINDER_H_INCLUDED_
/*
#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <udp_hdr.h>
#include <comms-infras/ss_protflow.h>
#include "es_protbinder.h"*/
#include "HungryNifFlow.h"
#include "Dummynifbinder.h"

/*
class CHungryIfLink : public CDummyIfLink
/**
@internalComponent
*/
/*	{
public:
	CHungryIfLink(CNifIfFactory& aFactory);
    virtual CNifIfBase* GetBinderL(const TDesC& aName);
	};
*/

class CHungryNifSubConnectionFlow;
class CHungryNifBinder4 : public CDummyNifBinder4
/**
@internalComponent
*/
	{
public:
    static CHungryNifBinder4* NewL(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow);

    // from MLowerDataSender
	virtual MLowerDataSender::TSendResult Send(RMBufChain& aData);

	// from MLowerControl
	virtual TInt GetName(TDes& aName);

protected:
	CHungryNifBinder4(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow);
	};

class CHungryNifBinder6 : public CDummyNifBinder6
/**
@internalComponent
*/
	{
public:
    static CHungryNifBinder6* NewL(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow);

    // from MLowerDataSender
	virtual MLowerDataSender::TSendResult Send(RMBufChain& aData);

	// from MLowerControl
	virtual TInt GetName(TDes& aName);

protected:
	CHungryNifBinder6(CHungryNifSubConnectionFlow& aHungryNifSubConnectionFlow);
	};

#endif // HUNGRYNIFBINDER_H_INCLUDED_
