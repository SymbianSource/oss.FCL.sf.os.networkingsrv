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
// Implementation file for the QoS Mapping Messages
// 
//

/**
 @file qos_msg.cpp
*/

#include "pfqos_stream.h"
#include "pfqoslib.h"
#include "IPSCPR.h"
#include "qos_msg.h"
#include "../../iptransportlayer/src/ipscprlog.h"
#include <comms-infras/ss_log.h>


const TIp6Addr KInet6AddrMask = {{{0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
                                  0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff}}};

const TInt KQoSDefaultBufSize = 8000;


CQoSMsg* CQoSMsg::NewL( TPfqosMessages aMsgType )
/**
Create a new QoS PRT Message

@param aMsgType Message Type
*/
	{
	CQoSMsg* msg = new (ELeave) CQoSMsg();

	CleanupStack::PushL( msg );
	msg->ConstructL( aMsgType );
	CleanupStack::Pop();

	return msg;
	}


CQoSMsg::~CQoSMsg()
/**
Destructor
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsg::Destroy [%08x] Type=%d"), this, iType));

	delete iMsg;
	iMsg = NULL;
	}


CQoSMsg::CQoSMsg()
/**
Constructor
*/
	{
	}


void CQoSMsg::ConstructL( TPfqosMessages aMsgType )
/**
QoS PRT Message second phase construction

@param aMsgType Message Type
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsg::Construct [%08x] Type=%d"), this, aMsgType));

	iMsg = CPfqosStream::NewL(KQoSDefaultBufSize);
	iMsg->Init((TUint8)aMsgType);
	iType = aMsgType;
	}


void CQoSMsg::AddSrcAddr(const TInetAddr &aAddr)
/**
Adds Source Address Information to the QoS PRT Message

@param aAddr Source Address
*/
	{
	TInetAddr srcInetAddr(aAddr);
	srcInetAddr.SetFamily(KAFUnspec);
	srcInetAddr.SetAddress(KInet6AddrNone);

	TInetAddr mask;
	mask.SetAddress(KInet6AddrMask);
	iMsg->AddSrcAddress(srcInetAddr, mask, (TUint16)srcInetAddr.Port()); 
	}


void CQoSMsg::AddDstAddr(const TInetAddr &aAddr)
/**
Adds Destination Address Information to the QoS PRT Message

@param aAddr Destination Address
*/
	{
	TInetAddr dstInetAddr(aAddr);
	if (dstInetAddr.Family() == KAfInet)
		{
		dstInetAddr.ConvertToV4Mapped();
		}

	TInetAddr mask;
	mask.SetAddress(KInet6AddrMask);
	iMsg->AddDstAddress(dstInetAddr, mask, (TUint16)dstInetAddr.Port()); 
	}


void CQoSMsg::AddExtensionPolicy(TQoSExtensionQueue& aExtensions)
/**
Add QoS Extension Parameters to the QoS Message

@param aExtensions Collection of Extensions Parameters
*/
	{
	TQoSExtensionQueueIter iter(aExtensions);
	CExtensionBase* extension;
	while ((extension=iter++) != NULL)
		{
		TDesC8& extData = extension->Data();
		iMsg->AddExtensionPolicy(extData);
		}
	}


void CQoSMsg::Send(RInternalSocket &aSocket, TRequestStatus& aStatus)
/** 
Sends the current message to the QoS PRT

@param aSocket Internal Socket over which to send a message
@param aStatus Request Status
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsg::Send [%08x] Type=%d"), this, iType));
	
	iMsg->Send(aSocket, aStatus);
	}


// ###########################################################


CQoSMsgWriter* CQoSMsgWriter::NewL(CIpSubConnectionProvider* aOwner, RInternalSocket& aSocket)
/**
Create QoS PRT Message Writer

@param aOwner The IP SubConnection Provider that creates this object
@param aSocket reference to an Internal Socket owned by the IP SubConnection Provider
*/
	{
	return new (ELeave) CQoSMsgWriter(aOwner, aSocket);
	}


CQoSMsgWriter::CQoSMsgWriter(CIpSubConnectionProvider* aOwner, RInternalSocket& aSocket)
/**
Constructor

@param aOwner The IP SubConnection Provider that creates this object
@param aSocket reference to an Internal Socket owned by the IP SubConnection Provider
*/
	: CActive(EPriorityStandard)
	, iOwner(aOwner)
	, iSocket(aSocket)
	, iClosing(EFalse)
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::Construct [%08x]"), this));

	CActiveScheduler::Add(this);
	iPendingMsg.SetOffset(_FOFF(CQoSMsg, iLink));
	}


