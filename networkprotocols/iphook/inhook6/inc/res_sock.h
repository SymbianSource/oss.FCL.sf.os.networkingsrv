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
// res_sock.h - Resolver socket API
// This API defines structures and constants which are used
// between the TCP/IP stack and an external DNS resolver
// daemon.
//

#ifndef __RES_SOCK_H__
#define __RES_SOCK_H__
/**
@file res_sock.h
The resolver socket message format
@internalComponent	Domain Name Resolver
*/

#include <e32def.h>
#include <dns_qry.h>


/**
* DND Socket option to specify the max sessions.
*
* The DND needs to tell the gateway code how many
* parallel RHostResolvers (sessions) it can handle.
* This is done by setting this socket option.
*/
const TUint KSolDnd			= 0x10000882;	// Use DND UID value as a level.
const TUint KSoDndSessions	= 1;			// Only SetOption, and parameter is "int" (= number of sessions)


const TInt KDnsRequestType_GetByName = 0;	//< Query is GetByName
const TInt KDnsRequestType_GetByAddress = 1;//< Query is GetByAddress
const TInt KDnsRequestType_TDnsQuery = 2;	//< Query is special DNS query
const TInt KDnsRequestType_GetHostName = 3;	//< Query current hostname (blocks until unique if LLMNR)
const TInt KDnsRequestType_SetHostName = 4;	//< Set new hostname (start unique checking if LLMNR)
const TInt KDnsRequestType_Configure = 5;	//< Interface state in stack has changed.

class TDnsRequestBase
/**
  The resolver socket message format

  The communication between the resolver gateway code in the stack and
  the resolver implementation occurs over a datagram socket. The
  queries and replies are passed as datagrams. Each datagram contains
  a binary message (TDnsMessage) with three fixed 32 bit header fields
  and a payload.

  In query the fixed fields are defined as follows:
@li
  (1) the payload type (TNameRecord, TDnsQuery or THostName)
@li
  (2) the "next indicator" (0 is for new query, non-zero requests next
  answer for earlier query).
@li
  (3) the network id. The value ZERO indicates unknown; in such case
  the resolver only looks name from local host file, and if not found,
  a KErrCompletion error is returned. This error is special, and requests
  the Socket Server to assign a non-zero network id and repeat the query.
@li
  followed by the query payload, as defined by the first header field.
  The payload is TNameRecord, if the query was GetByName or GetByAddress.
  TDnsQuery payload defines the query explicitly.

  Any message shorter than the required minimum length is
  interpreted by the resolver as <b>cancel request</b>. Such message
  will stop all currently occurring activity associated with this session,
  and disconnect the resolver from the socket. This
  is part of the protocol and is also used by the resolver gateway
  code, when application cancels the request (it puts a zero length
  message into the socket).

  In a reply message, the same header format is used, but the fields have
  partially different interpretation:
@li
  (1) the payload type (TNameRecord, TDnsQryRespBase derived object,
  THostName).
  The value of the field is the same as in the originating query message,
  except for GetHostName.
@li
  (2) the result code of the query (KErrNone or some error indication)
@li
  (3) the network id (same as in query)
@li
   followed by the response payload, as defined by the first header field.
   The payload is returned to the client application as a reply to the query.
*/
	{
public:
	TDnsRequestBase() {}
	TDnsRequestBase(TInt aType, TInt aNext) : iType((TUint8)aType), iNext(aNext) {}

	inline const TInt HeaderSize() const
		{
		return (TInt)sizeof(*this);
		}

	inline TPtrC8 Header() const
		{
		return TPtrC8((TUint8 *)this, HeaderSize());
		}

	TUint8 iType;		//< Type of the payload: TDnsQuery/TDnsQryResp or TNameRecord
	TUint8 iScope;		//< The scope level of the Address Domain Id [0..15]
#ifdef SYMBIAN_DNS_PUNYCODE
				//< The MSB 4 bits are used to indicate whether the IDN support is enabled / disabled
#endif //SYMBIAN_DNS_PUNYCODE
	TUint16 iSession;	//< The session ID.
	TInt iNext;			//< Next indicator: 0, 1, 2, ... (query), or the result (on reply)
	TUint32 iId;		//< Address domain Id
	};

// Need write access to members, and just for this, need to derive all
// responce classes... (yuk!)
//
class TDndRespA : public TDnsRespA
	{
public:
	inline TDndRespA() : TDnsRespA() {} 
	inline TInetAddr&  HostAddress() { return iInetAddr; }
	};

