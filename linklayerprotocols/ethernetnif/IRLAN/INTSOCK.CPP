// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// Internal Sockets API
// Written by Mal, September 1997
// Based on work by AdamG 
// 
//

/**
 @file 
*/

#include <e32std.h>
#include <e32base.h>
#include <es_prot.h>
#include <es_mbuf.h>
#include <es_prot_internal.h>
#include "INTSOCK.H"
#include "IRLANUTL.H"
#include "irlantimer.h"

#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

/**
Constructor.
*/
CInternalSocket::CInternalSocket():iSendBufPtr(NULL,0)
{
	__DECLARE_NAME(_S("CInternalSocket"));
	iState = ENew;
}

/**
Only needed when cloning an accept from a listen socket.

@param aSock   A pointer to CServProviderBase object
@param aNotify A pointer to MSocketNotify object.
@return A pointer to CInternalSocket object.
*/
CInternalSocket *CInternalSocket::NewL(CServProviderBase *aSock,MSocketNotify *aNotify)
{
	CInternalSocket *is=new (ELeave) CInternalSocket;
	// Need to set up send buffer for
	is->iSendBufLength=KInternalMaxBufSize;
	is->iSendBuffer=HBufC8::NewMaxL(is->iSendBufLength);
	TPtr8 temp=is->iSendBuffer->Des();
	is->iSendBufPtr.Set(temp);

	is->SetNotify(aNotify);
	is->iCServSocket=aSock;
	is->iCServSocket->SetNotify(is);
	is->iState = EOpen;
	return is;
}

/**
Destructor.
*/
CInternalSocket::~CInternalSocket()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	~CInternalSocket %x\r\n"),this));
#endif
	delete iSendBuffer;
	delete iTimers;
	__ASSERT_DEBUG(iState==EClosing,IrlanUtil::Panic(EIrlanBadState));
	delete iCServSocket;   // underlying CServProviderBase
	if (iProtocol)
		iProtocol->Close();
}

/**
Opens a socket of the required type and protocol.
@param aProtocolName A Protocol Name.
@param aNotify		 A pointer to MSocketNotify object.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::OpenL(const TDesC& aProtocolName,MSocketNotify* aNotify)
{
	// Assert a new socket
	if (iState!=ENew)
		return KErrAlreadyExists;
	TInt err = KErrNone;
	// Ensure protocol is loaded
	TRAP(err, iProtocol = SocketServExt::FindAndLoadProtocolL(aProtocolName));
	if (err!=KErrNone)
		return err;
	iProtocol->Open();
	// Cache some protocol information	
	TServerProtocolDesc info;
	info.iName=aProtocolName;
	iProtocol->Identify(&info);
	if (!(info.iServiceTypeInfo & ESocketSupport))
		{
		iProtocol->Close();
		iProtocol = NULL;
		return KErrNotSupported;
		}
	if (info.iServiceTypeInfo & EUseCanSend)
		iFlags |= KSoUseCanSend;
	if (info.iServiceInfo & KSIConnectionLess)
		iFlags |= KSoConnectionless;
	if (info.iServiceInfo & KSIGracefulClose)
		iFlags |= KSoGracefulClose;

	// Create the SAP
	TRAP(err, iCServSocket = iProtocol->NewSAPL(info.iSockType));
	if (err!=KErrNone)
		{
		iProtocol->Close();
		iProtocol = NULL;
		return err;
		}
	// Need to set up send buffer for
	iSendBufLength=KInternalMaxBufSize;
	iSendBuffer=HBufC8::NewMaxL(iSendBufLength);
	TPtr8 temp=iSendBuffer->Des();
	iSendBufPtr.Set(temp);

	// Set state to OPEN
	iCServSocket->SetNotify(this);
	this->SetNotify(aNotify);
	iState = EOpen;
	iCServSocket->Start();
	return KErrNone;
}

/**
Need to construct the TIrdaSockAddr with the appropriate port number.
Note that this is a real simple connect - no notion of connect data or
select or a blocking socket.
@param aAddr A reference to TSockAddr class.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::Connect(TSockAddr& aAddr)
{
	TInt ret=iCServSocket->SetRemName(aAddr);
	if (ret!=KErrNone)
		return ret;
  	iState=EConnecting;
	iCServSocket->ActiveOpen();
	return ret;
}

/**
Does the work of listen and accept together.
@param aQSize	 No of clients to be done passive open.
@return KErrNone if succesfull.
*/
TInt CInternalSocket::WaitForConnect(TUint aQSize)
{
	iCServSocket->PassiveOpen(aQSize);
	iState=EListening;
	return KErrNone;
}

