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
// resolver.cpp - name resolver
//

#include <in_sock.h>
#include <timeout.h>
#include "listener.h"
#include "resolver.h"
#include "engine.h"
#include "hosts.h"
#include <networking/dnd_err.h>
#include "dns_ext.h"
#include "inet6log.h"

#ifdef DND_DCM_EXTENSION
#include "dnd.hrh"		// EDndFlush
#endif

#ifdef EXCLUDE_SYMBIAN_DNS_PUNYCODE
#undef SYMBIAN_DNS_PUNYCODE
#endif //EXCLUDE_SYMBIAN_DNS_PUNYCODE

struct SQueryStep
	{
	EDnsQType iQType;			// the query type
	};

struct SQueryExtension
	{
	const TDesC *iPrefix;
	SQueryStep iQuery;
	};

// ...for GetByAddr, perform only PTR query (for each server)
const SQueryStep KGetByAddress[1] =
	{
	{ EDnsQType_PTR }
	};

// ...for GetByName, try first AAAA and then A (for each server)
const SQueryStep KGetByName1[2] =
	{
	{ EDnsQType_AAAA },
	{ EDnsQType_A }
	};

// ...for GetByName, try first A and then AAAA (for each server)
const SQueryStep KGetByName2[2] =
	{
	{ EDnsQType_A },
	{ EDnsQType_AAAA }
	};

// Define the search order of the sources
const TDnsServerScope KSourceOrder[] =
	{
	EDnsServerScope_HOSTFILE,	// 1. Search hosts file first
	EDnsServerScope_UC_GLOBAL,	// 2. Search global DNS
#ifdef LLMNR_ENABLED
	EDnsServerScope_MC_LOCAL,	// 3. Try Link Local Multicast
#endif
	};

const SQueryExtension KQueryExtension[] =
	{
#if 0
	{ &KDnsExtQType_A,		EDnsQType_A },
	{ &KDnsExtQType_AAAA,	EDnsQType_AAAA },
	{ &KDnsExtQType_MX,		EDnsQType_MX },
	{ &KDnsExtQType_NS,		EDnsQType_NS },
	{ &KDnsExtQType_SOA,	EDnsQType_SOA },
	{ &KDnsExtQType_CNAME,	EDnsQType_CNAME },
	{ &KDnsExtQType_PTR,	EDnsQType_PTR },
	{ &KDnsExtQType_SRV,	EDnsQType_SRV },
	{ &KDnsExtQType_NAPTR,	EDnsQType_NAPTR },
	{ &KDnsExtQType_ANY,	EDnsQType_ANY }
#else
	// Workaround for ARMV5 compiler bug? Above declaration always
	// produces 80 bytes of writable global data. The following
	// ugly definition does not.
	{ (const TDesC *)&KDnsExtQType_A.iTypeLength,		EDnsQType_A },
	{ (const TDesC *)&KDnsExtQType_AAAA.iTypeLength,	EDnsQType_AAAA },
	{ (const TDesC *)&KDnsExtQType_MX.iTypeLength,		EDnsQType_MX },
	{ (const TDesC *)&KDnsExtQType_NS.iTypeLength,		EDnsQType_NS },
	{ (const TDesC *)&KDnsExtQType_SOA.iTypeLength,		EDnsQType_SOA },
	{ (const TDesC *)&KDnsExtQType_CNAME.iTypeLength,	EDnsQType_CNAME },
	{ (const TDesC *)&KDnsExtQType_PTR.iTypeLength,		EDnsQType_PTR },
	{ (const TDesC *)&KDnsExtQType_SRV.iTypeLength,		EDnsQType_SRV },
	{ (const TDesC *)&KDnsExtQType_NAPTR.iTypeLength,	EDnsQType_NAPTR },
	{ (const TDesC *)&KDnsExtQType_ANY.iTypeLength,		EDnsQType_ANY }
#endif
	};

// KMaxQuerySessions
// *****************
/**
// The maximum number of query sessions for single resolver,
// for example, supporting A and AAAA query, requires this
// to be at least 2.
// (if someone ever adds A6 to the list, it needs to be 3).
*/
const TUint KMaxQuerySessions = 2;

// KMaxDeprecated
// **************
/**
// The size of the deprecated answers buffer. When processing
// the replies for GetByName, the resolver will first return
// those A and AAAA answers, for which there is an existing
// route and source address in the current system. Other answers
// are put into deprecated buffer, and returned after the
// "good" answers.
*/
const TUint KMaxDeprecated = 20;

class CDndResolver;

/**
// The resolver state automaton for one DNS query.
//
// This executes the state automaton for a single DNS query
// (for example A, AAAA, MX, etc.). The processing includes
// automatic retransmissions (potentially to multiple DNS
// servers) and timeout handling 
//
// The query process is started by Start() and it will
// then proceed independently to the conclusion.
// When the query has completed or failed,
// CDndResolver::GetNextAnswer() is called.
//
// In some cases an error is considered to be non-recoverable,
// and instead of calling GetNextAnswer(), CDndResolver::QueryDone(error)
// is called instead. This cancel the main query processing and
// return the indicated error directly to the client process
// (RHostResolver).
//
*/
class TQuerySession: public MDnsResolver
	{
	friend class TQuerySessionTimeoutLinkage;
public:
	// Default constructor
	TQuerySession();
	// Supplement default constructor
	void Init(CDndResolver *const aResolver);
	// Open a new session to the DNS handler
	void Open();
	// Close the session with the DNS handler
	void Close();
	// Start a DNS query on the session
	TBool Start(EDnsQType aQType);

	MDnsSession *iSession;		//< Session handle with DndDnsclient
	TInt iStatus;				//< Session status / next record to retrieve (if >= 0)
	EDnsQType iQType;			//< the query type
private:
	static inline TInt Max(const TInt a, const TInt b) { return a > b ? a : b;}

	// A callback method when the timeout expires
	void Timeout(const TTime &aNow);
	// Request a call to Timeout after specified time (seconds)
	void SetTimer(TUint aTimeout);
	// A callback method when the request to DNS handler completes
	void ReplyCallback(const TInt aErr);

	// Cancel pending operations
	void Cancel();
	// Activate a query in the DNS handler
	void SendDnsQuery();

	// Internal, handles the common part of timeout and reply callbacks
	void Event(TBool aTimeout);
	
	CDndResolver *iResolver;	//< The back pointer to resolver

	TUint8 iQueryRetry;			//< Current retry index (for the current step)
	TUint8 iQueryTimeouts;		//< Number of true timeouts (used by some resolver modes)
	TUint8 iServerCount;		//< used in "spray mode" to count the servers.
	TUint iWaitingQuerySent:1;	//< used in detecting and timing out interface startup.
	TUint iWaitingAfterSpray:1;	//< used only in "spray mode"
	TUint iSendingQuery:1;		//< set to 1, when DoQueryL is being called (recursion safeguard in SendDnsQuery)
	TUint iPendingQuery:1;		//< set to 1, when DoQueryL is required (recursion safeguard in SendDnsQuery)
	TUint iQueryTimeoutValue;	//< The current timeout for waiting the reply AFTER packet is sent
public:	// Needed for gcc!
	RTimeout iTimeout;
#ifdef _LOG
	TBuf<2> iName;
#endif
	};

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KQuerySessionTimeoutLinkageOffset 28
__ASSERT_COMPILE(KQuerySessionTimeoutLinkageOffset == _FOFF(TQuerySession, iTimeout));
#else
#define KQuerySessionTimeoutLinkageOffset _FOFF(TQuerySession, iTimeout)
#endif

