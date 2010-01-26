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
// message.h - name resolver DNS message interpreter
//

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

/**
@file message.h
DNS message formats for queries and replies
@internalComponent	Domain Name Resolver
*/
#include "dns_hdr.h"
#include <es_sock.h>
#include <in_sock.h>
#include "res_sock.h"

#ifdef EXCLUDE_SYMBIAN_DNS_PUNYCODE
#undef SYMBIAN_DNS_PUNYCODE
#endif //EXCLUDE_SYMBIAN_DNS_PUNYCODE

#ifdef SYMBIAN_DNS_PUNYCODE
#include "punycodeconverter.h"
_LIT8(KAcePrefix, "xn--");
#endif

const TInt KErrDndDiscard = KErrUnknown;	// Discard the received packet without sending any response
											// to the application
const TUint KDndMaxLoopCount = 16;	//< This constant is used to detect infinite looping in compressed domain names.


typedef TBuf8<KDnsMaxName> THostNameEx;		//< Domain Name storage
typedef TBuf8<KDnsMaxLabel> TNodeLabel;		//< Label storage (one component of domain name)

#ifdef LLMNR_ENABLED
typedef enum
    {
    ELlmnr_NoMsg,
    ELlmnr_Query,       //< LLMNR query from a sender
    ELlmnr_Resp,        //< LLMNR response from a responder
    ELlmnr_Update,      //< LLMNR update req from a responder
    ELlmnr_Yxrrset,     //< conflict response from a responder to an update
    ELlmnr_ConflictResp //< conflict response from a sender, that has 
                        //< received multiple LLMNR responses
    } ELlmnrMessage;    //< LLMNR message type
#endif

class TDndQuestion;
//	Extends the base class with modifying functions
class TDndHeader : public TInet6HeaderDNS
	{
public:
	// Intialize all fields of fixed header
	void Init(const TUint16 aID = 0, const TUint8 aOpcode = 0);
	void SetId(const TUint16 aID);
	void SetOpcode(const TUint8 aOpcode);
#ifdef LLMNR_ENABLED
    void SetQR(const TUint aQR);
    void SetAA(const TUint aAA);
    void SetRCode(const TUint aRCode);
#endif
	void SetQdCount(const TUint16 aQdCount);
	void SetAnCount(const TUint16 aAnCount);
	void SetArCount(const TUint16 aArCount);
#ifdef SYMBIAN_DNS_PROXY
	void SetNsCount(const TUint16 aNsCount);
#endif

//#ifdef LLMNR_ENABLED
//    TInt CheckLlmnrHeader(ELlmnrMessage *aMsgType);   //< To check the header of the message received by LLMNR respoonder
//#endif
	};

/**
// @brief	A buffer for a DNS message packet
//
// TMsgBuf is a binary buffer which holds the message packet of the
// DNS protocol. This class adds a set of utility methods to the
// basic buffer class.
*/
class TMsgBuf : public TDes8
	{
private:
	TMsgBuf() {};	// Just to prevent construction
public:
	static inline TMsgBuf &Cast(TDes8 &aDes)
		{ return *((TMsgBuf *)&aDes); }
	static inline TMsgBuf &Cast(const TDes8 &aDes)
		{ return *((TMsgBuf *)&aDes); }
	static inline const TMsgBuf &Cast(const TDesC8 &aDes)
		{ return *((TMsgBuf *)&aDes); }
	// Returns raw message mapped as TDndHeader
	inline TDndHeader &Header() const { return (TDndHeader &)Ptr()[0]; }

#if 0
	// Extract the message ID from DNS message
	TInt GetID(TUint16 &aID) const;
#else
	TInt VerifyMessage(TInt &aRCode, TDndQuestion &aQuestion) const;
#endif
	// Decompress <domain-name> from DNS message
	TInt GetNextName(TInt aOffset, TDes8 &aName, const TUint aDepth = 0) const;

	// Extract <character-string> from DNS message
	TInt GetNextString(TInt aOffset, TPtrC8 &aString) const;

#ifndef SYMBIAN_DNS_PUNYCODE 
	// Decompress <domain-name> from DNS message 
	TInt GetName(TInt aOffset, TDes &aName) const;
#else
	// Decompress <domain-name> from DNS message with IDN option
	TInt GetName(TInt aOffset, TDes &aName, TBool aIdnEnabled=EFalse) const;
#endif //SYMBIAN_DNS_PUNYCODE

	
	// Append <domain-name> into DNS message
	TInt AppendName(const TDesC8 &aName, const TUint aCompressed = 0);

	// Extract RR from the specified offset.
	TInt GetRR(const TInt aOffset, TUint16 &aType, TUint16 &aClass, TUint32 &aTTL, TUint &aRDataOffset, TUint &aRDataLength) const;

	// Skip <domain-name> in DNS message
	TInt SkipName(TInt aOffset) const;
	};


// Perform the "DNS case folding" comparison between two names (or labels)
TBool DnsCompareNames(const TDesC8 &aName1, const TDesC8 &aName2);


//	A base class to hold uncompressed DNS name
class TDndName : public THostNameEx
	{
public:
#ifdef SYMBIAN_DNS_PUNYCODE
	// Constructor to initialse the member variables
	TDndName():iPunyCodeConverted(EFalse),iIdnEnabled(EFalse)
		{
		iPunyCodeName.SetLength(0);
		}
#endif //SYMBIAN_DNS_PUNYCODE

	// Set name from address (for PTR query)
	TInt SetName(const TInetAddr &aAddr);

	// Set name from hostname (system encoding)
	TInt SetName(const THostName &aName);

	// Set name from DNS message (domain name)
	TInt SetName(const TMsgBuf &aBuf, const TInt aOffset);

	//	Get (Append) name into buffer
	TInt GetName(TDes &aName, const TInt aStart = 0) const;

	// Get IP address from PTR name
	TBool GetAddress(TInetAddr &aAddr) const;
#ifdef SYMBIAN_DNS_PUNYCODE
	void EnableIdn(TBool aEnable)
	{
		iIdnEnabled = aEnable;
	}
private:
	TPunyCodeDndName iPunyCodeName;
	TBool iPunyCodeConverted;
	TBool iIdnEnabled;
#endif //SYMBIAN_DNS_PUNYCODE
	};

