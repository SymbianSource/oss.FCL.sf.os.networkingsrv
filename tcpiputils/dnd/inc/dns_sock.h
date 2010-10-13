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
// dns_sock.h - Domain Name resolver socket
//

#ifndef __DNS_SOCK_H__
#define __DNS_SOCK_H__

/**
@file dns_sock.h
Basic class for sending and receiving DNS protocol packets
@internalComponent	Domain Name Resolver
*/


#include <e32std.h>
class TMsgBuf;
class CDnsSocketWriter;
class TDnsRequest;


class CDnsSocket : public CBase
/**
Manage sending and receiving of DNS protocol packets on a socket

  @internalComponent	Domain Name Resolver

  Class contains the basic control mechanisms for reading and
  writing the socket. A number of send requests (TDnsRequest) can
  be queued into the "system" and they will be processed one
  after another as socket becomes available for a new send.
  At same time, any incoming packets are received and offered
  for processing.
*/
    {
	friend class CDnsSocketWriter;
protected:
	CDnsSocket(TUint aMaxDnsMessage = 0);
	virtual ~CDnsSocket();
	void ConstructL();
	/**
	Open and activate the UDP socket for DNS traffic (unless already done).
	
	The CDnsSocket has private memory for local address and port, which
	both are initialized to unspecified. However, if the
	ActivateSocketL(const TInetAddr &) has been called, then the last
	such call defines the local address and port for this method also.
	The aNetworkId parameter is used to pickup the correct RConnection
 	handle when the EXPLICIT_SOCKET_BINDING option and NON_SEAMLESS_
	BEARER mobility is enabled.
	*/
	CDnsSocketWriter * ActivateSocketL(TUint aNetworkId=0);

	/**
	Open and activate the UDP socket for DNS traffic (unless already done).
	
	@param aBind
		defines the local port and addres for the socket. Address
		and port can be unspecified (the default, if nothing is specified).
	*/
	void ActivateSocketL(const TInetAddr &aBind);
	/**
	Open and activate the TCP listen socket for DNS traffic.
	
	@param aBind
		defines the local port and addres for the socket. Address
		and port can be unspecified (the default, if nothing is specified).
	*/
	void ActivateListenL(const TInetAddr &aBind, TInt aTTL);
	// Close and deactivate the UDP socket for DNS traffic
	void DeactivateSocket(TInt aReason = KErrCancel);
	void SetHoplimit(const TInt aTTL);
	/**
	(Re)Queue the request for processing.
	
	@param aRequest to be queued
	@param aId of the request, if >= 0. If < 0, then a new random ID is generated
	*/
	void Queue(TDnsRequest &aRequest, CDnsSocketWriter * aWriter, const TInt aId = -1);
	/**
	(Re)Queue the request for processing.
	
	@param aRequest to be queued
	@param aSocket identifies a specific active socket within the
			CDnsSocket to be used (see CDnsSocket::Query).
	@param aId of the request, if >= 0. If < 0, then a new random ID is generated
	@return
		@li KErrNone, if queued successfully,
		@li KErrNotFound, if the socket does not exist (not queued)
	*/
	TInt Queue(TDnsRequest &aRequest, const RSocket &aSocket, CDnsSocketWriter * aWriter, const TInt aId = -1);
	/**
	(Re)Queue the request for processing using connected socket (TCP)
	
	@param aRequest to be queued
	@param aServer The address and port of the DNS server
	@param aId of the request, if >= 0. If < 0, then a new random ID is generated
	@param aTTL of the connection (= -1, the default, requests the system default)
	
	@return KErrNone, if queued successfully, or error code if failed.
	*/
	TInt Queue(TDnsRequest &aRequest, const TInetAddr &aServer, CDnsSocketWriter * aWriter, const TInt aId = -1, const TInt aTTL = -1);
	/**
	Reque the request for a resend with same ID (if was queued).
	
	If the request is not already queued, action defaults to
	normal Queue().
	
	@param aRequest to be queued
	*/
	void ReSend(TDnsRequest &aRequest, CDnsSocketWriter * aWriter);

	/**
	Received a Query or unmatched Reply packet
	
	The Query method must be implemented by the derived class. The
	Query is called when a DNS protocol message containing a query
	or a reply which is not accepted by any of the current requests,
	is received.
	
	@param aBuf		the received message
	@param aServer	from which the messsage came
	@parma aSocket
		identify originating socket. Note that this
		"const", you cannot do any socket oprartions with
		this. It is only provided for the special Queue
		function, which queues a request to a specific
		socket.
	*/
	virtual void Query(const TMsgBuf &aBuf, const TInetAddr &aServer, const RSocket &aSocket) = 0;
	/**
	Communication error with a destination

    The Error method can be implemented by the derived class. The
	Error is called when DNS socket receives an ICMP error notification
	when communicating with this target.

	@param aServer for which the error relates
	@param the error from the socket
	*/
	virtual void Error(const TInetAddr &aServer, TInt aError) { (void)aServer; (void)aError; };

private:
	TInt AddSecondaryWriter(CDnsSocketWriter *aWriter);
	void DeleteWriter(CDnsSocketWriter *aWriter, TInt aReason);
	CDnsSocketWriter *iWriter;	//< Primary Socket Reader/Writer
protected:
	const TUint iMaxDnsMessage;	//< Max DNS message length.
	/**
	Get the primary RSocket
	
	@return a reference to primary RSocket (UDP)
	*/
	RSocket& Socket();
	TBool IsOpened() const;

	RSocketServ iSS;			//< Keep own Socket Server Session
	RSocket iListener;			//< Used for listening incoming TCP connections, if enabled.
	TUint iConnected:1;			//< = 1, if Socket Server Session is connected.
	TUint iListening:1;			//< = 1, if iListener is open

private:
	TUint iNetworkId;           		//< = network id fetched from the request
	};

