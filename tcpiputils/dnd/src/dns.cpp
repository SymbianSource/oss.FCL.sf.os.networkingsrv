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
// dns.cpp - the main client module of the DNS.
//

#include <e32math.h>
#include <in_sock.h>
#include <in6_opt.h>
#include "dns.h"
#include "dnd.hrh"		// EDndDump
#include <networking/dnd_err.h>
#include "engine.h"
#include "servers.h"
#include "cache.h"
#include "inet6log.h"
#ifdef LLMNR_ENABLED
#	include "llmnrresponder.h"
#endif
#include "dnd_ini.h"

#ifdef EXCLUDE_SYMBIAN_DNS_PUNYCODE
#undef SYMBIAN_DNS_PUNYCODE
#endif //EXCLUDE_SYMBIAN_DNS_PUNYCODE

class CDnsService : public CDndDnsclient
	{
public:
	CDnsService(CDndEngine &aControl, MDnsServerManager &aServerManager) : CDndDnsclient(aControl, aServerManager) {}
	void ConstructL();
	~CDnsService();
	virtual void ConfigurationChanged();

	TInt SetHostName(TUint32 aId, const THostName &aName);
	TInt GetHostName(TUint32 aId, THostName &aName, MDnsResolver &aCallback);

	TInt GetByAddress(TUint32 aId, const TInetAddr &aAddr, TInt aNext, THostName &aName);
	TInt GetByName(TUint32 aId, const THostName &aName, TInt aNext, TInetAddr &aAddr);
private:
#ifdef LLMNR_ENABLED
	CDndLlmnrResponder *iLlmnrResponder; //< Pointer to the LLMNR responder object
#endif
	TUint32 GetNetId(TUint32 aIndex);
	TInetAddressInfo *GetAddressList(TInt &aN);
	};


void CDnsService::ConstructL()
	{
	CDndDnsclient::ConstructL();
#ifdef LLMNR_ENABLED
	iLlmnrResponder = new (ELeave) CDndLlmnrResponder(iControl, iServerManager, iHostNames);
	iLlmnrResponder->ConstructL();
#endif
	}

CDnsService::~CDnsService()
	{
#ifdef LLMNR_ENABLED
	delete iLlmnrResponder;
#endif
	}
	

void CDnsService::ConfigurationChanged()
	{
#ifdef LLMNR_ENABLED
	iLlmnrResponder->ConfigurationChanged();
#endif
	}
//
// Gateway hostname processing to the responder
//
TInt CDnsService::SetHostName(TUint32 aId, const THostName &aName)
	{
	const TInt res = iHostNames.Map(aId, aName);
#ifdef LLMNR_ENABLED
	return res == KErrNone ? iLlmnrResponder->SetHostName(aId, iHostNames.Find(aId)) : res;
#else
	return res;
#endif
	}

TInt CDnsService::GetHostName(TUint32 aId, THostName &aName, MDnsResolver &aCallback)
	{
	aName = iHostNames.Find(aId);
#ifdef LLMNR_ENABLED
	return iLlmnrResponder->GetHostName(aId, aCallback);
#else
	(void)aCallback;
	return KErrNone;
#endif
	}



TUint32 CDnsService::GetNetId(TUint32 aIndex)
	{
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iIndex = aIndex;
	return iControl.iSocket.GetOpt(KSoInetIfQueryByIndex, KSolInetIfQuery, opt) == KErrNone
		? opt().iZone[15] : 0;
	}

TInetAddressInfo *CDnsService::GetAddressList(TInt &aN)
	/**
	* Returns a list of all current addresses.
	*
	* @retval aN	The number of addresses (if return is non-NULL).
	* @return		The address list, or NULL if not available.
	*
	* The returned address list must be freed by the caller
	* as "delete[] list".
	*/
	{
	TPtr8 empty(NULL, 0);
	aN = iControl.iSocket.GetOpt(KSoInetAddressInfo, KSolInetIfQuery, empty);
	if (aN <= 0)
		return NULL;
	TInetAddressInfo *p = new TInetAddressInfo[aN];
	if (p == NULL)
		return NULL;	// No memory available.
	TPtr8 opt((TUint8 *)p, aN*sizeof(TInetAddressInfo));
	aN = iControl.iSocket.GetOpt(KSoInetAddressInfo, KSolInetIfQuery, opt);
	if (aN < 0)
		aN = 0;
	else
		aN = opt.Length() / (TInt)sizeof(TInetAddressInfo);
	return p;
	}


TInt CDnsService::GetByAddress(TUint32 /*aId*/, const TInetAddr &aAddr, TInt /*aNext*/, THostName &aName)
	/**
	* Returns the local hostname, if address is my own.
	*/
	{
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iSrcAddr = aAddr;
	if (iControl.iSocket.GetOpt(KSoInetIfQueryBySrcAddr, KSolInetIfQuery, opt) != KErrNone)
		return KErrNotFound;
	aName = iHostNames.Find(opt().iZone[15]);	// Locate hostname for network id.
	return (aName.Length() > 0) ? KErrNone : KErrNotFound;
	}

TInt CDnsService::GetByName(TUint32 aId, const THostName &aName, TInt /*aNext*/, TInetAddr &aAddr)
	/**
	* Returns one of my own addresses, if the hostname is local.
	*/
	{
	const TDesC &name = iHostNames.Find(aId);
	if (aName.Length() > 0 && name.CompareF(aName) != 0)
		return KErrNotFound;	// The query name is not a local host.
	//
	// Locate a local addres from some interface within the specific network.
	//
	TInt N;
	TInetAddressInfo *list = GetAddressList(N);
	if (list == NULL)
		return KErrNotFound;

	TUint32 iface = 0;
	TUint32 netid = 0;
	TInt scope = -1;
	TInt best = 0;
	for (TInt i = 0; i < N; ++i)
		{
		if (list[i].iState != TInetAddressInfo::EAssigned)
			continue;
		if (list[i].iInterface != iface)
			{
				iface = list[i].iInterface;
				netid = GetNetId(iface);
			}
		if (netid != aId)
			continue;	// Not this address.
		if (list[i].iAddress.Scope() > scope)
			{
			scope = list[i].iAddress.Scope();
			best = i;
			}
		}
	aAddr.SetAddress(list[best].iAddress);
	aAddr.SetScope(list[best].iScopeId);
	delete[] list;
	return scope < 0 ? KErrNotFound : KErrNone;
	}