class TDndRespAAAA : public TDnsRespAAAA
	{
public:
	inline TDndRespAAAA() : TDnsRespAAAA() {}
	inline TInetAddr&  HostAddress() { return iInetAddr; }
	};

class TDndRespPTR : public TDnsRespPTR
	{
public:
	inline TDndRespPTR() : TDnsRespPTR() {}
	inline TDes8& HostName() { return iName; }
	};

class TDndRespMX : public TDnsRespMX
	{
public:
	inline TDndRespMX() : TDnsRespMX() {}
	inline TDes8 &HostName() { return iHostName; }
	};

class TDndRespSRV : public TDnsRespSRV
	{
public:
	inline TDndRespSRV() : TDnsRespSRV() {}
	inline TDes8 &Target() { return iTarget; }
	};

class TDndReply : public TDnsQryRespBase
	{
public:
	inline TDndReply(TUint16 aRRespType, TUint16 aRRClass) : TDnsQryRespBase(aRRespType, aRRClass) {}
	};

class TDndRespNAPTR : public TDnsRespNAPTR
	{
public:
	inline TDndRespNAPTR() : TDnsRespNAPTR() {}
	inline TDes8 &Replacement() { return iReplacement; }
	};

class TDnsMessage : public TDnsRequestBase
	{
public:
	inline TPtrC8 Payload(const TInt aSize) const
		{
		const TInt N = aSize < HeaderSize() ? 0 : aSize - HeaderSize();
		return TPtrC8((TUint8 *)this + HeaderSize(),  N);
		}
	inline TPtr8 Payload(const TInt aSize)
		{
		const TInt N = aSize < HeaderSize() ? 0 : aSize - HeaderSize();
		return TPtr8((TUint8 *)this + HeaderSize(),  N, N);
		}

private:
	// This union is here to make the size of this
	// class large enough for the largest of the
	// reply or query variants. (must use TUint8
	// arrays, because the classes themselves cannot
	// be used in union due to contructor problems).
	union
		{
		TUint8 iNameRecord[sizeof(TNameRecord)];		//< GetByName/GetByAddress query/reply
		TUint8 iDnsQuery[sizeof(TDnsQuery)];			//< Special Query
		TUint8 iDnsRespSRV[sizeof(TDnsRespSRV)];		//< SRV reply
		TUint8 iDnsRespA[sizeof(TDnsRespA)];			//< A reply
		TUint8 iDnsRespAAAA[sizeof(TDnsRespAAAA)];		//< AAAA reply
		TUint8 iDnsRespPTR[sizeof(TDnsRespPTR)];		//< PTR reply
		TUint8 iDnsRespNAPTR[sizeof(TDnsRespNAPTR)];	//< NAPTR reply
		TUint8 iDnsRespMX[sizeof(TDnsRespMX)];			//< MX reply
		TUint8 iHostName[sizeof(THostName)];			//< SetHostName/GetHostName
		} iPayload;
public:
	// Access to the query structures

	// GetByName/GetByAddress query/reply
	inline TNameRecord &NameRecord() const
		{ return (TNameRecord &)iPayload; }
	// Special Query
	inline TDnsQuery &Query() const
		{ return (TDnsQuery &)iPayload; }
	// Set/Get HostName
	inline THostName &HostName() const
		{ return (THostName &)iPayload; }

	// Access the the Reply structures

	// Generic Response header
	inline TDndReply &Reply() const	{ return (TDndReply &)iPayload; }
	// SRV reply
	inline TDndRespSRV &SRV() const { return (TDndRespSRV &)iPayload; }
	// A reply
	inline TDndRespA &A() const	{ return (TDndRespA &)iPayload; }
	// AAAA reply
	inline TDndRespAAAA &AAAA() const { return (TDndRespAAAA &)iPayload; }
	// PTR reply
	inline TDndRespPTR &PTR() const	{ return (TDndRespPTR &)iPayload; }
	// NAPTR reply
	inline TDndRespNAPTR &NAPTR() const	{ return (TDndRespNAPTR &)iPayload; }
	// MX reply
	inline TDndRespMX &MX() const { return (TDndRespMX &)iPayload; }
	};

typedef TPckgBuf<TDnsMessage> TDnsMessageBuf;

#endif