class TQuerySessionTimeoutLinkage : public TimeoutLinkage<TQuerySession, KQuerySessionTimeoutLinkageOffset>
	{
public:
	static void Timeout(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
		{
		Object(aLink)->Timeout(aNow);
		}
	};

/**
// A structure to remember an answer to be retrieved later.
*/
class TDeprecatedAnswer
	{
public:
	inline TDeprecatedAnswer() {}
	inline TDeprecatedAnswer(TUint8 aStep, TUint8 aIndex) : iStep(aStep), iIndex(aIndex) {}

	TUint8 iStep;		//< query step # (selects session)
	TUint8 iIndex;		//< answer index within the query reply
	};

/**
The real implementation of the MDndResolver.

Function:

@li	Accepts the request (see TDnsRequestBase) from the application for name resolving.
@li	Tries to resolve the request from host files or DNS sources
@li	Once the request is over (response obtained or time out),
	it sends back the response (again TDnsRequestBase) and/or error code
	to the application which requested it.

See also \ref resolvers

This class is designed so that it can be reused for serving
different clients. It has the following simplified state diagram:

@verbatim
                   _________________________
                 /      reuse               \
                V                            |
 construct -> Start() ---> WaitQuery() --> Stop() --> destruct
                               /  \          ^
                              |    |         |
                               \  /         /
                            QueryDone() ___/

@endverbatim
@li Start()
	serving a specific RHostResolver session. This method is called
	from CDndListener, after it has received a new query request
	from the stack.
@li WaitQuery()
	the resolver is waiting for next incoming request
	from the RHostResolver.
@li QueryDone()
	queue a result to be returned to the RHostResolver.
@li Stop()
	serving the current rquest. This may be called either
	internally or from CDndListener.
*/
class CDndResolver : public CBase, public MDndResolver, public MDnsResolver
	{
	friend class TQuerySession;
public:
	CDndResolver(const TInt aId, CDndEngine &aControl, MDndListener &aListener, 
					MDnsSource &aDnsclient);

	void Start(const TDnsMessageBuf &aMsg);	// Starts the resolver
	const TDesC8 &ReplyMessage();			// Return reference to the reply buffer.
	TUint16 Session() const;				// Return session id of the current query.
	void Stop();							// Stop the resolver
	virtual ~CDndResolver();

	static inline TInt Max(const TInt a, const TInt b) { return a > b ? a : b;}

	const TTime &RequestTime() const { return iRequestTime; }
	// only used for GetHostName
	void ReplyCallback(const TInt aResult);

private:
	// Return a result to the application
	void QueryDone(TInt aResult);
	// Wait for the next query from the application
	void WaitQuery();
	// Select next source for the name resolving (HOSTS, DNS, LLMNR)
	void NextSource();
	// Cancel all activity and resources being used for the current source
	void StopSource();
	TBool StartSource();
	// Return next result from HOSTS file source
	void HOSTFILE_Next();

#ifdef _LOG
	// Print out debugging information
	void ShowQuery(const TDnsMessage &aName, TInt aReply);
#endif

	// Retrieve next answer, possibly starting retrieval of additional information from source
	void GetNextAnswer();
	// Used to find alternate DNS servers (in some operating modes)
	void ProbeServers();
private:
	const TInt iId;					//< Resolver instance id (for debugging use)
	TTime iRequestTime;				//< Time at which the request was received from the application

	// The Query buffer for the reply
	TDnsMessageBuf iBuffer;
	// The current orignal query being processed.
	TDnsMessageBuf iCurrentQuery;

	const TDnsServerScope *iSourceList;	//< Information sources
	TDnsServerScope iSourceNow;		//< The current source
	TUint iQueryStepMaxTime;		//< Maximum time allowed for single query step to complete
	TUint iQueryStepMinTime;		//< Minimum time for query step (usage depends on mode)
	TUint8 iQueryStepRetries;		//< Additional retries after the primary attempt
	TInt iSourceCount;				//< Remaining number of sources.
	TInt iNext;						//< Non-zero, if executing Next operation

	// Specify Current Query state

	const SQueryStep *iQueryStep;	//< The Current Query Process (what queries to make)
	SQueryStep iSpecificQuery;		//< Used in implementing the Specific Single Queries
	TQuerySession iSession[KMaxQuerySessions]; //< Currently active queries or completed results
	TDeprecatedAnswer iDeprecate[KMaxDeprecated];	//< Buffer to store the delayed answers
	TUint iPhase;
	TUint8 iDeprecatedTop;		//< Number of delayed top answers in iDeprecate[]
	TUint8 iDeprecatedBottom;	//< Number of delayed bottom answers in iDeprecate[]
	TUint8 iQuerySteps;			//< Number of Query Steps in the process
	TUint8 iStepCount;			//< Number of Query Steps activated so far
	TUint8 iSessionCount;		//< Number of Query Sessions currently active
	/*
	// The scope level of the current query.
	//
	// Currently the level for GetByName and Query is always "NETWORK".
	// But, if implemented, it could be "LINK LOCAL", based on the
	// queried name, for example, if it ends with ".local".
	//
	// The level of GetByAddress is the scope level of the address being
	// queried.
	//
	// The query scope limits the sources available to the query: the scope
	// of the source must be less or equal to the query scope. (For example,
	// GetByAddress for a link local address is only asked from link local
	// name resolvers).
	*/
	TUint8 iQueryScope;
	TUint iQueryActive:1;		//< set to 1, when a query is being processed
	TUint iQueryIsAddr:1;		//< set to 1, when current query is GetByAddr, = 0, otherwise
	TUint iQueryIsSpecial:1;	//< set to 1, when current query is special (for exact DNS query type)
	TUint iReadyForAnswer:1;	//< set to 1, when an answer can be delivered back to RHostResolver.
	TUint iQueryDoneWait:1;		//< Tells what active object is currently waiting: = 0, if WaitQuery, = 1, if QueryDone
	TUint iQueryComplete:1;		//< set to 1, when query is completed fully (no Next allowed)
	TUint32 iQueryFlags;		//< Flags to the NewQuery (KDnsMofidier_RD, ...)

	TQuerySession iProbe;		//< Use for server probing
	CDndEngine &iControl;		//< The control context (for sending errors etc.)
	MDndListener &iListener;	//< Only for calling KeepRunning()
	MDnsSource &iDnsclient;		//< DNS protocol handler
	};


// CDndResolverBase::New
// *********************
MDndResolver *MDndResolver::New(const TInt aId, CDndEngine &aControl, MDndListener &aListener, MDnsSource &aDnsClient)
	/**
	* Create a new instance of CDndResolver.
	*
	* This static function creates the real object instance of
	* the class that has been derived from CDndResolverBase.
	*
	* This way, the real implementation of the class is hidden
	* from any other module (the real class CDndResolver is only
	* declared and known within resolver.cpp module).
	*
	* @param aId identifies the resolver instance (only for debuggind purpose)
	* @param aControl provides access to the DND environment
	* @param aListener
	*	provides access to the socket listener, which manages the
	*	pool of CDndResolver instances and delivers the requests
	*	to the resolvers.
	* @param aDnsClient
	*	provides access to the DNS protocol driver, which implements
	*	the DNS protocol details, and manages the associated UDP and
	*	TCP sockets.
	* @return
	*	@li	NULL, if object could not be created (out of memory)
	*	@li non-NULL, is a pointer to the newly constructed CDndResolver instance
	*/
	{
	return new(ELeave) CDndResolver(aId, aControl, aListener, aDnsClient);
	}

// Constructor
CDndResolver::CDndResolver(const TInt aId, CDndEngine &aControl, MDndListener &aListener, MDnsSource &aDnsclient)
	: iId(aId), iControl(aControl), iListener(aListener), 
	  iDnsclient(aDnsclient)
	{
	// Because array elements cannot have other than default constructors,
	// need to initialize the iSession[] and iProbe here..
	for (TInt i = KMaxQuerySessions; --i >= 0; )
		{
		iSession[i].Init(this);
		LOG(iSession[i].iName.Format(_L("%d"), i));
		}
	iProbe.Init(this);
	LOG(iProbe.iName = _L("*"));
	}

// Destructor
CDndResolver::~CDndResolver()
	{
	// Need to cancel TQuerySession's (if any active)
	for (TInt i = KMaxQuerySessions; --i >= 0; )
		iSession[i].Close();
	}


#ifdef _LOG
// CDndResolver::ShowQuery(aName)
// ******************************
void CDndResolver::ShowQuery(const TDnsMessage &aPacket, TInt aReply)
	/**
	* Displays the current query or reply (only a debugging tool).
	*
	* @param	aPacket	Query or Reply data
	* @param	aReply	Query vs. reply
	*/
	{
	_LIT(KReply, " Reply ");
	_LIT(KQuery, " Query ");

	const TDesC &mode = aReply ? KReply : KQuery;

	switch (aPacket.iType)
		{
		case KDnsRequestType_GetByName:
		case KDnsRequestType_GetByAddress:
			{
			const TNameRecord &rec = aPacket.NameRecord();
			TBuf<50> tmp;
			TInetAddr::Cast(rec.iAddr).OutputWithScope(tmp);
			if (aReply)

				Log::Printf(_L("\tresolver[%d] SESSION %d Reply [%d], %S, [%S] result=%d"), iId, aPacket.iSession, rec.iFlags, &rec.iName, &tmp, aPacket.iNext);
			else
				Log::Printf(_L("\tresolver[%d] SESSION %d Query (%d), %S, [%S]"), iId, aPacket.iSession, aPacket.iNext, &rec.iName, &tmp);
			}
			break;
		case KDnsRequestType_TDnsQuery:
			{
			if (aReply)
				{
				const TDndReply &reply = aPacket.Reply();
				Log::Printf(_L("\tresolver[%d] SESSION %d Special Reply, type=%d result=%d"), iId, aPacket.iSession, (TInt)reply.RRType(), aPacket.iNext);
				}
			else
				{
				const TDnsQuery &query = aPacket.Query();
				Log::Printf(_L8("\tresolver[%d] SESSION %d Special Query (%d), %S type=%d"), iId, aPacket.iSession, aPacket.iNext, &query.Data(), (TInt)query.Type());
				}
			}
			break;
		case KDnsRequestType_SetHostName:
			Log::Printf(_L("\tresolver[%d] SESSION %d SetHostName(%S)%S%d"), iId, aPacket.iSession, &aPacket.HostName(), &mode, aPacket.iNext);
			break;
		case KDnsRequestType_GetHostName:
			Log::Printf(_L("\tresolver[%d] SESSION %d GetHostName(%S)%S%d"), iId, aPacket.iSession, &aPacket.HostName(), &mode, aPacket.iNext);
			break;
		default:
			Log::Printf(_L("\tresolver[%d] SESSION %d Bad TDnsMessagePacket%S(%d,%d)"), iId, aPacket.iSession, &mode, aPacket.iType, aPacket.iNext);
			break;
		}
	}
#endif

// CDndResolver::Stop
// ******************
void CDndResolver::Stop()
	/**
	* Stop serving the current request.
	*
	* Terminate all pending activity (if any) and mark the
	* CDndResolver instace available for a new session.
	*/
	{
	iReadyForAnswer = 0;		// ...just to be safe.
	StopSource();				// stop all DNS activity related to this client, if any
	if (iQueryActive)
		{
		iQueryActive = 0;
        iDnsclient.QueryEnd();
		}
	LOG(Log::Printf(_L("\tresolver[%d] SESSION %d Stopped"), iId, iCurrentQuery().iSession));
	iCurrentQuery().iSession = 0;
	iQueryDoneWait = 0;
	}


// CDndResolver::WaitQuery
// ***********************
void CDndResolver::WaitQuery()
	/**
	* Put the CDndResolver to wait a next request from the RHostResolver.
	*
	* The next request could be a Next, or a new GetByName, GetByAddr or
	* Query.
	*/
	{
	iReadyForAnswer = 0;
	if (iQueryActive)
		{
		// There could be active sessions
		// doing some queries while we wait for
		// client to request something (doing
		// parallel queries).
		// Leave "query active" on, if any found.
		// The information is retrieved during this
		// time is/will be available only for
		// the Next() query.
		for (TInt i = 0;; ++i)
			{
			if (i == iSessionCount)
				{
				iQueryActive = 0;
				iDnsclient.QueryEnd();
				break;
				}
			if (iSession[i].iStatus == KRequestPending)
				break;
			};
	  }
	iQueryDoneWait = 0;
	}

// CDndResolver::QueryDone
// ***********************
void CDndResolver::QueryDone(TInt aResult)
	/**
	* A query has completed, return a result to the RHostResolver.
	*
	* Prepare for the reply into iBuffer for writing to the
	* "gateway socket". When the reply sender of the listener
	* is ready to start the transfer, it calls ReplyMessage()
	* for the buffer to be sent.
	*
	* @param	aResult
	*	the result code of the query. If KErrNone, the buffer
	*	will contain one answer (TNameRecord or some
	*	TDnsQryRespBase variant).
	*
	* If the reply is an error (< 0), this stops all resolving
	* activity that might be still going on. An error reply is
	* always final, continuation with a Next() is not possible.
	*/
	{
	if (!iReadyForAnswer)
		return;		// Gateway is not expecting answer now...
	iReadyForAnswer = 0;
	iQueryDoneWait = 1;
	iBuffer().iNext = aResult;
	if (aResult < 0)
		{
		StopSource();
		iBuffer.SetLength(iBuffer().HeaderSize());
		}
	LOG(ShowQuery(iBuffer(), 1));
	iListener.PostReply();	
	}

// CDndResolver::ReplyMessage
// **************************
const TDesC8 &CDndResolver::ReplyMessage()
	/**
	* Return the reply message buffer.
	*
	* Called by listener sender when it is ready to send the
	* reply message to the gateway socket. If this resolver
	* has a reply to send, this must return the buffer.
	* If no reply is to be sent for this resolver, this must
	* return an empty buffer reference.
	*
	* @return The reply buffer (empty = no reply to send).
	*/
	{
	_LIT8(KNoReply, "");
	if (iQueryDoneWait)
		{
		const TInt result = iBuffer().iNext;
		// Only set iQueryComplete, *never* clear it, if already set!
		if (result != KErrCompletion && result < 0)
			{
			iQueryComplete = 1;
			}
		iQueryDoneWait = 0;
		return iBuffer;
		}
	else
		return KNoReply;
	}

// CDndResolver::Session
// *********************
TUint16 CDndResolver::Session() const
	/**
	* Return the current session id.
	*
	* This function is used by the listener to find out the
	* session id of the request that is currently being
	* processed by this resolver.
	*
	* If the resolver is idle (no request under process), then
	* this must return 0. All valid session ids are non-zero.
	*
	* @return The session (or 0, if none).
	*/
	{
	return iCurrentQuery().iSession;
	}

void CDndResolver::GetNextAnswer()
	/**
	* Collect, sort and return answers to the application.
	*
	* Collecting answers from DNS. The answers are sorted into
	* three classes:
	*
	* @li (1)
	*	usable addresses (current system has a route and source address for them)
	* @li (2)
	*	addresses, for which no route exists are put into "TOP" category (if application
	*	uses one of these, then most likely, a new interface needs to be activated). These
	*	are returned after all usable addresses in class (1) have been returned..
	* @li (3)
	*	other (non-address) answers to the query
	*
	* If a GetByName request needs two queries (for example A and AAAA),
	* the queries may be performed in parallel or one after the other.
	* If queries are performed one at time, this method will automaticly
	* start the second query, after the answers (if any) from the first query
	* have been processed.
	*
	* If a query produces any answer belonging to the class (1), they are
	* returned to the application as soon as they become available.
	* If queries are not done in parallel, and application is satisfied
	* with the first answer (does not issue Next()), then the second
	* query is never sent.
	*
	* If a query produces no answers that sort into class (1), both queries
	* are always executed fully, before any answers are returned (from class
	* (2) or (3)) to the application.
	*/
	{
	if (!iReadyForAnswer)
		return;		// Gateway not expecting answer now...

	iBuffer = iCurrentQuery;			// Initialize reply to orginal query
	TInt pending = 0;
	TInt notfound = 0;

	// Initialize query buffer flags.
	iBuffer().NameRecord().iFlags = 0;

	if (iPhase == 0)
		{
		//
		// Collect Phase
		//
		for (TInt i = 0; i < iSessionCount; ++i)
			{
			TQuerySession &session = iSession[i];

			if (session.iStatus < 0)
				{
				// Either pending or no results available (all processed or some error state)
				// Detect pending queries (only possible to happen if doing queries in parallel)
				pending |= (session.iStatus == KRequestPending);
				notfound |= (session.iStatus == KErrDndBadName);
				continue;
				}
			for (;;)
				{
				//
				// Values iSession[i].iStatus >= 0 [0..n] is used for remembering
				// the number of the next answer to retrieve from the record
				const TInt ret = session.iSession->DoNext(iBuffer, session.iStatus);
				if (ret != KErrNone)
					{
					// All available answers have been retrieved.
					session.iStatus = ret;
					break;
					}
				// NOTE: session.iStatus++!!!
				const TDeprecatedAnswer answer((TUint8)(i), (TUint8)(session.iStatus++));

				if (iQueryIsSpecial)
					{
					// All answers are returned as is for special query
					// (no sorting or any other processing is done)
					QueryDone(KErrNone);
					return;
					}

				const TInetAddr &addr = TInetAddr::Cast(iBuffer().NameRecord().iAddr);
				if (addr.IsUnspecified() || (addr.Family() != KAfInet && addr.Family() != KAfInet6))
					{
					// ...things like CNAME's fall into here. If running out of
					// space, then just ignore these (Top entries are more important)
					if (iDeprecatedTop < iDeprecatedBottom)
						{
						iDeprecate[--iDeprecatedBottom] = answer;
						}
					}
				else if (iDnsclient.CheckAddress(addr) != KErrNone)
					{
					// There is no route for the address, don't return this yet
					if (iDeprecatedTop < KMaxDeprecated)
						{
						iDeprecate[iDeprecatedTop++] = answer;
						// If running out of space, overwrite deprecated bottom entries
						// as needed...
						if (iDeprecatedTop > iDeprecatedBottom)
							iDeprecatedBottom = iDeprecatedTop;
						}
					}
				else
					{
					// This is a usable address as is, return immediately
					QueryDone(KErrNone);
					return;
					}
				}
			}
		//
		// RR records from the current query have been processed.
		//
		if (pending)
			{
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d Request(s) pending"), iId, iCurrentQuery().iSession));
			// At least one query session is still in progress, continue waiting
			// Should set some timeout? what?
			return;
			}
		else if (!notfound && iStepCount < iQuerySteps)
			{
			// ... but there are alternate steps left, try the next step...
			//
			// If all DNS servers worked right, there should be no reason to probe
			// servers with A, if the probe for AAAA fails. However, sadly, there
			// broken things out there, which timeout on AAAA, but do reply to A.
			// Thus, if probing is active, restart it for new query type.
			if (iProbe.iSession)
				(void)iProbe.iSession->PickDefaultServer();

			TQuerySession &session = iSession[iSessionCount++];
			session.Open();
			if (session.Start(iQueryStep[iStepCount++].iQType))
				return;
			}
		}
	//
	// Return TOP deprecated
	//
	while (iPhase < iDeprecatedTop)
		{
		const MDnsSession *session = iSession[iDeprecate[iPhase].iStep].iSession;
		const TInt ret = session->DoNext(iBuffer, iDeprecate[iPhase].iIndex);
		// There is no route for the address, add info for application.
		iBuffer().NameRecord().iFlags |= EDnsNoRoute;		
		iPhase++;
		if (ret == KErrNone)
			{
			QueryDone(KErrNone);
			return;
			}
		}
	//
	// Return BOTTOM deprecated
	// (only for GetByAddr or for Next() after GetByName)
	//
	iPhase = KMaxDeprecated;
	if (iQueryIsAddr || iNext)
		{
		while (iDeprecatedBottom < KMaxDeprecated)
			{
			const MDnsSession *session = iSession[iDeprecate[iDeprecatedBottom].iStep].iSession;
			const TInt ret = session->DoNext(iBuffer, iDeprecate[iDeprecatedBottom].iIndex);
			iDeprecatedBottom++;
			if (ret == KErrNone)
				{
				QueryDone(KErrNone);
				return;
				}
			}
		}
	if (iNext)
		QueryDone(iQueryIsAddr ? KErrDndAddrNotFound : KErrDndNameNotFound);
	else
		// No answers returned from current source, try next
		NextSource();
	}

// CDndResolver::ProbeServers
// **************************
void CDndResolver::ProbeServers()
	/**
	* Probing servers.
	*
	* Send a dummy duplicate query to the next alternate
	* server. Used in some resolver modes. The result is ignored
	* by resolver, but the side effect is that the DNS protocol
	* handler will detect a good server, and make it the default
	* for the next try.
	*/
	{
	if (iSessionCount == 0)
		return;
	if (iProbe.iSession == NULL)
		{
		// Use the first primary session (iSession[0]) as a template
		// for the "probing".
		if (iSession[0].iSession == NULL)
			return;	// ...oops!?
		iProbe.iQType = iSession[0].iQType;
		iProbe.iSession = iDnsclient.OpenSession(&iProbe);
		if (iProbe.iSession == NULL)
			return;
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d Probing QType=%d (%S) -- Assigned DNS session [%u]"),
			iId, (TInt)iCurrentQuery().iSession, (TInt)iProbe.iQType, &iProbe.iName, (TInt)iProbe.iSession->Instance())); 
		// Use the current query as a probe starting point
		//(void)iProbe.iSession->NewQuery(*iSession[0].iSession);
		(void)iProbe.iSession->NewQuery(iCurrentQuery(),iSourceNow,iQueryFlags | KDnsModifier_PQ);
		
		// initialize server (which is not tried)
		(void)iProbe.iSession->PickDefaultServer();
		}
	// Start query, but only if there really is another server address
	if (iProbe.iSession->PickNewServer())
		{
		// Note: if DoQueryL fails, we do nothing. This is just probing...
		TRAP_IGNORE(iProbe.iSession->DoQueryL(iRequestTime, iProbe.iQType));
		}
	}


