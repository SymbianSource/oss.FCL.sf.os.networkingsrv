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
// message.cpp - name resolver DNS message interpreter
//

#include <dns_qry.h>
#include "res_sock.h"
#include "message.h"
#include <networking/dnd_err.h>
#include "dns_ext.h"

// Constants

_LIT8(KIPv4AddrToHost, ".in-addr.arpa");
#ifdef DNS_REVERSE_IP6_INT
_LIT8(KIPv6AddrToHost, ".ip6.int");
#else
_LIT8(KIPv6AddrToHost, ".ip6.arpa");
#endif

#ifdef SYMBIAN_DNS_PUNYCODE 
/* This macro is to check whether the character passed is of UTF encoded */
#define ISUTF16(x) ((x>= 0xD800 && x<= 0xDFFF)? ETrue : EFalse)
#endif //SYMBIAN_DNS_PUNYCODE

//
// DnsCompareNames
// ***************
// A global utility function
//
/**
// The DNS domain names have a special case folding rules. The names
// are BINARY data (can include anything in range [0..255], but when
// comparing two names, ASCII lower and and upper case letters should
// compare equal, e.g  A-Z == a-z.
//
// @param	aName1	to be compared
// @param	aName2	to be compared
// @returns	TRUE, if names are equal under DNS case folding rules
*/
TBool DnsCompareNames(const TDesC8 &aName1, const TDesC8 &aName2)
	{
	static const TUint8 KAscii_A = 0x41;
	static const TUint8 KAscii_Z = 0x5a;
	static const TUint8 KAscii_lower = 0x20;

	TInt n = aName1.Length();
	if (n != aName2.Length())
		return 0;	// not same, lengths differ
	//
	// Need to do the special "DNS case insensitive" compare
	//
	const TUint8 *const p = aName1.Ptr();
	const TUint8 *const q = aName2.Ptr();
	while (--n >= 0)
		{
		TUint8 x = p[n];
		TUint8 y = q[n];
		if (x != y)
			{
			if (x >= KAscii_A && x <= KAscii_Z)
				x += KAscii_lower;
			if (y >= KAscii_A && y <= KAscii_Z)
				y += KAscii_lower;
			if (x != y)
				return 0;
			}
		}
	return 1;
	}


//
// Initialize TDndHeader
//
void TDndHeader::Init(const TUint16 aID, const TUint8 aOpcode)
	{
	// Set ID to aID
	i[0] = (TUint8)(aID / 0x0100);
	i[1] = (TUint8)(aID % 0x0100);

	// Set QR = 0; Opcode = aOpcode; AA = 0; TC = 0; RD = 1
	i[2] = 1;
	i[2] |= (aOpcode << 3);

	// Set RA = 0; Z = 0; RCode = 0
	i[3] = 0;

	// Set QDCOUNT = 1
	i[4] = 0;
	i[5] = 1;
	
	// Set all other counts as zero.
	// ANCOUNT = 0;
	// NSCOUNT = 0;
	// ARCOUNT = 0;
	i[6] = 0;
	i[7] = 0;
	i[8] = 0;
	i[9] = 0;
	i[10] = 0;
	i[11] = 0;
	}

// to set the ID field of header
void TDndHeader::SetId(const TUint16 aID)
	{
	i[0] = (TUint8)(aID / 0x0100);
	i[1] = (TUint8)(aID % 0x0100);
	}

#ifdef LLMNR_ENABLED
// TDndHeader::SetQR() - To set the query/response bit of the header
void TDndHeader::SetQR(const TUint aQR)
    {
    i[2] |= ((aQR & 0x1) << 7);
    }
#endif

// TDndHeader::SetOpcode() - To set the opcode field of the header
void TDndHeader::SetOpcode(const TUint8 aOpcode)
	{
	i[2] |= ((aOpcode & 0xF) << 3);
	}

#ifdef LLMNR_ENABLED
// TDndHeader::SetAA() - To set the authoritative answer bit of the header
void TDndHeader::SetAA(const TUint aAA)
    {
    i[2] |= ((aAA & 0x1) << 2);
    }

// TDndHeader::SetRCode() - To set the value of RCode field of the header
void TDndHeader::SetRCode(const TUint aRCode)
    {
    i[3] |= aRCode & 0xF;
    }
#endif

// TDndHeader::SetQdCount() - To set the QdCount field
void TDndHeader::SetQdCount(const TUint16 aQdCount)
	{
	i[4] = (TUint8)(aQdCount / 0x0100);
	i[5] = (TUint8)(aQdCount % 0x0100);
	}


// TDndHeader::SetAnCount() - To set the answer count field
void TDndHeader::SetAnCount(const TUint16 aAnCount)
	{
	i[6] = (TUint8)(aAnCount / 0x0100);
	i[7] = (TUint8)(aAnCount % 0x0100);
	}

void TDndHeader::SetArCount(const TUint16 aArCount)
	{
	i[10] = (TUint8)(aArCount / 0x0100);
	i[11] = (TUint8)(aArCount % 0x0100);
	}
#ifdef SYMBIAN_DNS_PROXY
void TDndHeader::SetNsCount(const TUint16 aNsCount)
	{
	i[8] = (TUint8)(aNsCount / 0x0100);
	i[9] = (TUint8)(aNsCount % 0x0100);
	}
#endif


// TMsgBuf::VerifyMessage
// **********************
/**
// Verify basic header fields in the responce.
//
// @retval aRCode The RCode from the message (if return > 0)
// @retval aQuestion The question from the message (if return > 0)
//
// @return
//	@li < 0, if there are any format errors with the message
//	@li > 0, offset to the first answer RR after the question section
*/
TInt TMsgBuf::VerifyMessage(TInt &aRCode, TDndQuestion &aQuestion) const
	{
	// Check that message is actually long enough for the header
	// Return length of the fixed header (= offset pointing to next part)
	ASSERT(sizeof(TDndHeader) == KDnsMinHeader);
	if (Length() < TInt(sizeof(TDndHeader)))
		return KErrDndDiscard;

	const TDndHeader &hdr = Header();
	// ..do some simple insanity tests, and discard if odd things
	// detected.
	if (hdr.ANCOUNT() < 0)
		return KErrDndDiscard;
	if (hdr.NSCOUNT() < 0)
		return KErrDndDiscard;
	if (hdr.ARCOUNT() < 0)
		return KErrDndDiscard;

	TInt offset = aQuestion.Create(*this, sizeof(TDndHeader));
	if (offset > 0)
		{
		// Question extracted, now we can look for EDNS0 OPT record
		// to find the real RCODE...
		TDndRR opt_rr(*this);
		const TInt start = hdr.ANCOUNT() + hdr.NSCOUNT();
		if (opt_rr.LocateRR(offset, start + hdr.ARCOUNT(), EDnsQType_OPT, EDnsQClass_ANY, start) > offset)
			{
			// OPT RR has been located
			// ..return extend RCODE.
			aRCode = ((opt_rr.iTTL >> 20) & 0xFF0) | hdr.RCODE();
			}
		else
			{
			// NO OPT RR present, RCODE is as it
			aRCode = hdr.RCODE();
			}
		}
	return offset;
	}