class TDnsRequestLink : public TDblQueLink
/**
Link field of requests.

This object is ONLY used as a member of TDnsRequest to maintain
the links when the request is queued for processing.
*/
	{
	friend class TDnsRequest;
	friend class CDnsSocketWriter;
public:
	inline TDnsRequestLink() : iWriter(NULL) {}
	// @return TRUE, if request is currently queued
	inline TBool IsQueued() const { return iWriter != NULL; }
private:
	// @return The current socket object, or NULL if not queued
	inline CDnsSocketWriter *Writer() { return iWriter; }
	/**
	Set new DNS socket object.
	@param	aWriter	The new socket object
	*/
	inline void SetWriter(CDnsSocketWriter *aWriter) { iWriter = aWriter; }
	/**
	Identifies the queued status and handler for the request.
	@li == NULL,
		when the request is not queued.
	@li != NULL,
		a pointer to the internal socket handling object, which
		is currently assigned to handle the request.			
	*/
	CDnsSocketWriter *iWriter;
	};

	
class TDnsRequest
/**
// The base class for the DNS query request.
//
// The request is controlled by three CDnsScoket methods:
//
// @li CDnsSocket::Queue, (re)enter request into CDnsSocket
// @li CDnsSocket::ReSend, (re)enter request into CDnsSocket
// @li CDnsSocket::Remove, remove request from CDnsSocket
//
// While request is "active in the system" (inside CDnsSocket),
// it's state and progress is reported by the following callbacks:
//
// @li TDnsRequest::Build, ready to send a packet
// @li TDnsRequest::Sent, packet sent
// @li TDnsRequest::Reply, possible reply received
// @li TDnsRequest::Abort, request aborted
//
// The main life cycle of a request is as follows:
@verbatim
    .----->.
   /        \
Remove      Queue
 ^           |
 |___        |
 ^   \       V 
 |  ReSend   /
 |      \   /
 |       \ /
 |< - - - |
 |   wait for send
 |     possible
 |< - - - |-------> Build()
 |    send packet
 |     to socket
 |   and wait for
 |    completion
 |< - - - |-------> Sent()
 |        .
 |        .
 |        |
 |   reply matching
 |   the request id
 |     arrives
  < - - - |-------> Reply()

@endverbatim

  In addition to above, at any point in the lifetime, TDnsRequest::Abort()
  could be called, if the request cannot be processed. For example,
  deactivation of the socket will cause Abort call to all requests.

  At any point, the owner of the request can ReSend, Queue or Remove it
  from the system, regardless of the current state of the request. If
  ReSend is called with request that is not already in the system, the
  Queue action happens instead. Remove does nothing, if request is not
  in the system.

  Note that, when a request is queued, it is only a kind of <em>token</em>
  that is placed into queue to wait for its turn on sending socket.
  Nothing about the actual packet to be sent needs to be present, until
  the DnsRequest::Build is called.
*/
	{
	friend class TRequestQueue;
	friend class CDnsSocketWriter;
public:
	/** @brief Build the DNS packet for the request into buffer
	//
	// Build callback occurs just before the socket is ready to send a message
	// corresponding this request. This method should build the desired
	// message into aBuf, determine the actual destination address (aServer)
	// return TRUE (= 1). The DnsSocket will start a socket send for the
	// message. TDnsRequest::Sent will be called, when the socket send
	// operation completes.
	//
	// If message cannot be build (or should not be sent), return FALSE (= 0)
	//
	// In either case, the request will remain queued in DnsSocket (unless
	// removed).
	//
	// Build CAN call CDnsSocket::Remove, even if it returns TRUE (in which
	// case the message will be sent, but there will not be any Sent callback).
	//
	// @param aSource	the dns socket instance owning the request
	// @retval aBuf		the buffer to be initialized (it has already been preinitialized
	//					using the TDndHeader::Init method). Build method can
	//					either ignore this or use it as a base.
	// @retval aServer	must be initialized with the destination address and port
	//					of the message (if return is TRUE). If the request is
	//					using TCP, this is ignored.
	// @param aRecvLength
	//				the current size of the receive buffer
	//
	// @returns
	//	@li	TRUE,	if message and server has been set, and socket should send the
	//				message out, request is moved into "wait" state.
	//	@li	FALSE,	if socket should not send any message from this request. This
	//				causes the request to be moved into "wait" state.
	*/
	virtual TBool Build(CDnsSocket &aSource, TMsgBuf &aBuf, TInetAddr &aServer, TInt aRecvLength) = 0;
	/** @brief A packet has been sent to the server
	//
	// Sent callback occurs when the "send" of the message completes on the socket.
	// This is a notification only. The request remains queued (unless removed).
	//
	// Sent CAN call CDnsSocket::Remove.
	//
	// @param aSource	the dns socket instance owning the request
	*/
	virtual void Sent(CDnsSocket &aSource) = 0;
	/** @brief A reply received
	//
	// Reply callback occurs when a socket receives a DNS reply message from a
	// server with an ID matching this request (in wait state). The Reply
	// method should do some further verifications whether the reply really
	// is for this request.
	// If the reply matches this request, the method should process the reply
	// and return TRUE.
	//
	// If the reply does not match this request, the method should return
	// FALSE, and the DnsSocket will try another request with same id. If
	// there are no requests that match this ID (or if all of them return
	// FALSE, the reply message is offered to the CDnsSocket::Query method).
	//
	// In either return (TRUE or FALSE), the request will remain in "wait" state
	// and queued in DnsSocket (unless removed).
	//
	// Reply CAN call CDnsSocket::Remove, and still return either
	// TRUE or FALSE.
	//
	// @param aSource	the dns socket instance owning the request
	// @param aBuf		the buffer containing the received reply.
	// @param aServer	the from address of the reply message
	//
	// @returns
	//	@li	TRUE,	if the reply belongs to this request and has been
	//				processed,
	//	@li	FALSE,	if the reply does not belong to this request, and
	//				some other request should be asked.
	*/
	virtual TBool Reply(CDnsSocket &aSource, const TMsgBuf &aBuf, const TInetAddr &aServer) = 0;
	/** @brief Request aborted
	//
	// Abort callback occurs when the DNS socket throws out a request
	// that it has queued. This is NOT called, when the request is
	// removed explicitly by CDnsSocket::Remove. The normal reason
	// for Abort is CDnsSocket::DeactivateSocket, if called with
	// queued requests present.
	//
	// There is no reason for Abort to call CDnsSocket::Remove,
	// because the request has already been effectively removed
	// before the Abort call.
	//
	// @param aSource	the dns socket instance owning the request
	// @param aReason	the reason code for the abort
	*/
	virtual void Abort(CDnsSocket &aSource, const TInt aReason) = 0;
	// Return TRUE, if request is queued
	inline TBool IsQueued() const { return iQueueLink.IsQueued(); }
	// Cancel request (silently remove from the queue, if any)
	void Cancel();
	// Return ID value
	TInt Id() const { return iId; }
private:
	TDnsRequestLink iQueueLink;	//< Linking requests together
	TUint16 iId;				//< The assigned request ID
	};
#endif