// CDndResolver::StopSource
// ************************
void CDndResolver::StopSource()
	/**
	* Stop all resolving activity from current source.
	*
	* The answer can be searched from different sources (such as hosts file,
	* global DNS or link local name resolution). Answers from different sources are
	* never mixed, and the first source that gives an answer is used.
	*
	* StopSource() cancels all activity (if any) from current source.
	*/
	{
	// Release sessions, if they were created
	while (iSessionCount > 0)
		iSession[--iSessionCount].Close();
	iProbe.Close();
	}


// CDndResolver::NextSource
// ************************
void CDndResolver::NextSource()
	/**
	* Activate search from next source.
	*
	* The answer can be searched from different sources (such as hosts file,
	* global DNS or link local name resolution). Answers from different sources are
	* never mixed, and the first source that gives an answer is used.
	*
	* NextSource activates the next query source or terminates the
	* query, if all sources have been tried.
	*/
	{

	while (iSourceCount > 0)
		{
		StopSource();	// Cleanup from previous source, if any

		// Replies from different source are never mixed
		// Restart collecting answers too.
		iPhase = 0;
		iDeprecatedTop = 0;
		iDeprecatedBottom = KMaxDeprecated;
		iSourceNow = *iSourceList++;
		iSourceCount -= 1;

		// Is this source valid for the query scope (e.g. if source is global
		// DNS, one should not try to query site or link local queries from it!)
		// On the other hand, it is legal to make global scope query to link
		// local or site scope servers.
		const TUint level = (TUint)Abs((TInt)iSourceNow);
		if (iQueryScope < level)
			continue;	// Query scope is less than servers scope, try another source!

		if (StartSource())
			return;
		}

	// The name could not be resolved by any source
	QueryDone(iQueryIsAddr ? KErrDndAddrNotFound : KErrDndNameNotFound);
	}


