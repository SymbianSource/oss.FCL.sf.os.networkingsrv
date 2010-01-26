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
// sa_spec.h - security parser algorithm manager
//



/**
 @internalComponent
*/
#ifndef __SA_SPEC_H
#define __SA_SPEC_H

#include <networking/pfkeyv2.h>

//
// This file collects minimal definitions that need to be exported
// from the Security Associations Database into Security Policy
// database
//
//
// Mapping of low level types in pfkeyv2.h into more semantic names
// (This is to avoid a need to look many places in case pfkeyv2 changes)
//
typedef uint32_t TLifetimeAllocations;
#if EPOC_SDK >= 0x08000000  //  Symbian's changes for JetStream
typedef TInt64 TLifetimeBytes;
typedef TInt64 TLifetimeSeconds;
#else
typedef uint64_t TLifetimeBytes;
typedef uint64_t TLifetimeSeconds;
#endif


//
// The default life time in seconds for larval SA's created by
// GETSPI (may also be used as a default for iLarvalLifetime in
// TSecurityAssocSpec).
//
const TInt KLifetime_LARVAL_DEFAULT = 30;	// seonds!

class TIdentity : public TPtr8
    {
    public:
        TIdentity(): TPtr8(0,0) {}
    };

// TLifetime, a help structure

class TLifetime
    {
    public:
        TLifetime(const struct sadb_lifetime &aLifetime);
        static void Freeze(TTime &aTime, const TTime &aNow);
        TLifetime();
        // For current, these will count items used so far. For Hard and
        // Soft these will contain the limit values for the current
        // counts.
        // study: present unspecified limit with 0 or max value?
        TLifetimeAllocations iAllocations;	// Connections limit
        TLifetimeBytes iBytes;				// Transmitted bytes limit
        //
        // For Current, these will record the creation and first use times.
        // For Hard and Soft, these will record the expiration times (e.g.
        // simple comparison with the current time can be used to test for
        // expiration, and for returning CURRENT values to application, use
        // the SecondsFrom method with current.
        //
        TTime iAddtime;						// Lifetime limit from creation
        TTime iUsetime;						// Lifetime limit from first use
    };


//
// A template to be used while creating new security
// associations (subclassed in Security Policy Database)
//

    //
    //	TSecurityProposalSpec
#ifdef SYMBIAN_IPSEC_VOIP_SUPPORT   
    class CSecurityProposalSpec  : public CBase
    {
    public:
    	TUint8 iType;                 // SA type (AH or ESP)
    	TUint8 iAalg;                 // Authentication algorithm id
    	TUint16 iAalgLen;             // Authentication algorithm key length
    	TUint8 iEalg;                 // Encryption algorithm id	
    	TUint16 iEalgLen;             // Encryption algorithm Key length	
       //
        // iLarvalLifetime specifies the maximum time to wait in
        // larval/egg state, when an ACQUIRE request originating
        // from this template is sent to the key management.
        //
        TUint iLarvalLifetime;		// Seconds (0 => use KLifetime_LARVAL_DEFAULT)
        //
        // These are only used in specifying the life time requirements
        // for the acquire message and are thus preformatted to be used
        // directly as a component of the TPfkeyMessage.
        
        struct sadb_lifetime iHard;	// Hard Lifetime requirement
        struct sadb_lifetime iSoft;	// Soft Lifetime requirement 	
    };
#endif    
//
//	TSecurityAssocSpec
//
//	Specify what is required from the SA that can be used
//	for the outbound packet. If no matching SA is found, an
//	acquire message is generated, and these values specify
//	the requested values for the SA.
//
//	src, proxy
//		if no tunnel, proxy=INADDR_ANY, src=IP src
//		if tunnel, proxy=IP src, src=current host
//

class TSecurityAssocSpec
    {
    public:
        //
        // SA selection fields
        //
#ifndef SYMBIAN_IPSEC_VOIP_SUPPORT    
          TUint8 iType;				// SA type (AH or ESP)
          TUint8 iAalg;				// Authentication algorithm id
         TUint16 iAalgLen;			// Authentication algorithm key length
         TUint8 iEalg;				// Encryption algorithm id
         TUint16 iEalgLen;			// Encryption algorithm Key length
#endif    
        TUint8 iReplayWindowLength;	// Replay Window length (equal or greater)
        TUint8 iPfs:1;				// SA must have same value of PFS
        TUint8 iMatchSrc:1;			// SA must have a matching src
        TUint8 iMatchProxy:1;		// SA must have a matching proxy
        TUint8 iMatchProtocol:1;	// SA must have a matching protocol
        TUint8 iMatchLocalPort:1;		// SA must have a matching src port
        TUint8 iMatchRemotePort:1;		// SA must have a matching dst port
        TUint8 iMatchLocal:1;		//resulting SA is limited to the specific local adress defined by the packet
        TUint8 iMatchRemote:1;		//resulting SA is limited to the specific remote adress defined by the packet
        
        //
        // Identity reference
        // (This is currently only used for ACQUIRE Message)
        //
        
        //!!!!!!!!!!!!!!!!!!!!!!!!!! Fields deleted!!!!
        
        //struct sadb_ident iIdentity;	// Preformatted for the PFKEY
        //TIdentity iIdentityData;		// NUL terminated Identity (NUL included
        // in the length!)
        //!!!!!!!!!!!!!!!!!!!!!!!!!! 
#ifndef SYMBIAN_IPSEC_VOIP_SUPPORT        
          //
          // iLarvalLifetime specifies the maximum time to wait in
          // larval/egg state, when an ACQUIRE request originating
          // from this template is sent to the key management.
          //
          TUint iLarvalLifetime;		// Seconds (0 => use KLifetime_LARVAL_DEFAULT)
          //
          // These are only used in specifying the life time requirements
          // for the acquire message and are thus preformatted to be used
          // directly as a component of the TPfkeyMessage.
          
          struct sadb_lifetime iHard;	// Hard Lifetime requirement
          struct sadb_lifetime iSoft;	// Soft Lifetime requirement
#endif
    };

// endpoint name specification
class TEpSpec
    {
    public:
        TInetAddr iEpAddr;
        TBool iIsOptional;
    };


#endif
