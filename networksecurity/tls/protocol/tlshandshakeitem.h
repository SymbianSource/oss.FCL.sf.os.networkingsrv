/**
* Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
* SSL3.0 and TLS1.0 Handshake message items header file.
* This file contains definitions for SSL3.0 and TLS1.0 handshake items 
* (i.e., handshake protocol types, headers, message structures, etc).
* 
*
*/



/**
 @file TlsHandshakeItem.h
*/

#include "tlsrecorditem.h"

#ifndef _TLSHANDSHAKEITEM_H_
#define _TLSHANDSHAKEITEM_H_

#ifdef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <tlstypedef_internal.h>
#endif


// Handshake protocol types, as defined in RFC2246, section A.4
enum ETlsHandshakeMessage 
{
	ETlsHelloRequestMsg = 0,						/** Hello Request message */
	ETlsClientHelloMsg = 1,							/** Client Hello message */
	ETlsServerHelloMsg = 2,							/** Server Hello message */
	ETlsCertificateMsg = 11,						/** Certificate message */
	ETlsServerCertificateMsg = ETlsCertificateMsg,	/** Certificate message (server)	*/
	ETlsServerKeyExchMsg = 12,						/** Server Key exchange message */
	ETlsCertificateReqMsg = 13,						/** Certificate Request message */
	ETlsServerHelloDoneMsg = 14,					/** Server Hello Done message */
	ETlsClientCertificateMsg = ETlsCertificateMsg,	/** Certificate message (client) */
	ETlsCertificateVerifyMsg = 15,					/** Certificate verify message */
	ETlsClientKeyExchMsg = 16,						/** Client Key exchange message */
	ETlsFinishedMsg = 20,							/** Finished message */
	ETlsMsgNotDefined = 200							/** Undefined/unknown message */
};

// Constants for a Handshake header object
const TInt KTlsHandshakeHeaderSize = 4;		///< Size of Handshake protocol header, 4 bytes
const TInt KTlsHandshakeTypeOffset = 0;		///< Offset of Handshake type within the header
const TInt KTlsHandshakeLengthOffset = 1;	///< Offset of Handshake length within the header
const TInt KTlsHandshakeBodyLength = 3; 	///< Size of Handshake body length, 3 bytes
const TInt KTlsMaxHandshakeBodySize = 0x80000; 	///< Max Size of Handshake body

// Constants for Hello message (Client and Server) items
const TInt KTlsProtocolBodyLength = 2;		///< Number of bytes for the Protocol version's body item
const TInt KTlsSessionIdLength = 1;			///< Number of bytes for the Session Id's length item
const TInt KTlsCipherSuiteLength = 2;		///< Number of bytes for the CipherSuite's length item
const TInt KTlsCompressionLength = 1;		///< Number of bytes for the Compression's length item
const TInt KTlsCompressionBodyLength = 1;	///< Number of bytes for the Compression's body item, 
											///< currently 1 as only NULL compression is supported
const TInt KTlsCipherSuiteBodyLength = 2;	///< Number of bytes for the selected cipher suite body item