// CDndResolver::StartSource
// *************************
TBool CDndResolver::StartSource()
	/**
	* Initiate a source.
	*
	* Start/Activate retrieval of query answers from the current source,
	* as defined by CDndResolver::iSourceNow. Three classes of
	* sources are supported:
	*
	* @li	HOST file (= 0)
	* @li	Unicast DNS server ( < 0)
	* @li	Multicast name resolution service ( > 0)
	*
	* The absolute value of iSourceNow is the scope
	* level of the name resolution source (2 = link local, etc..)  
	*
	* @returns
	*	@li	TRUE, if process has been started (or answer already resolved).
	*	@li	FALSE, if the source is not available or usable
	*/
	{
	// If you define more complex queries, needing more than current max of
	// steps, then bump up the KMaxQuerySessions! Panic on debug, on release
	// just ignore the extra steps.
	ASSERT(iQuerySteps <= KMaxQuerySessions);
	if (iQuerySteps > KMaxQuerySessions)
		iQuerySteps = KMaxQuerySessions;

	
	const TDndConfigParameters &cf = iControl.GetConfig();

	TUint query_order = cf.iQueryOrder;

	if (iSourceNow == 0)
		{
		// HOST file or hostname source
		if (!iQueryIsSpecial)
			{
			// Check if the query is for local host name or my own address.
			TInt res;
			if (iQueryIsAddr)
				res = iDnsclient.GetByAddress(iBuffer().iId, TInetAddr::Cast(iBuffer().NameRecord().iAddr), iNext, iBuffer().NameRecord().iName);
			else
				res = iDnsclient.GetByName(iBuffer().iId, iBuffer().NameRecord().iName, iNext, TInetAddr::Cast(iBuffer().NameRecord().iAddr));
			if (res == KErrNone)
				{
				LOG(Log::Printf(_L("\tresolver[%d] SESSION %d matched local hostname/address"), iId, iCurrentQuery().iSession));
				iBuffer().NameRecord().iFlags = EDnsHostName;
				QueryDone(KErrNone);
				return TRUE;
				}
			// ..and now the HOSTS file
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d SEARCHING hosts file"), iId, iCurrentQuery().iSession));
			TRequestStatus tmp;
			if (iQueryIsAddr)
				iControl.iHostsFile.GetByAddress(iBuffer().NameRecord(), tmp);
			else
				iControl.iHostsFile.GetByName(iBuffer().NameRecord(), tmp);
			if (tmp.Int() == KErrNone)
				{
				iBuffer().NameRecord().iFlags = EDnsHostsFile;
				QueryDone(KErrNone);
				return TRUE;
				}
			}
	
		if (iBuffer().iId == 0)
			{
			// If there is no scope is specified, then only hosts file
			// is looked.
			QueryDone(KErrCompletion);
			return TRUE;
			}
		// No Answer from the host file, move on to the next
		return FALSE;
		}
	else if (iSourceNow < 0)
		{
		//
		// Unicast DNS source
		//
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d SEARCHING unicast DNS"), iId, iCurrentQuery().iSession));
		// Setup parameters for basic query step (steps are done one after another)
		iQueryStepMaxTime = Max(cf.iMinTime, cf.iMaxTime / iQuerySteps);
		iQueryStepMinTime = cf.iMinTime;
		iQueryStepRetries = (TUint8)cf.iRetries;
		}
	else
		{
		//
		// Multicast DNS source
		//
		// Setup parameters for basic query step 
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d SEARCHING multicast DNS"), iId, iCurrentQuery().iSession));
		query_order |= KQueryOrder_Parallel;	// Multicast steps are always done in parallel
#ifdef LLMNR_ENABLED
		// Assumed: "multicast DNS" == LLMNR!
		iQueryStepMaxTime = Max(cf.iLlmnrMinTime, cf.iLlmnrMaxTime);
		iQueryStepMinTime = cf.iLlmnrMinTime;
		iQueryStepRetries = (TUint8)cf.iLlmnrRetries;
#else
		iQueryStepMaxTime = Max(cf.iMinTime, cf.iMaxTime);
		iQueryStepMinTime = cf.iMinTime;
		iQueryStepRetries = (TUint8)cf.iRetries;
#endif
		}
	//
	// Start the query/queries
	//
	TBool started = FALSE;
	iSessionCount = 0;
	iStepCount = 0;
	while (iStepCount < iQuerySteps)
		{
		TQuerySession &session = iSession[iSessionCount++];
		session.Open();
		if (session.iSession == NULL)
			{
			// If the required session cannot be allocated,
			// abort the request totally.
			QueryDone(session.iStatus);
			return TRUE;
			}
		started |= session.Start(iQueryStep[iStepCount++].iQType);
		if (started && (query_order & KQueryOrder_Parallel) == 0)
			break;
		}
	return started;
	}

