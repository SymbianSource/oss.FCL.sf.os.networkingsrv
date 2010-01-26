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

#if !defined(__VJ_H__)
#define __VJ_H__

#include <networking/in_tcp.h>
#include <networking/vjif.h>

const TUint8 KVJDiscard			= 1;

// Limits on the number of slots that can be handled by the VJ code
// in terms of the enumerated slot number.
// The VJ code may work with KMinVjSlot=0, but 2 is safer without more testing.
const TUint8 KMinVjSlot			= 2;
const TUint8 KDesiredVjSlot		= 15;
const TUint8 KMaxVjSlot			= 255;

const TUint8 KVjCompSlotId		= 1;
const TUint8 KVjCompMaskConn	= 0x40;
const TUint8 KVjCompMaskIp		= 0x20;
const TUint8 KVjCompMaskPush	= 0x10;
const TUint8 KVjCompMaskSeq		= 0x08;
const TUint8 KVjCompMaskAck		= 0x04;
const TUint8 KVjCompMaskWindow	= 0x02;
const TUint8 KVjCompMaskUrgent	= 0x01;
const TUint8 KVjCompMaskSpecialD = KVjCompMaskSeq 
								| KVjCompMaskAck 
								| KVjCompMaskWindow 
								| KVjCompMaskUrgent;
const TUint8 KVjCompMaskSpecials = KVjCompMaskSpecialD;
const TUint8 KVjCompMaskSpecialI = KVjCompMaskSeq 
								| KVjCompMaskWindow 
								| KVjCompMaskUrgent;


const TUint32 KUidVJCompDll		=  0x10000ba8;
const TUint32 KUidUnicodeVJCompDll		=  0x10003d43;

// moved from ncpip.h
const TUint KPppIdIp = 0x0021;
const TUint KPppIdVjCompTcp = 0x002d;
const TUint KPppIdVjUncompTcp = 0x002f;
//

class TVJCompHdr
/**
Van Jacobson compressed TCP/IP header container class.
Implements a linked list.
@internalComponent
*/
	{
public:
	TVJCompHdr();
	void			StoreTCPIPHeader(ThdrIP * aHeader);
	void			RetrieveTCPIPHeader(ThdrIP* aIPHeader, ThdrTCP* aTCPHeader);
	inline void		SetNextPtr(TVJCompHdr * aNextPtr);
	inline TVJCompHdr*		NextPtr() const;
	inline void		SetConnectionNumber(TUint aConnection);
	inline TUint	ConnectionNumber() const;
	inline TBool	IsValid() const;
private:
	inline void		MarkValid();

private:
    /** Pointer to the next object in the linked list */
	TVJCompHdr*		iNextPtr;
	
	/** Flag for iConnectionId indicating this object contains valid data */
	enum {KVJValidFlag = 0x80000000};
	
	/** VJ connection number */
	TUint			iConnectionId;
	
	/** IP header */
	ThdrIP			iIPHeader;
	
	/** TCP header */
	ThdrTCP			iTCPHeader;
	};

#include <networking/vj.inl>

NONSHARABLE_CLASS(CVJDeCompressor) : public CVJDeCompressorIf
/**
Van Jacobson TCP/IP header decompressor (RFC 1144)
@internalComponent
*/
	{
public:
	~CVJDeCompressor();
	//IMPORT_C static CVJDeCompressor* NewL(TUint aSlots);
	TBool	DecompVJUncomp(RMBufChain& aPacket);
	TBool	DecompVJComp(RMBufChain& aPacket);
	void	CRCError();
	void	ConstructL( CVJCompFactory* aFactory, TUint aSlots );
	CVJDeCompressor();
private:
	inline	void	SetFlag( const TUint aFlag);
	inline	void	ClearFlag( const TUint aFlag);
	inline	TBool	TestFlag( const TUint aFlag);
	void	DecompSWAU(const TUint aChanges, TUint8** aVJCompHeader, ThdrTCP* aTCPHeader, TUint16 aPreviousFrameLength);
	void	DecompPushFlag(const TUint aChanges, ThdrTCP* aHeader);
	void	DecompIPId(const TUint aChanges, TUint8** aVJCompHeader, ThdrIP* aIPHeader );
	TUint16	DecodeDelta( TUint8 ** aVJCompHeader );
	void	EncodeDelta(TUint8** aVJCompHeader, TUint16 aValue);
	void	DecompUrgent(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader, TUint aChanges);
	void	DecompWindow(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader);
	void	DecompAck(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader);
	void	DecompSeq(TUint8** aVJCompHeader, ThdrTCP* aTCPHeader);
	TUint8*	GetVJPtr(RMBufChain &aChain, TUint16* aCurrentFrameLength);
	void	DecompressFrameL(RMBufChain& aPacket, TUint8 aConnection, TUint8 aChanges, TUint8* const aInitialHeaderPtr, TUint Offset, TUint16 aCurrentFrameLength);
	void	CopyInNewHeaderL(RMBufChain* aPacket, ThdrIP * aIPHeader, ThdrTCP * aTCPHeader, TUint aCompressedHeaderLength, TUint16 aIPHeaderLength, TUint16 aTCPHeaderLength);
	ThdrIP*	GetIPHeader(RMBufChain &aChain);
	void	DoIPChecksum(ThdrIP* aIPHeader, TUint16 aIPHeaderLength);
	void	CopyRecvHeader(const TUint aConnectionNumber, ThdrIP * aHeader);
	void	GetStoredRxHeader(const TUint aConnectionNumber, ThdrIP * aIPHeader, ThdrTCP * aTCPHeader);
	TBool	CheckStoredRxHeader(const TUint aConnection) const;
	
private:
    /** Index of the highest-numbered VJ connection */
	TUint		iNumVJSlots;
	
	/** VJ connection number of the last valid packet received */
	TUint		iLastRxConn;
	
	/** Set to KVJDiscard while discarding received packets due to lost synchronization */
	TUint		iFlags;

    /** Pointer to start of array of VJ connections */
	TVJCompHdr* iRxStates;
	};

