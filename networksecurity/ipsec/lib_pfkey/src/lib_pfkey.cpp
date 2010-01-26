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
// This file contains the implementations for the Pfkey library base classes 
// 
//

#include "lib_pfkey.h"

static const TInt KWordLen = 8;

/**
 *TPfkeySendMsgBase
 */
EXPORT_C TPfkeySendMsgBase::TPfkeySendMsgBase(TUint8 aType, TUint8 aSaType, TInt aRequestCount, TInt aPid)
	{
	Reset(aType, aSaType, aRequestCount, aPid);
	}

EXPORT_C TPfkeySendMsgBase::TPfkeySendMsgBase() 
	{
	Reset(0, 0, 0, 0);
	}

EXPORT_C void TPfkeySendMsgBase::Reset(TUint8 aType, TUint8 aSaType, TInt aRequestCount, TInt aPid)
	{
	// Reset the length
	FillZ(sizeof(sadb_msg));
	// Get an easy interface for manipulating the header
	sadb_msg &msg = MsgHdr();
	// Fills in the message header
	msg.sadb_msg_version = PF_KEY_V2;
    msg.sadb_msg_type = aType;
    msg.sadb_msg_satype = aSaType;
    msg.sadb_msg_seq = aRequestCount;
    msg.sadb_msg_pid = aPid;
    msg.sadb_msg_errno = 0;
    msg.sadb_msg_reserved = 0;
	}

EXPORT_C TInt TPfkeySendMsgBase::AddExt(const TPfkeyAnyExt& aExt, const TDesC8& aExtData)
	{
	const TInt addlen = aExtData.Length() + aExt.Length();
	Append(aExt);
	Append(aExtData);
	AppendFill(0, aExt.ExtLen()*KWordLen - addlen);
	return KErrNone;
	}

EXPORT_C void TPfkeySendMsgBase::Finalize()
	{
	const TInt length = Length()/KWordLen;
	__ASSERT_DEBUG(Length()%KWordLen == 0, User::Panic(_L("SA"),0));
	
	TPtrC8 len = TPtrC8((TUint8 *)&length, sizeof(TUint16));
	Replace(_FOFF(struct sadb_msg, sadb_msg_len), 
		sizeof(((struct sadb_msg *)0)->sadb_msg_len), len);
	}

/**
 * TPfkeyRecvMsg
 *
 * Usage Idiom - see test code
 *
 */
EXPORT_C TPfkeyRecvMsg::TPfkeyRecvMsg() : iReadOffset(0), iRemaining(0)
	{}

EXPORT_C TPfkeyRecvMsg::TPfkeyRecvMsg(const TDesC8& aOther) 
		:TPfkeyMsgBase(aOther), iReadOffset(0), iRemaining(0)
	{}

EXPORT_C TInt TPfkeyRecvMsg::NextExtension(TPfkeyAnyExt& aExt)
	{
	// Find the Message Header if not already done.
	if (iReadOffset == 0)
		{
		const struct sadb_msg& msg_hdr = MsgHdr();
		iRemaining = Length();
		// Make sure length is consistent
		if (iRemaining != msg_hdr.sadb_msg_len * KWordLen)
			goto end;

		iReadOffset += sizeof(struct sadb_msg);
		iRemaining -= sizeof(struct sadb_msg);
		}
	// Make sure there are more ext
	if (iRemaining >= (TInt)sizeof(sadb_ext))
		{
		// Find the Next ext
		const struct sadb_ext& ext = *(const struct sadb_ext*)&iBuf[iReadOffset];
		const TInt len = ext.sadb_ext_len * KWordLen;
		// Make sure length is OK
		if (iRemaining < len)
			goto end;
		aExt.Set(Mid(iReadOffset, len)); 
		iReadOffset += len;
		iRemaining -= len;
		return KErrNone;
		}
	else if(iRemaining == 0)
		{
		iReadOffset = 0;
		return KErrNotFound;
		}
end:
	aExt.Set(KNullDesC8);
	iReadOffset = 0;
	return KErrGeneral;
	}