// Other message constants
const TInt KTlsCertChainLength = 3;			///< Number of bytes for the Certificate chain length
const TInt KTlsVerifyDataLength = 12;		///< Number of bytes for the VerifyData item (Finished message)
const TInt KTlsSigLength = 2;				///< Number of bytes for a signature length
const TInt KTlsMd5Length = 16;				///< Number of bytes for MD5 hash
const TInt KTlsShaLength = 20;				///< Number of bytes for SHA hash
const TInt KTlsRsaModLength = 2;			///< Number of bytes for the RSA modulus's length item
const TInt KTlsRsaKeyExchLength = 2;			///< RSA key exchange length
const TInt KTlsRsaModBodyLength = 2;		///< Initial number of bytes for the RSA modulus's body item (initially == to  KTlsRsaModLength)
const TInt KTlsRsaExpLength = 2;			///< Number of bytes for the RSA exponent's length item
const TInt KTlsRsaExpBodyLength = 2;		///< Initial number of bytes for the RSA exponent's body item (initially == to  KTlsRsaExpLength)
const TInt KTlsDhpLength = 2;				///< Number of bytes for the DH prime modulus' length item
const TInt KTlsDhpBodyLength = 2;			///< Number of bytes for the DH prime modulus' body item
const TInt KTlsDhgLength = 2;				///< Number of bytes for the DH generator length item
const TInt KTlsDhgBodyLength = 2;			///< Number of bytes for the DH generator body item
const TInt KTlsDhYsLength = 2;				///< Number of bytes for the DH public value length item
const TInt KTlsDhYsBodyLength = 2;			///< Number of bytes for the DH public value body item
const TInt KTlsCertTypesLength = 1;			///< Number of bytes for the Certificate types length item
const TInt KTlsCertTypesBodyLength = 1;		///< Number of bytes for the Certificate types body item
const TInt KTlsCALength = 2;				///< Number of bytes for the Certificate Authorities length item
const TInt KTlsDhYcLength = 2;				///< Number of bytes for the DHE Yc length item
const TInt KTlsDhYcBodyLength = 2;			///< Number of bytes for the DHE Yc item (length item + DH Yc value), default is KTlsDhYcLength.
const TInt KTlsPskLength = 2;				///< Number of bytes for the PSK other_secret and psk length fields
const TInt KTlsExtensionLength = 2;			///< Number of bytes for the Extension length fields
const TInt KTlsExtensionTypeLength = 2;			///< Number of bytes for the Extension type field
const TInt KTlsExtensionNameTypeLength = 1;	///< Number of bytes for the Extension name type length fields

class CHandshakeHeader : public CItem< TConstant >
/** 
 * Represents a Handshake message header
 */ 
{
public:
   CHandshakeHeader( CItemBase* aNext ) :
      CItem< TConstant >( aNext, KTlsHandshakeHeaderSize),
      iRecord( NULL )
   {
      iRecord.iFirst = this;
   }

   void ComposeHeader( ETlsHandshakeMessage aHandshakeMessage, TInt aLength );
	
public:
   TRecord iRecord;
};

// Class initialisation for these message items strongly relies on the fact that 
// non-virtual base clases are initialised in the order in which they appear in 
// the class declaration. Also, member objects are initialised in the order in 
// which they are declared in the class.


class CClientHelloMsg : public CHandshakeHeader
/**
 * Represents the data items that make up a Client Hello message. 
 *
 * Its constructor sets up the default values for a Hello message object, e.g.,
 * the size of the Handshake header (4), the number of bytes to use for the protocol 
 * version (2), the number of bytes used to represent the length of session id's length
 * item (1), etc. The values of these items will subsequently be set when the body of 
 * the item has been retrieved.
 *
 * @note A Session Id is represented by 2 items, the 1st is 1 byte which will contain 
 * the actual length of the session id, the 2nd is the session id itself which is of 
 * variable length (32 bytes or less).
 */
{
public:
   CClientHelloMsg(CItemBase *aClientHelloExtension = NULL) :
      CHandshakeHeader( &iVersion ),
      iVersion( &iRandom, KTlsProtocolBodyLength ),
      iRandom( &iSessionId ),
      iSessionId( KTlsSessionIdLength, &iCipherSuite ),
      iCipherSuite( KTlsCipherSuiteLength, &iCompression ),
      iCompression( KTlsCompressionLength, aClientHelloExtension, KTlsCompressionBodyLength )
   {
   }

public:
   CConstItem iVersion;			///< SSL or TLS protocol version, 2 bytes
   CRandomItem iRandom;			///< 32 byte Random value
   CVariableItem iSessionId;	///< Session Id (variable size, 32 bytes or less)
   CVariableItem iCipherSuite;	///< Proposed cipher suites (variable size)
   CVariableItem iCompression;	///< Proposed compression methods (variable size)
};

/**
 The class represents a header of an CCompoundList. 
 
 An item list is a variable part of a message. It's created as the data is being parsed
 @see CCompoundList
 @internalComponent
 **/
NONSHARABLE_CLASS(CCompoundListHeader) : public CConstItem
{
public:
	CCompoundListHeader( CItemBase* aNext, TInt aLength) :
      CConstItem( aNext, aLength )
	{
   	}
};

