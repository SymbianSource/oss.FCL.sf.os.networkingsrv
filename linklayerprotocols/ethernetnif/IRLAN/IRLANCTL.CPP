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
// Irlan connection control engine 
// 
//

/**
 @file
*/

#include <nifman.h>
#include <nifvar.h>
#include <nifutl.h>
#include <es_mbuf.h>
#include <in_pkt.h>
#include <ir_sock.h>
#include "PKTDRV.H"
#include "ETHINTER.H"
#include "IRLAN.H"
#include "IRLANUTL.H"
#include "INTSOCK.H"
#include "IRLANBUF.H"
#include "IRLANDAT.H"
#include "irlantimer.h"
#include <nifmbuf.h>

//#define __TRACEWIN__
#ifdef __TRACEWIN__
  #include <log.h>
#else
  #define LOG(a)
#endif

/**
Create a new CIrlanControlEngine object.
@param aPktDrv A pointer to CIrlanPktDrv object.
@return A pointer to CIrlanControlEngine object.
*/
CIrlanControlEngine *CIrlanControlEngine::NewL(CIrlanPktDrv* aPktDrv)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanControlEngine::NewL\r\n")));
#endif
	CIrlanControlEngine *cc=new (ELeave) CIrlanControlEngine;
	CleanupStack::PushL(cc);
	cc->ConstructL(aPktDrv);
	CActiveScheduler::Add(cc);
	CleanupStack::Pop();
	return cc;
}

/**
Think this should be a deferred deletion here especially
if IrLAN is already connected.
Destructor.
*/
CIrlanControlEngine::~CIrlanControlEngine()
{
	LOG(Log::Printf(_L("IRLAN	~CIrlanControlEngine\r\n")));
	CIrlanParameter *par;
	TDblQueIter<CIrlanParameter> i(iIrlanParameterList);
	while (par=i++,par!=NULL)
		{
		par->iLink.Deque();
		delete par;
		}
	iDataSendQ.Free();

	if (iTimers)
		iTimers->StopIrlanControlEngineTimer();
	delete iTimers;
	if (iSender)
		{
		iSender->Cancel();
		delete iSender;
		}
	if (iReceiver)
		{
		iReceiver->Cancel();
		delete iReceiver;
		}
	delete iHostResolver;   // do close on this too?
	delete iNetDatabase;	// do close on this too?
	if (iDataSock)
		iDataSock->Close();
	if (iControlSock)
		iControlSock->Close();

	delete iRecvBuffer;
	CActive::Cancel();
}

/**
Default constructor
*/
CIrlanControlEngine::CIrlanControlEngine()
	:CActive(EIrlanControlPriority)
	,iRecvBufPtr(NULL,0)
{
}

/**
The CIrlanControlEngine::ConstructL performs all the memory allocating initialisations.
@param aNotify A pointer to CIrlanPktDrv object.
*/
void CIrlanControlEngine::ConstructL(CIrlanPktDrv* aNotify)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CIrlanControlEngine::ConstructL\r\n")));
#endif
	iRecvBufLength=KIrlanBufSize;
	iTimers=CIrlanControlEngineTimers::NewL(this);
	iRecvBuffer=HBufC8::NewMaxL(iRecvBufLength);
	TPtr8 temp=iRecvBuffer->Des();
	iRecvBufPtr.Set(temp);
	iNotify=aNotify;
    iIrlanParameterList.SetOffset(_FOFF(CIrlanParameter,iLink));
}

/**
Handles Errors.
*/
void CIrlanControlEngine::HandleErrorL()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Error in CIrlanControlEngine::RunL\r\n")));
#endif
   	switch (iState)
		{
	case E_Idle:
	case E_Query:
	case E_Conn:
		PassiveIdleTransitionL();
		iState=E_PassiveIdle;
		iDiscoveredDevice=0;
		break;
	case E_Data:
		{
		iState=E_Idle;
		iDiscoveredDevice=0;
		TPtr8 dest(&iHWAddr[0],6,6);
		dest.FillZ(6);
		// Tear down the link stuff.
		delete iSender;
		iSender=0;
		delete iReceiver;
		iReceiver=0;
		break;
		}
	default:
		break;
		}
}

