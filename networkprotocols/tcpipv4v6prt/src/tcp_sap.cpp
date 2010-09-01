// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// tcp_sap.cpp - TCP service access point
// TCP Service Access Point and most of the protocol logic.
//



/**
 @file tcp_sap.cpp
*/

#include "tcp.h"
#include <in6_dstcache.h>
#include <in6_opt.h>
#include <in6_if.h>
#include <in6_dstcache_internal.h>
#include <nifman_internal.h>

#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
#include <in_sock.h>
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

// speed optimisations
#ifdef __ARMCC__
#pragma push
#pragma arm
#endif

#define SIGNED_UNSIGNED_FIX

// Copied from ip6.cpp. Should move to some common definition file?
static const TLitC8<sizeof(TInt)> KInetOptionDisable = {sizeof(TInt), {0}};

#define SYMBIAN_NETWORKING_UPS

//
//
// TCP state diagram from RFC793
//
//
//                              +---------+ ---------\      active OPEN
//                              |  CLOSED |            \    -----------
//                              +---------+<---------\   \   create TCB
//                                |     ^              \   \  snd SYN
//                   passive OPEN |     |   CLOSE        \   \.
//                   ------------ |     | ----------       \   \.
//                    create TCB  |     | delete TCB         \   \.
//                                V     |                      \   \.
//                              +---------+            CLOSE    |    \.
//                              |  LISTEN |          ---------- |     |
//                              +---------+          delete TCB |     |
//                   rcv SYN      |     |     SEND              |     |
//                  -----------   |     |    -------            |     V
// +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
// |         |<-----------------           ------------------>|         |
// |   SYN   |                    rcv SYN                     |   SYN   |
// |   RCVD  |<-----------------------------------------------|   SENT  |
// |         |                    snd ACK                     |         |
// |         |------------------           -------------------|         |
// +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
//   |           --------------   |     |   -----------
//   |                  x         |     |     snd ACK
//   |                            V     V
//   |  CLOSE                   +---------+
//   | -------                  |  ESTAB  |
//   | snd FIN                  +---------+
//   |                   CLOSE    |     |    rcv FIN
//   V                  -------   |     |    -------
// +---------+          snd FIN  /       \   snd ACK          +---------+
// |  FIN    |<-----------------           ------------------>|  CLOSE  |
// | WAIT-1  |------------------                              |   WAIT  |
// +---------+          rcv FIN  \                            +---------+
//   | rcv ACK of FIN   -------   |                            CLOSE  |
//   | --------------   snd ACK   |                           ------- |
//   V        x                   V                           snd FIN V
// +---------+                  +---------+                   +---------+
// |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
// +---------+                  +---------+                   +---------+
//   |                rcv ACK of FIN |                 rcv ACK of FIN |
//   |  rcv FIN       -------------- |    Timeout=2MSL -------------- |
//   |  -------              x       V    ------------        x       V
//    \ snd ACK                 +---------+delete TCB         +---------+
//     ------------------------>|TIME WAIT|------------------>| CLOSED  |
//                              +---------+                   +---------+
//
//  LISTEN - represents waiting for a connection request from any remote
//  TCP and port.
//
//  SYN-SENT - represents waiting for a matching connection request
//  after having sent a connection request.
//
//  SYN-RECEIVED - represents waiting for a confirming connection
//  request acknowledgment after having both received and sent a
//  connection request.
//
//  ESTABLISHED - represents an open connection, data received can be
//  delivered to the user.  The normal state for the data transfer phase
//  of the connection.
//
//  FIN-WAIT-1 - represents waiting for a connection termination request
//  from the remote TCP, or an acknowledgment of the connection
//  termination request previously sent.
//
//  FIN-WAIT-2 - represents waiting for a connection termination request
//  from the remote TCP.
//
//  CLOSE-WAIT - represents waiting for a connection termination request
//  from the local user.
//
//  CLOSING - represents waiting for a connection termination request
//  acknowledgment from the remote TCP.
//
//  LAST-ACK - represents waiting for an acknowledgment of the
//  connection termination request previously sent to the remote TCP
//  (which includes an acknowledgment of its connection termination request).
//
//  TIME-WAIT - represents waiting for enough time to pass to be sure
//  the remote TCP received the acknowledgment of its connection
//  termination request.
//
//  CLOSED - represents no connection state at all.
//
//  CONNECT - represents waiting for a network path to become ready after
//  the user has issued an active OPEN request. The request waits in this
//  state until all the necessary negotiations have been accomplished,
//  including the establishment of security and mobility bindings. CONNECT is
//  specific to this TCP implementation and is therefore not visible in the
//  state chart above. It would be located between CLOSED and SYN-SENT.
//

//The below is UID of the client(http client) using this option. We are not exposing this right now...
const TUint32 KSoTcpLingerinMicroSec = 0x101F55F6;
#ifdef _LOG
const TText *CProviderTCP6::TcpState(TUint aState)
	{
	TInt i;
	static const TText* const tcpStates[] = {
		_S("CONSTRUCTING"),
			_S("INITIAL"),
			_S("LISTEN"),
			_S("SYN-SENT"),
			_S("SYN-RECEIVED"),
			_S("ESTABLISHED"),
			_S("FIN-WAIT-1"),
			_S("FIN-WAIT-2"),
			_S("CLOSE-WAIT"),
			_S("CLOSING"),
			_S("LAST-ACK"),
			_S("TIME-WAIT"),
			_S("CLOSED"),
			_S("CONNECT"),
			_S("INVALID"),
			NULL
			};

	if (aState == ~0UL)
		aState = iState;
	for (i = 0; aState && tcpStates[i+1]; aState >>= 1)
		i++;
	return tcpStates[i];
	}
#endif

CProviderTCP6::CProviderTCP6(CProtocolInet6Base* aProtocol)
	: CProviderInet6Transport(aProtocol)
	{
	__DECLARE_NAME(_S("CProviderTCP6"));
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW	
	iWindowSetByUser = EFalse;
#endif	
	}

CProviderTCP6::~CProviderTCP6()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] being deleted"), (TInt)this));
	Stop();
	FreeQueues();

	delete iTransmitter;
	delete iDelayAckTimer;
	delete iRetransTimer;
	delete iLingerTimer;

	// Delete server socket state
	if (iParent)
		{
		// Notify parent.
		iParent->SetChildDeleted(ETrue);
		iParent->DetachChild(this);
		}
	else if (iListenQueue)
		{
		//
		// Delete all pending child sockets. The child socket destructor will
		// call DetachChild(), removing itself from the socket queue. If the
		// socket has already been registered with the socket server, we will
		// simply detach it here.
		//
		while (iConnectCount)
			{
			ASSERT(iListenQueue[0]->iParent == this);
			if (iListenQueue[0]->iSockFlags.iAttached)
				DetachChild(iListenQueue[0]);
			else
			    {
			    if (iListenQueue[0]->InState(ETcpSynReceived))
			    	{
			    	iListenQueue[0]->ClearSYNSettings();
				    iListenQueue[0]->iSockFlags.iSendClose = ETrue;
				    iListenQueue[0]->iSockFlags.iRecvClose = ETrue;
				    iListenQueue[0]->SendSegments();
				    DetachChild(iListenQueue[0]);
			    	}
			   	else
					delete iListenQueue[0];
			    }
			}
		delete[] iListenQueue;
		}
	}

//
// Initialize a SAP with default values
//
void CProviderTCP6::InitL()
	{
	TCallBack sender(SenderCallBack, this);
	TCallBack receiver(ReceiverCallBack, this);
	TCallBack delack(DelayAckCallBack, this);
	TCallBack transmitter(TransmitterCallBack, this);
	TCallBack retransmitter(RetransmitterCallBack, this);
	TCallBack linger(LingerTimerCallBack, this);

	CProviderInet6Base::InitL();
	iFlow.SetProtocol(KProtocolInetTcp);
	iFlow.SetNotify(this);

	iSendQ.InitL(transmitter, 13);
	iRecvQ.InitL(receiver, 12);

	iTransmitter = new CAsyncCallBack(sender, KInet6DefaultPriority);
	iDelayAckTimer = new CTcpTimer(delack);
	iRetransTimer = new CTcpTimer(retransmitter);
	iLingerTimer = new CTcpTimer(linger);
	if (!iTransmitter || !iDelayAckTimer || !iRetransTimer || !iLingerTimer)
		User::Leave(KErrNoMemory);

	iDelayAckTimer->InitL();
	iRetransTimer->InitL();
	iLingerTimer->InitL();
	iSockInBufSize        = Protocol()->RecvBuf();
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	if(iSockInBufSize == Protocol()->RecvBufFromIniFile())
	    iSocketStartupCase = ETrue;
	else
	    iSocketStartupCase = EFalse;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW    	
	
	iSockOutBufSize       = Protocol()->SendBuf();
	iSsthresh		= KMaxTInt32;
	iRTO			= Protocol()->InitialRTO();
	ClearRTT();
	iMSS                  = Protocol()->MSS();
	iSMSS			= KTcpStandardMSS;
	iRMSS			= KTcpStandardMSS;
	iLinger		= -1;  // linger disabled
	iFlags.iSackOk	    = Protocol()->Sack();
	iFlags.iUseTimeStamps	    = Protocol()->TimeStamps();
	iFlags.iEcn		    = (Protocol()->Ecn() != 0);

	// Report ICMP errors to application
	iSockFlags.iReportIcmp    = ETrue;

	if (iFlags.iUseTimeStamps)
		iOptions.SetTimeStamps(0, 0);

	if (iFlags.iSackOk)
		iOptions.SetSackOk();

	iOptions.SetAlignOpt(Protocol()->AlignOpt());
	iStartTime.UniversalTime();

	iState = ETcpInitial;
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
    iHiddenFreeWindow 	= 0;
    iNewTcpWindow    	= 0;
    iTcpMaxRecvWin       = Protocol()->RecvMaxWnd();
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW

	}


void CProviderTCP6::ReadDestinationCache()
	{
	if (!Protocol()->DstCache())  // Dest. cache is not enabled as ini parameter
		{
		return;
		}

	const TInetAddr& dstaddr = iFlow.FlowContext()->RemoteAddr();
	if (dstaddr.IsUnspecified())
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ReadDestinationCache() : No destination address"), (TInt)this));
		return;
		}

	TInt err = KErrNone;
	MDestinationCache *dcache = NULL;
	TRAP(err, dcache = IMPORT_API_L(Protocol()->Interfacer(), MDestinationCache));
	if (err != KErrNone || dcache == NULL)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ReadDestinationCache() : DstCache not available"), (TInt)this));
		return;
		}

	const TCacheInfo *cinfo = dcache->Find(dstaddr);
	if (!cinfo)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ReadDestinationCache() : No match for address in cache"), (TInt)this));
		return;
		}

	if (cinfo->iMetrics[TCacheInfo::ESsThresh])
		{
		iSsthresh = cinfo->iMetrics[TCacheInfo::ESsThresh];
		}
	if (cinfo->iMetrics[TCacheInfo::ESRtt])
		{
		iSRTT = cinfo->iMetrics[TCacheInfo::ESRtt];
		}
	if (cinfo->iMetrics[TCacheInfo::ERto])
		{
		iRTO = cinfo->iMetrics[TCacheInfo::ERto];
		}

	LOG(Log::Printf(_L(
		"\ttcp SAP[%u] ReadDestinationCache() : Matching DstCache entry found [0x%08x] - ssthresh: %d"),
		(TInt)this, cinfo, iSsthresh));
	}

void CProviderTCP6::StoreDestinationCache()
	{
	if (!Protocol()->DstCache() || !iFlow.FlowContext())
		{
		return;
		}

	const TInetAddr& dstaddr = iFlow.FlowContext()->RemoteAddr();
	if (dstaddr.IsUnspecified())
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] StoreDestinationCache() : No destination address"), (TInt)this));
		return;
		}

	TInt err = KErrNone;
	MDestinationCache *dcache = NULL;
	TRAP(err, dcache = IMPORT_API_L(Protocol()->Interfacer(), MDestinationCache));
	if (err != KErrNone || dcache == NULL)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] StoreDestinationCache() : DstCache not available"), (TInt)this));
		return;
		}

	TCacheInfo cinfo;
	cinfo.ClearAll();
	cinfo.iMetrics[TCacheInfo::ESsThresh] = iSsthresh;
	cinfo.iMetrics[TCacheInfo::ESRtt] = iSRTT;
	cinfo.iMetrics[TCacheInfo::ERto] = iRTO;

	TRAP(err, dcache->StoreL(dstaddr, cinfo));
	if (err != KErrNone)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] StoreDestinationCache() : DstCache store failed"), (TInt)this));
		return;
		}
	}


void CProviderTCP6::Start()
	{
	LOG(Log::Printf(_L("Start\ttcp SAP[%u] enter"), (TInt)this));
	CProviderInet6Transport::Start();
	iFlags.iStarted = ETrue;

	//
	// Detach from parent socket
	//
	if (iParent)
		{
		iSockFlags.iNotify = ETrue;
		iParent->DetachChild(this);

		//
		// Tell ESock which network interface the child socket is using.
		//
		const MInterface *iface = Protocol()->Interfacer()->Interface(iFlow.FlowContext()->Interface());
		if (iface != NULL)
			{
			TPckgBuf<TSoIfConnectionInfo> netinfo;
			netinfo().iIAPId = iface->Scope(EScopeType_IAP);
			netinfo().iNetworkId = iface->Scope(EScopeType_NET);
			LOG(Log::Printf(_L("\ttcp SAP[%u] Bearer IAP=%d, NID=%d"), (TInt)this, netinfo().iIAPId, netinfo().iNetworkId));
			iSocket->Bearer(netinfo);
			}
		//
		// Report error if TCP has been disconnected already
		//
		if (InState(ETcpClosed))
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] %s, Error %d"), (TInt)this, TcpState(), KErrDisconnected));
			Error(KErrDisconnected);
			}
		}

	//
	// If an error has occurred, we must deliver it to the
	// socket server now that the socket notifier has been
	// initialised.
	//
	if (iLastError.iStatus != KErrNone && iErrorMask != 0)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Delayed error %d, mask %b delivered."),
			(TInt)this, iLastError.iStatus, iErrorMask));
		iSocket->Error(iLastError.iStatus, iErrorMask);
		}

	// Wake up the receiver. We might have something to do.
	iRecvQ.Wake();
	LOG(Log::Printf(_L("Start\ttcp SAP[%u] exit"), (TInt)this));
	}


//
// Stop all transmission (except for iSendQ, which will be allowed to drain)
//
void CProviderTCP6::Stop()
	{
	if (iTransmitter)
		CancelTransmit();
	if (iRetransTimer)
		CancelRetransmit();
	if (iDelayAckTimer)
		CancelDelayACK();
	if (iLingerTimer)
		iLingerTimer->Cancel();
	}


//
// Empty all queues
//
void CProviderTCP6::FreeQueues()
	{
	iSockInQ.Free();
	iSockInQLen = 0;
	iSockOutQ.Free();
	iSockOutQLen = 0;
	iFragQ.Free();
	iRecvQ.Free();
	iSendQ.Cancel();
	iSendQ.Free();
	}


//
// Close the socket down
//
void CProviderTCP6::Close()
	{
	StoreDestinationCache();

	iSockFlags.iSendClose = ETrue;
	iSockFlags.iRecvClose = ETrue;
	Protocol()->UnbindProvider(this);
	Stop();
	FreeQueues();
	EnterState(ETcpClosed);
	}


void CProviderTCP6::Ioctl(TUint aLevel, TUint aName, TDes8* aOption)
	{
	LOG(Log::Printf(_L("Ioctl\ttcp SAP[%u] %x, %x"), (TInt)this, aLevel, aName));
	if (aLevel == KSolInetTcp && aName == KIoctlTcpNotifyDataSent)
		{
		iFlags.iDataSentIoctl = ETrue;
		CompleteIoctl(KErrNone);
		}
	else
		CProviderInet6Transport::Ioctl(aLevel, aName, aOption);
	}


void CProviderTCP6::CancelIoctl(TUint aLevel, TUint aName)
	{
	LOG(Log::Printf(_L("CancelIoctl\ttcp SAP[%u] %x, %x"), (TInt)this, aLevel, aName));
	if (aLevel == KSolInetTcp && aName == KIoctlTcpNotifyDataSent)
		CompleteIoctl(KErrCancel);
	else
		CProviderInet6Transport::CancelIoctl(aLevel, aName);
	}