/**
Gets the local address of a socket. 
@param aAddr Local address which is filled..
*/
void CInternalSocket::LocalName(TSockAddr& anAddr)const
{
	iCServSocket->LocalName(anAddr);
}

/**
Gets the remote name (address) of the socket service provider entity. The format of the data 
in the TSockAddr object is defined by individual protocols.
@param aAddr The address to be filled in.
*/
void CInternalSocket::RemName(TSockAddr& /*aAddr*/)const
{
}

/**
Sets the local name (address) of the socket service provider entity. The format of the data 
in the TSockAddr object is defined by individual protocols.
@param aAddr The address .
@return KErrNone if the local name is correctly set or, if this is not the case, an 
		informative error number.
*/
TInt CInternalSocket::SetLocalName(TSockAddr& aAddr)
{
	return iCServSocket->SetLocalName(aAddr);
}

/**
Sets some protocol specific option when called by the socket server on behalf of a client. A 
protocol may pass the request down a protocol stack (to protocols it is bound to) using the 
SetOption() function of CProtocolBase.
@param aLevel Option level 
@param aName  Option Name 
@param aDes	  Option data.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::SetOption(TUint aLevel,TUint aName,const TDesC8& aDes)
{
	return iCServSocket->SetOption(aLevel,aName,aDes);
}

/**
Sets the remote name (address) of the socket service provider entity. The format of the data in the TSockAddr object is defined by individual protocols.
@param The address 
@return Returns KErrNone if the remote name is correctly set or, if this is not the case, an informative error number.
*/
TInt CInternalSocket::SetRemName(TSockAddr &aAddr)
{	
	return iCServSocket->SetRemName(aAddr);
}

/**
Create a Socket.
*/
void CInternalSocket::Start(void)
{
	iCServSocket->Start();
}

/**
Gets some protocol specific option when called by the socket server on behalf of a client. 
A protocol may pass the request down a protocol stack (to protocols it is bound to) using 
the GetOption() function of CInternalSocket.
@param aLevel	  Option level 
@param aName	  Option Name 
@param anOption	  Option data.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::GetOption(TUint aLevel,TUint aName,TDes8& anOption)const
{
	return iCServSocket->GetOption(aLevel,aName,anOption);
}

/**
Performs some protocol specific IO control.
@param aLevel IOCTL level 
@param aName  IOCTL Name 
@param aDes	  IOCTL option 
*/
void CInternalSocket::Ioctl(TUint aLevel,TUint aName,TDes8* aDes)
{
	iCServSocket->Ioctl(aLevel,aName,aDes);
}

/**
Cancels an outstanding Ioctl call. You are guaranteed only to have one outstanding at once. 
@param aLevel IOCTL level 
@param aName  IOCTL Name 
*/
void CInternalSocket::CancelIoctl(TUint aLevel,TUint aName)
{
	iCServSocket->CancelIoctl(aLevel,aName);
}

/**
Sends data onto the network via the protocol. Connection-oriented sockets must be in a connected 
state (that is ConnectComplete() has been called on their MSocketNotify) before Write() is 
called.
@param aDes  The data to be sent
@param aFlag Protocol specific options
@param aAddr Address to write the data to 
@return For stream-oriented protocols, the return value is the number of bytes actually written. 
		If this is less than the length of the descriptor, then the protocol should call CanSend() 
		when it is ready to send more data. For datagram-oriented protocols, the write should return 
		either 0 if the write cannot be completed, or the length of the descriptor if the write succeeds - 
		no other values are valid. If the Write() must return 0, then it should call CanSend() when 
		it is ready to send more data. If the Write() fails due to some error, then it should call 
		Error() with an informative error number,
*/
TUint CInternalSocket::Write(const TDesC8& aDes,TUint aFlag,TSockAddr *aAddr)
{
	return iCServSocket->Write(aDes,aFlag,aAddr);
}

/**
Gets data which the protocol has indicated is waiting in its buffers using the NewData up-call 
on the MSocketNotify.
@param The buffer for data 
@param Protocol specific options 
@param Where the data came from 
*/
void CInternalSocket::GetData(TDes8 &,TUint,TSockAddr *)
{
}

