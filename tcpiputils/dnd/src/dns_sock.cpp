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
// dns_sock.cpp - the main client module of the DNS.
//

#include <e32math.h>
#include <networking/dnd_err.h>
#include <in_sock.h>
#include "dns_sock.h"
#include "message.h"
#include "inet6log.h"
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY 
#include <es_enum.h>
#endif


/**
* Max number of consecutive RunL() calls with error condition.
* When this limit is exceeded, the socket it closed.
*/
const TUint KSocketErrorLimit = 20;

//seeds cannot be negative number. Need mask to seed to positive number.
const TInt KNegativeNumberMask = -1;

//
// TRequestQueue
// *************
//
class TRequestQueue : public TDblQue<TDnsRequest>
	{
public:
	// Construct an empty request queue
	TRequestQueue();
	// Construct new request queue and MOVE all requests from another into it
	TRequestQueue(TRequestQueue &);
	// Remove the first request from a queue, return NULL if queue is empty
	TDnsRequest *Remove();
	};


TRequestQueue::TRequestQueue()
	: TDblQue<TDnsRequest>(_FOFF(TDnsRequest, iQueueLink))
	{
	}

TRequestQueue::TRequestQueue(TRequestQueue &aQueue)
	: TDblQue<TDnsRequest>(_FOFF(TDnsRequest, iQueueLink))
	{
	if (aQueue.IsEmpty())
		Reset();
	else
		{
		iHead = aQueue.iHead;
		iHead.iPrev->iNext = &iHead;
		iHead.iNext->iPrev = &iHead;
		}
	aQueue.Reset();
	}

TDnsRequest *TRequestQueue::Remove()
	{
	if (!IsEmpty())
		{
		TDnsRequest *rq = First();
		rq->iQueueLink.Deque();
		return rq;
		}
	return NULL;
	}

class CDnsSocketReader;

// CDnsSocketWriter
// ****************
// Internal "hidden" class to do all the work
class CDnsSocketWriter : public CActive
    {
	CDnsSocketWriter(CDnsSocket &aMaster);
public:
	~CDnsSocketWriter();
	void ConstructL();

	// Create a basic (UDP) reader/writer (not activated)
	static CDnsSocketWriter *NewL(CDnsSocket &aMaster, TInt aTTL);
	// Create a TCP writer/reader (not activated)
	static CDnsSocketWriter *NewTcpL(CDnsSocket &aMaster, const TInetAddr &aServer, TInt aTTL);
	// Create a TCP listening writer/reader (not activated)
	static CDnsSocketWriter *NewTcpListenL(CDnsSocket &aMaster, TInt aTTL);

	// Test if a address matches current connection
	TBool Match(const TInetAddr &aServer) const;

	// (Re)Queue the request for processing with new ID
	void Queue(TDnsRequest &aRequest, const TInt aId = -1);
	// Cancel the request and dequeue it
	void Remove(TDnsRequest &aRequest);
	// Change the bind port (and address if specified)
	void SetBind(const TInetAddr &aBind);
	// Set hoplimit for for transmitted packets.
	void SetHoplimit(const TInt aTTL);
	 // Open and activate the UDP socket for DNS traffic (unless already done)
	TInt ActivateSocket();
	// Process socket errors
	TInt HandleError(TInt aReason, const TInetAddr *aServer = NULL);
	// Close and deactivate the UDP socket for DNS traffic
	TBool DeactivateSocket(TInt aReason); 
	// Handle receive compeleted, keep receiver active
	void RunReader(const TMsgBuf &aMsg, const TInetAddr &aFrom);
	// Handle send completed, keep sender active (if work to do)
	void RunWriter(TRequestStatus &aStatus);
 	//seed id generator
 	TInt64 SeedIdGenerator(TInt64 aSequence);
	// Return a reference to the RSocket
	inline RSocket &Socket() { return iSocket; }
	inline TBool IsTCP() const { return iTCP != 0; }
	inline TBool IsListen() const { return iListen != 0; }
	inline TBool IsOpened() const { return iOpened; }
	inline void ResetErrorCount() { iErrorCount = 0;}
	// A link chain used by the CDnsSocket to keep track of multiple writers
	CDnsSocketWriter *iNext;
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY 
		RConnection iAttachedConn;	//< The connection on which the DNS socket may be opened on
#endif	

	// Network ID associated with the writer;
	TInt   iNetworkIdofWriter;
	inline void SetDeferredDelete() { iDeferredDelete = ETrue; }
	
private:
	void RunL();
	void DoCancel();
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY	
	TBool CanUseConnection();
#endif

	CDnsSocket &iMaster;		//< The connection to the CDnsSocket owning this
	RSocket iSocket;			//< The DNS socket
	
	TUint iDeactivateCount;		//< Count number of deactivations (needed in reply processing)
	TUint iErrorCount;			//< Count number of consecutive errors in RunL (read and write)
	TUint iListen;				//< = 1, if socket is listening TCP socket, waiting on accept.
	TUint iTCP;					//< = 1, if socket is a connected TCP socket
	TUint iOpened:1;			//< = 1, if the socket is open
	TUint iRunWriter:1;			//< = 1, if executing RunWriter (callback safeguard)
	TUint iRunReader:1;			//< = 1, if executing RunReader (callback safeguard)	
	TInt iTTL;					//< The TTL/Hoplimit to be used
	TInt64 iSequence;			//< Used in computing the ID value for the queries
	TBuf8<KDnsMaxMessage> iOutMsg;	//< For sending the outgoing message
	TInetAddr iBind;			//< Address/Port to bind the socket.
	TInetAddr iTo;				//< Filled with the destination address of the query
	TRequestQueue iSendQueue;	//< Requests waiting to be sent
	TRequestQueue iWaitQueue;	//< Requests waiting for a reply
	TDnsRequest *iSending;		//< The request currently being sent, if non-NULL
	CDnsSocketReader *iReader;	//< The reader object
	TBool iDeferredDelete;
	};

void TDnsRequest::Cancel()
	{
	CDnsSocketWriter *const writer = iQueueLink.Writer();
	if (writer)
		writer->Remove(*this);
	}


