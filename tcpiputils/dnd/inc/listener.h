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
// listener.h - name resolver query dispatcher header
//

#ifndef __LISTENER_H__
#define __LISTENER_H__
/**
@file listener.h
Dispatch resolve requests to resolvers
@internalComponent	Domain Name Resolver
*/
#include <es_sock.h>
#include <in_sock.h>
#include "dns_hdr.h"
#include "servers.h"
#include "res_sock.h"
/**
@mainpage	Domain Name Resolver Daemon (DND)
@section	overview	Overview

  The EPOC32 differs architecturally from the usual Unix resolver
  library implementations:

@li
  Traditionally on Unix, the resolver library is linked with the application and from
  the view point of the kernel, the application is just using additional sockets to
  communicate with the name servers.
@li
  on EPOC32, the name name resolver for the internet is implemented by the TCP/IP
  protocol module, and is shared by all applications. Applications only establish
  a session to this service via RHostResolver class. EPOC32 has standardised the
  resolver interface, and each protocol can have its own implementation of the
  service (with TCPIP protocol, all protocols use the same instance of the name
  resolver).

  The IPv6/Ipv4 stack does not contain the name resolution code. Instead,
  it provides a special socket API for a protocol named <b>resolver</b>, which
  is used by an external server application providing the actual implementation
  of the name resolution.

  The stack maintains a <b>communication path</b> or session between user
  RHostResolver and the resolver instance within the name resolver implementation.
  This path is a chain of objects starting from the RHostResolver and ending
  into CDndResolver, which performs the real name resolution using the other
  modules of the name resolver application:
@verbatim
  RHostResolver (application -> Socket Server)
    CHostResolver (Socket Server -> TCPIP stack)
      CDndSession (TCPIP stack -> gateway socket)
        RSocket (gateway socket -> resolver application)
          CDndResolver (resolver instance)
          - use host files
          - use UDP sockets to communicate with DNS servers
@endverbatim

  The name resolver is a <b>server application</b> listening to the incoming
  requests from a special <b>resolver</b> socket.
  When an application requests a name resolution via some RHostResolver
  method (GetByName, GetByAddress or Query), the resolver gateway code
  in the stack (res.cpp) translates this into a message which is received
  by the DND listener. The message includes a session id, which is assigned
  by the resolver gateway. TDnsRequestBase defines the basic message format.

  The name resolver receives the request and uses the session id in choosing
  the correct resolver instance to serve the request. If a session does not
  yet have a resolver, it will be allocated.

  The resolver processes the request and returns the result as a reply
  message to the socket. The resolver gateway will then translate the result
  into RHostResolver completion.

@section	components	Major components of the implementation

@verbatim
Application                .
===================== RHostResolver ==========
Socket Server              .
===================== CHostResolver ==========
TCPIP Stack                .
                       CDndSession
           resolver        .
            gateway   (session id)
============ socket ===========================
DND                   (session id)
                           .
     main                  .
       \                   .
       engine              .
       /  \                .
      /    \               .
   hosts    \              .
   file    listener        .
           /   |  \        .
          /    |    \      .
         /     |      \    .
   DNS  /     DNS       \  .
 servers   protocol    resolvers
 manager  /  . |  . .    . .
         /  .  |   . .  . .
        /  .  DNS   . .. .
  (LLMNR) .  cache   query
       . .          sessions
        .
        .
     UDP/TCP
==== socket(s) =================================
        .
        .
      remote
     DNS Server
@endverbatim

@subsection	main		Main component

  The <b>main</b> contains the necessary glue and wrappings that are required to
  run an application under EPOC32. The main implements the user interface of
  the application. The standard DND is run as a <b>daemon process</b> and there
  is no real user interface. However, the architecture also allows one to
  write a main module with full user interface, if desired.

  The MDemonMain class defines the services which are required by the
  resolver implementation (engine) from the main module.

@subsection	engine		Engine component

  The <b>engine</b> contains usually the implementation of the application.
  This is intended to be independent of the user interface. The class
  MDemonEngine defines the API from engine to the <b>main</b>.

  In the name resolver implementation, the <b>engine</b> module is left as an
  intermediate between the real implementation in the listener and the main
  module. The CDndEngine is passed as the <b>control</b> instance to almost every
  other component of the resolver implementation. <b>Engine</b> creates and owns
  some common resources, for example: timer service (MTimeoutManager),
  host files handler (RHostsFile) and the configuration
  parameters (TDndConfigParameters).

@subsection	listener	Listener component

  The listener (CDndListener) is the real <b>main program</b> of the name
  resolver implementation. If opens the socket and listens to the
  incoming request messages.

  Using the session id, the listener delegates the task of
  serving the session to an available resolver instance (CDndResolver).
 
@subsection	resolvers	Resolver component

  The <b>resolver</b> (CDndResolver) serves one RHostResolver session at time.
  The same session may serve different RHostResolver's at different times.
  It receives the requests from the gateway socket via the listener.
  The resolver state diagram is roughly

@verbatim
     Start
       |----------------> Stop
     Receive a Query
       |----------------> Stop
     Return first result
       |
   +-->|
   |   | - - -> listener forced Stop
   |   |
   |   |----------------> Stop
   | Receive Next Query
   |   |----------------> Stop
   | Return next result
   +---|
     Stop
@endverbatim

  Because many applications are only interested about the first result,
  and because they often leave the RHostResolver session(s) open, the
  gateway may sometimes <em>steal a session for a new RHostResolver</em>,
  if all resolvers are already busy. The oldest resolver that has returned
  at least one answer is assigned to the new RHostResolver.

  The "stealing action" action does not harm the application in any other way,
  except that if it now tries to call Next(), it will return an error. If
  such application issues a new query (GetByName, GetByAddress or Query),
  a new resolver is just assigned to serve it.

  The resolver session is always stopped, after query has
  completed (no further answers are available).

  The resolver performs its work by establishing a number of query
  sessions (MDnsSession) with the DNS protocol module. The progress
  of an active query session is reported through callbacks
  (MDnsResolver).

@subsection	hosts		Host File component

  The hosts file handler (RHostsFile) provides the service for looking an
  answer from the local hosts file (if any is present). Only the GetByName
  or GetByAddr queries can be satisfied from the hosts file. The "hosts"
  file has a very simple format which binds a name to an address,
  for example:

@verbatim
127.0.0.1 localhost

10.0.0.2 server
10.0.0.3 server
10.0.0.3 alternatename

fe80::dead:beef local-beef

::1 localhost6

ff02::1 allnodes6
ff02::2 allrouters6
ff02::3 allhosts6

@endverbatim

@subsection dnsclient	DNS Protocol implementation

  The DNS procotol is implemented by CDndDnsclient class (or by some derived class). The
  class exports MDnsSource API, which is used by the resolvers to start query sessions
  (MDnsSession), which send and receive the actual DNS protocol messages.

@subsection	llmnr		Link Local Multicast Name Resolution

  The link local multicast name resolution uses the DNS protocol with link local
  multicast. The DNS protocol messages are sent to the link with multicast
  destinations and a node that owns the queried name, will respond as if it
  was a DNS server.

  When LLMNR is compiled in, the DNS protocol is implemented by CDndLlmnrSender
  class, which is derived from the CDndDnsclient. This class extends the normal
  DNS with the link local multicast name resolution additions.


@subsection	servers		Server Manager component

  The server manager (MDnsServerManager) maintains the list of currently known and
  avalable DNS servers. The content of the list may be constantly changing depending
  on which interfaces are active at any particular point of time. The issues are even
  more complicated by the fact that initially there are no interfaces up (or at least,
  there may be no known servers), and name resolver just has to wait until something
  becomes available.

@subsection cache		DNS Cache

  The cache component (CDndCache) stores cached answers from the DNS servers. When
  resolvers send questions to the protocol module, the cache is first checked for
  a matching valid answer. If found, the cached answer is returned to the resolver,
  and no actual queries need to be sent.

@section	scoped_dns	Scoped name resolution architecture

  The answers to a query can be found from different sources at different scope
  levels. This implementation uses three levels:

@li
  the <b>hosts file</b> is a node local scope source.

@li
  the <b>"real" DNS</b> is a global scope source.

@li
  <b>LLMNR</b> is a link local scope source.

  The above order is also the current default order of the sources: first,
  the <b>hosts</b> file is checked, then real DNS and finally, the link local name
  resolution is attempted. The first source to give an answer defines the whole
  answer. Answers from different sources are never mixed.

  A scope level is assigned to all queries that are received. Currently a global
  scope is given to all GetByName and Query requests. The scope level of the
  GetByAddress is the scope level of queried address.

  For a specific query, only sources that have same or smaller scope level are used.
  For example, only LLMNR is used, when GetByAddress to a link local address
  (169.254/16 or fe80::/10) is requested.

  GetByName queries could also have other than global scope. For example,
  it could be agreed that a query for a name ending with ".local" would have a
  link local scope, and thus only LLMNR would be tried for it (the current
  implementation does not have such feature coded).


*/