// TMsgBuf::GetNextName
// ********************
/**
// @param	aOffset	from the start of the message to the domain name
// @retval	aName	receives the extracted domain name (in raw DNS 8bit encoding)
// @param	aDepth	("internal" paramater) recursion count to catch looped compression
// @returns
//	@li > 0, updated offset pointing past the name portion
//	@li	< 0, an error was detected
*/
TInt TMsgBuf::GetNextName(TInt aOffset, TDes8 &aName, const TUint aDepth) const
	{
	TInt tag;
	TInt err;

	if (aDepth >= KDndMaxLoopCount ||	// Prevent infinite recursion...
		aOffset >= Length() ||			// ...offsets beyond the buffer end,
		aOffset < 0)					// ...or before buffer start (someone didn't check error return!)
		return KErrDndDiscard;

	const TUint8 *p = Ptr() + aOffset;

	while ((tag = *p) != 0)
		{
		if ((tag & 0xC0) == 0xC0)
			{
			if (Length() < aOffset + 2)
				return KErrDndDiscard;

			const TInt tempOffset = (tag - 0xC0) * 256 + *(p+1);
			err = GetNextName(tempOffset, aName, aDepth+1);
			if (err < KErrNone)
				return err;

			p += 2;
			aOffset += 2;
			break;
			}
		else
			{
			p++;
			aOffset++;
			if (aName.Length() + tag >= aName.MaxLength())
				return KErrDndNameTooBig;
			if (Length() < aOffset + tag)
				return KErrDndUnknown;

			aName.Append(TPtrC8(p, tag));
			aName.Append('.');
			p += tag;
			aOffset += tag;
			}
		}

	if (tag == 0)
		{
		p++;
		aOffset++;
		// if there is a 'dot' at the end, get rid of it
		if (aName.Length() != 0)
			aName.Delete(aName.Length() - 1, 1);
		}
	return aOffset;
	}


// TMsgBuf::GetNextString
// **********************
/**
// @param	aOffset	from the start of the message to the character string
// @retval	aString	receives the extracted string (in raw DNS 8bit encoding). This
//			is only a reference to the original DNS message buffer, which must be
//			kept around while this is used.
// @returns
//	@li	> 0, updated offset pointing past extracted string
//	@li	< 0, an error was detected
*/
TInt TMsgBuf::GetNextString(TInt aOffset, TPtrC8 &aString) const
	{
	if (aOffset >= Length())
		return KErrDndUnknown;	// corrupt data

	const TUint8 *p = Ptr() + aOffset;
	//
	// Extract <character-string> length
	//
	TInt length = *p++;
	aOffset += length + 1;
	if (aOffset > Length())
		return KErrDndUnknown;	// corrupt data

	aString.Set(p, length);
	return aOffset;
	}

// TMsgBuf::GetName
// ****************
/**
// @param	aOffset	from the start of the message to the start of domain name
#ifdef SYMBIAN_DNS_PUNYCODE
// @param   aIdnEnabled   
#endif
// @retval	aName	the extracted domain name (system dependent encoding)
// @returns
//	@li	> 0, offset updated pointing past the name portion
//	@li	< 0, an error was detected
*/
#ifdef SYMBIAN_DNS_PUNYCODE
TInt TMsgBuf::GetName(TInt aOffset, TDes &aName,TBool aIdnEnabled) const
#else
TInt TMsgBuf::GetName(TInt aOffset, TDes &aName) const
#endif //SYMBIAN_DNS_PUNYCODE
	{
	// Use TDndName as temporary buffer. This seemingly
	// unnecessary juggling is present because of the potentially
	// hairy/complex issues of DNS <-> UNICODE translation. The idea
	// is to have one clean location which implements it:
	// TDndName::SetName and TDndName::GetName methods.
	TDndName tmp;
#ifdef SYMBIAN_DNS_PUNYCODE
	tmp.EnableIdn(aIdnEnabled);
#endif
	TInt i = tmp.SetName(*this, aOffset);
	if (i < 0)
		return i;
	TInt err = tmp.GetName(aName);
	// This function must return offset when no errors
	return err == KErrNone ? i : err;
	}

// TMsgBuf::AppendName
// *******************
/**
// @param	aName
//		domain name to be appended
// @param	aCompressed
//	@li	= 0,
//		the name is assumed to be complete name will be terminated with "root label"
//	@li	> 0,
//		the name is assumed to be a prefix to another name already stored into the
//		message starting at indicated offset.
//		The name (prefix) is terminated with a "compression ptr".
//		The value must be in range [1..Length()-1].
//
// @returns
//	@li	KErrNone
//		if name appended succesfully
//	@li	KErrDndNameTooBig,
//		if name does not fit the message buffer
//	@li	KErrBadName,
//		if name is invalid, e.g. has empty or too long (> 63)
//		components
//	@li	KErrDndUnknown
//		if aCompressed is illegal, e.g. not in range [0..Length()-1]
*/
TInt TMsgBuf::AppendName(const TDesC8 &aName, const TUint aCompressed)
	{
	if (aCompressed >= (TUint)Length())
		return KErrDndUnknown; // Don't allow forward references in compression pointers.
	// The space requirement
	// + length of the iName
	// and
	//    + 1 (for the terminator NUL byte, root)
	//  or
	//    + 2 (for the compression pointer)
	//	
	// [note: if the name already has a trailing '.', the code below
	// will handle it, but the spacerequirement computation is too
	// large by one byte -- a minor discrepancy, which is
	// not worth coding -- msa]
	if (Length() + aName.Length() + 1 + (aCompressed > 0) > MaxLength())
		return KErrDndNameTooBig;

	//
	// Append <domain-name> as labels
	//
	TInt i;
	TPtrC8 name(aName);
	while ((i = name.Locate('.')) != KErrNotFound)
		{
		// a valid label contains at least one and at most 63 characters
		if (i < 1 || i > 63)
			return KErrDndBadName;	// ..abort message build!
		Append((TChar)i);
		Append(name.Left(i));
		name.Set(name.Mid(i+1));		// Skip the dot.
		}
	i = name.Length();
	if (i > 0)
		{
		// append the last component...
		if (i > 63)
			return KErrDndBadName;
		Append((TChar)i);
		Append(name);
		}
	//
	// Append terminator
	//
	if (!aCompressed)
		Append((TChar)0);		//Root
	else
		{
		Append((TChar)((aCompressed / 0x100) | 0xC0));
		Append((TChar)(aCompressed % 0x100));
		}
	return KErrNone;
	}

