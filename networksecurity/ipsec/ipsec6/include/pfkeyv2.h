// Copyright (c) 2007-2009 Nokia Corporation and/or its subsidiary(-ies).
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
// pfkeyv2.h - IPSEC KEY protocol family
// Definitions for the API and its' messages to access the SADB
// (Security Association Data Base, previously Key Engine).
// See: RFC-2367 (PF_KEY Key Management API, Version 2
// Note: This file follows the C syntax described in the above
// reference instead of C++. This should make it easier to keep this
// aligned with the evolving IETF specification.
// RFC-2367 PF_KEY v2 specification
//

/*
@publishedPartner
@released
*/

#ifndef __PFKEYV2_H__
#define __PFKEYV2_H__

//
// Temporary placeholder definitions for the posix uint types
// (these don't belong into this file!!!)
#include <e32std.h>
typedef TUint8 uint8_t;
typedef TUint16 uint16_t;
typedef TUint32 uint32_t;
typedef TInt64 uint64_t;    // Incorrect, but gets allocation right!


/*
This file defines structures and symbols for the PF_KEY Version 2
key management interface. It was written at the U.S. Naval Research
Laboratory. This file is in the public domain. The authors ask that
you leave this credit intact on any copies of this file.
*/
#define PF_KEY_V2   2
#define PFKEYV2_REVISION    199806L
/*
** Message Types
*/
#define SADB_RESERVED   0
#define SADB_GETSPI     1
#define SADB_UPDATE     2
#define SADB_ADD        3
#define SADB_DELETE     4
#define SADB_GET        5
#define SADB_ACQUIRE    6
#define SADB_REGISTER   7
#define SADB_EXPIRE     8
#define SADB_FLUSH      9
#define SADB_DUMP       10  /* Only for debugging purposes */

#define SADB_MAX        10


/*
** Security association (SA) flags
*/
#define SADB_SAFLAGS_PFS      1   /* Perfect forward secrecy */
#define SADB_SAFLAGS_TUNNEL   2   /* SA is used Tunnel mode (NRC IPSEC Addition for IKE) */
#define SADB_SAFLAGS_NAT_T    4   /* Nokia VPN NAT Traversal (Private extension)         */ 
#define SADB_SAFLAGS_INT_ADDR 8   /* Nokia VPN Internal address (Private extension)      */ 
#define SABD_SAFLAGS_ESN     16   /* Exteneded Sequence Numbers Enabled                  */
/*
** Security association states
*/
#define SADB_SASTATE_LARVAL 0   /* Unfinished SA initialized by GETSPI */
#define SADB_SASTATE_MATURE 1   /* SA ready for use */
#define SADB_SASTATE_DYING  2   /* Soft lifetime expired */
#define SADB_SASTATE_DEAD   3   /* Hard lifetime expired */

#define SADB_SASTATE_MAX    3


/*
** Security association types
*/
#define SADB_SATYPE_UNSPEC  0   /* */
#define SADB_SATYPE_AH      2   /* RFC-1826 */
#define SADB_SATYPE_ESP     3   /* RFC-1827 */

/*
** Security association types for security protocols implemented in user space
*/
#define SADB_SATYPE_RSVP    5   /* RSVP Authentication */
#define SADB_SATYPE_OSPFV2  6   /* OSPFv2 Authentication */
#define SADB_SATYPE_RIPV2   7   /* RIPv2 Authentication */
#define SADB_SATYPE_MIP     8   /* Mobile IP Auth. */

#define SADB_SATYPE_MAX     8


/*
** Algorithm Types (in the context of the SA type)
*/
#define SADB_AALG_NONE      0   /* No authentication */
#define SADB_AALG_MD5HMAC   2
#define SADB_AALG_SHA1HMAC  3
#ifdef  SYMBIAN_IPSEC_VOIP_SUPPORT
#define SADB_AALG_AES_XCBC_MAC 9
#define SADB_AALG_MAX       5
#else //SYMBIAN_IPSEC_VOIP_SUPPORT
#define SADB_AALG_MAX       3
#endif //SYMBIAN_IPSEC_VOIP_SUPPORT

#define SADB_EALG_NONE      0   /* No encryption            */
#define SADB_EALG_DESCBC    2   /* DES in CBC-mode for encryption   */
#define SADB_EALG_3DESCBC   3   /* DES-EDE3-CBC             */
#define SADB_EALG_NULL      11  
#define SADB_EALG_AESCBC   	12  

#ifdef SYMBIAN_CRYPTOSPI
#define SADB_EALG_AESCTR  	13  
#endif	//SYMBIAN_CRYPTOSPI 	

#define SADB_EALG_MAX       13 