/**
// The size of the resolver pool. This defines how many simultaneous
// clients (opened RHostResolver's) are served. Note, that this does
// is not set a limit for the total number open RHostResolver's in all
// applications. If there are more than KDndNumResolver RHostResolvers,
// the resolver gateway (res.cpp in the stack) will maintain a queue, and
// assign the service to applications, when resolvers become available
// from the pool.
*/
const TUint KDndNumResolver = 31;
/**
// Special Progress Notifications for the TReplyCallback: query has been sent.
//
// KDnsNotify_QUERY_SENT
// occurs when the DNS query has actually been sent to the
// server (e.g. socket write completes). Normally this occurs
// very fast after DoQueryL(), except when the packet or DNS
// has to activate the "netdial startup". In such case, the
// sending is blocked until the interface becomes available.
*/
const TInt KDnsNotify_QUERY_SENT = 1;
/**
// Special Progress Notifications for the TReplyCallback: DNS server list is available.
//
// KDnsNotify_HAVE_SERVERLIST
// occurs when the DNS has found at least one DNS server
// address. This signal is only returned if MDnsSession::NewQuery
// returned with > 0 (indicating it started interface startup).
*/
const TInt KDnsNotify_HAVE_SERVERLIST = 2;
/**
// The request is using TCP instead of UDP.
//
// KDnsNotify_USING_TCP
// occurs when the DNS decides to use a connected TCP socket
// for the query to the DNS server. This may happen on receipt of
// DNS reply with TC=1 (truncated), or if some configuration setting
// forces the use of TCP.
//
// The expected effect should be to disable the retransmission
// logic of the resolver, because TCP handles it internally.
*/
const TInt KDnsNotify_USING_TCP = 3;

