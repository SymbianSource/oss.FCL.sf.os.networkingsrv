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
// res.cpp - name resolver
// This is an implementation of the name service, which is based
// on a external resolver application, and translating the queries
// into socket read/writes.
//



/**
 @file res.cpp
*/

#define SYMBIAN_NETWORKING_UPS

#include <in_sock.h>
#include "res.h"
#include <in_bind.h>	// CProtocolBaseUnbind, MInterfaceManager
#include <timeout.h>
#include <in6_event.h>
#include <in6_opt.h>
#include "iface.h"
#include "inet6log.h"
#include "tcpip_ini.h"
#include "networkinfo.h"
#include "res_sock.h"
#include "inet6err.h"
#include <networking/dnd_err.h>

#	include <comms-infras/nifif.h>	// Need for TSoIfConnectionInfo
#include <es_prot_internal.h>

#ifdef SYMBIAN_DNS_PUNYCODE
#define ENABLEIDN_SCOPE 0x80
#endif //SYMBIAN_DNS_PUNYCODE

_LIT_SECURITY_POLICY_C1(KPolicyNetworkControl,  ECapabilityNetworkControl);
_LIT_SECURITY_POLICY_C1(KPolicyNetworkServices, ECapabilityNetworkServices);

class TDnsRequest : public TDnsRequestBase
	/**
	* The internal representation of a pending query from application/SocketServer.
	*/
	{
public:
	TDnsRequest();
	TDnsRequest(const THostName &aName, TDes &aResponse, TInt aType);
	TDnsRequest(TNameRecord &aQuery, TInt aNext, TInt aType);
	TDnsRequest(const TDesC8 &aQuery, TDes8 &aResponse, TInt aNext);
	void BuildMessage(TUint16 aId, TDes8 &aBuffer) const;

	TBool IsPending() const { return iQuery.Length() > 0; }

	TPtrC8 iQuery;		//< The query buffer
	// Reply data
	union
		{
		TUint8 *iResponse;			//< For initializing.
		TNameRecord *iNameRecord;	//< GetByAddr/GetByName
		TDes8 *iQueryResponse;		//< Query response
		TDes *iHostName;			//< GetHostName/SetHostName
		};
	};

// CProtocolRes
// ************
class CHostResolver;
class CProviderRes;
class CDndSession;
class CProtocolRes : public CProtocolBaseUnbind, public MEventListener
	/**
	* The "resolver protocol".
	*/
	{
	friend class CProviderRes;
	friend class CHostResolver;
public:
	CProtocolRes(CIfManager &aInterfacer);
	virtual ~CProtocolRes();
	virtual CServProviderBase *NewSAPL(TUint aProtocol);
	virtual CHostResolvProvdBase* NewHostResolverL();
	virtual CServiceResolvProvdBase *NewServiceResolverL();
	virtual CNetDBProvdBase *NewNetDatabaseL();
	virtual void Identify(TServerProtocolDesc *) const;
	virtual void BindL(CProtocolBase* protocol, TUint id);
	virtual void Unbind(CProtocolBase* protocol, TUint id);
	virtual void StartL();
	static void FillinInfo(TServerProtocolDesc &anEntry);
	//
	// Resolver specific Methods
	//
	inline MInterfaceManager &Interfacer() const { return iInterfacer; }

	CHostResolver *FindPending() const;
	CProviderRes *NewQuery(CHostResolver *aResolver);
private:
	void CancelSAP(const CProviderRes &aSAP);		// ..for ~CProviderRes() only!
	void CancelQuery(const CHostResolver &aQuery);	// ..for ~CHostResolver() only!
	virtual void Notify(TUint aEventClass, TUint aEventType, const void *aData);

	CProviderRes *iDND;				//< The Domain Name Resolver Daemon (DND)
	CHostResolver *iQueries;		//< List of Queries (RHostResolver sessions)
	CIfManager &iInterfacer;		//< The Interface manager link
	TUint iConfigureDone:1;			//< = 1, when the Configure request has been sent to DND
	};


// CProviderRes
// ************
class CHostResolver;
class CDndSession;
class CProviderRes: public CServProviderBase
	/**
	* The "DND provider" (SAP).
	*/
	{
	friend class CProtocolRes;
public:
	CProviderRes(CProtocolRes &aProtocol);
	~CProviderRes();

	// 'resolver' has no use for local or remote addresses or ports.
	// (these methods should never be called by DND) .None of the Open's are called
	// (resolver is always "connectionless datagram socket"). Just silence the
	// compiler by defining dummy implementations for all those functions.
	virtual void ActiveOpen() {}
	virtual void ActiveOpen(const TDesC8 &) {}
	virtual TInt PassiveOpen(TUint) { return KErrNone; }
	virtual TInt PassiveOpen(TUint ,const TDesC8 &) { return KErrNone; };
	virtual void LocalName(TSockAddr &) const {}
	virtual TInt SetLocalName(TSockAddr &) { return KErrNone; }
	virtual void RemName(TSockAddr &) const {}
	virtual TInt SetRemName(TSockAddr &) { return KErrNone; }
	virtual void AutoBind() {}

	// These are potentially useful, but don't do much currently
	virtual void Shutdown(TCloseType option,const TDesC8 &aDisconnectionData);
	virtual void Shutdown(TCloseType option);
	virtual TInt GetOption(TUint level,TUint name,TDes8 &anOption) const;
	virtual void Ioctl(TUint level,TUint name,TDes8* anOption);
	virtual void CancelIoctl(TUint aLevel, TUint aName);
	virtual TInt SetOption(TUint level,TUint name, const TDesC8 &anOption);

	// The real "beef"...
	virtual TUint Write(const TDesC8 &aDesc,TUint options, TSockAddr* aAddr=NULL);
	virtual void GetData(TDes8 &aDesc,TUint options,TSockAddr *anAddr=NULL);
	virtual void Start();
	TInt SecurityCheck(MProvdSecurityChecker *aChecker);

	// Resolver methods
	void Unlink();							//< Called by iResolver to remove the iResolver
	void Activate();						//< Used when an attached Session needs servicing.
	TBool AssignSession(CHostResolver &aHR);//< Assign a session for the Host Resolver
	CProtocolRes &iProtocol;				//< Protocol instance of the 'resolver'
protected:
	const CDndSession *Find(const TUint16 aId) const;

	CDndSession *iSessions;					//< List of associated sessions.
	TUint16 iSessionId;						//< Last used session id.
	TInt iAvailable;						//< Available sessions in DND.
	};
	
