// Copyright (c) 2006-2009 Nokia Corporation and/or its subsidiary(-ies).
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
//



/**
 @internalComponent
*/
#ifndef	__QOS_CHANNEL_H__
#define	__QOS_CHANNEL_H__

#include <e32base.h>
#include <in_sock.h>

#include "pfqosparser.h"	// Required because TQoSParameters is a member variable
#include "qoslog.h"			// Needed because there are DEBUG only log methods.

// Forward declarations
class CExtensionPolicy;
class CFlowContext;
class CFlowHook;
class CInterface;
class CInternalQoSChannel;
class CModuleSelector;
class CProtocolQoS;
class CQoSSessionBase;
class CQoSProvider;
class CSapItem;
class RModule;
class TPfqosAddress;
class TPfqosMessage;
class TPfqosSelector;

class TIpAddress : public TIp6Addr
	/**
	* Container for IP address with scope in unform IPv6 format.
	*/
	{
public:
	TIpAddress(const TInetAddr& aAddr);
	TBool Match(const TInetAddr& aAddr) const;
	TInt operator==(const TIpAddress& aAddr) const;

	TUint32 iScope;
	};

class TQoSConnId
	/**
	* The connection identification.
	*
	* This class identifies single application level RSocket.
	*
	* Currently this is based on selector values.
	* The source address is not used (for some reason?)
	*
	* Note: the selector values are not reliable way of identifying
	* the application level RSocket. This is at best a fuzzy solution
	* and should be replaced by something else. Something, which uniquely
	* identifies the RSocket instance and can be found from both
	* TPfqosMessage and from CFlowContext.
	*/
	{
public:
	TQoSConnId(const TPfqosMessage& aMsg);
	TQoSConnId(const CFlowContext &aFlow);
	TBool Match(const CFlowContext &aFlow) const;
	TInt operator==(const TQoSConnId &aId) const;
private:
	const TUint16 iSrcPort;			//< The source port
	const TUint16 iDstPort;			//< The destination port
	const TUint32 iProtocol;		//< The internet protocol number (such as 6 for TCP, 17 for UDP, etc.)
	const TIpAddress iDst;			//< The destination address.

#ifdef _LOG
public:
	void Dump() const;
#endif
	};


class CQoSConn : public CBase
	/**
	* QoS Connection.
	*
	* CQoSConn describes an entity which can be joined to
	* a QoS channel. This correcsponds to a RSocket in application
	* side.
	*
	* One QoS connection may temporarily associate with multiple
	* flows (CFlowHook), but in normal case there is usually only
	* at most one flow at any specific time.
	*/
	{
public:
	CQoSConn(const TQoSConnId &aId, CInternalQoSChannel& aChannel) : iId(aId), iChannel(aChannel), iFlows(2) {}
	~CQoSConn();
	TInt HookReadyL(CFlowHook& aHook);
	void Detach(CFlowHook& aHook);
	void Attach(CFlowHook& aHook);
	CFlowHook* Hook(TInt aIndex = 0);
	/**
	* The connection identification.
	* Identify the single connection which is associated with the
	* application socket that has been assigned to the channel.
	*
	* @li Note
	*	The current solution of using selector information is not
	*	bullet proof. The selector information can match unintended
	*	flows, especially if data is not UDP or TCP.
	*	Some other unique information, that is accessible
	*	to socket server and to the stack hook mechanism, should be
	*	used.
	*/
	const TQoSConnId iId;
	/**
	* The channel.
	* The QoS connection only exists as immutably attached to a
	* specific channel.
	*/
	CInternalQoSChannel& iChannel;
	/**
	* The list of currently attached flows. Usually none
	* or at most one flow at any time. But, is possible
	* to have more than one occasionally.
	*/
	RPointerArray<CFlowHook> iFlows;
	/**
	* Link connections of one channel together
	*/
	CQoSConn* iNext;

#ifdef _LOG
public:
	void Dump() const;
#endif
	};


enum TChannelStatus
	{
	EChannelClosed,			// Channel closed
	EChannelReady,			// Channel open and ready
	EChannelPending			// Channel has negotiation request pending.
	};