/**
 Represents an item list consisting of CItemBase items and 
 having a CCompoundListHeader header.
 @see CCompoundListHeader
 @see CListItemHeader
 @internalComponent
 **/
NONSHARABLE_CLASS(CCompoundList) : public CCompoundListHeader
{
public:
   CCompoundList( CItemBase* aNext, TInt aLength );
   virtual ~CCompoundList();

   virtual void Dump( const TDesC& aTag, const TDesC& aFile );
   void AddNodeL( CItemBase* aNode );
   virtual void InitialiseL( TPtr8& aBuf8 );  //buffer must be initialised beforehand
   virtual TInt CalcTotalInitialiseLength() const;

   virtual void ParseL( TPtr8& aDes8 );

   CItemBase* Node(TInt aIndex) const;

   void DropNodes();

private:
   //TRecord iRecord; // list containing all fields of all nodes
   RArray<TRecord> iRecords;
};

inline CCompoundList::CCompoundList( CItemBase* aNext, TInt aLength) :
   CCompoundListHeader( aNext, aLength )
{
}

inline CItemBase* CCompoundList::Node(TInt aIndex) const
   {
   	  if((aIndex < 0) || (aIndex >= iRecords.Count()))
   	  	{
   	  	return 0;
   	  	}
      return iRecords[aIndex].First();
   }

inline void CCompoundList::DropNodes()
{
	iRecords.Reset();
}

#ifdef _DEBUG
inline void CCompoundList::Dump( const TDesC& aTag, const TDesC& aFile )
/** 
 * Dumps the variable item list's header and item list.

 * The items must be parsed or initialised
 *
 * @param aTag A log file tag
 * @param aFile A log file
 */
{
   CCompoundListHeader::Dump( aTag, aFile );
   TInt nodeCount = iRecords.Count();
   for(TInt i=0; i<nodeCount; ++i)
		{
		iRecords[i].Dump( aTag, aFile );
		}
}
#else
inline void CCompoundList::Dump( const TDesC& /*aTag*/, const TDesC& /*aFile*/ )
{
}
#endif

/**
Base class for TLS extensions.

 All extensions start with a 2 byte type field followed by a length field, but this class only represents the type field.

 @internalComponent
 **/
NONSHARABLE_CLASS(CExtensionNode) : public CConstItem
{
public:
 	enum TTlsExtensionType
	{
		KExtServerName = 0,
		KExtMaxFraqmentLength = 1,
		KExtClientCertificateUrl = 2,
		KExtTrustedCaKeys = 3,
		KExtTruncatedHmac = 4,
		KExtStatusRequest = 5
	};
	virtual TInt ExtensionLength();
protected:
   CExtensionNode(CItemBase* aNext);
   virtual ~CExtensionNode();
};


/**
 Represents one item of the extension list.
 The body of the extension is contained in a class derived from this class and is wrapped by our iOpaqueData member.
  @internalComponent
 **/
NONSHARABLE_CLASS(CKnownExtensionNode) : public CExtensionNode
{
protected:
   CKnownExtensionNode();
   void ConstructOpaqueDataWrapperL(CItemBase* aNext);
   ~CKnownExtensionNode();
private:
	CCompoundList iOpaqueData;
};


/**
 Represents a generic extension
 **/
NONSHARABLE_CLASS(CGenericExtension) : public CExtensionNode
{
public:
	static CGenericExtension* NewLC(TInt aInitialLength);
	virtual TInt ExtensionLength();

private:
   CGenericExtension( TInt aInitialLength );
   ~CGenericExtension();
public:
  	CVariableItem iOpaqueData; // The opaque extension data
};


NONSHARABLE_CLASS(CGenericExtensionList) : public CCompoundList
{
public:
   CGenericExtensionList( CItemBase* aNext);

   virtual void ParseL( TPtr8& aDes8 );

   CGenericExtension* Node(TInt aIndex) const;
};


NONSHARABLE_CLASS(CClientServerNameEntry) : public CConstItem
{
public:
 	enum TTlsExtensionServerNameType
	{
		KExtServerNameTypeDns = 0
 	};

	static CClientServerNameEntry* NewLC(TInt aInitialLength);
private:
   CClientServerNameEntry( TInt aInitialLength );
public:
  	CVariableItem iName; // Server name
};