// CDndSession
// ***********
class CDndSession : public CBase
	/**
	* The session assigned to the host resolver.
	*
	* When the DND accepts a host resolver to be served,
	* this class is created to represent the session.
	*
	* Note: This is needed separate from the CHostResolver,
	* because
	* - the destruction of host resolver is controlled by the socket server,
	* - after host resolver has been deleted, the cancel message needs to be delivered to DND
	* - when detached from host resolver, this class represents pending session cancel.
	*/
	{
	friend class CProviderRes;
public:
	void Link(CHostResolver &aHR);		//< Attach Host Resolver to session.
	void Unlink();						//< Detach Host Resolver, if any.
	void Reply(const TDnsMessage &aReply, const TDesC8 &aPayload) const;
	void Submit();
	void Answered();
	CDndSession *Delete();				//< Trigger destruction of the session.

	const TUint16 iSessionId;			//< The session id

private:
	CDndSession(CProviderRes &aDnd, TUint16 aId, CDndSession *aNext);
	~CDndSession();						//< Private destructor, delete only from Delete()


	CProviderRes &iDnd;					//< The provider (DND) associated with this session
	CDndSession *iNext;					//< Links sessions under provider together
	CHostResolver *iResolver;			//< Non-null, if this session is (still) associated with RHostResolver
	TTime iAnswerTime;					//< Time of last good answer (only valid if iAnswered == 1).
	TUint iPending:1;					//< = 1, when resolver is pending, and query not yet delivered to DND
	TUint iAnswered:1;					//< = 1, if at least ONE good answer has been returned.
#ifdef SYMBIAN_DNS_PUNYCODE
	TUint iEnableIdn:1;					//< = 1, if support for resolving International Domain Names are enabled
#endif //SYMBIAN_DNS_PUNYCODE
	};


// CHostResolver
// *************
class CHostResolver : public CHostResolvProvdBase
	{
	/**
	* The host resolver.
	*
	* This is the representative of the application RHostResolver within
	* the TCPIP stack. This is created by the CProtocolRes::NewHostResolverL.
	*/
public:
	CHostResolver(CProtocolRes &aProtocol);
	~CHostResolver();
	void GetByName(TNameRecord &aName);
	void GetByAddress(TNameRecord &aName);
	void SetHostName(TDes& aNameBuf);
	void GetHostName(TDes& aNameBuf);
	void CancelCurrentOperation();
	TInt SetOption(TUint level, TUint aName,const TDesC8 &option);

	void Unlink();						//< Called by iSession, to remove the iSession link
	void Reply(const TDnsMessage &aReply, const TDesC8 &aPayload);	//< Called by iSession, when DND has replied.
	void QueryComplete(TInt aResult);	//< Called by iSession, when query completed ??
	void Query(const TDesC8& aQuery, TDes8& aResult, TInt aCount);
	TInt SecurityCheck(MProvdSecurityChecker *aChecker);
	TBool IsPending() { return iRequest.IsPending(); }
	void BuildMessage(TUint16 aId, TDes8 &aBuffer) const { iRequest.BuildMessage(aId, aBuffer); }
#ifdef SYMBIAN_DNS_PUNYCODE
	TBool IsIdnEnabled();
#endif //SYMBIAN_DNS_PUNYCODE

	CHostResolver *iNext;		//< Next resolver link chain
	CDndSession *iSession;		//< Assigned DND session or NULL, if none yet.
private:
	LOG(TInt Session() {return iSession ? (TInt)iSession->iSessionId : 0;})

	void Submit();

	CProtocolRes &iProtocol;	//< Link to the 'resolver' protocol instance
	TUint32 iNetworkId;			//< The default network ID.
	TUint32 iCurrentId;			//< The network ID for the current query
	TDnsRequest iRequest;		//< The query being served
	THostName iHostName;		//< The hostname (only used for SetHostName)
	TUint iNoNext:1;			//< if = 1, next should return Not Found
	TUint iHasNetworkServices:1;//< = 1, if client has network services.
#ifdef SYMBIAN_DNS_PUNYCODE
	TUint iEnableIdn:1;			// if =1 , Idn support is enabled.
#endif //SYMBIAN_DNS_PUNYCODE
	MProvdSecurityChecker *iSecurityChecker;
public:
	void NoDndAvailable();
	RTimeout iTimeout;
	};

//	CHostResolverLinkage
//	***********************
//	Glue to bind timeout callback from the timeout manager. Only used for
//	detecting a missing or misbehaving DND.

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KHostResolverTimeoutOffset 584
__ASSERT_COMPILE(KHostResolverTimeoutOffset == _FOFF(CHostResolver, iTimeout));
#else
#define KHostResolverTimeoutOffset _FOFF(CHostResolver, iTimeout)
#endif

class CHostResolverLinkage : public TimeoutLinkage<CHostResolver, KHostResolverTimeoutOffset>
	{
public:
	static void Timeout(RTimeout &aLink, const TTime & /*aNow*/, TAny * /*aPtr*/)
		{
		Object(aLink)->NoDndAvailable();
		}
	};

//
//	RES Class Implementation

// RES::Identify
// *************
void RES::Identify(TServerProtocolDesc &aEntry)
	{
	_LIT(KResolver, "resolver");

	aEntry.iName = KResolver;
	aEntry.iAddrFamily = KAfInet;
	aEntry.iSockType = KSockDatagram;
	aEntry.iProtocol = KProtocolInet6Res;
	aEntry.iVersion = TVersion(1, 0, 0);
	aEntry.iByteOrder = ELittleEndian;
	aEntry.iServiceInfo = KSIConnectionLess | KSIMessageBased | KSIRequiresOwnerInfo;
	aEntry.iNamingServices = 0;
	aEntry.iSecurity = KSocketNoSecurity;
	aEntry.iMessageSize = KSocketMessageSizeNoLimit;
	aEntry.iServiceTypeInfo = ESocketSupport;
	aEntry.iNumSockets = 1;	// Only 1 DND allowed.
	}

CProtocolBase *RES::NewL(CIfManager *const aInterfacer)
	{
	return new (ELeave) CProtocolRes(*aInterfacer);
	}

// Constructors for the TDnsRequest
// ********************************

TDnsRequest::TDnsRequest() :
	TDnsRequestBase(),
	iQuery(TPtrC8(0, 0)),
	iResponse(NULL)
	/**
	* Construct empty request = No Query Active
	*/
	{
	}

TDnsRequest::TDnsRequest(TNameRecord &aQuery, TInt aNext, TInt aType) :
	TDnsRequestBase(aType, aNext),
	iQuery(TPtrC8((TUint8 *)&aQuery, sizeof(aQuery))),
	iResponse((TUint8 *)&aQuery)
	/**
	* Construct a GetByName/GetByAddress request.
	*
	* @param aQuery	The query buffer in the SocketServer
	* @param aNext = 0 for the actual query, > 0 for additional results.
	* @param aType GetByName or GetByAddress.
	*/
	{
	}

TDnsRequest::TDnsRequest(const TDesC8 &aQuery, TDes8 &aResponse, TInt aNext) :
	TDnsRequestBase(KDnsRequestType_TDnsQuery, aNext),
	iQuery(aQuery),
	iResponse((TUint8 *)&aResponse)
	/**
	* Construct a special DNS query request.
	*
	* @param aQuery The query buffer in the SocketServer
	* @param aResponce The responece buffer in the SocketServer
	* @param aNext = 0 for the actual query, > 0 for additional results.
	*/
	{
	}

TDnsRequest::TDnsRequest(const THostName &aName, TDes &aResponse, TInt aType) :
	TDnsRequestBase(aType, 0),
	iQuery(TPtrC8((TUint8 *)&aName, sizeof(aName))),
	iResponse((TUint8 *)&aResponse)
	/**
	* Construct Get/Set HostName.
	*
	* @param aName The hostname (for SetHostName)
	* @param aResponce The responce buffer in the SocketServer
	* @param aType Set or get hostname.
	*/
	{
	}

