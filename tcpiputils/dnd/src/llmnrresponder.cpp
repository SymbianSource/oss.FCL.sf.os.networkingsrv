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
// llmnrresponder.cpp - DND Link-local Multicast Name Resolution 
// responder module 
//

#ifdef LLMNR_ENABLED
#include <e32math.h>
#include <in_iface.h>
#include <in6_if.h>
#include "engine.h"
#include "llmnrresponder.h"
#include "llmnrconflict.h"
#include <networking/dnd_err.h>
#include "serverconf.h"
#include "inet6log.h"


enum TEntryState
	{
	EInactive = 0,	//< Entry is not yet available
	EUnique = 1,	//< Entry is unique and alive
	EDisabled = -1,	//< Entry is faulty, disabled
	EConflict = -2	//< Entry has conflict with another host
	};

class CNotifyConflict : public CActive
	/**
	Exists while a conflict notifier process is active.
	*/
	{
private:
	CNotifyConflict(CDndLlmnrResponder &aResponder, CLlmnrEntry &aEntry);
	void DoStart();
public:
	~CNotifyConflict();
	static void Start(CDndLlmnrResponder &aResponder, CLlmnrEntry &aEntry);

	void DoCancel();
	void RunL();

	CDndLlmnrResponder &iResponder;
	CLlmnrEntry &iEntry;
	TPckgBuf<TLlmnrConflictNotify> iInfo;
	RNotifier iNotifier;
	TUint iConnected:1;
	};

class CLlmnrEntry : public CBase
	/**
	Describes an answer (RR) that can be asked from this node.

	An instance of this object is created for each unique answer
	which this node is responsible for at this point of time.
	Each entry describes an answer for a specific interface.
	If the same name is defined for multiple interfaces,
	then multiple instances are created
	(one for each interface).
	
	These entries are created and deleted dynamically, as
	interfaces go up and down.
	*/
	{
public:
	~CLlmnrEntry();

	CLlmnrEntry *iNext;				//< Links the entry set together (head is CDndLlmnrResponder::iEntryList)
	TDndQuestion iQuestion;			//< The question that is answered by this entry
	TInetScopeIds iZone;			//< The scope vector of the related interface.
	TIpVer iVersion:8;				//< Which protocol: IPv4 or IPv6
	TEntryState iState:8;			//< State of this entry.
	TUint iDown:1;					//< Used in interface tracking
	TUint iHostName:1;				//< Set 1, if the name depends on the local hostname.
	CNotifyConflict *iNotify;		//< Non-NULL when conlict nofitication is in progress
	};



// TRawAddr
// *********
// Lightweight internal help class for handling the link layer addresses
// (modified from ipadm engine.cpp)
class TRawAddr : public TSockAddr
	{
public:

	TInt Append(TDes8& aBuf) const
		{
		TUint8* p = (TUint8*)UserPtr();
		TInt len = ((TSockAddr *)this)->GetUserLen();

		if (len == 0)
			return KErrNotSupported;
		if (aBuf.MaxLength() < aBuf.Length() + len * 2)
			return KErrNoMemory;

		while (--len >= 0)
			aBuf.AppendFormat(_L8("%02X"), *p++ & 0xFF);

		return KErrNone;
		}

	inline static TRawAddr& Cast(const TSockAddr& aAddr)
		{
		return *((TRawAddr *)&aAddr);
		}
	};
	

CLlmnrEntry::~CLlmnrEntry()
	{
	delete iNotify;
	}


CDndLlmnrResponder::CDndLlmnrResponder(CDndEngine &aControl,
									   MDnsServerManager &aServerManager,
									   THostNames &aHostNames)
	: iControl(aControl),
	  iServerManager(aServerManager),
	  iHostNames(aHostNames)
	{

	// Need to initialize the request blocks with "parent" pointer
	for (TUint i = 0; i < sizeof(iLlmnrDataList) / sizeof(iLlmnrDataList[0]); ++i)
		iLlmnrDataList[i].iParent = this;
	}

CDndLlmnrResponder::~CDndLlmnrResponder()
	{
	// Make sure all requests are totally idle
	for (TUint i = 0; i < sizeof(iLlmnrDataList) / sizeof(iLlmnrDataList[0]); ++i)
		{
		iLlmnrDataList[i].iTimeout.Cancel();
		iLlmnrDataList[i].Cancel();
		}
	while (iEntryList)
		{
		CLlmnrEntry *p = iEntryList;
		iEntryList = p->iNext;
		delete p;
		}
	delete iLlmnrNotifyHandler;
	delete iLlmnrConf;
	}