/**
Irlan state machine - on hitting this RunL, either iStatus is KErrNone in which case
we have completed OK and can try the next stage or it isn't in which case we must
handle the error appropriately.  The state machine is only rearmed if the phase action
produces some error.  Otherwise, it is left up to the IrDA PRT to call back via the
socket/hostresolver/netDB notifer and for the request to be completed that way.
*/
void CIrlanControlEngine::RunL()
{
#ifdef __TRACEWIN__
	PrintState();
#endif
	RMBufChain pkt;
	TInt ret=KErrNone;
	if (iStatus!=KErrNone)
		{
		HandleErrorL();
		return;
		}
	switch (iState)
		{
	case E_Idle:
		if (iDiscoveredDevice)
			{// An IrDA discovery has been successfully completed.
//			iHostResolver.Close();
			TRAP(ret,QueryRemoteIASL());
			User::LeaveIfError(ret);
			iState=E_Query;
			break;
			}
		ret=AttemptingDiscoveryL();
		// a KErrNone here indicates we're doing a discovery...
		// any other error will be picked up as a PassiveIdleTransition
		break;
	case E_PassiveIdle:
		// the remote side has successfully connected to us
		// now we wait on receipt of remote command frames
		QueueWaitForControlCommand();
		iState=E_Info;
		break;
	case E_Query:
		// A query on remote device's IAS has successfully completed.
  		iOpenRetries=0;
  		ret=ConnectToProviderL();
  		iState=E_Conn;
  		break;
	case E_Conn:
		// Completed OK - send the GetInfoCmd
		ret=GetInfoCmd();
		iState=E_Info;
		break;
	case E_Info:
		// Need to parse response from this query if it comes.
		switch (iAccessType)
			{
		case EAccessPoint:
			if (iAwaitingResponse)
				{
				ParseInfoReply();
				ret=GetMediaCmd();
				iState=E_Media;
				break;
				}
			QueueWaitForControlResponse();
			break;
		case EPeerToPeer:
			if (iAwaitingCommand)
				{
				ParseControlCommand();  // also sends the reply
				break;
				}
			QueueWaitForControlCommand();
			break;
		case EHosted:
			break;
			}
		break;
	case E_Media:
		if (iAwaitingResponse)
			{
			ret=ParseMediaReply();
			User::LeaveIfError(ret);
			ret=OpenDataCmd();
			iState=E_Open;
			break;
			}
		QueueWaitForControlResponse();
		break;
	case E_Open:
		if (iAwaitingResponse)
			{
			ParseOpenDataReply();
			// WE'RE ONLY SUPPORTING ACCESS MODE IRLAN AT PRESENT
			__ASSERT_DEBUG(iAccessType==EAccessPoint,IrlanUtil::Fault(EIrlanInvalidIrlanMode));
			ret=GetDirectedFilterConfigCmd();
			iState=E_FilterConfig;
			break;
			}
		QueueWaitForControlResponse();
		break;
	case E_FilterConfig:
		// This is an extra state I've added to handle filter config stuff
		if (iAwaitingResponse)
			{
			ParseDirectedFilterConfigReply();
			ret=SetDirectedFilterOperationCmd();
			iState=E_FilterOperation;
			break;
			}
		QueueWaitForControlResponse();
		break;
	case E_FilterOperation:
		// This is an extra state I've added to handle filter operation stuff
		if (iAwaitingResponse)
			{
			ParseDirectedFilterOperationReply();
			ret=SetBroadcastFilterOperationCmd();
			iState=E_FilterBroadcast;
			break;
			}
		QueueWaitForControlResponse();
		break;
	case E_FilterBroadcast:
		// This is an extra state I've added to handle broadcast filter stuff
		if (iAwaitingResponse)
			{
			ParseBroadcastFilterOperationReply();
			ConnectToDataChannelL();
			break;
			}
		QueueWaitForControlResponse();
		break;
	case E_Wait:
		// Only needed if acting as a provd.
		break;
	case E_Arb:
		// Only needed if acting as a provd.
		break;
	case E_Data:
		// Once we're here, we can send/receive ethernet packets as data over IrLAN.
		iSender=CIrlanSender::NewL(this,iDataSock);
		iReceiver=CIrlanReceiver::NewL(this,iDataSock);
 		iReceiver->QueueRead();
		if (iDataSendQ.Remove(pkt))
			{
			iSender->QueueSend(pkt);
			pkt.Free();
			}
        iNotify->LinkLayerUp(); //bring up the tcpip stack ect.
		return;
	case E_Close:

		break;
	case E_Sync:
		break;
	default:
		IrlanUtil::Panic(EIrlanErrorState);
		break;
		}
	if (ret!=KErrNone)
	  	ActivateStateMachine(ret);
}


