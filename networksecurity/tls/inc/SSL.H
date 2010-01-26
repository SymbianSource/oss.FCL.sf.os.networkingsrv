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
// SSL exported functions. 
// 
//

/**
 @file
*/

#ifndef __TCPSSL_H__
#define __TCPSSL_H__

#include <in_sock.h>
#include <sslerr.h>

#include <ssl_compatibility.h>

#ifndef SYMBIAN_ENABLE_SPLIT_HEADERS
#include <ssl_internal.h>
#endif

/**
 * @publishedAll
 * @released
 *
 * Enable the use of TLS_RSA_WITH_NULL_MD5 and TLS_RSA_WITH_NULL_SHA ciphersuites
 * These ciphersuites use NULL encryption and therefore offer no protection against evesdropping.
 * Server authentication (and client, if a client certificate is used) is performed and data integrity
 * is still checked.
 *
 * (Ciphersuite TLS_NULL_WITH_NULL_NULL is never supported).
 *
 * An argument of 0 disables the ciphersuites and non-zero enables them.
*/
const TUint KSoEnableNullCiphers = 0x408;				//< Enable/disable NULL ciphers

/**
 * @publishedAll
 * @released
 *
 * Set the PSK Key Exchange configuration. Argument is a TPckgBuf<MSoPskKeyHandler *>.
 * The structure and buffers will be copied.
 *
 * @see MSoPskKeyHandler
*/
const TUint KSoPskConfig = 0x409;				//< Set PSK key exchange configuration

/**
 * @publishedAll
 * @released
 *
 * Set the list of server names to be passed to the server in the ClientHello as described in RFC3546 "Server Name Indication".
 * The argument should be a TPckgBuf<CDesC8Array *>.
*/
const TUint KSoServerNameIndication = 0x40a;				//< Set Server Name Indication

#endif
