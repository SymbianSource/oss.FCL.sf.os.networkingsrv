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
// pfkeyvext.h - IPSEC KEY protocol family
// This definition file contains private extensions for PFKEY V2 API
// defined is rfc2367.
// also: draft-mcdonald-pf-key-v2-06.txt (PF_KEY Key Management API,
// Version 2)
// Note: This file follows the C syntax described in the above
// reference instead of C++. This should make it easier to keep this
// aligned with the evolving IETF specification.
// Nokia specific additions to the PF_KEY message format..
//



/**
 @file pfkeyext.h
 @internalTechnology
 @released
*/
#ifndef __PFKEYEXT_H__
#define __PFKEYEXT_H__
//
// Private generic PFKEY extension type value 
//
#define SADB_PRIV_GENERIC_EXT		128
#define SADB_X_EXT_ENDPOINT_SRC		129	// Extension layout is identity (sadb_ident, type = 0, id = 0)
#define SADB_X_EXT_ENDPOINT_DST		130	// Extension layout is identity (sadb_ident, type = 0, id = 0)
#define SADB_X_EXT_TS				131	// Traffic Selector Extension

//
// Private generic Extension. Header definition corresponds PFKEYv2 extension header definition
// struct sadb_ext. Generic extension data begins right after the header. 
//
struct sadb_gen_ext
	{
	uint16_t sadb_len;
	uint16_t sadb_ext_type;		// SADB_PRIV_GENERIC_EXT
	};

//
// These macro definitions takes care of 16 data packing and
// unpacking.
// The following assumptions are in use for macros:
// -- The connection memory is a little-endian configured (= Intel format)
// -- A macro does always an unconditional conversion for the parameter data
//	P16(d, s) = Source data is supposed to be in "Network order". Data
//				is stored to *p as little-endian.
//	G16(s)	= Source data is supposed to be in memory as little-endian.
//				Macro return data in "Network order"
//
#define P16(d, s)	\
	(*(unsigned char*)((unsigned char*)(d)+1))	= (unsigned char)((s) & 0xff);\
	(*(unsigned char*)(d))						= (unsigned char)(((s) >> 8 ) & 0xff)

#define G16(s)	\
	 (((unsigned short)(*((unsigned char*)(s)+1)))	 | \
		((unsigned short)(*(unsigned char*)(s)) << 8 ))

/**-------------------------------------------------------------------------------------------
 *
 *	TPfkeyGenExtension implements a class to handle PFKEY generic extension data buffer.
 *	generic extension data format is LID format begining with four bytes extension header.
 *	Extension header consists two bytes extension length and two bytes	extension ID.
 *	LID format consists from one byte length, one byte ID and parameter data.
 *	Buffer format: HL,HID,LID,LID,...LID
 *	
 *------------------------------------------------------------------------------------------*/
