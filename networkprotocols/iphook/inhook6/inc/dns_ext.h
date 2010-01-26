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
// dns_ext.h - Extensions to the INET DNS API
//



/**
 @internalComponent
*/

#ifndef __DNS_EXT_H__
#define __DNS_EXT_H__

#include <es_sock.h>

_LIT(KDnsExtQType_A,	"A?"	);
_LIT(KDnsExtQType_NS,	"NS?"	);
_LIT(KDnsExtQType_CNAME,"CNAME?");
_LIT(KDnsExtQType_SOA,	"SOA?"	);
//_LIT(KDnsExtQType_WKS,	"WKS?"	);
_LIT(KDnsExtQType_PTR,	"PTR?"	);
//_LIT(KDnsExtQType_HINFO,"HINFO?");
_LIT(KDnsExtQType_MX,	"MX?"	);
//_LIT(KDnsExtQType_TXT,	"TXT?"	);
_LIT(KDnsExtQType_AAAA,	"AAAA?"	);
_LIT(KDnsExtQType_NAPTR,"NAPTR?");
_LIT(KDnsExtQType_SRV,	"SRV?"	);
_LIT(KDnsExtQType_ANY,	"ANY?"	);


const TUint KAfDns = 0x807;		//


// There is no TDnsRR_A:		Reply is KAfInet
// There is no TDnsRR_AAAA:		Reply is KAfInet6
// There is no TDnsRR_CNAME:	Reply is KAfInet with 0.0.0.0
//
// DNS extension answers:		Reply is KAfDns


// 3.3.9. MX RDATA format
//
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     |                  PREFERENCE                   |
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//     /                   EXCHANGE                    /
//     /                                               /
//     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
// 
// where:
// 
// PREFERENCE      A 16 bit integer which specifies the preference given to
//                 this RR among others at the same owner.  Lower values
//                 are preferred.
// 
// EXCHANGE        A <domain-name> which specifies a host willing to act as
//                 a mail exchange for the owner name.
// 
class TDnsRR_MX
	{
public:
	TUint16 iPreference;	// PREFERENCE
	//
	// iName
	//
	// - EXCHANGE
	};

// 3.3.13. SOA RDATA format
//
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    /                     MNAME                     /
//    /                                               /
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    /                     RNAME                     /
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                    SERIAL                     |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                    REFRESH                    |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                     RETRY                     |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                    EXPIRE                     |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//    |                    MINIMUM                    |
//    |                                               |
//    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
// where:
//
//MNAME           The <domain-name> of the name server that was the
//                original or primary source of data for this zone.
//
//RNAME           A <domain-name> which specifies the mailbox of the
//                person responsible for this zone.
//
//SERIAL          The unsigned 32 bit version number of the original copy
//                of the zone.  Zone transfers preserve this value.  This
//                value wraps and should be compared using sequence space
//                arithmetic.
//
//REFRESH         A 32 bit time interval before the zone should be
//                refreshed.
//
//RETRY           A 32 bit time interval that should elapse before a
//                failed refresh should be retried.
//
//EXPIRE          A 32 bit time value that specifies the upper limit on
//                the time interval that can elapse before the zone is no
//                longer authoritative.
//
//MINIMUM         The unsigned 32 bit minimum TTL field that should be
//                exported with any RR from this zone.
class TDnsRR_SOA
	{
public:
	TUint32 iSerial;
	TUint32 iRefresh;
	TUint32 iRetry;
	TUint32 iExpire;
	TUint32 iMinimum;
	//
	// iName
	//
	// - MNAME @ RNAME
	};

// RFC-2915
//
//         The packet format for the NAPTR record is:
//
//                                          1  1  1  1  1  1
//            0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          |                     ORDER                     |
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          |                   PREFERENCE                  |
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          /                     FLAGS                     /
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          /                   SERVICES                    /
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          /                    REGEXP                     /
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//          /                  REPLACEMENT                  /
//          /                                               /
//          +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
//
//
//    where:
//
//   FLAGS A <character-string> which contains various flags.
//
//   SERVICES A <character-string> which contains protocol and service
//      identifiers.
//
//   REGEXP A <character-string> which contains a regular expression.
//
//   REPLACEMENT A <domain-name> which specifies the new value in the
//      case where the regular expression is a simple replacement
//      operation.
//
//   <character-string> and <domain-name> as used here are defined in
//   RFC1035 [1].
//
class TDnsRR_NAPTR
	{
public:
	TUint16 iOrder;		// ORDER
	TUint16 iPreference;// PREFERENCE
	TUint8 iL1;			// Number of characters in iName REPLACEMENT
	TUint8 iL2;			// Number of characters in iName SERVICES
	TUint8 iFlags[12];	// FLAGS

	//
	// iName
	// - REPLACEMENT + SERVICES + REGEXP
	//
	inline TPtrC REPLACEMENT(const TDesC &aName) const { return aName.Left((TInt)iL1); }
	inline TPtrC SERVICES(const TDesC &aName) const { return aName.Mid((TInt)iL1, (TInt)iL2); }
	inline TPtrC REGEXP(const TDesC &aName) const { return aName.Right(aName.Length() - (TInt)iL1 - (TInt)iL2); }
	};


// RFC-2782
class TDnsRR_SRV
	{
	// Port is in the Port() field of the address
public:
	TUint16 iPriority;
	TUint16 iWeight;
	// iName
	// - Target is returned in iName
	};


struct SDnsRR
	{
	TUint16 iClass;		// RR Class
	TUint16 iType;		// RR Type
	union
		{
		TDnsRR_SOA iSOA;
		TDnsRR_NAPTR iNAPTR;
		TDnsRR_SRV iSRV;
		TDnsRR_MX iMX;
		};
	};


class TInetDnsRR : public TSockAddr
	{
public:
	TInetDnsRR() : TSockAddr(KAfDns) { SetUserLen(sizeof(SDnsRR)); }
	inline SDnsRR &RR() { return *(SDnsRR *)UserPtr(); }

	inline static TInetDnsRR& Cast(const TSockAddr& aAddr)
		{
		return *((TInetDnsRR *)&aAddr);
		}
	inline static TInetDnsRR& Cast(const TSockAddr* aAddr)
		{
		return *((TInetDnsRR *)aAddr);
		}
	};

#endif