void TDnsRequest::BuildMessage(TUint16 aId, TDes8 &aBuffer) const
	/**
	* Build the message from the request parameters.
	*/
	{
	aBuffer.SetLength(0);
	if (aBuffer.MaxLength() >= (TInt)sizeof(TDnsRequestBase) + iQuery.Length())
		{
		aBuffer = Header();
		((TDnsRequestBase *)aBuffer.Ptr())->iSession = aId;
		aBuffer.Append(iQuery);
		}
	else
		{
		// This means that DND is trying to read with too short buffer.
		LOG(Log::Printf(_L("\tres *** DND buffer is short ***")));
		}
	}

//
//	CProtocolRes Class Implementation

CProtocolRes::CProtocolRes(CIfManager &aInterfacer) : iInterfacer(aInterfacer)
	{
	LOG(Log::Printf(_L("new\tres resolver[%u] construct"), this));
	}

void CProtocolRes::BindL(CProtocolBase * /*aProtocol*/, TUint /*aId*/)
	/** Dummy. No real functionality. */
	{
	// Allow anyone to bind, and forget about it...
	}

void CProtocolRes::Unbind(CProtocolBase * /*aProtocol*/, TUint /*aId*/)
	/** Dummy. No real functionality. */
	{
	// Just pass any unbind too..
	}
	
	
// CProtocolRes::StartL
// ********************
void CProtocolRes::StartL()
	{
	/**
	* Register for the event service.
	* Called after all binding is complete. Register for the event
	* service to receive configuration changes in the interfaces.
	*/
	MEventService &mgr = *IMPORT_API_L((&iInterfacer), MEventService);
	mgr.RegisterListener(this, EClassAddress);
	mgr.RegisterListener(this, EClassInterface);
	}

// CProtocolRes::Notify
// ********************
void CProtocolRes::Notify(TUint aEventClass, TUint aEventType, const void *aData)
	/**
	* Configuration changed event.
	*
	* This is called by the event manager (MEventService) for the registered events.
	* Analyze the event and if the conditions are met, request a "configuration changed"
	* message to be sent to the DND.
	*
	* @param aEventClass The event class
	* @param aEventType The event type
	* @param aData The event infrormation (TInetAddressInfo or TInetInterfaceInfo)
	*/
	{
	if (!iConfigureDone)
		return;	// Configure request is already pending, nothing to do!
	
	if (aEventClass == EClassAddress)
		{
		// ...for now, all address events require reconfigure
		const TInetAddressInfo &ai = *(TInetAddressInfo *)aData;
		if ((ai.iFlags & TInetAddressInfo::EF_Id) == 0)
			return;	// Only interested in addresses, not prefixes.
		if (aEventType != EventTypeDelete && (ai.iState != TInetAddressInfo::EAssigned))
			return;	// Only interested in valid or deleted addresses.
#ifdef _LOG
		TInetAddr addr(ai.iAddress, 0);
		TBuf<70> tmp;
		addr.Output(tmp);
		Log::Printf(_L("<>\tres Address Event(%d) IF %d [%S]"),  aEventType, ai.iInterface, &tmp);
#endif
		}
	else if (aEventClass == EClassInterface)
		{
		// ...for now all interface events require reconfigure
#ifdef _LOG
		const TInetInterfaceInfo &ii = *(TInetInterfaceInfo *)aData;
		Log::Printf(_L("<>\tres Interface Event(%d) IF %d [%S]"), aEventType, ii.iIndex, &ii.iName);
#endif
		}
	else
		return;
	//
	// An event that requires DND reconfiguration has occurred.
	//
	iConfigureDone = 0;
	if (iDND)
		iDND->Activate();

	}

// CProtocolRes::NewSAPL
// **********************
CServProviderBase* CProtocolRes::NewSAPL(TUint /*aSockType*/)
	/**
	* Create a new DNS SAP.
	* This SAP should be the DND server offering the
	* name resolution services through this datagram
	* socket.
	*
	* Only ONE DND server at any time can be active. Multiple
	* SAP creations are rejected.
	*/
	{
	LOG(Log::Printf(_L("NewSAPL\tres resolver[%u]"), this));
	if (iDND)
		User::Leave(KErrAlreadyExists); // Only one DND is allowed (should not get here)
	iDND = new (ELeave) CProviderRes(*this);
	LOG(Log::Printf(_L("\tres DND[%u] new"), iDND));
	return iDND;
	}

// CProtocolRes::NewHostResolverL
// ******************************
CHostResolvProvdBase *CProtocolRes::NewHostResolverL()
	/**
	* Create a resolver object.
	*
	* Create the internal object match the client RHostResolver. These
	* objects are stored in the linked list (iQueries) at CProtocolRes.
	*
	* @return The host resolver
	*/
	{
	CHostResolver *hr = new (ELeave) CHostResolver(*this);
	hr->iNext = iQueries;
	iQueries = hr;
	return hr;
	}


// CProtocolRes::NewServiceResolverL
// *********************************
CServiceResolvProvdBase *CProtocolRes::NewServiceResolverL()
	/**
	* NewServiceResolverL is not supported.
	* (Override the default Panic implementation with KErrNotSupported Leave!)
	*/
	{
	User::Leave(KErrNotSupported);
	// NOTREACHED
	return NULL;
	}

// CProtocolRes::NewNetDataBaseL
// *****************************
CNetDBProvdBase *CProtocolRes::NewNetDatabaseL()
	/**
	* NewNetDatabaseL is not supported.
	* (Override the default Panic implementation with KErrNotSupported Leave!)
	*/
	{
	User::Leave(KErrNotSupported);
	// NOTREACHED
	return NULL;
	}

// CProtocolRes::CancelSAP
// ***********************
void CProtocolRes::CancelSAP(const CProviderRes &aSAP)
	/**
	* The DND has exited.
	*
	* Only the ~CProviderRes() destructor calls this. This
	* removes the reference iDND to the instance.
	*/
	{
	LOG(Log::Printf(_L("\tres DND[%u] terminating"), &aSAP));
	if (&aSAP == iDND)
		iDND = NULL;
	iConfigureDone = 0;	// Clear for the new DND (if any coming).
	}

// CProtocolRes::CancelQuery
// *************************
void CProtocolRes::CancelQuery(const CHostResolver &aQuery)
	/**
	* The host resolver is being deleted.
	*
	* Only the ~CHostResolver() destructor calls this. This
	* removes the reference from the iQueries list.
	*/
	{
	CHostResolver **h, *p;
	h = &iQueries;
	while ((p = *h) != NULL)
		if (p == &aQuery)
			{
			*h = p->iNext;
			return;
			}
		else
			h = &p->iNext;
	}

// CProtocolRes::FindPending
// *************************
CHostResolver *CProtocolRes::FindPending() const
	/**
	* Find a pending request without a session.
	*
	* Looks through all queries and returns the first one
	* which is waiting for a request, but does not yet have
	* session object (CDndSession) assigned.
	*
	* There can be pending requests without session ONLY IF
	* DND is not yet running, or if CProviderRes::AssignSession()
	* has failed due to all resolver slots being reserved.
	* The latter condition should be a rare occurrence
	* (should never happen!).
	*
	* @return The host resolver or NULL if none found.
	*/
	{
	for (CHostResolver *hr = iQueries; hr != NULL; hr = hr->iNext)
		if (hr->iSession == NULL && hr->IsPending())
			return hr;
	return NULL;
	}