NONSHARABLE_CLASS(CClientServerNameExtension) : public CKnownExtensionNode
/**
 Represents a client server name identification extension
 **/
{
public:
	static CClientServerNameExtension* NewLC();
	void AddServerNameEntryL(CClientServerNameEntry *aServerNameEntry);
	CClientServerNameEntry* Node(TInt aIndex);
private:
   CClientServerNameExtension();
public:
  	CCompoundList iServerNames;
};


NONSHARABLE_CLASS(CClientHelloWithExtensionsMsg) : public CClientHelloMsg
	/**
	Represents a Client Hello message which includes one or more Extensions.
	*/
{
public:
	CClientHelloWithExtensionsMsg(CItemBase *aNext = NULL) :
		CClientHelloMsg(&iClientHelloExtensionList),
		iClientHelloExtensionList(aNext, KTlsExtensionLength)
  {
  }
	inline void AddExtensionL(CExtensionNode *aExtension);
	inline CExtensionNode* Node(TInt aIndex);
	
	CCompoundList iClientHelloExtensionList;
};

inline void CClientHelloWithExtensionsMsg::AddExtensionL(CExtensionNode *aExtension)
{
	iClientHelloExtensionList.AddNodeL(aExtension);
}

inline CExtensionNode* CClientHelloWithExtensionsMsg::Node(TInt aIndex)
{
	return static_cast<CExtensionNode*>(iClientHelloExtensionList.Node(aIndex));
}


class CServerHelloMsg : public CHandshakeHeader
/**
 * Represents the data items that make up a Server Hello message. 
 *
 * Its constructor sets up the default values for a Hello message object, e.g.,
 * the size of the Handshake header (4), the number of bytes to use for the protocol 
 * version (2), the number of bytes used to represent the length of session id's length
 * item (1), etc. The values of these items will subsequently be set when the body of 
 * the item has been retrieved.
 * Note that for a Server Hello message, a single cipher suite (2 bytes) and a single 
 * compression value (1 byte) is returned. These items do not have length items 
 * (compared to the Client Hello message). 
 */
{
public:
   CServerHelloMsg(CItemBase *aNext = 0) :
      CHandshakeHeader( &iVersion ),
      iVersion( &iRandom, KTlsProtocolBodyLength ),
      iRandom( &iSessionId ),
      iSessionId( KTlsSessionIdLength, &iCipherSuite ),
      iCipherSuite( &iCompression, KTlsCipherSuiteBodyLength ),
      iCompression( aNext, KTlsCompressionBodyLength )
   {
   }

public:
   CConstItem iVersion;			///< Selected protocol version, 2 bytes
   CRandomItem iRandom;			///< 32 byte Server random value
   CVariableItem iSessionId;	///< Session Id (variable size, 32 bytes or less)
   CConstItem iCipherSuite;		///< Selected cipher suites, 2 bytes
   CConstItem iCompression;		///< Selected compression methods, 1 byte
};

class CServerHelloMsgWithOptionalExtensions : public CServerHelloMsg
{
public:
	CServerHelloMsgWithOptionalExtensions()
		: CServerHelloMsg(&iExtensions),
		  iExtensions(NULL)
		{
		}
	CGenericExtensionList iExtensions;
};

class CCertificateMsg : public CHandshakeHeader
/**
 * Represents the Certificate (Client or Server) message items.
 *
 * The body of the Certificate message contains a chain of public key 
 * certificates with the first 3 bytes of this message indicating the
 * length of this chain.
 */ 
{
public:
   CCertificateMsg() :
      CHandshakeHeader( &iCertificateList ),
      iCertificateList( 0, KTlsCertChainLength )
   {
   }
   void ComposeHeader( TInt aLength ) { CHandshakeHeader::ComposeHeader( ETlsCertificateMsg, aLength );}

public:
   CListItem iCertificateList;	///< List of certificates (Certificate chain)
};