/**
Initiates a connection operation - this means that it tells the protocol to attempt to connect 
to a peer. It is called by the socket server in response to a connect request from a client. 
@param If the protocol supports user specified connection data, then it will be held in this buffer
*/
void CInternalSocket::ActiveOpen(const class TDesC8 &)
{
}

/**
Initiates a connection operation - this means that it tells the protocol to attempt to connect 
to a peer. It is called by the socket server in response to a connect request from a client.
*/
void CInternalSocket::ActiveOpen(void)
{
}

/**
Tells the protocol to start waiting for an incoming connection request on this socket (i.e. 
port). It is called by the socket server in response to a listen request from a client.
@param aNum size of connect queue 
@param aDes if the protocol supports user specified connection data, then it will be held in 			this buffer
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::PassiveOpen(TUint aNum,const TDesC8 &aDes)
{
	return iCServSocket->PassiveOpen(aNum,aDes);
}

/**
Tells the protocol to start waiting for an incoming connection request on this socket (i.e. 
port). It is called by the socket server in response to a listen request from a client.
@param aNum size of connect queue 
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalSocket::PassiveOpen(TUint aNum)
{
	return iCServSocket->PassiveOpen(aNum);
}

/**
Terminates a connection (or closes a non connection-oriented socket down). 
@param The shutdown type 
@param If the protocol supports disconnect data, any such data required will be held in this 
		buffer
*/
void CInternalSocket::Shutdown(enum CServProviderBase::TCloseType,const TDesC8 &)
{
}

/**
Terminates a connection (or closes a non connection-oriented socket down). 
@param The shutdown type 
*/
void CInternalSocket::Shutdown(enum CServProviderBase::TCloseType)
{
}	

/**
Specifies that the protocol should choose a local address for the service access point itself.
*/
void CInternalSocket::AutoBind(void)
{	
	iCServSocket->AutoBind();
}

/**
Close the socket - causes connection closure if required
*/
void CInternalSocket::Close()
{
	// If allready closing or closed, ignore the call
	switch (iState)
		{
	case ENew:
	case EClosing:
	case EClosed:
	case EFault:
		return;
	case EOpen:
	case EConnecting:
	case EConnected:
	case EListening:
		// attempting graceful closure of SAP
		if (iCServSocket)
			iCServSocket->Shutdown(CServProviderBase::ENormal);
		TRAPD(ret,iTimers=CInternalSocketTimers::NewL(this));
		if (ret!=KErrNone)
			{
			delete this;
			break;
			}
		else
			{
			TCallBack callback(CInternalSocket::InternalSocketTimerExpired,this);
			iTimers->StartInternalSocketTimer(callback,1500000L);  // 1.5 secs
			}
		iState=EClosing;
		break;
	default:
		IrlanUtil::Panic(EIrlanBadState);
		break;
		}
}

TInt CInternalSocket::InternalSocketTimerExpired(TAny *aCIntSock)
{
	CInternalSocket *ss=(CInternalSocket *)aCIntSock;

	ss->iTimers->DoInternalSocketTimerExpired();
	delete ss;
	return 0;
}

/**
Recv into descriptor via internal socket RecvFrom.
@param aDes		A descriptor where data read will be placed
@param anAddr	@param aAddr Address to write the data to 
@param aStat	On completion, KErrNone if successful, KErrEof if a remote connection is 
				closed and there is no more data, KErrNotReady if called when an operation 
				is still outstanding, or a system-wide error code
@param anOptions Option data.
@return KErrNone if successful, KErrEof if a remote connection is closed and there is no more 
		data, KErrNotReady if called when an operation is still outstanding, or a system-wide error code
*/
TUint CInternalSocket::Recv(TDes8& aDes,TSockAddr* anAddr,TRequestStatus& aStat,TUint aOptions)
{
	RecvFrom(aDes,*anAddr,aOptions,aStat);  
	return KErrNone;
}