TInt CProviderTCP6::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	{
	TInt ret = KErrNotSupported;

	if (aLevel == KSolInetTcp)
		{
		TInt intValue;
		ret = GetOptionInt(aOption, intValue);

#ifdef _LOG
		if (ret == KErrNone)
			Log::Printf(_L("SetOpt\ttcp SAP[%u] KSolInetTcp, %d, %d"), (TInt)this, aName, intValue);
		else
			Log::Printf(_L("SetOpt\ttcp SAP[%u] KSolInetTcp, %d"), (TInt)this, aName);
#endif

		switch (aName)
			{
		case KSoTcpAsync2MslWait:
			// Not implemented. Not very useful.
			ret = KErrNotSupported;
			break;

		case KSoTcpKeepAlive:
			if (ret == KErrNone)
				{
				if (!intValue)
					{
					iFlags.iHaveKeepAlive = FALSE;
					iFlags.iHaveTriggeredKeepAlive = FALSE;
					}
				if (intValue & 1)
					iFlags.iHaveKeepAlive = TRUE;
				if (intValue & 2)
					iFlags.iHaveTriggeredKeepAlive = TRUE;
				}
			break;

		case KSoTcpMaxSegSize:
			if (ret == KErrNone)
				{
				if (!InState(ETcpInitial))
					ret = KErrLocked;
				else if (intValue < STATIC_CAST(TInt, KTcpMinimumMSS))
					ret = KErrArgument;
				else
					{
					iMSS = intValue;
					iSMSS = iMSS;
					iRMSS = iMSS;
					}
				}
			break;

		case KSoTcpNextSendUrgentData:
			if (ret == KErrNone)
				iFlags.iNextIsUrgent = intValue ? TRUE : FALSE;
			break;

		case KSoTcpNoDelay:
			if (ret == KErrNone)
				iFlags.iNoDelay = intValue ? TRUE : FALSE;
			break;

		case KSoTcpCork:
		case KSoTcpNoPush:
			if (ret == KErrNone)
				{
				iFlags.iCork = intValue ? TRUE : FALSE;
				if (iFlags.iCork == EFalse && aName == KSoTcpCork)
					{
					// When turning Cork off, send pending data from output queue immediately.
					// This is the only difference between Cork and NoPush
					SchedTransmit();
					}
				}
			break;

		case KSoTcpOobInline:
			if (ret == KErrNone)
				iFlags.iOobInline = intValue ? TRUE : FALSE;
			break;
		
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW	
		case KSoTcpMaxRecvWin:
			//Case to set the Max window size from shim.
			if(ret == KErrNone)
				{
				if (InState(ETcpClosed|ETcpFinWait2|ETcpCloseWait|ETcpLastAck))
					ret = KErrLocked;
				else	
				    if(intValue > iTcpMaxRecvWin)
				    {
				    iTcpMaxRecvWin = intValue;
				    }
				
				}
			break;
		case KSoTcpRecvWinAuto:
		    {
		    // If user sets the window size, we have to give the preference for user setting. Automatic
		    // window setting will be then disabled by default, once user calls the SetOption to set the 
		    // receive window, this feature will be diabled by default till the socket is opened.
		    if(iWindowSetByUser)
		        {
		        return KErrNone;
		        }
		    else if (InState(ETcpClosed|ETcpFinWait2|ETcpCloseWait|ETcpLastAck))
		        ret = KErrLocked;
            else if (intValue < STATIC_CAST(TInt, KTcpMinimumWindow))
                iSockInBufSize = KTcpMinimumWindow;
            else
                {
                //If its the startup case, then there should be no algorithm used to shrink
				//or expand the window size from the default value provided in the ini file
                //the new value should be set directly
                if(iSocketStartupCase)
                    {
					//Add the extra window to free window pool
					//if the window being set is greater than what is specified in ini file (startup case), then just overwrite the new window.
					//Add difference to free window
					//else set free window to zero
					if(intValue > iSockInBufSize)
						iFreeWindow += intValue - iSockInBufSize;
					else
						iFreeWindow = 0;
					//set the buffer
                    iSockInBufSize = intValue;
					//disable startup flag.
                    iSocketStartupCase = EFalse;
                    }
                else 
                {
					// Check for minimum value
	                if (intValue < STATIC_CAST(TInt, KTcpMinimumWindow))
	                    {
	                    intValue = STATIC_CAST(TInt, KTcpMinimumWindow);
	                    }
	                // Handle the situation where the connection has been established and 
	                // window scaling is not in use
	                if ( InState( ETcpSynReceived | ETcpEstablished ) && !iRcvWscale )
	                    {
	                    // Do not allow window sizes larger than 0xFFFF
	                    intValue = Min ( intValue, 0xFFFF );
	                    }

	                // Check whether we are increasing or decreasing window size
	                if ( intValue >= iSockInBufSize )
	                    {
	                    // New window is larger than current one, check if a
	                    // shrinking process is active
	                    if ( !iNewTcpWindow )
	                        {
	                        // Mark new "space" as free, it will be updated to
	                        // peer on next operation.
	                        iFreeWindow += intValue - iSockInBufSize;
	                        }
	                    else
	                        {
	                        // In the middle of shrinking process.
                      if ( iShrinkedWindowSize <= ( intValue - iSockInBufSize ))
	                            {
	                            // Increment to window size is enough to complete
	                            // shrinking process. Update variables and exit
	                            // from shrinking process.
                          iFreeWindow = ( intValue - iSockInBufSize ) - iShrinkedWindowSize;
	                            iShrinkedWindowSize = 0;
	                            iNewTcpWindow = 0;
	                            }
	                        else
	                            {
	                            // Not quite there yet but closer. Less to shrink,
	                            // update this, but do not exit from shrinking
	                            // process
                          iShrinkedWindowSize -= intValue - iSockInBufSize;
	                            iNewTcpWindow = intValue;
	                            }
	                        }
	                    }
	                else
	                    {
	                    // Requested window is smaller than current one. Start or
	                    // continue shrinking process. RCV window can be occupied
	                    // for two different purpose at the moment
	                    // 1. Client data in iSockInQ not read by application
	                    // 2. Free window "opened" to peer (iAdvertisedWindow)
	                    // When shrinking, we must ensure that when reopening
	                    // the window to client there must be truly empty space
	                    // in the window. Thus, freeze the right edge of the
	                    // window (iRCV.NXT + iRCV.WND stays constant) until
	                    // shrinking is completed.
	                
	                    if ( iNewTcpWindow )
	                        {
	                        // There is an ongoing shrink process, add the
	                        // change to the amount to be shrinked
	                        iShrinkedWindowSize += iSockInBufSize - intValue;
	                        iNewTcpWindow = intValue;
	                        }
	                    else
	                        {
	                        // This is a new shrinking process, count how much
	                        // needs to be shrinked
                      iShrinkedWindowSize = iSockInQLen + iRCV.WND;
	                        if ( iShrinkedWindowSize >= intValue )
	                            {
	                            // We need to shrink since the currently occupied
	                            // window does not fit to new one
	                            iShrinkedWindowSize -= intValue;
	                            // There is now free space in the window
	                            iFreeWindow = 0;
	                            // iNewTcpWindow is used as a state variable for
	                            // shrinking
	                            iNewTcpWindow = intValue;
	                            }
	                        else
	                            {
	                            // No need to shrink since we can fit the current
	                            // contents to the new window, update free window
	                            // If TCP connection is not yet setup, the free
	                            // window will be updated on connection setup, 
	                            // for existing connection it will be used
	                            // next time application reads data
	                            if ( iFreeWindow >= ( iSockInBufSize - intValue ))
	                                {
	                                iFreeWindow -= iSockInBufSize - intValue;
	                                }
	                            else 
	                                {
	                                // Something wrong. Try to reevaluate...
	                                iFreeWindow = intValue - iShrinkedWindowSize;
	                                }
	                            iShrinkedWindowSize = 0;
	                            }
	                        }
	                    }
	                // Even in case of window shrink we can set the receive buffer size
	                // immediately. This will be helpful, for processing SYN-ACK and other
	                // receiver side processing.
	                // For already connected sockets iNewTcpWindow will be taking care
	                // of shrinking the window size for that TCP session.
	                iSockInBufSize = intValue;
	                }
	            } 
		    }
		    break;
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
			case KSoTcpRecvWinSize:
			    {
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW  
                iWindowSetByUser = ETrue;
#endif // SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW                
                if (ret == KErrNone)
                    {
                    if (!InState(ETcpInitial))
                        ret = KErrLocked;
                    else if (intValue < STATIC_CAST(TInt, KTcpMinimumWindow))
                        iSockInBufSize = KTcpMinimumWindow;
                    else
                        {
                        iSockInBufSize = intValue;
                        }
                    }
			    }
			break;

		case KSoTcpSendWinSize:
			if (ret == KErrNone)
				{
				if (!InState(ETcpInitial))
					ret = KErrLocked;
				else if (intValue < STATIC_CAST(TInt, KTcpMinimumWindow))
					iSockOutBufSize = KTcpMinimumWindow;
				else
					iSockOutBufSize = intValue;
				}
			break;

		case KSoTcpLingerinMicroSec:
            RDebug::Printf("TSoTcpLingerinMicroSec is set");
            //Enable micro sec calculation for TCP linger timer. User (currently just exposed to browser)
            //will specify linger time in microsecs. 
            iMicroSecCalcFlag=ETrue;           
		case KSoTcpLinger:
			if (aOption.Length() < (TInt)sizeof(TSoTcpLingerOpt))
				{
				return KErrArgument;
				}
			if (iSockFlags.iSendClose)
				{
				return KErrInUse;
				}

				{
				TSoTcpLingerOpt *opt = (TSoTcpLingerOpt *)aOption.Ptr();
				if (opt->iOnOff != 0 && opt->iLinger >= 0)
					{
					if (opt->iLinger > KTcpMaxLingerTime)
						{
						return KErrArgument;
						}
					iLinger = opt->iLinger;  // linger enabled.
					}
				else
					{
					iLinger = -1;  // linger disabled.
					}
				}
			break;

		default:
			ret = KErrNotSupported;
			break;
			}
		}

	if (aLevel == KSolInetIp)
		{
		TInt intValue;
		ret = GetOptionInt(aOption, intValue);

		switch(aName)
        	{
		case KSoIpTOS:
			// Silently filter out the ECN bits from TOS setting.
            // We're assuming that Protocol->Ecn() (based on ini params) holds the correct
            // values for ECN bits.
            	{
	            intValue = (intValue & 0xfc) | Protocol()->Ecn();
            	TPckgBuf<TInt> tosopt(intValue);

	            // Rest of the option processing is done at the lower levels.
    	        return CProviderInet6Transport::SetOption(aLevel, aName, tosopt);
    	        }

		case KSoHeaderIncluded:
		case KSoRawMode:
			// The base class implements HeaderIncluded and RawMode by default.
			// Force "Not Supported" for TCP here!
			return KErrNotSupported;
		default:
        	ret = KErrNotSupported;
            }
		}


	if (ret == KErrNotSupported)
		ret = CProviderInet6Transport::SetOption(aLevel, aName, aOption);
	
	return ret;
	}


TInt CProviderTCP6::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	{
	LOG(Log::Printf(_L("GetOpt\ttcp SAP[%u] %d, %d"), (TInt)this, aLevel, aName));
	CProviderTCP6 *This = CONST_CAST(CProviderTCP6*, this);
	TInt ret = KErrNotSupported, urgentChar;

	switch (aLevel)
		{
	case KSOLSocket:
		switch (aName)
			{
		case KSOReadBytesPending:
			ret = SetOptionInt(aOption, iPending);
			break;

		case KSOUrgentDataOffset:
			ret = SetOptionInt(aOption, iUpCount ? UrgentOffset() - iUpCount + 1 : 0);
			LOG(Log::Printf(_L("\ttcp SAP[%u] Urgent data offset = %d"),
				(TInt)this, iUpCount ? UrgentOffset() : 0));
			break;

		default:
			break;
			}
		break;

	case KSolInetTcp:
		switch (aName)
			{
		case KSoTcpAsync2MslWait:
			// XXX - Not implemented. Not very useful.
			break;

		case KSoTcpKeepAlive:
			ret = SetOptionInt(aOption, iFlags.iHaveKeepAlive);
			break;

		case KSoTcpMaxSegSize:
			ret = SetOptionInt(aOption, iMSS);
			break;

		case KSoTcpNextSendUrgentData:
			ret = SetOptionInt(aOption, iFlags.iNextIsUrgent);
			break;

		case KSoTcpNoDelay:
			ret = SetOptionInt(aOption, iFlags.iNoDelay);
			break;

		case KSoTcpCork:
		case KSoTcpNoPush:
			ret = SetOptionInt(aOption, iFlags.iCork);
			break;

		case KSoTcpOobInline:
			ret = SetOptionInt(aOption, iFlags.iOobInline);
			break;

		case KSoTcpRecvWinSize:
			ret = SetOptionInt(aOption, iSockInBufSize);
			break;

		case KSoTcpSendWinSize:
			ret = SetOptionInt(aOption, iSockOutBufSize);
			break;

		case KSoTcpListening:
			ret = SetOptionInt(aOption, InState(ETcpListen) ? 1 : 0);
			break;

		case KSoTcpNumSockets:
			ret = SetOptionInt(aOption, Protocol()->SapCount());
			break;

		case KSoTcpLinger:
			if (aOption.MaxLength() < (TInt)sizeof(TSoTcpLingerOpt))
				{
				return KErrTooBig;
				}

			TSoTcpLingerOpt opt;
			if (iLinger == -1)
				{
				opt.iOnOff = 0;
				opt.iLinger = 0;
				}
			else
				{
				opt.iOnOff = 1;
				opt.iLinger = iLinger;
				}
			aOption.SetLength(sizeof(opt));
			aOption.Copy((TUint8*)&opt, sizeof(opt));
			ret = KErrNone;
			break;

		case KSoTcpPeekUrgentData:
			if (ret = This->GetUrgent(urgentChar, KSockReadPeek), ret == KErrNone)
				ret = SetOptionInt(aOption, urgentChar);
			break;

		case KSoTcpRcvAtMark:
			ret = SetOptionInt(aOption, (UrgentOffset() == 0) ? 1 : 0);
			break;

		case KSoTcpReadBytesPending:
			ret = SetOptionInt(aOption, iPending);
			break;

		case KSoTcpReadUrgentData:
			if (ret = This->GetUrgent(urgentChar, 0), ret == KErrNone)
				ret = SetOptionInt(aOption, urgentChar);
			break;

		case KSoTcpSendBytesPending:
			ret = SetOptionInt(aOption, iSockOutQLen);
			break;

		default:
			break;
			}
		break;

	case KSolInetIp:
		switch(aName)
			{
        case KSoIpTOS:
			// Clear the ECN bits from the returned value before delivering it to socket.
            ret = CProviderInet6Transport::GetOption(aLevel, aName, aOption);
            if (ret != KErrNone)
            	{
                TInt intValue;
                if (GetOptionInt(aOption, intValue) == KErrNone)
                	ret = SetOptionInt(aOption, intValue & 0xfc);
                }
            break;
		case KSoHeaderIncluded:
		case KSoRawMode:
			// The base class implements HeaderIncluded and RawMode by default.
			// Force "Not Supported" for TCP here!
			return KErrNotSupported;
        default:
        	break;
			}
		break;

	default:
		break;
		}

	if (ret == KErrNotSupported)
		ret = CProviderInet6Transport::GetOption(aLevel, aName, aOption);

	return ret;
	}


TInt CProviderTCP6::SetRemName(TSockAddr &aAddr)
	{
	TInt err;
	TInetAddr addr = aAddr;

	// Check port range
	if (addr.Port() < 1 || addr.Port() > 65535)
		return KErrGeneral;

	// Check address
	if (addr.IsUnspecified() || !addr.IsUnicast())
		return KErrBadName;	

	if (addr.Family() == KAfInet)
		addr.ConvertToV4Mapped();

	TInt family = addr.IsV4Mapped() ? KAfInet : KAfInet6;

	if(iSockFamily == KAFUnspec)
		iSockFamily = family;
	else if (iSockFamily != family)
		return KErrBadName;		
		
	//
	// If we're reusing a local address we must check for an existing
	// connection before we can accept the remote address.
	//
	if (iSockFlags.iReuse && iFlow.FlowContext()->LocalPort() != KInetPortNone)
		{
		if (Protocol()->LocateSap(EMatchConnection, KAFUnspec, iFlow.FlowContext()->LocalAddr(), addr))
			return KErrInUse;
		}

	// Set remote address and port.. use original to get iAppFamily valid.
	if (err = CProviderInet6Transport::SetRemName(aAddr), err != KErrNone)
		return err;

	return KErrNone;
	}


void CProviderTCP6::ActiveOpen()
	{
	LOG(Log::Printf(_L("ActiveOpen\ttcp SAP[%u]"), (TInt)this));
	ASSERT(InState(ETcpInitial));

	TInt status = iFlow.Connect();
	if (status < 0)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Flow status = %d"), (TInt)this, status));
		iSockFlags.iConnected = EFalse;
		Error(status, MSocketNotify::EErrorConnect);
		return;
		}

	// Store KeepInterfaceUp to be restored when entering Established state.
	StoreKeepInterfaceUp();

	EnterState(ETcpConnect);
	if (status == EFlow_READY)
		SendSYN();
	}


TInt CProviderTCP6::PassiveOpen(TUint aQueSize)
	{
	ASSERT(InState(ETcpInitial));
	ASSERT(!iListenQueue);
	LOG(Log::Printf(_L("PassiveOpen\ttcp SAP[%u] QueSize=%d"), (TInt)this, aQueSize));

	//
	// On EPOC you can only call listen once, so it is safe to do this here.
	//
	iListenQueue = new CProviderTCP6*[aQueSize];
	if (!iListenQueue)
		return KErrNoMemory;

	iListenQueueSize = aQueSize;
	for (TUint i=0; i < iListenQueueSize; i++)
		iListenQueue[i] = 0;
	EnterState(ETcpListen);
	return KErrNone;
	}

//
// This routine tries to detach the socket from the socket server.
// Detach() is called from Shutdown(), when the user has called
// Close() or Shutdown(ENormal).
//
// If linger timeout is active, Shutdown() will complete with
// error KErrWouldBlock.
//
void CProviderTCP6::Detach()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] Detach()"), (TInt)this));

	StoreDestinationCache();

	CompleteIoctl(KErrCancel);
	if (iLinger > 0)
		{
		iLingerTimer->Cancel();
		Error(KErrWouldBlock, MSocketNotify::EErrorClose);
		}
	iLinger = -1;
	NoSecurityChecker();				// The checker will be unusable.
	iSocket->CanClose(MSocketNotify::EDetach);
	iSockFlags.iNotify = EFalse;            // No more upcalls
	iSockFlags.iAttached = (iSocket != 0);  // Did we get detached?
	LOG(if (iSockFlags.iAttached) Log::Printf(_L("\ttcp SAP[%u] DETACH FAILED!"), (TInt)this));
	ASSERT(!iSockFlags.iAttached);
	}

void CProviderTCP6::Expire()
	{
	if (!iSockFlags.iAttached)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Expire(): SELF DESTRUCT!"), (TInt)this));
		delete this;
		return;
		}
	if (iSockFlags.iNotify)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Expire(): DISCONNECT!"), (TInt)this));
		Close();
		iSockFlags.iNotify = EFalse;
		iSocket->Disconnect();
		}
	}

void CProviderTCP6::Shutdown(TCloseType aOption)
	{
	LOG(Log::Printf(_L("Shutdown\ttcp SAP[%u] TCloseType=%d"), (TInt)this, aOption));

	switch(aOption)
		{
	case ENormal:
		if (InState(ETcpListen|ETcpInitial|ETcpConnect|ETcpSynSent|ETcpClosed))
			{
			// Just do a brutal shutdown in these states.
			Close();
			break;
			}
		
		// No need to negotiate parameters as we only want to notify other end point.
		if (InState(ETcpSynReceived))
			ClearSYNSettings();

		// Send RST if receive queue is not empty.
		if (iLinger == 0 || SockInQLen() || !iFragQ.IsEmpty())
			{
			SendReset(iSND.NXT);
			if (FatalState())
				break;

			// If linger is enabled and timer==0, close the socket immediately
			if (iLinger == 0)
				{
				Close();
				break;
				}
			}

		iSockFlags.iSendClose = ETrue;
		iSockFlags.iRecvClose = ETrue;
		iSockInQ.Free();
		iSockInQLen = 0;
		iFragQ.Free();
		iNewData = 0;

		if (iLinger == -1 || iSockOutQLen == 0)
			{
			Detach();
			}
		else
			{
			//
			// Start linger timer. RSocket::Close() returns when timer
			// expires or when all data has been succesfully transmitted.
			//
		    if(iMicroSecCalcFlag)
                {
                //expecting iLinger timer to be specified in microsec.This will be set currently by browser where in
                //it is expected to be close with in certian time
                iLingerTimer->Start(iLinger * 1);
                }
            else
                {
                iLingerTimer->Start(iLinger * KOneSecondInUs);
                }			
			}
		SchedTransmit();

		break;

	case EStopInput:
	case EStopOutput:
		if (InState(ETcpListen|ETcpInitial))
			{
			Error(KErrNotSupported, MSocketNotify::EErrorClose);
			return;
			}

		if (aOption == EStopInput)
			{
			iSockFlags.iRecvClose = ETrue;

			// Send RST if receive queue is not empty.
			if (SockInQLen() || !iFragQ.IsEmpty())
				{
				SendReset(iSND.NXT);
				if (FatalState())
					break;
				}
			iSockInQ.Free();
			iFragQ.Free();
			iNewData = 0;
			}
		else
			{
			// HalfDuplex Close and simultaneous SYN_RCVD -> FIN_WAIT_1 is not supported.
			iSockFlags.iSendClose = ETrue;
			SchedTransmit();
			}

		Error(KErrNone, MSocketNotify::EErrorClose);
		Nif::SetSocketState(ENifSocketConnected, this);
		break;

	case EImmediate:
		if (InState(ETcpSynReceived|ETcpEstablished|ETcpFinWait1|ETcpFinWait2|ETcpCloseWait))
			SendReset(iSND.NXT);
		CompleteIoctl(KErrCancel);
		Close();
		return;

	default:
		Panic(EInet6Panic_NotSupported);
		break;
		}

	//
	// If we encountered a fatal error during the above processing,
	// stop everything and enter CLOSED state.
	//
	if (FatalState())
		Close();

	if (InState(ETcpFinWait2) && iSockFlags.iRecvClose)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Setting FIN-WAIT-2 timeout"), (TInt)this));
		SchedMsl2Wait();
		}

	//
	// If we're in CLOSED state and we're not detached,
	// tell the socket server that we're ready to die.
	//
	if (iSocket && InState(ETcpClosed))
		iSocket->CanClose();

	//
	// Note!  The socket server can immediately delete the socket
	//        within CanClose(). Don't add anything here!
	//
	}