class CFinishedMsg : public CHandshakeHeader
/**
 * Represents the contents/body of a SSL3.0 and/or a TLS1.0 Protocol 
 * Finished message. This message indicates that the negotiation is complete and 
 * that the negotiated cipher suite is in effect. As such, it is the first message  
 * protected by the negotiated algorithms, keys and secrets.
 *
 * The message body consists of 12 bytes of data calculated using a Pseudo Random
 * Function (PRF) for TLS1.0 and a concatenation of two hash results using MD5 and 
 * SHA hash algorithms for SSL3.0.
 */
{
public:
   CFinishedMsg(TInt aMsgLength) :
      CHandshakeHeader( &iFinishedData ),
      iFinishedData( 0, aMsgLength )
   {
   }
   void ComposeHeader( TInt aLength ) { CHandshakeHeader::ComposeHeader( ETlsFinishedMsg, aLength );}

public:
   CConstItem iFinishedData;		///< 12 bytes (TLS1.0) or 36 bytes (SSL3.0)
};


class CRsaParams
/**
 * Represents a Server's RSA key exchange parameters. 
 * It consists of the RSA modulus and public exponent.
 */
{
public:
   CRsaParams( CItemBase* aNext = NULL ) :
      iRsaModulus( KTlsRsaModLength, &iRsaExponent, KTlsRsaModBodyLength ),
      iRsaExponent( KTlsRsaExpLength, aNext, KTlsRsaExpBodyLength )
   {
   }

public:
   CVariableItem iRsaModulus;	///< The modulus of the server's temporary RSA key.
   CVariableItem iRsaExponent;	///< The public exponent of the server's temporary RSA key.

};

class CDhParams
/**
 * Represents a Server's Ephemeral DH key exchange parameters.
 * It consists of the prime modulus, generator and DH public value.
 */
{
public:
   CDhParams( CItemBase* aNext = NULL ) :
      iDh_p( KTlsDhpLength, &iDh_g, KTlsDhpBodyLength ),
      iDh_g( KTlsDhgLength, &iDh_Ys, KTlsDhgBodyLength ),
      iDh_Ys( KTlsDhYsLength, aNext, KTlsDhYsBodyLength )
   {
   }

public:
   CVariableItem iDh_p;		///< The prime modulus used for the Diffie-Hellman operation.
   CVariableItem iDh_g;		///< The generator used for the Diffie-Hellman operation.
   CVariableItem iDh_Ys;	///< The server's Diffie-Hellman public value (g^X mod p).

};

class CPskServerParams
/**
 * Represents a Server's PreShared Key (PSK) key exchange parameters.
 * It consists of the just the psk_identity_hint.
 */
{
public:
   CPskServerParams( CItemBase* aNext = NULL ) :
      iPskIdentityHint( KTlsPskLength, aNext )
   {
   }

public:
   CVariableItem iPskIdentityHint;		///< The "psk identity hint" field from the server
};

class CRsaSignature
/**
 * Represents a RSA signature. This is a hash (MD5 + SHA) of the 
 * Server's key with the signature appropriate to that hash applied (RSA).
 * Note that 1 signature covers both hashes.
 */
{
public:
   CRsaSignature() :
      iMd5Sha( KTlsSigLength, 0, KTlsMd5Length + KTlsShaLength )
   {
   }

public:
   CVariableItem iMd5Sha;		///< 36 bytes
};

class CDsaSignature
/**
 * Represents a DSA signature. This is a SHA hash of the Server's key
 * exchange params with the signature appropriate to that hash applied (DSA).
 */
{
public:
   CDsaSignature() :
      iSha( KTlsSigLength, 0, KTlsShaLength )
   {
   }

public:
   CVariableItem iSha;		///< 20 bytes
};

// Server-side messages. 
// Note that the Hello Request message (CHelloRequestMsg) is an empty message.
class CMessageDigest;
class CTlsCryptoAttributes;
class CServerKeyExchMsg : public CHandshakeHeader
/**
 * Server Key Exchange message - abstract
 */
{
public:
   CServerKeyExchMsg( CItemBase* aNext ) :
      CHandshakeHeader( aNext )
   {
   }

public:
   virtual void CopyParamsL( CTlsCryptoAttributes *aAttrs ) = 0;
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest ) = 0;
   virtual TPtr8 Signature() = 0;