// TMsgBuf::GetRR
// **************
/**
// @param aOffset	of the recource record (points to NAME)
// @retval aType	RR type
// @retval aClass	RR class
// @retval aTTL		RR TTL (time to live)
// @retval aRD		offset to the beginning of the RDATA
// @retval aRDLength	the length of the RDATA
//	
// @return	offset pointing after the RR.
*/
TInt TMsgBuf::GetRR(const TInt aOffset, TUint16 &aType, TUint16 &aClass, TUint32 &aTTL, TUint &aRd, TUint &aRdLength) const
	{
	return Header().Resource(aOffset, Length(), aType, aClass, aTTL, aRd, aRdLength);
	}

TInt TMsgBuf::SkipName(TInt aOffset) const
	{
	return Header().NameSkip(aOffset, Length());
	}


// TDndName::SetName
// *****************
// Set PTR name from address
//
/**
// Translate the address into a query name for the PTR query.
// For example 172.21.33.80 -> 80.33.21.172.in-addr.arpa.
//
// @param	aAddr	the address to be translated into PTR name
// @returns
//	@li		KErrnone, if translation succeeded
//	@li		KErrNotSupported, if translation not supported
//			(bad address or unknown address family)
*/
TInt TDndName::SetName(const TInetAddr &aAddr)
	{
 
	if (aAddr.Family() == KAfInet ||
		aAddr.IsV4Mapped() ||	// V4 Mapped is internal, and always means IPv4
		aAddr.IsV4Compat())		// Treating V4 Compatible as IPv4 might be questionable?
		{
		TUint32 address = aAddr.Address();
		if (address == KInetAddrNone)
			return KErrNotSupported;
	
		Format(_L8("%d.%d.%d.%d"),
			(TUint8)(address),
			(TUint8)(address >> 8),
			(TUint8)(address >> 16),
			(TUint8)(address >> 24));

		Append(KIPv4AddrToHost);
		return KErrNone;
		}

	else if (aAddr.Family() == KAfInet6)	// for IPv6
		{
		SetLength(0);

		const TIp6Addr &address = aAddr.Ip6Address();
		// ..are there any addresses that should never be
		// PTR queried? unspecified? loopback? anything else?
		// Or, should even these tests removed and let the
		// query just fail normally? -- msa
		if (address.IsUnspecified() ||
			address.IsLoopback())
			return KErrNotSupported;

		for (TInt i = 15; i >= 0; i--)
			{
			for (TInt j = 0; j < 2; j++)
				{
				TChar c;
				TUint8 digit = (TUint8)((address.u.iAddr8[i] >> (4 * j)) & 0x0f);
				if (digit < 10)
					c = (TChar)('0' + digit);
				else
					c = (TChar)('a' + digit - 10);
				Append(c);
				if (i != 0 || j != 1)
					Append('.');
				}
			}
		Append(KIPv6AddrToHost);
		return KErrNone;
		}
	return KErrNotSupported;
	}


//
// TDndName::SetName
// *****************
/**
// Uncompres the <domain-name> from a DNS message into name
//
// @param	aBuf	buffer containing the DNS message packet
// @param	aOffset	from the start of the message to the name
// @returns
//	@li		> 0, updated offset past the extracted name, if succesful
//	@li		< 0, if some error
*/
TInt TDndName::SetName(const TMsgBuf &aBuf, const TInt aOffset)
	{
	Zero();
	return aBuf.GetNextName(aOffset, *this);
	}
		

// TDndName::SetName
// *****************
// Set name from THostName
//
/**
// Translate system encoding into query name. In Unicode this
// means converting the Unicode to equivalent DNS name. For
// narrow builds, this is usually a straight copy.
//
// @param	aName	the name to be translated into DNS format
// @return
//	@li		KErrNone, if translation succeeded
//	@li		KErrNotSupported, if translation failed
*/
TInt TDndName::SetName(const THostName &aName)
	{
#ifdef SYMBIAN_DNS_PUNYCODE
	/* The UTF-16 encoded domain names are not supported irrespective
	 * of whether the IDN support is enable or not.
	 */ 
	 
	TBool isNameIDN= EFalse;
	TUint8 nameLen = aName.Length();
	for (TUint8 idxCounter = 0;  idxCounter < nameLen;  idxCounter++)
		{
		const TUint16 currVal = aName[idxCounter];
		if (currVal >= 0x80 )
			{
			if(ISUTF16(currVal))
				{
				return KErrDndBadName; // UTF-16 encoding is currently not supported.
				}
			isNameIDN = ETrue;
			}
		}
	if(isNameIDN && iIdnEnabled)
		{
		TInt err = iPunyCodeName.IdnToPunycode(aName);
		// Set this to offset any previous setting (if any)
		iPunyCodeConverted = EFalse; 
		if (err == KErrNone)
			{
			Copy(iPunyCodeName);
			iPunyCodeConverted = ETrue;
			}
		else
			return KErrDndBadName; // returning error that PunyCode conversion failed.
		}
	else
		{
#endif
		Copy(aName);
#ifdef SYMBIAN_DNS_PUNYCODE
		}
#endif //SYMBIAN_DNS_PUNYCODE
	//

	// Eliminate single trailing '.', if present

	const TInt index = Length() - 1;
	if (index >= 0 && (*this)[index] == '.')
		SetLength(index);

	return KErrNone;
	}

