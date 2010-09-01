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
// servers.cpp - server manager module
//

#include "servers.h"
#include "engine.h"
#include "inet6log.h"
#include "dns_hdr.h"	// only for KDnsPort!
#include "dnd.hrh"

// Item of the server list, hold DNS server information
class TDnsServerData
	{
public:
	TInetAddr iAddr;			//< DNS Server Address
	TDnsServerScope iScope;		//< The Sever Scope and type (MC/UC)
	TInt iServerId;				//< Id of the server
	TInt iInterface;			//< Index to Interface Array
	};

// Item of the interface list, hold interface information for DNS
class TDnsInterfaceData
	{
public:
	TName iName;				//< Name of the interface
	TUint32 iScope[16];			//< The scope vector
	RInetSuffixList iSuffixList;//< Structure to hold the interface specific domain search list
	};

// Item of the configured servers list
class TDnsConfiguredServer
	{
public:
	TName iName;
	TInetAddr iAddr;
	};

class CDnsServerManager : public CBase, public MDnsServerManager
	{
	~CDnsServerManager();
public:
	CDnsServerManager(CDndEngine &aControl);
	void ConstructL();
	void ConfigurationChanged();

	//
	// MDnsServerManager API
	// (for docs, see the MDnsServerManger definitions)
	//
	TInt OpenList(const TDnsServerFilter &aFilter, MDnsServerListNotify *aNotify);
	TInt Count(const TDnsServerFilter &aFilter) const;
	TInt Next(const TDnsServerFilter &aFilter, TInt aServerId) const;
	void CloseList(const TDnsServerFilter &aFilter);

	TInt Address(TInt aServerId, TInetAddr &aAddr) const;
	TUint32 Scope(TInt aServerId, const TInetAddr &aAddr);
	TInt ServerId(const TInetAddr &aAddr) const;
	TUint32 NameSpace(const TDnsServerFilter &aFilter, TInt aServerId) const;
	TInt BuildServerList();
	void AddServerAddress(const TName &aInterface, const TInetAddr &aAddr);
	void LockByAddress(const TInetAddr &aAddr, TUint32 aNid, TDnsServerFilter &aFilter);
	// Retrieves the domain search list configured on the interface associated with the nominated name server
	void InterfaceSuffixList(TInt aServerId, RInetSuffixList& aSuffixList);
	// Performs the network id selection for the query based on the domain name on the query
	void UpdateDomain(TDnsServerFilter &aFilter) const;

private:
	// Build and add interface entry to the list (basic operation)
	TInt AddInterfaceEntry(const TSoInetIfQuery &aInfo);
	// Find interface matching the destination address
	TInt FindInterface(const TInetAddr &aAddr);
	// Add new server address to the server list
	void AddToServerList(const TInetAddr &aAddr, TInt aIf, RSocket &aSocket);
	// Add new interface to the interface list
	TInt AddToInterfaceList(const TSoInetInterfaceInfo &aInfo, RSocket &aSocket);
	// Compare the filter with a DNS server
	TBool Match(const TDnsServerFilter &aFilter, const TDnsServerData &aServer) const;

	CDndEngine &iControl;
	TUint32 iServerId;				//< The last used server id.
	TTime iMark;					//< The time of the last configuration change.
	TUint iConfigure:1;				//< Reconfigure needed, if set.
	TUint iStable:1;				//< = 1, when scanned list is supposed to be stable
	TUint iCacheFlushed:1;			//< = 1, when cache has been flushed (only used with "FlushOnConfig" ini-option)

	CArrayFixFlat<TDnsServerData> *iServerList;	//< Current list of servers
	CArrayFixFlat<TDnsInterfaceData> *iInterfaceList; //< Current list of interfaces
	CArrayFixFlat<TDnsConfiguredServer> *iConfiguredList; //< Current list of configured servers
	
private:
	TInt AddInterfaceEntry(const TSoInetIfQuery &aInfo, RSocket& aSocket);
	};