protected:
   void ComputeDSADigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const;
   void ComputeRSADigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const;
   void ComputeDigest( CMessageDigest* pDigest, const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const;
};

class CRsaAnonServerKeyExchMsg : public CServerKeyExchMsg
/**
 * Server Key Exchange message with RSA params and no signature (anonymous).
 */
{
public:
   CRsaAnonServerKeyExchMsg() :
      CServerKeyExchMsg( &iRsaParams.iRsaModulus )
   {
   }

   virtual void CopyParamsL( CTlsCryptoAttributes *aAttrs );
public:
   CRsaParams iRsaParams;

protected:
   TUint8* GetDigestParamsPtr() const;
   TInt GetDigestParamsLength() const;
};

class CRsaDsaServerKeyExchMsg : public CRsaAnonServerKeyExchMsg
/**
 * Server Key Exchange message with RSA params and a DSA signature.
 */
{
public:
   CRsaDsaServerKeyExchMsg()
   {
      iRsaParams.iRsaExponent.iNext = &iDsaSignature.iSha;
   }
   
   virtual TPtr8 Signature();
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest );
public:
   CDsaSignature iDsaSignature;
};

class CRsaRsaServerKeyExchMsg : public CRsaAnonServerKeyExchMsg
/**
 * Server Key Exchange message with RSA params and a RSA signature.
 */
{
public:
   CRsaRsaServerKeyExchMsg()
   {
      iRsaParams.iRsaExponent.iNext = &iRsaSignature.iMd5Sha;
   }

   virtual TPtr8 Signature();
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest );

public:
   CRsaSignature iRsaSignature;
};

class CDhAnonServerKeyExchMsg : public CServerKeyExchMsg
/**
 * Server Key Exchange message with DH params and no signature (anonymous).
 */
{
public:
   CDhAnonServerKeyExchMsg() :
      CServerKeyExchMsg( &iDhParams.iDh_p )
   {
   }
   virtual void CopyParamsL( CTlsCryptoAttributes *aAttrs );

public:
   CDhParams iDhParams;

protected:
   TUint8* GetDigestParamsPtr() const;
   TInt GetDigestParamsLength() const;
};

class CDhDsaServerKeyExchMsg : public CDhAnonServerKeyExchMsg
/**
 * Server Key Exchange message with DH params and a DSA signature.
 */
{
public:
   CDhDsaServerKeyExchMsg()
   {
      iDhParams.iDh_Ys.iNext = &iDsaSignature.iSha;
   }
   
   virtual TPtr8 Signature();
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest );
   
public:
   CDsaSignature iDsaSignature;
};

class CDhRsaServerKeyExchMsg : public CDhAnonServerKeyExchMsg
/**
 * Server Key Exchange message with DH params and a RSA signature.
 */
{
public:
   CDhRsaServerKeyExchMsg()
   {
      iDhParams.iDh_Ys.iNext = &iRsaSignature.iMd5Sha;
   }

   virtual TPtr8 Signature();
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest );

public:
   CRsaSignature iRsaSignature;
};

class CPskServerKeyExchMsg : public CServerKeyExchMsg
/**
 * PSK Server Key Exchange message
 */
{
public:
   CPskServerKeyExchMsg()
      : CServerKeyExchMsg( &iPskServerParams.iPskIdentityHint)
   {
   }

   virtual void CopyParamsL( CTlsCryptoAttributes *aAttrs );
   virtual TPtr8 Signature();
   virtual void ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest );

public:
   CPskServerParams iPskServerParams;
};

class CCertificateReqMsg : public CHandshakeHeader
/**
 * Represents a Certificate Request message sent by a Server.
 * This message asks a Client to send its certificate (to authenticate itself)
 * and to sign information using the private key for that Certificate. It also
 * tells a client which Certificates are acceptable to the Server.
 *
 * Note that SSL3.0 defines 3 Certificate types more than TLS1.0 (rsa_ephemeral_dh,
 * dss_ephemeral_dh, fortezza_kea(20)). Fortezza is not supported in this implementation.
 */