// CDnsSocketReader
// ****************
// Internal "hidden" class for hanlding asynchronous reading
class CDnsSocketReader : public CActive
	{
	friend class CDnsSocketWriter;
public:
	CDnsSocketReader(CDnsSocketWriter &aWriter);
	~CDnsSocketReader();
	void ConstructL(TUint aDnsMaxMessage);
	void Activate();

private:
	void RunL();
	void DoCancel();

	CDnsSocketWriter &iWriter;
	TUint iReadLength:1;
	TInetAddr iFrom;		//< Filled with source address of a received packet
	HBufC8 *iInMsg;			//< The real allocated buffer
	TInt iAllocatedLength;	//< The real max length of the buffer
	TPtr8 iBuf;				//< The current receive buffer
	};

CDnsSocketWriter::CDnsSocketWriter(CDnsSocket &aMaster) : CActive(0), iMaster(aMaster), iDeferredDelete(EFalse)
	{
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CDnsSocketWriter([%u])"), this, &aMaster));
	iNetworkIdofWriter = -1;
	CActiveScheduler::Add(this);

	TTime seed;
	seed.UniversalTime();
	iSequence = seed.Int64();
	}

void CDnsSocketWriter::ConstructL()
	{
	iReader = new (ELeave) CDnsSocketReader(*this);
	iReader->ConstructL(iMaster.iMaxDnsMessage);
	}


CDnsSocketWriter *CDnsSocketWriter::NewL(CDnsSocket &aMaster, TInt aTTL)
	{
	CDnsSocketWriter *const writer = new (ELeave) CDnsSocketWriter(aMaster);
	CleanupStack::PushL(writer);
	writer->iTTL = aTTL;
	writer->ConstructL();
	CleanupStack::Pop();
	return writer;
	}

CDnsSocketWriter *CDnsSocketWriter::NewTcpL(CDnsSocket &aMaster, const TInetAddr &aServer, TInt aTTL)
	{
	CDnsSocketWriter *const writer = NewL(aMaster, aTTL);
	// Note: with TCP iTo is dummy!
	writer->iTo = aServer;
	writer->iTCP = 1;
	return writer;
	}

CDnsSocketWriter *CDnsSocketWriter::NewTcpListenL(CDnsSocket &aMaster, TInt aTTL)
	{
	CDnsSocketWriter *const writer = NewL(aMaster, aTTL);
	writer->iTCP = 1;
	writer->iListen = 1;
	return writer;
	}



CDnsSocketWriter::~CDnsSocketWriter()
	{
	// As this class is managed by CDnsSocket, it is impossible
	// to get here if IsActive(), or with requests in any queue.
	// (DeactivateSocket is ALWAYS called before destructor)
	//
	ASSERT(!IsActive());
	ASSERT(iSendQueue.IsEmpty());
	ASSERT(iWaitQueue.IsEmpty());

	delete iReader;

	Cancel();	// should not be needed...
	if (IsAdded())
		Deque();
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY		
	iAttachedConn.Close();
#endif	
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::~CDnsSocketWriter()"), this));
	}

TBool CDnsSocketWriter::Match(const TInetAddr &aServer) const
	{
	// The match applies only to connected TCP readers/writers
	return iTCP && !iListen && aServer.CmpAddr(iTo) && aServer.Scope() == iTo.Scope();
	}

//
// RunReader
//
void CDnsSocketWriter::RunReader(const TMsgBuf &aMsg, const TInetAddr &aFrom)
	{
	if (iRunReader)
		return;
	iRunReader = 1;
#ifdef _LOG
		{
		TBuf<50> tmp;
		aFrom.OutputWithScope(tmp);
		Log::Printf(_L("CDnsSocketWriter[%u]::RunReader() read %d bytes from %S#%d"), this, (TInt)aMsg.Length(),
			&tmp, aFrom.Port());
		}
#endif
	if (aMsg.Length() >= TInt(sizeof(TDndHeader)))
		{
		const TDndHeader &hdr = aMsg.Header();
		const TUint16 id = (TUint16)hdr.ID();

		if (hdr.QR())
			{
			//
			// This is a reply message, match it with
			// a waiting request, if any...
			//
			// Things get somewhat tricky, because almost anything
			// can happen inside the Reply (like DeactivateSocket,
			// ActivateSocketL, removal of any requests, requeing
			// for sent, etc...
			// Grab the current list into separate list. Note, that
			// Query or Reply callbacks are allowed to remove entries
			// from this temporary queue (the TDnsRequest::Cancel and
			// CDnsSocket::Redmove() still work correctly--they don't
			// care which chain the request belongs!),
			TRequestQueue reply(iWaitQueue);
			TUint mark = iDeactivateCount;
			TDnsRequest *rq;
			for (;;)
				{
				rq = reply.Remove();
				if (rq == NULL)
					{
					// Didn't match any request, punt it into Query()
					iMaster.Query(aMsg, aFrom, iSocket);
					break;
					}
				iWaitQueue.AddLast(*rq);
				if (rq->iId == id && rq->Reply(iMaster, aMsg, aFrom))
					break;
				}
			if (iDeactivateCount == mark)
				{
				//
				// No socket shutdown occurred, just reinsert remaining requests
				//
				while ((rq = reply.Remove()) != NULL)
					iWaitQueue.AddLast(*rq);
				}
			else
				{
				//
				// Socket was shut within Reply, all requests
				// should have been cancelled then...
				//
				while ((rq = reply.Remove()) != NULL)
					{
					rq->iQueueLink.SetWriter(NULL);
					rq->Abort(iMaster, KErrCancel);
					}
        if (iDeferredDelete)
          {
          LOG(Log::Printf(_L("CDnsSocketWriter[%u]::RunReader() deleting itself due to deferred delete"), this));
          delete this;
          return;
          }
				}
			}
		else
			{
			//
			// Not a reply message, let the derived
			// class decide on how to deal with it.
			//
			iMaster.Query(aMsg, aFrom, iSocket);
			}
		}

	iRunReader = 0;
 	if (iOpened ) 	// still open for business?
		{
		// Start a new read
		iReader->Activate();
		}
	}

