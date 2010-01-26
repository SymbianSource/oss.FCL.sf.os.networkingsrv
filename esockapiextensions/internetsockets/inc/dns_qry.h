/**
* Copyright (c) 1997-2009 Nokia Corporation and/or its subsidiary(-ies).
* All rights reserved.
* This component and the accompanying materials are made available
* under the terms of "Eclipse Public License v1.0"
* which accompanies this distribution, and is available
* at the URL "http://www.eclipse.org/legal/epl-v10.html".
*
* Initial Contributors:
* Nokia Corporation - initial contribution.
*
* Contributors:
*
* Description:
* DNS queries and results definition
* 
*
*/



/**
 @file dns_qry.h
 @publishedPartner
 @released
*/

#ifndef __DNS_QRY_H__
#define __DNS_QRY_H__

#include <e32base.h>
#include <es_sock.h>
#include <in_sock.h>
#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <dns_qry_internal.h>
#endif


/** 
DNS query buffer type 
*/
typedef TBuf8<255>  TDnsQryData;

/**
representation of domain name in terms of DNS responses
*/
typedef TBuf8<255>  TDnsDomainName;

/**
representation of a character string in DNS responses 
*/
typedef TBuf8<255>  TDnsString;

//-- DNS RR class values, see RFC 1035
const TUint16	KDnsRRClassIN = 1;  ///< Internet class
const TUint16	KDnsRRClassCS = 2;  ///< CSNET class
const TUint16	KDnsRRClassCH = 3;  ///< CHAOS class
const TUint16   KDnsRRClassHS = 4;  ///< Hesiod


//-- DNS RR and query type values, see RFC1035
const TUint16   KDnsRRTypeInvalid   = 0;    ///< invalid terminal value
const TUint16   KDnsRRTypeA         = 1;    ///< host address RR type
const TUint16   KDnsRRTypeNS        = 2;    ///< authoritative name server
const TUint16   KDnsRRTypeCNAME     = 5;    ///< canonical name
const TUint16   KDnsRRTypeWKS       = 11;   ///< well known service description
const TUint16   KDnsRRTypePTR       = 12;   ///< domain name pointer
const TUint16   KDnsRRTypeHINFO     = 13;   ///< host information
const TUint16   KDnsRRTypeMX        = 15;   ///< mail exchange
const TUint16   KDnsRRTypeTXT       = 16;   ///< text strings

const TUint16   KDnsRRTypeAAAA      = 28;   ///< AAAA RR type
const TUint16   KDnsRRTypeSRV       = 33;   ///< SRV RR type
const TUint16   KDnsRRTypeNAPTR     = 35;   ///< NAPTR RR type

//-- DNS RR query values only
const TUint16   KDnsQTypeAXFR       = 252;  ///< request for a transfer of an entire zone
const TUint16   KDnsQTypeMAILB      = 253;  ///< request for mailbox-related records (MB, MG or MR)
const TUint16   KDnsQTypeANY        = 255;  ///< request for all records


class   TDnsQuery
/**
DNS query representation.
See RFC1035.

@publishedPartner
@released
*/
    {
    public:    
        inline  TDnsQuery();    //- default constructor
        inline  TDnsQuery(const TDesC8& aQryDomainName, TUint16 aType, TUint16 aClass = KDnsRRClassIN);

        inline  TUint16 Type()        const; 
        inline  TUint16 Class()       const; 
        inline  const TDesC8& Data()  const; 
        
        inline  void SetType(TUint16 aType);
        inline  void SetClass(TUint16 aClass);
        inline  void SetData(const TDesC8& aData);

    protected:

        TUint16     iQryType;   ///< DNS query code
        TUint16     iQryClass;  ///< DNS query class code
        TDnsQryData iQryData;   ///< DNS query data (buffer)

    };

typedef TPckgBuf<TDnsQuery> TDnsQueryBuf;



class TDnsQryRespBase
/**
DNS query response representation.
This is a base class and is not intended to be instantinated.
See RFC1035.

@publishedPartner
@released
*/
    {
    protected:
        //-- protected constructors to make instantination of this class impossible.
        inline  TDnsQryRespBase();
        inline  TDnsQryRespBase(TUint16 aRRespType, TUint16 aRRClass);

    public:

        inline TUint16 RRType()  const; 
        inline TUint16 RRClass() const; 
        inline TUint32 RRTtl()   const; 

        inline void SetRRTtl  (TUint32 aRRTtl);

    protected:
        //-- common data members for all DNS query results
        const TUint16 iRespType;     ///< RR type
        const TUint16 iRespClass;    ///< RR Class
              TUint32 iRespTtl;      ///< RR TTL

    };


class TDnsRespSRV : public TDnsQryRespBase
/**
DNS SRV query response representation.
See RFC2782.

@publishedPartner
@released
*/
    {
    public:

        inline TDnsRespSRV();

        inline TUint16 Priority() const;        
        inline TUint16 Weight() const;          
        inline TUint16 Port() const;            
        inline const TDesC8& Target() const;    
    
        inline void SetPriority(TUint16 aPriority);
        inline void SetWeight  (TUint16 aWeight);
        inline void SetPort    (TUint16 aPort);
        inline void SetTarget (const TDesC8& aTarget);

    protected:

        TUint16         iPriority;  ///< The priority of this target host
        TUint16         iWeight;    ///< the value of the weight field
        TUint16         iPort;      ///< port number
        TDnsDomainName  iTarget;    ///< domain name of the target host.
    };