// -- for now, no other notifys --

/**
// Query modifier flag -- KDnsModifier_RD
//
// Used in aFlags parameter of MDnsSession::NewQuery. This controls
// the state of RD (Recursion Desired) bit in the DNS query messages
// generated from for this query. Normally, this is always set.
*/
const TUint32 KDnsModifier_RD = 1;

/**
//
// Set probe flag -- KDnsModifier_PQ
// 
// Set in ProbeServer() to switch off worker assigning for the query.
// If not set, DNS will check if there's already same query assigned
// to some resolver.
//
*/
const TUint32 KDnsModifier_PQ = 2;

/**
// The query session.
//
// The query session is a handle through which the resolver can request
// the DNS protocol module to do queries and return answers to them. The
// use of separatate session concept (instead of having the methods
// directly on MDnsSource) allows resolver to do multiple queries
// in parallel.
//
// The usage is:
//
// @li (1) open a session with MDnsSource::OpenSession
// @li (2) define the domain name using MDnsSession::NewQuery
// @li (3) activate a specific query (type) with MDnsSession::DoQueryL
// @li (3) after the query is complete (MDnsResolver::ReplyCallback),
//		retrieve answers one by one with repeated calls to MDnsSession::DoNext
// @li (4) session can be used for another query with same domain-name (-> 3),
//		or different domain-name (-> 2)
// @li (6) when session is no more needed, it must be released by MDnsSession::Close
//
// The MDnsSession::CancelQuery can be used at any point to cancel
// all activity, that has been started by MDnsSession::DoQueryL.
//
// When a DNS server is required, the query session will select the current
// default server. This choice will not be changed even if the server is not
// responding or is not reachable. It is the duty of the controlling resolver
// to decide when to try another server, if any are available. For this
// purpose, the MDnsSession has the following methods:
//
// @li MDnsSession::PickDefaultServer, reset to default
// @li MDnsSession::PickNewServer, pick next server in list
*/
class MDnsSession
	{
public:
	/**
	// Load new query information to be resolved.
	//
	// Load a new query into session by converting the information from the
	// TNameRecord or TDnsQuery into DNS format. This only loads the name
	// to be queried, it does not define query type or class.
	//
	// For TNameRecord, if the iName field is empty, then iAddr is used to
	// construct a name to be searched (*.in-addr.arpa or .ip6.int).
	//
	// @param aQuery
	//	defines the name or address to be queried. Does not define
	//	the actual query type. This same question may be used for
	//	different query types (for example A and AAAA).
	// @param aServerScope
	//	the scope level of the query. This limits the servers that
	//	are going to be used.
	// @param aFlags
	//	modify the query operation. See KDnsModifier_* symbols.
	//
	// @returns
	// @li	< 0, if load failed (bad query information)
	// @li	= 0, if query initialized (KErrNone)
	*/
	virtual TInt NewQuery(const TDnsMessage &aQuery, TDnsServerScope aServerScope, TUint32 aFlags) = 0;
	/**
	// Cancel query activity
	//
	// Cancel all (if any) activity caused by a DoQueryL on this session.
	// This makes session usable for a new query, it does not close the
	// session. MDnsResolver::ReplyCallback is disabled and not called
	// (until reactivated by a new DoQueryL).
	*/
	virtual void CancelQuery() = 0;
	/**
	// Activate a query of specified type for the loaded query information.
	//
	// Enables the MDnsResolver::ReplyCallback.

	// @param aRequestTime
	//	the time when resolver received the request that is causing this
	//	query to be asked. The value is mainly used to determine whether
	//	the cached answer is still valid for this request.
	// @param aQType
	//	the actual query to be done. IN class is assumed.
	//
	// @returns
	// @li	< 0, if query cannot be activated
	// @li	= 0, if query processed (KErrNone)
	*/
	virtual TInt DoQueryL(const TTime &aRequestTime, const EDnsQType aQType) = 0;
	/**
	// Retrieve answer RR's from the previous completed reply.
	//
	// @param aReply
	//	the reply buffer that receives the ansver
	// @param aNext
	//	the index of the answer to return. The first answer has the
	//	index == 0.
	//
	// @returns
	// @li	< 0, if no more answers, or some error is detected
	// @li	= 0, if requested answer has been placed into aReply
	*/
	virtual TInt DoNext(TDnsMessageBuf &aReply, TInt aNext) const = 0;
	/**
	// Set current record into error state.
	//
	// @param aError
	//	the error code to set
	// @param aTTL
	//	the lifetime (seconds) for the error to persist
	//	in the cache before it becoming invalid.
	*/
	virtual void DoError(TInt aError, TUint aTTL) = 0;
	/**
	// Choose next server for the session.
	//
	// @returns
	//	@li	TRUE, if a different server could be picked
	//	@li	FALSE, if no different server available
	*/
	virtual TBool PickNewServer() = 0;
	/**
	// Set default server for the session.
	//
	// Reassign the session to the current chosen "best" server
	// (if any).
	//
	// @returns
	//	@li	TRUE, if a server was selected (or still waiting configuration to stabilise)
	//	@li	FALSE, otherwise (no suitable servers at this point).
	*/
	virtual TBool PickDefaultServer() = 0;
	/**
	// Close the session.
	//
	// Release resources associated with the session and
	// make it available for re-use. CancelQuery is done
	// automaticly.
	*/
	virtual void Close() = 0;

	// Return instance ptr of the actual class instance (only for comparisons)
	virtual const void *const Instance() const = 0;
	};