// TDndName::GetName
// *****************
// Get name into buffer
/**
// This is reverse of the SetName and translates DNS name
// into system specific encoding.
//
// This function behaves like a "copy", if aStart = 0 (the default),
// or "append", if aStart = aName.Length().
//
// @retval	aName
//		receives the translated name. The length of the
//		buffer is determined by the end of the translated
//		name.
// @param	aStart	a start offset for the name
// @return
//	@li		KErrNone, if translation succeeded
//	@li		KErrDndNameTooBig, name does not fit the buffer
*/
TInt TDndName::GetName(TDes &aBuf, const TInt aStart) const
	{

#ifdef SYMBIAN_DNS_PUNYCODE

	TPunyCodeDndName tempPunycodeName(iPunyCodeName);
	TInt checkPrefix = tempPunycodeName.Find(KAcePrefix());

	if( checkPrefix  != KErrNotFound && iPunyCodeConverted && iIdnEnabled)
		{
		TInt err = tempPunycodeName.PunycodeToIdn(aBuf,aStart);
		if (err == KErrNone)
			{
			const TInt punycodeLength = tempPunycodeName.Length();
			if (aStart + punycodeLength > aBuf.MaxLength())
				{
				return KErrDndNameTooBig;
				}

			aBuf.SetLength(aStart + punycodeLength);

			TUint8 nameLen = aBuf.Length();
			
			for (TUint8 idxCounter = aStart;  idxCounter < nameLen;  idxCounter++)
				{
				const TUint16 currVal = aBuf[idxCounter];
				if (currVal >= 0x80 && ISUTF16(currVal))
					{
					// UTF-16 encoding is not supported. 
					// so copying the punycode as it is.
					for (TInt i = 0; i < punycodeLength; ++i)
						{
						aBuf[aStart+i] = (TText)((tempPunycodeName)[i]);
						}
					return err;
					}
				}

			return err;
			}
		else
			{
			return KErrDndBadName; // returning error that PunyCode conversion failed.
			}
		}
		else
			{
#endif //SYMBIAN_DNS_PUNYCODE	
	const TInt N = Length();
	if (aStart + N > aBuf.MaxLength())
		return KErrDndNameTooBig;

	aBuf.SetLength(aStart + N);
	for (TInt i = 0; i < N; ++i)
		aBuf[aStart+i] = (TText)((*this)[i]);
	return KErrNone;
#ifdef SYMBIAN_DNS_PUNYCODE	
			}
#endif //SYMBIAN_DNS_PUNYCODE

}

// TDndName::GetAddress
// ********************
/**
// This is reverse of SetName with TInetAddr and translates
// the content of *.ip6.arpa or *.in-addr.arpa into TInetAddr,
// if the address is complete.
//
// @retval aAddr The address, if conversion is possible
// @return TRUE, if conversion was possible, and FALSE otherwise.
*/
TBool TDndName::GetAddress(TInetAddr &aAddr) const
	{
	TInt k;
	TLex8 decode(*this);

	// Initialize to V4 mapped format, so that it is ready
	// to be "patched" with IPv4 address. If the address
	// is IPv6, all of the address will be overwritten.
	TIp6Addr addr = {{{0,0,0,0,0,0,0,0,0,0,0xff,0xff,0,0,0,0}}};

	if ((k = Find(KIPv4AddrToHost)) >= 0)
		{
		if (k + KIPv4AddrToHost().Length() != Length())
			return FALSE;
		for (TInt i = sizeof(addr); --i >= (TInt)(sizeof(addr) - 4); )
			{
			// Note: the check is loose on extra leading zeroes
			// (which should not be present, but are accepted
			// here).
			TUint32 octet;
			if (decode.Val(octet, EDecimal, 255) != KErrNone)
				return FALSE;	// Too large value
			if (decode.Get() != '.')
				return FALSE;
			addr.u.iAddr8[i] = (TUint8)octet;
			}
		}
	else if ((k = Find(KIPv6AddrToHost)) >= 0)
		{
		if (k + KIPv6AddrToHost().Length() != Length())
			return FALSE;
		TInt j = sizeof(addr);
		for (TUint i = 0; i < sizeof(addr)*2; ++i)
			{
			// Note: the check is loose on extra leading zeroes
			// (which should not be present, but are accepted
			// here).
			TUint32 nibble;
			if (decode.Val(nibble, EHex, 0xF) != KErrNone)
				return FALSE;
			if (decode.Get() != '.')
				return FALSE;
			if (i & 1)
				{
				// high nibble (always after low nibble, just OR high bits in)
				addr.u.iAddr8[j] |= (TUint8)(nibble << 4);
				}
			else
				{
				ASSERT(j > 0);
				// low nibble (this will happen first)
				j -= 1;
				addr.u.iAddr8[j] = (TUint8)nibble;
				}
			}
		}
	// The decoding must eat everything upto in-addr.arpa or ip6.arpa!
	if (decode.Offset() != k+1)
		return FALSE;
	aAddr.SetAddress(addr);
	return TRUE;
	}


// Creating TDndQuestion from the question section of the UDP message. The question
// section starts at position 'aIndex' of the buffer 'aBuf'. The function returns the
// offset pointing over the question section on return (or error).
//
// Returns
//	> 0, the offset pointing over the question section
//	< 0, an error is detected
//
TInt TDndQuestion::Create(const TMsgBuf &aBuf, TInt aIndex)
	{
	Zero();
	TInt i = aBuf.GetNextName(aIndex, *this);
	if (i > 0)
		{
		if (aBuf.Length() < i + 4)
			return KErrDndDiscard;

		iQType = EDnsQType((aBuf[i] << 8) | aBuf[i+1]);
		iQClass = EDnsQClass((aBuf[i+2] << 8) | aBuf[i+3]);
		i += 4;
		}
	return i;
	}