// CDndResolver::HOSTFILE_Next
// ***************************
void CDndResolver::HOSTFILE_Next()
	/**
	* Return next answer from HOST file.
	*
	* The HOST file answers are not currently sorted in any way. The
	* answers are returned as is, in whatever order the host file
	* module returns them.
	*
	* Only TNameRecord based queries are asked from the HOST file
	* (e.g. the standart RHostResolver::GetByName and RHostResolver::GetByAddr).
	* No special queries.
	*/
	{
	// Should never get called with special queries, but if that
	// happens, just return KErrEof as a safety measure...
	TRequestStatus tmp(KErrEof);
	if (!iQueryIsSpecial)
		{
		// Temp. kludge: next indicator for hosts file reader is passed in iFlags!
		iBuffer().NameRecord().iFlags = iNext;
		if (iQueryIsAddr)
			iControl.iHostsFile.GetByAddress(iBuffer().NameRecord(), tmp);
		else
			iControl.iHostsFile.GetByName(iBuffer().NameRecord(), tmp);
		// Note: iHostFile returns the result code in TRequestStatus.
		iBuffer().NameRecord().iFlags = EDnsHostsFile;
		}
	QueryDone(tmp.Int());
	}

// CDndResolver::Start
// *******************
void CDndResolver::Start(const TDnsMessageBuf &aMsg)
	/**
	* Process a new request from listener/RHostResolver.
	*
	* @param aMsg The request message
	*/
	{
	const TDndConfigParameters &cf = iControl.GetConfig();

	// Sanity checks -- determine the min size of a proper request
	const TInt min_size = aMsg().HeaderSize() +
		TInt(aMsg().iType == KDnsRequestType_TDnsQuery ? sizeof(TDnsQuery) :
			 aMsg().iType == KDnsRequestType_GetByName ? sizeof(TNameRecord) :
			 aMsg().iType == KDnsRequestType_GetByAddress ? sizeof(TNameRecord) :
			 sizeof(THostName));
	if (aMsg.Length() < min_size) // If short query => treat as cancel!
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d Cancelled by client (len=%d)"), iId, iCurrentQuery().iSession,  aMsg.Length()));
		Stop();
		return;
		}

	// Stack should never start new request before delivering the response.
	// Exception is the Cancel-request which is treated above.
	ASSERT(iQueryDoneWait == 0);
	
	iBuffer = aMsg;
	//
	// A new command/query from the application
	//
	LOG(ShowQuery(iBuffer(), 0));

	// If query is not active, it will be after this
	if (iQueryActive == 0)
		{
		iQueryActive = 1;
		iDnsclient.QueryBegin();
		}

	iReadyForAnswer = 1;			// an answer can now be returned...
	iRequestTime.UniversalTime();	// Set the time of request

	iNext = iBuffer().iNext;
	if (iNext)
		{
		// This is Next() processing, retrieving
		// the additional records.
		if (iQueryComplete)
			{
			// An application/something is stubbornly calling Next() after
			// receiving an error (query complete) or the query was "one shot"
			// that returned "KErrNone", and was complete at that (no Next).
			// This error should terminate the Next() loop in app!
			QueryDone(KErrNotFound);
			return;
			}
		iBuffer = iCurrentQuery;	// Reset to orginal query (Next() cannot change the query)
		if (iSourceNow == 0)
			HOSTFILE_Next();
		else
			GetNextAnswer();
		}
	else
		{
		//
		// Start processing of new query, not Next()
		//
		iQueryComplete = 0;
		iCurrentQuery = iBuffer;		// Remember current original query
		iQueryFlags = KDnsModifier_RD;	// By default, request recursive querying from the server(s).
		StopSource();
		switch (iBuffer().iType)
			{
			case KDnsRequestType_GetByName:
			case KDnsRequestType_GetByAddress:
				{
				TNameRecord &rec = iCurrentQuery().NameRecord();

				if (iCurrentQuery().iType == KDnsRequestType_GetByAddress)
					{
					iQueryIsAddr = 1;
					// The scope of GetByAddress query is determined from the scope level
					// of the address being queried.
					iQueryScope = (TUint8)TInetAddr::Cast(rec.iAddr).Ip6Address().Scope();
					iQueryIsSpecial = 0;
					iQueryStep = KGetByAddress;
					iQuerySteps = sizeof(KGetByAddress) / sizeof(KGetByAddress[0]);
					}
				else
					{
					iQueryIsAddr = 0;
					// All getbyname queries are global for now. As a future feature
					// It might be considered, that single label or names ending with
					// ".local" would have have local scope -- msa
					iQueryScope = KIp6AddrScopeNetwork;

#ifdef DND_DCM_EXTENSION
					iQueryIsSpecial = 0;
					TInt i;
					while ((i = rec.iName.Locate('?')) >= 0)
						{
						const TPtrC prefix = rec.iName.Left(i+1);
						if (prefix.Compare(KDnsNonrecursive) == 0)
							{
							iQueryFlags &= ~KDnsModifier_RD;	// Reset recursive flag.
							}
						else if (cf.iQueryHack)
							{
							// Special Query
							iQueryIsSpecial = 1;
							TInt q = sizeof(KQueryExtension) / sizeof(KQueryExtension[0]);
							while (--q >= 0 && prefix.Compare(*KQueryExtension[q].iPrefix) != 0)
								/* NOTHING */;
							if (q < 0)
								{
								// Requested query is not supported
								QueryDone(KErrNotSupported);
								return;
								}
							iQueryStep = &KQueryExtension[q].iQuery;
							iQuerySteps = 1;
							}
						else
							{
							QueryDone(KErrNotSupported);
							return;
							}
						rec.iName.Delete(0,i+1);
						}

					if (!iQueryIsSpecial)
						{
						// Standard GetByName
						if ((cf.iQueryOrder & KQueryOrder_PreferA) == 0)
							{
							iQueryStep = KGetByName1;
							iQuerySteps = sizeof(KGetByName1) / sizeof(KGetByName1[0]);
							}
						else
							{
							iQueryStep = KGetByName2;
							iQuerySteps = sizeof(KGetByName2) / sizeof(KGetByName2[0]);
							}
						// If query is to be limited to only one (either A or AAAA), do
						// the preferred query only (the first in the chosen list).
						if (cf.iQueryOrder & KQueryOrder_OneQuery)
							iQuerySteps = 1;
						}
#else
					const TInt i = rec.iName.Locate('?');

					if (i < 0)
						{
						// Standard GetByName
						iQueryIsSpecial = 0;
						if ((cf.iQueryOrder & KQueryOrder_PreferA) == 0)
							{
							iQueryStep = KGetByName1;
							iQuerySteps = sizeof(KGetByName1) / sizeof(KGetByName1[0]);
							}
						else
							{
							iQueryStep = KGetByName2;
							iQuerySteps = sizeof(KGetByName2) / sizeof(KGetByName2[0]);
							}
						// If query is to be limited to only one (either A or AAAA), do
						// the preferred query only (the first in the chosen list).
						if (cf.iQueryOrder & KQueryOrder_OneQuery)
							iQuerySteps = 1;
						}
					else if (cf.iQueryHack)
						{
						// Special Query
						iQueryIsSpecial = 1;
						const TPtrC prefix = rec.iName.Left(i+1);
						TInt q = sizeof(KQueryExtension) / sizeof(KQueryExtension[0]);
						while (--q >= 0 && prefix.Compare(*KQueryExtension[q].iPrefix) != 0)
							/* NOTHING */;
						rec.iName.Delete(0,i+1);
						if (q < 0)
							{
							// Requested query is not supported
							QueryDone(KErrNotSupported);
							return;
							}
						iQueryStep = &KQueryExtension[q].iQuery;
						iQuerySteps = 1;
						}
					else
						{
						QueryDone(KErrNotSupported);
						return;
						}
#endif
					}
				}
				break;
			case KDnsRequestType_TDnsQuery:
				// Special Query
				iQueryIsSpecial = 1;
				iQueryIsAddr = 0;
				iQueryScope = KIp6AddrScopeNetwork;
				iSpecificQuery.iQType = EDnsQType(iBuffer().Query().Type());
				iQueryStep = &iSpecificQuery;
				iQuerySteps = 1;
#ifdef DND_DCM_EXTENSION
				if (iBuffer().Query().Type() == KDnsQTypeCacheClear)
					{
					TRAPD(err, iListener.HandleCommandL(EDndFlush));
					iQueryComplete = 1;	// "Next()" is not allowed!
					QueryDone(err);
					return;							
					}
#endif
				break;
			case KDnsRequestType_SetHostName:
				iQueryComplete = 1;	// "Next()" is not allowed (only needed for ret == KErrNone case)
				QueryDone(iDnsclient.SetHostName(iBuffer().iId, iBuffer().HostName()));
				return;
			case KDnsRequestType_GetHostName:
				{
				const TInt ret = iDnsclient.GetHostName(iBuffer().iId, iBuffer().HostName(), *this);
				if (ret <=  0)
					{
					iQueryComplete = 1;	// "Next()" is not allowed (only needed for ret == KErrNone case)
					QueryDone(ret);		// GetHostName completed, we are done!
					}
				// GetHostName is pending on uniquenes test (ret == 1)
				return;
				}
			default:
				QueryDone(KErrNotSupported);
				return;
			}
		//
		// Setup NS source (now fixed, make configurable later)
		//
		iSourceList = KSourceOrder;
		iSourceCount = sizeof(KSourceOrder) / sizeof(KSourceOrder[0]);
		NextSource();
		}
	}

