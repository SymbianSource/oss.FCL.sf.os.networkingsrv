// Copyright (c) 2003-2009 Nokia Corporation and/or its subsidiary(-ies).
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
 @file 
 @publishedPartner
 @released
*/

#ifndef __LIB_PFKEY_H__
#define __LIB_PFKEY_H__

#include <e32std.h>
#include <in_sock.h>

#include <networking/pfkeyv2.h>

/**
 *	This class is only used directly by the incoming packet processing path.
 *	Here we are only interested in the start position of the extension here.
 */
class TPfkeyAnyExt : public TPtrC8
	{
public:
	/**
	 * Create an extension from a ByteStream, checking for basic length
	 */
	inline TPfkeyAnyExt(const TDesC8& aDes);
	inline TPfkeyAnyExt();
	inline TUint16 ExtLen() const;
	inline TUint16 ExtType() const;

protected:
	/**
	 * Gets a reference to the extension header
	 */ 
	inline const struct sadb_ext& ExtHdr() const;
	inline struct sadb_ext& ExtHdr();
	/**
	 * Base class copy constructor will work
	 * Assignment operator is banned in the base class
	 */
	};

/**
 * TPfkeyExt is the whole extension including the data in the extension
 * This is aware of the Size and Start position of the Header, as well 
 */
// Outgoing messages have access to the type
// Incoming ones do not
template <class T>
class TPfkeyExt : public TBuf8<sizeof(T)> 
	{
public:
    /**
	 * Gets a reference to the raw extension
	 */
	inline const T& Ext() const;
	inline T& Ext();

protected:
	/**
	 * Default Constructor
	 */
	inline TPfkeyExt();
	};

/** 
 *  The base class for all messages to and from the SADB.
 */  
#ifdef SYMBIAN_NETWORKING_IPSEC_IKE_V2
#ifdef  SYMBIAN_IPSEC_VOIP_SUPPORT
static const TInt KPfkeyMsgMaxLen = 1600;
#else
static const TInt KPfkeyMsgMaxLen = 800;
#endif // SYMBIAN_IPSEC_VOIP_SUPPORT
#else
static const TInt KPfkeyMsgMaxLen = 400;
#endif
class TPfkeyMsgBase : public TBuf8<KPfkeyMsgMaxLen>
	{
public:
	/**
	 */
	inline const struct sadb_msg& MsgHdr() const;
	inline struct sadb_msg& MsgHdr();

protected:
	/**
	 *  Creates a Pfkey mesage of minimal(sadb_msg) size
	 */
	inline TPfkeyMsgBase();
	inline TPfkeyMsgBase(const TDesC8& aOther);
	};

/**
 *  Concrete class used for sending messages
 *  The class is completely generic and can be used for all messages.
 *  Has convenience functions to build up the message when demanded.
 */
class RSADB;
class TPfkeySendMsgBase : public TPfkeyMsgBase
	{
public:
	/**
	 *  This initializes the packet stream and sets the packet header 
	 *  This can leave
	 */  
	IMPORT_C TPfkeySendMsgBase(TUint8 aType, TUint8 aSatype, TInt aRequestCount, TInt aPid);
	
	IMPORT_C TPfkeySendMsgBase();
	/**
	 */
	IMPORT_C void Reset(TUint8 aType, TUint8 aSaType, TInt aRequestCount, TInt aPid);
	/**
	 *  This adds the extensions in the list to the message,
	 *  @param aExtHdr
	 *		extension header to be added to the message
	 *  @param aExtData
	 *		extension data to be added to the message
	 */
	IMPORT_C TInt AddExt(const TPfkeyAnyExt& aExtHdr, const TDesC8& aExtData = KNullDesC8);

	IMPORT_C void Finalize();
	};

/** 
 *  Concrete class used for receiving messages
 *  Has convenience functions to decode the message when demanded
 */
class TPfkeyRecvMsg : public TPfkeyMsgBase
	{
public:
	/** 
	 *  Read the next extension from the header and put it in the 
	 *  descriptor passed to it.
	 *	@return
	 *		KErrNone If this is a valid extension
	 *		KErrNotFound If there are no more extensions
	 *		KErrGeneral If this extension was invalid
	 *  Try our best not to Leave.
	 */  
	IMPORT_C TInt NextExtension(TPfkeyAnyExt& aExt);

	IMPORT_C void Reset();
	IMPORT_C TInt BytesUnparsed();
	
	IMPORT_C TPfkeyRecvMsg();
	IMPORT_C TPfkeyRecvMsg(const TDesC8& aOther);

private:
	TInt iReadOffset;
	TInt iRemaining;
	};

/**
 *  A handle to the stack side Ipsec SADB. All communication to the SADB
 *  takes place through this socket. As this happens using a well defined
 *  interface, PfkeyV2, the client is supposed to be aware of the semantics
 *  of using pfkey.
 *  
 *  This class has to be a singleton in a thread
 *	The implementation of this class is not thread safe.
 */
class RSADB 
	{
public:

	IMPORT_C TInt Open(RSocketServ& aServer);
	
	IMPORT_C void Close();
	
	IMPORT_C void SendRequest(const TDesC8& aMsg, TRequestStatus& aStatus);
	
	IMPORT_C void CancelSend();

	IMPORT_C void ReadRequest(TDes8& aMsg, TRequestStatus& aStatus);
	
	IMPORT_C void CancelRecv();

	IMPORT_C void FinalizeAndSend(TPfkeySendMsgBase& aMessage, TRequestStatus& aStatus);

	IMPORT_C void SetOpt(TUint aLevel,TUint aName,const TDesC8 &aOption);

	IMPORT_C ~RSADB();
	
	IMPORT_C RSADB();
	
private:
	RSADB(const RSADB&);
	RSocket iPfkeySocket;

private:
	TBool iRegistered;
	};

#include "lib_pfkey.inl"

#endif //__LIB_PFKEY_H__
