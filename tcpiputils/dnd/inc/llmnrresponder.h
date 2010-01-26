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
// llmnrresponder.h - DND Link-local Multicast Name Resolution  
// responder module header
//

#ifndef __LLMNRRESPONDER_H__
#define __LLMNRRESPONDER_H__

/**
@file llmnrresponder.h
LLMNR responder (listen and reply queries)
@internalComponent	Domain Name Resolver
*/
#include "dns.h"
#include "llmnrconf.h"
#include "llmnrnotifyhandler.h"
#include "inet6log.h"
#include "hostname.h"

class TInetAddressInfo;
class CLlmnrEntry;

const TInt KLlmnrMaxEnabled = 15;

const TInt KLlmnrMaxEntries = 2 * KLlmnrMaxEnabled; // NOTE: Must not  be bigger than 255

const TInt KLlmnrMaxSessions = KLlmnrMaxEntries;

class TLlmnrMsgData: public TDnsRequest
	{
public:
	TLlmnrMsgData();
	TBool Reply(CDnsSocket &aSource, const TMsgBuf &aBuf, const TInetAddr &aServer);
	void Sent(CDnsSocket &aSource);
	void Abort(CDnsSocket &/*aSource*/, const TInt aReason);
	TBool Build(CDnsSocket &aSource, TMsgBuf &aMsg, TInetAddr &aServer, TInt aMaxMessage);

	void Timeout(const TTime &aNow);//< called for timer expiration (when set)

	RTimeout iTimeout;		//< Timer for "unique test" queries

	CDndLlmnrResponder *iParent;
	TDndQuestion iQuestion;	//< The question being responded or queried
	TUint iQR:1;			//< QR (Query/Response) bit to be used in the header
	TUint iAA:1;			//< AA (Authoritative Answer) bit to be used in the header
	TUint iRCode:8;			//< RCODE field to be used in the header
	TUint iRepeat:8;		//< Count down for repeat transmissions
	TInetAddr iDstAddr;		//< The destination address of the message
	CLlmnrEntry *iLlmnrEntry;//< pointer to the corresponding TLlmnrEntry
	};

// This ungainly manoevure is forced on us because the offset is not evaluated early enough by GCC3.4 to be
// passed as a template parameter
#if defined(__X86GCC__) || defined(__GCCE__)
#define KLlmnrMsgDataTimeoutOffset 20
__ASSERT_COMPILE(KLlmnrMsgDataTimeoutOffset == _FOFF(TLlmnrMsgData, iTimeout));
#else
#define KLlmnrMsgDataTimeoutOffset _FOFF(TLlmnrMsgData, iTimeout)
#endif

class TLlmnrMsgDataTimeoutLinkage : public TimeoutLinkage<TLlmnrMsgData, KLlmnrMsgDataTimeoutOffset>
/**
Static timeout linkage for TLlmnrMsgData.
*/
	{
public:
	static void Timeout(RTimeout &aLink, const TTime &aNow, TAny * /*aPtr*/)
		{
		Object(aLink)->Timeout(aNow);
		}
	};



class CHostCallback
	{
public:
	CHostCallback(MDnsResolver &aCallback) : iCallback(aCallback) {}
	CHostCallback *iNext;
	MDnsResolver &iCallback;
	};

class CDndLlmnrResponder : public CDnsSocket
/**
Listens incoming LLMNR queries and replies to those that apply to this node.
*/
	{
	friend class TLlmnrMsgData;
	friend class CDndLlmnrNotifyHandler;
public:
	CDndLlmnrResponder(CDndEngine &aControl,MDnsServerManager &aServerManager, THostNames &aHostNames);
	void ConstructL();
	~CDndLlmnrResponder();
	void ConfigurationChanged();

	void UpdateStart();
	TInt UpdateInterface(const TName &aIfName, const TIpVer aIpVer, const TInetScopeIds &aZone, const TSockAddr &aHwAddr, TInt aLlmnrDisable);
	void UpdateFinish();
	void ActivateSocketL();


	CDndEngine &iControl;						//< The dnd engine
	MDnsServerManager &iServerManager;			//< The server manager
	THostNames &iHostNames;						//< The local hostnames database reference.

	TInt SetHostName(TUint32 aId, const TDesC &aName);
	TInt GetHostName(TUint32 aId, MDnsResolver &aCallback);
	//
	TInt DoUpdate(CLlmnrEntry &aEntry);			//< Start conflict test for the entry
private:
	void DoCallbacks(TUint32 aId);				//< Do callbacks if all hostname entries are unique
	TInt DoHostNameState(TUint32 aId);			//< Returns current state of hostname
	void CancelAll(const CLlmnrEntry &aEntry);	//< Cancel activity on the entry

	TInt FormatHostName(CLlmnrEntry &aEntry, const TSockAddr &aHwAddr);

	TInt JoinMulticastGroup(TUint32 aIndex, TIpVer aIpVer);
	TInt MakeMyAddr(const TUint32 aIfIndex, const TInetAddr &aTarget, EDnsType aType, TInetAddr &aAddr);
	TUint32 IsMyAddress(const TDndQuestion &aQuestion);
	void Query(const TMsgBuf &aBuf, const TInetAddr &aServer, const RSocket &aSocket);

#ifdef _LOG
	void LogPrint(const TDesC &aStr, const CLlmnrEntry &aEntry);
	void LogPrint(const TDesC &aStr, const TDndQuestion &aQuestion);
#endif

	CDndLlmnrNotifyHandler *iLlmnrNotifyHandler;
	TLlmnrMsgData	iLlmnrDataList[KLlmnrMaxSessions];	   //< List of active LLMNR responder message sessions

	CLlmnrEntry *iEntryList;		//< The current set of valid answers from this node.
	CLlmnrConf *iLlmnrConf;			//< Pointer to LLMNR configuration class
	CHostCallback *iCallbacks;		//< Installed callbacks that wait for the unique completion.
   };

#endif