MDnsServerManager *DnsServerManager::NewL(CDndEngine &aControl)
	{
	CDnsServerManager *mgr = new (ELeave) CDnsServerManager(aControl);

	CleanupStack::PushL(mgr);
	mgr->ConstructL();
	CleanupStack::Pop();
	return mgr;
	}

//
// CDnsServerManager
//

CDnsServerManager::CDnsServerManager(CDndEngine &aControl) : iControl(aControl)
	{
	}

void CDnsServerManager::ConstructL()
	{
	iServerList = new (ELeave) CArrayFixFlat<TDnsServerData>(2);
	iInterfaceList = new (ELeave) CArrayFixFlat<TDnsInterfaceData>(5);
	// iConfiguredList is allocated only if required.
	}

void CDnsServerManager::ConfigurationChanged()
	{
	LOG(Log::Printf(_L("CDnsServerManager -- Configuration changed")));
	// Just flag that building a new server list is needed. Try
	// to delay heavy interface scanning operation until really
	// needed (because configuration changes may come in burts).
	iConfigure = 1;
	iStable = 0;
	iCacheFlushed = 0;
	iMark.UniversalTime();
	}

CDnsServerManager::~CDnsServerManager()
	{
	delete iServerList;
	delete iInterfaceList;
	delete iConfiguredList;
	}

// CDnsServerManager::AddInterfaceEntry
TInt CDnsServerManager::AddInterfaceEntry(const TSoInetIfQuery &aInfo, RSocket& aSocket)
	{
    TRAPD(err,
	TDnsInterfaceData &ifd = iInterfaceList->ExtendL();

    ifd.iName = aInfo.iName;
    for (TInt i = sizeof(ifd.iScope) / sizeof(ifd.iScope[0]); --i >= 0; )
        ifd.iScope[i] = aInfo.iZone[i];
    
    if (aSocket.SetOpt(KSoInetEnumDomainSuffix, KSolInetIfCtrl) == KErrNone)
        {
        ifd.iSuffixList.Reset();
        TInetSuffix data;
        TPckg<TInetSuffix> opt(data);
        while (aSocket.GetOpt(KSoInetNextDomainSuffix, KSolInetIfCtrl, opt) == KErrNone)
            {
            TSuffixName tmpBuf;
            tmpBuf.Copy(opt().iSuffixName);
            ifd.iSuffixList.AppendL(tmpBuf);
            }
        }
		);
		
	return err < 0 ? err : iInterfaceList->Count() - 1;
	}

TInt CDnsServerManager::AddInterfaceEntry(const TSoInetIfQuery &aInfo)
    {
    TRAPD(err,
        TDnsInterfaceData &ifd = iInterfaceList->ExtendL();
        ifd.iName = aInfo.iName;
        for (TInt i = sizeof(ifd.iScope) / sizeof(ifd.iScope[0]); --i >= 0; )
            ifd.iScope[i] = aInfo.iZone[i];
        );
    return err < 0 ? err : iInterfaceList->Count() - 1;
    }

// CDnsServerManager::AddToInterfaceList
// *************************************
/**
// Add the interface to the interface list, if it does not already exist.
// The existence is based on comparing the interface names.
//
// @param	aInfo	the information that identifies the interface
// @param	aSocket	(must be open) to be used for GetOpt, if needed
// @returns
//	@li	< 0, if there are some errors (interface was not added)
//	@li index to the interface (>= 0) in the interface list.
*/
TInt CDnsServerManager::AddToInterfaceList(const TSoInetInterfaceInfo &aInfo, RSocket &aSocket)
	{
	if (aInfo.iName.Length() == 0)
		return -1;	// Interface must have a name.
	//
	// Check if interface already exists and don't insert duplicates
	//
	TInt i;
	for (i = iInterfaceList->Count(); --i >= 0; )
		{
		const TDnsInterfaceData &data = iInterfaceList->At(i);
		if (data.iName.Compare(aInfo.iName) == 0)
			return i;	// Interface already present in the list
		}

	//
	// A new interface, get the scope vector
	//
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iName = aInfo.iName;
	const TInt err = aSocket.GetOpt(KSoInetIfQueryByName, KSolInetIfQuery, opt);
	return err < 0 ? err : AddInterfaceEntry(opt(), aSocket);
	}