/*
** Extension header values
*/
#define SADB_EXT_RESERVED           0
#define SADB_EXT_SA                 1
#define SADB_EXT_LIFETIME_CURRENT   2
#define SADB_EXT_LIFETIME_HARD      3
#define SADB_EXT_LIFETIME_SOFT      4
#define SADB_EXT_ADDRESS_SRC        5
#define SADB_EXT_ADDRESS_DST        6
#define SADB_EXT_ADDRESS_PROXY      7
#define SADB_EXT_KEY_AUTH           8
#define SADB_EXT_KEY_ENCRYPT        9
#define SADB_EXT_IDENTITY_SRC       10
#define SADB_EXT_IDENTITY_DST       11
#define SADB_EXT_SENSITIVITY        12
#define SADB_EXT_PROPOSAL           13
#define SADB_EXT_SUPPORTED_AUTH     14
#define SADB_EXT_SUPPORTED_ENCRYPT  15
#define SADB_EXT_SPIRANGE           16

#define SADB_EXT_MAX                16


/*
** Identity extension values
 */
#define SADB_IDENTTYPE_RESERVED     0
#define SADB_IDENTTYPE_PREFIX       1
#define SADB_IDENTTYPE_FQDN         2
#define SADB_IDENTTYPE_USERFQDN     3

#define SADB_IDENTTYPE_MAX          3

/*
** Base Message Header Format
*/
struct sadb_msg
    {
    uint8_t sadb_msg_version;   /* PF_KEY_V2 */
    uint8_t sadb_msg_type;      /* Message type: see SADB_xxx defines */
    uint8_t sadb_msg_errno;     /* Error return value */
    uint8_t sadb_msg_satype;    /* Association type: see SADB_SATYPE_xxx */
    uint16_t sadb_msg_len;      /* Total msg length in 64-bit words     */
                                /* ..an earlier revision used 32-bit words! */
    uint16_t sadb_msg_reserved; /* Padding = 0 */
    uint32_t sadb_msg_seq;      /* Sequence number assigned by original sender */
    uint32_t sadb_msg_pid;      /* Id of the user-process */
    };
    /* sizeof(struct sadb_msg) == 16 */

/*
 * Base header is followed by additional message fields (extensions), all of which start with
 * a length-type pair. This is a generic struct used to decode the actual length and type of 
 * an extension, i.e. all extensions begin with these exactly same fields.
 */

/*
** Additional Message Fields
*/
struct sadb_ext
    {
    uint16_t sadb_ext_len;      /* In 64-bit words, inclusive   */
    uint16_t sadb_ext_type;     /* see SA_EXT_xxx */
    };
    /* sizeof(struct sadb_ext) == 4 */

/*
** Association Extension
*/
struct sadb_sa
     {
    uint16_t sadb_sa_len;
    uint16_t sadb_sa_exttype;   /* == SA_EXT_ASSOCIATION */
    uint32_t sadb_sa_spi;       /* Security parameter index */
    uint8_t sadb_sa_replay;     /* Size of anti-replay window */
    uint8_t sadb_sa_state;      /* see SADB_SASTATE_xxx */
    uint8_t sadb_sa_auth;       /* Authentication algorithm */
    uint8_t sadb_sa_encrypt;    /* Encryption algorithm */
    uint32_t sadb_sa_flags;     /*  */
};
/* sizeof(struct sadb_sa) == 16 */

/*
** Lifetime Extension
*/
struct sadb_lifetime
    {
    uint16_t sadb_lifetime_len;
    uint16_t sadb_lifetime_exttype; /* == SA_EXT_LIFETIME_CURRENT, _HARD, _SOFT */
    uint32_t sadb_lifetime_allocations;
    uint64_t sadb_lifetime_bytes;
    uint64_t sadb_lifetime_addtime;
    uint64_t sadb_lifetime_usetime;
};
/* sizeof(struct sadb_lifetime) == 32 */

/*
** Address Extension
*/
struct sadb_address
    {
    uint16_t sadb_address_len;
    uint16_t sadb_address_exttype;  /* == SA_EXT_ADDRESS_SRC, _DST, _PROXY */
    uint8_t sadb_address_proto;
    uint8_t sadb_address_prefixlen;
    uint16_t sadb_address_reserved;
    };
    /* sizeof(struct sadb_address) == 16 */

    /* Followed by some form of struct sockaddr */

/*
** Key Extension
*/
struct sadb_key
    {
    uint16_t sadb_key_len;
    uint16_t sadb_key_exttype;      /* SA_EXT_KEY_AUTH, _ENCRYPT */
    uint16_t sadb_key_bits;         /* The lenght of valid key data, in bits */
    uint16_t sadb_key_reserved;
    };
    /* sizeof(struct sadb_key) == 8 */

    /* Followed by the key data */