class CInternalQoSChannel : public CBase
	{
private:
	~CInternalQoSChannel();
public:
	static CInternalQoSChannel* NewL(TInt aChannelId, const TPfqosMessage& aMsg, CQoSProvider* aSap, TDblQue<CInternalQoSChannel>& aList);

	TInt Join(const TQoSConnId& aIdent, CProtocolQoS& aProtocol);
	TInt Leave(const TQoSConnId& aIdent);
	TInt SetPolicy(TPfqosMessage& aMsg);
	void RequestComplete(TInt aReason);
	void AddSapL(CQoSProvider* aSap);
	void GetExtensionsL(TPfqosMessage& aMsg);
	TInt HookReadyL(CFlowHook& aHook);
	void CloseChannel();

	inline TInt ChannelId() const;
	inline CExtensionPolicy& Extension();
	inline TQoSParameters& QoSParameters();
	inline CInterface* Interface();

private:
	CInternalQoSChannel(TInt aChannelId, TDblQue<CInternalQoSChannel>& aList);
	void ConstructL(const TPfqosMessage& aMsg, CQoSProvider& aSap);
	void SetQoSParametersL(const TPfqosMessage& aMsg);
	CQoSConn* Catched(CFlowHook* aFlow) const;
	CQoSConn* Exists(const TQoSConnId& aId) const;
	CFlowHook* Hook();

	inline void Lock();
	TBool Unlock();
	TBool DeleteIfUnlocked();

	void DeliverEvent(CFlowHook* aHook, TUint16 aEvent, TUint16 aValue=0, TInt aErrorCode=KErrNone);

	// ...only used from friend CQoSChannelManager
	void RemoveSap(CQoSProvider* aSap, TBool aAllReferences);

	const TInt iChannelId;				//< Unique channel id.

	TDblQueLink iLink;
	/**
	* "Callback" destruction protection.
	*
	* Some processing of channels requires calling external components,
	* and it is possible that those calls come back and try to destroy
	* the channel in middle of our processing. The iLocks delays the
	* actual delete to a safe point.
	* @li iLocks = 0, object can be deleted.
	* @li iLocks > 0, object cannot be deleted yet.
	*/
	TUint16 iLocks;
	/**
	* Delayed delete pending.
	* Object is sheduled for destruction, but cannot be done
	* due to iLocks > 0.
	*/
	TUint iPendingDelete:1;
	/**
	* The QoS parameters (iQoS/iExtensions) have been changed.
	* Non-zero, when QoS parameters have been changed and
	* need to be negotiated with the modules.
	*/
	TUint iQoSChanged:1;

	TQoSParameters iQoS;

	//?? C-class as member variable!! Logically should allocate this from heap,
	//?? but the usage assumes it always exists. The problem is really misnamed
	//?? class, this  should probably be RExtensionPolicy with appropriate Close()
	//?? or Free() method.
	CExtensionPolicy iExtension;
	TChannelStatus iStatus;
	CQoSConn *iConnections;				//< The current connections on this channel.
	CInterface* iInterface;				//< The interface copied from the first flow joined.
	const CModuleSelector* iModulesel;	//< The module selector copied from the flow joined
	RPointerArray<RModule> iModuleList;	//< The modules for which channel has been opened.
	CSapItem* iSapList;					//< The current set of control SAPs
	friend class CQoSChannelManager;

#ifdef _LOG
public:
	void Dump() const;
#endif
	};

const TInt KQoSChannelNone			= -1;
const TUint KQoSChannelNumberMin	= 1;
const TUint KQoSChannelNumberMax	= 1024;

class CQoSChannelManager : public CBase
	{
public:
	~CQoSChannelManager();
	static CQoSChannelManager* NewL(CProtocolQoS& aProtocol);

	CInternalQoSChannel* NewChannelL(const TPfqosMessage& aMsg, CQoSProvider* aSap);
	CInternalQoSChannel* OpenChannelL(const TPfqosMessage& aMsg, CQoSProvider* aSap);
	TInt DetachFromOne(CQoSProvider* aSap, TUint aChannelId);
	void DetachFromAll(CQoSProvider* aSap);

	TInt JoinChannel(const TPfqosMessage& aMsg, TInt aChannelId);
	TInt LeaveChannel(const TPfqosMessage& aMsg, TInt aChannelId);


	CQoSConn* AttachChannel(CFlowHook *aHook);


	CInternalQoSChannel* FindChannel(TUint aChannelId);
	void InterfaceDetached(CInterface* aInterface);

private:
	CQoSChannelManager(CProtocolQoS& aProtocol);
	TInt AssignChannelId();
	CInternalQoSChannel* FindChannel(const TQoSConnId& aFlow);

	CProtocolQoS& iProtocol;
	TUint iNextChannelNumber;
	TDblQue<CInternalQoSChannel> iChannels;
	};

//
inline void CInternalQoSChannel::Lock()
	{ ++iLocks; }
inline TInt CInternalQoSChannel::ChannelId() const
	{ return iChannelId; }

inline CExtensionPolicy&  CInternalQoSChannel::Extension()
	{ return iExtension; };

inline TQoSParameters& CInternalQoSChannel::QoSParameters()
	{ return iQoS; };

inline CInterface* CInternalQoSChannel::Interface()
	{ return iInterface; };

#endif