/**
Recv datagram and source address.  ASYNCHRONOUS FUNCTION.
@param aDes		A descriptor where data read will be placed
@param aAddr	A remote source address for unconnected receives
@param aFlags	Flags which are passed through to protocol
@param aStatus	On completion, will contain an error code: see the system-wide error codes. 
				Note that KErrEof indicates either that a remote connection is closed, and 
				that no more data is available for reading, or the socket has been shutdown 
				with option RSocket::EStopInput.
@return On completion, will contain an error code: see the system-wide error codes. 
		Note that KErrEof indicates either that a remote connection is closed, and 
		that no more data is available for reading, or the socket has been shutdown 
		with option RSocket::EStopInput.
*/
void CInternalSocket::RecvFrom(TDes8& aDes, TSockAddr& aAddr, TUint aFlags, 
							   TRequestStatus& aStatus)
{
	// Check if recv allready pending
	__ASSERT_ALWAYS(!(iFlags & KSoRecvPending), Panic(EPanic_InUse));
	// Check if state allows recv
	__ASSERT_ALWAYS((!(iFlags&KSoConnectionless) && iState==EConnected) || iState==EOpen, Panic(EPanic_NotReady));

	iRecvDes = &aDes;
	iRecvFlags = aFlags;
	iRecvAddr = &aAddr;
	iRecvStat = &aStatus;
	__ASSERT_ALWAYS(iRecvStat!=0,Panic(EPanic_StatusRecv));
//	iRecvOffset = 0;
	*iRecvStat = KRequestPending;	
	iFlags |= KSoRecvPending;
	DoRecv();
}

/**
Unpack RMBufChain and send the descriptor via internal socket SendTo.
@param aPdu     A reference to the packet to be sent (really an RMBufPkt)
@param aOptions Protocol specific options
@param aStat	On completion, will contain an error code: see the system-wide error codes. 
				Note that KErrEof indicates either that a remote connection is closed, and 
				that no more data is available for reading, or the socket has been shutdown 
				with option RSocket::EStopInput.
@param aAddr    Address to write the data to 
@return For stream-oriented protocols, the return value is the number of bytes actually written. 
		If this is less than the length of the descriptor, then the protocol should call CanSend() 
		when it is ready to send more data. For datagram-oriented protocols, the write should return 
		either 0 if the write cannot be completed, or the length of the descriptor if the write succeeds - 
		no other values are valid. If the Write() must return 0, then it should call CanSend() when 
		it is ready to send more data. If the Write() fails due to some error, then it should call 
		Error() with an informative error number,
*/
TUint CInternalSocket::Write(RMBufChain& aPdu,TUint aOptions,TRequestStatus& aStat,TSockAddr* anAddr)
{
	RMBuf *mbuf;
	TInt bptr=0;
	TMBufIter iter=aPdu;
	while ((mbuf=iter++)!=NULL)
		{// Process each individual MBuf in chain.
		TUint8* mptr=mbuf->Ptr();
		TInt len=mbuf->Length();
		while (len--)	// Byte-stuffing algorithm applied to each mbuf byte
			iSendBufPtr[bptr++]=*mptr++;
		}
	iSendBufPtr.SetLength(bptr);
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	CInternalSocket::Write\r\n")));
	LOG(Log::HexDump(_S("IRLAN	"),_S("IRLAN	"),
	  (TUint8 *)&iSendBufPtr[0],iSendBufPtr.Length()));
#endif
	SendTo(iSendBufPtr,*anAddr,aOptions,aStat);  
	return KErrNone;
}

/**
Send datagram to host/network.  ASYNCHRONOUS FUNCTION.
@param aDes		A constant descriptor
@param aAddr	A remote destination address for unconnected sends
@param aFlags	Flags which are passed through to protocol
@param aStatus	On completion, will contain an error code: see the system-wide error codes. 
				Note that KErrEof indicates that the socket has been shutdown with option 
				EStopOutput
*/
void CInternalSocket::SendTo(const TDesC8& aDes, const TSockAddr& aAddr, TUint aFlags, TRequestStatus& aStatus)
{
	// Check if send already pending
	__ASSERT_ALWAYS(!(iFlags & KSoSendPending), Panic(EPanic_InUse));
	// Check if state allows send
	__ASSERT_ALWAYS((!(iFlags&KSoConnectionless) && iState==EConnected) || iState==EOpen, Panic(EPanic_NotReady));

	iSendDes = &aDes;
	iSendFlags = aFlags;
	iSendAddr = (TSockAddr*)&aAddr;
	iSendStat = &aStatus;
//	iSendOffset = 0;
	*iSendStat = KRequestPending;	
	iFlags |= KSoSendPending;
	DoSend();
}

/**
Abort a recv request
Cancels an outstanding Recv() operation
*/
void CInternalSocket::CancelRecv()
{
	if (iFlags & KSoRecvPending)
		CompleteRecv(KErrCancel);
}