EXPORT_C void TPfkeyRecvMsg::Reset()
	{
	Zero();
	iReadOffset = 0;
	iRemaining = 0;
	}

EXPORT_C TInt TPfkeyRecvMsg::BytesUnparsed()
	{
	return iRemaining;
	}

/**
 *	Main SABD class
 */
EXPORT_C RSADB::RSADB()
/** Default constructor. */
	{}

EXPORT_C RSADB::~RSADB()
/** Default destructor, closes the handle to the stack side SADB if open. */
	{
	Close();
	}

EXPORT_C TInt RSADB::Open(RSocketServ& aServer)
/** Opens the handle to stack side SADB.
@param aServer A socketserver session to use.
@return KErrNone if successful. */  
	{
	_LIT(PFKEY_SOCKET_NAME, "pfkey");
	return iPfkeySocket.Open(aServer, PFKEY_SOCKET_NAME);
	}
	
EXPORT_C void RSADB::Close()
/** Closes the handle to the stack side SADB. */
	{
	if (iPfkeySocket.SubSessionHandle())
		iPfkeySocket.Close();
	}
	
// General one for All
EXPORT_C void RSADB::SendRequest(const TDesC8& aMsg, TRequestStatus& aStatus)
/** Sends a message to the stack side SADB.
SendRequest() should only be used with an open handle to stack side SADB.
@param aMsg A descriptor containing the message to be sent.
@param aStatus On completion, KErrNone if successful, otherwise one of the system wide error codes.

@capability NetworkControl Only privileged apps can affect policies. */	
	{                      
//	__ASSERT_ALWAYS(iPfkeySocket is connected);
	iPfkeySocket.Write(aMsg, aStatus);
	}

EXPORT_C void RSADB::CancelSend()
/** Cancels an outstanding Send() operation. 
Calling the function will cause any outstanding send operation to complete prematurely. */
	{
	iPfkeySocket.CancelSend();
	}
	
// Separate Read Request
EXPORT_C void RSADB::ReadRequest(TDes8& aMsg, TRequestStatus& aStatus)
/** Receives message from the stack side SADB.
ReadRequest() should only be used with an open handle to stack side SADB.
There may only be one receive operation outstanding at any one time.

@param aMsg A descriptor where the message will be placed.
@param aStatus On return, KErrNone if successful, otherwise another of the system-wide error codes.

@capability NetworkControl Only privileged apps can affect policies*/
	{
	iPfkeySocket.Recv(aMsg, 0, aStatus);
	}

EXPORT_C void RSADB::CancelRecv()
/** Cancels an outstanding ReadRequest() operation. 
Calling this function will cause any outstanding read request operation to complete prematurely.*/
	{
	iPfkeySocket.CancelRecv();
	}

EXPORT_C void RSADB::FinalizeAndSend(TPfkeySendMsgBase& aMessage, TRequestStatus& aStatus) 
/** Finalises the message passed then sends it to the stack side SADB.
@param aMessage The message to be sent.
@param aStatus On completion, KErrNone if successful, otherwise one of the system wide error codes.

@capability NetworkControl Only privileged apps can affect policies*/
	{
	// update the length of the header
	// Seek to the beginning of the stream
	// Rewrite the header at the start of the stream
	aMessage.Finalize();
	SendRequest(aMessage, aStatus);
	}

EXPORT_C void RSADB::SetOpt(TUint aLevel,TUint aName,const TDesC8 &aOption)
/** Changes the ip address of an endpoint.
@param aLevel The option level
@param aName The option name
@param aOption The option argument*/
	{
	iPfkeySocket.SetOpt(aLevel, aName, aOption);
	}	

#ifndef EKA2
GLDEF_C TInt E32Dll(TDllReason aReason)
	{
	(void)aReason;
	return KErrNone;
	}
#endif // EKA2