/**
// @brief	A Resource Record reference structure
//
// This is only a reference to the real data which is
// stored in a separate DNS message buffer (provided
// with the constructor).
*/
class TDndRR
	{
public:
	TDndRR(const TMsgBuf &aMsg) : iBuf(aMsg) {}
private:
	const TMsgBuf &iBuf;		//< A reference to he DNS reply message
public:

	// Initialize RR reference to point the specified RR value ("virtual ordering" of RR's)
	TInt FindRR(TInt aOffset, TInt aNumRR, EDnsQType aType, EDnsQClass aClass, TInt aNext = 0);
	// Initialize RR reference to point the specified RR value ("raw ordering").
	TInt LocateRR(TInt aOffset, TInt aNumRR, EDnsQType aType, EDnsQClass aClass, TInt aStart);

	// Extract information from RR, and pack it into aName and aAddr
	TInt GetResponse(TDes &aName, TInetAddr &aAddr) const; 
	// Extract information from RR, and pack it into generic TDnsQueryResp format
	TInt GetResponse(TDnsMessageBuf &aAnswer, TInetAddr **aAddr) const;

	// Extract address from the RR for A and AAAA type records.
	TInt GetSockAddr(TInetAddr& aAddr) const;

	// Extract <domain-name> from the RDATA of RR
	TInt GetNameFromRDATA(TDes8 &aName, const TUint aOffset = 0) const;

	// Extract <character-string> from the RDATA of RR
	TInt GetStringFromRDATA(TPtrC8 &aString, const TUint aOffset) const;

	TInt iName;					//< Offset of the RR (also the name)
	/*EDnsType*/ TUint16	iType;			//< Resource Type
	/*EDnsClass*/ TUint16	iClass;			//< Resource class
	TUint32		iTTL;			//< Time to live
	TUint		iRdLength;		//< Length of the resource Data
	TUint		iRd;			//< Offset of the RDATA content
#ifdef SYMBIAN_DNS_PUNYCODE
	TBool		iIdnEnabled;	//< whether IDN support is enabled or not.
#endif //SYMBIAN_DNS_PUNYCODE
	};

#ifdef LLMNR_ENABLED
/**
//	@brief	Resource Record builder class
//
//	A tool to build Resource records incrementally to a reply buffer.
//	The usage:
//
//	Construct TDndRROut(buf) and then for each Resource Record (RR),
//	repeat the following procedure
//
//	@li	initialize RR Type, Class and TTL
//	@li call Append() to create an RR the specified type with empty RData
//	@li call AppendRData methods to build the RData content (as many times
//		as required).
//
//	Or, alternately, append the RDATA by any other means to the message
//	buffer and call any one of the AppendRData methods as last to fix up the
//	correct RDLENGTH into the RR (calling AppendRData() will just fix the
//	RDLENGTH without adding anything).
*/
class TDndRROut
	{
public:
	TDndRROut(TMsgBuf &aMsg) : iBuf(aMsg), iRd(0) {}
private:
	TMsgBuf &iBuf;				//< A reference to he DNS message
public:
	TInt Append(const TDesC8 &aName, TInt aCompression);
	TInt AppendRData(const TDesC8 &aName, TInt aCompression) const;
	TInt AppendRData(const TInetAddr &aAddr) const;
	TInt AppendRData(const TDesC8 &aRData) const;
	void AppendRData() const;

	/*EDnsType*/ TUint16	iType;			//< Resource Type
	/*EDnsClass*/ TUint16	iClass;			//< Resource class
	TUint32		iTTL;			//< Time to live
#ifdef SYMBIAN_DNS_PUNYCODE
	TBool		iIdnEnabled;	//< whether IDN support is enabled or not.
#endif //SYMBIAN_DNS_PUNYCODE
private:
	TUint		iRd;			//< Offset of the RDATA content
	};
#endif

#ifdef LLMNR_ENABLED
class TDndReqData;
#endif

/**
// @brief	Unpacked representation of a Question Section
//
// The main components of a question are: the name, query
// type and class. This class is only for convenience and
// used becuase the name in real message can be compressed.
// It's sometimes easier to handle unpacked names.
*/
class TDndQuestion : public TDndName
	{
public:
	inline TDndQuestion() : iQType(EDnsQType(0)), iQClass(EDnsQClass(0)) {} //< Initialize to invalid type and class (= 0)

	// functions to set the private variables
	inline void SetQType(EDnsQType aQType) { iQType = aQType; }
	inline void SetQClass(EDnsQClass aQClass) { iQClass = aQClass; }

	inline EDnsQType QType() const { return iQType; }
	inline EDnsQClass QClass() const { return iQClass; }

	// Create a TDndQuestion from a DNS message
	TInt Create(const TMsgBuf &aBuf, TInt aIndex);

	// Append a Question section into DNS message
	TInt Append(TMsgBuf& aMsgBuf, TUint aCompressed = 0) const;

	// Compare if two questions are same
	TInt CheckQuestion(const TDndQuestion &aQuestion) const;
private:
	EDnsQType	iQType;
	EDnsQClass	iQClass;
	};
#endif