// Only used for GetHostName
void CDndResolver::ReplyCallback(const TInt aResult)
	{
	iBuffer = iCurrentQuery;
	QueryDone(aResult);
	}

//
//

TQuerySession::TQuerySession() : iTimeout(TQuerySessionTimeoutLinkage::Timeout)
	{
	}

// TQuerySession::Init
// *******************
void TQuerySession::Init(CDndResolver *const aResolver)
	/**
	* Initialize structure.
	*
	* A separate Init is required, because arrays cannot have non-defaulf
	* constructors. Need to patch the iResolver link "manually".
	*/
	{
	iResolver = aResolver;
	// ...for other members, the default constructor is sufficient
	}


// TQuerySession::Open
// *******************
void TQuerySession::Open()
	/**
	* Activate a query session.
	*
	* Assume CDndResolver::iCurrentQuery holds the request from the client.
	* Establish a session with the DNS protocol provides and initialize
	* it with the current question (NewQuery).
	*/
	{
	iSession = iResolver->iDnsclient.OpenSession(this);
	if (iSession == NULL)
		{
		// Cannot allocate a session
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- OUT OF DNS SESSIONS!"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		iStatus = KErrNoMemory;
		}
	else
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- Assigned DNS session [%u]"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, (TInt)iSession->Instance()));
		// ..if NewQuery returns an error, it will be the result of the query. Otherwise
		// use the KErrEof as initial value.
		iStatus = iSession->NewQuery(iResolver->iCurrentQuery(), iResolver->iSourceNow, iResolver->iQueryFlags);
		if (iStatus == KErrNone)
			iStatus = KErrEof;	// No content yet.
#ifdef SYMBIAN_DNS_PUNYCODE
		else // in case the Query String is UTF-16 encoded
			{
			iResolver->QueryDone(iStatus);
			}
#endif //SYMBIAN_DNS_PUNYCODE
		}
	}