/**
Process the packet received from the LinkLayer.
*/
void CIrlanControlEngine::ProcessReceivedPacketL()
{
	RMBufPacket pkt;

	pkt.CreateL(iRecvBufPtr,0);
	pkt.Pack();
	iNotify->Process(pkt);	// We had a read queued and that has completed
}


/**
Starts up the state machine in E_Idle state - will lead to a discovery attempt.
*/
void CIrlanControlEngine::StartL()
{
	TInt ret;
	iHostResolver=CInternalHostResolver::NewL();
	if ((ret=iHostResolver->OpenL(_L("IrTinyTP"),this))!=KErrNone)
		{
		LOG(Log::Printf(_L("IRLAN	CInternalHostResolver::OpenL failed with err=%d\r\n"),ret));
		User::Leave(ret);
		}
	iState=E_Idle;
	iProtocol=iHostResolver->iProtocol;
	ActivateStateMachine();
}

/**
cancellation of an outstanding request.
*/
void CIrlanControlEngine::DoCancel()
{
	switch (iState)
		{
	case E_Data:
		iSender->DoCancel();
		iReceiver->DoCancel();
		break;
	default:
		break;
		}
}

/** 
Protocol inspects return
It blocks sending if it receives a return <= 0
This value should be propogated up through the stack
@internalComponent
*/
const TInt KStopSending		= 0;

/**
Protocol inspects return to indicate Keep sending the data.
@internalComponent
*/
const TInt KContinueSending	= 1;

/**
Sender Class is generic and does not want to know about RMBuf's
Copy to a Heap Buffer and Free the packet. EtherII MAC layer comments
Say we should free the packet buffer
RMBuf could contain a chain so get into a contiguous buffer
@param aPdu		A reference to the packet to be sent (really an RMBufPkt)
@param aSource  A pointer to the source protocol.
@return 0 Tells the higher layer to stop sending the data.
		1 Tells higher layer that it can continue sending more data.
*/
TInt CIrlanControlEngine::Send(RMBufChain& aPdu, TAny*)
{
	if (iState!=E_Data)
		{// Not ready to send the packet.
		aPdu.Free();
		return KStopSending;
		}
	// Need to stick the packet in a queue and kick the sender...
	iDataSendQ.Append(aPdu);
	iSender->KickSender();
	return KContinueSending;
}


/**
This kicks off the state machine active object.  Completion in RunL..
@param aStat The active object Request complete status.
*/
void CIrlanControlEngine::ActivateStateMachine(TInt aStat)
{
	TRequestStatus *pS=&iStatus;
	User::RequestComplete(pS,aStat);
	SetActive();
}

//*************************** NOTIFIERS **********************************

/**
Upcall from SAP - more data arrived
Indicates that new data is available on a service access point
*/
void CIrlanControlEngine::NewData(TUint /*aCount*/)
{
}

/**
Upcall from SAP - flow control on
Indicates that new buffer space is available on a service 
*/
void CIrlanControlEngine::CanSend()
{
	if (iState==E_Data)
		{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CanSend received from SAP\r\n")));
#endif
		// if there's any data in the queue, force send it!
		if (!iDataSendQ.IsEmpty())
			iSender->KickSender();
		return;
		}
}

/**
This returns the hardware address pulled out of response to
IrLAN FILTER_OPERATION/DYNAMIC request.
@return NULL Failure.
		(NULL Terminated Binary String) The Hardware Address of the interface. LAN Device 
		Specific
@note this will not contain the correct MAC address until IRLAN recives a control frame.
*/
TUint8* CIrlanControlEngine::GetInterfaceAddress()
{
	return &iHWAddr[0];
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
*/
void CIrlanControlEngine::ConnectComplete()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect complete 1\r\n")));
#endif
	ActivateStateMachine();
}