/*
** Identity Extension
*/
struct sadb_ident
    {
    uint16_t sadb_ident_len;
    uint16_t sadb_ident_exttype;    /* SA_EXT_IDENTITY_SRC, _DST */
    uint16_t sadb_ident_type;       /* Type of the following identify information */
    uint16_t sadb_ident_reserved;   /* Padding */
    uint64_t sadb_ident_id;
    };
    /* sizeof(struct sadb_ident) == 16 */

    /* Followed by the identify string (C), if present */


/*
** Sensitivity extension
*/
struct sadb_sens
    {
    uint16_t sadb_sens_len;
    uint16_t sadb_sens_exttype; /* SA_EXT_SENSITIVITY */
    uint32_t sadb_sens_dpd;     /* Data protection domain: NONE, DOD_GENSER,
                       DOD_SCI, DOE, NATO or private */
    uint8_t sadb_sens_sens_level;
    uint8_t sadb_sens_sens_len; /* Sensitivity bitmap length in 64-bit words */
    uint8_t sadb_sens_integ_level;
    uint8_t sadb_sens_integ_len;/* Integrty bitmap length in 64-bit words */
    uint32_t sadb_sens_reserved;
    };
    /* sizeof(struct sadb_sens) == 16 */

    /* Followed by
        uint64_t sadb_sens_sens_bitmap[sens_len];
        uint64_t sadb_sens_integ_bitmap[integ_len];
    */
/*
** Proposal Extension
*/
struct sadb_prop
    {
    uint16_t sadb_prop_len;
    uint16_t sadb_prop_exttype; /* SA_EXT_PROPOSAL */
    uint8_t sadb_prop_replay;   /* Anti-replay window size */
    uint8_t sadb_prop_reserved[3]; /* Padding */
    };
    /* sizeof(struct sadb_prop) == 8 */

    /* Followed by
        struct sadb_comb sadb_combs
            [(sadb_prop_len * sizeof(uint64_t) - sizeof(struct sadb_prop))
            / sizeof(sadb_comb)];
    */

/*
** Proposal combination
*/
struct sadb_comb
    {
    uint8_t sadb_comb_auth;     /* Authentication algorithm         */
    uint8_t sadb_comb_encrypt;  /* Encryption algorithm             */
    uint16_t sadb_comb_flags;   /* Bitmask: USED, UNIQUE, INBOUND, OUTBOUND, 
                       FORWARD, PFS, REPLAY             */
    uint16_t sadb_comb_auth_minbits;
    uint16_t sadb_comb_auth_maxbits;
    uint16_t sadb_comb_encrypt_minbits;
    uint16_t sadb_comb_encrypt_maxbits;
    uint32_t sadb_comb_reserved;
    uint32_t sadb_comb_soft_allocations;
    uint32_t sadb_comb_hard_allocations;
    uint64_t sadb_comb_soft_bytes;
    uint64_t sadb_comb_hard_bytes;
    uint64_t sadb_comb_soft_addtime;
    uint64_t sadb_comb_hard_addtime;
    uint64_t sadb_comb_soft_usetime;
    uint64_t sadb_comb_hard_usetime;
    };
    /* sizeof(struct sadb_comb) = 72 */

/*
** Supported Algorithms Extension
*/
struct sadb_supported
    {
    uint16_t sadb_supported_len;
    uint16_t sadb_supported_exttype;/* SA_EXT_SUPPORTED         */
    uint32_t sadb_supported_reserved;       /* Padding */
    };
    /* sizeof(struct sadb_supported) == 8 */

    /* Followed by
        struct sadb_alg sadb_algs[(sadb_supported_len * sizeof(uint64_t) -
            sizeof(struct sadb_supported)) / sizeof(struct sadb_alg)];
    */

/*
 * Supported algorithm descriptor
 */
struct sadb_alg
    {
    uint8_t sadb_alg_id;            /* Algorithm type: MD5_HMAC, DES_CBC... */
    uint8_t sadb_alg_ivlen;         /* Initialization vector length in bits */
    uint16_t sadb_alg_minbits;      /* Min key len in bits */
    uint16_t sadb_alg_maxbits;      /* Max key len in bits */
    uint16_t sadb_alg_reserved;     /* Padding */
    };
    /* sizeof(struct sadb_alg) == 8 */

/*
** SPI Range Extension
*/
struct sadb_spirange
    {
    uint16_t sadb_spirange_len;
    uint16_t sadb_spirange_exttype; /* SA_EXT_SPI_RANGE     */
    uint32_t sadb_spirange_min;     /* Minimum acceptable SPI value */
    uint32_t sadb_spirange_max;     /* Maximum acceptable SPI value */
    uint32_t sadb_spirange_reserved;/* Padding */
    };
    /* sizeof(struct sadb_spirange) == 16 */
    
#endif