void CDnsSocketWriter::Queue(TDnsRequest &aRequest, TInt aId)
	{
	aRequest.Cancel();
	//
	// Assign a new random ID for the request
	//
	if (aId < 0)
 		{
 		TInt64 seed = SeedIdGenerator(iSequence);
 		if(seed<0)
 		seed = seed *(KNegativeNumberMask);
 		aRequest.iId = (TUint16)(Math::Rand(seed) >> 8);
 		}
	else
		aRequest.iId = (TUint16)aId;
	aRequest.iQueueLink.SetWriter(this);

	iSendQueue.AddLast(aRequest);
	LOG(Log::Printf(_L("\t\tDNS session [%u] Queued for send with ID=%d"), (TInt)&aRequest, (TInt)aRequest.iId));
	if (!IsActive())
		RunWriter(iStatus);
	}


void CDnsSocketWriter::SetBind(const TInetAddr &aBind)
	{
	iBind = aBind;
	}

void CDnsSocketWriter::SetHoplimit(const TInt aTTL)
	{
	if (aTTL != iTTL)
		{
		iTTL = aTTL;
		iSocket.SetOpt(KSoIp6UnicastHops, KSolInetIp, iTTL);
		iSocket.SetOpt(KSoIp6MulticastHops, KSolInetIp, iTTL);
		}
	}

TInt CDnsSocketWriter::ActivateSocket()
	{
 	if (iOpened) 
		return KErrNone;		// Nothing to do if already opened

	iErrorCount = 0;
#ifndef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY 	
	TInt err = 
		iListen ? iSocket.Open(iMaster.iSS) :
		iTCP ? iSocket.Open(iMaster.iSS, KAfInet, KSockStream, KProtocolInetTcp) :
		iSocket.Open(iMaster.iSS, KAfInet, KSockDatagram, KProtocolInetUdp);
#else			
	TInt err = KErrNone;
	if (iListen)
		{
		err = iSocket.Open(iMaster.iSS);	
		}
	else
		{		
		TUint addrFamily = iTCP ? KSockStream : KSockDatagram;
		TUint protocol = iTCP ? KProtocolInetTcp : KProtocolInetUdp;
		if (CanUseConnection())
			{
			err = iSocket.Open(iMaster.iSS, KAfInet, addrFamily, protocol, iAttachedConn);
			}
		else
			{
			err = iSocket.Open(iMaster.iSS, KAfInet, addrFamily, protocol);	
			}			
		}
#endif			
#ifdef _LOG
	// Initialize for LOG prints
	TBuf<50> tmp;
	iBind.OutputWithScope(tmp);
#endif
	if (err == KErrNone)
		{
		if (iListen || (err = iSocket.Bind(iBind)) == KErrNone)
			{
			iOpened = 1;
			if (iListen)
				{
				LOG(Log::Printf(_L("CDnsSocketWriter[%u]::ActivateSocketL() listening %S#%d"), this, &tmp, iBind.Port()));
				iMaster.iListener.Accept(iSocket, iStatus);
				SetActive();
				}
			else
				{
				iSocket.SetOpt(KSoIp6UnicastHops, KSolInetIp, iTTL);
				iSocket.SetOpt(KSoIp6MulticastHops, KSolInetIp, iTTL);
				if (iTCP)
					{
#ifdef _LOG
					TBuf<50> dst;
					iTo.OutputWithScope(dst);
					Log::Printf(_L("CDnsSocketWriter[%u]::ActivateSocketL() TCP connect src=%S#%d dst=%S#%d"), this,
						&tmp, iBind.Port(),
						&dst, iTo.Port());
#endif
					iSocket.Connect(iTo, iStatus);
					SetActive();
					}
				else
					{
					LOG(Log::Printf(_L("CDnsSocketWriter[%u]::ActivateSocketL() UDP %S#%d"), this, &tmp, iBind.Port()));
					iSocket.SetOpt(KSoUdpReceiveICMPError, KSolInetUdp, 1);
					if (!iRunReader)
						iReader->Activate();
					}
				}
			return KErrNone;
			}
		iSocket.Close();
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY		
		iAttachedConn.Close();
#endif		
		}
	// Should probably be logged in release too!
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::ActivateSocketL() bind=%S#%d (or open) failed %d"), this, &tmp, iBind.Port(), err));
	return err;
	}


/**
* Decides on action when the socket returns an error status. Not all
* errors are fatal errors, which require a reset of the socket.
*
* @param aReason	the error code from the socket (iStatus)
* @param aServer	the dns server address, if known (only for Send errors)
*
* @return
* @li	= 1, Error fully processed, Don't activate anything
* @li	= 0, Error handled, keep CActive running
*/
TInt CDnsSocketWriter::HandleError(TInt aReason, const TInetAddr *aServer)
	{
	if (++iErrorCount > KSocketErrorLimit)
		{
		// Too many consecutive errors, deactivate to reset
		LOG(Log::Printf(_L("CDnsSocketWriter[%u]::HandleError(%d) Too many (%d) consecutive errors"), this, aReason, iErrorCount));
		DeactivateSocket(aReason);
		return 1;
		}
	if (IsTCP())
		{
		// Any error on TCP socket means that it is "dead",
		// and we must close the socket to recover!
		DeactivateSocket(KErrDndServerUnusable);
		return 1;
		}
	// The error state should automaticly clear with datagram
	// sockets, but in some releases it does not. Issue a
	// dummy KSOSelectLastError, which in some cases also
	// clears the error code from socket server. (Note: the
	// option parameter is dummy, the error is actually
	// returned as a return value of GetOpt, very bizarre
	// -- msa)
	TInt socket_err = iSocket.GetOpt(KSOSelectLastError, KSOLSocket, socket_err);
	(void)socket_err;	// shut down compiler warning about unreference variable.

	if (aServer == NULL)
		{
		// The only errors on receive side, that we can do something about, are
		// the ICMP errors for some server (other errors are ignored).
		// Try to find out the server.
		TPckgBuf<TSoInetLastErr> opt;
		if (iSocket.GetOpt(KSoInetLastError, KSolInetIp, opt) == KErrNone)
			{
#ifdef _LOG
			TBuf<50> src;
			TBuf<50> dst;
			TBuf<50> err;
			opt().iSrcAddr.OutputWithScope(src);
			opt().iDstAddr.OutputWithScope(dst);
			opt().iErrAddr.OutputWithScope(err);
			Log::Printf(_L("CDnsSocketWriter[%u]::HandleError(%d) last=%d, icmp(%d,%d) src=%S dst=%S err=%S"),
				this, aReason, opt().iStatus, opt().iErrType, opt().iErrCode, &src, &dst, &err);
#endif
			if (opt().iStatus != aReason)
				return 0;		// Error does not match the ICMP last error, just ignore.

			aServer = &opt().iDstAddr;
			}
		else
			{
			DeactivateSocket(KErrDndServerUnusable);
			return 1;
			}			
		}

	// aServer possibly identifies a DNS server with which
	// we have some communication problems. Declare this
	// server unusable for all queries currently using it.
	const TUint mark = iDeactivateCount;
	iMaster.Error(*aServer, aReason);
	// If shutdown occurred within above call, then
	// this return must not try to reactivate the
	// reader or writer.
	return (mark != iDeactivateCount);
	}

