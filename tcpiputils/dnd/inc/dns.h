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
// dns.h - name resolver DNS client module header
//

#ifndef __DNS_H__
#define __DNS_H__

/**
@file dns.h
DNS protocol proviver
@internalComponent	Domain Name Resolver
*/

#include "listener.h"
#include "message.h"
#include "dns_sock.h"
#include "hostname.h"

#ifdef EXCLUDE_SYMBIAN_DNS_PUNYCODE
#undef SYMBIAN_DNS_PUNYCODE
#endif //EXCLUDE_SYMBIAN_DNS_PUNYCODE

const TUint KDndNumRequests = KDndNumResolver * 2;

class CDndRecord;
class CDndDnsclient;
class CDndCache;
/**
// @brief	The request data
//
// TDndReqData is the DNS protocol side of the <b>query session</b> used by a
// <b>resolver</b>. The <b>resolvers</b> side is only visible via the MDnsResolver
// callback class. On the other hand, the visible services of this class
// for the resolver (CDndResolver) are defined by the MDnsSession. 
*/
class TDndReqData : public TDnsRequest, public MDnsSession, public MDnsServerListNotify
	{
public:
	//
	// MDnsSession API (methods callable by resolver)
	// (documented in the mixin class)
	virtual TInt NewQuery(const TDnsMessage &aQuery, TDnsServerScope aServerScope, TUint32 aFlags, TBool aSuffixSupportEnabled);
	virtual void CancelQuery();
	virtual TInt DoQueryL(const TTime &aRequestTime, const EDnsQType aQType);
	virtual TInt DoNext(TDnsMessageBuf &aReply, TInt aNext) const;
	void DoError(TInt aError, TUint aTTL);
	virtual TBool PickNewServer();
	virtual TBool PickDefaultServer();
	void Close();


	//
	// MDnsServerListNotify callback
	//
	// The notify callback from the server manager
	virtual void ServerListComplete(const TDnsServerFilter &aFilter, TInt aResult);
	//
	// Upcalls from DNS Socket
	//
	// Build a complete DNS message from request data
	virtual TBool Build(CDnsSocket &aSource, TMsgBuf &aBuf, TInetAddr &aServer, TInt aMaxMessage);
	// Handle a reply message for the request (request remains queued)
	virtual TBool Reply(CDnsSocket &aSource, const TMsgBuf &aBuf, const TInetAddr &aServer);
	// The message has been sent (request remains queued)
	virtual void Sent(CDnsSocket &aSource);
	// The request has been aborted (not queued anymore)
	virtual void Abort(CDnsSocket &aSource, const TInt aReason);

	// Generate the callback event for the resolver owning this request
	void SendResponse(TInt aErr);
protected:
	//
	// Internal utilities
	//
	// Check the message question against request data
	TInt CheckQuestion(const TMsgBuf &aMsg, TInt &aRCode) const;
	// Check request against responce and translate RCODE value.
	TInt TranslateRCODE(const TDndHeader &aHdr, TInt aRCode) const;
	// Build a TNameRecord record from a RR
	TInt GetResponse(const TDndRR &aRR, TNameRecord &aReply) const;
	// Update DNS reply into cache
	TBool UpdateCacheData(const TMsgBuf &aMsg, const TInt aAnswerOffset, const TInt aErr);
	// Return instance ptr of the actual class instance
	virtual const void *const Instance() const {return this; }

public:
	TUint iIsReqPending:1;	//< Resolver is waiting for a reply
	TUint iIsUsingTCP:1;	//< Set when TCP is in use, only a safeguard against bad DNS servers.
	TUint iIsNewQuery:1;	//< Set in NewQuery, cleared in DoQuery (used in deciding whether a new ID is needed)
	TUint iIsFromCache:1;	//< Set if iRecord was from cached
	TUint iOpcode:4;		//< OPCODE field of the header
#ifdef SYMBIAN_DNS_PUNYCODE
	TUint iIdnEnabled:1;	//< =1, support for IDN is enabled for this request.
#endif //SYMBIAN_DNS_PUNYCODE

	TUint16 iQdCount;
	TUint32 iFlags;			//< Mofidier flags for the query (RD, PQ etc).
	TDndQuestion iQuestion;	//< Query Information
	TDnsServerFilter iFilter;
	TUint32 iCurrentServer;

	CDndRecord *iRecord;	//< Pointer to the record in the cache is stored here when a DNS query
							//< is sent - for quick update of cache.
	// Callback section
	MDnsResolver *iCallback;
	// "Owner" source
	CDndDnsclient *iOwner;	//< Actual owner of the request data

	TUint iNetworkId;      //< NetworkId from the request message.
	RInetSuffixList iSuffixList;	//< Container to store the domain search list on the interface where the query is sent
	TBool iIsIncompleteHostName;	//< Flag sent on the query to identify queries that need to retried on suffixes
	TBool iCanResolveIncompleteName;	//< Flag to confirm incomplete name without domain suffix tried for resolution as it is
	THostName iActualQueryName;		//< To store actual query name while domain suffixes are being applied
	TBool iPendingSuffixExist;		//< Flag set when suffixes are exhausted
	TInt iFlowReqType;				//< Differentiates IMPLICIT/EXPLICIT
	TBool iSuffixSupportEnabled;	//< Flag to switch ON/OFF the suffix support
	};

class TInetAddressInfo;
class CDndDnsclient : public CDnsSocket, public MDnsSource
    {
	friend class CDndMonitor;
	friend class TDndReqData;
protected:
	CDndDnsclient(CDndEngine &aControl, MDnsServerManager &aServerManager);
	void ConstructL();
public:
	static CDndDnsclient *NewL(CDndEngine &aControl, MDnsServerManager &aServerManager);

	void HandleCommandL(TInt aCommand);
	virtual ~CDndDnsclient();
	// Configuration has changed
	virtual void ConfigurationChanged() = 0;

	// Assign a session slot
	MDnsSession *OpenSession(MDnsResolver *const aCallback);
	// Check if there is a route and valid source address for the aDestination
	TInt CheckAddress(const TInetAddr &aDestination);
	// Resolver has received a query from application
	void QueryBegin();
	// Resolver has completed the query from application
	void QueryEnd();

protected:
	// Received a Query Message from some server -- ignore by default
	void Query(const TMsgBuf &/*aBuf*/, const TInetAddr &/*aServer*/, const RSocket &/*aSocket*/) {}
	// Received an error indication for a server
	void Error(const TInetAddr &aServer, TInt aError);
	inline void ActivateSocketL(TUint aNetworkId = 0) { CDnsSocket::ActivateSocketL(aNetworkId); } // lint placebo

	CDndEngine &iControl;					//< The DND Engine
	MDnsServerManager &iServerManager;		//< The server manager
	TDndReqData	iDndReqData[KDndNumRequests];	//< Requests being served
	TUint iActivityCount;			//< Count of activity, shut DNS sockets when ZERO.
	CDndCache *iCache;				//< The Cache
	THostNames iHostNames;			//< The current hostnames
	};

#endif