#pragma warning (disable:4100)
/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aConnectData Connect data (if supported)  
*/
void CIrlanControlEngine::ConnectComplete(const TDesC8& /*aConnectData*/)
{
	PrintState();
	TInt ret=KErrNone;
	switch (iState)
		{
	case E_Conn:
		// Successful completion of control channel connect.
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect complete 2 data len=%d\r\n"),aConnectData.Length()));
#endif
		break;
	case E_FilterBroadcast:
		// Successful completion of data channel connect.
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect complete 2 data len=%d\r\n"),aConnectData.Length()));
#endif
	    iState=E_Data;			// READY TO TRY AND SEND/RECEIVE PACKETS!
		break;
	default:
		ret=KErrNotSupported;
		break;
		}
	ActivateStateMachine(ret);
}
#pragma warning (default:4100)

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aSSP The new SSP for passive opens 
*/
void CIrlanControlEngine::ConnectComplete(CServProviderBase& aSSP)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect complete 3 - not supported!!\r\n")));
#endif
	iAcceptControlSock=CInternalSocket::NewL(&aSSP,this);
	ActivateStateMachine();
}

/**
Upcall from SAP.  Notify layer above
Indicates that a connection attempt has completed successfully
@param aSSP The new SSP for passive opens 
@param aConnectData Connect data (if supported)  
*/
void CIrlanControlEngine::ConnectComplete(CServProviderBase& aSSP,
  const TDesC8& /*aConnectData*/)
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect complete on accept socket\r\n")));
#endif
	iAcceptControlSock=CInternalSocket::NewL(&aSSP,this);
	ActivateStateMachine();
}

/**
Upcall from SAP.  Notify layer above.
Indicates that the SAP has finished closing down
@param aDelete     Delete SAP 
*/
void CIrlanControlEngine::CanClose(TDelete /*aDelete*/)
{
}

/**
Upcall from SAP.  Notify layer above.
Indicates that the SAP has finished closing down
@param aDisconnectData Any user data carried on the disconnect frame 
@param aDelete         Delete SAP 
*/
void CIrlanControlEngine::CanClose(const TDesC8& /*aDisconnectData*/,TDelete /*aDelete*/)
{
}

/**
Upcall from SAP - Error notification.
@param aError			The error code
@param anOperationMask  A bitmask of TOperationBitmasks values specifying which pending operations are
						affected by the Error up-call
*/
void CIrlanControlEngine::Error(TInt /*anError*/,TUint /*anOperationMask*/)
{
}

/**
Upcall from SAP - Not supported
Indicates that the other end of a connection has disconnected. 
*/
void CIrlanControlEngine::Disconnect(void)
{
}

/**
Indicates that the other end of a connection has disconnected

@param aDisconnectData User data in the disconnect frame.
*/
void CIrlanControlEngine::Disconnect(TDesC8& /*aDisconnectData*/)
{
}

/**
Upcall from SAP.  Notify layer above
Indicates that the currently pending Ioctl has completed
@param aBuf Any data requested by the Ioctl operation.
*/
void CIrlanControlEngine::IoctlComplete(TDesC8 * /*aBuf*/)
{
}

void CIrlanControlEngine::NoBearer(const TDesC8& /*aConnectionParams*/)
{
}

void CIrlanControlEngine::Bearer(const TDesC8& /*aConnectionInfo*/)
{
}

/**
Depending on error AND STATE!!, need to take corresponding action.
Notifier call back from IrDA PRT
This is where the request completes - it has already filled
in the name record passed through to GetByName so leave that.
@param aErr Error code.
*/
void CIrlanControlEngine::QueryComplete(TInt aError)
{
	PrintState();
	switch (iState)
		{
	case E_Idle:
		if (aError==KErrNone)	// Successful discovery
			{// iLog already filled in - pull out the device.
			TIrdaSockAddr addr(iLog().iAddr);
			// Check it has IrLAN set in it's discovery hint bits.
			iDiscoveredDevice=addr.GetRemoteDevAddr();
#ifdef __TRACEWIN__
			TUint numHintBytes=addr.GetServiceHintByteCount();
			TUint8 hintByte1=addr.GetFirstServiceHintByte();
			LOG(Log::Printf(_L("IRLAN	!!!!!!!!!!!!! SUCCESSFUL DISCOVERY !!!!!!!!!!!!!!!\r\n")));
			LOG(Log::Printf(_L("IRLAN	Remote machine dev addr=0x%08x\r\n"),iDiscoveredDevice));
			LOG(Log::Printf(_L("IRLAN	Remote no. of service hint bytes is %d\r\n"),numHintBytes));
			LOG(Log::Printf(_L("IRLAN	Remote first service hint byte is 0x%02x\r\n"),hintByte1));
			if (numHintBytes>1)
				{
				LOG(Log::Printf(_L("IRLAN	Remote second service hint byte is 0x%02x\r\n"),
					addr.GetSecondServiceHintByte()));
				}
			LOG(Log::Printf(_L("IRLAN	Remote machine name is \"%s\"\r\n"),iLog().iName.PtrZ()));
#endif
			}
		else					// Nothing discovered
			aError=KErrNone;	// This will ensure continued attempts.
		break;
	case E_Query:
		if (aError==KErrNone)
			{// IAS QUERY COMPLETED SUCCESSFULLY.
			iResults.Copy(iQuery);
			PrintIASResults();
			if (iResults.Type()==EIASDataInteger)
				{
				aError=iResults.GetInteger(iIrlanControlPortNum);
				}
			}
		else
			{// IAS QUERY COMPLETED UNSUCCESSFULLY.
			PrintIASError(aError);
			// Need to take corrective action!! (Done in HandleErrorL).
			}
		break;
	default:
		break;
		}
	ActivateStateMachine(aError);	// Kick off state machine again.
}

