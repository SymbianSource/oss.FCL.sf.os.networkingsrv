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
// dns_hdr.h - name resolver DNS protocol headers
//

#ifndef __DNS_HDR_H__
#define __DNS_HDR_H__

#include <e32base.h>
/**
@file dns_hdr.h
DNS Protocol definitions
@internalComponent	Domain Name Resolver
*/

/**
Max length of any label (raw data without the length octet)
*/
const TInt KDnsMaxLabel = 63;

/**
Max length of any *uncompressed* domain
name consisting of a sequence of labels
(this includes the length octets of the
labels and the terminating NUL byte for
the root label).
*/
const TInt KDnsMaxName = 256;

const TInt KDnsPort = 53;		//< Default port of the DNS server 

const TInt KDnsMaxMessage = 512;//< Max length of the DNS message

const TInt KDnsMinHeader = 12;	//< Min length of the UDP header

typedef enum
	{
	// from RFC-1035
	EDnsType_A			= 1,	//< a host address
	EDnsType_NS			= 2,	//< an authoritative name server
	EDnsType_MD			= 3,	//< a mail destination (Obsolete - use MX)
	EDnsType_MF			= 4,	//< a mail forwarder (Obsolete - use MX)
	EDnsType_CNAME		= 5,	//< the canonical name for an alias
	EDnsType_SOA		= 6,	//< marks the start of a zone of authority
	EDnsType_MB			= 7,	//< a mailbox domain name (EXPERIMENTAL)
	EDnsType_MG			= 8,	//< a mail group member (EXPERIMENTAL)
	EDnsType_MR			= 9,	//< a mail rename domain name (EXPERIMENTAL)
	EDnsType_NULL		= 10,	//< a null RR (EXPERIMENTAL)
	EDnsType_WKS		= 11,	//< a well known service description
	EDnsType_PTR		= 12,	//< a domain name pointer
	EDnsType_HINFO		= 13,	//< host information
	EDnsType_MINFO		= 14,	//< mailbox or mail list information
	EDnsType_MX			= 15,	//< mail exchange
	EDnsType_TXT		= 16,	//< text strings
	//
	// later additions
	//
	EDnsType_AAAA		= 28,	//< single IPv6 address (RFC-1886)
	EDnsType_DNAME		= 29,	//< Non-Terminal DNS Name Redirection (RFC-2672)

	EDnsType_SRV		= 33,	//< Location of Services (RFC-2782)

	EDnsType_NAPTR		= 35,	//< Naming Authority Pointer (RFC-2915)

	EDnsType_OPT		= 41	//< OPT pseudo-RR (RFC-2671) [never cached]
	} EDnsType;

typedef enum
	{
	// from RFC-1035
	// (all values of EDnsType and following)
	//
	// Putting all of them together -->
	EDnsQType_A			= 1,	//< a host address
	EDnsQType_NS		= 2,	//< an authoritative name server
	EDnsQType_MD		= 3,	//< a mail destination (Obsolete - use MX)
	EDnsQType_MF		= 4,	//< a mail forwarder (Obsolete - use MX)
	EDnsQType_CNAME		= 5,	//< the canonical name for an alias
	EDnsQType_SOA		= 6,	//< marks the start of a zone of authority
	EDnsQType_MB		= 7,	//< a mailbox domain name (EXPERIMENTAL)
	EDnsQType_MG		= 8,	//< a mail group member (EXPERIMENTAL)
	EDnsQType_MR		= 9,	//< a mail rename domain name (EXPERIMENTAL)
	EDnsQType_NULL		= 10,	//< a null RR (EXPERIMENTAL)
	EDnsQType_WKS		= 11,	//< a well known service description
	EDnsQType_PTR		= 12,	//< a domain name pointer
	EDnsQType_HINFO		= 13,	//< host information
	EDnsQType_MINFO		= 14,	//< mailbox or mail list information
	EDnsQType_MX		= 15,	//< mail exchange
	EDnsQType_TXT		= 16,	//< text strings
	//
	// later additions
	//
	EDnsQType_AAAA		= 28,	//< single IPv6 address (RFC-1866)
	EDnsQType_DNAME		= 29,	//< Non-Terminal DNS Name Redirection (RFC-2672)

	EDnsQType_SRV		= 33,	//< Location of Services (RFC-2782)
	EDnsQType_NAPTR		= 35,	//< Naming Authority Pointer (RFC-2915)

	EDnsQType_OPT		= 41,	//< OPT pseudo-RR (RFC-2671) [never cached]

	// Only Q-types (the combined types)
	EDnsQType_AXFR		= 252,	//< A request for a transfer of an entire zone
	EDnsQType_MAILB		= 253,	//< A request for mailbox-related records (MB, MG or MR)
	EDnsQType_MAILA		= 254,	//< A request for mail agent RRs (Obsolete - see MX)
	EDnsQType_ANY		= 255	//< A request for all records
	} EDnsQType;