{
public:
   CCertificateReqMsg() :
      CHandshakeHeader( &iClientCertificateTypes ),
      iClientCertificateTypes( KTlsCertTypesLength, &iDistinguishedNames, KTlsCertTypesBodyLength ),
      iDistinguishedNames( 0, KTlsCALength )
   {
   }

public:
   CVariableItem iClientCertificateTypes;	///< List of the requested Certificates types.
   CListItem iDistinguishedNames;			///< List of acceptable CAs.
};

class CServerHelloDoneMsg : public CHandshakeHeader
/**
 * Server Hello Done message. This message concludes the Server's part of a 
 * handshake negotiation. Apart from the handshake type, its message body is empty
 * (i.e., it does not carry any information).
 */
{
public:
   CServerHelloDoneMsg() :
      CHandshakeHeader( 0 )
   {
   }
};

// Client side messages
// Client side messages
class CClientKeyExchMsg : public CHandshakeHeader
{
public:
   CClientKeyExchMsg( CItemBase* iNext ) :
      CHandshakeHeader( iNext )
   {
   }

   void ComposeHeader( TInt aLength ) { CHandshakeHeader::ComposeHeader( ETlsClientKeyExchMsg, aLength );}
   virtual void SetKeyExchnage( TDesC8& aDes8 ) = 0;
};

class CRsaClientKeyExchMsg31 : public CClientKeyExchMsg
/**
 * Represents a RSA Client Key Exchange message for TLS1.0. This message enables a 
 * Client to provide the Server with the key materials necessary for securing the
 * communication.
 * 
 * For RSA, the message body consists of the Encrypted Premaster secret (2 bytes
 * for the Protocol version and 46 securely generated random bytes) encrypted with the
 * server's public key. Encryption of the premaster secret increases its size from 48 bytes.
 */
{
public:
   CRsaClientKeyExchMsg31( TInt aEncryptedDataLength ) :
      CClientKeyExchMsg( &iEncryptedPreMasterSecret ),
	  iEncryptedPreMasterSecret( KTlsRsaKeyExchLength, NULL, aEncryptedDataLength)
   {
   }
   virtual void SetKeyExchnage( TDesC8& aDes8 ) {iEncryptedPreMasterSecret.SetBody( aDes8 );}

public:
   CVariableItem iEncryptedPreMasterSecret;
};

class CRsaClientKeyExchMsg30 : public CClientKeyExchMsg
/**
 * Represents a RSA Client Key Exchange message for SSL3.0. This message enables a 
 * Client to provide the Server with the key materials necessary for securing the
 * communication.
 * 
 * For RSA, the message body consists of the Encrypted Premaster secret (2 bytes
 * for the Protocol version and 46 securely generated random bytes) encrypted with the
 * server's public key. Encryption of the premaster secret increases its size from 48 bytes.
 */
{
public:
   CRsaClientKeyExchMsg30( TInt aEncryptedDataLength ) :
      CClientKeyExchMsg( &iEncryptedPreMasterSecret ),
	  iEncryptedPreMasterSecret( NULL, aEncryptedDataLength)
   {
   }
   virtual void SetKeyExchnage( TDesC8& aDes8 ) {iEncryptedPreMasterSecret.SetBody( aDes8 );}

public:
   CConstItem iEncryptedPreMasterSecret;
};

class CDhExplicitClientKeyExchMsg : public CClientKeyExchMsg
/**
 * Represents an Explicit DH Client Key Exchange message (Ephemeral DH). 
 * This message enables a Client to provide the Server with the key materials 
 * necessary for securing the communication.
 * 
 * The message body contains the Client's DH public (Yc) value which is sent explicitly
 * in this message.
 */
{
public:
   CDhExplicitClientKeyExchMsg( TInt aEncryptedDataLength ) :
      CClientKeyExchMsg( &iDh_Yc ),
      iDh_Yc( KTlsDhYcLength, 0, aEncryptedDataLength )
   {
   }
   virtual void SetKeyExchnage( TDesC8& aDes8 ) {iDh_Yc.SetBody( aDes8 );}

public:
   CVariableItem iDh_Yc;
};