void CDndLlmnrResponder::ConstructL()
	{
	LOG(Log::Printf(_L("CDndLlmnrResponder::ConstructL() size=%d\r\n"), (TInt)sizeof(*this)));
	CDnsSocket::ConstructL ();
	iLlmnrConf = new (ELeave) CLlmnrConf(iControl);
	iLlmnrConf->ConstructL();
	iLlmnrConf->GetHostNamesL();

	iLlmnrNotifyHandler = new (ELeave) CDndLlmnrNotifyHandler(*this);
	iLlmnrNotifyHandler->ConstructL();
	}
	
void CDndLlmnrResponder::ConfigurationChanged()
	{
	iLlmnrNotifyHandler->ConfigurationChanged();
	}

void CDndLlmnrResponder::ActivateSocketL()
	{
	if (iConnected && IsOpened())
		return;			// Assume all done

	const TDndConfigParameters &cf = iControl.GetConfig();
	const TInetAddr bind(TInetAddr(cf.iLlmnrPort));

	CDnsSocket::ActivateSocketL(bind);

	// Disable multicast loopback to lessen the possibility to receive our own replies
	(void)Socket().SetOpt(KSoIp6MulticastLoop, KSolInetIp, 0);
	// Set Unicast and Multicast TTL/Hop limit to 1 for LLMNR responses
	(void)Socket().SetOpt(KSoIp6UnicastHops, KSolInetIp, cf.iLlmnrHoplimit);
	(void)Socket().SetOpt(KSoIp6MulticastHops, KSolInetIp, cf.iLlmnrHoplimit);
	(void)Socket().SetOpt(KSoUserSocket, KSolInetIp, 0);
	// THe LLMNR responder socket does not need to keep the interfaces up.
	(void)Socket().SetOpt(KSoKeepInterfaceUp, KSolInetIp, 0);
	ActivateListenL(bind, 1);
	}

// Join multicast group to LLMNR responder iSocket
TInt CDndLlmnrResponder::JoinMulticastGroup(TUint32 aIndex, TIpVer aIpVer)
	{

	TPckgBuf<TIp6Mreq> opt;

	TRAPD(err, ActivateSocketL());	// Need socket opened...
	if (err != KErrNone)
		return err;

	if(aIpVer == EIPv4)
		opt().iAddr = iControl.GetConfig().iLlmnrIpv4;
	else
		opt().iAddr = iControl.GetConfig().iLlmnrIpv6;
	opt().iInterface = aIndex;

	err=Socket().SetOpt(KSoIp6JoinGroup, KSolInetIp, opt);

	if ((err != KErrNone)&&(err != KErrInUse)) // may return KErrInUse when multicast hook is on
		{
		LOG(Log::Printf(_L("CDndLlmnrResponder::JoinMulticastGroup SetOpt KSoIp6JoinGroup if=%d error: %d"), aIndex, err));
		return err;
		}

	return KErrNone;
	}


TInt CDndLlmnrResponder::MakeMyAddr(const TUint32 aIfIndex, const TInetAddr &aTarget, EDnsType aType, TInetAddr &aAddr)
	{
	const EDnsType target_type = (aTarget.Family() == KAfInet || aTarget.IsV4Mapped()) ? EDnsType_A : EDnsType_AAAA;

	TPckgBuf<TSoInetIfQuery> opt;
	opt().iIndex = aIfIndex;

	for (;;)
		{
		TInt err;

		if (aType == target_type)
			{
			// The target address matches the address type being queried,
			// try obtaining the "best address" that matches the target.
			opt().iDstAddr = aTarget;
			if ((err = iControl.iSocket.GetOpt(KSoInetIfQueryByIndex, KSolInetIfQuery, opt)) != KErrNone)
				return err;
			if (!opt().iSrcAddr.IsUnspecified())
				break;	// Success!
			}

		// Try finding an acceptable link local or any higher scope address

		if (aType == EDnsType_A)
			opt().iDstAddr.SetV4MappedAddress(KInetAddrLinkLocalNet); 
		else if (aType == EDnsType_AAAA)
			opt().iDstAddr.SetAddress(KInet6AddrLinkLocal);
		else
			return KErrNotSupported;

		if ((err = iControl.iSocket.GetOpt(KSoInetIfQueryByIndex, KSolInetIfQuery, opt)) != KErrNone)
			{
			LOG(Log::Printf(_L("CDndLlmnrResponder::MakeMyAddr GetOpt KSoInetIfQueryByIndex error: %d"),err));
			return err;
			}
		break;
		// *NEVER GET HERE!*
		}
	//
	// Address is unspecified or some valid source address
	//
	aAddr = opt().iSrcAddr;
	return KErrNone;
	}