typedef enum
	{
	// from RFC-1035
	EDnsClass_IN		= 1,	//< the Internet
	EDnsClass_CS		= 2,	//< CSNET class (Obsolete - used only for examples in some obsolete RFCs)
	EDnsClass_CH		= 3,	//< the CHAOS class
#ifdef LLMNR_ENABLED
	EDnsClass_HS		= 4,	//< Hesiod [Dyer 87]
    EDnsClass_NONE      = 254   //< for dynamic DNS/LLMNR update, RFC 2136, 1.3
#else
	EDnsClass_HS		= 4		//< Hesiod [Dyer 87]
#endif
	} EDnsClass;

typedef enum
	{
	// from RFC-1035
	// (all values of EDnsCalss and following)
	// Putting all of them together --->
	EDnsQClass_IN		= 1,	//< the internet
	EDnsQClass_CS		= 2,	//< CSNET class (Obsolete - used only for examples in some obsolete RFCs)
	EDnsQClass_CH		= 3,	//< the CHAOS class
	EDnsQClass_HS		= 4,	//< Hesiod [Dyer 87]

	// Combined classes - only for question section
	EDnsQClass_ANY		= 255	//< any class
	} EDnsQClass;

//
// OPCODE
//
typedef enum
	{
	EDnsOpcode_QUERY	= 0x00,	//< Standard Query
	EDnsOpcode_IQUERY	= 0x01,	//< Inverse Query (historical)
#ifdef LLMNR_ENABLED
	EDnsOpcode_STATUS	= 0x02,	//< Server Status Query
    EDnsOpcode_UPDATE   = 0x05  //< Dynamic DNS/LLMNR update (RFC 2136, 1.3)
#else
	EDnsOpcode_STATUS	= 0x02	//< Server Status Query
#endif
	} EDnsOpcode;

/**
// RCODE [0..15]
*/
typedef enum
	{
	EDnsRcode_NOERROR			= 0,
	EDnsRcode_FORMAT_ERROR		= 1,
	EDnsRcode_SERVER_FAILURE	= 2,
	EDnsRcode_NAME_ERROR		= 3,
	EDnsRcode_NOT_IMPLEMENTED	= 4,
	EDnsRcode_REFUSED			= 5
	} EDnsRcode;

#ifdef LLMNR_ENABLED
typedef enum
	{
	EDnsUpdateRcode_NOERROR		= 0,
	EDnsUpdateRcode_FORMERR		= 1,
	EDnsUpdateRcode_SERVFAIL	= 2,
	EDnsUpdateRcode_NXDOMAIN	= 3,
	EDnsUpdateRcode_NOTIMP  	= 4,
	EDnsUpdateRcode_REFUSED		= 5,
	EDnsUpdateRcode_YXDOMAIN	= 6,
	EDnsUpdateRcode_YXRRSET		= 7,
	EDnsUpdateRcode_NXRRSET		= 8
	} EDnsUpdateRcode; //< RFC 2136, 2.2
#endif