// CProtocolRes::~CProtocolRes
// ***************************
CProtocolRes::~CProtocolRes()
	{
	LOG(Log::Printf(_L("\tres resolver[%u] destruct"), this));

	ASSERT(iDND == NULL);		// -- cannot get here if there is a DND attached
	ASSERT(iQueries == NULL);	// -- cannot get here if there are RHostResolver's

	TRAP_IGNORE(MEventService &mgr = *IMPORT_API_L((&iInterfacer), MEventService);
		mgr.RemoveListener(this, EClassAddress);
		mgr.RemoveListener(this, EClassInterface);
		);
	}

// CProtocolRes::Identify
// **********************
void CProtocolRes::Identify(TServerProtocolDesc *aInfo) const
	{
	RES::Identify(*aInfo);
	}

//
// CDndSession

CDndSession::CDndSession(CProviderRes &aDnd, TUint16 aSession, CDndSession *aNext)
	 : iSessionId(aSession), iDnd(aDnd), iNext(aNext)
	{
	}
	
CDndSession::~CDndSession()
	{
	}

// CDndSession::Delete
// *******************
CDndSession *CDndSession::Delete()
	/**
	* Delete self.
	*
	* Deletes self, but returns the link to the
	* next session in chain (or NULL, if last).
	*
	* @return The next in chain.
	*/
	{
	CDndSession *const next = iNext;	// Return iNext field.
	
	ASSERT(iResolver == NULL);

	delete this;
	return next;
	}

// CDndSession::Submit
// *******************
void CDndSession::Submit()
	/**
	* Requests a service from DND.
	*
	* Do the basic details for requesting service for this
	* session. Actual nature of the request is not considered
	* here (it's up to the caller of this function).
	*/
	{
	iPending = 1;
	iAnswered = 0;
	iDnd.Activate();	
	}

// CDndSession::Answered
// *********************
void CDndSession::Answered()
	/**
	* Mark session as answered.
	*
	* The associated host resolver of this session has received
	* an answer for the request. This function exists only to
	* handle the situation where system does not have enough
	* host resolver slots to use. If there is a pending
	* request without a session, then this session is
	* given to that request. This action makes prevents
	* use of the "Next()" action for previous owner of the
	* session.
	*/
	{
	CHostResolver *const hr = iDnd.iProtocol.FindPending();
	if (hr)
		{
		// Exceptional branch, reassign session to another.
		iResolver->Unlink();
		iResolver = NULL;
		Link(*hr);
		iDnd.Activate();
		}
	else
		{
		// Normal branch.
		iAnswerTime.UniversalTime();
		iAnswered = 1;
		}
	}

// CDndSession::Reply
// ******************
void CDndSession::Reply(const TDnsMessage &aReply, const TDesC8 &aPayload) const
	/**
	* A reply from DND.
	*
	* Relay the reply from DND to the host resolver. If the host resolver
	* has already gone away, the reply is simply dropped.
	*
	* @param aReply The DND reply message (for the header part).
	* @param aPayload The DND "payload" part of the reply message.
	*/
	{
	if (iResolver)
		iResolver->Reply(aReply, aPayload);
	}

// CDndSession::Link
// *****************
void CDndSession::Link(CHostResolver &aHR)
	/**
	* Link the host resolver to session.
	*
	* @param aHR The host resolver with query pending
	*/
	{
	ASSERT(aHR.IsPending());
	ASSERT(aHR.iSession == NULL);
	ASSERT(iResolver == NULL);
	aHR.iTimeout.Cancel();
	aHR.iSession = this;
	iResolver = &aHR;
	iPending = 1;
#ifdef SYMBIAN_DNS_PUNYCODE
	if( aHR.IsIdnEnabled() )
	{
		iEnableIdn = 1;
	}
#endif //SYMBIAN_DNS_PUNYCODE
	LOG(Log::Printf(_L("\tres HR[%u] SESSION %u assigned HR"), iResolver, (TInt)iSessionId));
	}

// CDndSession::Unlink()
// *********************
void CDndSession::Unlink()
	/**
	* Unlink host resolver from session.
	*
	* This is called from CHostResolver, when it does not
	* need the session any more.
	*/
	{
	ASSERT(iResolver != NULL);	// Should never be called with NULL iResolver!
	LOG(Log::Printf(_L("\tres HR[%u] SESSION %u detached HR"), iResolver, (TInt)iSessionId));
	iResolver = NULL;
	iPending = 0;
	// See, if a new resolver should be assigned to this session
	CHostResolver *const hr = iDnd.iProtocol.FindPending();
	if (hr)
		Link(*hr);
	// Activate needed always, either to pass a new query or
	// cancel, if no new resolver was assigned.
	iDnd.Activate();
	}

//
//	CProviderRes Implementation

CProviderRes::CProviderRes(CProtocolRes &aProtocol) : iProtocol(aProtocol)
	{
	}

CProviderRes::~CProviderRes()
	/**
	* DND server has terminated.
	* Cleanup all sessions
	*/
	{
	while (iSessions)
		iSessions = iSessions->Delete();
	iProtocol.CancelSAP(*this);
	}


// CProviderRes::Activate
// **********************
void CProviderRes::Activate()
	{
	/**
	* Call socket NewData.
	*/
	if (iSocket)
		iSocket->NewData(1);
	}

// CProviderRes::Find
// ******************
const CDndSession *CProviderRes::Find(const TUint16 aId) const
	/**
	* Find a session by id.
	*
	* @param aId The session id.
	* @return The session or NULL, if not found.
	*/
	{
	const CDndSession *s = iSessions;
	for ( ;s != NULL; s = s->iNext)
		if (s->iSessionId == aId)
			break;
	return s;
	}

//	CProviderRes::Write
//	*******************
TUint CProviderRes::Write(const TDesC8 &aDesc, TUint /* aResult */, TSockAddr* /*aAddr =NULL*/)
	/**
	* The reply from DND.
	*
	* The resolver is now returning some reply. Deliver the reply
	* to the matching host resolver (matched by the session id).
	*
	* @param aDesc The TDnsMessage containing the reply.
	* @return Always 1 (= data accepted).
	*/
	{
	const TDnsMessage &reply = *(TDnsMessage *)aDesc.Ptr();
	//
	// Locate the session for which the reply belongs
	//
	const CDndSession *const s = Find(reply.iSession);
	if (s == NULL)
		{
		// Ooops! DND is still sending replies to a session that do not
		// exist any more. Should there be implicitly generated cancel
		// message?
		LOG(Log::Printf(_L("Write\tres SESSION %u not found--DND reply ignored"), (TInt)reply.iSession));
		}
	else
		{
		const TPtrC8 payload = reply.Payload(aDesc.Length());
		s->Reply(reply, payload);
		}
	return 1;
	}
	