#ifdef _LOG
void CDndLlmnrResponder::LogPrint(const TDesC &aStr, const CLlmnrEntry &aEntry)
	{
	TBuf<KDnsMaxName> name;

	aEntry.iQuestion.GetName(name);

	Log::Printf(_L("%S%S QType=%d State=%d ip%d for if=%u\r\n"),
		&aStr,
		&name, (TInt)aEntry.iQuestion.QType(),
		(TInt)aEntry.iState,
		(TInt)aEntry.iVersion,
		(TInt)aEntry.iZone[0]);
	}

void CDndLlmnrResponder::LogPrint(const TDesC &aStr, const TDndQuestion &aQuestion)
	{
	TBuf<KDnsMaxName> name;

	aQuestion.GetName(name);

	Log::Printf(_L("%S%S QType=%d\r\n"),
		&aStr,
		&name, (TInt)aQuestion.QType());
	}
#endif

// Mark all entries as "unupdated"
void CDndLlmnrResponder::UpdateStart()
	{
	for(CLlmnrEntry *entry = iEntryList; entry != NULL; entry = entry->iNext)
		entry->iDown = 1;
	}

// Remove all entries that were not updated (interface has gone down)
void CDndLlmnrResponder::UpdateFinish()
	{
	for(CLlmnrEntry **h = &iEntryList;;)
		{
		CLlmnrEntry *entry = *h;
		if (entry == NULL)
			break;	// -- all done!
		if (entry->iDown ||
			JoinMulticastGroup(entry->iZone[0], STATIC_CAST(TIpVer, entry->iVersion)) != KErrNone)
			{
			//
			// Remove entry. Either interface is down (not exist) or
			// it does not support multicast, in which case it cannot
			// be used for LLMNR.
			//
			*h = entry->iNext;
			//
			// If entry has any associated requests outstanding,
			// they must be cancelled.
			CancelAll(*entry);
			LOG(LogPrint(_L("\tLLMNR removed: "), *entry));
			delete entry;
			}
		else
			{
			//
			// Entry is still active and usable
			//
			h = &entry->iNext;
			continue;
			}
		}
	if (iEntryList == NULL)
		{
		// Nothing enabled for LLMNR, no need to keep the socket either
		DeactivateSocket();
		LOG(Log::Printf(_L("CDndLlmnrResponder::UpdateFinish - no entries, socket deactivated\r\n")));
		}
	}

TInt CDndLlmnrResponder::UpdateInterface
	(const TName &aIfName, const TIpVer aIpVer, const TInetScopeIds &aZone, const TSockAddr &aHwAddr, TInt aLlmnrDisable)
	/**
	* Update the LLMNR entries for the specified interface.
	*
	* @param aIfName	The interface name
	* @parma aIpVer		The IP version (either EIPv4 or EIPv6)
	* @param aZone		The zone ids
	* @param aHwAddr	The hardware address (if present)
	* @param aLlmnrDisable	=1, if LLMNR is to be disabled on this interface
	*/
	{

	// Check that this is not a loopback interface
	if(!aZone[1])
		return KErrNone;

	// An interface is open. Check if it is LLMNR enabled and if update(s)
	// have not been sent or disabled

	TInt is_active = 0;
	for(CLlmnrEntry *entry = iEntryList; entry != NULL; entry = entry->iNext)
		{
		if (entry->iZone[0] == aZone[0])
			{
			is_active = 1;
			// If LLMNR is to be disable, this keeps iDown as 1 and
			// the entry will be deleted in UpdateFinish.
			entry->iDown = aLlmnrDisable;
			//
			// Copy the scope vector each time, in case it has been modified
			//
			for (TUint k = 0; k < sizeof(TInetScopeIds) / sizeof(entry->iZone[0]); k++)
				entry->iZone[k] = aZone[k];
			}
		}
	if (is_active || aLlmnrDisable)
		return KErrNone;	// Interface already activated or being disabled, nothing else to do.

	// Interface is not yet LLMNR enabled, Check if there
	// are hostnames enabled that match this interface

	const EDnsQType qt = aIpVer == EIPv4 ? EDnsQType_A : aIpVer == EIPv6 ? EDnsQType_AAAA : EDnsQType(0);

	const TInt N = iLlmnrConf->iHostList->Count();
	for (TInt i = 0; i < N; i++)
		{
		const THostNameEntry &host = iLlmnrConf->iHostList->At(i);

		if (host.iVersion != EIPany && host.iVersion != aIpVer)
			continue;	// Does not apply to this entry, go look next
		if (host.iIfName.Length() > 0 && host.iIfName.Compare(aIfName) != 0)
			continue;	// Entry is for a specific interface (not this), go look next
		//
		// Build a new LLMNR entry
		//
		TInetAddr addr(iControl.GetConfig().iLlmnrPort);
		// Give name and address to the server manager
		if(aIpVer == EIPv4)
			addr.SetAddress(iControl.GetConfig().iLlmnrIpv4);
		else
			addr.SetAddress(iControl.GetConfig().iLlmnrIpv6);
		iServerManager.AddServerAddress(aIfName, addr);

		// Make an entry to the list
		CLlmnrEntry *entry = new CLlmnrEntry();
		if (entry == NULL)
			break;	// Ooops, out of memory...
		entry->iNext = iEntryList;
		iEntryList = entry;

		entry->iQuestion.SetName(host.iName);
		entry->iQuestion.SetQType(qt);
		entry->iQuestion.SetQClass(EDnsQClass_IN);
		entry->iVersion = aIpVer;
		//
		// Copy the scope vector
		//
		for (TUint k = 0; k < sizeof(TInetScopeIds) / sizeof(entry->iZone[0]); k++)
			entry->iZone[k] = aZone[k];

		entry->iState = EInactive;
		if(FormatHostName(*entry, aHwAddr) == KErrNone)
			(void)DoUpdate(*entry);
		else
			entry->iState = EDisabled;
		LOG(LogPrint(_L("\tLLMNR Created: "), *entry));
		}
	return KErrNone;
	}