// TDndQuestion::Append(aMsgBuf, aFlags)
// *************************************
// Append a Question section to aMsgBuf 
/**
//
// @retval	aMsgBuf	the DNS message to be modified
// @param	aCompressed
//		value of is used to convey whether the name contains a full name or only
//		a prefix part.
//	@li	= 0,
//		the name is full name
//	@li > 0,
//		the name is only a prefix to the name already stored in to the
//		message start at this offset.
// @returns
//	@li	KErrNone
//		if question section appended succesfully
//	@li	KErrDndNameTooBig,
//		if question does not fit the message buffer
//	@li	KErrBadName,
//		if name is invalid, e.g. has empty or too long (> 63)
//		components
//	@li	KErrDndUnknown
//		if aFlags is illegal, not in [0..Length()-1]
*/
TInt TDndQuestion::Append(TMsgBuf& aMsgBuf, TUint aCompressed) const
	{
	const TInt err = aMsgBuf.AppendName(*this, aCompressed);
	if (err != KErrNone)
		return err;

	if (aMsgBuf.MaxLength() < aMsgBuf.Length() + 4)
		return KErrDndNameTooBig;

	// Append the QType
	aMsgBuf.Append((TChar)(iQType / 0x100));
	aMsgBuf.Append((TChar)(iQType % 0x100));

	// Append the QClass
	aMsgBuf.Append((TChar)(iQClass / 0x100));
	aMsgBuf.Append((TChar)(iQClass % 0x100));
	return KErrNone;
	}


// TDndQuestion::CheckQuestion
// ***************************
/**
// Compares the name, query type and query class.
//
// @param aQuestion	to be compared with
// @returns
//	@li	KErrNone, if questions are equal
//	@li	KErrDndDiscard, if questions are not equal
*/
TInt TDndQuestion::CheckQuestion(const TDndQuestion &aQuestion) const
	{
	return
		(
		aQuestion.iQClass == iQClass &&
		aQuestion.iQType == iQType &&
		DnsCompareNames(*this, aQuestion)) ? KErrNone : KErrDndDiscard;
	}


// TDndRR::GetSockAddr
// *******************
/**
// @retval	aName contains the resulting name, if return is OK
// @param	aOffset	to the start of name, relative to the RDATA
// @returns	offset pointing to next octet after the extracted value (this
//			offset is relative to the DNS message start). Negative returns
//			are error returns.
*/
TInt TDndRR::GetSockAddr(TInetAddr& aSockAddr) const
	{
	if (iType == EDnsType_A && iClass == EDnsClass_IN && iRdLength == 4)
		{
		const TUint32 KInetAddr = INET_ADDR(iBuf[iRd+0], iBuf[iRd+1], iBuf[iRd+2], iBuf[iRd+3]);
		aSockAddr.SetAddress(KInetAddr);
		return iRd + 4;
		}

	else if(iType == EDnsType_AAAA && iClass == EDnsClass_IN)
		{
		if (iRdLength < sizeof(TIp6Addr))
			return KErrDndUnknown;
		// TIp6Addr byteorder is network order, so can just copy
		// bytes as is from the resource data... but, as RR is
		// not necessarily aligned properly, must use a temporary
		// address buffer for the transfer..
		TIp6Addr addr;
		Mem::Copy(addr.u.iAddr8, &iBuf[iRd], sizeof(addr.u.iAddr8));
		aSockAddr.SetAddress(addr);
		return iRd + sizeof(addr.u.iAddr8);
		}
	return KErrDndUnknown;
	}


// TDndRR::GetNameFromRDATA
// ************************
/**
//
// @retval	aName contains the extracted domain name.
// @param	aOffset	to the start of the name, relative to the RDATA
// @returns	offset pointing to next octet after the extracted value (this
//			offset is relative to the DNS message start). Negative returns
//			are error returns.
*/
TInt TDndRR::GetNameFromRDATA(TDes8 &aName, TUint aOffset) const
	{
//	return aName.SetName(iBuf, iRd + aOffset);
	aName.Zero();
	return iBuf.GetNextName(iRd + aOffset, aName);
	}

// TDndRR::GetStringFromRDATA
// **************************
/**
// @retval	aString
//		is set to point to the string content in the message buffer.
//		No copying occurs, the message buffer must exist when this
//		return value is used!
// @param	aOffset	to the start of the character string, relative to the RDATA
// @returns	offset pointing to next octet after the extracted value (this
//			offset is relative to the DNS message start). Negative returns
//			are error returns.
*/
TInt TDndRR::GetStringFromRDATA(TPtrC8 &aString, TUint aOffset) const
	{
	return iBuf.GetNextString(iRd + aOffset, aString);
	}

// Return TUint16 from a buffer
static TUint16 Get16(const TUint8 *p)
	{
	return (TUint16)(p[0] << 8 | p[1]);
	}
// Return TUint32 from a buffer
static TUint32 Get32(const TUint8 *p)
	{
	return (TUint32)(p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3]);
	}