// CDnsServerManager::FindInterface
// ********************************
//
TInt CDnsServerManager::FindInterface(const TInetAddr &aAddr)
	{
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iDstAddr = aAddr;
	const TInt err = iControl.iSocket.GetOpt(KSoInetIfQueryByDstAddr, KSolInetIfQuery, opt);
	if (err < 0)
		return err;

	// Check if interface is already known

	for (TInt i = iInterfaceList->Count(); --i >= 0; )
		{
		const TDnsInterfaceData &data = iInterfaceList->At(i);
		if (data.iName.Compare(opt().iName) == 0)
			return i;
		}
	// Not present yet, just add it
	return AddInterfaceEntry(opt());
	}

// CDnsServerManager::AddToServerList
// **********************************
/**
// Add new address to the server list. A new entry is only
// added if the address does not already exist.
//
// @param	aAddr	address of the DNS server
// @param	aIf		the interface (index to the interface list)
*/
void CDnsServerManager::AddToServerList(const TInetAddr &aAddr, TInt aIf, RSocket &aSocket)
	{
	if (aAddr.IsUnspecified())
		return;		// No address, nothing to add
	//
	// Normalize all addresses into IPv6 format
	//
	TDnsServerData sd;
	sd.iAddr = aAddr;
	sd.iInterface = aIf;
	if (sd.iAddr.Family() == KAfInet)
		sd.iAddr.ConvertToV4Mapped();
	else if (sd.iAddr.Family() != KAfInet6)
		return;		// Only IPv4 or IPv6 addresses are valid
	if (sd.iAddr.IsMulticast())
		//	sd.iScope = (TDnsServerScope)(sd.iAddr.Ip6Address().Scope());
		sd.iScope = EDnsServerScope_MC_LOCAL;
	else
		sd.iScope = EDnsServerScope_UC_GLOBAL;

	if (!sd.iAddr.Port())
		sd.iAddr.SetPort(KDnsPort);

	LOG(TBuf<80> dst);
	LOG(sd.iAddr.OutputWithScope(dst));
	LOG(TBuf<80> src);
	// In typhoon and later, DND never activates interfaces. All
	// usable server addresses must have a valid route, before
	// they can be used. Thus, check it...
	//
	TPckgBuf<TSoInetIfQuery> opt;
	opt().iDstAddr = aAddr;
	const TBool has_route =
		(aSocket.GetOpt(KSoInetIfQueryByDstAddr, KSolInetIfQuery, opt) == KErrNone) && !opt().iSrcAddr.IsUnspecified();
	LOG(opt().iSrcAddr.OutputWithScope(src));
	if (!has_route)
		{
		LOG(Log::Printf(_L("\t\tnameserver [%S] (src=%S) has no route or no source address, skipped"), &dst, &src));
		return;				// No route, unusable for now -- ignore
		}
	//
	// Check if address already exists and don't insert duplicates
	//
	for (TInt i = iServerList->Count(); --i >= 0; )
		{
		TDnsServerData &a = iServerList->At(i);
		if (a.iAddr.Match(sd.iAddr) && a.iAddr.Scope() == sd.iAddr.Scope())
			{
			// However, if server didn't have assigned
			// interface yet, assign it from this call.
			//
			if (a.iInterface < 0)
				a.iInterface = aIf;
			LOG(Log::Printf(_L("\t\tnameserver (id=%d) [%S] (src=%S)"), a.iServerId, &dst, &src));
			return;	// Duplicate address, do not add again.
			}
		}

	sd.iServerId = ++iServerId;	// Assign a "server id"
	LOG(Log::Printf(_L("\t\tnameserver (id=%d) [%S] (src=%S) (new)"), sd.iServerId, &dst, &src));
	TRAP_IGNORE(iServerList->AppendL(sd));
	}

