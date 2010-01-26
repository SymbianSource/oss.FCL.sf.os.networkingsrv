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
* SSL3.0 and TLS1.0 Record items header file.
* This file contains definitions for SSL3.0 and TLS1.0 Record items.
* 
*
*/



/**
 @file TlsRecordItem.h
*/

#include <comms-infras/recorditem.h>

#ifndef _TLSRECORDITEM_H_
#define _TLSRECORDITEM_H_

// Enumeration values for the Record protocol's Content types
enum ETlsRecordType 
{ 
	ETlsChangeCipherContentType = 20,		/** Change Cipher Spec message */
	ETlsAlertContentType = 21,				/** Alert message */
	ETlsHandshakeContentType = 22,			/** Handshake message */
	ETlsAppDataContentType = 23				/** Application data */
};

// Constants for a Record header object
const TInt KTlsRecordHeaderSize = 5;		// No. of bytes in Record Protocol header
const TInt KTlsRecordTypeOffset = 0;		// Offset of Record Content type within header: 1 byte
const TInt KTlsRecordVersionOffset = 1;		// Offset of Protocol version within header: 2 bytes
const TInt KTlsRecordLengthOffset = 3;		// Offset of Record length within header: 3 bytes
const TInt KTlsRecordBodyLength = 2;		// Size of Record body length, 2 bytes
 
// Record protocol fragment sizes, these are RFC2246 specified values
// Compression adds 1024 bytes to Plain text, but NULL compression is currently supported
const TInt KTlsRecordCompressedText = 0x0000;
const TInt KTlsRecordPlainText = 0x4000;							
const TInt KTlsRecordCipherText = 0x0400; // Ciphering adds 1024 bytes to Compressed text
const TInt KTlsRecordMaxBodySize = KTlsRecordPlainText + KTlsRecordCompressedText + KTlsRecordCipherText;
const TInt KTlsRecordPlainTextToSend = 0x800; //max recordbody to be send
const TInt KTlsRecordPrealocate = KTlsRecordPlainTextToSend + KTlsRecordHeaderSize + KTlsRecordCompressedText + KTlsRecordCipherText;

// Constants for message items
const TInt KTlsRandomBodyLength = 32;		// Length of client/server random value

class CRecordHeader : public CItem< TConstant >
{
public:
   CRecordHeader( CItemBase* aNext ) :
      CItem< TConstant >( aNext, KTlsRecordHeaderSize ),
      iRecord( NULL )
   {
      iRecord.iFirst = this;
   }

public:
   TRecord iRecord;
};

class CRandomItem : public CConstItem
/**
 * @class This class represents a Client and Server Hello random value.
 *
 * @brief The Client random value is obtained via the TLS Provider API. This 32-byte 
 * value has 4 bytes which represent the system time and 28 bytes obtained from a PRNG
 * (Pseudo-random number generator). The Server Random is obtained from a Server Hello 
 * message.
 */
{
public:
	CRandomItem( CItemBase* aNext ) : 
	  CConstItem( aNext, KTlsRandomBodyLength ) {} 
};

#endif