//
//

CDndDnsclient *CDndDnsclient::NewL(CDndEngine &aControl, MDnsServerManager &aServerManager)
	{
	LOG(Log::Printf(_L("CDndDnsclient::NewL()")));

	CDnsService *const dns = new (ELeave) CDnsService(aControl, aServerManager);
	CleanupStack::PushL(dns);
	dns->ConstructL();
	CleanupStack::Pop();
	return dns;
	}

CDndDnsclient::CDndDnsclient(CDndEngine &aControl, MDnsServerManager &aServerManager)
	: CDnsSocket(aControl.GetConfig().iEDNS0), iControl(aControl), iServerManager(aServerManager)
	{
	}

void CDndDnsclient::ConstructL()
	{
	LOG(Log::Printf(_L("CDndDnsclient::ConstructL() size=%d"), (TInt)sizeof(*this)));
	CDnsSocket::ConstructL();

	iCache = new (ELeave) CDndCache();
	iCache->ConstructL();

	TPtrC localhost;
	if(iControl.FindVar(DND_INI_HOST, DND_INI_HOSTNAME, localhost))
		(void)iHostNames.Reset(localhost);
	else
		(void)iHostNames.Reset(KDndIni_Hostname);
	}




void CDndDnsclient::HandleCommandL(TInt aCommand)
	{
#ifdef _LOG
	switch(aCommand)
		{
		case EDndDump:
			if (iCache)
				iCache->Dump(iControl);
			break;
		default:
			break;
		}
#endif
	if (aCommand == EDndFlush)
		iCache->Flush();
	}

CDndDnsclient::~CDndDnsclient()
	{
	DeactivateSocket();

	// Just to be sure, cancel server list notifys (if any)
	// (this means that SERVER MANAGER MUST NOT BE DELETED
	// before this object!)
	for (TUint i = 0; i < KDndNumRequests; i++)
		iServerManager.CloseList(iDndReqData[i].iFilter);
	delete iCache;
	}

// CDndDnsclient::CheckAddress
// ***************************
TInt CDndDnsclient::CheckAddress(const TInetAddr &aDestination)
	{
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iDstAddr = aDestination;
	
	const TInt err = iControl.iSocket.GetOpt(KSoInetIfQueryByDstAddr, KSolInetIfQuery, opt);
	if (err == KErrNone && TInetAddr::Cast(opt().iSrcAddr).IsUnspecified())
		return KErrNotFound;	// No route or source address.
	return err;
	}



// TDndReqData::PickDefaultServer
// ********************************
TBool TDndReqData::PickDefaultServer()
	{
	TInetAddr tmp;
	TBool ret = FALSE;

	if (iCurrentServer == 0)
	    {
		ret = iOwner->iServerManager.OpenList(iFilter, this) > 0;
		// if suffix support is enabled in the resolver.ini
		// and query request had been initiated from a implicitly connected host resolver
		// Perform checks to find appropriate interface that can support the domain name
		// and update the server filter for the network id it should forward its request to
		if (iSuffixSupportEnabled && iFlowReqType == 0)
			iOwner->iServerManager.UpdateDomain(iFilter);
#ifdef _LOG
		if (iNetworkId != iFilter.iLockId)
			Log::Printf(_L("\t\tDNS session [%u] domain suffix match resulted in change of net id from %d to %d"), (TInt)this, iNetworkId, iFilter.iLockId);
#endif
	    }
	iOwner->iCache->GetServerAddress(iOwner->iServerManager.NameSpace(iFilter, iCurrentServer), tmp);
	iFilter.iServerId = iOwner->iServerManager.ServerId(tmp);
	iCurrentServer = iOwner->iServerManager.Next(iFilter, 0);
#ifdef _LOG
	if (iCurrentServer)
		Log::Printf(_L("\t\tDNS session [%u] default nameserver (id=%d) assigned"), (TInt)this, (TInt)iCurrentServer);
	else
		Log::Printf(_L("\t\tDNS session [%u] no nameservers available for this session"), (TInt)this);
#endif
	return ret || iCurrentServer != 0;
	}


// TDndReqData::PickNewServer
// ****************************
TBool TDndReqData::PickNewServer()
	{
	if (iCurrentServer == 0)
		iOwner->iServerManager.BuildServerList();
	iCurrentServer = iOwner->iServerManager.Next(iFilter, iCurrentServer);
#ifdef _LOG
	if (iCurrentServer)
		Log::Printf(_L("\t\tDNS session [%u] next nameserver (id=%d) assigned"), (TInt)this, (TInt)iCurrentServer);
	else
		Log::Printf(_L("\t\tDNS session [%u] next, no alternate nameservers to assign"), (TInt)this);
#endif
	return iCurrentServer != 0;
	}

void TDndReqData::ServerListComplete(const TDnsServerFilter &aFilter, TInt aResult)
	{
	if (&aFilter == &iFilter)
		{
		iCurrentServer = iOwner->iServerManager.Next(iFilter, 0);
		if (iIsReqPending)
			{
			if (aResult == KErrNone)
				{
				if (PickDefaultServer())
					{
					iOwner->ReSend(*this);
					aResult = KDnsNotify_HAVE_SERVERLIST;
					}
				else
					aResult = KErrDndServerUnusable;
				}
			SendResponse(aResult);
			}
		}
	}


// CDndDnsclient::OpenSession
// **************************
/**
// A session is internally represented by a TDndReqData. Find an unused
// entry and assign it. Unused entry is recognized by it having no
// callback function (= NULL).
*/
MDnsSession *CDndDnsclient::OpenSession(MDnsResolver *const aCallback)
	{
	TUint i;
	//
	// Locate available TDndReqData entry...
	//
	for (i = 0;; ++i)
		{
		if (i >= KDndNumRequests)
			return NULL;	// No "sessions" available
		// For now, assume the slot is available, if no callback has been installed
		if (iDndReqData[i].iCallback == NULL)
			break;
		}
	TDndReqData &rq = iDndReqData[i];
	rq.iCallback = aCallback;
	rq.iOwner = this;
	iActivityCount++;	// Count each open session as "activity".
	return &rq;
	}