CQoSMsgWriter::~CQoSMsgWriter()
/**
Destructor
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::Destroy [%08x]"), this));

	if (IsActive())
		{
		Cancel();
		}

	iClosing = ETrue;

	if (iCurrentMsg)
		{
		delete iCurrentMsg;
		iCurrentMsg = NULL;
		}

	while (!iPendingMsg.IsEmpty())
		{
		CQoSMsg* msg = iPendingMsg.First();
		iPendingMsg.Remove(*msg);
		delete msg;
		}

	iPendingMsg.Reset();
	}


void CQoSMsgWriter::Send(CQoSMsg* aMsg)
/**
Sends a Message to the QoS PRT

@param aMsg The message to send
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::Send [%08x]"), this));

	// Can only process one message at a time.
	if (IsActive())
		{
		iPendingMsg.AddLast(*aMsg);
		}
	else
		{
		iCurrentMsg = aMsg;
		iCurrentMsg->Send(iSocket, iStatus);
		SetActive();
		}
	}


void CQoSMsgWriter::RunL()
/**
Active Object main processing function
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::RunL [%08x] - Enter"), this));

	TInt err = iStatus.Int();
	if (err != KErrNone && iOwner)
		{
		__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::RunL [%08x] - Process Error"), this));
#ifdef _DEBUG
		TInt msgType = EPfqosReserved;
		if( iCurrentMsg )
			{
			msgType = iCurrentMsg->iType;
			}
		iOwner->ProcessPRTError(msgType, err);
#endif
		}

	delete iCurrentMsg;
	iCurrentMsg = NULL;

	if (!iClosing && !iPendingMsg.IsEmpty())
		{
		__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::RunL [%08x] - Send next message"), this));
		CQoSMsg* msg = iPendingMsg.First();
		iPendingMsg.Remove(*msg);
		iCurrentMsg = msg;
		iCurrentMsg->Send(iSocket, iStatus);
		SetActive();
		}

	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgWriter::RunL [%08x] - Exit"), this));
	}


// ###########################################################


CQoSMsgReader* CQoSMsgReader::NewL( CIpSubConnectionProvider *aOwner, RInternalSocket& aSocket)
/**
Create QoS PRT Message Reader

@param aOwner The IP SubConnection Provider that creates this object
@param aSocket reference to an Internal Socket owned by the IP SubConnection Provider
*/
	{
	CQoSMsgReader* reader = new (ELeave) CQoSMsgReader(aOwner, aSocket);

	CleanupStack::PushL( reader );
	reader->ConstructL();
	CleanupStack::Pop();

	return reader;
	}


CQoSMsgReader::CQoSMsgReader(CIpSubConnectionProvider *aOwner, RInternalSocket& aSocket)
/**
Constructor

@param aOwner The IP SubConnection Provider that creates this object
@param aSocket reference to an Internal Socket owned by the IP SubConnection Provider
*/
	: CActive(EPriorityStandard)
	, iOwner(aOwner)
	, iSocket(aSocket)
	, iRecvPtr(0,0)
	, iClosing(EFalse)
	{
	CActiveScheduler::Add(this);
	}


CQoSMsgReader::~CQoSMsgReader()
/**
Destructor
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::Destruct [%08x]"), this));

	if (IsActive())
		{
		Cancel();
		}

	iClosing = ETrue;

	if (iRecvBuf)
		{
		delete iRecvBuf;
		iRecvBuf = NULL;
		}
	}


void CQoSMsgReader::ConstructL()
/**
QoS PRT Message Reader second phase construction
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::Construct [%08x]"), this));

	iRecvBuf = HBufC8::NewL(KQoSDefaultBufSize);
	TPtr8 tmp(iRecvBuf->Des());
	iRecvPtr.Set(tmp);

	iRecvPtr.Zero();
	iRecvPtr.SetLength(KQoSDefaultBufSize);
	iSocket.Recv(iRecvPtr, 0, iStatus);
	SetActive();
	}


void CQoSMsgReader::RunL()
/**
Active Object main processing function
*/
	{
	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::RunL [%08x] - Enter"), this));

	if (iStatus.Int() == KErrNone && iRecvPtr.Length() > 0 )
		{
		__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::RunL [%08x] - Process Response"), this));
		TPfqosMessage msg(iRecvPtr);
		if (msg.iError == KErrNone)
			{
			iOwner->ProcessPRTMsg(msg);
			}
#ifdef _DEBUG
		else
			{
			TInt msgType = EPfqosReserved;
			iOwner->ProcessPRTError(msgType, msg.iError);
			}
#endif
		}

	if (!iClosing)
		{
		__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::RunL [%08x] - Wait for next message"), this));
		iRecvPtr.Zero();
		iRecvPtr.SetLength(KQoSDefaultBufSize);
		iSocket.Recv(iRecvPtr, 0, iStatus);
		SetActive();
		}

	__IPCPRLOG(IpCprLog::Printf(_L("CQoSMsgReader::RunL [%08x] - Exit"), this));
	}

CQoSSocketOpener* CQoSSocketOpener::NewL(CIpSubConnectionProvider& aIpScpr, Messages::TNodeCtxId& aOriginator)
	{
	return new(ELeave) CQoSSocketOpener(aIpScpr, aOriginator);
	}

CQoSSocketOpener::CQoSSocketOpener(CIpSubConnectionProvider& aIpScpr, Messages::TNodeCtxId& aOriginator)
	: CActive(EPriorityStandard), iIpScpr(aIpScpr), iOriginator(aOriginator)
	{
	CActiveScheduler::Add(this);
	}

CQoSSocketOpener::~CQoSSocketOpener()
	{
	}

void CQoSSocketOpener::Open()
	{
		// Open a connection to the QoS PRT
    _LIT(KDescPfqos, "pfqos");

    // If we couldn't open pipe to QoS framework then it is not supported

	iIpScpr.InternalSocket().Open(KDescPfqos, iStatus);
	SetActive();
	}
	
// Active Object Implementation
void CQoSSocketOpener::RunL()
	{
	TInt err = iStatus.Int();
	if(err == KErrNotFound)
		{
    	err = KErrNotSupported;
		}
	iIpScpr.InternalSocketOpened(err);

	Messages::RClientInterface::OpenPostMessageClose(iIpScpr.Id(), iOriginator,
    	TQoSIpSCprMessages::TInternalSocketOpened().CRef());

	delete this;
	}

void CQoSSocketOpener::DoCancel()
	{
	iIpScpr.InternalSocket().CancelAll();
	}