//
// PRTv1.0 API
//
TUint CProviderTCP6::Write(const TDesC8 & aDesc, TUint aOptions, TSockAddr* /*aAddr*/ /*=NULL*/)
	{
	TDualBufPtr buf(aDesc);
	return Send(buf, aDesc.Length(), aOptions);
	}

void CProviderTCP6::GetData(TDes8 & aDesc, TUint aOptions, TSockAddr* /*aAddr*/)
	{
	TDualBufPtr aBuf(aDesc);
	Recv(aBuf, aDesc.Length(), aOptions);
	}

//
// PRTv1.5 API
//
TInt CProviderTCP6::Write(RMBufChain& aData, TUint aOptions, TSockAddr* /* anAddr*/)
	{
	TDualBufPtr buf(aData);
	return Send(buf, aData.Length(), aOptions);
	}

TInt CProviderTCP6::GetData(RMBufChain& aData, TUint aLength, TUint aOptions, TSockAddr* /*aAddr*/)
	{
	TDualBufPtr aBuf(aData);
	return Recv(aBuf, aLength, aOptions);
	}

TInt CProviderTCP6::Send(TDualBufPtr& aBuf, TInt aLength, TUint aOptions)
	{
	LOG(Log::Printf(_L("Write\ttcp SAP[%u] len=%d, options=%d"), (TInt)this, aLength, aOptions));
	ASSERT(aLength > 0);

	//
	// Limit queue size to maximum socket buffer size rounded down to nearest multiple segment.
	// Note: during fast recover we allow the socket buffer to expand in order to have enough
	// data to keep the transmission going.
	//
	TInt effMSS = EffectiveMSS();
	TInt bufSize = iSockOutBufSize +
		(iSacked.Count() ? (iSacked.Last()->iRight - iSND.UNA) : (iDupAcks * iSMSS));
	TInt reserve = (bufSize - (iSND.NXT - iSND.UNA)) % effMSS;
	TInt space = bufSize - iSockOutQLen - reserve;

	LOG(Log::Printf(_L("\ttcp SAP[%u] len=%d space=%d"), (TInt)this, aLength, space));

	if ((aOptions & KSockWriteUrgent) || iFlags.iNextIsUrgent)
		{
		//
		// According to RFC1122 the urgent pointer should point to the
		// last byte of urgent data. However, practically all TCP stacks
		// today choose to conform to BSD unix and follow the original
		// behaviour stated in RFC793. Therefore, we set the urgent pointer
		// to point to the first non-urgent byte following the urgent data.
		//
		iFlags.iNextIsUrgent = EFalse;
		iSND.UP = iSND.UNA + iSockOutQLen + aLength;
		LOG(Log::Printf(_L("\ttcp SAP[%u] Urgent pointer set, seq = %u"), (TInt)this, iSND.UP.Uint32()));
		}

	if (space <= 0)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] FLOW STOPPED"), (TInt)this));
		iSockFlags.iFlowStopped = ETrue;
		return 0;
		}

	// Append data to socket queue. 
	aLength = aBuf.Append(iSockOutQ, (aLength < space) ? aLength : space);
	iSockOutQLen += aLength;

	// We make a simple Nagle check here to avoid starting the transmitter unnecessarily.
	// To spell it out:
	// - If Cork option is set, always wait for full-sized segment before sending.
	// - Without Cork option one under-sized segment is allowed to be outstanding.
	// - TODO: With NoPush option segments are only sent when send buffer is full or
	//   when the connection is closed.
	if (!iSND.WND)
		{
		// Start probing
		SchedRetransmit();
		}
	else if ((iPartialSeq <= iSND.UNA && !iFlags.iCork) ||
		iSND.UNA + iSockOutQLen + aLength >= iSND.NXT + effMSS ||
		iFlags.iNoDelay)
		{
		// Start transmitter
		SchedTransmit();
		}

	return aLength;
	}


TInt CProviderTCP6::Recv(TDualBufPtr& aBuf, TInt aLength, TUint aOptions)
	{
	LOG(Log::Printf(_L("GetData\ttcp SAP[%u] len=%d, options=%d"), (TInt)this, aLength, aOptions));

	// Update iPending
	if (!(aOptions & KSocketInternalReadBit))
		iPending -= aLength;

	// This might happen when aborting a connection.
	if (iSockInQ.IsEmpty())
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] No data available!"), (TInt)this));
		return 0;
		}

	//
	// Discard in-band urgent data junk
	//
	while (iUpCount && UrgentOffset(0) == 0 && (!iFlags.iUrgentMode || UrgentOffset() > 0))
		{
		iSockInQ.TrimStart(1);
		--iSockInQLen;
		++iFreeWindow;
		ForgetUrgentPointer();
		}

	ASSERT((aOptions & KSockReadPeek) || !iCopyOutOffset);
	ASSERT(iCopyOutOffset + aLength <= (TInt)iSockInQLen);

	//
	// Peek. Copy from queue, fix counters and force receiver restart.
	//
	if (aOptions & KSockReadPeek)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Peek"), (TInt)this));

		TInt err = aBuf.CopyIn(iSockInQ, iCopyOutOffset, aLength);
		if (err != KErrNone)
			return KErrNoMBufs;

		if (!(aOptions & KSocketInternalReadBit))
			{
			iFlags.iCompleteRecv = ETrue;
			iNewData += aLength;
			iCopyOutOffset += aLength;
			iRecvQ.Wake();
			}

		return aLength;
		}

	//
	// Normal read. Consume data from the queue.
	//
	// Note: aBuf.Consume() may return less than we asked
	// if it is unable to allocate an MBuf for splitting the
	// inbound queue. This can only happen with PRTv1.5.
	// If we get nothing at all we return KErrNoMBufs here.
	//
	aLength = aBuf.Consume(iSockInQ, aLength, iBufAllocator);
	if (aLength == 0)
		return KErrNoMBufs;

	//
	// If we are now reading the actual urgent data, force the
	// application level read operation to complete and exit
	// urgent mode.
	//
	if (iFlags.iUrgentMode && UrgentOffset() == 0)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Reading urgent data. Clear urgent mode"), (TInt)this));
		iFlags.iUrgentMode = EFalse;
		iFlags.iCompleteRecv = !(aOptions & KSocketInternalReadBit);
		ForgetUrgentPointer();
		}
		
	iSockInQLen -= aLength;
	
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//Application is reading the data from the receive Q buffer
	//We need to make sure, that we will show that window is not shrinked
	//to the sender, so tha right edge of the sender window remains constant.
	//if This is true, then it is a case of TCP window shrink and we need 
	//to handle it.
	if ( iNewTcpWindow )
	    {
	    // Check if we can complete shrinking process
	    if ( aLength > iShrinkedWindowSize )
	        {
	        // We can exit from the shrinking process. Reset variables and
	        // update free window.
	        iFreeWindow = aLength - iShrinkedWindowSize;
	        iShrinkedWindowSize = 0;
	        iNewTcpWindow = 0;
	        }
	    else
	        {
	        // Substract the needed shrinking amount by the amount of bytes client
	        // read from the buffer
	        iShrinkedWindowSize -= aLength;
	        }
	    }
	else
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	
		{
		iFreeWindow += aLength;	
		}

	//
	// If we have read everything upto the urgent data,
	// force the application level read to complete.
	// If the urgent data has not yet been read, signal
	// the application again (read completion will clear
	// the signal).
	//
	if (UrgentOffset() == 0)
		{
		iFlags.iCompleteRecv = ETrue;
		if (iFlags.iUrgentMode)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] Now at urgent data"), (TInt)this));
			iFlags.iNotifyUrgent = ETrue;
			}
		}

	//
	// Increase receive window in multiples of RMSS
	//
	// The following assumes that the path MTU is symmetric and that
	// the peer uses the same options as we do. We could use some
	// kind of peer effective MSS detection heuristic in order to
	// make this smarter.
	//
	TUint effMSS = EffectiveMSS();
	if (iFreeWindow >= effMSS)
		{
		//
		// Round down to nearest multiple of RMSS. However, be careful not to
		// shrink a previously advertised window.
		//
		iFreeWindow += iRCV.WND;
		iRCV.WND = Max(iRCV.WND, effMSS * (iFreeWindow / effMSS));
		iFreeWindow -= iRCV.WND;                   // Leave reminder for later.

		//
		// If our last advertised window is so small that the sender might
		// block before we would normally send our next ack, we will send
		// a window update. This will prevent the sender from unnecessarily
		// going into probe mode. We will only do this if we can advertise
		// at least half a window.
		//
		if (InState(ETcpEstablished|ETcpFinWait1|ETcpFinWait2))
			{
			if ((TInt)iAdvertisedWindow <= iSockInBufSize/4 && iRCV.WND >= iSockInBufSize/2)
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] Sending window update %d -> %d"), (TInt)this, iAdvertisedWindow, iRCV.WND));
				SendSegment(KTcpCtlACK);
				}
			}
		}

	//
	// If we have data that has not yet been announced to the socket server,
	// make sure we get around to it.
	//
	if (iNewData)
		iRecvQ.Wake();

	LOG(Log::Printf(_L("GetData\ttcp SAP[%u] returns %d bytes, %d bytes pending"), (TInt)this, aLength, iPending));
	return aLength;
	}


//
// Process ICMP Error. Treat it as a soft error unless we're trying to connect.
//
void CProviderTCP6::IcmpError(TInt aError, TUint aOperationMask, TInt aType, TInt aCode,
	const TInetAddr& aSrcAddr, const TInetAddr& aDstAddr, const TInetAddr& aErrAddr)
	{
	CProviderInet6Transport::IcmpError(aError,InState(ETcpSynSent|ETcpSynReceived) ?
		MSocketNotify::EErrorAllOperations : aOperationMask,
		aType, aCode, aSrcAddr, aDstAddr, aErrAddr);
	}

//
// This routine maintains a sorted rotating array of detected urgent pointers.
// The lowest detected UP is always stored in iUpArray[iUpIndex] and the highest
// one is stored in iUpArray[(iUpIndex + iUpCount - 1) % KTcpUpMax].
// If the array overflows, the lowest UP is always discarded.
//
void CProviderTCP6::RememberUrgentPointer(TTcpSeqNum aUp)
	{
	// Insert new UP into the sorted list.
	TInt i;
	TTcpSeqNum up;
	
	for (i = iUpIndex + iUpCount; i > iUpIndex; --i)
		{
		up = iUpArray[(i - 1) % KTcpUpMax];
		if (aUp > up)
			break;
		if (aUp == up)
			return;
		}

	if (i == iUpIndex && iUpCount == KTcpUpMax)
		return;
	TUint index;
	while (i < iUpIndex + iUpCount)
		{
		index = i % KTcpUpMax;
		up = iUpArray[index];
		iUpArray[index] = aUp;
		aUp = up;
		++i;
		}

	iUpArray[i % KTcpUpMax] = aUp;
	if (iUpCount < KTcpUpMax)
	    {
	    ++iUpCount;
	    }
	else
	    {
	    iUpIndex += 1;
	    if(iUpIndex >= KTcpUpMax)
	        iUpIndex -= KTcpUpMax;
	    }

	iFlags.iUrgentMode = ETrue;
	iFlags.iNotifyUrgent = !iFlags.iOobInline;
	}

void CProviderTCP6::ForgetUrgentPointer()
	{
	if (iUpCount)
		{
		if (++iUpIndex >= KTcpUpMax)
			iUpIndex = 0;
		--iUpCount;
		}
	}


TInt CProviderTCP6::GetUrgent(TInt& aUrgentChar, TUint aOptions)
	{
	TTcpSeqNum up;
	RMBuf *m;
	TBool found = EFalse;
	TInt urgentOffset = UrgentOffset();

	if (!iFlags.iUrgentMode || iFlags.iOobInline)
		return KErrNotFound;

	up = UrgentHigh();
	if (up <= iRCV.NXT)
		{
		// It's in the socket queue.
		if (urgentOffset == 0 && !(aOptions & KSockReadPeek))
			{
			//
			// It's first in queue. Fetch it with Recv(), so that receive
			// window is correctly updated.
			//
			TBuf8<1> buf(1);
			TDualBufPtr p(buf);
			iNewData--;
			Recv(p, 1, aOptions | KSocketInternalReadBit);
			aUrgentChar = buf[0];
			LOG(Log::Printf(_L("\ttcp SAP[%u] GetUrgent(): Urgent char first in socket queue = %02x"), (TInt)this, aUrgentChar));
			found = ETrue;
			}
		else
			{
			for (m = iSockInQ.First(); m != NULL; m = m->Next())
				{
				if (urgentOffset < m->Length())
					{
					aUrgentChar = *(m->Ptr() + urgentOffset);
					LOG(Log::Printf(_L("\ttcp SAP[%u] GetUrgent(): Urgent char in socket queue = %02x"), (TInt)this, aUrgentChar));
					found = ETrue;
					break;
					}
				urgentOffset -= m->Length();
				}
			}
		}
	else if (up <= iSND.WL1 + iRMSS)
		{
		// Maybe it's in the fragment queue.
		TMBufPktQIter iter(iFragQ);
		for (iter.SetToFirst(); iter.More(); iter++)
			{
			RMBufTcpFrag& frag = (RMBufTcpFrag&)iter.Current();
			TTcpSeqNum seq = frag.Offset();
			TUint32 len = frag.FragmentLength();

			if (up <= seq)
				break;

			if (up <= seq + len)
				{
				TTcpPacket seg(frag);
				RMBuf *m, *prev;
				TInt off, len;
				frag.Goto(seg.iHdr->HeaderLength() + (up - seq) - 1,
					m, off, len, prev);
				aUrgentChar = *(m->Ptr() + off);
				LOG(Log::Printf(_L("\ttcp SAP[%u] GetUrgent(): Urgent char in fragment queue = %02x"), (TInt)this, aUrgentChar));
				found = ETrue;
				break;
				}
			}
		}

	if (!found)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] GetUrgent(): Would block"), (TInt)this));
		return KErrWouldBlock;
		}

	if (!(aOptions & KSockReadPeek))
		{
		iFlags.iUrgentMode = EFalse;

		// Clear urgent mode exception flag
		if (iSockFlags.iNotify)
			iSocket->Error(KErrNone, 0);
		}

	return KErrNone;
	}


void CProviderTCP6::Process(RMBufChain& aPacket, CProtocolBase* /*aSourceProtocol*/)
	{
	RMBufRecvInfo *const info = RMBufRecvPacket::PeekInfoInChain(aPacket);
#ifdef SYMBIAN_NETWORKING_UPS
	if (info == NULL || (!HasNetworkServices() && (ConnectionInfoSet() == EFalse) && (info->iFlags & KIpLoopbackPacket) == 0))
#else
	if (info == NULL || (!HasNetworkServices() && (info->iFlags & KIpLoopbackPacket) == 0))
#endif
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Not allowed to receive external packets"), (TInt)this));
		aPacket.Free();
		return;
		}
	iRecvQ.Append(aPacket);
	iRecvQ.Wake();
	}


void CProviderTCP6::ErrorExpire(TInt aError)
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] ErrorExpire(): Closing SAP on fatal error: %d"), (TInt)this, aError));
	if (!iSockFlags.iAttached)
		{
		Expire();
		return;
		}
	Close();
	CProviderInet6Transport::Error(aError);
	}

void CProviderTCP6::CanSend()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] CanSend()"), (TInt)this));

	//
	// If the flow has become unblocked, process all missed events
	// and restart transmitter.
	//
	TInt flowStatus = iFlow.Status();
	if (flowStatus < 0)
		{
		ErrorExpire(flowStatus);
		return;
		}

	if (flowStatus == EFlow_READY)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] CanSend(): Flow UNBLOCKED"), (TInt)this));

		if (InState(ETcpListen|ETcpConnect))
			{
			//
			// Get the SAP started.
			//
			if (InState(ETcpConnect))
				SendSYN();
			else if (InState(ETcpListen) && !iRecvQ.IsEmpty())
				iRecvQ.Wake();
			}

		if (iFlags.iRetransmitPending)
			{
			iFlags.iRetransmitPending = EFalse;
			if(RetransmitSegments())
				return;
			}

		if (iFlags.iTransmitPending)
			{
			iFlags.iTransmitPending = EFalse;
			SendSegments();
			}

		if (!iSendQ.IsEmpty())
			iSendQ.Wake();
			
		if (CanTriggerKeepAlive())
			{
			// The heaviest time check only if we are otherwise allowed to send the keepalive.
			TUint32 time_now = TimeStamp();
			if (time_now - iLastTriggeredKeepAlive > KTcpKeepAliveTH * KOneSecondInMs)
				{
				iLastTriggeredKeepAlive = time_now;
				LOG(Log::Printf(_L("\ttcp SAP[%u] CanSend(): Sending a Keep-Alive probe"), (TInt)this));
				SendSegment(KTcpCtlACK, iSND.UNA - 1, 0);
				}
			}
		}
	}


//
// This is the actual event driven asynchronous transmitter loop.
//
void CProviderTCP6::Transmit()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] Transmit"), (TInt)this));

	RMBufSendPacket packet;
	TInt flowStatus;
	LOG(TInt count = 0);

	ASSERT(!iSendQ.IsEmpty());
	RMBufSendInfo *info  = NULL;
	// Transmit packets when flow is ready
	while ((flowStatus = iFlow.Status()) == EFlow_READY && iSendQ.Remove(packet))
		{
		info = packet.PeekInfo();
		info->iFlow.Open(iFlow);
		Protocol()->Send(packet);
		LOG(count++);
		}

	// Local congestion control
	if (flowStatus == EFlow_HOLD && !iSendQ.IsEmpty())
		SourceQuench();

	LOG(Log::Printf(_L("\ttcp SAP[%u] %d segments transmitted"), (TInt)this, count));
	LOG(if (flowStatus > EFlow_READY) Log::Printf(_L("\ttcp SAP[%u] Flow BLOCKED"), (TInt)this));
	//LOG(if (flowStatus != EFlow_READY) Log::Printf(_L("CProviderTCP6::Transmit(): Flow status = %d.\r\n"), flowStatus));

	if (flowStatus < 0)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Error %d. Segments"), (TInt)this, flowStatus));
		ErrorExpire(flowStatus);
		}
	}