void CDndDnsclient::Error(const TInetAddr &aServer, TInt /*aError*/)
	{
	const TUint server_id = iServerManager.ServerId(aServer);
	if (server_id == 0)
		return;		// -- nothing to do, address not registered as a server

	//
	// Try to notify all sessions currently using this server
	//
	for (TUint i = 0; i < KDndNumRequests; ++i)
		{
		TDndReqData &rq = iDndReqData[i];
		// For now, assume the slot is in used, if callback has been installed
		if (rq.iCallback == NULL)
			continue;
		if (rq.iCurrentServer == server_id)
			rq.SendResponse(KErrDndServerUnusable);
		}
	}



// TDndReqData::Close
// ******************
void TDndReqData::Close()
	{
	CancelQuery();
	iCallback = NULL;
	if (iRecord)
		{
		iRecord->Unlock(this);
		iRecord = NULL;
		}
	iOwner->iServerManager.CloseList(iFilter);
	// If this was the last active session, then
	// cancel all requests 
	ASSERT(iOwner->iActivityCount > 0);
	if (--iOwner->iActivityCount == 0)
		iOwner->DeactivateSocket();
	}

// TDndReqData::NewQuery
// ***********************
TInt TDndReqData::NewQuery(const TDnsMessage &aQuery, TDnsServerScope aServerScope, TUint32 aFlags, TBool aSuffixSupportEnabled)
	{
	iIsReqPending = FALSE;
	iIsNewQuery = TRUE;
	iIsUsingTCP = 0;
	iQdCount = 1;
	iFlags = aFlags;
	iFilter.iServerScope = aServerScope;
	iFilter.iServerId = 0;
	iNetworkId = aQuery.iId;  			// Get the networkId information from the Query.
	iFlowReqType = aQuery.iFlowRequestType; // Get the IMPLICIT/EXPLICIT differentiation from the Query
	iSuffixSupportEnabled = aSuffixSupportEnabled; // Whether suffix support enabled in the resolver.ini
	iIsIncompleteHostName = FALSE;		// Whether the query name is not fully qualified name
	iCanResolveIncompleteName = TRUE;	// Flag to confirm incomplete name without domain suffix tried for resolution as it is
	iPendingSuffixExist = FALSE;		// Identifies whether all of the available suffixes has been tried out
	iFilter.iDomainName.FillZ();
	iFilter.iDomainName.SetLength(0);

#ifdef SYMBIAN_DNS_PUNYCODE
	if( (aQuery.iScope & 0x80) == 0x80 )
		{
		iIdnEnabled = 1;
		iQuestion.EnableIdn(ETrue);
		}
	else 
		{
		iIdnEnabled = 0;
		iQuestion.EnableIdn(EFalse);
		}
#endif //SYMBIAN_DNS_PUNYCODE

	switch (aQuery.iType)
		{
		case KDnsRequestType_GetByName:
		case KDnsRequestType_GetByAddress:
			{
			const TNameRecord &query = aQuery.NameRecord();

			if (aQuery.iType == KDnsRequestType_GetByName)
				{
				THostName queryName(query.iName);
	
				// if the query is a qualified name, pick the domain name from the query
				// and update the server filter for the performing interface selection
				TInt posOfDomainNameStart = queryName.Locate('.');
			    if (posOfDomainNameStart != KErrNotFound)
			        {
			        iFilter.iDomainName = queryName.Mid(posOfDomainNameStart+1);
			        }
			        
				iFilter.iLockId = aQuery.iId;
				iFilter.iLockType = KIp6AddrScopeNetwork;
#ifdef SYMBIAN_DNS_PUNYCODE
				TInt err = iQuestion.SetName(queryName);
				if( err != KErrNone)
					{
					return err;
					}
				iActualQueryName.Copy(queryName);
#else
				iQuestion.SetName(queryName);
				iActualQueryName.Copy(queryName);
#endif // SYMBIAN_DNS_PUNYCODE
				}
			else
				{
				iOwner->iServerManager.LockByAddress(TInetAddr::Cast(query.iAddr), aQuery.iId, iFilter);
#ifdef SYMBIAN_DNS_PUNYCODE
				TInt err = iQuestion.SetName(TInetAddr::Cast(query.iAddr));
				if( err != KErrNone)
					{
					return err;
					}
#else
				iQuestion.SetName(TInetAddr::Cast(query.iAddr));
#endif // SYMBIAN_DNS_PUNYCODE
				}
			}
			break;
		case KDnsRequestType_TDnsQuery:
			{
			const TDnsQuery &query = aQuery.Query();
			iQuestion.Copy(query.Data());
			iFilter.iLockId = aQuery.iId;
			iFilter.iLockType = KIp6AddrScopeNetwork;
			}
			break;
		default:
			return KErrNotSupported;
		}
	iCurrentServer = 0;
	//
	// For forward query, the Scope identifier of the aQuery.iAddr is the network id
	// For pointer query, if the address is not of network scope, then need to find
	// the network id based on the scope id... (fix later). -- msa

	iOpcode = EDnsOpcode_QUERY;		// Only EStandard supported! (EInverse is not same as PTR query!!!)
	if (iRecord)
		{
		iRecord->Unlock(this);
		iRecord = NULL;
		}
	return KErrNone;
	}


// TDndReqData::CancelQuery
// **************************
void TDndReqData::CancelQuery()
	{
	iIsReqPending = FALSE;
	Cancel();
	iOwner->iServerManager.CloseList(iFilter);
	}