// CDnsServerManager::BuildServerList
// **********************************
TInt CDnsServerManager::BuildServerList()
	{
	LOG(Log::Printf(_L("CDnsServerManager -- Scanning interfaces and building the server list")));
	TInt err = KErrNone;

	// Refresh the current list of DNS server addresses
	// (this could be skipped if there was some definite way of knowing
	// that nothing has changed since the last collect...)

	// Use Delete instead of Reset, so that space is reused? (not freed and reallocated)
	iInterfaceList->Delete(0, iInterfaceList->Count());

	// Trying to keep "server id" stable, thus do not clear existing
	// server list, but just mark entries, so that unused ones can be 
	// reclaimed after build is complete.
	for (TInt i = iServerList->Count(); --i >= 0; )
		iServerList->At(i).iInterface = -2;

	LOG(Log::Printf(_L("\t* Scanning interfaces")));
	// Read the DNS address from the interface
	TSoInetInterfaceInfo *info = new TSoInetInterfaceInfo; // allocate large struct from heap!
	if (info && (err = iControl.iSocket.SetOpt(KSoInetEnumInterfaces, KSolInetIfCtrl)) == KErrNone)
		{
		TPckg<TSoInetInterfaceInfo> opt(*info);
		while (iControl.iSocket.GetOpt(KSoInetNextInterface, KSolInetIfCtrl, opt) == KErrNone)
			{
			if (opt().iName.Length() == 0)
				continue;	// "null" interface, ignore
			if (!opt().iAddress.IsUnspecified())
				{
#ifdef _LOG
				TBuf<60> tmp;
				opt().iAddress.OutputWithScope(tmp);
				Log::Printf(_L("\t%S [%S]"), &opt().iName, &tmp);
#endif
				TInt i = AddToInterfaceList(opt(), iControl.iSocket);
				AddToServerList((TInetAddr &)(opt().iNameSer1), i, iControl.iSocket);
				AddToServerList((TInetAddr &)(opt().iNameSer2), i, iControl.iSocket);
				}
			else
				{
				LOG(Log::Printf(_L("\tInterface: %S [no address, skipping]"), &opt().iName));
				}
			}
		}
	delete info;
	LOG(Log::Printf(_L("\t* Configured interface specific addresses")));

	//
	// Add configured servers, if a matching interface has become available
	// (assumes that configured list and interface list are always short, so
	// the loop over all lists is not too bad...)
	if (iConfiguredList)
		{
		for (TInt i = iConfiguredList->Count(); --i >= 0; )
			{
			const TDnsConfiguredServer &cs = iConfiguredList->At(i);
			for (TInt j = iInterfaceList->Count(); --j >= 0; )
				{
				const TDnsInterfaceData &data = iInterfaceList->At(j);
				if (data.iName.Compare(cs.iName) == 0)
					{
					// Complete the address with the scope id
					// from the interface!
					TInetAddr addr(cs.iAddr);
					if (addr.Family() == KAfInet)
						addr.ConvertToV4Mapped();
					const TUint s = addr.Ip6Address().Scope() - 1;
					if (s < 16)
						{
						addr.SetScope(data.iScope[s]);
						AddToServerList(addr, j, iControl.iSocket);
						}
					break;
					}
				}
			}
		}
	//
	// Remove unused servers from the list
	//
	LOG(Log::Printf(_L("\t* Remove stale addresses")));
	const TInt N = iServerList->Count();
	TInt k = 0;
	for (TInt j = 0; j < N; ++j)
		{
		const TDnsServerData &a = iServerList->At(j);
		if (a.iInterface != -2)
			{
			// This server entry is still used
			if (k != j)
				iServerList->At(k) = a;
			k++;
			}
#ifdef _LOG
		else
			{
			TBuf<80> tmp;
			a.iAddr.OutputWithScope(tmp);
			Log::Printf(_L("\t\tnameserver (id=%d) [%S] deleted"), a.iServerId, &tmp);
			}
#endif
		}
	if (k < N)
		{
		iServerList->Delete(k, N - k);
		}

	LOG(Log::Printf(_L("CDnsServerManager -- Done, current server count=%d"), iServerList->Count()));
	return err;
	}

