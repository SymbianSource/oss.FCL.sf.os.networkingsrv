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
// SSL3.0 and TLS1.0 Handshake message items source file.
// This file contains definitions for SSL3.0 and TLS1.0 handshake items 
// (i.e., handshake protocol types, headers, message structures, etc).
// 
//

/**
 @file
*/

#include "tlshandshakeitem.h"
#include <signed.h>
#include <hash.h>
#include <tlstypedef.h>

void CServerKeyExchMsg::ComputeDigest( CMessageDigest* pDigest, const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const
{
	pDigest->Update(aClientRandom);
	pDigest->Update(aServerRandom);
	aDigest.Copy( pDigest->Final(aDigestParams) );
}

void CServerKeyExchMsg::ComputeDSADigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const
{
	CMessageDigest* pDigest = CSHA1::NewL();
   ComputeDigest( pDigest, aClientRandom, aServerRandom, aDigestParams, aDigest );
   delete pDigest;
}

void CServerKeyExchMsg::ComputeRSADigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TPtrC8& aDigestParams, TDes8& aDigest ) const
{
	CMessageDigest* pDigest = CMD5::NewL();
   ComputeDigest( pDigest, aClientRandom, aServerRandom, aDigestParams, aDigest );
   delete pDigest;
   //append DSA digest (SHA1 hash)
   TInt nMax = aDigest.MaxLength() - aDigest.Length();
   TPtr8 digest( const_cast<TUint8*>(aDigest.Ptr()) + aDigest.Length(), 0, nMax );
   ComputeDSADigestL( aClientRandom, aServerRandom, aDigestParams, digest );
   aDigest.SetLength( digest.Length() + aDigest.Length() );
}

//RSA key exchange
void CRsaAnonServerKeyExchMsg::CopyParamsL( CTlsCryptoAttributes *aAttrs )
{
   iRsaParams.iRsaModulus.CopyBodyToL( aAttrs->iPublicKeyParams->iValue1);
   iRsaParams.iRsaExponent.CopyBodyToL( aAttrs->iPublicKeyParams->iValue2);
}

TPtr8 CRsaDsaServerKeyExchMsg::Signature()
{
   return iDsaSignature.iSha.GetBodyDes();
}

void CRsaDsaServerKeyExchMsg::ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest )
{
   TPtrC8 ptrParams( GetDigestParamsPtr(), GetDigestParamsLength() );
   ComputeDSADigestL( aClientRandom, aServerRandom, ptrParams, aDigest );
}

TPtr8 CRsaRsaServerKeyExchMsg::Signature()
{
   return iRsaSignature.iMd5Sha.GetBodyDes();
}

void CRsaRsaServerKeyExchMsg::ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest )
{
   TPtrC8 ptrParams( GetDigestParamsPtr(), GetDigestParamsLength() );
   ComputeRSADigestL( aClientRandom, aServerRandom, ptrParams, aDigest );
}

//DH key exchange
void CDhAnonServerKeyExchMsg::CopyParamsL( CTlsCryptoAttributes *aAttrs )
{
   iDhParams.iDh_p.CopyBodyToL( aAttrs->iPublicKeyParams->iValue1);
   iDhParams.iDh_g.CopyBodyToL( aAttrs->iPublicKeyParams->iValue2);
   iDhParams.iDh_Ys.CopyBodyToL( aAttrs->iPublicKeyParams->iValue3);
}

TPtr8 CDhDsaServerKeyExchMsg::Signature()
{
   return iDsaSignature.iSha.GetBodyDes();
}

void CDhDsaServerKeyExchMsg::ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest )
{
   TPtrC8 ptrParams( GetDigestParamsPtr(), GetDigestParamsLength() );
   ComputeDSADigestL( aClientRandom, aServerRandom, ptrParams, aDigest );
}

TPtr8 CDhRsaServerKeyExchMsg::Signature()
{
   return iRsaSignature.iMd5Sha.GetBodyDes();
}

void CDhRsaServerKeyExchMsg::ComputeDigestL( const TDesC8& aClientRandom, const TDesC8& aServerRandom, TDes8& aDigest )
{
   TPtrC8 ptrParams( GetDigestParamsPtr(), GetDigestParamsLength() );
   ComputeRSADigestL( aClientRandom, aServerRandom, ptrParams, aDigest );
}

//PSK key exchange
void CPskServerKeyExchMsg::CopyParamsL( CTlsCryptoAttributes *aAttrs )
{
   iPskServerParams.iPskIdentityHint.CopyBodyToL( aAttrs->iPskIdentityHint );
}

TPtr8 CPskServerKeyExchMsg::Signature()
{
	// Not used
	return TPtr8(0,0);
}

void CPskServerKeyExchMsg::ComputeDigestL( const TDesC8& /*aClientRandom*/, const TDesC8& /*aServerRandom*/, TDes8& /*aDigest */)
{
	// Not used
}

void CRsaCertificateVerifyMsg::SetSignature( TDesC8& aSign )
{
   iRsaSignature.iMd5Sha.SetBody( aSign );
}

void CRsaCertificateVerifyMsg::SetSignatureLength( TDesC8& aSign )
{
   iRsaSignature.iMd5Sha.Header().SetInitialValue( aSign.Length() );
}

void CDsaCertificateVerifyMsg::SetSignature( TDesC8& aSign )
{
   iDsaSignature.iSha.SetBody( aSign );
}

void CDsaCertificateVerifyMsg::SetSignatureLength( TDesC8& aSign )
{
   iDsaSignature.iSha.Header().SetInitialValue( aSign.Length() ) ;
}


CGenericExtension* CGenericExtension::NewLC(TInt aInitialLength)
{
	CGenericExtension* self = new(ELeave) CGenericExtension(aInitialLength);
	CleanupStack::PushL(self);
	return self;
}

CGenericExtension::CGenericExtension( TInt aInitialLength ) :
   CExtensionNode(&iOpaqueData),
   iOpaqueData(KTlsExtensionLength, NULL, aInitialLength)
{
}

CGenericExtension::~CGenericExtension()
{
}

TInt CGenericExtension::ExtensionLength()
{
	return CExtensionNode::ExtensionLength() + iOpaqueData.GetItemLength();
}

CClientServerNameEntry* CClientServerNameEntry::NewLC(TInt aInitialLength)
{
	CClientServerNameEntry* self = new(ELeave) CClientServerNameEntry(aInitialLength);
	CleanupStack::PushL(self);
	return self;
}

CClientServerNameEntry::CClientServerNameEntry( TInt aInitialLength ) :
   CConstItem(&iName, KTlsExtensionNameTypeLength),
   iName(KTlsExtensionLength, NULL, aInitialLength)
{
}

CClientServerNameExtension* CClientServerNameExtension::NewLC()
{
	CClientServerNameExtension* self = new(ELeave) CClientServerNameExtension();
	self->ConstructOpaqueDataWrapperL(&self->iServerNames);
	CleanupStack::PushL(self);
	return self;
}

void CClientServerNameExtension::AddServerNameEntryL(CClientServerNameEntry *aServerNameEntry)
	/**
	Add server name entry to the list of server names and take ownership of it.
	*/
{
	iServerNames.AddNodeL(aServerNameEntry);
}

CClientServerNameEntry* CClientServerNameExtension::Node(TInt aIndex)
{
	return static_cast<CClientServerNameEntry*>(iServerNames.Node(aIndex));
}

CClientServerNameExtension::CClientServerNameExtension() :
   CKnownExtensionNode(),
   iServerNames(NULL, KTlsExtensionLength)
{
}