TInt CDndLlmnrResponder::FormatHostName(CLlmnrEntry &aEntry, const TSockAddr &aHwAddr)
	{
	TInt i;
	aEntry.iHostName = 0;
	if((i = aEntry.iQuestion.Find(FORMATHWADDR)) != KErrNotFound)
		{ // hostname contains hardware address format string
		if(aHwAddr.Family() == KAFUnspec)
			{
			LOG(Log::Printf(_L("CDndLlmnrResponder::FormatHostName - no hw address\r\n")));
			return  KErrNotFound;
			};

		// Insert hw address into hostname
		// (We assume, that there is only one hw address insert in the hostname)
		TDndName tmpstr;
		tmpstr.Append(aEntry.iQuestion.Left(i));
		TInt err;
		if((err = TRawAddr::Cast(aHwAddr).Append(tmpstr)) != KErrNone)
			return err;
		tmpstr.Append(aEntry.iQuestion.Mid(i + FORMATHWADDR().Length()));
		(TDndName &)aEntry.iQuestion = tmpstr;
		return KErrNone;
		}
	else if((i = aEntry.iQuestion.Compare(WILDCARDPRIMARYHOSTNAME)) == 0)
		{
		// Entry requests use of local hostname. Find a matching local name
		// based on the network id.
		aEntry.iHostName = 1;
		const TUint32 nid = aEntry.iZone[KIp6AddrScopeNetwork-1];
		(void)iHostNames.Refresh(nid);	// Reread hostname from Comms Database.
		aEntry.iQuestion.SetName(iHostNames.Find(nid));
		return aEntry.iQuestion.Length() > 0 ? KErrNone : KErrNotFound;
		}
	else
		return KErrNone; // No format string in hostname
	}


void CDndLlmnrResponder::CancelAll(const CLlmnrEntry &aEntry)
	{

	// Abort all associated requests, if any

	for (TInt session = 0; session < KLlmnrMaxSessions; ++session)
		if(iLlmnrDataList[session].iLlmnrEntry == &aEntry)
			{
			iLlmnrDataList[session].iTimeout.Cancel();
			iLlmnrDataList[session].Cancel();
			iLlmnrDataList[session].iLlmnrEntry = NULL;
			}
	}


TInt CDndLlmnrResponder::DoUpdate(CLlmnrEntry &aEntry)
	{
	if (JoinMulticastGroup(aEntry.iZone[0], STATIC_CAST(TIpVer, aEntry.iVersion)) != KErrNone)
		{
		// The interface does not support multicast, disable entry.
		LOG(LogPrint(_L("CDndLlmnrResponder::DoUpdate() No multicast, disable: "), aEntry));
		aEntry.iState = EDisabled;
		return KErrNotSupported;
		}
	
	TInt session = 0;
	for(session=0; ; session++)
		{
		if(session == KLlmnrMaxSessions)
			{
			LOG(Log::Printf(_L("CDndLlmnrResponder::DoUpdate - No free slots in iLlmnrDataList")));
			return KErrNoMemory;
			}
		if(!iLlmnrDataList[session].iLlmnrEntry)
			break;
		}
	TLlmnrMsgData &upd = iLlmnrDataList[session];
	upd.iLlmnrEntry = &aEntry;

	// Do Unique check by sending a query for the name, and if nobody answers within
	// specified time, assume the name is valid to use for this node.
	upd.iQuestion = aEntry.iQuestion;
	upd.iQR = 0;
	upd.iAA = 0;
	upd.iRCode = EDnsRcode_NOERROR;
	if (aEntry.iVersion == EIPv4)
		upd.iDstAddr.SetAddress(iControl.GetConfig().iLlmnrIpv4);
	else
		upd.iDstAddr.SetAddress(iControl.GetConfig().iLlmnrIpv6);
	upd.iDstAddr.SetScope(aEntry.iZone[upd.iDstAddr.Ip6Address().Scope()-1]);
	upd.iDstAddr.SetPort(iControl.GetConfig().iLlmnrPort);
	upd.iRepeat = 1 + iControl.GetConfig().iLlmnrRetries;
	Queue(upd);
	return KErrNone;
	}