// NOTE: There are a few tricks in the ECN implementation that should be considered. ECT bit MUST NOT
// be set in IP packets in the following cases [RFC 3168, Sec. 6.1.4 -- 6.1.6]:
// 1) pure ACKs
// 2) retransmitted segments
// 3) zero window probes


//
// This routine generates and transmits a TCP segment.
//
TInt CProviderTCP6::SendSegment(TUint8 aFlags, TTcpSeqNum aSeq, TUint32 aDataLen)
	{
	//LOG(Log::Printf(_L("CProviderTCP6::SendSegment(%06b,%d,%d)\r\n"),aFlags,aSeq,aMaxLen));

	ASSERT(!(aFlags & KTcpCtlRST));

	RMBufSendPacket seg;
	RMBufSendInfo *info = NULL;
	TInt err;
	TInt up = iSND.UP - aSeq;
	TInt seqLen = aDataLen;
	if (aFlags & (KTcpCtlSYN|KTcpCtlFIN))
	    ++seqLen;

	if (iFlags.iEcnHaveCongestion)
		{
		aFlags |= KTcpCtlECE;
		}

	if (iFlags.iEcnSendCWR)
		{
		aFlags |= KTcpCtlCWR;
		iFlags.iEcnSendCWR = EFalse;
		}

	//
	// Don't output SACK blocks with data segments
	//
	iOptions.SuppressSack(aDataLen > 0);
	TUint headerLen = KTcpMinHeaderLength + iOptions.Length();

	//
	// Allocate memory for TCP segment and IP headers.
	//
	// Important: we assume that, when using the TimeStamps option,
	// iOptions always contains timestamps, so that the header length
	// does not change when we update them below.
	//
	for (;;)
		{
		TInt hdrReserve = iFlow.FlowContext()->HeaderSize() + headerLen;
		if (aDataLen)
			{
			err = iSockOutQ.Copy(seg, aSeq - iSND.UNA, aDataLen, hdrReserve);
			if(err == KErrNone)
				{
				err = seg.Prepend(hdrReserve, iBufAllocator);
				}
			}
		else
			{
			err = seg.Alloc(hdrReserve, iBufAllocator);
			}
		if(err == KErrNone)
			{
			info = seg.NewInfo();
			if(info)
				{
				break;
				}
			else
				{
				err = KErrNoMBufs;
				}
			}

		// Allocation failed. Try to recover.
		seg.Free();

		// Try to free up some memory. XXX - We could do better here.
		if (!iFragQ.IsEmpty())
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendSegment(): RENEGE!!!"), (TInt)this));
			iFragQ.Free();
			continue;
			}

		// If we can't scrounge up a buffer, the packet will be dropped.
		LOG(Log::Printf(_L("\ttcp SAP[%u] SendSegment(): No memory, packet DROPPED!"), (TInt)this));
		break;
		}

	if (err == KErrNone)
		{
		// Reserve space for IP headers
		seg.TrimStart(iFlow.FlowContext()->HeaderSize());

		// Check for urgent data
		if (up > 0 && up < 0x10000)
			aFlags |= KTcpCtlURG;
		else
			up = 0;

		//
		// Fill in TCP header. Note that the header length has already
		// been set and the checksum will be calculated later.
		//
		// Note: If we're probing or the receiver has shrunk its advertised
		// window our iSND.NXT may be pointing beyond its advertised window.
		// If the packet we're trying to send does not occupy any sequence
		// space, we will adjust the sequence number so that it falls withín
		// the receiver's window. This will make sure that our ACK and RST
		// packets will be accepted. In the worst case, a buggy receiver
		// might still ignore our window updates, which might lead to a probe
		// deadlock. Well, a TCP that shrinks its window without adjusting
		// SND.WL1 deserves what it gets...
		//
		TTcpPacket pkt(seg);
		TTcpSeqNum windowEdge = iSND.UNA + iSND.WND;
		pkt.iHdr->SetSrcPort(iFlow.FlowContext()->LocalPort());
		pkt.iHdr->SetDstPort(iFlow.FlowContext()->RemotePort());
		pkt.iHdr->SetSequence(seqLen > 0 || aSeq <= windowEdge ? aSeq : windowEdge);
		pkt.iHdr->SetAcknowledgment((aFlags & KTcpCtlACK) ? iRCV.NXT.Uint32() : 0);
		if (aFlags & KTcpCtlSYN)
			{
			// Window scale must not be used in SYN or SYN-ACK segments.
			// We do not truncate window size to be divisible by MSS.
			// The next window advertisments are properly aligned, though.
			pkt.iHdr->SetWindow(Min(iRCV.WND, 0xffff));
			}
		else
			{
			pkt.iHdr->SetWindow(iRCV.WND >> iRcvWscale);
			}
		pkt.iHdr->SetUrgent(up);
		pkt.iHdr->SetControl(aFlags);

		//
		// Fill in info struct
		//
		// coverity[dead_error_condition]
		// ASSERT statement required here as the condition "info != NULL" could be false
		ASSERT(info != NULL);
		info->iProtocol = KProtocolInetTcp;
		info->iSrcAddr  = iFlow.FlowContext()->LocalAddr();
		info->iDstAddr  = iFlow.FlowContext()->RemoteAddr();
		info->iLength   = headerLen + aDataLen;
		info->iFlags    = iFlags.iDoPMTUD ? KIpDontFragment : 0;

		// Check if we have to clear the ECN ECT flag because of the few exception cases listed above.
		if (iFlags.iEcn && (aSeq < iSND.NXT || aDataLen < 1))
			{
			info->iFlags |= KIpNoEcnEct;
			}

		//
		// Output TCP options (except for RST packets)
		//
		if (!(aFlags & KTcpCtlRST))
			{
			// Get current time
			TUint32 usec = TimeStamp();

			//
			// Measure RTT
			//
			if (iFlags.iUseTimeStamps)
				{
#if 0
				// XXX - This code can cause tsVal to go backwards.
				TUint32 lastSent, lastEchoed;
				iOptions.TimeStamps(lastSent, lastEchoed);
				if (usec == lastSent)
					usec++;
#endif
				iOptions.SetTimeStamps(usec, iTsRecent);
				}
			if (iFlags.iTiming)
				{
				// Check for retransmission. We shouldn't do RTT measurements on
				// retransmitted segments because of ambiguity (Karn algorithm).
				if (iTimingSeq > aSeq && iTimingSeq <= aSeq + aDataLen)
					iFlags.iTiming = EFalse;
				}
			else if (aSeq == iSND.NXT && seqLen > 0 && iSND.WND > 0)
				{
				// New segment and not a probe. Start timing.
				iFlags.iTiming = ETrue;
				iTimingSeq = aSeq + aDataLen;
				iTimeStamp = usec;
				}

			//
			// Output TCP options into the segment header.
			//
			pkt.iHdr->SetOptions(iOptions);
			}

		//
		// Compute checksum and send the segment.
		//
		pkt.ComputeChecksum(seg, info);
		LOG(CProtocolTCP6::LogPacket('>', seg, info));
		seg.Pack();

       if(!iFlags.iFastRetransMode)
           {
           iSendQ.Append(seg);
           }
       else
           {
           //If the fast retransmission mode is set then add the segment in the begining of the sendqueue.
           iSendQ.Prepend(seg);
           }
		iSendQ.Wake();
		}

	CancelDelayACK();

	// Schedule a retransmission. Restart RTO if there was no outstanding segments, otherwise
	// it is only restarted if it was not running already.
	if (seqLen)
		{
		if (iSND.NXT == iSND.UNA)
			ReSchedRetransmit();
		else
			SchedRetransmit();
		}

	aSeq += seqLen;
	if (aSeq > iSND.NXT)
		iSND.NXT = aSeq;
	iLastAck = iRCV.NXT;
	iAdvertisedWindow = iRCV.WND;

	return aDataLen;
	}

//
// Transmit one data segment at given position. Use current
// MTU, send window, and congestion window to limit segment
// size. Return number of bytes advanced in the transmit queue.
// This may be less than transmitted bytes if SACK is being used.
//
TInt CProviderTCP6::SendDataSegment(TTcpSeqNum aSeq, TBool aNagleOverride)
	{
	TInt effMSS = EffectiveMSS();
#ifdef SIGNED_UNSIGNED_FIX //if user app set the value which is bigger than 0x7ffffff,The largest possible value for a TInt16.
	TUint effWND = MinUU(iSND.WND, iSockOutQLen); 
#else
	TUint effWND = Min(iSND.WND, iSockOutQLen);
#endif
	TTcpSeqNum seq = aSeq;
	TUint8 flags = KTcpCtlACK;
	TInt len;

	// If RTO just occurred and F-RTO processing is underway, override Nagle.
	if (iFRTOsent)
		{
		aNagleOverride = ETrue;
		}

	//
	// Transmission guided by SACK (RFC2018)
	//
	if (iFlags.iSackOk)
		{
		TTcpSeqNum limitSeq, limit;
		TInt awnd, sacked;

		// Find a slot between SACKed blocks
		sacked = iSacked.FindGap(seq, limit);

		if (iFlags.iFastRetransMode)
			{
			// Apply the FACK algorithm
			TTcpSeqNum fack = iSacked.Count() ? iSacked.Last()->iRight : iSND.UNA;
			awnd = iCwnd - (iSND.NXT - fack);
			//
			// Allow at least one segment if there are no retransmits out.
			// This combines new-Reno style retransmit behaviour with the
			// FACK algorithm. Pure FACK is somewhat tardy sending out the
			// first retransmit, which causes problems in the usual single
			// segment loss case.
			//
			if (iRetranData)
				awnd -= iRetranData;
			else if (awnd < (TInt)iSMSS)
				awnd = iSMSS;
			limitSeq = iSND.UNA + effWND;
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): FAST RECOVERY: una=%u fack=%u awnd=%d"), (TInt)this, (iSND.UNA).Uint32(), fack.Uint32(), awnd));
			}
		else
			{
			//
			// Apply congestion window and limited transmit window
			//
			// Note: if there are SACKed blocks within the congestion window,
			// the window is extended accordingly. This is compatible with
			// the congestion control principles, since we will not be
			// retransmitting the SACKed blocks.
			//
#ifdef SIGNED_UNSIGNED_FIX
			awnd = (TInt)MinUU(effWND, iCwnd + iLwnd + (TUint)Max(sacked, 0));
#else
			awnd = Min(effWND, iCwnd + iLwnd + (sacked < 0 ? 0 : sacked));
#endif
			limitSeq = iSND.UNA + awnd;
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): NORMAL MODE: una=%u limit=%u wnd=%u"), (TInt)this, (iSND.UNA).Uint32(), limitSeq.Uint32(), awnd));
			}

		if (sacked >= 0)
			{
			if (limit < limitSeq)
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): SACK Adjust: RIGHT %u -> %u"), (TInt)this, limitSeq.Uint32(), limit.Uint32()));
				limitSeq = limit;
				if (limit <= seq + awnd)
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): NAGLE override (SACK)"), (TInt)this));
					aNagleOverride = ETrue;
					}
				}
			}
		else if (iFlags.iFastRetransMode)
			{
			//
			// We are beyond the last SACK block. Skip to new data.
			//
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): SACK Adjust: SEND NEW DATA"), (TInt)this));
			iSendHigh = seq;  // Store position of SACK transmit
			seq = iSND.NXT;   // Start transmitting new data
			}

		LOG(if (seq > aSeq) Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): SACK Adjust: LEFT %u -> %u"), (TInt)this, aSeq.Uint32(), seq.Uint32()));
#ifdef SIGNED_UNSIGNED_FIX
		if(limitSeq > seq)
			{
			len = (TInt)MinUS(limitSeq - seq, awnd);
			}
		else
			{
			len = Min(limitSeq - seq, awnd);
			}
#else
		len = Min(limitSeq - seq, awnd);
#endif
		}
	else
		{
#ifdef SIGNED_UNSIGNED_FIX
		len = iSND.UNA + MinUU(effWND, iCwnd + iLwnd) - seq;
#else
		len = iSND.UNA + Min(effWND, iCwnd + iLwnd) - seq;
#endif
		}
	LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(%u, %d): una=%u nxt=%u off=%d noff=%d len=%d"),
		(TInt)this, aSeq.Uint32(), aNagleOverride, (iSND.UNA).Uint32(), (iSND.NXT).Uint32(), aSeq - iSND.UNA, seq - iSND.UNA, len));
	LOG(Log::Printf(_L("\ttcp SAP[%u] Transmit state:  effMSS=%d iCwnd=%d iLwnd=%d effWND=%d iRetranData=%d recovery=%d"),
		(TInt)this, effMSS, iCwnd, iLwnd, effWND, iRetranData, iFlags.iFastRetransMode));

	//
	// Sender side SWS avoidance and Nagle.
	//
	if (len <= 0)
		len = 0;
	else if (len >= effMSS)
		len = effMSS;
	else if (seq > iSND.NXT && seq + len < iSND.UNA + iSockOutQLen && !aNagleOverride)
		//
	// Nagle override is not on and we're trying to send a partial
	// segment from the middle of the send queue. Therefore, we must
	// be constrained by the receiver window or the congestion window.
	//
	// If receiver window is very small we must allow a small packet out.
	//
	len = 0;
	else
		{
		//
		// Minshall's modification to the Nagle algorithm. We are
		// allowed to transmit a partial segment if there are no
		// other unacknowledged partial segments in flight.
		//
		if (iPartialSeq <= iSND.UNA && !Protocol()->StrictNagle() && !iFlags.iCork)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): NAGLE override (Minshall), seq=%u"), (TInt)this, seq.Uint32()));
			aNagleOverride = ETrue;
			iPartialSeq = seq + len;
			}

		//
		// Standard Nagle algorithm. Do not send partial packets
		// if there are outstanding unacknowledged segments.
		// We also refrain from sending small packets if we have
		// blocked the application as that means our send buffer
		// is full.
		//
		// Update: With Cork option enabled, only full-sized segments are sent
		//
		// Note that we temporarily disable Nagle if we have
		// some urgent data to send or if the user has already
		// closed the outgoing direction of the connection.
		//
		if((iSockFlags.iFlowStopped || iSND.UNA < seq || iFlags.iCork)
			&& iSND.UP <= seq && !iFlags.iNoDelay
			&& !iSockFlags.iSendClose && !aNagleOverride)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] SendDataSegment(): NAGLE kicked in"), (TInt)this));
			len = 0;
			}
		else
			flags |= KTcpCtlPSH;  // Set PSH on all partial segments
		}

	if (len)
		{
		if (seq + len == iSND.UNA + iSockOutQLen)
			{
			// Send queue has drained. Set PSH if application is not blocked.
			if (!iSockFlags.iFlowStopped)
				flags |= KTcpCtlPSH;

			// Set FIN if the socket is closing.
			if (iSockFlags.iSendClose)
				{
				flags |= KTcpCtlFIN;
				if (InState(ETcpEstablished|ETcpCloseWait))
					EnterState(InState(ETcpEstablished) ? ETcpFinWait1 : ETcpLastAck);
				}
			}

		if (len = SendSegment(flags, seq, len), len >= 0)
			{
			// Count retransmitted data for FACK
			if (iSacked.Count() && seq < iSacked.Last()->iRight)
				{
				iSendHigh = seq + len;
				if (iFlags.iFastRetransMode)
					iRetranData += len;
				}

			// Take SACK advance into account
			len += (seq - aSeq);
			}
		}
	else if (iSND.WND < (TUint)effMSS)
		SchedRetransmit(); // Start probing

	//
	// Return the number of bytes by which to advance transmit sequence.
	// Note: With SACK this is not necessarily the number of bytes sent.
	//
	return len;
	}


//
// Send multiple data segments.
//
void CProviderTCP6::SendSegments(TBool aNagleOverride)
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] SendSegments(): queue=%u wnd=%u cwnd=%u, ssthresh=%u"),
		(TInt)this, iSockOutQLen, iSND.WND, iCwnd, iSsthresh));

	iFlags.iTransmitPending = (iFlow.Status() == EFlow_PENDING);
	if (iFlags.iTransmitPending)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] SendSegments(): Flow pending"), (TInt)this));
		return;
		}

	if (InState(ETcpSynReceived|ETcpEstablished|ETcpCloseWait|ETcpLastAck|ETcpFinWait1|ETcpClosing))
		{
		TInt advance;
		TUint count = 0;

		if (iTransmitSeq < iSND.UNA)
			iTransmitSeq = iSND.UNA;

		// Transmit segments from queue
		while (advance = SendDataSegment(iTransmitSeq, aNagleOverride), advance > 0)
			{
			iTransmitSeq += advance;
			//
			// Limit transmission to KTcpMaxTransmit segments during
			// fast retransmit/recovery.
			//
			if (iFlags.iFastRetransMode && ++count >= Protocol()->MaxBurst())
				break;
			}

		//
		// User has closed the connection and send queue has drained?
		//
		// Note: Normally the FIN bit is slapped on the last data segment
		// in SendDataSegment(). However, if we have to send a FIN segement
		// with no data, we do it here.
		//
		if (iSockFlags.iSendClose && 
			((InState(ETcpEstablished|ETcpCloseWait) &&	iSND.NXT == iSND.UNA + iSockOutQLen && advance >= 0) ||
			(InState(ETcpSynReceived) && iSockFlags.iRecvClose)))
			{
			EnterState(InState(ETcpSynReceived|ETcpEstablished) ? ETcpFinWait1 : ETcpLastAck);
			SendSegment(KTcpCtlFIN|KTcpCtlACK);
			}
		}
	}


//
// Handle retransmission timeout
//
// The retransmission timer is shared for multiple purposes.
// There are different reasons why we might end up here:
//
//   1) A normal retransmission timeout. In this case, iSND.NXT > iSND.UNA.
//   2) We are probing a zero window. In this case iSND.WND == 0.
//   3) The Nagle override timeout has expired. In this case,
//      iSND.WND != 0 and unsent < effMSS.
//   4) TIME-WAIT or FIN-WAIT-2 timeout.
//   5) If TCP keepalive option is set, expiry of the keepalive timer
//
void CProviderTCP6::RetransmitTimeout()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitTimeout(): backoff=%d"), (TInt)this, iBackoff));

	TUint maxRetries = InState(ETcpSynSent|ETcpSynReceived)
		? Protocol()->SynRetries() : Protocol()->Retries2();

	// Handle backoff and expiration
	if (iSND.NXT > iSND.UNA || iSND.WND == 0)
		{
		//
		// Exponential backoff.
		//
		++iBackoff;
		if (iRTO < Protocol()->MaxRTO())  // Avoid RTO overflow
			ResetRTO();
		
		if(DetachIfDead())
			{
			Expire();
			return;
			}
		//
		// Timeout?
		//
		// Note: we time out if this is a connect attempt or a retransmission,
		// but we must not time out if we're probing a zero window.
		// Exception: probing timeouts if application has closed and we have zero window
		//
		if ((iBackoff > maxRetries) &&
			(iSND.WND > 0 
			|| InState(ETcpSynSent|ETcpSynReceived) 
			|| (iSockFlags.iSendClose && iSockFlags.iRecvClose)))
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitTimeout(): TCP timed out"), (TInt)this));
			ErrorExpire(iLastError.iStatus != KErrNone ? iLastError.iStatus : KErrTimedOut);
			return;
			}
		//
		// Simple black hole detection for PMTUD. Currently, we never try to re-enable
		// PMTUD after hitting a black hole.
		//
		if (iBackoff > Protocol()->Retries1() && iSND.WND > 0)
			{
			iFlags.iDoPMTUD = EFalse;
			iSMSS = Min(iSMSS, KTcpStandardMSS);
			}
		}

	else if (iFlags.iHaveKeepAlive && !iSockOutQLen && InState(ETcpEstablished | ETcpCloseWait))
		{
		KeepAliveTimeout();
		return;
		}

	// TIME-WAIT or FIN-WAIT-2 timeout
	if (InState(ETcpTimeWait) || (InState(ETcpFinWait2) && iSockFlags.iRecvClose))
		{
		Expire();
		return;
		}

	// Not dead yet. Go retransmit some segments.
	RetransmitSegments();
	}