TBool CDnsSocketWriter::DeactivateSocket(TInt aReason)
	{
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() Entry"), this));
	if (!iOpened )
	    {
	    LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() return without action"), this));
		return ETrue;			// Nothing to do if not open
	    }
	// Grab current set of requests away, so that
	// potentially newly entered requests won't
	// get affected by this...
	//
	TRequestQueue send(iSendQueue);
	TRequestQueue wait(iWaitQueue);
	//
	// Cancel all activity
	//
//	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() iReader->Cancel()"), this));
	iReader->Cancel();
//	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() Cancel()"), this));
	Cancel();
//	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() iSocket.Close()"), this));
	//
	// Shut down the sessions
	//
	iOpened = 0;
	iSocket.Close();
	iDeactivateCount += 1;
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY	
	iAttachedConn.Close();
#endif	
	//
	// Remove and abort all requests
	//
	TDnsRequest *rq;
	while ((rq = send.Remove()) != NULL)
		{
		rq->iQueueLink.SetWriter(0);
		rq->Abort(iMaster, aReason);
		}
	while ((rq = wait.Remove()) != NULL)
		{
		rq->iQueueLink.SetWriter(0);
		rq->Abort(iMaster, aReason);
		}
    if (iRunReader)
	  {
      LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() complete and defer delete"), this));
      return EFalse;
	  }
    LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DeactivateSocket() complete and allow delete"), this));
    return ETrue;
	}

//
// CDnsSocketWriter::Remove
// ************************
void CDnsSocketWriter::Remove(TDnsRequest &aQuery)
	{
	//
	// Must not be called for request that is not queued
	//
	ASSERT(aQuery.iQueueLink.Writer() == this);
	aQuery.iQueueLink.Deque();
	aQuery.iQueueLink.SetWriter(0);

	if (iSending == &aQuery)
		{
		iSending = NULL;
		// If IsActive() == FALSE, then this Remove is actually
		// called from the Build() callback!
		if (IsActive())
			{
			Cancel();
			// There can be other queries waiting
			// for writer..
			RunWriter(iStatus);
			}
		}
	}

void CDnsSocketWriter::RunL()
	{
	const TInt result = iStatus.Int();
	LOG(Log::Printf(_L("--> CDnsSocketWriter[%u]::RunL() -start- iStatus=%d"), this, result));
#ifdef _LOG
	TBuf<50> tmp;
	iTo.OutputWithScope(tmp);
#endif
	for (;;)	// ...just for easy exits..
		{
		if (result)
			{
			LOG(Log::Printf(_L("CDnsSocketWriter[%u]::RunL() Error on %S#%d"), this, &tmp, iTo.Port()));
			}
		else
			ResetErrorCount();
		if (iListen)
			{
			//
			// Keep listening active, allocate a new reader/writer to listen
			//
			CDnsSocketWriter *writer = NULL;
			TRAPD(err,
				writer = CDnsSocketWriter::NewTcpListenL(iMaster, iTTL);
				err = iMaster.AddSecondaryWriter(writer));
			if (err != KErrNone)
				{
				// Cannot start new accept for some reason, reject the current accept and
				// reinitialize the current writer to wait a new accept instead
				iMaster.DeleteWriter(writer, err);	// <-- safe to call with writer == NULL
				// Reactivate this socket for listening instead (iListen remains set)
				LOG(Log::Printf(_L("CDnsSocketWriter[%u]::RunL() Failed (%d) to activate new Accept() to=%S#%d"),
					this, err, &tmp, iTo.Port()));
				DeactivateSocket(err);
				ActivateSocket();
				break;
				}
			// Succesful Accept, update iTo to the address and port of the
			// remote end.
			iListen = 0;
			iSocket.RemoteName(iTo);
			LOG(iTo.OutputWithScope(tmp));
			LOG(Log::Printf(_L("CDnsSocketWriter[%u]::RunL() Listen accepted to=%S#%d"), this, &tmp, iTo.Port()));
			if (!iReader->IsActive())
				iReader->Activate();
			}
		if (iSending)
			{
			LOG(Log::Printf(_L("\t\tDNS session [%u] Sent to=%S#%d ID=%d"), iSending, &tmp, iTo.Port(), (TInt)iSending->Id()));
			TDnsRequest *query = iSending;
			iSending = NULL;
			if (result == KErrNone)
				query->Sent(iMaster);
			else if (HandleError(result, &iTo))
				break;
			}
		else if (result && HandleError(result))	// Error that cannot be associated with specific sending (does it ever happen?)
			break;
		// Keep writer busy, if not already active.
		if (!IsActive())
			RunWriter(iStatus);
		break;	// ** NOT REAL LOOP, ALWAYS EXIT AT END **
		}
	LOG(Log::Printf(_L("<-- CDnsSocketWriter[%u]::RunL() -exit- iStatus=%d"), this, iStatus.Int()));
	}