// TQuerySession::SetTimer
// ***********************
void TQuerySession::SetTimer(TUint aTimeout)
	/**
	* Request a Timeout() call after the specied number of seconds.
	*/
	{
	LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- set timeout = %ds"),
		iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, (TInt)aTimeout));
	iResolver->iControl.Timer().Set(iTimeout, aTimeout);
	}

// TQuerySession::Start
// ********************
TBool TQuerySession::Start(EDnsQType aQType)
	/**
	* Start the query process for a specific query type.
	*
	* @param aQType The query type (A, AAAA, MX, etc.)
	*/
	{
	iQType = aQType;
	LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- Start"),
		iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
	iServerCount = 1;
	iQueryRetry = 0;
	iQueryTimeouts = 0;

	if (!iSession)
		return FALSE;
	
	iStatus = KErrEof;

	if (iSession->PickDefaultServer())
		{
		SendDnsQuery();
		return TRUE;
		}
	return FALSE;
	}

// TQuerySession::Cancel
// *********************
void TQuerySession::Cancel()
	/**
	* Cancel and stop all activity (if any) with this session.
	*
	* This cancels set timers and any DNS queries that are in
	* progress.
	*/
	{
	iTimeout.Cancel();

	if (!iSession)
		return;

	iSession->CancelQuery();
	if (iStatus == KRequestPending)
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- CANCELED"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		iStatus = KErrCancel;
		}
	}


// TQuerySession::Close
// ********************
void TQuerySession::Close()
	/**
	* Close session.
	*
	* First calls Cancel() to call all activity, and then also
	* closes the session with DNS protocol provider.
	*/
	{
	iPendingQuery = 0;	// Cancel pending queries too!
	Cancel();
	if (!iSession)
		return;
	iSession->Close();
	LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- DNS [%u] closed"),
		iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, (TInt)iSession));
	iSession = NULL;
	}


// TQuerySession::RecvReply
// ***********************
void TQuerySession::ReplyCallback(TInt aErr)
	/**
	* Callback from DNS client.
	*
	* DNS client has completed something.
	*
	* @param aErr The completion result or some notify.
	*/
	{
	if (iStatus != KRequestPending)
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Ignore event"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
		return;	// Unexpected event, ignore!
		}

	if (aErr > 0)
		{
		// Notify Messages, just ignore if not recognised.
		if (aErr == KDnsNotify_QUERY_SENT)
			{
			if (iWaitingQuerySent)
				{
				iWaitingQuerySent = 0;	// got it...
				LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Query Sent"),
					iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
				SetTimer(iQueryTimeoutValue);
				}
			}
		else if (aErr == KDnsNotify_HAVE_SERVERLIST)
			{
			// Currently does nothing with this.
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Has serverlist"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
			}
		else if (aErr == KDnsNotify_USING_TCP)
			{
			iQueryRetry = iResolver->iQueryStepRetries;
			iQueryTimeoutValue = iResolver->iQueryStepMaxTime;
			iWaitingQuerySent = 0;
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Query using TCP, disable retransmit"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
			SetTimer(iQueryTimeoutValue);
			}
		}
	else if (aErr != KErrNone && (aErr > KErrDndNameNotFound || aErr < KErrDndServerUnusable))
		{
		// The error is not query specific DND error code, assume it is
		// something general and abort the query totally.
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Query completed"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
		iResolver->QueryDone(aErr);
		}
	else
		{
		iStatus = aErr;
		iTimeout.Cancel();
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- RecvReply(%d) Handle event"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, aErr));
		Event(FALSE);
		}
	}