//********************** IRLAN STATE MACHINE ACTIONS ************************

TInt CIrlanControlEngine::PassiveIdleTransitionL()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	PASSIVE IDLE STATE TRANSITION\r\n")));
#endif
	// first - queue the IAS db entry.
	TIASDatabaseEntry iasentry;
	if (!iNetDatabase)
		{
		iNetDatabase=CInternalNetDB::NewL();
		User::LeaveIfError(iNetDatabase->OpenL(TINYTP_PROTOCOL,this));
		}
	iasentry.SetClassName(IRLAN_CLASSNAME);
	iasentry.SetAttributeName(TINYTP_ATTRNAME);
	iasentry.SetToInteger(9);
	iNetDatabase->Add(iasentry);
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IrLAN	PEER service registered on port 9\r\n")));
#endif

	iListenSock=new (ELeave) CInternalSocket();
	User::LeaveIfError(iListenSock->OpenL(TINYTP_PROTOCOL,this));
	TIrdaSockAddr addr(iLog().iAddr);
	addr.SetPort(0x09);					    // This is where IrLAN PEER is.
	iListenSock->SetLocalName(addr);
	iListenSock->WaitForConnect(2);

	iAccessType=EPeerToPeer;
	/*
	ret=iAcceptor->Open();
	sock1.Accept(sock2,iStatus);	// the order of this accept call has changed in esock
	User::WaitForRequest(stat3);
	// second - do a listen and accept on corresponding port.
	*/

	return KErrNone;
}

TInt CIrlanControlEngine::AttemptingDiscoveryL()
{
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	ATTEMPTING DISCOVERY\r\n")));
#endif
	if (!iDiscoveryAttempts)
		{
		iLog().iName=_S("*");
		iDiscoveryAttempts++;
	    TInt ret=iHostResolver->GetByName(iLog());
		return ret;
		}
	else
		{
		if (iDiscoveryAttempts>=5)
			{// need to drop into passive idle state
			return KErrNotFound;
			}
		// 1 sec between discoveries
		TCallBack callback(CIrlanControlEngine::IrlanControlEngineTimerExpired,this);
		iTimers->StartIrlanControlEngineTimer(callback,1000000L);
		}
	return KErrNone;  // don't want to reactivate state machine
}

TInt CIrlanControlEngine::IrlanControlEngineTimerExpired(TAny *aIrlan)
{
	CIrlanControlEngine *cc=(CIrlanControlEngine *)aIrlan;
	switch (cc->iState)
		{
	case E_Idle:
		// discovery timer has completed.  try a discovery.
		cc->iTimers->DoIrlanControlEngineTimerExpired();
		cc->iDiscoveryAttempts++;
		cc->iHostResolver->GetByName(cc->iLog());
		break;
	default:
		break;
		}
	return 0;
}

/**
Need to find out if the remote supports IrLAN.  Note that this will involve
kicking up the IrLAP/IAS connections.  Requires opening up a net database.
@return TRUE if the Classname, AttributeType and the Device is present otherwise some error code.
*/
TInt CIrlanControlEngine::QueryRemoteIASL()
{
	TInt ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	QUERY REMOTE IAS\r\n")));
#endif
	TRAP(ret,iNetDatabase=CInternalNetDB::NewL());
	User::LeaveIfError(ret);
	if ((ret=iNetDatabase->OpenL(TINYTP_PROTOCOL,this))!=KErrNone)
		{
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	CInternalNetDB::OpenL failed with err=%d\r\n"),ret));
#endif
		User::Leave(ret);
		}
	return DoIASQuery(IRLAN_CLASSNAME,TINYTP_ATTRNAME,iDiscoveredDevice);
}