void CProviderTCP6::KeepAliveTimeout()
	{
	// Keepalive timer expired. Because 32-bit microseconds are not enough for the minimum keepalive
	// timeout of two hours, the keepalive interval needs to be consumed one hour at a time.

	ASSERT(iRetransTimer);

	if (iBackoff >= Protocol()->NumKeepAlives())
		{
		// No response from the peer. Terminate connection
		LOG(Log::Printf(_L("\ttcp SAP[%u] KeepAliveTimeout(): No response to Keep-Alive probes. Closing connection"), (TInt)this));
		ErrorExpire(KErrTimedOut);
		return;
		}

	TUint32 usec = TimeStamp();

	if (!iLastTimeout)
		iLastTimeout = usec;

	TUint32 distance = (usec - iLastTimeout) / KOneSecondInMs;  // seconds
	TUint32 interval = iBackoff ? Protocol()->KeepAliveRxmt() : Protocol()->KeepAliveIntv();

	if (distance > interval)
		{
		// Send a keepalive probe. If no ack arrives, next one is sent after a shorter time (75 s)
		LOG(Log::Printf(_L("\ttcp SAP[%u] KeepAliveTimeout(): Sending a Keep-Alive probe"), (TInt)this));
		SendSegment(KTcpCtlACK, iSND.UNA - 1, 0);
		iBackoff++;
		iRetransTimer->Restart(Protocol()->KeepAliveRxmt() * KOneSecondInUs);
		}
	else
		{
		// This branch is entered when the first keepalive has to be issued after an idle period.
		distance = Protocol()->KeepAliveIntv() - distance;
		iRetransTimer->Restart((distance > 1800) ?
			1800 * KOneSecondInUs : (distance * KOneSecondInUs));
		}
	}


void CProviderTCP6::ResetKeepAlives()
	{
	ASSERT(iRetransTimer);
	iRetransTimer->Restart((Protocol()->KeepAliveIntv() > 1800) ?
		1800 * KOneSecondInUs : (Protocol()->KeepAliveIntv() * KOneSecondInUs));
	// Backoff is used for counting unacknowledged keepalive retransmissions during idle periods
	iBackoff = 0;
	iLastTimeout = TimeStamp();
	}

inline TBool CProviderTCP6::CanTriggerKeepAlive()
	{ 
	//
	// We can only send keep-alive if we're idle and established.
	//
	return iFlags.iHaveTriggeredKeepAlive
		&& !iSockOutQLen 
		&& iSendQ.IsEmpty()
		&& InState(ETcpEstablished) 
		&& iSND.NXT == iSND.UNA 
		&& iSND.WND > 0;
	}

//
// Try to retransmit segments. This routine gets called from two places:
//  - directly from RetransmitTimeout()
//  - from CanSend(), in which case this is a delayed retransmission timeout
//
TBool CProviderTCP6::RetransmitSegments()
	{
	ASSERT(iRetransTimer);

	LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): queue=%u wnd=%u cwnd=%u, ssthresh=%u dupacks=%d"),
		(TInt)this, iSockOutQLen, iSND.WND, iCwnd, iSsthresh, iDupAcks));

	TInt unacked = Min(iSND.NXT - iSND.UNA, iSockOutQLen);  // Adjust for FIN
	TInt effMSS = EffectiveMSS();

	// Delay retransmission if the flow is pending
	iFlags.iRetransmitPending = (iFlow.Status() == EFlow_PENDING);
	if (iFlags.iRetransmitPending)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): Flow pending"), (TInt)this));
		ReSchedRetransmit();
		return EFalse;
		}

	//
	// Handle retransmission of data segments and zero window probing.
	// We must have something in output queue. SYN or FIN retransmissions
	// and TIME-WAIT or FIN-WAIT-2 timeouts are handled later.
	//
	if (iSockOutQLen && InState(ETcpEstablished|ETcpFinWait1|ETcpClosing|ETcpCloseWait|ETcpLastAck))
		{
		if (iSND.WND == 0)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): Window PROBE"), (TInt)this));

			//
			// We are probing a zero window.
			//
			switch (Protocol()->ProbeStyle())
				{
			case 1:
				// Probe with a full segment (BSD style)
				SendSegment(KTcpCtlACK, iSND.UNA, Min(iSockOutQLen - unacked, effMSS));
				break;

			case 2:
				//
				// Probe with a below-window ACK (Linux style).
				//
				// Note:   SendSegment() does not allow sending an above-window ACK.
				//         Below-window ACK is fine, though.
				//
				// Note 2: If we have urgent data to send, we fall back on standard probe.
				//         since a pure ack can not deliver the urgent pointer.
				//
				if (iSND.UP <= iSND.UNA)
					{
					SendSegment(KTcpCtlACK, iSND.UNA - 1);
					SchedRetransmit();
					break;
					}
				// Fall through

			default:
				// Standard probe. We use a single byte to probe a zero window.
				SendSegment(KTcpCtlACK, iSND.UNA, 1);
				}
			//
			// Note 1: With probes including data iSND.NXT will now be pointing beyond
			// the advertised window of the receiver! This will allow us to accept the
			// incoming probe ack without any special hacks. However, it also means
			// that if we send an ACK or RST, we need to make sure the sequence number
			// is within the receiver window! SendSegment() now makes this adjustment
			// as a special case.
			//
			// Note 2: We may also end up with iSND.NXT pointing beyond the window if
			// the receiver suddenly shrinks its window. The current solution covers
			// both cases.
			//
			return EFalse;
			}

		//
		// This is a retransmit timout. Do we have anything to do?
		//
		if (!unacked)
			return EFalse;

		LOG(if (iFlags.iFastRetransMode) Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): Leaving FAST RETRANS mode"), (TInt)this));
		iFlags.iFastRetransMode = EFalse;
		iDupAcks = 0;

		//
		// Congestion control
		//
		iSsthresh = Max((iSND.NXT - iSND.UNA) / 2, 2 * iSMSS);
		iCwnd = EffectiveMSS();
		iLwnd = 0;
		iRetranData = 0;
		iPartialSeq = iSND.UNA;

		//
		// The receiver may have reneged. Clear SACK info.
		//
		// Only clear the SACK queue if we have a SACKed block immediately
		// after iSND.UNA. In this case the sender has clearly reneged.
		// If the peer does this even once, we will no longer trust it
		// to retain its above-sequence queue and will revert to the normal
		// RFC2018 behaviour of clearing all SACK info on retransmit timeout.
		//
		if (!iFlags.iSackOk || (iFlags.iPeerHasReneged ||
			(iSacked.Count() && iSacked.First()->iLeft == iSND.UNA)))
			{
			if (iFlags.iSackOk)
				{
				LOG(if(!iFlags.iPeerHasReneged) Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): Peer reneged"), (TInt)this));
				iFlags.iPeerHasReneged = ETrue;
				iSacked.Clear();
				}

			//
			// New reno "bugfix" [RFC2582] is applied only if we are forced
			// to discard the SACK info. If we can keep the info we will
			// never send spurious retransmits and thus there will be no
			// dupacks to trigger fast recovery, unless segments have
			// really been lost.
			//
			iSendHigh = iSND.NXT;
			}

		iRealSendHigh = iSND.NXT;

		// Save timestamp for delay spike detection
		if (iFlags.iUseTimeStamps)
			iLastTimeout = TimeStamp();

		//
		// Retransmit segments.
		//
		if (Protocol()->FRTO())
			{
			// F-RTO: Send first unacknowledged segment and continue transmitting new data
			iFRTOsent = 1;
			SendDataSegment(iSND.UNA, ETrue);
			}
		else
			{
			// Normal retransmit
			iTransmitSeq = iSND.UNA;
			SendSegments(ETrue);
			}

		// Store retransmit sequence for SACK retransmit
		if (iTransmitSeq > iSendHigh && !iFRTOsent)
			iSendHigh = iTransmitSeq;

		// If the server doesn't respond because of broken NAT/FW or other, don't keep interface up.
		if (InState(ETcpFinWait1|ETcpClosing|ETcpLastAck))
			DetachIfDead();
		return EFalse;
		}

	//
	// Ok, this is either a SYN/FIN retransmit or a TIME-WAIT/FIN-WAIT-2 timeout.
	//
	if (InState(ETcpSynSent))
		{
		// Retransmit SYN
		SendSegment(KTcpCtlSYN, iSND.UNA);
		return EFalse;
		}

	if (InState(ETcpSynReceived))
		{
		// Retransmit SYN,ACK
		SendSegment(KTcpCtlSYN|KTcpCtlACK, iSND.UNA);
		return EFalse;
		}

	if (InState(ETcpFinWait1|ETcpClosing|ETcpLastAck))
		{
		// If the server doesn't respond because of broken NAT/FW or other, don't keep interface up.
	
	    //TSW error:JHAA-82JBNG -- FIN retransmission 
		//Depending on the function return value the decision to
	    //retransmitt FIN is decided
	
		// Retransmit FIN
		if(DetachIfDead()== EFalse)
			{
			SendSegment(KTcpCtlFIN|KTcpCtlACK, iSND.UNA);
			return EFalse;
			}
		}

	LOG(Log::Printf(_L("\ttcp SAP[%u] RetransmitSegments(): Retransmitter stopping"), (TInt)this));
	if (!iSockFlags.iAttached)
		{
		Expire();
		return ETrue;
		}
	return EFalse;
	}


//
// Respond to an explicit congestion control signal.
// Currently, the signal can come from the link layer
// or from the network as an ICMP source quench.
//
TBool CProviderTCP6::SourceQuench()
	{
	//
	// Allow source quenching approximately once per window.
	// Note: the test is written in such a way that iQuenchSeq
	// does not need to be updated during normal TCP operation.
	//
	// Do not shrink the congestion window if we're doing fast
	// retransmits. That would mess up the congestion window
	// deflation when exiting fast retransmit mode.
	//
	if (iQuenchSeq.Outside(iSND.NXT, iSND.NXT + iSND.WND) && !iFlags.iFastRetransMode)
		{
		iSsthresh = Max((iSND.NXT - iSND.UNA) / 2, 2 * iSMSS);
		iCwnd = iSsthresh;
		iQuenchSeq = iSND.NXT + iSND.WND;
		LOG(Log::Printf(_L("\ttcp SAP[%u] SourceQuench(): flight=%d, cwnd=%d, ssthresh=%d"),
			(TInt)this, iSND.NXT - iSND.UNA, iCwnd, iSsthresh));
		return ETrue;
		}
	return EFalse;
	}


TInt CProviderTCP6::SenderCallBack(TAny* aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] SenderCallBack"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->SendSegments();
	return 0;
	}

TInt CProviderTCP6::ReceiverCallBack(TAny* aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] ReceiverCallBack"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->ProcessSegments();
	return 0;
	}

TInt CProviderTCP6::DelayAckCallBack(TAny* aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] DelayAckCallBack"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->SendSegment(KTcpCtlACK);
	return 0;
	}

TInt CProviderTCP6::TransmitterCallBack(TAny* aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] TransmitterCallBack"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->Transmit();
	return 0;
	}

TInt CProviderTCP6::RetransmitterCallBack(TAny* aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] RetransmitterCallback"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->RetransmitTimeout();
	return 0;
	}


TInt CProviderTCP6::LingerTimerCallBack(TAny *aProviderTCP)
	{
	LOG(Log::Printf(_L("<>\ttcp SAP[%u] Linger timeout()"), (TInt)aProviderTCP));
	((CProviderTCP6*)aProviderTCP)->Detach();
	return 0;
	}


//
// Initialize SRTT measurement
//
void CProviderTCP6::ClearRTT()
	{
	iSRTT = 0;
	iMDEV = Protocol()->MdevSmooth() * Protocol()->InitialRTO();
	if (Protocol()->RTO_K())
		iMDEV /= Protocol()->RTO_K();
	}


//
// Update RTO using van Jacobson's algorithm
//
void CProviderTCP6::UpdateRTO(TUint32 aRTT)
	{
	if (!iSRTT && aRTT > Protocol()->InitialRTO() / 2)
		{
		iSRTT = Protocol()->SrttSmooth() * aRTT;
		iMDEV = Protocol()->MdevSmooth() * aRTT / 2;
		}
	else
		{
		TInt delta = aRTT - (iSRTT / Protocol()->SrttSmooth()); // delta >= -iSRTT
		iSRTT += delta;
		if (!iSRTT)
			iSRTT = 1;
		if (delta < 0)
			delta = -delta;
		delta -= (iMDEV / Protocol()->MdevSmooth()); // delta >= -iMDEV
		iMDEV += delta;
		}

	ResetRTO();
	LOG(Log::Printf(_L("\ttcp SAP[%u] UpdateRTO(): RTT=%d SRTT=%d MDEV=%d BACKOFF=%d RTO=%d"),
		(TInt)this, aRTT, iSRTT / Protocol()->SrttSmooth(), iMDEV / Protocol()->MdevSmooth(), iBackoff, iRTO));
	}

//
// Calculate RTO with backoff
//
void CProviderTCP6::ResetRTO()
	{
	iRTO = (iSRTT / Protocol()->SrttSmooth()) +
		Max(Protocol()->RTO_K() * iMDEV / Protocol()->MdevSmooth(), Protocol()->RTO_G());
	if (iRTO < Protocol()->MinRTO())
		iRTO = Protocol()->MinRTO();
	iRTO <<= iBackoff;
	if (iRTO > Protocol()->MaxRTO())
		iRTO = Protocol()->MaxRTO();
	LOG(Log::Printf(_L("\ttcp SAP[%u] ResetRTO(): SRTT=%d MDEV=%d BACKOFF=%d RTO=%d"),
		(TInt)this, iSRTT / Protocol()->SrttSmooth(), iMDEV / Protocol()->MdevSmooth(), iBackoff, iRTO));
	}

//
// Set initial congestion window
//
void CProviderTCP6::ResetCwnd(TUint aSMSS)
	{
	if (Protocol()->RFC2414())
		iCwnd = Min(4 * aSMSS, Max(2 * aSMSS, 4380));
	else
		iCwnd = Protocol()->InitialCwnd() * aSMSS;
	LOG(Log::Printf(_L("\ttcp SAP[%u] ResetCwnd(): cwnd=%d"), (TInt)this, iCwnd));
	}


//
// Schedule a FIN-WAIT-2 or TIME-WAIT timeout
//
void CProviderTCP6::SchedMsl2Wait()
	{
	ASSERT(iRetransTimer);

	iBackoff = 0;
	iRetransTimer->Restart(Protocol()->Msl2Delay());

	//
	// Remove this SAP from the user socket count.
	//
	DetachFromInterface();
	}

//
// Detach the SAP if the application has closed and we seem to keep resending stuff.
//
TBool CProviderTCP6::DetachIfDead()
	{
	if (iSockFlags.iRecvClose && iSockFlags.iSendClose && Protocol()->FinPersistency()
		&& iBackoff >= Protocol()->FinPersistency())
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] Peer looks dead while %d bytes unacked data. Detach!"), 
			(TInt)this, Min(iSND.NXT - iSND.UNA, iSockOutQLen)));
		DetachFromInterface();
		
		//TSW error:JHAA-82JBNG -- FIN retransmission 
		//The FinPersistency value read from the tcpip.ini file determines
		//the number of times FIN should be retransmitted
		//Once we reach the maximum value the DetachFromInterface() is called
		//and we return ETrue. 
		return ETrue;
		}
	return EFalse;
	}

void CProviderTCP6::DetachFromInterface()
/**
Removes this SAP from the user socket count and interface flow count.
This allows link layer and TCP/IP stack go down if no one else uses it.
*/
	{
	const TInt off = 0;
	SetOption(KSolInetIp, KSoUserSocket, TPtr8((TUint8*)&off, sizeof(TInt), sizeof(TInt)));
	SetOption(KSolInetIp, KSoKeepInterfaceUp, TPtr8((TUint8*)&off, sizeof(TInt), sizeof(TInt)));
	}


//
// Check ioctl completion
//
void CProviderTCP6::CompleteIoctl(TInt aError)
	{
	if (iFlags.iDataSentIoctl && iSockFlags.iNotify)
		{
		if (!iSockOutQLen)
			{
			iFlags.iDataSentIoctl = EFalse;
			iSocket->IoctlComplete(0);
			}
		else if (aError != KErrNone)
			{
			iFlags.iDataSentIoctl = EFalse;
			Error(KErrCancel, MSocketNotify::EErrorIoctl);
			}
		}
	}

//
//  Main incoming segment processing loop
//
void CProviderTCP6::ProcessSegments()
	{
	LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments()"), (TInt)this));

	ASSERT(iRetransTimer);

	RMBufRecvPacket packet;
	TBool immediateAck = EFalse;

	while (iRecvQ.Remove(packet))
		{
		RMBufRecvInfo* info = packet.Unpack();
		TIpHeader *ip = ((RMBufPacketPeek &)packet).GetIpHeader();
		TTcpPacket seg(packet, info->iOffset);
		TTcpOptions opt;
		seg.iHdr->Options(opt);
		TInt len = packet.Length() - seg.iHdr->HeaderLength() - info->iOffset;
		TTcpSeqNum seq = seg.iHdr->Sequence();
		TTcpSeqNum ack = seg.iHdr->ACK() ? seg.iHdr->Acknowledgment() : TTcpSeqNum(0);
		TInt fin = seg.iHdr->FIN();  // Save this for FIN processing
		CProviderTCP6 *nSAP = NULL;
		TTcpSeqNum rcvNxt = iRCV.NXT; // Used in SYN_RECEIVED state

		// Get current time
		TUint32 usec = TimeStamp();

		if (InState(ETcpListen))
			{
			TUint32 tsEcr;

			if (!iParent)
				{
				//
				// Server socket LISTEN state processing.
				//

				// Sanity checks.
				if (iConnectCount >= iListenQueueSize || seg.iHdr->RST())
					goto discard;

				// Check invalid address.
				if (!TInetAddr::Cast(info->iSrcAddr).IsUnicast() ||
					!TInetAddr::Cast(info->iDstAddr).IsUnicast())
					goto discard;

				if (IsLandAttack(info))
					goto discard;
				
				if (seg.iHdr->ACK())
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): ACK received in LISTEN state. Sending RST"), (TInt)this));
					SendReset(ack, info->iDstAddr, info->iSrcAddr);
					goto discard;
					}

				if (!seg.iHdr->SYN())
					goto discard;

				if (opt.Error())
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid TCP options"), (TInt)this));
					SendResetNoSync(seq+len+1, info->iDstAddr, info->iSrcAddr);
					goto discard;
					}

				if (opt.MSS() > 0 && opt.MSS() < STATIC_CAST(TInt, KTcpMinimumMSS))
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid MSS option"), (TInt)this));
					SendResetNoSync(seq+len+1, info->iDstAddr, info->iSrcAddr);
					goto discard;
					}

				//
				// Sometimes SYN retransmissions get compressed in the network
				// and get queued in the listen socket. If this happens we just
				// punt the SYN packet to the correct SAP.
				//
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
				nSAP = (CProviderTCP6*)Protocol()->LocateSap(EMatchConnection, KAFUnspec,
					info->iDstAddr, info->iSrcAddr);
