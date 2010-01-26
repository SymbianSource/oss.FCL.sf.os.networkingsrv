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
// spcrypto.h - security parser crypto manager
//



/**
 @internalComponent
*/
#ifndef __SPCRYPTO_H
#define __SPCRYPTO_H

#include <e32base.h>
#include <es_prot.h>

const TUint KProtocolCrypto =	0x104; // A dummy assignment for now (should have
// central registry)
const TUint KAfCrypto = 0x0803;			// Dummy

//
//	TAlgorithmDesc (and related types)
//
//		A description of available algorithm
//
typedef TBuf8<0x20> TAlgorithmName;
typedef enum
    {
    EAlgorithmClass_Digest,	// Message Digest algorithm
        EAlgorithmClass_Cipher,	// Symmetric Cipher algorithm
        //
        // New types are possible by adding the symbol here
        // and defining the corresponding abstract class
        // (similar to COwnMessageDigest and CSymmetricCipher)
        //
    } TAlgorithmClass;

class TAlgorithmDesc
    {
    public:
        TAlgorithmName iName;	// Descriptive name
        TAlgorithmClass iAlgType;
        TUint iMinBits;			// Min Length of the key in bits (all keys total)
        TUint iMaxBits;			// Max Length of the key in bits (all keys total)
        TUint iBlock;			// Length of the block in bytes
        TUint iVector;			// Initialization Vector length (bytes)
    };


//
// Each of the following includes virtual destructor
// just in case there is a need for a cleanup code
// when the object is deleted using a pointer to
// the base virtual class

//
//	COwnMessageDigest
//		Base Message Digest (abstract) class
//
class COwnMessageDigest : public CBase
    {
    public:
        virtual void Init()=0;
        virtual void Update(const TDesC8& aMessage)=0;
        virtual void Final(TDes8& aDigest)=0;
        virtual ~COwnMessageDigest() {}
    };


//
//	CSymmetricCipher
//		Base Symmetric Cipher (abstract) class
//
class CSymmetricCipher : public CBase
    {
    public:
        enum TAction { EEncrypt, EDecrypt };
        virtual void Setkey(const TDesC8& aKey)=0;
        virtual void InitL(const TDesC8 &anIV, TAction aMode)=0;
        //
        // ALL OutBuf's given to Update must exist up to Finish
        // call (or at least as long as at least blocksize octets
        // have been given to Update after it).
        //
        virtual void Update(TDes8& anOutBuf,const TDesC8& anInBuf)=0;
        //
        // Calling Finish is optional, it is needed if the total
        // bytes is not multiple of the blocksize, or if one wants
        // to get the final IV.
        virtual void Finish(TDes8& anIV)=0;
        virtual ~CSymmetricCipher() {}
    };

//
//	CProtocolCrypto
//		The algorithm manager (abstract) class
//
class CProtocolCrypto : public CProtocolBase
    {
    public:
        virtual TUint AlgorithmList(TAlgorithmDesc *&aList) = 0;
        virtual CSymmetricCipher* SymmetricCipher(TUint anAlg)=0;
        virtual COwnMessageDigest* MessageDigest(TUint anAlg)=0;
    protected:
        virtual ~CProtocolCrypto() {}
    };

#endif