// TDndReqData::DoNext
// *********************
/**
// @retval aReply	returns the requested Resource Record value, if KErrNone return
// @param aNext		the index of the value to be returned. 0 is the index of the first value
// @returns
//	@li	KErrNotFound, if there is no value at specified index
//	@li	KErrDndCache, if the reply from DNS stored in cache was corrupt
//	@li	KErrNone, if value successfully returned
*/
TInt TDndReqData::DoNext(TDnsMessageBuf &aReply, TInt aNext) const
	{
	if (iRecord == NULL)
		return KErrNotFound;
	TInt ret = iRecord->ErrorCode();
	if (ret < 0)
		return ret;	// No usable record present

	TDndRR tempRR(iRecord->Reply());
#ifdef SYMBIAN_DNS_PUNYCODE
		tempRR.iIdnEnabled = TBool(iIdnEnabled);
#endif //SYMBIAN_DNS_PUNYCODE

	const TInet6HeaderDNS &hdr = iRecord->Header();
	TInt answerCount = hdr.ANCOUNT();

	ret = tempRR.FindRR(iRecord->AnswerOffset(), answerCount, iQuestion.QType(), iQuestion.QClass(), aNext);
	if (ret < 0)
		{
		if (ret == KErrDndCache)
			iRecord->Invalidate();
		return ret;
		}

	TInetAddr *addr = NULL;

	switch (aReply().iType)
		{
		case KDnsRequestType_GetByName:
		case KDnsRequestType_GetByAddress:
			{
			TNameRecord &reply = aReply().NameRecord();
			aReply.SetLength(aReply().HeaderSize() + sizeof(reply));
			if (hdr.AA())
				reply.iFlags |= EDnsAuthoritive;
			ret = GetResponse(tempRR, reply);
			reply.iFlags |= EDnsServer | (iIsFromCache ? EDnsCache : 0);
			addr = &TInetAddr::Cast(reply.iAddr);
			break;
			}
#ifndef NO_DNS_QUERY_SUPPORT
		case KDnsRequestType_TDnsQuery:
			ret = tempRR.GetResponse(aReply, &addr);
			break;
#endif
		default:
			return KErrNotSupported;
		}
	if (ret < 0)
		return ret;		// Some error detected!

	if (addr)
		{
		// Reply contains a TInetAddr. This needs some special processing

		// Supplement IPv6 addresses with the scope value
		// (Should do the same with IPv4 after converting to IPv4 mapped?)
		if (addr->Family() == KAfInet)
			addr->ConvertToV4Mapped();
		if (addr->Family() == KAfInet6)
			{
			const TInt scope_level = addr->Ip6Address().Scope();

			// Add the scope id only for addresses with larger than node
			// local scope (this leaves the scope id as zero, if a loopback
			// address is returned from the name server (The id of the node
			// local scope is the interface index and non-zero value would
			// bind loopback destination to real interface instead of internal
			// loopback interface).
			if (scope_level > KIp6AddrScopeNodeLocal)
				addr->SetScope(iOwner->iServerManager.Scope(iRecord->Server(), *addr));

#ifdef LLMNR_ENABLED
			if(iOwner->iControl.GetConfig().iLlmnrLlOnly) // accept only linklocal replies to LLMNR queries
				if (iFilter.iServerScope == EDnsServerScope_MC_LOCAL)
					{
					// Check compliance w. link-local addressing requirements (ipv6/ipv4)
					if (scope_level != KIp6AddrScopeLinkLocal)
						return KErrNotFound;
					}
#endif

			//
			// A backward compatibility hack: if address is IPv4 global address
			// and the network scope in the query matches the scope of the address,
			// then convert the returned address into old KAfInet format (and lose
			// lose the scope id).
			if (addr->IsV4Mapped() &&
				addr->Scope() == aReply().iId &&
				scope_level == KIp6AddrScopeNetwork)
				addr->SetAddress(addr->Address());
			}
		}
	return KErrNone;
	}

// TDndReqData::DoError
// **********************
/**
// If a session has a DNS reply in cache associated with it, then
// set the state of this reply to indicated error code.
//
// @param aError	the error code to be stored
// @param aTLL		the new "Time To Live" for the record (in seconds). The
//					record (and error state) will expire after this time.
*/
void TDndReqData::DoError(TInt aError, TUint aTTL)
	{
	if (aError >= 0)
		return;		// Only errors can be set!

	if (iRecord == NULL)
		return;
	iRecord->FillErrorCode(aError, aTTL);
	}