// TDndRR::GetResponse
// *******************
// Extract content of RR
//
/**
// What exactly is returned, depends on the type of the RR (A, AAAA, CNAME and
// PTR return "normal" results, for others some special values may returned,
// see TInetDnsRR).
//
// @retval	aName	normally the domain name (system encoding)
// @retval	aAddr	normally the address (but, also see TInetDnsRR)
// @returns	KErrNone if extraction succeeded, and error otherwise.
*/
TInt TDndRR::GetResponse(TDes &aName, TInetAddr &aAddr) const
	{
	TInt err = iName;	// (just to inialize, to offset of this RR record)
	TDndName name;
#ifdef SYMBIAN_DNS_PUNYCODE
		name.EnableIdn(iIdnEnabled);
#endif //SYMBIAN_DNS_PUNYCODE

	if ((iType == EDnsType_A || iType == EDnsType_AAAA) && iClass == EDnsClass_IN)
		{
		err = name.SetName(iBuf, iName);
		if (err < 0)
			return err;
		err = GetSockAddr(aAddr);
		}
	else  if (iType == EDnsType_PTR && iClass == EDnsClass_IN)
		{
		//
		// For the PTR record, it is assumed the aAddr is already
		// initialized to the original queried address (do not
		// try to recompute address from the name part of the
		// PTR record -- that is *NOT* always the address due
		// to some CNAME games for the PTR records... -- msa)
		//
		// ..and get the real name from RData.
		err = GetNameFromRDATA(name);
		}
	else if (iType == EDnsType_CNAME && iClass == EDnsClass_IN)
		{
		aAddr.SetAddress(0);
		// ..and get the cname from RDATA
		err = GetNameFromRDATA(name);
		}
	else
		{
		//
		// Special RR values
		//
		TInetDnsRR::Cast(aAddr) = TInetDnsRR();
		SDnsRR &rr = TInetDnsRR::Cast(aAddr).RR();
		rr.iClass = (TUint16)iClass;
		rr.iType = (TUint16)iType;
		if (iClass == EDnsClass_IN)
			{
			const TUint8 *const rd = iBuf.Ptr() + iRd;
			//
			// Only a select subset of RR's are supported
			//
			if (iType == EDnsType_SRV && iRdLength > 4)
				{
				rr.iSRV.iPriority = Get16(rd);
				rr.iSRV.iWeight = Get16(rd + 2);
                aAddr.SetPort(Get16(rd + 4));
				err = GetNameFromRDATA(name, 6);
				}
			else if (iType == EDnsType_NAPTR && iRdLength > 7)
				{
				rr.iNAPTR.iOrder = Get16(rd);
				rr.iNAPTR.iPreference = Get16(rd + 2);
				// get FLAGS
				TPtrC8 flags;
				err = GetStringFromRDATA(flags, 4);
				if (err < 0)
					return err;
				TInt flagLength = sizeof(rr.iNAPTR.iFlags);
				//The buffer size of TInetAddr(0x20 bytes) may not be sufficient to store the 
				//realtime length(upto 0xFF) of FLAGS field, so it is limited to 12 bytes. Hence if the 
				//flag length is more than 12 bytes, error KErrDndNameTooBig is raised.
				if(flagLength < (err - iRd - 5))
					return KErrDndNameTooBig;
				TPtr8 rr_flags(rr.iNAPTR.iFlags, flagLength , flagLength);
				rr_flags.FillZ();
				rr_flags = flags;

				// get SERVICES
				TPtrC8 services;
				err = GetStringFromRDATA(services, err - iRd);
				if (err < 0)
					return err;
				// get REGEXP
				TPtrC8 regexp;
				err = GetStringFromRDATA(regexp, err - iRd);
				if (err < 0)
					return err;
				// Fetch REPLACEMENT
				err = GetNameFromRDATA(name, err - iRd);
				if (err < 0)
					return err;

				// Try to append SERVICES and REGEXP into name
				TInt length = name.Length() + services.Length() + regexp.Length();
				if (length > name.MaxLength())
					return KErrDndNameTooBig;
				// ***** WARNING ********
				// The iL1 and iL2 will be problematic if the future
				// UNICODE conversion of the name is not 1 to 1!
				// Should use separators instead? -- msa
				rr.iNAPTR.iL1 = (TUint8)name.Length();
				rr.iNAPTR.iL2 = (TUint8)services.Length();
				name.Append(services);
				name.Append(regexp);
				}
			else if (iType == EDnsType_MX && iRdLength > 2)
				{
				rr.iMX.iPreference = Get16(rd);
				err = GetNameFromRDATA(name, 2);
				}
			else if (iType == EDnsType_NS)
				{
				err = GetNameFromRDATA(name);
				}
			else if (iType == EDnsType_SOA)
				{
				err = GetNameFromRDATA(name);
				if (err < 0)
					return err;
				if (name.Length() == name.MaxLength())
					return KErrDndNameTooBig;
				name.Append('@');
				err = iBuf.GetNextName(err, name);// *APPEND* RNAME to hostname!
				if (err < 0)
					return err;

				err -= iRd;
				if (err < 0 || err + 20 > (TInt)iRdLength)
					return KErrDndUnknown;
				rr.iSOA.iSerial = Get32(rd+err);
				rr.iSOA.iRefresh = Get32(rd+err+4);
				rr.iSOA.iRetry = Get32(rd+err+8);
				rr.iSOA.iExpire = Get32(rd+err+12);
				rr.iSOA.iMinimum = Get32(rd+err+16);
				}
			}
		}
	return err > 0 ? name.GetName(aName) : err;
	}