// TQuerySession::Timeout
// **********************
void TQuerySession::Timeout(const TTime &aNow)
	/**
	* No response from the DNS handler within specified time, handle timeout.
	*
	* @param aNow The current time
	*/
	{
	LOG(Log::Printf(_L("-->\tresolver[%d] SESSION %d QType=%d (%S) -- timeout start"),
		iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
	if (iWaitingQuerySent)
		{
		//
		// Don't wait forever, abort resolving if max setup time exceeded
		//
		const TDndConfigParameters &cf = iResolver->iControl.GetConfig();
		TTimeIntervalSeconds seconds;
		if (aNow.SecondsFrom(iResolver->iRequestTime, seconds) != KErrNone ||
			seconds.Int() > STATIC_CAST(TInt, cf.iSetupTime))
			{
			iStatus = KErrDndServerUnusable;
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- Timeout, max wait expired [%d > %d]: %d"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, seconds.Int(), cf.iSetupTime, iStatus));
			}
		else
			{
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- Timeout, has waited %ds [max %d]"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName, seconds.Int(), cf.iSetupTime));
			}
		}
	else
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- timeout"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		}
	// Note that Timeout alone does not touch the iStatus, nor does it
	// cancel the request, if any is active. It's up to Event to decide
	// on these issues.
	Event(TRUE);
	LOG(Log::Printf(_L("<--\tresolver[%d] SESSION %d QType=%d (%S) -- timeout exit"),
		iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
	}


// TQuerySession::Event
// ********************
void TQuerySession::Event(TBool aTimeOut)
	/**
	* Shared handling of session events: timeouts and DNS query completions.
	*
	* @param aTimeOut is TRUE, if this is a timeout event; and FALSE, if
	*			event caused by DNS completion.
	*/
	{
	const TDndConfigParameters &cf = iResolver->iControl.GetConfig();

	// Handle events from DNS or timeout
	// *********************************
	// TIMEOUT or REPLY to a pending query has been received
	//
	const TBool server_unusable = (iStatus == KErrDndRefused || iStatus == KErrDndServerUnusable);
	if (!aTimeOut && !server_unusable)
		{
		// This query has been completed with some definite result, notify
		// the main resolver to handle the reply.
		//
		iResolver->GetNextAnswer();
		return;
		}


	if (iWaitingQuerySent && !server_unusable)
		{
		// Still waiting for the first query packet to be sent. Request a
		// resend attempt of the same query again (without counting it as
		// retry). This is a wait for getting interfaces up (normal timeouts
		// do not apply yet).

		// Reload the query too (this is only sometimes needed with PTR
		// queries if the filter iLockId cannot be determined due to
		// missing interfaces. And only happens if query address was
		// without scope id.
		(void)iSession->NewQuery(iResolver->iCurrentQuery(), iResolver->iSourceNow, iResolver->iQueryFlags);
		SendDnsQuery();
		return;
		}

	if (aTimeOut)
		iQueryTimeouts++;	// Timeout occurred while resolving the DNS Query


	// Careful with iStatus: if timeout occurs, it is probably KRequestPending.
	// Needs to be changed to something else, if GetNextAnswer is called (otherwise
	// it will think that there are pending query and may do nothing!)
	//
	if (cf.iSprayMode || iResolver->iSourceNow > 0 /* == multicast resolution mode */)
		{
		if (iWaitingAfterSpray)
			{
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- Delay after spray completed"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
			//
			// The delay after "spray" has expired, now start a new round (a retry)
			// (unless maximum retries has been reached)
			//
			iWaitingAfterSpray = 0;
			iQueryTimeouts = 0;
			(void)iSession->PickDefaultServer();
			iStatus = KErrTimedOut;
			iServerCount = 1;
			++iQueryRetry;
			if (iQueryRetry > iResolver->iQueryStepRetries)
				iResolver->GetNextAnswer();
			else
				SendDnsQuery();
			}
		else if (iSession->PickNewServer())
			{
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- More servers to spray"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
			//
			// Yet another server to spray, replicate query
			//
			iWaitingAfterSpray = 0;
			++iServerCount;
			SendDnsQuery();	// Replicate query to another server
			}
		else if (iQueryTimeouts > 0)
			{
			// All servers have been sprayed and at least one of them
			// has not answered, now wait maxtime - (servers * mintime),
			// before starting the next spraying round
			//
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- All servers sprayed, now wait results"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
			iWaitingAfterSpray = 1;
			const TInt maxtime = Max(iResolver->iQueryStepMinTime, iResolver->iQueryStepMaxTime / (iResolver->iQueryStepRetries+1));
			SetTimer(Max(iResolver->iQueryStepMinTime, maxtime - iResolver->iQueryStepMinTime * iServerCount));
			}
		else
			{
			LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- All servers replied not found"),
				iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
			// All servers sprayed, and none of the short spray timeouts expired
			// (make a heuristic GUESS, that all servers replied, but all were either
			// unusable or "skip not found" is active. Just goto to the next step or
			// return not found.
			iResolver->GetNextAnswer();
			}
		}
	else
		{
		++iQueryRetry;	// This is a true retry
		if (iQueryRetry > iResolver->iQueryStepRetries ||
			(server_unusable && !iSession->PickNewServer()))
			{
			iStatus = KErrTimedOut;
			iResolver->GetNextAnswer();// Activates next query step or returns not found.
			}
		else
			{
			iResolver->ProbeServers();	// ...keep probing for alternate servers in parallel (at each timeout)
			SendDnsQuery();
			}
		}
	}


// TQuerySession::SendDnsQuery
// ***************************
void TQuerySession::SendDnsQuery()
	/**
	* Activate a new DNS query to be sent.
	*/
	{
	if (iSendingQuery)
		{
		// Doing everyhing in callbacks forces extra work
		// to be done to prevent deep recursion...
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- NEW QUERY PENDING"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));

		iPendingQuery = 1;
		return;
		}
	// Create an UDP message to the DNS
	do
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- SENDING DNS QUERY"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		iStatus = KRequestPending;
		iSendingQuery = 1;		// ..to detect dangerous recursion
		iPendingQuery = 0;		// ..to detect that a new query should be sent
		iWaitingQuerySent = 1; // Mark: waiting for QUERY_SENT
		TRAPD(err, (void)iSession->DoQueryL(iResolver->iRequestTime, iQType));
		iSendingQuery = 0;
		if (err != KErrNone)
			{
			// Assume some serious error situation and forget about
			// possible pending query, cancel everything with error.
			iResolver->QueryDone(err);
			return;
			}
		}
	while (iPendingQuery);

	if (iStatus == KRequestPending)
		{
		const TDndConfigParameters &cf = iResolver->iControl.GetConfig();
		const TUint maxtime = iResolver->iQueryStepMaxTime;

		// Compute the timeout for the next query (starts ticking
		// when the packet has been sent (after QUERY_SENT)
		//
		if (cf.iSprayMode || iResolver->iSourceNow > 0)
			iQueryTimeoutValue = iResolver->iQueryStepMinTime;	// Send to all servers using mintime 
		else if (cf.iRetries == 0)
			iQueryTimeoutValue = maxtime;		// No retries, just single query
		else if (iQueryRetry == 0)
			iQueryTimeoutValue = iResolver->iQueryStepMinTime;	// This is the first send -- use min time
		else
			{
			// This a retransmission (divide the remaining time even with the retries)
			iQueryTimeoutValue = Max(iResolver->iQueryStepMinTime, (maxtime - iResolver->iQueryStepMinTime) / iResolver->iQueryStepRetries);
			}
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- WAITING FOR SEND"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		// Setup "poll" timer, if the
		// DNS side does not send the QUERY_SENT notification.
		// (retry count is not incremented, so it will "poll"
		// as long as notification/reply arrives or user cancels
		// the resolving).
		if (iWaitingQuerySent)
			SetTimer(cf.iSetupPoll);
		}
	else
		{
		LOG(Log::Printf(_L("\tresolver[%d] SESSION %d QType=%d (%S) -- COMPLETED WITHIN"),
			iResolver->iId, iResolver->iCurrentQuery().iSession, (TInt)iQType, &iName));
		}
	}