void CDnsSocketWriter::RunWriter(TRequestStatus &aStatus)
	{
	if (iRunWriter)
		return;		// This is a callback from Build, return
	iRunWriter = 1;
	//
	// Pick next query to be served
	// ****************************
	//
	iSending = iSendQueue.Remove();
	while ( iSending != NULL)
		{
		iWaitQueue.AddLast(*iSending);
		// Before calling Build, the message is reset into
		// initial state:
		TPtr8 payload((TUint8 *)iOutMsg.Ptr(), iOutMsg.MaxLength());
		if (iTCP)
			payload.Set((TUint8 *)payload.Ptr()+2, 0, payload.MaxLength()-2);
		TMsgBuf &buf = TMsgBuf::Cast(payload);
		buf.SetLength(sizeof(TDndHeader));
		buf.Header().Init(iSending->iId);
		// If Build fails, request(s) are simply moved
		// into iWaitQueue to wait for further actions.
		// (If iSending becomes NULL in Build, then Build has Removed the
		// request--do not start write)
		// Note: if receive buffer > KDnsMaxMessage, then it requests EDNS0. Don't
		// request EDNS0, if TCP is being used (report receive buffer = 0).
		if (iSending->Build(iMaster, buf, iTo, iTCP ? 0 : iReader->iAllocatedLength) && iOpened && iSending)
			{
			const TInt len = buf.Length();
			if (iTCP)
				{
				// Patch in the length
				TUint8 *const p = (TUint8 *)iOutMsg.Ptr();
				p[1] = (TUint8)len;
				p[0] = (TUint8)(len >> 8);
				iOutMsg.SetLength(len+2);
				}
			else
				{
				iOutMsg.SetLength(len);
				}
#ifdef _LOG
			TBuf<50> tmp;
			iTo.OutputWithScope(tmp);
			Log::Printf(_L("\t\tDNS session [%u] Send to=%S#%d %d bytes (TCP=%d)"), iSending, &tmp, iTo.Port(), iOutMsg.Length(), (TInt)iTCP);
#endif
			if (iTCP)
				{
				iSocket.Write(iOutMsg, aStatus);
				if (!iReader->IsActive())
					iReader->Activate();
				}
			else
				iSocket.SendTo(iOutMsg, iTo, 0, aStatus);
			SetActive();
			break;
			}
		}
	iRunWriter = 0;
	}

void CDnsSocketWriter::DoCancel()
	{
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::DoCancel()"), this));
	if (iListen)
		{
		iMaster.iListener.CancelAll();
		iListen = 0;
		}
	else
		{
		iSocket.CancelWrite();
		iSending = NULL;
		}
	}
	
#ifdef SYMBIAN_NON_SEAMLESS_NETWORK_BEARER_MOBILITY
TBool CDnsSocketWriter::CanUseConnection()
	{	
	TInt err = iAttachedConn.Open(iMaster.iSS);
	if (KErrNone != err)
        	{
	        LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() error opening connection: %d"), this, err));
	        return EFalse;
        	}

	    TUint noConnections = 0;
	    err = iAttachedConn.EnumerateConnections(noConnections);
	    if (KErrNone != err)
        	{
	        LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() error enumerating connections: %d"), this, err));
        	return EFalse;
	        }
	LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() no of connections: %u"), this, noConnections));

    if (noConnections > 0)
        {
        TPckgBuf<TConnectionInfo> info;
        TConnectionInfo currentInfo;
        TBool foundConnection = EFalse;
        for(TInt i=1; i<=noConnections; i++)
            {
            if (KErrNone != iAttachedConn.GetConnectionInfo(i, info))
                {
                return EFalse;
                }
            currentInfo = info();

            LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() currentInfo.NetId = %d "), this, currentInfo.iNetId));
            LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() iMaster.iNetworkId = %d "), this, iMaster.iNetworkId));
            //if(currentInfo.iNetId == iMaster.iNetworkId)
            if(currentInfo.iNetId == this->iNetworkIdofWriter)
                {
                foundConnection = ETrue;
                break;
                }
            }

        if ((foundConnection) && (KErrNone == iAttachedConn.Attach(info, RConnection::EAttachTypeNormal)))
            {
            LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() attached to existing connection at IAP: [%d] Network: [%d]"), this, info().iIapId, info().iNetId));
            return ETrue;
            }
        else
            {
            LOG(Log::Printf(_L("CDnsSocketWriter[%u]::CanUseConnection() could not find any connection matching the Network: [%d]"), this, iMaster.iNetworkId));
            return EFalse;
            }
        }

    return EFalse;
     }
#endif

// CDnsSocketReader
// ****************
CDnsSocketReader::CDnsSocketReader(CDnsSocketWriter &aWriter) : CActive(0), iWriter(aWriter), iBuf(0,0)
	{
	LOG(Log::Printf(_L("CDnsSocketReader[%u]::CDnsSocketReader()"), this));
	CActiveScheduler::Add(this);
	}

void CDnsSocketReader::ConstructL(TUint aDnsMaxMessage)
	{
	//
	// Any reader MUST have a buffer of capacity at least KDnsMaxMessage
	// octets. If this cannot be allocated, cancel the reader (leave!).
	//
	const TUint len = aDnsMaxMessage > (TUint)KDnsMaxMessage ? aDnsMaxMessage : KDnsMaxMessage;
	iInMsg = HBufC8::NewMaxL(len);
	iAllocatedLength = len;
	}

CDnsSocketReader::~CDnsSocketReader()
	{
	ASSERT(!IsActive());
	delete iInMsg;
	if (IsAdded())
		Deque();
	LOG(Log::Printf(_L("CDnsSocketReader[%u]::~CDnsSocketReader()"), this));
	}

