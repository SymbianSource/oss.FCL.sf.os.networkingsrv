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
 @file DummyNifBinder.h
*/

#ifndef DUMMYNIFBINDER_H_INCLUDED_
#define DUMMYNIFBINDER_H_INCLUDED_

#include <ip4_hdr.h>
#include <ip6_hdr.h>
#include <udp_hdr.h>
#include <comms-infras/ss_protflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <comms-infras/commsdebugutility.h>
#include <e32property.h>
#include "es_protbinder.h"
#include "DummyNifFlow.h"

/*
TODO NW: is there potential commonality that can be placed into a binder base class ?
*/

class CDummyErrorOneShot : public CAsyncOneShot
/**
Class used to asynchronously signal a binder error
*/
	{
	friend class CDummyNifBinder4;		// for iUpperControl
public:
	CDummyErrorOneShot();
	void RunL();
	void Schedule(ESock::MUpperControl* iUpperControl);

private:
	ESock::MUpperControl* iUpperControl;
	};

class CDummyNifSubConnectionFlow;
class CDummyNifFlowTestingSubscriber;

class CDummyNifBinder4 : public CBase, public ESock::MLowerDataSender, public ESock::MLowerControl
/**
IP4 binder
*/
	{
public:
	static CDummyNifBinder4* NewL(CDummyNifSubConnectionFlow& aDummyNifSubConnectionFlow);
	virtual ~CDummyNifBinder4();
	
	MLowerDataSender* Bind(ESock::MUpperDataReceiver& aUpperReceiver , ESock::MUpperControl& aUpperControl);
	void Unbind (ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aUpperControl);

	void BinderReady();
	void ProcessPacket(RMBufChain& aData);

	// from MLowerDataSender
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);
	
	// from MLowerControl
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(ESock::MLowerControl::TBlockOption aOption);
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);

	// Utility functions
	TBool MatchesUpperControl(ESock::MUpperControl* aUpperControl) const;

protected:
	CDummyNifBinder4(CDummyNifSubConnectionFlow& aDummyNifSubConnectionFlow);

private:
    void UpdateHeaders(TInet6HeaderIP4* aIp4, TInet6HeaderUDP* aUdp);
	inline CDummyNifSubConnectionFlow* Flow();

private:
   	TUint32 iLocalAddressBase;
   	TUint32 iLocalAddress;
    CDummyNifSubConnectionFlow& iDummyNifSubConnectionFlow;
	ESock::MUpperDataReceiver* iUpperReceiver;
	ESock::MUpperControl* iUpperControl;
	CDummyErrorOneShot iErrorOneShot;

public:
	CDummyNifFlowTestingSubscriber* iTestSubscriber;

protected:
    __FLOG_DECLARATION_MEMBER;
	};

class CDummyNifBinder6 : public CBase, public ESock::MLowerDataSender, public ESock::MLowerControl
/**
IP6 binder
*/
	{
public:
	static CDummyNifBinder6* NewL(CDummyNifSubConnectionFlow& aDummyNifSubConnectionFlow);
	virtual ~CDummyNifBinder6();
	
	MLowerDataSender* Bind(ESock::MUpperDataReceiver& aUpperReceiver , ESock::MUpperControl& aUpperControl);
	void Unbind (ESock::MUpperDataReceiver& aUpperReceiver, ESock::MUpperControl& aUpperControl);

	void BinderReady();
	void ProcessPacket(RMBufChain& aData);

	// from MLowerDataSender
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);
	
	// from MLowerControl
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(MLowerControl::TBlockOption /*aOption*/);
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);

	// Utility functions
	TBool MatchesUpperControl(ESock::MUpperControl* aUpperControl) const;

protected:
	CDummyNifBinder6(CDummyNifSubConnectionFlow& aDummyNifSubConnectionFlow);

private:
    void UpdateHeaders(TInet6HeaderIP* aIp6, TInet6HeaderUDP* /*aUdp*/);
	void StaticDnsConfiguration(TBinderConfig6& aConfig);
	inline CDummyNifSubConnectionFlow* Flow();

private:
    CDummyNifSubConnectionFlow& iDummyNifSubConnectionFlow;
	ESock::MUpperDataReceiver* iUpperReceiver;
	ESock::MUpperControl* iUpperControl;

protected:
    __FLOG_DECLARATION_MEMBER;

	};

//
// Inlines
//

CDummyNifSubConnectionFlow* CDummyNifBinder4::Flow()
	{
	return &iDummyNifSubConnectionFlow;
	}

CDummyNifSubConnectionFlow* CDummyNifBinder6::Flow()
	{
	return &iDummyNifSubConnectionFlow;
	}

// various things that will get set up on each interface by dummy nifs
// will be added to the address base to make the broadcast address...
const TUint KBroadcastAddressSuffix = 255;
// some arbitrary num to add to the base to give the default gateway machine...
const TUint KDefaultGatewayAddressSuffix = 10;
// some arbitrary num to add to the base to give the secondary dns server...
const TUint KSecondaryDnsAddressSuffix = 11;
// obviously all the above addresses are totally arbitrary to a certain extent... :-)


const TUid KDummyNifTestingPubSubUid={0x10272F42};

NONSHARABLE_CLASS(CDummyNifFlowTestingSubscriber) : public CActive
	{
public:
	TBool IsEnabled()
		{
		return iIsEnabled;
		}

	static TBool ShouldRun(TUint aApId);

	static CDummyNifFlowTestingSubscriber* NewL(CDummyNifSubConnectionFlow& aFlow, TUint aApId)
		{
		CDummyNifFlowTestingSubscriber* ats = new(ELeave)CDummyNifFlowTestingSubscriber(aFlow, aApId);
		CleanupStack::PushL(ats);
		ats->ConstructL();
		CleanupStack::Pop(ats);
		return ats;
		}

	virtual ~CDummyNifFlowTestingSubscriber();

public:
	CDummyNifFlowTestingSubscriber(CDummyNifSubConnectionFlow& aFlow, TUint aApId);
	void ConstructL();
	
	void RunL();
	void DoCancel();

private:
	CDummyNifSubConnectionFlow& iFlow;
	TInt iApId;
	RProperty iProperty;
	TBool iIsEnabled : 1;
	};

#endif // DUMMYNIFBINDER_H_INCLUDED_