// CProviderRes::AssignSession
// ***************************
TBool CProviderRes::AssignSession(CHostResolver &aHR)
	/**
	* Assign a session for a host resolver.
	*
	* Try to acquire a session for the host resolver.
	*
	* @param The Host resolver
	* @return ETrue, if session assiged, and EFalse otherwise.
	*/
	{
	ASSERT(aHR.IsPending());		// Host resolver must have a query to be done
	ASSERT(aHR.iSession == NULL);	// Host resolver must be without a session.

	// Check the existing sessions
	
	CDndSession *a = NULL;
	for (CDndSession *s = iSessions; s != NULL; s = s->iNext)
		{
		if (s->iResolver == NULL)
			{
			// There is already a session queued for "cancel" request.
			// This session can be efficiently reassigned to the new
			// host resolver.
			s->Link(aHR);
			// There is no need to Activate() anything, because there must
			// already be a pending Activate() for the cancel message.
			return ETrue;
			}
		else if (s->iAnswered)
			{
			if (a == NULL || a->iAnswerTime > s->iAnswerTime)
				a = s;
			}
		}

	if (iAvailable > 0)
		{
		// Assign unused session id.
		do
			{
			if (++iSessionId == 0)
				iSessionId = 1; // Avoid ZERO as id!
			}
		while (Find(iSessionId) != NULL);
	
		a = new CDndSession(*this, iSessionId, iSessions);
		if (a)
			{
			// Created a new session, assign to query.
			iAvailable -= 1;
			iSessions = a;
			a->Link(aHR);
			// Activate() is needed, because this is a new session.
			Activate();
			return ETrue;
			}
		else
			{
			aHR.QueryComplete(KErrNoMemory); // <-- check!!!
			return EFalse;
			}
		}

	if (a)
		{
		// Found a session that can be stolen from it's host resolver.
		a->iResolver->Unlink();
		a->iResolver = NULL;
		a->Link(aHR);
		// Activate is required
		Activate();
		return ETrue;
		}
	// No sessions available
	return EFalse;
	}

// CProviderRes::GetData
// *********************
void CProviderRes::GetData(TDes8 &aDesc, TUint /* aOptions */, TSockAddr * /*anAddr=NULL*/)
	/**
	* Prepare the query message for the DND.
	*
	* Build and return the query message for the DND. The buffer (aDesc) MUST be long enough
	* to receive any query.
	*
	* @retval aDesc The query message.
	*/
	{
	if (!iProtocol.iConfigureDone)
		{
		//
		// Generate a DNS Configure request
		//
		iProtocol.iConfigureDone = 1;
		aDesc.SetLength(sizeof(TDnsRequestBase));
		aDesc.FillZ();
		((TDnsRequestBase *)aDesc.Ptr())->iType = KDnsRequestType_Configure;
		LOG(Log::Printf(_L("GetData\tres DND Configure msg(%d)"), aDesc.Length()));
		return;
		}
	// Find a session that needs to be serviced, return it. If this was
	// a cancelled session, then delete entry.
	//
	// Note: The search starts always from the beginning of the Sessions list,
	// and the first one needing service is served. This is not "fair", but
	// it is assumed that in practise DND can accept all requests fast.
	CDndSession *s;
	for (CDndSession **h = &iSessions; (s = *h) != NULL; h = &s->iNext)
		{
		if (s->iResolver == NULL)
			{
			// Build a cancel session message with s->iSession
			// (just a header and session id is treated as cancel).
			aDesc.SetLength(sizeof(TDnsRequestBase));
			aDesc.FillZ();
			((TDnsRequestBase *)aDesc.Ptr())->iSession = s->iSessionId;
			LOG(Log::Printf(_L("GetData\tres SESSION %u DND Cancel msg(%d)"), (TInt)s->iSessionId, aDesc.Length()));
			*h = s->Delete();
			iAvailable += 1;
			return;
			}
		else if (s->iPending)
			{
			ASSERT(s->iResolver->IsPending());	// There must be a pending request!
			s->iPending = 0;	// Prevent this same request from being sent again to DND.
			s->iResolver->BuildMessage(s->iSessionId, aDesc);
			LOG(Log::Printf(_L("GetData\tres HR[%u] SESSION %u DND Query msg(%d)"), s->iResolver, (TInt)s->iSessionId, aDesc.Length()));
			return;
			}
		}

	// This path is reached ONLY when session is assigned to a host resolver
	// and queued (Activated) for processing, but is cancelled by application
	// before the DND gets to process the Activate...
	LOG(Log::Printf(_L("GetData\tres Nothing found, empty DND Query")));
	aDesc.SetLength(0);		// No data, nothing to do.
	}


void CProviderRes::Start()
	/** The DND is becoming ready to serve. */
	{
	LOG(Log::Printf(_L("\tres DND[%u] Start"), this));
	// Reconfiguration needed
	if (!iProtocol.iConfigureDone)
		Activate();
	}

void CProviderRes::Shutdown(TCloseType aOption, const TDesC8& /*aDisconnectionData*/)
	{
	/** The DND is shutting down */
	Shutdown(aOption);
	}

void CProviderRes::Shutdown(TCloseType aOption)
	/** The DND is shutting down */
	{
	LOG(Log::Printf(_L("Shutdown\res DND[%u] type=%d"), this, (TInt)aOption));
    if (aOption != EImmediate)
        iSocket->CanClose();
	}

void CProviderRes::Ioctl(TUint /*aLevel*/, TUint /*aName*/, TDes8* /*aOption*/)
	/** Dummy. No real functionality. */
	{
	LOG(Log::Printf(_L("ioctl\tres DND[%u]"), this));
	}


void CProviderRes::CancelIoctl(TUint /*aLevel*/, TUint /*aName*/)
	/** Dummy. No real functionality. */
	{
	LOG(Log::Printf(_L("ioctl\tres DND[%u] Cancel"), this));
	}

TInt CProviderRes::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	/**
	* SetOption.
	*
	* This implements a resolver gate specific option:
	*
	*	level= #KSolDnd, name= #KSoDndSessions
	*
	* The DND must tell the number of available resolver slots to
	* the resolver gateway code using this SetOption.
	*
	* Other options are passed to the interface manager.
	*/
	{
	if (aLevel == KSolDnd)
		{
		if (aName != KSoDndSessions)
			return KErrNotSupported;


		if (aOption.Length() < (TInt)sizeof(TInt))
			return KErrGeneral;
		// note: here it is assumed that the Ptr() is properly
		// aligned, but for debug check it! -- msa)
		ASSERT((((TUint)aOption.Ptr()) & 0x3) == 0);
		iAvailable = *((TInt *)aOption.Ptr());
		// Count current sessions and subtract them from iAvailable
		// (it's ok, even if result is negative!)
		for (CDndSession *s = iSessions; s != NULL; s = s->iNext)
			iAvailable -= 1;
		LOG(Log::Printf(_L("SetOpt\tres DND[%u] Sessions=%d"), this, iAvailable));
		// Activate additional pending sessions, if possible.
		CHostResolver *hr;
		while ((hr = iProtocol.FindPending()) != NULL)
			{
			if (!AssignSession(*hr))
				break;
			}
		return KErrNone;
		}
	LOG(Log::Printf(_L("SetOpt\tres DND[%u] level=%d, name=%d"), this, aLevel, aName));
	return iProtocol.Interfacer().SetOption(aLevel, aName, aOption);
	}