#include <networking/vjdecomp.inl>

NONSHARABLE_CLASS(CVJCompressor) : public CVJCompressorIf
/**
Van Jacobson TCP/IP header compressor (RFC 1144)
@internalComponent
*/
	{
public:

	~CVJCompressor();
	//IMPORT_C static CVJCompressor* NewL( TUint aSlots, TBool aCompressConnId );
	TInt	VJCompressFrame(RMBufChain& aPacket);
	void	ConstructL(CVJCompFactory* aFactory, TUint aMaxSlot, TBool aCompressConnId);
	CVJCompressor();

private:
	void	EncodeDelta(TUint8** aVJCompHeader, TInt16 aValue);
	inline TBool	IsDeltaCompressible(TUint32 aDelta) const;
	ThdrIP*	GetIPHeader(RMBufChain &aChain);
	TBool	SuitableForVJCompression(ThdrIP* aIPHeader, ThdrTCP* aTCPHeader, TUint* aConnection, ThdrIP* aRetrievedIPHdr, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressFrame(	RMBufChain& aPacket, 
							ThdrIP* aIPHeader, 
							ThdrTCP* aTCPHeader, 
							TUint aConnection,
							ThdrIP* aRetrievedIPHdr,
							ThdrTCP* aRetrievedTCPHdr);
	TBool	SendAsRawIP(ThdrIP* aIPHeader, ThdrTCP* aTCPHeader);
	TBool	IsIPCompressible( ThdrIP* aIPHeader, ThdrIP* aRetrievedIPHdr);
	TBool	IsTCPCompressible( ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressUrgentPtr(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressWindow(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressAck(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressSeq(TUint8** aVJPtr, TUint8* aChanges, ThdrTCP* aTCPHeader, ThdrTCP* aRetrievedTCPHdr);
	TBool	CompressIPId(TUint8** aVJPtr, TUint8* aChanges, ThdrIP* aIPHeader, ThdrIP* aRetrievedIPHdr);
	TBool	CompressPush(TUint8* aChanges,  ThdrTCP* aTCPHeader);
	TBool	IsSameConnAsLast(TUint* aCompressedHdrLen, TUint aConnection);
	void	SetFirstFewBytes(TUint8* aChanges, TBool aNewConnection, TUint8** aVJHeader, ThdrTCP* aTCPHeader, TUint aConnection);
	void	CopyInNewTxHeader(RMBufChain& aPacket, TUint8* aCompressedHdr, TUint aOldHeaderLength, TUint aNewHeaderLength);
	void	DecrementPacketLen( RMBufChain& aPacket, TUint SizeDecrease );
	TBool	CompressSpecialCases(	TUint8** aVJPtr, 
									TUint8* const aVJInitialDeltaPtr, 
									TUint8* aChanges, 
									ThdrTCP* aTCPHeader, 
									ThdrTCP* aRetrievedTCPHdr, 
									ThdrIP* aIPHeader,
									ThdrIP* aRetrievedIPHdr);
	void	ConvertFrameToUncompVJ(ThdrIP* aIPHeader, TUint aConnection);
	inline TUint8* GetTCPOpts(ThdrTCP* aTCPHeader) const;
	void	CopyTxHeader( ThdrIP* aIPHeader, TUint aConnection);
	TBool	GetStoredTxHeader(	TUint* aConnection, 
								ThdrIP * aIPHeader, 
								ThdrTCP* aTCPHeader, 
								ThdrIP* aRetrievedIPHdr, 
								ThdrTCP* aRetrievedTCPHdr);
	TBool	IPAddressesMatch(ThdrIP* aIPHeader,ThdrIP* aNotherIPHdr);	
	TBool	TCPPortsMatch(ThdrTCP* aHeader, ThdrTCP* aNotherHdr );
	
private:
    /** Maximum number of VJ connections */
	TUint		iMaxVJSlots;
	
	/** Whether VJ was configured to compress the connection number */
	TBool		iCompressConnId;
	
	/** Connection number of the last packet compressed */
	TUint		iLastTxConn;
	
    /** Pointer to the last element in the LRU circular list */
	TVJCompHdr*	iLastTxHdr;

    /** Pointer to start of array of VJ connections */
	TVJCompHdr*	iTxStates;
	};

#include <networking/vjcomp.inl>

NONSHARABLE_CLASS(CVJFactory) : public CVJCompFactory
/**
Factory base for creating a concrete instance of a
compressor or decompressor object
@internalTechnology
*/
	{
public:
	CVJFactory();
	void InstallL();
	CVJCompressorIf* NewVJCompressorL( TInt aMaxVJSlots, TInt aCompressConnId);
	CVJDeCompressorIf* NewVJDeCompressorL(TInt aMaxVJSlots);
	};

extern "C"
	{
/**
Generates and returns a CVJFactory object.
This is the polymorphic DLL entry point.

@return New CVJFactory object
@internalTechnology
*/
	IMPORT_C CVJCompFactory* NewVJCompFactoryL(void);
	}

#endif // __VJ_H__