// TDndReqData::DoQueryL
// ***********************
/**
// Activate a query of specified type for the loaded query information
// (enable RecvReply callback). Note: the callback may occur already within call!
//
// @param	aRequestTime is the time when the request is received from the application
// @param	aQType	type of the query (all queries assume IN class)
// @returns
//	@li	< 0, serious error, (resolving process should be aborted)
//	@li	= 0, reply found from cache (reply callback has been called)
//	@li	= 1, DNS Query message has been queued for transmission
// @execption
//	LEAVE on any serious error (resolving process should be aborted)
*/
TInt TDndReqData::DoQueryL(const TTime &aRequestTime, const EDnsQType aQType)
	{
	// -- class is now always IN,
	// -- should only look from cache if aQType is not a "wildcard" type
	//    (however, if wildcard, cache should not give hits...)
	iQuestion.SetQType(aQType);
	// -- only IN class queries are supported
	iQuestion.SetQClass(EDnsQClass_IN);
	// Possible that the there is a different interface (other than the default)
	// that can resolve the domain name we are interested at.
	// By this time, we would have updated the netid in the filter, if at all
	iNetworkId = iFilter.iLockId;

	if (iRecord)
		{
		iRecord->Unlock(this);
		iRecord = NULL;
		}

	if (iCurrentServer == 0)
		{
		if (!PickDefaultServer())
			{
			SendResponse(KErrDndServerUnusable);
			return 0;
			}
		if (iCurrentServer == 0)
			{
			// Cannot check cache nor start any query, if the
			// name space id cannot be determined (no interfaces
			// up). Return "query queued" anyway, and let the
			// resolver retry process try it again later.
			iIsReqPending = TRUE;
			return 1;
			}
		}
	TUint32 id = iOwner->iServerManager.NameSpace(iFilter, iCurrentServer);

	if (id == 0)
		{
		SendResponse(KErrDndServerUnusable);
		return 0;
		}


	// Check to see if the query is not fully qualified
    THostName queryName;
    iQuestion.GetName(queryName);
    if (queryName.Locate('.') == KErrNotFound)
        {
		// If query is not fully qualified, set flags to mark this for further processing
        iIsIncompleteHostName = TRUE;
		// Pick the domain suffix list on the interface
	    iOwner->iServerManager.InterfaceSuffixList(iCurrentServer, iSuffixList);
		// If count of suffices is 0, mark this, so that we do not have to perform any special processing
        if (iSuffixList.Count() > 0)
            iPendingSuffixExist = TRUE;
        }

	// Check in the cache first. If the record does not exist
	// in the cache, it will be created now (empty with KErrNotFound).
	iRecord = iOwner->iCache->FindL(
			id,
			iQuestion,
			iQuestion.QType(),
			// *HACK WARNING* To achieve independent caching of answers which
			// are result of queries where RD=1 or RD=0 (recursion desired),
			// the Class value is made different depending on the RD state.
			(EDnsQClass)(iQuestion.QClass() | ((iFlags & KDnsModifier_RD) ? 0 : 0x80)),
			aRequestTime);
	// The above FindL must either leave of return a valid
	// record pointer. Just as a safety measure, if record
	// is not returned, then panic on in DEBUG builds, and
	// in release, leave with KErrDndNoRecord
	// (** however, this should never happen! **)
	ASSERT(iRecord != NULL);
	if (iRecord == NULL)
		User::Leave(KErrDndNoRecord);

	// Prevent record from being deleted while the
	// iRecord pointer exists...
	iRecord->Lock();
	switch(iRecord->ErrorCode())
		{
		// If no error in the record, retrieve the informations and send
		case KErrNone:	
		// For certain errors, send the error code
		case KErrDndBadName:
		case KErrDndNotImplemented:
		case KErrDndRefused:
		case KErrDndNoRecord:
			iIsFromCache = TRUE;
			iIsNewQuery = FALSE;
			if (IsQueued())
				Cancel();
			SendResponse(iRecord->ErrorCode());
			return 0;
		
		// For other errors, try sending the query to the DNS
		default:
			break;
		}

	// Automatic restart of DNS socket, if closed for some reason
	iOwner->ActivateSocketL(iNetworkId);  // pass on the networkId information

	iIsReqPending = TRUE;
	// If the query is probe, do not AssignWork but continue..
	if (!(iFlags & KDnsModifier_PQ) && !iRecord->AssignWork(this))
		return 1;	// just let the other worker do the job.

	// Try to detect retransmissions of the same query and
	// reuse old ID in such case... (don't generate a new)
	if (iIsNewQuery ||
		iQuestion.QType() != aQType ||
		iQuestion.QClass() != EDnsQClass_IN)
		Cancel();				// A new ID required

	iIsNewQuery = FALSE;
	iIsFromCache = FALSE;

	// If the DNS "mode" is Multicast DNS, then assume that PTR queries
	// which are generated from IP address are to be made via TCP. Test
	// this and use TCP if query is PTR query for valid IP address.
	// [specified in draft-ietf-dnsext-mdns-22.txt, but generalized here
	// also for any future Multicast DNS]
	for ( ;iFilter.IsMulticast() && iQuestion.QType() == EDnsQType_PTR;)
		{
		// Use "for" just for easy "break" exits!
		TInetAddr server;

		// Get correct DNS port number into 'server' (actual address is thrown away)
		if (iOwner->iServerManager.Address(iCurrentServer, server) != KErrNone)
			break;
		// Use server address type as a flag, whether Ipv4 or IPv6 is done
		const TInt is_ipv4 = server.IsV4Mapped();

		if (!iQuestion.GetAddress(server) || !server.IsUnicast())
			break;
		// Usually LLMNR has both IPv4 and IPv6 multicast addresses as "servers".
		// There is no point in doing PTR query for the same address twice, thus
		// only do it once per matching "server" address (thus, if there is no
		// IPv4 multicast "server", no IPv4 reverse queries are done either).
		if (is_ipv4 != server.IsV4Mapped())
			{
			SendResponse(KErrDndServerUnusable);
			return 0;
			}

		// Supplement 'server' address with a scope id based on current server
		server.SetScope(iOwner->iServerManager.Scope(iCurrentServer, server));

		// For link local multicast, use TTL = 1 (otherwise, system default is used)
		const TInt ttl = iFilter.iServerScope == EDnsServerScope_MC_LOCAL ? 1 : -1;
		if (iOwner->Queue(*this, server, -1, ttl) != KErrNone)
			break;
		iIsUsingTCP = 1;
		SendResponse(KDnsNotify_USING_TCP);
		return 1;
		}
	iOwner->ReSend(*this);		// (uses old ID, if request queued already)
	return 1;
	}