TInt CProviderRes::GetOption(TUint aLevel, TUint aName, TDes8& aOption) const
	/**
	* GetOption.
	*
	* There is no local options, the call is passed to the interface manager.
	*/
	{
	LOG(Log::Printf(_L("GetOpt\tres DND[%u] level=%d, name=%d"), this, aLevel, aName));
	return iProtocol.Interfacer().GetOption(aLevel, aName, aOption);
	}


// CProviderRes::SecurityCheck
// ***************************
TInt CProviderRes::SecurityCheck(MProvdSecurityChecker *aChecker)
	/**
	* Check security of the DND implementor.
	* This represents a socket for the DND server. Check that the
	* application opening this socket actually has the sufficient
	* capability to be the DND.
	*/
	{
	return aChecker->CheckPolicy(KPolicyNetworkControl, "DND Server");
	}

//
//	CHostResolver Class Implementation
//

CHostResolver::CHostResolver(CProtocolRes &aProtocol) : iProtocol(aProtocol), iTimeout(CHostResolverLinkage::Timeout)
	{
	LOG(Log::Printf(_L("new\tres HR[%u]"), this));
	SocketServExt::OpenSession();
	// Any host resolver must count as "user" to prevent system
	// from killing the DND while doing resolving.
	iProtocol.Interfacer().IncUsers();
	}


CHostResolver::~CHostResolver()
	{
	// Because CancelCurrentOperation() is virtual, avoid using
	// it in desctructor!
	iTimeout.Cancel();
	iProtocol.CancelQuery(*this);
	if (iSession)
		iSession->Unlink();
	iProtocol.Interfacer().DecUsers();
	LOG(Log::Printf(_L("~\tres HR[%u] deleted"), this));
	SocketServExt::CloseSession();
	}
	
// CHostResolver::NoDndAvailable
// *****************************
void CHostResolver::NoDndAvailable()
	/**
	* The timeout expired.
	* This should only happen when there is no DND running.
	* See CHostResolver::Submit
	*/
	{
	if (iProtocol.iDND == NULL)
		QueryComplete(KErrInetNoDnsResolver);
	}

//
// CHostResolver::Submit
// *********************
void CHostResolver::Submit()
	/**
	* Submit the request to the DND.
	*
	* If there is no DND yet, set a short timer which calls
	* CHostResolver::NoDndAvailable when it expires. Normally,
	* there is always DND running. Only when the stack is
	* starting there is a short time period when host resolvers
	* can request service while DND is still starting up. Any
	* longer delay indicates that the DND startup has failed
	* and the timeout will expire the requests with a specific
	* error code (#KErrInetNoDnsResolver).
	*/
	{
	//
	// Complete the request with network id
	//
	iRequest.iId = iCurrentId;
#ifdef SYMBIAN_DNS_PUNYCODE
	iRequest.iScope |= EScopeType_NET;
#else
	iRequest.iScope = EScopeType_NET;
#endif //SYMBIAN_DNS_PUNYCODE


	if (iSession == NULL)
		{
		if (iProtocol.iDND != NULL)
			(void)iProtocol.iDND->AssignSession(*this);
		else
			{
			// Set short timeout, if DND is not yet running.
			iProtocol.iInterfacer.SetTimer(iTimeout, 10);
			}
		}
	else
		iSession->Submit();
	}

// CHostResolver::SetOption
// ************************
TInt CHostResolver::SetOption(TUint aLevel, TUint aName, const TDesC8& aOption)
	/**
	* SetOption.
	* Implements the #KSoConnectionInfo for the host resolver instance. Other options
	* are passed to the CProtocolRes.
	*/
    {
#ifdef SYMBIAN_NETWORKING_UPS

	if (aLevel == static_cast<TUint>(KSOLProvider))
		{
		if (aName == static_cast<TUint>(KSoConnectionInfo))
			{
			if (aOption.Length() >= sizeof(TSoIfConnectionInfo))
				{
				const TSoIfConnectionInfo& opt = *reinterpret_cast<const TSoIfConnectionInfo*>(aOption.Ptr());
				iNetworkId = opt.iNetworkId;
				return KErrNone;
				}
			else
				{
				return KErrArgument;
				}
			}
		else
		if (aName == static_cast<TUint>(KSoGetErrorCode))
			{
			// Return a TCP/IP failure code appropriate to the last operation.
			// Kludge - SetOption does not allow for any return value via aOption (being const), so
			// return a positive value representing the error code.
			return (iRequest.iType == KDnsRequestType_GetByAddress) ? -KErrDndAddrNotFound : -KErrDndNameNotFound;
			}
      	}

#else

      if (aLevel == STATIC_CAST(TUint, KSOLProvider) && aName == STATIC_CAST(TUint, KSoConnectionInfo))
		if (STATIC_CAST(TUint, aOption.Length()) >= sizeof(TSoIfConnectionInfo))
			{
			// If the client does not have network services, don't allow setting
			// of the network id (=> limit queries to local host data only!).
			if (!iHasNetworkServices)
				return KErrNone;
			TSoIfConnectionInfo &opt = *(TSoIfConnectionInfo*)aOption.Ptr();
			iNetworkId = opt.iNetworkId;
			return KErrNone;
			}
		else
			return KErrArgument;

#endif
#ifdef SYMBIAN_DNS_PUNYCODE
	else if (aLevel == KSolInetDns )
		{
		if(aName == static_cast<TUint>(KSoDnsEnableIdn) )
			{
			TPckgC<TBool>* setOptPckg=(TPckgC<TBool>*)(&aOption);
			const TBool& enableIdn=(*setOptPckg)();

			if(enableIdn)
				{
				iEnableIdn = 1;
				}
			else
				{
				if(iEnableIdn == 1)
					{
					iEnableIdn = 0;
					}
				}
			return KErrNone;
			}
		else
			{
			return KErrArgument;
			}
		}
#endif //SYMBIAN_DNS_PUNYCODE

    return iProtocol.SetOption(aLevel, aName, aOption);
    }

// CHostResolver::Unlink
// ************************
void CHostResolver::Unlink()
	/**
	* Unlink resolver from session.
	* Called from the destructor of the attached
	* session to remove the reference to it. If a
	* query is active at this point, the caller will take
	* care of any necessary QueryComplete notifys *after*
	* this! This only needs to remove the link!
	*/
	{
	LOG(Log::Printf(_L("\tres HR[%u] SESSION %d detached"), this, Session()));
	ASSERT(iSession != NULL);	// Should never get called with iSession == NULL!
	iSession = NULL;
	iNoNext = 1;				// Next() cannot be used if session is disconnected.
	}