// CDnsServerManager::AddServerAddress
// ***********************************
void CDnsServerManager::AddServerAddress(const TName &aInterface, const TInetAddr &aAddr)
	{
	if (iConfiguredList == NULL &&
		(iConfiguredList = new CArrayFixFlat<TDnsConfiguredServer>(5)) == NULL)
		return;		// No memory for the allocation, ignore..

	// Do not add duplicates, check existing entries
	for (TInt i = iConfiguredList->Count(); --i >= 0; )
		{
		const TDnsConfiguredServer &cs = iConfiguredList->At(i);
		if (cs.iName.Compare(aInterface) == 0 && cs.iAddr.Match(aAddr))
			return;	// Duplicate!
		}

	iConfigure = 1;	// Request rebuild of server list.

	TRAP_IGNORE(TDnsConfiguredServer &cf = iConfiguredList->ExtendL();
		cf.iAddr = aAddr;
		cf.iName = aInterface;
		);
	}

/**
// @param	aFilter	the server filter
// @parma	aServer	to be tested against the filter
// @returns
//	@li	TRUE, if the server matches the filter
//	@li	FALSE, if the server does not match the filter
*/
TBool CDnsServerManager::Match(const TDnsServerFilter &aFilter, const TDnsServerData &aServer) const
	{
	if (aFilter.iServerScope != aServer.iScope)
		return FALSE;
	if (aFilter.iLockType < KIp6AddrScopeNodeLocal || aFilter.iLockType > KIp6AddrScopeNetwork)
		return FALSE;	// actually invalid locking scope level

	// If a server address is specified without interface (-1), then this
	// server will match any filter (if server scopes are same).
	if (aServer.iInterface >= 0)
		{
		const TDnsInterfaceData &id = iInterfaceList->At(aServer.iInterface);
		if (aFilter.iLockId != id.iScope[aFilter.iLockType-1])
			return FALSE;	// Not in locked scope
		}
	return 	TRUE;
	}

/**
// @name	UpdateDomain
// @param   aFilter the server filter
// @param   aServer to be tested against the filter
*/
void CDnsServerManager::UpdateDomain(TDnsServerFilter &aFilter) const
    {
	LOG(Log::Printf(_L("CDnsServerManager -- RHostResolver opened on implicit connection")));
    if ( aFilter.iDomainName.Length() )
        {
        TBool updatedDomain(FALSE);
        for (TInt i = iInterfaceList->Count(); --i >= 0 && !updatedDomain; )
            {
            TDnsInterfaceData &id = iInterfaceList->At(i);
            for (TInt i=0; i<id.iSuffixList.Count();i++)
                {
                if (aFilter.iDomainName.Find(id.iSuffixList[i]) != KErrNotFound)
                    {
                    aFilter.iLockId = id.iScope[aFilter.iLockType-1];
                    updatedDomain = TRUE;
                    break;
                    }
                }
            }
        }
    }

