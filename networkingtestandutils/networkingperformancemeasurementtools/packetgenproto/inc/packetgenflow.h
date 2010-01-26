// Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
//  Defines the CPacketGenFlow class
//

/**
 @file
 @internalTechnology
*/

#ifndef PACKETGENFLOW_H
#define PACKETGENFLOW_H

#include <e32property.h>
#include <comms-infras/ss_subconnflow.h>
#include <comms-infras/ss_flowbinders.h>
#include <nifmbuf.h>

#include <elements/nm_signatures.h>


class CPacketGenBinder;


class CPacketGenFlow : public ESock::CSubConnectionFlowBase, public ESock::MFlowBinderControl
	{
	friend class CPacketGenFlowFactory;

protected:
	CPacketGenFlow(ESock::CSubConnectionFlowFactoryBase& aFactory, const Messages::TNodeId& aSubConnId, ESock::CProtocolIntfBase* aProtocolIntf);

public:
	virtual ~CPacketGenFlow();
	
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
	CPacketGenBinder* FindOrCreateBinderL(const TDesC8& aProtocol);
	
private:
	RPointerArray<CPacketGenBinder> iBinders;
	};

	
class CPacketGenBinder :	public CActive,
			public ESock::MLowerControl, // I receive control requests from above
			public ESock::MLowerDataSender, // I receive data from above (outgoing)
			public ESock::MUpperControl, // I receive control signals from below
			public ESock::MUpperDataReceiver // I receive data from below (incoming)
	{
	public:
	// My constructor
		static CPacketGenBinder* NewL(const TDesC8& aProtocolName);
		void ConstructL();
	protected:
		CPacketGenBinder(const TDesC8& aProtocolName);		
	public:
		virtual ~CPacketGenBinder();
		
		// from CActive:
		virtual void DoCancel();
		virtual void RunL();

		// from ESock::MLowerControl:
		virtual TInt GetName(TDes& aName);
		virtual TInt BlockFlow(TBlockOption aOption);
		virtual TInt GetConfig(TBinderConfig& aConfig);
		virtual TInt Control(TUint aLevel, TUint aName, TDes8& aOption);
		// from ESock::MLowerDataSender:
		virtual ESock::MLowerDataSender::TSendResult Send(RMBufChain& aData);
		
		// from ESock::MUpperControl:
		virtual void StartSending();
		virtual void Error(TInt anError);
		
		// from ESock::MUpperDataReceiver:
		virtual void Process(RMBufChain& aData);
		

		// my own methods
		TBool InUse() const {return iLowerControl || iLowerDataSender || iUpperControl || iUpperDataReceiver ; }
		void BindToUpperL(ESock::MUpperDataReceiver& aUpperDataReceiver, ESock::MUpperControl& aUpperControl);
		TBool UnbindFromUpper(ESock::MUpperDataReceiver& aUpperDataReceiver, ESock::MUpperControl& aUpperControl);
		void BindToLowerFlowL(ESock::MFlowBinderControl& aLowerControl);
		const TDesC8& ProtocolName() const;
		
		TBool ExtractIPv4ProtocolAndPort(RMBufChain& aData, /*returns*/RMBufChain& aPayload, TInt& aProtocol, TInt& aPort, TInt& aLength);
		ESock::MLowerDataSender::TSendResult CPacketGenBinder::GeneratePackets();
		void IncrementSequenceNumberAndUpdateChecksum(RMBufChain& aPayloadBufChain,TInt aStampOffset,TUint8* aChecksumPtr);
		
	private:
		ESock::MLowerControl* iLowerControl; 		// .. so I can send controls down
		ESock::MLowerDataSender* iLowerDataSender;		// .. so I can send data down (outgoing)
		ESock::MUpperControl* iUpperControl;		// .. so I can send controls up (err/startsending)
		ESock::MUpperDataReceiver* iUpperDataReceiver;	// .. so I can send data up (incoming)
		TBuf8<10> iProtocolName; // long enough for an acronym one would hope

		RProperty iProperty; // for controls (til we establish the proper path. pah)
		RMBufPacketBase iPacketTemplate; // template of packet to send
		TBool iGenerating; // whether we're currently generating
		
		TInt iFcf;    // fast counter frequency in Hz
		TBool iFcUp;  // whether fast counter counts up
		TUint64 iFcCount; // the fast counter running total
		TUint64 iFinishFc; // the fast counter running total value to stop at
		TUint32 iLatestFc; // last recorded fastcounter value (for working out diffs)
	};



#endif // PACKETGENFLOW_H