class MDnsResolver
	{
public:
	/**
	// Response callback.
	//
	// After a query has been activated by MDnsSession::DoQueryL,
	// its progress and completion is reported back by a call
	// to this callback function. The callback may also occur
	// during the DoQueryL.
	//
	// A completion is indicated by a result <= 0. Results > 0
	// are intermediate notifications.
	//
	// @param aErr is defined as follows:
	// @li	< 0, query complete result, some error condition
	// @li	= 0, query complete result, success
	// @li	> 0, query progress notification from DNS.
	//	Currently, only two are used: KDnsNotify_QUERY_SENT, notification
	//	that query has been sent, and KDnsNotify_HAVE_SERVERLIST, notification
	//	that DNS provider has some server list available (which could be empty).
	*/
	virtual void ReplyCallback(const TInt aResult) = 0;
	};

/**
// DNS protocol services
//
// This is a handle to the DNS protocol and some other services.
*/
class MDnsSource
	{
public:
	/**
	// Open a session.
	//
	// @param aCallback
	//	The reply callback to be used in reporting events back
	// @param aCallbackData
	//	The fixed data provided by the callback call
	// @returns
	//	@li	MDnsSession, if open succeeded
	//	@li NULL, if session could not be opened
	*/
	virtual MDnsSession *OpenSession(MDnsResolver *const aCallback) = 0;
	/**
	// Check if there is a route and valid source address for the aDestination.
	//
	// @param	aDestination	The address to test
	// @returns
	//	@li	KErrNone, if address is usable (route exists)
	//	@li KErrNotFound, if no route or valid source addess exists
	//	@li < 0, possible other errors (from failing to open sockets etc.)
	*/
	virtual TInt CheckAddress(const TInetAddr &aDestination) = 0;
	/**
	// Resolver has received a query from application.
	//
	// Just book keeping, which enables the protocol handler to
	// release resources, if there are no active queries.
	*/
	virtual void QueryBegin() = 0;
	/**
	// Resolver has completed the query from application.
	//
	// Just book keeping, which enables the protocol handler to
	// release resources, if there are no active queries.
	*/
	virtual void QueryEnd() = 0;
	/**
	// Change the hostname.
	//
	// This informs the LLMNR about the current hostname.
	//
	// @param	aId The network id
	// @param	aName The new host name
	// @return	KErrNone, if changed.
	*/
	virtual TInt SetHostName(TUint32 aId, const THostName &aName) = 0;
	/**
	// Get current hostname.
	//
	// If LLMNR is enabled and uniqueness testing is still in progressm,
	// since the last SetHostName then the callback is installed to be
	// called when testing is complete.
	//
	// @param	aId The network id
	// @retval	aName
	//	The current hostname (always returned)
	// @param	aCallBack
	//	If return is pending (= 1), then the callback WILL be
	//	called when the result of the uniqueness test is
	//	known. Otherwise callback is NOT used.
	// @return
	//	@li KErrNone, if the name is unique on current interfaces (no callback)
	//	@li	= 1, if uniqueness test is in progress (callback installed)
	//	@li < 0, if uniqueness test is complete, and failed (no callback)
	*/
	virtual TInt GetHostName(TUint32 aId, THostName &aName, MDnsResolver &aCallback) = 0;
	/**
	// Check for own address.
	//
	// @param aId	The network id
	// @param aAddr	The address to check
	// @param aNext	Non-zero, when next processing
	// @retval aName	The returned local host name matching the address.
	// @return KErrNone, if address was a local and name is returned
	*/
	virtual TInt GetByAddress(TUint32 aId, const TInetAddr &aAddr, TInt aNext, THostName &aName) = 0;
	/**
	// Check for own name.
	//
	// @param aId	The networkd id
	// @param aName	The local host name
	// @param aNext	Non-zero, when next processing
	// @retval aAddr	The own address
	*/
	virtual TInt GetByName(TUint32 aId, const THostName &aName, TInt aNext, TInetAddr &aAddr) = 0;
	};

class CDndEngine;

// MDndListener
// ************
/**
// Main "module" implementing the resolver gateway.
//
// Function
//
// The listener owns several resolver objects. It waits for
// any request from the applications, and whenever it receives one,
// it passes the request to the resolver object.
*/
class MDndListener
	{
public:
	static MDndListener *NewL(CDndEngine &aControl);
	virtual ~MDndListener() { }
	// for debugging only (not used currently)
	virtual void HandleCommandL(TInt aCommand) = 0;

	/**
	* Called when a resolver has a reply ready to be returned.
	*
	* The writer will eventually ask the resolver for the buffer
	* using the MdndResolver::ReplyMessage. This should return
	* ZERO length descriptor, if there is no reply to send.
	*/
	virtual void PostReply() = 0;
	};

#endif