/**
Need to connect to the IrLAN port number held in iIrlanPortNumber.  Requires
opening up a control socket.
@return TRUE if Success otherwise any error code.
*/
TInt CIrlanControlEngine::ConnectToProviderL()
{
	TInt ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CONNECT TO PROVIDER\r\n")));
#endif
	iControlSock=new (ELeave) CInternalSocket();
	if ((ret=iControlSock->OpenL(TINYTP_PROTOCOL,this))!=KErrNone)
		{
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	CInternalSocket::OpenL failed with err=%d\r\n"),ret));
#endif
		User::Leave(ret);
		}
	return DoControlConnect();
}

/**
Need to connect to the IrLAN port number held in iIrlanPortNumber.  Requires
opening up a control socket.
@return TRUE if Success otherwise any error code.
*/
TInt CIrlanControlEngine::ConnectToDataChannelL()
{
	TInt ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	CONNECT TO DATA CHANNEL\r\n")));
#endif
	iDataSock=new (ELeave) CInternalSocket();
	if ((ret=iDataSock->OpenL(TINYTP_PROTOCOL,this))!=KErrNone)
		{
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	CInternalSocket::OpenL failed with err=%d\r\n"),ret));
#endif
		User::Leave(ret);
		}
	return DoDataConnect();
}

//*************************** UTILITY FUNCTIONS **********************************

/**
Searchs a parameter in the parameter List.
@return A pointer to CIrlanParameter class
*/
CIrlanParameter* CIrlanControlEngine::LookUpParameter(const TDesC8& aName)
{
	CIrlanParameter *par;
	TDblQueIter<CIrlanParameter> i(iIrlanParameterList);

	while (par=i++,par!=NULL)
		{
		if (par->Match(aName))
			return par;
		}
	return NULL;
}

void CIrlanControlEngine::SendIrlanResponseFrame(RMBufChain& aPdu)
{
	TInt ret;
	ret =iAcceptControlSock->Write(aPdu,NULL,iStatus,0);
	if (ret!=KErrNone)
		{// what do we do here - panic?
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	Failure on iControlSock Send\r\n")));
#endif
		return;
		}
	SetActive();
	aPdu.Free();
}

void CIrlanControlEngine::SendIrlanControlFrame(RMBufChain& aPdu)
{
	TInt ret;
	ret =iControlSock->Write(aPdu,NULL,iStatus,0);
	if (ret!=KErrNone)
		{// what do we do here - panic?
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	Failure on iControlSock Send\r\n")));
#endif
		return;
		}
	SetActive();
	aPdu.Free();
}

void CIrlanControlEngine::QueueWaitForControlCommand()
{
	// gotta do something with new data here....

	iRecvBufPtr.SetLength(iRecvBufLength);
	TRAPD(ret,iAcceptControlSock->Recv(iRecvBufPtr,NULL,iStatus,0));
	if (ret !=KErrNone)
		{
#ifdef __TRACEWIN__		
		LOG(Log::Printf(_L("IRLAN	Failure on iControlSock Recv - Control Command : %d\r\n"),ret));
#endif		
		}
	iAwaitingCommand=ETrue;
	SetActive();
}

void CIrlanControlEngine::QueueWaitForControlResponse()
{
	iRecvBufPtr.SetLength(iRecvBufLength);
	TInt ret;
	ret =iControlSock->Recv(iRecvBufPtr,NULL,iStatus,0);
	if (ret !=KErrNone)
		{
#ifdef __TRACEWIN__
		LOG(Log::Printf(_L("IRLAN	Failure on iControlSock Recv\r\n")));
#endif
		return;
		}
	iAwaitingResponse=ETrue;
	SetActive();
}

/**
Asynchronous IAS query.  RTimer timeout of 5 seconds on the query.
@param aClassName		The Class Name.
@param aAttributeName	Attribute name.
@param aRemDevAddr		Remote device address.
@return Error code.
*/
TInt CIrlanControlEngine::DoIASQuery(const TDesC8& aClassName,
  const TDesC8& aAttributeName,TUint aRemDevAddr)
{
	TInt ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	IAS Query on \"%S\"|\"%S\"\r\n"),&aClassName,&aAttributeName));