void CDndLlmnrResponder::Query(const TMsgBuf &aBuf, const TInetAddr &aServer, const RSocket &aSocket)
	{
	ASSERT(aServer.Port() != 0);

	// Preselect the the session...
	TInt session = 0;
	for(;; session++)
		{
		if(session == KLlmnrMaxSessions)
			{
			LOG(Log::Printf(_L("\tLLMNR No free slots in iLlmnrDataList\r\n")));
			return;
			}
		if(!iLlmnrDataList[session].iLlmnrEntry)
			break;
		}
	TLlmnrMsgData &resp = iLlmnrDataList[session];
	TInt rcode;
	TInt offset = aBuf.VerifyMessage(rcode, resp.iQuestion);
	if (offset < 0)
		{
		LOG(Log::Printf(_L("\tLLMNR Corrupt message\r\n")));
		return; // A corrupted message
		}
	const TDndHeader &hdr = aBuf.Header();
	if (hdr.QR() || hdr.ANCOUNT() != 0)
		return;	// This is responce, Not a query. Ignore.

	CLlmnrEntry *entry = iEntryList;
	TUint32 index = 0;

	// Note: there are other PTR queries than just for address!
	// (fall to normal query processing for non address PTR queries).
	if (resp.iQuestion.QType() == EDnsQType_PTR &&
		(index = IsMyAddress(resp.iQuestion)) != 0)
		{
		// See if this interface is active...
		for(;; entry = entry->iNext)
			{
			if (entry == NULL)
				{
				LOG(LogPrint(_L("\tLLMNR No match: "), resp.iQuestion));
				return; // No match, ignore
				}
			if (entry->iState > 0)
				{
				if (entry->iZone[0] == index)
					break;
				}
			}
		}
	else
		{

		// The question must match one of the entries

		const TInt scopelevel = aServer.Ip6Address().Scope()-1;
		const TUint32 scopeid = aServer.Scope();
		for(CLlmnrEntry *my_name = NULL; ;entry = entry->iNext)
			{
			if (entry == NULL)
				{
				if (my_name == NULL)
					{
					LOG(LogPrint(_L("\tLLMNR Query does not match: "), resp.iQuestion));
					return; // No match, ignore
					}
				//
				// No exact match to the query, but the name
				// matched to at least one of my unique names.
				// Reply with empty RR-set, using one of those entries.
				entry = my_name;
				break;
				}
			// A iState > 0 means valid entry.
			if (entry->iState > 0)
				{
				if (scopeid == entry->iZone[scopelevel])
					{
					if (DnsCompareNames(resp.iQuestion, entry->iQuestion))
						{
						my_name = entry;
						if (resp.iQuestion.QClass() == entry->iQuestion.QClass() &&
							resp.iQuestion.QType() == entry->iQuestion.QType())
							break;	// Full Match, answer using this entry!
						}
					}
				}
			}
		}
	//
	// Actually answering the question, assign the request
	//
	LOG(LogPrint(_L("\tLLMNR Sending reply to matched query: "), *entry));
	resp.iLlmnrEntry = entry;
	resp.iQR = 1;
	resp.iAA = 1;
	resp.iRCode = EDnsRcode_NOERROR;
	resp.iDstAddr = aServer;
	resp.iRepeat = 0;
	Queue(resp, aSocket, hdr.ID());
	}


TUint32 CDndLlmnrResponder::IsMyAddress(const TDndQuestion &aQuestion)
	{
	TPckgBuf<TSoInetIfQuery> opt;
	if (!aQuestion.GetAddress(opt().iSrcAddr))
		return 0;	// Not a parsable PTR for IP address, ignore.
	if(Socket().GetOpt(KSoInetIfQueryBySrcAddr, KSolInetIfQuery, opt) != KErrNone)
		return 0;// not my address, ignore
	return opt().iIndex;
	}

TInt CDndLlmnrResponder::SetHostName(TUint32 aId, const TDesC &aName)
	{
	for (CLlmnrEntry *e = iEntryList; e; e = e->iNext)
		{
		if (e->iHostName && e->iZone[KIp6AddrScopeNetwork-1] == aId)
			{
			if (aName.Length() > 0)
				{
				e->iState = EInactive;
				e->iQuestion.SetName(aName);
				(void)DoUpdate(*e);
				}
			else
				{
				e->iState = EDisabled;
				CancelAll(*e);
				}
			}
		}
	return KErrNone;
	}


