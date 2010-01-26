// Copyright (c) 2005-2009 Nokia Corporation and/or its subsidiary(-ies).
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


#ifndef __TCPSSLCOMPATIBILITY_H__
#define __TCPSSLCOMPATIBILITY_H__

// TcertInfo was originately part of certman.h

/** 
* Class to hold the DN (Distinguished Name) information. 
* @publishedAll 
* @deprecated  Deprecated. Do not Use.
* @since v6.0 
*/
class TDNInfo
{
public:
	/** DN country string. */
	TBuf8<4> iCountry;
	/** DN organisation unit. */
	TBuf8<64> iOrganizationUnit;
	/** DN orginasation. */
	TBuf8<64> iOrganization;
	/** DN common name. */
	TBuf8<64> iCommonName;
	/** DN locality. */
	TBuf8<128> iLocality;
};

/**
* Class to hold the certificate information. 
* @since v6.0 
* @publishedAll
* @deprecated  Deprecated. Do not Use.
*/
class TCertInfo
{
public:
	/** Certificate's fingerprint. */
	TBuf8<20> iFingerprint;
	/** Certificate's serial number. */
	TBuf8<32> iSerialNo;
	/** Certificate's public key. */
	TBuf8<300> iPublicKey;
	/** Certificate's version. */
	TInt iVersionNo;
	/** Certificate's Public Key Algorithm, using the TPublicKeyAlg enum. */
	/*TPublicKeyAlg*/TInt iPkAlg;
	/** Certificate's start validity date. */
	TTime iStartValDate;
	/** Certificate's end validity date. */
	TTime iEndValDate;
	/** Subject DN Information. */
	TDNInfo iSubjectDNInfo;
	/** Issuer DN Information. */
	TDNInfo iIssuerDNInfo;
	/** DNS name. */
	TBuf<128> iDNSName;
	TInt iDigAlg;              
private:
	TBuf8<246> iSpare;

};

#endif // __TCPSSLCOMPATIBILITY_H__