void CDnsSocketReader::Activate()
	{
	if (IsActive())
		return;	// Do nothing, if already active!

	if (iWriter.IsTCP())
		{
		LOG(Log::Printf(_L("CDnsSocketReader[%u]::Activate() TCP"), this));
		// Initialize iFrom for connected socket, if not yet done.
		// (only for debugging purposes, no other functionality at momemnt)
		if (iFrom.IsUnspecified())
			iWriter.Socket().RemoteName(iFrom);
		iReadLength = 1;
		iBuf.Set((TUint8 *)iInMsg->Ptr(), 0, 2);
		iWriter.Socket().Read(iBuf, iStatus);
		}
	else
		{
		LOG(Log::Printf(_L("CDnsSocketReader[%u]::Activate() UDP"), this));
		iReadLength = 0;
		iBuf.Set(iInMsg->Des());
		iWriter.Socket().RecvFrom(iBuf, iFrom, 0, iStatus);
		}
	SetActive();
	}


void CDnsSocketReader::RunL()
	{
	LOG(Log::Printf(_L("--> CDnsSocketReader[%u]::RunL() -start- iStatus=%d"), this, iStatus.Int()));
	if (iStatus.Int())
		{
		LOG(Log::Printf(_L("CDnsSocketReader[%u]::RunL() error=%d"), this, iStatus.Int()));
		if (iWriter.HandleError(iStatus.Int()) == 0)
			{
			// Need to re-issue previous request
			if (iWriter.IsTCP())
				iWriter.Socket().Read(iBuf, iStatus);
			else
				iWriter.Socket().RecvFrom(iBuf, iFrom, 0, iStatus);
			LOG(Log::Printf(_L("CDnsSocketReader[%u]::RunL() soft error, restarted last read (TCP=%d)"), this, (TInt)iWriter.IsTCP()));
			SetActive();
			}
		}
	else if (iReadLength)
		{
		iWriter.ResetErrorCount();
		iReadLength = 0;
		const TUint8 *p = (TUint8 *)iInMsg->Ptr();
		const TInt len = p[0] << 8 | p[1];
		if (iAllocatedLength < len)
			{
			// Oops, the current buffer is too short for the packet.
			delete iInMsg;
			iAllocatedLength = len;
			iInMsg = HBufC8::New(len);
			}
		if (iInMsg == NULL)
			iWriter.DeactivateSocket(KErrNoMemory);
		else
			{
			iBuf.Set((TUint8 *)iInMsg->Ptr(), 0, len);
			iWriter.Socket().Read(iBuf, iStatus);
			SetActive();
			}
		}
	else
		{
		iWriter.ResetErrorCount();
		iWriter.RunReader(TMsgBuf::Cast(iBuf), iFrom);
		}
	LOG(Log::Printf(_L("<-- CDnsSocketReader[%u]::RunL() -exit-"), this));
	}

void CDnsSocketReader::DoCancel()
	{
	LOG(Log::Printf(_L("CDnsSocketReader[%u]::DoCancel()"), this));
	iWriter.Socket().CancelRead();
	}

// CDnsSocket
// **********
//
CDnsSocket::CDnsSocket(TUint aMaxDnsMessage) : iMaxDnsMessage(aMaxDnsMessage)
	{
	LOG(Log::Printf(_L("CDnsSocket[%u]::CDnsSocket()"), this));
	}

void CDnsSocket::ConstructL()
	{
	// Allocate the primary reader/writer for UDP
	iWriter = CDnsSocketWriter::NewL(*this, -1);
	iWriter->iNext=NULL;
	}

CDnsSocket::~CDnsSocket()
	{
	DeactivateSocket(KErrCancel);

	while (iWriter)
		{
		CDnsSocketWriter *const writer = iWriter;
		iWriter = iWriter->iNext;
		delete writer;
		}
	LOG(Log::Printf(_L("CDnsSocket[%u]::~CDnsSocket() completed"), this));
	}

RSocket &CDnsSocket::Socket()
	{
	return iWriter->Socket();
	}

/**
// Open the DNS socket and start listening incoming
// packets. The socket is bound to current "bind"
// address and port, which default to NONE and 0.
//
// Does nothing, if socket is already active
//
// Leaves, if socket cannot be activated
*/
CDnsSocketWriter * CDnsSocket::ActivateSocketL(TUint aNetworkId)
	{
	LOG(Log::Printf(_L("CDnsSocket[%u]::ActivateSocketL([NetId = %d])"), this, aNetworkId));
    if(!iConnected)
        {
        LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL([NetId = %d]) connecting socket server"),aNetworkId));
        User::LeaveIfError(iSS.Connect());
        iConnected = 1;
        }
    // use default in case of default network ID
    if(aNetworkId == 0)
        {
        LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL([NetId = %d]) using default writer"),aNetworkId));
        if(iWriter->IsOpened())
            {
            return iWriter;
            }
        else
            {
            const TInt ret = iWriter->ActivateSocket();
            if(ret != KErrNone)
                {
                DeactivateSocket(ret);
                User::Leave(ret);
                }
            else
                {
                return iWriter;
                }
            }
        
        }
    else
        {
        LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL([NetId = %d]) Looking for"),aNetworkId));
        // Find the appropriate writer for the network Id
        CDnsSocketWriter *loopWriter = iWriter;
        while (1)
            {
            if(loopWriter->iNetworkIdofWriter == aNetworkId)
                {
                LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL([NetId = %d]) found"),aNetworkId));
                if(loopWriter->IsOpened())
                    {
                    return loopWriter;
                    }
                else
                    {
                    const TInt ret = loopWriter->ActivateSocket();
                    if(ret != KErrNone)
                        {
                        DeactivateSocket(ret);
                        User::Leave(ret);
                        }
                    else
                        {
                        return loopWriter;
                        }
                    }
                }
            if(loopWriter->iNext == NULL)
                {
                break;
                }
            else
                {
                loopWriter = loopWriter->iNext;	            
                }
            }
        LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL([NetId = %d]) Creating new"),aNetworkId));
         // create a new writer
        loopWriter->iNext = CDnsSocketWriter::NewL(*this, -1);
        loopWriter->iNext->iNetworkIdofWriter = aNetworkId;
        const TInt ret = loopWriter->iNext->ActivateSocket();
        if(ret != KErrNone)
            {
            DeactivateSocket(ret);
            User::Leave(ret);
            }
        else
            {
            return loopWriter->iNext;
            }
        }
	return NULL;
	}