// TDndReqData::UpdateCacheData
// ******************************
/**
// @param aQuery	the session from which the reply is updated
// @param aMsg		the reply from the DNS server
// @param aAnswerOffset the start offset to the andwer secion in the reply
// @param aErr		the status code of the reply
// @returns
//	@li	TRUE, if cache record updated
//	@li	FALSE, if not updated (error code is "transient", concerns single query)
*/
TBool TDndReqData::UpdateCacheData(const TMsgBuf &aMsg, const TInt aAnswerOffset, const TInt aErr)
	{
	if (iRecord == NULL)
		return FALSE;

#ifdef _LOG
	THostName name;
	iQuestion.GetName(name);
	Log::Printf(_L("\t\tDNS session [%u] -- Update cache: %S (offset=%d) aErr=%d"), (TInt)this, &name, aAnswerOffset, aErr);
#endif
	if (aErr == KErrDndDiscard)
		return FALSE;

	// If there is already have valid answer cached, decide whether the new
	// answer is better and should replace the old?
	const TInet6HeaderDNS &new_hdr = aMsg.Header();
	if (iRecord->ErrorCode() == KErrNone && !new_hdr.AA())
		{
		// If new header is not authoritative, then it will replace the value
		// only if old is non-authoritative and there is no error.
		const TInet6HeaderDNS &old_hdr = iRecord->Header();
		if (old_hdr.AA() || aErr != KErrNone)
			return FALSE; // Not updated, keep previous valid value in cache.
		}

	TBool updated = FALSE;
	if (aErr == KErrNone || aErr == KErrDndBadName || aErr == KErrDndNotImplemented || aErr == KErrDndNoRecord)
		{
		// Try to locate the SOA record from the authority section and
		// use the minttl as the ttl of the negative caching.
		TInt ttl;
		TDndRR soa(aMsg);
#ifdef SYMBIAN_DNS_PUNYCODE
		soa.iIdnEnabled = (TBool) iIdnEnabled;
#endif //SYMBIAN_DNS_PUNYCODE
		TInt off = soa.LocateRR(aAnswerOffset, new_hdr.ANCOUNT()+new_hdr.NSCOUNT(), EDnsQType_SOA, EDnsQClass_IN, new_hdr.ANCOUNT());
		if (off < 0)
			{
			ttl = iOwner->iControl.GetConfig().iMaxTime;
			LOG(Log::Printf(_L("\t\tDNS session [%u] -- Update cache: no SOA, default ttl = %d"), (TInt)this, ttl));
			}
		else if ((off = aMsg.SkipName(soa.iRd)) > 0 &&
				 (off = aMsg.SkipName(off)) > 0 &&
				 off >= (TInt)soa.iRd &&
				 off + 20 <= (TInt)(soa.iRd + soa.iRdLength))
			{
			ttl = BigEndian::Get32(aMsg.Ptr()+off+16);
			LOG(Log::Printf(_L("\t\tDNS session [%u] -- Update cache: SOA minttl = %d"), (TInt)this, ttl));
			}
		else
			{
			ttl = 0;	// The reply is broken in some way, use ttl = 0
			LOG(Log::Printf(_L("\t\tDNS session [%u] -- Update cache: SOA access failed"), (TInt)this));
			}
		iRecord->FillData(aMsg, aAnswerOffset, iCurrentServer, aErr, ttl);
#ifdef SYMBIAN_DNS_PUNYCODE
		
		LOG(iRecord->Print(iOwner->iControl,(TBool)iIdnEnabled));
#else
		LOG(iRecord->Print(iOwner->iControl));
#endif //SYMBIAN_DNS_PUNYCODE
		updated = TRUE;
		}
	//
	// Got some answer from a server, update the "good" server (unless error indicates "bad"
	//
	if (iFilter.IsUnicast() && aErr != KErrDndRefused && aErr != KErrDndServerUnusable)
		{
		TInetAddr addr;
		if (iOwner->iServerManager.Address(iCurrentServer, addr) == KErrNone)
			{
#ifdef _LOG
			// borrow the 'name' from earlier LOG section!
			addr.OutputWithScope(name);
			Log::Printf(_L("\t\tDNS session [%u] -- Update cache: preferred server = %S port=%d ns=%d"),
				(TInt)this, &name, addr.Port(), iOwner->iServerManager.NameSpace(iFilter, iCurrentServer));
#endif
			iOwner->iCache->SetServerAddress(iOwner->iServerManager.NameSpace(iFilter, iCurrentServer), addr, KErrNone);
			}
		}

	return updated;
	}


void CDndDnsclient::QueryBegin()
	{
	++iActivityCount;
	}


void CDndDnsclient::QueryEnd()
	{
	ASSERT(iActivityCount > 0);
	if (--iActivityCount == 0)
		DeactivateSocket();
	}


//
// TDndReqData
//

// TDndReqData::TranslateRCODE
// ***************************
/**
//
// @param	aHdr	The fixed DNS reply header
//
// @returns
//	@li	KErrNone, if reply is ok
//	@li	KErrDndDiscard/KErrDndUnknown
//		if reply does not match the query, has errors
//		in the format or RCODE was unknown
//	@li	KErrDndFormat, RCODE was EDnsRcode_FORMAT_ERROR
//	@li	KErrDndServerFailure, RCODE was EDnsRcode_SERVER_FAILURE
//	@li	KErrDndBadName, RCODE was EDnsRcode_NAME_ERROR
//	@li	KErrDndNotImplemented, RCODE was EDnsRcode_NOT_IMPLEMENTED
//	@li	KErrDndRefuced, RCODE was EDnsRcode_REFUSED
//	@li	KErrDndServerUnusable, if empty reply is not authoritative
*/
TInt TDndReqData::TranslateRCODE(const TDndHeader &aHdr, TInt aRCode) const
	{
	switch (aRCode) 
		{
		case EDnsRcode_NOERROR:
			break;
		case EDnsRcode_FORMAT_ERROR:
			return KErrDndFormat;
		case EDnsRcode_SERVER_FAILURE:
			/*
			In case the server returns the server failure, we just ignore the error code and treat it as server unusable. so that the query is sent to the other available servers for resolution. Need more reasonable solution ???-- 
			return KErrDndServerFailure;  */
			return KErrDndServerUnusable;
		case EDnsRcode_NAME_ERROR:
			return KErrDndBadName;
		case EDnsRcode_NOT_IMPLEMENTED:
			return KErrDndNotImplemented;
		case EDnsRcode_REFUSED:
			return KErrDndRefused;
		default:
			return KErrDndUnknown;
		}	

	if (iOpcode == EDnsOpcode_QUERY && aHdr.QDCOUNT() != iQdCount)
		return KErrDndDiscard;
	//
	// A special heuristics: discard empty replies, if the selected server does
	// not do recursion as requested, and if it is not authority on the queried
	// name. => return a special "server unusable for this query" error
	if (aHdr.ANCOUNT() == 0 && (iFlags & KDnsModifier_RD) != 0 && !aHdr.RA() && !aHdr.AA())
		return KErrDndServerUnusable;
	return KErrNone;
	}