#ifndef NO_DNS_QUERY_SUPPORT
// TDndRR::GetResponse
// *******************
// Extract content of RR
//
/**
// What exactly is returned, depends on the type of the RR
//
// @retval	aAnswer	the "payload" will receive the RR content in TDnsQryRespBase format
// @retval	aAddr a pointer to "TInetAddr *" within the reply (only for A and AAAA, NULL otherwise)
// @returns	offset pointing to next octet after the extracted value (this
//			offset is relative to the DNS message start). Negative returns
//			are error returns.
*/
TInt TDndRR::GetResponse(TDnsMessageBuf &aAnswer, TInetAddr **aAddr) const
	{
	TInt err = KErrNotSupported;
	const TInt max_len = aAnswer.MaxLength() - aAnswer().HeaderSize();
	TInt len = 0;
	*aAddr = NULL;

	const TUint8 *const rd = iBuf.Ptr() + iRd;

	switch ((TUint16)iType)
		{
		case KDnsRRTypeA:
			{
			TDndRespA &a = aAnswer().A();
			len = sizeof(a);
			if (len > max_len)
				return KErrOverflow;
			(void)new (&a) TDndRespA();
			err = GetSockAddr(a.HostAddress());
			*aAddr = &a.HostAddress();
			}
			break;
		case KDnsRRTypeAAAA:
			{
			TDndRespAAAA &aaaa = aAnswer().AAAA();
			len = sizeof(aaaa);
			if (len > max_len)
				return KErrOverflow;
			(void)new (&aaaa) TDndRespAAAA();
			err = GetSockAddr(aaaa.HostAddress());
			*aAddr = &aaaa.HostAddress();
			}
			break;
#if 0
		case KDnsRRTypeNS:
			{
			TDndRespNS &ns = aAnswer().NS();
			len = sizeof(ns);
			if (len > max_len)
				return KErrOverflow;
			(void)new (&ns) TDndRespNS();
			err = GetNameFromRDATA(ns.HostName());
			}
			break;
		case KDnsRRTypeCNAME:
			break;
		case KDnsRRTypeWKS:
			break;
#endif
		case KDnsRRTypePTR:
			{
			TDndRespPTR &ptr = aAnswer().PTR();
			len = sizeof(ptr);
			if (len > max_len)
				return KErrOverflow;
			(void)new (&ptr) TDndRespPTR();
			err = GetNameFromRDATA(ptr.HostName());
			}
			break;
#if 0
		case KDnsRRTypeHINFO:
			break;
#endif
		case KDnsRRTypeMX:
			if (iRdLength > 2)
				{
				TDndRespMX &mx = aAnswer().MX();
				len = sizeof(mx);
				if (len > max_len)
					return KErrOverflow;
				(void)new (&mx) TDndRespMX();
				mx.SetPref(Get16(rd));
				err = GetNameFromRDATA(mx.HostName(), 2);
				}
			else
				err = KErrDndUnknown;
			break;
#if 0
		case KDnsRRTypeTXT:
			break;
#endif
		case KDnsRRTypeSRV:
			if (iRdLength > 4)
				{
				TDndRespSRV &srv = aAnswer().SRV();
				len = sizeof(srv);
				if (len > max_len)
					return KErrOverflow;
				(void)new (&srv) TDndRespSRV();
				srv.SetPriority(Get16(rd));
				srv.SetWeight(Get16(rd + 2));
				srv.SetPort(Get16(rd + 4));
				err = GetNameFromRDATA(srv.Target(), 6);
				}
			else
				err = KErrDndUnknown;
			break;
		case KDnsRRTypeNAPTR:
			if (iRdLength > 7)
				{
				TDndRespNAPTR &naptr = aAnswer().NAPTR();
				len = sizeof(naptr);
				if (len > max_len)
					return KErrOverflow;
				(void)new (&naptr) TDndRespNAPTR();
				naptr.SetOrder(Get16(rd));
				naptr.SetPref(Get16(rd + 2));
				// get FLAGS
				TPtrC8 flags;
				err = GetStringFromRDATA(flags, 4);
				if (err < 0)
					break;
				naptr.SetFlags(flags);
				// get SERVICES
				TPtrC8 services;
				err = GetStringFromRDATA(services, err - iRd);
				if (err < 0)
					break;
				naptr.SetService(services);
				// get REGEXP
				TPtrC8 regexp;
				err = GetStringFromRDATA(regexp, err - iRd);
				if (err < 0)
					break;
				naptr.SetRegexp(regexp);
				// Fetch REPLACEMENT
				err = GetNameFromRDATA(naptr.Replacement(), err - iRd);
				}
			else
				err = KErrDndUnknown;
			break;
#if 0
		case EDnsType_SOA:
			{
			TDndRespSOA &soa = aAnswer().SOA();
			len = sizeof(soa);
			if (len > max_len)
				return KErrOverflow;
			new (&soa) TDndRespSOA();

			err = GetNameFromRDATA(soa.MName());
			if (err < 0)
				return err;
			err = GetNameFromRDATA(soa.RName(), err - iRd);
			if (err < 0)
				return err;

			const TInt i = err - iRd;
			if (i < 0 || i + 20 > (TInt)iRdLength)
				return KErrDndUnknown;
			soa.SetSerial(Get32(rd+i));
			soa.SetRefresh(Get32(rd+i+4));
			soa.SetRetry(Get32(rd+i+8));
			soa.SetExpire(Get32(rd+i+12));
			soa.SetMininum(Get32(rd+i+16));
			}
			break;
#endif
		default:
			// For all yet unsupported replies, return only
			// the basic reply: TDnsQryRespBase
			{
			TDndReply &basic = aAnswer().Reply();
			len = sizeof(basic);
			if (len > max_len)
				return KErrOverflow;
			(void)new (&basic) TDndReply((TUint16)iType, (TUint16)iClass);
			basic.SetRRTtl(iTTL);
			}
			break;
		}
	aAnswer.SetLength(len + aAnswer().HeaderSize());
	// Return the TTL
	aAnswer().Reply().SetRRTtl(iTTL);
	return err;
	}
#endif

//
// TDndRR::FindRR
// **************
// Locate Nth (aNext) RR entry from the message
//
/**
// The RR's are "virtually" numbered from 0 to aNumRR-1.
// All RR's matching the original query are numbered
// first, and all others follow them.
//
// @param	aOffset	from the start of the message to the beginning of the RR section
// @param	aNumRR	the number of the RR's in this section
// @param	aType	the original query type
// @param	aClass	the original query class
// @param	aNext	select the Nth RR to be returned [0..aNumRR-1]
// @returns
// @li	> 0, Nth resource entry located (value is offset to first octet following this RR)
// @li	< 0, the following errors:
// @li	KErrDndNoRecord, there was no Nth RR.
// @li	KErrDndCache, the message is corrupt and should be discarded
// @li	Never returns 0 (KErrNone)!
*/
TInt TDndRR::FindRR(TInt aOffset, TInt aNumRR, EDnsQType aType, EDnsQClass aClass, TInt aNext)
	{
	TInt count = 0;

	aNext++;	// Looking for this! (internally numbered from 1..n)
	for (TInt any = 0;; any = 1)
		{
		TInt offset = aOffset;		// Start from beginning of RR's

		for (TInt j = 0; j < aNumRR; j++)
			{
			iName = offset;
			offset = iBuf.GetRR(iName, iType, iClass, iTTL, iRd, iRdLength);
			if (offset < 0)
				// ..if the above Resource() access fails (-1), then the stored cache
				// record must be corrupt, indicate so to the caller...
				return KErrDndCache;
			if (EDnsQType(iType) == aType && EDnsQClass(iClass) == aClass)
				// on first pass (any == 0), only count exact matches
				count += any ? 0 : 1;
			else
				// on second pass (any == 1), only count non-matches (CNAMES, etc.)
				count += any ? 1 : 0;
			if (count == aNext)
				{
				// Found the target RR
				return offset;
				}
			}
		// exit, if second pass completed!
		if (any != 0)
			break;
		}
	//
	// No matching entry located
	//
	return KErrDndNoRecord;
	}

