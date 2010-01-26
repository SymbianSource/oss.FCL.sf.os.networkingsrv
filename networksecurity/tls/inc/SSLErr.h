// Copyright (c) 2001-2009 Nokia Corporation and/or its subsidiary(-ies).
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



#ifndef __SSLERR_H__
#define __SSLERR_H__

/** Base offset for SSL errors. */
#define SSL_ERROR_BASE -7400
/** Base offset for SSL alerts. */
#define SSL_ALERT_BASE 100

/**
 * @file SSLErr.h
 * Error code definitions for TLS.
 */

/**
 * No shared cipher was found.
 * The handshake failed because there was no available shared cipher supported by both the client and the server.
 */
const TInt KErrSSLNoSharedCipher		= SSL_ERROR_BASE - 1;

/**
 * The socket already has received data pending on it.
 * This error is seen when StartClientHandshake is called when
 * there is already data waiting to be read from the socket. */
const TInt KErrSSLSocketBusy			= SSL_ERROR_BASE - 2;

/** One or more cipher suites passed as an argument was invalid. */
const TInt KErrSSLInvalidCipherSuite	= SSL_ERROR_BASE - 3;

/**
 * The certificate that was passed as an argument was invalid. 
 * (this could be that actual certificate is not a valid certificate, or if 
 * certman handles are passed, it wasn't found in certman) */
const TInt KErrSSLInvalidCert			= SSL_ERROR_BASE - 4;

/** No client certificate was supplied. */
const TInt KErrSSLNoClientCert			= SSL_ERROR_BASE - 5;

/** The size of the key is too big. */
const TInt KErrSSLUnsupportedKeySize	= SSL_ERROR_BASE - 6;

/** General unsupported key error */
const TInt KErrSSLUnsupportedKey		= SSL_ERROR_BASE - 7;

/** An invalid record was received. */
const TInt KErrSSLBadRecordHeader		= SSL_ERROR_BASE - 8; 

//
// Handshake related errors

/** Invalid protocol version. */
const TInt KErrSSLBadProtocolVersion	= SSL_ERROR_BASE - 9;

/** The server only supports the SSL2.0 protocol. */
const TInt KErrSSL2ServerOnly			= SSL_ERROR_BASE - 10;

/** Unexpected message. */
const TInt KErrSSLUnexpectedMessage		= SSL_ERROR_BASE - 11;

/** Unsupported cipher. */
const TInt KErrSSLUnsupportedCipher		= SSL_ERROR_BASE - 12;

/** Bad MAC. */
const TInt KErrSSLBadMAC				= SSL_ERROR_BASE - 13;

/** An SSL alert was received from the remote end, which caused the connection to be terminated. */
const TInt KErrSSLReceivedAlert			= SSL_ERROR_BASE - 14;


/** An invalid handshake message was received. */
const TInt KErrSSLRecvNotSupportedHS	= SSL_ERROR_BASE - 15;

/** A field in the handshake record being parsed was too big. */
const TInt KErrSSLHSRecordFieldTooBig	= SSL_ERROR_BASE - 16;

/** Record header field too big. */
const TInt KErrSSLRecordHeaderTooBig	= SSL_ERROR_BASE - 17;

/** Send data field too big. */
const TInt KErrSSLSendDataTooBig			= SSL_ERROR_BASE - 18;

/** No certificate. */
const TInt KErrSSLNoCertificate			= SSL_ERROR_BASE - 19;

/** Invalid hash. */
const TInt KErrSSLInvalidHash			= SSL_ERROR_BASE - 20;

/** Send cancelled. */
const TInt KErrSSLSendCanceled			= SSL_ERROR_BASE - 21;

/** Receieve cancelled. */
const TInt KErrSSLRecvCanceled			= SSL_ERROR_BASE - 22;

/** CancelHandshake was called during one of the handshake methods. */
const TInt KErrSSLHandshakeCanceled		= SSL_ERROR_BASE - 23;

/** Write failed. */
const TInt KErrSSLWriteFailed			= SSL_ERROR_BASE - 24;

/** The SSL.dll couldn't be loaded by the adaptor. */
const TInt KErrSSLFailedToLoad			= SSL_ERROR_BASE - 25;

/** An unspecified error was signaled from the SSL.dll to the adaptor. */
const TInt KErrSSLDisconnectIndication	= SSL_ERROR_BASE - 26;

/** A leave occured in the SSL.dll. */
const TInt KErrSSLDllLeave				= SSL_ERROR_BASE - 27;

/** A leave occured in the SSL.dll. */
const TInt KErrSSLNullTlsSession				= SSL_ERROR_BASE - 28;