TInt CDndLlmnrResponder::GetHostName(TUint32 aId, MDnsResolver &aCallback)
	{
	const TInt result = DoHostNameState(aId);

	if (result > 0)
		{
		//
		// Pending entries present, install callback
		//
		for (CHostCallback *c = iCallbacks; ; c = c->iNext)
			{
			if (c == NULL)
				{
				c = new CHostCallback(aCallback);
				if (c == NULL)
					return KErrNoMemory;
				c->iNext = iCallbacks;
				iCallbacks = c;
				break;
				}
			else if (&aCallback == &c->iCallback)
				break;	// Already installed.
			}
		}
	return result;
	}



/**
* Determines the state of the hostname.
*
* Scans through all LLMNR names and checks state of the
* entries that have been generated from the host name.
* Ignore entries in EDisabled state.
*
* @return result as follows:
*
* - KErrNone, if all existing entries are currently
* marked as "unique" (note: this includes the case,
* where none of the names are generated from the
* host name). No entry is in conflict state.
* - KErrAlreadyExists, if at least one of the entries
* is in conflict.
* - = 1 (Pending), if at least one of the entries is
* still being tested for uniqueness, and none of the
* entries is in conflict.
*/
TInt CDndLlmnrResponder::DoHostNameState(TUint32 aId)
	{
	TInt result = KErrNone;
	for (CLlmnrEntry *e = iEntryList; e; e = e->iNext)
		{
		if (e->iZone[KIp6AddrScopeNetwork-1] == aId && e->iHostName)
			{
			if (e->iState == EInactive)
				{
				// Pending state, unique test not yet complete
				result = 1;
				}
			else if (e->iState == EConflict)
				{
				// Conflict state, assume a name collision has occurred
				return KErrAlreadyExists;
				}
			}
		}
	return result;
	}

/**
* Do callbacks for pending GetHostName.
*
* Determines the state of the hostname and does the
* callbacks, if the state is not pending.
*/
void CDndLlmnrResponder::DoCallbacks(TUint32 aId)
	{
	if (iCallbacks == NULL)
		return;

	const TInt result = DoHostNameState(aId);
	if (result > 0)
		return;

	CHostCallback *c = iCallbacks;
	iCallbacks = NULL;
	while (c)
		{
		CHostCallback *cb = c;
		c = cb->iNext;
		cb->iCallback.ReplyCallback(result);
		delete cb;
		}
	}

//****************** TLlmnrMsgData ****************************

// constructor needed only to setup the timeout framework
TLlmnrMsgData::TLlmnrMsgData() : iTimeout(TLlmnrMsgDataTimeoutLinkage::Timeout)
	{
	}

void TLlmnrMsgData::Timeout(const TTime & /*aNow*/)
	{
	LOG(Log::Printf(_L("--> TLlmnrMsgData::Timeout() -start-\r\n")));
	if (iLlmnrEntry)
		{
		if (iRepeat > 0)
			{
			LOG(iParent->LogPrint(_L("\tLLMNR Resend query: "), *iLlmnrEntry));
			iParent->ReSend(*this);
			}
		else
			{
			Cancel();
			// The timeout is only activated with Unique testing, thus
			// when final repeat has copleted without conflict, mark
			// entry as unique
			LOG(iParent->LogPrint(_L("\tLLMNR Name is unique: "), *iLlmnrEntry));
			iLlmnrEntry->iState = EUnique;
			if (iLlmnrEntry->iHostName)
				{
				// If entry depended on the hostname and there are pending GetHostNames,
				// then check if this was the last such entry in pending state, and
				// if so, generate callbacks.
				iParent->DoCallbacks(iLlmnrEntry->iZone[KIp6AddrScopeNetwork-1]);
				}
			iLlmnrEntry = NULL;
			}
		}
	LOG(Log::Printf(_L("<-- TLlmnrMsgData::Timeout() -exit-\r\n")));
	}