/**
Abort a send request
Cancels an outstanding Send() operation.
*/
void CInternalSocket::CancelSend()
{
	if (iFlags & KSoSendPending)
		CompleteSend(KErrCancel);
}

/**
Upcall from SAP - more data arrived
Indicates that new data is available on a service access point
*/
void CInternalSocket::NewData(TUint aCount)
{
	if (aCount==KNewDataEndofData)
		// End of data indication
		iFlags |= KSoCantRecvMore;
	else
		iRecvNotifyCount += aCount;

	DoRecv();
}

/**
Upcall from SAP - flow control on
Indicates that new buffer space is available on a service 
*/
void CInternalSocket::CanSend()
{
	iFlags &= ~KSoSendFlowOff;
	iNotify->CanSend();
	DoSend();
}

/**
Upcall from SAP - can now delete or detach to SAP.
Indicates that the SAP has finished closing down.
*/
void CInternalSocket::CanClose(TDelete aDelete)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN:	CInternalSocket::CanClose - deleting CServProvdBase\r\n")));
#endif
	switch (iState)
		{
	case EClosing:
		// leave things alone
		break;
	default:
		delete iCServSocket;
		iCServSocket=NULL;
		if (aDelete==EDelete && iProtocol)
			iProtocol->Close();
		iProtocol=NULL;
		break;
		}	
}

/**
Upcall from SAP - Error notification.
@param aError  The error code
@param aOpMask A bitmask of TOperationBitmasks values specifying which pending operations are
			   affected by the Error up-call
*/
void CInternalSocket::Error(TInt aError, TUint aOpMask)
{
	if ((iFlags & KSoRecvPending) && (aOpMask & MSocketNotify::EErrorSend))
		CompleteRecv(aError);
	if ((iFlags & KSoSendPending) && (aOpMask & MSocketNotify::EErrorSend))
		CompleteSend(aError);
}
                                          