// TDndReqData::CheckQuestion
// **************************
/**
// @param	aOffset starting offset of the question section in the message
// @param	aMsg the reply message from a DNS server
//
// @returns
//	@li	> 0,
//		if message checks ok, the value is the new offset pointing
//		to the next section after the question.
//	@li	= 0, reply does not match the question.
//	@li < 0, bad DNS reply
*/
TInt TDndReqData::CheckQuestion(const TMsgBuf &aMsg, TInt &aRCode) const
	{
	if (!iIsReqPending)
		return 0;	// Not for me.

	TDndQuestion question;
	const TInt offset = aMsg.VerifyMessage(aRCode, question);
	if (offset < 0)
		return offset;	// Invalid message format, just ignore.
	const TDndHeader &hdr = aMsg.Header();

	// Only ONE question supported, sematics of receiving a reply
	// with more than one Question are hairy... [which answers
	// relate to which question?] (and multiple questions are
	// not supported by current servers anyway -- msa)
	if (hdr.QDCOUNT() != 1)
		return KErrDndUnknown;

	// This is supposed to be a REPLY, if not, then "no match".
	if (!hdr.QR())
		return 0;
	// Reply OPCODE match the query?
	if (hdr.OPCODE() != iOpcode)
		return KErrDndDiscard;

	// Does the question match the query?
	//
	if (question.CheckQuestion(iQuestion) != KErrNone)
		return 0;
	return offset;
	}


// TDndReqData::GetResponse
// ************************
/**
// Map the contents of single resource record into TNameRecord.
//
// @param	aRR	the resource from which the reply is extracted
// @retval	aAnswer receives the extracted value
// @returns
//	@li	KErrNone, if extraction successful
//	@li	KErrDndNameTooBig, if answer cannot be fit into aAnswer
//	@li and other errors
*/
TInt TDndReqData::GetResponse(const TDndRR &aRR, TNameRecord &aAnswer) const
	{
	TInt err = aRR.GetResponse(aAnswer.iName, TInetAddr::Cast(aAnswer.iAddr));
	if (err != KErrNone)
		return err;
	aAnswer.iFlags |= (aRR.iType == EDnsType_CNAME) ? (EDnsAlias | EDnsServer) : EDnsServer;
	return KErrNone;
	}


// TDndReqData::SendResponce
// *************************
/**
// @param	aErr is the status,
//	@li	= 0, the request has completed successfully
//	@li > 0, request being processed, just a progress noticification
//	@li	< 0, the request has completed with an error
*/
void TDndReqData::SendResponse(TInt aErr)
	{
	if (aErr <= 0)
		{
		iIsReqPending = FALSE;	// Current request completed
		Cancel();
		}
	if (iCallback)
		iCallback->ReplyCallback(aErr);
	}

// TDndReqData::Build
// ******************
/**
// @retval	aMsg
//		contains the fully constructed message to be sent to the DNS server,
//		if Build succeeds
// @retval	aServer
//		contains the server address for which the message should be sent
// @param	aMaxMessage
//		the size of the current receive buffer (if UDP, zero for TCP)
//
// @returns TRUE, successful Build, and error (< 0) otherwise
*/
TBool TDndReqData::Build(CDnsSocket &aSource, TMsgBuf &aMsg, TInetAddr &aServer, TInt aMaxMessage)
	{
	CDndDnsclient &dns = (CDndDnsclient &)aSource;

	if (dns.iServerManager.Address(iCurrentServer, aServer) != KErrNone)
		return 0;
	ASSERT(aServer.Port() != 0);

	// If the query us incomplete and we have pending domain suffix that can be applied
	// lets try to update the query name and forward it to the next level
    if ( iIsIncompleteHostName )
        {
		if ( iPendingSuffixExist )
			{
	        THostName queryName;
	        iQuestion.GetName(queryName);
	        
	        if ( queryName.Locate('.') == KErrNotFound )
	            {
				// If the query does not have a '.', we understand that the query has not tried 
				// any of the suffixes. So, lets apply the first domain suffix from the interface
	            TInt newLength = queryName.Length() + iSuffixList[0].Length();
	            THostName qName(queryName);
	            qName.Append(_L("."));
	            
	            // If the query name + suffix name exceeds allowed limit for length (256)
	            // truncate the suffix name while appending to the min possible length
	            // this automagically skips from getting resolved as the buffer does not have 
	            // enough space to append configuration msg along with it
	            if (newLength < KMaxHostNameLength)
	                qName.Append(iSuffixList[0]);
	            else
	                qName.Append(iSuffixList[0].Mid(0, KMaxHostNameLength - qName.Length()));
	            
	            iQuestion.SetName(qName);
	            
	            if ( iSuffixList.Count() == 1 )
	                iPendingSuffixExist = FALSE;
	                
	            LOG(Log::Printf(_L("\t\t Query name after appending suffix = %S"), &qName));
	            }
	        else
	            {
				// As we dont have a '.', we understand that there is some domain suffix already tried
				// and we have reached here because, we were not able to resolve with that domain suffix
				// Now, lets figure out in sequence, as to which one was applied previously
				// so that we shall apply the next one in the list and forward it for resolution
	            for (TInt index=0; /*iPendingSuffixExist*/ ; index++ )
	                {
	                // Crop the leadng query name and '.' from the previous query and check to see
	                // which suffix in the list was earlier applied

	                TSuffixName queryNameSuffix( queryName.Mid( iActualQueryName.Length()+1 ) );
	                TSuffixName domainSuffix(iSuffixList[index]);
	                TInt suffixLength = domainSuffix.Length();
	                TInt completeLength = suffixLength + iActualQueryName.Length() + 1;
	                
	                if (completeLength > KMaxHostNameLength)
	                	suffixLength = KMaxHostNameLength - (iActualQueryName.Length() + 1);
	                	
	                if ( queryNameSuffix.Compare( domainSuffix.Mid( 0, suffixLength ) ) != KErrNone )
	                    continue;
	                
                    index++;
                    TInt newLength = iActualQueryName.Length() + iSuffixList[index].Length();
                    THostName qName(iActualQueryName);
                    qName.Append(_L("."));
                    
                    if (newLength < KMaxHostNameLength)
                        qName.Append(iSuffixList[index]);
                    else
                        qName.Append(iSuffixList[index].Mid(0, KMaxHostNameLength - qName.Length()));
                    
                    iQuestion.SetName(qName);
                    
                    if ( iSuffixList.Count() == index+1 )
                        iPendingSuffixExist = FALSE;

                    LOG(Log::Printf(_L("\t\t Query name after appending suffix = %S"), &qName));
                    break;
	                }
	            }
	        }
	    else /* if ( iCanResolveIncompleteName )*/
	        {
	        iQuestion.SetName(iActualQueryName);
	        LOG(Log::Printf(_L("\t\t Query name after appending suffix = %S"), &iActualQueryName));
	        iCanResolveIncompleteName = FALSE;
	        }
		}

    
	aMsg.SetLength(sizeof(TDndHeader));
	TDndHeader &hdr = (TDndHeader &)aMsg.Header();
	if (aServer.IsMulticast())
		{
		ASSERT(iFilter.IsMulticast());
		hdr.SetRD(0);
		if (aServer.IsV4Mapped())
			{
			if(iQuestion.QType() == EDnsQType_AAAA)
				{
				SendResponse(KErrDndServerUnusable);
				return 0;
				}
			}
		else
			{

			if(iQuestion.QType() == EDnsQType_A)
				{
				SendResponse(KErrDndServerUnusable);
				return 0;
				}
			}
#ifdef LLMNR_ENABLED
		dns.SetHoplimit(dns.iControl.GetConfig().iLlmnrHoplimit);
#endif
		}
	else
		{
#ifdef LLMNR_ENABLED
		dns.SetHoplimit(-1);
#endif
		hdr.SetRD(iFlags & KDnsModifier_RD);
		}
	hdr.SetQdCount(1);
	if (iQuestion.Append(aMsg) < 0)
		return 0;

	// Assume EDNS0 enabled, if the current receive buffer
	// is larger than KDnsMaxMessage (all smaller
	// values are treated as "no EDNS0".
	if (aMaxMessage > KDnsMaxMessage)
		{
		TDndRROut opt(aMsg);
#ifdef SYMBIAN_DNS_PUNYCODE
		opt.iIdnEnabled = (TBool) iIdnEnabled;
#endif //SYMBIAN_DNS_PUNYCODE
		opt.iType = (TUint16)EDnsQType_OPT;
		opt.iClass = (TUint16)aMaxMessage;
		opt.iTTL = 0;	// RCODE = 0, version = 0, Z = 0
		if (opt.Append(KNullDesC8, 0) == KErrNone)
			{
			hdr.SetArCount(1);
			opt.AppendRData();
			}
		}
	return 1;
	}