TBool TLlmnrMsgData::Reply(CDnsSocket &aSource, const TMsgBuf &aBuf, const TInetAddr & /*aServer*/)
	{
	if (!iLlmnrEntry)
		return FALSE;			// Nothing to do if no entry associated

	TDndQuestion question;
	TInt rcode;
	TInt offset = aBuf.VerifyMessage(rcode, question);
	if (offset <= 0)
		return FALSE;

	const TDndHeader &hdr = aBuf.Header();
	if (!hdr.QR())
		return FALSE;	// This is query, not a response. Ignore.
	if (hdr.OPCODE() != EDnsOpcode_QUERY || rcode != EDnsRcode_NOERROR)
		return FALSE;	// Not an OK reply

	if (question.CheckQuestion(iQuestion) != KErrNone)
		return FALSE;

	// This is a matching reply to query of our own name, there is
	// a collision.
	iLlmnrEntry->iState = EConflict;
	if (iLlmnrEntry->iHostName)
		iParent->DoCallbacks(iLlmnrEntry->iZone[KIp6AddrScopeNetwork-1]);
	CNotifyConflict::Start(*iParent, *iLlmnrEntry);

	Cancel();
	CDndLlmnrResponder &responder = (CDndLlmnrResponder &)aSource;
	responder.CancelAll(*iLlmnrEntry);
	return TRUE;
	}


void TLlmnrMsgData::Sent(CDnsSocket &/*aSource*/)
	{
	if (iRepeat == 0)
		{
		iLlmnrEntry = NULL;
		Cancel();
		}
	else
		{
		iRepeat -= 1;
		iTimeout.Set(&iParent->iControl.Timer(), iParent->iControl.GetConfig().iLlmnrMinTime);
		}
	}

void TLlmnrMsgData::Abort(CDnsSocket &/*aSource*/, const TInt aReason)
	{
	LOG(Log::Printf(_L("TLlmnrMsgData::Abort - reason: %d\r\n"),aReason));
	(void)aReason; // silence compiler warning
	iLlmnrEntry = NULL;
	}

// TLlmnrMsgData::Build
// ******************
/**
// @retval	aMsg
//		contains the fully constructed message to be sent to the DNS server,
//		if Build succeeds
// @retval	aServer
//		contains the server address for which the message should be sent
//
// @returns TRUE, successful Build, and error (< 0) otherwise
*/
TBool TLlmnrMsgData::Build(CDnsSocket &/*aSource*/, TMsgBuf &aMsg, TInetAddr &aServer, TInt /*aMaxMessage*/)
	{
	TInt ret = KErrNone;
	for (;;)
		{
		if (iLlmnrEntry == NULL)
			{
			LOG(Log::Printf(_L("TLlmnrMsgData::Build() - Faulty component\r\n")));
			ret = KErrArgument;
			break;
			}

		aServer = iDstAddr;
		aMsg.SetLength(sizeof(TDndHeader));
		TDndHeader &hdr = (TDndHeader &)aMsg.Header();
		hdr.SetRD(0); // Part of zero field
		hdr.SetOpcode(EDnsOpcode_QUERY);
		hdr.SetQR(iQR);
		hdr.SetAA(iAA);
		hdr.SetRCode(iRCode);
		hdr.SetQdCount(1);
		const TInt qname_offset = aMsg.Length();
		ret = iQuestion.Append(aMsg);
		if (ret != KErrNone)
			break;

		if (iQR)
			{
			// This is a reply message
			TDndRROut rr(aMsg);
			rr.iType = (TUint16)iQuestion.QType();
			rr.iClass = (TUint16)iQuestion.QClass();
			rr.iTTL = iParent->iLlmnrConf->iTTL;

			if (rr.iType == EDnsType_PTR)
				{
				// answer contains one or several hostnames, which
				// are configured for the same interface as the
				// queried address.
				TUint16 count = 0;
				for(CLlmnrEntry *entry = iParent->iEntryList; entry != NULL; entry = entry->iNext)
					{
					if (entry->iState <= 0)
						continue;
					if (entry->iZone[0] != iLlmnrEntry->iZone[0])
						continue;
					//
					// Build an RR
					//
					ret = rr.Append(KNullDesC8, qname_offset);
					if (ret != KErrNone)
						break; // -- no reply sent!
					ret = rr.AppendRData(entry->iQuestion, 0);
					if (ret != KErrNone)
						break; // -- no reply sent!
					count++;
					}
				hdr.SetAnCount(count); // set answer or prerequisite count
				}
			else if (iQuestion.QType() == EDnsQType_ANY)
				{
				// For ANY query, return one address
				// (experimental, not fully worked out yet)
				TUint16 count = 0;
				TInetAddr addr;
				rr.iType = EDnsType_A;
				ret = iParent->MakeMyAddr(iLlmnrEntry->iZone[0], iDstAddr, (EDnsType)rr.iType, addr);
				if (ret != KErrNone)
					break;
				if (!addr.IsUnspecified())
					{
					ret = rr.Append(KNullDesC8, qname_offset);// compressed name and answer
					if (ret != KErrNone)
						ret = rr.AppendRData(addr);
					++count;
					}
				rr.iType = EDnsType_AAAA;
				ret = iParent->MakeMyAddr(iLlmnrEntry->iZone[0], iDstAddr, (EDnsType)rr.iType, addr);
				if (ret != KErrNone)
					break;
				if (!addr.IsUnspecified())
					{
					ret = rr.Append(KNullDesC8, qname_offset);// compressed name and answer
					if (ret != KErrNone)
						ret = rr.AppendRData(addr);
					++count;
					}
				hdr.SetAnCount(count);
				}
			else if (iQuestion.QType() != iLlmnrEntry->iQuestion.QType())
				{
				// The question and entry do not match. Assume this is
				// query for RR that we don't have, but the name matched.
				// Reply with empty RR-set.
				hdr.SetAnCount(0);
				}
			else if (rr.iType == EDnsType_A || rr.iType == EDnsType_AAAA)
				{
				// answer contains one address
				TInetAddr addr;
				ret = iParent->MakeMyAddr(iLlmnrEntry->iZone[0], iDstAddr, (EDnsType)rr.iType, addr);
				if (ret != KErrNone)
					break;
				if (addr.IsUnspecified())
					hdr.SetAnCount(0);	// No address
				else
					{
					ret = rr.Append(KNullDesC8, qname_offset);// compressed name and answer
					if (ret == KErrNone)
						ret = rr.AppendRData(addr);
					hdr.SetAnCount(1); // set answer count
					}
				}
			else
				break;	// Unsupported RR type, no answer!
			}
		if (ret != KErrNone)
			break;
		return 1;
		}
	//
	// Failed to build the message
	//
	iLlmnrEntry = NULL;
	Cancel();
	return 0;
	}