/**
// Change the current bind address and activate the
// DNS socket. If socket is already active, the bind
// address is set, but will only take effect after
// deactivation and next activate.
//
// Does nothing, if socket is already active.
//
// Leaves if socket cannot be activated.
//
// @param	aBind	specify the address and port for
//					received packets (also the source
//					address and port for any sent
//					messages).
*/
void CDnsSocket::ActivateSocketL(const TInetAddr &aBind)
	{
#ifdef _LOG
	TBuf<50> tmp;
	aBind.OutputWithScope(tmp);
	Log::Printf(_L("CDnsSocket[%u]::ActivateSocketL([%S#%d])"), this, &tmp, aBind.Port());
#endif
	LOG(Log::Printf(_L("CDnsSocket::ActivateSocketL with address bind")));
	iWriter->SetBind(aBind);
	ActivateSocketL();
	}

/**
// Activate the TCP listening socket.
//
// This will automaticly cancel any previsous listens.
//
// Leaves if listening cannot be activated.
//
// @param	aBind	specify the address and port for
//					received connections.
// @param	aTTL	specify the TTL for accepted connections
*/
void CDnsSocket::ActivateListenL(const TInetAddr &aBind, TInt aTTL = -1)
	{
#ifdef _LOG
	TBuf<50> tmp;
	aBind.OutputWithScope(tmp);
	Log::Printf(_L("CDnsSocket[%u]::ActivateListenL([%S#%d])"), this, &tmp, aBind.Port());
#endif
	ActivateSocketL();	// First, make sure normal socket is up and running.
	//
	// Abort pending listens, if any
	//
	CDnsSocketWriter **head = &iWriter->iNext;
	while (*head != NULL)
		{
		CDnsSocketWriter *writer = *head;
		if (writer->IsListen())
			{
			*head = writer->iNext;
			// Note: IsListen() writers should never have any
			// queued requests, so no Abort() callbacks will
			// occur and thus, we don't need to worry about
			// re-entries into CDnsSocket via callbacks!
			writer->DeactivateSocket(KErrCancel);
			delete writer;
			}
		else
			head = &writer->iNext;
		}

	if (iListening)
		iListener.Close();
	TInetAddr bind(aBind);	// Needed only because RSocket::Bind wants non-const argument! ARRGHH!
	User::LeaveIfError(iListener.Open(iSS, KAfInet, KSockStream, KProtocolInetTcp));
	iListening = 1;			// Socket opened.
	User::LeaveIfError(iListener.Bind(bind));
	(void)iListener.SetOpt(KSoIp6UnicastHops, KSolInetIp, aTTL);
	(void)iListener.SetOpt(KSoUserSocket, KSolInetIp, 0);
	User::LeaveIfError(iListener.Listen(10));

	//
	// Create and append a new socket writer waiting for listen accept
	//
	(void)AddSecondaryWriter(CDnsSocketWriter::NewTcpListenL(*this, aTTL));
	}

/**
// Close the socket and socket server session, if they
// were open. Abort all queued requests with the specifiec
// reason.
//
// @param aReason
//	the abort reason that is passed to any requests which
//	are removed from the system (see TDnsRequest::Abort)
//					
*/
void CDnsSocket::DeactivateSocket(TInt aReason)
	{
	// Need to grab the list away, because
	// re-activation might occur via callbacks.
	//
	CDnsSocketWriter *writer = iWriter->iNext;
	iWriter->iNext = NULL;

	iWriter->DeactivateSocket(aReason);
	LOG(Log::Printf(_L("CDnsSocket::DeactivateSocket Deativatuing other writers")));
	while (writer)
		{
		CDnsSocketWriter *tmp = writer;
		writer = tmp->iNext;
		LOG(Log::Printf(_L("CDnsSocket::DeactivateSocket calling deactivate socket for writer")));
		TBool allowedToDelete = tmp->DeactivateSocket(aReason);
		if (allowedToDelete)
			{
		delete tmp;
			}
		else
			{
			tmp->SetDeferredDelete();
			}
		}
	// Oops... Should not close if re-activated ---FIX!
	if (iListening)
		{
		iListening = 0;
		iListener.Close();
		}

	// Oops... Should not close if re-activated ---FIX!
	if (iConnected)
		{
		iConnected = 0;
		iSS.Close();
		}
	}

void CDnsSocket::SetHoplimit(const TInt aTTL)
	/**
	* Set TTL for for transmitted packets.
	*
	* The socket must be activated (open) when this is called. The set TTL
	* is used as is for both unicast and multicast. The value -1 selects
	* the system defaults.
	*
	* The set TTL will remain in effect for the primary writer until
	* changed again.
	* 
	* @param	aTTL	The TTL
	*/
	{
	iWriter->SetHoplimit(aTTL);
	}

/**
// Queue a request for sending. The request may get following
// "callbacks":
// @li	TDnsRequest::Build,
//			just before a request is ready to be sent, build
//			the outgoing packet into specified message buffer
// @li	TDnsRequest::Sent,
//			the packet has been sent to the interface
// @li	TDnsRequest::Reply,
//			a DNS reply packet matching the request id has
//			been received. Need to test whether rest of the
//			packet matches the request, and if so, handle
//			it.
// @li	TDnsRequest::Abort,
//			request is being aborted (usually DNS socket
//			is being deactivated).
//
// @param aRequest	the request to be queued
// @param aId		the request id
//	@li	-1 (< 0),
//		the default if parameter is omitted. The queue
//		method will assign random id automatically.
//	@li >= 0,
//		16 bits of this value is used as id.		
*/
void CDnsSocket::Queue(TDnsRequest &aRequest,  CDnsSocketWriter * aWriter, const TInt aId)
	{
    if(aWriter == NULL)
        {
        // use default
        iWriter->Queue(aRequest, aId);
        }
    else
        {
        aWriter->Queue(aRequest, aId);
        }
	}


