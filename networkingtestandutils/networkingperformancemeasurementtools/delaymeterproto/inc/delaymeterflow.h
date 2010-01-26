// Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Protocol used by netperfte to insert timestamps on incoming packets,
// and record delays of outgoing packets which it reports back to netperfte
// 
//

/**
 @file
 @internalTechnology
*/

#ifndef TIMESTAMPFLOW_H
#define TIMESTAMPFLOW_H

#include <e32property.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_flowbinders.h>

#include <elements/nm_signatures.h>

class CDelayMeterBinder;
namespace DelayMeter { class CSendRecorder; }
namespace DelayMeter { class CReceiveTimestamper; }

class CDelayMeterFlow : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
	{
	friend class CDelayMeterProtoFactory;

protected:
	CDelayMeterFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

public:
	virtual ~CDelayMeterFlow();

	// from CSubConnectionFlowBase:
	virtual ESock::MFlowBinderControl* DoGetBinderControlL();
	virtual void ReceivedL(
		const Messages::TRuntimeCtxId& aSender,
		const Messages::TNodeId& aRecipient,
		Messages::TSignatureBase& aMessage
		);

	// from MFlowBinderControl:
	virtual ESock::MLowerControl* GetControlL(const TDesC8& aProtocol);
	virtual ESock::MLowerDataSender* BindL(const TDesC8& aProtocol, ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual void Unbind( ESock::MUpperDataReceiver* aReceiver, ESock::MUpperControl* aControl);
	virtual ESock::CSubConnectionFlowBase* Flow();

	// my helper methods:
	CDelayMeterBinder* FindOrCreateBinderL(const TDesC8& aProtocol);
private:
	RPointerArray<CDelayMeterBinder> iBinders;
	};


namespace DelayMeter {class TDelayMeterPrefs;}

class CDelayMeterBinder :	public CActive,
			public ESock::MLowerControl, // I receive control requests from above
			public ESock::MLowerDataSender, // I receive data from above (outgoing)
			public ESock::MUpperControl, // I receive control signals from below
			public ESock::MUpperDataReceiver // I receive data from below (incoming)
	{
public:
	// My constructor
	static CDelayMeterBinder* NewL();
	void ConstructL();
protected:
	CDelayMeterBinder();
public:
	virtual ~CDelayMeterBinder();

	// from CActive:
	virtual void DoCancel();
	virtual void RunL();

	// from ESock::MLowerControl:
	virtual TInt GetName(TDes& aName);
	virtual TInt BlockFlow(TBlockOption aOption);
	virtual TInt GetConfig(TBinderConfig& aConfig);
	virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);
	// .. uses my leaving method:
	void ControlL(TUint aName, TDes8& aOption, DelayMeter::TDelayMeterPrefs& prefs);

	// handlers for the possible control calls:
	void StartSendRecorderL(TDes8& aOption, DelayMeter::TDelayMeterPrefs& prefs);
	void GetSendRecorderResultsL(TDes8& aOption, DelayMeter::TDelayMeterPrefs& prefs);
	void StartReceiveTimestamperL(TDes8& aOption, DelayMeter::TDelayMeterPrefs& prefs);
	void GetReceiveTimestamperResultsL(TDes8& aOption, DelayMeter::TDelayMeterPrefs& prefs);


	// from ESock::MLowerDataSender:
	virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);

	// from ESock::MUpperControl:
	virtual void StartSending();
	virtual void Error(TInt anError);

	// from ESock::MUpperDataReceiver:
	virtual void Process(RMBufChain& aData);


	// my own methods
	TBool InUse() const {return iLowerControl || iLowerDataSender ; }
	void BindToUpperL(ESock::MUpperDataReceiver& aUpperDataReceiver, ESock::MUpperControl& aUpperControl);
	TBool UnbindFromUpper(ESock::MUpperDataReceiver& aUpperDataReceiver, ESock::MUpperControl& aUpperControl);
	void BindToLowerFlowL(ESock::MFlowBinderControl& aLowerControl);
	const TDesC8& ProtocolName() const;

	DelayMeter::CSendRecorder* FindSendRecorder(TInt aProtocol, TInt aPortNumber, TBool aRemoveFoundEntry=EFalse);
	DelayMeter::CReceiveTimestamper* FindReceiveTimestamper(TInt aProtocol, TInt aPortNumber, TBool aRemoveFoundEntry=EFalse);

	// very bespoke method to perform mild validation and extract some info, common between send & receive
	TBool ExtractIPv4ProtocolAndPort(RMBufChain& aData, /*returns*/RMBufChain& aPayload, TInt& aProtocol, TInt& aPort, TInt& aLength);

	private:
	ESock::MLowerControl* iLowerControl; 		// .. so I can send controls down
	ESock::MLowerDataSender* iLowerDataSender;		// .. so I can send data down (outgoing)
	ESock::MUpperControl* iUpperControl;		// .. so I can send controls up (err/startsending)
	ESock::MUpperDataReceiver* iUpperDataReceiver;	// .. so I can send data up (incoming)

	RPointerArray<DelayMeter::CSendRecorder> iSendRecorders;
	RPointerArray<DelayMeter::CReceiveTimestamper> iReceiveTimestampers;

	RProperty iProperty; // for controls (til we establish the proper path. pah)

	};



#endif // TIMESTAMPFLOW_H