#else
				nSAP = (CProviderTCP6*)Protocol()->LocateSap(EMatchConnection, KAFUnspec,
					info->iDstAddr, info->iSrcAddr, NULL, info->iInterfaceIndex);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
				if (nSAP == NULL)
					{
					// Create a new SAP and initialize it
					if (CreateChild(nSAP) != KErrNone)
						goto discard;
					
#ifdef SYMBIAN_NETWORKING_UPS					
					nSAP->iConnectionInfoReceived = 1;
#endif
					RFlowContext flowCtxt = nSAP->iFlow;
					flowCtxt.SetLocalAddr(info->iDstAddr);
					flowCtxt.SetRemoteAddr(info->iSrcAddr);
					nSAP->EnterState(ETcpListen);

					// Create a flow context
					TInt status = flowCtxt.Connect();
					if (status < KErrNone)
						{
						LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSYN(): Error %d opening flow context"), (TInt)this, status));
						delete nSAP;
						SendResetNoSync(seq+len+1, info->iDstAddr, info->iSrcAddr);
						goto discard;
						}

					// Disable on-demand interface setup for the child
					flowCtxt.FlowContext()->iInfo.iNoInterfaceUp = 1;
					nSAP->iSockFlags.iConnected = ETrue;
					Protocol()->BindProvider(nSAP);
					}

				// Get the new SAP started. It will handle the rest,
				packet.Pack();
				nSAP->Process(packet);
				continue;
				}


			//
			// Child socket LISTEN state processing.
			//

	  /*
	      Set RCV.NXT to SEG.SEQ+1, IRS is set to SEG.SEQ and any other
	      control or text should be queued for processing later.  ISS
	      should be selected and a SYN segment sent of the form:

		<SEQ=ISS><ACK=RCV.NXT><CTL=SYN,ACK>

	      SND.NXT is set to ISS+1 and SND.UNA to ISS.  The connection
	      state should be changed to SYN-RECEIVED.  Note that any other
	      incoming control or data (combined with SYN) will be processed
	      in the SYN-RECEIVED state, but processing of SYN and ACK should
	      not be repeated.  If the listen was not fully specified (i.e.,
	      the foreign socket was not fully specified), then the
	      unspecified fields should be filled in now.
			*/

			// Peer groks timestamps?
			iFlags.iUseTimeStamps &= opt.TimeStamps(iTsRecent, tsEcr);
			if (!iFlags.iUseTimeStamps)
				iOptions.ClearTimeStamps();

			// Peer groks SACK?
			iFlags.iSackOk &= opt.SackOk();
			if (!iFlags.iSackOk)
				iOptions.ClearSackOk();

			// Peer denies ECN
			if (!seg.iHdr->ECE() || !seg.iHdr->CWR())
				{
				iFlags.iEcn = EFalse;
				}

			// Note: the parent does error checking before instantiating a child socket.
			if (opt.MSS() >= 0)
				iSMSS = opt.MSS();

			//
			// The following is based on network interface MTU.
			// We limit the advertised MSS to window size divided by four,
			// in case the interface supportes a large (or infinite) MTU.
			//
			iRMSS = Min(LinkRMSS(), iSockInBufSize / 2);
			iOptions.SetMSS(iRMSS);

			// Initialise send sequence number
			iTransmitSeq = iSND.NXT = Protocol()->RandomSequence();

			if (!opt.WindowScale() || Protocol()->WinScale() == -1)
				{
				iRcvWscale = 0;
				iOptions.SetWindowScale(0);
				iSockInBufSize = Min(iSockInBufSize, 0xffff);
				}
			else
				{
				iSndWscale = opt.WindowScale() - 1;
				iRcvWscale = (Protocol()->WinScale() > 0 ?
					Protocol()->WinScale() - 1 : NeedWindowScale());
				iOptions.SetWindowScale((TUint8)(iRcvWscale + 1));
				}
				
			// Initialize receiver sequence and window
			iFreeWindow = iSockInBufSize % iRMSS;
			iRCV.WND = iSockInBufSize - iFreeWindow;
			iRCV.NXT = seg.iHdr->Sequence() + 1;

			TUint8 flags = KTcpCtlSYN|KTcpCtlACK;
			if (iFlags.iEcn)
				{
				flags |= KTcpCtlECE;
				}

			// Do not kick idle timers until we are in established state
			StoreKeepInterfaceUp();
			iFlow.FlowContext()->SetOption(KSolInetIp, KSoKeepInterfaceUp, KInetOptionDisable);

			// Send SYN-ACK
			iSND.UNA = iSND.NXT;
			SendSegment(flags);
			++iTransmitSeq;

			// Make sure we don't miss our window update.
			iSND.WL1 = iRCV.NXT;
			iSND.WL2 = iSND.NXT;

			iPartialSeq = iSND.UNA;

			EnterState(ETcpSynReceived);
			goto accept;
			}

		if (InState(ETcpSynSent))
			{
			if (seg.iHdr->ACK() && ack.Outside(iSND.UNA, iSND.NXT))
				{
				LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid ack=%u"), (TInt)this, ack.Uint32()));
				if (!seg.iHdr->RST())
					SendReset(ack);
				goto discard;
				}

			if (seg.iHdr->RST())
				{
				if (seg.iHdr->ACK())  // No need to check ack sequence because it was tested above
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): RST packet"), (TInt)this));
					Error(KErrCouldNotConnect, MSocketNotify::EErrorConnect);
					EnterState(ETcpClosed);
					goto wrapup;
					}
				else
					goto discard;
				}

			if (!seg.iHdr->SYN())
				goto discard;

			// Process options for SYN packet.
			if (opt.Error())
				{
				// In case we receive invalid optons, we should just report an error and reset the connection.
				LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid options, reset connection"), (TInt)this));
				if (seg.iHdr->ACK())
					SendReset(ack);
				else
					SendResetNoSync(seq+len+1);
				Error(KErrCouldNotConnect, MSocketNotify::EErrorConnect);
				EnterState(ETcpClosed);
				goto wrapup;
				}

			if (opt.MSS() > 0)
				{
				if (opt.MSS() < STATIC_CAST(TInt, KTcpMinimumMSS))
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid MSS option"), (TInt)this));
					if (seg.iHdr->ACK())
						SendReset(ack);
					else
						SendResetNoSync(seq+len+1);
					goto discard;
					}
				iSMSS = opt.MSS();
				}

			iFlags.iUseTimeStamps &= opt.TimeStamps();
			iFlags.iSackOk &= opt.SackOk();

			if (opt.WindowScale())
				{
				iSndWscale = opt.WindowScale() - 1;
				iFreeWindow = iSockInBufSize % iRMSS;
				iRCV.WND = iSockInBufSize - iFreeWindow;
				}
			else
				{
				// iFreeWindow and iRCV.WND were confirmed to be less than 0xffff with SYN
				iRcvWscale = 0;
				iSockInBufSize = Min(iSockInBufSize, 0xffff);
				}

			iRCV.NXT = seq+1;

			// Our SYN was acked?
			if (seg.iHdr->ACK() && ack == iSND.NXT)
				{
				iSND.UNA = ack;
				iPartialSeq = ack;

				TUint32 tsEcr;
				// Check that the timestamp echo has sane values
				if (iFlags.iUseTimeStamps && opt.TimeStamps(iTsRecent, tsEcr)
						&& tsEcr && usec > tsEcr && usec - tsEcr <= Protocol()->MaxRTO())
					UpdateRTO(usec - tsEcr);
				else if (iFlags.iTiming)
					UpdateRTO(usec - iTimeStamp);
				iFlags.iTiming = EFalse;

				//
				// Initial window update as per RFC1122.
				//
				//LOG(Log::Printf(_L("CProviderTCP6::ProcessSegments(): Window update.\r\n")));
				iSND.WND = seg.iHdr->Window(); // no scaling in SYN
				iSND.WL1 = seq;
				iSND.WL2 = ack;
				iLastWnd = seg.iHdr->Window(); // no scaling in SYN

				ResetCwnd(iSMSS);
				iSsthresh = KMaxTInt;

				ClearSYNSettings();

				// Peer denies ECN with SYN-ACK.
				if (!seg.iHdr->ECE())
					{
					iFlags.iEcn = EFalse;
					}

				ReadDestinationCache();

				// Open up the window for sender
				SendSegment(KTcpCtlACK);
				EnterState(ETcpEstablished);
				//__ASSERT_DEBUG(iSockFlags.iNotify, User::Panic(_L("notifier"),0));
				// Assert removed. There's a rare case when the stack is unloading. -MikaL

				// if ECN negotiation was succesful, tell IP layer (iface.cpp) that ECT bit can be enabled.
				if (iFlags.iEcn)
					{
					SetEcn(Protocol()->Ecn());
					}

				if (iSockFlags.iNotify)
					iSocket->ConnectComplete();
					
				// SYN uses one slot in sequence number space. Account for it before processing data.
				++seq;
					
				goto process_data;
				}

			//
			// Simultaneous open, send SYN+ACK
			//
			--iSND.NXT;

			// Peer denies ECN in SYN
			if (!seg.iHdr->ECE() || !seg.iHdr->CWR())
				{
				iFlags.iEcn = EFalse;
				}

			TUint8 flags = KTcpCtlSYN|KTcpCtlACK;
			if (iFlags.iEcn)
				{
				flags |= KTcpCtlECE;
				}
			SendSegment(flags);
			EnterState(ETcpSynReceived);
			//
			// According to RFC793 we should queue any additional data
			// and controls (e.g. URG) for processing once the established
			// state has been reached.
			//
			// No current TCP implementations actually do this (aside from
			// T/TCP), because of its vulnerability to DoS attacks. In any
			// case, we're compatible with a peer that wants to send data with
			// the SYN packet, since the peer will retransmit the data after
			// we only ack the SYN.
			//
			goto accept;
			}

		if (!InState(ETcpSynReceived|ETcpEstablished|ETcpFinWait1|ETcpFinWait2|
			ETcpCloseWait|ETcpClosing|ETcpLastAck|ETcpTimeWait))
			{
			if (InState(ETcpClosed))
				{
				// We have extra segments after entering CLOSED state. Send RST.
				if (seg.iHdr->ACK())
					SendReset(ack, info->iDstAddr, info->iSrcAddr);
				else
					SendResetNoSync(seq+len, info->iDstAddr, info->iSrcAddr);
				}
 			goto wrapup;
			}