// Queue a request for sending with a specific socket
//
// Though the DNS socket writer instance was passed as part of this function, but not being used as this
// function matches the socket to find the appropriate writer instance
TInt CDnsSocket::Queue(TDnsRequest &aRequest, const RSocket &aSocket,  CDnsSocketWriter */* aWriter*/, const TInt aId)
	{
    for (CDnsSocketWriter *writer = iWriter; writer != NULL; writer = writer->iNext)
		{
		if (&aSocket == &writer->Socket())
			{
			// Found it!
			writer->Queue(aRequest, aId);
			return KErrNone;
			}
		}
	
	return KErrNotFound;
	}


/**
// Queue a request for sending with TCP.
//
// @param aRequest to be queued
// @param aServer The address and port of the DNS server
// @param aId of the request, if >= 0. If < 0, then a new random ID is generated
// @param aTTL of the connection (= -1, the default, requests the system default). This is
// only effective if the connection is created.
//
// @return KErrNone, if queued successfully, or error code if failed.
*/
TInt CDnsSocket::Queue(TDnsRequest &aRequest, const TInetAddr &aServer,  CDnsSocketWriter * /*aWriter*/, const TInt aId,  const TInt aTTL)
	{
	//
	// Locate or create connected Socket reader/writer instance
	//
	CDnsSocketWriter *writer = iWriter->iNext;
	while (writer != NULL)
		{
		if (writer->Match(aServer))
			{
			// A writer already exists
			writer->Queue(aRequest, aId);
			return KErrNone;
			}
		else
		    {
            //iWriter = iWriter->iNext;
            writer = writer->iNext;
            //do we need to return?
		    }
		}
	//
	// Create and append a new connected socket writer
	//
	TRAPD(err, writer = CDnsSocketWriter::NewTcpL(*this, aServer, aTTL));
	if (writer)
		{
		err = AddSecondaryWriter(writer);
		if (err == KErrNone)
			writer->Queue(aRequest, aId);
		else
			DeleteWriter(writer, err);
		}
	return err;
	}

/**
// Abort whatever the request is currently doing and reque
// it for new send (eventually, a call to DnsRequest::Build
// should happen). This preserves the old id of the request.
//
// If a request is not currently queued, this does an implicit
// Queue. (a new id is generated).
//
// @param aRequest	the request to be resent.
*/
void CDnsSocket::ReSend(TDnsRequest &aRequest, CDnsSocketWriter * aWriter)
	{
	Queue(aRequest, aWriter, aRequest.IsQueued() ? aRequest.Id() : -1);
	}

/**
// Add and activate a new secondary writer/reader unit.
//
// Insert a newly created and constructed reader/writer unit
// to the system and activate it. The secondearies are inserted
// into the iWriter chaing after the first primary unit, which
// always exists, as long as CDnsSocket exists.
//
// @param aWriter	The reader/writer instance
//
// @return KErrNone, if succesfully activated, and an error code otherwise
*/
TInt CDnsSocket::AddSecondaryWriter(CDnsSocketWriter *aWriter)
	{
	ASSERT(iWriter != NULL);		// Primary must always exist!
	ASSERT(aWriter != NULL);		// Silly caller, program error
	if (aWriter == NULL)			// Test again (for release version)
		return KErrArgument;		// Should really not happen!
	ASSERT(aWriter->iNext == NULL);	// The new writer should be just created

	// Insert the new unit after the primary reader/writer, and
	// before all previous secondary units (as the placement in
	// chain should not matter--this is just simplest to do).
	aWriter->iNext = iWriter->iNext;
	iWriter->iNext = aWriter;
	return aWriter->ActivateSocket();
	}

/**
// Deactivate and delete a writer instance.
//
// Deactivate the writer, remove it from the list of writers
// (if present), and delete it.
//
// @param aWriter	The reader/writer to be deleted
// @param aReason	The reason (passed to any requests that are aborted because of this)
*/
void CDnsSocket::DeleteWriter(CDnsSocketWriter *aWriter, TInt aReason)
	{
	// Allow call with NULL, as normal delete does
	if (aWriter == NULL)
		return;

	// Primary writer must not be deleted with this function!
	ASSERT(aWriter != iWriter);
	if (aWriter != iWriter)
		return;

	// Remove instance from the writer chain (allow
	// case where the aWriter is not in the chain!)

	CDnsSocketWriter **head = &iWriter->iNext;
	while (*head != NULL)
		{
		CDnsSocketWriter *const writer = *head;
		if (writer == aWriter)
			{
			*head = writer->iNext;
			break;
			}
		else
			head = &writer->iNext;
		}

	// Note: Deactivate call may cause the CDnsSocket to be deleted,
	// thus it is important that this no longer references it and
	// that the writer has already been removed from the list before
	// this!
	aWriter->DeactivateSocket(aReason);
	delete aWriter;	// a detached instance can now be deleted
	}


TBool CDnsSocket::IsOpened() const
	{
	return iWriter ? iWriter->IsOpened() : EFalse;
	}

TInt64 CDnsSocketWriter::SeedIdGenerator(TInt64 aSequence)
/*
* Function to generate seed for which will be passed to generate another level of sequence number
* The requirement came due the fact that microsoft DNS sequence where gussed. 
* Seed generation is divided into two levels of generation. 
*/
	{
	TInt64 operand = Math::Random() % 5;

	//Generation of random number secure ID which should be mathematically operated with the 
	//Seed. The generation will make guess difficult. Secure ID should not be negative so there is
	//an extra check done to make it positive number.
	TInt64 secureId;	
	secureId = Math::Random();
	if(secureId < 0)
	secureId = secureId * (KNegativeNumberMask);

	switch(operand)
		{
		case 1:
		return (secureId + aSequence);
						
		case 2:
		return (aSequence - secureId);
		
 		default:
		return (secureId * aSequence);
			
		};//end of switch
	}//end of function