// CHostResolver::QueryComplete
// ****************************
void CHostResolver::QueryComplete(TInt aResult)
	/**
	* Pass query completion to the Socket Server.
	*
	* Called by attached resolver service to notify that query has
	* been completed. A passthrough to the socket server. If the
	* result is KErrNone, the reply content has already been copied
	* to the buffer of the socket server.
	*
	* @param aResult The query result code.
	*/
	{
	if (iRequest.IsPending())
		{
#ifndef SYMBIAN_NETWORKING_UPS
		// The result KErrCompletion indicates that the request could not be resolved
		// without use of name servers. If the client does not have network services
		// capability, just return the appropriate "not found" error status.
		if (!iHasNetworkServices && aResult == KErrCompletion)
			aResult = iRequest.iType == KDnsRequestType_GetByAddress ? KErrDndAddrNotFound : KErrDndNameNotFound;
#endif //SYMBIAN_NETWORKING_UPS
		(void)new (&iRequest) TDnsRequest();
		if (aResult == KErrNone)
			{
			// If session attached, mark it answered
			// (note: this information is only used when
			// system is running out of available sessions)
			if (iSession)
				iSession->Answered();
			}
		else if (aResult != KErrCompletion)
			// Any error indicates that there cannot be any further use
			// for the socket gateway to the DND, thus tear it down to
			// release resources. However, KErrCompletion is such that
			// a new call will normally follow, so make exception for it.
			CancelCurrentOperation();
		// Note: CancelCurrentOperation (if executed) must be done before
		// the QueryComplete, because QueryComplete may call directly
		// GetByName or GetByAddress (may happen with KErrCompletion)
		LOG(Log::Printf(_L("\tres HR[%u] SESSION %d QueryComplete(%d)"), this, Session(), aResult));
		iNotify->QueryComplete(aResult);
		}
	else
		CancelCurrentOperation();
	}

// CHostResolver::Reply
// ********************
void CHostResolver::Reply(const TDnsMessage &aReply, const TDesC8 &aPayload)
	/**
	* The reply has become available.
	* The DND has returned a reply to the query. Pass the results back
	* to application/socket server. Called via CDndSession as follows:
	*
	* DND socket send - CProverRes::Write - CDndSession::Reply - this
	*
	* @param aReply The reply message (for header)
	* @param aPayload The actual reply content.
	*/
	{
	TInt result = aReply.iNext;
	if (result == KErrNone)
		{
		if (aReply.iType != iRequest.iType)
			result = KErrGeneral;	// ...reply format does not match the query format!
		else switch (aReply.iType)
			{
		case KDnsRequestType_GetByName:
		case KDnsRequestType_GetByAddress:
			// The payload is the TNameRecord
			*iRequest.iNameRecord = aReply.NameRecord();
#ifdef _LOG
			{
			TBuf<70> tmp;
			TInetAddr::Cast(iRequest.iNameRecord->iAddr).OutputWithScope(tmp);
			Log::Printf(_L("Write\tres HR[%u] SESSION %d DND Reply = %S [%S]"), this, Session(), &iRequest.iNameRecord->iName, &tmp);
			}
#endif
			break;
		case KDnsRequestType_TDnsQuery:
			if (iRequest.iQueryResponse->MaxSize() >= aPayload.Size())
				{
				*iRequest.iQueryResponse = aPayload;
				LOG(Log::Printf(_L("write\tres HR[%u] DND Reply to query, length=%d"),
							this, iRequest.iQueryResponse->Length()));
				}
			else
				result = KErrTooBig;
			break;
		case KDnsRequestType_GetHostName:
			if (iRequest.iHostName->MaxLength() >= aReply.HostName().Length())
				{
				*iRequest.iHostName = aReply.HostName();
				LOG(Log::Printf(_L("Write\tres HR[%u] SESSION %d DND Reply to GetHostName, %S"),
					this, Session(), iRequest.iHostName));
				}
			else
				result = KErrTooBig;
			break;
		case KDnsRequestType_SetHostName: // does not return any data.
			LOG(Log::Printf(_L("Write\tres HR[%u] SESSION %d DND Reply to SetHostName"), this, Session()));
			break;
		default:
			break;
			}
		}
	else
		LOG(Log::Printf(_L("Write\tres HR[%u] SESSION %d DND Reply Error=%d"), this, Session(), result));
		
	QueryComplete(result);
	}

// CHostResolver::CancelCurrentOperation
// *************************************
void CHostResolver::CancelCurrentOperation()
	/**
	* Release all extra resources assigned for the host resolver.
	*
	* This function is called when the host resolver does not need
	* any additional resources. Currently this means the release of
	* of the session with DND. The host resolver does not need the
	* session when
	*
	* - current query ends with an error (no additional info is available with session)
	* - the current request is being cancelled
	* - the host resolver is going to be deleted
	*/
	{
	LOG(Log::Printf(_L("\tres HR[%u] SESSION %d CancelCurrentOperation"), this, Session()));

	(void)new (&iRequest) TDnsRequest();
	if (iSession)
		{
		iSession->Unlink();
		iSession = NULL;
		}
	}

// CHostResolver::GetHostName
// **************************
void CHostResolver::GetHostName(TDes &aNameBuf)
	/**
	* GetHostName handler.
	* This implements the RHostResolver::GetHostName from the application.
	*/
	{
	iHostName.SetLength(0);
	(void)new (&iRequest) TDnsRequest(iHostName, aNameBuf, KDnsRequestType_GetHostName);
	iCurrentId = iNetworkId;
	LOG(Log::Printf(_L("<>\tres HR[%u] SESSION %d GetHostName(maxlen=%d) NID=%d"),
		 this, Session(), aNameBuf.MaxLength(), iNetworkId));
	Submit();
	}

// CHostResolver::SetHostName
// **************************
void CHostResolver::SetHostName(TDes& aNameBuf)
	/**
	* SetHostName handler.
	* This implements the RHostResolver::SetHostName from the application.
	*/
	{
	TInt result;
	for (;;)	// ...NOT A LOOP, JUST FOR BREAK EXITS!
		{
		result = iSecurityChecker->CheckPolicy(KPolicyNetworkControl, "SetHostName");
		if (result != KErrNone)
			break;	// ...operation not allowed for this user
		if (iHostName.MaxLength() < aNameBuf.Length())
			{
			result = KErrTooBig;
			break;	// ...the name is too long
			}
		iHostName = aNameBuf;
		(void)new (&iRequest) TDnsRequest(iHostName, iHostName, KDnsRequestType_SetHostName);
		iCurrentId = iNetworkId;
		LOG(Log::Printf(_L("<>\tres HR[%u] SESSION %d SetHostName(%S) NID=%d"), this, Session(), &aNameBuf, iNetworkId));
		Submit();
		// SetHostName has been succesfully submitted
		// to the DND, DND issues the completion later.
		return;
		}
	// SetHostName not done!
	LOG(Log::Printf(_L("<>\tres HR[%u] SESSION %d SetHostName(%S) not done: %d"), this, Session(), &aNameBuf, result));
	iNotify->QueryComplete(result);
	}