/*
      Four cases From RFC793 to test in order to determine whether
      an arriving segment is acceptable:

      Segment Receive  Test
      Length  Window
      ------- -------  -------------------------------------------
	 0       0     SEG.SEQ = RCV.NXT
	 0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
	>0       0     not acceptable
	>0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
		    or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND

      We transform these into two basic tests, both of which must evaluate
      true in order to DISCARD a segment. The first test clears the majority
      of segments that are correctly within receive window. The second test
      clears special cases where receive window is zero or the incoming segment
      is only partially within receive window.

      Note that we accept a segment that is straddling iRCV.NXT even if
      the receive window is zero. In this case the segment is discarded
      later during data segment processing.
		*/

		// Eliminate earlier DSACK blocks. One duplicate segment can be reported only once.
		if (iFlags.iSackOk && Protocol()->DSack())
			iOptions.SackBlocks().Prune(iRCV.NXT);

		if (InState(ETcpSynReceived))
			{
			// In case of simultanous connections TCP retransmits SYN with initial sequence
			// number when it gets incoming SYN in SYN_SENT state
			// (it acks received SYN+1 in that case).
			// Since RCV.NXT is increased after receiving the first SYN, the SYN retransmission
			// would appear as out-of-order segment, in which case it would be ignored and the TCP
			// session would never get to ESTABLISHED state.
			//
			// This adjustment is meant to avoid the above problem while causing minimal compromise
			// to TCP sequence number check safety.
			rcvNxt = iRCV.NXT - 1;
			}

		//      if ((seq < iRCV.NXT || seq >= iRCV.NXT + iRCV.WND) &&                          // 1st test
		//	  ((iRCV.WND > 0) ? (seq + len <= iRCV.NXT) : (len > 0 || seq > iRCV.NXT)))  // 2nd test
		// PeLu: The above test passed case where seq > nxt+wnd
		if ((seq < rcvNxt || seq >= iRCV.NXT + iRCV.WND) &&                          // 1st test
			((iRCV.WND > 0) ? ((len > 0) ? (seq+len <= iRCV.NXT) || (seq+len > iRCV.NXT + iRCV.WND) : 1)
			: ((len > 0) ? 1 : (seq.Outside(rcvNxt, iRCV.NXT)))))
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Segment out of window"), (TInt)this));
			if (!seg.iHdr->RST())
				{
				// if receiving data partially below RCV.NXT,
				// generate DSACK only for the duplicate portion
				if (iFlags.iSackOk && Protocol()->DSack() && seq < iRCV.NXT && len > 0)
					{
					if (seq + len <= iRCV.NXT)
						{
						iOptions.SackBlocks().AddUnordered(seq, seq + len);
						}
					else
						{
						iOptions.SackBlocks().AddUnordered(seq, iRCV.NXT);
						}
					iOptions.SackBlocks().Limit(4);
					}
				SendSegment(KTcpCtlACK);
				}
			if (InState(ETcpTimeWait|ETcpFinWait2))
				SchedMsl2Wait();
			// Accept RST segments that fit within the last advertised window.
			if (seg.iHdr->RST() && !seq.Outside(iLastAck, iLastAck + iRCV.WND))
				LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Lagged RST"), (TInt)this));
			else if (iRCV.WND > 0 || iRCV.NXT.Outside(seq, seq+len))
				goto discard;
			}

		if (seg.iHdr->RST())
			{
			// Ignore RST in TIME-WAIT state to prevent TIME-WAIT assassination
			if (InState(ETcpTimeWait))
				goto discard;

			// Stop processing and enter CLOSED state
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): RST packet"), (TInt)this));
			Error(KErrDisconnected);
			EnterState(ETcpClosed);
			goto wrapup;
			}

		if (seg.iHdr->SYN() && seq >= iRCV.NXT)
			{
	  		/*  If the SYN is in the window it is an error, send a reset, any
	  		outstanding RECEIVEs and SEND should receive "reset" responses,
 		 	all segment queues should be flushed, the user should also
			receive an unsolicited general "connection reset" signal, enter
			the CLOSED state, delete the TCB, and return.
				
			Note: the extra test against RCV.NXT is there to let through a SYN-ACK
			with the sequence number RCV.NXT-1. This may happen in a simultaneous
			connect case, where the SYN-ACK packet is retransmitted. The SYN-ACK
			will be hanled below as part of SYN-RECEIVED state processing.

			Otherwise, if the SYN is not in the window this step would not be
			reached and an ack would have been sent in the first step (sequence
			number check).
			*/
			SendReset(iSND.NXT);
			Error(KErrDisconnected);
			EnterState(ETcpClosed);
			goto wrapup;
			}


		//
		//  ACK PROCESSING
		//
		if (!seg.iHdr->ACK())
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): No ACK"), (TInt)this));
			goto discard;
			}

		// Check options
		if (opt.Error())
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Invalid options"), (TInt)this));
			SendReset(ack);
			goto discard;
			}

		if (InState(ETcpSynReceived))
			if(ack.Inside(iSND.UNA, iSND.NXT))
			{
			EnterState(ETcpEstablished);

			const TInt ifup = iFlags.iKeepInterfaceUp;
			SetOption(KSolInetIp, KSoKeepInterfaceUp, TPtr8((TUint8*)&ifup, sizeof(TInt), sizeof(TInt)));

			// if ECN negotiation was succesful, tell IP layer that ECT bit can be enabled.
			if (iFlags.iEcn)
				{
				SetEcn(Protocol()->Ecn());
				}

			//
			// Initial window update as per RFC1122.
			//
			//LOG(Log::Printf(_L("CProviderTCP6::ProcessSegments(): Window update.\r\n")));
			iSND.WND = seg.iHdr->Window() << iSndWscale;
			iSND.WL1 = seq;
			iSND.WL2 = ack;
			iLastWnd = seg.iHdr->Window() << iSndWscale;

			ResetCwnd(iSMSS);
			iCwnd -= iSMSS;   // Compensate for cwnd increase below during ack processing
			iSsthresh = KMaxTInt;

			ClearSYNSettings();

			ReadDestinationCache();

			// The following causes the socket server to (eventually) call Start()
			if (iParent)
				{
				iSockFlags.iAttached = ETrue;
				ASSERT(iParent->iSockFlags.iNotify);
				if (iParent->CompleteChildConnect(this) == KErrAbort)
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): SAP Deleted, abort ProcessSegments()"), (TInt)this));
					// Object in use was deleted. Just free the packet and return.
					packet.Free();
					return;
					}
				}
			else
				{
				//__ASSERT_DEBUG(iSockFlags.iNotify, User::Panic(_L("notifier"), 0));
				// Assert removed. There's a rare case when the stack is unloading. -MikaL
				if (iSockFlags.iNotify)
					iSocket->ConnectComplete();
				}
			}
		else
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Bad SYN-ACK"), (TInt)this));
			SendReset(ack);
			goto discard;
			}

		//
		// Check ack sequence for all remaining states
		//
		if(ack > iSND.NXT)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): ACK above window"), (TInt)this));
			SendSegment(KTcpCtlACK);
			goto discard;
			}

		// A good packet arrived and the connection is idle, reset the keep-alives state
		if (CanFireKeepAlives())
			ResetKeepAlives();

		// Check for ECN congestion established bit.
		if (info->iVersion == 4)
			{
			if (iFlags.iEcn && ip->ip4.EcnIsCongestion())
				{
				iFlags.iEcnHaveCongestion = ETrue;
				LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Have ECN CE bit in IPv4 packet"), (TInt)this));
				}
			}
		else
			{
			if (iFlags.iEcn && ip->ip6.EcnIsCongestion())
				{
				iFlags.iEcnHaveCongestion = ETrue;
				LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Have ECN CE bit in IPv6 packet"), (TInt)this));
				}
			}
		
		if (InState(ETcpEstablished|ETcpFinWait1|ETcpFinWait2|ETcpCloseWait|ETcpClosing|ETcpLastAck))
			{
			TInt acked = ack - iSND.UNA;

			if (acked >= 0)
				{
				// ECN CWR means that receiver has noticed our Congestion Echo. No more congestion.
				if (seg.iHdr->CWR())
					{
					iFlags.iEcnHaveCongestion = EFalse;
					}

				if (iFlags.iUseTimeStamps)
					{
					TUint32 tsVal, tsEcr;
					// Check that tsEcr has sane values
					if (opt.TimeStamps(tsVal, tsEcr) && tsEcr && usec - tsEcr <= KTcpMaxRTO)
						{
						if (acked > 0)
							{
							UpdateRTO(usec - tsEcr);

							// Got one RTT with timestamps. Don't take another.
							if (iFlags.iTiming && ack >= iTimingSeq)
								iFlags.iTiming = EFalse;

							//
							// Delay spike detection. If the echoed timestamp predates
							// our last retransmission timeout we abort the retransmission
							// sequence. However, we do not inflate the congestion window
							// for two reasons: 1) a long delay might also be a sign of
							// congestion, and 2) this feature could potentially be used
							// by a hostile peer to artificially inflate our congestion
							// window.
							//
							// PS: Delay spike detection with timestamps is not in use with F-RTO
							if (!Protocol()->FRTO() && tsEcr < iLastTimeout &&
								!opt.SackBlocks().Count())
								{
								iTransmitSeq = iSND.NXT;
								SpuriousTimeout(acked);
								}
							}

						//
						// The condition in RFC1323 is buggy(?):
						//
						//    SEG.SEQ <= Last.ACK.sent < SEG.SEQ + SEG.LEN
						//
						// The best current practise is the following, because it updates
						// TSrecent also on pure ACKs:
						//
						//    SEG.TSval >= TSrecent and SEG.SEQ <= Last.ACK.sent
						//
						if (tsVal > iTsRecent && seq <= iLastAck)
							iTsRecent = tsVal;
						}
					}
				if (iFlags.iTiming && ack >= iTimingSeq)
					{
					UpdateRTO(usec - iTimeStamp);
					iFlags.iTiming = EFalse;
					}

				// SACK book keeping
				if (iFlags.iSackOk)
					{
					// Remove acknowledged blocks
					if (opt.SackBlocks().Count())
						{
						SequenceBlockQueueIter iter(opt.SackBlocks());
						SequenceBlock *block;
						while (block = iter++, block != NULL)
							{
							//
							// Record SACK block but do some sanity checking first.
							//
							if (ack < block->iLeft && block->iLeft < block->iRight
								&& block->iRight <= iSND.NXT)
								{
								iSacked.AddOrdered(block);
								}
							}

						// Take RTT estimate using SACK info
						if (iFlags.iTiming && iSacked.Count() && iSacked.Last()->iRight >= iTimingSeq)
							{
							UpdateRTO(usec - iTimeStamp);
							iFlags.iTiming = EFalse;
							}
						}
					iSacked.Prune(ack);

					// Update iRetranData
					if (iSacked.Count() && ack < iSendHigh)
						{
						iRetranData = iSendHigh - ack;
						SequenceBlockQueueIter iter(opt.SackBlocks());
						SequenceBlock *block;
						while (block = iter++, block != NULL && block->iLeft < iSendHigh)
							{
							if (block->iRight < iSendHigh)
								iRetranData -= (block->iRight - block->iLeft);
							else
								iRetranData -= (iSendHigh - block->iLeft);
							}
						}
					else
						iRetranData = 0;

					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): acked=%d iRetranData = %d"),
						(TInt)this, acked, iRetranData));
					}

				// Did they acknowledge any new data?
				if (acked > 0)
					{
					// Reset retransmit backoff
					if (iBackoff && iSND.WND > 0)
						{
						LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Some data ACKed, clearing Backoff"), (TInt)this));
						iBackoff = 0;
						}

					// Trim send queue
					iSND.UNA = ack;
					if ((TInt)iSockOutQLen > acked) // Avoid barfing on acked SYN or FIN packet
						{
						iSockOutQ.TrimStart(acked);
						iSockOutQLen -= acked;
						}
					else
						{
						iSockOutQ.Free();
						iSockOutQLen = 0;
						CompleteIoctl(KErrNone);
						}

					// Tag along
					if (iSND.UP < ack)
						iSND.UP = ack - 1;

					// Tag along
					if (iPartialSeq < ack)
						iPartialSeq = ack;

					// Tag along
					if (iSendHigh < ack)
						iSendHigh = ack - 1;

					if (iFlags.iFastRetransMode)
						{
						if (iFlags.iSackOk ? !iSacked.Count() : ack >= iRecoverSeq)
							{
							LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Leaving FAST RETRANS mode"), (TInt)this));
							iFlags.iFastRetransMode = EFalse;
							iDupAcks = 0;
							// Deflate congestion window
							iCwnd = Min(iSsthresh, iSND.NXT - iSND.UNA + iSMSS);
							}
						else if (!iFlags.iSackOk)
							{
							// NewReno partial ACK processing.

						  /* From RFC2582:
						   If this ACK does *not* acknowledge all of the data up to and
						   including "recover", then this is a partial ACK.  In this case,
						   retransmit the first unacknowledged segment.  Deflate the
						   congestion window by the amount of new data acknowledged, then
						   add back one MSS and send a new segment if permitted by the new
						   value of cwnd.  This "partial window deflation" attempts to
						   ensure that, when Fast Recovery eventually ends, approximately
						   ssthresh amount of data will be outstanding in the network.  Do
						   not exit the Fast Recovery procedure (i.e., if any duplicate ACKs
						   subsequently arrive, execute Steps 3 and 4 above).

						   For the first partial ACK that arrives during Fast Recovery, also
						   reset the retransmit timer.
							*/

							iCwnd -= acked;
							iCwnd += iSMSS;
							LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): FAST RETRANSMIT on PARTIAL ACK"), (TInt)this));
							SendDataSegment(ack);

							//
							// Socket write makes use of the duplicate ack count to
							// temporarily extend the send buffer during fast recovery.
							// We deflate the buffer here.
							//
							ASSERT(iSMSS);
							iDupAcks = Max(iDupAcks - acked / (TInt)iSMSS, 0);
							}
						}
					else if ( iDupAcks )
                        {
                        // New data acknowledged, and not ongoing any recovery action
                        // Reset duplicate ack count
                        LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Reset iDupAcks to 0"), (TInt)this));
                        iDupAcks = 0;
                        }

					// Reset limited transmit window
					iLwnd = 0;

					// Everything acked?
					if (ack == iSND.NXT)
						{
						iTransmitSeq = iSND.NXT;
						}
					else
						{
						// Restart retransmission timeout
						ReSchedRetransmit();
						}

					//
					// Adjust congestion window.
					//
					TUint incr = iSMSS;

					if (iCwnd < iSsthresh)
						iCwnd += incr;			    // Slow-start
					else
						iCwnd += Max(incr * incr / iCwnd, 1); // Congestion avoidance
					}

				else if (ack < iSND.NXT)

					{
					//
					// Fast retransmit algorithm.
					//
					// Note! We only reset the duplicate ack count if the received
					// segment acknowledges some new data or if a timeout has
					// occurred. However, we simply ignore window updates and piggy-back
					// acknowledgements unless they also acknowledge new data. Other
					// duplicate acknowledgements increase the duplicate ack count and
					// may trigger a fast retransmission.
					//
					if (len == 0 && (seg.iHdr->Window() << iSndWscale) == iLastWnd)
						{
						if (iFlags.iSackOk)
							{
							TTcpSeqNum fack = iSacked.Count() ? iSacked.Last()->iRight : iSND.UNA;

							// Acks caused by out-of-window segments don't count
							if (opt.SackBlocks().Count() && opt.SackBlocks().First()->iRight > iSND.UNA)
								iDupAcks++;

							 //Removed the conditional checking ack > iSendHigh as it is only being used for if SACK option is not set.
							 // Set the retransmission mode as the fast retransmit if the three TCP duplicate ACKs are received from the server and
                             //send the lost segment(s) to the server.
							if (
								(iDupAcks >= Protocol()->Reordering() ||
								fack > iSND.UNA + Protocol()->Reordering() * iRMSS))
								{
								LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): SACK RETRANSMIT!"), (TInt)this));
								iFlags.iFastRetransMode = ETrue;
								iSsthresh = Max((iSND.NXT - iSND.UNA) / 2, 2 * iSMSS);
								iCwnd = iSsthresh;
								iSendHigh = ack;
								ReSchedRetransmit();    // Restart retransmission timeout
								iTransmitSeq = iSendHigh; // Rewind transmitter for SACK retransmit
								SendSegments(ETrue);
	                            iDupAcks = 0;

								}
							}
						else
							{
							if (ack > iSendHigh)        // Never retransmitted?
								iDupAcks++;
							if (iFlags.iFastRetransMode)
								{
								iCwnd += iSMSS;         // Inflate congestion window
								}
							else if (iDupAcks == Protocol()->Reordering())
								{
								LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): FAST RETRANSMIT!"), (TInt)this));
								iFlags.iFastRetransMode = ETrue;
								iSsthresh = Max((iSND.NXT - iSND.UNA) / 2, 2 * iSMSS);
								iCwnd = iSsthresh + 3 * iSMSS;
								iRecoverSeq = iSND.NXT;
								iLwnd = 0;                    // Reset limited transmit window
								ReSchedRetransmit();	    // Restart retransmission timeout
								SendDataSegment(ack);         // Retransmit a single segment
								}
							}
						if (!iFlags.iFastRetransMode)
							{
							//
							// Increment limited transmit window
							//
							if (iSND.UNA + iCwnd >= iSND.NXT)  // FIXME. Prevent lwnd increase during RTO
								iLwnd = Min(iLwnd + iSMSS, iSMSS * Protocol()->LtxWindow());
							ReSchedRetransmit();
							}
						}

					}
				else
					iDupAcks = 0;

				// F-RTO stuff. If we enter here, RTO has just occurred, and e.g. iDupAcks should be 0
				if (iFRTOsent)
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] F-RTO: iFRTOsent=%d   acked=%d   ack-sndhigh=%d"),
						(TInt)this, iFRTOsent, acked, (ack - iSendHigh)));

					// The ack following the RTO did not advance window, or we cannot transmit new
					// unsent data => conventional recovery
					//
					// The last term of the condition below is due to special case:
					// If the first ack after RTO covers all outstanding data, the RTO was due
					// to lost retransmit and fixed the whole outstanding window).
					if (!acked || !CanForwardTransmit() || (ack >= iRealSendHigh && iFRTOsent == 1))
						{

						LOG(Log::Printf(_L("\ttcp SAP[%u] F-RTO: Doing go-back-N"), (TInt)this));
						iTransmitSeq = iSND.UNA;
						if (iFRTOsent == 1 && !acked)
							{
							// dupack arrives before RTO retransmission is acknowledged.
							// don't retransmit the same segment again.
							iTransmitSeq += EffectiveMSS();
							}
						iCwnd = EffectiveMSS() * iFRTOsent; // number of RTTs after RTO
						iFRTOsent = 0;
						}
					else
						{
						// Force 2 segments out on first ack after RTO. For that purpose we have to
						// estimate the current flightsize (-> iCwnd) and set iLwnd to 2.
						// Note that after this step iCwnd is set either to 2*MSS or iSsthresh, so
						// the setup below is very temporary.
						if (iFRTOsent == 1)  // first new ACK after RTO
							{
							iCwnd = iSND.NXT - iSND.UNA - iSacked.ByteCount() + 2 * iSMSS;
							}
						else if (iFRTOsent == 2)
							{
							// Spurious RTO.
							// second new ACK after RTO, continue in earlier state because the second
							// ACK was delayed and the RTO was likely spurious.
							SpuriousTimeout(acked);
							}
						// coverity[write_write_order]
						iFRTOsent = (++iFRTOsent) % 3;
						}
					}

				iLastWnd = seg.iHdr->Window() << iSndWscale;

				if (iSND.WL1 < seq || (iSND.WL1 == seq && iSND.WL2 <= ack))
					{
					iSND.WND = seg.iHdr->Window() << iSndWscale;
					iSND.WL1 = seq;
					iSND.WL2 = ack;
					//LOG(Log::Printf(_L("CProviderTCP6::ProcessSegments(): Window update: %d.\r\n"), iSND.WND));
					}

				// if we are not yet in recovery, reduce congestion window on ECN CE Echo.
				if (iFlags.iEcn && seg.iHdr->ECE())
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Got ECN echo"), (TInt)this));

					// SourceQuench returns False, if congestion window was reduced for some other
					// reason in the last RTT. However, if it was reduced due to FR
					// (i.e. last QuenchSeq was more than one RTT ago), we better send CWR to suppress
					// ECE flags at the other end.
					if (SourceQuench() || iQuenchSeq.Outside(iSND.NXT, iSND.NXT + iSND.WND))
						{
						iFlags.iEcnSendCWR = ETrue;
						}
					}
				// This section used to hold the RetryACK concept, a reference can be checked
				// from older versions(9.2/9.3). Its being removed as not required.	
					
				}
			}

		//
		// Everything acked? Check if we need to do a state transition.
		//
		if (ack == iSND.NXT)
			{
			if (InState(ETcpFinWait1))
				{
				EnterState(ETcpFinWait2);

				//
				// If the peer does not send a FIN for some reason,
				// we might hang in FIN-WAIT-2 indefinitely. We use
				// a 2*MSL timeout to break out of FIN-WAIT-2.
				//
				// Note that we can only do this if the application
				// has closed both directions of the connection.
				// However, this should take care of cleanup in the
				// the most common case where a server socket hangs
				// in FIN-WAIT-2, because the client has crashed or
				// disappeared before sending a FIN-ACK.
				//
				if (iSockFlags.iRecvClose)
					{
					LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Setting FIN-WAIT-2 timeout"), (TInt)this));
					SchedMsl2Wait();
					}
				}

			//
			// If we have a lingering Close() or Shutdown(ENormal), complete it with KErrNone.
			//
			if (iLinger > 0 && InState(ETcpFinWait2|ETcpClosing|ETcpLastAck))
				{
				/*
				In addition to the processing for the ESTABLISHED state, if
				the retransmission queue is empty, the user's CLOSE can be
				acknowledged ("ok") but do not delete the TCB.*/
				iLinger = -1;
				iLingerTimer->Cancel();
				Detach();
				}

			if (InState(ETcpClosing))
				{
				EnterState(ETcpTimeWait);
				}

			if (InState(ETcpLastAck))
				{
				if (!Protocol()->LocalTimeWait())
					{
					//
					// Local resource optimization. If the peer is on localhost,
					// we will terminate it here. Normally, it would wait for
					// a duration of 2*MSL in TIME-WAIT state before deleting
					// itself.
					//
					CFlowContext *flow = iFlow.FlowContext();
#ifndef SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
					CProviderTCP6 *sap =
						(CProviderTCP6*)Protocol()->LocateSap(EMatchConnection,
						KAFUnspec,
						flow->RemoteAddr(),
						flow->LocalAddr());
#else
					CProviderTCP6 *sap =
						(CProviderTCP6*)Protocol()->LocateSap(EMatchConnection,
						KAFUnspec,
						flow->RemoteAddr(),
						flow->LocalAddr(),
						NULL, info->iInterfaceIndex);
#endif //SYMBIAN_STRICT_EXPLICIT_SOCKET_BINDING
					if (sap != NULL && sap->InState(ETcpTimeWait))
						{
						LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Deleting local peer"), (TInt)this));
						sap->Expire();
						}
					}
				/*
				The only thing that can arrive in this state is an
				acknowledgment of our FIN.  If our FIN is now acknowledged,
				delete the TCB, enter the CLOSED state, and return.*/
				EnterState(ETcpClosed);
				goto wrapup;
				}
			}

		if (InState(ETcpTimeWait))
			{
	  /*
          The only thing that can arrive in this state is a
          retransmission of the remote FIN.  Acknowledge it, and restart
			the 2 MSL timeout.*/
			//
			// RFC793 appears to be wrong here. A retransmitted FIN should
			// already have been acknowledged as an out-of-sequence packet.
			// Also, a simultaneous close can cause both end points to send
			// an ack and enter the TIME-WAIT state at the same time. In this
			// rare case, the following line will cause an ack storm, where
			// each side is acknowledging ack packets from the other side.
			//
			//SendSegment(KTcpCtlACK);
			SchedMsl2Wait();
			}

	process_data:

		//
		//  DATA SEGMENT PROCESSING
		//
		if (InState(ETcpEstablished|ETcpFinWait1|ETcpFinWait2))
			{
			if (seg.iHdr->URG())
				{
				TTcpSeqNum up = seq + seg.iHdr->Urgent();
				/*
				If the URG bit is set, RCV.UP <- max(RCV.UP,SEG.UP), and signal
				the user that the remote side has urgent data if the urgent
				pointer (RCV.UP) is in advance of the data consumed.  If the
				user has already been signaled (or is still in the "urgent
				mode") for this continuous sequence of urgent data, do not
				signal the user again.*/
				if (up > seq)
					RememberUrgentPointer(up);
				}

			//
			//  Process data segments.
			//
			if (len > 0)
				{
				if (iSockFlags.iRecvClose)
					{
					// Receive direction has been shut down. Send RST.
					SendReset(ack);
					}
				else
					{
					//
					// Zero window? We will already have sent and ACK in response,
					// so we can just discard the segment.
					//
					if (!iRCV.WND)
						goto accept;

					TInt maxLen = iRCV.NXT + iRCV.WND - seq;
					if (len > maxLen)
						{
						//
						// Part of segment is above window -> truncate. This
						// can happen when peer is probing a zero window.
						//
						LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Above window payload: %u"), (TInt)this, len - maxLen));
						if (maxLen <= 0)  // Sanity check
							goto accept;

						len = maxLen;
						packet.TrimEnd(seg.iHdr->HeaderLength() + info->iOffset + len);
						fin = 0;    // Cannot process FIN yet.
						}

					// Remember to ack a pushed segment immediately
					if (seg.iHdr->PSH() && Protocol()->PushAck())
						immediateAck = ETrue;

					if (seq <= iRCV.NXT)
						{
						//
						// WARNING! This will destroy the segment header!
						//
						LOG(if(seq<iRCV.NXT)Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Below window payload: %d"), (TInt)this, iRCV.NXT - seq));

						// Remove segment header and below-window payload.
						packet.TrimStart(iRCV.NXT - seq + seg.iHdr->HeaderLength() + info->iOffset);

						// Put in receive queue.
						iSockInQ.Append(packet);
						len -= (iRCV.NXT - seq);
						iSockInQLen += len;
						iNewData += len;
						iRCV.NXT += len;
						iRCV.WND -= len;

						//
						// Check if we can take something out of the reassembly queue.
						//
						TTcpSeqNum fragOff;
						TBool fastAck = !iFragQ.IsEmpty();
						while (!iFragQ.IsEmpty())
							{
							fragOff = iFragQ.First().Offset();
							if (fragOff > iRCV.NXT)
								break;

							RMBufTcpFrag frag;
							iFragQ.Remove(frag);
							TUint32 fragLen = frag.FragmentLength();

							// Already got this?
							if (fragOff + fragLen <= iRCV.NXT)
								{
								frag.Free();
								continue;
								}

							// Ok. Trim it and put it in receive queue.
							TTcpPacket seg(frag);

							frag.TrimStart(seg.iHdr->HeaderLength() + (iRCV.NXT - fragOff));
							fragLen -= (iRCV.NXT - fragOff);

							ASSERT(fragLen == (TUint)frag.Length());

							iSockInQ.Append(frag);
							iSockInQLen += fragLen;
							iNewData += fragLen;

							ASSERT((TUint)iSockInQLen >= iNewData);

							LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Took %d:%d(%d) from reassembly queue"),
								(TInt)this, iRCV.NXT.Uint32(), (iRCV.NXT + fragLen).Uint32(), fragLen));

							iRCV.NXT += fragLen;
							iRCV.WND -= fragLen;

							}
						//
						// Update SACK book keeping
						//
						if (iFlags.iSackOk)
							iOptions.SackBlocks().Prune(iRCV.NXT);

						/*
                      	To provide feedback to senders recovering from losses, the receiver
                      	SHOULD send an immediate ACK when it receives a data segment that
						fills in all or part of a gap in the sequence space. */
						if (fastAck)
							SendSegment(KTcpCtlACK);
						}
					else
						{
						LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Processing out-of-order segment!"), (TInt)this));
						TTcpSeqNum blockSeq;
						TUint32 blockLen;

						// Fragment queue assumes packet starts from TCP header
						packet.TrimStart(info->iOffset);

						//packet.FreeInfo(); --  Leave the info. We need it below.
						RMBufTcpFrag frag;
						frag.Assign(packet);
						iFragQ.Add(frag, (TUint32*)&blockSeq, &blockLen);
						if (iFlags.iSackOk)
							{
							iOptions.SackBlocks().AddUnordered(blockSeq, blockSeq + blockLen);
							iOptions.SackBlocks().Limit(4);
							}

                      /*
                      Out-of-order data segments SHOULD be acknowledged immediately, in
                      order to accelerate loss recovery.  To trigger the fast retransmit
                      algorithm, the receiver SHOULD send an immediate duplicate ACK when
						it receives a data segment above a gap in the sequence space. */
						SendSegment(KTcpCtlACK);
						}
					}
				}
			}


		//
		//  FIN PROCESSING
		//

