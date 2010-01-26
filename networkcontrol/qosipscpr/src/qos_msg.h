/**
* Copyright (c) 2004-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* Header file for the QoS Mapping Messages
* 
*
*/



/**
 @file qos_msg.h
*/

#ifndef __QOS_MSG_H__
#define __QOS_MSG_H__

#include <e32base.h>
#include <e32std.h>
#include <comms-infras/eintsock.h>
#include <networking/qoslib.h>
#include <elements/nm_messages_base.h>

class CPfqosStream;

class TQoSIpSCprMessages
/**
QoSIpSCpr message realm (messages specific to qosipscpr)
 

@internalComponent
*/
	{
  public:
    enum { ERealmId = 0x10204307 };

  private:
  	enum
  	    {
        EOpenInternalSocket = Messages::KNullMessageId + 1,
		EInternalSocketOpened
	    };

  public:

	typedef Messages::TMessageSigVoid<EOpenInternalSocket, TQoSIpSCprMessages::ERealmId> TOpenInternalSocket;
	typedef Messages::TMessageSigVoid<EInternalSocketOpened, TQoSIpSCprMessages::ERealmId> TInternalSocketOpened;
	};


NONSHARABLE_CLASS(CQoSMsg) : public CBase
/**
Encapsulation of a QoS PRT Message that can be stored in a 
link list of messages.

@internalComponent

@released Since v9.0
*/
	{
public:
	// Construction
	static CQoSMsg* NewL( TPfqosMessages aMsgType );
	~CQoSMsg();

	// Message Content
	inline void AddConnInfo(TUint32 aProtocol, const TUidType& aUid, TUint32 aIapId );
	void AddSrcAddr(const TInetAddr &aAddr);
	void AddDstAddr(const TInetAddr &aAddr);
	inline void AddChannel(TInt aChannelId);
	inline void AddQoSParameters(const TQoSParameters& aParameters);
	void AddExtensionPolicy(TQoSExtensionQueue& aExtensions);

	// Send Message to PRT
	void Send(RInternalSocket &aSocket, TRequestStatus& aStatus);

protected:
	// Construction
	CQoSMsg();
	void ConstructL( TPfqosMessages aMsgType );

public:
	/** QoS PRT formatted Message */
	CPfqosStream* iMsg;

	/** Message Type */
	TPfqosMessages iType;

	/** Link to next item */
	TSglQueLink iLink;
	};


class CIpSubConnectionProvider;

NONSHARABLE_CLASS(CQoSSocketOpener) : public CActive
/**
  Active object for asynchronously opening an Internal Socket

  @internalComponent
  @prototype
*/
	{
public:
	static CQoSSocketOpener* NewL(CIpSubConnectionProvider& aIpScpr, Messages::TNodeCtxId& aOriginator);
	~CQoSSocketOpener();

	void Open();
	
protected:
	CQoSSocketOpener(CIpSubConnectionProvider& aIpScpr, Messages::TNodeCtxId& aOriginator);
	
	// Active Object Implementation
	void RunL();
	void DoCancel();
	
private:
	CIpSubConnectionProvider& iIpScpr;
	Messages::TNodeCtxId iOriginator;
	};

NONSHARABLE_CLASS(CQoSMsgWriter) : public CActive
/**
Active Object that sends messages from IP Connection Provider to the QoS PRT

@internalComponent

@released Since v9.0
*/
	{
public:
	// Construction
	static CQoSMsgWriter* NewL(CIpSubConnectionProvider* aOwner, RInternalSocket& aSocket);
	~CQoSMsgWriter();

	// Send Message over Internal Socket
	void Send(CQoSMsg* aMsg);

protected:
	// Construction
	CQoSMsgWriter(CIpSubConnectionProvider* aOwner, RInternalSocket& aSocket);

	// Active Object Implementation
	void RunL();
	inline void DoCancel();

private:
	/** Message Owner */
	CIpSubConnectionProvider* iOwner;

	/** Reference to an Internal Socket */
	RInternalSocket& iSocket;

	/** Current Message */
	CQoSMsg* iCurrentMsg;

	/** List of Pending Messages */
	TSglQue<CQoSMsg> iPendingMsg;

	/** Flag Set when Active Object is shuttin down */
	TBool iClosing;
	};


NONSHARABLE_CLASS(CQoSMsgReader) : public CActive
/**
Active Object that receives messages from the QoS PRT and forwards them to the
IP Connection Provider

@internalComponent

@released Since v9.0
*/
	{
public:
	// Construction
	static CQoSMsgReader* NewL(CIpSubConnectionProvider *aOwner, RInternalSocket& aSocket);
	~CQoSMsgReader();

protected:
	// Construction
	CQoSMsgReader(CIpSubConnectionProvider* aOwner, RInternalSocket& aSocket);
	void ConstructL();

	// Active Object Implmentation
	void RunL();
	inline void DoCancel();

private:
	/** Message Owner */
	CIpSubConnectionProvider* iOwner;

	/** Reference of Internal Socket */
	RInternalSocket& iSocket;

	/** Pointer to Buffer to receive data from QoS PRT */
	TPtr8 iRecvPtr;

	/** Buffer to receive data from QoS PRT */
	HBufC8* iRecvBuf;

	/** Flag Set when Active Object is shuttin down */
	TBool iClosing;
	};

#include "qos_msg.inl"

#endif // __QOS_MSG_H__
