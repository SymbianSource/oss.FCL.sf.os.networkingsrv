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
//

/**
 @file
 @internalComponent 
*/

#if !defined(__ProtocolHeaders_H__)
#define __ProtocolHeaders_H__

#define DotToIp(a,b,c,d) (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

const TUint KUDPHeaderSize = 8;
const TUint KIPMinHeaderSize = 20;
const TUint KIPDefaultTOS			= 0;
const TInt KIPDefaultTTL			= 255;
const TUint KIPVersion				= 4;

struct TGenericHeader
{
};

/**
IP message header
*/
struct TIpHdr
{
public:
	//inline TUint32 DestAddr() const {return(BigEndian::Get32(iDestAddr.Begin()));};
	//inline TUint32 SrcAddr() const {return(BigEndian::Get32(iSrcAddr.Begin()));};
	//inline TUint16 FragOffset() const {return(BigEndian::Get16(iFragOffset.Begin()));};
	/** Bits 7..4 of iVersion */
	inline	TUint	Version() const {return (iVersion>>4); }; 
	inline	void	SetVersion(TUint aVal) { iVersion = (TUint8)((iVersion&0xf)|((aVal&0xf)<<4)); };
	/** Bits 3..0 of iVersion */
	inline	TUint	HdrLen() const { return (iVersion&0xf)<<2; }; 
	inline	void	SetHdrLen(TUint aVal) { iVersion = 
	                (TUint8)((iVersion&0xf0)|((TUint8)(aVal>>2)&0xf)); };
	inline	TUint	TOS() { return iTOS; };
	inline	void	SetTOS(TUint aVal) { iTOS = (TUint8)(aVal&0xff); };
	inline	TUint16	Length() { return BigEndian::Get16((TUint8*)&iLength); };
	inline	void	SetLength(TUint16 aVal) { BigEndian::Put16((TUint8*)&iLength,aVal); };
	inline	TUint16	Ident() { return BigEndian::Get16((TUint8*)&iIdent); };
	inline	void	SetIdent(TUint16 aVal) { BigEndian::Put16((TUint8*)&iIdent,aVal); };
	inline	TUint8	TTL() { return iTTL; };
	inline	void	SetTTL(TUint aVal) { iTTL = (TUint8)(aVal&0xff); };
	inline	TUint8	Protocol() { return iProtocol; };
	inline	void	SetProtocol(TUint aVal) { iProtocol = (TUint8)(aVal&0xff); };
	inline	TUint16	Checksum() { return BigEndian::Get16((TUint8*)&iChecksum); };
	inline	void	SetChecksum(TUint16 aVal) { BigEndian::Put16((TUint8*)&iChecksum,aVal); };
	inline TUint32 DestAddr() const {return(BigEndian::Get32(iDestAddr));};
	inline TUint32 SrcAddr() const {return(BigEndian::Get32(iSrcAddr));};
	inline TUint16 FragOffset() const {return BigEndian::Get16(iFragOffset);};
	inline void SetDestAddr(TUint32 aVal) const { BigEndian::Put32((TUint8*)(&iDestAddr),aVal);};
	inline void SetSrcAddr(TUint32 aVal) const { BigEndian::Put32((TUint8*)(&iSrcAddr),aVal);};
	inline void SetFragOffset(TUint16 aVal) const {BigEndian::Put16((TUint8*)(&iFragOffset),aVal);};
protected:
	TUint8 iVersion;
	TUint8 iTOS;
	TUint16 iLength;
	TUint16 iIdent;
	//TFixedArray<TUint8,2> iFragOffset;
	TUint8 iFragOffset[2];
	TUint8 iTTL;
	TUint8 iProtocol;
	TUint16 iChecksum;
	//TFixedArray<TUint8,4> iSrcAddr;
	//TFixedArray<TUint8,4> iDestAddr;
	TUint8 iSrcAddr[4];
	TUint8 iDestAddr[4];
};

#endif