class TPfkeyGenExtension
	{
public:

	TPfkeyGenExtension() { iExtDesc = NULL; iExtBfr = NULL; } 

	TPfkeyGenExtension(TDes8& aMsg)	{ iExtDesc = &aMsg; iExtBfr = (TUint8*)aMsg.Ptr();	}

	TPfkeyGenExtension(TDes8& aMsg, TUint16 aHdrId) 
		{
		iExtDesc = &aMsg;	
		iExtBfr = (TUint8*)aMsg.Ptr();
		P16((iExtBfr+2), aHdrId);
		P16(iExtBfr, 4);
		iExtDesc->SetLength(4);
		}
	
	
private:
	TUint32 GetExtLength()			 { return (TUint32)G16(iExtBfr); }
	TUint8* GetParameterStart()		{ return (iExtBfr + 4); }
	TBool	CompareParamId(TUint8 aId, TUint32 aIndex )	{ return (*(iExtBfr + aIndex + 1) == aId); }
	TUint8* GetParamPointer(TUint32 aIndex )	{ return (iExtBfr + aIndex + 2); }
	TUint32 GetParamLength(TUint32 aIndex, TUint32* aFoundLth)
		{
		TUint32 lth = (TUint32)*(iExtBfr + aIndex);
		if ( aFoundLth )
			*aFoundLth = lth;
		return lth;
		}
	
	void UpdateExtLength(TUint16 aLth)
		{
		TUint32 NewLth = GetExtLength() + aLth + 2;
		P16(iExtBfr, NewLth);
		}	

public: 
	
	void StoreParameter(TUint8 aId,	TUint8 aLth, TUint8* aData)
		{
		if ( !iExtBfr || !aData )
			return;
		TUint32 ExtLth = GetExtLength();
		if ( (ExtLth + (TUint32)aLth) > (TUint32)iExtDesc->MaxLength() )
			return;
		
		*(iExtBfr + ExtLth + 1) = aId;
		*(iExtBfr + ExtLth)	 = aLth;	 
		Mem::Copy((iExtBfr + ExtLth + 2), aData, aLth);
		UpdateExtLength((TUint16)aLth);
		iExtDesc->SetLength(GetExtLength());	 
		}
	
	TUint8* FindParameter(TUint8 aId, TUint32* aFoundLth)
		{
		if ( !iExtBfr )
			return NULL;
		TUint32 ParamLth = GetExtLength();
		TUint32 i = 4;
		while ( i < ParamLth )
			{
			if ( CompareParamId(aId, i) )
				{
				GetParamLength(i, aFoundLth);
				return GetParamPointer(i);
				}
			i += (GetParamLength(i, NULL) + 2);
			}
		return NULL;
		}

	TBool GetParameterData(TUint8 aId, TDes8& aParamDest )
		{
		TUint32 ParmLth;
		TUint8* ParmData = FindParameter(aId, &ParmLth);
		if ( !ParmData )
			return EFalse;
		aParamDest.Append(ParmData, ParmLth);
		return ETrue;
		}
	

	TBool CheckExtensionType(TUint16 aHeaderId)
		{
		if ( !iExtBfr )
			return EFalse;
		if ( G16(iExtBfr + 2) == aHeaderId )
			 return ETrue;
		else return EFalse;
		}
	

private:
	TDes8*	iExtDesc;		// Extension data descriptor
	TUint8* iExtBfr;		// Extension data buffer
	TUint32 iExtBfrSize;	// Extension data buffer max size
	};

//
// Paremeter definitions for ESP UDP capsulation PFKEY extension 
//
#define ESP_UDP_ENCAPSULATION_EXT	 (TUint16)1	// Header ID	

//
// Paremeter ID values for ESP UDP capsulation PFKEY extension 
//
#define UDP_ENCAPSULATION_PORT		(TUint8)1	// 16 bits value
#define NAT_KEEPALIVE_TIMEOUT		(TUint8)2	// 16 bits value
#define DESTINATION_ADDRESS			(TUint8)3	// Length = sizeof(TInetAddr)
#define PEER_ORIGINAL_ADDRESS		(TUint8)4	// Length = sizeof(TInetAddr)


/*
** Traffic Selector Extension
*/
struct sadb_x_ts
	{
	uint16_t sadb_x_ts_len;
	uint16_t sadb_x_ts_exttype;		/* SA_EXT_TS */
	uint32_t sadb_x_ts_numsel;		/* Number of sadb_selecter that follow */ 
	};
	/* sizeof(struct sadb_x_ts) == 8 */

	/* Followed by
		sadb_ts_numsel * (struct sadb_selector)
		Each two selectors (low, high) defines one traffic selector
		range. Implementation defines whether IPv6 and IPv4 "sockaddr"
		sizes are different. If different, then "sadb_selector_addrtype"
		of the low sadb_selector defines the size of all "sockaddr" in
		the range (e.g. low src, low dst, high src and high dst addresses).
	*/

/*
 * Basic Selector values
 */
struct sadb_x_selector
	{
	uint8_t sadb_x_selector_proto;	/* Protocol Number */
	uint8_t sadb_x_selector_addrtype; /* SADB_ADDRTYPE_IPV4 or SADB_ADDRTYPE_IPV6 */
	uint16_t sabd_x_selector_reserved;/* Padding */
	};
	/* sizeof(struct sadb_x_selector) == 4 */

	/* Followed by two some form of struct sockaddr, 1st = src, 2nd dst address
	 * The socket address includes the port field.
	 */

#endif