/**
Upcall from SAP - Not supported
Indicates that the other end of a connection has disconnected. 
*/
void CInternalSocket::Disconnect(void)
{
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
*/
void CInternalSocket::ConnectComplete()
{
}

/**
Upcall from SAP.  Notify layer above
Indicates that the currently pending Ioctl has completed

@param aBuf Any data requested by the Ioctl operation.
*/
void CInternalSocket::IoctlComplete(TDesC8* aBuf)
{
	iNotify->IoctlComplete(aBuf);
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aSSP The new SSP for passive opens 
*/
void CInternalSocket::ConnectComplete(CServProviderBase& aSSP)
{
	iState = EConnected;
	iNotify->ConnectComplete(aSSP);
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aConnectData Connect data (if supported)  
*/
void CInternalSocket::ConnectComplete(const TDesC8& aConnectData)
{
	iState = EConnected;
	iNotify->ConnectComplete(aConnectData);
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aSSP The new SSP for passive opens 
@param aConnectData Connect data (if supported)  
*/
void CInternalSocket::ConnectComplete(CServProviderBase& aSSP,const TDesC8& aConnectData)
{
	iState = EConnected;
	iNotify->ConnectComplete(aSSP,aConnectData);
}

/**
Indicates that the other end of a connection has disconnected
@param aDisconnectData User data in the disconnect frame.
*/
void CInternalSocket::Disconnect(TDesC8& /*aDisconnectData*/)
{
}

/**
Upcall from SAP.  Notify layer above.
Indicates that the SAP has finished closing down
@param aDisconnectData Any user data carried on the disconnect frame 
@param aDelete         Delete SAP 
*/
void CInternalSocket::CanClose(const TDesC8& /*aDisconnectData*/,TDelete /*aDelete*/)
{
}

/**
Receive processing
*/
void CInternalSocket::DoRecv()
{
	if (!(iFlags & KSoRecvPending))
		return;
	if (iRecvNotifyCount==0)
		return;
	iRecvNotifyCount -= 1;
	iCServSocket->GetData(*iRecvDes, iRecvFlags, iRecvAddr);
//	iRecvOffset = iRecvDes->Length();
	CompleteRecv(KErrNone);
}

/**
Complete the pending receive updating the length
@param aStatus Sets the status after completely receiving the data.
*/
void CInternalSocket::CompleteRecv(TInt aStatus)
{
	iFlags &= ~KSoRecvPending;
//	iRecvDes->SetLength(iRecvOffset);
//	if (iRecvXferLen)
//		{
//		TSockXfrLength len;
//		len() = iRecvOffset;
//		*iRecvXferLen = len;
//		}
	__ASSERT_ALWAYS(iRecvStat!=NULL,Panic(EPanic_StatusComplete));
	User::RequestComplete(iRecvStat, aStatus);
}

/**
Send processing
*/
void CInternalSocket::DoSend()
{
	if (!(iFlags & KSoSendPending))
		return;
	if (iCServSocket->Write(*iSendDes, iSendFlags, iSendAddr))
		{
//		iSendOffset = iSendDes->Length();
		CompleteSend(KErrNone);
		}
	else
		iFlags |= KSoSendFlowOff;
}

/**
Complete the pending send updating the length
@param aStatus On Completion sets the status.
*/
void CInternalSocket::CompleteSend(TInt aStatus)
{
	iFlags &= ~KSoSendPending;
//	if (iSendXferLen)
//		{
//		TSockXfrLength len;
//		len() = iSendOffset;
//		*iSendXferLen = len;
//		}
	iSendBufPtr.SetLength(iSendBufLength);
	User::RequestComplete(iSendStat, aStatus);
}

void CInternalSocket::NoBearer(const TDesC8& /*aConnectionParams*/)
{
}

void CInternalSocket::Bearer(const TDesC8& /*aConnectionInfo*/)
{
}

/**
Panics.
@param aPanic The panic number.
*/
void CInternalSocket::Panic(TPanicCode aPanic)
{
	__DEBUGGER();
	User::Panic(_L("CInSock"), aPanic);
}

//**********************************************************************

/**
Constructor
*/
CInternalHostResolver::CInternalHostResolver()
{
	__DECLARE_NAME(_S("CInternalHostResolver"));
}

/**
Destructor.
*/
CInternalHostResolver::~CInternalHostResolver()
{
	delete iHostResolver;
	if (iProtocol)
		iProtocol->Close();
}

/**
Create a new CInternalHostResolver object.
*/
CInternalHostResolver *CInternalHostResolver::NewL()
{
	return new (ELeave) CInternalHostResolver;
}

/**
Opens a socket of the required type and protocol.
@param aProtocolName A Protocol Name.
@param aNotify		 A pointer to MResolverNotify object.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalHostResolver::OpenL(const TDesC& aProtocolName,MResolverNotify *aEngine)
{
	// Assert a new host resolver
	if (iState!=ENew)
		return KErrAlreadyExists;
	TInt err = KErrNone;
	// Ensure protocol is loaded
	TRAP(err, iProtocol = SocketServExt::FindAndLoadProtocolL(aProtocolName));
	if (err!=KErrNone)
		return err;
	iProtocol->Open();
	// Create the host resolver
	TRAP(err, iHostResolver = iProtocol->NewHostResolverL());
	if (err!=KErrNone)
		{
		iProtocol->Close();
		iProtocol = NULL;
		return err;
		}
	// Set state to OPEN
	iHostResolver->SetNotify(this);
	this->SetNotify(aEngine);
	iState = EConnected;
	return KErrNone;
}

/**
Notifier call back from IrDA PRT
This is where the request completes - it has already filled
in the name record passed through to GetByName so leave that.
@param aErr Error code.
*/
void CInternalHostResolver::QueryComplete(TInt aErr)
{
	iBusy=EFalse;
	iNotify->QueryComplete(aErr);
}

/**
Resolves a machine name to a TSockAddress asynchronously
@param aName Name of the service to get
*/
TInt CInternalHostResolver::GetByName(TNameRecord& aName)
{
	if (iBusy)
		return KErrInUse;
	iBusy=ETrue;
	iHostResolver->GetByName(aName);
	return KErrNone;
}			   

/**
Gets the name of a host from its address asynchronously
@param aName Address of the service to get
*/
void CInternalHostResolver::GetByAddress(TNameRecord& /*aName*/)
{
}

/**
Sets the name of the local host
@param aNameBuf The local host name.
*/
void CInternalHostResolver::SetHostName(TDes8& /*aNameBuf*/)
{
}

/**
Gets the name of the local host.
@param aNameBuf The buffer passed in should have a minimum length of 256 characters, 
*/
void CInternalHostResolver::GetHostName(TDes8& /*aNameBuf*/)
{
}

//***********************************************************************

/**
Constructor.
*/
CInternalNetDB::CInternalNetDB()
{
}

/**
Destructor.
*/
CInternalNetDB::~CInternalNetDB()
{
	delete iNetDatabase;
	if (iProtocol)
		iProtocol->Close();
}

/**
Create a new CInternalNetDB object
*/
CInternalNetDB *CInternalNetDB::NewL()
{
	return new (ELeave) CInternalNetDB;
}

/**
Opens a socket of the required type and protocol.
@param aProtocolName A Protocol Name.
@param aNotify		 A pointer to MResolverNotify object.
@return KErrNone if succesfull otherwise any error code.
*/
TInt CInternalNetDB::OpenL(const TDesC& aProtocolName,MResolverNotify *aEngine)
{
	// Assert a new net database
	if (iState!=ENew)
		return KErrAlreadyExists;
	TInt err = KErrNone;
	// Ensure protocol is loaded
	TRAP(err, iProtocol = SocketServExt::FindAndLoadProtocolL(aProtocolName));
	if (err!=KErrNone)
		return err;
	iProtocol->Open();
	// Create the net database
	TRAP(err, iNetDatabase = iProtocol->NewNetDatabaseL());
	if (err!=KErrNone)
		{
		iProtocol->Close();
		iProtocol = NULL;
		return err;
		}
	// Set state to OPEN
	iNetDatabase->SetNotify(this);
	this->SetNotify(aEngine);
	iState = EOpen;
	return KErrNone;
}

/**
Notifier call back from IrDA PRT
This function is called when an operation querying the processing ability of a peer device 
completes
@param aError Error code.
*/
void CInternalNetDB::QueryComplete(TInt aError)
{
	if (iBusy)
		{
		iBusy=EFalse;
		iNotify->QueryComplete(aError);
		}
	// else we assume that it's an Add or Remove.
}

/**
Remember that results are returned in this buffer too.
Makes a query to the database
@param aBuffer The query to perform 
*/
TInt CInternalNetDB::Query(TDes8& aBuffer)
{
	if (iBusy)
		return KErrInUse;
	iBusy=ETrue;
	iNetDatabase->Query(aBuffer);
	return KErrNone;
}

/**
Adds a record to the database
@param aBuffer The record to add 
*/
void CInternalNetDB::Add(TDes8& aBuffer)
{
	iNetDatabase->Add(aBuffer);
}

/**
Removes a record from the database
@param aBuffer The record to remove  
*/
void CInternalNetDB::Remove(TDes8& aBuffer)
{
	iNetDatabase->Add(aBuffer);
}

//***********************************************************************

/**
Constructor.
*/
CInternalSocketTimers::CInternalSocketTimers(CInternalSocket *aCIntSock)
{
	__DECLARE_NAME(_S("CInternalSocketTimers"));
	iInternalSocket=aCIntSock;
	iInternalSocketTimerH=NULL;	
}

/**
Destructor.
*/
CInternalSocketTimers::~CInternalSocketTimers()
{
	if (iInternalSocketTimerH)
		StopInternalSocketTimer();
}

/**
Initialise the value of iInternalSocket for static member functions
@param aCIntSock A pointer to CInternalSocket object.
*/
CInternalSocketTimers *CInternalSocketTimers::NewL(CInternalSocket *aCIntSock)
{
	return new (ELeave) CInternalSocketTimers(aCIntSock);
}

/**
Invoked to start the Internal Socket timer
Can either complete as a call back to the
static CInternalSocket::InternalSocketTimerExpired or can cancel.
@param aCallBack Encapsulates a general call-back function.
@param aTimeout Time out period
*/
void CInternalSocketTimers::StartInternalSocketTimer(TCallBack aCallBack,TInt aTimeout)
{
	if (iInternalSocketTimerH)
		StopInternalSocketTimer();
	iInternalSocketTimer.Set(aCallBack);
	iInternalSocketTimerH=&iInternalSocketTimer;
	IrlanTimer::Queue(aTimeout,iInternalSocketTimer);
}

/**
Invoked to stop a previously queued Internal Socket timer
*/
void CInternalSocketTimers::StopInternalSocketTimer()
{
	if (iInternalSocketTimerH)
		IrlanTimer::Remove(iInternalSocketTimer);
	iInternalSocketTimerH=NULL;
}

void CInternalSocketTimers::DoInternalSocketTimerExpired()
{ 
	iInternalSocketTimerH=NULL;
}




