// Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef __POLICY_H__
#define __POLICY_H__

#include <ip6_hook.h>

class MPacketSpec
	{
public:
	// For now, do a bitwise match
	virtual TBool Match(const MPacketSpec& aSpec) const = 0;
	virtual ~MPacketSpec() {}
	};

class MActionSpec
	{
public:
	virtual TBool Match(const MPacketSpec& aSpec) const = 0;
	// For flow hooks
	virtual TInt ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo) = 0;
	// For Incoming hooks
	virtual TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo) = 0;

	virtual ~MActionSpec() {}
	};

/*
 *	A combination of match and corresponding action
 */
class CPolicyHolder;
class CPolicySpec : public CBase
	{
public:
	CPolicySpec(const MPacketSpec& aPacketSpec, const MActionSpec& aAction);
	~CPolicySpec();

	// Takes a packet spec & gives back the action we need to take
	TBool Match(const MPacketSpec& aSpec, MActionSpec& aAction) const;

private:
friend class CPolicyHolder;
	const MPacketSpec* iMatchSpec;
	const MActionSpec* iActionSpec;
	};

class CPolicyHolder : public CBase
	{
public:
	CPolicyHolder() {}
	~CPolicyHolder();

	void AddSpecL(const MPacketSpec& aPacket, const MActionSpec& aAction);
	void DelSpec(const TInt index);

	// Takes a packet header and gives the action that needs to be taken on the packet
	//const MActionSpec *const MatchSpec(const MPacketSpec& aPacket, TBool aIsIncoming) const;
	const MActionSpec * MatchSpec(const MPacketSpec& aPacket, TBool aIsIncoming) const;

private:
	TInt FindAction(const MPacketSpec& aPacket) const;
	TInt FindPolicy(const MPacketSpec& aPacket) const;

private:
	RPointerArray<CPolicySpec> iPolicyList;
	};

#include "ipip6.h"
/*
 *	A packet that needs to be matched
 */
class CIPIPPacketSpec : public CBase, public MPacketSpec
	{
public:
	static CIPIPPacketSpec* NewLC(const TInetAddr& aSrc, const TInetAddr& aDst);
	// Can also be created on stack
	CIPIPPacketSpec(const TInetAddr& aSrc, const TInetAddr& aDest);
	~CIPIPPacketSpec();

	// Returns whether the packetspec matched
	TBool Match(const MPacketSpec& aSpec) const;
	TBool Match(const TInetAddr& aSrc, const TInetAddr& aDst) const;
public:
	TIPIPPacketSpec iPacketSpec;
private:
	
	};

/*
 *	The action we need to take
 */
class CIpIpActionSpec : public CBase, public MActionSpec
	{
public:
	static CIpIpActionSpec* NewLC(const TInetAddr& aSrc, const TInetAddr& aDst);
	~CIpIpActionSpec();

	TBool Match(const MPacketSpec& aSpec) const;
	// For flow hooks
	TInt ApplyL(RMBufSendPacket& aPacket, RMBufSendInfo& aInfo);
	// For Incoming hooks
	TInt ApplyL(RMBufHookPacket& aPacket, RMBufRecvInfo& aInfo);
public:
	TIPIPActionSpec iActionSpec;
private:
	CIpIpActionSpec(const TInetAddr& aSrc, const TInetAddr& aDest);

    TInt iNextId;
    inline TUint AllocId() {if (iNextId >= 0xffff) iNextId = 1; else ++iNextId; return iNextId; }
	};

#endif //__POLICY_H__