// **********************************************************************

CNotifyConflict::CNotifyConflict(CDndLlmnrResponder &aResponder, CLlmnrEntry &aEntry)
	: CActive(0), iResponder(aResponder), iEntry(aEntry)
	{
	LOG(Log::Printf(_L("\tnotifier[%u] new"), (TUint)this));
	CActiveScheduler::Add(this);
	}

CNotifyConflict::~CNotifyConflict()
	{
	Cancel();
	if (iConnected)
		{
		iNotifier.Close();
		}

	// Detach from the entry.
	if (iEntry.iNotify == this)
		iEntry.iNotify = NULL;
	// Deque from the active scheduler
	if (IsAdded())
		Deque();
	LOG(Log::Printf(_L("\tnotifier[%u] deleted"), (TUint)this));
	}

void CNotifyConflict::Start(CDndLlmnrResponder &aResponder, CLlmnrEntry &aEntry)
	{
	if (aEntry.iNotify)
		{
		return;			// Notify process is already active for this hostname, nothing to do.
		}
	aEntry.iNotify = new CNotifyConflict(aResponder, aEntry);
	if (aEntry.iNotify)
		{
		aEntry.iNotify->DoStart();
		}
	}

void CNotifyConflict::DoStart()
	{
	if (iNotifier.Connect() == KErrNone)
		{
		iConnected = 1;
		// Fill in information
		iInfo().iIAPId = iEntry.iZone[1];		// IAP is iZone[1]
		iInfo().iNetworkId = iEntry.iZone[15];	// NET is iZone[15]
		(void)iEntry.iQuestion.GetName(iInfo().iName);
		LOG(Log::Printf(_L("\tnotifier[%u] activated IAP=%d NET=%d HOST='%S'"), (TUint)this,
			iInfo().iIAPId, iInfo().iNetworkId, &iInfo().iName));
		iNotifier.StartNotifierAndGetResponse(iStatus, TUid::Uid(KLlmnrConflictNotifyUid), iInfo, iInfo);
		SetActive();
		}
	else
		{
		// Cancel Notifier activity, cannot get it to work.
		delete this;
		}
	}

void CNotifyConflict::DoCancel()
	{
	LOG(Log::Printf(_L("\tnotifier[%u] canceling"), (TUint)this));
	iNotifier.CancelNotifier(TUid::Uid(KLlmnrConflictNotifyUid));
	}

void CNotifyConflict::RunL()
	{
	LOG(Log::Printf(_L("-->\tnotifier[%u] Completion result=%d"), (TUint)this, iStatus.Int()));
	if (iStatus.Int() == KErrNone && iInfo().IsSafe() && iInfo().iName.Length() > 0)
		{
		if (iEntry.iQuestion.SetName(iInfo().iName) == KErrNone)
			{
			LOG(Log::Printf(_L("-->\tnotifier[%u] Trying new name '%S'"), (TUint)this, &iInfo().iName));
			iEntry.iState = EInactive;
			// Detach from the entry.
			if (iEntry.iNotify == this)
				iEntry.iNotify = NULL;
			(void)iResponder.DoUpdate(iEntry);
			}
		}
	delete this;
	}

#endif