class CDhImplicitClientKeyExchMsg : public CClientKeyExchMsg
/**
 * Represents an Implicit DH Client Key Exchange message.
 * This message enables a Client to provide the Server with the key materials 
 * necessary for securing the communication.
 * 
 * The Client's public (Yc) value is implicit (contained in the Client certificate)
 * and does not need to be sent again. As such, the message body is empty, although
 * the message will be sent.
 */
{
public:
   CDhImplicitClientKeyExchMsg() :
      CClientKeyExchMsg( NULL )
   {
   }
   virtual void SetKeyExchnage( TDesC8& /*aDes8*/ ) {}
};

class CPskClientKeyExchMsg : public CClientKeyExchMsg
/**
 * Represents a PSK Client Key Exchange message. 
 * This message enables a Client to provide the Server with the key materials 
 * necessary for securing the communication.
 * 
 * The message body contains the PSK identity.
 */
{
public:
   CPskClientKeyExchMsg( TInt aDataLength ) :
      CClientKeyExchMsg( &iPskIdentity ),
      iPskIdentity( KTlsPskLength, 0, aDataLength )
   {
   }
   virtual void SetKeyExchnage( TDesC8& aDes8 ) {iPskIdentity.SetBody( aDes8 );}

public:
   CVariableItem iPskIdentity;
};

class CCertificateVerifyMsg : public CHandshakeHeader
/**
 * Represents a Certificate verify message base
 * 
 */
{
public:
   CCertificateVerifyMsg( CItemBase* aNext ) :
      CHandshakeHeader( aNext )
   {
   }
   void ComposeHeader( TInt aLength ) { CHandshakeHeader::ComposeHeader( ETlsCertificateVerifyMsg, aLength );}
   virtual void SetSignature( TDesC8& aSign ) = 0;
   virtual void SetSignatureLength( TDesC8& aSign ) = 0;
};

class CDsaCertificateVerifyMsg : public CCertificateVerifyMsg
/**
 * Represents a Certificate verify message. This message provides explicit
 * verification of a DSA certificate.
 * For SSL3.0 and TLS1.0, a SHA hash is created and signed. However, the input
 * into the hash differs for both protocols.
 * 
 * The message body consists of a DSA signature. 
 */
{
public:
   CDsaCertificateVerifyMsg() :
      CCertificateVerifyMsg( &iDsaSignature.iSha )
   {
   }

   virtual void SetSignature( TDesC8& aSign );
   virtual void SetSignatureLength( TDesC8& aSign );

public:
   CDsaSignature iDsaSignature;
};

class CRsaCertificateVerifyMsg : public CCertificateVerifyMsg
/**
 * Represents a Certificate verify message. This message provides explicit
 * verification of a RSA certificate. 
 * For SSL3.0 and TLS1.0, two hashes (MD5 and SHA) are combined and signed.
 * However, the input into the hash differs for both protocols.
 * 
 * The message body consists of a RSA signature. 
 */
{
public:
   CRsaCertificateVerifyMsg() :
      CCertificateVerifyMsg( &iRsaSignature.iMd5Sha )
   {
   }
   void ComposeHeader( TInt aLength ) { CHandshakeHeader::ComposeHeader( ETlsCertificateVerifyMsg, aLength );}
   
   virtual void SetSignature( TDesC8& aSign );
   virtual void SetSignatureLength( TDesC8& aSign );

public:   
   CRsaSignature iRsaSignature;
};

// Inline functions
inline TUint8* CRsaAnonServerKeyExchMsg::GetDigestParamsPtr() const
{
   return iRsaParams.iRsaModulus.Ptr();
}

inline TInt CRsaAnonServerKeyExchMsg::GetDigestParamsLength() const
{
   return iRsaParams.iRsaModulus.GetItemLength() + iRsaParams.iRsaExponent.GetItemLength();
}

inline TUint8* CDhAnonServerKeyExchMsg::GetDigestParamsPtr() const
{
   return iDhParams.iDh_p.Ptr();
}

inline TInt CDhAnonServerKeyExchMsg::GetDigestParamsLength() const
{
   return iDhParams.iDh_p.GetItemLength() + iDhParams.iDh_g.GetItemLength() + 
          iDhParams.iDh_Ys.GetItemLength();
}

#endif