//
// MDnsServerManager API
//
TInt CDnsServerManager::OpenList(const TDnsServerFilter &aFilter, MDnsServerListNotify * /*aNotify*/)
	{
	if (iConfigure)
		{
		const TInt err = BuildServerList();
		if (err != KErrNone)
			return err;
		iConfigure = 0;
		}
	if (iStable)
		return KErrNone;

	if (iControl.GetConfig().iFlushOnConfig && iCacheFlushed == 0)
		{
		iCacheFlushed = 1;
		TRAP_IGNORE(iControl.HandleCommandL(EDndFlush));
		}

	TTime stamp;
	stamp.UniversalTime();
	TTimeIntervalSeconds elapsed;
	stamp.SecondsFrom(iMark, elapsed);
	if (elapsed.Int() > (TInt)iControl.GetConfig().iSetupTime)
		{
		iStable = 1;
		return KErrNone;
		}
	if (aFilter.iServerScope == EDnsServerScope_MC_LOCAL)
		return KErrNone;		
	// We are still within setup time, return "pending" if no servers available
	return (Count(aFilter) == 0) ? 1 : KErrNone;
	}

TInt CDnsServerManager::Count(const TDnsServerFilter &aFilter) const
	{
	TInt count = 0;
	for (TInt i = iServerList->Count(); --i >= 0; )
		if (Match(aFilter, iServerList->At(i)))
			++count;
	return count;
	}

TInt CDnsServerManager::Next(const TDnsServerFilter &aFilter, TInt aServerId) const
	{
	const TInt N = iServerList->Count();

	TInt wrap = 0;
	TInt first_found = 0;
	TInt current_found = 0;

	if (aServerId == 0)
		{
		//
		// When "current" server is not specified, Next will
		// return the first server (specified by iServerId)
		//
		for (TInt i = 0; i < N; ++i)
			{
			const TDnsServerData &server = iServerList->At(i);
			if (server.iServerId == aFilter.iServerId)
				first_found = 1;
			if (first_found)
				{
				if (Match(aFilter, server))
					return server.iServerId;
				}
			else if (wrap == 0 && Match(aFilter, server))
				wrap = server.iServerId;
			}
		//
		// iServerId server is not present (or it and none
		// of the servers after it do not match). Return
		// the first matching server.
		return wrap;
		}
	else
		{
		//
		// When aServerId is given, the Next will return
		// the next matching server between (current, first)
		// (or fail, if none exists)
		for (TInt i = 0; i < N; ++i)
			{
			const TDnsServerData &server =  iServerList->At(i);
			if (current_found)
				{
				if (server.iServerId == aFilter.iServerId)
					return 0;	// Back to first, no Matching server
				if (Match(aFilter, server))
					return server.iServerId;
				}
			else
				{
				if (server.iServerId == aServerId)
					// Start looking for the next match
					current_found = 1;
				if (server.iServerId == aFilter.iServerId)
					first_found = 1;
				if (first_found == 0 && wrap == 0 && Match(aFilter, server))
					wrap = server.iServerId;
				}
			}
		//
		// Wrap around wrapping
		//
		if (current_found && first_found)
			return wrap;
		}
	return 0;	// No matching next server!
	}

void CDnsServerManager::CloseList(const TDnsServerFilter &/*aFilter*/)
	{
	}


TInt CDnsServerManager::Address(TInt aServerId, TInetAddr &aAddr) const
	{
	const TInt N = iServerList->Count();
	for (TInt i = 0; i < N; ++i)
		{
		const TDnsServerData &server =  iServerList->At(i);
		if (server.iServerId == aServerId)
			{
			aAddr = server.iAddr;
			return KErrNone;
			}
		}
	return KErrNotFound;
	}

TInt CDnsServerManager::ServerId(const TInetAddr &aAddr) const
	{
	for (TInt i = iServerList->Count(); --i >= 0; )
		{
		const TDnsServerData &server = iServerList->At(i);
		if (server.iAddr.CmpAddr(aAddr) && server.iAddr.Scope() == aAddr.Scope())
			return server.iServerId;
		}
	return 0;
	}