/**
// The RR's are numbered from 0 to aNumRR-1.
// @param	aOffset	from the start of the message to the first RR to be tested
// @param	aNumRR	the number of the RR's
// @param	aType	the query type
// @param	aClass	the query class (EDnsQClass_ANY matches any class)
// @retval	aStart	the first RR to test [0..aNumRR-1]
// @returns
// @li	> 0, Nth resource entry located (value is offset to first octet following this RR)
// @li	< 0, the following errors:
// @li	KErrDndNoRecord, there was no such RR
// @li	KErrDndCache, the message is corrupt and should be discarded
// @li	Never returns 0 (KErrNone)!
*/
TInt TDndRR::LocateRR(TInt aOffset, TInt aNumRR, EDnsQType aType, EDnsQClass aClass, TInt aStart)
	{
	for (TInt j = 0; j < aNumRR; j++)
		{
		iName = aOffset;
		aOffset = iBuf.GetRR(iName, iType, iClass, iTTL, iRd, iRdLength);
		if (aOffset < 0)
			// ..if the above Resource() access fails (-1), then the stored cache
			// record must be corrupt, indicate so to the caller...
			return KErrDndCache;
		if (j >= aStart && EDnsQType(iType) == aType && (EDnsQClass(aClass) == EDnsQClass_ANY || EDnsQClass(iClass) == aClass))
			{
			// The object has been located
			return aOffset;
			}
		}
	//
	// No matching entry located
	//
	return KErrDndNoRecord;
	}

#ifdef LLMNR_ENABLED

/**
// Append a new RR
//
// This will append a new complete RR (with empty RDATA, RDLENGTH = 0)
// to the message buffer.
//
// @param	aName
//		content of the NAME field
// @param	aCompression
//		if non-zero, the name portion indicated by this
//		offset will be logically conctenated into the
//		NAME field.
// @returns
//	@li	KErrNone, on success
//	@li	KErrDndNameTooBig, when RR does not fit into the buffer
*/
TInt TDndRROut::Append(const TDesC8 &aName, TInt aCompression)
	{
	// Append the NAME
	TInt ret = iBuf.AppendName(aName, aCompression);
	if (ret != KErrNone)
		return ret;

	// Available space after fixed portion of the RR (after RDLENGTH)
	// has been appended.
	const TInt room = iBuf.MaxLength() - iBuf.Length() - 10;
	if (room < 0)
		return KErrDndNameTooBig; // Oops.. cannot fit RR into message (a bit dubious error code)

	// Append the TYPE
	iBuf.Append((TChar)(iType / 0x100));
	iBuf.Append((TChar)(iType % 0x100));

	// Append the CLASS
	iBuf.Append((TChar)(iClass / 0x100));
	iBuf.Append((TChar)(iClass % 0x100));

	// Append the TTL
	iBuf.Append((TChar)(iTTL / 0x1000000));
	iBuf.Append((TChar)((iTTL / 0x10000) % 0x100));
	iBuf.Append((TChar)((iTTL / 0x100) % 0x100));
	iBuf.Append((TChar)(iTTL % 0x100));

	// Append the RDLENGTH (placeholder)

	iBuf.Append((TChar)(0));
	iBuf.Append((TChar)(0));

	iRd = iBuf.Length();
	return KErrNone;
	}


/**
// Extend the RR with address RDATA
//
// This will append address RDATA the Resource Record.
// Only types AAAA or A are supported, and the length
// and format of the RDATA is selected accordingly.
//
// @param	aAddr	specifies the address to be appended
//
// @returns
//	@li	KErrNone, on success
//	@li	KErrDndNameTooBig, when RR does not fit into the buffer
//	@li KErrDndNotImplemented, if RR type is not A or AAAA.
*/
TInt TDndRROut::AppendRData(const TInetAddr &aAddr) const
	{
	const TInt room = iBuf.MaxLength() - iBuf.Length();

	if (iType == EDnsType_A)
		{
		if (room < 4)
			return KErrDndNameTooBig; // Oops.. cannot fit RR into message (a bit dubious error code)
		TUint32 address = aAddr.Address();
		iBuf.Append((TChar)(address >> 24));
		iBuf.Append((TChar)(address >> 16));
		iBuf.Append((TChar)(address >> 8));
		iBuf.Append((TChar)address);
		}
	else if(iType == EDnsType_AAAA)
		{
		// assumes aAddr is in KAfInet6 format!!!
		const TIp6Addr &address = aAddr.Ip6Address();
		if (room < (TInt)sizeof(address.u.iAddr8))
			return KErrDndNameTooBig; // Oops.. cannot fit RR into message (a bit dubious error code)
		iBuf.Append(address.u.iAddr8, sizeof(address.u.iAddr8));
		}
	else
		return KErrDndNotImplemented;

	AppendRData();
	return KErrNone;
	}

/**
// Extend the RR with domain name
//
// This will append domain name RDATA the Resource Record.
//
// @param	aName
//		the domain name to be added
// @param	aCompression
//		if non-zero, the name portion indicated by this
//		offset will be logically conctenated into aName.
//
// @returns
//	@li	KErrNone, on success
//	@li	KErrDndNameTooBig, when RR does not fit into the buffer
*/
TInt TDndRROut::AppendRData(const TDesC8 &aName, TInt aCompression) const
	{

	TInt ret = iBuf.AppendName(aName, aCompression);

	AppendRData();
	return ret;
	}

/**
// Extend the RR with any binary data
//
// This will append binary data into the Resource Record.
// (just a convenience method, which provides length checking
// and implicitly calls AppendRData())
//
// @param	aRData	the data to be appended
//
// @returns
//	@li	KErrNone, on success
//	@li	KErrDndNameTooBig, when RR does not fit into the buffer
*/
TInt TDndRROut::AppendRData(const TDesC8 &aRData) const
	{
	const TInt room = iBuf.MaxLength() - iBuf.Length();
	if (room < aRData.Length())
		return KErrDndNameTooBig;
	iBuf.Append(aRData);

	AppendRData();
	return KErrNone;
	}

/**
//	Update the RDLENGTH to match the current appended data
//
//	Assume Recource Record is complete and compute
//	the total amount RDATA that has been appended.
//
//	This can be called any number of times. 
*/
void TDndRROut::AppendRData() const
	{
	// Append(..) must have been called before this!
	__ASSERT_DEBUG(iRd > 1, User::Panic(_L("DEBUG"), 0));
	// Patch in the RD lenght
	const TInt length = iBuf.Length() - iRd;
	iBuf[iRd-2] = (TUint8)(length / 0x100);
	iBuf[iRd-1] = (TUint8)(length % 0x100);
	}

#endif