void TDndReqData::Sent(CDnsSocket &aSource)
	{
	CDndDnsclient &dns = (CDndDnsclient &)aSource;

	dns.iServerManager.CloseList(iFilter);

	SendResponse(KDnsNotify_QUERY_SENT);
	}


TBool TDndReqData::Reply(CDnsSocket &aSource, const TMsgBuf &aBuf, const TInetAddr &aServer)
	{
	TInt rcode = 0;
	const TInt offset = CheckQuestion(aBuf, rcode);
	if (offset < 0)
		return 1;	// Invalid message format, just ignore.
	const TDndHeader &hdr = aBuf.Header();
	TInt err = TranslateRCODE(hdr, rcode);

	CDndDnsclient &dns = (CDndDnsclient &)aSource;
	ASSERT(&dns == iOwner);
	// If configuration requests that "Not found" replies from a server
	// are not to be cached, but ignored, then substitue err with
	// KErrDndServerUnusable (meaning that this server is not usable for
	// this query) This error is not cached!
	//
	if (err == KErrDndBadName && dns.iControl.GetConfig().iSkipNotFound)
		err = KErrDndServerUnusable;
	else if (iIsUsingTCP == 0 && hdr.TC() != 0)
		{
		// The current query was not TCP and got truncated reply,
		// restart query with TCP (if we get truncated reply with
		// TCP, something is broken...)
		//

		// It is assumed that the aServer has the correct port
		// already set (it should be the remote port of the original
		// UDP query)
		ASSERT(aServer.IsUnicast());	// Should always be true.
		if (aServer.IsUnicast())
			{
			if (iOwner->Queue(*this, aServer, hdr.ID()) == KErrNone)
				{
				iIsUsingTCP = 1;
				SendResponse(KDnsNotify_USING_TCP);
				return 1;
				}
			}
		// If cannot use the TCP, then just use the trunctated
		// answer as is...
		}
		
	// If a incomplete query has failed to resolve after application of a domain suffix
	// and we know that we still have some pending suffix that can be applied
	// let us retry with the available suffices
	LOG(Log::Printf(_L("Error from name resolution is %d"),err));
    if( err == KErrDndBadName && iIsIncompleteHostName && (iPendingSuffixExist || iCanResolveIncompleteName))
        {
        iOwner->ReSend(*this, ETrue); // Set the retryWithSuffix flag to TRUE so that a new request ID is assigned
        return 1;
        }

	//
	// UpdateCacheData updates data in cache only, if err is KErrNone, or
	// updates error status for some specific err codes,
	// otherwise, it does nothing.
	TBool updated = UpdateCacheData(aBuf, offset, err);

#ifdef DEBUG_CACHE
	iCache->Dump(*iControl);
#endif

	if (updated)
		{
		// Cache modified! In addition to the original query,
		// send the responce to every request that is waiting for
		// the same reply... (iRecord pointers are same).
		for (TUint i = 0; i < KDndNumRequests; ++i)
			{
			TDndReqData &rq = dns.iDndReqData[i];
			if (rq.iIsReqPending && iRecord == rq.iRecord)
				{
				rq.Cancel();	// No need to send the query
				rq.SendResponse(err);
				}
			}
		return 1;
		}

	// Cache not updated, send to original query only.
	SendResponse(err);
	return 1;
	}

void TDndReqData::Abort(CDnsSocket &/*aSource*/, const TInt aReason)
	{
	SendResponse(aReason);
	}