/*
      If the FIN bit is set, signal the user "connection closing" and
      return any pending RECEIVEs with same message, advance RCV.NXT
      over the FIN, and send an acknowledgment for the FIN.  Note that
      FIN implies PUSH for any segment text not yet delivered to the
      user.
		*/
		//
		// Remember that we have received a FIN. Note: the FIN may have
		// been received as part of an out-of-sequence segment, in which
		// case we may not be ready to process it yet.
		//
		if (fin && !iFlags.iFinReceived)
			{
			iFlags.iFinReceived = ETrue;
			iFinSeq = seq + len;
			}

		//
		// Process FIN when all data has been received.
		//
		if (iFlags.iFinReceived && iRCV.NXT == iFinSeq)
			{
			//
			// Advance iRCV.NXT past the FIN. This also ensures
			// that we never end up here again.
			//
			iRCV.NXT++;

			if (InState(ETcpFinWait1|ETcpFinWait2))
				{
				//LOG(Log::Printf(_L("IMMEDIATE FIN ACK\r\n")));
				SendSegment(KTcpCtlACK);
				}
			else
				{
				//LOG(Log::Printf(_L("DELAYED FIN ACK\r\n")));
				SendDelayACK();
				}

/*
        SYN-RECEIVED STATE
        ESTABLISHED STATE

          Enter the CLOSE-WAIT state.
			*/
			if (InState(ETcpSynReceived|ETcpEstablished))
				{
				EnterState(ETcpCloseWait);
				}

/*
        FIN-WAIT-1 STATE

          If our FIN has been ACKed (perhaps in this segment), then
          enter TIME-WAIT, start the time-wait timer, turn off the other
          timers; otherwise enter the CLOSING state.
			*/
			if (InState(ETcpFinWait1))
				{
	      /*
	      If our FIN has been ACKed (perhaps in this segment), then
	      enter TIME-WAIT, start the time-wait timer, turn off the other
				timers; otherwise enter the CLOSING state.*/
				if (iSND.UNA == iSND.NXT)
					{
					EnterState(ETcpTimeWait);
					SchedMsl2Wait();
					}
				else
					EnterState(ETcpClosing);
				}

/*
        FIN-WAIT-2 STATE

          Enter the TIME-WAIT state.  Start the time-wait timer, turn
          off the other timers.
			*/
			if (InState(ETcpFinWait2))
				{
	      /*
	      Enter the TIME-WAIT state.  Start the time-wait timer, turn
				off the other timers.*/
				Stop();
				EnterState(ETcpTimeWait);
				SchedMsl2Wait();
				}

			//
			// Anything that could arrive in TIME-WAIT is an out-of-window
			// segment and should never end up here.
			//
#ifdef notdef
/*
        TIME-WAIT STATE

          Remain in the TIME-WAIT state.  Restart the 2 MSL time-wait
          timeout.
			*/
			if (InState(ETcpTimeWait))
				{
	      /*
	      Remain in the TIME-WAIT state.  Restart the 2 MSL time-wait
				timeout.*/
				SchedMsl2Wait();
				}
#endif
			}

	wrapup:
		//
		// Time to die.
		//
		if (InState(ETcpClosed) && iRecvQ.IsEmpty())
			{
			packet.Free();
			Close();
			Expire();
			return;
			}

	accept:
			// Reset idle timers
		Protocol()->Interfacer()->PacketAccepted(info->iOriginalIndex);

	discard:
			packet.Free();
		}

	// Complete application level read.
	if (iFlags.iCompleteRecv)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Complete application read"), (TInt)this));
		iFlags.iCompleteRecv = EFalse;
		iCopyOutOffset = 0;
      if (iSockFlags.iNotify)
		iSocket->Error(KErrNone, MSocketNotify::EErrorRecv);
		}

	// This may cause an immediate ACK to be sent from within GetData()
	if (iNewData)
		{
		TInt newData = iNewData, up = 0;

		// Adjust for urgent data.
		if (iUpCount)
			{
			//
			// Find the offset of an urgent byte following a block of
			// non-urgent data. The number of junked (already delivered)
			// urgent bytes will be left in <up>. This looks complicated
			// but should normally be very quick.
			//
			newData = iSockInQLen;
			for (up = 0; up < iUpCount; up++)
				{
				TInt offset = UrgentOffset(up);
				if (offset > up)
					{
					newData = Min(offset, iSockInQLen);
					break;
					}
				}

			// We have an undelivered urgent byte within the block.
			if (iFlags.iUrgentMode && !iFlags.iOobInline && newData > UrgentOffset())
				newData = UrgentOffset();

			ASSERT(newData <= (TInt)iSockInQLen);

			// Subtract bytes that have already been advertised to ESock
			newData -= (iSockInQLen - iNewData);

			// Subtract out-of-band bytes
			if (!iFlags.iOobInline)
				newData -= up;
			}

		// Do we have something?
		if (newData > 0 && iFlags.iStarted && iSockFlags.iNotify)
			{
			ASSERT(newData + (iFlags.iOobInline ? 0 : up) <= (TInt)iNewData);
			iNewData -= newData;
			if (!iFlags.iOobInline)
				iNewData -= up;

			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): NewData(%d), %d urgent bytes %s, %d bytes not reported"),
				(TInt)this, newData, up, iFlags.iOobInline ? _S("inline") : _S("junked"), iNewData));
			iPending += newData;
			iSocket->NewData(newData);
			}
		}

	// Notify urgent data to application
	if (iFlags.iNotifyUrgent && iSockFlags.iNotify)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Urgent data notification"), (TInt)this));
		iFlags.iNotifyUrgent = EFalse;
		iSocket->Error(KErrUrgentData, 0);
		}

	// Notify application that connection has been closed by peer.
	if (InState(ETcpCloseWait|ETcpClosing|ETcpTimeWait) && !iFlags.iCloseNotified && !iNewData)
		{
		iFlags.iCloseNotified = ETrue;
		if (iFlags.iStarted && iSockFlags.iNotify && !iSockFlags.iRecvClose)
			{
			LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): Calling NewData(KNewDataEndOfData)"), (TInt)this));
			if(iSocket)
			iSocket->NewData(KNewDataEndofData);
			}
		}

	if (iLastAck < iRCV.NXT)
		{
		//
		// We are required to send an ACK for at least every two full sized
		// segments but at most once every full sized segment.
		//
		// In steady state the following rule will acknowledge every second
		// segment regardless of the options in the segments (unless maximum
		// segment size is VERY small). However, if we receive segments smaller
		// than MSS/2, we will send an ACK roughly once for every full segment
		// of received data. This works out pretty good. However, if the
		// received segments are significantly smaller, we will start to
		// experience stretch ACK problems. This might happen, for instance,
		// if peer is using PMTUD and the path is constrained. Currently, we
		// ignore the problem.
		//
		// We ack all pushed segments immediately. This improves performance
		// for interactive traffic, since partial segments typically have the
		// PSH bit set.
		//
		if (iLastAck + iRMSS < iRCV.NXT || immediateAck)
			SendSegment(KTcpCtlACK);
		else
			SendDelayACK();
		}

	// Wake up transmitter.
	SchedTransmit();

	//
	// Wake up application
	//
	// Note: We do this even if send window is zero, because
	// new data from application will trigger zero window probing.
	//
	// Note 2: Do not wake up app, if lingering close is pending.
	//
	if (iSockFlags.iFlowStopped)
		{
		LOG(Log::Printf(_L("\ttcp SAP[%u] ProcessSegments(): FLOW STARTED"), (TInt)this));
		iSockFlags.iFlowStopped = EFalse;
		if (iSocket && iSockFlags.iNotify && (iLinger == -1 || !iSockFlags.iSendClose))
			iSocket->CanSend();
		}
	}


void CProviderTCP6::SpuriousTimeout(TUint aAcked)
/**
Spurious timeout occurred.

Sets the congestion control parameters depending on the ini setting "tcp_spurious_rto_recovery".

@param aAcked	Number of bytes acknowledged with the ACK that triggered this method.
*/
	{
	// TODO: if ACK has ECN-Echo flag, congestion control should not be reverted
	LOG(Log::Printf(_L("\ttcp SAP[%u] SpuriousTimeout(%u) ENTER: cwnd: %u  ssthresh: %u"), 
			(TInt)this, aAcked, iCwnd, iSsthresh));

	switch(Protocol()->SpuriousRtoResponse())
		{
	case 1:
	default:
		// Eifel response (draft uses initial window as burst limit, we use MaxBurst)
		// Assume that ssthresh was set to FlightSize / 2 when RTO occurred.
		iSsthresh = iSsthresh << 1;
		iCwnd = iSND.NXT - iSND.UNA + Min(aAcked, Protocol()->MaxBurst());
		break;
		
	case 2:
		// Half sending rate (ssthresh has been adjusted when RTO occurred)
		iCwnd = iSsthresh;
		break;
		
	case 3:
		// DCLOR - like behaviour (from earlier draft versions)
		iSsthresh = iSsthresh << 1;
		iCwnd = 1;
		}
	LOG(Log::Printf(_L("\ttcp SAP[%u] SpuriousTimeout() EXIT: cwnd: %u  ssthresh: %u"), 
			(TInt)this, iCwnd, iSsthresh));
	// TODO: Do we want to do the RTO adjustment required in Eifel Response draft? Maybe not. -PS
	}


//
// Initiate a connection by sending the first SYN.
//
// Note: The flow MUST be ready when calling this method.
//
void CProviderTCP6::SendSYN()
	{
	ASSERT(iState == ETcpConnect);
	ASSERT(iFlow.FlowContext() != 0);

	// Initialise send sequence number
	iTransmitSeq = iSND.NXT = Protocol()->RandomSequence();

	if (Protocol()->WinScale() != -1)
		{
		iRcvWscale = (Protocol()->WinScale() > 0 ? Protocol()->WinScale() - 1 : NeedWindowScale());
		iOptions.SetWindowScale((TUint8)(iRcvWscale + 1));
		}

	iRMSS = Min(LinkRMSS(), iSockInBufSize / 2);
	iOptions.SetMSS(iRMSS);
	iFreeWindow = Min(iSockInBufSize, 0xffff) % iRMSS;
	iRCV.WND = Min(iSockInBufSize, 0xffff) - iFreeWindow;
	iSND.UNA = iSND.NXT;

	TUint8 flags = KTcpCtlSYN;
	// Start ECN negotiation by setting both ECN bits on in a SYN packet [RFC 3168].
	if (iFlags.iEcn)
		{
		flags |= KTcpCtlECE | KTcpCtlCWR;
		}

	SendSegment(flags);
	iTransmitSeq++;
	EnterState(ETcpSynSent);

	// Disable on-demand interface setup for this flow
	iFlow.FlowContext()->iInfo.iNoInterfaceUp = 1;

	// Lock source address for this flow. It needs to persist over link change events.
	iFlow.FlowContext()->iInfo.iLocalSet = 1;
	}


TInt CProviderTCP6::CreateChild(CProviderTCP6*& aSAP)
	{
	ASSERT(iConnectCount < iListenQueueSize);
	ASSERT(iListenQueue);

	// Create a new SAP and initialize it
	TRAPD(err, aSAP = (CProviderTCP6*)Protocol()->NewSAPL(KSockStream));
	if (err == KErrNone) //lint -save -esym(613, aSAP) Possible NULL trapped here
		{
		aSAP->iParent = this;
		aSAP->iSockFamily = iSockFamily;
		aSAP->iAppFamily = iAppFamily;

		// The new SAP is not yet known to socket server
		aSAP->iSockFlags.iAttached  = EFalse;
		aSAP->iSockFlags.iNotify    = EFalse;
		aSAP->iHasNetworkServices = HasNetworkServices();
		// Copy TCP options to the new socket.
		aSAP->iSockInBufSize        = iSockInBufSize;
		aSAP->iSockOutBufSize       = iSockOutBufSize;
		aSAP->iMSS                  = iMSS;
		aSAP->iSockFlags.iReuse     = iSockFlags.iReuse;
		aSAP->iFlags.iOobInline     = iFlags.iOobInline;
		aSAP->iFlags.iNoDelay       = iFlags.iNoDelay;
		aSAP->iFlags.iEcn		  = iFlags.iEcn;

		// Clone the flow. Lower layer socket options get copied to the new flow.
		aSAP->iFlow.Clone(iFlow);

		// Clone currently resets the notifier, so we reinstall it here
		aSAP->iFlow.SetNotify(aSAP);

		// If the listen queue is full, kill a random connection.
		if (iConnectCount == iListenQueueSize)
			{
			TInt i = Protocol()->Random(iListenQueueSize);
			CProviderTCP6 *sap = iListenQueue[i];
			sap->iParent = 0;
			iListenQueue[i] = iListenQueue[--iConnectCount];
			iListenQueue[iConnectCount] = 0;
			sap->Expire();
			}

		// Add the new connection into listen queue.
		iListenQueue[iConnectCount++] = aSAP;
		} //lint -restore
	return err;
	}


void CProviderTCP6::DetachChild(CProviderTCP6* aSAP)
	{
	ASSERT(iConnectCount > 0);
	ASSERT(iListenQueue);

	// Remove child from listen queue
	for (TUint i = 0; i < iConnectCount; i++)
		if (iListenQueue[i] == aSAP)
		{
		aSAP->iParent = 0;
		iListenQueue[i] = iListenQueue[--iConnectCount];
		iListenQueue[iConnectCount] = 0;
		return;
		}

#ifdef _DEBUG
	User::Panic(_L("CProviderTCP6::DetachChild()"), 0);
#endif
	}

TInt CProviderTCP6::CompleteChildConnect(CProviderTCP6* aSAP)
	{
	ASSERT(aSAP);
	ASSERT(iSocket);
	iChildDeleted = EFalse;
	iSocket->ConnectComplete(*aSAP);
	// In some cases SocketServer might have destroyed SAP.
	// If this happens, return error for deleted SAP.
	return iChildDeleted ? KErrAbort : KErrNone;
	}

inline void CProviderTCP6::SetChildDeleted(TBool aDeleted)
	{
	iChildDeleted = aDeleted;
	}


//
// Clear all set option bits for SYNs
//
void CProviderTCP6::ClearSYNSettings()
	{
	// Remove SYN options from active TCP options.
	iOptions.SetWindowScale(0);
	iOptions.ClearMSS();
	iOptions.ClearSackOk();
	if (!iFlags.iUseTimeStamps)
		iOptions.ClearTimeStamps();	
	}

TUint8 CProviderTCP6::NeedWindowScale()
	{
	TUint bufbits = 0;
#ifdef SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
	//Window scaling factor needs to be negotiated if the iMaxRecvWin
	//is more than the 64K, irrespective of the socket receive buffer
	//size. This will help in achieving the maximum throughput in case
	//modulation changes
	if(iTcpMaxRecvWin > iSockInBufSize )
		{
		bufbits = iTcpMaxRecvWin >> 16;
		}
	else
#endif //SYMBIAN_ADAPTIVE_TCP_RECEIVE_WINDOW
		{
		bufbits = iSockInBufSize >> 16;
		}
	TUint8 scale = 0;

	while (bufbits)
		{
		scale++;
		bufbits >>= 1;
		}

	return scale;
	}

TInt CProviderTCP6::CheckPolicy(const TSecurityPolicy& aPolicy, const char *aDiagnostic)
	{
	return iParent ? iParent->CheckPolicy(aPolicy, aDiagnostic) : CProviderInet6Transport::CheckPolicy(aPolicy, aDiagnostic);
	}
//
// TCP reassembly queue implementation
//
TUint RMBufTcpFrag::Offset()
	{
	//LOG(Log::Printf(_L("RMBufTcpFrag::FragOffset()\r\n")));

	TTcpPacket seg(*this);
	return seg.iHdr->Sequence().Uint32();
	}

TUint RMBufTcpFrag::FragmentLength()
	{
	//LOG(Log::Printf(_L("RMBufTcpFrag::FragLength(): length = %d\r\n"), Length()));

	TTcpPacket seg(*this);
	return Length() - seg.iHdr->HeaderLength();
	}

void RMBufTcpFrag::Join(RMBufChain& aSeg)
	{
	//LOG(Log::Printf(_L("RMBufTcpFrag::Join()\r\n")));

	TTcpPacket thisSeg(*this), newSeg(aSeg);

	// Remove header and overlapping data from aSeg.
	aSeg.TrimStart(newSeg.iHdr->HeaderLength() +
		(Length() - thisSeg.iHdr->HeaderLength()) -
		(newSeg.iHdr->Sequence() - thisSeg.iHdr->Sequence()));

	// Append.
	Append(aSeg);
	}

#ifdef __ARMCC__
#pragma pop
#endif