#endif
	// Remember that netDB's return results in SAME buffer.
	iQuery.Set(aClassName,aAttributeName,aRemDevAddr);
	ret=iNetDatabase->Query(iQuery);
	return ret;
}

TInt CIrlanControlEngine::DoControlConnect()
{
	TSockAddr addr;
	// Set home port number.
	TUint8 home=KIrlanHomeControlPort;
	addr.SetPort(home);
	TInt ret=iControlSock->SetLocalName(addr);
	if (ret!=KErrNone)
		return ret;
	addr.SetPort(TUint8(iIrlanControlPortNum));
	// Enable segmentation/reassembly.
	TPckgBuf<TUint> param(1024);	// Set segmentation/reassembly size
	// const TUint KTinyTPLocalSegmentSize=7: This value is advertised to the
	// remote machine as the max amount of data we can reassemble
	// const TUint KTinyTPRemoteSegmentSize=8: Remote machine is unable
	// to reassemble more data than this.
	if ((ret=iControlSock->SetOption(KLevelIrmux,KTinyTPLocalSegSizeOpt,param))!=KErrNone)
		return ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect on SLSAP=%d,DLSAP=%d\r\n"),
	  home,iIrlanControlPortNum));
#endif
	return iControlSock->Connect(addr);
}

TInt CIrlanControlEngine::DoDataConnect()
{
	TSockAddr addr;
	// Set home port number.
	TUint8 home=KIrlanHomeDataPort;
	addr.SetPort(home);
	TInt ret=iDataSock->SetLocalName(addr);
	if (ret!=KErrNone)
		return ret;
	addr.SetPort(TUint8(iIrlanDataPortNum));
	// Enable segmentation/reassembly.
	TPckgBuf<TUint> param(1518);	// Set segmentation/reassembly size
	if ((ret=iDataSock->SetOption(KLevelIrmux,KTinyTPLocalSegSizeOpt,param))!=KErrNone)
		return ret;
#ifdef __TRACEWIN__
	LOG(Log::Printf(_L("IRLAN	Control channel connect on SLSAP=%d,DLSAP=%d\r\n"),
	  home,iIrlanDataPortNum));
#endif
	return iDataSock->Connect(addr);
}

//######################################################################################

/**
Constructor.
@param aCIrlan A pointer to CIrlanControlEngine object.
*/
CIrlanControlEngineTimers::CIrlanControlEngineTimers(CIrlanControlEngine *aCIrlan)
{
	__DECLARE_NAME(_S("CIrlanControlEngineTimers"));
	iIrlanControlEngine=aCIrlan;
	iIrlanControlEngineTimerH=NULL;
}

/**
Destructor.
*/
CIrlanControlEngineTimers::~CIrlanControlEngineTimers()
{
	if (iIrlanControlEngineTimerH)
		StopIrlanControlEngineTimer();
}

/**
Initialise the value of iIrlanControl for static member functions
@param aCIrlan A pointer to CIrlanControlEngine object.
@return A pointer to CIrlanControlEngineTimers class.
*/
CIrlanControlEngineTimers *CIrlanControlEngineTimers::NewL(CIrlanControlEngine *aCIrlan)
{
	return new (ELeave) CIrlanControlEngineTimers(aCIrlan);
}

/**
Invoked to start the Internal Socket timer Can either complete as a call back to the
static CIrlanControlEngine::IrlanControlEngineTimerExpired or can cancel.
@param aCallBack Encapsulates a general call-back function.
@param aTimeout Time out period
*/
void CIrlanControlEngineTimers::StartIrlanControlEngineTimer(TCallBack aCallBack,TInt aTimeout)
{
	if (iIrlanControlEngineTimerH)
		StopIrlanControlEngineTimer();
	iIrlanControlEngineTimer.Set(aCallBack);
	iIrlanControlEngineTimerH=&iIrlanControlEngineTimer;
	IrlanTimer::Queue(aTimeout,iIrlanControlEngineTimer);
}

/**
Invoked to stop a previously queued Internal Socket timer
*/
void CIrlanControlEngineTimers::StopIrlanControlEngineTimer()
{
	if (iIrlanControlEngineTimerH)
		IrlanTimer::Remove(iIrlanControlEngineTimer);
	iIrlanControlEngineTimerH=NULL;
}

void CIrlanControlEngineTimers::DoIrlanControlEngineTimerExpired()
{
	iIrlanControlEngineTimerH=NULL;
}