// CHostResolver::GetByName
// ************************
void CHostResolver::GetByName(TNameRecord &aName)
	/**
	* GetByName handler.
	* This implements the RHostResolver::GetByName and RHostResolver::Next() (for the name) from the application.
	*
	* @retval aName The result (and the query in aName.iName as input)
	*/
	{
	ASSERT(!IsPending());	// Cannot be in pending state!
	LOG(Log::Printf(_L("ByName\tres HR[%u] SESSION %d Next=%d NID=%d Name=%S"), this, Session(),
		aName.iFlags, iNetworkId, &aName.iName));
	//
	// Note: aName.iFlage > 0 indicate resolver.Next() request
	//
	(void)new (&iRequest) TDnsRequest(aName, aName.iFlags, KDnsRequestType_GetByName);
#ifdef SYMBIAN_DNS_PUNYCODE
	if(this->iEnableIdn)
		{
		iRequest.iScope |=  ENABLEIDN_SCOPE;
		}
#endif //SYMBIAN_DNS_PUNYCODE

	TInt result = KErrNotFound;
	for (;;)// ** Just to allow easy exits from a block with 'break'
		{
		TInetAddr &addr = TInetAddr::Cast(aName.iAddr);

		if (aName.iFlags == 0)
			{
			iNoNext = 1;

			if (addr.Input(aName.iName) == KErrNone)
				{
				// The name was all numeric IPv4 or IPv6 address
				// (possibly with numeric %scope notation), just
				// return the result as is.
				result = KErrNone;
				break;
				}

			iCurrentId = iNetworkId;
			const TInt i = aName.iName.LocateReverse('%');
			if (i >= 0)
				{

				// The name is using the "name%interface" notation. Retrieve
				// the network ID based on the interface, and override
				// the default setting.
				const MInterface *const mi = iProtocol.Interfacer().Interface(aName.iName.Right(aName.iName.Length() - i - 1));
				if (mi == NULL)
					break;	// use default return KErrNotFound, if interface
							// cannot be located.
				iCurrentId = mi->Scope(EScopeType_NET);
				// Remove the "%"-part from the name.
				aName.iName.SetLength(i);
				if (addr.Input(aName.iName) == KErrNone)
					{
					// Remaining part was numeric address, fill in the
					// appropriate scope id. Always return IPv6 (KAfInet6)
					// format address.
					if (addr.Family() != KAfInet6)
						addr.ConvertToV4Mapped();
					addr.SetScope(mi->Scope((TScopeType)(addr.Ip6Address().Scope() - 1)));
					result = KErrNone;
					break;
					}
				// ...was "name%interface" notation, which overrides the
				// default network id.
				// Ignore %-notation without network service capability.
				// (maybe too harsh, but easiest to solve the problem: the
				// notation allows initial non-zero id, without the completion
				// phase (KErrCompletion). Thus, "hostname%RealIf" would allow use
				// network for DNS queries without network services capability!)
				//
				// No change for SYMBIAN_NETWORKING_UPS.  This is an undocumented feature which is not
				// worth changing the TCP/IP stack and ESock to get working with UPS.  The
				// user can make use of an explicit RHostResolver to achieve the same effect.
				if (!iHasNetworkServices)
					iCurrentId = 0;
				}
			iNoNext = 0;	// Allow Next Processing
			}
		else if (iNoNext)
			break;	// KErrNotFound

		Submit();
		return;	// ** NEVER FORGET TO TERMINATE THE 'FAKE' LOOP! **
		}
	QueryComplete(result);
	}

// CHostResolver::GetByAddress
// ***************************
void CHostResolver::GetByAddress(TNameRecord &aName)
	/**
	* GetByAddr handler.
	* This implements the RHostResolver::GetByAddr and RHostResolver::Next() (for the addr) from the application.
	*
	* @retval aName  The result (and query aName.iAddr as input)
	*/
	{
	ASSERT(!IsPending());	// Cannot be in pending state!
#ifdef _LOG
	TBuf<100> tmp;
	TInetAddr::Cast(aName.iAddr).OutputWithScope(tmp);
	Log::Printf(_L("ByAddr\tres HR[%u] SESSION %d Next=%d NID=%d Addr=%S"),
		this, Session(), aName.iFlags, iNetworkId, &tmp);
#endif
	//
	// Note: aName.iFlage > 0 indicate resolver.Next() request
	//
	(void)new (&iRequest) TDnsRequest(aName, aName.iFlags, KDnsRequestType_GetByAddress);
#ifdef SYMBIAN_DNS_PUNYCODE
	if(this->iEnableIdn)
		{
			iRequest.iScope |= ENABLEIDN_SCOPE ;
		}
#endif //SYMBIAN_DNS_PUNYCODE

	TInetAddr &addr = TInetAddr::Cast(aName.iAddr);
	if (aName.iFlags == 0)
		{
		iCurrentId = iNetworkId;
		if (addr.Family() != KAfInet6)
			addr.ConvertToV4Mapped();
		addr.SetPort(0);
		if (addr.Scope() == 0)
			{
			// Pick default scope, if none specified.
			addr.SetScope(iProtocol.Interfacer().RemoteScope(addr.Ip6Address(), iNetworkId, EScopeType_NET));
			}
		iNoNext = 0;	// Allow Next Processing
		}
	else if (iNoNext)
		{
		QueryComplete(KErrNotFound);
		return;
		}
	Submit();
	}

#ifdef SYMBIAN_DNS_PUNYCODE
// CHostResolver::EnableIdn
// ****************
/**
// @param	None	from the start of the message to the start of domain name
// @returns
//	@li	ETrue , if the IDN support is enabled
//	@li	EFalse, if the IDN support is disabled
*/
// ************************
TBool CHostResolver::IsIdnEnabled()
/** 
 * IDN Support Enabler
 * This implements the RHostResolver::EnableIdn from the application.
 * 
 * @param aEnable Boolean variable
 */
	{
	return (iEnableIdn == 1)? ETrue : EFalse;
	}
#endif //SYMBIAN_DNS_PUNYCODE

// CHostResolver::Query
// ********************
void CHostResolver::Query(const TDesC8& aQuery, TDes8& aResult, TInt aCount)
	/**
	* Query handler.
	* This implements the RHostResolver::Query and RHostResolver::Next() (for the query) from the application.
	*
	* @param aQuery The query
	* @retval aResult The reply buffer
	* @param aCount The next indicator (0 = first query, > 0 = next).
	*/
	{
	ASSERT(!IsPending());	// Cannot be in pending state!
	iCurrentId = iNetworkId;
	if (aQuery.Length() > 0)
		{
		(void)new (&iRequest) TDnsRequest(aQuery, aResult, aCount);
		LOG(Log::Printf(_L("Query\tres HR[%u] SESSION %d Query(%d, %d, %d) NID=%d"),
					this, Session(), aQuery.Length(), aResult.MaxLength(), aCount, iNetworkId));
#ifdef SYMBIAN_DNS_PUNYCODE
	if(this->iEnableIdn)
		{
		iRequest.iScope |= ENABLEIDN_SCOPE ;
		}
#endif //SYMBIAN_DNS_PUNYCODE
		Submit();
		}
	else
		{
		LOG(Log::Printf(_L("Query\tres HR[%u]::BAD QUERY-ZERO LENGTH"), this));
		iNotify->QueryComplete(KErrArgument);
		}
	}

// CHostResolver::SecurityCheck
// ****************************
TInt CHostResolver::SecurityCheck(MProvdSecurityChecker *aChecker)
	/**
	* Check the security of RHostResolver user.
	* This represents the host resolver of some application. Check if this application
	* has the capability for network services. If it does not have the capability,
	* it can only query information that can be resolved locally (e.g. hosts file).
	*/
	{
	iSecurityChecker = aChecker;
	iHasNetworkServices = aChecker->CheckPolicy(KPolicyNetworkServices, 0) == KErrNone;
	return KErrNone;
	}