typedef TPckgBuf<TDnsRespSRV> TDnsRespSRVBuf;

class TDnsRespA : public TDnsQryRespBase
/**
DNS Host Address query response representation.
See RFC1034, 1035.

@publishedPartner
@released
*/
    {
    public:

        TDnsRespA() : TDnsQryRespBase(KDnsRRTypeA, KDnsRRClassIN)  {}

        inline const TInetAddr&  HostAddress() const; 
        inline void  SetHostAddress(const TInetAddr& aInetAddr);

    protected:
        TInetAddr   iInetAddr;  ///< Host address
    };

typedef TPckgBuf<TDnsRespA> TDnsRespABuf;


class TDnsRespAAAA : public TDnsQryRespBase
/**
IPv6 DNS Host Address query response representation.
See RFC1035, RFC1886

@publishedPartner
@released
*/
    {
    public:

        TDnsRespAAAA() : TDnsQryRespBase(KDnsRRTypeAAAA, KDnsRRClassIN) {}

        inline const TInetAddr&  HostAddress() const;   
        inline void SetHostAddress(const TInetAddr& aInetAddr);

    protected:
        TInetAddr   iInetAddr;  ///< Host address
    };

typedef TPckgBuf<TDnsRespAAAA> TDnsRespAAAABuf;


class TDnsRespPTR : public TDnsQryRespBase
/**
DNS Domain Name query response representation.
See RFC1034.

@publishedPartner
@released
*/
    {
    public:

        TDnsRespPTR() : TDnsQryRespBase(KDnsRRTypePTR, KDnsRRClassIN) {}

        inline const TDesC8& HostName() const; 
        inline void  SetHostName(const TDesC8& aHostName);

    protected:
        TDnsDomainName    iName;  ///< domain this RR refers to.
    };

typedef TPckgBuf<TDnsRespPTR> TDnsRespPTRBuf;


class TDnsRespNAPTR : public TDnsQryRespBase
/** 
DNS NAPTR query response representation.
See RFC2915.

@publishedPartner
@released
*/
    {
    public:

        inline TDnsRespNAPTR();

        inline TUint16         Order()       const; 
        inline TUint16         Pref()        const; 
        inline const TDesC8&   Flags()       const; 
        inline const TDesC8&   Service()     const; 
        inline const TDesC8&   Regexp()      const; 
        inline const TDesC8&   Replacement() const; 

        inline void SetOrder(TUint16 aOrder);
        inline void SetPref(TUint16 aPref);
        inline void SetFlags(const TDesC8& aFlags);
        inline void SetService(const TDesC8& aService);
        inline void SetRegexp(const TDesC8& aRegexp);
        inline void SetReplacement(const TDesC8& aReplacement);

    protected:

        TUint16         iOrder;      ///< RR Order field
        TUint16         iPref;       ///< RR Preference field
        TDnsString      iFlags;      ///< RR Flags string
        TDnsString      iService;    ///< service name(s) available
        TDnsString      iRegexp;     ///< RR Regexp field
        TDnsDomainName  iReplacement;///< RR Replacement field
    };

typedef TPckgBuf<TDnsRespNAPTR> TDnsRespNAPTRBuf;

class TDnsRespMX: public TDnsQryRespBase
/**
DNS MX query response representation.
See RFC1035, RFC974.

@publishedPartner
@released
*/
    {
    public:

        TDnsRespMX() : TDnsQryRespBase(KDnsRRTypeMX, KDnsRRClassIN) {}

        inline TUint16         Pref()      const; 
        inline const TDesC8&   HostName()  const; 

        inline void SetPref(TUint16 aPref);
        inline void SetHostName(const TDesC8& aHostName);

    protected:

        TUint16         iPref;       ///< RR Preference field.
        TDnsDomainName  iHostName;   ///< Host name.

    };

typedef TPckgBuf<TDnsRespMX> TDnsRespMXBuf;

// -- DNS query type value, DoCoMo Cache requirement
// This is used in conjunction with RHostResolver::Query
// Example usage:
//
// 		TDnsQueryBuf dnsQryBuf;
//		TDnsRespABuf dummy;
//
//		dnsQryBuf().SetType(KDnsQTypeCacheClear);
//		resolver.Query(dnsQryBuf, dummy, status); 
//		User::WaitForRequest(status);
//
const TUint16   KDnsQTypeCacheClear = 99;   ///< Resolver Cache Clear type
// -- DNS non-recursive look up, DoCoMo requirement
// This is used in conjunction with RHostResolver::GetByName.
// Example usage:
//
//		TNameEntry nameEntry;
//		TBuf<256> hostName(KDnsNonrecursive);
//		hostName.Append(_L("www.symbian.com"));
//
//		resolver.GetByName(hostName,nameEntry, status);
//		User::WaitForRequest(status);
//
_LIT(KDnsNonrecursive, "NONRECURSIVE?");

#include "dns_qry.inl"

#endif //__DNS_QRY_H__
