//
// These error codes are equivalent to the standard TLS protocol Alert message 
// errors as defined in the TLS RFC. They include all those defined in SSL3.0
// The end number of each error is the same as per the RFC, so by using the
// SSL_ERROR_BASE and SSL_ALERT_BASE defines, errors codes can be constructed
// directly from the field within an alert message.
/** Close notification. */
const TInt KErrSSLAlertCloseNotify				= SSL_ERROR_BASE - SSL_ALERT_BASE - 0;
/** An inappropriate message was received. */
const TInt KErrSSLAlertUnexpectedMessage		= SSL_ERROR_BASE - SSL_ALERT_BASE - 10;
/** A record was received with an incorrect MAC. */
const TInt KErrSSLAlertBadRecordMac				= SSL_ERROR_BASE - SSL_ALERT_BASE - 20;
/** A TLS cipher text was decrypted in an invalid way. */
const TInt KErrSSLAlertDecryptionFailed			= SSL_ERROR_BASE - SSL_ALERT_BASE - 21;
/** A TLS cipher text record was received which was too long. */
const TInt KErrSSLAlertRecordOverflow			= SSL_ERROR_BASE - SSL_ALERT_BASE - 22;
/** The decompression function received improper input. */
const TInt KErrSSLAlertDecompressionFailure		= SSL_ERROR_BASE - SSL_ALERT_BASE - 30;
/** With given the options available, the sender was unable to negotiate an acceptable 
* set of security parameters . */
const TInt KErrSSLAlertHandshakeFailure			= SSL_ERROR_BASE - SSL_ALERT_BASE - 40;
/** No certificate. */
const TInt KErrSSLAlertNoCertificate			= SSL_ERROR_BASE - SSL_ALERT_BASE - 41;
/** A certificate was corrupt, e.g. contained signatures that could not be verified. */
const TInt KErrSSLAlertBadCertificate			= SSL_ERROR_BASE - SSL_ALERT_BASE - 42;
/** The certificate was of an unsupported type. */
const TInt KErrSSLAlertUnsupportedCertificate	= SSL_ERROR_BASE - SSL_ALERT_BASE - 43;
/** The certificate was revoked. */
const TInt KErrSSLAlertCertificateRevoked		= SSL_ERROR_BASE - SSL_ALERT_BASE - 44;
/** The certificate was expired. */
const TInt KErrSSLAlertCertificateExpired		= SSL_ERROR_BASE - SSL_ALERT_BASE - 45;
/** An unspecified problem with the certificate. Certificate can not be used. */
const TInt KErrSSLAlertCertificateUnknown		= SSL_ERROR_BASE - SSL_ALERT_BASE - 46;
/** A field in the handshake was out of range or inconsistent with other fields. */
const TInt KErrSSLAlertIllegalParameter			= SSL_ERROR_BASE - SSL_ALERT_BASE - 47;

/** The certificate was not accepted.
* 
* Either the CA certificate could not be located 
* or the CA could not be matched with a known trusted CA. */
const TInt KErrSSLAlertUnknownCA				= SSL_ERROR_BASE - SSL_ALERT_BASE - 48;
/** A valid certificate was received, but the access control denied access. */
const TInt KErrSSLAlertAccessDenied				= SSL_ERROR_BASE - SSL_ALERT_BASE - 49;
/** A message could not be decoded, e.g. some field was out of the specified range. */
const TInt KErrSSLAlertDecodeError				= SSL_ERROR_BASE - SSL_ALERT_BASE - 50;
/** A handshake cryptographic operation failed. */
const TInt KErrSSLAlertDecryptError				= SSL_ERROR_BASE - SSL_ALERT_BASE - 51;
/** A negotiation was not allowed due to export restrictions. */
const TInt KErrSSLAlertExportRestriction		= SSL_ERROR_BASE - SSL_ALERT_BASE - 60;
/** The protocol version was not supported, e.g. the client has attempted to negotiate 
* a recognised, but unsupported protocol. */
const TInt KErrSSLAlertProtocolVersion			= SSL_ERROR_BASE - SSL_ALERT_BASE - 70;
/** Returned instead of KErrSSLAlertHandshakeFailure when the negotiation fails 
* because the server requests ciphers more secure than those supported by the client. */
const TInt KErrSSLAlertInsufficientSecurity		= SSL_ERROR_BASE - SSL_ALERT_BASE - 71;
/** An internal error. */
const TInt KErrSSLAlertInternalError			= SSL_ERROR_BASE - SSL_ALERT_BASE - 80;
/** This handshake is being cancelled by the user. */
const TInt KErrSSLAlertUserCanceled				= SSL_ERROR_BASE - SSL_ALERT_BASE - 90;
/** No renegotiation will be accepted. */
const TInt KErrSSLAlertNoRenegotiation			= SSL_ERROR_BASE - SSL_ALERT_BASE - 100;


#endif // __SSLERR_H__