class TInet6HeaderDNS
/**
Domain Protocol Message format.
@verbatim
    +---------------------+
    |        Header       |
    +---------------------+
    |       Question      | the question for the name server
    +---------------------+
    |        Answer       | RRs answering the question
    +---------------------+
    |      Authority      | RRs pointing toward an authority
    +---------------------+
    |      Additional     | RRs holding additional information
    +---------------------+

 4.1.1. Header section format (RFC-1035)

                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      ID                       |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    QDCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ANCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    NSCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                    ARCOUNT                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
@endverbatim
*/
	{
public:
	inline static TInt MinHeaderLength() {return 12;}
	inline static TInt MaxHeaderLength() {return 512;}

	//
	// Access Methods
	//
	//		A general note: unless there is a specific reason,
	//		all bit and bit field access methods return
	//		unshifted result of the masking AND (&) operation.
	//		Conversions to "pure" boolean 1 or 0 is left up to
	//		caller if it really needs the value as such.

	// @return	ID (16 bits)
	inline TInt ID() const { return (i[0] << 8) + i[1]; }

	// @return	QR, Query (= 0) or responce  ( != 0).
	inline TInt QR() const { return i[2] & 0x80; }

	// @return OPCODE (0..15)
	inline TUint OPCODE() const { return (i[2] >> 3) & 0x0F; }

	// @return AA, Authoritative Answer (if != 0)
	inline TInt AA() const { return i[2] & 0x04; }

	// @return TC, Truncation occurred (if != 0)
	inline TInt TC() const { return i[2] & 0x02; }

	// @return RD, Recursion Desired (if != 0)
	inline TInt RD() const { return i[2] & 0x01; }

	// @return RA, Recursion Available (if != 0)
	inline TInt RA() const { return i[3] & 0x80; }

	// @return Z, Reserved (masked with 0x70)
	inline TInt Z() const { return i[3] & 0x70; }

	// @return RCODE, Responce Code (0..15)
	inline TInt RCODE() const { return i[3] & 0x0F; }

	// @return QDCOUNT, the number of entries in query section
	inline TInt QDCOUNT() const { return (i[4] << 8) + i[5]; }

	// @return ANCOUNT, the number of entries in answer section
	inline TInt ANCOUNT() const { return (i[6] << 8) + i[7]; }

	// @return NSCOUNT, the number of name server records in authority records
	inline TInt NSCOUNT() const { return (i[8] << 8) + i[9]; }

	// @return ARCOUNT, the number of entries in additional section
	inline TInt ARCOUNT() const { return (i[10] << 8) + i[11]; }

	/**
	// To set the value of RD bit of the header.
	//
	// @param aRD, set RD bit, if non-zero, clear otherwise.
	*/
	inline void SetRD(TInt aRD)
		{
		if (aRD)
			i[2] |= 0x01;
		else
			i[2] &= ~0x01;
		}

// private:
protected:			
	//
	// This allocation only covers the fixed minimal portion
	//
	TUint8 i[12];
	//
	// Methods to support accessing the variable sections
	//
public:
	TInt NameSkip(const TInt aOffset, const TInt aOffsetLimit) const
	/**
	Skip over a domain name in a message.
	
	A name consists of a sequence of labels.

	@param aOffset	start offset of the domain name
	@param aOffsetLimit	maximum value for the offset
	
	@return
	@li	< 0, unsupported format, the message is invalid or corrupt
	@li	> 0, offset of the next query or section

	<b>Note</b>: For a valid return, it should always be that
	returned value > aOffset!
	*/
		{
		TInt tag;
		TInt k = aOffset;
		//
		// Skip over the name
		//
		for (;;)
			{
			if (k >= aOffsetLimit)
				return -1;	// corrupt buffer (overflow);
			if ((tag = i[k++]) == 0)
				break;

			switch (tag & 0xC0)
				{
				default:
					return -1;	// Unsupported label format!

				case 0x00:	// 0 0 - Normal length of label
					k += tag;
					break;

				case 0xC0:	// 1 1 - A pointer (compression)
					k += 1;
					goto done;

				case 0x40:	// 0 1 - Extended Label
					//
					// As this is still in Draft stage, the following
					// code is just a reminder of possible things to
					// implement. -- msa
					//
					switch (tag & 0x3F)
						{
						case 0x00:	// 16bit Compression pointer follows
							k += 2;	// 
							break;
						case 0x01:	// Bit String label
							if (k >= aOffsetLimit)
								return -1;		// Corrupted!
							tag = i[k++];		// Number of significant bits
							if (tag == 0)
								tag = 256;
							k += ((tag + 7) / 8);
							break;
						default:
							return -1;	// Unsupported
						}
					break;
				}
			}
done:
		return k;
		}
	TInt Query(const TInt aOffset, const TInt aOffsetLimit, TInt &aQType, TInt &aQClass) const
	/**
	Extract and skip over single query in Question section.
	
	A query in question section is a domain-name
	followed by query type and class fields.
	
	@param aOffset	start offset of the query (domain name)
	@param aOffsetLimit	maximum value for the offset
	
	@return
	@li	< 0, unsupported format, the message is invalid or corrupt
	@li	> 0, offset of the next query or section
	
	<b>Note</b>: For a valid return, it should always be that
	returned value > aOffset!
	*/
		{
		//
		// Skip over the QNAME
		//
		TInt k = NameSkip(aOffset, aOffsetLimit);
		// ..ignore the fact that any return less than
		// the original aOffset is actually an error.. -- msa
		//
		if (k > 0)
			{
			if (k + 4 > aOffsetLimit)
				return -1;
			aQType = (i[k] << 8) + i[k+1];
			aQClass = (i[k+2] << 8) + i[k+3];
			k += 4;
			}
		return k;
		}
	TInt Resource(const TInt aOffset, const TInt aOffsetLimit, TUint16 &aType, TUint16 &aClass, TUint32 &aTTL, TUint &aRDataOffset, TUint &aRDataLength)
	/**
	Extract basic resource record information.
	
@verbatim
                                    1  1  1  1  1  1
      0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                                               |
    /                                               /
    /                      NAME                     /
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TYPE                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     CLASS                     |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                      TTL                      |
    |                                               |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   RDLENGTH                    |
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--|
    /                     RDATA                     /
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
@endverbatim

	@param aOffset	of the recource record (points to NAME)
	@param aOffsetLimit	maximum value for the offset
	@retval aType	RR type
	@retval aClass	RR class
	@retval aTTL		RR TTL (time to live)
	@retval aRDataOffset	offset to the beginning of the RDATA
	@retval aRDataLength	the length of the RDATA
	
	@return	offset pointing after the RR.
	*/
		{
		TInt k = NameSkip(aOffset, aOffsetLimit);
		if (k < 0)
			return k;
		if (k + 10 > aOffsetLimit)
			return -1; // no room for fixed part of the RR
		aType = (TUint16)((i[k] << 8) + i[k+1]);
		k += 2;
		aClass = (TUint16)((i[k] << 8) + i[k+1]);
		k += 2;
		aTTL = (i[k] << 24) +
				(i[k+1] << 16) +
				(i[k+2] << 8) +
				i[k+3];
		k += 4;

		aRDataLength = (i[k] << 8) + i[k+1];
		k += 2;
		aRDataOffset = k;
		k += aRDataLength;
		return k > aOffsetLimit ? -1 : k;
		}
	};
#endif