TUint32 CDnsServerManager::Scope(TInt aServerId, const TInetAddr &aAddr)
	{
	const TInt N = iServerList->Count();
	for (TInt i = 0; i < N; ++i)
		{
		const TDnsServerData &server = iServerList->At(i);
		if (server.iServerId == aServerId)
			{
			const TUint i = aAddr.Ip6Address().Scope() - 1;
			if (i > 15)
				return 0;	// Bad scope level.
			const TInt j = server.iInterface < 0 ? FindInterface(server.iAddr) : server.iInterface; 
			if (j >= 0)
				return (iInterfaceList->At(j)).iScope[i];
			}
		}
	return 0;
	}


static TUint32 MakeNameSpaceId(TInt aServerScope, const TDnsInterfaceData &aIf)
	{
	//
	// Construct the name space id from the server scope and
	// matching scope id.
	//
	if (aServerScope < 0)
		aServerScope = -aServerScope;
	aServerScope -= 1;
	if (aServerScope >= 0 && aServerScope < 16)
		return (aIf.iScope[aServerScope] & ~(0xFu << 28)) | ((aServerScope & 0xFu) << 28);
	return 0;
	}

TUint32 CDnsServerManager::NameSpace(const TDnsServerFilter &aFilter, TInt aServerId) const
	{
	if (aFilter.iLockType < KIp6AddrScopeNodeLocal || aFilter.iLockType > KIp6AddrScopeNetwork)
		return 0;	// actually invalid locking type

	if (aServerId)
		{
		const TInt N = iServerList->Count();
		for (TInt i = 0; i < N; ++i)
			{
			const TDnsServerData &server =  iServerList->At(i);
			if (server.iServerId == aServerId)
				{
				if (server.iInterface < 0)
					break;
				return MakeNameSpaceId(aFilter.iServerScope, iInterfaceList->At(server.iInterface));
				}
			}
		// Should this fall to generic search if server is not
		// found? Or, just return 0?
		}

	for (TInt i = iInterfaceList->Count(); --i >= 0; )
		{
		TDnsInterfaceData &id = iInterfaceList->At(i);
		if (aFilter.iLockId == id.iScope[aFilter.iLockType-1])
			return MakeNameSpaceId(aFilter.iServerScope, id);
		}
	// Cannot find name space id
	return 0;
	}

void CDnsServerManager::LockByAddress(const TInetAddr &aAddr, TUint32 aNid, TDnsServerFilter &aFilter)
	{
	aFilter.iLockType = aAddr.Ip6Address().Scope();
	aFilter.iLockId = aAddr.Scope();
	if (aFilter.iLockId)
		return;	// Address specified the scope id, all done.
	if (aFilter.iLockType == KIp6AddrScopeNetwork)
		{
		aFilter.iLockId = aNid;
		return;	// If lock scope level is network, we can use aNid as is.
		}

	// Address does not specify the scope id. Try to pick one
	// from known interfaces based on the network id.
	for (TInt i = iInterfaceList->Count(); --i >= 0; )
		{
		TDnsInterfaceData &id = iInterfaceList->At(i);
		if (aNid == id.iScope[KIp6AddrScopeNetwork-1])
			{
			aFilter.iLockId = id.iScope[aFilter.iLockType-1];
			break;
			}
		}
	}

/**
// @name 	InterfaceSuffixList
// @param	aServerId	Id of the server used for name resolution
// @param	aSuffixList	reference to array for reading the interface specific domain suffices
*/
void CDnsServerManager::InterfaceSuffixList(TInt aServerId, RInetSuffixList& aSuffixList)
    {
    const TInt N = iServerList->Count();
    for (TInt i = 0; i < N; ++i)
        {
        const TDnsServerData &server =  iServerList->At(i);
        if (server.iServerId == aServerId)
            {
            aSuffixList = iInterfaceList->At(server.iInterface).iSuffixList;
            break;
            }
        }
    }
